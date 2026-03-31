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
  static void requestStaOffForSleep();
  static void requestStaOn();
  static bool isStaOff();
  static bool isStaConnected();
  static bool isStaConnecting();
  static bool isServiceModeActive();
  static bool isTimeSyncInProgress();
  static String getAPName();
  static String getAPPassword();
  static String getConfiguredStaSsid();
  static String getConfiguredAPName();
  static String getConfiguredAPPassword();
  static String getStaSsid();
  static uint32_t getStaLastConnectedEpoch();
  static uint32_t getLastTimeSyncEpoch();
  static bool wasLastTimeSyncSuccessful();
  static String getLastTimeSyncStatus();
  static String getIP();
  static uint8_t getConnectedClients();
  static WebServer &getServer();
};

#endif
