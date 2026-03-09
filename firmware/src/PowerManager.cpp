#include "PowerManager.h"
#include "LogManager.h"
#include "SharedState.h"

BatteryReader *PowerManager::batteryReader = nullptr;
unsigned long PowerManager::lastActivityTime = 0;
PowerMode PowerManager::currentMode = MODE_ACTIVE;

float PowerManager::lastValidVoltage = 3.0f;
float PowerManager::batteryPercent = 0.0f;
bool PowerManager::hasValidBatteryVoltage = false;
bool PowerManager::batteryCriticalLogged = false;

uint8_t PowerManager::cr2032PercentFromVoltage(float vbat) {
  if (isnan(vbat))
    return 0;
  const float V_MIN = 2.8f;
  const float V_MAX = 3.239f; // Kalibracja 100% dla zadanego mapowania S3
  if (vbat <= V_MIN)
    return 0;
  if (vbat >= V_MAX)
    return 100;
  return (uint8_t)((vbat - V_MIN) / (V_MAX - V_MIN) * 100.0f);
}

void PowerManager::init(BatteryReader *reader) {
  batteryReader = reader;
  lastActivityTime = millis();
}

void PowerManager::registerActivity() {
  lastActivityTime = millis();
  if (currentMode != MODE_ACTIVE) {
    setMode(MODE_ACTIVE);
  }
}

void PowerManager::setMode(PowerMode mode) {
  currentMode = mode;
  // Prawdziwa logika wprowadzania w light sleep bedzie realizowana w
  // SystemController, tu trzymamy tylko flagę
}

void PowerManager::update() {
  if (batteryReader == nullptr)
    return;

  // Pobranie ostatniego pomiaru bez aktywnego polecenia blokujacego, jezeli
  // jest gotowy
  if (batteryReader->isMeasurementReady()) {
    float v = batteryReader->getLastMeasurement();
    // CR2025/CR2032 nie powinny przekraczac ok. 3.3V.
    bool validSample = (!isnan(v) && v >= 0.5f && v <= 3.6f);

    if (validSample) {
      lastValidVoltage = v;
      hasValidBatteryVoltage = true;
    } else if (hasValidBatteryVoltage) {
      v = lastValidVoltage;
    }

    if (!isnan(v) && hasValidBatteryVoltage) {
      batteryPercent = (float)cr2032PercentFromVoltage(v);

      // Ochrona przed spamem na logach przykrywana logika watchdoga
      if (batteryPercent <= 10.0f && !batteryCriticalLogged) {
        LogManager::logError(
            "Bateria podtrzymania RTC krytycznie rozladowana (<=10%)!");
        batteryCriticalLogged = true;
      } else if (batteryPercent > 25.0f) {
        batteryCriticalLogged = false;
      }

      SharedState::updateBattery(lastValidVoltage, batteryPercent);
    }
  }
}
