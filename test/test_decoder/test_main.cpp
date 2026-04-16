/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 * Decoder unit tests — run on native with: pio test -e native
 *
 * Add real captured meter frames as byte arrays in fixtures/ and assert
 * the parsed AmsData fields here.
 */

#include <unity.h>
#include "HdlcParser.h"
#include "DataParser.h"

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Placeholder: replace with real frame bytes from fixtures/
// ---------------------------------------------------------------------------

void test_hdlc_rejects_empty_buffer(void) {
    HDLCParser parser;
    uint8_t buf[4] = {0x00, 0x00, 0x00, 0x00};
    DataParserContext ctx = {DATA_TAG_HDLC, 4, 0, {}};
    int8_t ret = parser.parse(buf, ctx);
    TEST_ASSERT_EQUAL(DATA_PARSE_BOUNDARY_FLAG_MISSING, ret);
}

void test_hdlc_rejects_short_buffer(void) {
    HDLCParser parser;
    uint8_t buf[2] = {0x7E, 0xA0};
    DataParserContext ctx = {DATA_TAG_HDLC, 2, 0, {}};
    int8_t ret = parser.parse(buf, ctx);
    TEST_ASSERT_EQUAL(DATA_PARSE_INCOMPLETE, ret);
}

// ---------------------------------------------------------------------------

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_hdlc_rejects_empty_buffer);
    RUN_TEST(test_hdlc_rejects_short_buffer);
    return UNITY_END();
}
