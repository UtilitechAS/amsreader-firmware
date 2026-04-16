/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 * 
 */

#include "Cosem.h"
#include "byteorder.h"
#include <time.h>

time_t decodeCosemDateTime(CosemDateTime timestamp) {
    uint16_t year = ntohs(timestamp.year);
    if(year < 1970) return 0;

    struct tm tm = {};
    tm.tm_year  = year - 1900;        // tm_year is years since 1900
    tm.tm_mon   = timestamp.month - 1; // tm_mon is 0-based
    tm.tm_mday  = timestamp.dayOfMonth;
    tm.tm_hour  = timestamp.hour;
    tm.tm_min   = timestamp.minute;
    tm.tm_sec   = timestamp.second;
    tm.tm_isdst = 0;

    time_t t = mktime(&tm);
    int16_t deviation = ntohs(timestamp.deviation);
    if(deviation >= -720 && deviation <= 720) {
        t += deviation * 60;
    }
    return t;
}