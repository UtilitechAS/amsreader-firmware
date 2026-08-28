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

class AmsData;
class PriceService;
class AmsMqttHandler;
#if defined(AMS_CLOUD)
class CloudConnector;
#endif
#if defined(ZMART_CHARGE)
class ZmartChargeCloudConnector;
#endif

/**
 * Everything the services array reports on. Both the web UI and the MQTT
 * announcements render from this, so the two cannot report different state for
 * the same device. Callers fill in whatever they have; NULL members are reported
 * as not-yet-connected rather than skipped.
 */
struct ServiceStatusContext {
    AmsConfiguration* config = NULL;
    AmsData* meterState = NULL;
    PriceService* ps = NULL;
    AmsMqttHandler* mqttHandler = NULL;
    bool mqttEnabled = false;
    AmsMqttHandler* customMqttHandler = NULL;
    AmsMqttHandler* energySpeedometer = NULL;
    #if defined(AMS_CLOUD)
    CloudConnector* cloud = NULL;
    #endif
    #if defined(ZMART_CHARGE)
    ZmartChargeCloudConnector* zcloud = NULL;
    #endif
};

class AmsJsonGenerator {
public:
    static void generateDayPlotJson(AmsDataStorage* ds, char* buf, size_t bufSize);
    static void generateMonthPlotJson(AmsDataStorage* ds, char* buf, size_t bufSize);
    // Streamed generation: emits the configuration JSON to the sink section by section.
    static void generateConfigurationJson(AmsConfiguration* config, JsonSink& sink);
    // Convenience wrapper for callers that need the whole document in one buffer.
    static void generateConfigurationJson(AmsConfiguration* config, char* buf, size_t bufSize);

    // Four-state health of a single service: 0 disabled, 1 ok, 2 connecting, 3 error.
    static uint8_t hanState(AmsData* meterState);
    static uint8_t mqttHandlerState(AmsMqttHandler* h);

    // The contents of the services array, without the enclosing brackets, since
    // both callers embed it in a larger document. withDetail=false drops the "d"
    // and "n" fields to fit the 256 byte MQTT packet buffer used on ESP8266.
    //
    // Returns a String rather than streaming to a JsonSink: the array is small,
    // both call sites need it as one contiguous value, and a fixed buffer would
    // risk truncating to invalid JSON when hostnames are long.
    static String generateServicesJson(const ServiceStatusContext& ctx, bool withDetail = true);
};
