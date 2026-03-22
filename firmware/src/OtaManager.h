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
  static bool getHeldRelayState(bool &heater, bool &filter, bool &light);
  static bool takeBootRelayLevels(uint8_t &lightLevel, uint8_t &filterLevel,
                                  uint8_t &heaterLevel, uint8_t &feederLevel);
  static void prepareOutputsForRestart();

private:
  static char activeTransport[12];
  static bool otaInProgress;
  static bool heldRelayStateValid;
  static bool heldHeaterState;
  static bool heldFilterState;
  static bool heldLightState;
};

#endif // OTA_MANAGER_H
