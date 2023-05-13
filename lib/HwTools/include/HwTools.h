#ifndef _HWTOOLS_H
#define _HWTOOLS_H

#include "Arduino.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <soc/adc_channel.h>
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
    bool changed;
};

struct AdcConfig {
    uint8_t unit;
    uint8_t channel;
};

class HwTools {
public:
    void setup(GpioConfig*, AmsConfiguration*);
    float getVcc();
    uint8_t getTempSensorCount();
    TempSensorData* getTempSensorData(uint8_t);
    bool updateTemperatures();
    float getTemperature();
    float getTemperatureAnalog();
    float getTemperature(uint8_t address[8]);
    int getWifiRssi();
    bool ledOn(uint8_t color);
    bool ledOff(uint8_t color);
    bool ledBlink(uint8_t color, uint8_t blink);

    HwTools() {};
private:
    uint16_t analogRange = 1024;
    AdcConfig voltAdc, tempAdc;
    #if defined(ESP32)
        esp_adc_cal_characteristics_t* voltAdcChar, tempAdcChar;
    #endif
    GpioConfig* config;
    AmsConfiguration* amsConf;
    bool tempSensorInit;
    OneWire *oneWire = NULL;
    DallasTemperature *sensorApi = NULL;
    uint8_t sensorCount = 0;
    TempSensorData** tempSensors = NULL;

    bool writeLedPin(uint8_t color, uint8_t state);
    bool isSensorAddressEqual(uint8_t a[8], uint8_t b[8]);
    void getAdcChannel(uint8_t pin, AdcConfig&);
};

#endif
