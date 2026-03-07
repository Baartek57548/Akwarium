#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>

class OtaManager {
public:
  static void init();
  static void update();

  static bool tryBeginOtaUpdate(const char *transport);
  static void beginOtaUpdate();
  static void endOtaUpdate(bool success);
  static void cancelOtaUpdate(const char *reason);
  static bool isOtaInProgress();
  static const char *getActiveTransport();

private:
  static char activeTransport[12];
  static bool otaInProgress;
};

#endif // OTA_MANAGER_H
