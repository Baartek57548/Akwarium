#ifndef UNIT_TEST_ARDUINO_H
#define UNIT_TEST_ARDUINO_H

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>

using std::size_t;
using std::max;
using std::min;

constexpr int LOW = 0;
constexpr int HIGH = 1;
constexpr int INPUT = 0;
constexpr int OUTPUT = 1;
constexpr int INPUT_PULLUP = 2;
constexpr int INPUT_PULLDOWN = 3;

#ifndef F
#define F(x) x
#endif

class String {
public:
  String() = default;
  String(const char *value) : data_(value != nullptr ? value : "") {}
  String(const std::string &value) : data_(value) {}
  String(char value) : data_(1, value) {}
  String(int value) : data_(std::to_string(value)) {}
  String(unsigned int value) : data_(std::to_string(value)) {}
  String(long value) : data_(std::to_string(value)) {}
  String(unsigned long value) : data_(std::to_string(value)) {}

  String &operator=(const char *value) {
    data_ = value != nullptr ? value : "";
    return *this;
  }

  String &operator+=(const char *value) {
    data_ += value != nullptr ? value : "";
    return *this;
  }

  String &operator+=(const String &value) {
    data_ += value.data_;
    return *this;
  }

  String &operator+=(char value) {
    data_ += value;
    return *this;
  }

  size_t length() const { return data_.length(); }
  const char *c_str() const { return data_.c_str(); }
  bool equals(const char *other) const { return data_ == (other != nullptr ? other : ""); }
  bool equals(const String &other) const { return data_ == other.data_; }

  bool equalsIgnoreCase(const char *other) const {
    return lowercase(data_) == lowercase(other != nullptr ? other : "");
  }

  int indexOf(char needle) const {
    const size_t pos = data_.find(needle);
    return pos == std::string::npos ? -1 : static_cast<int>(pos);
  }

  void remove(unsigned int index) {
    if (index < data_.size()) {
      data_.erase(index);
    }
  }

  void remove(unsigned int index, unsigned int count) {
    if (index < data_.size()) {
      data_.erase(index, count);
    }
  }

  operator std::string() const { return data_; }

private:
  static std::string lowercase(const std::string &value) {
    std::string lowered = value;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return lowered;
  }

  std::string data_;
};

inline String operator+(const String &lhs, const String &rhs) {
  return String(static_cast<std::string>(lhs) + static_cast<std::string>(rhs));
}

inline String operator+(const String &lhs, const char *rhs) {
  return String(static_cast<std::string>(lhs) + (rhs != nullptr ? rhs : ""));
}

inline String operator+(const char *lhs, const String &rhs) {
  return String((lhs != nullptr ? lhs : "") + static_cast<std::string>(rhs));
}

template <typename T> inline T constrain(T value, T low, T high) {
  return std::min(high, std::max(low, value));
}

unsigned long millis();
void pinMode(int pin, int mode);
void digitalWrite(int pin, int value);
int digitalRead(int pin);
void delay(unsigned long ms);

struct SerialStub {
  template <typename T> void print(const T &) {}
  template <typename T> void println(const T &) {}
};

extern SerialStub Serial;

namespace ArduinoTest {
void reset();
void setMillis(unsigned long value);
void advanceMillis(unsigned long delta);
void setDigitalInput(int pin, int value);
int getDigitalOutput(int pin);
}

#endif // UNIT_TEST_ARDUINO_H
