/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 * Guards the stale whole-hour register detector (#1119).
 *
 * Some meters publish the previous whole hour's accumulated registers again at
 * the next whole hour: identical registers AND an identical meter clock, while
 * the instantaneous values in the same payload are current. Without detection
 * the ending hour is stored as 0 and the next hour is credited with two hours
 * of consumption, which destroys the month's tariff peak.
 *
 * gh1119-1..4 are four consecutive real whole-hour payloads from an Aidon meter
 * (2025-12-14 16:00, 17:00, 18:00, 19:00 wall clock). The 18:00 payload is the
 * defect: it repeats the 17:00 clock and the 17:00 registers.
 *
 * The assertions are deliberately *relative* between fixtures. decodeCosemDateTime()
 * builds its time_t with mktime(), so absolute values depend on the host TZ and
 * would make this test environment-dependent.
 */
#include <unity.h>
#include <stdio.h>
#include "AmsData.h"
#include "decoder_harness.h"

#define F1 "test/payloads/aidon/gh1119-1.hex"
#define F2 "test/payloads/aidon/gh1119-2.hex"
#define F3 "test/payloads/aidon/gh1119-3.hex"
#define F4 "test/payloads/aidon/gh1119-4.hex"

// Mirror of the firmware pipeline in handleDataSuccess(): stamp the packet with
// the verdict from the current state, then apply it.
static bool feed(AmsData& state, AmsData* packet) {
    bool stale = state.isStaleCounter(*packet);
    packet->setCounterStale(stale);
    state.apply(*packet);
    return stale;
}

// A synthetic sub-hour packet, standing in for the List 1/2 traffic that
// accumulates the integrated estimate between whole hours. AmsData's fields are
// protected, so a subclass is the least contrived way to build one.
class FakeList2 : public AmsData {
public:
    FakeList2(uint64_t millis, uint32_t importPower) {
        this->listType = 2;
        this->activeImportPower = importPower;
        this->lastUpdateMillis = millis;
    }
};

// The raw payloads carry the defect: fixture 3 repeats fixture 2's clock and
// registers, and fixture 4 jumps two hours ahead of fixture 2.
void test_stale_fixture_pair_is_identical(void) {
    AmsData* d1 = harness_decode_fixture(F1);
    AmsData* d2 = harness_decode_fixture(F2);
    AmsData* d3 = harness_decode_fixture(F3);
    AmsData* d4 = harness_decode_fixture(F4);
    TEST_ASSERT_NOT_NULL(d1);
    TEST_ASSERT_NOT_NULL(d2);
    TEST_ASSERT_NOT_NULL(d3);
    TEST_ASSERT_NOT_NULL(d4);

    // All four are whole-hour List 3 payloads with a meter clock.
    TEST_ASSERT_EQUAL(3, d1->getListType());
    TEST_ASSERT_EQUAL(3, d2->getListType());
    TEST_ASSERT_EQUAL(3, d3->getListType());
    TEST_ASSERT_EQUAL(3, d4->getListType());
    TEST_ASSERT_NOT_EQUAL(0, d1->getMeterTimestamp());

    // Hourly cadence.
    TEST_ASSERT_EQUAL_INT32(3600, (int32_t) (d2->getMeterTimestamp() - d1->getMeterTimestamp()));

    // The defect: the third payload repeats the second one's clock and registers.
    TEST_ASSERT_EQUAL_INT32(0, (int32_t) (d3->getMeterTimestamp() - d2->getMeterTimestamp()));
    TEST_ASSERT_TRUE(d2->getActiveImportCounter() == d3->getActiveImportCounter());

    // ...while the instantaneous values are current, so this is not a resend.
    TEST_ASSERT_NOT_EQUAL(d2->getActiveImportPower(), d3->getActiveImportPower());
    TEST_ASSERT_EQUAL_UINT32(12633, d3->getActiveImportPower());

    // The meter skips the hour entirely: the next clock is two hours on, and its
    // delta covers both hours.
    TEST_ASSERT_EQUAL_INT32(7200, (int32_t) (d4->getMeterTimestamp() - d2->getMeterTimestamp()));
    TEST_ASSERT_FLOAT_WITHIN(0.001, 8.056, (float) (d4->getActiveImportCounter() - d2->getActiveImportCounter()));

    delete d1; delete d2; delete d3; delete d4;
}

// The detector flags the third payload and only the third payload.
void test_stale_detector_flags_repeat_only(void) {
    AmsData state;

    // First accepted reading: no cadence learned yet, nothing to compare against.
    AmsData* d1 = harness_decode_fixture(F1);
    TEST_ASSERT_FALSE(feed(state, d1));
    TEST_ASSERT_FALSE(state.isCounterStale());
    double c1 = state.getActiveImportCounter();

    // Second reading establishes the hourly cadence.
    AmsData* d2 = harness_decode_fixture(F2);
    TEST_ASSERT_FALSE(feed(state, d2));
    TEST_ASSERT_FALSE(state.isCounterStale());
    TEST_ASSERT_TRUE(state.getActiveImportCounter() > c1);
    double c2 = state.getActiveImportCounter();

    // The repeat is rejected, and the state keeps the counter it already had
    // rather than accepting the stale register value as a new hourly reading.
    AmsData* d3 = harness_decode_fixture(F3);
    TEST_ASSERT_TRUE(feed(state, d3));
    TEST_ASSERT_TRUE(state.isCounterStale());
    TEST_ASSERT_TRUE(state.getActiveImportCounter() == c2);

    // The next genuine reading is accepted and clears the flag.
    AmsData* d4 = harness_decode_fixture(F4);
    TEST_ASSERT_FALSE(feed(state, d4));
    TEST_ASSERT_FALSE(state.isCounterStale());
    TEST_ASSERT_FLOAT_WITHIN(0.001, 8.056, (float) (state.getActiveImportCounter() - c2));

    delete d1; delete d2; delete d3; delete d4;
}

// A stale reading must not reset the estimated flag: the realtime estimate is
// what the firmware substitutes for the lost hour, and the MQTT handlers use the
// flag to withhold the repeated total.
void test_stale_reading_keeps_estimate_flagged(void) {
    AmsData state;
    AmsData* d1 = harness_decode_fixture(F1);
    AmsData* d2 = harness_decode_fixture(F2);
    feed(state, d1);
    feed(state, d2);
    double accepted = state.getActiveImportCounter();

    // A sub-hour packet integrates power into the counter and flags it estimated,
    // the way List 1/2 traffic does between whole hours.
    FakeList2 sub(state.getLastUpdateMillis() + 600000, 6000); // 10 min at 6 kW
    state.apply(sub);
    TEST_ASSERT_TRUE(state.isCounterEstimated());
    double estimated = state.getActiveImportCounter();
    TEST_ASSERT_TRUE(estimated > accepted);

    // The stale whole-hour reading must leave both the estimate and the flag be.
    AmsData* d3 = harness_decode_fixture(F3);
    TEST_ASSERT_TRUE(feed(state, d3));
    TEST_ASSERT_TRUE(state.isCounterEstimated());
    TEST_ASSERT_TRUE(state.getActiveImportCounter() == estimated);

    // A genuine reading clears the flag and snaps back to the meter's register.
    AmsData* d4 = harness_decode_fixture(F4);
    TEST_ASSERT_FALSE(feed(state, d4));
    TEST_ASSERT_FALSE(state.isCounterEstimated());
    TEST_ASSERT_TRUE(state.getActiveImportCounter() == d4->getActiveImportCounter());

    delete d1; delete d2; delete d3; delete d4;
}

// A meter that repeats its clock forever must not have its history suppressed
// indefinitely: after two consecutive rejections the reading is accepted.
void test_stale_detector_gives_up_after_two(void) {
    AmsData state;
    AmsData* d1 = harness_decode_fixture(F1);
    AmsData* d2 = harness_decode_fixture(F2);
    feed(state, d1);
    feed(state, d2);

    AmsData* a = harness_decode_fixture(F3);
    AmsData* b = harness_decode_fixture(F3);
    AmsData* c = harness_decode_fixture(F3);
    TEST_ASSERT_TRUE(feed(state, a));   // 1st repeat rejected
    TEST_ASSERT_TRUE(feed(state, b));   // 2nd repeat rejected
    TEST_ASSERT_FALSE(feed(state, c));  // give up, accept it
    TEST_ASSERT_FALSE(state.isCounterStale());

    delete d1; delete d2; delete a; delete b; delete c;
}

// Without an observed hourly cadence the detector stays out of the way, so
// meters that publish their registers continuously are unaffected.
void test_stale_detector_needs_hourly_cadence(void) {
    AmsData state;
    AmsData* d1 = harness_decode_fixture(F1);
    AmsData* d1again = harness_decode_fixture(F1);
    TEST_ASSERT_FALSE(feed(state, d1));
    // Same clock, same registers, but only one accepted reading so far: no
    // cadence has been learned, so this is not treated as the #1119 defect.
    TEST_ASSERT_FALSE(feed(state, d1again));
    delete d1; delete d1again;
}

// The MQTT handlers build a snapshot with apply(previousState) followed by
// apply(update). If the state kept the stale flag for the rest of the hour, that
// snapshot would inherit it and withhold counters that are perfectly good - and,
// worse, the skip branch would leave the snapshot's counters at zero. Sub-hour
// traffic must clear the flag.
void test_stale_flag_clears_on_subhour_packet(void) {
    AmsData state;
    AmsData* d1 = harness_decode_fixture(F1);
    AmsData* d2 = harness_decode_fixture(F2);
    feed(state, d1);
    feed(state, d2);

    AmsData* d3 = harness_decode_fixture(F3);
    TEST_ASSERT_TRUE(feed(state, d3));
    TEST_ASSERT_TRUE(state.isCounterStale());

    // The next sub-hour packet clears it.
    FakeList2 sub(state.getLastUpdateMillis() + 10000, 1500);
    state.apply(sub);
    TEST_ASSERT_FALSE(state.isCounterStale());

    // So a composed snapshot carries the last known counters rather than zero.
    AmsData snapshot;
    snapshot.apply(state);
    TEST_ASSERT_TRUE(snapshot.getActiveImportCounter() > 0);
    TEST_ASSERT_TRUE(snapshot.getActiveImportCounter() == state.getActiveImportCounter());

    delete d1; delete d2; delete d3;
}
