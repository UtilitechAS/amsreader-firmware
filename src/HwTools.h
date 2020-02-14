#ifndef _HWTOOLS_H
#define _HWTOOLS_H

#include "Arduino.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

#include <DallasTemperature.h>
#include <OneWire.h>

class HwTools {
public:
    static double getVcc();
    static double getTemperature();

    HwTools() {
#if defined TEMP_SENSOR_PIN
        oneWire = new OneWire(TEMP_SENSOR_PIN);
        tempSensor = new DallasTemperature(this->oneWire);
#endif
    };
private:
    bool tempSensorInit, hasTempSensor;
    OneWire *oneWire;
    DallasTemperature *tempSensor;
};

#endif
