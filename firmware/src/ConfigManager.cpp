#include "ConfigManager.h"
#include "ConfigValidation.h"
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
  sysConfig.lightMode = static_cast<uint8_t>(ScheduleMode::Schedule);
  sysConfig.dayStartHour = 10;
  sysConfig.dayStartMinute = 0;
  sysConfig.dayEndHour = 21;
  sysConfig.dayEndMinute = 30;
  sysConfig.aerationMode = static_cast<uint8_t>(ScheduleMode::Schedule);
  sysConfig.aerationHourOn = 10;
  sysConfig.aerationMinuteOn = 0;
  sysConfig.aerationHourOff = 21;
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

  preferences.begin(PREF_NAMESPACE, false);

  if (!lockConfig(portMAX_DELAY)) {
    Serial.println("[CONFIG] BLAD: timeout lock podczas init(), ladowanie default.");
    loadDefaultConfig();
    save();
    return;
  }

  bool shouldSave = false;
  size_t configBytes =
      preferences.getBytes("sysConfig", &sysConfig, sizeof(Config));

  if (configBytes == sizeof(Config) && sysConfig.magic == CONFIG_MAGIC &&
      sysConfig.version == CONFIG_VERSION) {
    uint32_t calculatedCrc = calculateCrc32(sysConfig);
    if (calculatedCrc == sysConfig.crc32) {
      Config sanitized = sysConfig;
      ConfigValidation::sanitizeConfig(sanitized);
      if (memcmp(&sanitized, &sysConfig, sizeof(Config)) != 0) {
        sysConfig = sanitized;
        shouldSave = true;
      }
      Serial.println(
          "[CONFIG] Konfiguracja zaladowana pomyslnie, CRC poprawne.");
      unlockConfig();
      if (shouldSave) {
        save();
      }
      return;
    } else {
      Serial.println("[CONFIG] BLAD CRC! Konfiguracja uszkodzona. Ladowanie "
                     "wartosci domyslnych.");
      loadDefaultConfig();
      shouldSave = true;
    }
  } else {
    // Spróbuj odzyskać z legacy (bez sprawdzania CRC bo nie istnialo)
    ConfigV1Legacy legacy{};
    size_t legacyBytes =
        preferences.getBytes("sysConfig", &legacy, sizeof(ConfigV1Legacy));

    if (legacyBytes == sizeof(ConfigV1Legacy) && legacy.magic == CONFIG_MAGIC) {
      Serial.println(
          "[CONFIG] Migracja ze starej wersji (bez CRC) do nowej struktury");
      loadDefaultConfig(); // Inicjacja nowych pol
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

  unlockConfig();

  if (shouldSave) {
    save(); // Przeliczenie i zapis CRC w bezpiecznym locku
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
  ConfigValidation::sanitizeConfig(sysConfig);
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
