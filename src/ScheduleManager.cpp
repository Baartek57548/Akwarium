#include "ScheduleManager.h"
#include "LogManager.h"
#include "SharedState.h"


FeederController *ScheduleManager::feederCtrl = nullptr;

void ScheduleManager::init(FeederController *feeder) { feederCtrl = feeder; }

uint16_t ScheduleManager::toMinutes(uint8_t hour, uint8_t minute) {
  return (uint16_t)hour * 60U + (uint16_t)minute;
}

bool ScheduleManager::isWithinWindow(uint16_t nowMin, uint16_t startMin,
                                     uint16_t endMin) {
  if (startMin == endMin)
    return false;
  if (startMin < endMin)
    return (nowMin >= startMin && nowMin < endMin);
  return (nowMin >= startMin || nowMin < endMin);
}

bool ScheduleManager::isDayTime(uint16_t nowMin) {
  Config &sysConfig = ConfigManager::getConfig();
  if (sysConfig.dayStartHour == 24)
    return true;
  if (sysConfig.dayEndHour == 24)
    return false;

  uint16_t dayOn = toMinutes(sysConfig.dayStartHour, sysConfig.dayStartMinute);
  uint16_t dayOff = toMinutes(sysConfig.dayEndHour, sysConfig.dayEndMinute);
  return isWithinWindow(nowMin, dayOn, dayOff);
}

bool ScheduleManager::isAerationActive(uint16_t nowMin) {
  Config &sysConfig = ConfigManager::getConfig();
  uint16_t aerationOn =
      toMinutes(sysConfig.aerationHourOn, sysConfig.aerationMinuteOn);
  uint16_t aerationOff =
      toMinutes(sysConfig.aerationHourOff, sysConfig.aerationMinuteOff);
  return isWithinWindow(nowMin, aerationOn, aerationOff);
}

bool ScheduleManager::isFilterActive(uint16_t nowMin) {
  Config &sysConfig = ConfigManager::getConfig();
  uint16_t filterOn =
      toMinutes(sysConfig.filterHourOn, sysConfig.filterMinuteOn);
  uint16_t filterOff =
      toMinutes(sysConfig.filterHourOff, sysConfig.filterMinuteOff);
  return isWithinWindow(nowMin, filterOn, filterOff);
}

int ScheduleManager::getMinutesUntilFilterOff(uint16_t nowMin) {
  Config &sysConfig = ConfigManager::getConfig();
  uint16_t startMin =
      toMinutes(sysConfig.filterHourOn, sysConfig.filterMinuteOn);
  uint16_t endMin =
      toMinutes(sysConfig.filterHourOff, sysConfig.filterMinuteOff);

  if (!isWithinWindow(nowMin, startMin, endMin))
    return -1;
  if (startMin < endMin)
    return (int)endMin - (int)nowMin;
  if (nowMin < endMin)
    return (int)endMin - (int)nowMin;
  return (24 * 60 - (int)nowMin) + (int)endMin;
}

void ScheduleManager::checkAutoFeed(const DateTime &now) {
  Config &sysConfig = ConfigManager::getConfig();
  if (sysConfig.feedMode == 0 || feederCtrl == nullptr)
    return;

  if (now.hour() == sysConfig.feedHour &&
      now.minute() == sysConfig.feedMinute && now.second() < 5) {
    uint32_t diff = now.unixtime() - sysConfig.lastFeedEpoch;
    bool shouldFeed = false;
    if (sysConfig.feedMode == 1 && diff >= 86000)
      shouldFeed = true;
    if (sysConfig.feedMode == 2 && diff >= 172000)
      shouldFeed = true;
    if (sysConfig.feedMode == 3 && diff >= 258000)
      shouldFeed = true;

    if (shouldFeed) {
      // Zabezpieczenie przed podwojnym karmieniem w krotkim czasie (odstep
      // minimum 3h)
      if (now.unixtime() - sysConfig.lastFeedEpoch < (3 * 3600))
        return;

      Error err = feederCtrl->startFeed(1500, true);
      if (err == Error::NONE) {
        feederCtrl->clearError();
        // Potem podlaczymy to pod systemController by ustawil STATE_FEEDING
        LogManager::logInfo("Auto Feed rozpoczety z harmonogramu.");
        sysConfig.lastFeedEpoch = now.unixtime();
        ConfigManager::save();
      } else {
        LogManager::logError(
            "Blad aktywacji auto-karmnika. Silnik lub czujnik nie odpowiada.");
      }
    }
  }
}

void ScheduleManager::update(const DateTime &now) { checkAutoFeed(now); }
