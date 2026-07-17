/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 * Regression for the "POW-P1 rebooter og har max oppetid på 10 sek" report
 * (Kamstrup OMNIA / KAM5, Denmark, June 2026).
 *
 * The meter sends a *plaintext* DSMR/P1 telegram, but the user had an
 * encryption key configured. With a key set, DSMRParser routes the telegram
 * to GCMParser, which read the first cleartext byte ('0' == 0x30 == 48) as the
 * system-title length and memcpy'd 48 bytes into the 8-byte ctx.system_title
 * and 12-byte initialization_vector — a stack buffer overflow that rebooted the
 * device in a loop (reboot reason "Software reset (3/0)").
 *
 * Expected safe behaviour: the GCM layer rejects the frame cleanly (no
 * overflow), unwrap fails, and the decode returns NULL. With AddressSanitizer
 * on the native env, the pre-fix overflow aborts the test instead of passing.
 */
#include <unity.h>
#include <string.h>
#include "AmsData.h"
#include "decoder_harness.h"

// The exact cleartext KAM5 telegram from the report. Header line "/KAM5",
// blank line, then OBIS body starting with the digit '0' — that leading byte
// is what GCMParser misread as a 48-byte system title.
static const char KAM5_PLAINTEXT[] =
    "/KAM5\r\n"
    "\r\n"
    "0-0:1.0.0(260611195950S)\r\n"
    "1-0:1.8.0(00000215.616*kWh)\r\n"
    "1-0:2.8.0(00000000.000*kWh)\r\n"
    "1-0:3.8.0(00000000.145*kVArh)\r\n"
    "1-0:4.8.0(00000059.680*kVArh)\r\n"
    "1-0:1.7.0(0000.702*kW)\r\n"
    "1-0:2.7.0(0000.000*kW)\r\n"
    "1-0:32.7.0(228.1*V)\r\n"
    "1-0:52.7.0(228.3*V)\r\n"
    "1-0:72.7.0(229.1*V)\r\n"
    "!A906\r\n";

void test_plaintext_dsmr_with_key_does_not_overflow(void) {
    // A non-zero encryption key makes the firmware build a GCMParser and treat
    // the DSMR telegram as encrypted — the exact precondition for the crash.
    uint8_t key[16];
    memset(key, 0xAB, sizeof(key));

    static uint8_t buf[1024];
    uint16_t len = (uint16_t)(sizeof(KAM5_PLAINTEXT) - 1);
    memcpy(buf, KAM5_PLAINTEXT, len);

    MeterConfig cfg;
    memset(&cfg, 0, sizeof(cfg));

    // Must not overflow/crash. A plaintext frame can't be decrypted, so the
    // safe outcome is a clean rejection (NULL), not a reboot.
    AmsData* d = harness_decode(buf, len, &cfg, key, NULL);
    TEST_ASSERT_NULL_MESSAGE(d, "plaintext DSMR with key set should be rejected, not decoded");
}
