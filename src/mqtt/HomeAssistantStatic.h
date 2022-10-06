#ifndef _HOMEASSISTANTSTATIC_H
#define _HOMEASSISTANTSTATIC_H

#include "Arduino.h"

const char* HA_TOPICS[17] PROGMEM = {"/state", "/state", "/state", "/power", "/power", "/power", "/power", "/power", "/power", "/power", "/power", "/power", "/power", "/energy", "/energy", "/energy", "/energy"};
const char* HA_NAMES[17] PROGMEM = {"Status", "Supply volt", "Temperature", "Active import", "Reactive import", "Active export", "Reactive export", "L1 current", "L2 current", "L3 current",
                            "L1 voltage", "L2 voltage", "L3 voltage", "Accumulated active import", "Accumulated active export", "Accumulated reactive import", "Accumulated reactive export"};
const char* HA_PARAMS[17] PROGMEM = {"rssi", "vcc", "temp", "P", "Q", "PO", "QO", "I1", "I2", "I3", "U1", "U2", "U3", "tPI", "tPO", "tQI", "tQO"};
const char* HA_UOM[17] PROGMEM = {"dBm", "V", "C", "W", "W", "W", "W", "A", "A", "A", "V", "V", "V", "kWh", "kWh", "kWh", "kWh"};
const char* HA_DEVCL[17] PROGMEM = {"signal_strength", "voltage", "temperature", "power", "power", "power", "power", "current", "current", "current", "voltage", "voltage", "voltage", "energy", "energy", "energy", "energy"};
const char* HA_STACL[17] PROGMEM = {"", "", "", "\"measurement\"", "\"measurement\"", "\"measurement\"", "\"measurement\"", "", "", "", "", "", "", "\"total_increasing\"", "\"total_increasing\"", "\"total_increasing\"", "\"total_increasing\""};

#endif