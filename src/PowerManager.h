#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include "BatteryReader.h"
#include "ConfigData.h"

class PowerManager {
public:
  static void init(BatteryReader *reader);
  static void update();
  static void registerActivity(); // Reset piku bezczynności (usypiania)

  static unsigned long getLastActivityTime() { return lastActivityTime; }

  static PowerMode getCurrentMode() { return currentMode; }
  static void setMode(PowerMode mode);

  // Pobiera gotowy procent do wrzucenia do UI i uaktualnia Mutex SharedState
  static float getBatteryVoltage() { return lastValidVoltage; }
  static float getBatteryPercent() { return batteryPercent; }

private:
  static BatteryReader *batteryReader;
  static unsigned long lastActivityTime;
  static PowerMode currentMode;

  static float lastValidVoltage;
  static float batteryPercent;
  static bool hasValidBatteryVoltage;
  static bool batteryCriticalLogged;

  static const unsigned long DEEP_SLEEP_TIMEOUT = 300000;
  static const unsigned long SCREEN_TIMEOUT = 240000;

  static uint8_t cr2032PercentFromVoltage(float vbat);
};

#endif // POWER_MANAGER_H
