#include "ZmartChargeCloudConnector.h"
#include "Uptime.h"
#include "ArduinoJson.h"

void ZmartChargeCloudConnector::setToken(const char* token) {
    strcpy(this->token, token);
}

void ZmartChargeCloudConnector::update(AmsData& data) {
    if(strlen(token) == 0) return;

    float maximum = max(max(data.getL1Current(), data.getL1Current()), data.getL3Current());
    bool fast = maximum > heartbeatFastThreshold;

    uint64_t now = millis64();
    if(((now - lastUpdate) / 1000) > (fast || lastFailed ? heartbeatFast : heartbeat)) {
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
        if(http->begin(F("https://central.zmartcharge.com/api/loadbalancer"))) {
            int status = http->POST(json);
            if(status == 200) {
                lastFailed = false;
                JsonDocument doc;
                deserializeJson(doc, http->getString());
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
                }
            }
        }
        lastUpdate = now;
    }
}