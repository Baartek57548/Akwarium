#include "WebApiProtocol.h"

#include "AkwariumWifi.h"
#include "ConfigManager.h"
#include "FirmwareInfo.h"
#include "LogManager.h"
#include "PowerManager.h"
#include "SharedState.h"
#include "SystemController.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

namespace {

static constexpr const char *WEB_API_SCHEMA_VERSION = "2026-04-30";

static const char *feedErrorToCode(Error err) {
  switch (err) {
  case Error::NONE:
    return "ok";
  case Error::BUSY:
    return "busy";
  case Error::SENSOR_NOT_OK:
    return "sensor_not_ok";
  case Error::TIMEOUT:
    return "timeout";
  default:
    return "unknown";
  }
}

static void appendJsonEscaped(String &json, const char *value) {
  const char *text = value != nullptr ? value : "";
  for (const char *p = text; *p != '\0'; ++p) {
    switch (*p) {
    case '\"':
      json += "\\\"";
      break;
    case '\\':
      json += "\\\\";
      break;
    case '\b':
      json += "\\b";
      break;
    case '\f':
      json += "\\f";
      break;
    case '\n':
      json += "\\n";
      break;
    case '\r':
      json += "\\r";
      break;
    case '\t':
      json += "\\t";
      break;
    default:
      if (static_cast<unsigned char>(*p) < 0x20) {
        char escaped[7];
        snprintf(escaped, sizeof(escaped), "\\u%04x",
                 static_cast<unsigned>(static_cast<unsigned char>(*p)));
        json += escaped;
      } else {
        json += *p;
      }
      break;
    }
  }
}

static void appendJsonQuoted(String &json, const char *value) {
  json += '"';
  appendJsonEscaped(json, value);
  json += '"';
}

static void appendJsonKey(String &json, const char *key) {
  appendJsonQuoted(json, key);
  json += ':';
}

static void appendUnsigned(String &json, unsigned long value) {
  char buffer[24];
  snprintf(buffer, sizeof(buffer), "%lu", value);
  json += buffer;
}

static void appendSigned(String &json, long value) {
  char buffer[24];
  snprintf(buffer, sizeof(buffer), "%ld", value);
  json += buffer;
}

static void trimFloatBuffer(char *buffer) {
  char *dot = strchr(buffer, '.');
  if (dot == nullptr) {
    return;
  }

  char *end = buffer + strlen(buffer) - 1;
  while (end > dot && *end == '0') {
    *end-- = '\0';
  }
  if (end == dot) {
    *end = '\0';
  }
}

static void appendFloat(String &json, float value, uint8_t decimals = 3) {
  char format[8];
  snprintf(format, sizeof(format), "%%.%uf", static_cast<unsigned>(decimals));
  char buffer[32];
  snprintf(buffer, sizeof(buffer), format, static_cast<double>(value));
  trimFloatBuffer(buffer);
  json += buffer;
}

static void appendJsonStringField(String &json, const char *key,
                                  const char *value, bool &firstField) {
  if (!firstField) {
    json += ',';
  }
  firstField = false;
  appendJsonKey(json, key);
  appendJsonQuoted(json, value);
}

static void appendJsonUIntField(String &json, const char *key,
                                unsigned long value, bool &firstField) {
  if (!firstField) {
    json += ',';
  }
  firstField = false;
  appendJsonKey(json, key);
  appendUnsigned(json, value);
}

static void appendJsonIntField(String &json, const char *key, long value,
                               bool &firstField) {
  if (!firstField) {
    json += ',';
  }
  firstField = false;
  appendJsonKey(json, key);
  appendSigned(json, value);
}

static void appendJsonFloatField(String &json, const char *key, float value,
                                 bool &firstField, uint8_t decimals = 3) {
  if (!firstField) {
    json += ',';
  }
  firstField = false;
  appendJsonKey(json, key);
  appendFloat(json, value, decimals);
}

static void appendJsonBoolField(String &json, const char *key, bool value,
                                bool &firstField) {
  if (!firstField) {
    json += ',';
  }
  firstField = false;
  appendJsonKey(json, key);
  json += value ? "true" : "false";
}

static void appendSleepBlockers(String &json, uint16_t flags) {
  bool first = true;
  auto appendBlocker = [&](const char *value) {
    if (!first) {
      json += ',';
    }
    first = false;
    appendJsonQuoted(json, value);
  };

  if ((flags & SLEEP_BLOCKER_IDLE_WINDOW) != 0U) {
    appendBlocker("idle_window");
  }
  if ((flags & SLEEP_BLOCKER_OUTPUTS_ACTIVE) != 0U) {
    appendBlocker("outputs_active");
  }
  if ((flags & SLEEP_BLOCKER_NOT_NIGHT) != 0U) {
    appendBlocker("not_night");
  }
  if ((flags & SLEEP_BLOCKER_OTA) != 0U) {
    appendBlocker("ota");
  }
  if ((flags & SLEEP_BLOCKER_AP_MODE) != 0U) {
    appendBlocker("ap_mode");
  }
  if ((flags & SLEEP_BLOCKER_SERVICE_MODE) != 0U) {
    appendBlocker("service_mode");
  }
  if ((flags & SLEEP_BLOCKER_TIME_SYNC) != 0U) {
    appendBlocker("time_sync");
  }
  if ((flags & SLEEP_BLOCKER_STA_ACTIVE) != 0U) {
    appendBlocker("sta_active");
  }
  if ((flags & SLEEP_BLOCKER_FEEDING) != 0U) {
    appendBlocker("feeding");
  }
}

} // namespace

String buildWebStatusJson(bool includeHistory) {
  const SharedStateData snap = SharedState::getSnapshot();
  const Config cfg = ConfigManager::getCopy();
  const FirmwareRuntimeInfo firmwareInfo = FirmwareInfo::getRuntimeInfo();
  const uint16_t sleepBlockers = SystemController::getCurrentLightSleepBlockers();
  const float voltage = isnan(PowerManager::getBatteryVoltage())
                            ? 0.0f
                            : PowerManager::getBatteryVoltage();
  const TemperatureHistoryCursor historyCursor =
      includeHistory ? SharedState::getTemperatureHistoryCursor()
                     : TemperatureHistoryCursor{0, 0, TEMP_HISTORY_SIZE};

  String json;
  json.reserve(includeHistory ? (5200U + (static_cast<size_t>(historyCursor.count) * 40U))
                              : 5200U);
  json += '{';

  bool rootFirst = true;
  appendJsonStringField(json, "schemaVersion", WEB_API_SCHEMA_VERSION, rootFirst);

  if (!rootFirst) {
    json += ',';
  }
  rootFirst = false;
  appendJsonKey(json, "temperature");
  json += '{';
  bool temperatureFirst = true;
  appendJsonFloatField(json, "current",
                       isnan(snap.temperature) ? -99.9f : snap.temperature,
                       temperatureFirst, 2);
  appendJsonFloatField(json, "target", cfg.targetTemp, temperatureFirst, 2);
  appendJsonFloatField(json, "threshold", cfg.targetTemp + cfg.tempHysteresis,
                       temperatureFirst, 2);
  appendJsonUIntField(json, "heaterMode", cfg.heaterMode, temperatureFirst);
  appendJsonFloatField(json, "hysteresis", cfg.tempHysteresis, temperatureFirst,
                       2);
  appendJsonFloatField(json, "min", isnan(snap.minTemp) ? -99.9f : snap.minTemp,
                       temperatureFirst, 2);
  appendJsonUIntField(json, "minTimeEpoch", snap.minTempEpoch, temperatureFirst);
  appendJsonUIntField(json, "historyIntervalMinutes",
                      TEMP_HISTORY_INTERVAL_SEC / 60UL, temperatureFirst);
  appendJsonUIntField(json, "historyCapacity", TEMP_HISTORY_SIZE,
                      temperatureFirst);
  if (includeHistory) {
    if (!temperatureFirst) {
      json += ',';
    }
    temperatureFirst = false;
    appendJsonKey(json, "history");
    json += '[';
    bool historyFirst = true;
    for (uint16_t i = 0; i < historyCursor.count; ++i) {
      TemperatureHistoryEntry entry = {};
      if (!SharedState::getTemperatureHistoryEntry(historyCursor, i, entry)) {
        continue;
      }
      if (isnan(entry.value)) {
        continue;
      }
      if (!historyFirst) {
        json += ',';
      }
      historyFirst = false;
      json += '{';
      bool entryFirst = true;
      appendJsonFloatField(json, "value", entry.value, entryFirst, 2);
      appendJsonUIntField(json, "epoch", entry.epoch, entryFirst);
      json += '}';
    }
    json += ']';
  }
  json += '}';

  json += ',';
  appendJsonKey(json, "battery");
  json += '{';
  bool batteryFirst = true;
  appendJsonFloatField(json, "voltage", voltage, batteryFirst, 2);
  appendJsonFloatField(json, "percent", PowerManager::getBatteryPercent(),
                       batteryFirst, 1);
  json += '}';

  json += ',';
  appendJsonKey(json, "display");
  json += '{';
  bool displayFirst = true;
  appendJsonBoolField(json, "alwaysScreenOn", cfg.alwaysScreenOn,
                      displayFirst);
  json += '}';

  json += ',';
  appendJsonKey(json, "relays");
  json += '{';
  bool relaysFirst = true;
  appendJsonBoolField(json, "light", snap.isLightOn, relaysFirst);
  appendJsonBoolField(json, "pump", snap.isFilterOn, relaysFirst);
  appendJsonBoolField(json, "heater", snap.isHeaterOn, relaysFirst);
  appendJsonUIntField(json, "aerationPercent", snap.aerationPercent,
                      relaysFirst);
  json += '}';

  json += ',';
  appendJsonKey(json, "schedule");
  json += '{';
  bool scheduleFirst = true;
  appendJsonUIntField(json, "lightMode", cfg.lightMode, scheduleFirst);
  appendJsonUIntField(json, "dayStartHour", cfg.dayStartHour, scheduleFirst);
  appendJsonUIntField(json, "dayStartMin", cfg.dayStartMinute, scheduleFirst);
  appendJsonUIntField(json, "dayEndHour", cfg.dayEndHour, scheduleFirst);
  appendJsonUIntField(json, "dayEndMin", cfg.dayEndMinute, scheduleFirst);
  appendJsonUIntField(json, "airMode", cfg.aerationMode, scheduleFirst);
  appendJsonUIntField(json, "airStartHour", cfg.aerationHourOn, scheduleFirst);
  appendJsonUIntField(json, "airStartMin", cfg.aerationMinuteOn, scheduleFirst);
  appendJsonUIntField(json, "airEndHour", cfg.aerationHourOff, scheduleFirst);
  appendJsonUIntField(json, "airEndMin", cfg.aerationMinuteOff, scheduleFirst);
  appendJsonUIntField(json, "filterMode", cfg.filterMode, scheduleFirst);
  appendJsonUIntField(json, "filterStartHour", cfg.filterHourOn, scheduleFirst);
  appendJsonUIntField(json, "filterStartMin", cfg.filterMinuteOn, scheduleFirst);
  appendJsonUIntField(json, "filterEndHour", cfg.filterHourOff, scheduleFirst);
  appendJsonUIntField(json, "filterEndMin", cfg.filterMinuteOff, scheduleFirst);
  appendJsonUIntField(json, "heaterMode", cfg.heaterMode, scheduleFirst);
  appendJsonUIntField(json, "servoPreOffMins", cfg.servoPreOffMins,
                      scheduleFirst);
  json += '}';

  json += ',';
  appendJsonKey(json, "feeding");
  json += '{';
  bool feedingFirst = true;
  appendJsonUIntField(json, "hour", cfg.feedHour, feedingFirst);
  appendJsonUIntField(json, "minute", cfg.feedMinute, feedingFirst);
  appendJsonUIntField(json, "freq", cfg.feedMode, feedingFirst);
  appendJsonUIntField(json, "lastFeedEpoch", cfg.lastFeedEpoch, feedingFirst);
  appendJsonBoolField(json, "active", SystemController::isFeedingNow(),
                      feedingFirst);
  appendJsonStringField(json, "lastResult",
                        feedErrorToCode(SystemController::getLastFeederError()),
                        feedingFirst);
  json += '}';

  json += ',';
  appendJsonKey(json, "network");
  json += '{';
  bool networkFirst = true;
  appendJsonStringField(json, "ip", AkwariumWifi::getIP().c_str(),
                        networkFirst);
  appendJsonBoolField(json, "apMode", AkwariumWifi::getIsAPMode(),
                      networkFirst);
  appendJsonStringField(json, "ssid", AkwariumWifi::getAPName().c_str(),
                        networkFirst);
  appendJsonUIntField(json, "clients", AkwariumWifi::getConnectedClients(),
                      networkFirst);
  appendJsonIntField(json, "rssi", AkwariumWifi::getStaRssi(), networkFirst);
  appendJsonBoolField(json, "staConnected", AkwariumWifi::isStaConnected(),
                      networkFirst);
  appendJsonBoolField(json, "staConnecting", AkwariumWifi::isStaConnecting(),
                      networkFirst);
  appendJsonBoolField(json, "serviceMode", AkwariumWifi::isServiceModeActive(),
                      networkFirst);
  appendJsonBoolField(json, "serviceModePending",
                      AkwariumWifi::isServiceModePending(), networkFirst);
  appendJsonStringField(json, "staSsid", AkwariumWifi::getStaSsid().c_str(),
                        networkFirst);
  appendJsonStringField(json, "configuredStaSsid",
                        AkwariumWifi::getConfiguredStaSsid().c_str(),
                        networkFirst);
  appendJsonStringField(json, "configuredApSsid",
                        AkwariumWifi::getConfiguredAPName().c_str(),
                        networkFirst);
  appendJsonUIntField(json, "staLastConnectedEpoch",
                      AkwariumWifi::getStaLastConnectedEpoch(), networkFirst);
  appendJsonBoolField(json, "apIdleCountdownActive",
                      AkwariumWifi::isApIdleCountdownActive(), networkFirst);
  appendJsonUIntField(json, "apIdleRemainingMs",
                      AkwariumWifi::getApIdleRemainingMs(), networkFirst);
  appendJsonBoolField(json, "timeSyncInProgress",
                      AkwariumWifi::isTimeSyncInProgress(), networkFirst);
  appendJsonUIntField(json, "lastTimeSyncEpoch",
                      AkwariumWifi::getLastTimeSyncEpoch(), networkFirst);
  appendJsonBoolField(json, "lastTimeSyncOk",
                      AkwariumWifi::wasLastTimeSyncSuccessful(), networkFirst);
  appendJsonStringField(json, "lastTimeSyncStatus",
                        AkwariumWifi::getLastTimeSyncStatus().c_str(),
                        networkFirst);
  json += '}';

  json += ',';
  appendJsonKey(json, "clock");
  json += '{';
  bool clockFirst = true;
  appendJsonIntField(json, "hour", snap.hour, clockFirst);
  appendJsonIntField(json, "minute", snap.minute, clockFirst);
  appendJsonIntField(json, "second", snap.second, clockFirst);
  appendJsonIntField(json, "day", snap.day, clockFirst);
  appendJsonIntField(json, "month", snap.month, clockFirst);
  appendJsonIntField(json, "year", snap.year, clockFirst);
  json += '}';

  json += ',';
  appendJsonKey(json, "firmware");
  json += '{';
  bool firmwareFirst = true;
  appendJsonStringField(json, "name", firmwareInfo.firmwareName, firmwareFirst);
  appendJsonStringField(json, "version", firmwareInfo.firmwareVersion,
                        firmwareFirst);
  appendJsonStringField(json, "buildRef", firmwareInfo.buildRef, firmwareFirst);
  appendJsonStringField(json, "buildDate", firmwareInfo.buildDate,
                        firmwareFirst);
  appendJsonStringField(json, "buildTime", firmwareInfo.buildTime,
                        firmwareFirst);
  appendJsonStringField(json, "idfVersion", firmwareInfo.idfVersion,
                        firmwareFirst);
  json += '}';

  json += ',';
  appendJsonKey(json, "web");
  json += '{';
  bool webFirst = true;
  appendJsonBoolField(json, "embeddedAssets", true, webFirst);
  appendJsonStringField(json, "settingsStylesheet",
                        "/vendor/tailwind.min.css", webFirst);
  appendJsonStringField(json, "settingsScript", "/vendor/alpine.min.js",
                        webFirst);
  json += '}';

  json += ',';
  appendJsonKey(json, "system");
  json += '{';
  bool systemFirst = true;
  appendJsonUIntField(json, "uptimeSeconds", SystemController::getUptimeSeconds(),
                      systemFirst);
  appendJsonUIntField(json, "resetCode", SystemController::getLastResetReason(),
                      systemFirst);
  appendJsonStringField(
      json, "resetReason",
      SystemController::getLastResetLabel() != nullptr
          ? SystemController::getLastResetLabel()
          : "unknown",
      systemFirst);
  appendJsonUIntField(json, "resetCount", SystemController::getResetCount(),
                      systemFirst);
  appendJsonStringField(json, "powerMode",
                        SystemController::getPowerModeLabel(), systemFirst);
  appendJsonBoolField(json, "sleepEligible", sleepBlockers == 0, systemFirst);
  if (!systemFirst) {
    json += ',';
  }
  appendJsonKey(json, "sleepBlockers");
  json += '[';
  appendSleepBlockers(json, sleepBlockers);
  json += ']';
  json += '}';

  json += '}';
  return json;
}

String buildWebLogsJson() { return LogManager::getLogsAsJson(); }

String buildWebActionResponseJson(bool success, const char *code,
                                  const char *message) {
  String json;
  json.reserve(160);
  json += '{';
  bool first = true;
  appendJsonStringField(json, "schemaVersion", WEB_API_SCHEMA_VERSION, first);
  appendJsonBoolField(json, "success", success, first);
  appendJsonStringField(json, "code",
                        (code != nullptr && code[0] != '\0') ? code : "unknown",
                        first);
  if (message != nullptr && message[0] != '\0') {
    appendJsonStringField(json, "message", message, first);
  }
  json += '}';
  return json;
}

void sendWebActionResponse(WebServer &server, int httpStatus, bool success,
                           const char *code, const char *message) {
  server.sendHeader("Connection", "close");
  server.send(httpStatus, "application/json",
              buildWebActionResponseJson(success, code, message));
}
