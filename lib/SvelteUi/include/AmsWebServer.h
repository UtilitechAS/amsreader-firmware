#ifndef _AMSWEBSERVER_h
#define _AMSWEBSERVER_h

#include "Arduino.h"
#include <MQTT.h>
#include "AmsConfiguration.h"
#include "HwTools.h"
#include "AmsData.h"
#include "AmsStorage.h"
#include "AmsDataStorage.h"
#include "EnergyAccounting.h"
#include "Uptime.h"
#include "RemoteDebug.h"
#include "EntsoeApi.h"

#if defined(ESP8266)
	#include <ESP8266WiFi.h>
	#include <ESP8266WebServer.h>
	#include <ESP8266HTTPClient.h>
	#include <ESP8266httpUpdate.h>
#elif defined(ESP32) // ARDUINO_ARCH_ESP32
	#include <WiFi.h>
	#include <WebServer.h>
	#include <HTTPClient.h>
	#include <HTTPUpdate.h>
#else
	#warning "Unsupported board type"
#endif

#include "LittleFS.h"

class AmsWebServer {
public:
	AmsWebServer(uint8_t* buf, RemoteDebug* Debug, HwTools* hw);
    void setup(AmsConfiguration*, GpioConfig*, MeterConfig*, AmsData*, AmsDataStorage*, EnergyAccounting*);
    void loop();
	void setMqtt(MQTTClient* mqtt);
	void setTimezone(Timezone* tz);
	void setMqttEnabled(bool);
	void setEntsoeApi(EntsoeApi* eapi);
	void setPriceRegion(String);

private:
	RemoteDebug* debugger;
	bool mqttEnabled = false;
	int maxPwr = 0;
	HwTools* hw;
	Timezone* tz;
	EntsoeApi* eapi = NULL;
	AmsConfiguration* config;
	GpioConfig* gpioConfig;
	MeterConfig* meterConfig;
	WebConfig webConfig;
	AmsData* meterState;
	AmsDataStorage* ds;
    EnergyAccounting* ea = NULL;
	MQTTClient* mqtt = NULL;
	bool uploading = false;
	File file;
	bool performRestart = false;
	bool performUpgrade = false;
	bool rebootForUpgrade = false;
	String priceRegion = "";
	#if defined(AMS2MQTT_FIRMWARE_URL)
	String customFirmwareUrl = AMS2MQTT_FIRMWARE_URL;
	#else
	String customFirmwareUrl;
	#endif

    static const uint16_t BufferSize = 2048;
    char* buf;

#if defined(ESP8266)
	ESP8266WebServer server;
#elif defined(ESP32)
	WebServer server;
#endif

	bool checkSecurity(byte level, bool send401 = true);

	void indexHtml();
	void indexJs();
	void indexCss();
	void githubSvg();
	void faviconSvg();

    void sysinfoJson();
    void dataJson();
	void dayplotJson();
	void monthplotJson();
	void energyPriceJson();
	void temperatureJson();
	void tariffJson();

	void configurationJson();
	void handleSave();
	void reboot();
	void upgrade();
	void firmwareHtml();
	void firmwarePost();
	void firmwareUpload();
	void isAliveCheck();

	void mqttCaUpload();
	void mqttCertUpload();
	void mqttKeyUpload();
	HTTPUpload& uploadFile(const char* path);

	void configFileDownload();
	void configFileUpload();
	void factoryResetPost();

	void notFound();
	void redirectToMain();
	void robotstxt();
};

#endif
