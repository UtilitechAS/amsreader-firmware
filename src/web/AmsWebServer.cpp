#include "AmsWebServer.h"
#include "version.h"

#include "root/index_html.h"
#include "root/configmeter_html.h"
#include "root/configwifi_html.h"
#include "root/configmqtt_html.h"
#include "root/configweb_html.h"
#include "root/boot_css.h"
#include "root/gaugemeter_js.h"

#include "Base64.h"

#if defined(ESP8266)
ESP8266WebServer server(80);
#elif defined(ESP32) // ARDUINO_ARCH_ESP32
WebServer server(80);
#endif

void AmsWebServer::setup(AmsConfiguration* config, Stream* debugger, MQTTClient* mqtt) {
    this->config = config;
    this->debugger = debugger;
	this->mqtt = mqtt;

	server.on("/", std::bind(&AmsWebServer::indexHtml, this));
	server.on("/config-meter", std::bind(&AmsWebServer::configMeterHtml, this));
	server.on("/config-wifi", std::bind(&AmsWebServer::configWifiHtml, this));
	server.on("/config-mqtt", std::bind(&AmsWebServer::configMqttHtml, this));
	server.on("/config-web", std::bind(&AmsWebServer::configWebHtml, this));
	server.on("/boot.css", std::bind(&AmsWebServer::bootCss, this));
	server.on("/gaugemeter.js", std::bind(&AmsWebServer::gaugemeterJs, this)); 
	server.on("/data.json", std::bind(&AmsWebServer::dataJson, this));

	server.on("/save", std::bind(&AmsWebServer::handleSave, this));

	server.begin(); // Web server start
}

void AmsWebServer::loop() {
	server.handleClient();
}

void AmsWebServer::setJson(StaticJsonDocument<1024> json) {
	if(!json.isNull()) {
		p = json["data"]["P"].as<int>();
		po = json["data"]["PO"].as<int>();

		if(json["data"].containsKey("U1")) {
			u1 = json["data"]["U1"].as<double>();
			i1 = json["data"]["I1"].as<double>();
	
			if(json["data"].containsKey("U2")) {
				u2 = json["data"]["U2"].as<double>();
				i2 = json["data"]["I2"].as<double>();

				if(json["data"].containsKey("U3")) {
					u3 = json["data"]["U3"].as<double>();
					i3 = json["data"]["I3"].as<double>();
				}

				// Only way to determine if you have more than one phase is to run this code here
				if(maxPwr == 0 && config->hasConfig() && config->getMainFuse() > 0 && config->getDistributionSystem() > 0) {
					int volt = config->getDistributionSystem() == 2 ? 400 : 230;
					if(u2 > 0) {
						maxPwr = config->getMainFuse() * sqrt(3) * volt;
					} else {
						maxPwr = config->getMainFuse() * 230;
					}
				}
			}

			if(json["data"].containsKey("tPI")) {
				tpi = json["data"]["tPI"].as<double>();
				tpo = json["data"]["tPO"].as<double>();
				tqi = json["data"]["tQI"].as<double>();
				tqo = json["data"]["tQO"].as<double>();
			}
		} else {
			if(po > 0) {
				json["data"]["PO"] = po;
			}
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
		}
	    this->json = json;
	}
}

bool AmsWebServer::checkSecurity(byte level) {
	bool access = WiFi.getMode() == WIFI_AP || !config->hasConfig() || config->getAuthSecurity() < level;
	if(!access && config->getAuthSecurity() >= level && server.hasHeader("Authorization")) {
		println(" forcing web security");
		String expectedAuth = String(config->getAuthUser()) + ":" + String(config->getAuthPassword());

		String providedPwd = server.header("Authorization");
		providedPwd.replace("Basic ", "");
		char inputString[providedPwd.length()];
		providedPwd.toCharArray(inputString, providedPwd.length()+1);

		int inputStringLength = sizeof(inputString);
		int decodedLength = Base64.decodedLength(inputString, inputStringLength);
		char decodedString[decodedLength];
		Base64.decode(decodedString, inputString, inputStringLength);
		print("Received auth: ");
		println(decodedString);
		access = String(decodedString).equals(expectedAuth);
	}

	if(!access) {
		println(" no access, requesting user/pass");
		server.sendHeader("WWW-Authenticate", "Basic realm=\"Secure Area\"");
		server.send(401, "text/html", "");
	}
	return access;
}

void AmsWebServer::indexHtml() {
	println("Serving /index.html over http...");

	if(!checkSecurity(2))
		return;

	String html = String((const __FlashStringHelper*) INDEX_HTML);
	html.replace("${version}", VERSION);
	html.replace("${data.P}", String(p));
	html.replace("${data.PO}", String(po));
	html.replace("${display.production}", config->getProductionCapacity() > 0 ? "" : "none");

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

	float rssi = WiFi.RSSI();
	rssi = isnan(rssi) ? -100.0 : rssi;
	html.replace("${wifi.rssi}", vcc > 0 ? String(rssi, 0) : "");
	html.replace("${wifi.channel}", WiFi.channel() > 0 ? String(WiFi.channel()) : "");
	html.replace("${wifi.ssid}", !WiFi.SSID().isEmpty() ? String(WiFi.SSID()) : "");

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	server.send(200, "text/html", html);
}

void AmsWebServer::configMeterHtml() {
	println("Serving /config/meter.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) CONFIGMETER_HTML);
	html.replace("${version}", VERSION);

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
	server.send(200, "text/html", html);
}

void AmsWebServer::configWifiHtml() {
	println("Serving /config/wifi.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) CONFIGWIFI_HTML);
	html.replace("${version}", VERSION);

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	html.replace("${config.wifiSsid}", config->getWifiSsid());
	html.replace("${config.wifiPassword}", config->getWifiPassword());
	html.replace("${config.wifiIpType1}", config->getWifiIp().isEmpty() ? "" : "selected");
	html.replace("${config.wifiIp}", config->getWifiIp());
	html.replace("${config.wifiGw}", config->getWifiGw());
	html.replace("${config.wifiSubnet}", config->getWifiSubnet());

	server.send(200, "text/html", html);
}

void AmsWebServer::configMqttHtml() {
	println("Serving /config/mqtt.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) CONFIGMQTT_HTML);
	html.replace("${version}", VERSION);

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	html.replace("${config.mqtt}", config->getMqttHost() == 0 ? "" : "checked");
	html.replace("${config.mqttHost}", config->getMqttHost());
	html.replace("${config.mqttPort}", String(config->getMqttPort()));
	html.replace("${config.mqttClientId}", config->getMqttClientId());
	html.replace("${config.mqttPublishTopic}", config->getMqttPublishTopic());
	html.replace("${config.mqttSubscribeTopic}", config->getMqttSubscribeTopic());
	html.replace("${config.mqttUser}", config->getMqttUser());
	html.replace("${config.mqttPassword}", config->getMqttPassword());

	server.send(200, "text/html", html);
}

void AmsWebServer::configWebHtml() {
	println("Serving /config/web.html over http...");

	if(!checkSecurity(1))
		return;

	String html = String((const __FlashStringHelper*) CONFIGWEB_HTML);
	html.replace("${version}", VERSION);

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");

	html.replace("${config.authSecurity}", String(config->getAuthSecurity()));
	for(int i = 0; i<3; i++) {
		html.replace("${config.authSecurity" + String(i) + "}", config->getAuthSecurity() == i ? "selected"  : "");
	}
	html.replace("${config.authUser}", config->getAuthUser());
	html.replace("${config.authPassword}", config->getAuthPassword());

	server.send(200, "text/html", html);
}

void AmsWebServer::bootCss() {
	println("Serving /boot.css over http...");

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	server.send(200, "text/css", BOOT_CSS);
}

void AmsWebServer::gaugemeterJs() {
	println("Serving /gaugemeter.js over http...");

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	server.send(200, "application/javascript", GAUGEMETER_JS);
}

void AmsWebServer::dataJson() {
	println("Serving /data.json over http...");

	if(!checkSecurity(2))
		return;

	StaticJsonDocument<768> json;

    String jsonStr;
	if(!this->json.isNull() && this->json.containsKey("data")) {
		int maxPwr = this->maxPwr;
		if(maxPwr == 0) {
			if(u2 > 0) {
				maxPwr = 20000;
			} else {
				maxPwr = 10000;
			}
		}

		json["up"] = this->json["up"];
		json["t"] = this->json["t"];
		json["data"] = this->json["data"];

		json["p_pct"] = min(p*100/maxPwr, 100);

		if(config->getProductionCapacity() > 0) {
			int maxPrd = config->getProductionCapacity() * 1000;
			json["po_pct"] = min(po*100/maxPrd, 100);
		}
	} else {
		json["p_pct"] = -1;
		json["po_pct"] = -1;
	}

	unsigned long now = millis();
	json["id"] = WiFi.macAddress();
	json["maxPower"] = maxPwr;
	json["meterType"] = config->getMeterType();
	json["currentMillis"] = now;
	double vcc = hw.getVcc();
	json["vcc"] = vcc > 0 ? vcc : 0;

	double temp = hw.getTemperature();
	json["temp"] = temp;

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

	unsigned long lastHan = json.isNull() ? 0 : json["up"].as<unsigned long>();
	String hanStatus;
	if(config->getMeterType() == 0) {
		hanStatus = "secondary";
	} else if(now - lastHan < 15000) {
		hanStatus = "success";
	} else if(now - lastHan < 30000) {
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

	serializeJson(json, jsonStr);

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
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
		} else {
			config->clearWifiIp();
		}
	}

	if(server.hasArg("mqttConfig") && server.arg("mqttConfig") == "true") {
		if(server.hasArg("mqtt") && server.arg("mqtt") == "true") {
			config->setMqttHost(server.arg("mqttHost"));
			config->setMqttPort(server.arg("mqttPort").toInt());
			config->setMqttClientId(server.arg("mqttClientId"));
			config->setMqttPublishTopic(server.arg("mqttPublishTopic"));
			config->setMqttSubscribeTopic(server.arg("mqttSubscribeTopic"));
			config->setMqttUser(server.arg("mqttUser"));
			config->setMqttPassword(server.arg("mqttPassword"));
			config->setAuthUser(server.arg("authUser"));
			config->setAuthPassword(server.arg("authPassword"));
		} else {
			config->clearMqtt();
		}
	}

	if(server.hasArg("authConfig") && server.arg("authConfig") == "true") {
		config->setAuthSecurity((byte)server.arg("authSecurity").toInt());
		if(config->getAuthSecurity() > 0) {
			config->setAuthUser(server.arg("authUser"));
			config->setAuthPassword(server.arg("authPassword"));
		} else {
			config->clearAuth();
		}
	}

	println("Saving configuration now...");

	if (debugger) config->print(debugger);
	if (config->save()) {
		println("Successfully saved.");
		if(config->isWifiChanged()) {
			String html = "<html><body><h1>Successfully Saved!</h1><a href=\"/\">Go to index</a></form>";
			server.send(200, "text/html", html);
			yield();
			println("Wifi config changed, rebooting");
			delay(1000);
#if defined(ESP8266)
			ESP.reset();
#elif defined(ESP32)
			ESP.restart();
#endif
		} else {
			server.sendHeader("Location", String("/"), true);
			server.send ( 302, "text/plain", "");
		}
	} else {
		println("Error saving configuration");
		String html = "<html><body><h1>Error saving configuration!</h1></form>";
		server.send(500, "text/html", html);
	}
}


size_t AmsWebServer::print(const char* text)
{
	if (debugger) debugger->print(text);
}
size_t AmsWebServer::println(const char* text)
{
	if (debugger) debugger->println(text);
}
size_t AmsWebServer::print(const Printable& data)
{
	if (debugger) debugger->print(data);
}
size_t AmsWebServer::println(const Printable& data)
{
	if (debugger) debugger->println(data);
}
