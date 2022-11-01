#include "AmsWebServer.h"
#include "AmsWebHeaders.h"
#include "base64.h"
#include "hexutils.h"

#include <ArduinoJson.h>

#include "html/index_html.h"
#include "html/index_css.h"
#include "html/index_js.h"
#include "html/github_svg.h"
#include "html/data_json.h"
#include "html/dayplot_json.h"
#include "html/monthplot_json.h"
#include "html/energyprice_json.h"
#include "html/tempsensor_json.h"

#include "version.h"


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

	// TODO
	server.on(F("/"), HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	server.on(F("/configuration"), HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	server.on(F("/index.css"), HTTP_GET, std::bind(&AmsWebServer::indexCss, this));
	server.on(F("/index.js"), HTTP_GET, std::bind(&AmsWebServer::indexJs, this));
	server.on(F("/github.svg"), HTTP_GET, std::bind(&AmsWebServer::githubSvg, this)); 
	server.on(F("/sysinfo.json"), HTTP_GET, std::bind(&AmsWebServer::sysinfoJson, this));
	server.on(F("/data.json"), HTTP_GET, std::bind(&AmsWebServer::dataJson, this));
	server.on(F("/dayplot.json"), HTTP_GET, std::bind(&AmsWebServer::dayplotJson, this));
	server.on(F("/monthplot.json"), HTTP_GET, std::bind(&AmsWebServer::monthplotJson, this));
	server.on(F("/energyprice.json"), HTTP_GET, std::bind(&AmsWebServer::energyPriceJson, this));
	server.on(F("/temperature.json"), HTTP_GET, std::bind(&AmsWebServer::temperatureJson, this));

	server.on(F("/configuration.json"), HTTP_GET, std::bind(&AmsWebServer::configurationJson, this));
	server.on(F("/save"), HTTP_POST, std::bind(&AmsWebServer::handleSave, this));
		
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

bool AmsWebServer::checkSecurity(byte level) {
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

	if(!access) {
		server.sendHeader(HEADER_AUTHENTICATE, AUTHENTICATE_BASIC);
		server.setContentLength(0);
		server.send(401, MIME_HTML, "");
	}
	return access;
}

void AmsWebServer::notFound() {
	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_1HR);
		server.send(404);
}
void AmsWebServer::githubSvg() {
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("Serving /github.svg over http...\n");

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_1HR);
	server.send_P(200, "image/svg+xml", GITHUB_SVG);
}

void AmsWebServer::sysinfoJson() {
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("Serving /sysinfo.json over http...\n");

	DynamicJsonDocument doc(256);
	doc["version"] = VERSION;
	#if defined(CONFIG_IDF_TARGET_ESP32S2)
	doc["chip"] = "esp32s2";
	#elif defined(CONFIG_IDF_TARGET_ESP32C3)
	doc["chip"] = "esp32c3";
	#elif defined(ESP32)
	doc["chip"] = "esp32";
	#elif defined(ESP8266)
	doc["chip"] = "esp8266";
	#endif

	serializeJson(doc, buf, BufferSize);
	server.send(200, MIME_JSON, buf);
}

void AmsWebServer::dataJson() {
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("Serving /data.json over http...\n");
	uint64_t now = millis64();

	if(!checkSecurity(2))
		return;

	float vcc = hw->getVcc();
	int rssi = hw->getWifiRssi();

	uint8_t espStatus;
	#if defined(ESP8266)
	if(vcc == 0) {
		espStatus = 1;
	} else if(vcc > 3.1 && vcc < 3.5) {
		espStatus = 1;
	} else if(vcc > 3.0 && vcc < 3.6) {
		espStatus = 2;
	} else {
		espStatus = 3;
	}
	#elif defined(ESP32)
	if(vcc == 0) {
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
	if(meterConfig->baud == 0) {
		hanStatus = 0;
	} else if(now - meterState->getLastUpdateMillis() < 15000) {
		hanStatus = 1;
	} else if(now - meterState->getLastUpdateMillis() < 30000) {
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
	if(eapi != NULL && strlen(eapi->getToken()) > 0)
		price = eapi->getValueForHour(0);

	String peaks = "";
	for(uint8_t i = 1; i <= ea->getConfig()->hours; i++) {
		if(!peaks.isEmpty()) peaks += ",";
		peaks += String(ea->getPeak(i));
	}

	snprintf_P(buf, BufferSize, DATA_JSON,
		maxPwr == 0 ? meterState->isThreePhase() ? 20000 : 10000 : maxPwr,
		meterConfig->productionCapacity,
		meterConfig->mainFuse == 0 ? 32 : meterConfig->mainFuse,
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
		(uint32_t) (now / 1000),
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
		ea->getUseToday(),
		ea->getCostToday(),
		ea->getProducedToday(),
		ea->getUseThisMonth(),
		ea->getCostThisMonth(),
		ea->getProducedThisMonth(),
		(uint32_t) time(nullptr)
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

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	if(!checkSecurity(2))
		return;

	server.setContentLength(INDEX_CSS_LEN);
	server.send_P(200, MIME_CSS, INDEX_CSS);
}

void AmsWebServer::indexJs() {
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("Serving /index.js over http...\n");

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	if(!checkSecurity(2))
		return;

	server.setContentLength(INDEX_JS_LEN);
	server.send_P(200, MIME_JS, INDEX_JS);
}

void AmsWebServer::configurationJson() {
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("Serving /configuration.json over http...\n");

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	if(!checkSecurity(1))
		return;

	DynamicJsonDocument doc(1024);
	doc["version"] = VERSION;

	WiFiConfig wifiConfig;
	config->getWiFiConfig(wifiConfig);
	WebConfig webConfig;
	config->getWebConfig(webConfig);
	doc["general"]["zone"] = "Europe/Oslo";
	doc["general"]["host"] = wifiConfig.hostname;
	doc["general"]["sec"] = webConfig.security;
	doc["general"]["user"] = webConfig.username;
	doc["general"]["pass"] = webConfig.password;

	config->getMeterConfig(*meterConfig);
	doc["meter"]["baud"] = meterConfig->baud;
	doc["meter"]["par"] = meterConfig->parity;
	doc["meter"]["inv"] = meterConfig->invert;
	doc["meter"]["dist"] = meterConfig->distributionSystem;
	doc["meter"]["fuse"] = meterConfig->mainFuse;
	doc["meter"]["prod"] = meterConfig->productionCapacity;
	doc["meter"]["enc"] = toHex(meterConfig->encryptionKey, 16);
	doc["meter"]["auth"] = toHex(meterConfig->authenticationKey, 16);

	// TODO: Tariff thresholds
	// TODO: Multipliers

	doc["wifi"]["ssid"] = wifiConfig.ssid;
	doc["wifi"]["psk"] = wifiConfig.psk;
	doc["wifi"]["pwr"] = wifiConfig.power / 10.0;
	doc["wifi"]["sleep"] = wifiConfig.sleep;

	NtpConfig ntpConfig;
	config->getNtpConfig(ntpConfig);
	doc["net"]["mode"] = strlen(wifiConfig.ip) > 0 ? "static" : "dhcp";
	doc["net"]["ip"] = wifiConfig.ip;
	doc["net"]["mask"] = wifiConfig.subnet;
	doc["net"]["gw"] = wifiConfig.gateway;
	doc["net"]["dns1"] = wifiConfig.dns1;
	doc["net"]["dns2"] = wifiConfig.dns2;
	doc["net"]["mdns"] = wifiConfig.mdns;
	doc["net"]["ntp1"] = ntpConfig.server;
	doc["net"]["ntpdhcp"] = ntpConfig.dhcp;

	MqttConfig mqttConfig;
	config->getMqttConfig(mqttConfig);
	doc["mqtt"]["host"] = mqttConfig.host;
	doc["mqtt"]["port"] = mqttConfig.port;
	doc["mqtt"]["user"] = mqttConfig.username;
	doc["mqtt"]["pass"] = mqttConfig.password;
	doc["mqtt"]["clid"] = mqttConfig.clientId;
	doc["mqtt"]["pub"] = mqttConfig.publishTopic;
	doc["mqtt"]["mode"] = mqttConfig.payloadFormat;
	doc["mqtt"]["ssl"] = mqttConfig.ssl;

	serializeJson(doc, buf, BufferSize);
	server.send(200, MIME_JSON, buf);
}
void AmsWebServer::handleSave() {
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Handling save method from http"));
	if(!checkSecurity(1))
		return;

	if(server.hasArg(F("meter")) && server.arg(F("meter")) == F("true")) {
		if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Received meter config"));
		config->getMeterConfig(*meterConfig);
		meterConfig->baud = server.arg(F("meter_baud")).toInt();
		meterConfig->parity = server.arg(F("meter_par")).toInt();
		meterConfig->invert = server.hasArg(F("meter_inv")) && server.arg(F("meter_inv")) == F("true");
		meterConfig->distributionSystem = server.arg(F("meter_dist")).toInt();
		meterConfig->mainFuse = server.arg(F("meter_fuse")).toInt();
		meterConfig->productionCapacity = server.arg(F("meter_prod")).toInt();
		maxPwr = 0;

		String encryptionKeyHex = server.arg(F("meter_enc"));
		if(!encryptionKeyHex.isEmpty()) {
			encryptionKeyHex.replace(F("0x"), F(""));
			fromHex(meterConfig->encryptionKey, encryptionKeyHex, 16);
		}

		String authenticationKeyHex = server.arg(F("meter_auth"));
		if(!authenticationKeyHex.isEmpty()) {
			authenticationKeyHex.replace(F("0x"), F(""));
			fromHex(meterConfig->authenticationKey, authenticationKeyHex, 16);
		}
		config->setMeterConfig(*meterConfig);
	}

	if(server.hasArg(F("ma")) && server.arg(F("ma")) == F("true")) {
		if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Received meter advanced config"));
		config->getMeterConfig(*meterConfig);
		meterConfig->wattageMultiplier = server.arg(F("wm")).toDouble() * 1000;
		meterConfig->voltageMultiplier = server.arg(F("vm")).toDouble() * 1000;
		meterConfig->amperageMultiplier = server.arg(F("am")).toDouble() * 1000;
		meterConfig->accumulatedMultiplier = server.arg(F("cm")).toDouble() * 1000;
		config->setMeterConfig(*meterConfig);
	}

	if(server.hasArg(F("wifi")) && server.arg(F("wifi")) == F("true")) {
		if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Received WiFi config"));
		WiFiConfig wifi;
		config->getWiFiConfig(wifi);
		strcpy(wifi.ssid, server.arg(F("wifi_ssid")).c_str());
		strcpy(wifi.psk, server.arg(F("wifi_psk")).c_str());
		wifi.power = server.arg(F("wifi_pwr")).toFloat() * 10;
		wifi.sleep = server.arg(F("wifi_sleep")).toInt();
		config->setWiFiConfig(wifi);
	}

	if(server.hasArg(F("net")) && server.arg(F("net")) == F("true")) {
		WiFiConfig wifi;
		config->getWiFiConfig(wifi);
		if(server.hasArg(F("net_mode")) && server.arg(F("net_mode")) == "static") {
			strcpy(wifi.ip, server.arg(F("net_ip")).c_str());
			strcpy(wifi.gateway, server.arg(F("net_gw")).c_str());
			strcpy(wifi.subnet, server.arg(F("net_sn")).c_str());
			strcpy(wifi.dns1, server.arg(F("net_dns1")).c_str());
			strcpy(wifi.dns2, server.arg(F("net_dns2")).c_str());
		}
		wifi.mdns = server.hasArg(F("net_mdns")) && server.arg(F("net_mdns")) == F("true");
		config->setWiFiConfig(wifi);

		NtpConfig ntp;
		config->getNtpConfig(ntp);
		ntp.dhcp = server.hasArg(F("net_ntpdhcp")) && server.arg(F("net_ntpdhcp")) == F("true");
		strcpy(ntp.server, server.arg(F("net_ntp1")).c_str());
		config->setNtpConfig(ntp);
	}

	if(server.hasArg(F("mqtt")) && server.arg(F("mqtt")) == F("true")) {
		if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Received MQTT config"));
		MqttConfig mqtt;
		if(server.hasArg(F("mqtt_host")) && !server.arg(F("mqtt_host")).isEmpty()) {
			strcpy(mqtt.host, server.arg(F("mqtt_host")).c_str());
			strcpy(mqtt.clientId, server.arg(F("mqtt_clid")).c_str());
			strcpy(mqtt.publishTopic, server.arg(F("mqtt_pub")).c_str());
			strcpy(mqtt.subscribeTopic, server.arg(F("mqtt_sub")).c_str());
			strcpy(mqtt.username, server.arg(F("mqtt_user")).c_str());
			strcpy(mqtt.password, server.arg(F("mqtt_pass")).c_str());
			mqtt.payloadFormat = server.arg(F("mqtt_mode")).toInt();
			mqtt.ssl = server.arg(F("mqtt_ssl")) == F("true");

			mqtt.port = server.arg(F("mqtt_port")).toInt();
			if(mqtt.port == 0) {
				mqtt.port = mqtt.ssl ? 8883 : 1883;
			}
		} else {
			config->clearMqtt(mqtt);
		}
		config->setMqttConfig(mqtt);
	}

	if(server.hasArg(F("dc")) && server.arg(F("dc")) == F("true")) {
		if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Received Domoticz config"));
		DomoticzConfig domo {
			static_cast<uint16_t>(server.arg(F("elidx")).toInt()),
			static_cast<uint16_t>(server.arg(F("vl1idx")).toInt()),
			static_cast<uint16_t>(server.arg(F("vl2idx")).toInt()),
			static_cast<uint16_t>(server.arg(F("vl3idx")).toInt()),
			static_cast<uint16_t>(server.arg(F("cl1idx")).toInt())
		};
		config->setDomoticzConfig(domo);
	}


	if(server.hasArg(F("general")) && server.arg(F("general")) == F("true")) {
		if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Received web config"));
		webConfig.security = server.arg(F("general_sec")).toInt();
		if(webConfig.security > 0) {
			strcpy(webConfig.username, server.arg(F("general_user")).c_str());
			strcpy(webConfig.password, server.arg(F("general_pass")).c_str());
			debugger->setPassword(webConfig.password);
		} else {
			strcpy_P(webConfig.username, PSTR(""));
			strcpy_P(webConfig.password, PSTR(""));
			debugger->setPassword(F(""));
		}
		config->setWebConfig(webConfig);

		WiFiConfig wifi;
		config->getWiFiConfig(wifi);
		if(server.hasArg(F("general_host")) && !server.arg(F("general_host")).isEmpty()) {
			strcpy(wifi.hostname, server.arg(F("general_host")).c_str());
		}
		config->setWiFiConfig(wifi);
	}

	if(server.hasArg(F("gc")) && server.arg(F("gc")) == F("true")) {
		if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Received GPIO config"));
		gpioConfig->hanPin = server.hasArg(F("h")) && !server.arg(F("h")).isEmpty() ? server.arg(F("h")).toInt() : 3;
		gpioConfig->ledPin = server.hasArg(F("l")) && !server.arg(F("l")).isEmpty() ? server.arg(F("l")).toInt() : 0xFF;
		gpioConfig->ledInverted = server.hasArg(F("i")) && server.arg(F("i")) == F("true");
		gpioConfig->ledPinRed = server.hasArg(F("r")) && !server.arg(F("r")).isEmpty() ? server.arg(F("r")).toInt() : 0xFF;
		gpioConfig->ledPinGreen = server.hasArg(F("e")) && !server.arg(F("e")).isEmpty() ? server.arg(F("e")).toInt() : 0xFF;
		gpioConfig->ledPinBlue = server.hasArg(F("b")) && !server.arg(F("b")).isEmpty() ? server.arg(F("b")).toInt() : 0xFF;
		gpioConfig->ledRgbInverted = server.hasArg(F("n")) && server.arg(F("n")) == F("true");
		gpioConfig->apPin = server.hasArg(F("a")) && !server.arg(F("a")).isEmpty() ? server.arg(F("a")).toInt() : 0xFF;
		gpioConfig->tempSensorPin = server.hasArg(F("t")) && !server.arg(F("t")).isEmpty() ?server.arg(F("t")).toInt() : 0xFF;
		gpioConfig->tempAnalogSensorPin = server.hasArg(F("m")) && !server.arg(F("m")).isEmpty() ?server.arg(F("m")).toInt() : 0xFF;
		gpioConfig->vccPin = server.hasArg(F("v")) && !server.arg(F("v")).isEmpty() ? server.arg(F("v")).toInt() : 0xFF;
		gpioConfig->vccOffset = server.hasArg(F("o")) && !server.arg(F("o")).isEmpty() ? server.arg(F("o")).toFloat() * 100 : 0;
		gpioConfig->vccMultiplier = server.hasArg(F("u")) && !server.arg(F("u")).isEmpty() ? server.arg(F("u")).toFloat() * 1000 : 1000;
		gpioConfig->vccBootLimit = server.hasArg(F("c")) && !server.arg(F("c")).isEmpty() ? server.arg(F("c")).toFloat() * 10 : 0;
		gpioConfig->vccResistorGnd = server.hasArg(F("d")) && !server.arg(F("d")).isEmpty() ? server.arg(F("d")).toInt() : 0;
		gpioConfig->vccResistorVcc = server.hasArg(F("s")) && !server.arg(F("s")).isEmpty() ? server.arg(F("s")).toInt() : 0;
		config->setGpioConfig(*gpioConfig);
	}

	if(server.hasArg(F("debugConfig")) && server.arg(F("debugConfig")) == F("true")) {
		if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Received Debug config"));
		DebugConfig debug;
		config->getDebugConfig(debug);
		bool active = debug.serial || debug.telnet;

		debug.telnet = server.hasArg(F("debugTelnet")) && server.arg(F("debugTelnet")) == F("true");
		debug.serial = server.hasArg(F("debugSerial")) && server.arg(F("debugSerial")) == F("true");
		debug.level = server.arg(F("debugLevel")).toInt();

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

	if(server.hasArg(F("nc")) && server.arg(F("nc")) == F("true")) {
		if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Received NTP config"));
		NtpConfig ntp {
			server.hasArg(F("n")) && server.arg(F("n")) == F("true"),
			server.hasArg(F("nd")) && server.arg(F("nd")) == F("true"),
			static_cast<int16_t>(server.arg(F("o")).toInt() / 10),
			static_cast<int16_t>(server.arg(F("so")).toInt() / 10)
		};
		strcpy(ntp.server, server.arg(F("ns")).c_str());
		config->setNtpConfig(ntp);
	}

	if(server.hasArg(F("ec")) && server.arg(F("ec")) == F("true")) {
		if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(PSTR("Received ENTSO-E config"));
		EntsoeConfig entsoe;
		strcpy(entsoe.token, server.arg(F("et")).c_str());
		strcpy(entsoe.area, server.arg(F("ea")).c_str());
		strcpy(entsoe.currency, server.arg(F("ecu")).c_str());
		entsoe.multiplier = server.arg(F("em")).toFloat() * 1000;
		config->setEntsoeConfig(entsoe);
	}

	if(server.hasArg(F("cc")) && server.arg(F("cc")) == F("true")) {
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
		eac.hours = server.arg(F("h")).toInt();
		config->setEnergyAccountingConfig(eac);
	}

	if(debugger->isActive(RemoteDebug::INFO)) debugger->printf(PSTR("Saving configuration now..."));

	DynamicJsonDocument doc(128);
	if (config->save()) {
		doc["success"] = true;
		if(debugger->isActive(RemoteDebug::INFO)) debugger->printf(PSTR("Successfully saved."));
		if(config->isWifiChanged() || performRestart) {
			performRestart = true;
			doc["reboot"] = true;
		} else {
			doc["reboot"] = false;
			hw->setup(gpioConfig, config);
		}
	} else {
		doc["success"] = false;
	}
	serializeJson(doc, buf, BufferSize);
	server.send(200, MIME_JSON, buf);

	delay(100);

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
