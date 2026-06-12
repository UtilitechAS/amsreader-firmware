/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 * Test-only harness that drives the decoder the same way the firmware does.
 *
 * It mirrors the unwrap loop in PassiveMeterCommunicator::unwrapData() and the
 * dispatch in PassiveMeterCommunicator::getData() — those live in an
 * Arduino-only translation unit, so the logic is replicated here for native
 * tests. Keep this in sync with PassiveMeterCommunicator if that loop changes.
 */
#ifndef _DECODER_HARNESS_H
#define _DECODER_HARNESS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DataParser.h"
#include "AmsConfiguration.h"

// ----- a no-op Stream so the decoder's debugger->printf_P calls are safe -----
// (the native Print/Stream stub writes to stdout; we silence it to keep test
//  output readable — see DECODER_HARNESS_VERBOSE to re-enable.)
#include "DebugPrint.h"
class NullStream : public Stream {
public:
    int available() override { return 0; }
    int read() override { return -1; }
    int peek() override { return -1; }
};

// Load a fixture file into out (max cap bytes). .hex files are parsed as hex
// (whitespace ignored); .txt files are read verbatim. Returns byte count, or
// -1 if the file can't be opened.
int harness_load_fixture(const char* path, uint8_t* out, int cap);

// Decode a raw frame buffer the way the firmware does and return a heap AmsData
// (caller frees). enc_key/auth_key may be NULL for unencrypted frames.
// Returns NULL if the frame could not be unwrapped to a payload.
class AmsData;
AmsData* harness_decode(uint8_t* buf, uint16_t len, MeterConfig* cfg,
                        const uint8_t* enc_key, const uint8_t* auth_key);

// Convenience: load + decode a fixture (unencrypted). Returns NULL on failure.
AmsData* harness_decode_fixture(const char* path);

#endif
