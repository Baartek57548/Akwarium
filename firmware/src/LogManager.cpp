#include "LogManager.h"

#include <Preferences.h>
#include <RTClib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <string.h>
#include <time.h>

namespace {

static constexpr const char *LOG_SCHEMA_VERSION = "2026-04-05";

static Preferences logPrefs;
static SemaphoreHandle_t logMutex = nullptr;
static portMUX_TYPE logMutexInitMux = portMUX_INITIALIZER_UNLOCKED;
static bool persistentCriticalLogsEnabled = true;
static bool persistentCriticalLogsErrorPrinted = false;

static const char *levelToLabel(LogLevel level) {
  switch (level) {
  case LogLevel::Info:
    return "INFO";
  case LogLevel::Warning:
    return "WARN";
  case LogLevel::Error:
    return "ERROR";
  default:
    return "INFO";
  }
}

static const char *levelToJsonValue(LogLevel level) {
  switch (level) {
  case LogLevel::Info:
    return "info";
  case LogLevel::Warning:
    return "warning";
  case LogLevel::Error:
    return "error";
  default:
    return "info";
  }
}

static const char *defaultCodeForLevel(LogLevel level) {
  switch (level) {
  case LogLevel::Info:
    return "info";
  case LogLevel::Warning:
    return "warning";
  case LogLevel::Error:
    return "error";
  default:
    return "info";
  }
}

static void copyLogText(char *dest, size_t destSize, const char *src) {
  if (dest == nullptr || destSize == 0) {
    return;
  }

  snprintf(dest, destSize, "%s", src != nullptr ? src : "");
}

static void disablePersistentCriticalLogs(const char *reason) {
  persistentCriticalLogsEnabled = false;
  if (!persistentCriticalLogsErrorPrinted) {
    Serial.print("[LOGS] Persistent critical logs disabled: ");
    Serial.println(reason != nullptr ? reason : "unknown");
    persistentCriticalLogsErrorPrinted = true;
  }
}

static DateTime getLogDateTime() {
  const time_t now = time(nullptr);
  if (now > 1700000000) {
    return DateTime(static_cast<uint32_t>(now));
  }

  // Fallback bez dotykania RTC/I2C z wielu zadan.
  return DateTime(2025, 1, 1, 0, 0, 0) +
         TimeSpan(static_cast<int32_t>(millis() / 1000UL));
}

static uint32_t getLogEpoch() {
  const time_t now = time(nullptr);
  if (now > 1700000000) {
    return static_cast<uint32_t>(now);
  }
  return static_cast<uint32_t>(getLogDateTime().unixtime());
}

static String escapeJsonString(const String &input) {
  String out;
  out.reserve(input.length() + 8);

  for (size_t i = 0; i < input.length(); i++) {
    char c = input[i];
    switch (c) {
    case '\"':
      out += "\\\"";
      break;
    case '\\':
      out += "\\\\";
      break;
    case '\n':
      out += "\\n";
      break;
    case '\r':
      out += "\\r";
      break;
    case '\t':
      out += "\\t";
      break;
    default:
      if (static_cast<uint8_t>(c) < 0x20) {
        out += '?';
      } else {
        out += c;
      }
      break;
    }
  }

  return out;
}

static bool getRingEntry(const LogEntrySnapshot *buffer, int count, int head,
                         int capacity, uint8_t indexFromOldest,
                         LogEntrySnapshot &entryOut) {
  if (buffer == nullptr || count <= 0 || indexFromOldest >= count) {
    return false;
  }

  const int startIdx = (count < capacity) ? 0 : head;
  const int idx = (startIdx + indexFromOldest) % capacity;
  entryOut = buffer[idx];
  return true;
}

static void appendRingEntry(LogEntrySnapshot *buffer, int capacity, int &head,
                            int &count, LogLevel level, const char *code,
                            const char *message) {
  if (buffer == nullptr || capacity <= 0) {
    return;
  }

  LogEntrySnapshot &entry = buffer[head];
  entry.epoch = getLogEpoch();
  entry.level = level;
  copyLogText(entry.code, sizeof(entry.code),
              (code != nullptr && code[0] != '\0') ? code
                                                   : defaultCodeForLevel(level));
  copyLogText(entry.message, sizeof(entry.message), message);

  head = (head + 1) % capacity;
  if (count < capacity) {
    count++;
  }
}

static void formatClockFromEpoch(uint32_t epoch, char *buffer,
                                 size_t bufferSize) {
  if (buffer == nullptr || bufferSize == 0) {
    return;
  }

  if (epoch > 0) {
    const DateTime logTime(epoch);
    snprintf(buffer, bufferSize, "%02d:%02d", logTime.hour(), logTime.minute());
    return;
  }

  snprintf(buffer, bufferSize, "%s", "--:--");
}

static void formatStampFromEpoch(uint32_t epoch, char *buffer,
                                 size_t bufferSize) {
  if (buffer == nullptr || bufferSize == 0) {
    return;
  }

  if (epoch > 0) {
    const DateTime logTime(epoch);
    snprintf(buffer, bufferSize, "%04d-%02d-%02d %02d:%02d:%02d",
             logTime.year(), logTime.month(), logTime.day(), logTime.hour(),
             logTime.minute(), logTime.second());
    return;
  }

  snprintf(buffer, bufferSize, "%s", "unknown-time");
}

static void formatLegacyMessage(const LogEntrySnapshot &entry, char *buffer,
                                size_t bufferSize) {
  if (buffer == nullptr || bufferSize == 0) {
    return;
  }

  snprintf(buffer, bufferSize, "%s: %s", levelToLabel(entry.level),
           entry.message);
}

static void appendLogEntryJson(String &json, const LogEntrySnapshot &entry) {
  json += "{\"ts\":";
  json += String(entry.epoch);
  json += ",\"level\":\"";
  json += levelToJsonValue(entry.level);
  json += "\",\"code\":\"";
  json += escapeJsonString(String(entry.code));
  json += "\",\"message\":\"";
  json += escapeJsonString(String(entry.message));
  json += "\"}";
}

static void appendLogArrayJson(String &json, const LogEntrySnapshot *buffer,
                               int count, int head, int capacity) {
  const int startIdx = (count < capacity) ? 0 : head;
  for (int i = 0; i < count; i++) {
    if (i > 0) {
      json += ",";
    }

    const int idx = (startIdx + i) % capacity;
    appendLogEntryJson(json, buffer[idx]);
  }
}

static void appendLogEntryText(String &text, const LogEntrySnapshot &entry) {
  char stamp[24];
  formatStampFromEpoch(entry.epoch, stamp, sizeof(stamp));
  text += stamp;
  text += " ";
  text += levelToLabel(entry.level);
  text += " ";
  text += entry.code[0] != '\0' ? entry.code : defaultCodeForLevel(entry.level);
  text += " ";
  text += entry.message;
  text += "\r\n";
}

static void appendLogGroupText(String &text, const char *title,
                               const LogEntrySnapshot *buffer, int count,
                               int head, int capacity) {
  text += title;
  text += "\r\n";

  const int startIdx = (count < capacity) ? 0 : head;
  for (int i = 0; i < count; i++) {
    const int idx = (startIdx + i) % capacity;
    appendLogEntryText(text, buffer[idx]);
  }
}

} // namespace

LogEntrySnapshot LogManager::webLogs[WEB_MAX_LOGS];
int LogManager::webLogsHead = 0;
int LogManager::webLogsCount = 0;

LogEntrySnapshot LogManager::criticalLogs[MAX_CRITICAL_LOGS];
int LogManager::criticalLogsCount = 0;
int LogManager::criticalLogsHead = 0;

void LogManager::loadCriticalLogs() {
  criticalLogsCount = logPrefs.getInt("critCount", 0);
  criticalLogsHead = logPrefs.getInt("critHead", 0);

  if (criticalLogsCount > MAX_CRITICAL_LOGS) {
    criticalLogsCount = MAX_CRITICAL_LOGS;
  }
  if (criticalLogsHead >= MAX_CRITICAL_LOGS) {
    criticalLogsHead = 0;
  }

  for (int i = 0; i < criticalLogsCount; i++) {
    char key[16];
    snprintf(key, sizeof(key), "critLog%d", i);
    const size_t sz =
        logPrefs.getBytes(key, &criticalLogs[i], sizeof(LogEntrySnapshot));
    if (sz != sizeof(LogEntrySnapshot)) {
      criticalLogs[i] = {};
    }
  }
}

void LogManager::saveCriticalLog(const LogEntrySnapshot &log, int index) {
  if (!persistentCriticalLogsEnabled) {
    return;
  }

  char key[16];
  snprintf(key, sizeof(key), "critLog%d", index);
  const size_t bytesWritten =
      logPrefs.putBytes(key, &log, sizeof(LogEntrySnapshot));
  if (bytesWritten != sizeof(LogEntrySnapshot)) {
    disablePersistentCriticalLogs("nvs_set_blob failed");
  }
}

bool LogManager::ensureMutex() {
  if (logMutex != nullptr) {
    return true;
  }

  portENTER_CRITICAL(&logMutexInitMux);
  if (logMutex == nullptr) {
    logMutex = xSemaphoreCreateMutex();
  }
  portEXIT_CRITICAL(&logMutexInitMux);

  return logMutex != nullptr;
}

void LogManager::init() {
  ensureMutex();
  persistentCriticalLogsEnabled = logPrefs.begin("Akwarium", false);
  persistentCriticalLogsErrorPrinted = false;
  if (!persistentCriticalLogsEnabled) {
    disablePersistentCriticalLogs("preferences.begin failed");
    return;
  }

  if (logMutex != nullptr &&
      xSemaphoreTake(logMutex, pdMS_TO_TICKS(250)) == pdTRUE) {
    loadCriticalLogs();
    xSemaphoreGive(logMutex);
  } else {
    loadCriticalLogs();
  }
}

void LogManager::appendWebLog(LogLevel level, const char *code,
                              const char *msg) {
  appendRingEntry(webLogs, WEB_MAX_LOGS, webLogsHead, webLogsCount, level, code,
                  msg);
}

void LogManager::logInfo(const char *msg) {
  Serial.print("[INFO] ");
  Serial.println(msg);
  if (ensureMutex() && logMutex != nullptr &&
      xSemaphoreTake(logMutex, pdMS_TO_TICKS(250)) == pdTRUE) {
    appendWebLog(LogLevel::Info, "info", msg);
    xSemaphoreGive(logMutex);
  }
}

void LogManager::logWarn(const char *msg) {
  Serial.print("[WARN] ");
  Serial.println(msg);
  if (ensureMutex() && logMutex != nullptr &&
      xSemaphoreTake(logMutex, pdMS_TO_TICKS(250)) == pdTRUE) {
    appendWebLog(LogLevel::Warning, "warning", msg);
    xSemaphoreGive(logMutex);
  }
}

void LogManager::logError(const char *msg) {
  Serial.print("[ERROR] ");
  Serial.println(msg);
  if (ensureMutex() && logMutex != nullptr &&
      xSemaphoreTake(logMutex, pdMS_TO_TICKS(250)) == pdTRUE) {
    appendWebLog(LogLevel::Error, "error", msg);

    appendRingEntry(criticalLogs, MAX_CRITICAL_LOGS, criticalLogsHead,
                    criticalLogsCount, LogLevel::Error, "error", msg);
    const int savedIndex =
        (criticalLogsHead + MAX_CRITICAL_LOGS - 1) % MAX_CRITICAL_LOGS;
    if (persistentCriticalLogsEnabled) {
      saveCriticalLog(criticalLogs[savedIndex], savedIndex);
    }

    if (persistentCriticalLogsEnabled) {
      const size_t countWritten = logPrefs.putInt("critCount", criticalLogsCount);
      const size_t headWritten = logPrefs.putInt("critHead", criticalLogsHead);
      if (countWritten != sizeof(int32_t) || headWritten != sizeof(int32_t)) {
        disablePersistentCriticalLogs("nvs_set_i32 failed");
      }
    }
    xSemaphoreGive(logMutex);
  }
}

void LogManager::clearCriticalLogs() {
  if (ensureMutex() && logMutex != nullptr &&
      xSemaphoreTake(logMutex, pdMS_TO_TICKS(250)) == pdTRUE) {
    criticalLogsCount = 0;
    criticalLogsHead = 0;
    memset(criticalLogs, 0, sizeof(criticalLogs));
    if (persistentCriticalLogsEnabled) {
      logPrefs.putInt("critCount", 0);
      logPrefs.putInt("critHead", 0);
    }
    xSemaphoreGive(logMutex);
  }
  Serial.println("[LOGS] WYCZYSZCZONO LOGI KRYTYCZNE");
}

void LogManager::clearNormalLogs() {
  if (ensureMutex() && logMutex != nullptr &&
      xSemaphoreTake(logMutex, pdMS_TO_TICKS(250)) == pdTRUE) {
    webLogsCount = 0;
    webLogsHead = 0;
    memset(webLogs, 0, sizeof(webLogs));
    xSemaphoreGive(logMutex);
  }
  Serial.println("[LOGS] WYCZYSZCZONO LOGI ZWYKLE");
}

uint8_t LogManager::getNormalLogsCount() {
  uint8_t count = static_cast<uint8_t>(webLogsCount);
  if (ensureMutex() && logMutex != nullptr &&
      xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    count = static_cast<uint8_t>(webLogsCount);
    xSemaphoreGive(logMutex);
  }
  return count;
}

uint8_t LogManager::getCriticalLogsCount() {
  uint8_t count = static_cast<uint8_t>(criticalLogsCount);
  if (ensureMutex() && logMutex != nullptr &&
      xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    count = static_cast<uint8_t>(criticalLogsCount);
    xSemaphoreGive(logMutex);
  }
  return count;
}

bool LogManager::getNormalLogEntryAt(uint8_t indexFromOldest,
                                     LogEntrySnapshot &entryOut) {
  if (!(ensureMutex() && logMutex != nullptr &&
        xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) == pdTRUE)) {
    return false;
  }

  const bool ok = getRingEntry(webLogs, webLogsCount, webLogsHead, WEB_MAX_LOGS,
                               indexFromOldest, entryOut);
  xSemaphoreGive(logMutex);
  return ok;
}

bool LogManager::getCriticalLogEntryAt(uint8_t indexFromOldest,
                                       LogEntrySnapshot &entryOut) {
  if (!(ensureMutex() && logMutex != nullptr &&
        xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) == pdTRUE)) {
    return false;
  }

  const bool ok = getRingEntry(criticalLogs, criticalLogsCount, criticalLogsHead,
                               MAX_CRITICAL_LOGS, indexFromOldest, entryOut);
  xSemaphoreGive(logMutex);
  return ok;
}

bool LogManager::getNormalLogAt(uint8_t indexFromOldest, char *messageOut,
                                size_t messageOutSize, char *timeOut,
                                size_t timeOutSize) {
  LogEntrySnapshot entry = {};
  if (!getNormalLogEntryAt(indexFromOldest, entry)) {
    return false;
  }

  formatLegacyMessage(entry, messageOut, messageOutSize);
  formatClockFromEpoch(entry.epoch, timeOut, timeOutSize);
  return true;
}

bool LogManager::getCriticalLogAt(uint8_t indexFromOldest, char *messageOut,
                                  size_t messageOutSize, char *timeOut,
                                  size_t timeOutSize) {
  LogEntrySnapshot entry = {};
  if (!getCriticalLogEntryAt(indexFromOldest, entry)) {
    return false;
  }

  if (messageOut != nullptr && messageOutSize > 0) {
    snprintf(messageOut, messageOutSize, "%s", entry.message);
  }
  formatClockFromEpoch(entry.epoch, timeOut, timeOutSize);
  return true;
}

String LogManager::getLogsAsJson() {
  if (!(ensureMutex() && logMutex != nullptr &&
        xSemaphoreTake(logMutex, pdMS_TO_TICKS(200)) == pdTRUE)) {
    return "{\"schemaVersion\":\"2026-04-05\",\"counts\":{\"normal\":0,\"critical\":0},\"normal\":[],\"critical\":[]}";
  }

  String jsonLog;
  jsonLog.reserve(6144);
  jsonLog += "{\"schemaVersion\":\"";
  jsonLog += LOG_SCHEMA_VERSION;
  jsonLog += "\",\"counts\":{\"normal\":";
  jsonLog += String(webLogsCount);
  jsonLog += ",\"critical\":";
  jsonLog += String(criticalLogsCount);
  jsonLog += "},\"normal\":[";
  appendLogArrayJson(jsonLog, webLogs, webLogsCount, webLogsHead, WEB_MAX_LOGS);
  jsonLog += "],\"critical\":[";
  appendLogArrayJson(jsonLog, criticalLogs, criticalLogsCount, criticalLogsHead,
                     MAX_CRITICAL_LOGS);
  jsonLog += "]}";

  xSemaphoreGive(logMutex);
  return jsonLog;
}

String LogManager::getLogsAsText(const char *type) {
  if (!(ensureMutex() && logMutex != nullptr &&
        xSemaphoreTake(logMutex, pdMS_TO_TICKS(200)) == pdTRUE)) {
    return "No logs available.\r\n";
  }

  const String requestedType = type != nullptr ? String(type) : String("all");
  const bool normalOnly = requestedType.equalsIgnoreCase("normal");
  const bool criticalOnly = requestedType.equalsIgnoreCase("critical");

  String text;
  text.reserve(4096);

  if (!criticalOnly) {
    appendLogGroupText(text, "[NORMAL]", webLogs, webLogsCount, webLogsHead,
                       WEB_MAX_LOGS);
  }

  if (!normalOnly) {
    if (text.length() > 0) {
      text += "\r\n";
    }
    appendLogGroupText(text, "[CRITICAL]", criticalLogs, criticalLogsCount,
                       criticalLogsHead, MAX_CRITICAL_LOGS);
  }

  xSemaphoreGive(logMutex);
  return text;
}
