/*
 * PROJEKT: Sterownik Akwarium wifi
 * AUTOR: Bartosz Wolny + (AI Assistant)
 * PLATFORMA: ESP32-S3 Zero 240Mhz (Dual Core - FreeRTOS)
 */

#include "AkwariumWifi.h"
#include "ApiHandlers.h"
#include "AquariumAnimation.h"
#include "ConfigManager.h"
#include "FirmwareInfo.h"
#include "LogManager.h"
#include "OledApp.h"
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
static constexpr uint32_t VIDEO_TASK_STACK_BYTES = 20480;
static constexpr BaseType_t VIDEO_TASK_CORE = 0;
static constexpr UBaseType_t VIDEO_TASK_PRIORITY = 1;
static constexpr TickType_t VIDEO_FRAME_DELAY = pdMS_TO_TICKS(42);

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
  SETTINGS_DISPLAY_POWER,
  SETTINGS_SYSTEM,
  TESTS,
  FEEDING,
  ACCESS_POINT
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
unsigned long systemFactoryResetHoldStartMs = 0;
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
#define SYSTEM_FACTORY_RESET_HOLD_MS 4000UL

static constexpr uint8_t BUTTON_MASK_UP = (1U << 0);
static constexpr uint8_t BUTTON_MASK_SELECT = (1U << 1);
static constexpr uint8_t BUTTON_MASK_DOWN = (1U << 2);
static constexpr uint8_t SYSTEM_MENU_INFO = 0;
static constexpr uint8_t SYSTEM_MENU_LOGS = 1;
static constexpr uint8_t SYSTEM_MENU_RESTART = 2;
static constexpr uint8_t SYSTEM_MENU_FACTORY_RESET = 3;
static constexpr uint8_t SETTINGS_MENU_SCHEDULES = 0;
static constexpr uint8_t SETTINGS_MENU_TEMPERATURE = 1;
static constexpr uint8_t SETTINGS_MENU_AERATION = 2;
static constexpr uint8_t SETTINGS_MENU_FEEDING = 3;
static constexpr uint8_t SETTINGS_MENU_WIFI = 4;
static constexpr uint8_t SETTINGS_MENU_DISPLAY_POWER = 5;
static constexpr uint8_t SETTINGS_MENU_SYSTEM = 6;

static void clampRelayPinsAtBoot() {
  // Szybkie "usztywnienie" pinow po restarcie, aby ograniczyc przypadkowe
  // klikniecia przekaznikow podczas normalnego programowania i bootowania.
  // Dla restartu po OTA odtwarzamy poziomy zapisane tuz przed restartem.
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
  if (state == UiState::ACCESS_POINT)
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

static void resetSystemFactoryResetHoldState() {
  systemFactoryResetHoldStartMs = 0;
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

static void resetUiStateAfterIdleTimeout() {
  logsViewState = LogsViewState::SELECT_TYPE;
  logsTypeSelection = 0;
  resetLogsDeleteHoldState();
  resetSystemFactoryResetHoldState();
  if (animation) {
    animation->resetNavigationState();
  }
}

static void returnUiToHomeAfterTimeout() {
  resetUiStateAfterIdleTimeout();
  uiState = UiState::HOME;
}

static bool isWifiSessionVisibleOnOled() {
  return AkwariumWifi::isServiceModePending() ||
         AkwariumWifi::isServiceModeActive() ||
         AkwariumWifi::isStaConnecting() ||
         AkwariumWifi::isStaConnected() ||
         AkwariumWifi::getIsAPMode();
}

void updateUiState() {
  if (!animation)
    return;

  unsigned long nowMs = millis();
  if (lastUiInteractionMs == 0) {
    lastUiInteractionMs = nowMs;
  }
  syncDailyFeedingsCounterDate();

  if (uiState == UiState::ACCESS_POINT && !isWifiSessionVisibleOnOled()) {
    returnUiToHomeAfterTimeout();
  }

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
      OledApp::suppressUiTimeSyncForManualFeed(nowMs, animation);
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
      if (uiStateBeforeFeeding == UiState::ACCESS_POINT &&
          !isWifiSessionVisibleOnOled()) {
        returnUiToHomeAfterTimeout();
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
    returnUiToHomeAfterTimeout();
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
      const uint8_t sel = animation->getMenuSelection();
      switch (sel) {
      case SETTINGS_MENU_SCHEDULES:
        uiState = UiState::SCHEDULE_LIGHT;
        animation->setActiveScheduleId(SettingsScreen::HARMONOGRAMY);
        break;
      case SETTINGS_MENU_TEMPERATURE:
        uiState = UiState::SCHEDULE_TEMP;
        animation->setActiveScheduleId(SettingsScreen::TEMPERATURE_AND_HEATER);
        break;
      case SETTINGS_MENU_AERATION:
        uiState = UiState::SCHEDULE_AERATION;
        animation->setActiveScheduleId(SettingsScreen::AERATION_CO2);
        break;
      case SETTINGS_MENU_FEEDING:
        uiState = UiState::SCHEDULE_FEEDING;
        animation->setActiveScheduleId(SettingsScreen::FEEDING);
        break;
      case SETTINGS_MENU_WIFI:
        animation->setActiveScheduleId(SettingsScreen::WIFI);
        AkwariumWifi::startAP();
        LogManager::logInfo(
            "Menu WiFi: start sesji WiFi (STA 6 s, potem fallback do AP).");
        uiState = UiState::ACCESS_POINT;
        break;
      case SETTINGS_MENU_DISPLAY_POWER:
        uiState = UiState::SETTINGS_DISPLAY_POWER;
        animation->setActiveScheduleId(SettingsScreen::DISPLAY_AND_POWER);
        break;
      case SETTINGS_MENU_SYSTEM:
        uiState = UiState::SETTINGS_SYSTEM;
        animation->setActiveScheduleId(SettingsScreen::SYSTEM);
        break;
      default:
        break;
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

  case UiState::SCHEDULE_AERATION:
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
          animation->setActiveScheduleId(SettingsScreen::AERATION_CO2);
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
      if (animation->isEditingActive())
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

  case UiState::SETTINGS_DISPLAY_POWER:
    if (downJustPressed && !animation->isEditingActive())
      animation->scheduleNext();
    if (upJustPressed) {
      animation->cancelEditing();
      uiState = UiState::MENU;
    }
    break;

  case UiState::SETTINGS_SYSTEM: {
    const uint8_t sel = animation->getScheduleSelection();
    const bool factoryResetSelected = sel == SYSTEM_MENU_FACTORY_RESET;

    if (factoryResetSelected && isSelectPressed) {
      if (!animation->isEditingActive()) {
        animation->startEditing();
      }
      if (systemFactoryResetHoldStartMs == 0) {
        systemFactoryResetHoldStartMs = nowMs;
      }
      if (nowMs - systemFactoryResetHoldStartMs >=
          SYSTEM_FACTORY_RESET_HOLD_MS) {
        LogManager::logWarn("Factory reset wywolany z OLED.");
        ConfigManager::resetToDefault();
        LogManager::clearCriticalLogs();
        delay(200);
        ESP.restart();
      }
    } else if (factoryResetSelected &&
               systemFactoryResetHoldStartMs != 0) {
      resetSystemFactoryResetHoldState();
      if (animation->isEditingActive()) {
        animation->cancelEditing();
      }
    }

    if (selectJustPressed && !factoryResetSelected) {
      if (sel == SYSTEM_MENU_INFO) {
        logsViewState = LogsViewState::SHOW_STATS;
        logsTypeSelection = 2;
        uiState = UiState::LOGS;
        resetSystemFactoryResetHoldState();
      } else if (sel == SYSTEM_MENU_LOGS) {
        logsViewState = LogsViewState::SELECT_TYPE;
        logsTypeSelection = 0;
        animation->setLogsCriticalMode(false);
        uiState = UiState::LOGS;
        resetSystemFactoryResetHoldState();
      } else if (!animation->isEditingActive()) {
        animation->startEditing();
      } else if (sel == SYSTEM_MENU_RESTART) {
        LogManager::logWarn("Restart urzadzenia wywolany z OLED.");
        delay(150);
        ESP.restart();
      }
    }
    if (downJustPressed && !animation->isEditingActive()) {
      resetSystemFactoryResetHoldState();
      animation->scheduleNext();
    }
    if (upJustPressed) {
      resetSystemFactoryResetHoldState();
      if (animation->isEditingActive()) {
        animation->cancelEditing();
      } else {
        uiState = UiState::MENU;
      }
    }
    break;
  }

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
  (void)pvParameters;
  const bool videoWdtRegistered = esp_task_wdt_add(NULL) == ESP_OK;

  while (true) {
    if (videoWdtRegistered) {
      esp_task_wdt_reset();
    }

    if (animation != nullptr) {
      updateUiState();
      syncTestOverridesWithUiState();
      OledApp::captureUiChanges(animation,
                                uiState == UiState::SETTINGS_DATETIME);

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
      OledApp::consumePendingUiSaveConfirmationAnimation(animation);

      if (videoWdtRegistered) {
        esp_task_wdt_reset();
      }

      if (!animation->drawConfirmAnimationFrame()) {
        switch (uiState) {
        case UiState::HOME:
          animation->drawFrame();
          break;
        case UiState::MENU:
          animation->drawMenu(isUp, isSel, isDn);
          break;
        case UiState::SCHEDULE_LIGHT:
          animation->drawSettingsSchedules(isUp, isSel, isDn);
          break;
        case UiState::SCHEDULE_AERATION:
          animation->drawSettingsAerationCo2(isUp, isSel, isDn);
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
        case UiState::SETTINGS_DISPLAY_POWER:
          animation->drawSettingsDisplayPower(isUp, isSel, isDn);
          break;
        case UiState::SETTINGS_SYSTEM:
          animation->drawSettingsSystem(isUp, isSel, isDn);
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
          } else if (AkwariumWifi::isServiceModePending()) {
            primaryLine = "Start sesji WiFi...";
            secondaryLine = "STA, potem fallback AP";
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
        case UiState::FEEDING:
          animation->drawFeedingScreen();
          break;
        }
      }
      display.sendBuffer();
      if (videoWdtRegistered) {
        esp_task_wdt_reset();
      }
    }

    // Obsluga usypiania po uplywie SCREEN_TIMEOUT
    SystemController::handlePowerManagement(&display, animation);

    vTaskDelay(VIDEO_FRAME_DELAY); // Okolo 24 FPS
  }
}

void setup() {
  clampRelayPinsAtBoot();

  Serial.begin(115200);

  // ESP32-S3 Zero: stabilna magistrala I2C dla OLED/RTC na GPIO8(GPIO SDA) i GPIO9(GPIO SCL)
  Wire.begin(8, 9);
  Wire.setTimeOut(50);
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
  if (xTaskCreatePinnedToCore(VideoTask, "VideoTask", VIDEO_TASK_STACK_BYTES,
                              NULL, VIDEO_TASK_PRIORITY, NULL,
                              VIDEO_TASK_CORE) == pdPASS) {
    logBootStage("VideoTask started");
  } else {
    Serial.println("[BOOT] ERROR: VideoTask start failed");
    logBootStage("VideoTask start FAIL");
  }

  AkwariumWifi::begin();
  logBootStage("WifiTask start requested");

  // Watchdog dopinamy dopiero po zakonczeniu ciezkiego setup(), aby uniknac
  // resetu na ekranie powitalnym przy dluzszym starcie stosu WiFi.
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
  OledApp::applyPendingUiChanges();

  // Glowna petla obslugujaca sensory, decyzje i wykonawcze elementy na Core 1
  SystemController::update();
  OtaManager::update();
  // Wifi Server handle juz leci asynchronicznie lub poprzez dedykowany
  // handleClient, wiec upewnijmy sie ze w Wifi.cpp tak jet. Tu ewentualnie
  // dodac AkwariumWifi::handleClient() jesli brakuje.

  vTaskDelay(pdMS_TO_TICKS(10)); // Swobodne oddychanie dla taskow FreeRTOS
}
