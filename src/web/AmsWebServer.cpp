#include "AmsWebServer.h"
#include "AmsWebHeaders.h"
#include "version.h"
#include "hexutils.h"
#include "AmsData.h"

#if defined(ESP8266)
	#include "root/head8266_html.h"
	#define HEAD_HTML HEAD8266_HTML
	#define HEAD_HTML_LEN HEAD8266_HTML_LEN
#elif defined(ESP32) 
	#include "root/head32_html.h"
	#define HEAD_HTML HEAD32_HTML
	#define HEAD_HTML_LEN HEAD32_HTML_LEN
#endif

#if defined(ESP32)
#include <esp_task_wdt.h>
#endif

#include "root/foot_html.h"
#include "root/index_html.h"
#include "root/application_js.h"
#include "root/setup_html.h"
#include "root/meter_html.h"
#include "root/wifi_html.h"
#include "root/mqtt_html.h"
#include "root/web_html.h"
#include "root/domoticz_html.h"
#include "root/entsoe_html.h"
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
#include "root/lowmem_html.h"
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

	snprintf(buf, 32, "/application-%s.js", VERSION);

	server.on("/", HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	server.on("/", HTTP_POST, std::bind(&AmsWebServer::handleSetup, this));
	server.on(buf, HTTP_GET, std::bind(&AmsWebServer::applicationJs, this));
	server.on("/temperature", HTTP_GET, std::bind(&AmsWebServer::temperature, this));
	server.on("/temperature", HTTP_POST, std::bind(&AmsWebServer::temperaturePost, this));
	server.on("/temperature.json", HTTP_GET, std::bind(&AmsWebServer::temperatureJson, this));
	server.on("/meter", HTTP_GET, std::bind(&AmsWebServer::configMeterHtml, this));
	server.on("/meteradvanced", HTTP_GET, std::bind(&AmsWebServer::configMeterAdvancedHtml, this));
	server.on("/wifi", HTTP_GET, std::bind(&AmsWebServer::configWifiHtml, this));
	server.on("/mqtt", HTTP_GET, std::bind(&AmsWebServer::configMqttHtml, this));
	server.on("/web", HTTP_GET, std::bind(&AmsWebServer::configWebHtml, this));
	server.on("/domoticz",HTTP_GET, std::bind(&AmsWebServer::configDomoticzHtml, this));
	server.on("/entsoe",HTTP_GET, std::bind(&AmsWebServer::configEntsoeHtml, this));
	server.on("/thresholds",HTTP_GET, std::bind(&AmsWebServer::configThresholdsHtml, this));
	server.on("/boot.css", HTTP_GET, std::bind(&AmsWebServer::bootCss, this));
	server.on("/github.svg", HTTP_GET, std::bind(&AmsWebServer::githubSvg, this)); 
	server.on("/data.json", HTTP_GET, std::bind(&AmsWebServer::dataJson, this));
	server.on("/dayplot.json", HTTP_GET, std::bind(&AmsWebServer::dayplotJson, this));
	server.on("/monthplot.json", HTTP_GET, std::bind(&AmsWebServer::monthplotJson, this));
	server.on("/energyprice.json", HTTP_GET, std::bind(&AmsWebServer::energyPriceJson, this));
	server.on("/configfile",HTTP_GET, std::bind(&AmsWebServer::configFileHtml, this));
	server.on("/configfile", HTTP_POST, std::bind(&AmsWebServer::uploadPost, this), std::bind(&AmsWebServer::configFileUpload, this));
	server.on("/configfile.cfg",HTTP_GET, std::bind(&AmsWebServer::configFileDownload, this));

	server.on("/save", HTTP_POST, std::bind(&AmsWebServer::handleSave, this));

	server.on("/ntp", HTTP_GET, std::bind(&AmsWebServer::configNtpHtml, this));
	server.on("/gpio", HTTP_GET, std::bind(&AmsWebServer::configGpioHtml, this));
	server.on("/debugging", HTTP_GET, std::bind(&AmsWebServer::configDebugHtml, this));

	server.on("/firmware", HTTP_GET, std::bind(&AmsWebServer::firmwareHtml, this));
	server.on("/firmware", HTTP_POST, std::bind(&AmsWebServer::uploadPost, this), std::bind(&AmsWebServer::firmwareUpload, this));
	server.on("/upgrade", HTTP_GET, std::bind(&AmsWebServer::firmwareDownload, this));
	server.on("/restart", HTTP_GET, std::bind(&AmsWebServer::restartHtml, this));
	server.on("/restart", HTTP_POST, std::bind(&AmsWebServer::restartPost, this));
	server.on("/restart-wait", HTTP_GET, std::bind(&AmsWebServer::restartWaitHtml, this));
	server.on("/is-alive", HTTP_GET, std::bind(&AmsWebServer::isAliveCheck, this));

	server.on("/mqtt-ca", HTTP_GET, std::bind(&AmsWebServer::mqttCa, this));
	server.on("/mqtt-ca", HTTP_POST, std::bind(&AmsWebServer::mqttCaDelete, this), std::bind(&AmsWebServer::mqttCaUpload, this));
	server.on("/mqtt-cert", HTTP_GET, std::bind(&AmsWebServer::mqttCert, this));
	server.on("/mqtt-cert", HTTP_POST, std::bind(&AmsWebServer::mqttCertDelete, this), std::bind(&AmsWebServer::mqttCertUpload, this));
	server.on("/mqtt-key", HTTP_GET, std::bind(&AmsWebServer::mqttKey, this));
	server.on("/mqtt-key", HTTP_POST, std::bind(&AmsWebServer::mqttKeyDelete, this), std::bind(&AmsWebServer::mqttKeyUpload, this));

	server.on("/reset", HTTP_GET, std::bind(&AmsWebServer::factoryResetHtml, this));
	server.on("/reset", HTTP_POST, std::bind(&AmsWebServer::factoryResetPost, this));
	
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

void AmsWebServer::temperature() {
	printD("Serving /temperature.html over http...");

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

	printD("Saving temperature sensors...");
	for(int i = 0; i < 32; i++) {
		if(!server.hasArg("sensor" + String(i, DEC))) break;
		String address  = server.arg("sensor" + String(i, DEC));
		String name = server.arg("sensor" + String(i, DEC) + "name").substring(0,16);
		bool common = server.hasArg("sensor" + String(i, DEC) + "common") && server.arg("sensor" + String(i, DEC) + "common") == "true";
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
		printD("Successfully saved temperature sensors");
		server.sendHeader("Location", String("/temperature"), true);
		server.send (302, MIME_PLAIN, "");
	} else {
		printE("Error saving configuration");
		String html = "<html><body><h1>Error saving configuration!</h1></body></html>";
		server.send(500, MIME_HTML, html);
	}
}

void AmsWebServer::temperatureJson() {
	printD("Serving /temperature.json over http...");

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
	printD("Serving /index.html over http...");

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
			html.replace("${config.boardType" + String(i) + "}", sys.boardType == i ? "selected"  : "");
		}
		html.replace("${config.wifiSsid}", wifi.ssid);
		html.replace("${config.wifiPassword}", wifi.psk);
		html.replace("${config.wifiStaticIp}", strlen(wifi.ip) > 0 ? "checked" : "");
		html.replace("${config.wifiIp}", wifi.ip);
		html.replace("${config.wifiGw}", wifi.gateway);
		html.replace("${config.wifiSubnet}", wifi.subnet);
		html.replace("${config.wifiDns1}", wifi.dns1);
		html.replace("${config.wifiDns2}", wifi.dns2);
		html.replace("${config.wifiHostname}", wifi.hostname);
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
	printD("Serving /application.js over http...");

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_1HR);
	server.send_P(200, "application/javascript", APPLICATION_JS);
}

void AmsWebServer::configMeterHtml() {
	printD("Serving /meter.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) METER_HTML);

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	String manufacturer;
	switch(meterState->getMeterType()) {
		case AmsTypeAidon:
			manufacturer = "Aidon";
			break;
		case AmsTypeKaifa:
			manufacturer = "Kaifa";
			break;
		case AmsTypeKamstrup:
			manufacturer = "Kamstrup";
			break;
		case AmsTypeIskra:
			manufacturer = "Iskra";
			break;
		case AmsTypeLandis:
			manufacturer = "Landis + Gyro";
			break;
		case AmsTypeSagemcom:
			manufacturer = "Sagemcom";
			break;
		default:
			manufacturer = "Unknown";
			break;
	}

	html.replace("{maf}", manufacturer);
	html.replace("{mod}", meterState->getMeterModel());
	html.replace("{mid}", meterState->getMeterId());
	html.replace("{b}", String(meterConfig->baud));
	html.replace("{b2400}", meterConfig->baud == 2400 ? "selected"  : "");
	html.replace("{b4800}", meterConfig->baud == 4800 ? "selected"  : "");
	html.replace("{b9600}", meterConfig->baud == 9600 ? "selected"  : "");
	html.replace("{b19200}", meterConfig->baud == 19200 ? "selected"  : "");
	html.replace("{b38400}", meterConfig->baud == 38400 ? "selected"  : "");
	html.replace("{b57600}", meterConfig->baud == 57600 ? "selected"  : "");
	html.replace("{b115200}", meterConfig->baud == 115200 ? "selected"  : "");
	html.replace("{c}", String(meterConfig->baud));
	html.replace("{c2}", meterConfig->parity == 2 ? "selected"  : "");
	html.replace("{c3}", meterConfig->parity == 3 ? "selected"  : "");
	html.replace("{c10}", meterConfig->parity == 10 ? "selected"  : "");
	html.replace("{c11}", meterConfig->parity == 11 ? "selected"  : "");
	html.replace("{i}", meterConfig->invert ? "checked"  : "");
	html.replace("{d}", String(meterConfig->distributionSystem));
	for(int i = 0; i<3; i++) {
		html.replace("{d" + String(i) + "}", meterConfig->distributionSystem == i ? "selected"  : "");
	}
	html.replace("{f}", String(meterConfig->mainFuse));
	html.replace("{p}", String(meterConfig->productionCapacity));

	if(meterConfig->encryptionKey[0] != 0x00) {
		String encryptionKeyHex = "0x";
		encryptionKeyHex += toHex(meterConfig->encryptionKey, 16);
		html.replace("{e}", encryptionKeyHex);

		String authenticationKeyHex = "0x";
		authenticationKeyHex += toHex(meterConfig->authenticationKey, 16);
		html.replace("{a}", authenticationKeyHex);
	} else {
		html.replace("{e}", "");
		html.replace("{a}", "");
	}

	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, MIME_HTML, HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::configMeterAdvancedHtml() {
	printD("Serving /meteradvanced.html over http...");

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
	printD("Serving /wifi.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) WIFI_HTML);

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	WiFiConfig wifi;
	config->getWiFiConfig(wifi);

	html.replace("{s}", wifi.ssid);
	html.replace("{p}", wifi.psk);
	if(strlen(wifi.ip) > 0) {
		html.replace("{st}", "checked");
		html.replace("{i}", wifi.ip);
		html.replace("{g}", wifi.gateway);
		html.replace("{sn}", wifi.subnet);
		html.replace("{d1}", wifi.dns1);
		html.replace("{d2}", wifi.dns2);
	} else {
		html.replace("{st}", "");
		html.replace("{i}", WiFi.localIP().toString());
		html.replace("{g}", WiFi.gatewayIP().toString());
		html.replace("{sn}", WiFi.subnetMask().toString());
		html.replace("{d1}", WiFi.dnsIP().toString());
		html.replace("{d2}", "");
	}
	html.replace("{h}", wifi.hostname);
	html.replace("{m}", wifi.mdns ? "checked" : "");
	html.replace("{w}", String(wifi.power / 10.0, 1));
	#if defined(ESP32)
		html.replace("{wm}", "19.5");
	#elif defined(ESP8266)
		html.replace("{wm}", "20.5");
	#endif

	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, MIME_HTML, HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::configMqttHtml() {
	printD("Serving /mqtt.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) MQTT_HTML);

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	MqttConfig mqtt;
	config->getMqttConfig(mqtt);

	html.replace("{m}", strlen(mqtt.host) == 0 ? "" : "checked");
	html.replace("{h}", mqtt.host);
	if(mqtt.port > 0) {
		html.replace("{p}", String(mqtt.port));
	} else {
		html.replace("{p}", String(1883));
	}
	html.replace("{i}", mqtt.clientId);
	html.replace("{t}", mqtt.publishTopic);
	html.replace("{st}", mqtt.subscribeTopic);
	html.replace("{u}", mqtt.username);
	html.replace("{pw}", mqtt.password);
	html.replace("{f}", String(mqtt.payloadFormat));
	for(int i = 0; i<5; i++) {
		html.replace("{f" + String(i) + "}", mqtt.payloadFormat == i ? "selected"  : "");
	}
	html.replace("{f255}", mqtt.payloadFormat == 255 ? "selected"  : "");

	html.replace("{s}", mqtt.ssl ? "checked" : "");

	if(LittleFS.begin()) {
		html.replace("{dcu}", LittleFS.exists(FILE_MQTT_CA) ? "none" : "");
		html.replace("{dcf}", LittleFS.exists(FILE_MQTT_CA) ? "" : "none");
		html.replace("{deu}", LittleFS.exists(FILE_MQTT_CERT) ? "none" : "");
		html.replace("{def}", LittleFS.exists(FILE_MQTT_CERT) ? "" : "none");
		html.replace("{dku}", LittleFS.exists(FILE_MQTT_KEY) ? "none" : "");
		html.replace("{dkf}", LittleFS.exists(FILE_MQTT_KEY) ? "" : "none");
		LittleFS.end();
	} else {
		html.replace("{dcu}", "");
		html.replace("{dcf}", "none");
		html.replace("{deu}", "");
		html.replace("{def}", "none");
		html.replace("{dku}", "");
		html.replace("{dkf}", "none");
	}

	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, MIME_HTML, HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::configDomoticzHtml() {
	printD("Serving /domoticz.html over http...");

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

void AmsWebServer::configEntsoeHtml() {
	printD("Serving /entsoe.html over http...");

	if(!checkSecurity(1))
		return;

	EntsoeConfig entsoe;
	config->getEntsoeConfig(entsoe);

	if(ESP.getFreeHeap() > 25000) {
		String html = String((const __FlashStringHelper*) ENTSOE_HTML);

		html.replace("{et}", entsoe.token);
		html.replace("{em}", String(entsoe.multiplier / 1000.0, 3));

		html.replace("{eaNo1}", strcmp(entsoe.area, "10YNO-1--------2") == 0 ? "selected" : "");
		html.replace("{eaNo2}", strcmp(entsoe.area, "10YNO-2--------T") == 0 ? "selected" : "");
		html.replace("{eaNo3}", strcmp(entsoe.area, "10YNO-3--------J") == 0 ? "selected" : "");
		html.replace("{eaNo4}", strcmp(entsoe.area, "10YNO-4--------9") == 0 ? "selected" : "");
		html.replace("{eaNo5}", strcmp(entsoe.area, "10Y1001A1001A48H") == 0 ? "selected" : "");

		html.replace("{eaSe1}", strcmp(entsoe.area, "10Y1001A1001A44P") == 0 ? "selected" : "");
		html.replace("{eaSe2}", strcmp(entsoe.area, "10Y1001A1001A45N") == 0 ? "selected" : "");
		html.replace("{eaSe3}", strcmp(entsoe.area, "10Y1001A1001A46L") == 0 ? "selected" : "");
		html.replace("{eaSe4}", strcmp(entsoe.area, "10Y1001A1001A47J") == 0 ? "selected" : "");

		html.replace("{eaDk1}", strcmp(entsoe.area, "10YDK-1--------W") == 0 ? "selected" : "");
		html.replace("{eaDk2}", strcmp(entsoe.area, "10YDK-2--------M") == 0 ? "selected" : "");

		html.replace("{ecNOK}", strcmp(entsoe.currency, "NOK") == 0 ? "selected" : "");
		html.replace("{ecSEK}", strcmp(entsoe.currency, "SEK") == 0 ? "selected" : "");
		html.replace("{ecDKK}", strcmp(entsoe.currency, "DKK") == 0 ? "selected" : "");
		html.replace("{ecEUR}", strcmp(entsoe.currency, "EUR") == 0 ? "selected" : "");

		server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
		server.send_P(200, MIME_HTML, HEAD_HTML);
		server.sendContent(html);
		server.sendContent_P(FOOT_HTML);
	} else {
		server.setContentLength(LOWMEM_HTML_LEN + HEAD_HTML_LEN + FOOT_HTML_LEN);
		server.send_P(200, MIME_HTML, HEAD_HTML);
		server.sendContent_P(LOWMEM_HTML);
		server.sendContent_P(FOOT_HTML);
	}
}

void AmsWebServer::configThresholdsHtml() {
	printD("Serving /thresholds.html over http...");

	if(!checkSecurity(1))
		return;

	EnergyAccountingConfig* config = ea->getConfig();

	String html = String((const __FlashStringHelper*) THRESHOLDS_HTML);
	for(int i = 0; i < 9; i++) {
		html.replace("{t" + String(i) + "}", String(config->thresholds[i]));
	}

	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, MIME_HTML, HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::configWebHtml() {
	printD("Serving /web.html over http...");

	if(!checkSecurity(1))
		return;

	snprintf_P(buf, BufferSize, WEB_HTML,
		webConfig.security == 0 ? "selected"  : "",
		webConfig.security == 1 ? "selected"  : "",
		webConfig.security == 2 ? "selected"  : "",
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
	printD("Serving /boot.css over http...");

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_1HR);
	server.send_P(200, "text/css", BOOT_CSS);
}

void AmsWebServer::githubSvg() {
	printD("Serving /github.svg over http...");

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_1HR);
	server.send_P(200, "image/svg+xml", GITHUB_SVG);
}

void AmsWebServer::dataJson() {
	printD("Serving /data.json over http...");
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
		ea->getCurrentThreshold(),
		ea->getUseThisHour(),
		ea->getCostThisHour(),
		ea->getUseToday(),
		ea->getCostToday(),
		ea->getUseThisMonth(),
		ea->getCostThisMonth(),
		(uint32_t) time(nullptr)
	);

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(strlen(buf));
	server.send(200, MIME_JSON, buf);
}

void AmsWebServer::dayplotJson() {
	printD("Serving /dayplot.json over http...");

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
	printD("Serving /monthplot.json over http...");

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
	printD("Serving /energyprice.json over http...");

	if(!checkSecurity(2))
		return;

	float prices[36];
	for(int i = 0; i < 36; i++) {
		prices[i] = eapi == NULL ? ENTSOE_NO_VALUE : eapi->getValueForHour(i);
	}

	snprintf_P(buf, BufferSize, ENERGYPRICE_JSON, 
		eapi == NULL ? "" : eapi->getCurrency(),
		prices[0] == ENTSOE_NO_VALUE ? "null" : String(prices[0], 2).c_str(),
		prices[1] == ENTSOE_NO_VALUE ? "null" : String(prices[1], 2).c_str(),
		prices[2] == ENTSOE_NO_VALUE ? "null" : String(prices[2], 2).c_str(),
		prices[3] == ENTSOE_NO_VALUE ? "null" : String(prices[3], 2).c_str(),
		prices[4] == ENTSOE_NO_VALUE ? "null" : String(prices[4], 2).c_str(),
		prices[5] == ENTSOE_NO_VALUE ? "null" : String(prices[5], 2).c_str(),
		prices[6] == ENTSOE_NO_VALUE ? "null" : String(prices[6], 2).c_str(),
		prices[7] == ENTSOE_NO_VALUE ? "null" : String(prices[7], 2).c_str(),
		prices[8] == ENTSOE_NO_VALUE ? "null" : String(prices[8], 2).c_str(),
		prices[9] == ENTSOE_NO_VALUE ? "null" : String(prices[9], 2).c_str(),
		prices[10] == ENTSOE_NO_VALUE ? "null" : String(prices[10], 2).c_str(),
		prices[11] == ENTSOE_NO_VALUE ? "null" : String(prices[11], 2).c_str(),
		prices[12] == ENTSOE_NO_VALUE ? "null" : String(prices[12], 2).c_str(),
		prices[13] == ENTSOE_NO_VALUE ? "null" : String(prices[13], 2).c_str(),
		prices[14] == ENTSOE_NO_VALUE ? "null" : String(prices[14], 2).c_str(),
		prices[15] == ENTSOE_NO_VALUE ? "null" : String(prices[15], 2).c_str(),
		prices[16] == ENTSOE_NO_VALUE ? "null" : String(prices[16], 2).c_str(),
		prices[17] == ENTSOE_NO_VALUE ? "null" : String(prices[17], 2).c_str(),
		prices[18] == ENTSOE_NO_VALUE ? "null" : String(prices[18], 2).c_str(),
		prices[19] == ENTSOE_NO_VALUE ? "null" : String(prices[19], 2).c_str(),
		prices[20] == ENTSOE_NO_VALUE ? "null" : String(prices[20], 2).c_str(),
		prices[21] == ENTSOE_NO_VALUE ? "null" : String(prices[21], 2).c_str(),
		prices[22] == ENTSOE_NO_VALUE ? "null" : String(prices[22], 2).c_str(),
		prices[23] == ENTSOE_NO_VALUE ? "null" : String(prices[23], 2).c_str(),
		prices[24] == ENTSOE_NO_VALUE ? "null" : String(prices[24], 2).c_str(),
		prices[25] == ENTSOE_NO_VALUE ? "null" : String(prices[25], 2).c_str(),
		prices[26] == ENTSOE_NO_VALUE ? "null" : String(prices[26], 2).c_str(),
		prices[27] == ENTSOE_NO_VALUE ? "null" : String(prices[27], 2).c_str(),
		prices[28] == ENTSOE_NO_VALUE ? "null" : String(prices[28], 2).c_str(),
		prices[29] == ENTSOE_NO_VALUE ? "null" : String(prices[29], 2).c_str(),
		prices[30] == ENTSOE_NO_VALUE ? "null" : String(prices[30], 2).c_str(),
		prices[31] == ENTSOE_NO_VALUE ? "null" : String(prices[31], 2).c_str(),
		prices[32] == ENTSOE_NO_VALUE ? "null" : String(prices[32], 2).c_str(),
		prices[33] == ENTSOE_NO_VALUE ? "null" : String(prices[33], 2).c_str(),
		prices[34] == ENTSOE_NO_VALUE ? "null" : String(prices[34], 2).c_str(),
		prices[35] == ENTSOE_NO_VALUE ? "null" : String(prices[35], 2).c_str()
	);

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(strlen(buf));
	server.send(200, MIME_JSON, buf);
}

void AmsWebServer::handleSetup() {
	printD("Handling setup method from http");

	if(!server.hasArg("wifiSsid") || server.arg("wifiSsid").isEmpty() || !server.hasArg("wifiPassword") || server.arg("wifiPassword").isEmpty()) {
		server.sendHeader("Location", String("/"), true);
		server.send (302, MIME_PLAIN, "");
	} else {
		SystemConfig sys { server.arg("board").toInt() };

		DebugConfig debugConfig;
		config->getDebugConfig(debugConfig);

		config->clear();

		switch(sys.boardType) {
			case 0: // roarfred
				gpioConfig->hanPin = 3;
				gpioConfig->apPin = 0;
				gpioConfig->ledPin = 2;
				gpioConfig->ledInverted = true;
				gpioConfig->tempSensorPin = 5;
				break;
			case 1: // Arnio Kamstrup
				gpioConfig->hanPin = 3;
				gpioConfig->apPin = 0;
				gpioConfig->ledPin = 2;
				gpioConfig->ledInverted = true;
				gpioConfig->ledPinRed = 13;
				gpioConfig->ledPinGreen = 14;
				gpioConfig->ledRgbInverted = true;
				break;
			case 2: // spenceme
				gpioConfig->hanPin = 3;
				gpioConfig->apPin = 0;
				gpioConfig->ledPin = 2;
				gpioConfig->ledInverted = true;
				gpioConfig->tempSensorPin = 5;
				gpioConfig->vccBootLimit = 33;
				break;
			case 3: // Pow UART0
				gpioConfig->hanPin = 3;
				gpioConfig->apPin = 0;
				gpioConfig->ledPin = 2;
				gpioConfig->ledInverted = true;
				gpioConfig->ledPinRed = 13;
				gpioConfig->ledPinGreen = 14;
				gpioConfig->ledRgbInverted = true;
				break;
			case 4: // Pow GPIO12
				gpioConfig->hanPin = 12;
				gpioConfig->apPin = 0;
				gpioConfig->ledPin = 2;
				gpioConfig->ledInverted = true;
				gpioConfig->ledPinRed = 13;
				gpioConfig->ledPinGreen = 14;
				gpioConfig->ledRgbInverted = true;
				break;
			case 5: // Pow-K+ UART2
				gpioConfig->hanPin = 16;
				gpioConfig->apPin = 0;
				gpioConfig->ledPinRed = 13;
				gpioConfig->ledPinGreen = 14;
				gpioConfig->ledRgbInverted = true;
				gpioConfig->vccPin = 35;
				gpioConfig->vccResistorGnd = 22;
				gpioConfig->vccResistorVcc = 33;
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
		}

		WiFiConfig wifi;
		config->clearWifi(wifi);

		strcpy(wifi.ssid, server.arg("wifiSsid").c_str());
		strcpy(wifi.psk, server.arg("wifiPassword").c_str());

		if(server.hasArg("wifiIpType") && server.arg("wifiIpType").toInt() == 1) {
			strcpy(wifi.ip, server.arg("wifiIp").c_str());
			strcpy(wifi.gateway, server.arg("wifiGw").c_str());
			strcpy(wifi.subnet, server.arg("wifiSubnet").c_str());
			strcpy(wifi.dns1, server.arg("wifiDns1").c_str());
		}
		if(server.hasArg("wifiHostname") && !server.arg("wifiHostname").isEmpty()) {
			strcpy(wifi.hostname, server.arg("wifiHostname").c_str());
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
			printD("Unable to set system config");
			success = false;
		}
		if(!config->setWiFiConfig(wifi)) {
			printD("Unable to set WiFi config");
			success = false;
		}
		if(!config->setMqttConfig(mqttConfig)) {
			printD("Unable to set MQTT config");
			success = false;
		}
		if(!config->setWebConfig(webConfig)) {
			printD("Unable to set web config");
			success = false;
		}
		if(!config->setGpioConfig(*gpioConfig)) {
			printD("Unable to set GPIO config");
			success = false;
		}
		if(!config->setNtpConfig(ntp)) {
			printD("Unable to set NTP config");
			success = false;
		}

		config->setDebugConfig(debugConfig);

		if(success && config->save()) {
			performRestart = true;
			server.sendHeader("Location","/restart-wait");
			server.send(303);
		} else {
			printE("Error saving configuration");
			String html = "<html><body><h1>Error saving configuration!</h1></body></html>";
			server.send(500, MIME_HTML, html);
		}
	}
}

void AmsWebServer::handleSave() {
	printD("Handling save method from http");
	if(!checkSecurity(1))
		return;

	String temp;

	if(server.hasArg("mc") && server.arg("mc") == "true") {
		printD("Received meter config");
		config->getMeterConfig(*meterConfig);
		meterConfig->baud = server.arg("b").toInt();
		meterConfig->parity = server.arg("c").toInt();
		meterConfig->invert = server.hasArg("i") && server.arg("i") == "true";
		meterConfig->distributionSystem = server.arg("d").toInt();
		meterConfig->mainFuse = server.arg("f").toInt();
		meterConfig->productionCapacity = server.arg("p").toInt();
		maxPwr = 0;

		String encryptionKeyHex = server.arg("e");
		if(!encryptionKeyHex.isEmpty()) {
			encryptionKeyHex.replace("0x", "");
			fromHex(meterConfig->encryptionKey, encryptionKeyHex, 16);
		}

		String authenticationKeyHex = server.arg("a");
		if(!authenticationKeyHex.isEmpty()) {
			authenticationKeyHex.replace("0x", "");
			fromHex(meterConfig->authenticationKey, authenticationKeyHex, 16);
		}
		config->setMeterConfig(*meterConfig);
	}

	if(server.hasArg("ma") && server.arg("ma") == "true") {
		printD("Received meter advanced config");
		config->getMeterConfig(*meterConfig);
		meterConfig->wattageMultiplier = server.arg("wm").toDouble() * 1000;
		meterConfig->voltageMultiplier = server.arg("vm").toDouble() * 1000;
		meterConfig->amperageMultiplier = server.arg("am").toDouble() * 1000;
		meterConfig->accumulatedMultiplier = server.arg("cm").toDouble() * 1000;
		config->setMeterConfig(*meterConfig);
	}

	if(server.hasArg("wc") && server.arg("wc") == "true") {
		printD("Received WiFi config");
		WiFiConfig wifi;
		config->clearWifi(wifi);
		strcpy(wifi.ssid, server.arg("s").c_str());
		strcpy(wifi.psk, server.arg("p").c_str());

		if(server.hasArg("st") && server.arg("st").toInt() == 1) {
			strcpy(wifi.ip, server.arg("i").c_str());
			strcpy(wifi.gateway, server.arg("g").c_str());
			strcpy(wifi.subnet, server.arg("sn").c_str());
			strcpy(wifi.dns1, server.arg("d1").c_str());
			strcpy(wifi.dns2, server.arg("d2").c_str());
		}
		if(server.hasArg("h") && !server.arg("h").isEmpty()) {
			strcpy(wifi.hostname, server.arg("h").c_str());
		}
		wifi.power = server.arg("w").toFloat() * 10;
		config->setWiFiConfig(wifi);
	}

	if(server.hasArg("mqc") && server.arg("mqc") == "true") {
		printD("Received MQTT config");
		MqttConfig mqtt;
		if(server.hasArg("m") && server.arg("m") == "true") {
			strcpy(mqtt.host, server.arg("h").c_str());
			strcpy(mqtt.clientId, server.arg("i").c_str());
			strcpy(mqtt.publishTopic, server.arg("t").c_str());
			strcpy(mqtt.subscribeTopic, server.arg("st").c_str());
			strcpy(mqtt.username, server.arg("u").c_str());
			strcpy(mqtt.password, server.arg("pw").c_str());
			mqtt.payloadFormat = server.arg("f").toInt();
			mqtt.ssl = server.arg("s") == "true";

			mqtt.port = server.arg("p").toInt();
			if(mqtt.port == 0) {
				mqtt.port = mqtt.ssl ? 8883 : 1883;
			}
		} else {
			config->clearMqtt(mqtt);
		}
		config->setMqttConfig(mqtt);
	}

	if(server.hasArg("dc") && server.arg("dc") == "true") {
		printD("Received Domoticz config");
		DomoticzConfig domo {
			server.arg("elidx").toInt(),
			server.arg("vl1idx").toInt(),
			server.arg("vl2idx").toInt(),
			server.arg("vl3idx").toInt(),
			server.arg("cl1idx").toInt()
		};
		config->setDomoticzConfig(domo);
	}


	if(server.hasArg("ac") && server.arg("ac") == "true") {
		printD("Received web config");
		webConfig.security = server.arg("as").toInt();
		if(webConfig.security > 0) {
			strcpy(webConfig.username, server.arg("au").c_str());
			strcpy(webConfig.password, server.arg("ap").c_str());
			debugger->setPassword(webConfig.password);
		} else {
			strcpy(webConfig.username, "");
			strcpy(webConfig.password, "");
			debugger->setPassword("");
		}
		config->setWebConfig(webConfig);
	}

	if(server.hasArg("gpioConfig") && server.arg("gpioConfig") == "true") {
		printD("Received GPIO config");
		gpioConfig->hanPin = server.hasArg("hanPin") && !server.arg("hanPin").isEmpty() ? server.arg("hanPin").toInt() : 3;
		gpioConfig->ledPin = server.hasArg("ledPin") && !server.arg("ledPin").isEmpty() ? server.arg("ledPin").toInt() : 0xFF;
		gpioConfig->ledInverted = server.hasArg("ledInverted") && server.arg("ledInverted") == "true";
		gpioConfig->ledPinRed = server.hasArg("ledPinRed") && !server.arg("ledPinRed").isEmpty() ? server.arg("ledPinRed").toInt() : 0xFF;
		gpioConfig->ledPinGreen = server.hasArg("ledPinGreen") && !server.arg("ledPinGreen").isEmpty() ? server.arg("ledPinGreen").toInt() : 0xFF;
		gpioConfig->ledPinBlue = server.hasArg("ledPinBlue") && !server.arg("ledPinBlue").isEmpty() ? server.arg("ledPinBlue").toInt() : 0xFF;
		gpioConfig->ledRgbInverted = server.hasArg("ledRgbInverted") && server.arg("ledRgbInverted") == "true";
		gpioConfig->apPin = server.hasArg("apPin") && !server.arg("apPin").isEmpty() ? server.arg("apPin").toInt() : 0xFF;
		gpioConfig->tempSensorPin = server.hasArg("tempSensorPin") && !server.arg("tempSensorPin").isEmpty() ?server.arg("tempSensorPin").toInt() : 0xFF;
		gpioConfig->tempAnalogSensorPin = server.hasArg("tempAnalogSensorPin") && !server.arg("tempAnalogSensorPin").isEmpty() ?server.arg("tempAnalogSensorPin").toInt() : 0xFF;
		gpioConfig->vccPin = server.hasArg("vccPin") && !server.arg("vccPin").isEmpty() ? server.arg("vccPin").toInt() : 0xFF;
		gpioConfig->vccOffset = server.hasArg("vccOffset") && !server.arg("vccOffset").isEmpty() ? server.arg("vccOffset").toFloat() * 100 : 0;
		gpioConfig->vccMultiplier = server.hasArg("vccMultiplier") && !server.arg("vccMultiplier").isEmpty() ? server.arg("vccMultiplier").toFloat() * 1000 : 1000;
		gpioConfig->vccBootLimit = server.hasArg("vccBootLimit") && !server.arg("vccBootLimit").isEmpty() ? server.arg("vccBootLimit").toFloat() * 10 : 0;
		gpioConfig->vccResistorGnd = server.hasArg("vccResistorGnd") && !server.arg("vccResistorGnd").isEmpty() ? server.arg("vccResistorGnd").toInt() : 0;
		gpioConfig->vccResistorVcc = server.hasArg("vccResistorVcc") && !server.arg("vccResistorVcc").isEmpty() ? server.arg("vccResistorVcc").toInt() : 0;
		config->setGpioConfig(*gpioConfig);
	}

	if(server.hasArg("debugConfig") && server.arg("debugConfig") == "true") {
		printD("Received Debug config");
		DebugConfig debug;
		config->getDebugConfig(debug);
		bool active = debug.serial || debug.telnet;

		debug.telnet = server.hasArg("debugTelnet") && server.arg("debugTelnet") == "true";
		debug.serial = server.hasArg("debugSerial") && server.arg("debugSerial") == "true";
		debug.level = server.arg("debugLevel").toInt();

		if(debug.telnet || debug.serial) {
			if(webConfig.security > 0) {
				debugger->setPassword(webConfig.password);
			} else {
				debugger->setPassword("");
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

	if(server.hasArg("nc") && server.arg("nc") == "true") {
		printD("Received NTP config");
		NtpConfig ntp {
			server.hasArg("n") && server.arg("n") == "true",
			server.hasArg("nd") && server.arg("nd") == "true",
			server.arg("o").toInt() / 10,
			server.arg("so").toInt() / 10
		};
		strcpy(ntp.server, server.arg("ns").c_str());
		config->setNtpConfig(ntp);
	}

	if(server.hasArg("ec") && server.arg("ec") == "true") {
		printD("Received ENTSO-E config");
		EntsoeConfig entsoe;
		strcpy(entsoe.token, server.arg("et").c_str());
		strcpy(entsoe.area, server.arg("ea").c_str());
		strcpy(entsoe.currency, server.arg("ecu").c_str());
		entsoe.multiplier = server.arg("em").toFloat() * 1000;
		config->setEntsoeConfig(entsoe);
	}

	if(server.hasArg("cc") && server.arg("cc") == "true") {
		printD("Received energy accounting config");
		EnergyAccountingConfig eac;
		eac.thresholds[0] = server.arg("t0").toInt();
		eac.thresholds[1] = server.arg("t1").toInt();
		eac.thresholds[2] = server.arg("t2").toInt();
		eac.thresholds[3] = server.arg("t3").toInt();
		eac.thresholds[4] = server.arg("t4").toInt();
		eac.thresholds[5] = server.arg("t5").toInt();
		eac.thresholds[6] = server.arg("t6").toInt();
		eac.thresholds[7] = server.arg("t7").toInt();
		eac.thresholds[8] = server.arg("t8").toInt();
		config->setEnergyAccountingConfig(eac);
	}

	printI("Saving configuration now...");

	//if (debugger->isActive(RemoteDebug::DEBUG)) config->print(debugger);
	if (config->save()) {
		printI("Successfully saved.");
		if(config->isWifiChanged() || performRestart) {
			performRestart = true;
            server.sendHeader("Location","/restart-wait");
            server.send(303);
		} else {
			server.sendHeader("Location", String("/"), true);
			server.send (302, MIME_PLAIN, "");

			hw->setup(gpioConfig, config);
		}
	} else {
		printE("Error saving configuration");
		String html = "<html><body><h1>Error saving configuration!</h1></body></html>";
		server.send(500, MIME_HTML, html);
	}
}

void AmsWebServer::configNtpHtml() {
	printD("Serving /ntp.html over http...");

	if(!checkSecurity(1))
		return;

	NtpConfig ntp;
	config->getNtpConfig(ntp);
	snprintf_P(buf, BufferSize, NTP_HTML,
		ntp.enable ? "checked" : "",
		ntp.offset == 0 ? "selected" : "",
		ntp.offset == 360 ? "selected" : "",
		ntp.offset == 720 ? "selected" : "",
		ntp.summerOffset == 0 ? "selected" : "",
		ntp.summerOffset == 360 ? "selected" : "",
		ntp.server,
		ntp.dhcp ? "checked" : ""
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
	printD("Serving /gpio.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) GPIO_HTML);

	#if defined(CONFIG_IDF_TARGET_ESP32S2)
		html.replace("${gpio.max}", "44");
	#elif defined(ESP32)
		html.replace("${gpio.max}", "39");
	#else
		html.replace("${gpio.max}", "16");
	#endif

	html.replace("${options.han}", getSerialSelectOptions(gpioConfig->hanPin));

	html.replace("${config.ledPin}", gpioConfig->ledPin == 0xFF ? "" : String(gpioConfig->ledPin));
	html.replace("${config.ledInverted}", gpioConfig->ledInverted ? "checked" : "");
	html.replace("${config.ledPinRed}", gpioConfig->ledPinRed == 0xFF ? "" : String(gpioConfig->ledPinRed));
	html.replace("${config.ledPinGreen}", gpioConfig->ledPinGreen == 0xFF ? "" : String(gpioConfig->ledPinGreen));
	html.replace("${config.ledPinBlue}", gpioConfig->ledPinBlue == 0xFF ? "" : String(gpioConfig->ledPinBlue));
	html.replace("${config.ledRgbInverted}", gpioConfig->ledRgbInverted ? "checked" : "");
	html.replace("${config.apPin}", gpioConfig->apPin == 0xFF ? "" : String(gpioConfig->apPin));
	html.replace("${config.tempSensorPin}", gpioConfig->tempSensorPin == 0xFF ? "" : String(gpioConfig->tempSensorPin));
	html.replace("${config.tempAnalogSensorPin}", gpioConfig->tempAnalogSensorPin == 0xFF ? "" : String(gpioConfig->tempAnalogSensorPin));
	html.replace("${config.vccPin}", gpioConfig->vccPin == 0xFF ? "" : String(gpioConfig->vccPin));

	html.replace("${config.vccOffset}", gpioConfig->vccOffset > 0 ? String(gpioConfig->vccOffset / 100.0, 2) : "");
	html.replace("${config.vccMultiplier}", gpioConfig->vccMultiplier > 0 ? String(gpioConfig->vccMultiplier / 1000.0, 2) : "");
	html.replace("${config.vccBootLimit}", gpioConfig->vccBootLimit > 0 ? String(gpioConfig->vccBootLimit / 10.0, 1) : "");

	html.replace("${config.vccResistorGnd}", gpioConfig->vccResistorGnd > 0 ? String(gpioConfig->vccResistorGnd) : "");
	html.replace("${config.vccResistorVcc}", gpioConfig->vccResistorVcc > 0 ? String(gpioConfig->vccResistorVcc) : "");

	server.sendHeader(HEADER_CACHE_CONTROL, "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, MIME_HTML, HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::configDebugHtml() {
	printD("Serving /debugging.html over http...");

	if(!checkSecurity(1))
		return;

	DebugConfig debug;
	config->getDebugConfig(debug);
	snprintf_P(buf, BufferSize, DEBUGGING_HTML,
		debug.telnet ? "checked" : "",
		debug.serial ? "checked" : "",
		debug.level == 1 ? "selected"  : "",
		debug.level == 2 ? "selected"  : "",
		debug.level == 3 ? "selected"  : "",
		debug.level == 4 ? "selected"  : ""
	);

	server.sendHeader(HEADER_CACHE_CONTROL, "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	server.setContentLength(strlen(buf) + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, MIME_HTML, HEAD_HTML);
	server.sendContent(buf);
	server.sendContent_P(FOOT_HTML);
}

String AmsWebServer::getSerialSelectOptions(int selected) {
	String gpioOptions;
	if(selected == 3) {
		gpioOptions += "<option value=\"3\" selected>UART0 (GPIO3)</option>";
	} else {
		gpioOptions += "<option value=\"3\">UART0 (GPIO3)</option>";
	}
	#if defined(CONFIG_IDF_TARGET_ESP32S2)
		int numGpio = 30;
		int gpios[] = {4,5,6,7,8,9,10,11,12,13,14,15,16,17,19,21,22,23,25,32,33,34,35,36,39,40,41,42,43,44};
		if(selected == 18) {
			gpioOptions += "<option value=\"18\" selected>UART1 (GPIO18)</option>";
		} else {
			gpioOptions += "<option value=\"18\">UART1 (GPIO18)</option>";
		}
	#elif defined(ESP32)
		int numGpio = 24;
		int gpios[] = {4,5,6,7,8,10,11,12,13,14,15,17,18,19,21,22,23,25,32,33,34,35,36,39};
		if(selected == 9) {
			gpioOptions += "<option value=\"9\" selected>UART1 (GPIO9)</option>";
		} else {
			gpioOptions += "<option value=\"9\">UART1 (GPIO9)</option>";
		}
		if(selected == 16) {
			gpioOptions += "<option value=\"16\" selected>UART2 (GPIO16)</option>";
		} else {
			gpioOptions += "<option value=\"16\">UART2 (GPIO16)</option>";
		}
	#elif defined(ESP8266)
		int numGpio = 9;
		int gpios[] = {4,5,9,10,12,13,14,15,16};
		if(selected == 113) {
			gpioOptions += "<option value=\"113\" selected>UART2 (GPIO13)</option>";
		} else {
			gpioOptions += "<option value=\"113\">UART2 (GPIO13)</option>";
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
			printE("Upload already in progress");
			String html = "<html><body><h1>Upload already in progress!</h1></body></html>";
			server.send(500, MIME_HTML, html);
		} else if (!LittleFS.begin()) {
			printE("An Error has occurred while mounting LittleFS");
			String html = "<html><body><h1>Unable to mount LittleFS!</h1></body></html>";
			server.send(500, MIME_HTML, html);
		} else {
			uploading = true;
			if(debugger->isActive(RemoteDebug::DEBUG)) {
				debugger->printf("handleFileUpload file: %s\n", path);
			}
			if(LittleFS.exists(path)) {
				LittleFS.remove(path);
			}
		    file = LittleFS.open(path, "w");
			if(debugger->isActive(RemoteDebug::DEBUG)) {
				debugger->printf("handleFileUpload Open file and write: %lu\n", upload.currentSize);
			}
            size_t written = file.write(upload.buf, upload.currentSize);
			if(debugger->isActive(RemoteDebug::DEBUG)) {
				debugger->printf("handleFileUpload Written: %lu\n", written);
			}
	    } 
    } else if(upload.status == UPLOAD_FILE_WRITE) {
		if(debugger->isActive(RemoteDebug::DEBUG)) {
			debugger->printf("handleFileUpload Writing: %lu\n", upload.currentSize);
		}
        if(file) {
            size_t written = file.write(upload.buf, upload.currentSize);
			if(debugger->isActive(RemoteDebug::DEBUG)) {
				debugger->printf("handleFileUpload Written: %lu\n", written);
			}
			delay(1);
			if(written != upload.currentSize) {
				file.flush();
				file.close();
				LittleFS.remove(path);
				LittleFS.end();

				printE("An Error has occurred while writing file");
				String html = "<html><body><h1>Unable to write file!</h1></body></html>";
				server.send(500, MIME_HTML, html);
			}
		}
    } else if(upload.status == UPLOAD_FILE_END) {
        if(file) {
			file.flush();
            file.close();
//			LittleFS.end();
        } else {
            server.send(500, MIME_PLAIN, "500: couldn't create file");
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
	printD("Serving /firmware.html over http...");

	if(!checkSecurity(1))
		return;

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	String html = String((const __FlashStringHelper*) FIRMWARE_HTML);

	#if defined(ESP8266)
	html.replace("{chipset}", "ESP8266");
	#elif defined(CONFIG_IDF_TARGET_ESP32S2)
	html.replace("{chipset}", "ESP32S2");
	#elif defined(ESP32)
	html.replace("{chipset}", "ESP32");
	#endif
	
	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, MIME_HTML, HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::firmwareUpload() {
	if(!checkSecurity(1))
		return;

	HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_START) {
        String filename = upload.filename;
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
		performRestart = true;
		server.sendHeader("Location","/restart-wait");
		server.send(303);
	}
}

const uint8_t githubFingerprint[] = {0x59, 0x74, 0x61, 0x88, 0x13, 0xCA, 0x12, 0x34, 0x15, 0x4D, 0x11, 0x0A, 0xC1, 0x7F, 0xE6, 0x67, 0x07, 0x69, 0x42, 0xF5};

void AmsWebServer::firmwareDownload() {
	if(!checkSecurity(1))
		return;

	printD("Firmware download URL triggered");
	if(server.hasArg("version")) {
		String version = server.arg("version");
		String versionStripped = version.substring(1);
		printI("Downloading firmware...");
		HTTPClient httpClient;
		httpClient.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
		httpClient.setTimeout(20000);
		httpClient.addHeader("User-Agent", "ams2mqtt/" + String(VERSION));

		#if defined(ESP8266)
			WiFiClient client;
			String url = "http://ams2mqtt.no23.cc/releases/download/" + version + "/ams2mqtt-esp8266-" + versionStripped + ".bin";
			/*
			t_httpUpdate_return ret = ESPhttpUpdate.update(client, url, VERSION);
			switch(ret) {
				case HTTP_UPDATE_FAILED:
					printE("[update] Update failed.");
					server.sendHeader("Location","/");
					server.send(303);
					break;
				case HTTP_UPDATE_NO_UPDATES:
					printI("[update] Update no Update.");
					server.sendHeader("Location","/");
					server.send(303);
					break;
				case HTTP_UPDATE_OK:
					printI("[update] Update ok."); // may not be called since we reboot the ESP
					performRestart = true;
					server.sendHeader("Location","/restart-wait");
					server.send(303);
					break;
			}
			
			return;
			*/
		#elif defined(CONFIG_IDF_TARGET_ESP32S2)
			WiFiClientSecure client;
			client.setInsecure();
			String url = "https://github.com/gskjold/AmsToMqttBridge/releases/download/" + version + "/ams2mqtt-esp32s2-" + versionStripped + ".bin";
			httpClient.addHeader("Referer", "https://github.com/gskjold/AmsToMqttBridge/releases");
		#elif defined(ESP32)
			WiFiClientSecure client;
			client.setInsecure();
			String url = "https://github.com/gskjold/AmsToMqttBridge/releases/download/" + version + "/ams2mqtt-esp32-" + versionStripped + ".bin";
			httpClient.addHeader("Referer", "https://github.com/gskjold/AmsToMqttBridge/releases");
		#endif

		if(httpClient.begin(client, url)) {
			printD("HTTP client setup successful");
			int status = httpClient.GET();
			if(status == HTTP_CODE_OK) {
				printD("Received OK from server");
				if(LittleFS.begin()) {
					#if defined(ESP32)
						esp_task_wdt_delete(NULL);
						esp_task_wdt_deinit();
					#elif defined(ESP8266)
						ESP.wdtDisable();
					#endif

					printI("Downloading firmware to LittleFS");
					file = LittleFS.open(FILE_FIRMWARE, "w");
					int len = httpClient.writeToStream(&file);
					file.close();
					LittleFS.end();
					performRestart = true;
					server.sendHeader("Location","/restart-wait");
					server.send(303);
				} else {
					printE("Unable to open LittleFS for writing");
					server.sendHeader("Location","/");
					server.send(303);
				}
			} else {
				printE("Communication error: ");
				printE(httpClient.errorToString(status));
				printI(url);
				printD(httpClient.getString());
				server.sendHeader("Location","/");
				server.send(303);
			}
		} else {
			printE("Unable to configure HTTP client");
			server.sendHeader("Location","/");
			server.send(303);
		}
		httpClient.end();
		client.stop();
	} else {
		printI("No firmware version specified...");
		server.sendHeader("Location","/");
		server.send(303);
	}
}

void AmsWebServer::restartHtml() {
	printD("Serving /restart.html over http...");

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

	printD("Setting restart flag and redirecting");
	performRestart = true;
	server.sendHeader("Location","/restart-wait");
	server.send(303);
}

void AmsWebServer::restartWaitHtml() {
	printD("Serving /restart-wait.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) RESTARTWAIT_HTML);

	WiFiConfig wifi;
	config->getWiFiConfig(wifi);

	if(WiFi.getMode() != WIFI_AP) {
		html.replace("boot.css", BOOTSTRAP_URL);
	}
	if(strlen(wifi.ip) == 0 && WiFi.getMode() != WIFI_AP) {
		html.replace("${ip}", WiFi.localIP().toString());
	} else {
		html.replace("${ip}", wifi.ip);
	}
	html.replace("${hostname}", wifi.hostname);

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(html.length());
	server.send(200, MIME_HTML, html);

	yield();
	if(performRestart) {
		if(ds != NULL) {
			ds->save();
		}
		printI("Rebooting");
		delay(1000);
#if defined(ESP8266)
		ESP.reset();
#elif defined(ESP32)
		ESP.restart();
#endif
		performRestart = false;
	}
}

void AmsWebServer::isAliveCheck() {
	server.sendHeader("Access-Control-Allow-Origin", "*");
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
	printD("Serving /mqtt-ca.html over http...");

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
		server.sendHeader("Location","/config-mqtt");
		server.send(303);
	}
}

void AmsWebServer::mqttCaUpload() {
	if(!checkSecurity(1))
		return;

	uploadFile(FILE_MQTT_CA);
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_END) {
		server.sendHeader("Location","/config-mqtt");
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
		server.sendHeader("Location","/config-mqtt");
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
	printD("Serving /mqtt-cert.html over http...");

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
		server.sendHeader("Location","/config-mqtt");
		server.send(303);
	}
}

void AmsWebServer::mqttCertUpload() {
	if(!checkSecurity(1))
		return;

	uploadFile(FILE_MQTT_CERT);
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_END) {
		server.sendHeader("Location","/config-mqtt");
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
		server.sendHeader("Location","/config-mqtt");
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
	printD("Serving /mqtt-key.html over http...");

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
		server.sendHeader("Location","/config-mqtt");
		server.send(303);
	}
}

void AmsWebServer::mqttKeyUpload() {
	if(!checkSecurity(1))
		return;

	uploadFile(FILE_MQTT_KEY);
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_END) {
		server.sendHeader("Location","/config-mqtt");
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
		server.sendHeader("Location","/config-mqtt");
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

	printD("Performing factory reset");
	if(server.hasArg("perform") && server.arg("perform") == "true") {
		printD("Formatting LittleFS");
		LittleFS.format();
		printD("Clearing configuration");
		config->clear();
		printD("Setting restart flag and redirecting");
		performRestart = true;
		server.sendHeader("Location","/restart-wait");
		server.send(303);
	} else {
		server.sendHeader("Location","/");
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
	printD("Serving /configfile.html over http...");

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
	printD("Serving /configfile.cfg over http...");

	if(!checkSecurity(1))
		return;

	bool includeSecrets = server.hasArg("ic") && server.arg("ic") == "true";
	bool includeWifi = server.hasArg("iw") && server.arg("iw") == "true";
	bool includeMqtt = server.hasArg("im") && server.arg("im") == "true";
	bool includeWeb = server.hasArg("ie") && server.arg("ie") == "true";
	bool includeMeter = server.hasArg("it") && server.arg("it") == "true";
	bool includeGpio = server.hasArg("ig") && server.arg("ig") == "true";
	bool includeDomo = server.hasArg("id") && server.arg("id") == "true";
	bool includeNtp = server.hasArg("in") && server.arg("in") == "true";
	bool includeEntsoe = server.hasArg("is") && server.arg("is") == "true";
	bool includeThresholds = server.hasArg("nh") && server.arg("nh") == "true";
	
	SystemConfig sys;
	config->getSystemConfig(sys);

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);
	server.sendHeader("Content-Disposition", "attachment; filename=configfile.cfg");
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);

	server.send(200, MIME_PLAIN, "amsconfig\n");
	server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("version %s\n"), VERSION));
	server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("boardType %d\n"), sys.boardType));
	
	if(includeWifi) {
		WiFiConfig wifi;
		config->getWiFiConfig(wifi);
		if(includeSecrets) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("hostname %s\n"), wifi.hostname));
		if(includeSecrets) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("ssid %s\n"), wifi.ssid));
		if(includeSecrets) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("psk %s\n"), wifi.psk));
		if(strlen(wifi.ip) > 0) {
			server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("ip %s\n"), wifi.ip));
			if(strlen(wifi.gateway) > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("gateway %s\n"), wifi.gateway));
			if(strlen(wifi.subnet) > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("subnet %s\n"), wifi.subnet));
			if(strlen(wifi.dns1) > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("dns1 %s\n"), wifi.dns1));
			if(strlen(wifi.dns2) > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("dns2 %s\n"), wifi.dns2));
		}
		server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("mdns %d\n"), wifi.mdns ? 1 : 0));
	}
	
	if(includeMqtt) {
		MqttConfig mqtt;
		config->getMqttConfig(mqtt);
		if(strlen(mqtt.host) > 0) {
			server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("mqttHost %s\n"), mqtt.host));
			if(mqtt.port > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("mqttPort %d\n"), mqtt.port));
			if(strlen(mqtt.clientId) > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("mqttClientId %s\n"), mqtt.clientId));
			if(strlen(mqtt.publishTopic) > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("mqttPublishTopic %s\n"), mqtt.publishTopic));
			if(includeSecrets) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("mqttUsername %s\n"), mqtt.username));
			if(includeSecrets) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("mqttPassword %s\n"), mqtt.password));
			server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("mqttPayloadFormat %d\n"), mqtt.payloadFormat));
			server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("mqttSsl %d\n"), mqtt.ssl ? 1 : 0));
		}
	}

	if(includeWeb && includeSecrets) {
		WebConfig web;
		config->getWebConfig(web);
		server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("webSecurity %d\n"), web.security));
		if(web.security > 0) {
			server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("webUsername %s\n"), web.username));
			server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("webPassword %s\n"), web.password));
		}
	}

	if(includeMeter) {
		MeterConfig meter;
		config->getMeterConfig(meter);
		server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("meterBaud %d\n"), meter.baud));
		char parity[4] = "";
		switch(meter.parity) {
			case 2:
				strcpy(parity, "7N1");
				break;
			case 3:
				strcpy(parity, "8N1");
				break;
			case 10:
				strcpy(parity, "7E1");
				break;
			case 11:
				strcpy(parity, "8E1");
				break;
		}
		if(strlen(parity) > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("meterParity %s\n"), parity));
		server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("meterInvert %d\n"), meter.invert ? 1 : 0));
		server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("meterDistributionSystem %d\n"), meter.distributionSystem));
		server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("meterMainFuse %d\n"), meter.mainFuse));
		server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("meterProductionCapacity %d\n"), meter.productionCapacity));
		if(includeSecrets) {
			if(meter.encryptionKey[0] != 0x00) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("meterEncryptionKey %s\n"), toHex(meter.encryptionKey, 16).c_str()));
			if(meter.authenticationKey[0] != 0x00) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("meterAuthenticationKey %s\n"), toHex(meter.authenticationKey, 16).c_str()));
		}
	}

	if(includeGpio) {
		GpioConfig gpio;
		config->getGpioConfig(gpio);
		if(gpio.hanPin != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("gpioHanPin %d\n"), gpio.hanPin));
		if(gpio.apPin != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("gpioApPin %d\n"), gpio.apPin));
		if(gpio.ledPin != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("gpioLedPin %d\n"), gpio.ledPin));
		if(gpio.ledPin != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("gpioLedInverted %d\n"), gpio.ledInverted ? 1 : 0));
		if(gpio.ledPinRed != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("gpioLedPinRed %d\n"), gpio.ledPinRed));
		if(gpio.ledPinGreen != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("gpioLedPinGreen %d\n"), gpio.ledPinGreen));
		if(gpio.ledPinBlue != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("gpioLedPinBlue %d\n"), gpio.ledPinBlue));
		if(gpio.ledPinRed != 0xFF || gpio.ledPinGreen != 0xFF || gpio.ledPinBlue != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("gpioLedRgbInverted %d\n"), gpio.ledRgbInverted ? 1 : 0));
		if(gpio.tempSensorPin != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("gpioTempSensorPin %d\n"), gpio.tempSensorPin));
		if(gpio.tempAnalogSensorPin != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("gpioTempAnalogSensorPin %d\n"), gpio.tempAnalogSensorPin));
		if(gpio.vccPin != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("gpioVccPin %d\n"), gpio.vccPin));
		server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("gpioVccOffset %.2f\n"), gpio.vccOffset / 100.0));
		server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("gpioVccMultiplier %.3f\n"), gpio.vccMultiplier / 1000.0));
		server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("gpioVccBootLimit %.1f\n"), gpio.vccBootLimit / 10.0));
		if(gpio.vccPin != 0xFF && gpio.vccResistorGnd != 0) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("gpioVccResistorGnd %d\n"), gpio.vccResistorGnd));
		if(gpio.vccPin != 0xFF && gpio.vccResistorVcc != 0) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("gpioVccResistorVcc %d\n"), gpio.vccResistorVcc));
	}

	if(includeDomo) {
		DomoticzConfig domo;
		config->getDomoticzConfig(domo);
		server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("domoticzElidx %d\n"), domo.elidx));
		server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("domoticzVl1idx %d\n"), domo.vl1idx));
		server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("domoticzVl2idx %d\n"), domo.vl2idx));
		server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("domoticzVl3idx %d\n"), domo.vl3idx));
		server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("domoticzCl1idx %d\n"), domo.cl1idx));
	}

	if(includeNtp) {
		NtpConfig ntp;
		config->getNtpConfig(ntp);
		server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("ntpEnable %d\n"), ntp.enable ? 1 : 0));
		server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("ntpDhcp %d\n"), ntp.dhcp ? 1 : 0));
		server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("ntpOffset %d\n"), ntp.offset * 10));
		server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("ntpSummerOffset %d\n"), ntp.summerOffset * 10));
		server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("ntpServer %s\n"), ntp.server));
	}

	if(includeEntsoe) {
		EntsoeConfig entsoe;
		config->getEntsoeConfig(entsoe);
		if(strlen(entsoe.token) == 36 && includeSecrets) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("entsoeToken %s\n"), entsoe.token));
		server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("entsoeArea %s\n"), entsoe.area));
		server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("entsoeCurrency %s\n"), entsoe.currency));
		server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("entsoeMultiplier %.3f\n"), entsoe.multiplier / 1000.0));
	}

	if(includeThresholds) {
		EnergyAccountingConfig eac;
		config->getEnergyAccountingConfig(eac);

		if(eac.thresholds[9] > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("thresholds %d %d %d %d %d %d %d %d %d %d\n"), 
			eac.thresholds[0],
			eac.thresholds[1],
			eac.thresholds[2],
			eac.thresholds[3],
			eac.thresholds[4],
			eac.thresholds[5],
			eac.thresholds[6],
			eac.thresholds[7],
			eac.thresholds[8],
			eac.thresholds[9]
		));
	}


	if(ds != NULL) {
		DayDataPoints day = ds->getDayData();
		server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("dayplot %d %lld %lu %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d"), 
			day.version,
			(int64_t) day.lastMeterReadTime,
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
			server.sendContent(buf, snprintf(buf, BufferSize, " %lu %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n", 
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
			server.sendContent("\n");
		}

		MonthDataPoints month = ds->getMonthData();
		server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("monthplot %d %lld %lu %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d"), 
			month.version,
			(int64_t) month.lastMeterReadTime,
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
			server.sendContent(buf, snprintf_P(buf, BufferSize, " %lu %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n", 
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
			server.sendContent("\n");
		}
	}

	if(ea != NULL) {
		EnergyAccountingData ead = ea->getData();
		server.sendContent(buf, snprintf_P(buf, BufferSize, (char*) F("energyaccounting %d %d %.2f %.2f %.2f %.2f"), 
			ead.version,
			ead.month,
			ead.maxHour / 100.0,
			ead.costYesterday / 100.0,
			ead.costThisMonth / 100.0,
			ead.costLastMonth / 100.0
		));
	}
}

void AmsWebServer::configFileUpload() {
	if(!checkSecurity(1))
		return;

	HTTPUpload& upload = uploadFile(FILE_CFG);
    if(upload.status == UPLOAD_FILE_END) {
		performRestart = true;
		server.sendHeader("Location","/restart-wait");
		server.send(303);
	}
}
