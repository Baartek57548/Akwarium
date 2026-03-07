#ifndef CONFIG_VALIDATION_H
#define CONFIG_VALIDATION_H

#include "ConfigData.h"
#include <Arduino.h>

struct ValidationProfileSnapshot {
  uint8_t minuteStep = 5;
  uint8_t minHour = 0;
  uint8_t maxHour = 23;
  uint8_t minTemperature = 18;
  uint8_t maxTemperature = 30;
  uint8_t temperatureStep = 1;
  uint16_t servoPreOffMin = 0;
  uint16_t servoPreOffMax = 255;
  float hysteresisMin = 0.1f;
  float hysteresisMax = 5.0f;
  float hysteresisStep = 0.1f;
  uint8_t feedModeMin = 0;
  uint8_t feedModeMax = 3;
};

struct ConfigPatch {
  bool hasLightMode = false;
  int lightMode = 0;
  bool hasDayStartHour = false;
  int dayStartHour = 0;
  bool hasDayStartMinute = false;
  int dayStartMinute = 0;
  bool hasDayEndHour = false;
  int dayEndHour = 0;
  bool hasDayEndMinute = false;
  int dayEndMinute = 0;

  bool hasAerationMode = false;
  int aerationMode = 0;
  bool hasAerationHourOn = false;
  int aerationHourOn = 0;
  bool hasAerationMinuteOn = false;
  int aerationMinuteOn = 0;
  bool hasAerationHourOff = false;
  int aerationHourOff = 0;
  bool hasAerationMinuteOff = false;
  int aerationMinuteOff = 0;

  bool hasFilterMode = false;
  int filterMode = 0;
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

  bool hasHeaterMode = false;
  int heaterMode = 0;
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
};

struct ConfigValidationResult {
  uint8_t providedFields = 0;
  uint8_t appliedFields = 0;
  uint8_t invalidFields = 0;
  char errorCode[40] = "";

  bool hasAnyProvided() const { return providedFields > 0; }
  bool hasAnyApplied() const { return appliedFields > 0; }
  bool hasInvalidFields() const { return invalidFields > 0; }
};

namespace ConfigValidation {
ValidationProfileSnapshot getValidationProfile();
bool isScheduleModeValue(int mode);
bool isHeaterModeValue(int mode);
bool isFeedModeValue(int feedMode);
bool isMinuteStepValid(int minute);
bool isScheduleTimeValid(int hour, int minute);
bool isTemperatureThresholdValid(float value);
bool isHysteresisValid(float value);
void sanitizeConfig(Config &cfg);
bool applyRuntimePatch(Config &cfg, const ConfigPatch &patch,
                       ConfigValidationResult &result);
}

#endif // CONFIG_VALIDATION_H
