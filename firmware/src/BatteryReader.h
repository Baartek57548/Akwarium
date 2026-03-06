#ifndef BATTERY_READER_H
#define BATTERY_READER_H

#include <Arduino.h>

// Czytnik napiecia baterii podtrzymania RTC (CR2025/CR2032).
// Dla tej plytki pomiar jest 1:1 (bez dzielnika), z mozliwoscia kalibracji.
class BatteryReader {
public:
  // pinAdc  - pin ADC do pomiaru napiecia baterii
  BatteryReader(uint8_t pinAdc);

  void init();

  // Natychmiastowy pomiar blokujacy (16 prob, dummy read)
  float readVoltageBlocking();

  // Asynchroniczny pomiar non-blocking:
  void startMeasurement();
  bool isMeasurementReady();
  float getLastMeasurement();

private:
  uint8_t _pinAdc;

  // Jesli w hardware jest dzielnik, ustaw tu odpowiedni mnoznik > 1.0f.
  static constexpr float INPUT_SCALE_MULT = 1.0f;
  // Dodatkowa kalibracja jednopunktowa (1.0f = brak korekty).
  static constexpr float CALIBRATION_MULT = 1.0f;
  static constexpr uint8_t SAMPLES = 30;

  // Stan pomiaru asynchronicznego
  bool _measActive = false;
  uint32_t _measStartMs = 0;
  uint32_t _measAccum = 0;
  uint8_t _measCount = 0;
  float _measResult = NAN;

  float convertMilliVoltsToVoltage(float milliVolts) const;
};

#endif
