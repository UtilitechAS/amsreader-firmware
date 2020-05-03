#ifndef _AMSWEBSERVER_h
#define _AMSWEBSERVER_h

#define BOOTSTRAP_URL "https://cdnjs.cloudflare.com/ajax/libs/twitter-bootstrap/4.4.1/css/bootstrap.min.css"

#include <MQTT.h>
#include <ArduinoJson.h>
#include "AmsConfiguration.h"
#include "HwTools.h"
#include "AmsData.h"
#include "Uptime.h"
#include "RemoteDebug.h"

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
	AmsWebServer(RemoteDebug* Debug);
    void setup(AmsConfiguration* config, MQTTClient* mqtt);
    void loop();

	void setData(AmsData& data);

private:
	RemoteDebug* debugger;
	int maxPwr = 0;
	HwTools hw;
    AmsConfiguration* config;
	AmsData data;
	MQTTClient* mqtt;
	File file;
	bool performRestart = false;

#if defined(ESP8266)
	ESP8266WebServer server;
#elif defined(ESP32) // ARDUINO_ARCH_ESP32
	WebServer server;
#endif

	bool checkSecurity(byte level);

	void indexHtml();
	void indexJs();
	void configMeterHtml();
	void configWifiHtml();
	void configMqttHtml();
	void configWebHtml();
	void configDomoticzHtml();
	void bootCss();
	void gaugemeterJs();
    void dataJson();

	void handleSave();

	void configSystemHtml();
	void configSystemUpload();
	void restartWaitHtml();
	void isAliveCheck();

	void uploadHtml(const char* label, const char* action, const char* menu);
	void deleteHtml(const char* label, const char* action, const char* menu);
	void uploadFile(const char* path);
	void deleteFile(const char* path);
	void uploadPost();
	void mqttCa();
	void mqttCaUpload();
	void mqttCaDelete();
	void mqttCert();
	void mqttCertUpload();
	void mqttCertDelete();
	void mqttKey();
	void mqttKeyUpload();
	void mqttKeyDelete();

	void printD(String fmt, ...);
	void printI(String fmt, ...);
	void printW(String fmt, ...);
	void printE(String fmt, ...);
};

#endif
