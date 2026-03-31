#include "LogManager.h"
#include <Preferences.h>
#include <RTClib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <time.h>

String LogManager::webLogs[WEB_MAX_LOGS];
int LogManager::webLogsHead = 0;
int LogManager::webLogsCount = 0;

LogManager::CriticalLog LogManager::criticalLogs[MAX_CRITICAL_LOGS];
int LogManager::criticalLogsCount = 0;
int LogManager::criticalLogsHead = 0;

static Preferences logPrefs;
static SemaphoreHandle_t logMutex = nullptr;
static portMUX_TYPE logMutexInitMux = portMUX_INITIALIZER_UNLOCKED;
static bool persistentCriticalLogsEnabled = true;
static bool persistentCriticalLogsErrorPrinted = false;

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

void LogManager::loadCriticalLogs() {
  criticalLogsCount = logPrefs.getInt("critCount", 0);
  criticalLogsHead = logPrefs.getInt("critHead", 0);

  if (criticalLogsCount > MAX_CRITICAL_LOGS)
    criticalLogsCount = MAX_CRITICAL_LOGS;
  if (criticalLogsHead >= MAX_CRITICAL_LOGS)
    criticalLogsHead = 0;

  for (int i = 0; i < criticalLogsCount; i++) {
    char key[16];
    snprintf(key, sizeof(key), "critLog%d", i);
    size_t sz = logPrefs.getBytes(key, &criticalLogs[i], sizeof(CriticalLog));
    if (sz != sizeof(CriticalLog)) {
      criticalLogs[i].epoch = 0;
      criticalLogs[i].message[0] = '\0';
    }
  }
}

void LogManager::saveCriticalLog(const CriticalLog &log, int index) {
  if (!persistentCriticalLogsEnabled) {
    return;
  }
  char key[16];
  snprintf(key, sizeof(key), "critLog%d", index);
  const size_t bytesWritten = logPrefs.putBytes(key, &log, sizeof(CriticalLog));
  if (bytesWritten != sizeof(CriticalLog)) {
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

void LogManager::appendWebLog(const char *msg) {
  char timeBufWeb[10];
  const DateTime now2 = getLogDateTime();
  snprintf(timeBufWeb, sizeof(timeBufWeb), "%02d:%02d:%02d", now2.hour(),
           now2.minute(), now2.second());
  String entry = String("[") + timeBufWeb + "] " + msg;
  webLogs[webLogsHead] = entry;
  webLogsHead = (webLogsHead + 1) % WEB_MAX_LOGS;
  if (webLogsCount < WEB_MAX_LOGS)
    webLogsCount++;
}

void LogManager::logInfo(const char *msg) {
  Serial.print("[INFO] ");
  Serial.println(msg);
  if (ensureMutex() && logMutex != nullptr &&
      xSemaphoreTake(logMutex, pdMS_TO_TICKS(250)) == pdTRUE) {
    char normalized[96];
    snprintf(normalized, sizeof(normalized), "INFO: %s", msg);
    appendWebLog(normalized);
    xSemaphoreGive(logMutex);
  }
}

void LogManager::logWarn(const char *msg) {
  Serial.print("[WARN] ");
  Serial.println(msg);
  if (ensureMutex() && logMutex != nullptr &&
      xSemaphoreTake(logMutex, pdMS_TO_TICKS(250)) == pdTRUE) {
    char normalized[96];
    snprintf(normalized, sizeof(normalized), "WARN: %s", msg);
    appendWebLog(normalized);
    xSemaphoreGive(logMutex);
  }
}

void LogManager::logError(const char *msg) {
  Serial.print("[ERROR] ");
  Serial.println(msg);
  if (ensureMutex() && logMutex != nullptr &&
      xSemaphoreTake(logMutex, pdMS_TO_TICKS(250)) == pdTRUE) {
    char normalized[96];
    snprintf(normalized, sizeof(normalized), "ERROR: %s", msg);
    appendWebLog(normalized);

    CriticalLog newLog;
    newLog.epoch = getLogEpoch();
    strncpy(newLog.message, msg, sizeof(newLog.message) - 1);
    newLog.message[sizeof(newLog.message) - 1] = '\0';

    criticalLogs[criticalLogsHead] = newLog;
    if (persistentCriticalLogsEnabled) {
      saveCriticalLog(newLog, criticalLogsHead);
    }

    criticalLogsHead = (criticalLogsHead + 1) % MAX_CRITICAL_LOGS;
    if (criticalLogsCount < MAX_CRITICAL_LOGS) {
      criticalLogsCount++;
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

bool LogManager::getNormalLogAt(uint8_t indexFromOldest, char *messageOut,
                                size_t messageOutSize, char *timeOut,
                                size_t timeOutSize) {
  if (!(ensureMutex() && logMutex != nullptr &&
        xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) == pdTRUE)) {
    return false;
  }

  if (indexFromOldest >= webLogsCount) {
    xSemaphoreGive(logMutex);
    return false;
  }

  const int startIdx = (webLogsCount < WEB_MAX_LOGS) ? 0 : webLogsHead;
  const int idx = (startIdx + indexFromOldest) % WEB_MAX_LOGS;
  const String entry = webLogs[idx];

  char parsedTime[6] = "--:--";
  if (entry.length() >= 7 && entry[0] == '[' && entry[3] == ':' &&
      entry[6] == ':') {
    parsedTime[0] = entry[1];
    parsedTime[1] = entry[2];
    parsedTime[2] = ':';
    parsedTime[3] = entry[4];
    parsedTime[4] = entry[5];
    parsedTime[5] = '\0';
  }

  String message = entry;
  const int closeBracket = entry.indexOf(']');
  if (closeBracket >= 0 && closeBracket + 2 < static_cast<int>(entry.length())) {
    message = entry.substring(closeBracket + 2);
  } else if (closeBracket >= 0 && closeBracket + 1 < static_cast<int>(entry.length())) {
    message = entry.substring(closeBracket + 1);
  }

  if (messageOut != nullptr && messageOutSize > 0) {
    snprintf(messageOut, messageOutSize, "%s", message.c_str());
  }
  if (timeOut != nullptr && timeOutSize > 0) {
    snprintf(timeOut, timeOutSize, "%s", parsedTime);
  }

  xSemaphoreGive(logMutex);
  return true;
}

bool LogManager::getCriticalLogAt(uint8_t indexFromOldest, char *messageOut,
                                  size_t messageOutSize, char *timeOut,
                                  size_t timeOutSize) {
  if (!(ensureMutex() && logMutex != nullptr &&
        xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) == pdTRUE)) {
    return false;
  }

  if (indexFromOldest >= criticalLogsCount) {
    xSemaphoreGive(logMutex);
    return false;
  }

  const int startCritIdx =
      (criticalLogsCount < MAX_CRITICAL_LOGS) ? 0 : criticalLogsHead;
  const int idx = (startCritIdx + indexFromOldest) % MAX_CRITICAL_LOGS;
  const CriticalLog &entry = criticalLogs[idx];

  if (messageOut != nullptr && messageOutSize > 0) {
    snprintf(messageOut, messageOutSize, "%s", entry.message);
  }
  if (timeOut != nullptr && timeOutSize > 0) {
    if (entry.epoch > 0) {
      const DateTime logTime(entry.epoch);
      snprintf(timeOut, timeOutSize, "%02d:%02d", logTime.hour(),
               logTime.minute());
    } else {
      snprintf(timeOut, timeOutSize, "%s", "--:--");
    }
  }

  xSemaphoreGive(logMutex);
  return true;
}

String LogManager::getLogsAsJson() {
  if (!(ensureMutex() && logMutex != nullptr &&
        xSemaphoreTake(logMutex, pdMS_TO_TICKS(200)) == pdTRUE)) {
    return "{\"normal\":[],\"critical\":[]}";
  }

  String jsonLog = "{";
  jsonLog += "\"normal\":[";
  int startIdx = (webLogsCount < WEB_MAX_LOGS) ? 0 : webLogsHead;
  for (int i = 0; i < webLogsCount; i++) {
    int idx = (startIdx + i) % WEB_MAX_LOGS;
    jsonLog += "\"" + escapeJsonString(webLogs[idx]) + "\"";
    if (i < webLogsCount - 1)
      jsonLog += ",";
  }
  jsonLog += "],\"critical\":[";
  int startCritIdx =
      (criticalLogsCount < MAX_CRITICAL_LOGS) ? 0 : criticalLogsHead;
  for (int i = 0; i < criticalLogsCount; i++) {
    int idx = (startCritIdx + i) % MAX_CRITICAL_LOGS;
    DateTime logTime(criticalLogs[idx].epoch);
    char buf[128];
    snprintf(buf, sizeof(buf), "[%04d-%02d-%02d %02d:%02d:%02d] %s",
             logTime.year(), logTime.month(), logTime.day(), logTime.hour(),
             logTime.minute(), logTime.second(), criticalLogs[idx].message);
    jsonLog += "\"" + escapeJsonString(String(buf)) + "\"";
    if (i < criticalLogsCount - 1)
      jsonLog += ",";
  }
  jsonLog += "]}";
  xSemaphoreGive(logMutex);
  return jsonLog;
}
