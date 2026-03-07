#ifndef CONFIG_DATA_H
#define CONFIG_DATA_H

#include <Arduino.h>

constexpr uint32_t CONFIG_MAGIC = 0xCAFEBAC4;
constexpr uint16_t CONFIG_VERSION = 6;
constexpr int SERVO_OPEN_ANGLE = 0;
constexpr int SERVO_PREOFF_ANGLE = 45;
constexpr int SERVO_CLOSED_ANGLE = 90;

enum class ScheduleMode : uint8_t {
  Schedule = 0,
  AlwaysOn = 1,
  AlwaysOff = 2
};

enum class HeaterMode : uint8_t {
  Threshold = 0,
  Off = 1
};

struct Config {
  uint8_t lightMode;
  uint8_t dayStartHour;
  uint8_t dayStartMinute;
  uint8_t dayEndHour;
  uint8_t dayEndMinute;
  uint8_t aerationMode;
  uint8_t aerationHourOn;
  uint8_t aerationMinuteOn;
  uint8_t aerationHourOff;
  uint8_t aerationMinuteOff;
  uint8_t filterMode;
  uint8_t filterHourOn;
  uint8_t filterMinuteOn;
  uint8_t filterHourOff;
  uint8_t filterMinuteOff;
  uint8_t servoPreOffMins;
  uint8_t heaterMode;
  float targetTemp;
  float tempHysteresis;
  int servoDayAngle;
  int servoNightAngle;
  int servoAlarmAngle;
  uint8_t feedMode;
  uint8_t feedHour;
  uint8_t feedMinute;
  uint32_t lastFeedEpoch;
  bool alwaysScreenOn;
  uint16_t version;
  uint32_t magic;
  uint32_t crc32; // Nowe pole dla CRC w celu zapewnienia weryfikacji zawartosci
                  // flash
};

enum PowerMode { MODE_ACTIVE, MODE_LOW_POWER, MODE_DEEP_SLEEP };

enum SystemState {
  STATE_NORMAL,
  STATE_MENU,
  STATE_QUIET,
  STATE_SHUTDOWN,
  STATE_ACCESS_POINT,
  STATE_FEEDING
};

#endif // CONFIG_DATA_H
