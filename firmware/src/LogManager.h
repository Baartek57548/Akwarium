#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include <Arduino.h>

enum class LogLevel : uint8_t {
  Info = 0,
  Warning = 1,
  Error = 2
};

struct LogEntrySnapshot {
  uint32_t epoch = 0;
  LogLevel level = LogLevel::Info;
  char code[24] = "";
  char message[96] = "";
};

class LogManager {
public:
  static void init();

  // Normalne logowanie do terminala (tylko w pamieci)
  static void logInfo(const char *msg);
  static void logWarn(const char *msg);

  // Zapisywane asynchronicznie krytyczne bledy w NVS
  static void logError(const char *msg);

  static void clearCriticalLogs();
  static void clearNormalLogs();
  static String getLogsAsJson();
  static String getLogsAsText(const char *type = nullptr);
  static uint32_t getChangeSequence();
  static uint8_t getNormalLogsCount();
  static uint8_t getCriticalLogsCount();
  static bool getNormalLogEntryAt(uint8_t indexFromOldest,
                                  LogEntrySnapshot &entryOut);
  static bool getCriticalLogEntryAt(uint8_t indexFromOldest,
                                    LogEntrySnapshot &entryOut);
  static bool getNormalLogAt(uint8_t indexFromOldest, char *messageOut,
                             size_t messageOutSize, char *timeOut,
                             size_t timeOutSize);
  static bool getCriticalLogAt(uint8_t indexFromOldest, char *messageOut,
                               size_t messageOutSize, char *timeOut,
                               size_t timeOutSize);

private:
  static void appendWebLog(LogLevel level, const char *code, const char *msg);
  static bool ensureMutex();

  static const int WEB_MAX_LOGS = 60;
  static LogEntrySnapshot webLogs[WEB_MAX_LOGS];
  static int webLogsHead;
  static int webLogsCount;
  static uint32_t logChangeSequence;

  static const int MAX_CRITICAL_LOGS = 32;
  static LogEntrySnapshot criticalLogs[MAX_CRITICAL_LOGS];
  static int criticalLogsCount;
  static int criticalLogsHead;

  static void loadCriticalLogs();
  static void saveCriticalLog(const LogEntrySnapshot &log, int index);
};

#endif // LOG_MANAGER_H
