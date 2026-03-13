#include "ApiHandlers.h"

#include "AkwariumWifi.h"
#include "ConfigManager.h"
#include "ConfigValidation.h"
#include "FirmwareInfo.h"
#include "LogManager.h"
#include "OtaManager.h"
#include "PowerManager.h"
#include "SharedState.h"
#include "SystemController.h"

#include <ArduinoJson.h>
#include <Arduino.h>
#include <WebServer.h>
#include <cstdlib>

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

static void populateValidationJson(JsonObject validation) {
  const ValidationProfileSnapshot profile = ConfigValidation::getValidationProfile();

  validation["minuteStep"] = profile.minuteStep;
  validation["scheduleModes"] = "schedule|always_on|always_off";
  validation["heaterModes"] = "threshold|off";
  validation["timeFieldsRequireScheduleMode"] = true;

  JsonObject temperature = validation.createNestedObject("temperature");
  temperature["min"] = profile.minTemperature;
  temperature["max"] = profile.maxTemperature;
  temperature["step"] = profile.temperatureStep;
  temperature["supportsOff"] = true;

  JsonObject hysteresis = validation.createNestedObject("hysteresis");
  hysteresis["min"] = profile.hysteresisMin;
  hysteresis["max"] = profile.hysteresisMax;
  hysteresis["step"] = profile.hysteresisStep;

  JsonObject servoPreOff = validation.createNestedObject("servoPreOffMinutes");
  servoPreOff["min"] = profile.servoPreOffMin;
  servoPreOff["max"] = profile.servoPreOffMax;
  servoPreOff["step"] = 1;

  JsonObject feeding = validation.createNestedObject("feeding");
  feeding["modeMin"] = profile.feedModeMin;
  feeding["modeMax"] = profile.feedModeMax;
}

static String buildStatusJson() {
  const SharedStateData snap = SharedState::getSnapshot();
  const Config cfg = ConfigManager::getCopy();
  const FirmwareRuntimeInfo firmwareInfo = FirmwareInfo::getRuntimeInfo();

  float voltage = PowerManager::getBatteryVoltage();
  if (isnan(voltage)) {
    voltage = 0.0f;
  }

  StaticJsonDocument<2048> doc;

  JsonObject temperature = doc.createNestedObject("temperature");
  temperature["current"] = isnan(snap.temperature) ? -99.9f : snap.temperature;
  temperature["target"] = cfg.targetTemp;
  temperature["threshold"] = cfg.targetTemp + cfg.tempHysteresis;
  temperature["heaterMode"] = cfg.heaterMode;
  temperature["hysteresis"] = cfg.tempHysteresis;
  temperature["min"] = isnan(snap.minTemp) ? -99.9f : snap.minTemp;
  temperature["minTimeEpoch"] = snap.minTempEpoch;

  JsonObject battery = doc.createNestedObject("battery");
  battery["voltage"] = voltage;
  battery["percent"] = PowerManager::getBatteryPercent();

  JsonObject relays = doc.createNestedObject("relays");
  relays["light"] = snap.isLightOn;
  relays["pump"] = snap.isFilterOn;
  relays["heater"] = snap.isHeaterOn;

  JsonObject servo = doc.createNestedObject("servo");
  servo["angle"] = SystemController::getServoPosition();

  JsonObject schedule = doc.createNestedObject("schedule");
  schedule["lightMode"] = cfg.lightMode;
  schedule["dayStartHour"] = cfg.dayStartHour;
  schedule["dayStartMin"] = cfg.dayStartMinute;
  schedule["dayEndHour"] = cfg.dayEndHour;
  schedule["dayEndMin"] = cfg.dayEndMinute;
  schedule["airMode"] = cfg.aerationMode;
  schedule["airStartHour"] = cfg.aerationHourOn;
  schedule["airStartMin"] = cfg.aerationMinuteOn;
  schedule["airEndHour"] = cfg.aerationHourOff;
  schedule["airEndMin"] = cfg.aerationMinuteOff;
  schedule["filterMode"] = cfg.filterMode;
  schedule["filterStartHour"] = cfg.filterHourOn;
  schedule["filterStartMin"] = cfg.filterMinuteOn;
  schedule["filterEndHour"] = cfg.filterHourOff;
  schedule["filterEndMin"] = cfg.filterMinuteOff;
  schedule["heaterMode"] = cfg.heaterMode;
  schedule["servoPreOffMins"] = cfg.servoPreOffMins;

  JsonObject feeding = doc.createNestedObject("feeding");
  feeding["hour"] = cfg.feedHour;
  feeding["minute"] = cfg.feedMinute;
  feeding["freq"] = cfg.feedMode;
  feeding["lastFeedEpoch"] = cfg.lastFeedEpoch;

  JsonObject network = doc.createNestedObject("network");
  network["ip"] = AkwariumWifi::getIP();
  network["apMode"] = AkwariumWifi::getIsAPMode();
  network["ssid"] = AkwariumWifi::getAPName();
  network["clients"] = AkwariumWifi::getConnectedClients();

  JsonObject system = doc.createNestedObject("system");
  system["firmwareName"] = firmwareInfo.firmwareName;
  system["firmwareVersion"] = firmwareInfo.firmwareVersion;
  system["buildRef"] = firmwareInfo.buildRef;
  system["buildDate"] = firmwareInfo.buildDate;
  system["buildTime"] = firmwareInfo.buildTime;
  system["idfVersion"] = firmwareInfo.idfVersion;
  system["runningPartition"] = firmwareInfo.runningPartitionLabel;
  system["bootPartition"] = firmwareInfo.bootPartitionLabel;
  system["nextPartition"] = firmwareInfo.nextPartitionLabel;
  system["otaPartitionSize"] = firmwareInfo.nextPartitionSizeBytes;
  system["otaInProgress"] = OtaManager::isOtaInProgress();
  system["otaTransport"] = OtaManager::getActiveTransport();
  system["bleOtaSupported"] = FirmwareInfo::supportsBleOta();
  system["httpOtaSupported"] = FirmwareInfo::supportsHttpOta();
  system["uptimeSec"] = static_cast<uint32_t>(millis() / 1000UL);
  system["resetReason"] = SystemController::getLastResetLabel();
  populateValidationJson(system.createNestedObject("validation"));

  JsonObject clock = doc.createNestedObject("clock");
  clock["hour"] = snap.hour;
  clock["minute"] = snap.minute;
  clock["second"] = snap.second;
  clock["day"] = snap.day;
  clock["month"] = snap.month;
  clock["year"] = snap.year;

  String json;
  serializeJson(doc, json);
  return json;
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
  server.send(400, "text/plain",
              (code != nullptr && code[0] != '\0') ? code : "invalid_values");
}

} // namespace

extern WebServer server;

void setupApiEndpoints() {
  WebServer &server = AkwariumWifi::getServer();

  server.on("/api/status", HTTP_GET, [&server]() {
    server.sendHeader("Connection", "close");
    server.send(200, "application/json", buildStatusJson());
  });

  server.on("/api/logs", HTTP_GET, [&server]() {
    String jsonLog = LogManager::getLogsAsJson();
    server.sendHeader("Connection", "close");
    server.send(200, "application/json", jsonLog);
  });

  server.on("/api/action", HTTP_POST, [&server]() {
    if (!server.hasArg("action")) {
      server.send(400, "text/plain", "missing_action");
      return;
    }

    PowerManager::registerActivity();
    const String action = server.arg("action");

    if (action == "feed_now") {
      SystemController::feedNow();
      server.send(200, "text/plain", "OK");
      return;
    }

    if (action == "set_servo") {
      if (!server.hasArg("angle")) {
        server.send(400, "text/plain", "missing_angle");
        return;
      }

      int ang = constrain(server.arg("angle").toInt(), 0, 90);
      SystemController::setManualServo(ang);
      server.send(200, "text/plain", "OK");
      return;
    }

    if (action == "clear_servo") {
      SystemController::clearManualServo();
      server.send(200, "text/plain", "OK");
      return;
    }

    if (action == "clear_critical_logs") {
      LogManager::clearCriticalLogs();
      server.send(200, "text/plain", "OK");
      return;
    }

    if (action == "restart_device") {
      server.send(200, "text/plain", "OK");
      delay(150);
      ESP.restart();
      return;
    }

    if (action == "factory_reset") {
      ConfigManager::resetToDefault();
      LogManager::clearCriticalLogs();
      server.send(200, "text/plain", "OK");
      delay(200);
      ESP.restart();
      return;
    }

    if (action != "save_schedule") {
      server.send(400, "text/plain", "unknown_action");
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

    if (parseInvalidFields > 0) {
      sendValidationError(server, "invalid_payload");
      return;
    }

    Config cfg = ConfigManager::getCopy();
    ConfigValidationResult validation = {};
    if (!ConfigValidation::applyRuntimePatch(cfg, patch, validation)) {
      sendValidationError(server,
                          validation.errorCode[0] != '\0' ? validation.errorCode
                                                          : "invalid_values");
      return;
    }

    if (!ConfigManager::updateAndSave(cfg)) {
      server.send(500, "text/plain", "save_failed");
      return;
    }

    server.send(200, "text/plain", "OK");
  });
}
