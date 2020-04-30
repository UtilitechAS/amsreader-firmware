#include "AmsWebServer.h"
#include "version.h"

#include "root/index_html.h"
#include "root/configmeter_html.h"
#include "root/configwifi_html.h"
#include "root/configmqtt_html.h"
#include "root/configweb_html.h"
#include "root/configdomoticz_html.h"
#include "root/configsystem_html.h"
#include "root/restartwait_html.h"
#include "root/boot_css.h"
#include "root/gaugemeter_js.h"

#include "Base64.h"

AmsWebServer::AmsWebServer(RemoteDebug* Debug) {
	this->debugger = Debug;
}

void AmsWebServer::setup(AmsConfiguration* config, MQTTClient* mqtt) {
    this->config = config;
	this->mqtt = mqtt;

	server.on("/", std::bind(&AmsWebServer::indexHtml, this));
	server.on("/config-meter", HTTP_GET, std::bind(&AmsWebServer::configMeterHtml, this));
	server.on("/config-wifi", HTTP_GET, std::bind(&AmsWebServer::configWifiHtml, this));
	server.on("/config-mqtt", HTTP_GET, std::bind(&AmsWebServer::configMqttHtml, this));
	server.on("/config-web", HTTP_GET, std::bind(&AmsWebServer::configWebHtml, this));
	server.on("/config-domoticz",HTTP_GET, std::bind(&AmsWebServer::configDomoticzHtml, this));
	server.on("/boot.css", HTTP_GET, std::bind(&AmsWebServer::bootCss, this));
	server.on("/gaugemeter.js", HTTP_GET, std::bind(&AmsWebServer::gaugemeterJs, this)); 
	server.on("/data.json", HTTP_GET, std::bind(&AmsWebServer::dataJson, this));

	server.on("/save", std::bind(&AmsWebServer::handleSave, this));

	server.on("/config-system", HTTP_GET, std::bind(&AmsWebServer::configSystemHtml, this));
	server.on("/config-system", HTTP_POST, std::bind(&AmsWebServer::configSystemPost, this), std::bind(&AmsWebServer::configSystemUpload, this));
	server.on("/restart-wait", HTTP_GET, std::bind(&AmsWebServer::restartWaitHtml, this));
	server.on("/is-alive", HTTP_GET, std::bind(&AmsWebServer::isAliveCheck, this));

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
	return access;
}

void AmsWebServer::indexHtml() {
	printD("Serving /index.html over http...");

	if(!checkSecurity(2))
		return;

	String html = String((const __FlashStringHelper*) INDEX_HTML);
	html.replace("${version}", VERSION);

	if(WiFi.getMode() != WIFI_AP) {
		html.replace("boot.css", BOOTSTRAP_URL);
	}

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

	double vcc = hw.getVcc();
	html.replace("${vcc}", vcc > 0 ? String(vcc, 2) : "");

	double temp = hw.getTemperature();
	html.replace("${temp}", temp > 0 ? String(temp, 1) : "");
	html.replace("${display.temp}", temp != DEVICE_DISCONNECTED_C ? "" : "none");

	int rssi = hw.getWifiRssi();
	html.replace("${wifi.rssi}", vcc > 0 ? String(rssi) : "");
	html.replace("${wifi.channel}", WiFi.channel() > 0 ? String(WiFi.channel()) : "");
	html.replace("${wifi.ssid}", !WiFi.SSID().isEmpty() ? String(WiFi.SSID()) : "");

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");

	server.setContentLength(html.length());
	server.send(200, "text/html", html);
}

void AmsWebServer::configMeterHtml() {
	printD("Serving /config-meter.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) CONFIGMETER_HTML);
	html.replace("${version}", VERSION);

	if(WiFi.getMode() != WIFI_AP) {
		html.replace("boot.css", BOOTSTRAP_URL);
	}

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

	server.setContentLength(html.length());
	server.send(200, "text/html", html);
}

void AmsWebServer::configWifiHtml() {
	printD("Serving /config-wifi.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) CONFIGWIFI_HTML);
	html.replace("${version}", VERSION);

	if(WiFi.getMode() != WIFI_AP) {
		html.replace("boot.css", BOOTSTRAP_URL);
	}

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	html.replace("${config.wifiSsid}", config->getWifiSsid());
	html.replace("${config.wifiPassword}", config->getWifiPassword());
	html.replace("${config.wifiIpType1}", config->getWifiIp().isEmpty() ? "" : "selected");
	html.replace("${config.wifiIp}", config->getWifiIp());
	html.replace("${config.wifiGw}", config->getWifiGw());
	html.replace("${config.wifiSubnet}", config->getWifiSubnet());
	html.replace("${config.wifiDns1}", config->getWifiDns1());
	html.replace("${config.wifiDns2}", config->getWifiDns2());
	html.replace("${config.wifiHostname}", config->getWifiHostname());

	server.setContentLength(html.length());
	server.send(200, "text/html", html);
}

void AmsWebServer::configMqttHtml() {
	printD("Serving /config-mqtt.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) CONFIGMQTT_HTML);
	html.replace("${version}", VERSION);

	if(WiFi.getMode() != WIFI_AP) {
		html.replace("boot.css", BOOTSTRAP_URL);
	}

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	html.replace("${config.mqtt}", config->getMqttHost() == 0 ? "" : "checked");
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
	for(int i = 0; i<3; i++) {
		html.replace("${config.mqttPayloadFormat" + String(i) + "}", config->getMqttPayloadFormat() == i ? "selected"  : "");
	}

	server.setContentLength(html.length());
	server.send(200, "text/html", html);
}

void AmsWebServer::configDomoticzHtml() {
	printD("Serving /config/domoticz.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) CONFIGDOMOTICZ_HTML);
	html.replace("${version}", VERSION);

	if(WiFi.getMode() != WIFI_AP) {
		html.replace("boot.css", BOOTSTRAP_URL);
	}

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	html.replace("${config.domo}", config->getDomoELIDX() == 0 ? "" : "checked");
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
	if(config->getDomoEnergy() > 0.0){ html.replace("${config.domoEnergy}", String(config->getDomoEnergy()));
	} else { html.replace("${config.domoEnergy}", ""); }
	server.setContentLength(html.length());
	server.send(200, "text/html", html);
}


void AmsWebServer::configWebHtml() {
	printD("Serving /config-web.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) CONFIGWEB_HTML);
	html.replace("${version}", VERSION);

	if(WiFi.getMode() != WIFI_AP) {
		html.replace("boot.css", BOOTSTRAP_URL);
	}

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	html.replace("${config.authSecurity}", String(config->getAuthSecurity()));
	for(int i = 0; i<3; i++) {
		html.replace("${config.authSecurity" + String(i) + "}", config->getAuthSecurity() == i ? "selected"  : "");
	}
	html.replace("${config.authUser}", config->getAuthUser());
	html.replace("${config.authPassword}", config->getAuthPassword());

	server.setContentLength(html.length());
	server.send(200, "text/html", html);
}

void AmsWebServer::bootCss() {
	printD("Serving /boot.css over http...");

	String css = String((const __FlashStringHelper*) BOOT_CSS);

	server.sendHeader("Cache-Control", "public, max-age=3600");

	server.setContentLength(css.length());
	server.send(200, "text/css", css);
}

void AmsWebServer::gaugemeterJs() {
	printD("Serving /gaugemeter.js over http...");

	String js = String((const __FlashStringHelper*) GAUGEMETER_JS);

	server.sendHeader("Cache-Control", "public, max-age=3600");
	
	server.setContentLength(js.length());
	server.send(200, "application/javascript", js);
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
	double vcc = hw.getVcc();
	json["vcc"] = serialized(String(vcc, 3));

	double temp = hw.getTemperature();
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
	if(config->getWifiSsid().isEmpty()) {
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
	if(config->getMqttHost().isEmpty()) {
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

	String domoStatus;
	if(String(config->getDomoELIDX()).isEmpty()) {
		domoStatus = "secondary";
	} else if(mqtt->connected() && config->getMqttPayloadFormat() == 2 && config->getDomoELIDX() > 0) {
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

void AmsWebServer::handleSave() {
	String temp;

	if(server.hasArg("meterConfig") && server.arg("meterConfig") == "true") {
		config->setMeterType(server.arg("meterType").toInt());
		config->setDistributionSystem(server.arg("distributionSystem").toInt());
		config->setMainFuse(server.arg("mainFuse").toInt());
		config->setProductionCapacity(server.arg("productionCapacity").toInt());
	}

	if(server.hasArg("wifiConfig") && server.arg("wifiConfig") == "true") {
		config->setWifiSsid(server.arg("wifiSsid"));
		config->setWifiPassword(server.arg("wifiPassword"));
		if(server.hasArg("wifiIpType") && server.arg("wifiIpType").toInt() == 1) {
			config->setWifiIp(server.arg("wifiIp"));
			config->setWifiGw(server.arg("wifiGw"));
			config->setWifiSubnet(server.arg("wifiSubnet"));
			config->setWifiDns1(server.arg("wifiDns1"));
			config->setWifiDns2(server.arg("wifiDns2"));
		} else {
			config->clearWifiIp();
		}
		config->setWifiHostname(server.arg("wifiHostname"));
	}

	if(server.hasArg("mqttConfig") && server.arg("mqttConfig") == "true") {
		if(server.hasArg("mqtt") && server.arg("mqtt") == "true") {
			config->setMqttHost(server.arg("mqttHost"));
			int port = server.arg("mqttPort").toInt();
			config->setMqttPort(port == 0 ? 1883 : port);
			config->setMqttClientId(server.arg("mqttClientId"));
			config->setMqttPublishTopic(server.arg("mqttPublishTopic"));
			config->setMqttSubscribeTopic(server.arg("mqttSubscribeTopic"));
			config->setMqttUser(server.arg("mqttUser"));
			config->setMqttPassword(server.arg("mqttPassword"));
			config->setMqttPayloadFormat(server.arg("mqttPayloadFormat").toInt());
		} else {
			config->clearMqtt();
		}
	}

	if(server.hasArg("domoConfig") && server.arg("domoConfig") == "true") {
		if(server.hasArg("domo") && server.arg("domo") == "true") {
			config->setDomoELIDX(server.arg("domoELIDX").toInt());
			config->setDomoVL1IDX(server.arg("domoVL1IDX").toInt());
			config->setDomoVL2IDX(server.arg("domoVL2IDX").toInt());
			config->setDomoVL3IDX(server.arg("domoVL3IDX").toInt());
			config->setDomoCL1IDX(server.arg("domoCL1IDX").toInt());
			config->setDomoEnergy(server.arg("domoEnergy").toDouble());
		} else {
			config->clearDomo();
		}
	}



	if(server.hasArg("authConfig") && server.arg("authConfig") == "true") {
		config->setAuthSecurity((byte)server.arg("authSecurity").toInt());
		if(config->getAuthSecurity() > 0) {
			config->setAuthUser(server.arg("authUser"));
			config->setAuthPassword(server.arg("authPassword"));
			debugger->setPassword(config->getAuthPassword());
		} else {
			debugger->setPassword("");
			config->clearAuth();
		}
	}

	if(server.hasArg("sysConfig") && server.arg("sysConfig") == "true") {
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
	html.replace("${version}", VERSION);

	if(WiFi.getMode() != WIFI_AP) {
		html.replace("boot.css", BOOTSTRAP_URL);
	}

	html.replace("${config.debugTelnet}", config->isDebugTelnet() ? "checked" : "");
	html.replace("${config.debugSerial}", config->isDebugSerial() ? "checked" : "");
	html.replace("${config.debugLevel}", String(config->getDebugLevel()));
	for(int i = 0; i<=RemoteDebug::ANY; i++) {
		html.replace("${config.debugLevel" + String(i) + "}", config->getDebugLevel() == i ? "selected"  : "");
	}

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	server.setContentLength(html.length());
	server.send(200, "text/html", html);
}

void AmsWebServer::configSystemPost() {
	server.send(200);
}

void AmsWebServer::configSystemUpload() {
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_START){
        String filename = upload.filename;
        if(!filename.endsWith(".bin")) {
            server.send(500, "text/plain", "500: couldn't create file");
		} else if (!SPIFFS.begin()) {
			printE("An Error has occurred while mounting SPIFFS");
			String html = "<html><body><h1>Error uploading!</h1></form>";
			server.send(500, "text/html", html);
		} else {
		    printD("handleFileUpload Name: %s", filename.c_str());
		    firmwareFile = SPIFFS.open("/firmware.bin", "w");
	  	    filename = String();
	    } 
    } else if(upload.status == UPLOAD_FILE_WRITE) {
        if(firmwareFile)
            firmwareFile.write(upload.buf, upload.currentSize);
    } else if(upload.status == UPLOAD_FILE_END) {
        if(firmwareFile) {
            firmwareFile.close();
			SPIFFS.end();
            printD("handleFileUpload Size: %d", upload.totalSize);
			performRestart = true;
            server.sendHeader("Location","/restart-wait");
            server.send(303);
        } else {
            server.send(500, "text/plain", "500: couldn't create file");
        }
    }
}

void AmsWebServer::restartWaitHtml() {
	printD("Serving /restart-wait.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) RESTARTWAIT_HTML);
	html.replace("${version}", VERSION);

	if(WiFi.getMode() != WIFI_AP) {
		html.replace("boot.css", BOOTSTRAP_URL);
	}
	if(config->getWifiIp().isEmpty() && WiFi.getMode() != WIFI_AP) {
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
