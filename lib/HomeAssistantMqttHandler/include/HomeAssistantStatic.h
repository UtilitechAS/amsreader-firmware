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


const uint8_t HA_SENSOR_COUNT PROGMEM = 60;
HomeAssistantSensor HA_SENSORS[HA_SENSOR_COUNT] PROGMEM = {
    {"Status",                     "/state",   "rssi",              "dBm",  "signal_strength", "\"measurement\""},
    {"Supply volt",                "/state",   "vcc",               "V",    "voltage",         "\"measurement\""},
    {"Temperature",                "/state",   "temp",              "°C",   "temperature",     "\"measurement\""},
    {"Active import",              "/power",   "P",                 "W",    "power",           "\"measurement\""},
    {"L1 active import",           "/power",   "P1",                "W",    "power",           "\"measurement\""},
    {"L2 active import",           "/power",   "P2",                "W",    "power",           "\"measurement\""},
    {"L3 active import",           "/power",   "P3",                "W",    "power",           "\"measurement\""},
    {"Reactive import",            "/power",   "Q",                 "var",  "reactive_power",  "\"measurement\""},
    {"Active export",              "/power",   "PO",                "W",    "power",           "\"measurement\""},
    {"L1 active export",           "/power",   "PO1",               "W",    "power",           "\"measurement\""},
    {"L2 active export",           "/power",   "PO2",               "W",    "power",           "\"measurement\""},
    {"L3 active export",           "/power",   "PO3",               "W",    "power",           "\"measurement\""},
    {"Reactive export",            "/power",   "QO",                "var",  "reactive_power",  "\"measurement\""},
    {"L1 current",                 "/power",   "I1",                "A",    "current",         "\"measurement\""},
    {"L2 current",                 "/power",   "I2",                "A",    "current",         "\"measurement\""},
    {"L3 current",                 "/power",   "I3",                "A",    "current",         "\"measurement\""},
    {"L1 voltage",                 "/power",   "U1",                "V",    "voltage",         "\"measurement\""},
    {"L2 voltage",                 "/power",   "U2",                "V",    "voltage",         "\"measurement\""},
    {"L3 voltage",                 "/power",   "U3",                "V",    "voltage",         "\"measurement\""},
    {"Accumulated active import",  "/energy",  "tPI",               "kWh",  "energy",          "\"total_increasing\""},
    {"Accumulated active export",  "/energy",  "tPO",               "kWh",  "energy",          "\"total_increasing\""},
    {"Accumulated reactive import","/energy",  "tQI",               "kvarh","energy",          "\"total_increasing\""},
    {"Accumulated reactive export","/energy",  "tQO",               "kvarh","energy",          "\"total_increasing\""},
    {"Power factor",               "/power",   "PF",                "%",    "power_factor",    "\"measurement\""},
    {"L1 power factor",            "/power",   "PF1",               "%",    "power_factor",    "\"measurement\""},
    {"L2 power factor",            "/power",   "PF2",               "%",    "power_factor",    "\"measurement\""},
    {"L3 power factor",            "/power",   "PF3",               "%",    "power_factor",    "\"measurement\""},
    {"Price current hour",         "/prices",  "prices['0']",       "",     "monetary",        ""},
    {"Price next hour",            "/prices",  "prices['1']",       "",     "monetary",        ""},
    {"Price in two hour",          "/prices",  "prices['2']",       "",     "monetary",        ""},
    {"Price in three hour",        "/prices",  "prices['3']",       "",     "monetary",        ""},
    {"Price in four hour",         "/prices",  "prices['4']",       "",     "monetary",        ""},
    {"Price in five hour",         "/prices",  "prices['5']",       "",     "monetary",        ""},
    {"Price in six hour",          "/prices",  "prices['6']",       "",     "monetary",        ""},
    {"Price in seven hour",        "/prices",  "prices['7']",       "",     "monetary",        ""},
    {"Price in eight hour",        "/prices",  "prices['8']",       "",     "monetary",        ""},
    {"Price in nine hour",         "/prices",  "prices['9']",       "",     "monetary",        ""},
    {"Price in ten hour",          "/prices",  "prices['10']",      "",     "monetary",        ""},
    {"Price in eleven hour",       "/prices",  "prices['11']",      "",     "monetary",        ""},
    {"Minimum price ahead",        "/prices",  "prices.min",        "",     "monetary",        ""},
    {"Maximum price ahead",        "/prices",  "prices.max",        "",     "monetary",        ""},
    {"Cheapest 1hr period ahead",  "/prices",  "prices.cheapest1hr","",     "timestamp",       ""},
    {"Cheapest 3hr period ahead",  "/prices",  "prices.cheapest3hr","",     "timestamp",       ""},
    {"Cheapest 6hr period ahead",  "/prices",  "prices.cheapest6hr","",     "timestamp",       ""},
    {"Month max",                  "/realtime","max",               "kWh",  "energy",          "\"total_increasing\""},
    {"Tariff threshold",           "/realtime","threshold",         "kWh",  "energy",          "\"total_increasing\""},
    {"Current hour used",          "/realtime","hour.use",          "kWh",  "energy",          "\"total_increasing\""},
    {"Current hour cost",          "/realtime","hour.cost",         "",     "monetary",        "\"total_increasing\""},
    {"Current hour produced",      "/realtime","hour.produced",     "kWh",  "energy",          "\"total_increasing\""},
    {"Current day used",           "/realtime","day.use",           "kWh",  "energy",          "\"total_increasing\""},
    {"Current day cost",           "/realtime","day.cost",          "",     "monetary",        "\"total_increasing\""},
    {"Current day produced",       "/realtime","day.produced",      "kWh",  "energy",          "\"total_increasing\""},
    {"Current month used",         "/realtime","month.use",         "kWh",  "energy",          "\"total_increasing\""},
    {"Current month cost",         "/realtime","month.cost",        "",     "monetary",        "\"total_increasing\""},
    {"Current month produced",     "/realtime","month.produced",    "kWh",  "energy",          "\"total_increasing\""},
    {"Current month peak 1",       "/realtime","peaks[0]",          "kWh",  "energy",          ""},
    {"Current month peak 2",       "/realtime","peaks[1]",          "kWh",  "energy",          ""},
    {"Current month peak 3",       "/realtime","peaks[2]",          "kWh",  "energy",          ""},
    {"Current month peak 4",       "/realtime","peaks[3]",          "kWh",  "energy",          ""},
    {"Current month peak 5",       "/realtime","peaks[4]",          "kWh",  "energy",          ""},
};


#endif
