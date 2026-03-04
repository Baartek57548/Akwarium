#include "TimeUtils.h"

#include <stdlib.h>
#include <time.h>

namespace {

static bool timezoneInitialized = false;
static const char *WARSAW_TZ = "CET-1CEST,M3.5.0/2,M10.5.0/3";

} // namespace

void TimeUtils::initTimezone() {
  if (timezoneInitialized) {
    return;
  }

  setenv("TZ", WARSAW_TZ, 1);
  tzset();
  timezoneInitialized = true;
}

DateTime TimeUtils::utcEpochToLocalDateTime(uint32_t epochUtc) {
  initTimezone();

  time_t raw = static_cast<time_t>(epochUtc);
  struct tm localTm = {};
  if (localtime_r(&raw, &localTm) == nullptr) {
    return DateTime(epochUtc);
  }

  return DateTime(localTm.tm_year + 1900, localTm.tm_mon + 1, localTm.tm_mday,
                  localTm.tm_hour, localTm.tm_min, localTm.tm_sec);
}

DateTime TimeUtils::utcDateTimeToLocal(const DateTime &utcDateTime) {
  return utcEpochToLocalDateTime(utcDateTime.unixtime());
}

uint32_t TimeUtils::localDateTimeToUtcEpoch(const DateTime &localDateTime) {
  initTimezone();

  struct tm localTm = {};
  localTm.tm_year = static_cast<int>(localDateTime.year()) - 1900;
  localTm.tm_mon = static_cast<int>(localDateTime.month()) - 1;
  localTm.tm_mday = static_cast<int>(localDateTime.day());
  localTm.tm_hour = static_cast<int>(localDateTime.hour());
  localTm.tm_min = static_cast<int>(localDateTime.minute());
  localTm.tm_sec = static_cast<int>(localDateTime.second());
  localTm.tm_isdst = -1;

  time_t utcEpoch = mktime(&localTm);
  if (utcEpoch < 0) {
    return localDateTime.unixtime();
  }

  return static_cast<uint32_t>(utcEpoch);
}
