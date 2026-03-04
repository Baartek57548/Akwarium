#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include <Arduino.h>

class LogManager {
public:
  static void init();

  // Normalne logowanie do terminala (tylko w pamieci)
  static void logInfo(const char *msg);
  static void logWarn(const char *msg);

  // Zapisywane asynchronicznie krytyczne bledy w NVS
  static void logError(const char *msg);

  static void clearCriticalLogs();
  static String getLogsAsJson();

private:
  static void appendWebLog(const char *msg);

  static const int WEB_MAX_LOGS = 20;
  static String webLogs[WEB_MAX_LOGS];
  static int webLogsHead;
  static int webLogsCount;

  struct CriticalLog {
    uint32_t epoch;
    char message[64];
  };

  static const int MAX_CRITICAL_LOGS = 20;
  static CriticalLog criticalLogs[MAX_CRITICAL_LOGS];
  static int criticalLogsCount;
  static int criticalLogsHead;

  static void loadCriticalLogs();
  static void saveCriticalLog(const CriticalLog &log, int index);
};

#endif // LOG_MANAGER_H
