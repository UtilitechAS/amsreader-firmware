#include "AmsWebServer.h"
#include "AmsWebHeaders.h"
#include "version.h"
#include "hexutils.h"
#include "AmsData.h"

#if defined(ESP32)
#include <esp_task_wdt.h>
#endif

#include "root/head_html.h"
#include "root/foot_html.h"
#include "root/index_html.h"
#include "root/application_js.h"
#include "root/setup_html.h"
#include "root/meter_html.h"
#include "root/wifi_html.h"
#include "root/mqtt_html.h"
#include "root/web_html.h"
#include "root/domoticz_html.h"
#include "root/priceapi_html.h"
#include "root/ntp_html.h"
#include "root/gpio_html.h"
#include "root/debugging_html.h"
#include "root/restart_html.h"
#include "root/restartwait_html.h"
#include "root/boot_css.h"
#include "root/github_svg.h"
#include "root/upload_html.h"
#include "root/firmware_html.h"
#include "root/delete_html.h"
#include "root/reset_html.h"
#include "root/temperature_html.h"
#include "root/notfound_html.h"
#include "root/data_json.h"
#include "root/tempsensor_json.h"
#include "root/dayplot_json.h"
#include "root/monthplot_json.h"
#include "root/energyprice_json.h"
#include "root/thresholds_html.h"
#include "root/configfile_html.h"
#include "root/meteradvanced_html.h"

#include "base64.h"

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

	snprintf_P(buf, 32, PSTR("/application-%s.js"), VERSION);

	server.on(F("/"), HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	server.on(F("/"), HTTP_POST, std::bind(&AmsWebServer::handleSetup, this));
	server.on(buf, HTTP_GET, std::bind(&AmsWebServer::applicationJs, this));
	server.on(F("/temperature"), HTTP_GET, std::bind(&AmsWebServer::temperature, this));
	server.on(F("/temperature"), HTTP_POST, std::bind(&AmsWebServer::temperaturePost, this));
	server.on(F("/temperature.json"), HTTP_GET, std::bind(&AmsWebServer::temperatureJson, this));
	server.on(F("/meter"), HTTP_GET, std::bind(&AmsWebServer::configMeterHtml, this));
	server.on(F("/meteradvanced"), HTTP_GET, std::bind(&AmsWebServer::configMeterAdvancedHtml, this));
	server.on(F("/wifi"), HTTP_GET, std::bind(&AmsWebServer::configWifiHtml, this));
	server.on(F("/mqtt"), HTTP_GET, std::bind(&AmsWebServer::configMqttHtml, this));
	server.on(F("/web"), HTTP_GET, std::bind(&AmsWebServer::configWebHtml, this));
	server.on(F("/domoticz"),HTTP_GET, std::bind(&AmsWebServer::configDomoticzHtml, this));
	server.on(F("/priceapi"),HTTP_GET, std::bind(&AmsWebServer::configPriceApiHtml, this));
	server.on(F("/thresholds"),HTTP_GET, std::bind(&AmsWebServer::configThresholdsHtml, this));
	server.on(F("/boot.css"), HTTP_GET, std::bind(&AmsWebServer::bootCss, this));
	server.on(F("/github.svg"), HTTP_GET, std::bind(&AmsWebServer::githubSvg, this)); 
	server.on(F("/data.json"), HTTP_GET, std::bind(&AmsWebServer::dataJson, this));
	server.on(F("/dayplot.json"), HTTP_GET, std::bind(&AmsWebServer::dayplotJson, this));
	server.on(F("/monthplot.json"), HTTP_GET, std::bind(&AmsWebServer::monthplotJson, this));
	server.on(F("/energyprice.json"), HTTP_GET, std::bind(&AmsWebServer::energyPriceJson, this));
	server.on(F("/configfile"),HTTP_GET, std::bind(&AmsWebServer::configFileHtml, this));
	server.on(F("/configfile"), HTTP_POST, std::bind(&AmsWebServer::uploadPost, this), std::bind(&AmsWebServer::configFileUpload, this));
	server.on(F("/configfile.cfg"),HTTP_GET, std::bind(&AmsWebServer::configFileDownload, this));

	server.on(F("/save"), HTTP_POST, std::bind(&AmsWebServer::handleSave, this));

	server.on(F("/ntp"), HTTP_GET, std::bind(&AmsWebServer::configNtpHtml, this));
	server.on(F("/gpio"), HTTP_GET, std::bind(&AmsWebServer::configGpioHtml, this));
	server.on(F("/debugging"), HTTP_GET, std::bind(&AmsWebServer::configDebugHtml, this));

	server.on(F("/firmware"), HTTP_GET, std::bind(&AmsWebServer::firmwareHtml, this));
	server.on(F("/firmware"), HTTP_POST, std::bind(&AmsWebServer::firmwarePost, this), std::bind(&AmsWebServer::firmwareUpload, this));
	server.on(F("/upgrade"), HTTP_GET, std::bind(&AmsWebServer::firmwareDownload, this));
	server.on(F("/restart"), HTTP_GET, std::bind(&AmsWebServer::restartHtml, this));
	server.on(F("/restart"), HTTP_POST, std::bind(&AmsWebServer::restartPost, this));
	server.on(F("/restart-wait"), HTTP_GET, std::bind(&AmsWebServer::restartWaitHtml, this));
	server.on(F("/is-alive"), HTTP_GET, std::bind(&AmsWebServer::isAliveCheck, this));

	server.on(F("/mqtt-ca"), HTTP_GET, std::bind(&AmsWebServer::mqttCa, this));
	server.on(F("/mqtt-ca"), HTTP_POST, std::bind(&AmsWebServer::mqttCaDelete, this), std::bind(&AmsWebServer::mqttCaUpload, this));
	server.on(F("/mqtt-cert"), HTTP_GET, std::bind(&AmsWebServer::mqttCert, this));
	server.on(F("/mqtt-cert"), HTTP_POST, std::bind(&AmsWebServer::mqttCertDelete, this), std::bind(&AmsWebServer::mqttCertUpload, this));
	server.on(F("/mqtt-key"), HTTP_GET, std::bind(&AmsWebServer::mqttKey, this));
	server.on(F("/mqtt-key"), HTTP_POST, std::bind(&AmsWebServer::mqttKeyDelete, this), std::bind(&AmsWebServer::mqttKeyUpload, this));

	server.on(F("/reset"), HTTP_GET, std::bind(&AmsWebServer::factoryResetHtml, this));
	server.on(F("/reset"), HTTP_POST, std::bind(&AmsWebServer::factoryResetPost, this));
	
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
	if(!access && webConfig.security >= level && server.hasHeader(F("Authorization"))) {
		String expectedAuth = String(webConfig.username) + ":" + String(webConfig.password);

		String providedPwd = server.header(F("Authorization"));
		providedPwd.replace(F("Basic "), F(""));

		#if defined(ESP8266)
		String expectedBase64 = base64::encode(expectedAuth, false);
		#elif defined(ESP32)
		String expectedBase64 = base64::encode(expectedAuth);
		#endif

		debugger->printf_P(PSTR("Expected auth: %s\n"), expectedBase64.c_str());
		debugger->printf_P(PSTR("Provided auth: %s\n"), providedPwd.c_str());

		access = providedPwd.equals(expectedBase64);
	}

	if(!access) {
		server.sendHeader(HEADER_AUTHENTICATE, AUTHENTICATE_BASIC);
		server.setContentLength(0);
		server.send_P(401, MIME_HTML, PSTR(""));
	}
	return access;
}

void AmsWebServer::temperature() {
	printD(F("Serving /temperature.html over http..."));

	if(!checkSecurity(2))
		return;

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(HEAD_HTML_LEN + TEMPERATURE_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, MIME_HTML, HEAD_HTML);
	server.sendContent_P(TEMPERATURE_HTML);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::temperaturePost() {
	if(!checkSecurity(1))
		return;

	printD(F("Saving temperature sensors..."));
	for(int i = 0; i < 32; i++) {
		if(!server.hasArg("sensor" + String(i, DEC))) break;
		String address  = server.arg("sensor" + String(i, DEC));
		String name = server.arg("sensor" + String(i, DEC) + "name").substring(0,16);
		bool common = server.hasArg("sensor" + String(i, DEC) + "common") && server.arg("sensor" + String(i, DEC) + "common") == F("true");
		if(debugger->isActive(RemoteDebug::DEBUG)) {
			debugger->printf("Addr: %s, name: %s\n", address.c_str(), name.c_str());
		}
		uint8_t hexStr[8];
		fromHex(hexStr, address, 8);
		config->updateTempSensorConfig(hexStr, name.c_str(), common);
		delay(1);
	}

	//if (debugger->isActive(RemoteDebug::DEBUG)) config->print(debugger);
	if(config->save()) {
		printD(F("Successfully saved temperature sensors"));
		server.sendHeader(HEADER_LOCATION, F("/temperature"), true);
		server.send (302, MIME_PLAIN, F(""));
	} else {
		printE(F("Error saving configuration"));
		server.send_P(500, MIME_HTML, PSTR("<html><body><h1>Error saving configuration!</h1></body></html>"));
	}
}

void AmsWebServer::temperatureJson() {
	printD(F("Serving /temperature.json over http..."));

	if(!checkSecurity(2))
		return;

	int count = hw->getTempSensorCount();
	snprintf_P(buf, 16, PSTR("{\"c\":%d,\"s\":["), count);

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
	snprintf_P(count == 0 ? pos : pos-1, 8, PSTR("]}"));

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(strlen(buf));
	server.send(200, MIME_JSON, buf);
}

void AmsWebServer::indexHtml() {
	printD(F("Serving /index.html over http..."));

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	if(WiFi.getMode() == WIFI_AP) {
		SystemConfig sys;
		config->getSystemConfig(sys);

		WiFiConfig wifi;
		config->clearWifi(wifi);

		String html = String((const __FlashStringHelper*) SETUP_HTML);
		for(int i = 0; i<255; i++) {
			html.replace("${config.boardType" + String(i) + "}", sys.boardType == i ? F("selected")  : F(""));
		}
		html.replace(F("${config.wifiSsid}"), wifi.ssid);
		html.replace(F("${config.wifiPassword}"), wifi.psk);
		html.replace(F("${config.wifiStaticIp}"), strlen(wifi.ip) > 0 ? F("checked") : F(""));
		html.replace(F("${config.wifiIp}"), wifi.ip);
		html.replace(F("${config.wifiGw}"), wifi.gateway);
		html.replace(F("${config.wifiSubnet}"), wifi.subnet);
		html.replace(F("${config.wifiDns1}"), wifi.dns1);
		html.replace(F("${config.wifiDns2}"), wifi.dns2);
		html.replace(F("${config.wifiHostname}"), wifi.hostname);
		server.send(200, MIME_HTML, html);
	} else {
		if(!checkSecurity(2))
			return;
		server.setContentLength(INDEX_HTML_LEN + HEAD_HTML_LEN + FOOT_HTML_LEN);
		server.send_P(200, MIME_HTML, HEAD_HTML);
		
		server.sendContent_P(INDEX_HTML);
		server.sendContent_P(FOOT_HTML);
	}
}

void AmsWebServer::applicationJs() {
	printD(F("Serving /application.js over http..."));

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_1HR);
	server.send_P(200, PSTR("application/javascript"), APPLICATION_JS);
}

void AmsWebServer::configMeterHtml() {
	printD(F("Serving /meter.html over http..."));

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) METER_HTML);

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	String manufacturer;
	switch(meterState->getMeterType()) {
		case AmsTypeAidon:
			manufacturer = F("Aidon");
			break;
		case AmsTypeKaifa:
			manufacturer = F("Kaifa");
			break;
		case AmsTypeKamstrup:
			manufacturer = F("Kamstrup");
			break;
		case AmsTypeIskra:
			manufacturer = F("Iskra");
			break;
		case AmsTypeLandisGyr:
			manufacturer = F("Landis+Gyr");
			break;
		case AmsTypeSagemcom:
			manufacturer = F("Sagemcom");
			break;
		case AmsTypeLng:
			manufacturer = F("L&G");
			break;
		default:
			manufacturer = F("Unknown");
			break;
	}

	html.replace(F("{maf}"), manufacturer);
	html.replace(F("{mod}"), meterState->getMeterModel());
	html.replace(F("{mid}"), meterState->getMeterId());
	html.replace(F("{b}"), String(meterConfig->baud));
	html.replace(F("{b300}"), meterConfig->baud == 300 ? F("selected") : F(""));
	html.replace(F("{b2400}"), meterConfig->baud == 2400 ? F("selected") : F(""));
	html.replace(F("{b4800}"), meterConfig->baud == 4800 ? F("selected") : F(""));
	html.replace(F("{b9600}"), meterConfig->baud == 9600 ? F("selected") : F(""));
	html.replace(F("{b19200}"), meterConfig->baud == 19200 ? F("selected") : F(""));
	html.replace(F("{b38400}"), meterConfig->baud == 38400 ? F("selected") : F(""));
	html.replace(F("{b57600}"), meterConfig->baud == 57600 ? F("selected") : F(""));
	html.replace(F("{b115200}"), meterConfig->baud == 115200 ? F("selected") : F(""));
	html.replace(F("{c}"), String(meterConfig->baud));
	html.replace(F("{c2}"), meterConfig->parity == 2 ? F("selected") : F(""));
	html.replace(F("{c3}"), meterConfig->parity == 3 ? F("selected") : F(""));
	html.replace(F("{c10}"), meterConfig->parity == 10 ? F("selected") : F(""));
	html.replace(F("{c11}"), meterConfig->parity == 11 ? F("selected") : F(""));
	html.replace(F("{i}"), meterConfig->invert ? F("checked") : F(""));
	html.replace(F("{d}"), String(meterConfig->distributionSystem));
	for(int i = 0; i<3; i++) {
		html.replace("{d" + String(i) + "}", meterConfig->distributionSystem == i ? F("selected")  : F(""));
	}
	html.replace(F("{f}"), String(meterConfig->mainFuse));
	html.replace(F("{p}"), String(meterConfig->productionCapacity));

	if(meterConfig->encryptionKey[0] != 0x00) {
		String encryptionKeyHex = "0x";
		encryptionKeyHex += toHex(meterConfig->encryptionKey, 16);
		html.replace(F("{e}"), encryptionKeyHex);

		String authenticationKeyHex = "0x";
		authenticationKeyHex += toHex(meterConfig->authenticationKey, 16);
		html.replace(F("{a}"), authenticationKeyHex);
	} else {
		html.replace(F("{e}"), F(""));
		html.replace(F("{a}"), F(""));
	}

	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, MIME_HTML, HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::configMeterAdvancedHtml() {
	printD(F("Serving /meteradvanced.html over http..."));

	if(!checkSecurity(1))
		return;

	snprintf_P(buf, BufferSize, METERADVANCED_HTML,
		meterConfig->wattageMultiplier / 1000.0,
		meterConfig->voltageMultiplier / 1000.0,
		meterConfig->amperageMultiplier / 1000.0,
		meterConfig->accumulatedMultiplier / 1000.0
	);

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(strlen(buf) + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, MIME_HTML, HEAD_HTML);
	server.sendContent(buf);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::configWifiHtml() {
	printD(F("Serving /wifi.html over http..."));

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) WIFI_HTML);

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	WiFiConfig wifi;
	config->getWiFiConfig(wifi);

	html.replace(F("{s}"), wifi.ssid);
	html.replace(F("{p}"), wifi.psk);
	if(strlen(wifi.ip) > 0) {
		html.replace(F("{st}"), F("checked"));
		html.replace(F("{i}"), wifi.ip);
		html.replace(F("{g}"), wifi.gateway);
		html.replace(F("{sn}"), wifi.subnet);
		html.replace(F("{d1}"), wifi.dns1);
		html.replace(F("{d2}"), wifi.dns2);
	} else {
		html.replace(F("{st}"), F(""));
		html.replace(F("{i}"), WiFi.localIP().toString());
		html.replace(F("{g}"), WiFi.gatewayIP().toString());
		html.replace(F("{sn}"), WiFi.subnetMask().toString());
		html.replace(F("{d1}"), WiFi.dnsIP().toString());
		html.replace(F("{d2}"), F(""));
	}
	html.replace(F("{h}"), wifi.hostname);
	html.replace(F("{m}"), wifi.mdns ? F("checked") : F(""));
	html.replace(F("{w}"), String(wifi.power / 10.0, 1));
	html.replace(F("{z0}"), wifi.sleep == 0 ? "selected" : "");
	html.replace(F("{z1}"), wifi.sleep == 1 ? "selected" : "");
	html.replace(F("{z2}"), wifi.sleep == 2 ? "selected" : "");
	#if defined(ESP32)
		html.replace(F("{wm}"), "19.5");
	#elif defined(ESP8266)
		html.replace(F("{wm}"), "20.5");
	#endif

	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, MIME_HTML, HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::configMqttHtml() {
	printD(F("Serving /mqtt.html over http..."));

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) MQTT_HTML);

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	MqttConfig mqtt;
	config->getMqttConfig(mqtt);

	html.replace(F("{m}"), strlen(mqtt.host) == 0 ? F("") : F("checked"));
	html.replace(F("{h}"), mqtt.host);
	if(mqtt.port > 0) {
		html.replace(F("{p}"), String(mqtt.port));
	} else {
		html.replace(F("{p}"), String(1883));
	}
	html.replace(F("{i}"), mqtt.clientId);
	html.replace(F("{t}"), mqtt.publishTopic);
	html.replace(F("{st}"), mqtt.subscribeTopic);
	html.replace(F("{u}"), mqtt.username);
	html.replace(F("{pw}"), mqtt.password);
	html.replace(F("{f}"), String(mqtt.payloadFormat));
	for(int i = 0; i<5; i++) {
		html.replace("{f" + String(i) + "}", mqtt.payloadFormat == i ? F("selected")  : F(""));
	}
	html.replace(F("{f255}"), mqtt.payloadFormat == 255 ? F("selected")  : F(""));

	html.replace(F("{s}"), mqtt.ssl ? F("checked") : F(""));

	if(LittleFS.begin()) {
		html.replace(F("{dcu}"), LittleFS.exists(FILE_MQTT_CA) ? F("none") : F(""));
		html.replace(F("{dcf}"), LittleFS.exists(FILE_MQTT_CA) ? F("") : F("none"));
		html.replace(F("{deu}"), LittleFS.exists(FILE_MQTT_CERT) ? F("none") : F(""));
		html.replace(F("{def}"), LittleFS.exists(FILE_MQTT_CERT) ? F("") : F("none"));
		html.replace(F("{dku}"), LittleFS.exists(FILE_MQTT_KEY) ? F("none") : F(""));
		html.replace(F("{dkf}"), LittleFS.exists(FILE_MQTT_KEY) ? F("") : F("none"));
		LittleFS.end();
	} else {
		html.replace(F("{dcu}"), F(""));
		html.replace(F("{dcf}"), F("none"));
		html.replace(F("{deu}"), F(""));
		html.replace(F("{def}"), F("none"));
		html.replace(F("{dku}"), F(""));
		html.replace(F("{dkf}"), F("none"));
	}

	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, MIME_HTML, HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::configDomoticzHtml() {
	printD(F("Serving /domoticz.html over http..."));

	if(!checkSecurity(1))
		return;

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	DomoticzConfig domo;
	config->getDomoticzConfig(domo);
	snprintf_P(buf, BufferSize, DOMOTICZ_HTML,
		domo.elidx,
		domo.cl1idx,
		domo.vl1idx,
		domo.vl2idx,
		domo.vl3idx
	);

	server.setContentLength(strlen(buf) + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, MIME_HTML, HEAD_HTML);
	server.sendContent(buf);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::configPriceApiHtml() {
	printD(F("Serving /priceapi.html over http..."));

	if(!checkSecurity(1))
		return;

	EntsoeConfig entsoe;
	config->getEntsoeConfig(entsoe);

	String html = String((const __FlashStringHelper*) PRICEAPI_HTML);

	if(ESP.getFreeHeap() > 32000) {
		html.replace("{et}", entsoe.token);
		html.replace("{dt}", "");
	} else {
		html.replace("{et}", "");
		html.replace("{dt}", "d-none");
	}
	html.replace("{em}", String(entsoe.multiplier / 1000.0, 3));

	html.replace(F("{no1}"), strcmp(entsoe.area, "10YNO-1--------2") == 0 ? "selected" : "");
	html.replace(F("{no2}"), strcmp(entsoe.area, "10YNO-2--------T") == 0 ? "selected" : "");
	html.replace(F("{no3}"), strcmp(entsoe.area, "10YNO-3--------J") == 0 ? "selected" : "");
	html.replace(F("{no4}"), strcmp(entsoe.area, "10YNO-4--------9") == 0 ? "selected" : "");
	html.replace(F("{no5}"), strcmp(entsoe.area, "10Y1001A1001A48H") == 0 ? "selected" : "");

	html.replace(F("{se1}"), strcmp(entsoe.area, "10Y1001A1001A44P") == 0 ? "selected" : "");
	html.replace(F("{se2}"), strcmp(entsoe.area, "10Y1001A1001A45N") == 0 ? "selected" : "");
	html.replace(F("{se3}"), strcmp(entsoe.area, "10Y1001A1001A46L") == 0 ? "selected" : "");
	html.replace(F("{se4}"), strcmp(entsoe.area, "10Y1001A1001A47J") == 0 ? "selected" : "");

	html.replace(F("{dk1}"), strcmp(entsoe.area, "10YDK-1--------W") == 0 ? "selected" : "");
	html.replace(F("{dk2}"), strcmp(entsoe.area, "10YDK-2--------M") == 0 ? "selected" : "");

	html.replace(F("{at}"), strcmp(entsoe.area, "10YAT-APG------L") == 0 ? F("selected") : F(""));
	html.replace(F("{be}"), strcmp(entsoe.area, "10YBE----------2") == 0 ? F("selected") : F(""));
	html.replace(F("{cz}"), strcmp(entsoe.area, "10YCZ-CEPS-----N") == 0 ? F("selected") : F(""));
	html.replace(F("{ee}"), strcmp(entsoe.area, "10Y1001A1001A39I") == 0 ? F("selected") : F(""));
	html.replace(F("{fi}"), strcmp(entsoe.area, "10YFI-1--------U") == 0 ? F("selected") : F(""));
	html.replace(F("{fr}"), strcmp(entsoe.area, "10YFR-RTE------C") == 0 ? F("selected") : F(""));
	html.replace(F("{de}"), strcmp(entsoe.area, "10Y1001A1001A83F") == 0 ? F("selected") : F(""));
	html.replace(F("{gb}"), strcmp(entsoe.area, "10YGB----------A") == 0 ? F("selected") : F(""));
	html.replace(F("{lv}"), strcmp(entsoe.area, "10YLV-1001A00074") == 0 ? F("selected") : F(""));
	html.replace(F("{lt}"), strcmp(entsoe.area, "10YLT-1001A0008Q") == 0 ? F("selected") : F(""));
	html.replace(F("{nl}"), strcmp(entsoe.area, "10YNL----------L") == 0 ? F("selected") : F(""));
	html.replace(F("{pl}"), strcmp(entsoe.area, "10YPL-AREA-----S") == 0 ? F("selected") : F(""));
	html.replace(F("{ch}"), strcmp(entsoe.area, "10YCH-SWISSGRIDZ") == 0 ? F("selected") : F(""));

	html.replace(F("{nok}"), strcmp(entsoe.currency, "NOK") == 0 ? "selected" : "");
	html.replace(F("{sek}"), strcmp(entsoe.currency, "SEK") == 0 ? "selected" : "");
	html.replace(F("{dkk}"), strcmp(entsoe.currency, "DKK") == 0 ? "selected" : "");
	html.replace(F("{eur}"), strcmp(entsoe.currency, "EUR") == 0 ? "selected" : "");

	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, MIME_HTML, HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::configThresholdsHtml() {
	printD(F("Serving /thresholds.html over http..."));

	if(!checkSecurity(1))
		return;

	EnergyAccountingConfig* config = ea->getConfig();

	String html = String((const __FlashStringHelper*) THRESHOLDS_HTML);
	for(int i = 0; i < 9; i++) {
		html.replace("{t" + String(i) + "}", String(config->thresholds[i], 10));
	}
	html.replace(F("{h}"), String(config->hours, 10));

	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, MIME_HTML, HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::configWebHtml() {
	printD(F("Serving /web.html over http..."));

	if(!checkSecurity(1))
		return;

	snprintf_P(buf, BufferSize, WEB_HTML,
		(char*) (webConfig.security == 0 ? F("selected")  : F("")),
		(char*) (webConfig.security == 1 ? F("selected")  : F("")),
		(char*) (webConfig.security == 2 ? F("selected")  : F("")),
		webConfig.username,
		webConfig.password
	);

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(strlen(buf) + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, MIME_HTML, HEAD_HTML);
	server.sendContent(buf);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::bootCss() {
	printD(F("Serving /boot.css over http..."));

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_1HR);
	server.send_P(200, "text/css", BOOT_CSS);
}

void AmsWebServer::githubSvg() {
	printD(F("Serving /github.svg over http..."));

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_1HR);
	server.send_P(200, PSTR("image/svg+xml"), GITHUB_SVG);
}

void AmsWebServer::dataJson() {
	printD(F("Serving /data.json over http..."));
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
	if(eapi != NULL)
		price = eapi->getValueForHour(0);

	String peaks = "";
    uint8_t peakCount = ea->getConfig()->hours;
    if(peakCount > 5) peakCount = 5;
	for(uint8_t i = 1; i <= peakCount; i++) {
		if(!peaks.isEmpty()) peaks += ",";
		peaks += String(ea->getPeak(i).value / 100.0);
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
		price == ENTSOE_NO_VALUE ? PSTR("null") : String(price, 2).c_str(),
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
		(uint32_t) time(nullptr)
	);

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(strlen(buf));
	server.send(200, MIME_JSON, buf);
}

void AmsWebServer::dayplotJson() {
	printD(F("Serving /dayplot.json over http..."));

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
	printD(F("Serving /monthplot.json over http..."));

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
	printD(F("Serving /energyprice.json over http..."));

	if(!checkSecurity(2))
		return;

	float prices[36];
	for(int i = 0; i < 36; i++) {
		prices[i] = eapi == NULL ? ENTSOE_NO_VALUE : eapi->getValueForHour(i);
	}

	snprintf_P(buf, BufferSize, ENERGYPRICE_JSON, 
		eapi == NULL ? "" : eapi->getCurrency(),
		prices[0] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[0], 4).c_str(),
		prices[1] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[1], 4).c_str(),
		prices[2] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[2], 4).c_str(),
		prices[3] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[3], 4).c_str(),
		prices[4] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[4], 4).c_str(),
		prices[5] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[5], 4).c_str(),
		prices[6] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[6], 4).c_str(),
		prices[7] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[7], 4).c_str(),
		prices[8] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[8], 4).c_str(),
		prices[9] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[9], 4).c_str(),
		prices[10] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[10], 4).c_str(),
		prices[11] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[11], 4).c_str(),
		prices[12] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[12], 4).c_str(),
		prices[13] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[13], 4).c_str(),
		prices[14] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[14], 4).c_str(),
		prices[15] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[15], 4).c_str(),
		prices[16] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[16], 4).c_str(),
		prices[17] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[17], 4).c_str(),
		prices[18] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[18], 4).c_str(),
		prices[19] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[19], 4).c_str(),
		prices[20] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[20], 4).c_str(),
		prices[21] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[21], 4).c_str(),
		prices[22] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[22], 4).c_str(),
		prices[23] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[23], 4).c_str(),
		prices[24] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[24], 4).c_str(),
		prices[25] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[25], 4).c_str(),
		prices[26] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[26], 4).c_str(),
		prices[27] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[27], 4).c_str(),
		prices[28] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[28], 4).c_str(),
		prices[29] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[29], 4).c_str(),
		prices[30] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[30], 4).c_str(),
		prices[31] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[31], 4).c_str(),
		prices[32] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[32], 4).c_str(),
		prices[33] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[33], 4).c_str(),
		prices[34] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[34], 4).c_str(),
		prices[35] == ENTSOE_NO_VALUE ? PSTR("null") : String(prices[35], 4).c_str()
	);

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(strlen(buf));
	server.send(200, MIME_JSON, buf);
}

void AmsWebServer::handleSetup() {
	printD(F("Handling setup method from http"));

	if(!server.hasArg(F("wifiSsid")) || server.arg(F("wifiSsid")).isEmpty() || !server.hasArg(F("wifiPassword")) || server.arg(F("wifiPassword")).isEmpty()) {
		server.sendHeader(HEADER_LOCATION, F("/"), true);
		server.send (302, MIME_PLAIN, F(""));
	} else {
		SystemConfig sys { static_cast<uint8_t>(server.arg(F("board")).toInt()) };

		DebugConfig debugConfig;
		config->getDebugConfig(debugConfig);
		config->clear();

		WiFiConfig wifi;
		config->clearWifi(wifi);

		switch(sys.boardType) {
			case 0: // roarfred
				config->clearGpio(*gpioConfig);
				gpioConfig->hanPin = 3;
				gpioConfig->apPin = 0;
				gpioConfig->ledPin = 2;
				gpioConfig->ledInverted = true;
				gpioConfig->tempSensorPin = 5;
				break;
			case 1: // Arnio Kamstrup
				config->clearGpio(*gpioConfig);
				gpioConfig->hanPin = 3;
				gpioConfig->apPin = 0;
				gpioConfig->ledPin = 2;
				gpioConfig->ledInverted = true;
				gpioConfig->ledPinRed = 13;
				gpioConfig->ledPinGreen = 14;
				gpioConfig->ledRgbInverted = true;
				break;
			case 2: // spenceme
				config->clearGpio(*gpioConfig);
				gpioConfig->hanPin = 3;
				gpioConfig->apPin = 0;
				gpioConfig->ledPin = 2;
				gpioConfig->ledInverted = true;
				gpioConfig->tempSensorPin = 5;
				gpioConfig->vccBootLimit = 33;
				wifi.sleep = 1;
				break;
			case 3: // Pow UART0
				config->clearGpio(*gpioConfig);
				gpioConfig->hanPin = 3;
				gpioConfig->apPin = 0;
				gpioConfig->ledPin = 2;
				gpioConfig->ledInverted = true;
				gpioConfig->ledPinRed = 13;
				gpioConfig->ledPinGreen = 14;
				gpioConfig->ledRgbInverted = true;
				wifi.sleep = 1;
				break;
			case 4: // Pow GPIO12
				config->clearGpio(*gpioConfig);
				gpioConfig->hanPin = 12;
				gpioConfig->apPin = 0;
				gpioConfig->ledPin = 2;
				gpioConfig->ledInverted = true;
				gpioConfig->ledPinRed = 13;
				gpioConfig->ledPinGreen = 14;
				gpioConfig->ledRgbInverted = true;
				wifi.sleep = 1;
				break;
			case 5: // Pow-K+ UART2
				config->clearGpio(*gpioConfig);
				gpioConfig->hanPin = 16;
				gpioConfig->apPin = 0;
				gpioConfig->ledPinRed = 13;
				gpioConfig->ledPinGreen = 14;
				gpioConfig->ledRgbInverted = true;
				gpioConfig->vccPin = 10;
				gpioConfig->vccResistorGnd = 22;
				gpioConfig->vccResistorVcc = 33;
				wifi.sleep = 1;
				break;
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
			case 7: // Pow-U+
				config->clearGpio(*gpioConfig);
				gpioConfig->hanPin = 16;
				gpioConfig->apPin = 0;
				gpioConfig->ledPinRed = 13;
				gpioConfig->ledPinGreen = 14;
				gpioConfig->ledRgbInverted = true;
				gpioConfig->vccPin = 10;
				gpioConfig->vccResistorGnd = 22;
				gpioConfig->vccResistorVcc = 33;
				wifi.sleep = 2;
				break;
			case 101: // D1
				gpioConfig->hanPin = 5;
				gpioConfig->apPin = 4;
				gpioConfig->ledPin = 2;
				gpioConfig->ledInverted = true;
				gpioConfig->vccMultiplier = 1100;
				break;
			case 100: // ESP8266
				gpioConfig->hanPin = 3;
				gpioConfig->ledPin = 2;
				gpioConfig->ledInverted = true;
				break;
			case 201: // D32
				gpioConfig->hanPin = 16;
				gpioConfig->apPin = 4;
				gpioConfig->ledPin = 5;
				gpioConfig->ledInverted = true;
				break;
			case 202: // Feather
				gpioConfig->hanPin = 16;
				gpioConfig->ledPin = 2;
				gpioConfig->ledInverted = false;
				break;
			case 203: // DevKitC
				gpioConfig->hanPin = 16;
				gpioConfig->ledPin = 2;
				gpioConfig->ledInverted = false;
				break;
			case 200: // ESP32
				gpioConfig->hanPin = 16;
				gpioConfig->ledPin = 2;
				gpioConfig->ledInverted = false;
				break;
			case 50: // S2
				gpioConfig->hanPin = 18;
				wifi.sleep = 1;
				break;
			case 51: // S2-mini
				gpioConfig->hanPin = 18;
				gpioConfig->ledPin = 15;
				gpioConfig->ledInverted = false;
				gpioConfig->apPin = 0;
				wifi.sleep = 1;
				break;
		}

		strcpy(wifi.ssid, server.arg(F("wifiSsid")).c_str());
		strcpy(wifi.psk, server.arg(F("wifiPassword")).c_str());

		if(server.hasArg(F("wifiIpType")) && server.arg(F("wifiIpType")).toInt() == 1) {
			strcpy(wifi.ip, server.arg(F("wifiIp")).c_str());
			strcpy(wifi.gateway, server.arg(F("wifiGw")).c_str());
			strcpy(wifi.subnet, server.arg(F("wifiSubnet")).c_str());
			strcpy(wifi.dns1, server.arg(F("wifiDns1")).c_str());
		}
		if(server.hasArg(F("wifiHostname")) && !server.arg(F("wifiHostname")).isEmpty()) {
			strcpy(wifi.hostname, server.arg(F("wifiHostname")).c_str());
			wifi.mdns = true;
		} else {
			wifi.mdns = false;
		}
		
		MqttConfig mqttConfig;
		config->clearMqtt(mqttConfig);
		
		config->clearAuth(webConfig);

		NtpConfig ntp;
		config->clearNtp(ntp);

		bool success = true;
		if(!config->setSystemConfig(sys)) {
			printD(F("Unable to set system config"));
			success = false;
		}
		if(!config->setWiFiConfig(wifi)) {
			printD(F("Unable to set WiFi config"));
			success = false;
		}
		if(!config->setMqttConfig(mqttConfig)) {
			printD(F("Unable to set MQTT config"));
			success = false;
		}
		if(!config->setWebConfig(webConfig)) {
			printD(F("Unable to set web config"));
			success = false;
		}
		if(!config->setGpioConfig(*gpioConfig)) {
			printD(F("Unable to set GPIO config"));
			success = false;
		}
		if(!config->setNtpConfig(ntp)) {
			printD(F("Unable to set NTP config"));
			success = false;
		}

		config->setDebugConfig(debugConfig);

		if(success && config->save()) {
			performRestart = true;
			server.sendHeader(HEADER_LOCATION,"/restart-wait");
			server.send(303);
		} else {
			printE(F("Error saving configuration"));
			server.send_P(500, MIME_HTML, PSTR("<html><body><h1>Error saving configuration!</h1></body></html>"));
		}
	}
}

void AmsWebServer::handleSave() {
	printD(F("Handling save method from http"));
	if(!checkSecurity(1))
		return;

	if(server.hasArg(F("mc")) && server.arg(F("mc")) == F("true")) {
		printD(F("Received meter config"));
		config->getMeterConfig(*meterConfig);
		meterConfig->baud = server.arg(F("b")).toInt();
		meterConfig->parity = server.arg(F("c")).toInt();
		meterConfig->invert = server.hasArg(F("i")) && server.arg(F("i")) == F("true");
		meterConfig->distributionSystem = server.arg(F("d")).toInt();
		meterConfig->mainFuse = server.arg(F("f")).toInt();
		meterConfig->productionCapacity = server.arg(F("p")).toInt();
		maxPwr = 0;

		String encryptionKeyHex = server.arg(F("e"));
		if(!encryptionKeyHex.isEmpty()) {
			encryptionKeyHex.replace(F("0x"), F(""));
			fromHex(meterConfig->encryptionKey, encryptionKeyHex, 16);
		}

		String authenticationKeyHex = server.arg(F("a"));
		if(!authenticationKeyHex.isEmpty()) {
			authenticationKeyHex.replace(F("0x"), F(""));
			fromHex(meterConfig->authenticationKey, authenticationKeyHex, 16);
		}
		config->setMeterConfig(*meterConfig);
	}

	if(server.hasArg(F("ma")) && server.arg(F("ma")) == F("true")) {
		printD(F("Received meter advanced config"));
		config->getMeterConfig(*meterConfig);
		meterConfig->wattageMultiplier = server.arg(F("wm")).toDouble() * 1000;
		meterConfig->voltageMultiplier = server.arg(F("vm")).toDouble() * 1000;
		meterConfig->amperageMultiplier = server.arg(F("am")).toDouble() * 1000;
		meterConfig->accumulatedMultiplier = server.arg(F("cm")).toDouble() * 1000;
		config->setMeterConfig(*meterConfig);
	}

	if(server.hasArg(F("wc")) && server.arg(F("wc")) == F("true")) {
		printD(F("Received WiFi config"));
		WiFiConfig wifi;
		config->clearWifi(wifi);
		strcpy(wifi.ssid, server.arg(F("s")).c_str());
		strcpy(wifi.psk, server.arg(F("p")).c_str());

		if(server.hasArg(F("st")) && server.arg(F("st")).toInt() == 1) {
			strcpy(wifi.ip, server.arg(F("i")).c_str());
			strcpy(wifi.gateway, server.arg(F("g")).c_str());
			strcpy(wifi.subnet, server.arg(F("sn")).c_str());
			strcpy(wifi.dns1, server.arg(F("d1")).c_str());
			strcpy(wifi.dns2, server.arg(F("d2")).c_str());
		}
		if(server.hasArg(F("h")) && !server.arg(F("h")).isEmpty()) {
			strcpy(wifi.hostname, server.arg(F("h")).c_str());
		}
		wifi.mdns = server.arg(F("m")) == F("true");
		wifi.power = server.arg(F("w")).toFloat() * 10;
		wifi.sleep = server.arg(F("z")).toInt();
		config->setWiFiConfig(wifi);
	}

	if(server.hasArg(F("mqc")) && server.arg(F("mqc")) == F("true")) {
		printD(F("Received MQTT config"));
		MqttConfig mqtt;
		if(server.hasArg(F("m")) && server.arg(F("m")) == F("true")) {
			strcpy(mqtt.host, server.arg(F("h")).c_str());
			strcpy(mqtt.clientId, server.arg(F("i")).c_str());
			strcpy(mqtt.publishTopic, server.arg(F("t")).c_str());
			strcpy(mqtt.subscribeTopic, server.arg(F("st")).c_str());
			strcpy(mqtt.username, server.arg(F("u")).c_str());
			strcpy(mqtt.password, server.arg(F("pw")).c_str());
			mqtt.payloadFormat = server.arg(F("f")).toInt();
			mqtt.ssl = server.arg(F("s")) == F("true");

			mqtt.port = server.arg(F("p")).toInt();
			if(mqtt.port == 0) {
				mqtt.port = mqtt.ssl ? 8883 : 1883;
			}
		} else {
			config->clearMqtt(mqtt);
		}
		config->setMqttConfig(mqtt);
	}

	if(server.hasArg(F("dc")) && server.arg(F("dc")) == F("true")) {
		printD(F("Received Domoticz config"));
		DomoticzConfig domo {
			static_cast<uint16_t>(server.arg(F("elidx")).toInt()),
			static_cast<uint16_t>(server.arg(F("vl1idx")).toInt()),
			static_cast<uint16_t>(server.arg(F("vl2idx")).toInt()),
			static_cast<uint16_t>(server.arg(F("vl3idx")).toInt()),
			static_cast<uint16_t>(server.arg(F("cl1idx")).toInt())
		};
		config->setDomoticzConfig(domo);
	}


	if(server.hasArg(F("ac")) && server.arg(F("ac")) == F("true")) {
		printD(F("Received web config"));
		webConfig.security = server.arg(F("as")).toInt();
		if(webConfig.security > 0) {
			strcpy(webConfig.username, server.arg(F("au")).c_str());
			strcpy(webConfig.password, server.arg(F("ap")).c_str());
			debugger->setPassword(webConfig.password);
		} else {
			strcpy_P(webConfig.username, PSTR(""));
			strcpy_P(webConfig.password, PSTR(""));
			debugger->setPassword(F(""));
		}
		config->setWebConfig(webConfig);
	}

	if(server.hasArg(F("gc")) && server.arg(F("gc")) == F("true")) {
		printD(F("Received GPIO config"));
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
		printD(F("Received Debug config"));
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
		printD(F("Received NTP config"));
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
		printD(F("Received ENTSO-E config"));
		EntsoeConfig entsoe;
		strcpy(entsoe.token, server.arg(F("et")).c_str());
		strcpy(entsoe.area, server.arg(F("ea")).c_str());
		strcpy(entsoe.currency, server.arg(F("ecu")).c_str());
		entsoe.multiplier = server.arg(F("em")).toFloat() * 1000;
		config->setEntsoeConfig(entsoe);
	}

	if(server.hasArg(F("cc")) && server.arg(F("cc")) == F("true")) {
		printD(F("Received energy accounting config"));
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

	printI(F("Saving configuration now..."));

	//if (debugger->isActive(RemoteDebug::DEBUG)) config->print(debugger);
	if (config->save()) {
		printI(F("Successfully saved."));
		if(config->isWifiChanged() || performRestart) {
			performRestart = true;
            server.sendHeader(HEADER_LOCATION,F("/restart-wait"));
            server.send(303);
		} else {
			server.sendHeader(HEADER_LOCATION, F("/"), true);
			server.send_P(302, MIME_PLAIN, PSTR(""));

			hw->setup(gpioConfig, config);
		}
	} else {
		printE(F("Error saving configuration"));
		server.send_P(500, MIME_HTML, PSTR("<html><body><h1>Error saving configuration!</h1></body></html>"));
	}
}

void AmsWebServer::configNtpHtml() {
	printD(F("Serving /ntp.html over http..."));

	if(!checkSecurity(1))
		return;

	NtpConfig ntp;
	config->getNtpConfig(ntp);
	snprintf_P(buf, BufferSize, NTP_HTML,
		(char*) (ntp.enable ? F("checked") : F("")),
		(char*) (ntp.offset == 0 ? F("selected") : F("")),
		(char*) (ntp.offset == 360 ? F("selected") : F("")),
		(char*) (ntp.offset == 720 ? F("selected") : F("")),
		(char*) (ntp.summerOffset == 0 ? F("selected") : F("")),
		(char*) (ntp.summerOffset == 360 ? F("selected") : F("")),
		(char*) (ntp.server),
		(char*) (ntp.dhcp ? F("checked") : F(""))
	);

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(strlen(buf) + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, MIME_HTML, HEAD_HTML);
	server.sendContent(buf);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::configGpioHtml() {
	printD(F("Serving /gpio.html over http..."));

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) GPIO_HTML);

	#if defined(CONFIG_IDF_TARGET_ESP32S2)
		html.replace(F("${g}"), F("44"));
	#elif defined(ESP32)
		html.replace(F("${g}"), F("39"));
	#else
		html.replace(F("${g}"), F("16"));
	#endif

	html.replace(F("${h}"), getSerialSelectOptions(gpioConfig->hanPin));

	html.replace(F("${l}"), gpioConfig->ledPin == 0xFF ? "" : String(gpioConfig->ledPin));
	html.replace(F("${i}"), gpioConfig->ledInverted ? F("checked") : F(""));
	html.replace(F("${r}"), gpioConfig->ledPinRed == 0xFF ? "" : String(gpioConfig->ledPinRed));
	html.replace(F("${e}"), gpioConfig->ledPinGreen == 0xFF ? "" : String(gpioConfig->ledPinGreen));
	html.replace(F("${b}"), gpioConfig->ledPinBlue == 0xFF ? "" : String(gpioConfig->ledPinBlue));
	html.replace(F("${n}"), gpioConfig->ledRgbInverted ? F("checked") : F(""));
	html.replace(F("${a}"), gpioConfig->apPin == 0xFF ? "" : String(gpioConfig->apPin));
	html.replace(F("${t}"), gpioConfig->tempSensorPin == 0xFF ? "" : String(gpioConfig->tempSensorPin));
	html.replace(F("${m}"), gpioConfig->tempAnalogSensorPin == 0xFF ? "" : String(gpioConfig->tempAnalogSensorPin));
	html.replace(F("${v}"), gpioConfig->vccPin == 0xFF ? "" : String(gpioConfig->vccPin));

	html.replace(F("${o}"), gpioConfig->vccOffset > 0 ? String(gpioConfig->vccOffset / 100.0, 2) : F(""));
	html.replace(F("${u}"), gpioConfig->vccMultiplier > 0 ? String(gpioConfig->vccMultiplier / 1000.0, 2) : F(""));
	html.replace(F("${c}"), gpioConfig->vccBootLimit > 0 ? String(gpioConfig->vccBootLimit / 10.0, 1) : F(""));

	html.replace(F("${d}"), gpioConfig->vccResistorGnd > 0 ? String(gpioConfig->vccResistorGnd) : F(""));
	html.replace(F("${s}"), gpioConfig->vccResistorVcc > 0 ? String(gpioConfig->vccResistorVcc) : F(""));

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);

	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, MIME_HTML, HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::configDebugHtml() {
	printD(F("Serving /debugging.html over http..."));

	if(!checkSecurity(1))
		return;

	DebugConfig debug;
	config->getDebugConfig(debug);
	snprintf_P(buf, BufferSize, DEBUGGING_HTML,
		(char*) (debug.telnet ? F("checked") : F("")),
		(char*) (debug.serial ? F("checked") : F("")),
		(char*) (debug.level == 1 ? F("selected")  : F("")),
		(char*) (debug.level == 2 ? F("selected")  : F("")),
		(char*) (debug.level == 3 ? F("selected")  : F("")),
		(char*) (debug.level == 4 ? F("selected")  : F(""))
	);

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);

	server.setContentLength(strlen(buf) + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, MIME_HTML, HEAD_HTML);
	server.sendContent(buf);
	server.sendContent_P(FOOT_HTML);
}

String AmsWebServer::getSerialSelectOptions(int selected) {
	String gpioOptions;
	if(selected == 3) {
		gpioOptions += F("<option value=\"3\" selected>UART0 (GPIO3)</option>");
	} else {
		gpioOptions += F("<option value=\"3\">UART0 (GPIO3)</option>");
	}
	#if defined(CONFIG_IDF_TARGET_ESP32S2)
		int numGpio = 30;
		int gpios[] = {4,5,6,7,8,9,10,11,12,13,14,15,16,17,19,21,22,23,25,32,33,34,35,36,39,40,41,42,43,44};
		if(selected == 18) {
			gpioOptions += F("<option value=\"18\" selected>UART1 (GPIO18)</option>");
		} else {
			gpioOptions += F("<option value=\"18\">UART1 (GPIO18)</option>");
		}
	#elif defined(ESP32)
		int numGpio = 24;
		int gpios[] = {4,5,6,7,8,10,11,12,13,14,15,17,18,19,21,22,23,25,32,33,34,35,36,39};
		if(selected == 9) {
			gpioOptions += F("<option value=\"9\" selected>UART1 (GPIO9)</option>");
		} else {
			gpioOptions += F("<option value=\"9\">UART1 (GPIO9)</option>");
		}
		if(selected == 16) {
			gpioOptions += F("<option value=\"16\" selected>UART2 (GPIO16)</option>");
		} else {
			gpioOptions += F("<option value=\"16\">UART2 (GPIO16)</option>");
		}
	#elif defined(ESP8266)
		int numGpio = 9;
		int gpios[] = {4,5,9,10,12,13,14,15,16};
		if(selected == 113) {
			gpioOptions += F("<option value=\"113\" selected>UART2 (GPIO13)</option>");
		} else {
			gpioOptions += F("<option value=\"113\">UART2 (GPIO13)</option>");
		}
	#endif

	for(int i = 0; i < numGpio; i++) {
		int gpio = gpios[i];
		char buffer [16];
		sprintf(buffer, "%02u", gpio);
		if(gpio == selected) {
			gpioOptions += "<option value=\"" + String(gpio) + "\" selected>GPIO" + buffer + "</option>";
		} else {
			gpioOptions += "<option value=\"" + String(gpio) + "\">GPIO" + buffer + "</option>";
		}
	}
	return gpioOptions;
}

void AmsWebServer::uploadPost() {
	server.send(200);
}

HTTPUpload& AmsWebServer::uploadFile(const char* path) {
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_START){
		if(uploading) {
			printE(F("Upload already in progress"));
			server.send_P(500, MIME_HTML, PSTR("<html><body><h1>Upload already in progress!</h1></body></html>"));
		} else if (!LittleFS.begin()) {
			printE(F("An Error has occurred while mounting LittleFS"));
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

				printE(F("An Error has occurred while writing file"));
				server.send_P(500, MIME_HTML, PSTR("<html><body><h1>Unable to write file!</h1></body></html>"));
			}
		}
    } else if(upload.status == UPLOAD_FILE_END) {
        if(file) {
			file.flush();
            file.close();
//			LittleFS.end();
        } else {
            server.send_P(500, MIME_PLAIN, PSTR("500: couldn't create file"));
        }
    }
	return upload;
}

void AmsWebServer::deleteFile(const char* path) {
	if(LittleFS.begin()) {
		LittleFS.remove(path);
		LittleFS.end();
	}
}

void AmsWebServer::firmwareHtml() {
	printD(F("Serving /firmware.html over http..."));

	if(!checkSecurity(1))
		return;

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	String html = String((const __FlashStringHelper*) FIRMWARE_HTML);

	#if defined(ESP8266)
		html.replace(F("{chipset}"), F("ESP8266"));
	#elif defined(CONFIG_IDF_TARGET_ESP32S2)
		html.replace(F("{chipset}"), F("ESP32S2"));
	#elif defined(ESP32)
		#if defined(CONFIG_FREERTOS_UNICORE)
			html.replace(F("{chipset}"), F("ESP32SOLO"));
		#else
			html.replace(F("{chipset}"), F("ESP32"));
		#endif
	#endif
	
	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, MIME_HTML, HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::firmwarePost() {
	printD(F("Handlling firmware post..."));
	if(!checkSecurity(1))
		return;
	
	if(rebootForUpgrade) {
		server.send(200);
	} else {
		if(server.hasArg(F("url"))) {
			String url = server.arg(F("url"));
			if(!url.isEmpty() && (url.startsWith(F("http://")) || url.startsWith(F("https://")))) {
				printD(F("Custom firmware URL was provided"));
				customFirmwareUrl = url;
				performUpgrade = true;
				server.sendHeader(HEADER_LOCATION,F("/restart-wait"));
				server.send(303);
				return;
			}
		}
		server.sendHeader(HEADER_LOCATION,F("/firmware"));
		server.send(303);
	}
}

void AmsWebServer::firmwareUpload() {
	printD(F("Handlling firmware upload..."));
	if(!checkSecurity(1))
		return;

	HTTPUpload& upload = server.upload();
	String filename = upload.filename;
	if(filename.isEmpty()) return;

    if(upload.status == UPLOAD_FILE_START) {
        if(!filename.endsWith(".bin")) {
            server.send_P(500, MIME_PLAIN, PSTR("500: couldn't create file"));
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
		server.sendHeader(HEADER_LOCATION,F("/restart-wait"));
		server.send(303);
	}
}

void AmsWebServer::firmwareDownload() {
	if(!checkSecurity(1))
		return;

	printD(F("Firmware download URL triggered"));
	performUpgrade = true;
	server.sendHeader(HEADER_LOCATION,F("/restart-wait"));
	server.send(303);
}

void AmsWebServer::restartHtml() {
	printD(F("Serving /restart.html over http..."));

	if(!checkSecurity(1))
		return;

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(RESTART_HTML_LEN + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, MIME_HTML, HEAD_HTML);
	server.sendContent_P(RESTART_HTML);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::restartPost() {
	if(!checkSecurity(1))
		return;

	printD(F("Setting restart flag and redirecting"));
	performRestart = true;
	server.sendHeader(HEADER_LOCATION,F("/restart-wait"));
	server.send(303);
}

void AmsWebServer::restartWaitHtml() {
	printD(F("Serving /restart-wait.html over http..."));

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) RESTARTWAIT_HTML);

	WiFiConfig wifi;
	config->getWiFiConfig(wifi);

	if(WiFi.getMode() != WIFI_AP) {
		html.replace(F("boot.css"), BOOTSTRAP_URL);
	}
	if(strlen(wifi.ip) == 0 && WiFi.getMode() != WIFI_AP) {
		html.replace(F("${ip}"), WiFi.localIP().toString());
	} else {
		html.replace(F("${ip}"), wifi.ip);
	}
	html.replace(F("${hostname}"), wifi.hostname);

	if(performUpgrade || rebootForUpgrade) {
		html.replace(F("{rs}"), "d-none");
		html.replace(F("{us}"), F(""));
	} else {
		html.replace(F("{rs}"), F(""));
		html.replace(F("{us}"), "d-none");
	}

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(html.length());
	server.send(200, MIME_HTML, html);

	yield();
	if(performRestart || rebootForUpgrade) {
		if(ds != NULL) {
			ds->save();
		}
		printI(F("Rebooting"));
		delay(1000);
#if defined(ESP8266)
		ESP.reset();
#elif defined(ESP32)
		ESP.restart();
#endif
		performRestart = false;
	} else if(performUpgrade) {
		WiFiClient client;
		String url = customFirmwareUrl.isEmpty() || !customFirmwareUrl.startsWith(F("http")) ? F("http://hub.amsleser.no/hub/firmware/update") : customFirmwareUrl;
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
				printE(F("Update failed"));
				break;
			case HTTP_UPDATE_NO_UPDATES:
				printI(F("No Update"));
				break;
			case HTTP_UPDATE_OK:
				printI(F("Update OK"));
				break;
		}
		performUpgrade = false;
	}
}

void AmsWebServer::isAliveCheck() {
	server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
	server.send(200);
}

void AmsWebServer::uploadHtml(const char* label, const char* action, const char* menu) {
	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);
	
	server.setContentLength(UPLOAD_HTML_LEN + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, MIME_HTML, HEAD_HTML);
	server.sendContent_P(UPLOAD_HTML);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::deleteHtml(const char* label, const char* action, const char* menu) {
	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);
	
	server.setContentLength(DELETE_HTML_LEN + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, MIME_HTML, HEAD_HTML);
	server.sendContent_P(DELETE_HTML);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::mqttCa() {
	printD(F("Serving /mqtt-ca.html over http..."));

	if(!checkSecurity(1))
		return;

	if(LittleFS.begin()) {
		if(LittleFS.exists(FILE_MQTT_CA)) {
			deleteHtml("CA file", "/mqtt-ca/delete", "mqtt");
		} else {
			uploadHtml("CA file", "/mqtt-ca", "mqtt");
		}
		LittleFS.end();
	} else {
		server.sendHeader(HEADER_LOCATION,F("/mqtt"));
		server.send(303);
	}
}

void AmsWebServer::mqttCaUpload() {
	if(!checkSecurity(1))
		return;

	uploadFile(FILE_MQTT_CA);
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_END) {
		server.sendHeader(HEADER_LOCATION,F("/mqtt"));
		server.send(303);

		MqttConfig mqttConfig;
		if(config->getMqttConfig(mqttConfig) && mqttConfig.ssl) {
			config->setMqttChanged();
		}
	}
}

void AmsWebServer::mqttCaDelete() {
	if(!checkSecurity(1))
		return;

	if(!uploading) { // Not an upload
		deleteFile(FILE_MQTT_CA);
		server.sendHeader(HEADER_LOCATION,F("/mqtt"));
		server.send(303);
		MqttConfig mqttConfig;
		if(config->getMqttConfig(mqttConfig) && mqttConfig.ssl) {
			config->setMqttChanged();
		}
	} else {
		uploading = false;
		server.send(200);
	}
}

void AmsWebServer::mqttCert() {
	printD(F("Serving /mqtt-cert.html over http..."));

	if(!checkSecurity(1))
		return;

	if(LittleFS.begin()) {
		if(LittleFS.exists(FILE_MQTT_CERT)) {
			deleteHtml("Certificate", "/mqtt-cert/delete", "mqtt");
		} else {
			uploadHtml("Certificate", "/mqtt-cert", "mqtt");
		}
		LittleFS.end();
	} else {
		server.sendHeader(HEADER_LOCATION,F("/mqtt"));
		server.send(303);
	}
}

void AmsWebServer::mqttCertUpload() {
	if(!checkSecurity(1))
		return;

	uploadFile(FILE_MQTT_CERT);
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_END) {
		server.sendHeader(HEADER_LOCATION,F("/mqtt"));
		server.send(303);
		MqttConfig mqttConfig;
		if(config->getMqttConfig(mqttConfig) && mqttConfig.ssl) {
			config->setMqttChanged();
		}
	}
}

void AmsWebServer::mqttCertDelete() {
	if(!checkSecurity(1))
		return;

	if(!uploading) { // Not an upload
		deleteFile(FILE_MQTT_CERT);
		server.sendHeader(HEADER_LOCATION,F("/mqtt"));
		server.send(303);
		MqttConfig mqttConfig;
		if(config->getMqttConfig(mqttConfig) && mqttConfig.ssl) {
			config->setMqttChanged();
		}
	} else {
		uploading = false;
		server.send(200);
	}
}

void AmsWebServer::mqttKey() {
	printD(F("Serving /mqtt-key.html over http..."));

	if(!checkSecurity(1))
		return;

	if(LittleFS.begin()) {
		if(LittleFS.exists(FILE_MQTT_KEY)) {
			deleteHtml("Private key", "/mqtt-key/delete", "mqtt");
		} else {
			uploadHtml("Private key", "/mqtt-key", "mqtt");
		}
		LittleFS.end();
	} else {
		server.sendHeader(HEADER_LOCATION,F("/mqtt"));
		server.send(303);
	}
}

void AmsWebServer::mqttKeyUpload() {
	if(!checkSecurity(1))
		return;

	uploadFile(FILE_MQTT_KEY);
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_END) {
		server.sendHeader(HEADER_LOCATION,F("/mqtt"));
		server.send(303);
		MqttConfig mqttConfig;
		if(config->getMqttConfig(mqttConfig) && mqttConfig.ssl) {
			config->setMqttChanged();
		}
	}
}

void AmsWebServer::mqttKeyDelete() {
	if(!checkSecurity(1))
		return;

	if(!uploading) { // Not an upload
		deleteFile(FILE_MQTT_KEY);
		server.sendHeader(HEADER_LOCATION,F("/mqtt"));
		server.send(303);
		MqttConfig mqttConfig;
		if(config->getMqttConfig(mqttConfig) && mqttConfig.ssl) {
			config->setMqttChanged();
		}
	} else {
		uploading = false;
		server.send(200);
	}
}

void AmsWebServer::factoryResetHtml() {
	if(!checkSecurity(1))
		return;

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_1HR);
	
	server.setContentLength(RESET_HTML_LEN + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, MIME_HTML, HEAD_HTML);
	server.sendContent_P(RESET_HTML);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::factoryResetPost() {
	if(!checkSecurity(1))
		return;

	printD(F("Performing factory reset"));
	if(server.hasArg(F("perform")) && server.arg(F("perform")) == F("true")) {
		printD(F("Formatting LittleFS"));
		LittleFS.format();
		printD(F("Clearing configuration"));
		config->clear();
		printD(F("Setting restart flag and redirecting"));
		performRestart = true;
		server.sendHeader(HEADER_LOCATION,F("/restart-wait"));
		server.send(303);
	} else {
		server.sendHeader(HEADER_LOCATION,F("/"));
		server.send(303);
	}
}


void AmsWebServer::notFound() {
	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_1HR);
	
	server.setContentLength(NOTFOUND_HTML_LEN + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(404, MIME_HTML, HEAD_HTML);
	server.sendContent_P(NOTFOUND_HTML);
	server.sendContent_P(FOOT_HTML);
}


void AmsWebServer::printD(String fmt, ...) {
	va_list args;
 	va_start(args, fmt);
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(String("(AmsWebServer)" + fmt + "\n").c_str(), args);
	va_end(args);
}

void AmsWebServer::printI(String fmt, ...) {
	va_list args;
 	va_start(args, fmt);
	if(debugger->isActive(RemoteDebug::INFO)) debugger->printf(String("(AmsWebServer)" + fmt + "\n").c_str(), args);
	va_end(args);
}

void AmsWebServer::printW(String fmt, ...) {
	va_list args;
 	va_start(args, fmt);
	if(debugger->isActive(RemoteDebug::WARNING)) debugger->printf(String("(AmsWebServer)" + fmt + "\n").c_str(), args);
	va_end(args);
}

void AmsWebServer::printE(String fmt, ...) {
	va_list args;
 	va_start(args, fmt);
	if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf(String("(AmsWebServer)" + fmt + "\n").c_str(), args);
	va_end(args);
}

void AmsWebServer::configFileHtml() {
	printD(F("Serving /configfile.html over http..."));

	if(!checkSecurity(1))
		return;

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(CONFIGFILE_HTML_LEN + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, MIME_HTML, HEAD_HTML);
	server.sendContent_P(CONFIGFILE_HTML);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::configFileDownload() {
	printD(F("Serving /configfile.cfg over http..."));

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
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("ntpOffset %d\n"), ntp.offset * 10));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("ntpSummerOffset %d\n"), ntp.summerOffset * 10));
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
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("dayplot %d %lu %lu %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d"), 
			day.version,
			(int32_t) day.lastMeterReadTime,
			day.activeImport,
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
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("monthplot %d %lu %lu %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d"), 
			month.version,
			(int32_t) month.lastMeterReadTime,
			month.activeImport,
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
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("energyaccounting %d %d %.2f %.2f %.2f %.2f %d %.2f %d %.2f %d %.2f %d %.2f %d %.2f"), 
			ead.version,
			ead.month,
			0.0,
			ead.costYesterday / 10.0,
			ead.costThisMonth / 1.0,
			ead.costLastMonth / 1.0,
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
		server.sendHeader(HEADER_LOCATION,F("/restart-wait"));
		server.send(303);
	}
}
