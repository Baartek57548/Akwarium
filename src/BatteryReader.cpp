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
  analogRead(_pinAdc);

  uint32_t sum = 0;
  for (uint8_t i = 0; i < SAMPLES; i++) {
    sum += analogRead(_pinAdc);
  }
  float rawAvg = (float)sum / SAMPLES;
  return convertRawToVoltage(rawAvg);
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
  analogRead(_pinAdc);
}

bool BatteryReader::isMeasurementReady() {
  if (!_measActive)
    return false;

  // Zbieraj probki az nie zbierzemy SAMPLES
  if (_measCount < SAMPLES) {
    _measAccum += analogRead(_pinAdc);
    _measCount++;
  }
  return (_measCount >= SAMPLES);
}

float BatteryReader::getLastMeasurement() {
  if (!_measActive || _measCount < SAMPLES)
    return NAN;

  float rawAvg = (float)_measAccum / SAMPLES;
  // to samo absolutne mapowanie dla odczytu pętli
  _measResult = convertRawToVoltage(rawAvg);
  _measActive = false;
  return _measResult;
}

float BatteryReader::convertRawToVoltage(float rawValue) const {
  float pinVoltage = (rawValue * V_REF) / ADC_RESOLUTION;
  return pinVoltage * DIVIDER_MULT;
}
