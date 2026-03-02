#ifndef CONFIG_VALIDATION_H
#define CONFIG_VALIDATION_H

#include "ConfigData.h"
#include <Arduino.h>

struct ConfigPatch {
  bool hasDayStartHour = false;
  int dayStartHour = 0;
  bool hasDayStartMinute = false;
  int dayStartMinute = 0;
  bool hasDayEndHour = false;
  int dayEndHour = 0;
  bool hasDayEndMinute = false;
  int dayEndMinute = 0;

  bool hasAerationHourOn = false;
  int aerationHourOn = 0;
  bool hasAerationMinuteOn = false;
  int aerationMinuteOn = 0;
  bool hasAerationHourOff = false;
  int aerationHourOff = 0;
  bool hasAerationMinuteOff = false;
  int aerationMinuteOff = 0;

  bool hasFilterHourOn = false;
  int filterHourOn = 0;
  bool hasFilterMinuteOn = false;
  int filterMinuteOn = 0;
  bool hasFilterHourOff = false;
  int filterHourOff = 0;
  bool hasFilterMinuteOff = false;
  int filterMinuteOff = 0;

  bool hasServoPreOffMins = false;
  int servoPreOffMins = 0;

  bool hasTargetTemp = false;
  float targetTemp = 0.0f;
  bool hasTempHysteresis = false;
  float tempHysteresis = 0.0f;

  bool hasFeedMode = false;
  int feedMode = 0;
  bool hasFeedHour = false;
  int feedHour = 0;
  bool hasFeedMinute = false;
  int feedMinute = 0;

  bool hasAlwaysScreenOn = false;
  bool alwaysScreenOn = false;

  bool hasHeaterEnabled = false;
  bool heaterEnabled = true;
};

struct ConfigValidationResult {
  uint8_t providedFields = 0;
  uint8_t appliedFields = 0;
  uint8_t invalidFields = 0;

  bool hasAnyProvided() const { return providedFields > 0; }
  bool hasAnyApplied() const { return appliedFields > 0; }
  bool hasInvalidFields() const { return invalidFields > 0; }
};

namespace ConfigValidation {
void applyPatchAndClamp(Config &cfg, const ConfigPatch &patch,
                        ConfigValidationResult &result);
}

#endif // CONFIG_VALIDATION_H
