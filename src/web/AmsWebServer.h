#ifndef _AMSWEBSERVER_h
#define _AMSWEBSERVER_h

#include <ArduinoJson.h>
#include "configuration.h"

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

class AmsWebServer {
public:
    void setup(configuration* config, Stream* debugger);
    void loop();
	void setJson(StaticJsonDocument<500> json);

private:
    configuration* config;
	Stream* debugger;
    StaticJsonDocument<500> json;
    int maxPwr;
	int p;
	double u1, u2, u3, i1, i2, i3;

#if defined(ESP8266)
	ESP8266WebServer server;
#elif defined(ESP32) // ARDUINO_ARCH_ESP32
	WebServer server;
#endif

	bool checkSecurity(byte level);

	void indexHtml();
	void configurationHtml();
	void bootCss();
	void applicationCss();
	void gaugemeterJs();
	void indexJs();
    void dataJson();

	void handleSave();

   	size_t print(const char* text);
	size_t println(const char* text);
	size_t print(const Printable& data);
	size_t println(const Printable& data);

};

#endif
