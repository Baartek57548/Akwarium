#include "BatteryReader.h"
#include <math.h>

BatteryReader::BatteryReader(uint8_t pinAdc) : _pinAdc(pinAdc) {}

void BatteryReader::init() {
  analogSetAttenuation(ADC_11db);
  analogSetPinAttenuation(_pinAdc, ADC_11db);
  analogReadResolution(12);
}

float BatteryReader::readVoltageBlocking() {
  // Dummy read - odrzucenie pierwszego niedokladnego odczytu
  analogReadMilliVolts(_pinAdc);

  uint32_t sum = 0;
  for (uint8_t i = 0; i < SAMPLES; i++) {
    sum += static_cast<uint32_t>(analogReadMilliVolts(_pinAdc));
  }
  float milliVoltsAvg = static_cast<float>(sum) / SAMPLES;
  return convertMilliVoltsToVoltage(milliVoltsAvg);
}

// --- Asynchroniczny pomiar (non-blocking) ---

void BatteryReader::startMeasurement() {
  if (_measActive)
    return;
  _measAccum = 0;
  _measCount = 0;
  _measResult = NAN;
  _measStartMs = millis();
  _measActive = true;
  // Dummy read przy starcie
  analogReadMilliVolts(_pinAdc);
}

bool BatteryReader::isMeasurementReady() {
  if (!_measActive)
    return false;

  // Zbieraj probki az nie zbierzemy SAMPLES
  if (_measCount < SAMPLES) {
    _measAccum += static_cast<uint32_t>(analogReadMilliVolts(_pinAdc));
    _measCount++;
  }
  return (_measCount >= SAMPLES);
}

float BatteryReader::getLastMeasurement() {
  if (!_measActive || _measCount < SAMPLES)
    return NAN;

  float milliVoltsAvg = static_cast<float>(_measAccum) / SAMPLES;
  _measResult = convertMilliVoltsToVoltage(milliVoltsAvg);
  _measActive = false;
  return _measResult;
}

float BatteryReader::convertMilliVoltsToVoltage(float milliVolts) const {
  float pinVoltage = milliVolts / 1000.0f;
  return pinVoltage * INPUT_SCALE_MULT * CALIBRATION_MULT;
}
