#pragma once

#include "RemoteDebug.h"
#include "AmsData.h"
#include "FirmwareVersion.h"

#if defined(ESP8266)
	#include <ESP8266HTTPClient.h>
#elif defined(ESP32) // ARDUINO_ARCH_ESP32
	#include <HTTPClient.h>
#else
	#warning "Unsupported board type"
#endif

static const char ZC_LB_JSON[] PROGMEM = "{\"LBToken\":\"%s\",\"L1A\":\"%.1f\",\"L2A\":\"%.1f\",\"L3A\":\"%.1f\",\"HighConsumption\":\"%d\",\"CurrentPower\":\"%d\"}";

class ZmartChargeCloudConnector {
public:
    ZmartChargeCloudConnector(RemoteDebug* debugger, char* buf) {
        this->debugger = debugger;
        this->json = buf;
        http = new HTTPClient();
        http->setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http->setReuse(false);
        http->setTimeout(10000);
        http->setUserAgent("ams2mqtt/" + String(FirmwareVersion::VersionString));
    };
    void setup(const char* baseUrl, const char* token);
    void update(AmsData& data);

    bool isConfigChanged();
    void ackConfigChanged();

    const char* getBaseUrl();

private:
    RemoteDebug* debugger;
    char baseUrl[64];
    char token[21];
    uint16_t BufferSize = 2048;
    char* json;

    bool configChanged = false;

    bool lastFailed = false;
    uint64_t lastUpdate = 0;
    HTTPClient* http = NULL;

    uint16_t heartbeat = 30;
    uint16_t heartbeatFast = 10;
    uint16_t heartbeatFastThreshold = 32;
};