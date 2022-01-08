#ifndef _AMSTOMQTTBRIDGE_H
#define _AMSTOMQTTBRIDGE_H

#define WIFI_CONNECTION_TIMEOUT 60000;

#define INVALID_BUTTON_PIN  0xFFFFFFFF

#define MAX_PEM_SIZE 4096

#include <SoftwareSerial.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ESPmDNS.h>
#include "Update.h"
#endif

#include "LittleFS.h"

#endif
