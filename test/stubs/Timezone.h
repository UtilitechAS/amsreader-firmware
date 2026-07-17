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

/* Arduino Timezone TimeChangeRule (offset in minutes from UTC). */
struct TimeChangeRule {
    char abbrev[6];
    uint8_t week;
    uint8_t dow;
    uint8_t month;
    uint8_t hour;
    int offset;
};

/* Minimal Timezone: applies a single (standard) offset and ignores DST. That is
 * enough for the decoder tests, which only probe the standard offset (at a
 * winter date) — and Kamstrup meters report standard time year-round anyway.
 * Default construction is identity (UTC), so existing fixtures/golden decode
 * unchanged. */
class Timezone {
    int stdOffsetMin = 0;
public:
    Timezone() {}
    Timezone(TimeChangeRule dstStart, TimeChangeRule stdStart) : stdOffsetMin(stdStart.offset) {}
    Timezone(TimeChangeRule stdTime) : stdOffsetMin(stdTime.offset) {}
    time_t toUTC(time_t t) { return t - (time_t)stdOffsetMin * 60; }
    time_t toLocal(time_t t) { return t + (time_t)stdOffsetMin * 60; }
};
