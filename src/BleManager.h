#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>

class BleManager {
public:
    static void init();
    static void update();
    static void notifyStatus();
    static bool isConnected();
};

#endif // BLE_MANAGER_H
