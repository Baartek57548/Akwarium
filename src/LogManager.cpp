#include "LogManager.h"
#include <Preferences.h>
#include <RTClib.h> // Potrzebne uzytkownika getCurrentDateTime lub z SharedState / z globalnego contextu. Zakladajac wstrzykniecie DateTime przez zaleznosc, lub polegajac na zewnetrznym mechanizmie.
// Zrobmy uzycie zewnetrznej funkcji, ktora bedzie zdefiniowana nizej bo to
// jedyny zewnetrzny glos.
extern DateTime getCurrentDateTime();
// Ewentualnie jesli to klopot, wezmiemy epoch z shared state - zeby nie mieszac
// zaleznosci miedzy modulami.
extern class AquariumAnimation *animation;

String LogManager::webLogs[WEB_MAX_LOGS];
int LogManager::webLogsHead = 0;
int LogManager::webLogsCount = 0;

LogManager::CriticalLog LogManager::criticalLogs[MAX_CRITICAL_LOGS];
int LogManager::criticalLogsCount = 0;
int LogManager::criticalLogsHead = 0;

static Preferences logPrefs;

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
  char key[16];
  snprintf(key, sizeof(key), "critLog%d", index);
  logPrefs.putBytes(key, &log, sizeof(CriticalLog));
}

void LogManager::init() {
  logPrefs.begin("Akwarium", false);
  loadCriticalLogs();
}

void LogManager::appendWebLog(const char *msg) {
  char timeBufWeb[10];
  DateTime now2 = getCurrentDateTime();
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
  appendWebLog(msg);

  // Legacy support for animation
  if (animation != nullptr) {
    char timeBuf[6];
    DateTime now = getCurrentDateTime();
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", now.hour(), now.minute());
    // animation->addLog(msg, timeBuf); // Zostawimy to na potem w
    // SystemController
  }
}

void LogManager::logWarn(const char *msg) {
  Serial.print("[WARN] ");
  Serial.println(msg);
  appendWebLog(msg);
}

void LogManager::logError(const char *msg) {
  Serial.print("[ERROR] ");
  Serial.println(msg);

  DateTime now = getCurrentDateTime();
  CriticalLog newLog;
  newLog.epoch = now.unixtime();
  strncpy(newLog.message, msg, sizeof(newLog.message) - 1);
  newLog.message[sizeof(newLog.message) - 1] = '\0';

  criticalLogs[criticalLogsHead] = newLog;
  saveCriticalLog(newLog, criticalLogsHead);

  criticalLogsHead = (criticalLogsHead + 1) % MAX_CRITICAL_LOGS;
  if (criticalLogsCount < MAX_CRITICAL_LOGS) {
    criticalLogsCount++;
  }

  logPrefs.putInt("critCount", criticalLogsCount);
  logPrefs.putInt("critHead", criticalLogsHead);
}

void LogManager::clearCriticalLogs() {
  criticalLogsCount = 0;
  criticalLogsHead = 0;
  logPrefs.putInt("critCount", 0);
  logPrefs.putInt("critHead", 0);
  Serial.println("[LOGS] WICZYSZCZONO LOGI KRYTYCZNE");
}

String LogManager::getLogsAsJson() {
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
  return jsonLog;
}
