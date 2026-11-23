/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 */

// Hour budget, headroom and status for the capacity tariff (#857). Peak values are in
// 10 Wh units, thresholds in kWh, as in EnergyAccounting.

#include <unity.h>
#include <stdlib.h>

#include "CapacityBudget.h"

void setUp(void) {}
void tearDown(void) {}

static void setPeaks(EnergyAccountingPeak peaks[5], const uint16_t values[5]) {
    for(uint8_t i = 0; i < 5; i++) {
        peaks[i].day = values[i] > 0 ? i + 1 : 0;
        peaks[i].hour = 12;
        peaks[i].value = values[i];
    }
}

static uint32_t monthMaxAfter(const EnergyAccountingPeak peaks[5], uint8_t hours, uint8_t day, uint16_t val) {
    EnergyAccountingPeak test[5];
    memcpy(test, peaks, sizeof(test));
    capacityUpdatePeaks(test, val, day, 0);
    return capacityMonthMax(test, hours);
}

static void test_month_max_averages_top_hours(void) {
    EnergyAccountingPeak peaks[5];
    const uint16_t v[5] = { 400, 300, 200, 0, 0 };
    setPeaks(peaks, v);
    TEST_ASSERT_EQUAL_UINT32(300, capacityMonthMax(peaks, 3));
    TEST_ASSERT_EQUAL_UINT32(350, capacityMonthMax(peaks, 2));
    // Fewer days than hours: averaged over the days there are
    TEST_ASSERT_EQUAL_UINT32(300, capacityMonthMax(peaks, 5));
}

static void test_replaced_peak_gets_its_hour(void) {
    EnergyAccountingPeak peaks[5];
    const uint16_t v[5] = { 500, 450, 300, 350, 400 };
    setPeaks(peaks, v); // days 1-5 at hour 12, all slots taken
    TEST_ASSERT_TRUE(capacityUpdatePeaks(peaks, 420, 6, 18));
    // The lowest peak (day 3) makes room for day 6
    TEST_ASSERT_EQUAL_UINT8(6, peaks[2].day);
    TEST_ASSERT_EQUAL_UINT8(18, peaks[2].hour);
    TEST_ASSERT_EQUAL_UINT16(420, peaks[2].value);
}

static void test_first_day_of_month_budget_is_threshold(void) {
    EnergyAccountingPeak peaks[5];
    const uint16_t v[5] = { 0, 0, 0, 0, 0 };
    setPeaks(peaks, v);
    TEST_ASSERT_EQUAL_UINT16(500, capacityHourBudget(peaks, 3, 1, 5));
}

static void test_second_day_shares_average_with_first(void) {
    EnergyAccountingPeak peaks[5];
    const uint16_t v[5] = { 400, 0, 0, 0, 0 };
    setPeaks(peaks, v);
    // The average would allow 6.01 kWh, but the budget never exceeds the threshold
    TEST_ASSERT_EQUAL_UINT16(500, capacityHourBudget(peaks, 3, 2, 5));
}

static void test_mid_month_budget(void) {
    EnergyAccountingPeak peaks[5];
    const uint16_t v[5] = { 400, 300, 200, 0, 0 };
    setPeaks(peaks, v);
    // The average would allow 8.02 kWh, the budget stops at the 5 kWh threshold
    TEST_ASSERT_EQUAL_UINT16(500, capacityHourBudget(peaks, 3, 4, 5));
}

static void test_peaks_above_threshold_lower_the_budget(void) {
    EnergyAccountingPeak peaks[5];
    const uint16_t v[5] = { 700, 600, 200, 0, 0 };
    setPeaks(peaks, v); // month max 5 kWh, two days above the threshold
    // Today (day 4) replaces the 2 kWh day: (700 + 600 + v) / 3 <= 500
    TEST_ASSERT_EQUAL_UINT16(202, capacityHourBudget(peaks, 3, 4, 5));
}

static void test_today_already_has_a_peak(void) {
    EnergyAccountingPeak peaks[5];
    const uint16_t v[5] = { 400, 300, 200, 600, 0 };
    setPeaks(peaks, v); // day 4 is today with 6 kWh so far
    TEST_ASSERT_EQUAL_UINT16(500, capacityHourBudget(peaks, 3, 4, 5));
    // An hour below today's peak changes nothing, so the budget is never below it, even
    // when the month max may not rise at all (above every step). 1601 / 3 still truncates to 533.
    const uint16_t w[5] = { 400, 300, 200, 900, 0 };
    setPeaks(peaks, w);
    TEST_ASSERT_EQUAL_UINT16(901, capacityHourBudget(peaks, 3, 4, CAPACITY_NO_THRESHOLD));
}

static void test_all_five_slots_taken(void) {
    EnergyAccountingPeak peaks[5];
    const uint16_t v[5] = { 500, 450, 400, 350, 300 };
    setPeaks(peaks, v); // days 1-5, today is day 6
    // A new day enters the top 3 only above 4 kWh: (500 + 450 + v) / 3 <= 500 allows 5.52,
    // capped at the threshold
    TEST_ASSERT_EQUAL_UINT16(500, capacityHourBudget(peaks, 3, 6, 5));
}

static void test_above_every_step_holds_month_max(void) {
    EnergyAccountingPeak peaks[5];
    const uint16_t v[5] = { 400, 300, 200, 0, 0 };
    setPeaks(peaks, v);
    // thresholds[9] is the 0xFFFF sentinel: do not lift the month max (3 kWh) at all
    TEST_ASSERT_EQUAL_UINT16(202, capacityHourBudget(peaks, 3, 4, CAPACITY_NO_THRESHOLD));
}

// Closing the hour at the budget keeps the month max within the limit and the budget is
// never above the threshold. Below the threshold, one step more goes over the limit.
// Random peaks, hours and thresholds.
static void test_budget_is_the_edge(void) {
    srand(857);
    for(int n = 0; n < 2000; n++) {
        EnergyAccountingPeak peaks[5];
        uint8_t days = rand() % 6;
        uint16_t v[5] = { 0, 0, 0, 0, 0 };
        for(uint8_t i = 0; i < days; i++) v[i] = 1 + rand() % 1500;
        setPeaks(peaks, v);
        uint8_t hours = 1 + rand() % 5;
        uint8_t today = (rand() % 2 && days > 0) ? 1 + rand() % days : 6;
        uint16_t threshold = 1 + rand() % 20;

        uint32_t limit = capacityMonthMax(peaks, hours);
        if(threshold * 100UL > limit) limit = threshold * 100UL;

        uint16_t budget = capacityHourBudget(peaks, hours, today, threshold);
        TEST_ASSERT_TRUE(monthMaxAfter(peaks, hours, today, budget) <= limit);
        TEST_ASSERT_TRUE(budget <= threshold * 100);
        if(budget < threshold * 100) {
            TEST_ASSERT_TRUE(monthMaxAfter(peaks, hours, today, budget + 1) > limit);
        }
    }
}

static void test_headroom(void) {
    // 3 kWh left over half an hour is 6 kW; 3 kW is drawn now
    TEST_ASSERT_FLOAT_WITHIN(0.001, 3.0, capacityHeadroom(5.0, 2.0, 3.0, 1800, 0));
    // Drawing 8 kW: shed 2
    TEST_ASSERT_FLOAT_WITHIN(0.001, -2.0, capacityHeadroom(5.0, 2.0, 8.0, 1800, 0));
    // Over the budget: everything drawn is reported as to be shed
    TEST_ASSERT_FLOAT_WITHIN(0.001, -4.0, capacityHeadroom(5.0, 6.0, 4.0, 1800, 0));
    // Last seconds of the hour: time floored at 30 s
    TEST_ASSERT_FLOAT_WITHIN(0.001, 12.0, capacityHeadroom(5.0, 4.9, 0.0, 5, 0));
}

static void test_headroom_limited_by_main_fuse(void) {
    // 4 kWh left in 10 minutes would allow 24 kW; a 17.3 kW fuse with 3 kW drawn leaves 14.3
    TEST_ASSERT_FLOAT_WITHIN(0.001, 14.3, capacityHeadroom(5.0, 1.0, 3.0, 600, 17.3));
    // Below the fuse the budget decides
    TEST_ASSERT_FLOAT_WITHIN(0.001, 3.0, capacityHeadroom(5.0, 2.0, 3.0, 1800, 17.3));
    // Shedding is not limited by the fuse
    TEST_ASSERT_FLOAT_WITHIN(0.001, -4.0, capacityHeadroom(5.0, 6.0, 4.0, 1800, 17.3));
}

static void test_fuse_capacity(void) {
    // 25 A: TN 400 V three phase, IT 230 V three phase, two phase, single phase
    TEST_ASSERT_EQUAL_UINT32(17320, fuseCapacityW(2, 25, true, false));
    TEST_ASSERT_EQUAL_UINT32(9959, fuseCapacityW(1, 25, true, false));
    TEST_ASSERT_EQUAL_UINT32(5750, fuseCapacityW(1, 25, false, true));
    TEST_ASSERT_EQUAL_UINT32(5750, fuseCapacityW(2, 25, false, false));
}

static void test_status(void) {
    TEST_ASSERT_EQUAL_UINT8(CAPACITY_STATUS_OK, capacityStatus(5.0, 4.0));
    TEST_ASSERT_EQUAL_UINT8(CAPACITY_STATUS_OK, capacityStatus(5.0, 4.5));
    TEST_ASSERT_EQUAL_UINT8(CAPACITY_STATUS_WARNING, capacityStatus(5.0, 4.6));
    TEST_ASSERT_EQUAL_UINT8(CAPACITY_STATUS_WARNING, capacityStatus(5.0, 5.0));
    TEST_ASSERT_EQUAL_UINT8(CAPACITY_STATUS_ALARM, capacityStatus(5.0, 5.1));
    TEST_ASSERT_EQUAL_STRING("alarm", capacityStatusName(CAPACITY_STATUS_ALARM));
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_month_max_averages_top_hours);
    RUN_TEST(test_replaced_peak_gets_its_hour);
    RUN_TEST(test_first_day_of_month_budget_is_threshold);
    RUN_TEST(test_second_day_shares_average_with_first);
    RUN_TEST(test_mid_month_budget);
    RUN_TEST(test_peaks_above_threshold_lower_the_budget);
    RUN_TEST(test_today_already_has_a_peak);
    RUN_TEST(test_all_five_slots_taken);
    RUN_TEST(test_above_every_step_holds_month_max);
    RUN_TEST(test_budget_is_the_edge);
    RUN_TEST(test_headroom);
    RUN_TEST(test_headroom_limited_by_main_fuse);
    RUN_TEST(test_fuse_capacity);
    RUN_TEST(test_status);
    return UNITY_END();
}
