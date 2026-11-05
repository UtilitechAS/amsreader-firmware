/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 */

#ifndef _LOCALDAY_H
#define _LOCALDAY_H

#include <time.h>
#include <stdint.h>
#include "Timezone.h"

// A local day is not always 24 hours: the day DST ends has 25 and the day it starts has 23.
// Positions within the day (hour of day, price point) are therefore counted in elapsed time
// from local midnight. The wall clock repeats an hour in autumn and skips one in spring.

// UTC time of the local midnight that started the day `now` is in
static inline time_t localMidnightUtc(Timezone* tz, time_t now) {
    time_t local = tz->toLocal(now);
    return tz->toUTC(local - (local % 86400));
}

// UTC time of the first local midnight after `now`
static inline time_t nextLocalMidnightUtc(Timezone* tz, time_t now) {
    // 26 hours after midnight is in the next day whether today has 23, 24 or 25 hours
    return localMidnightUtc(tz, localMidnightUtc(tz, now) + 26 * 3600);
}

// Whole hours since local midnight: 0-23, 0-24 on a 25 hour day and 0-22 on a 23 hour day
static inline uint8_t hoursSinceLocalMidnight(Timezone* tz, time_t now) {
    return (now - localMidnightUtc(tz, now)) / 3600;
}

#endif
