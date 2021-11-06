#include "AmsWebServer.h"
#include "version.h"
#include "AmsStorage.h"
#include "hexutils.h"
#include "AmsData.h"

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
#include "root/entsoe_html.h"
#include "root/ntp_html.h"
#include "root/gpio_html.h"
#include "root/debugging_html.h"
#include "root/restart_html.h"
#include "root/restartwait_html.h"
#include "root/boot_css.h"
#include "root/gaugemeter_js.h"
#include "root/github_svg.h"
#include "root/upload_html.h"
#include "root/delete_html.h"
#include "root/reset_html.h"
#include "root/temperature_html.h"
#include "root/price_html.h"
#include "root/notfound_html.h"
#include "root/data_json.h"
#include "root/tempsensor_json.h"
#include "root/lowmem_html.h"

#include "base64.h"

AmsWebServer::AmsWebServer(RemoteDebug* Debug, HwTools* hw) {
	this->debugger = Debug;
	this->hw = hw;
}

void AmsWebServer::setup(AmsConfiguration* config, GpioConfig* gpioConfig, MeterConfig* meterConfig, AmsData* meterState, MQTTClient* mqtt) {
    this->config = config;
	this->gpioConfig = gpioConfig;
	this->meterConfig = meterConfig;
	this->meterState = meterState;
	this->mqtt = mqtt;

	char jsuri[32];
	snprintf(jsuri, 32, "/application-%s.js", VERSION);

	server.on("/", HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	server.on("/", HTTP_POST, std::bind(&AmsWebServer::handleSetup, this));
	server.on(jsuri, HTTP_GET, std::bind(&AmsWebServer::applicationJs, this));
	server.on("/temperature", HTTP_GET, std::bind(&AmsWebServer::temperature, this));
	server.on("/temperature", HTTP_POST, std::bind(&AmsWebServer::temperaturePost, this));
	server.on("/temperature.json", HTTP_GET, std::bind(&AmsWebServer::temperatureJson, this));
	server.on("/price", HTTP_GET, std::bind(&AmsWebServer::price, this));
	server.on("/meter", HTTP_GET, std::bind(&AmsWebServer::configMeterHtml, this));
	server.on("/wifi", HTTP_GET, std::bind(&AmsWebServer::configWifiHtml, this));
	server.on("/mqtt", HTTP_GET, std::bind(&AmsWebServer::configMqttHtml, this));
	server.on("/web", HTTP_GET, std::bind(&AmsWebServer::configWebHtml, this));
	server.on("/domoticz",HTTP_GET, std::bind(&AmsWebServer::configDomoticzHtml, this));
	server.on("/entsoe",HTTP_GET, std::bind(&AmsWebServer::configEntsoeHtml, this));
	server.on("/boot.css", HTTP_GET, std::bind(&AmsWebServer::bootCss, this));
	server.on("/gaugemeter.js", HTTP_GET, std::bind(&AmsWebServer::gaugemeterJs, this)); 
	server.on("/github.svg", HTTP_GET, std::bind(&AmsWebServer::githubSvg, this)); 
	server.on("/data.json", HTTP_GET, std::bind(&AmsWebServer::dataJson, this));

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
		server.sendHeader("WWW-Authenticate", "Basic realm=\"Secure Area\"");
		server.setContentLength(0);
		server.send(401, "text/html", "");
	}
	return access;
}

void AmsWebServer::temperature() {
	printD("Serving /temperature.html over http...");

	if(!checkSecurity(2))
		return;

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");

	server.setContentLength(HEAD_HTML_LEN + TEMPERATURE_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, "text/html", HEAD_HTML);
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

	if (debugger->isActive(RemoteDebug::DEBUG)) config->print(debugger);
	if(config->save()) {
		printD("Successfully saved temperature sensors");
		server.sendHeader("Location", String("/temperature"), true);
		server.send (302, "text/plain", "");
	} else {
		printE("Error saving configuration");
		String html = "<html><body><h1>Error saving configuration!</h1></body></html>";
		server.send(500, "text/html", html);
	}
}

void AmsWebServer::temperatureJson() {
	printD("Serving /temperature.json over http...");

	if(!checkSecurity(2))
		return;

	int count = hw->getTempSensorCount();
	int size = 16 + (count * 72);

	char buf[size];
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
		delay(1);
	}
	char* pos = buf+strlen(buf);
	snprintf(count == 0 ? pos : pos-1, 8, "]}");

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");

	server.setContentLength(strlen(buf));
	server.send(200, "application/json", buf);
}

void AmsWebServer::price() {
	printD("Serving /price.html over http...");

	if(!checkSecurity(2))
		return;

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");

	if(ESP.getFreeHeap() > 25000) {
		String html = String((const __FlashStringHelper*) PRICE_HTML);
		for(int i = 0; i < 24; i++) {
			tmElements_t tm;
			breakTime(tz->toLocal(time(nullptr)) + (SECS_PER_HOUR * i), tm);
			char ts[5];
			sprintf(ts, "%02d:00", tm.Hour);
			html.replace("${time" + String(i) + "}", String(ts));

			if(eapi != NULL) {
				double price = eapi->getValueForHour(i);
				if(price == ENTSOE_NO_VALUE) {
					html.replace("${price" + String(i) + "}", "--");
				} else {
					html.replace("${price" + String(i) + "}", String(price, 4));
				}
			} else {
				html.replace("${price" + String(i) + "}", "--");
			}
		}

		server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
		server.send_P(200, "text/html", HEAD_HTML);
		server.sendContent(html);
		server.sendContent_P(FOOT_HTML);
	} else {
		server.setContentLength(LOWMEM_HTML_LEN + HEAD_HTML_LEN + FOOT_HTML_LEN);
		server.send_P(200, "text/html", HEAD_HTML);
		server.sendContent_P(LOWMEM_HTML);
		server.sendContent_P(FOOT_HTML);
	}

}

void AmsWebServer::indexHtml() {
	printD("Serving /index.html over http...");

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");

	if(WiFi.getMode() == WIFI_AP) {
		SystemConfig sys;
		config->getSystemConfig(sys);

		WiFiConfig wifi;
		config->clearWifi(wifi);

		String html = String((const __FlashStringHelper*) SETUP_HTML);
		for(int i = 0; i<255; i++) {
			html.replace("${config.boardType" + String(i) + "}", sys.boardType == i ? "selected"  : "");
		}
		for(int i = 0; i<5; i++) {
			html.replace("${config.meterType" + String(i) + "}", sys.boardType == i ? "selected"  : "");
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
		server.send(200, "text/html", html);
	} else {
		if(!checkSecurity(2))
			return;

		String html = String((const __FlashStringHelper*) INDEX_HTML);

		double u1 = meterState->getL1Voltage();
		double u2 = meterState->getL2Voltage();
		double u3 = meterState->getL3Voltage();
		double i1 = meterState->getL1Current();
		double i2 = meterState->getL2Current();
		double i3 = meterState->getL3Current();
		double tpi = meterState->getActiveImportCounter();
		double tpo = meterState->getActiveExportCounter();
		double tqi = meterState->getReactiveImportCounter();
		double tqo = meterState->getReactiveExportCounter();

		html.replace("{P}", String(meterState->getActiveImportPower()));
		html.replace("{PO}", String(meterState->getActiveExportPower()));
		html.replace("{de}", meterConfig->productionCapacity > 0 ? "" : "none");
		html.replace("{dn}", meterConfig->productionCapacity > 0 ? "none" : "");
		html.replace("{ti}", meterConfig->productionCapacity > 0 ? "Import" : "Use");
		html.replace("{3p}", meterState->isThreePhase() ? "" : "none");

		html.replace("{U1}", u1 > 0 ? String(u1, 1) : "");
		html.replace("{I1}", u1 > 0 ? String(i1, 1) : "");

		html.replace("{U2}", u2 > 0 ? String(u2, 1) : "");
		html.replace("{I2}", u2 > 0 ? String(i2, 1) : "");

		html.replace("{U3}", u3 > 0 ? String(u3, 1) : "");
		html.replace("{I3}", u3 > 0 ? String(i3, 1) : "");

		html.replace("{tPI}", tpi > 0 ? String(tpi, 1) : "");
		html.replace("{tPO}", tpi > 0 ? String(tpo, 1) : "");
		html.replace("{tQI}", tpi > 0 ? String(tqi, 1) : "");
		html.replace("{tQO}", tpi > 0 ? String(tqo, 1) : "");
		html.replace("{da}", tpi > 0 ? "" : "none");

		double vcc = hw->getVcc();
		html.replace("{vcc}", vcc > 0 ? String(vcc, 2) : "");

		double temp = hw->getTemperature();
		html.replace("{temp}", temp > 0 ? String(temp, 1) : "");

		int rssi = hw->getWifiRssi();
		html.replace("{rssi}", String(rssi));

		html.replace("{mem}", String(ESP.getFreeHeap()/1000, 1));

		html.replace("{cs}", String((uint32_t)(millis64()/1000), 10));

		server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
		server.send_P(200, "text/html", HEAD_HTML);
		server.sendContent(html);
		server.sendContent_P(FOOT_HTML);
	}
}

void AmsWebServer::applicationJs() {
	printD("Serving /application.js over http...");

	server.sendHeader("Cache-Control", "public, max-age=3600");
	server.send_P(200, "application/javascript", APPLICATION_JS);
}

void AmsWebServer::configMeterHtml() {
	printD("Serving /meter.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) METER_HTML);

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	String manufacturer;
	switch(meterState->getMeterType()) {
		case AmsTypeAidon:
			manufacturer = "Aidon";
			break;
		case AmsTypeKamstrup:
			manufacturer = "Kamstrup";
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
	html.replace("{b115200}", meterConfig->baud == 115200 ? "selected"  : "");
	html.replace("{c}", String(meterConfig->baud));
	html.replace("{c3}", meterConfig->parity == 3 ? "selected"  : "");
	html.replace("{c11}", meterConfig->parity == 11 ? "selected"  : "");
	html.replace("{i}", meterConfig->invert ? "checked"  : "");
	html.replace("{d}", String(meterConfig->distributionSystem));
	for(int i = 0; i<3; i++) {
		html.replace("{d" + String(i) + "}", meterConfig->distributionSystem == i ? "selected"  : "");
	}
	html.replace("{f}", String(meterConfig->mainFuse));
	for(int i = 0; i<64; i++) {
		html.replace("{f" + String(i) + "}", meterConfig->mainFuse == i ? "selected"  : "");
	}
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
	server.send_P(200, "text/html", HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::configWifiHtml() {
	printD("Serving /wifi.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) WIFI_HTML);

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	WiFiConfig wifi;
	config->getWiFiConfig(wifi);

	html.replace("{s}", wifi.ssid);
	html.replace("{p}", wifi.psk);
	html.replace("{st}", strlen(wifi.ip) > 0 ? "checked" : "");
	html.replace("{i}", wifi.ip);
	html.replace("{g}", wifi.gateway);
	html.replace("{sn}", wifi.subnet);
	html.replace("{d1}", wifi.dns1);
	html.replace("{d2}", wifi.dns2);
	html.replace("{h}", wifi.hostname);
	html.replace("{m}", wifi.mdns ? "checked" : "");

	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, "text/html", HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::configMqttHtml() {
	printD("Serving /mqtt.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) MQTT_HTML);

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

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
	for(int i = 0; i<4; i++) {
		html.replace("{f" + String(i) + "}", mqtt.payloadFormat == i ? "selected"  : "");
	}

	html.replace("{s}", mqtt.ssl ? "checked" : "");

	if(SPIFFS.begin()) {
		html.replace("{dcu}", SPIFFS.exists(FILE_MQTT_CA) ? "none" : "");
		html.replace("{dcf}", SPIFFS.exists(FILE_MQTT_CA) ? "" : "none");
		html.replace("{deu}", SPIFFS.exists(FILE_MQTT_CERT) ? "none" : "");
		html.replace("{def}", SPIFFS.exists(FILE_MQTT_CERT) ? "" : "none");
		html.replace("{dku}", SPIFFS.exists(FILE_MQTT_KEY) ? "none" : "");
		html.replace("{dkf}", SPIFFS.exists(FILE_MQTT_KEY) ? "" : "none");
		SPIFFS.end();
	} else {
		html.replace("{dcu}", "");
		html.replace("{dcf}", "none");
		html.replace("{deu}", "");
		html.replace("{def}", "none");
		html.replace("{dku}", "");
		html.replace("{dkf}", "none");
	}

	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, "text/html", HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::configDomoticzHtml() {
	printD("Serving /domoticz.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) DOMOTICZ_HTML);

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	DomoticzConfig domo;
	config->getDomoticzConfig(domo);

	html.replace("{elidx}", domo.elidx ? String(domo.elidx, 10) : "");
	html.replace("{vl1idx}", domo.vl1idx ? String(domo.vl1idx, 10) : "");
	html.replace("{vl2idx}", domo.vl2idx ? String(domo.vl2idx, 10) : "");
	html.replace("{vl3idx}", domo.vl3idx ? String(domo.vl3idx, 10) : "");
	html.replace("{cl1idx}", domo.cl1idx ? String(domo.cl1idx, 10) : "");

	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, "text/html", HEAD_HTML);
	server.sendContent(html);
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

		html.replace("{ecNOK}", strcmp(entsoe.area, "NOK") == 0 ? "selected" : "");
		html.replace("{ecSEK}", strcmp(entsoe.area, "SEK") == 0 ? "selected" : "");
		html.replace("{ecDKK}", strcmp(entsoe.area, "DKK") == 0 ? "selected" : "");
		html.replace("{ecEUR}", strcmp(entsoe.area, "EUR") == 0 ? "selected" : "");

		server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
		server.send_P(200, "text/html", HEAD_HTML);
		server.sendContent(html);
		server.sendContent_P(FOOT_HTML);
	} else {
		server.setContentLength(LOWMEM_HTML_LEN + HEAD_HTML_LEN + FOOT_HTML_LEN);
		server.send_P(200, "text/html", HEAD_HTML);
		server.sendContent_P(LOWMEM_HTML);
		server.sendContent_P(FOOT_HTML);
	}
}

void AmsWebServer::configWebHtml() {
	printD("Serving /web.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) WEB_HTML);

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	html.replace("{as}", String(webConfig.security));
	for(int i = 0; i<3; i++) {
		html.replace("{as" + String(i) + "}", webConfig.security == i ? "selected"  : "");
	}
	html.replace("{au}", webConfig.username);
	html.replace("{ap}", webConfig.password);

	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, "text/html", HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::bootCss() {
	printD("Serving /boot.css over http...");

	server.sendHeader("Cache-Control", "public, max-age=3600");
	server.send_P(200, "text/css", BOOT_CSS);
}

void AmsWebServer::gaugemeterJs() {
	printD("Serving /gaugemeter.js over http...");

	server.sendHeader("Cache-Control", "public, max-age=3600");
	server.send_P(200, "application/javascript", GAUGEMETER_JS);
}

void AmsWebServer::githubSvg() {
	printD("Serving /github.svg over http...");

	server.sendHeader("Cache-Control", "public, max-age=3600");
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
		espStatus = 0;
	} else if(vcc > 3.1 && vcc < 3.5) {
		espStatus = 1;
	} else if(vcc > 3.0 && vcc < 3.6) {
		espStatus = 2;
	} else {
		espStatus = 3;
	}
	#elif defined(ESP32)
	if(vcc == 0) {
		espStatus = 0;
	} else if(vcc > 2.8 && vcc < 3.5) {
		espStatus = 1;
	} else if(vcc > 2.2 && vcc < 3.6) {
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
	} else if(mqtt->connected()) {
		mqttStatus = 1;
	} else if(mqtt->lastError() == 0) {
		mqttStatus = 2;
	} else {
		mqttStatus = 3;
	}

	char json[290];
	snprintf_P(json, sizeof(json), DATA_JSON,
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
		vcc,
		rssi,
		hw->getTemperature(),
		(uint32_t) (now / 1000),
		ESP.getFreeHeap(),
		espStatus,
		hanStatus,
		wifiStatus,
		mqttStatus,
		(int) mqtt->lastError()
	);

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");

	server.setContentLength(strlen(json));
	server.send(200, "application/json", json);
}

void AmsWebServer::handleSetup() {
	printD("Handling setup method from http");

	if(!server.hasArg("wifiSsid") || server.arg("wifiSsid").isEmpty() || !server.hasArg("wifiPassword") || server.arg("wifiPassword").isEmpty()) {
		server.sendHeader("Location", String("/"), true);
		server.send (302, "text/plain", "");
	} else {
		SystemConfig sys { server.arg("board").toInt() };

		DebugConfig debugConfig;
		config->getDebugConfig(debugConfig);

		config->clear();

		config->clearGpio(*gpioConfig);
		config->clearMeter(*meterConfig);

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
				gpioConfig->apPin = 0;
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
		if(!config->setMeterConfig(*meterConfig)) {
			printD("Unable to set meter config");
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
			server.send(500, "text/html", html);
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
		meterConfig->baud = server.arg("b").toInt();
		meterConfig->parity = server.arg("c").toInt();
		meterConfig->invert = server.hasArg("i") && server.arg("i") == "true";
		meterConfig->distributionSystem = server.arg("d").toInt();
		meterConfig->mainFuse = server.arg("f").toInt();
		meterConfig->productionCapacity = server.arg("p").toInt();

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
		config->setGpioConfig(*gpioConfig);
	}

	if(server.hasArg("debugConfig") && server.arg("debugConfig") == "true") {
		printD("Received Debug config");
		DebugConfig debug;
		debug.telnet = server.hasArg("debugTelnet") && server.arg("debugTelnet") == "true";
		debug.serial = server.hasArg("debugSerial") && server.arg("debugSerial") == "true";
		debug.level = server.arg("debugLevel").toInt();

		debugger->stop();
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

	printI("Saving configuration now...");

	if (debugger->isActive(RemoteDebug::DEBUG)) config->print(debugger);
	if (config->save()) {
		printI("Successfully saved.");
		if(config->isWifiChanged()) {
			performRestart = true;
            server.sendHeader("Location","/restart-wait");
            server.send(303);
		} else {
			server.sendHeader("Location", String("/"), true);
			server.send (302, "text/plain", "");

			hw->setup(gpioConfig, config);
		}
	} else {
		printE("Error saving configuration");
		String html = "<html><body><h1>Error saving configuration!</h1></body></html>";
		server.send(500, "text/html", html);
	}
}

void AmsWebServer::configNtpHtml() {
	printD("Serving /ntp.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) NTP_HTML);

	NtpConfig ntp;
	config->getNtpConfig(ntp);

	html.replace("{n}", ntp.enable ? "checked" : "");

	for(int i = (3600*-13); i<(3600*15); i+=3600) {
		html.replace("{o" + String(i) + "}", ntp.offset * 10 == i ? "selected"  : "");
	}

	for(int i = 0; i<(3600*3); i+=3600) {
		html.replace("{so" + String(i) + "}", ntp.summerOffset * 10 == i ? "selected"  : "");
	}

	html.replace("{ns}", ntp.server);
	html.replace("{nd}", ntp.dhcp ? "checked" : "");

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, "text/html", HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::configGpioHtml() {
	printD("Serving /gpio.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) GPIO_HTML);

	#if defined(ESP32)
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

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, "text/html", HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::configDebugHtml() {
	printD("Serving /debugging.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) DEBUGGING_HTML);

	DebugConfig debug;
	config->getDebugConfig(debug);

	html.replace("${config.debugTelnet}", debug.telnet ? "checked" : "");
	html.replace("${config.debugSerial}", debug.serial ? "checked" : "");
	html.replace("${config.debugLevel}", String(debug.level));
	for(int i = 0; i<=RemoteDebug::ANY; i++) {
		html.replace("${config.debugLevel" + String(i) + "}", debug.level == i ? "selected"  : "");
	}

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, "text/html", HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

String AmsWebServer::getSerialSelectOptions(int selected) {
	String gpioOptions;
	if(selected == 3) {
		gpioOptions += "<option value=\"3\" selected>UART0 (GPIO3)</option>";
	} else {
		gpioOptions += "<option value=\"3\">UART0 (GPIO3)</option>";
	}
	#if defined(ESP32)
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

void AmsWebServer::uploadFile(const char* path) {
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_START){
		if(uploading) {
			printE("Upload already in progress");
			String html = "<html><body><h1>Upload already in progress!</h1></body></html>";
			server.send(500, "text/html", html);
		} else if (!SPIFFS.begin()) {
			printE("An Error has occurred while mounting SPIFFS");
			String html = "<html><body><h1>Unable to mount SPIFFS!</h1></body></html>";
			server.send(500, "text/html", html);
		} else {
			uploading = true;
			if(debugger->isActive(RemoteDebug::DEBUG)) {
				debugger->printf("handleFileUpload file: %s\n", path);
			}
			#if defined(ESP32)
				if(debugger->isActive(RemoteDebug::DEBUG)) {
					debugger->printf("handleFileUpload Free heap: %lu\n", ESP.getFreeHeap());
					debugger->printf("handleFileUpload SPIFFS size: %lu\n", SPIFFS.totalBytes());
					debugger->printf("handleFileUpload SPIFFS used: %lu\n", SPIFFS.usedBytes());
					debugger->printf("handleFileUpload SPIFFS free: %lu\n", SPIFFS.totalBytes()-SPIFFS.usedBytes());
				}
			#endif
		    file = SPIFFS.open(path, "w");
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
				#if defined(ESP32)
					if(debugger->isActive(RemoteDebug::DEBUG)) {
						debugger->printf("handleFileUpload Free heap: %lu\n", ESP.getFreeHeap());
						debugger->printf("handleFileUpload SPIFFS size: %lu\n", SPIFFS.totalBytes());
						debugger->printf("handleFileUpload SPIFFS used: %lu\n", SPIFFS.usedBytes());
						debugger->printf("handleFileUpload SPIFFS free: %lu\n", SPIFFS.totalBytes()-SPIFFS.usedBytes());
					}
				#endif

				file.flush();
				file.close();
				SPIFFS.remove(path);
				SPIFFS.end();

				printE("An Error has occurred while writing file");
				String html = "<html><body><h1>Unable to write file!</h1></body></html>";
				server.send(500, "text/html", html);
			}
		}
    } else if(upload.status == UPLOAD_FILE_END) {
        if(file) {
			file.flush();
            file.close();
		    file = SPIFFS.open(path, "r");
			if(debugger->isActive(RemoteDebug::DEBUG)) {
				debugger->printf("handleFileUpload Size: %lu\n", upload.totalSize);
				debugger->printf("handleFileUpload File size: %lu\n", file.size());
			}
            file.close();
			SPIFFS.end();
        } else {
            server.send(500, "text/plain", "500: couldn't create file");
        }
    }
}

void AmsWebServer::deleteFile(const char* path) {
	if(SPIFFS.begin()) {
		SPIFFS.remove(path);
		SPIFFS.end();
	}
}

void AmsWebServer::firmwareHtml() {
	printD("Serving /firmware.html over http...");

	if(!checkSecurity(1))
		return;

	uploadHtml("Firmware", "/firmware", "system");
}

void AmsWebServer::firmwareUpload() {
	if(!checkSecurity(1))
		return;

	HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_START) {
        String filename = upload.filename;
        if(!filename.endsWith(".bin")) {
            server.send(500, "text/plain", "500: couldn't create file");
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
		WiFiClientSecure client;
#if defined(ESP8266)
		client.setBufferSizes(512, 512);
		String url = "https://github.com/gskjold/AmsToMqttBridge/releases/download/" + version + "/ams2mqtt-esp8266-" + versionStripped + ".bin";
#elif defined(ESP32)
		String url = "https://github.com/gskjold/AmsToMqttBridge/releases/download/" + version + "/ams2mqtt-esp32-" + versionStripped + ".bin";
#endif
		client.setInsecure();
		HTTPClient https;
		https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

		https.addHeader("Referer", "https://github.com/gskjold/AmsToMqttBridge/releases");
		if(https.begin(client, url)) {
			printD("HTTP client setup successful");
			int status = https.GET();
			if(status == HTTP_CODE_OK) {
				printD("Received OK from server");
				if(SPIFFS.begin()) {
					printI("Downloading firmware to SPIFFS");
					file = SPIFFS.open(FILE_FIRMWARE, "w");
					int len = https.writeToStream(&file);
					file.close();
					SPIFFS.end();
					performRestart = true;
					server.sendHeader("Location","/restart-wait");
					server.send(303);
				} else {
					printE("Unable to open SPIFFS for writing");
					server.sendHeader("Location","/");
					server.send(303);
				}
			} else {
				printE("Communication error: ");
				printE(https.errorToString(status));
				printI(url);
				printD(https.getString());
				server.sendHeader("Location","/");
				server.send(303);
			}
		} else {
			printE("Unable to configure HTTP client");
			
			#if defined(ESP8266)
			char buf[64];
			client.getLastSSLError(buf,64);
			printE(buf);
			#endif
			
			server.sendHeader("Location","/");
			server.send(303);
		}
		https.end();
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

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	server.setContentLength(RESTART_HTML_LEN + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, "text/html", HEAD_HTML);
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

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	server.setContentLength(html.length());
	server.send(200, "text/html", html);

	yield();
	if(performRestart) {
		SPIFFS.end();
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
	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	
	server.setContentLength(UPLOAD_HTML_LEN + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, "text/html", HEAD_HTML);
	server.sendContent_P(UPLOAD_HTML);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::deleteHtml(const char* label, const char* action, const char* menu) {
	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	
	server.setContentLength(DELETE_HTML_LEN + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, "text/html", HEAD_HTML);
	server.sendContent_P(DELETE_HTML);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::mqttCa() {
	printD("Serving /mqtt-ca.html over http...");

	if(!checkSecurity(1))
		return;

	if(SPIFFS.begin()) {
		if(SPIFFS.exists(FILE_MQTT_CA)) {
			deleteHtml("CA file", "/mqtt-ca/delete", "mqtt");
		} else {
			uploadHtml("CA file", "/mqtt-ca", "mqtt");
		}
		SPIFFS.end();
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

	if(SPIFFS.begin()) {
		if(SPIFFS.exists(FILE_MQTT_CERT)) {
			deleteHtml("Certificate", "/mqtt-cert/delete", "mqtt");
		} else {
			uploadHtml("Certificate", "/mqtt-cert", "mqtt");
		}
		SPIFFS.end();
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

	if(SPIFFS.begin()) {
		if(SPIFFS.exists(FILE_MQTT_KEY)) {
			deleteHtml("Private key", "/mqtt-key/delete", "mqtt");
		} else {
			uploadHtml("Private key", "/mqtt-key", "mqtt");
		}
		SPIFFS.end();
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

	server.sendHeader("Cache-Control", "public, max-age=3600");
	
	server.setContentLength(RESET_HTML_LEN + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, "text/html", HEAD_HTML);
	server.sendContent_P(RESET_HTML);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::factoryResetPost() {
	if(!checkSecurity(1))
		return;

	printD("Performing factory reset");
	if(server.hasArg("perform") && server.arg("perform") == "true") {
		printD("Formatting SPIFFS");
		SPIFFS.format();
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
	server.sendHeader("Cache-Control", "public, max-age=3600");
	
	server.setContentLength(NOTFOUND_HTML_LEN + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(404, "text/html", HEAD_HTML);
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
