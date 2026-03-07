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
#include "ConfigValidation.h"
#include "OtaManager.h"
#include "PowerManager.h"
#include "SharedState.h"
#include "SystemController.h"

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
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

bool lastUpPressed = false;
bool lastSelectPressed = false;
bool lastDownPressed = false;
unsigned long allButtonsHoldStartMs = 0;
bool manualFeedComboTriggered = false;
bool feedingUiActive = false;
UiState uiStateBeforeFeeding = UiState::HOME;
unsigned long lastUiInteractionMs = 0;

#define BUTTON_UP_PIN 15
#define BUTTON_SELECT_PIN 16
#define BUTTON_DOWN_PIN 14
#define MANUAL_FEED_HOLD_MS 1000UL
#define UI_IDLE_RETURN_HOME_MS 30000UL

static bool shouldApplyUiIdleHomeTimeout(UiState state) {
  if (state == UiState::HOME)
    return false;
  if (state == UiState::ACCESS_POINT || state == UiState::BLUETOOTH)
    return false;
  if (state == UiState::FEEDING)
    return false;
  return true;
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
static PendingScheduleUpdate pendingScheduleUpdate = {};
static PendingTimeUpdate pendingTimeUpdate = {};

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

  if (animation->hasTimeChanged()) {
    queueTimeUpdateFromAnimation();
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
    ConfigPatch patch = {};
    patch.hasLightMode = true;
    patch.lightMode = localSchedule.lightMode;
    patch.hasDayStartHour = true;
    patch.dayStartHour = localSchedule.dayStartHour;
    patch.hasDayStartMinute = true;
    patch.dayStartMinute = localSchedule.dayStartMinute;
    patch.hasDayEndHour = true;
    patch.dayEndHour = localSchedule.dayEndHour;
    patch.hasDayEndMinute = true;
    patch.dayEndMinute = localSchedule.dayEndMinute;

    patch.hasAerationMode = true;
    patch.aerationMode = localSchedule.aerationMode;
    patch.hasAerationHourOn = true;
    patch.aerationHourOn = localSchedule.aerationHourOn;
    patch.hasAerationMinuteOn = true;
    patch.aerationMinuteOn = localSchedule.aerationMinuteOn;
    patch.hasAerationHourOff = true;
    patch.aerationHourOff = localSchedule.aerationHourOff;
    patch.hasAerationMinuteOff = true;
    patch.aerationMinuteOff = localSchedule.aerationMinuteOff;

    patch.hasFilterMode = true;
    patch.filterMode = localSchedule.filterMode;
    patch.hasFilterHourOn = true;
    patch.filterHourOn = localSchedule.filterHourOn;
    patch.hasFilterMinuteOn = true;
    patch.filterMinuteOn = localSchedule.filterMinuteOn;
    patch.hasFilterHourOff = true;
    patch.filterHourOff = localSchedule.filterHourOff;
    patch.hasFilterMinuteOff = true;
    patch.filterMinuteOff = localSchedule.filterMinuteOff;

    patch.hasHeaterMode = true;
    patch.heaterMode = localSchedule.heaterMode;
    patch.hasTargetTemp = true;
    patch.targetTemp = localSchedule.targetTemp;
    patch.hasFeedHour = true;
    patch.feedHour = localSchedule.feedHour;
    patch.hasFeedMinute = true;
    patch.feedMinute = localSchedule.feedMinute;
    patch.hasFeedMode = true;
    patch.feedMode = localSchedule.feedMode;

    Config cfg = ConfigManager::getCopy();
    ConfigValidationResult validation = {};
    if (ConfigValidation::applyRuntimePatch(cfg, patch, validation)) {
      ConfigManager::updateAndSave(cfg);
    }
  }

  if (applyTime) {
    uint8_t hour = constrain(localTime.hour, 0, 23);
    uint8_t minute = constrain(localTime.minute, 0, 59);
    uint8_t second = constrain(localTime.second, 0, 59);
    uint8_t day = constrain(localTime.day, 1, 31);
    uint8_t month = constrain(localTime.month, 1, 12);
    uint16_t year = constrain(localTime.year, 2024, 2099);
    SystemController::rtc.adjust(
        DateTime(year, month, day, hour, minute, second));
  }
}

void updateUiState() {
  if (!animation)
    return;

  unsigned long nowMs = millis();
  if (lastUiInteractionMs == 0) {
    lastUiInteractionMs = nowMs;
  }

  bool isUpPressed = (digitalRead(BUTTON_UP_PIN) == LOW);
  bool isSelectPressed = (digitalRead(BUTTON_SELECT_PIN) == LOW);
  bool isDownPressed = (digitalRead(BUTTON_DOWN_PIN) == LOW);
  bool allButtonsPressed = isUpPressed && isSelectPressed && isDownPressed;
  bool upJustPressed = isUpPressed && !lastUpPressed;
  bool selectJustPressed = isSelectPressed && !lastSelectPressed;
  bool downJustPressed = isDownPressed && !lastDownPressed;

  if (allButtonsPressed) {
    if (allButtonsHoldStartMs == 0) {
      allButtonsHoldStartMs = millis();
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

  bool feedingNow = SystemController::isFeedingNow();
  if (feedingNow && !feedingUiActive) {
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
           !BleManager::isAdvertising())) {
        uiState = UiState::HOME;
      } else {
        uiState = uiStateBeforeFeeding;
      }
    }
  }

  if (allButtonsPressed || manualFeedComboTriggered) {
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
      } else if (sel == 1)
        uiState = UiState::LOGS;
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
        uiState = UiState::ACCESS_POINT;
      } else if (sel == 6) {
        BleManager::start();
        uiState = UiState::BLUETOOTH;
      }
    }
    break;

  case UiState::ACCESS_POINT: {
    static uint8_t maxClients = 0;
    uint8_t currentClients = AkwariumWifi::getConnectedClients();
    if (currentClients > maxClients) {
      maxClients = currentClients;
    }

    // Auto disconnect when client leaves
    if (maxClients > 0 && currentClients == 0) {
      AkwariumWifi::stopAP();
      uiState = UiState::HOME;
      maxClients = 0;
    }

    // Manual exit
    if (upJustPressed) {
      AkwariumWifi::stopAP();
      uiState = UiState::MENU;
      maxClients = 0;
    }
    break;
  }

  case UiState::BLUETOOTH: {
    static bool hasAnyClient = false;
    bool connectedNow = BleManager::isConnected();

    if (connectedNow) {
      hasAnyClient = true;
    }

    // Auto-exit analogicznie do AP (po pierwszym polaczeniu i rozlaczeniu)
    if (hasAnyClient && !connectedNow) {
      BleManager::stop();
      uiState = UiState::HOME;
      hasAnyClient = false;
    }

    // Manual exit
    if (upJustPressed) {
      BleManager::stop();
      uiState = UiState::MENU;
      hasAnyClient = false;
    }
    break;
  }

  case UiState::LOGS:
    if (upJustPressed)
      uiState = UiState::MENU;
    if (downJustPressed)
      animation->logScrollNext();
    break;

  case UiState::TESTS:
    if (upJustPressed)
      uiState = UiState::MENU;
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

  lastUpPressed = isUpPressed;
  lastSelectPressed = isSelectPressed;
  lastDownPressed = isDownPressed;
}

void VideoTask(void *pvParameters) {
  while (true) {
    if (animation != nullptr) {
      updateUiState();
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
          animation->drawLogs(isUp, isSel, isDn);
          break;
        case UiState::SETTINGS_DATETIME:
          animation->drawSettingsDateTime(isUp, isSel, isDn);
          break;
        case UiState::TESTS:
          animation->drawTests(isUp, isSel, isDn);
          break;
        case UiState::ACCESS_POINT:
          animation->drawAccessPointScreen(
              AkwariumWifi::getAPName().c_str(),
              AkwariumWifi::getAPPassword().c_str(),
              AkwariumWifi::getIP().c_str(),
              AkwariumWifi::getConnectedClients());
          break;
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
  Serial.begin(115200);

  // ESP32-S3 Zero: stabilna magistrala I2C dla OLED/RTC na GPIO8(GPIO SDA) i GPIO9(GPIO SCL)
  Wire.begin(8, 9);
  Wire.setClock(100000L);

  // Natychmiastowa odpowiedz po wybudzeniu z deep sleep.
  display.begin();
  display.setContrast(255);
  display.setPowerSave(0);
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tr);
  display.drawStr(0, 12, "Wybudzanie...");
  display.sendBuffer();

  // Inicjalizacja sprzetu, pamieci (CRC NVS), baterii i logow
  SystemController::init();

  animation = new AquariumAnimation(&display);

  setupApiEndpoints();
  AkwariumWifi::begin();
  BleManager::init();

  // Uruchomienie wyswietlania na Core 0 z mechanizmem SharedState Snapshot
  xTaskCreatePinnedToCore(VideoTask, "VideoTask", 10000, NULL, 1, NULL, 0);

  Serial.println("[SYSTEM] Setup zakonczony na rdzeniu: " +
                 String(xPortGetCoreID()));
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
