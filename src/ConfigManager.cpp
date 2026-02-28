#include "ConfigManager.h"
#include <Arduino.h>
#include <Preferences.h>


Config ConfigManager::sysConfig;
static Preferences preferences;
static const char *PREF_NAMESPACE = "Akwarium";

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
  sysConfig.version = CONFIG_VERSION;
  sysConfig.magic = CONFIG_MAGIC;
  sysConfig.crc32 = calculateCrc32(sysConfig);
}

void ConfigManager::init() {
  preferences.begin(PREF_NAMESPACE, false);

  size_t configBytes =
      preferences.getBytes("sysConfig", &sysConfig, sizeof(Config));

  if (configBytes == sizeof(Config) && sysConfig.magic == CONFIG_MAGIC &&
      sysConfig.version == CONFIG_VERSION) {
    uint32_t calculatedCrc = calculateCrc32(sysConfig);
    if (calculatedCrc == sysConfig.crc32) {
      Serial.println(
          "[CONFIG] Konfiguracja zaladowana pomyslnie, CRC poprawne.");
      return;
    } else {
      Serial.println("[CONFIG] BLAD CRC! Konfiguracja uszkodzona. Ladowanie "
                     "wartosci domyslnych.");
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
      sysConfig.dayStartHour = constrain(legacy.dayStartHour, 0, 24);
      sysConfig.dayStartMinute = constrain(legacy.dayStartMinute, 0, 59);
      sysConfig.dayEndHour = constrain(legacy.dayEndHour, 0, 24);
      sysConfig.dayEndMinute = constrain(legacy.dayEndMinute, 0, 59);

      sysConfig.aerationHourOn = constrain(legacy.aerationHourOn, 0, 23);
      sysConfig.aerationMinuteOn = constrain(legacy.aerationMinuteOn, 0, 59);
      sysConfig.aerationHourOff = constrain(legacy.aerationHourOff, 0, 23);
      sysConfig.aerationMinuteOff = constrain(legacy.aerationMinuteOff, 0, 59);

      sysConfig.filterHourOn = constrain(legacy.filterHourOn, 0, 23);
      sysConfig.filterMinuteOn = constrain(legacy.filterMinuteOn, 0, 59);
      sysConfig.filterHourOff = constrain(legacy.filterHourOff, 0, 23);
      sysConfig.filterMinuteOff = constrain(legacy.filterMinuteOff, 0, 59);

      sysConfig.servoPreOffMins = legacy.servoPreOffMins;
      sysConfig.targetTemp = constrain(legacy.targetTemp, 15.0f, 35.0f);
      sysConfig.tempHysteresis = constrain(legacy.tempHysteresis, 0.1f, 5.0f);
      sysConfig.servoDayAngle = constrain(legacy.servoDayAngle, 0, 90);
      sysConfig.servoNightAngle = constrain(legacy.servoNightAngle, 0, 90);
      sysConfig.servoAlarmAngle = constrain(legacy.servoAlarmAngle, 0, 90);
      sysConfig.feedMode = constrain(legacy.feedMode, 0, 3);
      sysConfig.feedHour = constrain(legacy.feedHour, 0, 23);
      sysConfig.feedMinute = constrain(legacy.feedMinute, 0, 59);
      sysConfig.lastFeedEpoch = legacy.lastFeedEpoch;
      sysConfig.alwaysScreenOn = legacy.alwaysScreenOn;
      save(); // Przeliczenie i zapis wymuszonego CRC
      return;
    } else {
      Serial.println("[CONFIG] Brak lub niepoprawna sygnatura MAGIC. Ladowanie "
                     "default() z CRC.");
    }
  }

  loadDefaultConfig();
  save();
}

void ConfigManager::save() {
  sysConfig.version = CONFIG_VERSION;
  sysConfig.magic = CONFIG_MAGIC;
  sysConfig.crc32 = calculateCrc32(sysConfig);
  preferences.putBytes("sysConfig", &sysConfig, sizeof(Config));
  Serial.println("[CONFIG] Zapisano nowa konfiguracje (zabezpieczona CRC32)");
}

void ConfigManager::resetToDefault() {
  loadDefaultConfig();
  save();
}

Config &ConfigManager::getConfig() { return sysConfig; }
