/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 * 
 */

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
    bool changed;
};

class HwTools {
public:
    bool applyBoardConfig(uint8_t boardType, GpioConfig& gpioConfig, MeterConfig& meterConfig, uint8_t hanPin);
    void setup(SystemConfig* sys, GpioConfig* gpio);
    float getVcc();
    void setMaxVcc(float maxVcc);
    uint8_t getTempSensorCount();
    TempSensorData* getTempSensorData(uint8_t);
    bool updateTemperatures();
    float getTemperature();
    float getTemperatureAnalog();
    float getTemperature(uint8_t address[8]);
    int getWifiRssi();
    bool ledOn(uint8_t color);
    bool ledOff(uint8_t color);
    bool ledFlash(uint8_t color, uint8_t count, bool fast = true, bool suppressMeterLed = false);
    bool ledFlashBlocking(uint8_t color, uint8_t count, bool fast = true);
    void ledLoop();
    bool ledBusy();
    void setBootSuccessful(bool value);
    bool isVoltageOptimal(float range = 0.4);
    uint8_t getBoardType();

    HwTools() {};
private:
    uint8_t boardType;
    uint8_t ledPin, redPin, greenPin, bluePin, tempPin, atempPin;
    uint8_t ledDisablePin = 0xFF, ledBehaviour;
    bool ledInvert, rgbInvert;
    uint8_t vccPin, vccGnd_r, vccVcc_r;
    float vccOffset, vccMultiplier;
    float vcc = 3.3; // Last known Vcc
    float maxVcc = 3.28; // Best to have this close to max as a start, in case Pow-U reboots and starts off with a low voltage, we dont want that to be perceived as max
    unsigned long lastVccRead = 0;

    uint16_t analogRange = 1024;
    bool tempSensorInit;
    OneWire *oneWire = NULL;
    DallasTemperature *sensorApi = NULL;
    uint8_t sensorCount = 0;
    TempSensorData** tempSensors = NULL;

    bool bootSuccessful = false;

    // Non-blocking flash sequence state (see ledFlash/ledLoop)
    uint8_t flashColor = 0;
    uint8_t flashLeft = 0;          // on-phases remaining
    bool flashOn = false;           // LED currently lit
    uint16_t flashOnMs = 200, flashOffMs = 350;
    unsigned long flashAt = 0;      // millis() of last transition
    bool flashSuppressHw = false;   // hold the meter-activity LED off during this burst

    bool writeLedPin(uint8_t color, uint8_t state);
    void applyLedDisablePin();
    bool ledColorAvailable(uint8_t color);
    bool isSensorAddressEqual(uint8_t a[8], uint8_t b[8]);
};

#endif
