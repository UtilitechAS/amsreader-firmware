#include "AmsWebServer.h"
#include "version.h"
#include "AmsStorage.h"

#include "root/head_html.h"
#include "root/foot_html.h"
#include "root/index_html.h"
#include "root/application_js.h"
#include "root/setup_html.h"
#include "root/configmeter_html.h"
#include "root/configwifi_html.h"
#include "root/configmqtt_html.h"
#include "root/configweb_html.h"
#include "root/configdomoticz_html.h"
#include "root/configsystem_html.h"
#include "root/restartwait_html.h"
#include "root/boot_css.h"
#include "root/gaugemeter_js.h"
#include "root/github_svg.h"
#include "root/upload_html.h"
#include "root/delete_html.h"
#include "root/reset_html.h"

#include "Base64.h"

AmsWebServer::AmsWebServer(RemoteDebug* Debug, HwTools* hw) {
	this->debugger = Debug;
	this->hw = hw;
}

void AmsWebServer::setup(AmsConfiguration* config, MQTTClient* mqtt) {
    this->config = config;
	this->mqtt = mqtt;

	server.on("/", HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	server.on("/", HTTP_POST, std::bind(&AmsWebServer::handleSetup, this));
	server.on("/application.js", HTTP_GET, std::bind(&AmsWebServer::applicationJs, this));
	server.on("/config-meter", HTTP_GET, std::bind(&AmsWebServer::configMeterHtml, this));
	server.on("/config-wifi", HTTP_GET, std::bind(&AmsWebServer::configWifiHtml, this));
	server.on("/config-mqtt", HTTP_GET, std::bind(&AmsWebServer::configMqttHtml, this));
	server.on("/config-web", HTTP_GET, std::bind(&AmsWebServer::configWebHtml, this));
	server.on("/config-domoticz",HTTP_GET, std::bind(&AmsWebServer::configDomoticzHtml, this));
	server.on("/boot.css", HTTP_GET, std::bind(&AmsWebServer::bootCss, this));
	server.on("/gaugemeter.js", HTTP_GET, std::bind(&AmsWebServer::gaugemeterJs, this)); 
	server.on("/github.svg", HTTP_GET, std::bind(&AmsWebServer::githubSvg, this)); 
	server.on("/data.json", HTTP_GET, std::bind(&AmsWebServer::dataJson, this));

	server.on("/save", HTTP_POST, std::bind(&AmsWebServer::handleSave, this));

	server.on("/config-system", HTTP_GET, std::bind(&AmsWebServer::configSystemHtml, this));
	server.on("/firmware", HTTP_GET, std::bind(&AmsWebServer::firmwareHtml, this));
	server.on("/firmware", HTTP_POST, std::bind(&AmsWebServer::uploadPost, this), std::bind(&AmsWebServer::firmwareUpload, this));
	server.on("/upgrade", HTTP_GET, std::bind(&AmsWebServer::firmwareDownload, this));
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
		char inputString[providedPwd.length()];
		providedPwd.toCharArray(inputString, providedPwd.length()+1);

		int inputStringLength = sizeof(inputString);
		int decodedLength = Base64.decodedLength(inputString, inputStringLength);
		char decodedString[decodedLength];
		Base64.decode(decodedString, inputString, inputStringLength);
		printD("Received auth: %s", decodedString);
		access = String(decodedString).equals(expectedAuth);
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
		for(int i = 0; i<4; i++) {
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

		html.replace("${data.P}", String(data.getActiveImportPower()));
		html.replace("${data.PO}", String(data.getActiveExportPower()));
		html.replace("${display.export}", config->getProductionCapacity() > 0 ? "" : "none");
		html.replace("${text.import}", config->getProductionCapacity() > 0 ? "Import" : "Consumption");

		html.replace("${data.U1}", u1 > 0 ? String(u1, 1) : "");
		html.replace("${data.I1}", u1 > 0 ? String(i1, 1) : "");
		html.replace("${display.P1}", u1 > 0 ? "" : "none");

		html.replace("${data.U2}", u2 > 0 ? String(u2, 1) : "");
		html.replace("${data.I2}", u2 > 0 ? String(i2, 1) : "");
		html.replace("${display.P2}", u2 > 0 ? "" : "none");

		html.replace("${data.U3}", u3 > 0 ? String(u3, 1) : "");
		html.replace("${data.I3}", u3 > 0 ? String(i3, 1) : "");
		html.replace("${display.P3}", u3 > 0 ? "" : "none");

		html.replace("${data.tPI}", tpi > 0 ? String(tpi, 1) : "");
		html.replace("${data.tPO}", tpi > 0 ? String(tpo, 1) : "");
		html.replace("${data.tQI}", tpi > 0 ? String(tqi, 1) : "");
		html.replace("${data.tQO}", tpi > 0 ? String(tqo, 1) : "");
		html.replace("${display.accumulative}", tpi > 0 ? "" : "none");

		double vcc = hw->getVcc();
		html.replace("${vcc}", vcc > 0 ? String(vcc, 2) : "");

		double temp = hw->getTemperature();
		html.replace("${temp}", temp > 0 ? String(temp, 1) : "");
		html.replace("${display.temp}", temp != DEVICE_DISCONNECTED_C ? "" : "none");

		int rssi = hw->getWifiRssi();
		html.replace("${wifi.rssi}", vcc > 0 ? String(rssi) : "");
		html.replace("${wifi.channel}", WiFi.channel() > 0 ? String(WiFi.channel()) : "");
		html.replace("${wifi.ssid}", !WiFi.SSID().isEmpty() ? String(WiFi.SSID()) : "");

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
	printD("Serving /config-meter.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) CONFIGMETER_HTML);

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	html.replace("${config.meterType}", String(config->getMainFuse()));
	for(int i = 0; i<4; i++) {
		html.replace("${config.meterType" + String(i) + "}", config->getMeterType() == i ? "selected"  : "");
	}
	html.replace("${config.distributionSystem}", String(config->getDistributionSystem()));
	for(int i = 0; i<3; i++) {
		html.replace("${config.distributionSystem" + String(i) + "}", config->getDistributionSystem() == i ? "selected"  : "");
	}
	html.replace("${config.mainFuse}", String(config->getMainFuse()));
	for(int i = 0; i<64; i++) {
		html.replace("${config.mainFuse" + String(i) + "}", config->getMainFuse() == i ? "selected"  : "");
	}
	html.replace("${config.productionCapacity}", String(config->getProductionCapacity()));
	html.replace("${config.substituteMissing}", config->isSubstituteMissing() ? "checked" : "");
	html.replace("${config.sendUnknown}", config->isSendUnknown() ? "checked" : "");

	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, "text/html", HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::configWifiHtml() {
	printD("Serving /config-wifi.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) CONFIGWIFI_HTML);

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	html.replace("${config.wifiSsid}", config->getWifiSsid());
	html.replace("${config.wifiPassword}", config->getWifiPassword());
	html.replace("${config.wifiStaticIp}", strlen(config->getWifiIp()) > 0 ? "checked" : "");
	html.replace("${config.wifiIp}", config->getWifiIp());
	html.replace("${config.wifiGw}", config->getWifiGw());
	html.replace("${config.wifiSubnet}", config->getWifiSubnet());
	html.replace("${config.wifiDns1}", config->getWifiDns1());
	html.replace("${config.wifiDns2}", config->getWifiDns2());
	html.replace("${config.wifiHostname}", config->getWifiHostname());

	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, "text/html", HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::configMqttHtml() {
	printD("Serving /config-mqtt.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) CONFIGMQTT_HTML);

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	html.replace("${config.mqtt}", strlen(config->getMqttHost()) == 0 ? "" : "checked");
	html.replace("${config.mqttHost}", config->getMqttHost());
	if(config->getMqttPort() > 0) {
		html.replace("${config.mqttPort}", String(config->getMqttPort()));
	} else {
		html.replace("${config.mqttPort}", String(1883));
	}
	html.replace("${config.mqttClientId}", config->getMqttClientId());
	html.replace("${config.mqttPublishTopic}", config->getMqttPublishTopic());
	html.replace("${config.mqttSubscribeTopic}", config->getMqttSubscribeTopic());
	html.replace("${config.mqttUser}", config->getMqttUser());
	html.replace("${config.mqttPassword}", config->getMqttPassword());
	html.replace("${config.mqttPayloadFormat}", String(config->getMqttPayloadFormat()));
	for(int i = 0; i<4; i++) {
		html.replace("${config.mqttPayloadFormat" + String(i) + "}", config->getMqttPayloadFormat() == i ? "selected"  : "");
	}

	html.replace("${config.mqttSsl}", config->isMqttSsl() ? "checked" : "");
	html.replace("${display.ssl}", config->isMqttSsl() ? "" : "none");

	if(SPIFFS.begin()) {
		html.replace("${display.ca.upload}", SPIFFS.exists(FILE_MQTT_CA) ? "none" : "");
		html.replace("${display.ca.file}", SPIFFS.exists(FILE_MQTT_CA) ? "" : "none");
		html.replace("${display.cert.upload}", SPIFFS.exists(FILE_MQTT_CERT) ? "none" : "");
		html.replace("${display.cert.file}", SPIFFS.exists(FILE_MQTT_CERT) ? "" : "none");
		html.replace("${display.key.upload}", SPIFFS.exists(FILE_MQTT_KEY) ? "none" : "");
		html.replace("${display.key.file}", SPIFFS.exists(FILE_MQTT_KEY) ? "" : "none");
		SPIFFS.end();
	} else {
		html.replace("${display.ca.upload}", "");
		html.replace("${display.ca.file}", "none");
		html.replace("${display.cert.upload}", "");
		html.replace("${display.cert.file}", "none");
		html.replace("${display.key.upload}", "");
		html.replace("${display.key.file}", "none");
	}

	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, "text/html", HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::configDomoticzHtml() {
	printD("Serving /config/domoticz.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) CONFIGDOMOTICZ_HTML);

	if(WiFi.getMode() != WIFI_AP) {
		html.replace("boot.css", BOOTSTRAP_URL);
	}

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	if(config->getDomoELIDX() > 0){ html.replace("${config.domoELIDX}", String(config->getDomoELIDX()));
	} else { html.replace("${config.domoELIDX}", ""); }
	if(config->getDomoVL1IDX() > 0){ html.replace("${config.domoVL1IDX}", String(config->getDomoVL1IDX()));
	} else { html.replace("${config.domoVL1IDX}", ""); }
	if(config->getDomoVL2IDX() > 0){ html.replace("${config.domoVL2IDX}", String(config->getDomoVL2IDX()));
	} else { html.replace("${config.domoVL2IDX}", ""); }
	if(config->getDomoVL3IDX() > 0){ html.replace("${config.domoVL3IDX}", String(config->getDomoVL3IDX()));
	} else { html.replace("${config.domoVL3IDX}", ""); }
	if(config->getDomoCL1IDX() > 0){ html.replace("${config.domoCL1IDX}", String(config->getDomoCL1IDX()));
	} else { html.replace("${config.domoCL1IDX}", ""); }

	server.setContentLength(html.length() + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, "text/html", HEAD_HTML);
	server.sendContent(html);
	server.sendContent_P(FOOT_HTML);
}


void AmsWebServer::configWebHtml() {
	printD("Serving /config-web.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) CONFIGWEB_HTML);

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	html.replace("${config.authSecurity}", String(config->getAuthSecurity()));
	for(int i = 0; i<3; i++) {
		html.replace("${config.authSecurity" + String(i) + "}", config->getAuthSecurity() == i ? "selected"  : "");
	}
	html.replace("${config.authUser}", config->getAuthUser());
	html.replace("${config.authPassword}", config->getAuthPassword());

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

		if(u1 > 0) {
			json["data"]["U1"] = u1;
			json["data"]["I1"] = i1;
		}
		if(u2 > 0) {
			json["data"]["U2"] = u2;
			json["data"]["I2"] = i2;
		}
		if(u3 > 0) {
			json["data"]["U3"] = u3;
			json["data"]["I3"] = i3;
		}

		if(tpi > 0) {
			json["data"]["tPI"] = tpi;
			json["data"]["tPO"] = tpo;
			json["data"]["tQI"] = tqi;
			json["data"]["tQO"] = tqo;
		}

		json["p_pct"] = min(data.getActiveImportPower()*100/maxPwr, 100);

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

	// not in use anymore and  assumes that if configured ELIDX is alsway given.....   Remove??
	String domoStatus;
	if(String(config->getDomoELIDX()).isEmpty()) {
		domoStatus = "secondary";
	} else if(mqtt->connected() && config->getMqttPayloadFormat() == 3 && config->getDomoELIDX() > 0) {
		domoStatus = "success";
	} else if(mqtt->lastError() == 0) {
		domoStatus = "warning";
	} else {
		domoStatus = "danger";
	}
	json["status"]["domo"] = domoStatus;

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
		config->setBoardType(server.arg("board").toInt());
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
			case 199: // ESP8266
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
			case 299: // ESP32
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
	String temp;

	if(server.hasArg("meterConfig") && server.arg("meterConfig") == "true") {
		config->setMeterType(server.arg("meterType").toInt());
		config->setDistributionSystem(server.arg("distributionSystem").toInt());
		config->setMainFuse(server.arg("mainFuse").toInt());
		config->setProductionCapacity(server.arg("productionCapacity").toInt());
		config->setSubstituteMissing(server.hasArg("substituteMissing") && server.arg("substituteMissing") == "true");
		config->setSendUnknown(server.hasArg("sendUnknown") && server.arg("sendUnknown") == "true");
	}

	if(server.hasArg("wifiConfig") && server.arg("wifiConfig") == "true") {
		config->setWifiSsid(server.arg("wifiSsid").c_str());
		config->setWifiPassword(server.arg("wifiPassword").c_str());
		if(server.hasArg("wifiIpType") && server.arg("wifiIpType").toInt() == 1) {
			config->setWifiIp(server.arg("wifiIp").c_str());
			config->setWifiGw(server.arg("wifiGw").c_str());
			config->setWifiSubnet(server.arg("wifiSubnet").c_str());
			config->setWifiDns1(server.arg("wifiDns1").c_str());
			config->setWifiDns2(server.arg("wifiDns2").c_str());
		} else {
			config->clearWifiIp();
		}
		config->setWifiHostname(server.arg("wifiHostname").c_str());
	}

	if(server.hasArg("mqttConfig") && server.arg("mqttConfig") == "true") {
		if(server.hasArg("mqtt") && server.arg("mqtt") == "true") {
			config->setMqttHost(server.arg("mqttHost").c_str());
			int port = server.arg("mqttPort").toInt();
			config->setMqttPort(port == 0 ? 1883 : port);
			config->setMqttClientId(server.arg("mqttClientId").c_str());
			config->setMqttPublishTopic(server.arg("mqttPublishTopic").c_str());
			config->setMqttSubscribeTopic(server.arg("mqttSubscribeTopic").c_str());
			config->setMqttUser(server.arg("mqttUser").c_str());
			config->setMqttPassword(server.arg("mqttPassword").c_str());
			config->setMqttPayloadFormat(server.arg("mqttPayloadFormat").toInt());
			config->setMqttSsl(server.arg("mqttSsl") == "true");
		} else {
			config->clearMqtt();
		}
	}

	if(server.hasArg("domoConfig") && server.arg("domoConfig") == "true") {
		config->setDomoELIDX(server.arg("domoELIDX").toInt());
		config->setDomoVL1IDX(server.arg("domoVL1IDX").toInt());
		config->setDomoVL2IDX(server.arg("domoVL2IDX").toInt());
		config->setDomoVL3IDX(server.arg("domoVL3IDX").toInt());
		config->setDomoCL1IDX(server.arg("domoCL1IDX").toInt());
	}



	if(server.hasArg("authConfig") && server.arg("authConfig") == "true") {
		config->setAuthSecurity((byte)server.arg("authSecurity").toInt());
		if(config->getAuthSecurity() > 0) {
			config->setAuthUser(server.arg("authUser").c_str());
			config->setAuthPassword(server.arg("authPassword").c_str());
			debugger->setPassword(config->getAuthPassword());
		} else {
			debugger->setPassword("");
			config->clearAuth();
		}
	}

	if(server.hasArg("sysConfig") && server.arg("sysConfig") == "true") {
		// Unset all pins to avoid conflicts if GPIO have been swapped between pins
		config->setLedPin(0xFF);
		config->setLedPinRed(0xFF);
		config->setLedPinGreen(0xFF);
		config->setLedPinBlue(0xFF);
		config->setApPin(0xFF);
		config->setTempSensorPin(0xFF);
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
		config->setVccPin(server.hasArg("vccPin") && !server.arg("vccPin").isEmpty() ? server.arg("vccPin").toInt() : 0xFF);
		config->setVccMultiplier(server.hasArg("vccMultiplier") && !server.arg("vccMultiplier").isEmpty() ? server.arg("vccMultiplier").toDouble() : 1.0);
		config->setVccBootLimit(server.hasArg("vccBootLimit") && !server.arg("vccBootLimit").isEmpty() ? server.arg("vccBootLimit").toDouble() : 0.0);

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

		hw->setLed(config->getLedPin(), config->isLedInverted());
		hw->setLedRgb(config->getLedPinRed(), config->getLedPinGreen(), config->getLedPinBlue(), config->isLedRgbInverted());
		hw->setTempSensorPin(config->getTempSensorPin());
		hw->setVccPin(config->getVccPin());
		hw->setVccMultiplier(config->getVccMultiplier());
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
			hw->setVccPin(config->getVccPin());
			hw->setVccMultiplier(config->getVccMultiplier());
		}
	} else {
		printE("Error saving configuration");
		String html = "<html><body><h1>Error saving configuration!</h1></form>";
		server.send(500, "text/html", html);
	}
}

void AmsWebServer::configSystemHtml() {
	printD("Serving /config-system.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) CONFIGSYSTEM_HTML);

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

	html.replace("${config.vccMultiplier}", config->getVccMultiplier() > 0 ? String(config->getVccMultiplier(), 2) : "");
	html.replace("${config.vccBootLimit}", config->getVccBootLimit() > 0.0 ? String(config->getVccBootLimit(), 1) : "");

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

	uploadHtml("CA file", "/firmware", "mqtt");
}

void AmsWebServer::firmwareUpload() {
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
	printD("Firmware download URL triggered");
	if(server.hasArg("version")) {
		String version = server.arg("version");
		String versionStripped = version.substring(1);
		printI("Downloading firmware...");
		WiFiClientSecure client;
		//client.setFingerprint(githubFingerprint);
#if defined(ESP8266)
		client.setInsecure();
		client.setBufferSizes(512, 512);
#endif
		HTTPClient https;
		String url = "https://github.com/gskjold/AmsToMqttBridge/releases/download/" + version + "/ams2mqtt-d1mini-" + versionStripped + ".bin";
/* The following does not work... Maybe someone will make it work in the future?
		https.setFollowRedirects(true);

		if(https.begin(client, url)) {
			printD("HTTP client setup successful");
			int status = https.GET();
			if(status == HTTP_CODE_OK) {
				printD("Received OK from server");
				if(SPIFFS.begin()) {
					printI("Downloading firmware to SPIFFS");
					file = SPIFFS.open(FILE_FIRMWARE, "w");
					https.writeToStream(&file);
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
			char buf[256];
			client.getLastSSLError(buf,256);
			printE(buf);
			server.sendHeader("Location","/");
			server.send(303);
		}
		*/
	} else {
		printI("No firmware version specified...");
		server.sendHeader("Location","/");
		server.send(303);
	}
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
	server.sendHeader("Cache-Control", "public, max-age=3600");
	
	server.setContentLength(RESET_HTML_LEN + HEAD_HTML_LEN + FOOT_HTML_LEN);
	server.send_P(200, "text/html", HEAD_HTML);
	server.sendContent_P(RESET_HTML);
	server.sendContent_P(FOOT_HTML);
}

void AmsWebServer::factoryResetPost() {
	if(server.hasArg("perform") && server.arg("perform") == "true") {
		SPIFFS.format();
		config->clear();
		performRestart = true;
		server.sendHeader("Location","/restart-wait");
		server.send(303);
	} else {
		server.sendHeader("Location","/");
		server.send(303);
	}
}


void AmsWebServer::notFound() {
	server.sendHeader("Location","/");
	server.send(303);
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
