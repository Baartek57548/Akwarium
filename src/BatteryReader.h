#ifndef BATTERY_READER_H
#define BATTERY_READER_H

#include <Arduino.h>

// Czytnik napiecia baterii RTC (CR2032) przez dzielnik rezystorowy.
// Implementacja: oversampling 16 prob, mnoznik dzielnika 1.597 (skalibrowany)
class BatteryReader {
public:
  // pinAdc  - pin ADC do pomiaru napiecia (przez dzielnik)
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

  static constexpr float V_REF = 3.3f;
  static constexpr float ADC_RESOLUTION = 4095.0f;
  static constexpr float DIVIDER_MULT = 1.57769f;
  static constexpr uint8_t SAMPLES = 30;

  // Stan pomiaru asynchronicznego
  bool _measActive = false;
  uint32_t _measStartMs = 0;
  uint32_t _measAccum = 0;
  uint8_t _measCount = 0;
  float _measResult = NAN;

  float convertRawToVoltage(float rawValue) const;
};

#endif
