#include "ScheduleManager.h"
#include "LogManager.h"
#include "SharedState.h"


FeederController *ScheduleManager::feederCtrl = nullptr;

namespace {

static bool isModeActive(uint8_t rawMode, uint16_t nowMin, uint8_t hourOn,
                         uint8_t minuteOn, uint8_t hourOff,
                         uint8_t minuteOff) {
  const ScheduleMode mode = static_cast<ScheduleMode>(rawMode);
  switch (mode) {
  case ScheduleMode::AlwaysOn:
    return true;
  case ScheduleMode::AlwaysOff:
    return false;
  case ScheduleMode::Schedule:
  default: {
    const uint16_t startMin = ScheduleManager::toMinutes(hourOn, minuteOn);
    const uint16_t endMin = ScheduleManager::toMinutes(hourOff, minuteOff);
    return ScheduleManager::isWithinWindow(nowMin, startMin, endMin);
  }
  }
}

} // namespace

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
  const Config sysConfig = ConfigManager::getCopy();
  return isModeActive(sysConfig.lightMode, nowMin, sysConfig.dayStartHour,
                      sysConfig.dayStartMinute, sysConfig.dayEndHour,
                      sysConfig.dayEndMinute);
}

bool ScheduleManager::isAerationActive(uint16_t nowMin) {
  const Config sysConfig = ConfigManager::getCopy();
  return isModeActive(sysConfig.aerationMode, nowMin, sysConfig.aerationHourOn,
                      sysConfig.aerationMinuteOn, sysConfig.aerationHourOff,
                      sysConfig.aerationMinuteOff);
}

bool ScheduleManager::isFilterActive(uint16_t nowMin) {
  const Config sysConfig = ConfigManager::getCopy();
  return isModeActive(sysConfig.filterMode, nowMin, sysConfig.filterHourOn,
                      sysConfig.filterMinuteOn, sysConfig.filterHourOff,
                      sysConfig.filterMinuteOff);
}

int ScheduleManager::getMinutesUntilFilterOff(uint16_t nowMin) {
  const Config sysConfig = ConfigManager::getCopy();
  if (sysConfig.filterMode != static_cast<uint8_t>(ScheduleMode::Schedule)) {
    return -1;
  }

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
  Config sysConfig = ConfigManager::getCopy();
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
        ConfigManager::updateAndSave(sysConfig);
      } else {
        LogManager::logError(
            "Blad aktywacji auto-karmnika. Silnik lub czujnik nie odpowiada.");
      }
    }
  }
}

void ScheduleManager::update(const DateTime &now) { checkAutoFeed(now); }
