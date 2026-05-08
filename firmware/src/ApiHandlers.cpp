#include "ApiHandlers.h"

#include "AkwariumWifi.h"
#include "ConfigManager.h"
#include "ConfigService.h"
#include "ConfigValidation.h"
#include "FirmwareInfo.h"
#include "LogManager.h"
#include "PowerManager.h"
#include "SharedState.h"
#include "SystemController.h"
#include "WebApiProtocol.h"

#include <ArduinoJson.h>
#include <Arduino.h>
#include <WebServer.h>
#include <cstdlib>
#include <cstring>

namespace {

static bool parseLongStrict(const String &raw, long &out) {
  if (raw.length() == 0) {
    return false;
  }

  char *endPtr = nullptr;
  long parsed = strtol(raw.c_str(), &endPtr, 10);
  if (endPtr == raw.c_str() || *endPtr != '\0') {
    return false;
  }

  out = parsed;
  return true;
}

static bool parseFloatStrict(const String &raw, float &out) {
  if (raw.length() == 0) {
    return false;
  }

  char *endPtr = nullptr;
  float parsed = strtof(raw.c_str(), &endPtr);
  if (endPtr == raw.c_str() || *endPtr != '\0') {
    return false;
  }

  out = parsed;
  return true;
}

static bool parseTimeArg(const String &value, int &hour, int &minute) {
  if (value.length() < 5 || value[2] != ':') {
    return false;
  }

  if (!isDigit(value[0]) || !isDigit(value[1]) || !isDigit(value[3]) ||
      !isDigit(value[4])) {
    return false;
  }

  hour = value.substring(0, 2).toInt();
  minute = value.substring(3, 5).toInt();
  return true;
}

static bool parseBoolArg(const String &raw, bool &out) {
  if (raw.length() == 0) {
    return false;
  }

  if (raw == "1" || raw.equalsIgnoreCase("true") || raw.equalsIgnoreCase("on")) {
    out = true;
    return true;
  }

  if (raw == "0" || raw.equalsIgnoreCase("false") || raw.equalsIgnoreCase("off")) {
    out = false;
    return true;
  }

  return false;
}

static bool parseModeArg(WebServer &server, const char *name,
                         bool &hasValue, int &outValue) {
  hasValue = false;
  if (!server.hasArg(name)) {
    return true;
  }

  long parsed = 0;
  if (!parseLongStrict(server.arg(name), parsed)) {
    return false;
  }

  hasValue = true;
  outValue = static_cast<int>(parsed);
  return true;
}

static void sendValidationError(WebServer &server, const char *code) {
  sendWebActionResponse(server, 400, false,
                        (code != nullptr && code[0] != '\0')
                            ? code
                            : "invalid_values");
}

static void sendConfigApplyResult(WebServer &server,
                                  const ConfigApplyResult &applyResult,
                                  bool forcePartial = false) {
  if (!applyResult.ok) {
    if (applyResult.status == ConfigApplyStatus::ValidationFailed) {
      sendValidationError(server, applyResult.responseCode);
      return;
    }

    sendWebActionResponse(server, 500, false, "save_failed");
    return;
  }

  requestUiSaveConfirmationAnimation();
  sendWebActionResponse(server, 200, true,
                        forcePartial ? "settings_partial"
                                     : applyResult.responseCode);
}

static const char *feederErrorCode(Error err) {
  switch (err) {
  case Error::NONE:
    return "feed_started";
  case Error::BUSY:
    return "feed_busy";
  case Error::SENSOR_NOT_OK:
    return "feed_sensor_not_ok";
  case Error::TIMEOUT:
    return "feed_timeout";
  default:
    return "feed_failed";
  }
}

static int feederErrorHttpStatus(Error err) {
  switch (err) {
  case Error::NONE:
    return 200;
  case Error::BUSY:
    return 409;
  case Error::SENSOR_NOT_OK:
    return 422;
  case Error::TIMEOUT:
    return 504;
  default:
    return 500;
  }
}

static const char *timeSyncCommandCode(TimeSyncCommandResult result) {
  switch (result) {
  case TimeSyncCommandResult::Ok:
    return "time_sync_ok";
  case TimeSyncCommandResult::Busy:
    return "time_sync_busy";
  case TimeSyncCommandResult::StaUnavailable:
    return "time_sync_sta_unavailable";
  case TimeSyncCommandResult::WifiConnectFailed:
    return "time_sync_wifi_failed";
  case TimeSyncCommandResult::SyncFailed:
    return "time_sync_failed";
  default:
    return "time_sync_failed";
  }
}

static int timeSyncCommandHttpStatus(TimeSyncCommandResult result) {
  switch (result) {
  case TimeSyncCommandResult::Ok:
    return 200;
  case TimeSyncCommandResult::Busy:
    return 409;
  case TimeSyncCommandResult::StaUnavailable:
    return 409;
  case TimeSyncCommandResult::WifiConnectFailed:
    return 502;
  case TimeSyncCommandResult::SyncFailed:
    return 504;
  default:
    return 500;
  }
}

static bool readJsonInt(JsonVariantConst value, int &out) {
  if (value.isNull()) {
    return false;
  }
  if (value.is<int>() || value.is<long>()) {
    out = value.as<int>();
    return true;
  }
  if (value.is<const char *>()) {
    long parsed = 0;
    if (!parseLongStrict(String(value.as<const char *>()), parsed)) {
      return false;
    }
    out = static_cast<int>(parsed);
    return true;
  }
  return false;
}

static bool readJsonFloat(JsonVariantConst value, float &out) {
  if (value.isNull()) {
    return false;
  }
  if (value.is<float>() || value.is<double>() || value.is<int>() ||
      value.is<long>()) {
    out = value.as<float>();
    return true;
  }
  if (value.is<const char *>()) {
    return parseFloatStrict(String(value.as<const char *>()), out);
  }
  return false;
}

static bool readJsonBool(JsonVariantConst value, bool &out) {
  if (value.isNull()) {
    return false;
  }
  if (value.is<bool>()) {
    out = value.as<bool>();
    return true;
  }
  if (value.is<int>() || value.is<long>()) {
    out = value.as<int>() != 0;
    return true;
  }
  if (value.is<const char *>()) {
    return parseBoolArg(String(value.as<const char *>()), out);
  }
  return false;
}

static bool readScheduleMode(JsonVariantConst value, int &out) {
  if (readJsonInt(value, out)) {
    return true;
  }

  if (!value.is<const char *>()) {
    return false;
  }

  const String raw = String(value.as<const char *>());
  if (raw.equalsIgnoreCase("schedule") ||
      raw.equalsIgnoreCase("harmonogram")) {
    out = static_cast<int>(ScheduleMode::Schedule);
    return true;
  }
  if (raw.equalsIgnoreCase("always_on") || raw.equalsIgnoreCase("on") ||
      raw.equalsIgnoreCase("zawsze_wlaczone")) {
    out = static_cast<int>(ScheduleMode::AlwaysOn);
    return true;
  }
  if (raw.equalsIgnoreCase("always_off") || raw.equalsIgnoreCase("off") ||
      raw.equalsIgnoreCase("zawsze_wylaczone")) {
    out = static_cast<int>(ScheduleMode::AlwaysOff);
    return true;
  }

  return false;
}

static bool readFeedMode(JsonVariantConst value, int &out) {
  if (readJsonInt(value, out)) {
    return true;
  }

  if (!value.is<const char *>()) {
    return false;
  }

  const String raw = String(value.as<const char *>());
  if (raw.equalsIgnoreCase("off") || raw.equalsIgnoreCase("wylaczone")) {
    out = 0;
    return true;
  }
  if (raw.equalsIgnoreCase("daily") || raw.equalsIgnoreCase("codziennie")) {
    out = 1;
    return true;
  }
  if (raw.equalsIgnoreCase("every_2_days") || raw.equalsIgnoreCase("co_2_dni")) {
    out = 2;
    return true;
  }
  if (raw.equalsIgnoreCase("every_3_days") || raw.equalsIgnoreCase("co_3_dni")) {
    out = 3;
    return true;
  }

  return false;
}

static bool readJsonTime(JsonVariantConst value, int &hour, int &minute) {
  if (value.is<const char *>()) {
    return parseTimeArg(String(value.as<const char *>()), hour, minute);
  }

  if (!value.is<JsonObjectConst>()) {
    return false;
  }

  JsonObjectConst obj = value.as<JsonObjectConst>();
  JsonVariantConst hourValue = obj["hour"];
  if (hourValue.isNull()) {
    hourValue = obj["h"];
  }
  JsonVariantConst minuteValue = obj["minute"];
  if (minuteValue.isNull()) {
    minuteValue = obj["min"];
  }

  return readJsonInt(hourValue, hour) && readJsonInt(minuteValue, minute);
}

static bool hasAnyRuntimePatchField(const ConfigPatch &patch) {
  return patch.hasLightMode || patch.hasDayStartHour ||
         patch.hasDayStartMinute || patch.hasDayEndHour ||
         patch.hasDayEndMinute || patch.hasAerationMode ||
         patch.hasAerationHourOn || patch.hasAerationMinuteOn ||
         patch.hasAerationHourOff || patch.hasAerationMinuteOff ||
         patch.hasFilterMode || patch.hasFilterHourOn ||
         patch.hasFilterMinuteOn || patch.hasFilterHourOff ||
         patch.hasFilterMinuteOff || patch.hasServoPreOffMins ||
         patch.hasHeaterMode || patch.hasTargetTemp ||
         patch.hasTempHysteresis || patch.hasFeedMode || patch.hasFeedHour ||
         patch.hasFeedMinute;
}

static void handlePatchConfig(WebServer &server) {
  if (!server.hasArg("plain")) {
    sendWebActionResponse(server, 400, false, "missing_json");
    return;
  }

  PowerManager::registerActivity();

  DynamicJsonDocument doc(4096);
  const DeserializationError jsonError =
      deserializeJson(doc, server.arg("plain"));
  if (jsonError) {
    sendWebActionResponse(server, 400, false, "invalid_json",
                          jsonError.c_str());
    return;
  }

  JsonObjectConst root = doc.as<JsonObjectConst>();
  if (root.isNull()) {
    sendWebActionResponse(server, 400, false, "invalid_json_object");
    return;
  }

  const Config current = ConfigManager::getCopy();
  Config candidate = current;
  ConfigPatch patch = {};
  bool hasAnyField = false;
  char firstParseError[40] = "";

  auto markParseError = [&](const char *code) {
    if (firstParseError[0] == '\0') {
      snprintf(firstParseError, sizeof(firstParseError), "%s",
               code != nullptr ? code : "invalid_values");
    }
  };

  JsonObjectConst schedule = root["schedule"].as<JsonObjectConst>();
  JsonObjectConst light = schedule["light"].as<JsonObjectConst>();
  JsonObjectConst aerationSchedule = schedule["aeration"].as<JsonObjectConst>();
  JsonObjectConst filter = schedule["filter"].as<JsonObjectConst>();
  JsonObjectConst feedingSchedule = schedule["feeding"].as<JsonObjectConst>();
  JsonObjectConst temperature = root["temperature"].as<JsonObjectConst>();
  JsonObjectConst aeration = root["aeration"].as<JsonObjectConst>();
  JsonObjectConst feeding = root["feeding"].as<JsonObjectConst>();
  JsonObjectConst network = root["network"].as<JsonObjectConst>();
  JsonObjectConst displaySettings = root["display"].as<JsonObjectConst>();

  auto assignScheduleMode = [&](JsonVariantConst value,
                                bool ConfigPatch::*hasField,
                                int ConfigPatch::*field,
                                const char *errorCode) {
    if (value.isNull()) {
      return;
    }

    int parsed = 0;
    if (!readScheduleMode(value, parsed)) {
      markParseError(errorCode);
      return;
    }

    patch.*hasField = true;
    patch.*field = parsed;
    hasAnyField = true;
  };

  JsonVariantConst value = root["lightMode"];
  if (value.isNull()) {
    value = schedule["lightMode"];
  }
  if (value.isNull()) {
    value = light["mode"];
  }
  assignScheduleMode(value, &ConfigPatch::hasLightMode,
                     &ConfigPatch::lightMode, "invalid_light_mode");

  value = root["aerationMode"];
  if (value.isNull()) {
    value = schedule["airMode"];
  }
  if (value.isNull()) {
    value = schedule["aerationMode"];
  }
  if (value.isNull()) {
    value = aerationSchedule["mode"];
  }
  assignScheduleMode(value, &ConfigPatch::hasAerationMode,
                     &ConfigPatch::aerationMode, "invalid_aeration_mode");

  value = root["filterMode"];
  if (value.isNull()) {
    value = schedule["filterMode"];
  }
  if (value.isNull()) {
    value = filter["mode"];
  }
  assignScheduleMode(value, &ConfigPatch::hasFilterMode,
                     &ConfigPatch::filterMode, "invalid_filter_mode");

  auto assignTimePair = [&](JsonVariantConst combined,
                            JsonVariantConst hourValue,
                            JsonVariantConst minuteValue,
                            uint8_t effectiveMode,
                            bool ConfigPatch::*hasHour,
                            int ConfigPatch::*hourField,
                            bool ConfigPatch::*hasMinute,
                            int ConfigPatch::*minuteField,
                            const char *errorCode) {
    if (combined.isNull() && hourValue.isNull() && minuteValue.isNull()) {
      return;
    }

    hasAnyField = true;
    if (effectiveMode != static_cast<uint8_t>(ScheduleMode::Schedule)) {
      return;
    }

    if (!combined.isNull()) {
      int hour = 0;
      int minute = 0;
      if (!readJsonTime(combined, hour, minute)) {
        markParseError(errorCode);
        return;
      }
      patch.*hasHour = true;
      patch.*hourField = hour;
      patch.*hasMinute = true;
      patch.*minuteField = minute;
      return;
    }

    if (!hourValue.isNull()) {
      int hour = 0;
      if (!readJsonInt(hourValue, hour)) {
        markParseError(errorCode);
      } else {
        patch.*hasHour = true;
        patch.*hourField = hour;
      }
    }
    if (!minuteValue.isNull()) {
      int minute = 0;
      if (!readJsonInt(minuteValue, minute)) {
        markParseError(errorCode);
      } else {
        patch.*hasMinute = true;
        patch.*minuteField = minute;
      }
    }
  };

  const uint8_t effectiveLightMode =
      patch.hasLightMode ? static_cast<uint8_t>(patch.lightMode)
                         : current.lightMode;
  const uint8_t effectiveAerationMode =
      patch.hasAerationMode ? static_cast<uint8_t>(patch.aerationMode)
                            : current.aerationMode;
  const uint8_t effectiveFilterMode =
      patch.hasFilterMode ? static_cast<uint8_t>(patch.filterMode)
                          : current.filterMode;

  JsonVariantConst hourValue = root["dayStartHour"];
  if (hourValue.isNull()) {
    hourValue = schedule["dayStartHour"];
  }
  JsonVariantConst minuteValue = root["dayStartMinute"];
  if (minuteValue.isNull()) {
    minuteValue = root["dayStartMin"];
  }
  if (minuteValue.isNull()) {
    minuteValue = schedule["dayStartMin"];
  }
  if (minuteValue.isNull()) {
    minuteValue = schedule["dayStartMinute"];
  }
  assignTimePair(root["dayStart"], hourValue, minuteValue,
                 effectiveLightMode, &ConfigPatch::hasDayStartHour,
                 &ConfigPatch::dayStartHour,
                 &ConfigPatch::hasDayStartMinute,
                 &ConfigPatch::dayStartMinute, "invalid_light_start");
  assignTimePair(light["start"], light["startHour"], light["startMinute"],
                 effectiveLightMode, &ConfigPatch::hasDayStartHour,
                 &ConfigPatch::dayStartHour,
                 &ConfigPatch::hasDayStartMinute,
                 &ConfigPatch::dayStartMinute, "invalid_light_start");

  hourValue = root["dayEndHour"];
  if (hourValue.isNull()) {
    hourValue = schedule["dayEndHour"];
  }
  minuteValue = root["dayEndMinute"];
  if (minuteValue.isNull()) {
    minuteValue = root["dayEndMin"];
  }
  if (minuteValue.isNull()) {
    minuteValue = schedule["dayEndMin"];
  }
  if (minuteValue.isNull()) {
    minuteValue = schedule["dayEndMinute"];
  }
  assignTimePair(root["dayEnd"], hourValue, minuteValue,
                 effectiveLightMode, &ConfigPatch::hasDayEndHour,
                 &ConfigPatch::dayEndHour, &ConfigPatch::hasDayEndMinute,
                 &ConfigPatch::dayEndMinute, "invalid_light_end");
  assignTimePair(light["end"], light["endHour"], light["endMinute"],
                 effectiveLightMode, &ConfigPatch::hasDayEndHour,
                 &ConfigPatch::dayEndHour, &ConfigPatch::hasDayEndMinute,
                 &ConfigPatch::dayEndMinute, "invalid_light_end");

  hourValue = root["aerationHourOn"];
  if (hourValue.isNull()) {
    hourValue = root["airStartHour"];
  }
  if (hourValue.isNull()) {
    hourValue = schedule["airStartHour"];
  }
  if (hourValue.isNull()) {
    hourValue = schedule["aerationHourOn"];
  }
  minuteValue = root["aerationMinuteOn"];
  if (minuteValue.isNull()) {
    minuteValue = root["airStartMin"];
  }
  if (minuteValue.isNull()) {
    minuteValue = schedule["airStartMin"];
  }
  if (minuteValue.isNull()) {
    minuteValue = schedule["aerationMinuteOn"];
  }
  assignTimePair(root["airOn"], hourValue, minuteValue,
                 effectiveAerationMode, &ConfigPatch::hasAerationHourOn,
                 &ConfigPatch::aerationHourOn,
                 &ConfigPatch::hasAerationMinuteOn,
                 &ConfigPatch::aerationMinuteOn, "invalid_aeration_start");
  assignTimePair(aerationSchedule["start"], aerationSchedule["startHour"],
                 aerationSchedule["startMinute"], effectiveAerationMode,
                 &ConfigPatch::hasAerationHourOn,
                 &ConfigPatch::aerationHourOn,
                 &ConfigPatch::hasAerationMinuteOn,
                 &ConfigPatch::aerationMinuteOn, "invalid_aeration_start");

  hourValue = root["aerationHourOff"];
  if (hourValue.isNull()) {
    hourValue = root["airEndHour"];
  }
  if (hourValue.isNull()) {
    hourValue = schedule["airEndHour"];
  }
  if (hourValue.isNull()) {
    hourValue = schedule["aerationHourOff"];
  }
  minuteValue = root["aerationMinuteOff"];
  if (minuteValue.isNull()) {
    minuteValue = root["airEndMin"];
  }
  if (minuteValue.isNull()) {
    minuteValue = schedule["airEndMin"];
  }
  if (minuteValue.isNull()) {
    minuteValue = schedule["aerationMinuteOff"];
  }
  assignTimePair(root["airOff"], hourValue, minuteValue,
                 effectiveAerationMode, &ConfigPatch::hasAerationHourOff,
                 &ConfigPatch::aerationHourOff,
                 &ConfigPatch::hasAerationMinuteOff,
                 &ConfigPatch::aerationMinuteOff, "invalid_aeration_end");
  assignTimePair(aerationSchedule["end"], aerationSchedule["endHour"],
                 aerationSchedule["endMinute"], effectiveAerationMode,
                 &ConfigPatch::hasAerationHourOff,
                 &ConfigPatch::aerationHourOff,
                 &ConfigPatch::hasAerationMinuteOff,
                 &ConfigPatch::aerationMinuteOff, "invalid_aeration_end");

  hourValue = root["filterHourOn"];
  if (hourValue.isNull()) {
    hourValue = root["filterStartHour"];
  }
  if (hourValue.isNull()) {
    hourValue = schedule["filterStartHour"];
  }
  if (hourValue.isNull()) {
    hourValue = schedule["filterHourOn"];
  }
  minuteValue = root["filterMinuteOn"];
  if (minuteValue.isNull()) {
    minuteValue = root["filterStartMin"];
  }
  if (minuteValue.isNull()) {
    minuteValue = schedule["filterStartMin"];
  }
  if (minuteValue.isNull()) {
    minuteValue = schedule["filterMinuteOn"];
  }
  assignTimePair(root["filterOn"], hourValue, minuteValue,
                 effectiveFilterMode, &ConfigPatch::hasFilterHourOn,
                 &ConfigPatch::filterHourOn,
                 &ConfigPatch::hasFilterMinuteOn,
                 &ConfigPatch::filterMinuteOn, "invalid_filter_start");
  assignTimePair(filter["start"], filter["startHour"], filter["startMinute"],
                 effectiveFilterMode, &ConfigPatch::hasFilterHourOn,
                 &ConfigPatch::filterHourOn,
                 &ConfigPatch::hasFilterMinuteOn,
                 &ConfigPatch::filterMinuteOn, "invalid_filter_start");

  hourValue = root["filterHourOff"];
  if (hourValue.isNull()) {
    hourValue = root["filterEndHour"];
  }
  if (hourValue.isNull()) {
    hourValue = schedule["filterEndHour"];
  }
  if (hourValue.isNull()) {
    hourValue = schedule["filterHourOff"];
  }
  minuteValue = root["filterMinuteOff"];
  if (minuteValue.isNull()) {
    minuteValue = root["filterEndMin"];
  }
  if (minuteValue.isNull()) {
    minuteValue = schedule["filterEndMin"];
  }
  if (minuteValue.isNull()) {
    minuteValue = schedule["filterMinuteOff"];
  }
  assignTimePair(root["filterOff"], hourValue, minuteValue,
                 effectiveFilterMode, &ConfigPatch::hasFilterHourOff,
                 &ConfigPatch::filterHourOff,
                 &ConfigPatch::hasFilterMinuteOff,
                 &ConfigPatch::filterMinuteOff, "invalid_filter_end");
  assignTimePair(filter["end"], filter["endHour"], filter["endMinute"],
                 effectiveFilterMode, &ConfigPatch::hasFilterHourOff,
                 &ConfigPatch::filterHourOff,
                 &ConfigPatch::hasFilterMinuteOff,
                 &ConfigPatch::filterMinuteOff, "invalid_filter_end");

  auto assignPatchInt = [&](JsonVariantConst jsonValue,
                            bool ConfigPatch::*hasField,
                            int ConfigPatch::*field,
                            const char *errorCode) {
    if (jsonValue.isNull()) {
      return;
    }

    int parsed = 0;
    if (!readJsonInt(jsonValue, parsed)) {
      markParseError(errorCode);
      return;
    }

    patch.*hasField = true;
    patch.*field = parsed;
    hasAnyField = true;
  };

  auto assignPatchFloat = [&](JsonVariantConst jsonValue,
                              bool ConfigPatch::*hasField,
                              float ConfigPatch::*field,
                              const char *errorCode) {
    if (jsonValue.isNull()) {
      return;
    }

    float parsed = 0.0f;
    if (!readJsonFloat(jsonValue, parsed)) {
      markParseError(errorCode);
      return;
    }

    patch.*hasField = true;
    patch.*field = parsed;
    hasAnyField = true;
  };

  value = root["heaterMode"];
  if (value.isNull()) {
    value = schedule["heaterMode"];
  }
  if (value.isNull()) {
    value = temperature["heaterMode"];
  }
  if (!value.isNull()) {
    assignPatchInt(value, &ConfigPatch::hasHeaterMode,
                   &ConfigPatch::heaterMode, "invalid_heater_mode");
  } else if (!temperature["enabled"].isNull()) {
    bool enabled = false;
    if (!readJsonBool(temperature["enabled"], enabled)) {
      markParseError("invalid_heater_mode");
    } else {
      patch.hasHeaterMode = true;
      patch.heaterMode =
          enabled ? static_cast<int>(HeaterMode::Threshold)
                  : static_cast<int>(HeaterMode::Off);
      hasAnyField = true;
    }
  }

  value = root["targetTemp"];
  if (value.isNull()) {
    value = temperature["target"];
  }
  if (value.isNull()) {
    value = temperature["targetTemp"];
  }
  assignPatchFloat(value, &ConfigPatch::hasTargetTemp,
                   &ConfigPatch::targetTemp, "invalid_target_temp");

  value = root["tempHyst"];
  if (value.isNull()) {
    value = root["tempHysteresis"];
  }
  if (value.isNull()) {
    value = temperature["hysteresis"];
  }
  if (value.isNull()) {
    value = temperature["tempHysteresis"];
  }
  assignPatchFloat(value, &ConfigPatch::hasTempHysteresis,
                   &ConfigPatch::tempHysteresis, "invalid_hysteresis");

  value = root["servoPreOffMins"];
  if (value.isNull()) {
    value = aeration["servoPreOffMins"];
  }
  assignPatchInt(value, &ConfigPatch::hasServoPreOffMins,
                 &ConfigPatch::servoPreOffMins, "invalid_servo_preoff");

  value = root["feedMode"];
  if (value.isNull()) {
    value = root["feedFreq"];
  }
  if (value.isNull()) {
    value = feeding["freq"];
  }
  if (value.isNull()) {
    value = feeding["mode"];
  }
  if (value.isNull()) {
    value = feedingSchedule["freq"];
  }
  if (!value.isNull()) {
    int parsed = 0;
    if (!readFeedMode(value, parsed)) {
      markParseError("invalid_feed_mode");
    } else {
      patch.hasFeedMode = true;
      patch.feedMode = parsed;
      hasAnyField = true;
    }
  }

  hourValue = root["feedHour"];
  if (hourValue.isNull()) {
    hourValue = feeding["hour"];
  }
  minuteValue = root["feedMinute"];
  if (minuteValue.isNull()) {
    minuteValue = feeding["minute"];
  }
  assignTimePair(root["feedTime"], hourValue, minuteValue,
                 static_cast<uint8_t>(ScheduleMode::Schedule),
                 &ConfigPatch::hasFeedHour, &ConfigPatch::feedHour,
                 &ConfigPatch::hasFeedMinute, &ConfigPatch::feedMinute,
                 "invalid_feed_time");
  assignTimePair(feeding["time"], feedingSchedule["hour"],
                 feedingSchedule["minute"],
                 static_cast<uint8_t>(ScheduleMode::Schedule),
                 &ConfigPatch::hasFeedHour, &ConfigPatch::feedHour,
                 &ConfigPatch::hasFeedMinute, &ConfigPatch::feedMinute,
                 "invalid_feed_time");
  assignTimePair(feedingSchedule["time"], JsonVariantConst(),
                 JsonVariantConst(),
                 static_cast<uint8_t>(ScheduleMode::Schedule),
                 &ConfigPatch::hasFeedHour, &ConfigPatch::feedHour,
                 &ConfigPatch::hasFeedMinute, &ConfigPatch::feedMinute,
                 "invalid_feed_time");

  auto assignWifi = [&](JsonVariantConst jsonValue, char *dest,
                        size_t destSize, bool (*validator)(const char *),
                        const char *errorCode) {
    if (jsonValue.isNull()) {
      return;
    }
    if (!jsonValue.is<const char *>()) {
      markParseError(errorCode);
      return;
    }
    const char *raw = jsonValue.as<const char *>();
    if (!validator(raw)) {
      markParseError(errorCode);
      return;
    }
    snprintf(dest, destSize, "%s", raw);
    hasAnyField = true;
  };

  value = root["staSsid"];
  if (value.isNull()) {
    value = network["staSsid"];
  }
  assignWifi(value, candidate.staSsid, sizeof(candidate.staSsid),
             ConfigValidation::isStaSsidValid, "invalid_sta_ssid");

  value = root["staPassword"];
  if (value.isNull()) {
    value = network["staPassword"];
  }
  assignWifi(value, candidate.staPassword, sizeof(candidate.staPassword),
             ConfigValidation::isStaPasswordValid, "invalid_sta_password");

  value = root["apSsid"];
  if (value.isNull()) {
    value = network["apSsid"];
  }
  assignWifi(value, candidate.apSsid, sizeof(candidate.apSsid),
             ConfigValidation::isApSsidValid, "invalid_ap_ssid");

  value = root["apPassword"];
  if (value.isNull()) {
    value = network["apPassword"];
  }
  assignWifi(value, candidate.apPassword, sizeof(candidate.apPassword),
             ConfigValidation::isApPasswordValid, "invalid_ap_password");

  auto assignServoAngle = [&](JsonVariantConst jsonValue, int &field,
                              const char *errorCode) {
    if (jsonValue.isNull()) {
      return;
    }
    int parsed = 0;
    if (!readJsonInt(jsonValue, parsed) || parsed < SERVO_OPEN_ANGLE ||
        parsed > SERVO_CLOSED_ANGLE) {
      markParseError(errorCode);
      return;
    }
    field = parsed;
    hasAnyField = true;
  };

  value = root["servoDayAngle"];
  if (value.isNull()) {
    value = aeration["servoDayAngle"];
  }
  assignServoAngle(value, candidate.servoDayAngle, "invalid_servo_day_angle");

  value = root["servoNightAngle"];
  if (value.isNull()) {
    value = aeration["servoNightAngle"];
  }
  assignServoAngle(value, candidate.servoNightAngle,
                   "invalid_servo_night_angle");

  value = root["servoAlarmAngle"];
  if (value.isNull()) {
    value = aeration["servoAlarmAngle"];
  }
  assignServoAngle(value, candidate.servoAlarmAngle,
                   "invalid_servo_alarm_angle");

  value = root["alwaysScreenOn"];
  if (value.isNull()) {
    value = displaySettings["alwaysScreenOn"];
  }
  if (!value.isNull()) {
    bool alwaysOn = false;
    if (!readJsonBool(value, alwaysOn)) {
      markParseError("invalid_display_mode");
    } else {
      candidate.alwaysScreenOn = alwaysOn;
      hasAnyField = true;
    }
  }

  if (firstParseError[0] != '\0') {
    sendValidationError(server, firstParseError);
    return;
  }

  if (!hasAnyField) {
    sendWebActionResponse(server, 400, false, "empty_settings");
    return;
  }

  if (hasAnyRuntimePatchField(patch)) {
    ConfigValidationResult validation = {};
    if (!ConfigValidation::applyRuntimePatch(candidate, patch, validation) ||
        validation.hasInvalidFields()) {
      sendValidationError(server, validation.errorCode[0] != '\0'
                                      ? validation.errorCode
                                      : "invalid_values");
      return;
    }
  }

  const ConfigSaveResult saveResult = ConfigManager::updateAndSaveDetailed(candidate);
  if (!saveResult.ok) {
    char msg[160];
    snprintf(msg, sizeof(msg),
             "PATCH /api/config failed (written=%u, read=%u, status=%u)",
             static_cast<unsigned>(saveResult.bytesWritten),
             static_cast<unsigned>(saveResult.bytesReadBack),
             static_cast<unsigned>(saveResult.status));
    LogManager::logError(msg);
    sendWebActionResponse(server, 500, false, "save_failed");
    return;
  }

  LogManager::logInfo("PATCH /api/config: zapisano konfiguracje.");
  requestUiSaveConfirmationAnimation();
  sendWebActionResponse(server, 200, true,
                        saveResult.sanitizedChanged ? "settings_partial"
                                                    : "settings_saved");
}

} // namespace

extern WebServer server;

void setupApiEndpoints() {
  WebServer &server = AkwariumWifi::getServer();

  server.on("/api/status", HTTP_GET, [&server]() {
    server.sendHeader("Connection", "close");
    const bool includeHistory =
        server.hasArg("history") && server.arg("history") != "0";
    server.send(200, "application/json", buildWebStatusJson(includeHistory));
  });

  server.on("/api/config", HTTP_PATCH, [&server]() { handlePatchConfig(server); });

  server.on("/api/logs", HTTP_GET, [&server]() {
    server.sendHeader("Connection", "close");
    const String format = server.hasArg("format") ? server.arg("format") : "";
    const String type = server.hasArg("type") ? server.arg("type") : "all";

    if (format.equalsIgnoreCase("text")) {
      const String textPayload = LogManager::getLogsAsText(type.c_str());
      server.sendHeader("Content-Disposition",
                        "attachment; filename=akwarium_logs.txt");
      server.send(200, "text/plain; charset=utf-8", textPayload);
      return;
    }

    server.send(200, "application/json", buildWebLogsJson());
  });

  server.on("/api/action", HTTP_POST, [&server]() {
    if (!server.hasArg("action")) {
      sendWebActionResponse(server, 400, false, "missing_action");
      return;
    }

    PowerManager::registerActivity();
    const String action = server.arg("action");

    if (action == "feed_now") {
      const Error feedResult = SystemController::feedNow();
      const bool ok = feedResult == Error::NONE;
      sendWebActionResponse(server, feederErrorHttpStatus(feedResult), ok,
                            feederErrorCode(feedResult));
      return;
    }

    if (action == "set_light" || action == "set_filter") {
      if (!server.hasArg("state")) {
        sendWebActionResponse(server, 400, false, "missing_state");
        return;
      }

      bool state = false;
      if (!parseBoolArg(server.arg("state"), state)) {
        sendWebActionResponse(server, 400, false, "invalid_state");
        return;
      }

      ConfigPatch patch = {};
      if (action == "set_light") {
        patch.hasLightMode = true;
        patch.lightMode = state ? static_cast<int>(ScheduleMode::AlwaysOn)
                                : static_cast<int>(ScheduleMode::AlwaysOff);
      } else {
        patch.hasFilterMode = true;
        patch.filterMode = state ? static_cast<int>(ScheduleMode::AlwaysOn)
                                 : static_cast<int>(ScheduleMode::AlwaysOff);
      }

      const ConfigApplyResult applyResult = ConfigService::applyPatch(patch);
      if (!applyResult.ok && applyResult.status == ConfigApplyStatus::SaveFailed) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "API save failed (%s, written=%u, read=%u)",
                 action.c_str(),
                 static_cast<unsigned>(applyResult.saveResult.bytesWritten),
                 static_cast<unsigned>(applyResult.saveResult.bytesReadBack));
        LogManager::logError(msg);
      }
      sendConfigApplyResult(server, applyResult);
      return;
    }

    if (action == "set_servo") {
      if (!server.hasArg("angle")) {
        sendWebActionResponse(server, 400, false, "missing_angle");
        return;
      }

      long parsedAngle = 0;
      if (!parseLongStrict(server.arg("angle"), parsedAngle)) {
        sendWebActionResponse(server, 400, false, "invalid_angle");
        return;
      }

      int ang = constrain(static_cast<int>(parsedAngle), 0, 90);
      SystemController::setManualServo(ang);
      sendWebActionResponse(server, 200, true, "set_servo");
      return;
    }

    if (action == "clear_servo") {
      SystemController::clearManualServo();
      sendWebActionResponse(server, 200, true, "clear_servo");
      return;
    }

    if (action == "clear_critical_logs") {
      LogManager::clearCriticalLogs();
      sendWebActionResponse(server, 200, true, "clear_critical_logs");
      return;
    }

    if (action == "save_temperature") {
      ConfigPatch patch = {};
      uint8_t parseInvalidFields = 0;

      if (server.hasArg("heaterMode")) {
        long value = 0;
        if (!parseLongStrict(server.arg("heaterMode"), value)) {
          parseInvalidFields++;
        } else {
          patch.hasHeaterMode = true;
          patch.heaterMode = static_cast<int>(value);
        }
      }

      if (server.hasArg("targetTemp")) {
        float value = 0.0f;
        if (!parseFloatStrict(server.arg("targetTemp"), value)) {
          parseInvalidFields++;
        } else {
          patch.hasTargetTemp = true;
          patch.targetTemp = value;
        }
      }

      if (server.hasArg("tempHyst")) {
        float value = 0.0f;
        if (!parseFloatStrict(server.arg("tempHyst"), value)) {
          parseInvalidFields++;
        } else {
          patch.hasTempHysteresis = true;
          patch.tempHysteresis = value;
        }
      }

      const ConfigApplyResult applyResult = ConfigService::applyPatch(patch);
      const bool forcePartial = parseInvalidFields > 0;
      if (!applyResult.ok && applyResult.status == ConfigApplyStatus::SaveFailed) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "API save_temperature failed (written=%u, read=%u, status=%u)",
                 static_cast<unsigned>(applyResult.saveResult.bytesWritten),
                 static_cast<unsigned>(applyResult.saveResult.bytesReadBack),
                 static_cast<unsigned>(applyResult.saveResult.status));
        LogManager::logError(msg);
      }
      sendConfigApplyResult(server, applyResult, forcePartial);
      return;
    }

    if (action == "sync_time_ntp") {
      const TimeSyncCommandResult result = AkwariumWifi::syncTimeWithNtpNow();
      const bool ok = result == TimeSyncCommandResult::Ok;
      const String status = AkwariumWifi::getLastTimeSyncStatus();
      sendWebActionResponse(server, timeSyncCommandHttpStatus(result), ok,
                            timeSyncCommandCode(result), status.c_str());
      return;
    }

    if (action == "wifi_session_start") {
      AkwariumWifi::startAP();
      sendWebActionResponse(
          server, 200, true, "wifi_session_start",
          "Uruchamiam lub ponawiam sesje WiFi: najpierw STA, potem fallback do AP.");
      return;
    }

    if (action == "wifi_session_stop") {
      AkwariumWifi::stopAP();
      sendWebActionResponse(server, 200, true, "wifi_session_stop",
                            "Wylaczam aktywna sesje WiFi.");
      return;
    }

    if (action == "restart_device") {
      sendWebActionResponse(server, 200, true, "restart_device");
      delay(150);
      ESP.restart();
      return;
    }

    if (action == "factory_reset") {
      ConfigManager::resetToDefault();
      LogManager::clearCriticalLogs();
      sendWebActionResponse(server, 200, true, "factory_reset");
      delay(200);
      ESP.restart();
      return;
    }

    if (action == "save_network") {
      Config cfg = ConfigManager::getCopy();
      bool anyField = false;

      auto applyWifiField = [&](const char *argName, char *dest,
                                size_t destSize,
                                bool (*validator)(const char *),
                                const char *errorCode,
                                bool ignoreEmpty) -> bool {
        if (!server.hasArg(argName)) {
          return true;
        }

        String raw = server.arg(argName);
        raw.trim();
        if (ignoreEmpty && raw.length() == 0) {
          return true;
        }

        anyField = true;
        if (!validator(raw.c_str())) {
          sendValidationError(server, errorCode);
          return false;
        }

        snprintf(dest, destSize, "%s", raw.c_str());
        return true;
      };

      if (!applyWifiField("staSsid", cfg.staSsid, sizeof(cfg.staSsid),
                          ConfigValidation::isStaSsidValid, "invalid_sta_ssid",
                          false)) {
        return;
      }
      if (!applyWifiField("staPassword", cfg.staPassword,
                          sizeof(cfg.staPassword),
                          ConfigValidation::isStaPasswordValid,
                          "invalid_sta_password", true)) {
        return;
      }
      if (!applyWifiField("apSsid", cfg.apSsid, sizeof(cfg.apSsid),
                          ConfigValidation::isApSsidValid, "invalid_ap_ssid",
                          false)) {
        return;
      }
      if (!applyWifiField("apPassword", cfg.apPassword, sizeof(cfg.apPassword),
                          ConfigValidation::isApPasswordValid,
                          "invalid_ap_password", true)) {
        return;
      }

      if (!anyField) {
        sendWebActionResponse(server, 400, false, "empty_settings");
        return;
      }

      const ConfigSaveResult saveResult = ConfigManager::updateAndSaveDetailed(cfg);
      if (!saveResult.ok) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "API save_network failed (written=%u, read=%u, status=%u)",
                 static_cast<unsigned>(saveResult.bytesWritten),
                 static_cast<unsigned>(saveResult.bytesReadBack),
                 static_cast<unsigned>(saveResult.status));
        LogManager::logError(msg);
        sendWebActionResponse(server, 500, false, "save_failed");
        return;
      }

      LogManager::logInfo(
          "Ustawienia WiFi zapisane. Nowe SSID/hasla beda uzyte w kolejnej sesji WiFi.");
      requestUiSaveConfirmationAnimation();
      sendWebActionResponse(server, 200, true, "settings_saved");
      return;
    }

    if (action != "save_schedule") {
      sendWebActionResponse(server, 400, false, "unknown_action");
      return;
    }

    ConfigPatch patch = {};
    uint8_t parseInvalidFields = 0;

    auto assignMode = [&](const char *name, bool ConfigPatch::*hasField,
                          int ConfigPatch::*field) {
      bool hasValue = false;
      int parsed = 0;
      if (!parseModeArg(server, name, hasValue, parsed)) {
        parseInvalidFields++;
        return;
      }

      if (hasValue) {
        patch.*hasField = true;
        patch.*field = parsed;
      }
    };

    assignMode("lightMode", &ConfigPatch::hasLightMode, &ConfigPatch::lightMode);
    assignMode("aerationMode", &ConfigPatch::hasAerationMode,
               &ConfigPatch::aerationMode);
    assignMode("filterMode", &ConfigPatch::hasFilterMode,
               &ConfigPatch::filterMode);
    assignMode("heaterMode", &ConfigPatch::hasHeaterMode,
               &ConfigPatch::heaterMode);

    auto parseTimeIntoPatch = [&](const char *argName, bool ConfigPatch::*hasHour,
                                  int ConfigPatch::*hourField,
                                  bool ConfigPatch::*hasMinute,
                                  int ConfigPatch::*minuteField) {
      if (!server.hasArg(argName)) {
        return;
      }

      int hour = 0;
      int minute = 0;
      if (!parseTimeArg(server.arg(argName), hour, minute)) {
        parseInvalidFields++;
        return;
      }

      patch.*hasHour = true;
      patch.*hourField = hour;
      patch.*hasMinute = true;
      patch.*minuteField = minute;
    };

    parseTimeIntoPatch("dayStart", &ConfigPatch::hasDayStartHour,
                       &ConfigPatch::dayStartHour,
                       &ConfigPatch::hasDayStartMinute,
                       &ConfigPatch::dayStartMinute);
    parseTimeIntoPatch("dayEnd", &ConfigPatch::hasDayEndHour,
                       &ConfigPatch::dayEndHour, &ConfigPatch::hasDayEndMinute,
                       &ConfigPatch::dayEndMinute);
    parseTimeIntoPatch("airOn", &ConfigPatch::hasAerationHourOn,
                       &ConfigPatch::aerationHourOn,
                       &ConfigPatch::hasAerationMinuteOn,
                       &ConfigPatch::aerationMinuteOn);
    parseTimeIntoPatch("airOff", &ConfigPatch::hasAerationHourOff,
                       &ConfigPatch::aerationHourOff,
                       &ConfigPatch::hasAerationMinuteOff,
                       &ConfigPatch::aerationMinuteOff);
    parseTimeIntoPatch("filterOn", &ConfigPatch::hasFilterHourOn,
                       &ConfigPatch::filterHourOn,
                       &ConfigPatch::hasFilterMinuteOn,
                       &ConfigPatch::filterMinuteOn);
    parseTimeIntoPatch("filterOff", &ConfigPatch::hasFilterHourOff,
                       &ConfigPatch::filterHourOff,
                       &ConfigPatch::hasFilterMinuteOff,
                       &ConfigPatch::filterMinuteOff);
    parseTimeIntoPatch("feedTime", &ConfigPatch::hasFeedHour,
                       &ConfigPatch::feedHour, &ConfigPatch::hasFeedMinute,
                       &ConfigPatch::feedMinute);

    if (server.hasArg("feedFreq")) {
      long value = 0;
      if (!parseLongStrict(server.arg("feedFreq"), value)) {
        parseInvalidFields++;
      } else {
        patch.hasFeedMode = true;
        patch.feedMode = static_cast<int>(value);
      }
    }

    if (server.hasArg("targetTemp")) {
      float value = 0.0f;
      if (!parseFloatStrict(server.arg("targetTemp"), value)) {
        parseInvalidFields++;
      } else {
        patch.hasTargetTemp = true;
        patch.targetTemp = value;
      }
    }

    if (server.hasArg("tempHyst")) {
      float value = 0.0f;
      if (!parseFloatStrict(server.arg("tempHyst"), value)) {
        parseInvalidFields++;
      } else {
        patch.hasTempHysteresis = true;
        patch.tempHysteresis = value;
      }
    }

    if (server.hasArg("servoPreOffMins")) {
      long value = 0;
      if (!parseLongStrict(server.arg("servoPreOffMins"), value)) {
        parseInvalidFields++;
      } else {
        patch.hasServoPreOffMins = true;
        patch.servoPreOffMins = static_cast<int>(value);
      }
    }

    const ConfigApplyResult applyResult = ConfigService::applyPatch(patch);
    if (!applyResult.ok && applyResult.status == ConfigApplyStatus::SaveFailed) {
      char msg[160];
      snprintf(msg, sizeof(msg),
               "API save_schedule failed (written=%u, read=%u, status=%u)",
               static_cast<unsigned>(applyResult.saveResult.bytesWritten),
               static_cast<unsigned>(applyResult.saveResult.bytesReadBack),
               static_cast<unsigned>(applyResult.saveResult.status));
      LogManager::logError(msg);
    }
    sendConfigApplyResult(server, applyResult, parseInvalidFields > 0);
  });
}
