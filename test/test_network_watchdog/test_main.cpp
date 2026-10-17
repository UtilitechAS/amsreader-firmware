/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 */

#include <unity.h>

#include "NetworkWatchdog.h"

#define MINUTE 60000ULL

static ResetDataContainer rdc;

void setUp(void) {
    memset(&rdc, 0, sizeof(rdc));
}

void tearDown(void) {}

static void probe(NetworkWatchdog& wd, uint64_t now, bool replied) {
    wd.probeStarted(now);
    wd.probeFinished(now, replied);
}

// A gateway that never answers ping must never lead to restarts or reboots
static void test_not_armed_until_gateway_answers(void) {
    NetworkWatchdog wd(&rdc);
    for(uint64_t now = 0; now < 5 * 60 * MINUTE; now += MINUTE) {
        probe(wd, now, false);
        TEST_ASSERT_EQUAL(NETWORK_WATCHDOG_ACTION_NONE, wd.check(now));
    }
    TEST_ASSERT_FALSE(wd.isArmed());
    TEST_ASSERT_EQUAL(0, rdc.wdRestarts);
}

static void test_probe_interval(void) {
    NetworkWatchdog wd(&rdc);
    TEST_ASSERT_TRUE(wd.isProbeDue(1000));
    wd.probeStarted(1000);
    TEST_ASSERT_FALSE(wd.isProbeDue(1000 + MINUTE - 1));
    TEST_ASSERT_TRUE(wd.isProbeDue(1000 + MINUTE));
}

static void test_healthy_network_takes_no_action(void) {
    NetworkWatchdog wd(&rdc);
    for(uint64_t now = 1000; now < 24 * 60 * MINUTE; now += MINUTE) {
        probe(wd, now, true);
        TEST_ASSERT_EQUAL(NETWORK_WATCHDOG_ACTION_NONE, wd.check(now));
    }
    TEST_ASSERT_EQUAL(0, rdc.wdRestarts);
    TEST_ASSERT_EQUAL(0, rdc.wdLongestOutage);
}

// Restart WiFi at 3, 6, 12, 24 and 48 minutes without a reply, reboot at 60
static void test_escalation_schedule(void) {
    NetworkWatchdog wd(&rdc);
    uint64_t t0 = 1000;
    probe(wd, t0, true);

    uint64_t restarts[] = { 3, 6, 12, 24, 48 };
    uint8_t next = 0;
    for(uint64_t now = t0 + 1000; now < t0 + 60 * MINUTE; now += 1000) {
        uint8_t action = wd.check(now);
        if(action == NETWORK_WATCHDOG_ACTION_RESTART_WIFI) {
            TEST_ASSERT_TRUE_MESSAGE(next < 5, "Too many restarts");
            TEST_ASSERT_EQUAL_UINT64(t0 + restarts[next] * MINUTE, now);
            next++;
        } else {
            TEST_ASSERT_EQUAL(NETWORK_WATCHDOG_ACTION_NONE, action);
        }
    }
    TEST_ASSERT_EQUAL(5, next);
    TEST_ASSERT_EQUAL(5, rdc.wdRestarts);
    TEST_ASSERT_EQUAL(NETWORK_WATCHDOG_ACTION_REBOOT, wd.check(t0 + 60 * MINUTE));
    TEST_ASSERT_EQUAL(3600, rdc.wdLongestOutage);
}

static void test_reply_resets_escalation(void) {
    NetworkWatchdog wd(&rdc);
    probe(wd, 0, true);
    TEST_ASSERT_EQUAL(NETWORK_WATCHDOG_ACTION_RESTART_WIFI, wd.check(3 * MINUTE));

    // Gateway answers again after the restart, and goes quiet once more
    probe(wd, 4 * MINUTE, true);
    TEST_ASSERT_EQUAL(240, rdc.wdLongestOutage);
    TEST_ASSERT_EQUAL(NETWORK_WATCHDOG_ACTION_NONE, wd.check(4 * MINUTE + 3 * MINUTE - 1));
    TEST_ASSERT_EQUAL(NETWORK_WATCHDOG_ACTION_RESTART_WIFI, wd.check(4 * MINUTE + 3 * MINUTE));
    TEST_ASSERT_EQUAL(2, rdc.wdRestarts);
}

static void test_failed_probes_counted_and_cleared(void) {
    NetworkWatchdog wd(&rdc);
    probe(wd, 0, true);
    probe(wd, MINUTE, false);
    probe(wd, 2 * MINUTE, false);
    TEST_ASSERT_EQUAL(2, wd.getFailedProbes());
    probe(wd, 3 * MINUTE - 1, true);
    TEST_ASSERT_EQUAL(0, wd.getFailedProbes());
    TEST_ASSERT_EQUAL(179, rdc.wdLongestOutage);
}

// The regular gap between two successful probes is not an outage
static void test_normal_probe_gap_not_recorded(void) {
    NetworkWatchdog wd(&rdc);
    probe(wd, 0, true);
    probe(wd, 2 * MINUTE, true);
    TEST_ASSERT_EQUAL(0, rdc.wdLongestOutage);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_not_armed_until_gateway_answers);
    RUN_TEST(test_probe_interval);
    RUN_TEST(test_healthy_network_takes_no_action);
    RUN_TEST(test_escalation_schedule);
    RUN_TEST(test_reply_resets_escalation);
    RUN_TEST(test_failed_probes_counted_and_cleared);
    RUN_TEST(test_normal_probe_gap_not_recorded);
    return UNITY_END();
}
