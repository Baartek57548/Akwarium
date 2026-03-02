#include "ConfigManager.h"
#include <Arduino.h>
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

Config ConfigManager::sysConfig;
static Preferences preferences;
static const char *PREF_NAMESPACE = "Akwarium";
static SemaphoreHandle_t configMutex = nullptr;

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

struct ConfigV5Legacy {
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
  uint32_t crc32;
};

static void copyLegacyFields(Config &dst, const ConfigV1Legacy &src) {
  dst.dayStartHour = constrain(src.dayStartHour, 0, 24);
  dst.dayStartMinute = constrain(src.dayStartMinute, 0, 59);
  dst.dayEndHour = constrain(src.dayEndHour, 0, 24);
  dst.dayEndMinute = constrain(src.dayEndMinute, 0, 59);

  dst.aerationHourOn = constrain(src.aerationHourOn, 0, 23);
  dst.aerationMinuteOn = constrain(src.aerationMinuteOn, 0, 59);
  dst.aerationHourOff = constrain(src.aerationHourOff, 0, 23);
  dst.aerationMinuteOff = constrain(src.aerationMinuteOff, 0, 59);

  dst.filterHourOn = constrain(src.filterHourOn, 0, 23);
  dst.filterMinuteOn = constrain(src.filterMinuteOn, 0, 59);
  dst.filterHourOff = constrain(src.filterHourOff, 0, 23);
  dst.filterMinuteOff = constrain(src.filterMinuteOff, 0, 59);

  dst.servoPreOffMins = src.servoPreOffMins;
  dst.targetTemp = constrain(src.targetTemp, 15.0f, 35.0f);
  dst.tempHysteresis = constrain(src.tempHysteresis, 0.1f, 5.0f);
  dst.servoDayAngle = constrain(src.servoDayAngle, 0, 90);
  dst.servoNightAngle = constrain(src.servoNightAngle, 0, 90);
  dst.servoAlarmAngle = constrain(src.servoAlarmAngle, 0, 90);
  dst.feedMode = constrain(src.feedMode, 0, 3);
  dst.feedHour = constrain(src.feedHour, 0, 23);
  dst.feedMinute = constrain(src.feedMinute, 0, 59);
  dst.lastFeedEpoch = src.lastFeedEpoch;
  dst.alwaysScreenOn = src.alwaysScreenOn;
}

static void copyLegacyFields(Config &dst, const ConfigV5Legacy &src) {
  dst.dayStartHour = constrain(src.dayStartHour, 0, 24);
  dst.dayStartMinute = constrain(src.dayStartMinute, 0, 59);
  dst.dayEndHour = constrain(src.dayEndHour, 0, 24);
  dst.dayEndMinute = constrain(src.dayEndMinute, 0, 59);

  dst.aerationHourOn = constrain(src.aerationHourOn, 0, 23);
  dst.aerationMinuteOn = constrain(src.aerationMinuteOn, 0, 59);
  dst.aerationHourOff = constrain(src.aerationHourOff, 0, 23);
  dst.aerationMinuteOff = constrain(src.aerationMinuteOff, 0, 59);

  dst.filterHourOn = constrain(src.filterHourOn, 0, 23);
  dst.filterMinuteOn = constrain(src.filterMinuteOn, 0, 59);
  dst.filterHourOff = constrain(src.filterHourOff, 0, 23);
  dst.filterMinuteOff = constrain(src.filterMinuteOff, 0, 59);

  dst.servoPreOffMins = src.servoPreOffMins;
  dst.targetTemp = constrain(src.targetTemp, 15.0f, 35.0f);
  dst.tempHysteresis = constrain(src.tempHysteresis, 0.1f, 5.0f);
  dst.servoDayAngle = constrain(src.servoDayAngle, 0, 90);
  dst.servoNightAngle = constrain(src.servoNightAngle, 0, 90);
  dst.servoAlarmAngle = constrain(src.servoAlarmAngle, 0, 90);
  dst.feedMode = constrain(src.feedMode, 0, 3);
  dst.feedHour = constrain(src.feedHour, 0, 23);
  dst.feedMinute = constrain(src.feedMinute, 0, 59);
  dst.lastFeedEpoch = src.lastFeedEpoch;
  dst.alwaysScreenOn = src.alwaysScreenOn;
}

// Extremely simple CRC32 tableless implementation (sufficient for small config)
uint32_t ConfigManager::calculateCrc32(const Config &cfg) {
  uint32_t crc = 0xFFFFFFFF;
  const uint8_t *data = reinterpret_cast<const uint8_t *>(&cfg);
  // Calc CRC minus the last 4 bytes (the crc32 field itself)
  size_t length = sizeof(Config) - sizeof(uint32_t);

  for (size_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 1)
        crc = (crc >> 1) ^ 0xEDB88320;
      else
        crc >>= 1;
    }
  }
  return ~crc;
}

void ConfigManager::loadDefaultConfig() {
  sysConfig.dayStartHour = 10;
  sysConfig.dayStartMinute = 0;
  sysConfig.dayEndHour = 21;
  sysConfig.dayEndMinute = 30;
  sysConfig.aerationHourOn = 10;
  sysConfig.aerationMinuteOn = 0;
  sysConfig.aerationHourOff = 21;
  sysConfig.aerationMinuteOff = 0;
  sysConfig.filterHourOn = 10;
  sysConfig.filterMinuteOn = 30;
  sysConfig.filterHourOff = 20;
  sysConfig.filterMinuteOff = 30;
  sysConfig.servoPreOffMins = 30;
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
  sysConfig.heaterEnabled = true;
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

  preferences.begin(PREF_NAMESPACE, false);

  if (!lockConfig(portMAX_DELAY)) {
    Serial.println("[CONFIG] BLAD: timeout lock podczas init(), ladowanie default.");
    loadDefaultConfig();
    save();
    return;
  }

  bool shouldSave = false;
  size_t configBytes = preferences.getBytes("sysConfig", &sysConfig, sizeof(Config));

  if (configBytes == sizeof(Config) && sysConfig.magic == CONFIG_MAGIC &&
      sysConfig.version == CONFIG_VERSION) {
    uint32_t calculatedCrc = calculateCrc32(sysConfig);
    if (calculatedCrc == sysConfig.crc32) {
      Serial.println("[CONFIG] Konfiguracja zaladowana pomyslnie, CRC poprawne.");
      unlockConfig();
      return;
    }

    Serial.println("[CONFIG] BLAD CRC! Konfiguracja uszkodzona. Ladowanie default.");
    loadDefaultConfig();
    shouldSave = true;
  } else {
    ConfigV5Legacy legacyV5{};
    size_t legacyV5Bytes =
        preferences.getBytes("sysConfig", &legacyV5, sizeof(ConfigV5Legacy));

    if (legacyV5Bytes == sizeof(ConfigV5Legacy) &&
        legacyV5.magic == CONFIG_MAGIC && legacyV5.version == 5) {
      Serial.println("[CONFIG] Migracja z v5 do v6 (heaterEnabled=true).");
      loadDefaultConfig();
      copyLegacyFields(sysConfig, legacyV5);
      sysConfig.heaterEnabled = true;
      shouldSave = true;
    } else {
      ConfigV1Legacy legacyV1{};
      size_t legacyV1Bytes =
          preferences.getBytes("sysConfig", &legacyV1, sizeof(ConfigV1Legacy));

      if (legacyV1Bytes == sizeof(ConfigV1Legacy) &&
          legacyV1.magic == CONFIG_MAGIC) {
        Serial.println("[CONFIG] Migracja ze starej wersji (bez CRC) do nowej struktury.");
        loadDefaultConfig();
        copyLegacyFields(sysConfig, legacyV1);
        sysConfig.heaterEnabled = true;
        shouldSave = true;
      } else {
        Serial.println("[CONFIG] Brak lub niepoprawna sygnatura MAGIC. Ladowanie default().");
        loadDefaultConfig();
        shouldSave = true;
      }
    }
  }

  unlockConfig();

  if (shouldSave) {
    save(); // Recalculate and write current format.
  }
}

void ConfigManager::save() {
  if (!lockConfig(portMAX_DELAY)) {
    Serial.println("[CONFIG] BLAD: timeout lock podczas save().");
    return;
  }

  Config cfg = sysConfig;
  unlockConfig();

  if (!updateAndSave(cfg)) {
    Serial.println("[CONFIG] BLAD: nie udalo sie zapisac konfiguracji w save().");
  }
}

bool ConfigManager::updateAndSave(const Config &cfg) {
  if (!lockConfig(portMAX_DELAY)) {
    Serial.println("[CONFIG] BLAD: timeout lock podczas updateAndSave().");
    return false;
  }

  sysConfig = cfg;
  sysConfig.version = CONFIG_VERSION;
  sysConfig.magic = CONFIG_MAGIC;
  sysConfig.crc32 = calculateCrc32(sysConfig);
  size_t written = preferences.putBytes("sysConfig", &sysConfig, sizeof(Config));
  unlockConfig();

  if (written != sizeof(Config)) {
    Serial.println("[CONFIG] BLAD: niepelny zapis konfiguracji.");
    return false;
  }

  Serial.println("[CONFIG] Zapisano konfiguracje przez updateAndSave().");
  return true;
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
  Config defaults = sysConfig;
  unlockConfig();
  updateAndSave(defaults);
}
