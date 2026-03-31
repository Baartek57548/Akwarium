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
  static void clearNormalLogs();
  static String getLogsAsJson();
  static uint8_t getNormalLogsCount();
  static uint8_t getCriticalLogsCount();
  static bool getNormalLogAt(uint8_t indexFromOldest, char *messageOut,
                             size_t messageOutSize, char *timeOut,
                             size_t timeOutSize);
  static bool getCriticalLogAt(uint8_t indexFromOldest, char *messageOut,
                               size_t messageOutSize, char *timeOut,
                               size_t timeOutSize);

private:
  static void appendWebLog(const char *msg);
  static bool ensureMutex();

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
