#include "SharedState.h"

SemaphoreHandle_t SharedState::mutex = NULL;
SharedStateData SharedState::state = {};

void SharedState::init() {
  mutex = xSemaphoreCreateMutex();

  // Ustawienie początkowych bezpiecznych danych NAN itp.
  state.temperature = NAN;
  state.minTemp = NAN;
  state.maxTemp = NAN;
  state.batteryVoltage = 0.0f;
  state.batteryPercent = 0.0f;
  state.isHeaterOn = false;
  state.isFilterOn = false;
  state.isLightOn = false;
  state.isDay = true;
  state.aerationPercent = 0;
  state.hour = 0;
  state.minute = 0;
  state.second = 0;
  state.day = 1;
  state.month = 1;
  state.year = 2025;
}

SharedStateData SharedState::getSnapshot() {
  SharedStateData snapshot = {};
  if (mutex != NULL && xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    snapshot =
        state; // Gwarantowana niepodzielnosc (Atomic copy na calej struct)
    xSemaphoreGive(mutex);
  } else {
    // Jesli Mutex bedzie hard-locked (np zly portMUX), fallback na stan ostatni
    // ale potencjalnie narazony na uszkodzenia w czasie kopiowania (chociaz dla
    // renderingu rzadko mialoby to problem)
    snapshot = state;
  }
  return snapshot;
}

void SharedState::updateTemperature(float current, float min, uint32_t minEp,
                                    float max) {
  if (mutex != NULL && xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    state.temperature = current;
    state.minTemp = min;
    state.minTempEpoch = minEp;
    state.maxTemp = max;
    xSemaphoreGive(mutex);
  }
}

void SharedState::updateBattery(float volt, float pct) {
  if (mutex != NULL && xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    state.batteryVoltage = volt;
    state.batteryPercent = pct;
    xSemaphoreGive(mutex);
  }
}

void SharedState::updateRelays(bool heater, bool filter, bool light,
                               bool dayMode) {
  if (mutex != NULL && xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    state.isHeaterOn = heater;
    state.isFilterOn = filter;
    state.isLightOn = light;
    state.isDay = dayMode;
    xSemaphoreGive(mutex);
  }
}

void SharedState::updateTime(int h, int m, int s, int d, int mo, int y) {
  if (mutex != NULL && xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    state.hour = h;
    state.minute = m;
    state.second = s;
    state.day = d;
    state.month = mo;
    state.year = y;
    xSemaphoreGive(mutex);
  }
}

void SharedState::updateAeration(uint8_t pct) {
  if (mutex != NULL && xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    state.aerationPercent = pct;
    xSemaphoreGive(mutex);
  }
}
