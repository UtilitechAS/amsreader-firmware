#include "HanToJson.h"

void hanToJson(JsonDocument& json, AmsData& data, HwTools& hw, double temperature) {
    json["id"] = WiFi.macAddress();
    json["up"] = millis();
    json["t"] = data.getPackageTimestamp();

    double vcc = hw.getVcc();
    if(vcc > 0) {
        json["vcc"] = vcc;
    }

    json["rssi"] = hw.getWifiRssi();

    if(temperature != DEVICE_DISCONNECTED_C) {
        json["temp"] = temperature;
    }

    // Add a sub-structure to the json object,
    // to keep the data from the meter itself
    JsonObject jd = json.createNestedObject("data");

    switch(data.getListType()) {
        case 3:
            jd["rtc"]  = data.getMeterTimestamp();
            jd["tPI"]  = data.getActiveImportCounter();
            jd["tPO"]  = data.getActiveExportCounter();
            jd["tQI"]  = data.getReactiveImportCounter();
            jd["tQO"]  = data.getReactiveExportCounter();
        case 2:
            jd["lv"]   = data.getListId();
            jd["id"]   = data.getMeterId();
            jd["type"] = data.getMeterType();
            jd["Q"]    = data.getReactiveImportPower();
            jd["PO"]   = data.getActiveExportPower();
            jd["QO"]   = data.getReactiveExportPower();
            jd["I1"]   = data.getL1Current();
            jd["I2"]   = data.getL2Current();
            jd["I3"]   = data.getL3Current();
            jd["U1"]   = data.getL1Voltage();
            jd["U2"]   = data.getL2Voltage();
            jd["U3"]   = data.getL3Voltage();
        case 1:
            jd["P"]    = data.getActiveImportPower();
    }
}
