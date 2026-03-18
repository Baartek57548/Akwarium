#ifndef ARDUINO_COMPAT_H
#define ARDUINO_COMPAT_H

#include <algorithm>
#include <cstdint>

using byte = std::uint8_t;

#ifndef LOW
#define LOW 0
#endif

#ifndef HIGH
#define HIGH 1
#endif

#ifndef PROGMEM
#define PROGMEM
#endif

unsigned long millis();
void delay(unsigned long ms);
long random(long maxExclusive);
long random(long minInclusive, long maxExclusive);
void fake_set_millis(unsigned long value);
void fake_advance_millis(unsigned long deltaMs);

template <typename T, typename Tmin, typename Tmax>
constexpr T constrain(T value, Tmin minimum, Tmax maximum) {
  const T minValue = static_cast<T>(minimum);
  const T maxValue = static_cast<T>(maximum);
  return std::clamp(value, minValue, maxValue);
}

#endif
