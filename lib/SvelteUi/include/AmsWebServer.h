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
#include "AmsFirmwareUpdater.h"
#include "EnergyAccounting.h"
#include "Uptime.h"
#if defined(AMS_REMOTE_DEBUG)
#include "RemoteDebug.h"
#endif
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
#include <ESP32SSDP.h>
#else
#warning "Unsupported board type"
#endif

#if defined(AMS_CLOUD)
#include "CloudConnector.h"
#endif

#include "LittleFS.h"

class AmsWebServer
{
public:
#if defined(AMS_REMOTE_DEBUG)
	AmsWebServer(uint8_t *buf, RemoteDebug *Debug, HwTools *hw, ResetDataContainer *rdc);
#else
	AmsWebServer(uint8_t *buf, Stream *Debug, HwTools *hw, ResetDataContainer *rdc);
#endif
	void setup(AmsConfiguration *, GpioConfig *, AmsData *, AmsDataStorage *, EnergyAccounting *, RealtimePlot *, AmsFirmwareUpdater *);
	void loop();
#if defined(_CLOUDCONNECTOR_H)
	void setCloud(CloudConnector *cloud);
#endif
	void setTimezone(Timezone *tz);
	void setMqttEnabled(bool);
	void setPriceService(PriceService *ps);
	void setPriceSettings(String region, String currency);
	void setMeterConfig(uint8_t distributionSystem, uint16_t mainFuse, uint16_t productionCapacity);
	void setMqttHandler(AmsMqttHandler *mqttHandler);
	void setConnectionHandler(ConnectionHandler *ch);

private:
#if defined(AMS_REMOTE_DEBUG)
	RemoteDebug *debugger;
#else
	Stream *debugger;
#endif
	ResetDataContainer *rdc;
	bool mqttEnabled = false;
	int maxPwr = 0;
	uint8_t distributionSystem = 0;
	uint16_t mainFuse = 0, productionCapacity = 0;

	HwTools *hw;
	Timezone *tz;
	PriceService *ps = NULL;
	AmsConfiguration *config;
	GpioConfig *gpioConfig;
	WebConfig webConfig;
	AmsData *meterState;
	AmsDataStorage *ds;
	EnergyAccounting *ea = NULL;
	RealtimePlot *rtp = NULL;
	AmsFirmwareUpdater *updater = NULL;
	AmsMqttHandler *mqttHandler = NULL;
	ConnectionHandler *ch = NULL;
#if defined(_CLOUDCONNECTOR_H)
	CloudConnector *cloud = NULL;
#endif
	bool uploading = false;
	File file;
	bool performRestart = false;
	String priceRegion = "";
	String priceCurrency = "";
#if defined(AMS2MQTT_FIRMWARE_URL)
	String customFirmwareUrl = AMS2MQTT_FIRMWARE_URL;
#else
	String customFirmwareUrl;
#endif

	static const uint16_t BufferSize = 2048;
	char *buf;

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
	void neasLogoSvg();
	void wifiHighLightSvg();
	void wifiMediumLightSvg();
	void wifiLowLightSvg();
	void wifiOffSvg();

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

	void wifiScan();

	void configurationJson();
	void handleSave();
	void reboot();
	void upgrade();
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
	HTTPUpload &uploadFile(const char *path);
	void deleteFile(const char *path);

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

	void addConditionalCloudHeaders();
	void optionsGet();
};

#endif
