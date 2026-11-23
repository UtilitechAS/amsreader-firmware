/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 */

#ifndef _BOUNDEDWIFICLIENT_H
#define _BOUNDEDWIFICLIENT_H

#if defined(ESP32)
#include <WiFiClient.h>
#include "BoundedSend.h"

// A WiFiClient whose write() gives up after a fixed time instead of blocking
// for as long as the link trickles (see BoundedSend.h). While a write blocks,
// the main loop does not read the HAN port. A write that does not complete
// closes the socket, so the MQTT client reconnects instead of continuing a
// stream with half a packet in it.
class BoundedWiFiClient : public WiFiClient {
public:
    void setWriteTimeout(uint32_t ms) {
        writeTimeoutMs = ms;
    }

    size_t write(const uint8_t *buf, size_t size) override {
        // Not connected(): it peeks the socket and reads errno left by other calls
        if(fd() < 0) return 0;
        bool failed;
        size_t sent = boundedSend(fd(), buf, size, writeTimeoutMs, &failed);
        if(failed) stop();
        return sent;
    }

    using WiFiClient::write;

private:
    uint32_t writeTimeoutMs = 1000;
};
#endif

#endif
