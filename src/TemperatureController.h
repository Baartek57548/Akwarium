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
  unsigned long lastTempRead;
  const unsigned long TEMP_READ_INTERVAL = 2000;

  // Nowe zmienne do statystyk
  float dailyMinTemp;
  float dailyMaxTemp;
  bool isValidTempSample(float t) const;
  void refreshSensorPresence();

public:
  TemperatureController(int oneWirePin, int heaterPin, float targetTemp = 24.0f, float hysteresis = 0.5f);
  void begin();
  float readTemperature();
  void controlHeater(float currentTemp);
  void setTargetTemperature(float temp);
  void setHysteresis(float value);
  bool isHeaterOn();
  void forceHeaterOff();

  // Nowe metody
  void resetDailyStats(float currentTemp);
  float getDailyMin();
  float getDailyMax();
};

#endif
