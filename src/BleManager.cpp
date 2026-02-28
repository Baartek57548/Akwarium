#include "BleManager.h"
#include "AkwariumWifi.h"
#include "ConfigManager.h"
#include "LogManager.h"
#include "PowerManager.h"
#include "SharedState.h"
#include "SystemController.h"
#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <string>


// UUIDs
#define SERVICE_UUID "4fafc201-1fb5-459e-8bcc-c5c9c331914b"
#define CHAR_STATUS_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_COMMAND_UUID "828917c1-ea55-4d4a-a66e-fd202cea0645"
#define CHAR_SETTINGS_UUID "d2912856-de63-11ed-b5ea-0242ac120002"

static BLEServer *pServer = nullptr;
static BLECharacteristic *pCharStatus = nullptr;
static BLECharacteristic *pCharCommand = nullptr;
static BLECharacteristic *pCharSettings = nullptr;
static bool deviceConnected = false;
static bool oldDeviceConnected = false;
static bool bleInitialized = false;
static bool bleAdvertising = false;
static unsigned long lastNotifyTime = 0;
static const unsigned long NOTIFY_INTERVAL_MS = 2000;
static const char *BLE_DEVICE_NAME = "Akwarium_BLE";

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) override {
    deviceConnected = true;
    Serial.println("[BLE] Client połączony");
    // Na wypadek wielu urządzeń, możemy pozwolić dalej na parowanie
    // BLEDevice::startAdvertising();
  };

  void onDisconnect(BLEServer *pServer) override {
    deviceConnected = false;
    Serial.println("[BLE] Client rozłączony");
  }
};

// Prosty wbudowany ekstraktor wartości JSON aby uniknąć zależności ArduinoJson
static String extractJsonValue(const String &json, const String &key) {
  String searchKey = "\"" + key + "\":";
  int startIndex = json.indexOf(searchKey);
  if (startIndex == -1)
    return "";
  startIndex += searchKey.length();

  // Pomiń spacje usunięte
  while (startIndex < json.length() &&
         (json[startIndex] == ' ' || json[startIndex] == '\t')) {
    startIndex++;
  }

  if (startIndex >= json.length())
    return "";

  int endIndex = -1;
  if (json[startIndex] == '\"') {
    // Obiekty znakowe (String)
    startIndex++;
    endIndex = json.indexOf('\"', startIndex);
  } else {
    // Liczby / boole
    endIndex = json.indexOf(',', startIndex);
    int bracketIndex = json.indexOf('}', startIndex);

    if (endIndex == -1 || (bracketIndex != -1 && bracketIndex < endIndex)) {
      endIndex = bracketIndex;
    }
  }

  if (endIndex != -1) {
    String value = json.substring(startIndex, endIndex);
    value.trim();
    return value;
  }
  return "";
}

class CommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    std::string rawValue = pCharacteristic->getValue();
    if (rawValue.empty())
      return;

    String value = String(rawValue.c_str());
    Serial.println("[BLE] Command rx: " + value);
    PowerManager::registerActivity();

    String action = extractJsonValue(value, "action");
    if (action == "feed_now") {
      SystemController::feedNow();
    } else if (action == "set_servo") {
      String angleStr = extractJsonValue(value, "angle");
      if (angleStr.length() > 0) {
        SystemController::setManualServo(angleStr.toInt());
      }
    } else if (action == "clear_servo") {
      SystemController::clearManualServo();
    } else if (action == "clear_critical_logs") {
      LogManager::clearCriticalLogs();
    }
  }
};

class SettingsCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    std::string rawValue = pCharacteristic->getValue();
    if (rawValue.empty())
      return;

    String value = String(rawValue.c_str());
    Serial.println("[BLE] Settings rx: " + value);
    PowerManager::registerActivity();

    bool updated = false;
    Config &cfg = ConfigManager::getConfig();
    String tempStr;

    tempStr = extractJsonValue(value, "tar");
    if (tempStr.length() > 0) {
      cfg.targetTemp = constrain(tempStr.toFloat(), 15.0f, 35.0f);
      updated = true;
    }

    tempStr = extractJsonValue(value, "hys");
    if (tempStr.length() > 0) {
      cfg.tempHysteresis = constrain(tempStr.toFloat(), 0.1f, 5.0f);
      updated = true;
    }

    tempStr = extractJsonValue(value, "fdH");
    if (tempStr.length() > 0) {
      cfg.feedHour = constrain(tempStr.toInt(), 0, 23);
      updated = true;
    }

    tempStr = extractJsonValue(value, "fdM");
    if (tempStr.length() > 0) {
      cfg.feedMinute = constrain(tempStr.toInt(), 0, 59);
      updated = true;
    }

    tempStr = extractJsonValue(value, "fdF");
    if (tempStr.length() > 0) {
      cfg.feedMode = constrain(tempStr.toInt(), 0, 3);
      updated = true;
    }

    tempStr = extractJsonValue(value, "lsH");
    if (tempStr.length() > 0) {
      cfg.dayStartHour = constrain(tempStr.toInt(), 0, 24);
      updated = true;
    }
    tempStr = extractJsonValue(value, "lsM");
    if (tempStr.length() > 0) {
      cfg.dayStartMinute = constrain(tempStr.toInt(), 0, 59);
      updated = true;
    }

    tempStr = extractJsonValue(value, "leH");
    if (tempStr.length() > 0) {
      cfg.dayEndHour = constrain(tempStr.toInt(), 0, 24);
      updated = true;
    }
    tempStr = extractJsonValue(value, "leM");
    if (tempStr.length() > 0) {
      cfg.dayEndMinute = constrain(tempStr.toInt(), 0, 59);
      updated = true;
    }

    tempStr = extractJsonValue(value, "asH");
    if (tempStr.length() > 0) {
      cfg.aerationHourOn = constrain(tempStr.toInt(), 0, 23);
      updated = true;
    }
    tempStr = extractJsonValue(value, "asM");
    if (tempStr.length() > 0) {
      cfg.aerationMinuteOn = constrain(tempStr.toInt(), 0, 59);
      updated = true;
    }

    tempStr = extractJsonValue(value, "aeH");
    if (tempStr.length() > 0) {
      cfg.aerationHourOff = constrain(tempStr.toInt(), 0, 23);
      updated = true;
    }
    tempStr = extractJsonValue(value, "aeM");
    if (tempStr.length() > 0) {
      cfg.aerationMinuteOff = constrain(tempStr.toInt(), 0, 59);
      updated = true;
    }

    tempStr = extractJsonValue(value, "fsH");
    if (tempStr.length() > 0) {
      cfg.filterHourOn = constrain(tempStr.toInt(), 0, 23);
      updated = true;
    }
    tempStr = extractJsonValue(value, "fsM");
    if (tempStr.length() > 0) {
      cfg.filterMinuteOn = constrain(tempStr.toInt(), 0, 59);
      updated = true;
    }

    tempStr = extractJsonValue(value, "feH");
    if (tempStr.length() > 0) {
      cfg.filterHourOff = constrain(tempStr.toInt(), 0, 23);
      updated = true;
    }
    tempStr = extractJsonValue(value, "feM");
    if (tempStr.length() > 0) {
      cfg.filterMinuteOff = constrain(tempStr.toInt(), 0, 59);
      updated = true;
    }

    tempStr = extractJsonValue(value, "spO");
    if (tempStr.length() > 0) {
      cfg.servoPreOffMins = constrain(tempStr.toInt(), 0, 255);
      updated = true;
    }

    if (updated) {
      ConfigManager::save();
      Serial.println("[BLE] Settings updated & saved.");
      BleManager::notifyStatus();
    }
  }

  void onRead(BLECharacteristic *pCharacteristic) override {
    char json[500];
    const Config &cfg = ConfigManager::getConfig();
    snprintf(json, sizeof(json),
             "{\"tar\":%.1f,\"hys\":%.1f,\"fdH\":%d,\"fdM\":%d,\"fdF\":%d,"
             "\"lsH\":%d,\"lsM\":%d,\"leH\":%d,\"leM\":%d,"
             "\"asH\":%d,\"asM\":%d,\"aeH\":%d,\"aeM\":%d,"
             "\"fsH\":%d,\"fsM\":%d,\"feH\":%d,\"feM\":%d,\"spO\":%d}",
             cfg.targetTemp, cfg.tempHysteresis, cfg.feedHour, cfg.feedMinute,
             cfg.feedMode, cfg.dayStartHour, cfg.dayStartMinute, cfg.dayEndHour,
             cfg.dayEndMinute, cfg.aerationHourOn, cfg.aerationMinuteOn,
             cfg.aerationHourOff, cfg.aerationMinuteOff, cfg.filterHourOn,
             cfg.filterMinuteOn, cfg.filterHourOff, cfg.filterMinuteOff,
             cfg.servoPreOffMins);
    pCharacteristic->setValue((uint8_t *)json, strlen(json));
  }
};

void BleManager::init() {
  if (bleInitialized) {
    return;
  }

  Serial.println("[BLE] Inicjalizacja serwera...");
  BLEDevice::init(BLE_DEVICE_NAME);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Charakterystyka Status
  pCharStatus = pService->createCharacteristic(
      CHAR_STATUS_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pCharStatus->addDescriptor(new BLE2902());

  // Charakterystyka Command
  pCharCommand = pService->createCharacteristic(
      CHAR_COMMAND_UUID, BLECharacteristic::PROPERTY_WRITE);
  pCharCommand->setCallbacks(new CommandCallbacks());

  // Charakterystyka Settings
  pCharSettings = pService->createCharacteristic(
      CHAR_SETTINGS_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  pCharSettings->setCallbacks(new SettingsCallbacks());

  pService->start();

  // Konfiguracja rozgłaszania
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  // iOS requirements for min interval (ok. 20-30 ms)
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMaxPreferred(0x12);
  bleInitialized = true;
  bleAdvertising = false;
  Serial.println("[BLE] Serwer gotowy. Advertising uruchamiany z menu Bluetooth.");
}

void BleManager::start() {
  if (!bleInitialized) {
    init();
  }

  if (bleAdvertising) {
    return;
  }

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  if (!pAdvertising) {
    return;
  }

  pAdvertising->start();
  bleAdvertising = true;
  oldDeviceConnected = false;
  lastNotifyTime = 0;
  Serial.println("[BLE] Advertising wlaczony.");
}

void BleManager::stop() {
  if (!bleInitialized || !bleAdvertising) {
    return;
  }

  if (deviceConnected && pServer) {
    pServer->disconnect(pServer->getConnId());
    deviceConnected = false;
  }

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  if (pAdvertising) {
    pAdvertising->stop();
  }

  bleAdvertising = false;
  oldDeviceConnected = false;
  lastNotifyTime = 0;
  Serial.println("[BLE] Advertising wylaczony.");
}

void BleManager::notifyStatus() {
  if (!deviceConnected)
    return;

  SharedStateData snap = SharedState::getSnapshot();
  const Config &cfg = ConfigManager::getConfig();

  float vBat = PowerManager::getBatteryVoltage();
  if (isnan(vBat))
    vBat = 0.0f;

  char json[600];
  snprintf(
      json, sizeof(json),
      "{\"tmp\":%.1f,\"tar\":%.1f,\"hys\":%.1f,\"minT\":%.1f,\"minE\":%u,"
      "\"btV\":%.2f,\"btP\":%d,\"lgt\":%s,\"flt\":%s,\"srv\":%d,"
      "\"sch\":{\"lsH\":%d,\"lsM\":%d,\"leH\":%d,\"leM\":%d,"
      "\"asH\":%d,\"asM\":%d,\"aeH\":%d,\"aeM\":%d,"
      "\"fsH\":%d,\"fsM\":%d,\"feH\":%d,\"feM\":%d,\"spO\":%d},"
      "\"fd\":{\"h\":%d,\"m\":%d,\"f\":%d,\"l\":%u},"
      "\"net\":{\"ip\":\"%s\",\"ap\":%s}}",
      isnan(snap.temperature) ? -99.9 : snap.temperature, cfg.targetTemp,
      cfg.tempHysteresis, isnan(snap.minTemp) ? 20.0 : snap.minTemp,
      (unsigned int)snap.minTempEpoch, vBat,
      (int)PowerManager::getBatteryPercent(), snap.isLightOn ? "true" : "false",
      snap.isFilterOn ? "true" : "false", SystemController::getServoPosition(),
      cfg.dayStartHour, cfg.dayStartMinute, cfg.dayEndHour, cfg.dayEndMinute,
      cfg.aerationHourOn, cfg.aerationMinuteOn, cfg.aerationHourOff,
      cfg.aerationMinuteOff, cfg.filterHourOn, cfg.filterMinuteOn,
      cfg.filterHourOff, cfg.filterMinuteOff, cfg.servoPreOffMins, cfg.feedHour,
      cfg.feedMinute, cfg.feedMode, (unsigned int)cfg.lastFeedEpoch,
      AkwariumWifi::getIP().c_str(),
      AkwariumWifi::getIsAPMode() ? "true" : "false");

  pCharStatus->setValue((uint8_t *)json, strlen(json));
  pCharStatus->notify();
}

void BleManager::update() {
  if (!bleAdvertising) {
    return;
  }

  if (deviceConnected) {
    if (millis() - lastNotifyTime >= NOTIFY_INTERVAL_MS) {
      notifyStatus();
      lastNotifyTime = millis();
    }
  }

  // rozłączenie - wznów pingowanie
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    bleAdvertising = true;
    Serial.println("[BLE] Wznowiono advertising po rozłączeniu");
    oldDeviceConnected = deviceConnected;
  }

  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
}

bool BleManager::isConnected() { return deviceConnected; }
bool BleManager::isAdvertising() { return bleAdvertising; }
uint8_t BleManager::getConnectedClients() { return deviceConnected ? 1 : 0; }
const char *BleManager::getDeviceName() { return BLE_DEVICE_NAME; }
