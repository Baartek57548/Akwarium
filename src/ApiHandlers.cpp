#include "ApiHandlers.h"
#include "AkwariumWifi.h"
#include "ConfigManager.h"
#include "LogManager.h"
#include "PowerManager.h"
#include "SharedState.h"
#include "SystemController.h"
#include <WebServer.h>

extern WebServer server; // from AkwariumWifi

void setupApiEndpoints() {
  WebServer &server = AkwariumWifi::getServer();

  server.on("/api/status", HTTP_GET, [&server]() {
    const SharedStateData snap = SharedState::getSnapshot();
    const Config cfg = ConfigManager::getConfigSnapshot();

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
        SystemController::feedNow();
        server.send(200, "text/plain", "OK");
        return;
      } else if (action == "set_servo") {
        if (server.hasArg("angle")) {
          int ang = constrain(server.arg("angle").toInt(), 0, 90);
          SystemController::setManualServo(ang);
          server.send(200, "text/plain", "OK");
          return;
        }
      } else if (action == "clear_servo") {
        SystemController::clearManualServo();
        server.send(200, "text/plain", "OK");
        return;
      } else if (action == "clear_critical_logs") {
        LogManager::clearCriticalLogs();
        server.send(200, "text/plain", "OK");
        return;
      } else if (action == "save_schedule") {
        Config cfg = ConfigManager::getConfigSnapshot();
        bool updated = false;

        auto parseTimeArg = [](const String &value, int &hour,
                               int &minute) -> bool {
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
        };

        if (server.hasArg("feedTime")) {
          String ft = server.arg("feedTime");
          int h = 0;
          int m = 0;
          if (!parseTimeArg(ft, h, m)) {
            server.send(400, "text/plain", "Invalid feedTime");
            return;
          }
          cfg.feedHour = constrain(h, 0, 23);
          cfg.feedMinute = constrain(m, 0, 59);
          updated = true;
        }
        if (server.hasArg("feedFreq")) {
          cfg.feedMode = constrain(server.arg("feedFreq").toInt(), 0, 3);
          updated = true;
        }
        if (server.hasArg("dayStart")) {
          String ds = server.arg("dayStart");
          int h = 0;
          int m = 0;
          if (!parseTimeArg(ds, h, m)) {
            server.send(400, "text/plain", "Invalid dayStart");
            return;
          }
          cfg.dayStartHour = constrain(h, 0, 24);
          cfg.dayStartMinute = constrain(m, 0, 59);
          updated = true;
        }
        if (server.hasArg("dayEnd")) {
          String de = server.arg("dayEnd");
          int h = 0;
          int m = 0;
          if (!parseTimeArg(de, h, m)) {
            server.send(400, "text/plain", "Invalid dayEnd");
            return;
          }
          cfg.dayEndHour = constrain(h, 0, 24);
          cfg.dayEndMinute = constrain(m, 0, 59);
          updated = true;
        }
        if (server.hasArg("targetTemp")) {
          float tTemp = server.arg("targetTemp").toFloat();
          cfg.targetTemp = constrain(tTemp, 15.0f, 35.0f);
          updated = true;
        }
        if (server.hasArg("tempHyst")) {
          float hTemp = server.arg("tempHyst").toFloat();
          cfg.tempHysteresis = constrain(hTemp, 0.1f, 5.0f);
          updated = true;
        }
        if (server.hasArg("airOn")) {
          String ao = server.arg("airOn");
          int h = 0;
          int m = 0;
          if (!parseTimeArg(ao, h, m)) {
            server.send(400, "text/plain", "Invalid airOn");
            return;
          }
          cfg.aerationHourOn = constrain(h, 0, 23);
          cfg.aerationMinuteOn = constrain(m, 0, 59);
          updated = true;
        }
        if (server.hasArg("airOff")) {
          String af = server.arg("airOff");
          int h = 0;
          int m = 0;
          if (!parseTimeArg(af, h, m)) {
            server.send(400, "text/plain", "Invalid airOff");
            return;
          }
          cfg.aerationHourOff = constrain(h, 0, 23);
          cfg.aerationMinuteOff = constrain(m, 0, 59);
          updated = true;
        }
        if (server.hasArg("filterOn")) {
          String fo = server.arg("filterOn");
          int h = 0;
          int m = 0;
          if (!parseTimeArg(fo, h, m)) {
            server.send(400, "text/plain", "Invalid filterOn");
            return;
          }
          cfg.filterHourOn = constrain(h, 0, 23);
          cfg.filterMinuteOn = constrain(m, 0, 59);
          updated = true;
        }
        if (server.hasArg("filterOff")) {
          String ff = server.arg("filterOff");
          int h = 0;
          int m = 0;
          if (!parseTimeArg(ff, h, m)) {
            server.send(400, "text/plain", "Invalid filterOff");
            return;
          }
          cfg.filterHourOff = constrain(h, 0, 23);
          cfg.filterMinuteOff = constrain(m, 0, 59);
          updated = true;
        }
        if (server.hasArg("servoPreOffMins")) {
          cfg.servoPreOffMins =
              constrain(server.arg("servoPreOffMins").toInt(), 0, 255);
          updated = true;
        }

        if (!updated) {
          server.send(400, "text/plain", "No valid fields");
          return;
        }

        ConfigManager::saveConfig(cfg);
        server.send(200, "text/plain", "OK");
        return;
      }
    }
    server.send(400, "text/plain", "Bad Request");
  });
}
