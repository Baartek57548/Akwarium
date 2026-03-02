#include "ConfigValidation.h"
#include <math.h>

namespace {

static void markProvided(ConfigValidationResult &result) {
  if (result.providedFields < 255) {
    result.providedFields++;
  }
}

static void markApplied(ConfigValidationResult &result) {
  if (result.appliedFields < 255) {
    result.appliedFields++;
  }
}

static void markInvalid(ConfigValidationResult &result) {
  if (result.invalidFields < 255) {
    result.invalidFields++;
  }
}

} // namespace

void ConfigValidation::applyPatchAndClamp(Config &cfg, const ConfigPatch &patch,
                                          ConfigValidationResult &result) {
  result = {};

  if (patch.hasDayStartHour) {
    markProvided(result);
    cfg.dayStartHour = static_cast<uint8_t>(constrain(patch.dayStartHour, 0, 24));
    markApplied(result);
  }
  if (patch.hasDayStartMinute) {
    markProvided(result);
    cfg.dayStartMinute =
        static_cast<uint8_t>(constrain(patch.dayStartMinute, 0, 59));
    markApplied(result);
  }
  if (patch.hasDayEndHour) {
    markProvided(result);
    cfg.dayEndHour = static_cast<uint8_t>(constrain(patch.dayEndHour, 0, 24));
    markApplied(result);
  }
  if (patch.hasDayEndMinute) {
    markProvided(result);
    cfg.dayEndMinute = static_cast<uint8_t>(constrain(patch.dayEndMinute, 0, 59));
    markApplied(result);
  }

  // 24:00 to specjalna wartosc sentinel (caly dzien / cala noc).
  // Dla godziny 24 wymuszamy minuty = 0 niezaleznie od interfejsu.
  if (cfg.dayStartHour == 24) {
    cfg.dayStartMinute = 0;
  }
  if (cfg.dayEndHour == 24) {
    cfg.dayEndMinute = 0;
  }

  if (patch.hasAerationHourOn) {
    markProvided(result);
    cfg.aerationHourOn =
        static_cast<uint8_t>(constrain(patch.aerationHourOn, 0, 23));
    markApplied(result);
  }
  if (patch.hasAerationMinuteOn) {
    markProvided(result);
    cfg.aerationMinuteOn =
        static_cast<uint8_t>(constrain(patch.aerationMinuteOn, 0, 59));
    markApplied(result);
  }
  if (patch.hasAerationHourOff) {
    markProvided(result);
    cfg.aerationHourOff =
        static_cast<uint8_t>(constrain(patch.aerationHourOff, 0, 23));
    markApplied(result);
  }
  if (patch.hasAerationMinuteOff) {
    markProvided(result);
    cfg.aerationMinuteOff =
        static_cast<uint8_t>(constrain(patch.aerationMinuteOff, 0, 59));
    markApplied(result);
  }

  if (patch.hasFilterHourOn) {
    markProvided(result);
    cfg.filterHourOn = static_cast<uint8_t>(constrain(patch.filterHourOn, 0, 23));
    markApplied(result);
  }
  if (patch.hasFilterMinuteOn) {
    markProvided(result);
    cfg.filterMinuteOn =
        static_cast<uint8_t>(constrain(patch.filterMinuteOn, 0, 59));
    markApplied(result);
  }
  if (patch.hasFilterHourOff) {
    markProvided(result);
    cfg.filterHourOff =
        static_cast<uint8_t>(constrain(patch.filterHourOff, 0, 23));
    markApplied(result);
  }
  if (patch.hasFilterMinuteOff) {
    markProvided(result);
    cfg.filterMinuteOff =
        static_cast<uint8_t>(constrain(patch.filterMinuteOff, 0, 59));
    markApplied(result);
  }

  if (patch.hasServoPreOffMins) {
    markProvided(result);
    cfg.servoPreOffMins =
        static_cast<uint8_t>(constrain(patch.servoPreOffMins, 0, 255));
    markApplied(result);
  }

  if (patch.hasTargetTemp) {
    markProvided(result);
    if (isnan(patch.targetTemp)) {
      markInvalid(result);
    } else {
      cfg.targetTemp = constrain(patch.targetTemp, 15.0f, 35.0f);
      markApplied(result);
    }
  }
  if (patch.hasTempHysteresis) {
    markProvided(result);
    if (isnan(patch.tempHysteresis)) {
      markInvalid(result);
    } else {
      cfg.tempHysteresis = constrain(patch.tempHysteresis, 0.1f, 5.0f);
      markApplied(result);
    }
  }

  if (patch.hasFeedMode) {
    markProvided(result);
    cfg.feedMode = static_cast<uint8_t>(constrain(patch.feedMode, 0, 3));
    markApplied(result);
  }
  if (patch.hasFeedHour) {
    markProvided(result);
    cfg.feedHour = static_cast<uint8_t>(constrain(patch.feedHour, 0, 23));
    markApplied(result);
  }
  if (patch.hasFeedMinute) {
    markProvided(result);
    cfg.feedMinute = static_cast<uint8_t>(constrain(patch.feedMinute, 0, 59));
    markApplied(result);
  }

  if (patch.hasAlwaysScreenOn) {
    markProvided(result);
    cfg.alwaysScreenOn = patch.alwaysScreenOn;
    markApplied(result);
  }

  if (patch.hasHeaterEnabled) {
    markProvided(result);
    cfg.heaterEnabled = patch.heaterEnabled;
    markApplied(result);
  }
}
