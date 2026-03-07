#include "OtaManager.h"
#include "LogManager.h"
#include "SharedState.h"

#include <cstdio>
#include <cstring>

bool OtaManager::otaInProgress = false;
char OtaManager::activeTransport[12] = "idle";

void OtaManager::init() {
  otaInProgress = false;
  snprintf(activeTransport, sizeof(activeTransport), "%s", "idle");
}

bool OtaManager::tryBeginOtaUpdate(const char *transport) {
  if (otaInProgress) {
    return false;
  }

  otaInProgress = true;
  snprintf(activeTransport, sizeof(activeTransport), "%s",
           (transport != nullptr && transport[0] != '\0') ? transport
                                                          : "unknown");

  char message[96];
  snprintf(message, sizeof(message),
           "Rozpoczeto aktualizacje OTA przez %s. System wstrzymany.",
           activeTransport);
  LogManager::logWarn(message);

  // Podczas OTA warto wylaczyc grzalke by zaden watchdog ani timeout nie
  // spowodowal problemow
  SharedState::updateRelays(false, false, false, false);
  return true;
}

void OtaManager::beginOtaUpdate() {
  (void)tryBeginOtaUpdate("legacy");
}

void OtaManager::endOtaUpdate(bool success) {
  otaInProgress = false;
  snprintf(activeTransport, sizeof(activeTransport), "%s", "idle");
  if (success) {
    LogManager::logInfo(
        "Aktualizacja OTA zakonczona pomyslnie. Trwa restart...");
  } else {
    LogManager::logError("Blad aktualizacji OTA!");
  }
}

bool OtaManager::isOtaInProgress() { return otaInProgress; }

void OtaManager::cancelOtaUpdate(const char *reason) {
  otaInProgress = false;
  snprintf(activeTransport, sizeof(activeTransport), "%s", "idle");

  char message[128];
  if (reason != nullptr && reason[0] != '\0') {
    snprintf(message, sizeof(message), "Anulowano aktualizacje OTA: %s",
             reason);
  } else {
    snprintf(message, sizeof(message), "%s",
             "Anulowano aktualizacje OTA.");
  }

  LogManager::logWarn(message);
}

const char *OtaManager::getActiveTransport() { return activeTransport; }

void OtaManager::update() {
  // Miejsce na potencjalne wywolanie ArduinoOTA.handle() jesli byloby to Native
  // OTA zamiast HTTP. Skoro korzystamy z WebServer Update.h to glowna logika
  // siedzi w AkwariumWifi.cpp.
}
