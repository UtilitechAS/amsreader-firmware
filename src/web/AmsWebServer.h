#ifndef _AMSWEBSERVER_h
#define _AMSWEBSERVER_h

#define BOOTSTRAP_URL "https://cdnjs.cloudflare.com/ajax/libs/twitter-bootstrap/4.4.1/css/bootstrap.min.css"

#include <MQTT.h>
#include <ArduinoJson.h>
#include "AmsConfiguration.h"
#include "HwTools.h"
#include "AmsData.h"
#include "Uptime.h"

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
	#include "SPIFFS.h"
	#include "Update.h"
#else
	#warning "Unsupported board type"
#endif

class AmsWebServer {
public:
    void setup(AmsConfiguration* config, Stream* debugger, MQTTClient* mqtt);
    void loop();

	void setData(AmsData& data);

private:
	int maxPwr = 0;
	HwTools hw;
    AmsConfiguration* config;
	AmsData data;
	Stream* debugger;
	MQTTClient* mqtt;
	File firmwareFile;
	bool performRestart = false;

#if defined(ESP8266)
	ESP8266WebServer server;
#elif defined(ESP32) // ARDUINO_ARCH_ESP32
	WebServer server;
#endif

	bool checkSecurity(byte level);

	void indexHtml();
	void configMeterHtml();
	void configWifiHtml();
	void configMqttHtml();
	void configWebHtml();
	void bootCss();
	void gaugemeterJs();
    void dataJson();

	void handleSave();

	void configSystemHtml();
	void configSystemPost();
	void configSystemUpload();
	void restartWaitHtml();
	void isAliveCheck();

   	size_t print(const char* text);
	size_t println(const char* text);
	size_t print(const Printable& data);
	size_t println(const Printable& data);

};

#endif
