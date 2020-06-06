#ifndef _HANTOJSON_h
#define _HANTOJSON_h

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include <ArduinoJson.h>
#include "AmsData.h"
#include "HwTools.h"

void hanToJson(JsonDocument& json, AmsData& data, HwTools& hw, double temperature, String name);

#endif
