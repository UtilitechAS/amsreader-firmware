#ifndef _HANTOJSON_h
#define _HANTOJSON_h

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include <ArduinoJson.h>
#include "HanReader.h"

void hanToJson(JsonObject& data, byte meterType, HanReader& hanReader);
void hanToJson(JsonObject& root, byte meterType, HanReader& hanReader, Stream *debugPort);


#endif
