#include "AmsWebServer.h"
#include "AmsWebHeaders.h"
#include "base64.h"
#include "hexutils.h"

#include "html/index_html.h"
#include "html/index_css.h"
#include "html/index_js.h"
#include "html/github_svg.h"
#include "html/favicon_svg.h"
#include "html/data_json.h"
#include "html/dayplot_json.h"
#include "html/monthplot_json.h"
#include "html/energyprice_json.h"
#include "html/tempsensor_json.h"
#include "html/response_json.h"
#include "html/sysinfo_json.h"
#include "html/tariff_json.h"
#include "html/peak_json.h"
#include "html/conf_general_json.h"
#include "html/conf_meter_json.h"
#include "html/conf_wifi_json.h"
#include "html/conf_net_json.h"
#include "html/conf_mqtt_json.h"
#include "html/conf_price_json.h"
#include "html/conf_thresholds_json.h"
#include "html/conf_debug_json.h"
#include "html/conf_gpio_json.h"
#include "html/conf_domoticz_json.h"
#include "html/conf_ui_json.h"
#include "html/firmware_html.h"

#include "version.h"

#if defined(ESP32)
#include <esp_task_wdt.h>
#include <esp_wifi.h>
#endif


AmsWebServer::AmsWebServer(uint8_t* buf, RemoteDebug* Debug, HwTools* hw) {
	this->debugger = Debug;
	this->hw = hw;
	this->buf = (char*) buf;
}

void AmsWebServer::setup(AmsConfiguration* config, GpioConfig* gpioConfig, MeterConfig* meterConfig, AmsData* meterState, AmsDataStorage* ds, EnergyAccounting* ea) {
    this->config = config;
	this->gpioConfig = gpioConfig;
	this->meterConfig = meterConfig;
	this->meterState = meterState;
	this->ds = ds;
	this->ea = ea;

	server.on(F("/"), HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	snprintf_P(buf, 32, PSTR("/index-%s.js"), VERSION);
	server.on(buf, HTTP_GET, std::bind(&AmsWebServer::indexJs, this));
	snprintf_P(buf, 32, PSTR("/index-%s.css"), VERSION);
	server.on(buf, HTTP_GET, std::bind(&AmsWebServer::indexCss, this));

	server.on(F("/configuration"), HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	server.on(F("/status"), HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	server.on(F("/consent"), HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	server.on(F("/vendor"), HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	server.on(F("/setup"), HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	server.on(F("/mqtt-ca"), HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	server.on(F("/mqtt-cert"), HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	server.on(F("/mqtt-key"), HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	
	server.on(F("/github.svg"), HTTP_GET, std::bind(&AmsWebServer::githubSvg, this)); 
	server.on(F("/favicon.svg"), HTTP_GET, std::bind(&AmsWebServer::faviconSvg, this)); 
	server.on(F("/sysinfo.json"), HTTP_GET, std::bind(&AmsWebServer::sysinfoJson, this));
	server.on(F("/data.json"), HTTP_GET, std::bind(&AmsWebServer::dataJson, this));
	server.on(F("/dayplot.json"), HTTP_GET, std::bind(&AmsWebServer::dayplotJson, this));
	server.on(F("/monthplot.json"), HTTP_GET, std::bind(&AmsWebServer::monthplotJson, this));
	server.on(F("/energyprice.json"), HTTP_GET, std::bind(&AmsWebServer::energyPriceJson, this));
	server.on(F("/temperature.json"), HTTP_GET, std::bind(&AmsWebServer::temperatureJson, this));
	server.on(F("/tariff.json"), HTTP_GET, std::bind(&AmsWebServer::tariffJson, this));

	server.on(F("/configuration.json"), HTTP_GET, std::bind(&AmsWebServer::configurationJson, this));
	server.on(F("/save"), HTTP_POST, std::bind(&AmsWebServer::handleSave, this));
	server.on(F("/reboot"), HTTP_POST, std::bind(&AmsWebServer::reboot, this));
	server.on(F("/upgrade"), HTTP_POST, std::bind(&AmsWebServer::upgrade, this));
	server.on(F("/firmware"), HTTP_GET, std::bind(&AmsWebServer::firmwareHtml, this));
	server.on(F("/firmware"), HTTP_POST, std::bind(&AmsWebServer::firmwarePost, this), std::bind(&AmsWebServer::firmwareUpload, this));
	server.on(F("/is-alive"), HTTP_GET, std::bind(&AmsWebServer::isAliveCheck, this));

	server.on(F("/reset"), HTTP_POST, std::bind(&AmsWebServer::factoryResetPost, this));

	server.on(F("/robots.txt"), HTTP_GET, std::bind(&AmsWebServer::robotstxt, this));

	server.on(F("/mqtt-ca"), HTTP_POST, std::bind(&AmsWebServer::firmwarePost, this), std::bind(&AmsWebServer::mqttCaUpload, this));
	server.on(F("/mqtt-cert"), HTTP_POST, std::bind(&AmsWebServer::firmwarePost, this), std::bind(&AmsWebServer::mqttCertUpload, this));
	server.on(F("/mqtt-key"), HTTP_POST, std::bind(&AmsWebServer::firmwarePost, this), std::bind(&AmsWebServer::mqttKeyUpload, this));

	server.on(F("/configfile"), HTTP_POST, std::bind(&AmsWebServer::firmwarePost, this), std::bind(&AmsWebServer::configFileUpload, this));
	server.on(F("/configfile.cfg"), HTTP_GET, std::bind(&AmsWebServer::configFileDownload, this));

	/* These trigger captive portal. Only problem is that after you have "signed in", the portal is closed and the user has no idea how to reach the device
	server.on(F("/generate_204"), HTTP_GET, std::bind(&AmsWebServer::redirectToMain, this)); // Android captive portal check: http://connectivitycheck.gstatic.com/generate_204
	server.on(F("/ncsi.txt"), HTTP_GET, std::bind(&AmsWebServer::redirectToMain, this)); // Microsoft connectivity check: http://www.msftncsi.com/ncsi.txt 
	server.on(F("/fwlink"), HTTP_GET, std::bind(&AmsWebServer::redirectToMain, this)); // Microsoft connectivity check
	server.on(F("/library/test/success.html"), HTTP_GET, std::bind(&AmsWebServer::redirectToMain, this)); // Apple connectivity check: http://www.apple.com/library/test/success.html
	*/

	server.onNotFound(std::bind(&AmsWebServer::notFound, this));
	
	server.begin(); // Web server start

	config->getWebConfig(webConfig);
	MqttConfig mqttConfig;
	config->getMqttConfig(mqttConfig);
	mqttEnabled = strlen(mqttConfig.host) > 0;
}


void AmsWebServer::setMqtt(MQTTClient* mqtt) {
	this->mqtt = mqtt;
}

void AmsWebServer::setTimezone(Timezone* tz) {
	this->tz = tz;
}

void AmsWebServer::setMqttEnabled(bool enabled) {
	mqttEnabled = enabled;
}

void AmsWebServer::setEntsoeApi(EntsoeApi* eapi) {
	this->eapi = eapi;
}

void AmsWebServer::loop() {
	server.handleClient();

	if(maxPwr == 0 && meterState->getListType() > 1 && meterConfig->mainFuse > 0 && meterConfig->distributionSystem > 0) {
		int voltage = meterConfig->distributionSystem == 2 ? 400 : 230;
		if(meterState->isThreePhase()) {
			maxPwr = meterConfig->mainFuse * sqrt(3) * voltage;
		} else if(meterState->isTwoPhase()) {
			maxPwr = meterConfig->mainFuse * voltage;
		} else {
			maxPwr = meterConfig->mainFuse * 230;
		}
	}
}

bool AmsWebServer::checkSecurity(byte level, bool send401) {
	bool access = WiFi.getMode() == WIFI_AP || webConfig.security < level;
	if(!access && webConfig.security >= level && server.hasHeader("Authorization")) {
		String expectedAuth = String(webConfig.username) + ":" + String(webConfig.password);

		String providedPwd = server.header("Authorization");
		providedPwd.replace("Basic ", "");

		#if defined(ESP8266)
		String expectedBase64 = base64::encode(expectedAuth, false);
		#elif defined(ESP32)
		String expectedBase64 = base64::encode(expectedAuth);
		#endif

		debugger->printf("Expected auth: %s\n", expectedBase64.c_str());
		debugger->printf("Provided auth: %s\n", providedPwd.c_str());

		access = providedPwd.equals(expectedBase64);
	}

	if(!access && send401) {
		server.sendHeader(HEADER_AUTHENTICATE, AUTHENTICATE_BASIC);
		server.setContentLength(0);
		server.send(401, MIME_HTML, "");
	}
	return access;
}

void AmsWebServer::notFound() {
	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);
	server.send_P(404, MIME_HTML, PSTR("Not found"));
}

void AmsWebServer::githubSvg() {
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("Serving /github.svg over http...\n");

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_1HR);
	server.send_P(200, "image/svg+xml", GITHUB_SVG);
}

void AmsWebServer::faviconSvg() {
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("Serving /favicon.ico over http...\n");

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_1HR);
	server.send_P(200, "image/svg+xml", FAVICON_SVG);
}

void AmsWebServer::sysinfoJson() {
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("Serving /sysinfo.json over http...\n");

	SystemConfig sys;
	config->getSystemConfig(sys);

	uint32_t chipId;
	#if defined(ESP32)
		chipId = ( ESP.getEfuseMac() >> 32 ) % 0xFFFFFFFF;
	#else
		chipId = ESP.getChipId();
	#endif
	String chipIdStr = String(chipId, HEX);

	String hostname;
	if(sys.userConfigured) {
		WiFiConfig wifiConfig;
		config->getWiFiConfig(wifiConfig);
		hostname = String(wifiConfig.hostname);
	} else {
		hostname = "ams-"+chipIdStr;
	}

	IPAddress dns1 = WiFi.dnsIP(0);
	IPAddress dns2 = WiFi.dnsIP(1);

    char macStr[18] = { 0 };
    char apMacStr[18] = { 0 };

	uint8_t mac[6];
	uint8_t apmac[6];

	#if defined(ESP8266)
    wifi_get_macaddr(STATION_IF, mac);
    wifi_get_macaddr(SOFTAP_IF, apmac);
	#elif defined(ESP32)
    esp_wifi_get_mac((wifi_interface_t)ESP_IF_WIFI_STA, mac);
    esp_wifi_get_mac((wifi_interface_t)ESP_IF_WIFI_AP, apmac);
	#endif

    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    sprintf(apMacStr, "%02X:%02X:%02X:%02X:%02X:%02X", apmac[0], apmac[1], apmac[2], apmac[3], apmac[4], apmac[5]);

	UiConfig ui;
	config->getUiConfig(ui);

	String meterModel = meterState->getMeterModel();
	if(!meterModel.isEmpty())
		meterModel.replace("\\", "\\\\");

	String meterId = meterState->getMeterId();
	if(!meterId.isEmpty())
		meterId.replace("\\", "\\\\");

	int size = snprintf_P(buf, BufferSize, SYSINFO_JSON,
		VERSION,
		#if defined(CONFIG_IDF_TARGET_ESP32S2)
		"esp32s2",
		#elif defined(CONFIG_IDF_TARGET_ESP32C3)
		"esp32c3",
		#elif defined(CONFIG_FREERTOS_UNICORE)
		"esp32solo",
		#elif defined(ESP32)
		"esp32",
		#elif defined(ESP8266)
		"esp8266",
		#endif
		chipIdStr.c_str(),
		macStr,
		apMacStr,
		sys.boardType,
		sys.vendorConfigured ? "true" : "false",
		sys.userConfigured ? "true" : "false",
		sys.dataCollectionConsent,
		hostname.c_str(),
		performRestart ? "true" : "false",
		rebootForUpgrade ? "true" : "false",
		WiFi.localIP().toString().c_str(),
		WiFi.subnetMask().toString().c_str(),
		WiFi.gatewayIP().toString().c_str(),
		#if defined(ESP8266)
		dns1.isSet() ? dns1.toString().c_str() : "",
		dns2.isSet() ? dns2.toString().c_str() : "",
		#else
		dns1.toString().c_str(),
		dns2.toString().c_str(),
		#endif
		meterState->getMeterType(),
		meterModel.c_str(),
		meterId.c_str(),
		ui.showImport,
		ui.showExport,
		ui.showVoltage,
		ui.showAmperage,
		ui.showReactive,
		ui.showRealtime,
		ui.showPeaks,
		ui.showPricePlot,
		ui.showDayPlot,
		ui.showMonthPlot,
		ui.showTemperaturePlot,
		webConfig.security
	);

	stripNonAscii((uint8_t*) buf, size+1);

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(strlen(buf));
	server.send(200, MIME_JSON, buf);

	server.handleClient();
	delay(250);

	if(performRestart || rebootForUpgrade) {
		if(ds != NULL) {
			ds->save();
		}
		if(debugger->isActive(RemoteDebug::INFO)) debugger->printf(PSTR("Rebooting"));
		delay(1000);
		#if defined(ESP8266)
			ESP.reset();
		#elif defined(ESP32)
			ESP.restart();
		#endif
		performRestart = false;
	}
}

void AmsWebServer::dataJson() {
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("Serving /data.json over http...\n");
	uint64_t millis = millis64();

	if(!checkSecurity(2, true))
		return;

	float vcc = hw->getVcc();
	int rssi = hw->getWifiRssi();

	uint8_t espStatus;
	#if defined(ESP8266)
	if(vcc < 2.0) { // Voltage not correct, ESP would not run on this voltage
		espStatus = 1;
	} else if(vcc > 3.1 && vcc < 3.5) {
		espStatus = 1;
	} else if(vcc > 3.0 && vcc < 3.6) {
		espStatus = 2;
	} else {
		espStatus = 3;
	}
	#elif defined(ESP32)
	if(vcc < 2.0) { // Voltage not correct, ESP would not run on this voltage
		espStatus = 1;
	} else if(vcc > 2.8 && vcc < 3.5) {
		espStatus = 1;
	} else if(vcc > 2.7 && vcc < 3.6) {
		espStatus = 2;
	} else {
		espStatus = 3;
	}
	#endif

	uint8_t hanStatus;
	if(meterState->getLastError() != 0) {
		hanStatus = 3;
	} else if((meterConfig->baud == 0 || meterState->getLastUpdateMillis() == 0) && millis < 30000) {
		hanStatus = 0;
	} else if(millis - meterState->getLastUpdateMillis() < 15000) {
		hanStatus = 1;
	} else if(millis - meterState->getLastUpdateMillis() < 30000) {
		hanStatus = 2;
	} else {
		hanStatus = 3;
	}

	uint8_t wifiStatus;
	if(rssi > -75) {
		wifiStatus = 1;
	} else if(rssi > -95) {
		wifiStatus = 2;
	} else {
		wifiStatus = 3;
	}

	uint8_t mqttStatus;
	if(!mqttEnabled) {
		mqttStatus = 0;
	} else if(mqtt != NULL && mqtt->connected()) {
		mqttStatus = 1;
	} else if(mqtt != NULL && mqtt->lastError() == 0) {
		mqttStatus = 2;
	} else {
		mqttStatus = 3;
	}

	float price = ENTSOE_NO_VALUE;
	if(eapi != NULL)
		price = eapi->getValueForHour(0);

	String peaks = "";
	for(uint8_t i = 1; i <= ea->getConfig()->hours; i++) {
		if(!peaks.isEmpty()) peaks += ",";
		peaks += String(ea->getPeak(i).value / 100.0);
	}

	time_t now = time(nullptr);

	snprintf_P(buf, BufferSize, DATA_JSON,
		maxPwr == 0 ? meterState->isThreePhase() ? 20000 : 10000 : maxPwr,
		meterConfig->productionCapacity,
		meterConfig->mainFuse == 0 ? 40 : meterConfig->mainFuse,
		meterState->getActiveImportPower(),
		meterState->getActiveExportPower(),
		meterState->getReactiveImportPower(),
		meterState->getReactiveExportPower(),
		meterState->getActiveImportCounter(),
		meterState->getActiveExportCounter(),
		meterState->getReactiveImportCounter(),
		meterState->getReactiveExportCounter(),
		meterState->getL1Voltage(),
		meterState->getL2Voltage(),
		meterState->getL3Voltage(),
		meterState->getL1Current(),
		meterState->getL2Current(),
		meterState->getL3Current(),
		meterState->getPowerFactor(),
		meterState->getL1PowerFactor(),
		meterState->getL2PowerFactor(),
		meterState->getL3PowerFactor(),
		vcc,
		rssi,
		hw->getTemperature(),
		(uint32_t) (millis / 1000),
		ESP.getFreeHeap(),
		espStatus,
		hanStatus,
		wifiStatus,
		mqttStatus,
		mqtt == NULL ? 0 : (int) mqtt->lastError(),
		price == ENTSOE_NO_VALUE ? "null" : String(price, 2).c_str(),
		meterState->getMeterType(),
		meterConfig->distributionSystem,
		ea->getMonthMax(),
		peaks.c_str(),
		ea->getCurrentThreshold(),
		ea->getUseThisHour(),
		ea->getCostThisHour(),
		ea->getProducedThisHour(),
		ea->getIncomeThisHour(),
		ea->getUseToday(),
		ea->getCostToday(),
		ea->getProducedToday(),
		ea->getIncomeToday(),
		ea->getUseThisMonth(),
		ea->getCostThisMonth(),
		ea->getProducedThisMonth(),
		ea->getIncomeThisMonth(),
		eapi == NULL ? "" : priceRegion.c_str(),
		meterState->getLastError(),
		eapi == NULL ? 0 : eapi->getLastError(),
		(uint32_t) now,
		checkSecurity(1, false) ? "true" : "false"
	);

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(strlen(buf));
	server.send(200, MIME_JSON, buf);
}

void AmsWebServer::dayplotJson() {
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("Serving /dayplot.json over http...\n");

	if(!checkSecurity(2))
		return;

	if(ds == NULL) {
		notFound();
	} else {
		snprintf_P(buf, BufferSize, DAYPLOT_JSON,
			ds->getHourImport(0) / 1000.0,
			ds->getHourImport(1) / 1000.0,
			ds->getHourImport(2) / 1000.0,
			ds->getHourImport(3) / 1000.0,
			ds->getHourImport(4) / 1000.0,
			ds->getHourImport(5) / 1000.0,
			ds->getHourImport(6) / 1000.0,
			ds->getHourImport(7) / 1000.0,
			ds->getHourImport(8) / 1000.0,
			ds->getHourImport(9) / 1000.0,
			ds->getHourImport(10) / 1000.0,
			ds->getHourImport(11) / 1000.0,
			ds->getHourImport(12) / 1000.0,
			ds->getHourImport(13) / 1000.0,
			ds->getHourImport(14) / 1000.0,
			ds->getHourImport(15) / 1000.0,
			ds->getHourImport(16) / 1000.0,
			ds->getHourImport(17) / 1000.0,
			ds->getHourImport(18) / 1000.0,
			ds->getHourImport(19) / 1000.0,
			ds->getHourImport(20) / 1000.0,
			ds->getHourImport(21) / 1000.0,
			ds->getHourImport(22) / 1000.0,
			ds->getHourImport(23) / 1000.0,
			ds->getHourExport(0) / 1000.0,
			ds->getHourExport(1) / 1000.0,
			ds->getHourExport(2) / 1000.0,
			ds->getHourExport(3) / 1000.0,
			ds->getHourExport(4) / 1000.0,
			ds->getHourExport(5) / 1000.0,
			ds->getHourExport(6) / 1000.0,
			ds->getHourExport(7) / 1000.0,
			ds->getHourExport(8) / 1000.0,
			ds->getHourExport(9) / 1000.0,
			ds->getHourExport(10) / 1000.0,
			ds->getHourExport(11) / 1000.0,
			ds->getHourExport(12) / 1000.0,
			ds->getHourExport(13) / 1000.0,
			ds->getHourExport(14) / 1000.0,
			ds->getHourExport(15) / 1000.0,
			ds->getHourExport(16) / 1000.0,
			ds->getHourExport(17) / 1000.0,
			ds->getHourExport(18) / 1000.0,
			ds->getHourExport(19) / 1000.0,
			ds->getHourExport(20) / 1000.0,
			ds->getHourExport(21) / 1000.0,
			ds->getHourExport(22) / 1000.0,
			ds->getHourExport(23) / 1000.0
		);

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

		server.setContentLength(strlen(buf));
		server.send(200, MIME_JSON, buf);
	}
}

void AmsWebServer::monthplotJson() {
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("Serving /monthplot.json over http...\n");

	if(!checkSecurity(2))
		return;

	if(ds == NULL) {
		notFound();
	} else {
		snprintf_P(buf, BufferSize, MONTHPLOT_JSON,
			ds->getDayImport(1) / 1000.0,
			ds->getDayImport(2) / 1000.0,
			ds->getDayImport(3) / 1000.0,
			ds->getDayImport(4) / 1000.0,
			ds->getDayImport(5) / 1000.0,
			ds->getDayImport(6) / 1000.0,
			ds->getDayImport(7) / 1000.0,
			ds->getDayImport(8) / 1000.0,
			ds->getDayImport(9) / 1000.0,
			ds->getDayImport(10) / 1000.0,
			ds->getDayImport(11) / 1000.0,
			ds->getDayImport(12) / 1000.0,
			ds->getDayImport(13) / 1000.0,
			ds->getDayImport(14) / 1000.0,
			ds->getDayImport(15) / 1000.0,
			ds->getDayImport(16) / 1000.0,
			ds->getDayImport(17) / 1000.0,
			ds->getDayImport(18) / 1000.0,
			ds->getDayImport(19) / 1000.0,
			ds->getDayImport(20) / 1000.0,
			ds->getDayImport(21) / 1000.0,
			ds->getDayImport(22) / 1000.0,
			ds->getDayImport(23) / 1000.0,
			ds->getDayImport(24) / 1000.0,
			ds->getDayImport(25) / 1000.0,
			ds->getDayImport(26) / 1000.0,
			ds->getDayImport(27) / 1000.0,
			ds->getDayImport(28) / 1000.0,
			ds->getDayImport(29) / 1000.0,
			ds->getDayImport(30) / 1000.0,
			ds->getDayImport(31) / 1000.0,
			ds->getDayExport(1) / 1000.0,
			ds->getDayExport(2) / 1000.0,
			ds->getDayExport(3) / 1000.0,
			ds->getDayExport(4) / 1000.0,
			ds->getDayExport(5) / 1000.0,
			ds->getDayExport(6) / 1000.0,
			ds->getDayExport(7) / 1000.0,
			ds->getDayExport(8) / 1000.0,
			ds->getDayExport(9) / 1000.0,
			ds->getDayExport(10) / 1000.0,
			ds->getDayExport(11) / 1000.0,
			ds->getDayExport(12) / 1000.0,
			ds->getDayExport(13) / 1000.0,
			ds->getDayExport(14) / 1000.0,
			ds->getDayExport(15) / 1000.0,
			ds->getDayExport(16) / 1000.0,
			ds->getDayExport(17) / 1000.0,
			ds->getDayExport(18) / 1000.0,
			ds->getDayExport(19) / 1000.0,
			ds->getDayExport(20) / 1000.0,
			ds->getDayExport(21) / 1000.0,
			ds->getDayExport(22) / 1000.0,
			ds->getDayExport(23) / 1000.0,
			ds->getDayExport(24) / 1000.0,
			ds->getDayExport(25) / 1000.0,
			ds->getDayExport(26) / 1000.0,
			ds->getDayExport(27) / 1000.0,
			ds->getDayExport(28) / 1000.0,
			ds->getDayExport(29) / 1000.0,
			ds->getDayExport(30) / 1000.0,
			ds->getDayExport(31) / 1000.0
		);

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

		server.setContentLength(strlen(buf));
		server.send(200, MIME_JSON, buf);
	}
}

void AmsWebServer::energyPriceJson() {
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("Serving /energyprice.json over http...\n");

	if(!checkSecurity(2))
		return;

	float prices[36];
	for(int i = 0; i < 36; i++) {
		prices[i] = eapi == NULL ? ENTSOE_NO_VALUE : eapi->getValueForHour(i);
	}

	snprintf_P(buf, BufferSize, ENERGYPRICE_JSON, 
		eapi == NULL ? "" : eapi->getCurrency(),
		prices[0] == ENTSOE_NO_VALUE ? "null" : String(prices[0], 4).c_str(),
		prices[1] == ENTSOE_NO_VALUE ? "null" : String(prices[1], 4).c_str(),
		prices[2] == ENTSOE_NO_VALUE ? "null" : String(prices[2], 4).c_str(),
		prices[3] == ENTSOE_NO_VALUE ? "null" : String(prices[3], 4).c_str(),
		prices[4] == ENTSOE_NO_VALUE ? "null" : String(prices[4], 4).c_str(),
		prices[5] == ENTSOE_NO_VALUE ? "null" : String(prices[5], 4).c_str(),
		prices[6] == ENTSOE_NO_VALUE ? "null" : String(prices[6], 4).c_str(),
		prices[7] == ENTSOE_NO_VALUE ? "null" : String(prices[7], 4).c_str(),
		prices[8] == ENTSOE_NO_VALUE ? "null" : String(prices[8], 4).c_str(),
		prices[9] == ENTSOE_NO_VALUE ? "null" : String(prices[9], 4).c_str(),
		prices[10] == ENTSOE_NO_VALUE ? "null" : String(prices[10], 4).c_str(),
		prices[11] == ENTSOE_NO_VALUE ? "null" : String(prices[11], 4).c_str(),
		prices[12] == ENTSOE_NO_VALUE ? "null" : String(prices[12], 4).c_str(),
		prices[13] == ENTSOE_NO_VALUE ? "null" : String(prices[13], 4).c_str(),
		prices[14] == ENTSOE_NO_VALUE ? "null" : String(prices[14], 4).c_str(),
		prices[15] == ENTSOE_NO_VALUE ? "null" : String(prices[15], 4).c_str(),
		prices[16] == ENTSOE_NO_VALUE ? "null" : String(prices[16], 4).c_str(),
		prices[17] == ENTSOE_NO_VALUE ? "null" : String(prices[17], 4).c_str(),
		prices[18] == ENTSOE_NO_VALUE ? "null" : String(prices[18], 4).c_str(),
		prices[19] == ENTSOE_NO_VALUE ? "null" : String(prices[19], 4).c_str(),
		prices[20] == ENTSOE_NO_VALUE ? "null" : String(prices[20], 4).c_str(),
		prices[21] == ENTSOE_NO_VALUE ? "null" : String(prices[21], 4).c_str(),
		prices[22] == ENTSOE_NO_VALUE ? "null" : String(prices[22], 4).c_str(),
		prices[23] == ENTSOE_NO_VALUE ? "null" : String(prices[23], 4).c_str(),
		prices[24] == ENTSOE_NO_VALUE ? "null" : String(prices[24], 4).c_str(),
		prices[25] == ENTSOE_NO_VALUE ? "null" : String(prices[25], 4).c_str(),
		prices[26] == ENTSOE_NO_VALUE ? "null" : String(prices[26], 4).c_str(),
		prices[27] == ENTSOE_NO_VALUE ? "null" : String(prices[27], 4).c_str(),
		prices[28] == ENTSOE_NO_VALUE ? "null" : String(prices[28], 4).c_str(),
		prices[29] == ENTSOE_NO_VALUE ? "null" : String(prices[29], 4).c_str(),
		prices[30] == ENTSOE_NO_VALUE ? "null" : String(prices[30], 4).c_str(),
		prices[31] == ENTSOE_NO_VALUE ? "null" : String(prices[31], 4).c_str(),
		prices[32] == ENTSOE_NO_VALUE ? "null" : String(prices[32], 4).c_str(),
		prices[33] == ENTSOE_NO_VALUE ? "null" : String(prices[33], 4).c_str(),
		prices[34] == ENTSOE_NO_VALUE ? "null" : String(prices[34], 4).c_str(),
		prices[35] == ENTSOE_NO_VALUE ? "null" : String(prices[35], 4).c_str()
	);

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(strlen(buf));
	server.send(200, MIME_JSON, buf);
}

void AmsWebServer::temperatureJson() {
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("Serving /temperature.json over http...\n");

	if(!checkSecurity(2))
		return;

	int count = hw->getTempSensorCount();
	snprintf(buf, 16, "{\"c\":%d,\"s\":[", count);

	for(int i = 0; i < count; i++) {
		TempSensorData* data = hw->getTempSensorData(i);
		if(data == NULL) continue;

		TempSensorConfig* conf = config->getTempSensorConfig(data->address);
		char* pos = buf+strlen(buf);
		snprintf_P(pos, 72, TEMPSENSOR_JSON, 
			i,
			toHex(data->address, 8).c_str(),
			conf == NULL ? "" : String(conf->name).substring(0,16).c_str(),
			conf == NULL || conf->common ? 1 : 0,
			data->lastRead
		);
		delay(10);
	}
	char* pos = buf+strlen(buf);
	snprintf(count == 0 ? pos : pos-1, 8, "]}");

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(strlen(buf));
	server.send(200, MIME_JSON, buf);
}

void AmsWebServer::indexHtml() {
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("Serving /index.html over http...\n");

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	if(!checkSecurity(2))
		return;
	server.setContentLength(INDEX_HTML_LEN);
	server.send_P(200, MIME_HTML, INDEX_HTML);
}

void AmsWebServer::indexCss() {
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("Serving /index.css over http...\n");

	if(!checkSecurity(2))
		return;

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_1MO);
	server.setContentLength(INDEX_CSS_LEN);
	server.send_P(200, MIME_CSS, INDEX_CSS);
}

void AmsWebServer::indexJs() {
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("Serving /index.js over http...\n");

	if(!checkSecurity(2))
		return;

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_1MO);
	server.setContentLength(INDEX_JS_LEN);
	server.send_P(200, MIME_JS, INDEX_JS);
}

void AmsWebServer::configurationJson() {
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("Serving /configuration.json over http...\n");

	if(!checkSecurity(1))
		return;

	NtpConfig ntpConfig;
	config->getNtpConfig(ntpConfig);
	WiFiConfig wifiConfig;
	config->getWiFiConfig(wifiConfig);

	bool encen = false;
	for(uint8_t i = 0; i < 16; i++) {
		if(meterConfig->encryptionKey[i] > 0) {
			encen = true;
		}
	}

	EnergyAccountingConfig* eac = ea->getConfig();
	MqttConfig mqttConfig;
	config->getMqttConfig(mqttConfig);

	EntsoeConfig entsoe;
	config->getEntsoeConfig(entsoe);
	DebugConfig debugConfig;
	config->getDebugConfig(debugConfig);
	DomoticzConfig domo;
	config->getDomoticzConfig(domo);
	UiConfig ui;
	config->getUiConfig(ui);

	bool qsc = false;
	bool qsr = false;
	bool qsk = false;

	if(LittleFS.begin()) {
		qsc = LittleFS.exists(FILE_MQTT_CA);
		qsr = LittleFS.exists(FILE_MQTT_CERT);
		qsk = LittleFS.exists(FILE_MQTT_KEY);
		LittleFS.end();
	}

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send_P(200, MIME_JSON, PSTR("{\"version\":\""));
	server.sendContent_P(VERSION);
	server.sendContent_P(PSTR("\","));
	snprintf_P(buf, BufferSize, CONF_GENERAL_JSON,
		ntpConfig.timezone,
		wifiConfig.hostname,
		webConfig.security,
		webConfig.username,
		strlen(webConfig.password) > 0 ? "***" : ""
	);
	server.sendContent(buf);
	snprintf_P(buf, BufferSize, CONF_METER_JSON,
		meterConfig->baud,
		meterConfig->parity,
		meterConfig->invert ? "true" : "false",
		meterConfig->distributionSystem,
		meterConfig->mainFuse,
		meterConfig->productionCapacity,
		encen ? "true" : "false",
		toHex(meterConfig->encryptionKey, 16).c_str(),
		toHex(meterConfig->authenticationKey, 16).c_str(),
		meterConfig->wattageMultiplier > 1 || meterConfig->voltageMultiplier > 1 || meterConfig->amperageMultiplier > 1 || meterConfig->accumulatedMultiplier > 1 ? "true" : "false",
		meterConfig->wattageMultiplier / 1000.0,
		meterConfig->voltageMultiplier / 1000.0,
		meterConfig->amperageMultiplier / 1000.0,
		meterConfig->accumulatedMultiplier / 1000.0
	);
	server.sendContent(buf);

	snprintf_P(buf, BufferSize, CONF_THRESHOLDS_JSON,
		eac->thresholds[0],
		eac->thresholds[1],
		eac->thresholds[2],
		eac->thresholds[3],
		eac->thresholds[4],
		eac->thresholds[5],
		eac->thresholds[6],
		eac->thresholds[7],
		eac->thresholds[8],
		eac->thresholds[9],
		eac->hours
	);
	server.sendContent(buf);
	snprintf_P(buf, BufferSize, CONF_WIFI_JSON,
		wifiConfig.ssid,
		strlen(wifiConfig.psk) > 0 ? "***" : "",
		wifiConfig.power / 10.0,
		wifiConfig.sleep,
		wifiConfig.autoreboot ? "true" : "false"
	);
	server.sendContent(buf);
	snprintf_P(buf, BufferSize, CONF_NET_JSON,
		strlen(wifiConfig.ip) > 0 ? "static" : "dhcp",
		wifiConfig.ip,
		wifiConfig.subnet,
		wifiConfig.gateway,
		wifiConfig.dns1,
		wifiConfig.dns2,
		wifiConfig.mdns ? "true" : "false",
		ntpConfig.server,
		ntpConfig.dhcp ? "true" : "false"
	);
	server.sendContent(buf);
	snprintf_P(buf, BufferSize, CONF_MQTT_JSON,
		mqttConfig.host,
		mqttConfig.port,
		mqttConfig.username,
		strlen(mqttConfig.password) > 0 ? "***" : "",
		mqttConfig.clientId,
		mqttConfig.publishTopic,
		mqttConfig.payloadFormat,
		mqttConfig.ssl ? "true" : "false",
		qsc ? "true" : "false",
		qsr ? "true" : "false",
		qsk ? "true" : "false"
	);
	server.sendContent(buf);
	snprintf_P(buf, BufferSize, CONF_PRICE_JSON,
		entsoe.enabled ? "true" : "false",
		entsoe.token,
		entsoe.area,
		entsoe.currency,
		entsoe.multiplier / 1000.0
	);
	server.sendContent(buf);
	snprintf_P(buf, BufferSize, CONF_DEBUG_JSON,
		debugConfig.serial ? "true" : "false",
		debugConfig.telnet ? "true" : "false",
		debugConfig.level
	);
	server.sendContent(buf);
	snprintf_P(buf, BufferSize, CONF_GPIO_JSON,
		gpioConfig->hanPin == 0xff ? "null" : String(gpioConfig->hanPin, 10).c_str(),
		gpioConfig->apPin == 0xff ? "null" : String(gpioConfig->apPin, 10).c_str(),
		gpioConfig->ledPin == 0xff ? "null" : String(gpioConfig->ledPin, 10).c_str(),
		gpioConfig->ledInverted ? "true" : "false",
		gpioConfig->ledPinRed == 0xff ? "null" : String(gpioConfig->ledPinRed, 10).c_str(),
		gpioConfig->ledPinGreen == 0xff ? "null" : String(gpioConfig->ledPinGreen, 10).c_str(),
		gpioConfig->ledPinBlue == 0xff ? "null" : String(gpioConfig->ledPinBlue, 10).c_str(),
		gpioConfig->ledRgbInverted ? "true" : "false",
		gpioConfig->tempSensorPin == 0xff ? "null" : String(gpioConfig->tempSensorPin, 10).c_str(),
		gpioConfig->tempAnalogSensorPin == 0xff ? "null" : String(gpioConfig->tempAnalogSensorPin, 10).c_str(),
		gpioConfig->vccPin == 0xff ? "null" : String(gpioConfig->vccPin, 10).c_str(),
		gpioConfig->vccOffset / 100.0,
		gpioConfig->vccMultiplier / 1000.0,
		gpioConfig->vccResistorVcc,
		gpioConfig->vccResistorGnd,
		gpioConfig->vccBootLimit / 10.0
	);
	server.sendContent(buf);
	snprintf_P(buf, BufferSize, CONF_UI_JSON,
		ui.showImport,
		ui.showExport,
		ui.showVoltage,
		ui.showAmperage,
		ui.showReactive,
		ui.showRealtime,
		ui.showPeaks,
		ui.showPricePlot,
		ui.showDayPlot,
		ui.showMonthPlot,
		ui.showTemperaturePlot
	);
	server.sendContent(buf);
	snprintf_P(buf, BufferSize, CONF_DOMOTICZ_JSON,
		domo.elidx,
		domo.cl1idx,
		domo.vl1idx,
		domo.vl2idx,
		domo.vl3idx
	);
	server.sendContent(buf);
	server.sendContent("}");
}

void AmsWebServer::handleSave() {
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Handling save method from http"));
	if(!checkSecurity(1))
		return;

	bool success = true;
	if(server.hasArg(F("v")) && server.arg(F("v")) == F("true")) {
		int boardType = server.arg(F("vb")).toInt();
		int hanPin = server.arg(F("vh")).toInt();
		if(server.hasArg(F("vr")) && server.arg(F("vr")) == F("true")) {
			config->clear();
		}

		#if defined(CONFIG_IDF_TARGET_ESP32S2)
			switch(boardType) {
				case 5: // Pow-K+
				case 7: // Pow-U+
				case 6: // Pow-P1
					config->clearGpio(*gpioConfig);
					gpioConfig->hanPin = 16;
					gpioConfig->apPin = 0;
					gpioConfig->ledPinRed = 13;
					gpioConfig->ledPinGreen = 14;
					gpioConfig->ledRgbInverted = true;
					gpioConfig->vccPin = 10;
					gpioConfig->vccResistorGnd = 22;
					gpioConfig->vccResistorVcc = 33;
					break;
				case 51: // Wemos S2 mini
					gpioConfig->ledPin = 15;
					gpioConfig->ledInverted = false;
					gpioConfig->apPin = 0;
				case 50: // Generic ESP32-S2
					gpioConfig->hanPin = hanPin > 0 ? hanPin : 18;
					break;
				default:
					success = false;
			}
		#elif defined(CONFIG_IDF_TARGET_ESP32C3)
		#elif defined(ESP32)
			switch(boardType) {
				case 201: // D32
					gpioConfig->hanPin = hanPin > 0 ? hanPin : 16;
					gpioConfig->apPin = 4;
					gpioConfig->ledPin = 5;
					gpioConfig->ledInverted = true;
					break;
				case 202: // Feather
				case 203: // DevKitC
				case 200: // ESP32
					gpioConfig->hanPin = hanPin > 0 ? hanPin : 16;
					gpioConfig->ledPin = 2;
					gpioConfig->ledInverted = false;
					break;
				default:
					success = false;
			}
		#elif defined(ESP8266)
			switch(boardType) {
				case 2: // spenceme
					config->clearGpio(*gpioConfig);
					gpioConfig->vccBootLimit = 32;
					gpioConfig->hanPin = 3;
					gpioConfig->apPin = 0;
					gpioConfig->ledPin = 2;
					gpioConfig->ledInverted = true;
					gpioConfig->tempSensorPin = 5;
					break;
				case 0: // roarfred
					config->clearGpio(*gpioConfig);
					gpioConfig->hanPin = 3;
					gpioConfig->apPin = 0;
					gpioConfig->ledPin = 2;
					gpioConfig->ledInverted = true;
					gpioConfig->tempSensorPin = 5;
					break;
				case 1: // Arnio Kamstrup
				case 3: // Pow-K UART0
				case 4: // Pow-U UART0
					config->clearGpio(*gpioConfig);
					gpioConfig->hanPin = 3;
					gpioConfig->apPin = 0;
					gpioConfig->ledPin = 2;
					gpioConfig->ledInverted = true;
					gpioConfig->ledPinRed = 13;
					gpioConfig->ledPinGreen = 14;
					gpioConfig->ledRgbInverted = true;
					break;
				case 5: // Pow-K GPIO12
				case 7: // Pow-U GPIO12
					config->clearGpio(*gpioConfig);
					gpioConfig->hanPin = 12;
					gpioConfig->apPin = 0;
					gpioConfig->ledPin = 2;
					gpioConfig->ledInverted = true;
					gpioConfig->ledPinRed = 13;
					gpioConfig->ledPinGreen = 14;
					gpioConfig->ledRgbInverted = true;
					break;
				case 101: // D1
					gpioConfig->hanPin = hanPin > 0 ? hanPin : 5;
					gpioConfig->apPin = 4;
					gpioConfig->ledPin = 2;
					gpioConfig->ledInverted = true;
					gpioConfig->vccMultiplier = 1100;
					break;
				case 100: // ESP8266
					gpioConfig->hanPin = hanPin > 0 ? hanPin : 3;
					gpioConfig->ledPin = 2;
					gpioConfig->ledInverted = true;
					break;
				default:
					success = false;
			}
		#endif
		if(success) {
			config->setGpioConfig(*gpioConfig);

			SystemConfig sys;
			config->getSystemConfig(sys);
			sys.boardType = success ? boardType : 0xFF;
			sys.vendorConfigured = success;
			config->setSystemConfig(sys);
		}
	}

	if(server.hasArg(F("s")) && server.arg(F("s")) == F("true") && server.hasArg(F("ss")) && !server.arg(F("ss")).isEmpty()) {
		SystemConfig sys;
		config->getSystemConfig(sys);

		config->clear();

		WiFiConfig wifi;
		config->clearWifi(wifi);

		strcpy(wifi.ssid, server.arg(F("ss")).c_str());

		String psk = server.arg(F("sp"));
		if(!psk.equals("***")) {
			strcpy(wifi.psk, psk.c_str());
		}
		wifi.mode = 1; // WIFI_STA

		if(server.hasArg(F("sm")) && server.arg(F("sm")) == "static") {
			strcpy(wifi.ip, server.arg(F("si")).c_str());
			strcpy(wifi.gateway, server.arg(F("sg")).c_str());
			strcpy(wifi.subnet, server.arg(F("su")).c_str());
			strcpy(wifi.dns1, server.arg(F("sd")).c_str());
		}

		if(server.hasArg(F("sh")) && !server.arg(F("sh")).isEmpty()) {
			strcpy(wifi.hostname, server.arg(F("sh")).c_str());
			wifi.mdns = true;
		} else {
			wifi.mdns = false;
		}
		
		switch(sys.boardType) {
			case 6: // Pow-P1
				meterConfig->baud = 115200;
				meterConfig->parity = 3; // 8N1
				break;
			case 3: // Pow-K UART0
			case 5: // Pow-K+
				meterConfig->parity = 3; // 8N1
			case 2: // spenceme
			case 50: // Generic ESP32-S2
			case 51: // Wemos S2 mini
				meterConfig->baud = 2400;
				wifi.sleep = 1; // Modem sleep
				break;
			case 4: // Pow-U UART0
			case 7: // Pow-U+
				wifi.sleep = 2; // Light sleep
				break;
		}
		config->setWiFiConfig(wifi);
		config->setMeterConfig(*meterConfig);
		
		sys.userConfigured = success;
		sys.dataCollectionConsent = 0;
		config->setSystemConfig(sys);

		performRestart = true;
	} else if(server.hasArg(F("sf")) && !server.arg(F("sf")).isEmpty()) {
		SystemConfig sys;
		config->getSystemConfig(sys);
		sys.dataCollectionConsent = server.hasArg(F("sf")) && (server.arg(F("sf")) == F("true") || server.arg(F("sf")) == F("1")) ? 1 : 2;
		config->setSystemConfig(sys);
	}

	if(server.hasArg(F("m")) && server.arg(F("m")) == F("true")) {
		if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Received meter config"));
		config->getMeterConfig(*meterConfig);
		meterConfig->baud = server.arg(F("mb")).toInt();
		meterConfig->parity = server.arg(F("mp")).toInt();
		meterConfig->invert = server.hasArg(F("mi")) && server.arg(F("mi")) == F("true");
		meterConfig->distributionSystem = server.arg(F("md")).toInt();
		meterConfig->mainFuse = server.arg(F("mf")).toInt();
		meterConfig->productionCapacity = server.arg(F("mr")).toInt();
		maxPwr = 0;

		String encryptionKeyHex = server.arg(F("mek"));
		if(!encryptionKeyHex.isEmpty()) {
			encryptionKeyHex.replace(F("0x"), F(""));
			fromHex(meterConfig->encryptionKey, encryptionKeyHex, 16);
		}

		String authenticationKeyHex = server.arg(F("mea"));
		if(!authenticationKeyHex.isEmpty()) {
			authenticationKeyHex.replace(F("0x"), F(""));
			fromHex(meterConfig->authenticationKey, authenticationKeyHex, 16);
		}

		meterConfig->wattageMultiplier = server.arg(F("mmw")).toDouble() * 1000;
		meterConfig->voltageMultiplier = server.arg(F("mmv")).toDouble() * 1000;
		meterConfig->amperageMultiplier = server.arg(F("mma")).toDouble() * 1000;
		meterConfig->accumulatedMultiplier = server.arg(F("mmc")).toDouble() * 1000;
		config->setMeterConfig(*meterConfig);
	}

	if(server.hasArg(F("w")) && server.arg(F("w")) == F("true")) {
		if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Received WiFi config"));
		WiFiConfig wifi;
		config->getWiFiConfig(wifi);
		strcpy(wifi.ssid, server.arg(F("ws")).c_str());
		String psk = server.arg(F("wp"));
		if(!psk.equals("***")) {
			strcpy(wifi.psk, psk.c_str());
		}
		wifi.power = server.arg(F("ww")).toFloat() * 10;
		wifi.sleep = server.arg(F("wz")).toInt();
		wifi.autoreboot = server.hasArg(F("wa")) && server.arg(F("wa")) == F("true");
		config->setWiFiConfig(wifi);

		if(server.hasArg(F("nm")) && server.arg(F("nm")) == "static") {
			strcpy(wifi.ip, server.arg(F("ni")).c_str());
			strcpy(wifi.gateway, server.arg(F("ng")).c_str());
			strcpy(wifi.subnet, server.arg(F("ns")).c_str());
			strcpy(wifi.dns1, server.arg(F("nd1")).c_str());
			strcpy(wifi.dns2, server.arg(F("nd2")).c_str());
		}
		wifi.mdns = server.hasArg(F("nd")) && server.arg(F("nd")) == F("true");
		config->setWiFiConfig(wifi);
	}

	if(server.hasArg(F("ntp")) && server.arg(F("ntp")) == F("true")) {
		NtpConfig ntp;
		config->getNtpConfig(ntp);
		ntp.enable = true;
		ntp.dhcp = server.hasArg(F("ntpd")) && server.arg(F("ntpd")) == F("true");
		strcpy(ntp.server, server.arg(F("ntph")).c_str());
		config->setNtpConfig(ntp);
	}

	if(server.hasArg(F("q")) && server.arg(F("q")) == F("true")) {
		if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Received MQTT config"));
		MqttConfig mqtt;
		config->getMqttConfig(mqtt);
		if(server.hasArg(F("qh")) && !server.arg(F("qh")).isEmpty()) {
			strcpy(mqtt.host, server.arg(F("qh")).c_str());
			strcpy(mqtt.clientId, server.arg(F("qc")).c_str());
			strcpy(mqtt.publishTopic, server.arg(F("qb")).c_str());
			strcpy(mqtt.subscribeTopic, server.arg(F("qr")).c_str());
			strcpy(mqtt.username, server.arg(F("qu")).c_str());
			String pass = server.arg(F("qa"));
			if(!pass.equals("***")) {
				strcpy(mqtt.password, pass.c_str());
			}
			mqtt.payloadFormat = server.arg(F("qm")).toInt();
			#if defined(ESP8266)
			mqtt.ssl = false;
			#else
			mqtt.ssl = server.arg(F("qs")) == F("true");
			#endif

			mqtt.port = server.arg(F("qp")).toInt();
			if(mqtt.port == 0) {
				mqtt.port = mqtt.ssl ? 8883 : 1883;
			}
		} else {
			config->clearMqtt(mqtt);
		}
		config->setMqttConfig(mqtt);
	}

	if(server.hasArg(F("o")) && server.arg(F("o")) == F("true")) {
		if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Received Domoticz config"));
		DomoticzConfig domo {
			static_cast<uint16_t>(server.arg(F("oe")).toInt()),
			static_cast<uint16_t>(server.arg(F("ou1")).toInt()),
			static_cast<uint16_t>(server.arg(F("ou2")).toInt()),
			static_cast<uint16_t>(server.arg(F("ou3")).toInt()),
			static_cast<uint16_t>(server.arg(F("oc")).toInt())
		};
		config->setDomoticzConfig(domo);
	}


	if(server.hasArg(F("g")) && server.arg(F("g")) == F("true")) {
		if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Received web config"));
		webConfig.security = server.arg(F("gs")).toInt();
		if(webConfig.security > 0) {
			strcpy(webConfig.username, server.arg(F("gu")).c_str());
			String pass = server.arg(F("gp"));
			if(!pass.equals("***")) {
				strcpy(webConfig.password, pass.c_str());
			}
			debugger->setPassword(webConfig.password);
		} else {
			strcpy_P(webConfig.username, PSTR(""));
			strcpy_P(webConfig.password, PSTR(""));
			debugger->setPassword(F(""));
		}
		config->setWebConfig(webConfig);

		WiFiConfig wifi;
		config->getWiFiConfig(wifi);
		if(server.hasArg(F("gh")) && !server.arg(F("gh")).isEmpty()) {
			strcpy(wifi.hostname, server.arg(F("gh")).c_str());
		}
		config->setWiFiConfig(wifi);

		NtpConfig ntp;
		config->getNtpConfig(ntp);
		strcpy(ntp.timezone, server.arg(F("gt")).c_str());
		config->setNtpConfig(ntp);
	}

	if(server.hasArg(F("i")) && server.arg(F("i")) == F("true")) {
		if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Received GPIO config"));
		gpioConfig->hanPin = server.hasArg(F("ih")) && !server.arg(F("ih")).isEmpty() ? server.arg(F("ih")).toInt() : 3;
		gpioConfig->ledPin = server.hasArg(F("ilp")) && !server.arg(F("ilp")).isEmpty() ? server.arg(F("ilp")).toInt() : 0xFF;
		gpioConfig->ledInverted = server.hasArg(F("ili")) && server.arg(F("ili")) == F("true");
		gpioConfig->ledPinRed = server.hasArg(F("irr")) && !server.arg(F("irr")).isEmpty() ? server.arg(F("irr")).toInt() : 0xFF;
		gpioConfig->ledPinGreen = server.hasArg(F("irg")) && !server.arg(F("irg")).isEmpty() ? server.arg(F("irg")).toInt() : 0xFF;
		gpioConfig->ledPinBlue = server.hasArg(F("irb")) && !server.arg(F("irb")).isEmpty() ? server.arg(F("irb")).toInt() : 0xFF;
		gpioConfig->ledRgbInverted = server.hasArg(F("iri")) && server.arg(F("iri")) == F("true");
		gpioConfig->apPin = server.hasArg(F("ia")) && !server.arg(F("ia")).isEmpty() ? server.arg(F("ia")).toInt() : 0xFF;
		gpioConfig->tempSensorPin = server.hasArg(F("itd")) && !server.arg(F("itd")).isEmpty() ?server.arg(F("itd")).toInt() : 0xFF;
		gpioConfig->tempAnalogSensorPin = server.hasArg(F("ita")) && !server.arg(F("ita")).isEmpty() ?server.arg(F("ita")).toInt() : 0xFF;
		gpioConfig->vccPin = server.hasArg(F("ivp")) && !server.arg(F("ivp")).isEmpty() ? server.arg(F("ivp")).toInt() : 0xFF;
		gpioConfig->vccResistorGnd = server.hasArg(F("ivdg")) && !server.arg(F("ivdg")).isEmpty() ? server.arg(F("ivdg")).toInt() : 0;
		gpioConfig->vccResistorVcc = server.hasArg(F("ivdv")) && !server.arg(F("ivdv")).isEmpty() ? server.arg(F("ivdv")).toInt() : 0;
		config->setGpioConfig(*gpioConfig);
	}

	if(server.hasArg(F("iv")) && server.arg(F("iv")) == F("true")) {
		if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Received Vcc config"));
		gpioConfig->vccOffset = server.hasArg(F("ivo")) && !server.arg(F("ivo")).isEmpty() ? server.arg(F("ivo")).toFloat() * 100 : 0;
		gpioConfig->vccMultiplier = server.hasArg(F("ivm")) && !server.arg(F("ivm")).isEmpty() ? server.arg(F("ivm")).toFloat() * 1000 : 1000;
		gpioConfig->vccBootLimit = server.hasArg(F("ivb")) && !server.arg(F("ivb")).isEmpty() ? server.arg(F("ivb")).toFloat() * 10 : 0;
		config->setGpioConfig(*gpioConfig);
	}

	if(server.hasArg(F("d")) && server.arg(F("d")) == F("true")) {
		if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Received Debug config"));
		DebugConfig debug;
		config->getDebugConfig(debug);
		bool active = debug.serial || debug.telnet;

		debug.telnet = server.hasArg(F("dt")) && server.arg(F("dt")) == F("true");
		debug.serial = server.hasArg(F("ds")) && server.arg(F("ds")) == F("true");
		debug.level = server.arg(F("dl")).toInt();

		if(debug.telnet || debug.serial) {
			if(webConfig.security > 0) {
				debugger->setPassword(webConfig.password);
			} else {
				debugger->setPassword(F(""));
			}
			debugger->setSerialEnabled(debug.serial);
			WiFiConfig wifi;
			if(config->getWiFiConfig(wifi) && strlen(wifi.hostname) > 0) {
				debugger->begin(wifi.hostname, (uint8_t) debug.level);
				if(!debug.telnet) {
					debugger->stop();
				}
			}
		} else if(active) {
			performRestart = true;
		}
		config->setDebugConfig(debug);
	}

	if(server.hasArg(F("u")) && server.arg(F("u")) == F("true")) {
		UiConfig ui;
		config->getUiConfig(ui);
		ui.showImport = server.arg(F("ui")).toInt();
		ui.showExport = server.arg(F("ue")).toInt();
		ui.showVoltage = server.arg(F("uv")).toInt();
		ui.showAmperage = server.arg(F("ua")).toInt();
		ui.showReactive = server.arg(F("ur")).toInt();
		ui.showRealtime = server.arg(F("uc")).toInt();
		ui.showPeaks = server.arg(F("ut")).toInt();
		ui.showPricePlot = server.arg(F("up")).toInt();
		ui.showDayPlot = server.arg(F("ud")).toInt();
		ui.showMonthPlot = server.arg(F("um")).toInt();
		ui.showTemperaturePlot = server.arg(F("us")).toInt();
		config->setUiConfig(ui);
	}

	if(server.hasArg(F("p")) && server.arg(F("p")) == F("true")) {
		if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Received price API config"));

		priceRegion = server.arg(F("pr"));

		EntsoeConfig entsoe;
		entsoe.enabled = server.hasArg(F("pe")) && server.arg(F("pe")) == F("true");
		strcpy(entsoe.token, server.arg(F("pt")).c_str());
		strcpy(entsoe.area, priceRegion.c_str());
		strcpy(entsoe.currency, server.arg(F("pc")).c_str());
		entsoe.multiplier = server.arg(F("pm")).toFloat() * 1000;
		config->setEntsoeConfig(entsoe);
	}

	if(server.hasArg(F("t")) && server.arg(F("t")) == F("true")) {
		if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Received energy accounting config"));
		EnergyAccountingConfig eac;
		eac.thresholds[0] = server.arg(F("t0")).toInt();
		eac.thresholds[1] = server.arg(F("t1")).toInt();
		eac.thresholds[2] = server.arg(F("t2")).toInt();
		eac.thresholds[3] = server.arg(F("t3")).toInt();
		eac.thresholds[4] = server.arg(F("t4")).toInt();
		eac.thresholds[5] = server.arg(F("t5")).toInt();
		eac.thresholds[6] = server.arg(F("t6")).toInt();
		eac.thresholds[7] = server.arg(F("t7")).toInt();
		eac.thresholds[8] = server.arg(F("t8")).toInt();
		eac.hours = server.arg(F("th")).toInt();
		config->setEnergyAccountingConfig(eac);
	}

	if(debugger->isActive(RemoteDebug::INFO)) debugger->printf(PSTR("Saving configuration now..."));

	if (config->save()) {
		if(debugger->isActive(RemoteDebug::INFO)) debugger->printf(PSTR("Successfully saved."));
		if(config->isWifiChanged() || performRestart) {
			performRestart = true;
		} else {
			hw->setup(gpioConfig, config);
		}
	} else {
		success = false;
	}

	snprintf_P(buf, BufferSize, RESPONSE_JSON,
		success ? "true" : "false",
		"",
		performRestart ? "true" : "false"
	);
	server.setContentLength(strlen(buf));
	server.send(200, MIME_JSON, buf);

	server.handleClient();
	delay(250);

	if(performRestart || rebootForUpgrade) {
		if(ds != NULL) {
			ds->save();
		}
		if(debugger->isActive(RemoteDebug::INFO)) debugger->printf(PSTR("Rebooting"));
		delay(1000);
		#if defined(ESP8266)
			ESP.reset();
		#elif defined(ESP32)
			ESP.restart();
		#endif
		performRestart = false;
	}
}

void AmsWebServer::reboot() {
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("Serving /reboot over http...\n");

	if(!checkSecurity(1))
		return;

	server.send(200, MIME_JSON, "{\"reboot\":true}");

	server.handleClient();
	delay(250);

	if(debugger->isActive(RemoteDebug::INFO)) debugger->printf(PSTR("Rebooting"));
	delay(1000);
	#if defined(ESP8266)
		ESP.reset();
	#elif defined(ESP32)
		ESP.restart();
	#endif
	performRestart = false;
}

void AmsWebServer::upgrade() {
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("Serving /upgrade over http...\n");

	if(!checkSecurity(1))
		return;

	SystemConfig sys;
	config->getSystemConfig(sys);

	snprintf_P(buf, BufferSize, RESPONSE_JSON,
		sys.dataCollectionConsent == 1 ? "true" : "false",
		"",
		sys.dataCollectionConsent == 1 ? "true" : "false"
	);
	server.setContentLength(strlen(buf));
	server.send(200, MIME_JSON, buf);

	if(sys.dataCollectionConsent == 1) {
		server.handleClient();
		delay(250);

		if(server.hasArg(F("url"))) {
			customFirmwareUrl = server.arg(F("url"));
		}

		String url = customFirmwareUrl.isEmpty() || !customFirmwareUrl.startsWith(F("http")) ? F("http://hub.amsleser.no/hub/firmware/update") : customFirmwareUrl;

		if(server.hasArg(F("version"))) {
			url += "/" + server.arg(F("version"));
		}

		WiFiClient client;
		#if defined(ESP8266)
			String chipType = F("esp8266");
		#elif defined(CONFIG_IDF_TARGET_ESP32S2)
			String chipType = F("esp32s2");
		#elif defined(ESP32)
			#if defined(CONFIG_FREERTOS_UNICORE)
				String chipType = F("esp32solo");
			#else
				String chipType = F("esp32");
			#endif
		#endif

		#if defined(ESP8266)
			ESPhttpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
			t_httpUpdate_return ret = ESPhttpUpdate.update(client, url, VERSION);
		#elif defined(ESP32)
			HTTPUpdate httpUpdate;
			httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
			HTTPUpdateResult ret = httpUpdate.update(client, url, String(VERSION) + "-" + chipType);
		#endif

		switch(ret) {
			case HTTP_UPDATE_FAILED:
				debugger->printf(PSTR("Update failed"));
				break;
			case HTTP_UPDATE_NO_UPDATES:
				debugger->printf(PSTR("No Update"));
				break;
			case HTTP_UPDATE_OK:
				debugger->printf(PSTR("Update OK"));
				break;
		}
	}
}
void AmsWebServer::firmwareHtml() {
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Serving /firmware.html over http..."));

	if(!checkSecurity(1))
		return;

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(FIRMWARE_HTML_LEN);
	server.send_P(200, MIME_HTML, FIRMWARE_HTML);
}

void AmsWebServer::firmwarePost() {
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Handling firmware post..."));
	if(!checkSecurity(1))
		return;
	
	if(rebootForUpgrade) {
		server.send(200);
	} else {
		if(server.hasArg(F("url"))) {
			String url = server.arg(F("url"));
			if(!url.isEmpty() && (url.startsWith(F("http://")) || url.startsWith(F("https://")))) {
				if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Custom firmware URL was provided"));

				WiFiClient client;
				#if defined(ESP8266)
					String chipType = F("esp8266");
				#elif defined(CONFIG_IDF_TARGET_ESP32S2)
					String chipType = F("esp32s2");
				#elif defined(ESP32)
					#if defined(CONFIG_FREERTOS_UNICORE)
						String chipType = F("esp32solo");
					#else
						String chipType = F("esp32");
					#endif
				#endif

				#if defined(ESP8266)
					ESPhttpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
					t_httpUpdate_return ret = ESPhttpUpdate.update(client, url, VERSION);
				#elif defined(ESP32)
					HTTPUpdate httpUpdate;
					httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
					HTTPUpdateResult ret = httpUpdate.update(client, url, String(VERSION) + "-" + chipType);
				#endif

				switch(ret) {
					case HTTP_UPDATE_FAILED:
						debugger->printf(PSTR("Update failed"));
						break;
					case HTTP_UPDATE_NO_UPDATES:
						debugger->printf(PSTR("No Update"));
						break;
					case HTTP_UPDATE_OK:
						debugger->printf(PSTR("Update OK"));
						break;
				}			
				server.send(200, MIME_PLAIN, "OK");
				return;
			}
		}
		server.sendHeader(HEADER_LOCATION,F("/firmware"));
		server.send(303);
	}
}


void AmsWebServer::firmwareUpload() {
	if(!checkSecurity(1))
		return;

	HTTPUpload& upload = server.upload();
	if(upload.status == UPLOAD_FILE_START) {
        String filename = upload.filename;
		if(filename.isEmpty()) {
			if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf(PSTR("No file, falling back to post\n"));
			return;
		}
        if(!filename.endsWith(".bin")) {
            server.send(500, MIME_PLAIN, "500: couldn't create file");
		} else {
			#if defined(ESP32)
				esp_task_wdt_delete(NULL);
				esp_task_wdt_deinit();
			#elif defined(ESP8266)
				ESP.wdtDisable();
			#endif
		}
	}
	uploadFile(FILE_FIRMWARE);
	if(upload.status == UPLOAD_FILE_END) {
		rebootForUpgrade = true;
		server.sendHeader("Location","/");
		server.send(302);
	}
}

HTTPUpload& AmsWebServer::uploadFile(const char* path) {
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_START){
		if(uploading) {
			if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf(PSTR("Upload already in progress\n"));
			server.send_P(500, MIME_HTML, PSTR("<html><body><h1>Upload already in progress!</h1></body></html>"));
		} else if (!LittleFS.begin()) {
			if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf(PSTR("An Error has occurred while mounting LittleFS\n"));
			server.send_P(500, MIME_HTML, PSTR("<html><body><h1>Unable to mount LittleFS!</h1></body></html>"));
		} else {
			uploading = true;
			if(debugger->isActive(RemoteDebug::DEBUG)) {
				debugger->printf_P(PSTR("handleFileUpload file: %s\n"), path);
			}
			if(LittleFS.exists(path)) {
				LittleFS.remove(path);
			}
		    file = LittleFS.open(path, "w");
			if(debugger->isActive(RemoteDebug::DEBUG)) {
				debugger->printf_P(PSTR("handleFileUpload Open file and write: %u\n"), upload.currentSize);
			}
            size_t written = file.write(upload.buf, upload.currentSize);
			if(debugger->isActive(RemoteDebug::DEBUG)) {
				debugger->printf_P(PSTR("handleFileUpload Written: %u\n"), written);
			}
	    } 
    } else if(upload.status == UPLOAD_FILE_WRITE) {
		if(debugger->isActive(RemoteDebug::DEBUG)) {
			debugger->printf_P(PSTR("handleFileUpload Writing: %u\n"), upload.currentSize);
		}
        if(file) {
            size_t written = file.write(upload.buf, upload.currentSize);
			if(debugger->isActive(RemoteDebug::DEBUG)) {
				debugger->printf_P(PSTR("handleFileUpload Written: %u\n"), written);
			}
			delay(1);
			if(written != upload.currentSize) {
				file.flush();
				file.close();
				LittleFS.remove(path);
				LittleFS.end();

				if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf(PSTR("An Error has occurred while writing file"));
				snprintf_P(buf, BufferSize, RESPONSE_JSON,
					"false",
					"File size does not match",
					"false"
				);
				server.setContentLength(strlen(buf));
				server.send(500, MIME_JSON, buf);
			}
		}
    } else if(upload.status == UPLOAD_FILE_END) {
		if(debugger->isActive(RemoteDebug::DEBUG)) {
			debugger->printf_P(PSTR("handleFileUpload Ended\n"));
		}
        if(file) {
			file.flush();
            file.close();
//			LittleFS.end();
        } else {
			debugger->printf_P(PSTR("File was not valid in the end... Write error: %d, \n"), file.getWriteError());
			snprintf_P(buf, BufferSize, RESPONSE_JSON,
				"false",
				"Upload ended, but file is missing",
				"false"
			);
			server.setContentLength(strlen(buf));
			server.send(500, MIME_JSON, buf);
        }
    }
	return upload;
}

void AmsWebServer::isAliveCheck() {
	server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
	server.send(200);
}

void AmsWebServer::factoryResetPost() {
	if(!checkSecurity(1))
		return;

	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Performing factory reset"));

	bool success = false;
	if(server.hasArg(F("perform")) && server.arg(F("perform")) == F("true")) {
		if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Formatting LittleFS"));
		LittleFS.format();
		if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Clearing configuration"));
		config->clear();

		success = true;
	}

	snprintf_P(buf, BufferSize, RESPONSE_JSON,
		success ? "true" : "false",
		"",
		"true"
	);
	server.setContentLength(strlen(buf));
	server.send(200, MIME_JSON, buf);

	server.handleClient();
	delay(250);

	if(debugger->isActive(RemoteDebug::INFO)) debugger->printf(PSTR("Rebooting"));
	delay(1000);
	#if defined(ESP8266)
		ESP.reset();
	#elif defined(ESP32)
		ESP.restart();
	#endif
}

void AmsWebServer::robotstxt() {
	server.send_P(200, MIME_HTML, PSTR("User-agent: *\nDisallow: /\n"));
}

void AmsWebServer::mqttCaUpload() {
	if(!checkSecurity(1))
		return;

	uploadFile(FILE_MQTT_CA);
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_END) {
		server.sendHeader(HEADER_LOCATION,F("/configuration"));
		server.send(303);

		MqttConfig mqttConfig;
		if(config->getMqttConfig(mqttConfig) && mqttConfig.ssl) {
			config->setMqttChanged();
		}
	}
}

void AmsWebServer::mqttCertUpload() {
	if(!checkSecurity(1))
		return;

	uploadFile(FILE_MQTT_CERT);
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_END) {
		server.sendHeader(HEADER_LOCATION,F("/configuration"));
		server.send(303);
		MqttConfig mqttConfig;
		if(config->getMqttConfig(mqttConfig) && mqttConfig.ssl) {
			config->setMqttChanged();
		}
	}
}

void AmsWebServer::mqttKeyUpload() {
	if(!checkSecurity(1))
		return;

	uploadFile(FILE_MQTT_KEY);
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_END) {
		server.sendHeader(HEADER_LOCATION,F("/configuration"));
		server.send(303);
		MqttConfig mqttConfig;
		if(config->getMqttConfig(mqttConfig) && mqttConfig.ssl) {
			config->setMqttChanged();
		}
	}
}

void AmsWebServer::tariffJson() {
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("Serving /tariff.json over http...\n");

	if(!checkSecurity(2))
		return;

	EnergyAccountingConfig* eac = ea->getConfig();

	String peaks;
    for(uint8_t x = 0;x < min((uint8_t) 5, eac->hours); x++) {
		EnergyAccountingPeak peak = ea->getPeak(x+1);
		int len = snprintf_P(buf, BufferSize, PEAK_JSON,
			peak.day,
			peak.value / 100.0
		);
		buf[len] = '\0';
		if(!peaks.isEmpty()) peaks += ",";
		peaks += String(buf);
	}

	snprintf_P(buf, BufferSize, TARIFF_JSON,
		eac->thresholds[0],
		eac->thresholds[1],
		eac->thresholds[2],
		eac->thresholds[3],
		eac->thresholds[4],
		eac->thresholds[5],
		eac->thresholds[6],
		eac->thresholds[7],
		eac->thresholds[8],
		eac->thresholds[9],
		peaks.c_str(),
		ea->getCurrentThreshold(),
		ea->getMonthMax()
	);

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(strlen(buf));
	server.send(200, MIME_JSON, buf);
}

void AmsWebServer::setPriceRegion(String priceRegion) {
	this->priceRegion = priceRegion;
}

void AmsWebServer::configFileDownload() {
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("Serving /configfile.cfg over http...\n");

	if(!checkSecurity(1))
		return;

	bool includeSecrets = server.hasArg(F("ic")) && server.arg(F("ic")) == F("true");
	bool includeWifi = server.hasArg(F("iw")) && server.arg(F("iw")) == F("true");
	bool includeMqtt = server.hasArg(F("im")) && server.arg(F("im")) == F("true");
	bool includeWeb = server.hasArg(F("ie")) && server.arg(F("ie")) == F("true");
	bool includeMeter = server.hasArg(F("it")) && server.arg(F("it")) == F("true");
	bool includeGpio = server.hasArg(F("ig")) && server.arg(F("ig")) == F("true");
	bool includeDomo = server.hasArg(F("id")) && server.arg(F("id")) == F("true");
	bool includeNtp = server.hasArg(F("in")) && server.arg(F("in")) == F("true");
	bool includeEntsoe = server.hasArg(F("is")) && server.arg(F("is")) == F("true");
	bool includeThresholds = server.hasArg(F("ih")) && server.arg(F("ih")) == F("true");
	
	SystemConfig sys;
	config->getSystemConfig(sys);

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);
	server.sendHeader(F("Content-Disposition"), F("attachment; filename=configfile.cfg"));
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);

	server.send_P(200, MIME_PLAIN, PSTR("amsconfig\n"));
	server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("version %s\n"), VERSION));
	server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("boardType %d\n"), sys.boardType));
	
	if(includeWifi) {
		WiFiConfig wifi;
		config->getWiFiConfig(wifi);
		if(includeSecrets) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("hostname %s\n"), wifi.hostname));
		if(includeSecrets) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("ssid %s\n"), wifi.ssid));
		if(includeSecrets) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("psk %s\n"), wifi.psk));
		if(strlen(wifi.ip) > 0) {
			server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("ip %s\n"), wifi.ip));
			if(strlen(wifi.gateway) > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gateway %s\n"), wifi.gateway));
			if(strlen(wifi.subnet) > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("subnet %s\n"), wifi.subnet));
			if(strlen(wifi.dns1) > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("dns1 %s\n"), wifi.dns1));
			if(strlen(wifi.dns2) > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("dns2 %s\n"), wifi.dns2));
		}
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("mdns %d\n"), wifi.mdns ? 1 : 0));
	}
	
	if(includeMqtt) {
		MqttConfig mqtt;
		config->getMqttConfig(mqtt);
		if(strlen(mqtt.host) > 0) {
			server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("mqttHost %s\n"), mqtt.host));
			if(mqtt.port > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("mqttPort %d\n"), mqtt.port));
			if(strlen(mqtt.clientId) > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("mqttClientId %s\n"), mqtt.clientId));
			if(strlen(mqtt.publishTopic) > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("mqttPublishTopic %s\n"), mqtt.publishTopic));
			if(includeSecrets) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("mqttUsername %s\n"), mqtt.username));
			if(includeSecrets) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("mqttPassword %s\n"), mqtt.password));
			server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("mqttPayloadFormat %d\n"), mqtt.payloadFormat));
			server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("mqttSsl %d\n"), mqtt.ssl ? 1 : 0));
		}
	}

	if(includeWeb && includeSecrets) {
		WebConfig web;
		config->getWebConfig(web);
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("webSecurity %d\n"), web.security));
		if(web.security > 0) {
			server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("webUsername %s\n"), web.username));
			server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("webPassword %s\n"), web.password));
		}
	}

	if(includeMeter) {
		MeterConfig meter;
		config->getMeterConfig(meter);
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("meterBaud %d\n"), meter.baud));
		char parity[4] = "";
		switch(meter.parity) {
			case 2:
				strcpy_P(parity, PSTR("7N1"));
				break;
			case 3:
				strcpy_P(parity, PSTR("8N1"));
				break;
			case 10:
				strcpy_P(parity, PSTR("7E1"));
				break;
			case 11:
				strcpy_P(parity, PSTR("8E1"));
				break;
		}
		if(strlen(parity) > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("meterParity %s\n"), parity));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("meterInvert %d\n"), meter.invert ? 1 : 0));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("meterDistributionSystem %d\n"), meter.distributionSystem));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("meterMainFuse %d\n"), meter.mainFuse));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("meterProductionCapacity %d\n"), meter.productionCapacity));
		if(includeSecrets) {
			if(meter.encryptionKey[0] != 0x00) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("meterEncryptionKey %s\n"), toHex(meter.encryptionKey, 16).c_str()));
			if(meter.authenticationKey[0] != 0x00) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("meterAuthenticationKey %s\n"), toHex(meter.authenticationKey, 16).c_str()));
		}
	}

	if(includeGpio) {
		GpioConfig gpio;
		config->getGpioConfig(gpio);
		if(gpio.hanPin != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioHanPin %d\n"), gpio.hanPin));
		if(gpio.apPin != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioApPin %d\n"), gpio.apPin));
		if(gpio.ledPin != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioLedPin %d\n"), gpio.ledPin));
		if(gpio.ledPin != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioLedInverted %d\n"), gpio.ledInverted ? 1 : 0));
		if(gpio.ledPinRed != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioLedPinRed %d\n"), gpio.ledPinRed));
		if(gpio.ledPinGreen != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioLedPinGreen %d\n"), gpio.ledPinGreen));
		if(gpio.ledPinBlue != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioLedPinBlue %d\n"), gpio.ledPinBlue));
		if(gpio.ledPinRed != 0xFF || gpio.ledPinGreen != 0xFF || gpio.ledPinBlue != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioLedRgbInverted %d\n"), gpio.ledRgbInverted ? 1 : 0));
		if(gpio.tempSensorPin != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioTempSensorPin %d\n"), gpio.tempSensorPin));
		if(gpio.tempAnalogSensorPin != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioTempAnalogSensorPin %d\n"), gpio.tempAnalogSensorPin));
		if(gpio.vccPin != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioVccPin %d\n"), gpio.vccPin));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioVccOffset %.2f\n"), gpio.vccOffset / 100.0));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioVccMultiplier %.3f\n"), gpio.vccMultiplier / 1000.0));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioVccBootLimit %.1f\n"), gpio.vccBootLimit / 10.0));
		if(gpio.vccPin != 0xFF && gpio.vccResistorGnd != 0) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioVccResistorGnd %d\n"), gpio.vccResistorGnd));
		if(gpio.vccPin != 0xFF && gpio.vccResistorVcc != 0) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioVccResistorVcc %d\n"), gpio.vccResistorVcc));
	}

	if(includeDomo) {
		DomoticzConfig domo;
		config->getDomoticzConfig(domo);
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("domoticzElidx %d\n"), domo.elidx));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("domoticzVl1idx %d\n"), domo.vl1idx));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("domoticzVl2idx %d\n"), domo.vl2idx));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("domoticzVl3idx %d\n"), domo.vl3idx));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("domoticzCl1idx %d\n"), domo.cl1idx));
	}

	if(includeNtp) {
		NtpConfig ntp;
		config->getNtpConfig(ntp);
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("ntpEnable %d\n"), ntp.enable ? 1 : 0));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("ntpDhcp %d\n"), ntp.dhcp ? 1 : 0));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("ntpTimezone %s\n"), ntp.timezone));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("ntpServer %s\n"), ntp.server));
	}

	if(includeEntsoe) {
		EntsoeConfig entsoe;
		config->getEntsoeConfig(entsoe);
		if(strlen(entsoe.token) == 36 && includeSecrets) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("entsoeToken %s\n"), entsoe.token));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("entsoeArea %s\n"), entsoe.area));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("entsoeCurrency %s\n"), entsoe.currency));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("entsoeMultiplier %.3f\n"), entsoe.multiplier / 1000.0));
	}

	if(includeThresholds) {
		EnergyAccountingConfig eac;
		config->getEnergyAccountingConfig(eac);

		if(eac.thresholds[9] > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("thresholds %d %d %d %d %d %d %d %d %d %d %d\n"), 
			eac.thresholds[0],
			eac.thresholds[1],
			eac.thresholds[2],
			eac.thresholds[3],
			eac.thresholds[4],
			eac.thresholds[5],
			eac.thresholds[6],
			eac.thresholds[7],
			eac.thresholds[8],
			eac.thresholds[9],
			eac.hours
		));
	}


	if(ds != NULL) {
		DayDataPoints day = ds->getDayData();
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("dayplot %d %lu %lu %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d"), 
			day.version,
			(int32_t) day.lastMeterReadTime,
			day.activeImport,
			day.accuracy,
			ds->getHourImport(0),
			ds->getHourImport(1),
			ds->getHourImport(2),
			ds->getHourImport(3),
			ds->getHourImport(4),
			ds->getHourImport(5),
			ds->getHourImport(6),
			ds->getHourImport(7),
			ds->getHourImport(8),
			ds->getHourImport(9),
			ds->getHourImport(10),
			ds->getHourImport(11),
			ds->getHourImport(12),
			ds->getHourImport(13),
			ds->getHourImport(14),
			ds->getHourImport(15),
			ds->getHourImport(16),
			ds->getHourImport(17),
			ds->getHourImport(18),
			ds->getHourImport(19),
			ds->getHourImport(20),
			ds->getHourImport(21),
			ds->getHourImport(22),
			ds->getHourImport(23)
		));
		if(day.activeExport > 0) {
			server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR(" %u %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n"), 
				day.activeExport,
				ds->getHourExport(0),
				ds->getHourExport(1),
				ds->getHourExport(2),
				ds->getHourExport(3),
				ds->getHourExport(4),
				ds->getHourExport(5),
				ds->getHourExport(6),
				ds->getHourExport(7),
				ds->getHourExport(8),
				ds->getHourExport(9),
				ds->getHourExport(10),
				ds->getHourExport(11),
				ds->getHourExport(12),
				ds->getHourExport(13),
				ds->getHourExport(14),
				ds->getHourExport(15),
				ds->getHourExport(16),
				ds->getHourExport(17),
				ds->getHourExport(18),
				ds->getHourExport(19),
				ds->getHourExport(20),
				ds->getHourExport(21),
				ds->getHourExport(22),
				ds->getHourExport(23)
			));
		} else {
			server.sendContent(F("\n"));
		}

		MonthDataPoints month = ds->getMonthData();
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("monthplot %d %lu %lu %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d"), 
			month.version,
			(int32_t) month.lastMeterReadTime,
			month.activeImport,
			month.accuracy,
			ds->getDayImport(1),
			ds->getDayImport(2),
			ds->getDayImport(3),
			ds->getDayImport(4),
			ds->getDayImport(5),
			ds->getDayImport(6),
			ds->getDayImport(7),
			ds->getDayImport(8),
			ds->getDayImport(9),
			ds->getDayImport(10),
			ds->getDayImport(11),
			ds->getDayImport(12),
			ds->getDayImport(13),
			ds->getDayImport(14),
			ds->getDayImport(15),
			ds->getDayImport(16),
			ds->getDayImport(17),
			ds->getDayImport(18),
			ds->getDayImport(19),
			ds->getDayImport(20),
			ds->getDayImport(21),
			ds->getDayImport(22),
			ds->getDayImport(23),
			ds->getDayImport(24),
			ds->getDayImport(25),
			ds->getDayImport(26),
			ds->getDayImport(27),
			ds->getDayImport(28),
			ds->getDayImport(29),
			ds->getDayImport(30),
			ds->getDayImport(31)
		));
		if(month.activeExport > 0) {
			server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR(" %u %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n"), 
				month.activeExport,
				ds->getDayExport(1),
				ds->getDayExport(2),
				ds->getDayExport(3),
				ds->getDayExport(4),
				ds->getDayExport(5),
				ds->getDayExport(6),
				ds->getDayExport(7),
				ds->getDayExport(8),
				ds->getDayExport(9),
				ds->getDayExport(10),
				ds->getDayExport(11),
				ds->getDayExport(12),
				ds->getDayExport(13),
				ds->getDayExport(14),
				ds->getDayExport(15),
				ds->getDayExport(16),
				ds->getDayExport(17),
				ds->getDayExport(18),
				ds->getDayExport(19),
				ds->getDayExport(20),
				ds->getDayExport(21),
				ds->getDayExport(22),
				ds->getDayExport(23),
				ds->getDayExport(24),
				ds->getDayExport(25),
				ds->getDayExport(26),
				ds->getDayExport(27),
				ds->getDayExport(28),
				ds->getDayExport(29),
				ds->getDayExport(30),
				ds->getDayExport(31)
			));
		} else {
			server.sendContent(F("\n"));
		}
	}

	if(ea != NULL) {
		EnergyAccountingConfig eac;
		config->getEnergyAccountingConfig(eac);
		EnergyAccountingData ead = ea->getData();
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("energyaccounting %d %d %.2f %d %d %.2f %d %d %d %.2f %d %.2f %d %.2f %d %.2f %d %.2f"), 
			ead.version,
			ead.month,
			ead.costYesterday / 10.0,
			ead.costThisMonth,
			ead.costLastMonth,
			ead.incomeYesterday / 10.0,
			ead.incomeThisMonth,
			ead.incomeLastMonth,
			ead.peaks[0].day,
			ead.peaks[0].value / 100.0,
			ead.peaks[1].day,
			ead.peaks[1].value / 100.0,
			ead.peaks[2].day,
			ead.peaks[2].value / 100.0,
			ead.peaks[3].day,
			ead.peaks[3].value / 100.0,
			ead.peaks[4].day,
			ead.peaks[4].value / 100.0
		));
		server.sendContent("\n");
	}
}

void AmsWebServer::configFileUpload() {
	if(!checkSecurity(1))
		return;

	HTTPUpload& upload = uploadFile(FILE_CFG);
    if(upload.status == UPLOAD_FILE_END) {
		performRestart = true;
		server.sendHeader("Location","/");
		server.send(303);
	}
}

void AmsWebServer::redirectToMain() {
	server.sendHeader("Location","/");
	server.send(302);
}