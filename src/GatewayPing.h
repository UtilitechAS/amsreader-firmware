/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 * @brief Non-blocking ICMP echo of the gateway, for the network watchdog
 */

#ifndef _GATEWAYPING_H
#define _GATEWAYPING_H

#include <Arduino.h>
#include <IPAddress.h>
#if defined(ESP32)
#include "ping/ping_sock.h"
#elif defined(ESP8266)
extern "C" {
#include <ping.h>
}
#endif

// Echo requests per probe, one second apart. A single reply is enough.
#define GATEWAY_PING_COUNT 3

class GatewayPing {
public:
    /** Starts a probe in the background. False if one is running or it could not start */
    bool start(IPAddress target);

    /** True once when a started probe has finished, with whether any request was answered */
    bool finished(bool& replied);

private:
    bool running = false;
    volatile bool done = false;
    volatile uint32_t replies = 0;
    #if defined(ESP32)
    esp_ping_handle_t session = NULL;
    static void onEnd(esp_ping_handle_t hdl, void* args);
    #elif defined(ESP8266)
    struct ping_option option;
    static void onReply(void* option, void* response);
    static void onEnd(void* option, void* response);
    #endif
};

#endif
