#include "OledApp.h"

#include "AkwariumWifi.h"
#include "AquariumAnimation.h"
#include "ConfigService.h"
#include "LogManager.h"

#include <Arduino.h>
#include <RTClib.h>

namespace {

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

static void queueScheduleUpdateFromAnimation(AquariumAnimation *animation) {
  if (animation == nullptr) {
    return;
  }

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

static void queueTimeUpdateFromAnimation(AquariumAnimation *animation) {
  if (animation == nullptr) {
    return;
  }

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

} // namespace

void requestUiSaveConfirmationAnimation() {
  portENTER_CRITICAL(&pendingUiMux);
  hasPendingSaveConfirmAnimation = true;
  portEXIT_CRITICAL(&pendingUiMux);
}

namespace OledApp {

void suppressUiTimeSyncForManualFeed(unsigned long nowMs,
                                     AquariumAnimation *animation) {
  suppressUiTimeSyncUntilMs = nowMs + 3000UL;

  portENTER_CRITICAL(&pendingUiMux);
  hasPendingTimeUpdate = false;
  pendingTimeUpdate = {};
  portEXIT_CRITICAL(&pendingUiMux);

  if (animation != nullptr) {
    if (animation->isEditingActive()) {
      animation->cancelEditing();
    }
    animation->hasTimeChanged();
  }
}

void captureUiChanges(AquariumAnimation *animation, bool allowTimeUpdate) {
  if (animation == nullptr) {
    return;
  }

  if (animation->hasScheduleChanged()) {
    queueScheduleUpdateFromAnimation(animation);
  }

  const bool timeChanged = animation->hasTimeChanged();
  const unsigned long nowMs = millis();
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

  if (allowTimeUpdate) {
    queueTimeUpdateFromAnimation(animation);
  } else {
    LogManager::logWarn(
        "Odrzucono zalegla zmiane czasu poza ekranem daty/czasu.");
  }
}

void applyPendingUiChanges() {
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

    const ConfigApplyResult applyResult = ConfigService::applyPatch(patch);
    if (applyResult.ok) {
      if (applyResult.partial) {
        LogManager::logWarn(
            "Harmonogram zapisany po korekcie wartosci do dozwolonego formatu.");
      } else if (applyResult.saveResult.status ==
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
               configSaveStatusToString(applyResult.saveResult.status),
               static_cast<unsigned>(applyResult.saveResult.bytesWritten),
               static_cast<unsigned>(applyResult.saveResult.bytesReadBack));
      LogManager::logError(msg);
    }
  }

  if (applyTime) {
    const uint8_t hour = constrain(localTime.hour, 0, 23);
    const uint8_t minute = constrain(localTime.minute, 0, 59);
    const uint8_t second = constrain(localTime.second, 0, 59);
    const uint8_t day = constrain(localTime.day, 1, 31);
    const uint8_t month = constrain(localTime.month, 1, 12);
    const uint16_t year = constrain(localTime.year, 2024, 2099);
    const DateTime newTime(year, month, day, hour, minute, second);
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

void consumePendingUiSaveConfirmationAnimation(AquariumAnimation *animation) {
  bool shouldPlayAnimation = false;

  portENTER_CRITICAL(&pendingUiMux);
  shouldPlayAnimation = hasPendingSaveConfirmAnimation;
  hasPendingSaveConfirmAnimation = false;
  portEXIT_CRITICAL(&pendingUiMux);

  if (shouldPlayAnimation && animation != nullptr) {
    animation->playConfirmAnimation();
  }
}

} // namespace OledApp
