/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "Cosem.h"
#include "lwip/def.h"
#include <TimeLib.h>

time_t decodeCosemDateTime(CosemDateTime timestamp) {
    tmElements_t tm;
    uint16_t year = ntohs(timestamp.year);
    if(year < 1970) return 0;
    tm.Year = year - 1970;
    tm.Month = timestamp.month;
    tm.Day = timestamp.dayOfMonth;
    tm.Hour = timestamp.hour;
    tm.Minute = timestamp.minute;
    tm.Second = timestamp.second;

    time_t time = makeTime(tm);
    int16_t deviation = ntohs(timestamp.deviation);
    if(deviation >= -720 && deviation <= 720) {
        time += deviation * 60;
    }
    return time;
}