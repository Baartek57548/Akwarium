#include "Arduino.h"
#include "DallasTemperature.h"

#include <deque>
#include <map>

namespace {

unsigned long currentMillis = 0;
std::map<int, int> digitalInputs;
std::map<int, int> digitalOutputs;
int deviceCount = 1;
std::deque<float> sensorReadings;

} // namespace

SerialStub Serial;

unsigned long millis() { return currentMillis; }

void pinMode(int, int) {}

void digitalWrite(int pin, int value) { digitalOutputs[pin] = value; }

int digitalRead(int pin) {
  const auto it = digitalInputs.find(pin);
  return it == digitalInputs.end() ? LOW : it->second;
}

void delay(unsigned long ms) { currentMillis += ms; }

namespace ArduinoTest {

void reset() {
  currentMillis = 0;
  digitalInputs.clear();
  digitalOutputs.clear();
  DallasTemperatureTest::reset();
}

void setMillis(unsigned long value) { currentMillis = value; }

void advanceMillis(unsigned long delta) { currentMillis += delta; }

void setDigitalInput(int pin, int value) { digitalInputs[pin] = value; }

int getDigitalOutput(int pin) {
  const auto it = digitalOutputs.find(pin);
  return it == digitalOutputs.end() ? LOW : it->second;
}

} // namespace ArduinoTest

namespace DallasTemperatureTest {

void reset() {
  deviceCount = 1;
  sensorReadings.clear();
}

void setDeviceCount(int count) { deviceCount = count; }

void queueReading(float value) { sensorReadings.push_back(value); }

} // namespace DallasTemperatureTest

int DallasTemperature::getDeviceCount() const { return deviceCount; }

float DallasTemperature::getTempCByIndex(int) const {
  if (sensorReadings.empty()) {
    return DEVICE_DISCONNECTED_C;
  }

  const float value = sensorReadings.front();
  sensorReadings.pop_front();
  return value;
}
