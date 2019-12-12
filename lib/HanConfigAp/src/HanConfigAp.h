// ap.h

#ifndef _ACCESSPOINT_h
#define _ACCESSPOINT_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#if defined(ESP8266)
	#include <ESP8266WiFi.h>
	#include <ESP8266WebServer.h>
#elif defined(ESP32) // ARDUINO_ARCH_ESP32
	#include <WiFi.h>
	#include <WebServer.h>
#else
	#warning "Unsupported board type"
#endif

#include <DNSServer.h>
#include "configuration.h"

#define INVALID_BUTTON_PIN  0xFFFFFFFF

class HanConfigAp {
public:
	void setup(int accessPointButtonPin, Stream* debugger);
	bool loop();
	bool hasConfig();
	configuration config;
	bool isActivated = false;

private:
	const char* AP_SSID = "AMS2MQTT";

	// DNS server
	const byte DNS_PORT = 53;
	DNSServer dnsServer;

	static size_t print(const char* text);
	static size_t println(const char* text);
	static size_t print(const Printable& data);
	static size_t println(const Printable& data);

	// Web server
	static void handleRoot();
	static void handleSave();
#if defined(ESP8266)
	static ESP8266WebServer server;
#elif defined(ESP32) // ARDUINO_ARCH_ESP32
	static WebServer server;
#endif

	static Stream* debugger;
};

#endif

