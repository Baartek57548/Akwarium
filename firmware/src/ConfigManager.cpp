#include "ConfigManager.h"
#include "ConfigValidation.h"
#include "SecretConfig.h"

#include <Arduino.h>
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <string.h>

Config ConfigManager::sysConfig;
static Preferences preferences;
static const char *PREF_NAMESPACE = "Akwarium";
static SemaphoreHandle_t configMutex = nullptr;
static constexpr int PERSISTED_CRITICAL_LOG_SLOTS = 20;

static bool lockConfig(TickType_t timeoutTicks = portMAX_DELAY) {
  if (configMutex == nullptr) {
    return false;
  }
  return xSemaphoreTake(configMutex, timeoutTicks) == pdTRUE;
}

static void unlockConfig() {
  if (configMutex != nullptr) {
    xSemaphoreGive(configMutex);
  }
}

static void copyConfigString(char *dest, size_t destSize, const char *src) {
  if (dest == nullptr || destSize == 0) {
    return;
  }

  snprintf(dest, destSize, "%s", src != nullptr ? src : "");
}

static void freeLowPriorityNvsKeys() {
  preferences.remove("critCount");
  preferences.remove("critHead");
  for (int i = 0; i < PERSISTED_CRITICAL_LOG_SLOTS; i++) {
    char key[16];
    snprintf(key, sizeof(key), "critLog%d", i);
    preferences.remove(key);
  }
}

struct ConfigV1Legacy {
  uint8_t dayStartHour;
  uint8_t dayStartMinute;
  uint8_t dayEndHour;
  uint8_t dayEndMinute;
  uint8_t aerationHourOn;
  uint8_t aerationMinuteOn;
  uint8_t aerationHourOff;
  uint8_t aerationMinuteOff;
  uint8_t filterHourOn;
  uint8_t filterMinuteOn;
  uint8_t filterHourOff;
  uint8_t filterMinuteOff;
  uint8_t servoPreOffMins;
  float targetTemp;
  float tempHysteresis;
  int servoDayAngle;
  int servoNightAngle;
  int servoAlarmAngle;
  uint8_t feedMode;
  uint8_t feedHour;
  uint8_t feedMinute;
  uint32_t lastFeedEpoch;
  bool alwaysScreenOn;
  uint16_t version;
  uint32_t magic;
};

struct ConfigV6Legacy {
  uint8_t lightMode;
  uint8_t dayStartHour;
  uint8_t dayStartMinute;
  uint8_t dayEndHour;
  uint8_t dayEndMinute;
  uint8_t aerationMode;
  uint8_t aerationHourOn;
  uint8_t aerationMinuteOn;
  uint8_t aerationHourOff;
  uint8_t aerationMinuteOff;
  uint8_t filterMode;
  uint8_t filterHourOn;
  uint8_t filterMinuteOn;
  uint8_t filterHourOff;
  uint8_t filterMinuteOff;
  uint8_t servoPreOffMins;
  uint8_t heaterMode;
  float targetTemp;
  float tempHysteresis;
  int servoDayAngle;
  int servoNightAngle;
  int servoAlarmAngle;
  uint8_t feedMode;
  uint8_t feedHour;
  uint8_t feedMinute;
  uint32_t lastFeedEpoch;
  bool alwaysScreenOn;
  uint16_t version;
  uint32_t magic;
  uint32_t crc32;
};

static uint32_t calculateCrc32Bytes(const void *buffer, size_t length) {
  uint32_t crc = 0xFFFFFFFF;
  const uint8_t *data = reinterpret_cast<const uint8_t *>(buffer);

  for (size_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 1) {
        crc = (crc >> 1) ^ 0xEDB88320;
      } else {
        crc >>= 1;
      }
    }
  }

  return ~crc;
}

uint32_t ConfigManager::calculateCrc32(const Config &cfg) {
  return calculateCrc32Bytes(&cfg, sizeof(Config) - sizeof(uint32_t));
}

void ConfigManager::loadDefaultConfig() {
  sysConfig.lightMode = static_cast<uint8_t>(ScheduleMode::Schedule);
  sysConfig.dayStartHour = 10;
  sysConfig.dayStartMinute = 0;
  sysConfig.dayEndHour = 21;
  sysConfig.dayEndMinute = 30;
  sysConfig.aerationMode = static_cast<uint8_t>(ScheduleMode::Schedule);
  sysConfig.aerationHourOn = 10;
  sysConfig.aerationMinuteOn = 0;
  sysConfig.aerationHourOff = 19;
  sysConfig.aerationMinuteOff = 0;
  sysConfig.filterMode = static_cast<uint8_t>(ScheduleMode::Schedule);
  sysConfig.filterHourOn = 10;
  sysConfig.filterMinuteOn = 30;
  sysConfig.filterHourOff = 20;
  sysConfig.filterMinuteOff = 30;
  sysConfig.servoPreOffMins = 30;
  sysConfig.heaterMode = static_cast<uint8_t>(HeaterMode::Threshold);
  sysConfig.targetTemp = 25.0f;
  sysConfig.tempHysteresis = 0.5f;
  sysConfig.servoDayAngle = SERVO_OPEN_ANGLE;
  sysConfig.servoNightAngle = SERVO_CLOSED_ANGLE;
  sysConfig.servoAlarmAngle = SERVO_PREOFF_ANGLE;
  sysConfig.feedMode = 1;
  sysConfig.feedHour = 18;
  sysConfig.feedMinute = 0;
  sysConfig.lastFeedEpoch = 0;
  sysConfig.alwaysScreenOn = false;
  copyConfigString(sysConfig.staSsid, sizeof(sysConfig.staSsid), SECRET_SSID);
  copyConfigString(sysConfig.staPassword, sizeof(sysConfig.staPassword),
                   SECRET_PASS);
  copyConfigString(sysConfig.apSsid, sizeof(sysConfig.apSsid), AP_SSID);
  copyConfigString(sysConfig.apPassword, sizeof(sysConfig.apPassword),
                   AP_PASSWORD);
  ConfigValidation::sanitizeConfig(sysConfig);
  sysConfig.version = CONFIG_VERSION;
  sysConfig.magic = CONFIG_MAGIC;
  sysConfig.crc32 = calculateCrc32(sysConfig);
}

void ConfigManager::init() {
  if (configMutex == nullptr) {
    configMutex = xSemaphoreCreateMutex();
    if (configMutex == nullptr) {
      Serial.println("[CONFIG] BLAD: nie mozna utworzyc mutexa konfiguracji.");
      loadDefaultConfig();
      return;
    }
  }

  if (!preferences.begin(PREF_NAMESPACE, false)) {
    Serial.println(
        "[CONFIG] BLAD: preferences.begin nie powiodlo sie, ladowanie default.");
    loadDefaultConfig();
    return;
  }

  if (!lockConfig(portMAX_DELAY)) {
    Serial.println(
        "[CONFIG] BLAD: timeout lock podczas init(), ladowanie default.");
    loadDefaultConfig();
    save();
    return;
  }

  bool shouldSave = false;
  const size_t configBytes =
      preferences.getBytes("sysConfig", &sysConfig, sizeof(Config));

  if (configBytes == sizeof(Config) && sysConfig.magic == CONFIG_MAGIC &&
      sysConfig.version == CONFIG_VERSION) {
    const uint32_t calculatedCrc = calculateCrc32(sysConfig);
    if (calculatedCrc == sysConfig.crc32) {
      Config sanitized = sysConfig;
      ConfigValidation::sanitizeConfig(sanitized);
      if (memcmp(&sanitized, &sysConfig, sizeof(Config)) != 0) {
        sysConfig = sanitized;
        shouldSave = true;
      }
      Serial.println("[CONFIG] Konfiguracja zaladowana pomyslnie, CRC poprawne.");
      unlockConfig();
      if (shouldSave) {
        save();
      }
      return;
    }

    Serial.println("[CONFIG] BLAD CRC! Konfiguracja uszkodzona. Ladowanie "
                   "wartosci domyslnych.");
    loadDefaultConfig();
    shouldSave = true;
  } else {
    ConfigV6Legacy legacyV6{};
    const size_t legacyV6Bytes =
        preferences.getBytes("sysConfig", &legacyV6, sizeof(ConfigV6Legacy));

    if (legacyV6Bytes == sizeof(ConfigV6Legacy) &&
        legacyV6.magic == CONFIG_MAGIC && legacyV6.version == 6 &&
        calculateCrc32Bytes(&legacyV6,
                            sizeof(ConfigV6Legacy) - sizeof(uint32_t)) ==
            legacyV6.crc32) {
      Serial.println("[CONFIG] Migracja z v6 do v7 (dodano ustawienia WiFi).");
      loadDefaultConfig();
      sysConfig.lightMode = legacyV6.lightMode;
      sysConfig.dayStartHour = legacyV6.dayStartHour;
      sysConfig.dayStartMinute = legacyV6.dayStartMinute;
      sysConfig.dayEndHour = legacyV6.dayEndHour;
      sysConfig.dayEndMinute = legacyV6.dayEndMinute;
      sysConfig.aerationMode = legacyV6.aerationMode;
      sysConfig.aerationHourOn = legacyV6.aerationHourOn;
      sysConfig.aerationMinuteOn = legacyV6.aerationMinuteOn;
      sysConfig.aerationHourOff = legacyV6.aerationHourOff;
      sysConfig.aerationMinuteOff = legacyV6.aerationMinuteOff;
      sysConfig.filterMode = legacyV6.filterMode;
      sysConfig.filterHourOn = legacyV6.filterHourOn;
      sysConfig.filterMinuteOn = legacyV6.filterMinuteOn;
      sysConfig.filterHourOff = legacyV6.filterHourOff;
      sysConfig.filterMinuteOff = legacyV6.filterMinuteOff;
      sysConfig.servoPreOffMins = legacyV6.servoPreOffMins;
      sysConfig.heaterMode = legacyV6.heaterMode;
      sysConfig.targetTemp = legacyV6.targetTemp;
      sysConfig.tempHysteresis = legacyV6.tempHysteresis;
      sysConfig.servoDayAngle = legacyV6.servoDayAngle;
      sysConfig.servoNightAngle = legacyV6.servoNightAngle;
      sysConfig.servoAlarmAngle = legacyV6.servoAlarmAngle;
      sysConfig.feedMode = legacyV6.feedMode;
      sysConfig.feedHour = legacyV6.feedHour;
      sysConfig.feedMinute = legacyV6.feedMinute;
      sysConfig.lastFeedEpoch = legacyV6.lastFeedEpoch;
      sysConfig.alwaysScreenOn = legacyV6.alwaysScreenOn;
      ConfigValidation::sanitizeConfig(sysConfig);
      shouldSave = true;
    } else {
      ConfigV1Legacy legacy{};
      const size_t legacyBytes =
          preferences.getBytes("sysConfig", &legacy, sizeof(ConfigV1Legacy));

      if (legacyBytes == sizeof(ConfigV1Legacy) &&
          legacy.magic == CONFIG_MAGIC) {
        Serial.println(
            "[CONFIG] Migracja ze starej wersji (bez CRC) do nowej struktury");
        loadDefaultConfig();
        sysConfig.dayStartHour = legacy.dayStartHour;
        sysConfig.dayStartMinute = legacy.dayStartMinute;
        sysConfig.dayEndHour = legacy.dayEndHour;
        sysConfig.dayEndMinute = legacy.dayEndMinute;
        sysConfig.lightMode = (legacy.dayStartHour == 24)
                                  ? static_cast<uint8_t>(ScheduleMode::AlwaysOn)
                                  : (legacy.dayEndHour == 24)
                                        ? static_cast<uint8_t>(ScheduleMode::AlwaysOff)
                                        : static_cast<uint8_t>(ScheduleMode::Schedule);

        sysConfig.aerationHourOn = legacy.aerationHourOn;
        sysConfig.aerationMinuteOn = legacy.aerationMinuteOn;
        sysConfig.aerationHourOff = legacy.aerationHourOff;
        sysConfig.aerationMinuteOff = legacy.aerationMinuteOff;
        sysConfig.aerationMode = static_cast<uint8_t>(ScheduleMode::Schedule);

        sysConfig.filterHourOn = legacy.filterHourOn;
        sysConfig.filterMinuteOn = legacy.filterMinuteOn;
        sysConfig.filterHourOff = legacy.filterHourOff;
        sysConfig.filterMinuteOff = legacy.filterMinuteOff;
        sysConfig.filterMode = static_cast<uint8_t>(ScheduleMode::Schedule);

        sysConfig.servoPreOffMins = legacy.servoPreOffMins;
        sysConfig.heaterMode = legacy.targetTemp <= 0.0f
                                   ? static_cast<uint8_t>(HeaterMode::Off)
                                   : static_cast<uint8_t>(HeaterMode::Threshold);
        sysConfig.targetTemp = legacy.targetTemp;
        sysConfig.tempHysteresis = legacy.tempHysteresis;
        sysConfig.servoDayAngle = constrain(legacy.servoDayAngle, 0, 90);
        sysConfig.servoNightAngle = constrain(legacy.servoNightAngle, 0, 90);
        sysConfig.servoAlarmAngle = constrain(legacy.servoAlarmAngle, 0, 90);
        sysConfig.feedMode = legacy.feedMode;
        sysConfig.feedHour = legacy.feedHour;
        sysConfig.feedMinute = legacy.feedMinute;
        sysConfig.lastFeedEpoch = legacy.lastFeedEpoch;
        sysConfig.alwaysScreenOn = legacy.alwaysScreenOn;
        ConfigValidation::sanitizeConfig(sysConfig);
        shouldSave = true;
      } else {
        Serial.println("[CONFIG] Brak lub niepoprawna sygnatura MAGIC. Ladowanie "
                       "default() z CRC.");
        loadDefaultConfig();
        shouldSave = true;
      }
    }
  }

  unlockConfig();

  if (shouldSave) {
    save();
  }
}

void ConfigManager::save() {
  if (!lockConfig(portMAX_DELAY)) {
    Serial.println("[CONFIG] BLAD: timeout lock podczas save().");
    return;
  }

  const Config cfg = sysConfig;
  unlockConfig();

  if (!updateAndSave(cfg)) {
    Serial.println("[CONFIG] BLAD: nie udalo sie zapisac konfiguracji w save().");
  }
}

bool ConfigManager::updateAndSave(const Config &cfg) {
  const ConfigSaveResult result = updateAndSaveDetailed(cfg);
  return result.ok;
}

ConfigSaveResult ConfigManager::updateAndSaveDetailed(const Config &cfg) {
  ConfigSaveResult result;

  if (!lockConfig(portMAX_DELAY)) {
    Serial.println("[CONFIG] BLAD: timeout lock podczas updateAndSave().");
    result.status = ConfigSaveStatus::LockTimeout;
    return result;
  }

  sysConfig = cfg;
  const Config beforeSanitize = sysConfig;
  ConfigValidation::sanitizeConfig(sysConfig);
  result.sanitizedChanged =
      memcmp(&beforeSanitize, &sysConfig, sizeof(Config)) != 0;
  sysConfig.version = CONFIG_VERSION;
  sysConfig.magic = CONFIG_MAGIC;
  sysConfig.crc32 = calculateCrc32(sysConfig);
  result.appliedConfig = sysConfig;
  result.bytesWritten =
      preferences.putBytes("sysConfig", &sysConfig, sizeof(Config));

  if (result.bytesWritten != sizeof(Config)) {
    Serial.println(
        "[CONFIG] Ostrzezenie: brak miejsca w NVS, czyszcze stare logi krytyczne i ponawiam zapis konfiguracji.");
    freeLowPriorityNvsKeys();
    preferences.remove("sysConfig");
    result.bytesWritten =
        preferences.putBytes("sysConfig", &sysConfig, sizeof(Config));
  }

  Config readBack = {};
  result.bytesReadBack =
      preferences.getBytes("sysConfig", &readBack, sizeof(Config));
  unlockConfig();

  const bool writeReportedOk = result.bytesWritten == sizeof(Config);
  const bool verifyReadOk = result.bytesReadBack == sizeof(Config);
  const bool verifyCrcOk =
      verifyReadOk && readBack.magic == CONFIG_MAGIC &&
      readBack.version == CONFIG_VERSION &&
      calculateCrc32(readBack) == readBack.crc32;
  const bool verifyMatch =
      verifyCrcOk &&
      memcmp(&readBack, &result.appliedConfig, sizeof(Config)) == 0;

  if (verifyMatch) {
    result.ok = true;
    result.status = writeReportedOk
                        ? ConfigSaveStatus::Ok
                        : ConfigSaveStatus::OkVerifiedAfterWriteMismatch;
    Serial.printf(
        "[CONFIG] Zapis OK (written=%u, read=%u, sanitized=%d, status=%u).\n",
        static_cast<unsigned>(result.bytesWritten),
        static_cast<unsigned>(result.bytesReadBack),
        result.sanitizedChanged ? 1 : 0,
        static_cast<unsigned>(result.status));
    return result;
  }

  if (!writeReportedOk) {
    Serial.println("[CONFIG] BLAD: niepelny zapis konfiguracji.");
    result.status = ConfigSaveStatus::WriteFailed;
    return result;
  }

  if (!verifyReadOk) {
    Serial.println(
        "[CONFIG] BLAD: nie udalo sie odczytac konfiguracji do weryfikacji.");
    result.status = ConfigSaveStatus::VerifyReadFailed;
    return result;
  }

  Serial.println(
      "[CONFIG] BLAD: odczytana konfiguracja po zapisie nie zgadza sie z oczekiwana wartoscia.");
  result.status = ConfigSaveStatus::VerifyMismatch;
  return result;
}

Config ConfigManager::getCopy() {
  Config snapshot = {};
  if (!lockConfig(portMAX_DELAY)) {
    Serial.println("[CONFIG] BLAD: timeout lock podczas getCopy().");
    return snapshot;
  }

  snapshot = sysConfig;
  unlockConfig();
  return snapshot;
}

void ConfigManager::saveConfig(const Config &cfg) {
  if (!updateAndSave(cfg)) {
    Serial.println("[CONFIG] BLAD: zapis saveConfig() nieudany.");
  }
}

Config ConfigManager::getConfigSnapshot() { return getCopy(); }

void ConfigManager::resetToDefault() {
  if (!lockConfig(portMAX_DELAY)) {
    Serial.println("[CONFIG] BLAD: timeout lock podczas resetToDefault().");
    return;
  }

  loadDefaultConfig();
  const Config defaults = sysConfig;
  unlockConfig();
  updateAndSave(defaults);
}
