#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>

class BleManager {
public:
  static void init();
  static void start();
  static void stop();
  static void update();
  static void notifyStatus();
  static bool isConnected();
  static bool isAdvertising();
  static uint8_t getConnectedClients();
  static const char *getDeviceName();
};

#endif // BLE_MANAGER_H
