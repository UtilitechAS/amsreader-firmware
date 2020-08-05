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

#define LED_INTERNAL 0
#define LED_RED 1
#define LED_GREEN 2
#define LED_BLUE 3
#define LED_YELLOW 4

struct TempSensorData {
    uint8_t address[8];
    char name[32];
    bool common;
    float lastRead;
    float lastValidRead;
};

class HwTools {
public:
    void setTempSensorPin(int tempSensorPin);
    void setVccPin(int vccPin);
    void setVccOffset(double vccOffset);
    void setVccMultiplier(double vccMultiplier);
    double getVcc();
    void confTempSensor(uint8_t address[8], const char name[32], bool common);
    uint8_t getTempSensorCount();
    TempSensorData* getTempSensorData(uint8_t i);
    bool updateTemperatures();
    double getTemperature();
    double getTemperature(uint8_t address[8]);
    int getWifiRssi();
    void setLed(uint8_t ledPin, bool ledInverted);
    void setLedRgb(uint8_t ledPinRed, uint8_t ledPinGreen, uint8_t ledPinBlue, bool ledRgbInverted);
    bool ledOn(uint8_t color);
    bool ledOff(uint8_t color);
    bool ledBlink(uint8_t color, uint8_t blink);

    HwTools() {};
private:
    uint8_t tempSensorPin = -1;
    uint8_t vccPin = -1;
    uint8_t ledPin = -1, ledPinRed = -1, ledPinGreen = -1, ledPinBlue = -1;
    bool ledInverted, ledRgbInverted;
    double vccOffset = 0.0;
    double vccMultiplier = 1.0;
    
    bool tempSensorInit;
    OneWire *oneWire;
    DallasTemperature *sensorApi;
    uint8_t sensorCount = 0;
    TempSensorData *tempSensors[32];

    bool writeLedPin(uint8_t color, uint8_t state);
    bool isSensorAddressEqual(uint8_t a[8], uint8_t b[8]);
};

#endif
