/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 * Guards the stale whole-hour register detector (#1119).
 *
 * Some meters publish a stale snapshot of their accumulated registers at the
 * whole hour, while the instantaneous values in the same payload are current.
 * Without detection the ending hour is stored as 0 and the next hour is
 * credited with two hours of consumption, which destroys the month's tariff
 * peak. A stale register is recognised by not having moved although the power
 * integrated since the last accepted reading says it should have.
 *
 * gh1119-1..4 are four consecutive real whole-hour payloads from an Aidon meter
 * (2025-12-14 16:00, 17:00, 18:00, 19:00 wall clock). The 18:00 payload is the
 * defect: it repeats the 17:00 clock and the 17:00 registers.
 *
 * test/payloads/aidon/gh1119-history/stale-events.tsv holds every stale reading
 * the same meter produced in a year of logging (43 events), with two readings
 * before and one after each, and the energy integrated from the meter's own
 * power readings over the hour before each reading. In 41 of the 43 events the
 * meter clock is current, so a clock-based detector misses them. See the
 * README.md next to it.
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

// Sub-hour traffic worth kwh of import over one hour, the way List 1/2 packets
// build the integrated estimate between two whole-hour readings.
static void integrateHour(AmsData& state, double kwh) {
    uint32_t watts = (uint32_t) (kwh * 1000.0 + 0.5);
    uint64_t t0 = state.getLastUpdateMillis();
    FakeList2 first(t0 + 1, watts);           // sets the power level, adds ~nothing
    state.apply(first);
    FakeList2 last(t0 + 1 + 3600000, watts);  // one hour at that level
    state.apply(last);
}

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
    integrateHour(state, 1.768);
    AmsData* d2 = harness_decode_fixture(F2);
    TEST_ASSERT_FALSE(feed(state, d2));
    TEST_ASSERT_FALSE(state.isCounterStale());
    TEST_ASSERT_TRUE(state.getActiveImportCounter() > c1);
    double c2 = state.getActiveImportCounter();

    // The repeat is rejected, and the state keeps its integrated estimate rather
    // than accepting the stale register value as a new hourly reading. (The
    // power logged over that hour integrates to 3.957 kWh.)
    integrateHour(state, 3.957);
    double estimate = state.getActiveImportCounter();
    AmsData* d3 = harness_decode_fixture(F3);
    TEST_ASSERT_TRUE(feed(state, d3));
    TEST_ASSERT_TRUE(state.isCounterStale());
    TEST_ASSERT_TRUE(state.getActiveImportCounter() == estimate);
    TEST_ASSERT_TRUE(state.getActiveImportCounter() > c2);

    // The next genuine reading is accepted and clears the flag.
    integrateHour(state, 4.100);
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

    integrateHour(state, 3.957);
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
    // Same registers despite an hour of consumption, but only one accepted
    // reading so far: no cadence has been learned, so this is not treated as
    // the #1119 defect.
    integrateHour(state, 2.0);
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

    integrateHour(state, 3.957);
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

// Without sub-hour traffic there is no evidence to judge the register by, so an
// unmoved register is taken at face value - the meter may simply have been idle.
void test_stale_detector_needs_integrated_evidence(void) {
    AmsData state;
    AmsData* d1 = harness_decode_fixture(F1);
    AmsData* d2 = harness_decode_fixture(F2);
    AmsData* d3 = harness_decode_fixture(F3);
    feed(state, d1);
    feed(state, d2);
    TEST_ASSERT_FALSE(feed(state, d3));
    delete d1; delete d2; delete d3;
}

// An unmoved register over an hour where less than STALE_COUNTER_MIN_ESTIMATE_KWH
// was integrated is plausible (an idle installation, a coarse register), so it
// is accepted.
void test_stale_detector_ignores_small_estimate(void) {
    AmsData state;
    AmsData* d1 = harness_decode_fixture(F1);
    AmsData* d2 = harness_decode_fixture(F2);
    AmsData* d3 = harness_decode_fixture(F3);
    feed(state, d1);
    integrateHour(state, 1.768);
    feed(state, d2);
    integrateHour(state, 0.05);
    TEST_ASSERT_FALSE(feed(state, d3));
    delete d1; delete d2; delete d3;
}

// ---------------------------------------------------------------------------
// Replay of a year of real stale readings (stale-events.tsv)
// ---------------------------------------------------------------------------

#define HISTORY "test/payloads/aidon/gh1119-history/stale-events.tsv"

struct HistoryRow {
    int event;
    bool stale;            // ground truth: register stood still under load
    char logged[20];       // local wall clock of the logger
    double kwhBefore;      // energy integrated from the meter's power over the hour before this reading
    uint8_t frame[1024];
    int frameLen;
};

static int hexval(char c) {
    if(c >= '0' && c <= '9') return c - '0';
    if(c >= 'A' && c <= 'F') return c - 'A' + 10;
    if(c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

// Reads the next data row. Columns: event, role (pre|stale|post|ok), logged
// local time, integrated kWh for the hour before the reading, raw HDLC frame.
static bool readHistoryRow(FILE* f, HistoryRow& row) {
    static char line[4096];
    while(fgets(line, sizeof(line), f)) {
        if(line[0] == '#' || line[0] == '\n') continue;
        char* cols[5];
        int n = 0;
        char* p = line;
        cols[n++] = p;
        while(*p && n < 5) {
            if(*p == '\t') { *p = '\0'; cols[n++] = p + 1; }
            p++;
        }
        if(n < 5) return false;
        row.event = atoi(cols[0]);
        row.stale = strcmp(cols[1], "stale") == 0;
        strncpy(row.logged, cols[2], sizeof(row.logged) - 1);
        row.logged[sizeof(row.logged) - 1] = '\0';
        row.kwhBefore = atof(cols[3]);
        row.frameLen = 0;
        for(char* h = cols[4]; hexval(h[0]) >= 0 && hexval(h[1]) >= 0 && row.frameLen < (int) sizeof(row.frame); h += 2) {
            row.frame[row.frameLen++] = (hexval(h[0]) << 4) | hexval(h[1]);
        }
        return true;
    }
    return false;
}

static AmsData* decodeRow(HistoryRow& row) {
    MeterConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    return harness_decode(row.frame, (uint16_t) row.frameLen, &cfg, NULL, NULL);
}

// Replays the readings of one logged event (or, for the full-year file, the
// whole year) through the pipeline: sub-hour traffic worth the energy the meter
// itself reported over the hour, then the whole-hour reading. Returns the number
// of readings where the detector's verdict differed from the ground truth.
static int replayHistory(const char* path, int* staleSeen, int* rowsSeen) {
    FILE* f = fopen(path, "r");
    TEST_ASSERT_NOT_NULL_MESSAGE(f, path);
    static HistoryRow row;
    AmsData* state = NULL;
    int event = -1, mismatches = 0;
    *staleSeen = 0;
    *rowsSeen = 0;
    while(readHistoryRow(f, row)) {
        if(state == NULL || row.event != event) {
            delete state;
            state = new AmsData();
            event = row.event;
        } else {
            integrateHour(*state, row.kwhBefore);
        }
        AmsData* packet = decodeRow(row);
        TEST_ASSERT_NOT_NULL_MESSAGE(packet, row.logged);
        TEST_ASSERT_EQUAL_MESSAGE(3, packet->getListType(), row.logged);
        bool flagged = feed(*state, packet);
        if(flagged != row.stale) {
            printf("  %s: detector %s, expected %s\n", row.logged, flagged ? "stale" : "ok", row.stale ? "stale" : "ok");
            mismatches++;
        }
        if(row.stale) (*staleSeen)++;
        (*rowsSeen)++;
        delete packet;
    }
    delete state;
    fclose(f);
    return mismatches;
}

// Every stale reading the meter produced in a year is flagged, and none of the
// readings around them. AMS_GH1119_HISTORY can point at a longer capture in the
// same format (e.g. every hourly reading of the year) for a false-positive sweep.
void test_stale_history_replay(void) {
    const char* path = getenv("AMS_GH1119_HISTORY");
    if(path == NULL || path[0] == '\0') path = HISTORY;
    int stale = 0, rows = 0;
    int mismatches = replayHistory(path, &stale, &rows);
    printf("  %s: %d readings, %d stale\n", path, rows, stale);
    TEST_ASSERT_TRUE(stale > 0);
    TEST_ASSERT_EQUAL_INT(0, mismatches);
}

// Finds the reading logged at the given local time and the two before it.
static bool findInHistory(const char* logged, HistoryRow* out3) {
    FILE* f = fopen(HISTORY, "r");
    if(f == NULL) return false;
    static HistoryRow ring[3];
    int n = 0;
    bool found = false;
    while(readHistoryRow(f, ring[n % 3])) {
        if(strncmp(ring[n % 3].logged, logged, strlen(logged)) == 0 && n >= 2) {
            out3[0] = ring[(n - 2) % 3];
            out3[1] = ring[(n - 1) % 3];
            out3[2] = ring[n % 3];
            found = true;
            break;
        }
        n++;
    }
    fclose(f);
    return found;
}

// 2026-11-18 16:00: the signature since the meter's clock format changed in
// January 2026. The register repeats the 15:00 value while the meter clock in
// the same payload is current (16:00). The previous detector required a
// repeated clock and let this through.
void test_stale_detector_flags_register_repeat_with_current_clock(void) {
    static HistoryRow r[3];
    TEST_ASSERT_TRUE(findInHistory("2026-11-18 16:00", r));
    AmsData* p0 = decodeRow(r[0]);
    AmsData* p1 = decodeRow(r[1]);
    AmsData* p2 = decodeRow(r[2]);

    // The defect as it appears in the payload.
    TEST_ASSERT_EQUAL_INT32(3600, (int32_t) (p2->getMeterTimestamp() - p1->getMeterTimestamp()));
    TEST_ASSERT_TRUE(p2->getActiveImportCounter() == p1->getActiveImportCounter());
    TEST_ASSERT_TRUE(r[2].kwhBefore > 3.0);

    AmsData state;
    TEST_ASSERT_FALSE(feed(state, p0));
    integrateHour(state, r[1].kwhBefore);
    TEST_ASSERT_FALSE(feed(state, p1));
    integrateHour(state, r[2].kwhBefore);
    TEST_ASSERT_TRUE(feed(state, p2));
    TEST_ASSERT_TRUE(state.isCounterEstimated());
    TEST_ASSERT_FLOAT_WITHIN(0.01, r[2].kwhBefore, (float) (state.getActiveImportCounter() - p1->getActiveImportCounter()));
    delete p0; delete p1; delete p2;
}

// 2026-02-21 19:00: a partial stale snapshot. Active import stood still through
// an hour of about 10 kWh, while reactive export moved. Judging all registers
// together would let this through.
void test_stale_detector_flags_partial_repeat(void) {
    static HistoryRow r[3];
    TEST_ASSERT_TRUE(findInHistory("2026-02-21 19:00", r));
    AmsData* p0 = decodeRow(r[0]);
    AmsData* p1 = decodeRow(r[1]);
    AmsData* p2 = decodeRow(r[2]);

    TEST_ASSERT_TRUE(p2->getActiveImportCounter() == p1->getActiveImportCounter());
    TEST_ASSERT_TRUE(p2->getReactiveExportCounter() != p1->getReactiveExportCounter());
    TEST_ASSERT_TRUE(r[2].kwhBefore > 9.0);

    AmsData state;
    feed(state, p0);
    integrateHour(state, r[1].kwhBefore);
    TEST_ASSERT_FALSE(feed(state, p1));
    integrateHour(state, r[2].kwhBefore);
    TEST_ASSERT_TRUE(feed(state, p2));
    delete p0; delete p1; delete p2;
}
