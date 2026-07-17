/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 */
#pragma once

#include <string.h>
#include "AmsDataStorage.h"
#include "AmsConfiguration.h"

/**
 * Sink for streamed JSON generation. Lets a generator emit its output in small
 * chunks without ever materializing the whole document, so output size is
 * decoupled from any single buffer size.
 */
class JsonSink {
public:
    virtual ~JsonSink() {}
    virtual void write(const char* data, size_t len) = 0;
};

/**
 * Sink that accumulates into a fixed, caller-owned buffer. Always NUL-terminated
 * and overflow-safe: writes past capacity are truncated and flagged rather than
 * corrupting memory. Used where a single contiguous payload is required (e.g. an
 * MQTT publish).
 */
class BufferJsonSink : public JsonSink {
public:
    BufferJsonSink(char* buf, size_t capacity) : buf(buf), capacity(capacity) {
        if(capacity > 0) buf[0] = '\0';
    }
    void write(const char* data, size_t len) override {
        if(capacity == 0) return;
        size_t avail = capacity - 1 - len_; // room excluding the NUL terminator
        if(len > avail) { len = avail; overflowed_ = true; }
        if(len > 0) {
            memcpy(buf + len_, data, len);
            len_ += len;
            buf[len_] = '\0';
        }
    }
    size_t length() const { return len_; }
    bool overflowed() const { return overflowed_; }
private:
    char* buf;
    size_t capacity;
    size_t len_ = 0;
    bool overflowed_ = false;
};

class AmsJsonGenerator {
public:
    static void generateDayPlotJson(AmsDataStorage* ds, char* buf, size_t bufSize);
    static void generateMonthPlotJson(AmsDataStorage* ds, char* buf, size_t bufSize);
    // Streamed generation: emits the configuration JSON to the sink section by section.
    static void generateConfigurationJson(AmsConfiguration* config, JsonSink& sink);
    // Convenience wrapper for callers that need the whole document in one buffer.
    static void generateConfigurationJson(AmsConfiguration* config, char* buf, size_t bufSize);
};
