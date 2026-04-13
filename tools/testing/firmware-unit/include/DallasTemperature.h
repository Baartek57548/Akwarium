#ifndef UNIT_TEST_DALLAS_TEMPERATURE_H
#define UNIT_TEST_DALLAS_TEMPERATURE_H

#include "Arduino.h"
#include "OneWire.h"

#include <deque>

constexpr float DEVICE_DISCONNECTED_C = -127.0f;

namespace DallasTemperatureTest {
void reset();
void setDeviceCount(int count);
void queueReading(float value);
}

class DallasTemperature {
public:
  explicit DallasTemperature(OneWire *) {}

  void begin() {}
  void setWaitForConversion(bool) {}
  int getDeviceCount() const;
  void requestTemperatures() {}
  float getTempCByIndex(int) const;
};

#endif // UNIT_TEST_DALLAS_TEMPERATURE_H
