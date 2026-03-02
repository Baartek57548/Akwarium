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

static void addCorsHeaders(WebServer &server) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

static void handleCorsPreflight(WebServer &server) {
  addCorsHeaders(server);
  server.send(204, "text/plain", "");
}

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

static bool parseBoolStrict(const String &raw, bool &out) {
  if (raw.length() == 0) {
    return false;
  }

  String value = raw;
  value.trim();
  value.toLowerCase();

  if (value == "1" || value == "true" || value == "on") {
    out = true;
    return true;
  }
  if (value == "0" || value == "false" || value == "off") {
    out = false;
    return true;
  }

  long parsed = 0;
  if (parseLongStrict(value, parsed)) {
    out = (parsed != 0);
    return true;
  }

  return false;
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

static bool applyPatchAndSave(ConfigPatch &patch, uint8_t invalidFields,
                              WebServer &server) {
  Config cfg = ConfigManager::getCopy();
  ConfigValidationResult validation = {};
  ConfigValidation::applyPatchAndClamp(cfg, patch, validation);
  if (!validation.hasAnyApplied()) {
    server.send(400, "text/plain", "No valid fields");
    return false;
  }

  if (!ConfigManager::updateAndSave(cfg)) {
    server.send(500, "text/plain", "Save failed");
    return false;
  }

  if (validation.hasInvalidFields() || invalidFields > 0) {
    server.send(200, "text/plain", "OK_PARTIAL");
  } else {
    server.send(200, "text/plain", "OK");
  }
  return true;
}

} // namespace

extern WebServer server; // from AkwariumWifi

void setupApiEndpoints() {
  WebServer &server = AkwariumWifi::getServer();

  server.on("/api/status", HTTP_OPTIONS, [&server]() {
    handleCorsPreflight(server);
  });
  server.on("/api/logs", HTTP_OPTIONS, [&server]() {
    handleCorsPreflight(server);
  });
  server.on("/api/action", HTTP_OPTIONS, [&server]() {
    handleCorsPreflight(server);
  });

  server.on("/api/status", HTTP_GET, [&server]() {
    PowerManager::registerActivity();
    addCorsHeaders(server);
    const SharedStateData snap = SharedState::getSnapshot();
    const Config cfg = ConfigManager::getCopy();

    float voltage = PowerManager::getBatteryVoltage();
    if (isnan(voltage))
      voltage = 0.0f;

    char json[1200];
    snprintf(
        json, sizeof(json),
        "{\"temperature\":{\"current\":%.1f,\"target\":%.1f,\"hysteresis\":%.1f,"
        "\"min\":%.1f,\"minTimeEpoch\":%u,\"heaterEnabled\":%s},"
        "\"battery\":{\"voltage\":%.2f,\"percent\":%d},"
        "\"relays\":{\"light\":%s,\"pump\":%s,\"heater\":%s},"
        "\"servo\":{\"angle\":%d},"
        "\"schedule\":{\"dayStartHour\":%d,\"dayStartMin\":%d,\"dayEndHour\":%d,\"dayEndMin\":%d,"
        "\"airStartHour\":%d,\"airStartMin\":%d,\"airEndHour\":%d,\"airEndMin\":%d,"
        "\"filterStartHour\":%d,\"filterStartMin\":%d,\"filterEndHour\":%d,\"filterEndMin\":%d,"
        "\"servoPreOffMins\":%d},"
        "\"feeding\":{\"hour\":%d,\"minute\":%d,\"freq\":%d,\"lastFeedEpoch\":%u},"
        "\"network\":{\"ip\":\"%s\",\"apMode\":%s},"
        "\"settings\":{\"alwaysScreenOn\":%s},"
        "\"manual\":{\"lightOverride\":%s,\"lightState\":%s,\"filterOverride\":%s,\"filterState\":%s}}",
        isnan(snap.temperature) ? -99.9f : snap.temperature, cfg.targetTemp,
        cfg.tempHysteresis, isnan(snap.minTemp) ? 20.0f : snap.minTemp,
        (unsigned int)snap.minTempEpoch,
        cfg.heaterEnabled ? "true" : "false", voltage,
        (int)PowerManager::getBatteryPercent(),
        snap.isLightOn ? "true" : "false", snap.isFilterOn ? "true" : "false",
        snap.isHeaterOn ? "true" : "false", SystemController::getServoPosition(),
        cfg.dayStartHour, cfg.dayStartMinute, cfg.dayEndHour, cfg.dayEndMinute,
        cfg.aerationHourOn, cfg.aerationMinuteOn, cfg.aerationHourOff,
        cfg.aerationMinuteOff, cfg.filterHourOn, cfg.filterMinuteOn,
        cfg.filterHourOff, cfg.filterMinuteOff, cfg.servoPreOffMins,
        cfg.feedHour, cfg.feedMinute, cfg.feedMode,
        (unsigned int)cfg.lastFeedEpoch, AkwariumWifi::getIP().c_str(),
        AkwariumWifi::getIsAPMode() ? "true" : "false",
        cfg.alwaysScreenOn ? "true" : "false",
        SystemController::isManualLightOverrideEnabled() ? "true" : "false",
        SystemController::getManualLightState() ? "true" : "false",
        SystemController::isManualFilterOverrideEnabled() ? "true" : "false",
        SystemController::getManualFilterState() ? "true" : "false");

    server.sendHeader("Connection", "close");
    server.send(200, "application/json", json);
  });

  server.on("/api/logs", HTTP_GET, [&server]() {
    PowerManager::registerActivity();
    addCorsHeaders(server);
    String jsonLog = LogManager::getLogsAsJson();
    server.sendHeader("Connection", "close");
    server.send(200, "application/json", jsonLog);
  });

  server.on("/api/action", HTTP_POST, [&server]() {
    addCorsHeaders(server);
    if (!server.hasArg("action")) {
      server.send(400, "text/plain", "Bad Request");
      return;
    }

    PowerManager::registerActivity();
    String action = server.arg("action");

    if (action == "feed_now") {
      SystemController::feedNow();
      server.send(200, "text/plain", "OK");
      return;
    }

    if (action == "set_servo") {
      if (server.hasArg("angle")) {
        long angleRaw = 0;
        if (!parseLongStrict(server.arg("angle"), angleRaw)) {
          server.send(400, "text/plain", "Invalid angle");
          return;
        }
        if (angleRaw < 0 || angleRaw > 90) {
          server.send(400, "text/plain", "Angle out of range");
          return;
        }
        SystemController::setManualServo(static_cast<int>(angleRaw));
        server.send(200, "text/plain", "OK");
        return;
      }
      server.send(400, "text/plain", "Missing angle");
      return;
    }

    if (action == "clear_servo") {
      SystemController::clearManualServo();
      server.send(200, "text/plain", "OK");
      return;
    }

    if (action == "set_light") {
      if (server.hasArg("state")) {
        bool state = false;
        if (!parseBoolStrict(server.arg("state"), state)) {
          server.send(400, "text/plain", "Invalid state");
          return;
        }
        SystemController::setManualLight(state);
        server.send(200, "text/plain", "OK");
        return;
      }
      server.send(400, "text/plain", "Missing state");
      return;
    }

    if (action == "clear_light") {
      SystemController::clearManualLight();
      server.send(200, "text/plain", "OK");
      return;
    }

    if (action == "set_filter") {
      if (server.hasArg("state")) {
        bool state = false;
        if (!parseBoolStrict(server.arg("state"), state)) {
          server.send(400, "text/plain", "Invalid state");
          return;
        }
        SystemController::setManualFilter(state);
        server.send(200, "text/plain", "OK");
        return;
      }
      server.send(400, "text/plain", "Missing state");
      return;
    }

    if (action == "clear_filter") {
      SystemController::clearManualFilter();
      server.send(200, "text/plain", "OK");
      return;
    }

    if (action == "set_ap") {
      if (server.hasArg("state")) {
        bool state = false;
        if (!parseBoolStrict(server.arg("state"), state)) {
          server.send(400, "text/plain", "Invalid state");
          return;
        }
        if (state) {
          AkwariumWifi::startAP();
        } else {
          AkwariumWifi::stopAP();
        }
        server.send(200, "text/plain", "OK");
        return;
      }
      server.send(400, "text/plain", "Missing state");
      return;
    }

    if (action == "set_always_screen") {
      if (server.hasArg("state")) {
        bool state = false;
        if (!parseBoolStrict(server.arg("state"), state)) {
          server.send(400, "text/plain", "Invalid state");
          return;
        }
        Config cfg = ConfigManager::getCopy();
        cfg.alwaysScreenOn = state;
        if (!ConfigManager::updateAndSave(cfg)) {
          server.send(500, "text/plain", "Save failed");
          return;
        }
        server.send(200, "text/plain", "OK");
        return;
      }
      server.send(400, "text/plain", "Missing state");
      return;
    }

    if (action == "set_heater_enabled") {
      if (server.hasArg("state")) {
        bool state = false;
        if (!parseBoolStrict(server.arg("state"), state)) {
          server.send(400, "text/plain", "Invalid state");
          return;
        }
        Config cfg = ConfigManager::getCopy();
        cfg.heaterEnabled = state;
        if (!ConfigManager::updateAndSave(cfg)) {
          server.send(500, "text/plain", "Save failed");
          return;
        }
        server.send(200, "text/plain", "OK");
        return;
      }
      server.send(400, "text/plain", "Missing state");
      return;
    }

    if (action == "clear_critical_logs") {
      LogManager::clearCriticalLogs();
      server.send(200, "text/plain", "OK");
      return;
    }

    if (action == "save_schedule") {
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
      if (server.hasArg("alwaysScreenOn")) {
        bool state = false;
        if (parseBoolStrict(server.arg("alwaysScreenOn"), state)) {
          patch.hasAlwaysScreenOn = true;
          patch.alwaysScreenOn = state;
        } else {
          invalidFields++;
        }
      }
      if (server.hasArg("heaterEnabled")) {
        bool state = false;
        if (parseBoolStrict(server.arg("heaterEnabled"), state)) {
          patch.hasHeaterEnabled = true;
          patch.heaterEnabled = state;
        } else {
          invalidFields++;
        }
      }

      applyPatchAndSave(patch, invalidFields, server);
      return;
    }

    server.send(400, "text/plain", "Bad Request");
  });
}
