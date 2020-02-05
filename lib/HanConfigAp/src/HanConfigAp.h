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
#elif defined(ESP32) // ARDUINO_ARCH_ESP32
	#include <WiFi.h>
#else
	#warning "Unsupported board type"
#endif

#include <DNSServer.h>
#include "configuration.h"

#define INVALID_BUTTON_PIN  0xFFFFFFFF

class HanConfigAp {
public:
	void setup(int accessPointButtonPin, configuration* config, Stream* debugger);
	bool loop();
	bool hasConfig();
	bool isActivated = false;

private:
	const char* AP_SSID = "AMS2MQTT";

	configuration* config;

	// DNS server
	const byte DNS_PORT = 53;
	DNSServer dnsServer;

	static size_t print(const char* text);
	static size_t println(const char* text);
	static size_t print(const Printable& data);
	static size_t println(const Printable& data);

	static Stream* debugger;
};

#endif

