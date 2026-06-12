/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 */
#include "decoder_harness.h"

#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

#include "HdlcParser.h"
#include "MbusParser.h"
#include "LlcParser.h"
#include "GbtParser.h"
#include "DlmsParser.h"
#include "DsmrParser.h"
#include "GcmParser.h"
#include "Cosem.h"
#include "Timezone.h"
#include "AmsData.h"
#include "IEC6205675.h"
#include "IEC6205621.h"
#include "LNG.h"
#include "LNG2.h"
#include "Uptime.h"
#include <chrono>

// millis64() lives in Uptime.cpp (Arduino-only). Provide a native definition
// so the decoder links; tests don't assert on it.
uint64_t millis64() {
    using namespace std::chrono;
    return (uint64_t)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

int harness_load_fixture(const char* path, uint8_t* out, int cap) {
    FILE* f = fopen(path, "rb");
    if (!f) return -1;
    int n = 0;
    bool txt = false;
    size_t plen = strlen(path);
    if (plen > 4 && strcmp(path + plen - 4, ".txt") == 0) txt = true;

    if (txt) {
        // DSMR/P1 telegram — read verbatim
        int c;
        while ((c = fgetc(f)) != EOF && n < cap) out[n++] = (uint8_t)c;
    } else {
        // hex dump — collect hex nibbles, ignore everything else
        int hi = -1, c;
        while ((c = fgetc(f)) != EOF && n < cap) {
            int v;
            if (c >= '0' && c <= '9') v = c - '0';
            else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') v = c - 'A' + 10;
            else continue;
            if (hi < 0) { hi = v; }
            else { out[n++] = (uint8_t)((hi << 4) | v); hi = -1; }
        }
    }
    fclose(f);
    return n;
}

// fd-level stdout suppression around the (chatty) decode
#ifndef DECODER_HARNESS_VERBOSE
static int s_saved = -1;
static void mute_stdout() {
    fflush(stdout);
    s_saved = dup(STDOUT_FILENO);
    int dn = open("/dev/null", O_WRONLY);
    dup2(dn, STDOUT_FILENO);
    close(dn);
}
static void unmute_stdout() {
    fflush(stdout);
    if (s_saved >= 0) { dup2(s_saved, STDOUT_FILENO); close(s_saved); s_saved = -1; }
}
#else
static void mute_stdout() {}
static void unmute_stdout() {}
#endif

// Mirror of PassiveMeterCommunicator::unwrapData (sans debug/MQTT). Returns the
// offset to the payload start (>=0) with ctx.type/length set, or <0 on failure.
static int16_t unwrap(uint8_t* buf, DataParserContext& ctx, MeterConfig* cfg,
                      const uint8_t* enc_key, const uint8_t* auth_key,
                      bool* reachedGcm = NULL, bool stopAfterGcm = false) {
    HDLCParser hdlc;
    MBUSParser mbus;
    GBTParser gbt;
    LLCParser llc;
    DLMSParser dlms;
    GCMParser* gcm = NULL;
    DSMRParser* dsmr = NULL;
    if (enc_key) {
        uint8_t ek[16], ak[16];
        memcpy(ek, enc_key, 16);
        memset(ak, 0, 16);
        if (auth_key) memcpy(ak, auth_key, 16);
        gcm = new GCMParser(ek, ak);
    }

    static NullStream dbg;
    int16_t ret = 0;
    int16_t end = (int16_t)ctx.length;
    bool doRet = false;
    uint8_t tag = *buf;
    uint8_t lastTag = DATA_TAG_NONE;

    uint8_t* hdlcStart = NULL;   // start of the current HDLC frame (for GBT reassembly)
    int16_t hdlcLen = 0;
    while (tag != DATA_TAG_NONE) {
        int8_t res = 0;
        switch (tag) {
            case DATA_TAG_HDLC:
                hdlcStart = buf;
                hdlcLen = (int16_t)((((buf[1] << 8) | buf[2]) & 0x7FF) + 2);
                res = hdlc.parse(buf, ctx); if (ctx.length < 3) doRet = true; break;
            case DATA_TAG_MBUS: res = mbus.parse(buf, ctx); break;
            case DATA_TAG_GBT:  res = gbt.parse(buf, ctx); break;
            case DATA_TAG_LLC:  res = llc.parse(buf, ctx); break;
            case DATA_TAG_GCM:
                if (reachedGcm) *reachedGcm = true;
                if (!gcm) { delete dsmr; return DATA_PARSE_UNKNOWN_DATA; }
                res = gcm->parse(buf, ctx);
                // Probe mode: the GCM header (incl. system title) is now parsed;
                // stop before decoding the plaintext (which, with a dummy key, is
                // garbage and not worth parsing).
                if (stopAfterGcm) { delete gcm; delete dsmr; return res; }
                break;
            case DATA_TAG_DLMS: res = dlms.parse(buf, ctx); if (res >= 0) doRet = true; break;
            case DATA_TAG_DSMR:
                if (!dsmr) dsmr = new DSMRParser(gcm);
                res = dsmr->parse(buf, ctx, lastTag != DATA_TAG_NONE, &dbg);
                if (res >= 0) doRet = true;
                break;
            case DATA_TAG_SNRM:
            case DATA_TAG_AARE:
            case DATA_TAG_RES:
                res = DATA_PARSE_OK; doRet = true; break;
            default:
                delete gcm; delete dsmr;
                return DATA_PARSE_UNKNOWN_DATA;
        }
        lastTag = tag;
        if (res == DATA_PARSE_INCOMPLETE) { delete gcm; delete dsmr; return res; }

        // Multi-segment M-Bus: the parser accumulates each frame internally.
        // In the firmware each frame arrives in its own read; here the whole
        // capture is one buffer, so advance past each consumed M-Bus frame
        // ourselves (68 LL LL 68 + LL user bytes + checksum + 0x16 = LL + 6).
        if (tag == DATA_TAG_MBUS && res == DATA_PARSE_INTERMEDIATE_SEGMENT) {
            uint16_t consumed = buf[1] + 6;
            buf += consumed; end -= consumed; ret += consumed;
            tag = *buf;
            continue;
        }
        if (tag == DATA_TAG_MBUS && res == DATA_PARSE_FINAL_SEGMENT) {
            mbus.write(buf, ctx);     // assembles into buf, sets ctx.length
            end = (int16_t)ctx.length;
            tag = *buf;               // continue parsing the assembled payload
            continue;
        }

        // Multi-frame GBT (General Block Transfer): each block sits inside its
        // own HDLC frame and the parser accumulates them. Jump to the next HDLC
        // frame (firmware gets each in a separate read); the final block returns
        // OK with the reassembled DLMS in place.
        if (tag == DATA_TAG_GBT && res == DATA_PARSE_INTERMEDIATE_SEGMENT) {
            if (!hdlcStart) { delete gcm; delete dsmr; return DATA_PARSE_FAIL; }
            int16_t delta = (int16_t)((hdlcStart + hdlcLen) - buf);
            if (delta <= 0 || delta >= end) { delete gcm; delete dsmr; return DATA_PARSE_INCOMPLETE; }
            buf += delta; end -= delta; ret += delta;
            ctx.length = (uint16_t)end;   // next frame's per-read budget (mirrors a fresh serial read)
            tag = *buf;
            continue;
        }
        if (tag == DATA_TAG_GBT && res == DATA_PARSE_OK) {
            end = (int16_t)ctx.length;   // reassembled DLMS now at buf
            tag = *buf;
            continue;
        }

        if ((int16_t)ctx.length > end) { delete gcm; delete dsmr; return DATA_PARSE_FAIL; }
        if (res < 0) { delete gcm; delete dsmr; return res; }
        buf += res; end -= res; ret += res;
        if (doRet) { ctx.type = tag; delete gcm; delete dsmr; return ret; }
        tag = *buf;
    }
    delete gcm; delete dsmr;
    return DATA_PARSE_UNKNOWN_DATA;
}

AmsData* harness_decode(uint8_t* buf, uint16_t len, MeterConfig* cfg,
                        const uint8_t* enc_key, const uint8_t* auth_key) {
    DataParserContext ctx;
    ctx.type = *buf;
    ctx.length = len;
    ctx.timestamp = 0;
    memset(ctx.system_title, 0, sizeof(ctx.system_title));

    mute_stdout();
    int16_t pos = unwrap(buf, ctx, cfg, enc_key, auth_key);
    if (pos < 0) { unmute_stdout(); return NULL; }

    char* payload = ((char*)buf) + pos;
    static Timezone tz;
    static NullStream dbg;
    AmsData state;
    AmsData* data = NULL;

    if (ctx.type == DATA_TAG_DLMS) {
        if (payload[0] == CosemTypeStructure && payload[2] == CosemTypeArray &&
            payload[1] == payload[3]) {
            LNG lng(state, payload, state.getMeterType(), cfg, ctx);
            if (lng.getListType() >= 1) { data = new AmsData(); data->apply(state); data->apply(lng); }
        } else if (payload[0] == CosemTypeStructure &&
                   payload[2] == CosemTypeLongUnsigned && payload[5] == CosemTypeLongUnsigned &&
                   payload[8] == CosemTypeLongUnsigned && payload[11] == CosemTypeLongUnsigned &&
                   payload[14] == CosemTypeLongUnsigned && payload[17] == CosemTypeLongUnsigned) {
            LNG2 lng(state, payload, state.getMeterType(), cfg, ctx);
            if (lng.getListType() >= 1) { data = new AmsData(); data->apply(state); data->apply(lng); }
        } else {
            data = new IEC6205675(payload, &tz, state.getMeterType(), cfg, ctx, state, &dbg);
        }
    } else if (ctx.type == DATA_TAG_DSMR) {
        data = new IEC6205621(payload, &tz, cfg);
    }
    unmute_stdout();
    return data;
}

AmsData* harness_decode_fixture(const char* path) {
    static uint8_t buf[4096];
    int n = harness_load_fixture(path, buf, sizeof(buf));
    if (n <= 0) return NULL;
    MeterConfig cfg;
    memset(&cfg, 0, sizeof(cfg));   // multipliers 0 == x1 (matches clearMeterConfig)
    return harness_decode(buf, (uint16_t)n, &cfg, NULL, NULL);
}

void harness_probe_fixture(const char* path, HarnessProbe* out) {
    out->reached_gcm = false;
    memset(out->system_title, 0, sizeof(out->system_title));
    out->unwrap_result = DATA_PARSE_FAIL;

    static uint8_t buf[4096];
    int n = harness_load_fixture(path, buf, sizeof(buf));
    if (n <= 0) return;

    // Dummy key: the GCM header (incl. system title) is parsed before any
    // decryption, so framing/header coverage works without the real key.
    uint8_t zero[16];
    memset(zero, 0, sizeof(zero));
    DataParserContext ctx;
    ctx.type = buf[0];
    ctx.length = (uint16_t)n;
    ctx.timestamp = 0;
    memset(ctx.system_title, 0, sizeof(ctx.system_title));

    bool reached = false;
    mute_stdout();
    out->unwrap_result = unwrap(buf, ctx, NULL, zero, zero, &reached, /*stopAfterGcm=*/true);
    unmute_stdout();
    out->reached_gcm = reached;
    memcpy(out->system_title, ctx.system_title, sizeof(out->system_title));
}
