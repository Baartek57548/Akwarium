/*
 * PROJEKT: Sterownik Akwarium wifi
 * AUTOR: Bartosz Wolny + (AI Assistant)
 * PLATFORMA: ESP32-S3 Zero 240Mhz (Dual Core - FreeRTOS)
 */

#include "AkwariumWifi.h"
#include "ApiHandlers.h"
#include "AquariumAnimation.h"
#include "BleManager.h"
#include "ConfigManager.h"
#include "FirmwareInfo.h"
#include "LogManager.h"
#include "OtaManager.h"
#include "PowerManager.h"
#include "SharedState.h"
#include "SystemController.h"

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>

// Obiekty glowne
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C display(U8G2_R0, /* reset=*/-1);
AquariumAnimation *animation = nullptr;

// Osobne zadanie wyswietlacza

// --- UI STATE MACHINE ---
enum class UiState {
  HOME,
  MENU,
  SCHEDULE_LIGHT,
  SCHEDULE_AERATION,
  SCHEDULE_FILTER,
  SCHEDULE_TEMP,
  SCHEDULE_FEEDING,
  LOGS,
  SETTINGS_DATETIME,
  TESTS,
  FEEDING,
  ACCESS_POINT,
  BLUETOOTH
};
UiState uiState = UiState::HOME;

enum class LogsViewState { SELECT_TYPE, SHOW_NORMAL, SHOW_CRITICAL, SHOW_STATS };
LogsViewState logsViewState = LogsViewState::SELECT_TYPE;
uint8_t logsTypeSelection = 0; // 0=Zwykle, 1=Krytyczne, 2=Statystyki

bool lastUpPressed = false;
bool lastSelectPressed = false;
bool lastDownPressed = false;
unsigned long allButtonsHoldStartMs = 0;
bool manualFeedComboTriggered = false;
unsigned long singleButtonGuardStartMs = 0;
uint8_t singleButtonGuardMask = 0;
bool feedingUiActive = false;
UiState uiStateBeforeFeeding = UiState::HOME;
unsigned long lastUiInteractionMs = 0;
unsigned long logsDeleteHoldStartMs = 0;
bool logsDeleteHoldTriggered = false;
bool logsDeleteHoldActive = false;
uint8_t logsDeleteHoldProgress = 0;
uint16_t todayFeedingsCount = 0;
uint8_t feedingsCounterDay = 1;
uint8_t feedingsCounterMonth = 1;
uint16_t feedingsCounterYear = 2025;

#ifndef RELAY_HEATER_PIN
#define RELAY_HEATER_PIN 4
#endif
#ifndef RELAY_FILTER_PIN
#define RELAY_FILTER_PIN 2
#endif
#ifndef RELAY_LIGHT_PIN
#define RELAY_LIGHT_PIN 5
#endif
#ifndef RELAY_FEEDER_PIN
#define RELAY_FEEDER_PIN 3
#endif

#define BUTTON_UP_PIN 15
#define BUTTON_SELECT_PIN 16
#define BUTTON_DOWN_PIN 14
#define MANUAL_FEED_HOLD_MS 1000UL
#define MANUAL_FEED_SINGLE_GUARD_MS 140UL
#define UI_IDLE_RETURN_HOME_MS 30000UL
#define LOGS_DELETE_HOLD_MS 3000UL

static constexpr uint8_t BUTTON_MASK_UP = (1U << 0);
static constexpr uint8_t BUTTON_MASK_SELECT = (1U << 1);
static constexpr uint8_t BUTTON_MASK_DOWN = (1U << 2);

static void clampRelayPinsAtBoot() {
  // Szybkie "usztywnienie" pinow po restarcie, aby ograniczyc przypadkowe
  // klikniecia przekaznikow podczas normalnego programowania i bootowania.
  // Dla restartu po OTA/BLE odtwarzamy poziomy zapisane tuz przed restartem.
  uint8_t lightLevel = HIGH;
  uint8_t filterLevel = HIGH;
  uint8_t heaterLevel = HIGH;
  uint8_t feederLevel = HIGH;
  const bool hasSavedLevels = OtaManager::takeBootRelayLevels(
      lightLevel, filterLevel, heaterLevel, feederLevel);

  const uint8_t relayPins[] = {RELAY_LIGHT_PIN, RELAY_FILTER_PIN,
                               RELAY_HEATER_PIN, RELAY_FEEDER_PIN};
  const uint8_t relayLevels[] = {lightLevel, filterLevel, heaterLevel,
                                 feederLevel};

  for (size_t i = 0; i < (sizeof(relayPins) / sizeof(relayPins[0])); i++) {
    const uint8_t pin = relayPins[i];
    const uint8_t level = hasSavedLevels ? relayLevels[i] : HIGH;
    digitalWrite(pin, level); // Preload latch (najpierw poziom, potem OUTPUT).
    pinMode(pin, OUTPUT);
    digitalWrite(pin, level);
  }
}

static bool shouldApplyUiIdleHomeTimeout(UiState state) {
  if (state == UiState::HOME)
    return false;
  if (state == UiState::ACCESS_POINT || state == UiState::BLUETOOTH)
    return false;
  if (state == UiState::FEEDING)
    return false;
  return true;
}

static void resetLogsDeleteHoldState() {
  logsDeleteHoldStartMs = 0;
  logsDeleteHoldTriggered = false;
  logsDeleteHoldActive = false;
  logsDeleteHoldProgress = 0;
}

static void syncDailyFeedingsCounterDate() {
  SharedStateData snap = SharedState::getSnapshot();
  if (snap.day != feedingsCounterDay || snap.month != feedingsCounterMonth ||
      snap.year != feedingsCounterYear) {
    todayFeedingsCount = 0;
    feedingsCounterDay = snap.day;
    feedingsCounterMonth = snap.month;
    feedingsCounterYear = snap.year;
  }
}

static void syncBleSessionWithUiState() {
  // BLE ma byc wlaczane tylko z funkcji "Bluetooth".
  // Wyjatek: podczas ekranu karmienia utrzymujemy sesje, jesli wejsciem
  // byl ekran Bluetooth.
  const bool shouldEnableBle =
      (uiState == UiState::BLUETOOTH) ||
      (uiState == UiState::FEEDING && uiStateBeforeFeeding == UiState::BLUETOOTH);

  if (shouldEnableBle) {
    BleManager::start();
  } else {
    BleManager::stop();
  }
}

struct PendingScheduleUpdate {
  uint8_t lightMode;
  uint8_t dayStartHour;
  uint8_t dayStartMinute;
  uint8_t dayEndHour;
  uint8_t dayEndMinute;
  uint8_t aerationMode;
  uint8_t aerationHourOn;
  uint8_t aerationMinuteOn;
  uint8_t aerationHourOff;
  uint8_t aerationMinuteOff;
  uint8_t filterMode;
  uint8_t filterHourOn;
  uint8_t filterMinuteOn;
  uint8_t filterHourOff;
  uint8_t filterMinuteOff;
  uint8_t heaterMode;
  uint8_t feedHour;
  uint8_t feedMinute;
  uint8_t feedMode;
  float targetTemp;
};

struct PendingTimeUpdate {
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t day;
  uint8_t month;
  uint16_t year;
};

static portMUX_TYPE pendingUiMux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool hasPendingScheduleUpdate = false;
static volatile bool hasPendingTimeUpdate = false;
static volatile bool hasPendingSaveConfirmAnimation = false;
static PendingScheduleUpdate pendingScheduleUpdate = {};
static PendingTimeUpdate pendingTimeUpdate = {};
static unsigned long suppressUiTimeSyncUntilMs = 0;

static bool isUiTimeSyncSuppressed(unsigned long nowMs) {
  if (suppressUiTimeSyncUntilMs == 0) {
    return false;
  }
  return static_cast<long>(suppressUiTimeSyncUntilMs - nowMs) > 0;
}

static void suppressUiTimeSyncForManualFeed(unsigned long nowMs) {
  // Krótkie okno ochronne: kombinacja 3 przycisków ma uruchamiać karmienie,
  // bez jakiejkolwiek ingerencji w RTC.
  suppressUiTimeSyncUntilMs = nowMs + 3000UL;

  portENTER_CRITICAL(&pendingUiMux);
  hasPendingTimeUpdate = false;
  pendingTimeUpdate = {};
  portEXIT_CRITICAL(&pendingUiMux);

  if (animation) {
    // Wyczyść ewentualne "pending" z edycji daty/czasu.
    if (uiState == UiState::SETTINGS_DATETIME && animation->isEditingActive()) {
      animation->cancelEditing();
    }
    animation->hasTimeChanged();
  }
}

void requestUiSaveConfirmationAnimation() {
  portENTER_CRITICAL(&pendingUiMux);
  hasPendingSaveConfirmAnimation = true;
  portEXIT_CRITICAL(&pendingUiMux);
}

static void consumePendingUiSaveConfirmationAnimation() {
  bool shouldPlayAnimation = false;

  portENTER_CRITICAL(&pendingUiMux);
  shouldPlayAnimation = hasPendingSaveConfirmAnimation;
  hasPendingSaveConfirmAnimation = false;
  portEXIT_CRITICAL(&pendingUiMux);

  if (shouldPlayAnimation && animation) {
    animation->playConfirmAnimation();
  }
}

static const char *configSaveStatusToString(ConfigSaveStatus status) {
  switch (status) {
  case ConfigSaveStatus::Ok:
    return "ok";
  case ConfigSaveStatus::OkVerifiedAfterWriteMismatch:
    return "ok_verified_after_write_mismatch";
  case ConfigSaveStatus::LockTimeout:
    return "lock_timeout";
  case ConfigSaveStatus::WriteFailed:
    return "write_failed";
  case ConfigSaveStatus::VerifyReadFailed:
    return "verify_read_failed";
  case ConfigSaveStatus::VerifyMismatch:
    return "verify_mismatch";
  default:
    return "unknown";
  }
}

static void logBootStage(const char *stage) {
  char line1[22] = {0};
  char line2[22] = {0};
  if (stage != nullptr) {
    snprintf(line1, sizeof(line1), "%.21s", stage);
    const size_t len = strlen(stage);
    if (len > 21) {
      snprintf(line2, sizeof(line2), "%.21s", stage + 21);
    }
  }

  display.clearBuffer();
  display.setFont(u8g2_font_5x7_tr);
  display.drawStr(0, 8, "BOOT");
  if (line1[0] != '\0') {
    display.drawStr(0, 19, line1);
  }
  if (line2[0] != '\0') {
    display.drawStr(0, 30, line2);
  }
  display.sendBuffer();

  Serial.print("[BOOT] ");
  Serial.println(stage != nullptr ? stage : "stage");
  delay(1);
}

static void queueScheduleUpdateFromAnimation() {
  if (!animation)
    return;

  PendingScheduleUpdate update = {};
  update.lightMode = animation->getLightMode();
  update.dayStartHour = animation->getScheduleHourOn();
  update.dayStartMinute = animation->getScheduleMinOn();
  update.dayEndHour = animation->getScheduleHourOff();
  update.dayEndMinute = animation->getScheduleMinOff();

  update.aerationMode = animation->getAerationMode();
  update.aerationHourOn = animation->getAerationHourOn();
  update.aerationMinuteOn = animation->getAerationMinOn();
  update.aerationHourOff = animation->getAerationHourOff();
  update.aerationMinuteOff = animation->getAerationMinOff();

  update.filterMode = animation->getFilterMode();
  update.filterHourOn = animation->getFilterHourOn();
  update.filterMinuteOn = animation->getFilterMinOn();
  update.filterHourOff = animation->getFilterHourOff();
  update.filterMinuteOff = animation->getFilterMinOff();

  update.heaterMode = animation->getHeaterMode();
  update.targetTemp = animation->getTargetTemp();
  update.feedHour = animation->getFeedHour();
  update.feedMinute = animation->getFeedMinute();
  update.feedMode = animation->getFeedFreq();

  portENTER_CRITICAL(&pendingUiMux);
  pendingScheduleUpdate = update;
  hasPendingScheduleUpdate = true;
  portEXIT_CRITICAL(&pendingUiMux);
}

static void queueTimeUpdateFromAnimation() {
  if (!animation)
    return;

  PendingTimeUpdate update = {};
  update.hour = animation->getNewHour();
  update.minute = animation->getNewMinute();
  update.second = animation->getNewSecond();
  update.day = animation->getNewDay();
  update.month = animation->getNewMonth();
  update.year = animation->getNewYear();

  portENTER_CRITICAL(&pendingUiMux);
  pendingTimeUpdate = update;
  hasPendingTimeUpdate = true;
  portEXIT_CRITICAL(&pendingUiMux);
}

static void captureUiChanges() {
  if (!animation)
    return;

  if (animation->hasScheduleChanged()) {
    queueScheduleUpdateFromAnimation();
  }

  const bool timeChanged = animation->hasTimeChanged();
  unsigned long nowMs = millis();
  if (isUiTimeSyncSuppressed(nowMs)) {
    if (timeChanged) {
      LogManager::logWarn(
          "Pominieto zapis czasu z UI podczas recznego karmienia.");
    }
    return;
  }

  if (!timeChanged) {
    return;
  }

  // Accept RTC time changes only from Date/Time screen. Any pending flag seen
  // in other views is treated as stale and must be dropped.
  if (uiState == UiState::SETTINGS_DATETIME) {
    queueTimeUpdateFromAnimation();
  } else {
    LogManager::logWarn(
        "Odrzucono zalegla zmiane czasu poza ekranem daty/czasu.");
  }
}

static void syncTestOverridesWithUiState() {
  if (!animation) {
    return;
  }

  if (uiState == UiState::TESTS) {
    SystemController::setTestOverrides(
        animation->getTestLight(), animation->getTestFilter(),
        animation->getTestHeater(), animation->getTestFeeder(),
        animation->getTestAeration());
  } else if (SystemController::isTestOverrideActive()) {
    SystemController::clearTestOverrides();
  }
}

static void applyPendingUiChanges() {
  PendingScheduleUpdate localSchedule = {};
  PendingTimeUpdate localTime = {};
  bool applySchedule = false;
  bool applyTime = false;

  portENTER_CRITICAL(&pendingUiMux);
  if (hasPendingScheduleUpdate) {
    localSchedule = pendingScheduleUpdate;
    hasPendingScheduleUpdate = false;
    applySchedule = true;
  }
  if (hasPendingTimeUpdate) {
    localTime = pendingTimeUpdate;
    hasPendingTimeUpdate = false;
    applyTime = true;
  }
  portEXIT_CRITICAL(&pendingUiMux);

  if (applySchedule) {
    Config cfg = ConfigManager::getCopy();
    cfg.lightMode = constrain(localSchedule.lightMode, 0, 2);
    cfg.dayStartHour = constrain(localSchedule.dayStartHour, 0, 23);
    cfg.dayStartMinute = constrain(localSchedule.dayStartMinute, 0, 59);
    cfg.dayEndHour = constrain(localSchedule.dayEndHour, 0, 23);
    cfg.dayEndMinute = constrain(localSchedule.dayEndMinute, 0, 59);

    cfg.aerationMode = constrain(localSchedule.aerationMode, 0, 2);
    cfg.aerationHourOn = constrain(localSchedule.aerationHourOn, 0, 23);
    cfg.aerationMinuteOn = constrain(localSchedule.aerationMinuteOn, 0, 59);
    cfg.aerationHourOff = constrain(localSchedule.aerationHourOff, 0, 23);
    cfg.aerationMinuteOff = constrain(localSchedule.aerationMinuteOff, 0, 59);

    cfg.filterMode = constrain(localSchedule.filterMode, 0, 2);
    cfg.filterHourOn = constrain(localSchedule.filterHourOn, 0, 23);
    cfg.filterMinuteOn = constrain(localSchedule.filterMinuteOn, 0, 59);
    cfg.filterHourOff = constrain(localSchedule.filterHourOff, 0, 23);
    cfg.filterMinuteOff = constrain(localSchedule.filterMinuteOff, 0, 59);

    cfg.heaterMode = constrain(localSchedule.heaterMode, 0, 1);
    cfg.targetTemp = constrain(localSchedule.targetTemp, 18.0f, 30.0f);
    cfg.feedHour = constrain(localSchedule.feedHour, 0, 23);
    cfg.feedMinute = constrain(localSchedule.feedMinute, 0, 59);
    cfg.feedMode = constrain(localSchedule.feedMode, 0, 3);

    const ConfigSaveResult saveResult = ConfigManager::updateAndSaveDetailed(cfg);
    if (saveResult.ok) {
      if (saveResult.sanitizedChanged) {
        LogManager::logWarn(
            "Harmonogram zapisany po korekcie wartosci do dozwolonego formatu.");
      } else if (saveResult.status ==
                 ConfigSaveStatus::OkVerifiedAfterWriteMismatch) {
        LogManager::logWarn(
            "Harmonogram zapisany i zweryfikowany mimo niejednoznacznej odpowiedzi storage.");
      } else {
        LogManager::logInfo("Zapisano harmonogramy.");
      }
    } else {
      char msg[160];
      snprintf(msg, sizeof(msg),
               "Blad zapisu harmonogramow. status=%s written=%u read=%u",
               configSaveStatusToString(saveResult.status),
               static_cast<unsigned>(saveResult.bytesWritten),
               static_cast<unsigned>(saveResult.bytesReadBack));
      LogManager::logError(msg);
    }
  }

  if (applyTime) {
    uint8_t hour = constrain(localTime.hour, 0, 23);
    uint8_t minute = constrain(localTime.minute, 0, 59);
    uint8_t second = constrain(localTime.second, 0, 59);
    uint8_t day = constrain(localTime.day, 1, 31);
    uint8_t month = constrain(localTime.month, 1, 12);
    uint16_t year = constrain(localTime.year, 2024, 2099);
    DateTime newTime(year, month, day, hour, minute, second);
    syncSystemTime(static_cast<uint32_t>(newTime.unixtime()));
    char msg[96];
    snprintf(msg, sizeof(msg),
             "Menu Data/Czas: zapisano %04u-%02u-%02u %02u:%02u:%02u.", year,
             static_cast<unsigned>(month), static_cast<unsigned>(day),
             static_cast<unsigned>(hour), static_cast<unsigned>(minute),
             static_cast<unsigned>(second));
    LogManager::logInfo(msg);
  }
}

static void resetUiStateAfterIdleTimeout() {
  logsViewState = LogsViewState::SELECT_TYPE;
  logsTypeSelection = 0;
  resetLogsDeleteHoldState();
  if (animation) {
    animation->resetNavigationState();
  }
}

void updateUiState() {
  if (!animation)
    return;

  unsigned long nowMs = millis();
  if (lastUiInteractionMs == 0) {
    lastUiInteractionMs = nowMs;
  }
  syncDailyFeedingsCounterDate();

  bool isUpPressed = (digitalRead(BUTTON_UP_PIN) == LOW);
  bool isSelectPressed = (digitalRead(BUTTON_SELECT_PIN) == LOW);
  bool isDownPressed = (digitalRead(BUTTON_DOWN_PIN) == LOW);
  const uint8_t buttonMask =
      (isUpPressed ? BUTTON_MASK_UP : 0U) |
      (isSelectPressed ? BUTTON_MASK_SELECT : 0U) |
      (isDownPressed ? BUTTON_MASK_DOWN : 0U);
  const uint8_t pressedButtonsCount =
      (isUpPressed ? 1U : 0U) + (isSelectPressed ? 1U : 0U) +
      (isDownPressed ? 1U : 0U);
  bool allButtonsPressed = isUpPressed && isSelectPressed && isDownPressed;
  bool upJustPressedRaw = isUpPressed && !lastUpPressed;
  bool selectJustPressedRaw = isSelectPressed && !lastSelectPressed;
  bool downJustPressedRaw = isDownPressed && !lastDownPressed;
  bool upJustPressed = upJustPressedRaw;
  bool selectJustPressed = selectJustPressedRaw;
  bool downJustPressed = downJustPressedRaw;
  const bool oledWasSleeping = (PowerManager::getCurrentMode() != MODE_ACTIVE);

  // Pierwsze klikniecie po wygaszeniu ekranu tylko wybudza OLED.
  // Zabezpiecza to przed przypadkowym wejsciem do menu z HOME.
  if (oledWasSleeping && (upJustPressed || selectJustPressed || downJustPressed)) {
    PowerManager::registerActivity();
    lastUiInteractionMs = nowMs;
    lastUpPressed = isUpPressed;
    lastSelectPressed = isSelectPressed;
    lastDownPressed = isDownPressed;
    return;
  }

  // Delay single-button edges briefly so a 3-button chord can form without
  // firing intermediate UI actions (e.g. toggles in Tests).
  if (singleButtonGuardMask != 0) {
    const bool guardedButtonStillPressed =
        (buttonMask & singleButtonGuardMask) == singleButtonGuardMask;
    if (pressedButtonsCount >= 2) {
      singleButtonGuardMask = 0;
      singleButtonGuardStartMs = 0;
      upJustPressed = false;
      selectJustPressed = false;
      downJustPressed = false;
    } else if (!guardedButtonStillPressed ||
               static_cast<long>(nowMs - singleButtonGuardStartMs) >=
                   MANUAL_FEED_SINGLE_GUARD_MS) {
      upJustPressed = (singleButtonGuardMask & BUTTON_MASK_UP) != 0U;
      selectJustPressed = (singleButtonGuardMask & BUTTON_MASK_SELECT) != 0U;
      downJustPressed = (singleButtonGuardMask & BUTTON_MASK_DOWN) != 0U;
      singleButtonGuardMask = 0;
      singleButtonGuardStartMs = 0;
    } else {
      lastUpPressed = isUpPressed;
      lastSelectPressed = isSelectPressed;
      lastDownPressed = isDownPressed;
      return;
    }
  } else if (pressedButtonsCount == 1 &&
             (upJustPressedRaw || selectJustPressedRaw || downJustPressedRaw)) {
    singleButtonGuardMask = buttonMask;
    singleButtonGuardStartMs = nowMs;
    lastUpPressed = isUpPressed;
    lastSelectPressed = isSelectPressed;
    lastDownPressed = isDownPressed;
    return;
  }

  if (allButtonsPressed) {
    if (allButtonsHoldStartMs == 0) {
      allButtonsHoldStartMs = millis();
      suppressUiTimeSyncForManualFeed(nowMs);
    } else if (!manualFeedComboTriggered &&
               (millis() - allButtonsHoldStartMs >= MANUAL_FEED_HOLD_MS)) {
      SystemController::feedNow();
      PowerManager::registerActivity();
      lastUiInteractionMs = nowMs;
      manualFeedComboTriggered = true;
    }
  } else {
    allButtonsHoldStartMs = 0;
    manualFeedComboTriggered = false;
  }

  if (pressedButtonsCount >= 2 && !allButtonsPressed) {
    resetLogsDeleteHoldState();
    lastUpPressed = isUpPressed;
    lastSelectPressed = isSelectPressed;
    lastDownPressed = isDownPressed;
    return;
  }

  bool feedingNow = SystemController::isFeedingNow();
  if (feedingNow && !feedingUiActive) {
    todayFeedingsCount++;
    uiStateBeforeFeeding = uiState;
    uiState = UiState::FEEDING;
    feedingUiActive = true;
    animation->setFeedingAnimation(true);
  } else if (!feedingNow && feedingUiActive) {
    feedingUiActive = false;
    animation->setFeedingAnimation(false);
    if (uiState == UiState::FEEDING) {
      if ((uiStateBeforeFeeding == UiState::ACCESS_POINT &&
           !AkwariumWifi::getIsAPMode()) ||
          (uiStateBeforeFeeding == UiState::BLUETOOTH &&
           !BleManager::isAdvertising() && !BleManager::isConnected())) {
        uiState = UiState::HOME;
      } else {
        uiState = uiStateBeforeFeeding;
      }
    }
  }

  if (allButtonsPressed || manualFeedComboTriggered) {
    resetLogsDeleteHoldState();
    lastUpPressed = isUpPressed;
    lastSelectPressed = isSelectPressed;
    lastDownPressed = isDownPressed;
    return;
  }

  if (upJustPressed || selectJustPressed || downJustPressed) {
    PowerManager::registerActivity();
    lastUiInteractionMs = nowMs;
  }

  if (shouldApplyUiIdleHomeTimeout(uiState) &&
      (nowMs - lastUiInteractionMs >= UI_IDLE_RETURN_HOME_MS)) {
    resetUiStateAfterIdleTimeout();
    uiState = UiState::HOME;
  }

  switch (uiState) {
  case UiState::HOME:
    if (selectJustPressed)
      uiState = UiState::MENU;
    break;

  case UiState::MENU:
    if (upJustPressed)
      uiState = UiState::HOME;
    if (downJustPressed)
      animation->menuNext();
    if (selectJustPressed) {
      uint8_t sel = animation->getMenuSelection();
      if (sel == 0) {
        uiState = UiState::SCHEDULE_LIGHT;
        animation->setActiveScheduleId(0);
      } else if (sel == 1) {
        uiState = UiState::LOGS;
        logsViewState = LogsViewState::SELECT_TYPE;
        logsTypeSelection = 0;
        animation->setLogsCriticalMode(false);
      }
      else if (sel == 2) {
        uiState = UiState::SETTINGS_DATETIME;
        animation->setActiveScheduleId(5);
      } else if (sel == 3) {
        uiState = UiState::TESTS;
        animation->enterTestMode();
      } else if (sel == 4) {
        SystemController::runFeederCalibration(&display);
        uiState = UiState::MENU;
      } else if (sel == 5) {
        AkwariumWifi::startAP();
        LogManager::logInfo(
            "Menu WiFi: start sesji WiFi (STA 6 s, potem fallback do AP).");
        uiState = UiState::ACCESS_POINT;
      } else if (sel == 6) {
        BleManager::start();
        LogManager::logInfo("Bluetooth: zlecono uruchomienie z menu.");
        uiState = UiState::BLUETOOTH;
      }
    }
    break;

  case UiState::ACCESS_POINT: {
    if (upJustPressed) {
      AkwariumWifi::stopAP();
      LogManager::logInfo("Sesja WiFi zakonczona z menu (wylaczono STA/AP).");
      uiState = UiState::MENU;
    }
    break;
  }

  case UiState::BLUETOOTH: {
    static uint8_t maxClients = 0;
    static unsigned long lastClientSeenMs = 0;
    constexpr unsigned long BLE_CLIENT_GRACE_MS = 60000UL;
    uint8_t currentClients = BleManager::getConnectedClients();
    if (currentClients > maxClients) {
      maxClients = currentClients;
    }
    if (currentClients > 0) {
      lastClientSeenMs = millis();
    }

    // Auto-disconnect analogicznie do AP: po wyjsciu ostatniego klienta
    // czekamy chwile i zamykamy sesje BLE.
    if (maxClients > 0 && currentClients == 0 && lastClientSeenMs > 0 &&
        (millis() - lastClientSeenMs >= BLE_CLIENT_GRACE_MS)) {
      BleManager::stop();
      LogManager::logInfo("Bluetooth wylaczony automatycznie.");
      uiState = UiState::HOME;
      maxClients = 0;
      lastClientSeenMs = 0;
    }

    if (upJustPressed) {
      BleManager::stop();
      LogManager::logInfo("Bluetooth wylaczony z menu.");
      uiState = UiState::MENU;
      maxClients = 0;
      lastClientSeenMs = 0;
    }
    break;
  }

  case UiState::LOGS:
    if (logsViewState == LogsViewState::SELECT_TYPE) {
      resetLogsDeleteHoldState();
      if (upJustPressed)
        uiState = UiState::MENU;
      if (downJustPressed) {
        logsTypeSelection = (logsTypeSelection + 1) % 3;
      }
      if (selectJustPressed) {
        if (logsTypeSelection == 0) {
          logsViewState = LogsViewState::SHOW_NORMAL;
          animation->setLogsCriticalMode(false);
        } else if (logsTypeSelection == 1) {
          logsViewState = LogsViewState::SHOW_CRITICAL;
          animation->setLogsCriticalMode(true);
        } else {
          logsViewState = LogsViewState::SHOW_STATS;
        }
      }
    } else if (logsViewState == LogsViewState::SHOW_STATS) {
      resetLogsDeleteHoldState();
      if (upJustPressed) {
        logsViewState = LogsViewState::SELECT_TYPE;
      }
      if (selectJustPressed) {
        logsViewState = LogsViewState::SHOW_NORMAL;
        logsTypeSelection = 0;
        animation->setLogsCriticalMode(false);
      }
    } else {
      if (upJustPressed) {
        logsViewState = LogsViewState::SELECT_TYPE;
        resetLogsDeleteHoldState();
      }
      if (downJustPressed) {
        animation->logScrollNext();
      }

      if (isSelectPressed) {
        if (logsDeleteHoldStartMs == 0) {
          logsDeleteHoldStartMs = nowMs;
          logsDeleteHoldTriggered = false;
        }

        if (!logsDeleteHoldTriggered) {
          unsigned long heldMs = nowMs - logsDeleteHoldStartMs;
          logsDeleteHoldActive = true;
          logsDeleteHoldProgress =
              static_cast<uint8_t>(min(100UL, (heldMs * 100UL) / LOGS_DELETE_HOLD_MS));

          if (heldMs >= LOGS_DELETE_HOLD_MS) {
            if (logsViewState == LogsViewState::SHOW_NORMAL) {
              LogManager::clearNormalLogs();
            } else {
              LogManager::clearCriticalLogs();
            }
            logsDeleteHoldTriggered = true;
            logsDeleteHoldActive = false;
            logsDeleteHoldProgress = 0;
          }
        }
      } else {
        if (logsDeleteHoldStartMs != 0 && !logsDeleteHoldTriggered) {
          if (logsViewState == LogsViewState::SHOW_NORMAL) {
            logsViewState = LogsViewState::SHOW_CRITICAL;
            logsTypeSelection = 1;
            animation->setLogsCriticalMode(true);
          } else if (logsViewState == LogsViewState::SHOW_CRITICAL) {
            logsViewState = LogsViewState::SHOW_STATS;
            logsTypeSelection = 2;
          }
        }
        logsDeleteHoldStartMs = 0;
        logsDeleteHoldTriggered = false;
        logsDeleteHoldActive = false;
        logsDeleteHoldProgress = 0;
      }
    }
    break;

  case UiState::TESTS:
    if (upJustPressed) {
      animation->cancelEditing();
      uiState = UiState::MENU;
    }
    if (downJustPressed) {
      if (animation->isEditingActive())
        animation->incrementTestValue();
      else
        animation->testNext();
    }
    if (selectJustPressed)
      animation->toggleTestOption();
    break;

  case UiState::SETTINGS_DATETIME:
    if (selectJustPressed) {
      if (!animation->isEditingActive())
        animation->startEditing();
      else
        animation->nextEditStep();
    }
    if (downJustPressed) {
      if (!animation->isEditingActive())
        animation->scheduleNext();
      else
        animation->scheduleEditIncrement();
    }
    if (upJustPressed) {
      if (!animation->isEditingActive())
        uiState = UiState::MENU;
    }
    break;

  case UiState::SCHEDULE_LIGHT:
    if (selectJustPressed) {
      if (!animation->isEditingActive())
        animation->startEditing();
      else
        animation->nextEditStep();
    }
    if (downJustPressed) {
      if (!animation->isEditingActive()) {
        if (animation->getScheduleSelection() == 2) {
          uiState = UiState::SCHEDULE_FILTER;
          animation->setActiveScheduleId(2);
        } else
          animation->scheduleNext();
      } else
        animation->scheduleEditIncrement();
    }
    if (upJustPressed) {
      if (!animation->isEditingActive())
        uiState = UiState::MENU;
    }
    break;

  case UiState::SCHEDULE_AERATION:
    if (selectJustPressed) {
      if (!animation->isEditingActive())
        animation->startEditing();
      else
        animation->nextEditStep();
    }
    if (downJustPressed) {
      if (!animation->isEditingActive()) {
        if (animation->getScheduleSelection() == 2) {
          uiState = UiState::SCHEDULE_TEMP;
          animation->setActiveScheduleId(3);
        } else
          animation->scheduleNext();
      } else
        animation->scheduleEditIncrement();
    }
    if (upJustPressed) {
      if (!animation->isEditingActive())
        uiState = UiState::MENU;
    }
    break;

  case UiState::SCHEDULE_FILTER:
    if (selectJustPressed) {
      if (!animation->isEditingActive())
        animation->startEditing();
      else
        animation->nextEditStep();
    }
    if (downJustPressed) {
      if (!animation->isEditingActive()) {
        if (animation->getScheduleSelection() == 2) {
          uiState = UiState::SCHEDULE_AERATION;
          animation->setActiveScheduleId(1);
        } else
          animation->scheduleNext();
      } else
        animation->scheduleEditIncrement();
    }
    if (upJustPressed) {
      if (!animation->isEditingActive())
        uiState = UiState::MENU;
    }
    break;

  case UiState::SCHEDULE_TEMP:
    if (selectJustPressed) {
      if (!animation->isEditingActive())
        animation->startEditing();
      else
        animation->nextEditStep();
    }
    if (downJustPressed) {
      if (!animation->isEditingActive()) {
        uiState = UiState::SCHEDULE_FEEDING;
        animation->setActiveScheduleId(4);
      } else
        animation->scheduleEditIncrement();
    }
    if (upJustPressed) {
      if (!animation->isEditingActive())
        uiState = UiState::MENU;
    }
    break;

  case UiState::SCHEDULE_FEEDING:
    if (selectJustPressed) {
      if (!animation->isEditingActive())
        animation->startEditing();
      else
        animation->nextEditStep();
    }
    if (downJustPressed) {
      if (!animation->isEditingActive()) {
        if (animation->getScheduleSelection() == 0)
          animation->scheduleNext();
        else {
          uiState = UiState::SCHEDULE_LIGHT;
          animation->setActiveScheduleId(0);
        }
      } else
        animation->scheduleEditIncrement();
    }
    if (upJustPressed) {
      if (!animation->isEditingActive())
        uiState = UiState::MENU;
    }
    break;

  case UiState::FEEDING:
    // exit logic handled elswhere or just exit
    break;
  }

  if (uiState != UiState::LOGS) {
    resetLogsDeleteHoldState();
  }

  lastUpPressed = isUpPressed;
  lastSelectPressed = isSelectPressed;
  lastDownPressed = isDownPressed;
}

void VideoTask(void *pvParameters) {
  while (true) {
    if (animation != nullptr) {
      updateUiState();
      syncTestOverridesWithUiState();
      syncBleSessionWithUiState();
      captureUiChanges();

      bool isUp = (digitalRead(BUTTON_UP_PIN) == LOW);
      bool isSel = (digitalRead(BUTTON_SELECT_PIN) == LOW);
      bool isDn = (digitalRead(BUTTON_DOWN_PIN) == LOW);

      display.clearBuffer();

      // Podczepienie SharedState danych przed rysowaniem
      SharedStateData snap = SharedState::getSnapshot();
      animation->setTemperature(snap.temperature);
      animation->setAeration(snap.aerationPercent);
      animation->setFilterStatus(snap.isFilterOn);
      animation->setLightStatus(snap.isLightOn);
      animation->setHeaterStatus(snap.isHeaterOn);
      animation->setTime(snap.hour, snap.minute, snap.second);
      animation->setDate(snap.day, snap.month, snap.year);
      animation->setBatteryVoltage(PowerManager::getBatteryVoltage());
      animation->setBattery(PowerManager::getBatteryPercent());

      Config cfg = ConfigManager::getCopy();
        animation->setLightSchedule(cfg.dayStartHour, cfg.dayStartMinute,
                                  cfg.dayEndHour, cfg.dayEndMinute);
      animation->setLightMode(cfg.lightMode);
      animation->setAerationSchedule(cfg.aerationHourOn, cfg.aerationMinuteOn,
                                     cfg.aerationHourOff,
                                     cfg.aerationMinuteOff);
      animation->setAerationMode(cfg.aerationMode);
      animation->setFilterSchedule(cfg.filterHourOn, cfg.filterMinuteOn,
                                   cfg.filterHourOff, cfg.filterMinuteOff);
      animation->setFilterMode(cfg.filterMode);
      animation->setTargetTempSetting(static_cast<uint8_t>(cfg.targetTemp));
      animation->setHeaterMode(cfg.heaterMode);
      char feedTime[6];
      snprintf(feedTime, sizeof(feedTime), "%02u:%02u", cfg.feedHour,
               cfg.feedMinute);
      animation->setFeedingSchedule(feedTime, cfg.feedMode, 0);
      consumePendingUiSaveConfirmationAnimation();

      if (!animation->drawConfirmAnimationFrame()) {
        switch (uiState) {
        case UiState::HOME:
          animation->drawFrame();
          break;
        case UiState::MENU:
          animation->drawMenu(isUp, isSel, isDn);
          break;
        case UiState::SCHEDULE_LIGHT:
          animation->drawSchedule(isUp, isSel, isDn);
          break;
        case UiState::SCHEDULE_AERATION:
          animation->drawScheduleAeration(isUp, isSel, isDn);
          break;
        case UiState::SCHEDULE_FILTER:
          animation->drawScheduleFilter(isUp, isSel, isDn);
          break;
        case UiState::SCHEDULE_TEMP:
          animation->drawScheduleTemp(isUp, isSel, isDn);
          break;
        case UiState::SCHEDULE_FEEDING:
          animation->drawScheduleFeeding(isUp, isSel, isDn);
          break;
        case UiState::LOGS:
          if (logsViewState == LogsViewState::SELECT_TYPE) {
            animation->drawLogsCategoryMenu(logsTypeSelection, isUp, isSel,
                                            isDn);
          } else if (logsViewState == LogsViewState::SHOW_STATS) {
            animation->drawLogsStats(AQUARIUM_FIRMWARE_VERSION,
                                     SystemController::getResetCount(),
                                     SystemController::getUptimeSeconds(),
                                     todayFeedingsCount,
                                     isUp, isSel, isDn);
          } else {
            animation->drawLogs(isUp, isSel, isDn, logsDeleteHoldActive,
                                logsDeleteHoldProgress);
          }
          break;
        case UiState::SETTINGS_DATETIME:
          animation->drawSettingsDateTime(isUp, isSel, isDn);
          break;
        case UiState::TESTS:
          animation->drawTests(isUp, isSel, isDn);
          break;
        case UiState::ACCESS_POINT: {
          String modeLabel = "WiFi";
          String primaryLine = "Offline";
          String secondaryLine = "UP: wyjscie";

          if (AkwariumWifi::getIsAPMode()) {
            modeLabel = "AP";
            primaryLine = String("SSID: ") + AkwariumWifi::getConfiguredAPName();
            secondaryLine =
                String("Haslo: ") + AkwariumWifi::getConfiguredAPPassword();
          } else if (AkwariumWifi::isStaConnecting()) {
            primaryLine = "Proba polaczenia STA...";
            secondaryLine = "Po 6 s fallback do AP";
          } else if (AkwariumWifi::isStaConnected()) {
            modeLabel = "STA";
            primaryLine = String("SSID: ") + AkwariumWifi::getStaSsid();
            secondaryLine = "Polaczono z routerem";
          } else if (AkwariumWifi::isServiceModeActive()) {
            primaryLine = "Brak polaczenia STA";
            secondaryLine = "Fallback do AP nieudany";
          }

          animation->drawAccessPointScreen(
              modeLabel.c_str(), primaryLine.c_str(), secondaryLine.c_str(),
              AkwariumWifi::getIP().c_str(), AkwariumWifi::getConnectedClients());
          break;
        }
        case UiState::BLUETOOTH:
          animation->drawBluetoothScreen(
              BleManager::getDeviceName(),
              BleManager::isAdvertising(),
              BleManager::isConnected(),
              BleManager::getConnectedClients(),
              BleManager::getPasskey());
          break;
        case UiState::FEEDING:
          animation->drawFeedingScreen();
          break;
        }
      }
      display.sendBuffer();
    }

    // Obsluga usypiania po uplywie SCREEN_TIMEOUT
    SystemController::handlePowerManagement(&display, animation);

    vTaskDelay(pdMS_TO_TICKS(42)); // OkoĹ‚o 24 FPS
  }
}

void setup() {
  clampRelayPinsAtBoot();

  Serial.begin(115200);

  // ESP32-S3 Zero: stabilna magistrala I2C dla OLED/RTC na GPIO8(GPIO SDA) i GPIO9(GPIO SCL)
  Wire.begin(8, 9);
  Wire.setClock(100000L);

  // Natychmiastowa odpowiedz po starcie urzadzenia.
  display.begin();
  display.setContrast(255);
  display.setPowerSave(0);
  display.clearBuffer();
  display.setFont(u8g2_font_4x6_tr);

  const char *rawVersion = AQUARIUM_FIRMWARE_VERSION;
  char versionLabel[40] = {0};
  if (!rawVersion || rawVersion[0] == '\0') {
    rawVersion = "dev";
  }

  if (rawVersion[0] == 'v' || rawVersion[0] == 'V') {
    snprintf(versionLabel, sizeof(versionLabel), "%s", rawVersion);
  } else {
    snprintf(versionLabel, sizeof(versionLabel), "v%s", rawVersion);
  }

  const char *startupTitle = "Sterownik Akwarium";
  int16_t titleX =
      (display.getDisplayWidth() - display.getStrWidth(startupTitle)) / 2;
  if (titleX < 0) {
    titleX = 0;
  }
  display.drawStr(titleX, 8, startupTitle);

  const char *authorLine = "by Bartosz Wolny";
  int16_t authorX =
      (display.getDisplayWidth() - display.getStrWidth(authorLine)) / 2;
  if (authorX < 0) {
    authorX = 0;
  }
  display.drawStr(authorX, 18, authorLine);

  char versionLine[40] = {0};
  snprintf(versionLine, sizeof(versionLine), "ver. %s", versionLabel);
  int16_t versionX =
      (display.getDisplayWidth() - display.getStrWidth(versionLine)) / 2;
  if (versionX < 0) {
    versionX = 0;
  }
  display.drawStr(versionX, 28, versionLine);
  display.sendBuffer();
  delay(4000);
  logBootStage("ekran startowy gotowy");

  // Inicjalizacja sprzetu, pamieci (CRC NVS), baterii i logow
  logBootStage("SystemController::init start");
  SystemController::init();
  logBootStage("SystemController::init done");

  animation = new AquariumAnimation(&display);
  logBootStage("AquariumAnimation ready");

  setupApiEndpoints();
  logBootStage("API handlers ready");

  // UI uruchamiamy jak najwczesniej, aby nie zostawiac sterownika na samym
  // splash screen przy ciezszej inicjalizacji pozostalych modulow.
  if (xTaskCreatePinnedToCore(VideoTask, "VideoTask", 12288, NULL, 1, NULL, 0) ==
      pdPASS) {
    logBootStage("VideoTask started");
  } else {
    Serial.println("[BOOT] ERROR: VideoTask start failed");
  }

  AkwariumWifi::begin();
  logBootStage("WifiTask start requested");

  // BLE ma byc wlaczane z menu Bluetooth, wiec inicjalizujemy je leniwie.
  logBootStage("BLE deferred until first use");

  // Watchdog dopinamy dopiero po zakonczeniu ciezkiego setup(), aby uniknac
  // resetu na ekranie powitalnym przy dluzszym starcie stosow BLE/WiFi.
  const esp_err_t wdtErr = esp_task_wdt_add(NULL);
  if (wdtErr == ESP_OK) {
    esp_task_wdt_reset();
    logBootStage("loop watchdog armed");
  } else {
    Serial.printf("[BOOT] WARN: esp_task_wdt_add failed (%d)\n",
                  static_cast<int>(wdtErr));
  }

  Serial.println("[SYSTEM] Setup zakonczony na rdzeniu: " +
                 String(xPortGetCoreID()));
  LogManager::logInfo("BOOT: setup zakonczony.");
}

void loop() {
  applyPendingUiChanges();

  // Glowna petla obslugujaca sensory, decyzje i wykonawcze elementy na Core 1
  SystemController::update();
  OtaManager::update();
  BleManager::update();
  // Wifi Server handle juz leci asynchronicznie lub poprzez dedykowany
  // handleClient, wiec upewnijmy sie ze w Wifi.cpp tak jet. Tu ewentualnie
  // dodac AkwariumWifi::handleClient() jesli brakuje.

  vTaskDelay(pdMS_TO_TICKS(10)); // Swobodne oddychanie dla taskow FreeRTOS
}
