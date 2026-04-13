#include "ConfigManager.h"
#include "LogManager.h"

namespace {

Config gConfig = {};

} // namespace

Config ConfigManager::sysConfig = {};

void ConfigManager::init() {}

void ConfigManager::save() {}

bool ConfigManager::updateAndSave(const Config &cfg) {
  gConfig = cfg;
  sysConfig = cfg;
  return true;
}

ConfigSaveResult ConfigManager::updateAndSaveDetailed(const Config &cfg) {
  gConfig = cfg;
  sysConfig = cfg;
  ConfigSaveResult result = {};
  result.ok = true;
  result.status = ConfigSaveStatus::Ok;
  result.appliedConfig = cfg;
  return result;
}

Config ConfigManager::getCopy() { return gConfig; }

void ConfigManager::saveConfig(const Config &cfg) { gConfig = cfg; }

Config ConfigManager::getConfigSnapshot() { return gConfig; }

void ConfigManager::resetToDefault() { gConfig = {}; }

uint32_t ConfigManager::calculateCrc32(const Config &) { return 0; }

void ConfigManager::loadDefaultConfig() {}

void LogManager::init() {}
void LogManager::logInfo(const char *) {}
void LogManager::logWarn(const char *) {}
void LogManager::logError(const char *) {}
void LogManager::clearCriticalLogs() {}
void LogManager::clearNormalLogs() {}
String LogManager::getLogsAsJson() { return String("{}"); }
String LogManager::getLogsAsText(const char *) { return String(); }
uint8_t LogManager::getNormalLogsCount() { return 0; }
uint8_t LogManager::getCriticalLogsCount() { return 0; }
bool LogManager::getNormalLogEntryAt(uint8_t, LogEntrySnapshot &) { return false; }
bool LogManager::getCriticalLogEntryAt(uint8_t, LogEntrySnapshot &) { return false; }
bool LogManager::getNormalLogAt(uint8_t, char *, size_t, char *, size_t) { return false; }
bool LogManager::getCriticalLogAt(uint8_t, char *, size_t, char *, size_t) { return false; }
void LogManager::appendWebLog(LogLevel, const char *, const char *) {}
bool LogManager::ensureMutex() { return true; }
LogEntrySnapshot LogManager::webLogs[WEB_MAX_LOGS];
int LogManager::webLogsHead = 0;
int LogManager::webLogsCount = 0;
LogEntrySnapshot LogManager::criticalLogs[MAX_CRITICAL_LOGS];
int LogManager::criticalLogsCount = 0;
int LogManager::criticalLogsHead = 0;
void LogManager::loadCriticalLogs() {}
void LogManager::saveCriticalLog(const LogEntrySnapshot &, int) {}
