/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 * Decoder unit tests — run on native with: pio test -e native
 */

#include <unity.h>
#include "HdlcParser.h"
#include "DataParser.h"
#include "AmsData.h"
#include "decoder_harness.h"

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Low-level parser guards
// ---------------------------------------------------------------------------

void test_hdlc_rejects_non_hdlc_buffer(void) {
    // Format nibble is not 0xA0 (frame format type 3) -> unknown data.
    HDLCParser parser;
    uint8_t buf[4] = {0x00, 0x00, 0x00, 0x00};
    DataParserContext ctx = {DATA_TAG_HDLC, 4, 0, {}};
    int8_t ret = parser.parse(buf, ctx);
    TEST_ASSERT_EQUAL(DATA_PARSE_UNKNOWN_DATA, ret);
}

void test_hdlc_rejects_short_buffer(void) {
    HDLCParser parser;
    uint8_t buf[2] = {0x7E, 0xA0};
    DataParserContext ctx = {DATA_TAG_HDLC, 2, 0, {}};
    int8_t ret = parser.parse(buf, ctx);
    TEST_ASSERT_EQUAL(DATA_PARSE_INCOMPLETE, ret);
}

// ---------------------------------------------------------------------------
// Smoke test: one unencrypted Iskra AM550 (Slovenia) frame decodes to a list
// ---------------------------------------------------------------------------

void test_decode_iskra_gh956(void) {
    AmsData* d = harness_decode_fixture("test/payloads/iskraemeco/gh956-1.hex");
    TEST_ASSERT_NOT_NULL(d);
    printf("gh956-1: listType=%d meterType=%d P+=%u L1V=%.1f id=%s\n",
           d->getListType(), d->getMeterType(), d->getActiveImportPower(),
           d->getL1Voltage(), d->getMeterId().c_str());
    TEST_ASSERT_GREATER_OR_EQUAL(1, d->getListType());
    delete d;
}

// defined in test_unencrypted.cpp
void harness_emit_golden(void);
void test_unencrypted_golden(void);
void test_iskra_am550_slovenia(void);
void test_aidon_norway_list2(void);
void test_kamstrup_norway(void);
void test_dsmr_accepts_lf_and_crlf(void);
// defined in test_encrypted.cpp
void test_encrypted_decode(void);
void test_encrypted_iskra_787(void);
void test_encrypted_landisgyr_501(void);
void test_encrypted_kaifa_905(void);
void test_encrypted_kamstrup_73(void);

int main(int argc, char** argv) {
    if (argc > 1 && strcmp(argv[1], "gen") == 0) {
        harness_emit_golden();   // regenerate test/test_decoder/expected_unencrypted.h
        return 0;
    }
    UNITY_BEGIN();
    RUN_TEST(test_hdlc_rejects_non_hdlc_buffer);
    RUN_TEST(test_hdlc_rejects_short_buffer);
    RUN_TEST(test_decode_iskra_gh956);
    RUN_TEST(test_iskra_am550_slovenia);
    RUN_TEST(test_aidon_norway_list2);
    RUN_TEST(test_kamstrup_norway);
    RUN_TEST(test_dsmr_accepts_lf_and_crlf);
    RUN_TEST(test_unencrypted_golden);
    RUN_TEST(test_encrypted_decode);
    RUN_TEST(test_encrypted_iskra_787);
    RUN_TEST(test_encrypted_landisgyr_501);
    RUN_TEST(test_encrypted_kaifa_905);
    RUN_TEST(test_encrypted_kamstrup_73);
    return UNITY_END();
}
