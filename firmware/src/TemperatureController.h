#ifndef TEMPERATURE_CONTROLLER_H
#define TEMPERATURE_CONTROLLER_H

#include <OneWire.h>
#include <DallasTemperature.h>

class TemperatureController {
private:
  OneWire oneWire;
  DallasTemperature sensors;
  int oneWirePin;
  int heaterPin;
  float targetTemp;
  float hysteresis;
  bool heaterState;
  unsigned long lastSwitchTime;
  const unsigned long MIN_SWITCH_INTERVAL = 120000;
  bool sensorPresent;
  float lastTemperature;
  bool hasValidTemperature;
  uint8_t invalidSampleCount;
  unsigned long lastTempRead;
  const unsigned long TEMP_READ_INTERVAL = 2000;
  static constexpr uint8_t MAX_INVALID_SAMPLES = 3;
  bool heaterOutputActiveHigh;

  // Nowe zmienne do statystyk
  float dailyMinTemp;
  float dailyMaxTemp;
  bool isValidTempSample(float t) const;
  void refreshSensorPresence();
  void writeHeaterOutput(bool enabled);

public:
  TemperatureController(int oneWirePin, int heaterPin, float targetTemp = 24.0f,
                        float hysteresis = 0.5f,
                        bool heaterOutputActiveHigh = true);
  void begin();
  float readTemperature();
  void controlHeater(float currentTemp);
  void setTargetTemperature(float temp);
  void setHysteresis(float value);
  bool isHeaterOn();
  void forceHeaterOn();
  void forceHeaterOff();

  // Nowe metody
  void resetDailyStats(float currentTemp);
  float getDailyMin();
  float getDailyMax();
};

#endif
