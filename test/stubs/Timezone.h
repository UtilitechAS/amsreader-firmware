/* Minimal Timezone stub for native builds.
 * On device this is the Arduino Timezone library, which also pulls in the
 * Time library (tmElements_t / makeTime). The decoder uses those, so the stub
 * provides them too. */
#pragma once
#include <time.h>
#include <stdint.h>
#include <string.h>

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

/* Arduino Timezone TimeChangeRule (offset in minutes from UTC). week 0 is the
 * last week of the month, dow 1 is Sunday. */
struct TimeChangeRule {
    char abbrev[6];
    uint8_t week;
    uint8_t dow;
    uint8_t month;
    uint8_t hour;
    int offset;
};

/* Timezone with the same DST rule handling as the Arduino Timezone library
 * (JChristensen), so code that depends on DST changes can be tested on native.
 * Default construction is identity (UTC), so existing fixtures/golden decode
 * unchanged. */
class Timezone {
    TimeChangeRule m_dst = {"UTC", 0, 1, 1, 0, 0};
    TimeChangeRule m_std = {"UTC", 0, 1, 1, 0, 0};
    time_t m_dstUTC = 0, m_stdUTC = 0, m_dstLoc = 0, m_stdLoc = 0;

    static int yearOf(time_t t) {
        struct tm tmv;
        gmtime_r(&t, &tmv);
        return tmv.tm_year + 1900;
    }

    static time_t toTime_t(TimeChangeRule r, int yr) {
        uint8_t m = r.month;
        uint8_t w = r.week;
        if(w == 0) { // Last week: go to the first week of next month, then back a week
            if(++m > 12) { m = 1; ++yr; }
            w = 1;
        }
        tmElements_t tm = {};
        tm.Hour = r.hour;
        tm.Day = 1;
        tm.Month = m;
        tm.Year = yr - 1970;
        time_t t = makeTime(tm);
        int weekday = ((t / 86400 + 4) % 7) + 1; // 1970-01-01 was a Thursday
        t += (time_t)(((r.dow - weekday + 7) % 7) + (w - 1) * 7) * 86400;
        if(r.week == 0) t -= 7 * 86400;
        return t;
    }

    void calcTimeChanges(int yr) {
        m_dstLoc = toTime_t(m_dst, yr);
        m_stdLoc = toTime_t(m_std, yr);
        m_dstUTC = m_dstLoc - (time_t)m_std.offset * 60;
        m_stdUTC = m_stdLoc - (time_t)m_dst.offset * 60;
    }

    bool utcIsDST(time_t utc) {
        if(m_dst.offset == m_std.offset) return false;
        if(yearOf(utc) != yearOf(m_dstUTC)) calcTimeChanges(yearOf(utc));
        if(m_stdUTC > m_dstUTC) return utc >= m_dstUTC && utc < m_stdUTC;
        return !(utc >= m_stdUTC && utc < m_dstUTC);
    }

    bool locIsDST(time_t local) {
        if(m_dst.offset == m_std.offset) return false;
        if(yearOf(local) != yearOf(m_dstLoc)) calcTimeChanges(yearOf(local));
        if(m_stdLoc > m_dstLoc) return local >= m_dstLoc && local < m_stdLoc;
        return !(local >= m_stdLoc && local < m_dstLoc);
    }

public:
    Timezone() {}
    Timezone(TimeChangeRule dstStart, TimeChangeRule stdStart) : m_dst(dstStart), m_std(stdStart) {}
    Timezone(TimeChangeRule stdTime) : m_dst(stdTime), m_std(stdTime) {}
    time_t toUTC(time_t t) { return t - (time_t)(locIsDST(t) ? m_dst.offset : m_std.offset) * 60; }
    time_t toLocal(time_t t) { return t + (time_t)(utcIsDST(t) ? m_dst.offset : m_std.offset) * 60; }
};
