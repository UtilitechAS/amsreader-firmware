/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 */

// Positions within a local day on the days DST ends (25 hours) and starts (23 hours).
// PriceService uses these for the current price point and the ENTSO-E fetch window,
// EnergyAccounting for the hour rollover and the day totals. See #1296 and #1299.

#include <unity.h>

#include "LocalDay.h"

// Same rules as PriceService (ENTSO-E) and the Europe/Oslo and Europe/Helsinki presets
static TimeChangeRule CEST = {"CEST", 0, 1, 3, 2, 120};
static TimeChangeRule CET  = {"CET",  0, 1, 10, 3, 60};
static TimeChangeRule EEST = {"EEST", 0, 1, 3, 3, 180};
static TimeChangeRule EET  = {"EET",  0, 1, 10, 4, 120};

static Timezone cet(CEST, CET);
static Timezone eet(EEST, EET);

static time_t utc(int y, int mo, int d, int h, int mi) {
    tmElements_t t = {};
    t.Year = y - 1970; t.Month = mo; t.Day = d; t.Hour = h; t.Minute = mi;
    return makeTime(t);
}

// The local wall clock hour, which is what the code used before
static uint8_t wallHour(Timezone& tz, time_t now) {
    return (tz.toLocal(now) % 86400) / 3600;
}

// The 15 minute price point the code used before: from the wall clock
static uint8_t wallPricePoint(Timezone& tz, time_t now) {
    time_t local = tz.toLocal(now) % 86400;
    return local / (15 * 60);
}

static uint8_t pricePoint(Timezone& tz, time_t now, uint8_t resolutionMinutes) {
    return (now - localMidnightUtc(&tz, now)) / (resolutionMinutes * 60);
}

void setUp(void) {}
void tearDown(void) {}

static void test_stub_applies_dst(void) {
    // CEST ends at 01:00Z (03:00 CEST becomes 02:00 CET), starts at 01:00Z (02:00 CET becomes 03:00 CEST)
    TEST_ASSERT_EQUAL(utc(2026, 10, 25, 2, 59), cet.toLocal(utc(2026, 10, 25, 0, 59)));
    TEST_ASSERT_EQUAL(utc(2026, 10, 25, 2, 0), cet.toLocal(utc(2026, 10, 25, 1, 0)));
    TEST_ASSERT_EQUAL(utc(2027, 3, 28, 1, 59), cet.toLocal(utc(2027, 3, 28, 0, 59)));
    TEST_ASSERT_EQUAL(utc(2027, 3, 28, 3, 0), cet.toLocal(utc(2027, 3, 28, 1, 0)));
    // Local midnight is unambiguous and maps back to UTC
    TEST_ASSERT_EQUAL(utc(2026, 10, 24, 22, 0), cet.toUTC(utc(2026, 10, 25, 0, 0)));
    TEST_ASSERT_EQUAL(utc(2027, 3, 27, 23, 0), cet.toUTC(utc(2027, 3, 28, 0, 0)));
}

static void test_normal_day(void) {
    time_t now = utc(2026, 11, 5, 12, 0);
    TEST_ASSERT_EQUAL(utc(2026, 11, 4, 23, 0), localMidnightUtc(&cet, now));
    TEST_ASSERT_EQUAL(utc(2026, 11, 5, 23, 0), nextLocalMidnightUtc(&cet, now));
    TEST_ASSERT_EQUAL(13, hoursSinceLocalMidnight(&cet, now));
    TEST_ASSERT_EQUAL(wallHour(cet, now), hoursSinceLocalMidnight(&cet, now));
    TEST_ASSERT_EQUAL(wallPricePoint(cet, now), pricePoint(cet, now, 15));
}

static void test_day_length(void) {
    // Fall back 2026-10-25: 25 hours, also what the ENTSO-E request must cover
    TEST_ASSERT_EQUAL(utc(2026, 10, 24, 22, 0), localMidnightUtc(&cet, utc(2026, 10, 25, 12, 0)));
    TEST_ASSERT_EQUAL(utc(2026, 10, 25, 23, 0), nextLocalMidnightUtc(&cet, utc(2026, 10, 25, 12, 0)));
    // Asked from the day before, as the fetch of tomorrow's prices does with t + 24 h
    TEST_ASSERT_EQUAL(utc(2026, 10, 24, 22, 0), localMidnightUtc(&cet, utc(2026, 10, 24, 12, 0) + 86400));

    // Spring forward 2027-03-28: 23 hours
    TEST_ASSERT_EQUAL(utc(2027, 3, 27, 23, 0), localMidnightUtc(&cet, utc(2027, 3, 28, 12, 0)));
    TEST_ASSERT_EQUAL(utc(2027, 3, 28, 22, 0), nextLocalMidnightUtc(&cet, utc(2027, 3, 28, 12, 0)));
    TEST_ASSERT_EQUAL(utc(2027, 3, 27, 23, 0), localMidnightUtc(&cet, utc(2027, 3, 27, 12, 0) + 86400));

    // Helsinki, the #1296 reporter's zone
    TEST_ASSERT_EQUAL(utc(2026, 10, 24, 21, 0), localMidnightUtc(&eet, utc(2026, 10, 25, 12, 0)));
    TEST_ASSERT_EQUAL(utc(2026, 10, 25, 22, 0), nextLocalMidnightUtc(&eet, utc(2026, 10, 25, 12, 0)));
}

// Walk the day minute by minute and count hour rollovers the way EnergyAccounting::update() does
static void walkDay(Timezone& tz, time_t start, uint8_t expectedHours) {
    time_t end = nextLocalMidnightUtc(&tz, start);
    TEST_ASSERT_EQUAL(expectedHours * 3600, end - start);

    uint8_t current = hoursSinceLocalMidnight(&tz, start);
    uint8_t currentWall = wallHour(tz, start);
    TEST_ASSERT_EQUAL(0, current);
    int rollovers = 0, wallRollovers = 0;
    int minutesInHour = 0;
    for(time_t now = start; now < end; now += 60) {
        uint8_t h = hoursSinceLocalMidnight(&tz, now);
        if(h != current) {
            TEST_ASSERT_EQUAL(current + 1, h);
            TEST_ASSERT_EQUAL_MESSAGE(60, minutesInHour, "every hour of the day lasts 60 minutes");
            current = h;
            rollovers++;
            minutesInHour = 0;
        }
        minutesInHour++;
        uint8_t w = wallHour(tz, now);
        if(w != currentWall) {
            currentWall = w;
            wallRollovers++;
        }
    }
    TEST_ASSERT_EQUAL(60, minutesInHour);
    TEST_ASSERT_EQUAL(expectedHours - 1, current);
    TEST_ASSERT_EQUAL(expectedHours - 1, rollovers);
    TEST_ASSERT_EQUAL(0, hoursSinceLocalMidnight(&tz, end));
    // The wall clock has one rollover less in autumn: 02 runs into 02 (#1299)
    if(expectedHours == 25) TEST_ASSERT_EQUAL(23, wallRollovers);
}

static void test_hour_rollover_fall_back(void) {
    walkDay(cet, utc(2026, 10, 24, 22, 0), 25);
    walkDay(eet, utc(2026, 10, 24, 21, 0), 25);
}

static void test_hour_rollover_spring_forward(void) {
    walkDay(cet, utc(2027, 3, 27, 23, 0), 23);
    walkDay(eet, utc(2027, 3, 27, 22, 0), 23);
}

static void test_price_point_fall_back(void) {
    // #1296: around 14:00 CET on 2026-10-25 the device reported cursor 56.
    // 15 hours have passed since CET midnight (22:00Z), so the current point is 60.
    time_t now = utc(2026, 10, 25, 13, 5);
    TEST_ASSERT_EQUAL(56, wallPricePoint(cet, now));
    TEST_ASSERT_EQUAL(60, pricePoint(cet, now, 15));
    TEST_ASSERT_EQUAL(15, pricePoint(cet, now, 60));

    // Before the change the two agree
    TEST_ASSERT_EQUAL(wallPricePoint(cet, utc(2026, 10, 25, 0, 30)), pricePoint(cet, utc(2026, 10, 25, 0, 30), 15));
    // The first and second 02:00-03:00 are different points
    TEST_ASSERT_EQUAL(8, pricePoint(cet, utc(2026, 10, 25, 0, 0), 15));
    TEST_ASSERT_EQUAL(12, pricePoint(cet, utc(2026, 10, 25, 1, 0), 15));
    // The last point of the day is 99 of 100
    TEST_ASSERT_EQUAL(99, pricePoint(cet, utc(2026, 10, 25, 22, 59), 15));
    TEST_ASSERT_EQUAL(24, pricePoint(cet, utc(2026, 10, 25, 22, 59), 60));
    TEST_ASSERT_EQUAL(0, pricePoint(cet, utc(2026, 10, 25, 23, 0), 15));
}

static void test_price_point_spring_forward(void) {
    // 14:00 CEST on 2027-03-28 is 13 hours after CET midnight (23:00Z)
    time_t now = utc(2027, 3, 28, 12, 5);
    TEST_ASSERT_EQUAL(56, wallPricePoint(cet, now));
    TEST_ASSERT_EQUAL(52, pricePoint(cet, now, 15));
    // The last point of the day is 91 of 92
    TEST_ASSERT_EQUAL(91, pricePoint(cet, utc(2027, 3, 28, 21, 59), 15));
    TEST_ASSERT_EQUAL(22, pricePoint(cet, utc(2027, 3, 28, 21, 59), 60));
    TEST_ASSERT_EQUAL(0, pricePoint(cet, utc(2027, 3, 28, 22, 0), 15));
}

// The UTC hours that EnergyAccounting::getUseToday() adds up, before and after the fix
static void dayHours(Timezone& tz, time_t now, uint8_t* fixed, uint8_t* old, uint8_t* count) {
    *count = hoursSinceLocalMidnight(&tz, now);
    time_t midnight = localMidnightUtc(&tz, now);
    uint8_t localHour = wallHour(tz, now);
    for(uint8_t i = 0; i < *count; i++) {
        fixed[i] = ((midnight + i * 3600) / 3600) % 24;
        old[i] = ((now - (localHour - i) * 3600) / 3600) % 24;
    }
}

static void test_day_use_fall_back(void) {
    // #1299: at 15:53 CET on 2026-10-25 the day total missed 00:00-01:00 CEST (22Z)
    uint8_t fixed[25], old[25], count;
    dayHours(cet, utc(2026, 10, 25, 14, 53), fixed, old, &count);
    TEST_ASSERT_EQUAL(16, count);
    for(uint8_t i = 0; i < count; i++) {
        TEST_ASSERT_EQUAL((22 + i) % 24, fixed[i]);
    }
    TEST_ASSERT_EQUAL(23, old[0]); // The old sum started an hour late
}

static void test_day_use_spring_forward(void) {
    // At 14:30 CEST on 2027-03-28 the day started at 23Z; the old sum took 22Z from the day before
    uint8_t fixed[25], old[25], count;
    dayHours(cet, utc(2027, 3, 28, 12, 30), fixed, old, &count);
    TEST_ASSERT_EQUAL(13, count);
    for(uint8_t i = 0; i < count; i++) {
        TEST_ASSERT_EQUAL((23 + i) % 24, fixed[i]);
    }
    TEST_ASSERT_EQUAL(22, old[0]);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_stub_applies_dst);
    RUN_TEST(test_normal_day);
    RUN_TEST(test_day_length);
    RUN_TEST(test_hour_rollover_fall_back);
    RUN_TEST(test_hour_rollover_spring_forward);
    RUN_TEST(test_price_point_fall_back);
    RUN_TEST(test_price_point_spring_forward);
    RUN_TEST(test_day_use_fall_back);
    RUN_TEST(test_day_use_spring_forward);
    return UNITY_END();
}
