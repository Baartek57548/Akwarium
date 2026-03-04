#include "ApiHandlers.h"
#include "AkwariumWifi.h"
#include "ConfigManager.h"
#include "ConfigValidation.h"
#include "InterfaceCore.h"
#include "LogManager.h"
#include "PowerManager.h"
#include "SharedState.h"
#include "SystemController.h"
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

static bool parseTimeArg(const String &value, int &hour, int &minute,
                         bool allow24Hour = false) {
  if (value.length() != 5 || value[2] != ':') {
    return false;
  }
  if (!isDigit(value[0]) || !isDigit(value[1]) || !isDigit(value[3]) ||
      !isDigit(value[4])) {
    return false;
  }

  hour = value.substring(0, 2).toInt();
  minute = value.substring(3, 5).toInt();
  if (minute < 0 || minute > 59) {
    return false;
  }
  if (hour == 24) {
    return allow24Hour && minute == 0;
  }
  return hour >= 0 && hour <= 23;
}

static void sendRuleResult(WebServer &server, const InterfaceRuleResult &result) {
  switch (result.code) {
  case InterfaceRuleCode::OK:
    server.send(200, "text/plain", "OK");
    return;
  case InterfaceRuleCode::OK_PARTIAL:
    server.send(200, "text/plain", "OK_PARTIAL");
    return;
  case InterfaceRuleCode::INVALID_VALUE:
    server.send(400, "text/plain", "Invalid value");
    return;
  case InterfaceRuleCode::SAVE_FAILED:
    server.send(500, "text/plain", "Save failed");
    return;
  }
}

} // namespace

extern WebServer server; // from AkwariumWifi

void setupApiEndpoints() {
  WebServer &server = AkwariumWifi::getServer();

  server.on("/api/status", HTTP_GET, [&server]() {
    const SharedStateData snap = SharedState::getSnapshot();
    const Config cfg = ConfigManager::getCopy();

    float voltage = PowerManager::getBatteryVoltage();
    if (isnan(voltage))
      voltage = 0.0f;

    char json[700];
    snprintf(
        json, sizeof(json),
        "{\"temperature\":{\"current\":%.1f,\"target\":%.1f,\"hysteresis\":%."
        "1f,"
        "\"min\":%.1f,\"minTimeEpoch\":%u},"
        "\"battery\":{\"voltage\":%.2f,\"percent\":%d},"
        "\"relays\":{\"light\":%s,\"pump\":%s},"
        "\"servo\":{\"angle\":%d},"
        "\"schedule\":{\"dayStartHour\":%d,\"dayStartMin\":%d,\"dayEndHour\":%"
        "d,\"dayEndMin\":%d,"
        "\"airStartHour\":%d,\"airStartMin\":%d,\"airEndHour\":%d,"
        "\"airEndMin\":%d,"
        "\"filterStartHour\":%d,\"filterStartMin\":%d,\"filterEndHour\":%d,"
        "\"filterEndMin\":%d,"
        "\"servoPreOffMins\":%d},"
        "\"feeding\":{\"hour\":%d,\"minute\":%d,\"freq\":%d,\"lastFeedEpoch\":%"
        "u},"
        "\"network\":{\"ip\":\"%s\",\"apMode\":%s}}",
        isnan(snap.temperature) ? -99.9 : snap.temperature, cfg.targetTemp,
        cfg.tempHysteresis, isnan(snap.minTemp) ? 20.0 : snap.minTemp,
        (unsigned int)snap.minTempEpoch, voltage,
        (int)PowerManager::getBatteryPercent(),
        snap.isLightOn ? "true" : "false", snap.isFilterOn ? "true" : "false",
        SystemController::getServoPosition(),
        cfg.dayStartHour, cfg.dayStartMinute, cfg.dayEndHour, cfg.dayEndMinute,
        cfg.aerationHourOn, cfg.aerationMinuteOn, cfg.aerationHourOff,
        cfg.aerationMinuteOff, cfg.filterHourOn, cfg.filterMinuteOn,
        cfg.filterHourOff, cfg.filterMinuteOff, cfg.servoPreOffMins,
        cfg.feedHour, cfg.feedMinute, cfg.feedMode,
        (unsigned int)cfg.lastFeedEpoch,
        AkwariumWifi::getIP().c_str(),
        AkwariumWifi::getIsAPMode() ? "true" : "false");
    server.sendHeader("Connection", "close");
    server.send(200, "application/json", json);
  });

  server.on("/api/logs", HTTP_GET, [&server]() {
    String jsonLog = LogManager::getLogsAsJson();
    server.sendHeader("Connection", "close");
    server.send(200, "application/json", jsonLog);
  });

  server.on("/api/action", HTTP_POST, [&server]() {
    if (server.hasArg("action")) {
      PowerManager::registerActivity();
      String action = server.arg("action");
      if (action == "feed_now") {
        InterfaceCore::triggerFeedNow();
        server.send(200, "text/plain", "OK");
        return;
      } else if (action == "set_servo") {
        if (server.hasArg("angle")) {
          long angle = 0;
          if (!parseLongStrict(server.arg("angle"), angle)) {
            server.send(400, "text/plain", "Invalid angle");
            return;
          }
          InterfaceRuleResult servoResult = InterfaceCore::setManualServoAngle(angle);
          sendRuleResult(server, servoResult);
          return;
        }
      } else if (action == "clear_servo") {
        InterfaceCore::clearManualServo();
        server.send(200, "text/plain", "OK");
        return;
      } else if (action == "clear_critical_logs") {
        InterfaceCore::clearCriticalLogs();
        server.send(200, "text/plain", "OK");
        return;
      } else if (action == "save_schedule") {
        ConfigPatch patch = {};
        uint8_t invalidFields = 0;

        if (server.hasArg("feedTime")) {
          String ft = server.arg("feedTime");
          int h = 0;
          int m = 0;
          if (parseTimeArg(ft, h, m)) {
            patch.hasFeedHour = true;
            patch.feedHour = h;
            patch.hasFeedMinute = true;
            patch.feedMinute = m;
          } else {
            invalidFields++;
          }
        }
        if (server.hasArg("feedFreq")) {
          long v = 0;
          if (parseLongStrict(server.arg("feedFreq"), v)) {
            patch.hasFeedMode = true;
            patch.feedMode = static_cast<int>(v);
          } else {
            invalidFields++;
          }
        }
        if (server.hasArg("dayStart")) {
          String ds = server.arg("dayStart");
          int h = 0;
          int m = 0;
          if (parseTimeArg(ds, h, m, true)) {
            patch.hasDayStartHour = true;
            patch.dayStartHour = h;
            patch.hasDayStartMinute = true;
            patch.dayStartMinute = m;
          } else {
            invalidFields++;
          }
        }
        if (server.hasArg("dayEnd")) {
          String de = server.arg("dayEnd");
          int h = 0;
          int m = 0;
          if (parseTimeArg(de, h, m, true)) {
            patch.hasDayEndHour = true;
            patch.dayEndHour = h;
            patch.hasDayEndMinute = true;
            patch.dayEndMinute = m;
          } else {
            invalidFields++;
          }
        }
        if (server.hasArg("targetTemp")) {
          float v = 0.0f;
          if (parseFloatStrict(server.arg("targetTemp"), v)) {
            patch.hasTargetTemp = true;
            patch.targetTemp = v;
          } else {
            invalidFields++;
          }
        }
        if (server.hasArg("tempHyst")) {
          float v = 0.0f;
          if (parseFloatStrict(server.arg("tempHyst"), v)) {
            patch.hasTempHysteresis = true;
            patch.tempHysteresis = v;
          } else {
            invalidFields++;
          }
        }
        if (server.hasArg("airOn")) {
          String ao = server.arg("airOn");
          int h = 0;
          int m = 0;
          if (parseTimeArg(ao, h, m)) {
            patch.hasAerationHourOn = true;
            patch.aerationHourOn = h;
            patch.hasAerationMinuteOn = true;
            patch.aerationMinuteOn = m;
          } else {
            invalidFields++;
          }
        }
        if (server.hasArg("airOff")) {
          String af = server.arg("airOff");
          int h = 0;
          int m = 0;
          if (parseTimeArg(af, h, m)) {
            patch.hasAerationHourOff = true;
            patch.aerationHourOff = h;
            patch.hasAerationMinuteOff = true;
            patch.aerationMinuteOff = m;
          } else {
            invalidFields++;
          }
        }
        if (server.hasArg("filterOn")) {
          String fo = server.arg("filterOn");
          int h = 0;
          int m = 0;
          if (parseTimeArg(fo, h, m)) {
            patch.hasFilterHourOn = true;
            patch.filterHourOn = h;
            patch.hasFilterMinuteOn = true;
            patch.filterMinuteOn = m;
          } else {
            invalidFields++;
          }
        }
        if (server.hasArg("filterOff")) {
          String ff = server.arg("filterOff");
          int h = 0;
          int m = 0;
          if (parseTimeArg(ff, h, m)) {
            patch.hasFilterHourOff = true;
            patch.filterHourOff = h;
            patch.hasFilterMinuteOff = true;
            patch.filterMinuteOff = m;
          } else {
            invalidFields++;
          }
        }
        if (server.hasArg("servoPreOffMins")) {
          long v = 0;
          if (parseLongStrict(server.arg("servoPreOffMins"), v)) {
            patch.hasServoPreOffMins = true;
            patch.servoPreOffMins = static_cast<int>(v);
          } else {
            invalidFields++;
          }
        }

        InterfaceRuleResult patchResult =
            InterfaceCore::applyConfigPatchAndSave(patch, invalidFields);
        sendRuleResult(server, patchResult);
        return;
      }
    }
    server.send(400, "text/plain", "Bad Request");
  });
}
