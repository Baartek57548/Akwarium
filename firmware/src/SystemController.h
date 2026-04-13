#ifndef SYSTEM_CONTROLLER_H
#define SYSTEM_CONTROLLER_H

#include <Arduino.h>
#include <RTClib.h>
#include <U8g2lib.h>

class AquariumAnimation;

#include <Wire.h>

#include "BatteryReader.h"
#include "ConfigManager.h"
#include "FeederController.h"
#include "ServoController.h"
#include "SharedState.h"
#include "TemperatureController.h"

enum SleepBlockerFlags : uint16_t {
  SLEEP_BLOCKER_IDLE_WINDOW = 1U << 0,
  SLEEP_BLOCKER_OUTPUTS_ACTIVE = 1U << 1,
  SLEEP_BLOCKER_NOT_NIGHT = 1U << 2,
  SLEEP_BLOCKER_OTA = 1U << 3,
  SLEEP_BLOCKER_AP_MODE = 1U << 4,
  SLEEP_BLOCKER_SERVICE_MODE = 1U << 5,
  SLEEP_BLOCKER_TIME_SYNC = 1U << 6,
  SLEEP_BLOCKER_STA_ACTIVE = 1U << 7,
  SLEEP_BLOCKER_FEEDING = 1U << 8
};

class SystemController {
public:
  using BootStageReporter = void (*)(const char *stage);

  static void init();
  static void update();
  static void setBootStageReporter(BootStageReporter reporter);

  // Akcje wymuszane z WebUI / Menu
  static Error feedNow();
  static bool isFeedingNow();
  static Error getLastFeederError();
  static void setManualServo(int angle);
  static void clearManualServo();
  static int getServoPosition();
  static void
  setTestOverrides(bool lightOn, bool filterOn, bool heaterConnected,
                   bool feederRelayOn, uint8_t aerationAngle);
  static void clearTestOverrides();
  static bool isTestOverrideActive();

  // Reczna kalibracja karmnika uruchamiana z menu
  static bool runFeederCalibration(U8G2 *display);

  // Funkcja zarzadzania energia, wywolywana glownie przez VideoTask lub loop
  static void handlePowerManagement(U8G2 *display, AquariumAnimation *anim);
  static bool canEnterLightSleep(unsigned long nowMs,
                                 unsigned long lastActionMs);
  static uint16_t getLightSleepBlockers(unsigned long nowMs,
                                        unsigned long lastActionMs);
  static uint16_t getCurrentLightSleepBlockers();
  static void enterNightLightSleep();
  static const char *powerModeToString(PowerMode mode);
  static const char *getPowerModeLabel();

  // Publiczna instancja RTC do globalnych funkcji czasowych
  static RTC_DS3231 rtc;
  static bool isRtcReady();

  // Diagnostyka resetow
  static int getLastResetReason();
  static const char *getLastResetLabel();
  static uint32_t getResetCount();
  static uint32_t getUptimeSeconds();

private:
  static void hardwareSetup();
  static void updateSensors();
  static void updateDecisions();
  static void applyOutputs();

  // Czujniki i kontrolery
  static TemperatureController tempController;
  static FeederController feederController;
  static ServoController servoController;
  static BatteryReader batteryReader;

  // Zmienne systemowe
  static bool manualServoOverride;
  static int manualServoAngle;
  static unsigned long manualServoTimer;

  static uint8_t tempInvalidReadCount;
  static bool tempSensorErrorLogged;
  static bool rtcReady;
  static int lastResetReason;

  // Opcjonalne obiekty Timera by nie zamrazac petli
  static unsigned long lastTempCheckMs;
  static unsigned long lastBatCheckMs;
};

#endif // SYSTEM_CONTROLLER_H
