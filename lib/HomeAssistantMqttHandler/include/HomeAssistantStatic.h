#ifndef _HOMEASSISTANTSTATIC_H
#define _HOMEASSISTANTSTATIC_H

#include "Arduino.h"

struct HomeAssistantSensor {
    const char* name;
    const char* topic;
    const char* path;
    uint16_t ttl;
    const char* uom;
    const char* devcl;
    const char* stacl;
};


const uint8_t List1SensorCount PROGMEM = 1;
const HomeAssistantSensor List1Sensors[List1SensorCount] PROGMEM = {
    {"Active import",              "/power",   "P",                   30, "W",    "power",           "measurement"}
};

const uint8_t List2SensorCount PROGMEM = 8;
const HomeAssistantSensor List2Sensors[List2SensorCount] PROGMEM = {
    {"Reactive import",            "/power",   "Q",                   30, "var",  "reactive_power",  "measurement"},
    {"Reactive export",            "/power",   "QO",                  30, "var",  "reactive_power",  "measurement"},
    {"L1 current",                 "/power",   "I1",                  30, "A",    "current",         "measurement"},
    {"L2 current",                 "/power",   "I2",                  30, "A",    "current",         "measurement"},
    {"L3 current",                 "/power",   "I3",                  30, "A",    "current",         "measurement"},
    {"L1 voltage",                 "/power",   "U1",                  30, "V",    "voltage",         "measurement"},
    {"L2 voltage",                 "/power",   "U2",                  30, "V",    "voltage",         "measurement"},
    {"L3 voltage",                 "/power",   "U3",                  30, "V",    "voltage",         "measurement"}
};

const uint8_t List2ExportSensorCount PROGMEM = 1;
const HomeAssistantSensor List2ExportSensors[List2ExportSensorCount] PROGMEM = {
    {"Active export",              "/power",   "PO",                  30, "W",    "power",           "measurement"}
};

const uint8_t List3SensorCount PROGMEM = 3;
const HomeAssistantSensor List3Sensors[List3SensorCount] PROGMEM = {
    {"Accumulated active import",  "/energy",  "tPI",               4000, "kWh",  "energy",          "total_increasing"},
    {"Accumulated reactive import","/energy",  "tQI",               4000, "kvarh","",                "total_increasing"},
    {"Accumulated reactive export","/energy",  "tQO",               4000, "kvarh","",                "total_increasing"}
};

const uint8_t List3ExportSensorCount PROGMEM = 1;
const HomeAssistantSensor List3ExportSensors[List3ExportSensorCount] PROGMEM = {
    {"Accumulated active export",  "/energy",  "tPO",               4000, "kWh",  "energy",          "total_increasing"}
};

const uint8_t List4SensorCount PROGMEM = 7;
const HomeAssistantSensor List4Sensors[List4SensorCount] PROGMEM = {
    {"Power factor",               "/power",   "PF",                  30, "%",    "power_factor",    "measurement"},
    {"L1 power factor",            "/power",   "PF1",                 30, "%",    "power_factor",    "measurement"},
    {"L2 power factor",            "/power",   "PF2",                 30, "%",    "power_factor",    "measurement"},
    {"L3 power factor",            "/power",   "PF3",                 30, "%",    "power_factor",    "measurement"},
    {"L1 active import",           "/power",   "P1",                  30, "W",    "power",           "measurement"},
    {"L2 active import",           "/power",   "P2",                  30, "W",    "power",           "measurement"},
    {"L3 active import",           "/power",   "P3",                  30, "W",    "power",           "measurement"}
};

const uint8_t List4ExportSensorCount PROGMEM = 3;
const HomeAssistantSensor List4ExportSensors[List4ExportSensorCount] PROGMEM = {
    {"L1 active export",           "/power",   "PO1",                 30, "W",    "power",           "measurement"},
    {"L2 active export",           "/power",   "PO2",                 30, "W",    "power",           "measurement"},
    {"L3 active export",           "/power",   "PO3",                 30, "W",    "power",           "measurement"}
};

const uint8_t RealtimeSensorCount PROGMEM = 8;
const HomeAssistantSensor RealtimeSensors[RealtimeSensorCount] PROGMEM = {
    {"Month max",                  "/realtime","max",                120, "kWh",  "energy",          "total_increasing"},
    {"Tariff threshold",           "/realtime","threshold",          120, "kWh",  "energy",          "total_increasing"},
    {"Current hour used",          "/realtime","hour.use",           120, "kWh",  "energy",          "total_increasing"},
    {"Current hour cost",          "/realtime","hour.cost",          120, "",     "monetary",        ""},
    {"Current day used",           "/realtime","day.use",            120, "kWh",  "energy",          "total_increasing"},
    {"Current day cost",           "/realtime","day.cost",           120, "",     "monetary",        ""},
    {"Current month used",         "/realtime","month.use",          120, "kWh",  "energy",          "total_increasing"},
    {"Current month cost",         "/realtime","month.cost",         120, "",     "monetary",        ""}
};

const uint8_t RealtimeExportSensorCount PROGMEM = 6;
const HomeAssistantSensor RealtimeExportSensors[RealtimeExportSensorCount] PROGMEM = {
    {"Current hour produced",      "/realtime","hour.produced",      120, "kWh",  "energy",          "total_increasing"},
    {"Current hour income",        "/realtime","hour.income",        120, "",     "monetary",        ""},
    {"Current day produced",       "/realtime","day.produced",       120, "kWh",  "energy",          "total_increasing"},
    {"Current day income",         "/realtime","day.income",         120, "",     "monetary",        ""},
    {"Current month produced",     "/realtime","month.produced",     120, "kWh",  "energy",          "total_increasing"},
    {"Current month income",       "/realtime","month.income",       120, "",     "monetary",        ""}
};

const HomeAssistantSensor RealtimePeakSensor PROGMEM = {"Current month peak %d", "/realtime", "peaks[%d]", 4000, "kWh", "energy", ""};

const uint8_t PriceSensorCount PROGMEM = 5;
const HomeAssistantSensor PriceSensors[PriceSensorCount] PROGMEM = {
    {"Minimum price ahead",        "/prices",  "prices.min",        4000, "",     "monetary",        ""},
    {"Maximum price ahead",        "/prices",  "prices.max",        4000, "",     "monetary",        ""},
    {"Cheapest 1hr period ahead",  "/prices",  "prices.cheapest1hr",4000, "",     "timestamp",       ""},
    {"Cheapest 3hr period ahead",  "/prices",  "prices.cheapest3hr",4000, "",     "timestamp",       ""},
    {"Cheapest 6hr period ahead",  "/prices",  "prices.cheapest6hr",4000, "",     "timestamp",       ""}
};

const HomeAssistantSensor PriceSensor PROGMEM = {"Price in %02d %s", "/prices", "prices['%d']", 4000, "", "monetary", ""};

const uint8_t SystemSensorCount PROGMEM = 2;
const HomeAssistantSensor SystemSensors[SystemSensorCount] PROGMEM = {
    {"Status",                     "/state",   "rssi",               180, "dBm",  "signal_strength", "measurement"},
    {"Supply volt",                "/state",   "vcc",                180, "V",    "voltage",         "measurement"}
};

const HomeAssistantSensor TemperatureSensor PROGMEM = {"Temperature sensor %s", "/temperatures", "temperatures['%s']", 900, "°C", "temperature", "measurement"};


#endif
