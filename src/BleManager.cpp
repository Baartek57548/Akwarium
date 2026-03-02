#include "BleManager.h"
#include "AkwariumWifi.h"
#include "ConfigManager.h"
#include "ConfigValidation.h"
#include "InterfaceCore.h"
#include "LogManager.h"
#include "PowerManager.h"
#include "SecretConfig.h"
#include "SharedState.h"
#include "SystemController.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLESecurity.h>
#include <BLEUtils.h>
#include <esp_err.h>

#include <cstdlib>
#include <errno.h>
#include <map>
#include <string>
#include <sys/time.h>

// UUIDs
#define SERVICE_UUID "4fafc201-1fb5-459e-8bcc-c5c9c331914b"
#define CHAR_STATUS_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_COMMAND_UUID "828917c1-ea55-4d4a-a66e-fd202cea0645"
#define CHAR_SETTINGS_UUID "d2912856-de63-11ed-b5ea-0242ac120002"
#define CHAR_RESULT_UUID "8e22cb9c-1728-45f9-8c50-2f7252f07379"

static BLEServer *pServer = nullptr;
static BLECharacteristic *pCharStatus = nullptr;
static BLECharacteristic *pCharCommand = nullptr;
static BLECharacteristic *pCharSettings = nullptr;
static BLECharacteristic *pCharResult = nullptr;

static volatile bool deviceConnected = false;
static bool bleInitialized = false;
static bool bleAdvertising = false;
static volatile bool resumeAdvertisingPending = false;
static volatile unsigned long resumeAdvertisingAtMs = 0;
static unsigned long lastNotifyTime = 0;

static const unsigned long NOTIFY_INTERVAL_MS = 2000;
static const unsigned long RESUME_ADVERTISING_DELAY_MS = 150;
static const uint16_t BLE_PREFERRED_MTU = 185;
static const char *BLE_DEVICE_NAME = "Akwarium_BLE";

static void refreshConnectionState() {
  if (pServer) {
    deviceConnected = (pServer->getConnectedCount() > 0);
  } else {
    deviceConnected = false;
  }
}

static bool tryGetFloat(JsonObjectConst obj, const char *key, float &out) {
  if (!obj.containsKey(key)) {
    return false;
  }

  JsonVariantConst v = obj[key];
  if (v.is<float>() || v.is<double>() || v.is<int>() || v.is<long>() ||
      v.is<unsigned int>() || v.is<unsigned long>()) {
    out = v.as<float>();
    return true;
  }

  if (v.is<const char *>()) {
    const char *raw = v.as<const char *>();
    if (raw == nullptr || raw[0] == '\0') {
      return false;
    }

    char *endPtr = nullptr;
    float parsed = strtof(raw, &endPtr);
    if (endPtr == raw || *endPtr != '\0') {
      return false;
    }

    out = parsed;
    return true;
  }

  return false;
}

static bool tryGetInt(JsonObjectConst obj, const char *key, long &out) {
  if (!obj.containsKey(key)) {
    return false;
  }

  JsonVariantConst v = obj[key];
  if (v.is<int>() || v.is<long>() || v.is<unsigned int>() ||
      v.is<unsigned long>()) {
    out = v.as<long>();
    return true;
  }

  if (v.is<const char *>()) {
    const char *raw = v.as<const char *>();
    if (raw == nullptr || raw[0] == '\0') {
      return false;
    }

    char *endPtr = nullptr;
    long parsed = strtol(raw, &endPtr, 10);
    if (endPtr == raw || *endPtr != '\0') {
      return false;
    }

    out = parsed;
    return true;
  }

  return false;
}

static bool tryGetUInt64(JsonObjectConst obj, const char *key, uint64_t &out) {
  if (!obj.containsKey(key)) {
    return false;
  }

  JsonVariantConst v = obj[key];
  if (v.is<unsigned long>()) {
    out = static_cast<uint64_t>(v.as<unsigned long>());
    return true;
  }
  if (v.is<unsigned int>()) {
    out = static_cast<uint64_t>(v.as<unsigned int>());
    return true;
  }
  if (v.is<long>()) {
    long value = v.as<long>();
    if (value < 0) {
      return false;
    }
    out = static_cast<uint64_t>(value);
    return true;
  }
  if (v.is<int>()) {
    int value = v.as<int>();
    if (value < 0) {
      return false;
    }
    out = static_cast<uint64_t>(value);
    return true;
  }
  if (v.is<const char *>()) {
    const char *raw = v.as<const char *>();
    if (raw == nullptr || raw[0] == '\0') {
      return false;
    }

    errno = 0;
    char *endPtr = nullptr;
    unsigned long long parsed = strtoull(raw, &endPtr, 10);
    if (errno == ERANGE || endPtr == raw || *endPtr != '\0') {
      return false;
    }

    out = static_cast<uint64_t>(parsed);
    return true;
  }

  return false;
}

static bool parseEpochForRtc(uint64_t rawEpoch, uint32_t &epochUtc,
                             uint32_t &epochRtc, long tzOffsetMin) {
  uint64_t normalized = rawEpoch;

  // Akceptujemy epoch w sekundach lub milisekundach.
  if (normalized > 4102444800ULL && normalized <= 4102444800000ULL) {
    normalized /= 1000ULL;
  }
  if (normalized < 1704067200ULL || normalized > 4102444800ULL) {
    return false;
  }

  int64_t adjusted = static_cast<int64_t>(normalized) -
                     static_cast<int64_t>(tzOffsetMin) * 60LL;
  if (adjusted < 1704067200LL || adjusted > 4102444800LL) {
    return false;
  }

  epochUtc = static_cast<uint32_t>(normalized);
  epochRtc = static_cast<uint32_t>(adjusted);
  return true;
}

static void publishResult(const char *type, const char *code) {
  if (!pCharResult || !deviceConnected) {
    return;
  }

  char json[96];
  snprintf(json, sizeof(json), "{\"t\":\"%s\",\"c\":\"%s\"}", type,
           code);
  pCharResult->setValue((uint8_t *)json, strlen(json));
  pCharResult->notify();
}

static void buildStatusJson(char *json, size_t jsonSize) {
  SharedStateData snap = SharedState::getSnapshot();
  const Config cfg = ConfigManager::getCopy();

  float vBat = PowerManager::getBatteryVoltage();
  if (isnan(vBat)) {
    vBat = 0.0f;
  }

  uint8_t clients = 0;
  if (pServer) {
    uint32_t count = pServer->getConnectedCount();
    clients = (count > 255) ? 255 : static_cast<uint8_t>(count);
  }

  snprintf(json, jsonSize,
           "{\"tmp\":%.1f,\"tar\":%.1f,\"hys\":%.1f,\"mn\":%.1f,\"me\":%u,"
           "\"bv\":%.2f,\"bp\":%d,\"l\":%s,\"f\":%s,\"h\":%s,\"srv\":%d,"
           "\"ip\":\"%s\",\"ap\":%s,\"cli\":%u}",
           isnan(snap.temperature) ? -99.9f : snap.temperature, cfg.targetTemp,
           cfg.tempHysteresis, isnan(snap.minTemp) ? 20.0f : snap.minTemp,
           static_cast<unsigned int>(snap.minTempEpoch), vBat,
           static_cast<int>(PowerManager::getBatteryPercent()),
           snap.isLightOn ? "true" : "false",
           snap.isFilterOn ? "true" : "false",
           snap.isHeaterOn ? "true" : "false",
           SystemController::getServoPosition(), AkwariumWifi::getIP().c_str(),
           AkwariumWifi::getIsAPMode() ? "true" : "false", clients);
}

class MySecurityCallbacks : public BLESecurityCallbacks {
  uint32_t onPassKeyRequest() override {
    return static_cast<uint32_t>(SECRET_BLE_PASSKEY);
  }

  void onPassKeyNotify(uint32_t pass_key) override {
    Serial.printf("[BLE] Pairing PIN: %06lu\n",
                  static_cast<unsigned long>(pass_key));
  }

  bool onSecurityRequest() override { return true; }

  bool onConfirmPIN(uint32_t pass_key) override {
    Serial.printf("[BLE] Potwierdzenie PIN: %06lu\n",
                  static_cast<unsigned long>(pass_key));
    return true;
  }

  void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl) override {
    if (!auth_cmpl.success) {
      Serial.printf("[BLE] Autoryzacja nieudana, reason=%d\n",
                    auth_cmpl.fail_reason);
    } else {
      Serial.println("[BLE] Autoryzacja zakonczona sukcesem (bonded). ");
    }
  }
};

class MyServerCallbacks : public BLEServerCallbacks {
private:
  static void handleConnect(BLEServer *server) {
    deviceConnected = true;
    resumeAdvertisingPending = false;
    lastNotifyTime = 0;

    uint32_t count = server ? server->getConnectedCount() : 1;
    Serial.printf("[BLE] Client polaczony. Klienci: %lu\n",
                  static_cast<unsigned long>(count));
    BleManager::notifyStatus();
    publishResult("ack", "connected");
  }

  static void handleDisconnect(BLEServer *server) {
    uint32_t count = server ? server->getConnectedCount() : 0;
    deviceConnected = (count > 0);
    Serial.printf("[BLE] Client rozlaczony. Klienci: %lu\n",
                  static_cast<unsigned long>(count));

    if (bleAdvertising && !deviceConnected) {
      resumeAdvertisingPending = true;
      resumeAdvertisingAtMs = millis() + RESUME_ADVERTISING_DELAY_MS;
    }
  }

public:
  void onConnect(BLEServer *server) override { (void)server; }

  void onConnect(BLEServer *server, esp_ble_gatts_cb_param_t *param) override {
    (void)param;
    handleConnect(server);
  }

  void onDisconnect(BLEServer *server) override { (void)server; }

  void onDisconnect(BLEServer *server,
                    esp_ble_gatts_cb_param_t *param) override {
    (void)param;
    handleDisconnect(server);
  }
};

class StatusCallbacks : public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic *pCharacteristic) override {
    char json[220];
    buildStatusJson(json, sizeof(json));
    pCharacteristic->setValue((uint8_t *)json, strlen(json));
  }
};

class CommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    std::string rawValue = pCharacteristic->getValue();
    if (rawValue.empty()) {
      publishResult("err", "empty_command");
      return;
    }

    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, rawValue);
    if (err) {
      Serial.printf("[BLE] Command JSON parse error: %s\n", err.c_str());
      publishResult("err", "bad_json");
      return;
    }

    JsonObjectConst root = doc.as<JsonObjectConst>();
    const char *action = root["action"] | "";
    if (action[0] == '\0') {
      publishResult("err", "missing_action");
      return;
    }

    PowerManager::registerActivity();
    Serial.println("[BLE] Command action: " + String(action));

    if (strcmp(action, "feed_now") == 0) {
      InterfaceCore::triggerFeedNow();
      publishResult("ack", "feed_now");
      BleManager::notifyStatus();
      return;
    }

    if (strcmp(action, "set_servo") == 0) {
      long angle = 0;
      if (!tryGetInt(root, "angle", angle)) {
        publishResult("err", "missing_angle");
        return;
      }
      InterfaceRuleResult servoResult = InterfaceCore::setManualServoAngle(angle);
      if (!servoResult.isOk()) {
        publishResult("err", "invalid_angle");
        return;
      }
      publishResult("ack", "set_servo");
      BleManager::notifyStatus();
      return;
    }

    if (strcmp(action, "clear_servo") == 0) {
      InterfaceCore::clearManualServo();
      publishResult("ack", "clear_servo");
      BleManager::notifyStatus();
      return;
    }

    if (strcmp(action, "clear_critical_logs") == 0) {
      InterfaceCore::clearCriticalLogs();
      publishResult("ack", "clear_logs");
      return;
    }

    if (strcmp(action, "set_time") == 0) {
      uint64_t rawEpoch = 0;
      if (!tryGetUInt64(root, "epoch", rawEpoch)) {
        publishResult("err", "missing_epoch");
        return;
      }

      long tzOffsetMin = 0;
      if (root.containsKey("tzOffsetMin")) {
        if (!tryGetInt(root, "tzOffsetMin", tzOffsetMin) ||
            tzOffsetMin < -840 || tzOffsetMin > 840) {
          publishResult("err", "invalid_tz");
          return;
        }
      }

      uint32_t epochUtc = 0;
      uint32_t epochRtc = 0;
      if (!parseEpochForRtc(rawEpoch, epochUtc, epochRtc, tzOffsetMin)) {
        publishResult("err", "invalid_epoch");
        return;
      }

      timeval tv = {};
      tv.tv_sec = static_cast<time_t>(epochUtc);
      tv.tv_usec = 0;
      settimeofday(&tv, nullptr);
      syncSystemTime(epochRtc);

      publishResult("ack", "set_time");
      BleManager::notifyStatus();
      return;
    }

    publishResult("err", "unknown_action");
  }
};

class SettingsCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    std::string rawValue = pCharacteristic->getValue();
    if (rawValue.empty()) {
      publishResult("err", "empty_settings");
      return;
    }

    StaticJsonDocument<640> doc;
    DeserializationError err = deserializeJson(doc, rawValue);
    if (err) {
      Serial.printf("[BLE] Settings JSON parse error: %s\n", err.c_str());
      publishResult("err", "bad_json");
      return;
    }

    JsonObjectConst root = doc.as<JsonObjectConst>();
    if (root.isNull()) {
      publishResult("err", "bad_payload");
      return;
    }

    PowerManager::registerActivity();

    bool anyField = false;
    uint8_t parseInvalidFields = 0;
    ConfigPatch patch = {};
    float floatVal = 0.0f;
    long intVal = 0;

    if (root.containsKey("tar")) {
      anyField = true;
      if (tryGetFloat(root, "tar", floatVal)) {
        patch.hasTargetTemp = true;
        patch.targetTemp = floatVal;
      } else {
        parseInvalidFields++;
      }
    }

    if (root.containsKey("hys")) {
      anyField = true;
      if (tryGetFloat(root, "hys", floatVal)) {
        patch.hasTempHysteresis = true;
        patch.tempHysteresis = floatVal;
      } else {
        parseInvalidFields++;
      }
    }

    if (root.containsKey("fdH")) {
      anyField = true;
      if (tryGetInt(root, "fdH", intVal)) {
        patch.hasFeedHour = true;
        patch.feedHour = static_cast<int>(intVal);
      } else {
        parseInvalidFields++;
      }
    }

    if (root.containsKey("fdM")) {
      anyField = true;
      if (tryGetInt(root, "fdM", intVal)) {
        patch.hasFeedMinute = true;
        patch.feedMinute = static_cast<int>(intVal);
      } else {
        parseInvalidFields++;
      }
    }

    if (root.containsKey("fdF")) {
      anyField = true;
      if (tryGetInt(root, "fdF", intVal)) {
        patch.hasFeedMode = true;
        patch.feedMode = static_cast<int>(intVal);
      } else {
        parseInvalidFields++;
      }
    }

    if (root.containsKey("lsH")) {
      anyField = true;
      if (tryGetInt(root, "lsH", intVal)) {
        patch.hasDayStartHour = true;
        patch.dayStartHour = static_cast<int>(intVal);
      } else {
        parseInvalidFields++;
      }
    }

    if (root.containsKey("lsM")) {
      anyField = true;
      if (tryGetInt(root, "lsM", intVal)) {
        patch.hasDayStartMinute = true;
        patch.dayStartMinute = static_cast<int>(intVal);
      } else {
        parseInvalidFields++;
      }
    }

    if (root.containsKey("leH")) {
      anyField = true;
      if (tryGetInt(root, "leH", intVal)) {
        patch.hasDayEndHour = true;
        patch.dayEndHour = static_cast<int>(intVal);
      } else {
        parseInvalidFields++;
      }
    }

    if (root.containsKey("leM")) {
      anyField = true;
      if (tryGetInt(root, "leM", intVal)) {
        patch.hasDayEndMinute = true;
        patch.dayEndMinute = static_cast<int>(intVal);
      } else {
        parseInvalidFields++;
      }
    }

    if (root.containsKey("asH")) {
      anyField = true;
      if (tryGetInt(root, "asH", intVal)) {
        patch.hasAerationHourOn = true;
        patch.aerationHourOn = static_cast<int>(intVal);
      } else {
        parseInvalidFields++;
      }
    }

    if (root.containsKey("asM")) {
      anyField = true;
      if (tryGetInt(root, "asM", intVal)) {
        patch.hasAerationMinuteOn = true;
        patch.aerationMinuteOn = static_cast<int>(intVal);
      } else {
        parseInvalidFields++;
      }
    }

    if (root.containsKey("aeH")) {
      anyField = true;
      if (tryGetInt(root, "aeH", intVal)) {
        patch.hasAerationHourOff = true;
        patch.aerationHourOff = static_cast<int>(intVal);
      } else {
        parseInvalidFields++;
      }
    }

    if (root.containsKey("aeM")) {
      anyField = true;
      if (tryGetInt(root, "aeM", intVal)) {
        patch.hasAerationMinuteOff = true;
        patch.aerationMinuteOff = static_cast<int>(intVal);
      } else {
        parseInvalidFields++;
      }
    }

    if (root.containsKey("fsH")) {
      anyField = true;
      if (tryGetInt(root, "fsH", intVal)) {
        patch.hasFilterHourOn = true;
        patch.filterHourOn = static_cast<int>(intVal);
      } else {
        parseInvalidFields++;
      }
    }

    if (root.containsKey("fsM")) {
      anyField = true;
      if (tryGetInt(root, "fsM", intVal)) {
        patch.hasFilterMinuteOn = true;
        patch.filterMinuteOn = static_cast<int>(intVal);
      } else {
        parseInvalidFields++;
      }
    }

    if (root.containsKey("feH")) {
      anyField = true;
      if (tryGetInt(root, "feH", intVal)) {
        patch.hasFilterHourOff = true;
        patch.filterHourOff = static_cast<int>(intVal);
      } else {
        parseInvalidFields++;
      }
    }

    if (root.containsKey("feM")) {
      anyField = true;
      if (tryGetInt(root, "feM", intVal)) {
        patch.hasFilterMinuteOff = true;
        patch.filterMinuteOff = static_cast<int>(intVal);
      } else {
        parseInvalidFields++;
      }
    }

    if (root.containsKey("spO")) {
      anyField = true;
      if (tryGetInt(root, "spO", intVal)) {
        patch.hasServoPreOffMins = true;
        patch.servoPreOffMins = static_cast<int>(intVal);
      } else {
        parseInvalidFields++;
      }
    }

    if (!anyField) {
      publishResult("err", "empty_settings");
      return;
    }

    InterfaceRuleResult patchResult =
        InterfaceCore::applyConfigPatchAndSave(patch, parseInvalidFields);

    if (!patchResult.isOk()) {
      publishResult("err", patchResult.code == InterfaceRuleCode::SAVE_FAILED
                               ? "save_failed"
                               : "invalid_values");
      return;
    }

    BleManager::notifyStatus();
    Serial.println("[BLE] Settings updated & saved.");
    publishResult("ack", patchResult.isPartial() ? "settings_partial"
                                                 : "settings_saved");
  }

  void onRead(BLECharacteristic *pCharacteristic) override {
    char json[500];
    const Config cfg = ConfigManager::getCopy();
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

  esp_err_t mtuErr = BLEDevice::setMTU(BLE_PREFERRED_MTU);
  if (mtuErr != ESP_OK) {
    Serial.printf("[BLE] Nie udalo sie ustawic MTU %u (err=%d)\n",
                  BLE_PREFERRED_MTU, static_cast<int>(mtuErr));
  }

  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_MITM);
  BLEDevice::setSecurityCallbacks(new MySecurityCallbacks());

  BLESecurity *pSecurity = new BLESecurity();
  pSecurity->setStaticPIN(static_cast<uint32_t>(SECRET_BLE_PASSKEY));
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);
  pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
  pSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
  pSecurity->setKeySize(16);
  pSecurity->setCapability(ESP_IO_CAP_OUT);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Characteristic: status (read/notify)
  pCharStatus = pService->createCharacteristic(
      CHAR_STATUS_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pCharStatus->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED);
  pCharStatus->setCallbacks(new StatusCallbacks());
  BLE2902 *status2902 = new BLE2902();
  status2902->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED |
                                   ESP_GATT_PERM_WRITE_ENCRYPTED);
  pCharStatus->addDescriptor(status2902);

  // Characteristic: command (write)
  pCharCommand = pService->createCharacteristic(
      CHAR_COMMAND_UUID, BLECharacteristic::PROPERTY_WRITE);
  pCharCommand->setAccessPermissions(ESP_GATT_PERM_WRITE_ENCRYPTED);
  pCharCommand->setCallbacks(new CommandCallbacks());

  // Characteristic: settings (read/write)
  pCharSettings = pService->createCharacteristic(
      CHAR_SETTINGS_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  pCharSettings->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED |
                                      ESP_GATT_PERM_WRITE_ENCRYPTED);
  pCharSettings->setCallbacks(new SettingsCallbacks());

  // Characteristic: result ACK/ERR (read/notify)
  pCharResult = pService->createCharacteristic(
      CHAR_RESULT_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pCharResult->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED);
  BLE2902 *result2902 = new BLE2902();
  result2902->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED |
                                   ESP_GATT_PERM_WRITE_ENCRYPTED);
  pCharResult->addDescriptor(result2902);

  pService->start();

  char initialStatus[220];
  buildStatusJson(initialStatus, sizeof(initialStatus));
  pCharStatus->setValue((uint8_t *)initialStatus, strlen(initialStatus));
  const char *resultReady = "{\"t\":\"info\",\"c\":\"ready\"}";
  pCharResult->setValue((uint8_t *)resultReady, strlen(resultReady));

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMaxPreferred(0x12);

  bleInitialized = true;
  bleAdvertising = false;
  lastNotifyTime = 0;
  deviceConnected = false;
  resumeAdvertisingPending = false;
  resumeAdvertisingAtMs = 0;
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
  resumeAdvertisingPending = false;
  lastNotifyTime = 0;
  refreshConnectionState();
  Serial.println("[BLE] Advertising wlaczony.");
}

void BleManager::stop() {
  if (!bleInitialized) {
    return;
  }

  if (pServer) {
    std::map<uint16_t, conn_status_t> peers = pServer->getPeerDevices(false);
    for (const auto &entry : peers) {
      pServer->disconnect(entry.first);
    }
  }

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  if (pAdvertising) {
    pAdvertising->stop();
  }

  bleAdvertising = false;
  resumeAdvertisingPending = false;
  resumeAdvertisingAtMs = 0;
  deviceConnected = false;
  lastNotifyTime = 0;
  Serial.println("[BLE] Advertising wylaczony.");
}

void BleManager::notifyStatus() {
  if (!pCharStatus) {
    return;
  }

  char json[220];
  buildStatusJson(json, sizeof(json));
  pCharStatus->setValue((uint8_t *)json, strlen(json));

  refreshConnectionState();
  if (deviceConnected) {
    pCharStatus->notify();
  }
}

void BleManager::update() {
  if (!bleAdvertising) {
    return;
  }

  refreshConnectionState();

  if (resumeAdvertisingPending && !deviceConnected &&
      static_cast<long>(millis() - resumeAdvertisingAtMs) >= 0) {
    if (pServer) {
      pServer->startAdvertising();
      Serial.println("[BLE] Wznowiono advertising po rozlaczeniu");
    }
    resumeAdvertisingPending = false;
  }

  if (deviceConnected && (millis() - lastNotifyTime >= NOTIFY_INTERVAL_MS)) {
    notifyStatus();
    lastNotifyTime = millis();
  }
}

bool BleManager::isConnected() {
  refreshConnectionState();
  return deviceConnected;
}

bool BleManager::isAdvertising() { return bleAdvertising; }

uint8_t BleManager::getConnectedClients() {
  if (pServer == nullptr) {
    return deviceConnected ? 1 : 0;
  }

  uint32_t count = pServer->getConnectedCount();
  return (count > 255) ? 255 : static_cast<uint8_t>(count);
}

const char *BleManager::getDeviceName() { return BLE_DEVICE_NAME; }

uint32_t BleManager::getPasskey() {
  return static_cast<uint32_t>(SECRET_BLE_PASSKEY);
}
