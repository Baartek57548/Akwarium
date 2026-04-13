#ifndef UNIT_TEST_RTCLIB_H
#define UNIT_TEST_RTCLIB_H

#include <cstdint>

class DateTime {
public:
  DateTime() : epoch_(0), year_(2000), month_(1), day_(1), hour_(0), minute_(0), second_(0) {}
  explicit DateTime(uint32_t epoch)
      : epoch_(epoch), year_(2000), month_(1), day_(1), hour_(0), minute_(0), second_(0) {}
  DateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour,
           uint8_t minute, uint8_t second)
      : epoch_(0), year_(year), month_(month), day_(day), hour_(hour),
        minute_(minute), second_(second) {}

  uint8_t hour() const { return hour_; }
  uint8_t minute() const { return minute_; }
  uint8_t second() const { return second_; }
  uint8_t day() const { return day_; }
  uint8_t month() const { return month_; }
  uint16_t year() const { return year_; }
  uint32_t unixtime() const { return epoch_; }

private:
  uint32_t epoch_;
  uint16_t year_;
  uint8_t month_;
  uint8_t day_;
  uint8_t hour_;
  uint8_t minute_;
  uint8_t second_;
};

#endif // UNIT_TEST_RTCLIB_H
