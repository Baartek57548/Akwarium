#ifndef SCHEDULE_MANAGER_H
#define SCHEDULE_MANAGER_H

#include "ConfigManager.h"
#include "FeederController.h"
#include <Arduino.h>
#include <RTClib.h> // Potrzebne do DateTime


class ScheduleManager {
public:
  static void init(FeederController *feeder);
  static void update(const DateTime &now);

  // Zapytania o harmonogram
  static bool isDayTime(uint16_t nowMin);
  static bool isAerationActive(uint16_t nowMin);
  static bool isFilterActive(uint16_t nowMin);
  static int getMinutesUntilFilterOff(uint16_t nowMin);

  static uint16_t toMinutes(uint8_t hour, uint8_t minute);
  static bool isWithinWindow(uint16_t nowMin, uint16_t startMin,
                             uint16_t endMin);

private:
  static void checkAutoFeed(const DateTime &now);
  static FeederController *feederCtrl;
};

#endif // SCHEDULE_MANAGER_H
