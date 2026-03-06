#ifndef SHARED_STATE_H
#define SHARED_STATE_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// STRUKTURA PRZEKAZYWANA POMIĘDZY RDZENIAMI (ZNAJDUJE SIE W TYM SAMYM
// PAMIECIOWYM BLOKU)
struct SharedStateData {
  float temperature;
  float minTemp;
  uint32_t minTempEpoch;
  float maxTemp;

  float batteryVoltage;
  float batteryPercent;

  bool isHeaterOn;
  bool isFilterOn;
  bool isLightOn;
  bool isDay;

  uint8_t aerationPercent;

  int hour;
  int minute;
  int second;
  int day;
  int month;
  int year;
};

// Singleton zarządzający dostępem równoległym do stuktury poprzez FreeRTOS
// XSemaphore
class SharedState {
public:
  static void init();

  // Core 0 (Read Only) - Pobieranie pełnego Snapshota jako wartość (izolacja)
  static SharedStateData getSnapshot();

  // Core 1 (Write - Szybko i wydajnie by nie stwarzać problemów lockowania)
  static void updateTemperature(float current, float min, uint32_t minEp,
                                float max);
  static void updateBattery(float volt, float pct);
  static void updateRelays(bool heater, bool filter, bool light, bool dayMode);
  static void updateTime(int h, int m, int s, int d, int mo, int y);
  static void updateAeration(uint8_t pct);

private:
  static SemaphoreHandle_t mutex;
  static SharedStateData state;
};

#endif // SHARED_STATE_H
