/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 * End-to-end decode tests for encrypted fixtures we hold a key for.
 *
 * Requires native mbedTLS (HAVE_MBEDTLS, set by scripts/native_crypto.py when
 * libmbedtls-dev is present). Without it the GcmParser native branch can't
 * decrypt and these tests self-ignore.
 *
 * Keys are resolved per fixture from fixtures_generated.h (ENC_KEYED), by the
 * secret name: first from the environment (GitHub Actions secrets in CI), then
 * from the gitignored test/payloads/keys/keys.local.json for local runs.
 */
#include <unity.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "AmsData.h"
#include "decoder_harness.h"
#include "fixtures_generated.h"

#define COUNT(a) (sizeof(a) / sizeof((a)[0]))

static int hex16(const char* hex, uint8_t out[16]) {
    int n = 0;
    for (int i = 0; i < 16; i++) {
        unsigned v;
        if (sscanf(hex + i * 2, "%2x", &v) != 1) return 0;
        out[i] = (uint8_t)v;
        n++;
    }
    return n == 16;
}

// Look up a key by secret name: environment first, then keys.local.json.
static bool load_key(const char* secret, uint8_t out[16]) {
    if (!secret) return false;
    const char* env = getenv(secret);
    if (env && strlen(env) >= 32) return hex16(env, out);

    FILE* f = fopen("test/payloads/keys/keys.local.json", "rb");
    if (!f) return false;
    static char buf[8192];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';
    fclose(f);
    char needle[96];
    snprintf(needle, sizeof(needle), "\"%s\"", secret);
    char* p = strstr(buf, needle);
    if (!p) return false;
    p = strchr(p + strlen(needle), ':');
    if (!p) return false;
    p = strchr(p, '"');
    if (!p) return false;
    return hex16(p + 1, out);
}

void test_encrypted_decode(void) {
#if !defined(HAVE_MBEDTLS)
    TEST_IGNORE_MESSAGE("native mbedTLS not available (install libmbedtls-dev) — skipping encrypted decode");
#else
    int tested = 0, failures = 0, no_key = 0;
    printf("\n--- ENC_KEYED (%zu) ---\n", COUNT(ENC_KEYED));
    for (size_t i = 0; i < COUNT(ENC_KEYED); i++) {
        const KeyedFixture& k = ENC_KEYED[i];
        uint8_t ek[16], ak[16];
        if (!load_key(k.ek_secret, ek)) { printf("  no key %-22s %s\n", k.ek_secret, k.path); no_key++; continue; }
        bool haveAk = k.ak_secret && load_key(k.ak_secret, ak);

        static uint8_t buf[4096];
        int len = harness_load_fixture(k.path, buf, sizeof(buf));
        if (len <= 0) { printf("  load FAIL %s\n", k.path); failures++; continue; }
        MeterConfig cfg; memset(&cfg, 0, sizeof(cfg));
        AmsData* d = harness_decode(buf, (uint16_t)len, &cfg, ek, haveAk ? ak : NULL);
        tested++;
        if (!d) { printf("  DECRYPT/DECODE FAIL %s\n", k.path); failures++; continue; }
        printf("  list=%d type=%-2d P+=%-6u L1V=%-6.1f id=%s  %s\n",
               d->getListType(), d->getMeterType(), d->getActiveImportPower(),
               d->getL1Voltage(), d->getMeterId().c_str(), k.path);
        if (d->getListType() < 1) failures++;
        delete d;
    }
    if (tested == 0) {
        TEST_IGNORE_MESSAGE("no keys available (set env secrets or provide keys.local.json)");
    } else {
        char msg[96];
        snprintf(msg, sizeof(msg), "%d/%d encrypted fixtures failed (%d had no key)", failures, tested, no_key);
        TEST_ASSERT_EQUAL_MESSAGE(0, failures, msg);
    }
#endif
}

// Decrypt a fixture with the named key(s); IGNORE the test if mbedTLS or the
// key is unavailable. Returns NULL only on a genuine decrypt/decode failure.
static AmsData* decrypt_or_ignore(const char* path, const char* ekName, const char* akName) {
#if !defined(HAVE_MBEDTLS)
    (void)path; (void)ekName; (void)akName;
    TEST_IGNORE_MESSAGE("native mbedTLS not available (install libmbedtls-dev)");
    return NULL;
#else
    uint8_t ek[16], ak[16];
    if (!load_key(ekName, ek)) { TEST_IGNORE_MESSAGE("encryption key not available"); return NULL; }
    bool haveAk = akName && load_key(akName, ak);
    static uint8_t buf[4096];
    int len = harness_load_fixture(path, buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, len);
    MeterConfig cfg; memset(&cfg, 0, sizeof(cfg));
    return harness_decode(buf, (uint16_t)len, &cfg, ek, haveAk ? ak : NULL);
#endif
}

// #787 Iskra AM550 (Austria) — EK only, security byte 0x20 (encrypt-only).
void test_encrypted_iskra_787(void) {
    AmsData* d = decrypt_or_ignore("test/payloads/iskraemeco/gh787-1.hex", "AMS_TEST_KEY_GH787_EK", NULL);
    TEST_ASSERT_NOT_NULL_MESSAGE(d, "gh787-1 failed to decrypt/decode");
    TEST_ASSERT_EQUAL(AmsTypeIskra, d->getMeterType());
    TEST_ASSERT_GREATER_OR_EQUAL(1, d->getListType());
    delete d;
}

// #73 Kamstrup Omnipower (Denmark) — the project's first encrypted meter.
// The reporter's emailed EK+AK decrypt + authenticate (sec 0x30) to a valid
// Kamstrup list (data-notification dated 2020-05-12).
void test_encrypted_kamstrup_73(void) {
    AmsData* d = decrypt_or_ignore("test/payloads/kamstrup/gh73-1.hex",
                                   "AMS_TEST_KEY_EM20200710_EK", "AMS_TEST_KEY_EM20200710_AK");
    TEST_ASSERT_NOT_NULL_MESSAGE(d, "gh73-1 failed to decrypt/authenticate/decode");
    TEST_ASSERT_EQUAL(AmsTypeKamstrup, d->getMeterType());
    TEST_ASSERT_GREATER_OR_EQUAL(1, d->getListType());
    TEST_ASSERT_FLOAT_WITHIN(10.0, 228.0, d->getL1Voltage());
    delete d;
}

// #501 Landis+Gyr E450 (Austria, Netz Burgenland) — EK + AK, proprietary
// non-OBIS LNG format. Decrypts to meter id 30137181 (per the issue breakdown).
void test_encrypted_landisgyr_501(void) {
    AmsData* d = decrypt_or_ignore("test/payloads/landis-gyr/gh501-1.hex",
                                   "AMS_TEST_KEY_GH501_EK", "AMS_TEST_KEY_GH501_AK");
    TEST_ASSERT_NOT_NULL_MESSAGE(d, "gh501-1 failed to decrypt/decode");
    TEST_ASSERT_EQUAL(AmsTypeLandisGyr, d->getMeterType());
    TEST_ASSERT_GREATER_OR_EQUAL(1, d->getListType());
    TEST_ASSERT_EQUAL_STRING("30137181", d->getMeterId().c_str());
    delete d;
}

// #905 Kaifa MA309 (Poland, Stoen) — EK only, M-Bus wrapped (multi-segment).
// Decrypts to serial 1KFM0200169986.
void test_encrypted_kaifa_905(void) {
    AmsData* d = decrypt_or_ignore("test/payloads/kaifa/gh905-1.hex", "AMS_TEST_KEY_GH905_EK", NULL);
    TEST_ASSERT_NOT_NULL_MESSAGE(d, "gh905-1 failed to decrypt/decode");
    TEST_ASSERT_EQUAL(AmsTypeKaifa, d->getMeterType());
    TEST_ASSERT_GREATER_OR_EQUAL(1, d->getListType());
    TEST_ASSERT_EQUAL_STRING("1KFM0200169986", d->getMeterId().c_str());
    delete d;
}



// Framing / GCM-header coverage for encrypted frames we have no key for.
// Does not (cannot) test decryption — instead it guards that the transport
// framing and GCM-header parse never crash on real-world encrypted bytes, and
// that every frame which reaches the GCM layer yields a system title. (This is
// what surfaced — and now guards against — the GcmParser ciphertext-length
// underflow.) Runs without mbedTLS: the system title is read before decryption.
void test_encrypted_framing_no_key(void) {
    size_t n = sizeof(ENC_NOKEY) / sizeof(ENC_NOKEY[0]);
    int reached = 0, missing_title = 0;
    for (size_t i = 0; i < n; i++) {
        HarnessProbe p;
        harness_probe_fixture(ENC_NOKEY[i].path, &p);   // must not crash
        if (!p.reached_gcm) continue;
        reached++;
        bool nonzero = false;
        for (int k = 0; k < 8; k++) nonzero |= (p.system_title[k] != 0);
        if (!nonzero) { printf("  no system title: %s\n", ENC_NOKEY[i].path); missing_title++; }
    }
    printf("encrypted-no-key: %d/%zu reached GCM header\n", reached, n);
    TEST_ASSERT_EQUAL_MESSAGE(0, missing_title, "frame reached GCM but extracted no system title");
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, reached, "no encrypted frame reached the GCM layer");
}
