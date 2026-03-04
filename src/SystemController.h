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

class SystemController {
public:
  static void init();
  static void update();

  // Akcje wymuszane z WebUI / Menu
  static void feedNow();
  static bool isFeedingNow();
  static void setManualServo(int angle);
  static void clearManualServo();
  static int getServoPosition();

  // Kalibracja karmnika przy uruchomieniu (wywoływane w setup przed VideoTask)
  static void runFeederCalibrationOnPowerUp(U8G2 *display);

  // Funkcja zarzadzania energia, wywolywana glownie przez VideoTask lub loop
  static void handlePowerManagement(U8G2 *display, AquariumAnimation *anim);
  static bool canEnterDeepSleep(unsigned long nowMs, unsigned long lastActionMs);
  static void enterNightDeepSleep();

  // Publiczna instancja RTC do globalnych funkcji czasowych
  static RTC_DS3231 rtc;
  static bool isRtcReady();

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

  // Opcjonalne obiekty Timera by nie zamrazac petli
  static unsigned long lastTempCheckMs;
  static unsigned long lastBatCheckMs;
};

#endif // SYSTEM_CONTROLLER_H
