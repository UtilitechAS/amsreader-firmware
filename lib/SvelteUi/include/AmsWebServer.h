/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _AMSWEBSERVER_h
#define _AMSWEBSERVER_h

#include "Arduino.h"
#include "AmsMqttHandler.h"
#include "AmsConfiguration.h"
#include "HwTools.h"
#include "AmsData.h"
#include "AmsStorage.h"
#include "AmsDataStorage.h"
#include "EnergyAccounting.h"
#include "Uptime.h"
#include "RemoteDebug.h"
#include "PriceService.h"
#include "RealtimePlot.h"
#include "ConnectionHandler.h"

#if defined(ESP8266)
	#include <ESP8266WiFi.h>
	#include <ESP8266WebServer.h>
	#include <ESP8266HTTPClient.h>
	#include <ESP8266httpUpdate.h>
	#include <ESP8266SSDP.h>
#elif defined(ESP32) // ARDUINO_ARCH_ESP32
	#include <WiFi.h>
	#include <WebServer.h>
	#include <HTTPClient.h>
	#include <HTTPUpdate.h>
	#include <ESP32SSDP.h>
	#if defined(CONFIG_IDF_TARGET_ESP32C3)
	#warning "Cloud disabled"
	#else
	#include "CloudConnector.h"
	#endif
#else
	#warning "Unsupported board type"
#endif

#include "LittleFS.h"

class AmsWebServer {
public:
	AmsWebServer(uint8_t* buf, RemoteDebug* Debug, HwTools* hw, ResetDataContainer* rdc);
    void setup(AmsConfiguration*, GpioConfig*, AmsData*, AmsDataStorage*, EnergyAccounting*, RealtimePlot*);
    void loop();
	#if defined(_CLOUDCONNECTOR_H)
	void setCloud(CloudConnector* cloud);
	#endif
	void setTimezone(Timezone* tz);
	void setMqttEnabled(bool);
	void setPriceService(PriceService* ps);
	void setPriceSettings(String region, String currency);
	void setMeterConfig(uint8_t distributionSystem, uint16_t mainFuse, uint16_t productionCapacity);
	void setMqttHandler(AmsMqttHandler* mqttHandler);
	void setConnectionHandler(ConnectionHandler* ch);

private:
	RemoteDebug* debugger;
	ResetDataContainer* rdc;
	bool mqttEnabled = false;
	int maxPwr = 0;
	uint8_t distributionSystem = 0;
	uint16_t mainFuse = 0, productionCapacity = 0;

	HwTools* hw;
	Timezone* tz;
	PriceService* ps = NULL;
	AmsConfiguration* config;
	GpioConfig* gpioConfig;
	WebConfig webConfig;
	AmsData* meterState;
	AmsDataStorage* ds;
    EnergyAccounting* ea = NULL;
	RealtimePlot* rtp = NULL;
	AmsMqttHandler* mqttHandler = NULL;
	ConnectionHandler* ch = NULL;
	#if defined(_CLOUDCONNECTOR_H)
	CloudConnector* cloud = NULL;
	#endif
	bool uploading = false;
	File file;
	bool performRestart = false;
	bool performUpgrade = false;
	bool rebootForUpgrade = false;
	String priceRegion = "";
	String priceCurrency = "";
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
	void faviconSvg();
	void logoSvg();

    void sysinfoJson();
    void dataJson();
	void dayplotJson();
	void monthplotJson();
	void energyPriceJson();
	void temperatureJson();
	void tariffJson();
	void realtimeJson();
	void priceConfigJson();
	void translationsJson();
	void cloudkeyJson();

	void configurationJson();
	void handleSave();
	void reboot();
	void upgrade();
	void upgradeFromUrl(String url, String nextVersion);
	void firmwareHtml();
	void firmwarePost();
	void firmwareUpload();
	void isAliveCheck();

	void mqttCaUpload();
	void mqttCaDelete();
	void mqttCertUpload();
	void mqttCertDelete();
	void mqttKeyUpload();
	void mqttKeyDelete();
	HTTPUpload& uploadFile(const char* path);
	void deleteFile(const char* path);

	void configFileDownload();
	void configFileUpload();
	void configFilePost();
	void factoryResetPost();

	void modifyDayPlot();
	void modifyMonthPlot();

	void notFound();
	void redirectToMain();
	void robotstxt();
	void ssdpSchema();

	void updaterRequestCallback(HTTPClient*);
};

#endif
