#include "BleManager.h"
#include "AkwariumWifi.h"
#include "ConfigManager.h"
#include "ConfigValidation.h"
#include "FirmwareInfo.h"
#include "LogManager.h"
#include "OtaManager.h"
#include "PowerManager.h"
#include "SecretConfig.h"
#include "SharedState.h"
#include "SystemController.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLESecurity.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <esp_err.h>
#include <esp_ota_ops.h>

#include <cstdlib>
#include <cstring>
#include <map>
#include <string>

// UUIDs
#define SERVICE_UUID "4fafc201-1fb5-459e-8bcc-c5c9c331914b"
#define CHAR_STATUS_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_COMMAND_UUID "828917c1-ea55-4d4a-a66e-fd202cea0645"
#define CHAR_SETTINGS_UUID "d2912856-de63-11ed-b5ea-0242ac120002"
#define CHAR_RESULT_UUID "8e22cb9c-1728-45f9-8c50-2f7252f07379"
#define CHAR_DEVICE_INFO_UUID "73d4b922-9d7d-4f5a-9f88-0871b07ec21b"
#define CHAR_OTA_CONTROL_UUID "b5f6d0d0-0c6a-4cb0-a9b8-6b4e6cb6e550"
#define CHAR_OTA_DATA_UUID "f2a4f5f5-89d0-4d3c-a4f7-e1db30c6ff0c"

static BLEServer *pServer = nullptr;
static BLECharacteristic *pCharStatus = nullptr;
static BLECharacteristic *pCharCommand = nullptr;
static BLECharacteristic *pCharSettings = nullptr;
static BLECharacteristic *pCharResult = nullptr;
static BLECharacteristic *pCharDeviceInfo = nullptr;
static BLECharacteristic *pCharOtaControl = nullptr;
static BLECharacteristic *pCharOtaData = nullptr;

static volatile bool deviceConnected = false;
static bool bleInitialized = false;
static volatile bool bleAdvertising = false;
static volatile uint8_t connectedClientCount = 0;
static volatile bool bleAdvertisingDesired = false;
static volatile bool bleAuthenticated = false;
static volatile bool blePostAuthSyncPending = false;
static volatile bool resumeAdvertisingPending = false;
static volatile unsigned long resumeAdvertisingAtMs = 0;
static unsigned long lastNotifyTime = 0;
static bool bleOtaActive = false;
static esp_ota_handle_t bleOtaHandle = 0;
static const esp_partition_t *bleOtaPartition = nullptr;
static size_t bleOtaExpectedSize = 0;
static size_t bleOtaReceivedSize = 0;
static unsigned long bleOtaLastChunkAtMs = 0;
static unsigned long bleOtaLastProgressNotifyAtMs = 0;
static bool bleOtaRestartPending = false;
static unsigned long bleOtaRestartAtMs = 0;
static char bleOtaLastType[12] = "info";
static char bleOtaLastCode[32] = "ready";
static char bleOtaDeclaredVersion[32] = "";
static char bleOtaDeclaredProject[32] = "";

static const unsigned long NOTIFY_INTERVAL_MS = 2000;
static const unsigned long BLE_COMMAND_MIN_INTERVAL_MS = 120;
static const unsigned long BLE_SETTINGS_MIN_INTERVAL_MS = 250;
static const unsigned long RESUME_ADVERTISING_DELAY_MS = 150;
static const unsigned long BLE_OTA_STALL_TIMEOUT_MS = 15000;
static const unsigned long BLE_OTA_PROGRESS_NOTIFY_INTERVAL_MS = 300;
static const unsigned long BLE_OTA_RESTART_DELAY_MS = 1200;
static const uint16_t BLE_PREFERRED_MTU = 185;
static const char *BLE_DEVICE_NAME = "Akwarium_BLE";
static const size_t BLE_OTA_CHUNK_HEADER_SIZE = 4;
static const size_t BLE_OTA_RECOMMENDED_DATA_CHUNK_BYTES = 160;
static unsigned long lastCommandAtMs = 0;
static unsigned long lastSettingsAtMs = 0;
static portMUX_TYPE bleStateMux = portMUX_INITIALIZER_UNLOCKED;

// --- Deferred BLE command queue (flag-based) ---
// Komendy BLE nie moga byc wykonywane bezposrednio w callbacku BLE,
// poniewaz stos NimBLE jest za maly (~4KB) na operacje sprzetowe
// i alokacje JSON, co prowadzi do watchdog reset / stack overflow.
struct PendingBleCommand {
  enum class Type : uint8_t {
    NONE,
    FEED_NOW,
    SET_SERVO,
    CLEAR_SERVO,
    CLEAR_LOGS
  };
  Type type = Type::NONE;
  int servoAngle = 0;
};
static volatile bool hasPendingBleCommand = false;
static PendingBleCommand pendingBleCommand = {};

static void abortBleOtaSession(const char *code, const char *reason);

static void cleanupExcessBonds(int maxBonds) {
  int devNum = esp_ble_get_bond_device_num();
  if (devNum <= maxBonds)
    return;

  esp_ble_bond_dev_t *devList =
      (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * devNum);
  if (!devList)
    return;

  esp_ble_get_bond_device_list(&devNum, devList);
  for (int i = 0; i < devNum - maxBonds; i++) {
    esp_ble_remove_bond_device(devList[i].bd_addr);
    Serial.printf("[BLE] Usunieto stary bond #%d\n", i);
  }
  free(devList);
}

static void setBleAdvertisingDesired(bool desired) {
  portENTER_CRITICAL(&bleStateMux);
  bleAdvertisingDesired = desired;
  portEXIT_CRITICAL(&bleStateMux);
}

static bool getBleAdvertisingDesired() {
  portENTER_CRITICAL(&bleStateMux);
  const bool desired = bleAdvertisingDesired;
  portEXIT_CRITICAL(&bleStateMux);
  return desired;
}

static void cacheConnectionCount(uint32_t count) {
  const uint8_t capped = (count > 255U) ? 255U : static_cast<uint8_t>(count);
  portENTER_CRITICAL(&bleStateMux);
  connectedClientCount = capped;
  deviceConnected = (count > 0U);
  portEXIT_CRITICAL(&bleStateMux);
}

static void setBleAdvertisingActive(bool active) {
  portENTER_CRITICAL(&bleStateMux);
  bleAdvertising = active;
  portEXIT_CRITICAL(&bleStateMux);
}

static void setBleAuthenticated(bool authenticated) {
  portENTER_CRITICAL(&bleStateMux);
  bleAuthenticated = authenticated;
  portEXIT_CRITICAL(&bleStateMux);
}

static bool isBleAuthenticated() {
  portENTER_CRITICAL(&bleStateMux);
  const bool authenticated = bleAuthenticated;
  portEXIT_CRITICAL(&bleStateMux);
  return authenticated;
}

static void setBlePostAuthSyncPending(bool pending) {
  portENTER_CRITICAL(&bleStateMux);
  blePostAuthSyncPending = pending;
  portEXIT_CRITICAL(&bleStateMux);
}

static bool isBlePostAuthSyncPending() {
  portENTER_CRITICAL(&bleStateMux);
  const bool pending = blePostAuthSyncPending;
  portEXIT_CRITICAL(&bleStateMux);
  return pending;
}

static void refreshConnectionState() {
  if (pServer) {
    cacheConnectionCount(pServer->getConnectedCount());
  } else {
    cacheConnectionCount(0);
  }
}

static void startAdvertisingNow() {
  if (!bleInitialized || bleAdvertising) {
    return;
  }

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  if (!pAdvertising) {
    return;
  }

  pAdvertising->start();
  setBleAdvertisingActive(true);
  resumeAdvertisingPending = false;
  lastNotifyTime = 0;
  refreshConnectionState();
  Serial.println("[BLE] Advertising wlaczony.");
}

static void stopAdvertisingNow() {
  if (!bleInitialized || !bleAdvertising) {
    return;
  }

  if (bleOtaActive || bleOtaHandle != 0) {
    abortBleOtaSession(
        "ota_aborted",
        "Sesja BLE OTA zostala przerwana podczas zatrzymywania BLE.");
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

  setBleAdvertisingActive(false);
  setBleAuthenticated(false);
  setBlePostAuthSyncPending(false);
  resumeAdvertisingPending = false;
  resumeAdvertisingAtMs = 0;
  cacheConnectionCount(0);
  lastNotifyTime = 0;
  Serial.println("[BLE] Advertising wylaczony.");
}

static bool isRateLimited(unsigned long &lastAtMs,
                          unsigned long minIntervalMs) {
  unsigned long nowMs = millis();
  if (lastAtMs != 0 && (nowMs - lastAtMs) < minIntervalMs) {
    return true;
  }
  lastAtMs = nowMs;
  return false;
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

static void publishResult(const char *type, const char *code) {
  if (!bleInitialized || !pCharResult || !deviceConnected ||
      !isBleAuthenticated()) {
    return;
  }

  char json[96];
  snprintf(json, sizeof(json), "{\"t\":\"%s\",\"c\":\"%s\"}", type, code);
  pCharResult->setValue((uint8_t *)json, strlen(json));
  pCharResult->notify();
}

static uint32_t decodeUInt32LE(const uint8_t *data) {
  return static_cast<uint32_t>(data[0]) |
         (static_cast<uint32_t>(data[1]) << 8U) |
         (static_cast<uint32_t>(data[2]) << 16U) |
         (static_cast<uint32_t>(data[3]) << 24U);
}

static void setOtaLastState(const char *type, const char *code) {
  snprintf(bleOtaLastType, sizeof(bleOtaLastType), "%s",
           (type != nullptr && type[0] != '\0') ? type : "info");
  snprintf(bleOtaLastCode, sizeof(bleOtaLastCode), "%s",
           (code != nullptr && code[0] != '\0') ? code : "unknown");
}

static void populateValidationJson(JsonObject validation) {
  const ValidationProfileSnapshot profile =
      ConfigValidation::getValidationProfile();

  validation["ms"] = profile.minuteStep;
  validation["sch"] = "schedule|always_on|always_off";
  validation["heat"] = "threshold|off";
  validation["req"] = true;

  JsonObject temperature = validation.createNestedObject("tmp");
  temperature["min"] = profile.minTemperature;
  temperature["max"] = profile.maxTemperature;
  temperature["step"] = profile.temperatureStep;
  temperature["off"] = true;

  JsonObject hysteresis = validation.createNestedObject("hys");
  hysteresis["min"] = profile.hysteresisMin;
  hysteresis["max"] = profile.hysteresisMax;
  hysteresis["step"] = profile.hysteresisStep;

  JsonObject feeding = validation.createNestedObject("fd");
  feeding["min"] = profile.feedModeMin;
  feeding["max"] = profile.feedModeMax;

  JsonObject preOff = validation.createNestedObject("sp");
  preOff["min"] = profile.servoPreOffMin;
  preOff["max"] = profile.servoPreOffMax;
  preOff["step"] = 1;
}

static String buildDeviceInfoJsonString() {
  const FirmwareRuntimeInfo firmwareInfo = FirmwareInfo::getRuntimeInfo();

  DynamicJsonDocument doc(896);
  doc["nm"] = firmwareInfo.firmwareName;
  doc["ver"] = firmwareInfo.firmwareVersion;
  doc["ref"] = firmwareInfo.buildRef;
  doc["dt"] = firmwareInfo.buildDate;
  doc["tm"] = firmwareInfo.buildTime;
  doc["idf"] = firmwareInfo.idfVersion;
  doc["rp"] = firmwareInfo.runningPartitionLabel;
  doc["bp"] = firmwareInfo.bootPartitionLabel;
  doc["np"] = firmwareInfo.nextPartitionLabel;
  doc["slot"] = firmwareInfo.nextPartitionSizeBytes;
  doc["busy"] = OtaManager::isOtaInProgress();
  doc["ota"] = OtaManager::getActiveTransport();
  doc["ble"] = FirmwareInfo::supportsBleOta();
  doc["http"] = FirmwareInfo::supportsHttpOta();
  doc["chunk"] = BLE_OTA_RECOMMENDED_DATA_CHUNK_BYTES;
  populateValidationJson(doc.createNestedObject("val"));

  String json;
  json.reserve(384);
  serializeJson(doc, json);
  return json;
}

static void updateDeviceInfoValue() {
  if (!pCharDeviceInfo) {
    return;
  }

  String json = buildDeviceInfoJsonString();
  pCharDeviceInfo->setValue((uint8_t *)json.c_str(), json.length());
}

static String buildOtaStateJsonString() {
  unsigned long rebootDelayMs = 0;
  if (bleOtaRestartPending) {
    long deltaMs = static_cast<long>(bleOtaRestartAtMs - millis());
    rebootDelayMs = deltaMs > 0 ? static_cast<unsigned long>(deltaMs) : 0UL;
  }

  char json[256];
  snprintf(json, sizeof(json),
           "{\"t\":\"%s\",\"c\":\"%s\",\"busy\":%s,\"ota\":\"%s\","
           "\"rx\":%u,\"size\":%u,\"chunk\":%u,\"reboot\":%lu,"
           "\"ver\":\"%s\",\"prj\":\"%s\"}",
           bleOtaLastType, bleOtaLastCode, bleOtaActive ? "true" : "false",
           OtaManager::getActiveTransport(),
           static_cast<unsigned int>(bleOtaReceivedSize),
           static_cast<unsigned int>(bleOtaExpectedSize),
           static_cast<unsigned int>(BLE_OTA_RECOMMENDED_DATA_CHUNK_BYTES),
           rebootDelayMs,
           bleOtaDeclaredVersion[0] != '\0' ? bleOtaDeclaredVersion : "",
           bleOtaDeclaredProject[0] != '\0' ? bleOtaDeclaredProject : "");
  return String(json);
}

static void publishOtaState(const char *type, const char *code) {
  setOtaLastState(type, code);
  updateDeviceInfoValue();

  if (!pCharOtaControl) {
    return;
  }

  String json = buildOtaStateJsonString();
  pCharOtaControl->setValue((uint8_t *)json.c_str(), json.length());

  refreshConnectionState();
  if (deviceConnected && isBleAuthenticated()) {
    pCharOtaControl->notify();
  }
}

static void clearBleOtaSessionState(bool clearRestartFlag) {
  bleOtaActive = false;
  bleOtaHandle = 0;
  bleOtaPartition = nullptr;
  bleOtaExpectedSize = 0;
  bleOtaReceivedSize = 0;
  bleOtaLastChunkAtMs = 0;
  bleOtaLastProgressNotifyAtMs = 0;
  bleOtaDeclaredVersion[0] = '\0';
  bleOtaDeclaredProject[0] = '\0';
  if (clearRestartFlag) {
    bleOtaRestartPending = false;
    bleOtaRestartAtMs = 0;
  }
}

static void abortBleOtaSession(const char *code, const char *reason) {
  if (bleOtaHandle != 0) {
    esp_ota_abort(bleOtaHandle);
  }

  clearBleOtaSessionState(true);
  OtaManager::cancelOtaUpdate(reason);
  publishOtaState("err", code);
}

static String buildStatusJsonString() {
  SharedStateData snap = SharedState::getSnapshot();
  const Config cfg = ConfigManager::getCopy();

  float vBat = PowerManager::getBatteryVoltage();
  if (isnan(vBat)) {
    vBat = 0.0f;
  }

  const uint8_t clients = connectedClientCount;

  DynamicJsonDocument doc(768);
  doc["tmp"] = isnan(snap.temperature) ? -99.9f : snap.temperature;
  doc["tar"] = cfg.targetTemp;
  doc["thr"] = cfg.targetTemp;
  doc["hm"] = cfg.heaterMode;
  doc["hys"] = cfg.tempHysteresis;
  doc["mn"] = isnan(snap.minTemp) ? -99.9f : snap.minTemp;
  doc["me"] = snap.minTempEpoch;
  doc["bv"] = vBat;
  doc["bp"] = PowerManager::getBatteryPercent();
  doc["l"] = snap.isLightOn;
  doc["f"] = snap.isFilterOn;
  doc["h"] = snap.isHeaterOn;
  doc["srv"] = SystemController::getServoPosition();
  doc["ip"] = AkwariumWifi::getIP();
  doc["ap"] = AkwariumWifi::getIsAPMode();
  doc["cli"] = clients;
  doc["lm"] = cfg.lightMode;
  doc["am"] = cfg.aerationMode;
  doc["fm"] = cfg.filterMode;
  doc["rst"] = SystemController::getLastResetReason();
  const char *rstLabel = SystemController::getLastResetLabel();
  if (rstLabel != nullptr) {
    doc["err"] = rstLabel;
  }

  String json;
  json.reserve(256);
  serializeJson(doc, json);
  return json;
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
      setBleAuthenticated(false);
      setBlePostAuthSyncPending(false);
      Serial.printf("[BLE] Autoryzacja nieudana, reason=%d\n",
                    auth_cmpl.fail_reason);
      // Usun uszkodzony bond, zeby ponowne parowanie bylo mozliwe
      esp_ble_remove_bond_device(auth_cmpl.bd_addr);
      Serial.println("[BLE] Usunięto bond po nieudanej autoryzacji.");
    } else {
      setBleAuthenticated(true);
      setBlePostAuthSyncPending(true);
      Serial.println("[BLE] Autoryzacja zakonczona sukcesem (bonded). ");
      cleanupExcessBonds(4);
    }
  }
};

class MyServerCallbacks : public BLEServerCallbacks {
private:
  static void handleConnect(BLEServer *server) {
    cacheConnectionCount(server ? server->getConnectedCount() : 1U);
    setBleAuthenticated(false);
    setBlePostAuthSyncPending(false);
    resumeAdvertisingPending = false;
    lastNotifyTime = 0;

    uint32_t count = server ? server->getConnectedCount() : 1;
    Serial.printf("[BLE] Client polaczony. Klienci: %lu\n",
                  static_cast<unsigned long>(count));
  }

  static void handleDisconnect(BLEServer *server) {
    uint32_t count = server ? server->getConnectedCount() : 0;
    cacheConnectionCount(count);
    setBleAuthenticated(false);
    setBlePostAuthSyncPending(false);
    Serial.printf("[BLE] Client rozlaczony. Klienci: %lu\n",
                  static_cast<unsigned long>(count));

    if (bleAdvertising && !deviceConnected && getBleAdvertisingDesired()) {
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
    if (!bleInitialized || !pCharacteristic)
      return;
    String json = buildStatusJsonString();
    pCharacteristic->setValue((uint8_t *)json.c_str(), json.length());
  }
};

class CommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    if (!bleInitialized || !pCharacteristic)
      return;
    if (OtaManager::isOtaInProgress()) {
      publishResult("err", "ota_busy");
      return;
    }

    if (isRateLimited(lastCommandAtMs, BLE_COMMAND_MIN_INTERVAL_MS)) {
      publishResult("err", "rate_limited");
      return;
    }

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

    // --- Deferred command execution ---
    // Komendy hardware NIE SA wykonywane bezposrednio w callbacku BLE!
    // Ustawiamy flage, a wykonanie nastapi w BleManager::update() na głównym
    // wątku.

    if (strcmp(action, "feed_now") == 0) {
      portENTER_CRITICAL(&bleStateMux);
      pendingBleCommand.type = PendingBleCommand::Type::FEED_NOW;
      hasPendingBleCommand = true;
      portEXIT_CRITICAL(&bleStateMux);
      publishResult("ack", "feed_now");
      return;
    }

    if (strcmp(action, "set_servo") == 0) {
      long angle = 0;
      if (!tryGetInt(root, "angle", angle)) {
        publishResult("err", "missing_angle");
        return;
      }
      portENTER_CRITICAL(&bleStateMux);
      pendingBleCommand.type = PendingBleCommand::Type::SET_SERVO;
      pendingBleCommand.servoAngle = constrain(static_cast<int>(angle), 0, 90);
      hasPendingBleCommand = true;
      portEXIT_CRITICAL(&bleStateMux);
      publishResult("ack", "set_servo");
      return;
    }

    if (strcmp(action, "clear_servo") == 0) {
      portENTER_CRITICAL(&bleStateMux);
      pendingBleCommand.type = PendingBleCommand::Type::CLEAR_SERVO;
      hasPendingBleCommand = true;
      portEXIT_CRITICAL(&bleStateMux);
      publishResult("ack", "clear_servo");
      return;
    }

    if (strcmp(action, "clear_critical_logs") == 0) {
      portENTER_CRITICAL(&bleStateMux);
      pendingBleCommand.type = PendingBleCommand::Type::CLEAR_LOGS;
      hasPendingBleCommand = true;
      portEXIT_CRITICAL(&bleStateMux);
      publishResult("ack", "clear_logs");
      return;
    }

    publishResult("err", "unknown_action");
  }
};

class SettingsCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    if (!bleInitialized || !pCharacteristic)
      return;
    if (OtaManager::isOtaInProgress()) {
      publishResult("err", "ota_busy");
      return;
    }

    if (isRateLimited(lastSettingsAtMs, BLE_SETTINGS_MIN_INTERVAL_MS)) {
      publishResult("err", "rate_limited");
      return;
    }

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

    if (root.containsKey("lm")) {
      anyField = true;
      if (tryGetInt(root, "lm", intVal)) {
        patch.hasLightMode = true;
        patch.lightMode = static_cast<int>(intVal);
      } else {
        parseInvalidFields++;
      }
    }

    if (root.containsKey("am")) {
      anyField = true;
      if (tryGetInt(root, "am", intVal)) {
        patch.hasAerationMode = true;
        patch.aerationMode = static_cast<int>(intVal);
      } else {
        parseInvalidFields++;
      }
    }

    if (root.containsKey("fm")) {
      anyField = true;
      if (tryGetInt(root, "fm", intVal)) {
        patch.hasFilterMode = true;
        patch.filterMode = static_cast<int>(intVal);
      } else {
        parseInvalidFields++;
      }
    }

    if (root.containsKey("hm")) {
      anyField = true;
      if (tryGetInt(root, "hm", intVal)) {
        patch.hasHeaterMode = true;
        patch.heaterMode = static_cast<int>(intVal);
      } else {
        parseInvalidFields++;
      }
    }

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

    if (parseInvalidFields > 0) {
      publishResult("err", "invalid_payload");
      return;
    }

    Config cfg = ConfigManager::getCopy();
    ConfigValidationResult validation = {};
    if (!ConfigValidation::applyRuntimePatch(cfg, patch, validation)) {
      publishResult("err", validation.errorCode[0] != '\0'
                               ? validation.errorCode
                               : "invalid_values");
      return;
    }

    if (!ConfigManager::updateAndSave(cfg)) {
      publishResult("err", "save_failed");
      return;
    }

    // notifyStatus() NIE jest wołane tutaj - ciezkie alokacje
    // (DynamicJsonDocument
    // + String + serializeJson) na stosie BLE callbacku (~4KB) powodowaly
    // stack overflow -> LoadProhibited panic. Status zostanie zsynchronizowany
    // automatycznie w BleManager::update() co 2 sekundy.
    Serial.println("[BLE] Settings updated & saved.");
    publishResult("ack", "settings_saved");
  }

  void onRead(BLECharacteristic *pCharacteristic) override {
    if (!bleInitialized || !pCharacteristic)
      return;
    const Config cfg = ConfigManager::getCopy();
    DynamicJsonDocument doc(640);
    doc["lm"] = cfg.lightMode;
    doc["tar"] = cfg.targetTemp;
    doc["hm"] = cfg.heaterMode;
    doc["hys"] = cfg.tempHysteresis;
    doc["fdH"] = cfg.feedHour;
    doc["fdM"] = cfg.feedMinute;
    doc["fdF"] = cfg.feedMode;
    doc["lsH"] = cfg.dayStartHour;
    doc["lsM"] = cfg.dayStartMinute;
    doc["leH"] = cfg.dayEndHour;
    doc["leM"] = cfg.dayEndMinute;
    doc["am"] = cfg.aerationMode;
    doc["asH"] = cfg.aerationHourOn;
    doc["asM"] = cfg.aerationMinuteOn;
    doc["aeH"] = cfg.aerationHourOff;
    doc["aeM"] = cfg.aerationMinuteOff;
    doc["fm"] = cfg.filterMode;
    doc["fsH"] = cfg.filterHourOn;
    doc["fsM"] = cfg.filterMinuteOn;
    doc["feH"] = cfg.filterHourOff;
    doc["feM"] = cfg.filterMinuteOff;
    doc["spO"] = cfg.servoPreOffMins;

    String json;
    json.reserve(192);
    serializeJson(doc, json);
    pCharacteristic->setValue((uint8_t *)json.c_str(), json.length());
  }
};

class ResultCallbacks : public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic *pCharacteristic) override {
    if (!bleInitialized || !pCharacteristic)
      return;
    const char *json = "{\"t\":\"info\",\"c\":\"result\"}";
    pCharacteristic->setValue((uint8_t *)json, strlen(json));
  }
};

class DeviceInfoCallbacks : public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic *pCharacteristic) override {
    if (!bleInitialized || !pCharacteristic)
      return;
    String json = buildDeviceInfoJsonString();
    pCharacteristic->setValue((uint8_t *)json.c_str(), json.length());
  }
};

class OtaControlCallbacks : public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic *pCharacteristic) override {
    if (!bleInitialized || !pCharacteristic)
      return;
    String json = buildOtaStateJsonString();
    pCharacteristic->setValue((uint8_t *)json.c_str(), json.length());
  }

  void onWrite(BLECharacteristic *pCharacteristic) override {
    if (!bleInitialized || !pCharacteristic)
      return;
    std::string rawValue = pCharacteristic->getValue();
    if (rawValue.empty()) {
      publishOtaState("err", "ota_empty_request");
      return;
    }

    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, rawValue);
    if (err) {
      Serial.printf("[BLE OTA] Control JSON parse error: %s\n", err.c_str());
      publishOtaState("err", "bad_json");
      return;
    }

    JsonObjectConst root = doc.as<JsonObjectConst>();
    const char *action = root["action"] | "";
    if (action[0] == '\0') {
      publishOtaState("err", "missing_action");
      return;
    }

    PowerManager::registerActivity();

    if (strcmp(action, "status") == 0) {
      publishOtaState("info", bleOtaActive ? "ota_receiving" : "ready");
      return;
    }

    if (strcmp(action, "abort") == 0) {
      if (!bleOtaActive || bleOtaHandle == 0) {
        publishOtaState("err", "ota_not_started");
        return;
      }

      abortBleOtaSession("ota_aborted",
                         "Sesja BLE OTA anulowana przez klienta.");
      return;
    }

    if (strcmp(action, "begin") == 0) {
      if (bleOtaActive || OtaManager::isOtaInProgress()) {
        publishOtaState("err", "ota_busy");
        return;
      }

      long imageSizeValue = 0;
      if (!tryGetInt(root, "size", imageSizeValue) || imageSizeValue <= 0) {
        publishOtaState("err", "ota_bad_size");
        return;
      }

      const esp_partition_t *updatePartition =
          esp_ota_get_next_update_partition(nullptr);
      if (updatePartition == nullptr) {
        publishOtaState("err", "ota_no_partition");
        return;
      }

      const size_t imageSize = static_cast<size_t>(imageSizeValue);
      if (imageSize > static_cast<size_t>(updatePartition->size)) {
        publishOtaState("err", "ota_too_large");
        return;
      }

      if (!OtaManager::tryBeginOtaUpdate("ble")) {
        publishOtaState("err", "ota_busy");
        return;
      }

      esp_ota_handle_t updateHandle = 0;
      esp_err_t otaErr =
          esp_ota_begin(updatePartition, imageSize, &updateHandle);
      if (otaErr != ESP_OK) {
        Serial.printf("[BLE OTA] esp_ota_begin failed: %s (%d)\n",
                      esp_err_to_name(otaErr), static_cast<int>(otaErr));
        OtaManager::cancelOtaUpdate("Nie udalo sie zainicjowac OTA przez BLE.");
        publishOtaState("err", "ota_begin_failed");
        return;
      }

      bleOtaHandle = updateHandle;
      bleOtaPartition = updatePartition;
      bleOtaExpectedSize = imageSize;
      bleOtaReceivedSize = 0;
      bleOtaActive = true;
      bleOtaLastChunkAtMs = millis();
      bleOtaLastProgressNotifyAtMs = 0;
      bleOtaRestartPending = false;
      bleOtaRestartAtMs = 0;
      bleOtaDeclaredVersion[0] = '\0';
      bleOtaDeclaredProject[0] = '\0';

      const char *declaredVersion = root["version"] | "";
      const char *declaredProject = root["project"] | "";
      if (declaredVersion[0] != '\0') {
        snprintf(bleOtaDeclaredVersion, sizeof(bleOtaDeclaredVersion), "%s",
                 declaredVersion);
      }
      if (declaredProject[0] != '\0') {
        snprintf(bleOtaDeclaredProject, sizeof(bleOtaDeclaredProject), "%s",
                 declaredProject);
      }

      Serial.printf("[BLE OTA] Przyjeto obraz %u B dla partycji %s\n",
                    static_cast<unsigned int>(bleOtaExpectedSize),
                    bleOtaPartition->label);
      publishOtaState("ack", "ota_ready");
      return;
    }

    if (strcmp(action, "finish") == 0) {
      if (!bleOtaActive || bleOtaHandle == 0 || bleOtaPartition == nullptr) {
        publishOtaState("err", "ota_not_started");
        return;
      }

      if (bleOtaReceivedSize != bleOtaExpectedSize) {
        publishOtaState("err", "ota_size_mismatch");
        return;
      }

      esp_err_t endErr = esp_ota_end(bleOtaHandle);
      bleOtaHandle = 0;
      if (endErr != ESP_OK) {
        Serial.printf("[BLE OTA] esp_ota_end failed: %s (%d)\n",
                      esp_err_to_name(endErr), static_cast<int>(endErr));
        clearBleOtaSessionState(true);
        OtaManager::endOtaUpdate(false);
        publishOtaState("err", "ota_end_failed");
        return;
      }

      esp_err_t bootErr = esp_ota_set_boot_partition(bleOtaPartition);
      if (bootErr != ESP_OK) {
        Serial.printf("[BLE OTA] set boot partition failed: %s (%d)\n",
                      esp_err_to_name(bootErr), static_cast<int>(bootErr));
        clearBleOtaSessionState(true);
        OtaManager::endOtaUpdate(false);
        publishOtaState("err", "ota_boot_failed");
        return;
      }

      bleOtaActive = false;
      bleOtaPartition = nullptr;
      bleOtaExpectedSize = bleOtaReceivedSize;
      bleOtaLastChunkAtMs = millis();
      bleOtaRestartPending = true;
      bleOtaRestartAtMs = millis() + BLE_OTA_RESTART_DELAY_MS;
      OtaManager::endOtaUpdate(true);
      publishOtaState("ack", "ota_complete");
      return;
    }

    publishOtaState("err", "unknown_action");
  }
};

class OtaDataCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    if (!bleOtaActive || bleOtaHandle == 0) {
      publishOtaState("err", "ota_not_started");
      return;
    }

    std::string rawValue = pCharacteristic->getValue();
    if (rawValue.size() <= BLE_OTA_CHUNK_HEADER_SIZE) {
      publishOtaState("err", "ota_empty_chunk");
      return;
    }

    PowerManager::registerActivity();

    const uint8_t *rawData = reinterpret_cast<const uint8_t *>(rawValue.data());
    const uint32_t chunkOffset = decodeUInt32LE(rawData);
    const size_t chunkSize = rawValue.size() - BLE_OTA_CHUNK_HEADER_SIZE;

    if (chunkOffset != bleOtaReceivedSize) {
      abortBleOtaSession(
          "ota_offset_mismatch",
          "Pakiety BLE OTA przyszly poza oczekiwana kolejnoscia.");
      return;
    }

    if ((bleOtaReceivedSize + chunkSize) > bleOtaExpectedSize) {
      abortBleOtaSession("ota_overflow",
                         "Pakiet BLE OTA przekroczyl zadeklarowany rozmiar.");
      return;
    }

    esp_err_t otaErr = esp_ota_write(
        bleOtaHandle, rawData + BLE_OTA_CHUNK_HEADER_SIZE, chunkSize);
    if (otaErr != ESP_OK) {
      Serial.printf("[BLE OTA] esp_ota_write failed: %s (%d)\n",
                    esp_err_to_name(otaErr), static_cast<int>(otaErr));
      abortBleOtaSession("ota_write_failed",
                         "Nie udalo sie zapisac danych OTA do flash.");
      return;
    }

    bleOtaReceivedSize += chunkSize;
    bleOtaLastChunkAtMs = millis();

    if (bleOtaReceivedSize == bleOtaExpectedSize ||
        (millis() - bleOtaLastProgressNotifyAtMs) >=
            BLE_OTA_PROGRESS_NOTIFY_INTERVAL_MS) {
      bleOtaLastProgressNotifyAtMs = millis();
      publishOtaState("info", "ota_receiving");
    } else {
      updateDeviceInfoValue();
    }
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
  pSecurity->setCapability(ESP_IO_CAP_KBDISP);

  // Usun nadmiarowe bondy z NVS przy starcie
  cleanupExcessBonds(4);

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

  // Characteristic: device info (read/notify)
  pCharDeviceInfo = pService->createCharacteristic(
      CHAR_DEVICE_INFO_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pCharDeviceInfo->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED);
  pCharDeviceInfo->setCallbacks(new DeviceInfoCallbacks());
  BLE2902 *deviceInfo2902 = new BLE2902();
  deviceInfo2902->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED |
                                       ESP_GATT_PERM_WRITE_ENCRYPTED);
  pCharDeviceInfo->addDescriptor(deviceInfo2902);

  // Characteristic: OTA control (read/write/notify)
  pCharOtaControl = pService->createCharacteristic(
      CHAR_OTA_CONTROL_UUID, BLECharacteristic::PROPERTY_READ |
                                 BLECharacteristic::PROPERTY_WRITE |
                                 BLECharacteristic::PROPERTY_NOTIFY);
  pCharOtaControl->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED |
                                        ESP_GATT_PERM_WRITE_ENCRYPTED);
  pCharOtaControl->setCallbacks(new OtaControlCallbacks());
  BLE2902 *otaControl2902 = new BLE2902();
  otaControl2902->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED |
                                       ESP_GATT_PERM_WRITE_ENCRYPTED);
  pCharOtaControl->addDescriptor(otaControl2902);

  // Characteristic: OTA data stream (write)
  pCharOtaData = pService->createCharacteristic(
      CHAR_OTA_DATA_UUID,
      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  pCharOtaData->setAccessPermissions(ESP_GATT_PERM_WRITE_ENCRYPTED);
  pCharOtaData->setCallbacks(new OtaDataCallbacks());

  pService->start();

  if (pCharStatus) {
    String initialStatus = buildStatusJsonString();
    pCharStatus->setValue((uint8_t *)initialStatus.c_str(),
                          initialStatus.length());
  }

  if (pCharResult) {
    const char *resultReady = "{\"t\":\"info\",\"c\":\"ready\"}";
    pCharResult->setValue((uint8_t *)resultReady, strlen(resultReady));
  }

  updateDeviceInfoValue();
  publishOtaState("info", "ready");

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMaxPreferred(0x12);

  bleInitialized = true;
  setBleAdvertisingActive(false);
  setBleAdvertisingDesired(false);
  setBleAuthenticated(false);
  setBlePostAuthSyncPending(false);
  cacheConnectionCount(0);
  lastNotifyTime = 0;
  resumeAdvertisingPending = false;
  resumeAdvertisingAtMs = 0;
  clearBleOtaSessionState(true);
  setOtaLastState("info", "ready");
  Serial.println(
      "[BLE] Serwer gotowy. Advertising uruchamiany z menu Bluetooth.");
}

void BleManager::start() {
  if (!bleInitialized) {
    return;
  }
  setBleAdvertisingDesired(true);
}

void BleManager::stop() {
  if (!bleInitialized) {
    return;
  }
  setBleAdvertisingDesired(false);
}

void BleManager::notifyStatus() {
  if (!pCharStatus) {
    return;
  }

  refreshConnectionState();
  if (deviceConnected && isBleAuthenticated()) {
    unsigned long nowMs = millis();
    if (lastNotifyTime != 0 && (nowMs - lastNotifyTime < NOTIFY_INTERVAL_MS)) {
      return;
    }

    String json = buildStatusJsonString();
    pCharStatus->setValue((uint8_t *)json.c_str(), json.length());
    pCharStatus->notify();
    lastNotifyTime = nowMs;
  }
}

void BleManager::update() {
  // --- Deferred BLE command processing ---
  // Komendy odlozone z callbacku BLE sa teraz wykonywane na głównym wątku
  // z pelnym stosem, co zapobiega watchdog reset / stack overflow.
  if (hasPendingBleCommand) {
    PendingBleCommand cmd;
    portENTER_CRITICAL(&bleStateMux);
    cmd = pendingBleCommand;
    pendingBleCommand.type = PendingBleCommand::Type::NONE;
    hasPendingBleCommand = false;
    portEXIT_CRITICAL(&bleStateMux);

    switch (cmd.type) {
    case PendingBleCommand::Type::FEED_NOW:
      SystemController::feedNow();
      Serial.println("[BLE] Deferred: feedNow executed.");
      break;
    case PendingBleCommand::Type::SET_SERVO:
      SystemController::setManualServo(cmd.servoAngle);
      Serial.printf("[BLE] Deferred: setManualServo(%d) executed.\n",
                    cmd.servoAngle);
      break;
    case PendingBleCommand::Type::CLEAR_SERVO:
      SystemController::clearManualServo();
      Serial.println("[BLE] Deferred: clearManualServo executed.");
      break;
    case PendingBleCommand::Type::CLEAR_LOGS:
      LogManager::clearCriticalLogs();
      Serial.println("[BLE] Deferred: clearCriticalLogs executed.");
      break;
    default:
      break;
    }
    notifyStatus();
  }

  if (bleOtaRestartPending &&
      static_cast<long>(millis() - bleOtaRestartAtMs) >= 0) {
    Serial.println("[BLE OTA] Restart po zakonczonej aktualizacji.");
    delay(40);
    ESP.restart();
    return;
  }

  if (bleOtaActive && bleOtaHandle != 0 && bleOtaLastChunkAtMs > 0 &&
      (millis() - bleOtaLastChunkAtMs) >= BLE_OTA_STALL_TIMEOUT_MS) {
    abortBleOtaSession("ota_timeout",
                       "Sesja BLE OTA wygasla przez brak kolejnych pakietow.");
  }

  const bool shouldAdvertise = getBleAdvertisingDesired();
  if (shouldAdvertise != bleAdvertising) {
    if (shouldAdvertise) {
      startAdvertisingNow();
    } else {
      stopAdvertisingNow();
    }
  }

  if (!bleAdvertising) {
    return;
  }

  refreshConnectionState();

  if (isBlePostAuthSyncPending() && deviceConnected && isBleAuthenticated()) {
    setBlePostAuthSyncPending(false);
    notifyStatus();
    publishOtaState("info", bleOtaActive ? "ota_receiving" : "ready");
    publishResult("ack", "connected");
  }

  if (resumeAdvertisingPending && !deviceConnected &&
      static_cast<long>(millis() - resumeAdvertisingAtMs) >= 0) {
    if (pServer) {
      pServer->startAdvertising();
      Serial.println("[BLE] Wznowiono advertising po rozlaczeniu");
    }
    resumeAdvertisingPending = false;
  }

  if (deviceConnected && isBleAuthenticated() &&
      (millis() - lastNotifyTime >= NOTIFY_INTERVAL_MS)) {
    notifyStatus();
  }
}

bool BleManager::isConnected() { return deviceConnected; }

bool BleManager::isAdvertising() { return bleAdvertising; }

uint8_t BleManager::getConnectedClients() { return connectedClientCount; }

const char *BleManager::getDeviceName() { return BLE_DEVICE_NAME; }

uint32_t BleManager::getPasskey() {
  return static_cast<uint32_t>(SECRET_BLE_PASSKEY);
}
