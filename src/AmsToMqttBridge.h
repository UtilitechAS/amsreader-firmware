#ifndef _AMSTOMQTTBRIDGE_H
#define _AMSTOMQTTBRIDGE_H

#define WIFI_CONNECTION_TIMEOUT 30000;

#define INVALID_BUTTON_PIN  0xFFFFFFFF


#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <ESPmDNS.h>
#include "SPIFFS.h"
#include "Update.h"
#endif

#define RGB_RED 1
#define RGB_GREEN 2
#define RGB_YELLOW 3
#define RGB_ON 1
#define RGB_OFF 0

// Build settings for custom hardware by Roar Fredriksen
#if HW_ROARFRED
#define LED_PIN 2 // The blue on-board LED of the ESP8266 custom AMS board
#define LED_ACTIVE_HIGH 0
#define AP_BUTTON_PIN 0

HardwareSerial *hanSerial = &Serial;

// Build settings for Wemos Lolin D32
#elif defined(ARDUINO_LOLIN_D32)
#define LED_PIN 5
#define LED_ACTIVE_HIGH 0
#define AP_BUTTON_PIN 4

HardwareSerial *hanSerial = &Serial2;

// Build settings for Wemos D1 mini
#elif defined(ARDUINO_ESP8266_WEMOS_D1MINI)
#define LED_PIN D4
#define LED_ACTIVE_HIGH 0
#define AP_BUTTON_PIN D2

#define SOFTWARE_SERIAL 1
#include <SoftwareSerial.h>
SoftwareSerial *hanSerial = new SoftwareSerial(D1);

// Build settings for Adafruit Feather ESP32
#elif defined(ARDUINO_FEATHER_ESP32)
#define LED_PIN LED_BUILTIN
#define LED_ACTIVE_HIGH 1
#define AP_BUTTON_PIN INVALID_BUTTON_PIN

HardwareSerial *hanSerial = &Serial2;

// Build settings for AZ-Delivery ESP-32 DevKitC V4 and DOIT DevKit V1
#elif defined(ARDUINO_ESP32_DEV)
#define LED_PIN 2                        // external 2 for V4 , 2 for doit v1   
#define LED_ACTIVE_HIGH 1
#define AP_BUTTON_PIN 0

HardwareSerial *hanSerial = &Serial2;    // use gpio 16 for rx2 

// Default build for ESP32
#elif defined(ESP32)
#define LED_PIN INVALID_BUTTON_PIN
#define LED_ACTIVE_HIGH 1
#define AP_BUTTON_PIN INVALID_BUTTON_PIN

HardwareSerial *hanSerial = &Serial2;

// Default build settings
#else
#define LED_PIN 2
#define LED_ACTIVE_HIGH 0
#define AP_BUTTON_PIN INVALID_BUTTON_PIN
#define SOFTWARE_SERIAL 1
#include <SoftwareSerial.h>
SoftwareSerial *hanSerial = new SoftwareSerial(5);
#endif

#endif
