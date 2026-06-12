/* Minimal Timezone stub for native builds.
 * On device this is the Arduino Timezone library, which also pulls in the
 * Time library (tmElements_t / makeTime). The decoder uses those, so the stub
 * provides them too. Tests treat all times as UTC. */
#pragma once
#include <time.h>
#include <stdint.h>

/* Arduino TimeLib tmElements_t (Year is offset from 1970). */
typedef struct {
    uint8_t Second;
    uint8_t Minute;
    uint8_t Hour;
    uint8_t Wday;   // day of week, 1=Sunday
    uint8_t Day;
    uint8_t Month;
    uint8_t Year;   // years since 1970
} tmElements_t;

static inline time_t makeTime(const tmElements_t& te) {
    struct tm t;
    memset(&t, 0, sizeof(t));
    t.tm_year = te.Year + 1970 - 1900;
    t.tm_mon  = te.Month - 1;
    t.tm_mday = te.Day;
    t.tm_hour = te.Hour;
    t.tm_min  = te.Minute;
    t.tm_sec  = te.Second;
    return timegm(&t);
}

class Timezone {
public:
    time_t toUTC(time_t t) { return t; }
    time_t toLocal(time_t t) { return t; }
};
