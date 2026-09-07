/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 * 
 */

#include "Cosem.h"
#include "byteorder.h"
#include <time.h>

// Days since 1970-01-01 for a proleptic Gregorian date (Howard Hinnant's
// days_from_civil). Pure integer arithmetic: unlike mktime() it never consults
// the process timezone.
static int32_t daysFromCivil(int32_t y, uint32_t m, uint32_t d) {
    y -= m <= 2;
    const int32_t era = (y >= 0 ? y : y - 399) / 400;
    const uint32_t yoe = (uint32_t) (y - era * 400);                     // [0, 399]
    const uint32_t doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; // [0, 365]
    const uint32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;          // [0, 146096]
    return era * 146097 + (int32_t) doe - 719468;
}

time_t decodeCosemDateTime(CosemDateTime timestamp) {
    uint16_t year = ntohs(timestamp.year);
    if(year < 1970) return 0;

    // The broken-down fields are converted as if they were UTC. The offset is
    // carried by the meter's own deviation, applied below. Converting with a
    // timezone-aware call such as mktime() would apply the firmware's
    // configured zone a second time, putting the clock one whole UTC offset in
    // the past (-1h in CET, -2h in CEST).
    time_t t = (time_t) daysFromCivil(year, timestamp.month, timestamp.dayOfMonth) * 86400
             + timestamp.hour * 3600 + timestamp.minute * 60 + timestamp.second;

    int16_t deviation = ntohs(timestamp.deviation);
    if(deviation >= -720 && deviation <= 720) {
        t += deviation * 60;
    }
    return t;
}