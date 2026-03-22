#include "OtaManager.h"
#include "LogManager.h"
#include "SharedState.h"

#include <cstdio>
#include <cstring>

#ifndef RELAY_HEATER_PIN
#define RELAY_HEATER_PIN 4
#endif
#ifndef RELAY_FILTER_PIN
#define RELAY_FILTER_PIN 2
#endif
#ifndef RELAY_LIGHT_PIN
#define RELAY_LIGHT_PIN 5
#endif
#ifndef RELAY_FEEDER_PIN
#define RELAY_FEEDER_PIN 3
#endif

static constexpr uint32_t BOOT_RELAY_LEVELS_MAGIC = 0xA10A5A5AUL;

struct BootRelayLevelsSnapshot {
  uint32_t magic;
  uint8_t lightLevel;
  uint8_t filterLevel;
  uint8_t heaterLevel;
  uint8_t feederLevel;
};

RTC_DATA_ATTR static BootRelayLevelsSnapshot bootRelayLevels = {0, HIGH, HIGH,
                                                                HIGH, HIGH};

static void holdOutputLevelBeforeRestart(uint8_t pin, uint8_t level) {
  // Preload + OUTPUT, aby ograniczyc skoki poziomow tuz przed resetem OTA.
  digitalWrite(pin, level);
  pinMode(pin, OUTPUT);
  digitalWrite(pin, level);
}

bool OtaManager::otaInProgress = false;
char OtaManager::activeTransport[12] = "idle";
bool OtaManager::heldRelayStateValid = false;
bool OtaManager::heldHeaterState = false;
bool OtaManager::heldFilterState = false;
bool OtaManager::heldLightState = false;

void OtaManager::init() {
  otaInProgress = false;
  heldRelayStateValid = false;
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

  // Zapamietujemy stan przekaznikow dokladnie z chwili startu OTA i ten stan
  // ma byc trzymany przez caly proces aktualizacji.
  SharedStateData snap = SharedState::getSnapshot();
  heldHeaterState = snap.isHeaterOn;
  heldFilterState = snap.isFilterOn;
  heldLightState = snap.isLightOn;
  heldRelayStateValid = true;

  return true;
}

void OtaManager::beginOtaUpdate() {
  (void)tryBeginOtaUpdate("legacy");
}

void OtaManager::endOtaUpdate(bool success) {
  otaInProgress = false;
  heldRelayStateValid = false;
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
  heldRelayStateValid = false;
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

bool OtaManager::getHeldRelayState(bool &heater, bool &filter, bool &light) {
  if (!heldRelayStateValid) {
    return false;
  }

  heater = heldHeaterState;
  filter = heldFilterState;
  light = heldLightState;
  return true;
}

bool OtaManager::takeBootRelayLevels(uint8_t &lightLevel, uint8_t &filterLevel,
                                     uint8_t &heaterLevel,
                                     uint8_t &feederLevel) {
  if (bootRelayLevels.magic != BOOT_RELAY_LEVELS_MAGIC) {
    return false;
  }

  lightLevel = bootRelayLevels.lightLevel;
  filterLevel = bootRelayLevels.filterLevel;
  heaterLevel = bootRelayLevels.heaterLevel;
  feederLevel = bootRelayLevels.feederLevel;
  bootRelayLevels.magic = 0;
  return true;
}

void OtaManager::prepareOutputsForRestart() {
  // Zachowujemy aktualne poziomy pinow przekaznikow i "usztywniamy" je tuz
  // przed ESP.restart(), aby zminimalizowac slyszalne klikniecia.
  const uint8_t lightLevel = digitalRead(RELAY_LIGHT_PIN);
  const uint8_t filterLevel = digitalRead(RELAY_FILTER_PIN);
  const uint8_t heaterLevel = digitalRead(RELAY_HEATER_PIN);
  const uint8_t feederLevel = digitalRead(RELAY_FEEDER_PIN);

  bootRelayLevels.magic = BOOT_RELAY_LEVELS_MAGIC;
  bootRelayLevels.lightLevel = lightLevel;
  bootRelayLevels.filterLevel = filterLevel;
  bootRelayLevels.heaterLevel = heaterLevel;
  bootRelayLevels.feederLevel = feederLevel;

  holdOutputLevelBeforeRestart(RELAY_LIGHT_PIN, lightLevel);
  holdOutputLevelBeforeRestart(RELAY_FILTER_PIN, filterLevel);
  holdOutputLevelBeforeRestart(RELAY_HEATER_PIN, heaterLevel);
  holdOutputLevelBeforeRestart(RELAY_FEEDER_PIN, feederLevel);
}

void OtaManager::update() {
  // Miejsce na potencjalne wywolanie ArduinoOTA.handle() jesli byloby to Native
  // OTA zamiast HTTP. Skoro korzystamy z WebServer Update.h to glowna logika
  // siedzi w AkwariumWifi.cpp.
}
