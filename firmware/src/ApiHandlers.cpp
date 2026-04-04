#include "ApiHandlers.h"

#include "AkwariumWifi.h"
#include "ConfigManager.h"
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

} // namespace

extern WebServer server;

void setupApiEndpoints() {
  WebServer &server = AkwariumWifi::getServer();

  server.on("/api/status", HTTP_GET, [&server]() {
    server.sendHeader("Connection", "close");
    server.send(200, "application/json", buildWebStatusJson());
  });

  server.on("/api/logs", HTTP_GET, [&server]() {
    server.sendHeader("Connection", "close");
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
      SystemController::feedNow();
      sendWebActionResponse(server, 200, true, "feed_now");
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

      Config cfg = ConfigManager::getCopy();
      ConfigValidationResult validation = {};
      if (!ConfigValidation::applyRuntimePatch(cfg, patch, validation)) {
        sendValidationError(server,
                            validation.errorCode[0] != '\0' ? validation.errorCode
                                                             : "invalid_values");
        return;
      }

      const ConfigSaveResult saveResult = ConfigManager::updateAndSaveDetailed(cfg);
      if (!saveResult.ok) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "API save failed (%s, written=%u, read=%u)",
                 action.c_str(), static_cast<unsigned>(saveResult.bytesWritten),
                 static_cast<unsigned>(saveResult.bytesReadBack));
        LogManager::logError(msg);
        sendWebActionResponse(server, 500, false, "save_failed");
        return;
      }

      requestUiSaveConfirmationAnimation();
      sendWebActionResponse(server, 200, true, "settings_saved");
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

    Config cfg = ConfigManager::getCopy();
    ConfigValidationResult validation = {};
    if (!ConfigValidation::applyRuntimePatch(cfg, patch, validation)) {
      sendValidationError(server, parseInvalidFields > 0
                                      ? "invalid_payload"
                                      : validation.errorCode[0] != '\0'
                                            ? validation.errorCode
                                            : "invalid_values");
      return;
    }

    const ConfigSaveResult saveResult = ConfigManager::updateAndSaveDetailed(cfg);
    if (!saveResult.ok) {
      char msg[160];
      snprintf(msg, sizeof(msg),
               "API save_schedule failed (written=%u, read=%u, status=%u)",
               static_cast<unsigned>(saveResult.bytesWritten),
               static_cast<unsigned>(saveResult.bytesReadBack),
               static_cast<unsigned>(saveResult.status));
      LogManager::logError(msg);
      sendWebActionResponse(server, 500, false, "save_failed");
      return;
    }

    requestUiSaveConfirmationAnimation();
    sendWebActionResponse(server, 200, true,
                          (parseInvalidFields > 0 ||
                           validation.hasInvalidFields())
                              ? "settings_partial"
                              : "settings_saved");
  });
}
