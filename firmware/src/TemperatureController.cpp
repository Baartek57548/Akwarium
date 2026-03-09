#include "TemperatureController.h"

#include <Arduino.h>
#include <math.h>

TemperatureController::TemperatureController(int oneWirePin, int heaterPin,
                                             float targetTemp, float hysteresis,
                                             bool heaterOutputActiveHigh)
    : oneWire(oneWirePin), sensors(&oneWire), oneWirePin(oneWirePin),
      heaterPin(heaterPin), targetTemp(targetTemp), hysteresis(hysteresis),
      heaterState(false), lastSwitchTime(0), sensorPresent(false),
      lastTemperature(0.0f), hasValidTemperature(false), lastTempRead(0),
      heaterOutputActiveHigh(heaterOutputActiveHigh), dailyMinTemp(100.0f),
      dailyMaxTemp(-100.0f) {}

void TemperatureController::writeHeaterOutput(bool enabled) {
  digitalWrite(heaterPin,
               enabled ? (heaterOutputActiveHigh ? HIGH : LOW)
                       : (heaterOutputActiveHigh ? LOW : HIGH));
}

void TemperatureController::begin() {
  pinMode(oneWirePin, INPUT_PULLUP);
  pinMode(heaterPin, OUTPUT);
  heaterState = true;
  writeHeaterOutput(true);
  lastSwitchTime = millis() - MIN_SWITCH_INTERVAL;

  sensors.begin();
  sensors.setWaitForConversion(true);
  refreshSensorPresence();

  Serial.print(F("[TEMP] OneWire GPIO "));
  Serial.print(oneWirePin);
  Serial.print(F(", devices found: "));
  Serial.println(sensors.getDeviceCount());
}

bool TemperatureController::isValidTempSample(float t) const {
  if (t == DEVICE_DISCONNECTED_C) {
    return false;
  }

  if (fabsf(t - 85.0f) < 0.01f) {
    return false;
  }

  return (t > -50.0f) && (t < 50.0f);
}

void TemperatureController::refreshSensorPresence() {
  sensorPresent = (sensors.getDeviceCount() > 0);
}

float TemperatureController::readTemperature() {
  unsigned long now = millis();

  if (now - lastTempRead < TEMP_READ_INTERVAL) {
    return hasValidTemperature ? lastTemperature : DEVICE_DISCONNECTED_C;
  }
  lastTempRead = now;

  if (!sensorPresent) {
    refreshSensorPresence();
    if (!sensorPresent) {
      return hasValidTemperature ? lastTemperature : DEVICE_DISCONNECTED_C;
    }
  }

  sensors.requestTemperatures();
  float t = sensors.getTempCByIndex(0);

  if (isValidTempSample(t)) {
    lastTemperature = t;
    hasValidTemperature = true;

    if (t < dailyMinTemp) {
      dailyMinTemp = t;
    }
    if (t > dailyMaxTemp) {
      dailyMaxTemp = t;
    }
  } else {
    refreshSensorPresence();
  }

  return hasValidTemperature ? lastTemperature : DEVICE_DISCONNECTED_C;
}

void TemperatureController::controlHeater(float currentTemp) {
  unsigned long now = millis();

  if (currentTemp == DEVICE_DISCONNECTED_C) {
    return;
  }

  if (heaterState && currentTemp >= targetTemp) {
    if (now - lastSwitchTime >= MIN_SWITCH_INTERVAL) {
      writeHeaterOutput(false);
      heaterState = false;
      lastSwitchTime = now;
    }
  } else if (!heaterState && currentTemp <= (targetTemp - hysteresis)) {
    if (now - lastSwitchTime >= MIN_SWITCH_INTERVAL) {
      writeHeaterOutput(true);
      heaterState = true;
      lastSwitchTime = now;
    }
  }
}

void TemperatureController::setTargetTemperature(float temp) {
  targetTemp = constrain(temp, 18.0f, 30.0f);
}

void TemperatureController::setHysteresis(float value) {
  hysteresis = constrain(value, 0.1f, 5.0f);
}

bool TemperatureController::isHeaterOn() { return heaterState; }

void TemperatureController::forceHeaterOff() {
  writeHeaterOutput(false);
  heaterState = false;
  lastSwitchTime = millis();
}

void TemperatureController::resetDailyStats(float currentTemp) {
  dailyMinTemp = currentTemp;
  dailyMaxTemp = currentTemp;
}

float TemperatureController::getDailyMin() { return dailyMinTemp; }

float TemperatureController::getDailyMax() { return dailyMaxTemp; }
