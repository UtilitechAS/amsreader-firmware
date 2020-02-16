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

#if HW_ROARFRED
#define TEMP_SENSOR_PIN 5
#elif defined(ARDUINO_LOLIN_D32)
#define TEMP_SENSOR_PIN 14
#elif defined(ARDUINO_ESP8266_WEMOS_D1MINI)
#define TEMP_SENSOR_PIN D5
#else
#define TEMP_SENSOR_PIN 0xFFFFFFFF
#endif



class HwTools {
public:
    double getVcc();
    double getTemperature();

    HwTools() {
        oneWire = new OneWire(TEMP_SENSOR_PIN);
        tempSensor = new DallasTemperature(this->oneWire);
    };
private:
    bool tempSensorInit, hasTempSensor;
    OneWire *oneWire;
    DallasTemperature *tempSensor;
};

#endif
