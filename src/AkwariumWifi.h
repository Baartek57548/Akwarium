#ifndef AKWARIUM_WIFI_H
#define AKWARIUM_WIFI_H

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

extern void syncSystemTime(uint32_t epoch);

class AkwariumWifi {
public:
  static void begin();
  static bool getIsAPMode();
  static void startAP();
  static void stopAP();
  static void requestStaOffForDeepSleep();
  static bool isStaOff();
  static String getAPName();
  static String getAPPassword();
  static String getIP();
  static uint8_t getConnectedClients();
  static WebServer &getServer();
};

#endif
