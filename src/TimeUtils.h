#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <RTClib.h>

namespace TimeUtils {

void initTimezone();
DateTime utcEpochToLocalDateTime(uint32_t epochUtc);
DateTime utcDateTimeToLocal(const DateTime &utcDateTime);
uint32_t localDateTimeToUtcEpoch(const DateTime &localDateTime);

} // namespace TimeUtils

#endif // TIME_UTILS_H
