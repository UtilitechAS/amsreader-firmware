// ap.h

#ifndef _ACCESSPOINT_h
#define _ACCESSPOINT_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include "configuration.h"

class accesspoint {
public:
	void setup(int accessPointButtonPin, Stream& debugger);
	bool loop();

private:
	const char* AP_SSID = "AMS2MQTT";

	configuration config;
	bool isActivated = false;
	Stream* debugger = NULL;

	// DNS server
	const byte DNS_PORT = 53;
	DNSServer dnsServer;

	size_t print(const char* text);
	size_t println(const char* text);
	size_t print(const Printable& data);
	size_t println(const Printable& data);

	// Web server
	static void handleRoot();
	static ESP8266WebServer server;
};

#endif

