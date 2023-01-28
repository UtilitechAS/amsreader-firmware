#ifndef _AMSWEBSERVER_h
#define _AMSWEBSERVER_h

#define BOOTSTRAP_URL "https://cdnjs.cloudflare.com/ajax/libs/twitter-bootstrap/4.4.1/css/bootstrap.min.css"

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
	#if defined(AMS2MQTT_FIRMWARE_URL)
	String customFirmwareUrl = AMS2MQTT_FIRMWARE_URL;
	#else
	String customFirmwareUrl;
	#endif

    static const uint16_t BufferSize = 2048;
    char* buf;

#if defined(ESP8266)
	ESP8266WebServer server;
#elif defined(ESP32) // ARDUINO_ARCH_ESP32
	WebServer server;
#endif

	bool checkSecurity(byte level);

	void indexHtml();
	void applicationJs();
	void temperature();
	void temperaturePost();
	void temperatureJson();
	void configMeterHtml();
	void configMeterAdvancedHtml();
	void configWifiHtml();
	void configMqttHtml();
	void configWebHtml();
	void configDomoticzHtml();
	void configPriceApiHtml();
	void configNtpHtml();
	void configGpioHtml();
	void configDebugHtml();
	void configThresholdsHtml();
	void bootCss();
	void githubSvg();
    void dataJson();
	void dayplotJson();
	void monthplotJson();
	void energyPriceJson();
	void configFileHtml();
	void configFileDownload();
	void configFileUpload();

	void handleSetup();
	void handleSave();

	String getSerialSelectOptions(int selected);
	void firmwareHtml();
	void firmwarePost();
	void firmwareUpload();
	void firmwareDownload();
	void restartHtml();
	void restartPost();
	void restartWaitHtml();
	void isAliveCheck();

	void uploadHtml(const char* label, const char* action, const char* menu);
	void deleteHtml(const char* label, const char* action, const char* menu);
	HTTPUpload& uploadFile(const char* path);
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

	void factoryResetHtml();
	void factoryResetPost();

	void notFound();

	void printD(String fmt, ...);
	void printI(String fmt, ...);
	void printW(String fmt, ...);
	void printE(String fmt, ...);
};

#endif
