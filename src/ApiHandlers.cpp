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
        isnan(SharedState::getSnapshot().temperature)
            ? -99.9
            : SharedState::getSnapshot().temperature,
        ConfigManager::getConfig().targetTemp,
        ConfigManager::getConfig().tempHysteresis,
        isnan(SharedState::getSnapshot().minTemp)
            ? 20.0
            : SharedState::getSnapshot().minTemp,
        (unsigned int)SharedState::getSnapshot().minTempEpoch, voltage,
        (int)PowerManager::getBatteryPercent(),
        SharedState::getSnapshot().isLightOn ? "true" : "false",
        SharedState::getSnapshot().isFilterOn ? "true" : "false",
        SystemController::getServoPosition(),
        ConfigManager::getConfig().dayStartHour,
        ConfigManager::getConfig().dayStartMinute,
        ConfigManager::getConfig().dayEndHour,
        ConfigManager::getConfig().dayEndMinute,
        ConfigManager::getConfig().aerationHourOn,
        ConfigManager::getConfig().aerationMinuteOn,
        ConfigManager::getConfig().aerationHourOff,
        ConfigManager::getConfig().aerationMinuteOff,
        ConfigManager::getConfig().filterHourOn,
        ConfigManager::getConfig().filterMinuteOn,
        ConfigManager::getConfig().filterHourOff,
        ConfigManager::getConfig().filterMinuteOff,
        ConfigManager::getConfig().servoPreOffMins,
        ConfigManager::getConfig().feedHour,
        ConfigManager::getConfig().feedMinute,
        ConfigManager::getConfig().feedMode,
        (unsigned int)ConfigManager::getConfig().lastFeedEpoch,
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
          int ang = server.arg("angle").toInt();
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
        if (server.hasArg("feedTime")) {
          String ft = server.arg("feedTime");
          if (ft.length() >= 5) {
            ConfigManager::getConfig().feedHour = ft.substring(0, 2).toInt();
            ConfigManager::getConfig().feedMinute = ft.substring(3, 5).toInt();
          }
        }
        if (server.hasArg("feedFreq")) {
          ConfigManager::getConfig().feedMode = server.arg("feedFreq").toInt();
        }
        if (server.hasArg("dayStart")) {
          String ds = server.arg("dayStart");
          if (ds.length() >= 5) {
            ConfigManager::getConfig().dayStartHour =
                ds.substring(0, 2).toInt();
            ConfigManager::getConfig().dayStartMinute =
                ds.substring(3, 5).toInt();
          }
        }
        if (server.hasArg("dayEnd")) {
          String de = server.arg("dayEnd");
          if (de.length() >= 5) {
            ConfigManager::getConfig().dayEndHour = de.substring(0, 2).toInt();
            ConfigManager::getConfig().dayEndMinute =
                de.substring(3, 5).toInt();
          }
        }
        if (server.hasArg("targetTemp")) {
          float tTemp = server.arg("targetTemp").toFloat();
          ConfigManager::getConfig().targetTemp =
              constrain(tTemp, 15.0f, 35.0f);
        }
        if (server.hasArg("tempHyst")) {
          float hTemp = server.arg("tempHyst").toFloat();
          ConfigManager::getConfig().tempHysteresis =
              constrain(hTemp, 0.1f, 5.0f);
        }
        if (server.hasArg("airOn")) {
          String ao = server.arg("airOn");
          if (ao.length() >= 5) {
            ConfigManager::getConfig().aerationHourOn =
                ao.substring(0, 2).toInt();
            ConfigManager::getConfig().aerationMinuteOn =
                ao.substring(3, 5).toInt();
          }
        }
        if (server.hasArg("airOff")) {
          String af = server.arg("airOff");
          if (af.length() >= 5) {
            ConfigManager::getConfig().aerationHourOff =
                af.substring(0, 2).toInt();
            ConfigManager::getConfig().aerationMinuteOff =
                af.substring(3, 5).toInt();
          }
        }
        if (server.hasArg("filterOn")) {
          String fo = server.arg("filterOn");
          if (fo.length() >= 5) {
            ConfigManager::getConfig().filterHourOn =
                fo.substring(0, 2).toInt();
            ConfigManager::getConfig().filterMinuteOn =
                fo.substring(3, 5).toInt();
          }
        }
        if (server.hasArg("filterOff")) {
          String ff = server.arg("filterOff");
          if (ff.length() >= 5) {
            ConfigManager::getConfig().filterHourOff =
                ff.substring(0, 2).toInt();
            ConfigManager::getConfig().filterMinuteOff =
                ff.substring(3, 5).toInt();
          }
        }
        if (server.hasArg("servoPreOffMins")) {
          ConfigManager::getConfig().servoPreOffMins =
              server.arg("servoPreOffMins").toInt();
        }
        ConfigManager::save();
        server.send(200, "text/plain", "OK");
        return;
      }
    }
    server.send(400, "text/plain", "Bad Request");
  });
}
