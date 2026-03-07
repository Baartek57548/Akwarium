#include "ConfigValidation.h"

#include <math.h>
#include <string.h>

namespace {

constexpr uint8_t MINUTE_STEP = 5;
constexpr uint8_t MIN_HOUR = 0;
constexpr uint8_t MAX_HOUR = 23;
constexpr uint8_t MIN_TEMPERATURE = 18;
constexpr uint8_t MAX_TEMPERATURE = 30;
constexpr float MIN_HYSTERESIS = 0.1f;
constexpr float MAX_HYSTERESIS = 5.0f;
constexpr float HYSTERESIS_STEP = 0.1f;
constexpr uint8_t DEFAULT_THRESHOLD = 25;
constexpr float DEFAULT_HYSTERESIS = 0.5f;

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

static void markInvalid(ConfigValidationResult &result, const char *errorCode) {
  if (result.invalidFields < 255) {
    result.invalidFields++;
  }

  if (result.errorCode[0] == '\0' && errorCode != nullptr &&
      errorCode[0] != '\0') {
    snprintf(result.errorCode, sizeof(result.errorCode), "%s", errorCode);
  }
}

static uint8_t snapMinuteToStep(int minute) {
  const int bounded = constrain(minute, 0, 59);
  const int snapped = ((bounded + (MINUTE_STEP / 2)) / MINUTE_STEP) * MINUTE_STEP;
  return static_cast<uint8_t>(min(snapped, 55));
}

static float snapHysteresis(float value) {
  const float bounded = constrain(value, MIN_HYSTERESIS, MAX_HYSTERESIS);
  const float stepped = roundf(bounded / HYSTERESIS_STEP) * HYSTERESIS_STEP;
  return constrain(stepped, MIN_HYSTERESIS, MAX_HYSTERESIS);
}

static uint8_t snapTemperature(float value) {
  const int rounded = static_cast<int>(lroundf(value));
  return static_cast<uint8_t>(constrain(rounded, MIN_TEMPERATURE, MAX_TEMPERATURE));
}

static bool isIntegralStep(float value, float step) {
  if (!isfinite(value) || step <= 0.0f) {
    return false;
  }

  const float units = value / step;
  return fabsf(units - roundf(units)) <= 0.001f;
}

static bool isScheduleFieldPresent(const ConfigPatch &patch) {
  return patch.hasDayStartHour || patch.hasDayStartMinute || patch.hasDayEndHour ||
         patch.hasDayEndMinute || patch.hasAerationHourOn ||
         patch.hasAerationMinuteOn || patch.hasAerationHourOff ||
         patch.hasAerationMinuteOff || patch.hasFilterHourOn ||
         patch.hasFilterMinuteOn || patch.hasFilterHourOff ||
         patch.hasFilterMinuteOff;
}

static bool hasLightTimePatch(const ConfigPatch &patch) {
  return patch.hasDayStartHour || patch.hasDayStartMinute || patch.hasDayEndHour ||
         patch.hasDayEndMinute;
}

static bool hasAerationTimePatch(const ConfigPatch &patch) {
  return patch.hasAerationHourOn || patch.hasAerationMinuteOn ||
         patch.hasAerationHourOff || patch.hasAerationMinuteOff;
}

static bool hasFilterTimePatch(const ConfigPatch &patch) {
  return patch.hasFilterHourOn || patch.hasFilterMinuteOn ||
         patch.hasFilterHourOff || patch.hasFilterMinuteOff;
}

static void sanitizeScheduleWindow(uint8_t &hourOn, uint8_t &minuteOn,
                                   uint8_t &hourOff, uint8_t &minuteOff) {
  hourOn = static_cast<uint8_t>(constrain(hourOn, MIN_HOUR, MAX_HOUR));
  minuteOn = snapMinuteToStep(minuteOn);
  hourOff = static_cast<uint8_t>(constrain(hourOff, MIN_HOUR, MAX_HOUR));
  minuteOff = snapMinuteToStep(minuteOff);
}

static uint8_t sanitizeScheduleMode(uint8_t rawMode) {
  switch (rawMode) {
  case static_cast<uint8_t>(ScheduleMode::Schedule):
  case static_cast<uint8_t>(ScheduleMode::AlwaysOn):
  case static_cast<uint8_t>(ScheduleMode::AlwaysOff):
    return rawMode;
  default:
    return static_cast<uint8_t>(ScheduleMode::Schedule);
  }
}

static uint8_t sanitizeHeaterMode(uint8_t rawMode) {
  switch (rawMode) {
  case static_cast<uint8_t>(HeaterMode::Threshold):
  case static_cast<uint8_t>(HeaterMode::Off):
    return rawMode;
  default:
    return static_cast<uint8_t>(HeaterMode::Threshold);
  }
}

static uint8_t deriveLegacyLightMode(const Config &cfg) {
  if (cfg.dayStartHour == 24) {
    return static_cast<uint8_t>(ScheduleMode::AlwaysOn);
  }

  if (cfg.dayEndHour == 24) {
    return static_cast<uint8_t>(ScheduleMode::AlwaysOff);
  }

  return static_cast<uint8_t>(ScheduleMode::Schedule);
}

static void copyCandidateIfValid(Config &cfg, const Config &candidate,
                                 ConfigValidationResult &result) {
  if (!result.hasInvalidFields() && result.hasAnyProvided()) {
    cfg = candidate;
    result.appliedFields = result.providedFields;
  }
}

} // namespace

ValidationProfileSnapshot ConfigValidation::getValidationProfile() {
  ValidationProfileSnapshot profile;
  profile.minuteStep = MINUTE_STEP;
  profile.minHour = MIN_HOUR;
  profile.maxHour = MAX_HOUR;
  profile.minTemperature = MIN_TEMPERATURE;
  profile.maxTemperature = MAX_TEMPERATURE;
  profile.temperatureStep = 1;
  profile.hysteresisMin = MIN_HYSTERESIS;
  profile.hysteresisMax = MAX_HYSTERESIS;
  profile.hysteresisStep = HYSTERESIS_STEP;
  profile.feedModeMin = 0;
  profile.feedModeMax = 3;
  profile.servoPreOffMin = 0;
  profile.servoPreOffMax = 255;
  return profile;
}

bool ConfigValidation::isScheduleModeValue(int mode) {
  return mode == static_cast<int>(ScheduleMode::Schedule) ||
         mode == static_cast<int>(ScheduleMode::AlwaysOn) ||
         mode == static_cast<int>(ScheduleMode::AlwaysOff);
}

bool ConfigValidation::isHeaterModeValue(int mode) {
  return mode == static_cast<int>(HeaterMode::Threshold) ||
         mode == static_cast<int>(HeaterMode::Off);
}

bool ConfigValidation::isFeedModeValue(int feedMode) {
  return feedMode >= 0 && feedMode <= 3;
}

bool ConfigValidation::isMinuteStepValid(int minute) {
  return minute >= 0 && minute <= 59 && (minute % MINUTE_STEP) == 0;
}

bool ConfigValidation::isScheduleTimeValid(int hour, int minute) {
  return hour >= MIN_HOUR && hour <= MAX_HOUR && isMinuteStepValid(minute);
}

bool ConfigValidation::isTemperatureThresholdValid(float value) {
  return isfinite(value) && isIntegralStep(value, 1.0f) &&
         value >= MIN_TEMPERATURE && value <= MAX_TEMPERATURE;
}

bool ConfigValidation::isHysteresisValid(float value) {
  return isfinite(value) && value >= MIN_HYSTERESIS && value <= MAX_HYSTERESIS &&
         isIntegralStep(value, HYSTERESIS_STEP);
}

void ConfigValidation::sanitizeConfig(Config &cfg) {
  const uint8_t legacyLightMode = deriveLegacyLightMode(cfg);

  cfg.lightMode = sanitizeScheduleMode(cfg.lightMode);
  cfg.aerationMode = sanitizeScheduleMode(cfg.aerationMode);
  cfg.filterMode = sanitizeScheduleMode(cfg.filterMode);
  cfg.heaterMode = sanitizeHeaterMode(cfg.heaterMode);

  if (cfg.version < CONFIG_VERSION) {
    cfg.lightMode = legacyLightMode;
  }

  sanitizeScheduleWindow(cfg.dayStartHour, cfg.dayStartMinute, cfg.dayEndHour,
                         cfg.dayEndMinute);
  sanitizeScheduleWindow(cfg.aerationHourOn, cfg.aerationMinuteOn,
                         cfg.aerationHourOff, cfg.aerationMinuteOff);
  sanitizeScheduleWindow(cfg.filterHourOn, cfg.filterMinuteOn, cfg.filterHourOff,
                         cfg.filterMinuteOff);

  cfg.servoPreOffMins =
      static_cast<uint8_t>(constrain(cfg.servoPreOffMins, 0, 255));

  if (!isfinite(cfg.targetTemp) || cfg.targetTemp <= 0.0f) {
    cfg.heaterMode = static_cast<uint8_t>(HeaterMode::Off);
    cfg.targetTemp = DEFAULT_THRESHOLD;
  } else {
    cfg.targetTemp = snapTemperature(cfg.targetTemp);
  }

  if (!isfinite(cfg.tempHysteresis)) {
    cfg.tempHysteresis = DEFAULT_HYSTERESIS;
  } else {
    cfg.tempHysteresis = snapHysteresis(cfg.tempHysteresis);
  }

  cfg.servoDayAngle =
      constrain(cfg.servoDayAngle, SERVO_OPEN_ANGLE, SERVO_CLOSED_ANGLE);
  cfg.servoNightAngle =
      constrain(cfg.servoNightAngle, SERVO_OPEN_ANGLE, SERVO_CLOSED_ANGLE);
  cfg.servoAlarmAngle =
      constrain(cfg.servoAlarmAngle, SERVO_OPEN_ANGLE, SERVO_CLOSED_ANGLE);

  cfg.feedMode = static_cast<uint8_t>(constrain(cfg.feedMode, 0, 3));
  cfg.feedHour = static_cast<uint8_t>(constrain(cfg.feedHour, MIN_HOUR, MAX_HOUR));
  cfg.feedMinute = snapMinuteToStep(cfg.feedMinute);
}

bool ConfigValidation::applyRuntimePatch(Config &cfg, const ConfigPatch &patch,
                                         ConfigValidationResult &result) {
  result = {};
  Config candidate = cfg;

  if (patch.hasLightMode) {
    markProvided(result);
    if (!isScheduleModeValue(patch.lightMode)) {
      markInvalid(result, "invalid_light_mode");
    } else {
      candidate.lightMode = static_cast<uint8_t>(patch.lightMode);
    }
  }

  if (patch.hasAerationMode) {
    markProvided(result);
    if (!isScheduleModeValue(patch.aerationMode)) {
      markInvalid(result, "invalid_aeration_mode");
    } else {
      candidate.aerationMode = static_cast<uint8_t>(patch.aerationMode);
    }
  }

  if (patch.hasFilterMode) {
    markProvided(result);
    if (!isScheduleModeValue(patch.filterMode)) {
      markInvalid(result, "invalid_filter_mode");
    } else {
      candidate.filterMode = static_cast<uint8_t>(patch.filterMode);
    }
  }

  if (patch.hasHeaterMode) {
    markProvided(result);
    if (!isHeaterModeValue(patch.heaterMode)) {
      markInvalid(result, "invalid_heater_mode");
    } else {
      candidate.heaterMode = static_cast<uint8_t>(patch.heaterMode);
    }
  }

  if (hasLightTimePatch(patch) &&
      candidate.lightMode != static_cast<uint8_t>(ScheduleMode::Schedule)) {
    if (patch.hasDayStartHour || patch.hasDayStartMinute || patch.hasDayEndHour ||
        patch.hasDayEndMinute) {
      markProvided(result);
      markInvalid(result, "light_time_requires_schedule");
    }
  } else {
    if (patch.hasDayStartHour) {
      markProvided(result);
      if (patch.dayStartHour < MIN_HOUR || patch.dayStartHour > MAX_HOUR) {
        markInvalid(result, "invalid_light_start");
      } else {
        candidate.dayStartHour = static_cast<uint8_t>(patch.dayStartHour);
      }
    }
    if (patch.hasDayStartMinute) {
      markProvided(result);
      if (!isMinuteStepValid(patch.dayStartMinute)) {
        markInvalid(result, "invalid_light_start");
      } else {
        candidate.dayStartMinute = static_cast<uint8_t>(patch.dayStartMinute);
      }
    }
    if (patch.hasDayEndHour) {
      markProvided(result);
      if (patch.dayEndHour < MIN_HOUR || patch.dayEndHour > MAX_HOUR) {
        markInvalid(result, "invalid_light_end");
      } else {
        candidate.dayEndHour = static_cast<uint8_t>(patch.dayEndHour);
      }
    }
    if (patch.hasDayEndMinute) {
      markProvided(result);
      if (!isMinuteStepValid(patch.dayEndMinute)) {
        markInvalid(result, "invalid_light_end");
      } else {
        candidate.dayEndMinute = static_cast<uint8_t>(patch.dayEndMinute);
      }
    }
  }

  if (hasAerationTimePatch(patch) &&
      candidate.aerationMode != static_cast<uint8_t>(ScheduleMode::Schedule)) {
    if (patch.hasAerationHourOn || patch.hasAerationMinuteOn ||
        patch.hasAerationHourOff || patch.hasAerationMinuteOff) {
      markProvided(result);
      markInvalid(result, "aeration_time_requires_schedule");
    }
  } else {
    if (patch.hasAerationHourOn) {
      markProvided(result);
      if (patch.aerationHourOn < MIN_HOUR || patch.aerationHourOn > MAX_HOUR) {
        markInvalid(result, "invalid_aeration_start");
      } else {
        candidate.aerationHourOn = static_cast<uint8_t>(patch.aerationHourOn);
      }
    }
    if (patch.hasAerationMinuteOn) {
      markProvided(result);
      if (!isMinuteStepValid(patch.aerationMinuteOn)) {
        markInvalid(result, "invalid_aeration_start");
      } else {
        candidate.aerationMinuteOn =
            static_cast<uint8_t>(patch.aerationMinuteOn);
      }
    }
    if (patch.hasAerationHourOff) {
      markProvided(result);
      if (patch.aerationHourOff < MIN_HOUR ||
          patch.aerationHourOff > MAX_HOUR) {
        markInvalid(result, "invalid_aeration_end");
      } else {
        candidate.aerationHourOff = static_cast<uint8_t>(patch.aerationHourOff);
      }
    }
    if (patch.hasAerationMinuteOff) {
      markProvided(result);
      if (!isMinuteStepValid(patch.aerationMinuteOff)) {
        markInvalid(result, "invalid_aeration_end");
      } else {
        candidate.aerationMinuteOff =
            static_cast<uint8_t>(patch.aerationMinuteOff);
      }
    }
  }

  if (hasFilterTimePatch(patch) &&
      candidate.filterMode != static_cast<uint8_t>(ScheduleMode::Schedule)) {
    if (patch.hasFilterHourOn || patch.hasFilterMinuteOn ||
        patch.hasFilterHourOff || patch.hasFilterMinuteOff) {
      markProvided(result);
      markInvalid(result, "filter_time_requires_schedule");
    }
  } else {
    if (patch.hasFilterHourOn) {
      markProvided(result);
      if (patch.filterHourOn < MIN_HOUR || patch.filterHourOn > MAX_HOUR) {
        markInvalid(result, "invalid_filter_start");
      } else {
        candidate.filterHourOn = static_cast<uint8_t>(patch.filterHourOn);
      }
    }
    if (patch.hasFilterMinuteOn) {
      markProvided(result);
      if (!isMinuteStepValid(patch.filterMinuteOn)) {
        markInvalid(result, "invalid_filter_start");
      } else {
        candidate.filterMinuteOn = static_cast<uint8_t>(patch.filterMinuteOn);
      }
    }
    if (patch.hasFilterHourOff) {
      markProvided(result);
      if (patch.filterHourOff < MIN_HOUR || patch.filterHourOff > MAX_HOUR) {
        markInvalid(result, "invalid_filter_end");
      } else {
        candidate.filterHourOff = static_cast<uint8_t>(patch.filterHourOff);
      }
    }
    if (patch.hasFilterMinuteOff) {
      markProvided(result);
      if (!isMinuteStepValid(patch.filterMinuteOff)) {
        markInvalid(result, "invalid_filter_end");
      } else {
        candidate.filterMinuteOff = static_cast<uint8_t>(patch.filterMinuteOff);
      }
    }
  }

  if (patch.hasServoPreOffMins) {
    markProvided(result);
    if (patch.servoPreOffMins < 0 || patch.servoPreOffMins > 255) {
      markInvalid(result, "invalid_servo_preoff");
    } else {
      candidate.servoPreOffMins = static_cast<uint8_t>(patch.servoPreOffMins);
    }
  }

  if (patch.hasTargetTemp) {
    markProvided(result);
    if (!isTemperatureThresholdValid(patch.targetTemp)) {
      markInvalid(result, "invalid_target_temp");
    } else {
      candidate.targetTemp = patch.targetTemp;
    }
  }

  if (patch.hasTempHysteresis) {
    markProvided(result);
    if (!isHysteresisValid(patch.tempHysteresis)) {
      markInvalid(result, "invalid_hysteresis");
    } else {
      candidate.tempHysteresis = patch.tempHysteresis;
    }
  }

  if (patch.hasFeedMode) {
    markProvided(result);
    if (!isFeedModeValue(patch.feedMode)) {
      markInvalid(result, "invalid_feed_mode");
    } else {
      candidate.feedMode = static_cast<uint8_t>(patch.feedMode);
    }
  }

  if (patch.hasFeedHour) {
    markProvided(result);
    if (patch.feedHour < MIN_HOUR || patch.feedHour > MAX_HOUR) {
      markInvalid(result, "invalid_feed_time");
    } else {
      candidate.feedHour = static_cast<uint8_t>(patch.feedHour);
    }
  }

  if (patch.hasFeedMinute) {
    markProvided(result);
    if (!isMinuteStepValid(patch.feedMinute)) {
      markInvalid(result, "invalid_feed_time");
    } else {
      candidate.feedMinute = static_cast<uint8_t>(patch.feedMinute);
    }
  }

  copyCandidateIfValid(cfg, candidate, result);
  return result.hasAnyApplied();
}
