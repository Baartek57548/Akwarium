#include "OtaManager.h"
#include "LogManager.h"
#include "SharedState.h"

bool OtaManager::otaInProgress = false;

void OtaManager::init() { otaInProgress = false; }

void OtaManager::beginOtaUpdate() {
  otaInProgress = true;
  LogManager::logWarn("Rozpoczeto aktualizacje OTA. System wstrzymany.");
  // Podczas OTA warto wylaczyc grzalke by zaden watchdog ani timeout nie
  // spowodowal problemow
  SharedState::updateRelays(false, false, false, false);
}

void OtaManager::endOtaUpdate(bool success) {
  otaInProgress = false;
  if (success) {
    LogManager::logInfo(
        "Aktualizacja OTA zakonczona pomyslnie. Trwa restart...");
  } else {
    LogManager::logError("Blad aktualizacji OTA!");
  }
}

bool OtaManager::isOtaInProgress() { return otaInProgress; }

void OtaManager::update() {
  // Miejsce na potencjalne wywolanie ArduinoOTA.handle() jesli byloby to Native
  // OTA zamiast HTTP. Skoro korzystamy z WebServer Update.h to glowna logika
  // siedzi w AkwariumWifi.cpp.
}
