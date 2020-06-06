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

class HwTools {
public:
    void setTempSensorPin(int tempSensorPin);
    void setVccPin(int vccPin);
    void setVccMultiplier(double vccMultiplier);
    double getVcc();
    double getTemperature();
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
    double vccMultiplier = 1.0;
    bool tempSensorInit, hasTempSensor;
    OneWire *oneWire;
    DallasTemperature *tempSensor;

    bool writeLedPin(uint8_t color, uint8_t state);
};

#endif
