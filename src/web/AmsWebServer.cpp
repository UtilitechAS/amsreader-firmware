#include "AmsWebServer.h"
#include "version.h"
#include "AmsStorage.h"

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

#include "base64.h"

AmsWebServer::AmsWebServer(RemoteDebug* Debug, HwTools* hw, EntsoeApi* eapi) {
	this->debugger = Debug;
	this->hw = hw;
	this->eapi = eapi;
}

void AmsWebServer::setup(AmsConfiguration* config, MQTTClient* mqtt) {
    this->config = config;
	this->mqtt = mqtt;

	server.on("/", HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	server.on("/", HTTP_POST, std::bind(&AmsWebServer::handleSetup, this));
	server.on("/application.js", HTTP_GET, std::bind(&AmsWebServer::applicationJs, this));
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

	TimeChangeRule STD = {"STD", Last, Sun, Oct, 3, config->getNtpOffset() / 60};
	TimeChangeRule DST = {"DST", Last, Sun, Mar, 2, (config->getNtpOffset() + config->getNtpSummerOffset()) / 60};
	tz = new Timezone(DST, STD);
	
	server.begin(); // Web server start
}

void AmsWebServer::loop() {
	server.handleClient();
}


void AmsWebServer::setData(AmsData& data) {
	millis64(); // Make sure it catch all those rollovers

	this->data.apply(data);

	if(maxPwr == 0 && data.getListType() > 1 && config->hasConfig() && config->getMainFuse() > 0 && config->getDistributionSystem() > 0) {
		int volt = config->getDistributionSystem() == 2 ? 400 : 230;
		if(data.isThreePhase()) {
			maxPwr = config->getMainFuse() * sqrt(3) * volt;
		} else {
			maxPwr = config->getMainFuse() * 230;
		}
	}
}

bool AmsWebServer::checkSecurity(byte level) {
	bool access = WiFi.getMode() == WIFI_AP || !config->hasConfig() || config->getAuthSecurity() < level;
	if(!access && config->getAuthSecurity() >= level && server.hasHeader("Authorization")) {
		printD(" forcing web security");
		String expectedAuth = String(config->getAuthUser()) + ":" + String(config->getAuthPassword());

		String providedPwd = server.header("Authorization");
		providedPwd.replace("Basic ", "");

		String expectedBase64 = base64::encode(expectedAuth);
		access = providedPwd.equals(expectedBase64);
	}

	if(!access) {
		printD(" no access, requesting user/pass");
		server.sendHeader("WWW-Authenticate", "Basic realm=\"Secure Area\"");
		server.setContentLength(0);
		server.send(401, "text/html", "");
	}
	if(access)
		printD(" access granted");
	else
		printD(" access denied");
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

	if(config->save()) {
		printD("Successfully saved temperature sensors");
		server.sendHeader("Location", String("/temperature"), true);
		server.send (302, "text/plain", "");

		uint8_t c = config->getTempSensorCount();
		for(int i = 0; i < c; i++) {
			TempSensorConfig* tsc = config->getTempSensorConfig(i);
			hw->confTempSensor(tsc->address, tsc->name, tsc->common);
			delay(1);
		}
	}
}

void AmsWebServer::temperatureJson() {
	printD("Serving /temperature.json over http...");

	if(!checkSecurity(2))
		return;

	int count = hw->getTempSensorCount();

	StaticJsonDocument<4096> json;
	json["c"] = count;
	JsonArray sensors = json.createNestedArray("s");
	for(int i = 0; i < count; i++) {
		TempSensorData* data = hw->getTempSensorData(i);
		JsonObject obj = sensors.createNestedObject();
		obj["i"] = i;
		obj["a"] = toHex(data->address, 8);
		obj["n"] = String(data->name).substring(0,16);
		obj["v"] = String(data->lastRead, 2);
		obj["c"] = data->common;
		delay(1);
	}

    String jsonStr;
	serializeJson(json, jsonStr);

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");

	server.setContentLength(jsonStr.length());
	server.send(200, "application/json", jsonStr);
}

void AmsWebServer::price() {
	printD("Serving /price.html over http...");

	if(!checkSecurity(2))
		return;

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");

	String html = String((const __FlashStringHelper*) PRICE_HTML);
	for(int i = 0; i < 24; i++) {
        tmElements_t tm;
        breakTime(tz->toLocal(time(nullptr)) + (SECS_PER_HOUR * i), tm);
		char ts[5];
		sprintf(ts, "%02d:00", tm.Hour);
		html.replace("${time" + String(i) + "}", String(ts));

		double price = eapi->getValueForHour(i);
		if(price == ENTSOE_NO_VALUE) {
			html.replace("${price" + String(i) + "}", "--");
		} else {
			html.replace("${price" + String(i) + "}", String(price, 4));
		}
	}

	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, "text/html", HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::indexHtml() {
	printD("Serving /index.html over http...");

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");

	if(WiFi.getMode() == WIFI_AP) {
		String html = String((const __FlashStringHelper*) SETUP_HTML);
		for(int i = 0; i<255; i++) {
			html.replace("${config.boardType" + String(i) + "}", config->getBoardType() == i ? "selected"  : "");
		}
		for(int i = 0; i<5; i++) {
			html.replace("${config.meterType" + String(i) + "}", config->getMeterType() == i ? "selected"  : "");
		}
		html.replace("${config.wifiSsid}", config->getWifiSsid());
		html.replace("${config.wifiPassword}", config->getWifiPassword());
		html.replace("${config.wifiStaticIp}", strlen(config->getWifiIp()) > 0 ? "checked" : "");
		html.replace("${config.wifiIp}", config->getWifiIp());
		html.replace("${config.wifiGw}", config->getWifiGw());
		html.replace("${config.wifiSubnet}", config->getWifiSubnet());
		html.replace("${config.wifiDns1}", config->getWifiDns1());
		html.replace("${config.wifiDns2}", config->getWifiDns2());
		html.replace("${config.wifiHostname}", config->getWifiHostname());
		server.send(200, "text/html", html);
	} else {
		if(!checkSecurity(2))
			return;

		String html = String((const __FlashStringHelper*) INDEX_HTML);

		double u1 = data.getL1Voltage();
		double u2 = data.getL2Voltage();
		double u3 = data.getL3Voltage();
		double i1 = data.getL1Current();
		double i2 = data.getL2Current();
		double i3 = data.getL3Current();
		double tpi = data.getActiveImportCounter();
		double tpo = data.getActiveExportCounter();
		double tqi = data.getReactiveImportCounter();
		double tqo = data.getReactiveExportCounter();

		html.replace("{P}", String(data.getActiveImportPower()));
		html.replace("{PO}", String(data.getActiveExportPower()));
		html.replace("{de}", config->getProductionCapacity() > 0 ? "" : "none");
		html.replace("{dn}", config->getProductionCapacity() > 0 ? "none" : "");
		html.replace("{ti}", config->getProductionCapacity() > 0 ? "Import" : "Use");
		html.replace("{3p}", data.isThreePhase() ? "" : "none");

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
		html.replace("{rssi}", vcc > 0 ? String(rssi) : "");

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

	html.replace("{m}", String(config->getMainFuse()));
	for(int i = 0; i<5; i++) {
		html.replace("{m" + String(i) + "}", config->getMeterType() == i ? "selected"  : "");
	}
	html.replace("{d}", String(config->getDistributionSystem()));
	for(int i = 0; i<3; i++) {
		html.replace("{d" + String(i) + "}", config->getDistributionSystem() == i ? "selected"  : "");
	}
	html.replace("{f}", String(config->getMainFuse()));
	for(int i = 0; i<64; i++) {
		html.replace("{f" + String(i) + "}", config->getMainFuse() == i ? "selected"  : "");
	}
	html.replace("{p}", String(config->getProductionCapacity()));

	if(config->getMeterType() == METER_TYPE_OMNIPOWER) {
		String encryptionKeyHex = "0x";
		encryptionKeyHex += toHex(config->getMeterEncryptionKey(), 16);
		html.replace("{e}", encryptionKeyHex);

		String authenticationKeyHex = "0x";
		authenticationKeyHex += toHex(config->getMeterAuthenticationKey(), 16);
		html.replace("{a}", authenticationKeyHex);
	} else {
		html.replace("{e}", "");
		html.replace("{a}", "");
	}

	html.replace("{s}", config->isSubstituteMissing() ? "checked" : "");
	html.replace("{u}", config->isSendUnknown() ? "checked" : "");

	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, "text/html", HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

String AmsWebServer::toHex(uint8_t* in, uint8_t size) {
	String hex;
	for(int i = 0; i < size; i++) {
		if(in[i] < 0x10) {
			hex += '0';
		}
		hex += String(in[i], HEX);
	}
	hex.toUpperCase();
	return hex;
}

void AmsWebServer::fromHex(uint8_t *out, String in, uint8_t size) {
	for(int i = 0; i < size*2; i += 2) {
		out[i/2] = strtol(in.substring(i, i+2).c_str(), 0, 16);
	}
}

void AmsWebServer::configWifiHtml() {
	printD("Serving /wifi.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) WIFI_HTML);

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	html.replace("{s}", config->getWifiSsid());
	html.replace("{p}", config->getWifiPassword());
	html.replace("{st}", strlen(config->getWifiIp()) > 0 ? "checked" : "");
	html.replace("{i}", config->getWifiIp());
	html.replace("{g}", config->getWifiGw());
	html.replace("{sn}", config->getWifiSubnet());
	html.replace("{d1}", config->getWifiDns1());
	html.replace("{d2}", config->getWifiDns2());
	html.replace("{h}", config->getWifiHostname());
	html.replace("{m}", config->isMdnsEnable() ? "checked" : "");

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

	html.replace("{m}", strlen(config->getMqttHost()) == 0 ? "" : "checked");
	html.replace("{h}", config->getMqttHost());
	if(config->getMqttPort() > 0) {
		html.replace("{p}", String(config->getMqttPort()));
	} else {
		html.replace("{p}", String(1883));
	}
	html.replace("{i}", config->getMqttClientId());
	html.replace("{t}", config->getMqttPublishTopic());
	html.replace("{st}", config->getMqttSubscribeTopic());
	html.replace("{u}", config->getMqttUser());
	html.replace("{pw}", config->getMqttPassword());
	html.replace("{f}", String(config->getMqttPayloadFormat()));
	for(int i = 0; i<4; i++) {
		html.replace("{f" + String(i) + "}", config->getMqttPayloadFormat() == i ? "selected"  : "");
	}

	html.replace("{s}", config->isMqttSsl() ? "checked" : "");

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

	if(config->getDomoELIDX() > 0){ html.replace("{elidx}", String(config->getDomoELIDX()));
	} else { html.replace("{elidx}", ""); }
	if(config->getDomoVL1IDX() > 0){ html.replace("{vl1idx}", String(config->getDomoVL1IDX()));
	} else { html.replace("{vl1idx}", ""); }
	if(config->getDomoVL2IDX() > 0){ html.replace("{vl2idx}", String(config->getDomoVL2IDX()));
	} else { html.replace("{vl2idx}", ""); }
	if(config->getDomoVL3IDX() > 0){ html.replace("{vl3idx}", String(config->getDomoVL3IDX()));
	} else { html.replace("{vl3idx}", ""); }
	if(config->getDomoCL1IDX() > 0){ html.replace("{cl1idx}", String(config->getDomoCL1IDX()));
	} else { html.replace("{cl1idx}", ""); }

	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, "text/html", HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::configEntsoeHtml() {
	printD("Serving /entsoe.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) ENTSOE_HTML);

	html.replace("{et}", config->getEntsoeApiToken());
	html.replace("{em}", String(config->getEntsoeApiMultiplier(), 3));

	html.replace("{eaNo1}", strcmp(config->getEntsoeApiArea(), "10YNO-1--------2") == 0 ? "selected" : "");
	html.replace("{eaNo2}", strcmp(config->getEntsoeApiArea(), "10YNO-2--------T") == 0 ? "selected" : "");
	html.replace("{eaNo3}", strcmp(config->getEntsoeApiArea(), "10YNO-3--------J") == 0 ? "selected" : "");
	html.replace("{eaNo4}", strcmp(config->getEntsoeApiArea(), "10YNO-4--------9") == 0 ? "selected" : "");
	html.replace("{eaNo5}", strcmp(config->getEntsoeApiArea(), "10Y1001A1001A48H") == 0 ? "selected" : "");

	html.replace("{eaSe1}", strcmp(config->getEntsoeApiArea(), "10Y1001A1001A44P") == 0 ? "selected" : "");
	html.replace("{eaSe2}", strcmp(config->getEntsoeApiArea(), "10Y1001A1001A45N") == 0 ? "selected" : "");
	html.replace("{eaSe3}", strcmp(config->getEntsoeApiArea(), "10Y1001A1001A46L") == 0 ? "selected" : "");
	html.replace("{eaSe4}", strcmp(config->getEntsoeApiArea(), "10Y1001A1001A47J") == 0 ? "selected" : "");

	html.replace("{eaDk1}", strcmp(config->getEntsoeApiArea(), "10YDK-1--------W") == 0 ? "selected" : "");
	html.replace("{eaDk2}", strcmp(config->getEntsoeApiArea(), "10YDK-2--------M") == 0 ? "selected" : "");

	html.replace("{ecNOK}", strcmp(config->getEntsoeApiArea(), "NOK") == 0 ? "selected" : "");
	html.replace("{ecSEK}", strcmp(config->getEntsoeApiArea(), "SEK") == 0 ? "selected" : "");
	html.replace("{ecDKK}", strcmp(config->getEntsoeApiArea(), "DKK") == 0 ? "selected" : "");
	html.replace("{ecEUR}", strcmp(config->getEntsoeApiArea(), "EUR") == 0 ? "selected" : "");

	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, "text/html", HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::configWebHtml() {
	printD("Serving /web.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) WEB_HTML);

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	html.replace("{as}", String(config->getAuthSecurity()));
	for(int i = 0; i<3; i++) {
		html.replace("{as" + String(i) + "}", config->getAuthSecurity() == i ? "selected"  : "");
	}
	html.replace("{au}", config->getAuthUser());
	html.replace("{ap}", config->getAuthPassword());

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

	if(!checkSecurity(2))
		return;

	StaticJsonDocument<768> json;

    String jsonStr;
	if(data.getLastUpdateMillis() > 0) {
		int maxPwr = this->maxPwr;
		if(maxPwr == 0) {
			if(data.isThreePhase()) {
				maxPwr = 20000;
			} else {
				maxPwr = 10000;
			}
		}

		json["up"] = data.getLastUpdateMillis();
		json["t"] = data.getPackageTimestamp();
		json.createNestedObject("data");
		json["data"]["P"] = data.getActiveImportPower();
		json["data"]["PO"] = data.getActiveExportPower();

		double u1 = data.getL1Voltage();
		double u2 = data.getL2Voltage();
		double u3 = data.getL3Voltage();
		double i1 = data.getL1Current();
		double i2 = data.getL2Current();
		double i3 = data.getL3Current();
		double tpi = data.getActiveImportCounter();
		double tpo = data.getActiveExportCounter();
		double tqi = data.getReactiveImportCounter();
		double tqo = data.getReactiveExportCounter();

		double volt = u1;
		double amp = i1;

		if(u1 > 0) {
			json["data"]["U1"] = u1;
			json["data"]["I1"] = i1;
		}
		if(u2 > 0) {
			json["data"]["U2"] = u2;
			json["data"]["I2"] = i2;
			if(i2 > amp) amp = i2;
		}
		if(u3 > 0) {
			json["data"]["U3"] = u3;
			json["data"]["I3"] = i3;
			volt = (u1+u2+u3)/3;
			if(i3 > amp) amp = i3;
		}

		if(tpi > 0) {
			json["data"]["tPI"] = tpi;
			json["data"]["tPO"] = tpo;
			json["data"]["tQI"] = tqi;
			json["data"]["tQO"] = tqo;
		}

		json["p_pct"] = min(data.getActiveImportPower()*100/maxPwr, 100);

		json["v"] = volt;
		json["v_pct"] = (max((int)volt-207, 1)*100/46);

		int maxAmp = config->getMainFuse() == 0 ? 32 : config->getMainFuse();

		json["a"] = amp;
		json["a_pct"] = amp * 100 / maxAmp;

		if(config->getProductionCapacity() > 0) {
			int maxPrd = config->getProductionCapacity() * 1000;
			json["po_pct"] = min(data.getActiveExportPower()*100/maxPrd, 100);
		}
	} else {
		json["p_pct"] = -1;
		json["po_pct"] = -1;
	}

	json["id"] = WiFi.macAddress();
	json["maxPower"] = maxPwr;
	json["meterType"] = config->getMeterType();
	json["uptime_seconds"] = millis64() / 1000;
	double vcc = hw->getVcc();
	json["vcc"] = serialized(String(vcc, 3));

	double temp = hw->getTemperature();
	json["temp"] = serialized(String(temp, 2));

	json.createNestedObject("wifi");
	float rssi = WiFi.RSSI();
	rssi = isnan(rssi) ? -100.0 : rssi;
	json["wifi"]["ssid"] = WiFi.SSID();
	json["wifi"]["channel"] = (int) WiFi.channel();
	json["wifi"]["rssi"] = rssi;

	json.createNestedObject("status");

	String espStatus;
	if(vcc == 0) {
		espStatus = "secondary";
	} else if(vcc > 3.1) {
		espStatus = "success";
	} else if(vcc > 2.8) {
		espStatus = "warning";
	} else {
		espStatus = "danger";
	}
	json["status"]["esp"] = espStatus;

	unsigned long now = millis();
	String hanStatus;
	if(config->getMeterType() == 0) {
		hanStatus = "secondary";
	} else if(now - data.getLastUpdateMillis() < 15000) {
		hanStatus = "success";
	} else if(now - data.getLastUpdateMillis() < 30000) {
		hanStatus = "warning";
	} else {
		hanStatus = "danger";
	}
	json["status"]["han"] = hanStatus;

	String wifiStatus;
	if(strlen(config->getWifiSsid()) == 0) {
		wifiStatus = "secondary";
	} else if(rssi > -75) {
		wifiStatus = "success";
	} else if(rssi > -95) {
		wifiStatus = "warning";
	} else {
		wifiStatus = "danger";
	}
	json["status"]["wifi"] = wifiStatus;

	String mqttStatus;
	if(strlen(config->getMqttHost()) == 0) {
		mqttStatus = "secondary";
	} else if(mqtt->connected()) {
		mqttStatus = "success";
	} else if(mqtt->lastError() == 0) {
		mqttStatus = "warning";
	} else {
		mqttStatus = "danger";
	}
	json["status"]["mqtt"] = mqttStatus;

	json.createNestedObject("mqtt");
	json["mqtt"]["lastError"] = (int) mqtt->lastError();

	serializeJson(json, jsonStr);

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");

	server.setContentLength(jsonStr.length());
	server.send(200, "application/json", jsonStr);
}

void AmsWebServer::handleSetup() {
	if(!server.hasArg("wifiSsid") || server.arg("wifiSsid").isEmpty() || !server.hasArg("wifiPassword") || server.arg("wifiPassword").isEmpty()) {
		server.sendHeader("Location", String("/"), true);
		server.send (302, "text/plain", "");
	} else {
		config->setLedPin(0xFF);
		config->setLedPinRed(0xFF);
		config->setLedPinGreen(0xFF);
		config->setLedPinBlue(0xFF);
		config->setApPin(0xFF);
		config->setTempSensorPin(0xFF);
		config->setVccPin(0xFF);

		config->setBoardType(server.arg("board").toInt());
		config->setVccOffset(0.0);
		config->setVccMultiplier(1.0);
		config->setVccBootLimit(0);
		switch(config->getBoardType()) {
			case 0: // roarfred
				config->setHanPin(3);
				config->setApPin(0);
				config->setLedPin(2);
				config->setLedInverted(true);
				config->setTempSensorPin(5);
				break;
			case 1: // Arnio Kamstrup
				config->setHanPin(3);
				config->setApPin(0);
				config->setLedPin(2);
				config->setLedInverted(true);
				config->setTempSensorPin(5);
				config->setLedPinRed(13);
				config->setLedPinGreen(14);
				config->setLedRgbInverted(true);
				break;
			case 2: // spenceme
				config->setHanPin(3);
				config->setApPin(0);
				config->setLedPin(2);
				config->setLedInverted(true);
				config->setTempSensorPin(5);
				config->setVccBootLimit(3.3);
				break;
			case 101: // D1
				config->setHanPin(5);
				config->setApPin(4);
				config->setLedPin(2);
				config->setLedInverted(true);
				config->setTempSensorPin(14);
				config->setVccMultiplier(1.1);
				break;
			case 100: // ESP8266
				config->setHanPin(3);
				config->setLedPin(2);
				config->setLedInverted(true);
				break;
			case 201: // D32
				config->setHanPin(16);
				config->setApPin(4);
				config->setLedPin(5);
				config->setLedInverted(true);
				config->setTempSensorPin(14);
				config->setVccPin(35);
				config->setVccMultiplier(2.25);
				break;
			case 202: // Feather
				config->setHanPin(16);
				config->setLedPin(2);
				config->setLedInverted(false);
				config->setTempSensorPin(14);
				break;
			case 203: // DevKitC
				config->setHanPin(16);
				config->setLedPin(2);
				config->setLedInverted(false);
				break;
			case 200: // ESP32
				config->setHanPin(16);
				config->setApPin(0);
				config->setLedPin(2);
				config->setLedInverted(false);
				config->setTempSensorPin(14);
				break;
		}
		config->setMeterType(server.arg("meterType").toInt());
		config->setWifiSsid(server.arg("wifiSsid").c_str());
		config->setWifiPassword(server.arg("wifiPassword").c_str());
		if(server.hasArg("wifiIpType") && server.arg("wifiIpType").toInt() == 1) {
			config->setWifiIp(server.arg("wifiIp").c_str());
			config->setWifiGw(server.arg("wifiGw").c_str());
			config->setWifiSubnet(server.arg("wifiSubnet").c_str());
			config->setWifiDns1(server.arg("wifiDns1").c_str());
		} else {
			config->clearWifiIp();
		}
		if(server.hasArg("wifiHostname") && !server.arg("wifiHostname").isEmpty()) {
			config->setWifiHostname(server.arg("wifiHostname").c_str());
			config->setMdnsEnable(true);
		} else {
			config->setMdnsEnable(false);
		}
		if(config->save()) {
			performRestart = true;
			server.sendHeader("Location","/restart-wait");
			server.send(303);
		} else {
			printE("Error saving configuration");
			String html = "<html><body><h1>Error saving configuration!</h1></form>";
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
		config->setMeterType(server.arg("m").toInt());
		config->setDistributionSystem(server.arg("d").toInt());
		config->setMainFuse(server.arg("f").toInt());
		config->setProductionCapacity(server.arg("p").toInt());
		config->setSubstituteMissing(server.hasArg("s") && server.arg("substituteMissing") == "true");
		config->setSendUnknown(server.hasArg("u") && server.arg("sendUnknown") == "true");

		String encryptionKeyHex = server.arg("e");
		if(!encryptionKeyHex.isEmpty()) {
			encryptionKeyHex.replace("0x", "");
			uint8_t hexStr[16];
			fromHex(hexStr, encryptionKeyHex, 16);
			config->setMeterEncryptionKey(hexStr);
		}

		String authenticationKeyHex = server.arg("a");
		if(!authenticationKeyHex.isEmpty()) {
			authenticationKeyHex.replace("0x", "");
			uint8_t hexStr[16];
			fromHex(hexStr, encryptionKeyHex, 16);
			config->setMeterAuthenticationKey(hexStr);
		}
	}

	if(server.hasArg("wc") && server.arg("wc") == "true") {
		config->setWifiSsid(server.arg("s").c_str());
		config->setWifiPassword(server.arg("p").c_str());
		if(server.hasArg("st") && server.arg("st").toInt() == 1) {
			config->setWifiIp(server.arg("i").c_str());
			config->setWifiGw(server.arg("g").c_str());
			config->setWifiSubnet(server.arg("sn").c_str());
			config->setWifiDns1(server.arg("d1").c_str());
			config->setWifiDns2(server.arg("d2").c_str());
		} else {
			config->clearWifiIp();
		}
		config->setWifiHostname(server.arg("h").c_str());
		config->setMdnsEnable(server.hasArg("m") && server.arg("m") == "true");
	}

	if(server.hasArg("mqc") && server.arg("mqc") == "true") {
		if(server.hasArg("m") && server.arg("m") == "true") {
			config->setMqttHost(server.arg("h").c_str());
			int port = server.arg("p").toInt();
			config->setMqttPort(port == 0 ? 1883 : port);
			config->setMqttClientId(server.arg("i").c_str());
			config->setMqttPublishTopic(server.arg("t").c_str());
			config->setMqttSubscribeTopic(server.arg("st").c_str());
			config->setMqttUser(server.arg("u").c_str());
			config->setMqttPassword(server.arg("pw").c_str());
			config->setMqttPayloadFormat(server.arg("f").toInt());
			config->setMqttSsl(server.arg("s") == "true");
		} else {
			config->clearMqtt();
		}
	}

	if(server.hasArg("dc") && server.arg("dc") == "true") {
		config->setDomoELIDX(server.arg("elidx").toInt());
		config->setDomoVL1IDX(server.arg("vl1idx").toInt());
		config->setDomoVL2IDX(server.arg("vl2idx").toInt());
		config->setDomoVL3IDX(server.arg("vl3idx").toInt());
		config->setDomoCL1IDX(server.arg("cl1idx").toInt());
	}



	if(server.hasArg("ac") && server.arg("ac") == "true") {
		config->setAuthSecurity((byte)server.arg("as").toInt());
		if(config->getAuthSecurity() > 0) {
			config->setAuthUser(server.arg("au").c_str());
			config->setAuthPassword(server.arg("ap").c_str());
			debugger->setPassword(config->getAuthPassword());
		} else {
			debugger->setPassword("");
			config->clearAuth();
		}
	}

	if(server.hasArg("gpioConfig") && server.arg("gpioConfig") == "true") {
		// Unset all pins to avoid conflicts if GPIO have been swapped between pins
		config->setLedPin(0xFF);
		config->setLedPinRed(0xFF);
		config->setLedPinGreen(0xFF);
		config->setLedPinBlue(0xFF);
		config->setApPin(0xFF);
		config->setTempSensorPin(0xFF);
		config->setTempAnalogSensorPin(0xFF);
		config->setVccPin(0xFF);

		config->setHanPin(server.hasArg("hanPin") && !server.arg("hanPin").isEmpty() ? server.arg("hanPin").toInt() : 3);
		config->setLedPin(server.hasArg("ledPin") && !server.arg("ledPin").isEmpty() ? server.arg("ledPin").toInt() : 0xFF);
		config->setLedInverted(server.hasArg("ledInverted") && server.arg("ledInverted") == "true");
		config->setLedPinRed(server.hasArg("ledPinRed") && !server.arg("ledPinRed").isEmpty() ? server.arg("ledPinRed").toInt() : 0xFF);
		config->setLedPinGreen(server.hasArg("ledPinGreen") && !server.arg("ledPinGreen").isEmpty() ? server.arg("ledPinGreen").toInt() : 0xFF);
		config->setLedPinBlue(server.hasArg("ledPinBlue") && !server.arg("ledPinBlue").isEmpty() ? server.arg("ledPinBlue").toInt() : 0xFF);
		config->setLedRgbInverted(server.hasArg("ledRgbInverted") && server.arg("ledRgbInverted") == "true");
		config->setApPin(server.hasArg("apPin") && !server.arg("apPin").isEmpty() ? server.arg("apPin").toInt() : 0xFF);
		config->setTempSensorPin(server.hasArg("tempSensorPin") && !server.arg("tempSensorPin").isEmpty() ?server.arg("tempSensorPin").toInt() : 0xFF);
		config->setTempAnalogSensorPin(server.hasArg("tempAnalogSensorPin") && !server.arg("tempAnalogSensorPin").isEmpty() ?server.arg("tempAnalogSensorPin").toInt() : 0xFF);
		config->setVccPin(server.hasArg("vccPin") && !server.arg("vccPin").isEmpty() ? server.arg("vccPin").toInt() : 0xFF);
		config->setVccOffset(server.hasArg("vccOffset") && !server.arg("vccOffset").isEmpty() ? server.arg("vccOffset").toDouble() : 0.0);
		config->setVccMultiplier(server.hasArg("vccMultiplier") && !server.arg("vccMultiplier").isEmpty() ? server.arg("vccMultiplier").toDouble() : 1.0);
		config->setVccBootLimit(server.hasArg("vccBootLimit") && !server.arg("vccBootLimit").isEmpty() ? server.arg("vccBootLimit").toDouble() : 0.0);
	}

	if(server.hasArg("debugConfig") && server.arg("debugConfig") == "true") {
		config->setDebugTelnet(server.hasArg("debugTelnet") && server.arg("debugTelnet") == "true");
		config->setDebugSerial(server.hasArg("debugSerial") && server.arg("debugSerial") == "true");
		config->setDebugLevel(server.arg("debugLevel").toInt());

		debugger->stop();
		if(config->getAuthSecurity() > 0) {
			debugger->setPassword(config->getAuthPassword());
		} else {
			debugger->setPassword("");
		}
		debugger->setSerialEnabled(config->isDebugSerial());
		debugger->begin(config->getWifiHostname(), (uint8_t) config->getDebugLevel());
		if(!config->isDebugTelnet()) {
			debugger->stop();
		}
	}

	if(server.hasArg("nc") && server.arg("nc") == "true") {
		config->setNtpEnable(server.hasArg("n") && server.arg("n") == "true");
		config->setNtpDhcp(server.hasArg("nd") && server.arg("nd") == "true");
		if(server.hasArg("o") && !server.arg("o").isEmpty()) {
			int offset = server.arg("o").toInt();
			config->setNtpOffset(offset);
			if(server.hasArg("so") && !server.arg("so").isEmpty()) {
				int summerOffset = server.arg("so").toInt();
				config->setNtpSummerOffset(summerOffset);
			} else {
				config->setNtpSummerOffset(0);
			}
		} else {
			config->setNtpOffset(0);
		}
		config->setNtpServer(server.arg("ns").c_str());
	}

	if(server.hasArg("ec") && server.arg("ec") == "true") {
		config->setEntsoeApiToken(server.arg("et").c_str());
		config->setEntsoeApiArea(server.arg("ea").c_str());
		config->setEntsoeApiCurrency(server.arg("ec").c_str());
		config->setEntsoeApiMultiplier(server.arg("em").toDouble());
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

			hw->setLed(config->getLedPin(), config->isLedInverted());
			hw->setLedRgb(config->getLedPinRed(), config->getLedPinGreen(), config->getLedPinBlue(), config->isLedRgbInverted());
			hw->setTempSensorPin(config->getTempSensorPin());
			hw->setTempAnalogSensorPin(config->getTempAnalogSensorPin());
			hw->setVccPin(config->getVccPin());
			hw->setVccOffset(config->getVccOffset());
			hw->setVccMultiplier(config->getVccMultiplier());

			eapi->setToken(config->getEntsoeApiToken());
			eapi->setArea(config->getEntsoeApiArea());
			eapi->setCurrency(config->getEntsoeApiCurrency());
			eapi->setMultiplier(config->getEntsoeApiMultiplier());
		}
	} else {
		printE("Error saving configuration");
		String html = "<html><body><h1>Error saving configuration!</h1></form>";
		server.send(500, "text/html", html);
	}
}

void AmsWebServer::configNtpHtml() {
	printD("Serving /ntp.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) NTP_HTML);

	html.replace("{n}", config->isNtpEnable() ? "checked" : "");

	for(int i = (3600*-13); i<(3600*15); i+=3600) {
		html.replace("{o" + String(i) + "}", config->getNtpOffset() == i ? "selected"  : "");
	}

	for(int i = 0; i<(3600*3); i+=3600) {
		html.replace("{so" + String(i) + "}", config->getNtpSummerOffset() == i ? "selected"  : "");
	}

	html.replace("{ns}", config->getNtpServer());
	html.replace("{nd}", config->isNtpDhcp() ? "checked" : "");

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

	html.replace("${options.han}", getSerialSelectOptions(config->getHanPin()));

	html.replace("${config.ledPin}", config->getLedPin() == 0xFF ? "" : String(config->getLedPin()));
	html.replace("${config.ledInverted}", config->isLedInverted() ? "checked" : "");
	html.replace("${config.ledPinRed}", config->getLedPinRed() == 0xFF ? "" : String(config->getLedPinRed()));
	html.replace("${config.ledPinGreen}", config->getLedPinGreen() == 0xFF ? "" : String(config->getLedPinGreen()));
	html.replace("${config.ledPinBlue}", config->getLedPinBlue() == 0xFF ? "" : String(config->getLedPinBlue()));
	html.replace("${config.ledRgbInverted}", config->isLedRgbInverted() ? "checked" : "");
	html.replace("${config.apPin}", config->getApPin() == 0xFF ? "" : String(config->getApPin()));
	html.replace("${config.tempSensorPin}", config->getTempSensorPin() == 0xFF ? "" : String(config->getTempSensorPin()));
	html.replace("${config.vccPin}", config->getVccPin() == 0xFF ? "" : String(config->getVccPin()));

	html.replace("${config.vccOffset}", config->getVccOffset() > 0 ? String(config->getVccOffset(), 2) : "");
	html.replace("${config.vccMultiplier}", config->getVccMultiplier() > 0 ? String(config->getVccMultiplier(), 2) : "");
	html.replace("${config.vccBootLimit}", config->getVccBootLimit() > 0.0 ? String(config->getVccBootLimit(), 1) : "");

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

	html.replace("${config.debugTelnet}", config->isDebugTelnet() ? "checked" : "");
	html.replace("${config.debugSerial}", config->isDebugSerial() ? "checked" : "");
	html.replace("${config.debugLevel}", String(config->getDebugLevel()));
	for(int i = 0; i<=RemoteDebug::ANY; i++) {
		html.replace("${config.debugLevel" + String(i) + "}", config->getDebugLevel() == i ? "selected"  : "");
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
		gpioOptions += "<option value=\"3\" selected>UART0</option>";
	} else {
		gpioOptions += "<option value=\"3\">UART0</option>";
	}
	#if defined(ESP32)
		int numGpio = 24;
		int gpios[] = {4,5,6,7,8,10,11,12,13,14,15,17,18,19,21,22,23,25,32,33,34,35,36,39};
		if(selected == 9) {
			gpioOptions += "<option value=\"9\" selected>UART1</option>";
		} else {
			gpioOptions += "<option value=\"9\">UART1</option>";
		}
		if(selected == 16) {
			gpioOptions += "<option value=\"16\" selected>UART2</option>";
		} else {
			gpioOptions += "<option value=\"16\">UART2</option>";
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
        String filename = upload.filename;
		if(uploading) {
			printE("Upload already in progress");
			String html = "<html><body><h1>Upload already in progress!</h1></form>";
			server.send(500, "text/html", html);
		} else if (!SPIFFS.begin()) {
			printE("An Error has occurred while mounting SPIFFS");
			String html = "<html><body><h1>Unable to mount SPIFFS!</h1></form>";
			server.send(500, "text/html", html);
		} else {
			uploading = true;
		    printD("handleFileUpload Name: %s", filename.c_str());
		    file = SPIFFS.open(path, "w");
	  	    filename = String();
	    } 
    } else if(upload.status == UPLOAD_FILE_WRITE) {
        if(file)
            file.write(upload.buf, upload.currentSize);
    } else if(upload.status == UPLOAD_FILE_END) {
        if(file) {
            file.close();
			SPIFFS.end();
            printD("handleFileUpload Size: %d", upload.totalSize);
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

	uploadHtml("CA file", "/firmware", "mqtt");
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
		client.setInsecure();
#endif
		String url = "https://github.com/gskjold/AmsToMqttBridge/releases/download/" + version + "/ams2mqtt-esp12e-" + versionStripped + ".bin";
		HTTPClient https;
#if defined(ESP8266)
		https.setFollowRedirects(true);
#endif

		if(https.begin(client, url)) {
			https.addHeader("Referer", "https://github.com/gskjold/AmsToMqttBridge/releases");
			printD("HTTP client setup successful");
			int status = https.GET();
			if(status == HTTP_CODE_OK) {
				printD("Received OK from server");
				if(SPIFFS.begin()) {
					printI("Downloading firmware to SPIFFS");
					file = SPIFFS.open(FILE_FIRMWARE, "w");
					// The following does not work... Maybe someone will make it work in the future? It seems to be disconnected at this point.
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
			char buf[256];
			client.getLastSSLError(buf,256);
			printE(buf);
#endif
			server.sendHeader("Location","/");
			server.send(303);
		}
		https.end();
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

	if(WiFi.getMode() != WIFI_AP) {
		html.replace("boot.css", BOOTSTRAP_URL);
	}
	if(strlen(config->getWifiIp()) == 0 && WiFi.getMode() != WIFI_AP) {
		html.replace("${ip}", WiFi.localIP().toString());
	} else {
		html.replace("${ip}", config->getWifiIp());
	}
	html.replace("${hostname}", config->getWifiHostname());

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	server.setContentLength(html.length());
	server.send(200, "text/html", html);

	yield();
	if(performRestart) {
		SPIFFS.end();
		printI("Firmware uploaded, rebooting");
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
		if(config->isMqttSsl()) {
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
		if(config->isMqttSsl()) {
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
		if(config->isMqttSsl()) {
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
		if(config->isMqttSsl()) {
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
		if(config->isMqttSsl()) {
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
		if(config->isMqttSsl()) {
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
