#include "AmsWebServer.h"
#include "version.h"

#include "index_html.h"
#include "configuration_html.h"
#include "bootstrap_css.h"
#include "application_css.h"
#include "jquery_js.h"
#include "gaugemeter_js.h"
#include "index_js.h"

#include "Base64.h"

#if defined(ESP8266)
ESP8266WebServer server(80);
#elif defined(ESP32) // ARDUINO_ARCH_ESP32
WebServer server(80);
#endif

void AmsWebServer::setup(configuration* config, Stream* debugger) {
    this->config = config;
    this->debugger = debugger;

	server.on("/", std::bind(&AmsWebServer::indexHtml, this));
	server.on("/configuration", std::bind(&AmsWebServer::configurationHtml, this));
	server.on("/css/bootstrap.css", std::bind(&AmsWebServer::bootstrapCss, this));
	server.on("/css/application.css", std::bind(&AmsWebServer::applicationCss, this));
	server.on("/js/jquery.js", std::bind(&AmsWebServer::jqueryJs, this));
	server.on("/js/gaugemeter.js", std::bind(&AmsWebServer::gaugemeterJs, this)); 
	server.on("/js/index.js", std::bind(&AmsWebServer::indexJs, this));
	server.on("/data.json", std::bind(&AmsWebServer::dataJson, this));

	server.on("/save", std::bind(&AmsWebServer::handleSave, this));

	server.begin(); // Web server start

	print("Web server is ready for config at http://");
	if(WiFi.getMode() == WIFI_AP) {
		print(WiFi.softAPIP());
	} else {
		print(WiFi.localIP());
	}
	println("/");

    if(config->hasConfig() && config->fuseSize > 0) {
        maxPwr = config->fuseSize * 230;
    } else {
		maxPwr = 20000;
	}
}

void AmsWebServer::loop() {
	server.handleClient();
}

void AmsWebServer::setJson(StaticJsonDocument<500> json) {
    this->json = json;
}

bool AmsWebServer::checkSecurity(byte level) {
	bool access = !config->hasConfig() || config->authSecurity < level;
	if(!access && config->authSecurity >= level && server.hasHeader("Authorization")) {
		println(" forcing web security");
		String expectedAuth = String(config->authUser) + ":" + String(config->authPass);

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

	String html = INDEX_HTML;
	html.replace("${version}", VERSION);
	if(WiFi.getMode() != WIFI_AP) {
		html.replace("/css/bootstrap.css", "https://stackpath.bootstrapcdn.com/bootstrap/4.4.1/css/bootstrap.min.css");
		html.replace("/js/jquery.js", "https://code.jquery.com/jquery-3.4.1.min.js");
	}

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	server.send(200, "text/html", html);
}

void AmsWebServer::configurationHtml() {
	println("Serving /configuration.html over http...");

	if(!checkSecurity(1))
		return;

	String html = CONFIGURATION_HTML;
	html.replace("${version}", VERSION);
	if(WiFi.getMode() != WIFI_AP) {
		html.replace("/css/bootstrap.css", "https://stackpath.bootstrapcdn.com/bootstrap/4.4.1/css/bootstrap.min.css");
		html.replace("/js/jquery.js", "https://code.jquery.com/jquery-3.4.1.min.js");
	}

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");

	if(config->hasConfig()) {
		html.replace("${config.ssid}", config->ssid);
		html.replace("${config.ssidPassword}", config->ssidPassword);
		html.replace("${config.meterType}", String(config->fuseSize));
		for(int i = 0; i<4; i++) {
			html.replace("${config.meterType" + String(i) + "}", config->meterType == i ? "selected"  : "");
		}
		html.replace("${config.mqtt}", config->mqtt);
		html.replace("${config.mqttPort}", String(config->mqttPort));
		html.replace("${config.mqttClientID}", config->mqttClientID);
		html.replace("${config.mqttPublishTopic}", config->mqttPublishTopic);
		html.replace("${config.mqttSubscribeTopic}", config->mqttSubscribeTopic);
		html.replace("${config.mqttUser}", config->mqttUser);
		html.replace("${config.mqttPass}", config->mqttPass);
		html.replace("${config.authUser}", config->authUser);
		html.replace("${config.authSecurity}", String(config->authSecurity));
		for(int i = 0; i<3; i++) {
			html.replace("${config.authSecurity" + String(i) + "}", config->authSecurity == i ? "selected"  : "");
		}
		html.replace("${config.authPass}", config->authPass);
		html.replace("${config.fuseSize}", String(config->fuseSize));
		for(int i = 0; i<64; i++) {
			html.replace("${config.fuseSize" + String(i) + "}", config->fuseSize == i ? "selected"  : "");
		}
	} else {
		html.replace("${config.ssid}", "");
		html.replace("${config.ssidPassword}", "");
		html.replace("${config.meterType}", "");
		for(int i = 0; i<4; i++) {
			html.replace("${config.meterType" + String(i) + "}", i == 0 ? "selected"  : "");
		}
		html.replace("${config.mqtt}", "");
		html.replace("${config.mqttPort}", "1883");
		html.replace("${config.mqttClientID}", "");
		html.replace("${config.mqttPublishTopic}", "");
		html.replace("${config.mqttSubscribeTopic}", "");
		html.replace("${config.mqttUser}", "");
		html.replace("${config.mqttPass}", "");
		html.replace("${config.authSecurity}", "");
		for(int i = 0; i<3; i++) {
			html.replace("${config.authSecurity" + String(i) + "}", i == 0 ? "selected"  : "");
		}
		html.replace("${config.authUser}", "");
		html.replace("${config.authPass}", "");
		html.replace("${config.fuseSize}", "");
		for(int i = 0; i<64; i++) {
			html.replace("${config.fuseSize" + String(i) + "}", i == 0 ? "selected"  : "");
		}
	}
	server.send(200, "text/html", html);
}

void AmsWebServer::bootstrapCss() {
	println("Serving /bootstrap.css over http...");

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	server.send(200, "text/css", BOOTSTRAP_CSS);
}

void AmsWebServer::applicationCss() {
	println("Serving /application.css over http...");

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	server.send(200, "text/css", APPLICATION_CSS);
}

void AmsWebServer::jqueryJs() {
	println("Serving /jquery.js over http...");

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	server.send(200, "application/javascript", JQUERY_JS);
}

void AmsWebServer::gaugemeterJs() {
	println("Serving /gaugemeter.js over http...");

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	server.send(200, "application/javascript", GAUEGMETER_JS);
}

void AmsWebServer::indexJs() {
	println("Serving /index.js over http...");

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	server.send(200, "application/javascript", INDEX_JS);
}

void AmsWebServer::dataJson() {
	println("Serving /data.json over http...");

	if(!checkSecurity(2))
		return;

    String jsonStr;
	if(!json.isNull()) {
		println(" json has data");

		int u1 = json["data"]["U1"].as<int>();
		int u2 = json["data"]["U2"].as<int>();
		int u3 = json["data"]["U3"].as<int>();
		int pwr = json["data"]["P"].as<int>();

		if(config->hasConfig() && u1 > 0) {
			maxPwr = config->fuseSize * u1;
			if(u2 > 0) {
				maxPwr += config->fuseSize * u2;
				if(u3 > 0) {
					maxPwr += config->fuseSize * u3;
				}
			}
		}

		json["maxPower"] = maxPwr;
		json["pct"]     = min(pwr*100/maxPwr, 100);
		json["meterType"] = config->meterType;
		json["currentMillis"] = millis();

		serializeJson(json, jsonStr);
	} else {
		println(" json is empty");
		jsonStr = "{}";
	}

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	server.send(200, "application/json", jsonStr);
}

void AmsWebServer::handleSave() {
	String temp;

	temp = server.arg("ssid");
	config->ssid = new char[temp.length() + 1];
	temp.toCharArray(config->ssid, temp.length() + 1, 0);

	temp = server.arg("ssidPassword");
	config->ssidPassword = new char[temp.length() + 1];
	temp.toCharArray(config->ssidPassword, temp.length() + 1, 0);

	config->meterType = (byte)server.arg("meterType").toInt();

	temp = server.arg("mqtt");
	config->mqtt = new char[temp.length() + 1];
	temp.toCharArray(config->mqtt, temp.length() + 1, 0);

	config->mqttPort = (int)server.arg("mqttPort").toInt();

	temp = server.arg("mqttClientID");
	config->mqttClientID = new char[temp.length() + 1];
	temp.toCharArray(config->mqttClientID, temp.length() + 1, 0);

	temp = server.arg("mqttPublishTopic");
	config->mqttPublishTopic = new char[temp.length() + 1];
	temp.toCharArray(config->mqttPublishTopic, temp.length() + 1, 0);

	temp = server.arg("mqttSubscribeTopic");
	config->mqttSubscribeTopic = new char[temp.length() + 1];
	temp.toCharArray(config->mqttSubscribeTopic, temp.length() + 1, 0);

	temp = server.arg("mqttUser");
	config->mqttUser = new char[temp.length() + 1];
	temp.toCharArray(config->mqttUser, temp.length() + 1, 0);

	temp = server.arg("mqttPass");
	config->mqttPass = new char[temp.length() + 1];
	temp.toCharArray(config->mqttPass, temp.length() + 1, 0);

	config->authSecurity = (byte)server.arg("authSecurity").toInt();

	temp = server.arg("authUser");
	config->authUser = new char[temp.length() + 1];
	temp.toCharArray(config->authUser, temp.length() + 1, 0);

	temp = server.arg("authPass");
	config->authPass = new char[temp.length() + 1];
	temp.toCharArray(config->authPass, temp.length() + 1, 0);

	config->fuseSize = (int)server.arg("fuseSize").toInt();

	println("Saving configuration now...");

	if (debugger) config->print(debugger);
	if (config->save())
	{
		println("Successfully saved. Will reboot now.");
		String html = "<html><body><h1>Successfully Saved!</h1><h3>Device is restarting now...</h3><a href=\"/\">Go to index</a></form>";
		server.send(200, "text/html", html);
		yield();
		delay(1000);
#if defined(ESP8266)
		ESP.reset();
#elif defined(ESP32)
		ESP.restart();
#endif
	}
	else
	{
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
