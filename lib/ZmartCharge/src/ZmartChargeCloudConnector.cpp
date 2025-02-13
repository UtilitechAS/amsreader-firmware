#include "ZmartChargeCloudConnector.h"
#include "Uptime.h"
#include "ArduinoJson.h"

void ZmartChargeCloudConnector::setup(const char* baseUrl, const char* token) {
    memset(this->baseUrl, 0, 64);
    memset(this->token, 0, 21);
    strcpy(this->baseUrl, baseUrl);
    strcpy(this->token, token);
}

bool ZmartChargeCloudConnector::isConfigChanged() {
    return configChanged;
}

void ZmartChargeCloudConnector::ackConfigChanged() {
    configChanged = false;
}

const char* ZmartChargeCloudConnector::getBaseUrl() {
    return baseUrl;
}


void ZmartChargeCloudConnector::update(AmsData& data) {
    if(strlen(token) == 0) return;

    uint64_t now = millis64();

    float maximum = max(max(data.getL1Current(), data.getL1Current()), data.getL3Current());
    bool fast = maximum > heartbeatFastThreshold;

    if(now - lastUpdate < (fast ? heartbeatFast : heartbeat) * 1000) return;

    if(strlen(token) != 20) {
        lastUpdate = now;
        if(debugger->isActive(RemoteDebug::WARNING)) debugger->printf_P(PSTR("(ZmartCharge) Token defined, but is incorrect length (%s, %d)\n"), token, strlen(token));
        return;
    }

    if(((now - lastUpdate) / 1000) > (fast || lastFailed ? heartbeatFast : heartbeat)) {
        if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf_P(PSTR("(ZmartCharge) Preparing to update cloud\n"));
        memset(json, 0, BufferSize);
        snprintf_P(json, BufferSize, ZC_LB_JSON,
            token,
            data.getL1Current(),
            data.getL2Current(),
            data.getL3Current(),
            fast ? 1 : 0,
            data.getActiveImportPower()
        );
        lastFailed = true;
        char url[128];
        memset(url, 0, 128);
        snprintf_P(url, 128, PSTR("%s/loadbalancer"), baseUrl);
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(ZmartCharge) Connecting to: %s\n"), baseUrl);
        if(http->begin(url)) {
            if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(ZmartCharge) Sending data: %s\n"), json);
            int status = http->POST(json);
            if(status == 200) {
                lastFailed = false;
                JsonDocument doc;
                String body = http->getString();
                if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(ZmartCharge) Received data: %s\n"), body.c_str());
                deserializeJson(doc, body);
                if(doc.containsKey("Settings")) {
                    if(doc["Settings"].containsKey("HeartBeatTime")) {
                        heartbeat = doc["Settings"]["HeartBeatTime"].as<long>();
                    }
                    if(doc["Settings"].containsKey("HearBeatTimeFast")) {
                        heartbeatFast = doc["Settings"]["HearBeatTimeFast"].as<long>();
                    }
                    if(doc["Settings"].containsKey("HeartBeatTimeFastThreshold")) {
                        heartbeatFastThreshold = doc["Settings"]["HeartBeatTimeFastThreshold"].as<long>();
                    }
                    if(doc["Settings"].containsKey("ZmartChargeUrl")) {
                        String newBaseUrl = doc["Settings"]["ZmartChargeUrl"].as<String>();
                        if(newBaseUrl.startsWith("https:") && strncmp(newBaseUrl.c_str(), baseUrl, strlen(baseUrl)) != 0) {
                            newBaseUrl.replace("\\/", "/");
                            if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("(ZmartCharge) Received new URL: %s\n"), newBaseUrl.c_str());
                            memset(baseUrl, 0, 64);
                            memcpy(baseUrl, newBaseUrl.c_str(), strlen(newBaseUrl.c_str()));
                            configChanged = true;
                        }
                    }
                }
                http->end();
            } else {
                if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf_P(PSTR("(ZmartCharge) Communication error, returned status: %d\n"), status);
                if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf(http->errorToString(status).c_str());
                if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(http->getString().c_str());

                http->end();
            }
        } else {
            if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf_P(PSTR("(ZmartCharge) Unable to establish connection with cloud\n"));
        }
        lastUpdate = now;
    }
}