#ifndef _HOMEASSISTANTSTATIC_H
#define _HOMEASSISTANTSTATIC_H

#include "Arduino.h"

struct HomeAssistantSensor {
    const char* name;
    const char* topic;
    const char* path;
    const char* uom;
    const char* devcl;
    const char* stacl;
};


const uint8_t List1SensorCount PROGMEM = 1;
const HomeAssistantSensor List1Sensors[List1SensorCount] PROGMEM = {
    {"Active import",              "/power",   "P",                 "W",    "power",           "measurement"}
};

const uint8_t List2SensorCount PROGMEM = 8;
const HomeAssistantSensor List2Sensors[List2SensorCount] PROGMEM = {
    {"Reactive import",            "/power",   "Q",                 "var",  "reactive_power",  "measurement"},
    {"Reactive export",            "/power",   "QO",                "var",  "reactive_power",  "measurement"},
    {"L1 current",                 "/power",   "I1",                "A",    "current",         "measurement"},
    {"L2 current",                 "/power",   "I2",                "A",    "current",         "measurement"},
    {"L3 current",                 "/power",   "I3",                "A",    "current",         "measurement"},
    {"L1 voltage",                 "/power",   "U1",                "V",    "voltage",         "measurement"},
    {"L2 voltage",                 "/power",   "U2",                "V",    "voltage",         "measurement"},
    {"L3 voltage",                 "/power",   "U3",                "V",    "voltage",         "measurement"}
};

const uint8_t List2ExportSensorCount PROGMEM = 1;
const HomeAssistantSensor List2ExportSensors[List2ExportSensorCount] PROGMEM = {
    {"Active export",              "/power",   "PO",                "W",    "power",           "measurement"}
};

const uint8_t List3SensorCount PROGMEM = 3;
const HomeAssistantSensor List3Sensors[List3SensorCount] PROGMEM = {
    {"Accumulated active import",  "/energy",  "tPI",               "kWh",  "energy",          "total_increasing"},
    {"Accumulated reactive import","/energy",  "tQI",               "kvarh","",                "total_increasing"},
    {"Accumulated reactive export","/energy",  "tQO",               "kvarh","",                "total_increasing"}
};

const uint8_t List3ExportSensorCount PROGMEM = 1;
const HomeAssistantSensor List3ExportSensors[List3ExportSensorCount] PROGMEM = {
    {"Accumulated active export",  "/energy",  "tPO",               "kWh",  "energy",          "total_increasing"}
};

const uint8_t List4SensorCount PROGMEM = 7;
const HomeAssistantSensor List4Sensors[List4SensorCount] PROGMEM = {
    {"Power factor",               "/power",   "PF",                "%",    "power_factor",    "measurement"},
    {"L1 power factor",            "/power",   "PF1",               "%",    "power_factor",    "measurement"},
    {"L2 power factor",            "/power",   "PF2",               "%",    "power_factor",    "measurement"},
    {"L3 power factor",            "/power",   "PF3",               "%",    "power_factor",    "measurement"},
    {"L1 active import",           "/power",   "P1",                "W",    "power",           "measurement"},
    {"L2 active import",           "/power",   "P2",                "W",    "power",           "measurement"},
    {"L3 active import",           "/power",   "P3",                "W",    "power",           "measurement"}
};

const uint8_t List4ExportSensorCount PROGMEM = 3;
const HomeAssistantSensor List4ExportSensors[List4ExportSensorCount] PROGMEM = {
    {"L1 active export",           "/power",   "PO1",               "W",    "power",           "measurement"},
    {"L2 active export",           "/power",   "PO2",               "W",    "power",           "measurement"},
    {"L3 active export",           "/power",   "PO3",               "W",    "power",           "measurement"}
};

const uint8_t RealtimeSensorCount PROGMEM = 8;
const HomeAssistantSensor RealtimeSensors[RealtimeSensorCount] PROGMEM = {
    {"Month max",                  "/realtime","max",               "kWh",  "energy",          "total_increasing"},
    {"Tariff threshold",           "/realtime","threshold",         "kWh",  "energy",          "total_increasing"},
    {"Current hour used",          "/realtime","hour.use",          "kWh",  "energy",          "total_increasing"},
    {"Current hour cost",          "/realtime","hour.cost",         "",     "monetary",        ""},
    {"Current day used",           "/realtime","day.use",           "kWh",  "energy",          "total_increasing"},
    {"Current day cost",           "/realtime","day.cost",          "",     "monetary",        ""},
    {"Current month used",         "/realtime","month.use",         "kWh",  "energy",          "total_increasing"},
    {"Current month cost",         "/realtime","month.cost",        "",     "monetary",        ""}
};

const uint8_t RealtimeExportSensorCount PROGMEM = 6;
const HomeAssistantSensor RealtimeExportSensors[RealtimeExportSensorCount] PROGMEM = {
    {"Current hour produced",      "/realtime","hour.produced",     "kWh",  "energy",          "total_increasing"},
    {"Current hour income",        "/realtime","hour.income",       "",     "monetary",        ""},
    {"Current day produced",       "/realtime","day.produced",      "kWh",  "energy",          "total_increasing"},
    {"Current day income",         "/realtime","day.income",        "",     "monetary",        ""},
    {"Current month produced",     "/realtime","month.produced",    "kWh",  "energy",          "total_increasing"},
    {"Current month income",       "/realtime","month.income",      "",     "monetary",        ""}
};

const HomeAssistantSensor RealtimePeakSensor PROGMEM = {"Current month peak %d", "/realtime", "peaks[%d]", "kWh", "energy", ""};

const uint8_t PriceSensorCount PROGMEM = 5;
const HomeAssistantSensor PriceSensors[PriceSensorCount] PROGMEM = {
    {"Minimum price ahead",        "/prices",  "prices.min",        "",     "monetary",        ""},
    {"Maximum price ahead",        "/prices",  "prices.max",        "",     "monetary",        ""},
    {"Cheapest 1hr period ahead",  "/prices",  "prices.cheapest1hr","",     "timestamp",       ""},
    {"Cheapest 3hr period ahead",  "/prices",  "prices.cheapest3hr","",     "timestamp",       ""},
    {"Cheapest 6hr period ahead",  "/prices",  "prices.cheapest6hr","",     "timestamp",       ""}
};

const HomeAssistantSensor PriceSensor PROGMEM = {"Price in %02d %s", "/prices", "prices['%d']", "", "monetary", ""};

const uint8_t SystemSensorCount PROGMEM = 2;
const HomeAssistantSensor SystemSensors[SystemSensorCount] PROGMEM = {
    {"Status",                     "/state",   "rssi",              "dBm",  "signal_strength", "measurement"},
    {"Supply volt",                "/state",   "vcc",               "V",    "voltage",         "measurement"}
};

const HomeAssistantSensor TemperatureSensor PROGMEM = {"Temperature sensor %s", "/temperatures", "temperatures['%s']", "°C", "temperature", "measurement"};


#endif
