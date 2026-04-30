#include "SharedState.h"

#include <cmath>
#include <cstring>

SemaphoreHandle_t SharedState::mutex = NULL;
SharedStateData SharedState::state = {};
TemperatureHistoryEntry SharedState::temperatureHistory[TEMP_HISTORY_SIZE] = {};
uint16_t SharedState::temperatureHistoryCount = 0;
uint16_t SharedState::temperatureHistoryNextIndex = 0;

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
  temperatureHistoryCount = 0;
  temperatureHistoryNextIndex = 0;
  for (uint16_t i = 0; i < TEMP_HISTORY_SIZE; ++i) {
    temperatureHistory[i].value = NAN;
    temperatureHistory[i].epoch = 0;
  }
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
                                    float max, uint32_t currentEpoch) {
  if (mutex != NULL && xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    state.temperature = current;
    state.minTemp = min;
    state.minTempEpoch = minEp;
    state.maxTemp = max;

    const bool validTemperature =
        !isnan(current) && current > -50.0f && current < 100.0f;
    if (validTemperature) {
      bool shouldAppend = temperatureHistoryCount == 0;
      if (!shouldAppend) {
        const uint16_t lastIndex =
            (temperatureHistoryNextIndex + TEMP_HISTORY_SIZE - 1U) %
            TEMP_HISTORY_SIZE;
        const TemperatureHistoryEntry &lastEntry =
            temperatureHistory[lastIndex];
        if (lastEntry.epoch == 0 || currentEpoch == 0) {
          shouldAppend = true;
        } else if (currentEpoch >= lastEntry.epoch) {
          shouldAppend =
              (currentEpoch - lastEntry.epoch) >= TEMP_HISTORY_INTERVAL_SEC;
        } else {
          shouldAppend = true;
        }
      }

      if (shouldAppend) {
        TemperatureHistoryEntry &entry =
            temperatureHistory[temperatureHistoryNextIndex];
        entry.value = current;
        entry.epoch = currentEpoch;
        if (temperatureHistoryCount < TEMP_HISTORY_SIZE) {
          temperatureHistoryCount++;
        }
        temperatureHistoryNextIndex =
            (temperatureHistoryNextIndex + 1U) % TEMP_HISTORY_SIZE;
      }
    }

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

TemperatureHistoryCursor SharedState::getTemperatureHistoryCursor() {
  TemperatureHistoryCursor cursor = {0, 0, TEMP_HISTORY_SIZE};
  if (mutex != NULL && xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    cursor.count = temperatureHistoryCount;
    cursor.startIndex =
        temperatureHistoryCount < TEMP_HISTORY_SIZE ? 0 : temperatureHistoryNextIndex;
    xSemaphoreGive(mutex);
  }
  return cursor;
}

bool SharedState::getTemperatureHistoryEntry(
    const TemperatureHistoryCursor &cursor, uint16_t indexFromOldest,
    TemperatureHistoryEntry &entryOut) {
  if (cursor.count == 0 || indexFromOldest >= cursor.count ||
      cursor.capacity != TEMP_HISTORY_SIZE) {
    return false;
  }

  const uint16_t physicalIndex =
      (cursor.startIndex + indexFromOldest) % TEMP_HISTORY_SIZE;
  if (mutex != NULL && xSemaphoreTake(mutex, pdMS_TO_TICKS(2)) == pdTRUE) {
    entryOut = temperatureHistory[physicalIndex];
    xSemaphoreGive(mutex);
    return true;
  }
  return false;
}
