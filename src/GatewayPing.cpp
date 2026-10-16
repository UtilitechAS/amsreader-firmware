/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 * @details On ESP32 the probe runs in its own task from the ping_sock component,
 * so the result is handed over through the volatile flags. On ESP8266 the SDK
 * ping runs on lwIP timers in the same context as the main loop.
 */

#include "GatewayPing.h"

bool GatewayPing::start(IPAddress target) {
    if(running) return false;
    done = false;
    replies = 0;

    #if defined(ESP32)
    esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
    config.count = GATEWAY_PING_COUNT;
    config.target_addr.type = IPADDR_TYPE_V4;
    config.target_addr.u_addr.ip4.addr = (uint32_t) target;

    esp_ping_callbacks_t callbacks = {};
    callbacks.cb_args = this;
    callbacks.on_ping_end = onEnd;

    if(esp_ping_new_session(&config, &callbacks, &session) != ESP_OK) {
        return false;
    }
    if(esp_ping_start(session) != ESP_OK) {
        esp_ping_delete_session(session);
        return false;
    }
    running = true;
    #elif defined(ESP8266)
    memset(&option, 0, sizeof(option));
    option.count = GATEWAY_PING_COUNT;
    option.ip = (uint32_t) target;
    option.coarse_time = 1;
    option.reverse = this;
    ping_regist_recv(&option, onReply); // Without it, the SDK prints every timeout to serial
    ping_regist_sent(&option, onEnd);
    running = ping_start(&option);
    #endif
    return running;
}

bool GatewayPing::finished(bool& replied) {
    if(!running || !done) return false;
    #if defined(ESP32)
    esp_ping_delete_session(session);
    session = NULL;
    #endif
    running = false;
    replied = replies > 0;
    return true;
}

#if defined(ESP32)
void GatewayPing::onEnd(esp_ping_handle_t hdl, void* args) {
    GatewayPing* self = (GatewayPing*) args;
    uint32_t received = 0;
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
    self->replies = received;
    self->done = true;
}
#elif defined(ESP8266)
void GatewayPing::onReply(void* option, void* response) {
}

void GatewayPing::onEnd(void* option, void* response) {
    GatewayPing* self = (GatewayPing*) ((struct ping_option*) option)->reverse;
    struct ping_resp* resp = (struct ping_resp*) response;
    self->replies = resp->total_count - resp->timeout_count;
    self->done = true;
}
#endif
