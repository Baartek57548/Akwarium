#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include <Arduino.h>
#include <RTClib.h>

class LogManager {
public:
  struct LogRecord {
    uint32_t epoch;
    char level; // 'I' / 'W' / 'E'
    char message[72];
  };

  static void init();

  // Logi dzienne (kasowane automatycznie po zmianie dnia)
  static void logInfo(const char *msg);

  // Logi wazne (oznaczone '*', trwale w NVS)
  static void logWarn(const char *msg);
  static void logError(const char *msg);

  static void syncOledLogs();
  static void clearCriticalLogs();
  static String getLogsAsJson();

private:
  static const int MAX_DAILY_LOGS = 80;
  static const int MAX_IMPORTANT_LOGS = 40;

  static LogRecord dailyLogs[MAX_DAILY_LOGS];
  static int dailyLogsHead;
  static int dailyLogsCount;

  static LogRecord importantLogs[MAX_IMPORTANT_LOGS];
  static int importantLogsHead;
  static int importantLogsCount;

  static int currentDayKey;
  static bool initialized;

  static void logMessage(char level, const char *msg);
  static void appendDailyLogLocked(const LogRecord &log);
  static void appendImportantLogLocked(const LogRecord &log);
  static void pushLogToOled(const LogRecord &log, bool important);
  static void rotateDailyIfNeededLocked(const DateTime &now);

  static void loadImportantLogsLocked();
  static void saveImportantLogLocked(const LogRecord &log, int index);
  static void saveImportantMetaLocked();
};

#endif // LOG_MANAGER_H
