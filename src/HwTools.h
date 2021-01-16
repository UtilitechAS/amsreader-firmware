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
#include "AmsConfiguration.h"

#define LED_INTERNAL 0
#define LED_RED 1
#define LED_GREEN 2
#define LED_BLUE 3
#define LED_YELLOW 4

struct TempSensorData {
    uint8_t address[8];
    float lastRead;
    float lastValidRead;
};

class HwTools {
public:
    void setup(GpioConfig*, AmsConfiguration*);
    double getVcc();
    uint8_t getTempSensorCount();
    TempSensorData* getTempSensorData(uint8_t);
    bool updateTemperatures();
    double getTemperature();
    double getTemperatureAnalog();
    double getTemperature(uint8_t address[8]);
    int getWifiRssi();
    bool ledOn(uint8_t color);
    bool ledOff(uint8_t color);
    bool ledBlink(uint8_t color, uint8_t blink);

    HwTools() {};
private:
    GpioConfig* config;
    AmsConfiguration* amsConf;
    bool tempSensorInit;
    OneWire *oneWire = NULL;
    DallasTemperature *sensorApi = NULL;
    uint8_t sensorCount = 0;
    TempSensorData** tempSensors = NULL;

    bool writeLedPin(uint8_t color, uint8_t state);
    bool isSensorAddressEqual(uint8_t a[8], uint8_t b[8]);
};

#endif
