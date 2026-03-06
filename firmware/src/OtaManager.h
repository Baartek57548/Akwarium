#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>

class OtaManager {
public:
  static void init();
  static void update();

  static void beginOtaUpdate();
  static void endOtaUpdate(bool success);
  static bool isOtaInProgress();

private:
  static bool otaInProgress;
};

#endif // OTA_MANAGER_H
