#ifndef _AMSTOMQTTBRIDGE_H
#define _AMSTOMQTTBRIDGE_H

#define WIFI_CONNECTION_TIMEOUT 30000

#define INVALID_BUTTON_PIN  0xFFFFFFFF

#define MAX_PEM_SIZE 4096

#define METER_SOURCE_NONE 0
#define METER_SOURCE_SERIAL 1
#define METER_SOURCE_MQTT 2
#define METER_SOURCE_ESPNOW 3

#define METER_ERROR_NO_DATA 90
#define METER_ERROR_BREAK 91
#define METER_ERROR_BUFFER 92
#define METER_ERROR_FIFO 93
#define METER_ERROR_FRAME 94
#define METER_ERROR_PARITY 95
#define METER_ERROR_RX 96
#define METER_ERROR_EXCEPTION 98
#define METER_ERROR_AUTODETECT 99

#include <SoftwareSerial.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266SSDP.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <ESP32SSDP.h>
#endif

#include "LittleFS.h"

#endif
