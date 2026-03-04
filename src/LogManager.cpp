#include "LogManager.h"

#include "AquariumAnimation.h"
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

extern DateTime getCurrentDateTime();
extern AquariumAnimation *animation;

LogManager::LogRecord LogManager::dailyLogs[MAX_DAILY_LOGS];
int LogManager::dailyLogsHead = 0;
int LogManager::dailyLogsCount = 0;

LogManager::LogRecord LogManager::importantLogs[MAX_IMPORTANT_LOGS];
int LogManager::importantLogsHead = 0;
int LogManager::importantLogsCount = 0;

int LogManager::currentDayKey = 0;
bool LogManager::initialized = false;

static Preferences logPrefs;
static SemaphoreHandle_t logMutex = nullptr;

namespace {

struct LegacyCriticalLog {
  uint32_t epoch;
  char message[64];
};

static String escapeJsonString(const String &input) {
  String out;
  out.reserve(input.length() + 8);

  for (size_t i = 0; i < input.length(); i++) {
    const char c = input[i];
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

static int buildDayKey(const DateTime &now) {
  return now.year() * 10000 + now.month() * 100 + now.day();
}

static bool isImportantLevel(const char level) {
  return (level == 'W' || level == 'E');
}

static const char *levelTag(const char level) {
  switch (level) {
  case 'W':
    return "WARN";
  case 'E':
    return "ERROR";
  case 'I':
  default:
    return "INFO";
  }
}

static String formatWebLogLine(const LogManager::LogRecord &log, bool important) {
  DateTime ts(log.epoch);
  char line[180];
  if (important) {
    snprintf(line, sizeof(line), "[%04d-%02d-%02d %02d:%02d:%02d] * [%c] %s",
             ts.year(), ts.month(), ts.day(), ts.hour(), ts.minute(),
             ts.second(), log.level, log.message);
  } else {
    snprintf(line, sizeof(line), "[%04d-%02d-%02d %02d:%02d:%02d] [%c] %s",
             ts.year(), ts.month(), ts.day(), ts.hour(), ts.minute(),
             ts.second(), log.level, log.message);
  }
  return String(line);
}

} // namespace

void LogManager::saveImportantLogLocked(const LogRecord &log, int index) {
  char key[16];
  snprintf(key, sizeof(key), "impLog%d", index);
  logPrefs.putBytes(key, &log, sizeof(LogRecord));
}

void LogManager::saveImportantMetaLocked() {
  logPrefs.putInt("impCount", importantLogsCount);
  logPrefs.putInt("impHead", importantLogsHead);
}

void LogManager::appendDailyLogLocked(const LogRecord &log) {
  dailyLogs[dailyLogsHead] = log;
  dailyLogsHead = (dailyLogsHead + 1) % MAX_DAILY_LOGS;
  if (dailyLogsCount < MAX_DAILY_LOGS) {
    dailyLogsCount++;
  }
}

void LogManager::appendImportantLogLocked(const LogRecord &log) {
  importantLogs[importantLogsHead] = log;
  saveImportantLogLocked(log, importantLogsHead);

  importantLogsHead = (importantLogsHead + 1) % MAX_IMPORTANT_LOGS;
  if (importantLogsCount < MAX_IMPORTANT_LOGS) {
    importantLogsCount++;
  }

  saveImportantMetaLocked();
}

void LogManager::rotateDailyIfNeededLocked(const DateTime &now) {
  const int newDayKey = buildDayKey(now);

  if (currentDayKey == 0) {
    currentDayKey = newDayKey;
    return;
  }

  if (newDayKey != currentDayKey) {
    dailyLogsCount = 0;
    dailyLogsHead = 0;
    currentDayKey = newDayKey;
    Serial.println("[INFO] Rotacja logow dziennych: wyczyszczono logi zwykle.");
  }
}

void LogManager::pushLogToOled(const LogRecord &log, bool important) {
  if (animation == nullptr) {
    return;
  }

  DateTime ts(log.epoch);
  char stamp[12];
  snprintf(stamp, sizeof(stamp), "%02d.%02d %02d:%02d", ts.day(), ts.month(),
           ts.hour(), ts.minute());

  char msg[24];
  snprintf(msg, sizeof(msg), "%c:%s", log.level, log.message);
  animation->addLog(msg, stamp, important);
}

void LogManager::loadImportantLogsLocked() {
  importantLogsCount = logPrefs.getInt("impCount", 0);
  importantLogsHead = logPrefs.getInt("impHead", 0);

  if (importantLogsCount < 0) {
    importantLogsCount = 0;
  }
  if (importantLogsCount > MAX_IMPORTANT_LOGS) {
    importantLogsCount = MAX_IMPORTANT_LOGS;
  }
  if (importantLogsHead < 0 || importantLogsHead >= MAX_IMPORTANT_LOGS) {
    importantLogsHead = 0;
  }

  for (int i = 0; i < importantLogsCount; i++) {
    char key[16];
    snprintf(key, sizeof(key), "impLog%d", i);
    const size_t sz = logPrefs.getBytes(key, &importantLogs[i], sizeof(LogRecord));
    if (sz != sizeof(LogRecord)) {
      importantLogs[i].epoch = 0;
      importantLogs[i].level = 'E';
      snprintf(importantLogs[i].message, sizeof(importantLogs[i].message),
               "niepoprawny wpis");
    }
  }

  // Jednorazowa migracja legacy "critical" -> "important", jesli nowe logi puste.
  if (importantLogsCount == 0) {
    const int legacyMax = 20;
    int legacyCount = logPrefs.getInt("critCount", 0);
    int legacyHead = logPrefs.getInt("critHead", 0);

    if (legacyCount < 0) {
      legacyCount = 0;
    }
    if (legacyCount > legacyMax) {
      legacyCount = legacyMax;
    }
    if (legacyHead < 0 || legacyHead >= legacyMax) {
      legacyHead = 0;
    }

    if (legacyCount > 0) {
      const int legacyStart = (legacyCount < legacyMax) ? 0 : legacyHead;
      for (int i = 0; i < legacyCount; i++) {
        const int legacyIdx = (legacyStart + i) % legacyMax;
        char key[16];
        snprintf(key, sizeof(key), "critLog%d", legacyIdx);

        LegacyCriticalLog legacy = {};
        const size_t sz = logPrefs.getBytes(key, &legacy, sizeof(legacy));
        if (sz != sizeof(legacy)) {
          continue;
        }

        LogRecord migrated = {};
        migrated.epoch = legacy.epoch;
        migrated.level = 'E';
        strncpy(migrated.message, legacy.message, sizeof(migrated.message) - 1);
        migrated.message[sizeof(migrated.message) - 1] = '\0';
        appendImportantLogLocked(migrated);
      }
      Serial.println("[INFO] Zmigrowano legacy logi krytyczne do nowego formatu.");
    }
  }
}

void LogManager::init() {
  if (initialized) {
    return;
  }

  if (logMutex == nullptr) {
    logMutex = xSemaphoreCreateMutex();
  }

  logPrefs.begin("Akwarium", false);

  if (logMutex != nullptr &&
      xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    loadImportantLogsLocked();
    rotateDailyIfNeededLocked(getCurrentDateTime());
    xSemaphoreGive(logMutex);
  } else {
    loadImportantLogsLocked();
    rotateDailyIfNeededLocked(getCurrentDateTime());
  }

  initialized = true;
}

void LogManager::logMessage(char level, const char *msg) {
  if (msg == nullptr || msg[0] == '\0') {
    return;
  }
  if (!initialized) {
    init();
  }

  const bool important = isImportantLevel(level);
  DateTime now = getCurrentDateTime();

  LogRecord record = {};
  record.epoch = now.unixtime();
  record.level = level;
  strncpy(record.message, msg, sizeof(record.message) - 1);
  record.message[sizeof(record.message) - 1] = '\0';

  Serial.print("[");
  Serial.print(levelTag(level));
  Serial.print("] ");
  if (important) {
    Serial.print("* ");
  }
  Serial.println(record.message);

  if (logMutex != nullptr &&
      xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    rotateDailyIfNeededLocked(now);
    if (important) {
      appendImportantLogLocked(record);
    } else {
      appendDailyLogLocked(record);
    }
    xSemaphoreGive(logMutex);
  }

  pushLogToOled(record, important);
}

void LogManager::logInfo(const char *msg) { logMessage('I', msg); }

void LogManager::logWarn(const char *msg) { logMessage('W', msg); }

void LogManager::logError(const char *msg) { logMessage('E', msg); }

void LogManager::syncOledLogs() {
  if (!initialized) {
    init();
  }
  if (animation == nullptr) {
    return;
  }

  struct OledLine {
    LogRecord record;
    bool important;
  };

  OledLine latest[20];
  int latestCount = 0;

  auto insertLatest = [&](const LogRecord &record, bool important) {
    OledLine candidate = {record, important};

    if (latestCount < 20) {
      int pos = latestCount;
      while (pos > 0 && latest[pos - 1].record.epoch > candidate.record.epoch) {
        latest[pos] = latest[pos - 1];
        pos--;
      }
      latest[pos] = candidate;
      latestCount++;
      return;
    }

    // Mamy juz 20 wpisow (posortowane rosnaco po epoce). Odrzucaj starsze.
    if (candidate.record.epoch <= latest[0].record.epoch) {
      return;
    }

    for (int i = 1; i < 20; i++) {
      latest[i - 1] = latest[i];
    }
    latestCount = 19;

    int pos = latestCount;
    while (pos > 0 && latest[pos - 1].record.epoch > candidate.record.epoch) {
      latest[pos] = latest[pos - 1];
      pos--;
    }
    latest[pos] = candidate;
    latestCount = 20;
  };

  if (logMutex != nullptr &&
      xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    rotateDailyIfNeededLocked(getCurrentDateTime());

    const int dailyStart = (dailyLogsCount < MAX_DAILY_LOGS) ? 0 : dailyLogsHead;
    for (int i = 0; i < dailyLogsCount; i++) {
      const int idx = (dailyStart + i) % MAX_DAILY_LOGS;
      insertLatest(dailyLogs[idx], false);
    }

    const int importantStart =
        (importantLogsCount < MAX_IMPORTANT_LOGS) ? 0 : importantLogsHead;
    for (int i = 0; i < importantLogsCount; i++) {
      const int idx = (importantStart + i) % MAX_IMPORTANT_LOGS;
      insertLatest(importantLogs[idx], true);
    }

    xSemaphoreGive(logMutex);
  }

  animation->clearLogs();
  for (int i = 0; i < latestCount; i++) {
    pushLogToOled(latest[i].record, latest[i].important);
  }
}

void LogManager::clearCriticalLogs() {
  if (!initialized) {
    init();
  }

  if (logMutex != nullptr &&
      xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    importantLogsCount = 0;
    importantLogsHead = 0;
    saveImportantMetaLocked();
    for (int i = 0; i < MAX_IMPORTANT_LOGS; i++) {
      char key[16];
      snprintf(key, sizeof(key), "impLog%d", i);
      logPrefs.remove(key);
    }
    xSemaphoreGive(logMutex);
  }

  Serial.println("[INFO] Usunieto trwale logi wazne.");
  syncOledLogs();
}

String LogManager::getLogsAsJson() {
  if (!initialized) {
    init();
  }

  String json = "{";
  json += "\"normal\":[";

  if (logMutex != nullptr &&
      xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    rotateDailyIfNeededLocked(getCurrentDateTime());

    const int dailyStart = (dailyLogsCount < MAX_DAILY_LOGS) ? 0 : dailyLogsHead;
    for (int i = 0; i < dailyLogsCount; i++) {
      const int idx = (dailyStart + i) % MAX_DAILY_LOGS;
      json += "\"";
      json += escapeJsonString(formatWebLogLine(dailyLogs[idx], false));
      json += "\"";
      if (i < (dailyLogsCount - 1)) {
        json += ",";
      }
    }

    json += "],\"critical\":[";

    const int importantStart =
        (importantLogsCount < MAX_IMPORTANT_LOGS) ? 0 : importantLogsHead;
    for (int i = 0; i < importantLogsCount; i++) {
      const int idx = (importantStart + i) % MAX_IMPORTANT_LOGS;
      json += "\"";
      json += escapeJsonString(formatWebLogLine(importantLogs[idx], true));
      json += "\"";
      if (i < (importantLogsCount - 1)) {
        json += ",";
      }
    }

    xSemaphoreGive(logMutex);
  } else {
    json += "],\"critical\":[";
  }

  json += "]}";
  return json;
}
