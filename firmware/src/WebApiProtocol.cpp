#include "WebApiProtocol.h"

#include "AkwariumWifi.h"
#include "BleManager.h"
#include "ConfigManager.h"
#include "FirmwareInfo.h"
#include "LogManager.h"
#include "PowerManager.h"
#include "SharedState.h"
#include "SystemController.h"

#include <ArduinoJson.h>

namespace {

static constexpr const char *WEB_API_SCHEMA_VERSION = "2026-04-01";

} // namespace

String buildWebStatusJson() {
  const SharedStateData snap = SharedState::getSnapshot();
  const Config cfg = ConfigManager::getCopy();

  float voltage = PowerManager::getBatteryVoltage();
  if (isnan(voltage)) {
    voltage = 0.0f;
  }

  DynamicJsonDocument doc(5376);
  doc["schemaVersion"] = WEB_API_SCHEMA_VERSION;

  JsonObject temperature = doc.createNestedObject("temperature");
  temperature["current"] = isnan(snap.temperature) ? -99.9f : snap.temperature;
  temperature["target"] = cfg.targetTemp;
  temperature["threshold"] = cfg.targetTemp + cfg.tempHysteresis;
  temperature["heaterMode"] = cfg.heaterMode;
  temperature["hysteresis"] = cfg.tempHysteresis;
  temperature["min"] = isnan(snap.minTemp) ? -99.9f : snap.minTemp;
  temperature["minTimeEpoch"] = snap.minTempEpoch;
  temperature["historyIntervalMinutes"] = 10;
  temperature["historyCapacity"] = TEMP_HISTORY_SIZE;
  JsonArray history = temperature.createNestedArray("history");
  for (uint8_t i = 0; i < snap.temperatureHistoryCount; ++i) {
    const TemperatureHistoryEntry &entry = snap.temperatureHistory[i];
    if (isnan(entry.value)) {
      continue;
    }
    JsonObject item = history.createNestedObject();
    item["value"] = entry.value;
    item["epoch"] = entry.epoch;
  }

  JsonObject battery = doc.createNestedObject("battery");
  battery["voltage"] = voltage;
  battery["percent"] = PowerManager::getBatteryPercent();

  JsonObject relays = doc.createNestedObject("relays");
  relays["light"] = snap.isLightOn;
  relays["pump"] = snap.isFilterOn;
  relays["heater"] = snap.isHeaterOn;
  relays["aerationPercent"] = snap.aerationPercent;

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
  feeding["active"] = SystemController::isFeedingNow();

  JsonObject network = doc.createNestedObject("network");
  network["ip"] = AkwariumWifi::getIP();
  network["apMode"] = AkwariumWifi::getIsAPMode();
  network["ssid"] = AkwariumWifi::getAPName();
  network["clients"] = AkwariumWifi::getConnectedClients();
  network["staConnected"] = AkwariumWifi::isStaConnected();
  network["staConnecting"] = AkwariumWifi::isStaConnecting();
  network["serviceMode"] = AkwariumWifi::isServiceModeActive();
  network["staSsid"] = AkwariumWifi::getStaSsid();
  network["configuredStaSsid"] = AkwariumWifi::getConfiguredStaSsid();
  network["configuredApSsid"] = AkwariumWifi::getConfiguredAPName();
  network["staLastConnectedEpoch"] = AkwariumWifi::getStaLastConnectedEpoch();
  network["timeSyncInProgress"] = AkwariumWifi::isTimeSyncInProgress();
  network["lastTimeSyncEpoch"] = AkwariumWifi::getLastTimeSyncEpoch();
  network["lastTimeSyncOk"] = AkwariumWifi::wasLastTimeSyncSuccessful();
  network["lastTimeSyncStatus"] = AkwariumWifi::getLastTimeSyncStatus();
  const bool bleAdvertising = BleManager::isAdvertising();
  const bool bleConnected = BleManager::isConnected();
  network["bleAdvertising"] = bleAdvertising;
  network["bleConnected"] = bleConnected;
  network["bleActive"] = bleAdvertising || bleConnected;

  JsonObject clock = doc.createNestedObject("clock");
  clock["hour"] = snap.hour;
  clock["minute"] = snap.minute;
  clock["second"] = snap.second;
  clock["day"] = snap.day;
  clock["month"] = snap.month;
  clock["year"] = snap.year;

  const FirmwareRuntimeInfo firmwareInfo = FirmwareInfo::getRuntimeInfo();
  JsonObject firmware = doc.createNestedObject("firmware");
  firmware["name"] = firmwareInfo.firmwareName;
  firmware["version"] = firmwareInfo.firmwareVersion;
  firmware["buildRef"] = firmwareInfo.buildRef;
  firmware["buildDate"] = firmwareInfo.buildDate;
  firmware["buildTime"] = firmwareInfo.buildTime;
  firmware["idfVersion"] = firmwareInfo.idfVersion;

  String json;
  json.reserve(3712);
  serializeJson(doc, json);
  return json;
}

String buildWebLogsJson() { return LogManager::getLogsAsJson(); }

String buildWebActionResponseJson(bool success, const char *code,
                                  const char *message) {
  DynamicJsonDocument doc(256);
  doc["schemaVersion"] = WEB_API_SCHEMA_VERSION;
  doc["success"] = success;
  doc["code"] = (code != nullptr && code[0] != '\0') ? code : "unknown";
  if (message != nullptr && message[0] != '\0') {
    doc["message"] = message;
  }

  String json;
  json.reserve(128);
  serializeJson(doc, json);
  return json;
}

void sendWebActionResponse(WebServer &server, int httpStatus, bool success,
                           const char *code, const char *message) {
  server.sendHeader("Connection", "close");
  server.send(httpStatus, "application/json",
              buildWebActionResponseJson(success, code, message));
}
