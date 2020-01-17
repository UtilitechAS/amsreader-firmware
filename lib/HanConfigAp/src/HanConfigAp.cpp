#include "HanConfigAp.h"

#include "index_html.h"
#include "configuration_html.h"
#include "bootstrap_css.h"
#include "application_css.h"
#include "jquery_js.h"
#include "gaugemeter_js.h"
#include "index_js.h"

#include "Base64.h"

#if defined(ESP8266)
ESP8266WebServer HanConfigAp::server(80);
#elif defined(ESP32) // ARDUINO_ARCH_ESP32
WebServer HanConfigAp::server(80);
#endif
Stream* HanConfigAp::debugger;

bool HanConfigAp::hasConfig() {
	return config.hasConfig();
}

void HanConfigAp::setup(int accessPointButtonPin, Stream* debugger)
{
	this->debugger = debugger;

	// Test if we're missing configuration
	if (!config.hasConfig())
	{
		print("No config. We're booting as AP. Look for SSID ");
		println(this->AP_SSID);
		isActivated = true;
	}
	else
	{
		// Load the configuration
		config.load();
		if (this->debugger) config.print(this->debugger);

		if (accessPointButtonPin != INVALID_BUTTON_PIN)
		{
			// Assign pin for boot as AP
			pinMode(accessPointButtonPin, INPUT_PULLUP);

			// Test if we're holding down the AP pin, over 5 seconds
			int time = millis() + 5000;
			print("Press the AP button now to boot as access point");
			while (millis() < time)
			{
				print(".");
				if (digitalRead(accessPointButtonPin) == LOW)
				{
					print("AP button was pressed. Booting as access point now. Look for SSID ");
					println(this->AP_SSID);
					isActivated = true;
					break;
				}
				delay(100);
			}
			println("");
		}
	}

	if (isActivated)
	{
		// Setup AP
		WiFi.disconnect(true);
		WiFi.softAPdisconnect(true);
		WiFi.mode(WIFI_OFF);
		delay(2000);

		WiFi.softAP(AP_SSID);
		WiFi.mode(WIFI_AP);

		/* Setup the DNS server redirecting all the domains to this IP */
		dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
		dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
	}
}

void HanConfigAp::enableWeb() {
	server.on("/", indexHtml);
	server.on("/configuration", configurationHtml);
	server.on("/css/bootstrap.css", bootstrapCss);
	server.on("/css/application.css", applicationCss);
	server.on("/js/jquery.js", jqueryJs);
	server.on("/js/gaugemeter.js", gaugemeterJs);
	server.on("/js/index.js", indexJs);

	server.on("/save", handleSave);

	server.begin(); // Web server start

	print("Web server is ready for config at http://");
	if(isActivated) {
		print(WiFi.softAPIP());
	} else {
		print(WiFi.localIP());
	}
	println("/");
}

bool HanConfigAp::loop() {
	if(isActivated) {
		//DNS
		dnsServer.processNextRequest();
	}

	//HTTP
	server.handleClient();

	return isActivated;
}

void HanConfigAp::handleSave() {
	configuration *config = new configuration();

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

	if (HanConfigAp::debugger) config->print(HanConfigAp::debugger);
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

void HanConfigAp::indexHtml() {
	println("Serving /index.html over http...");

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	server.send(200, "text/html", INDEX_HTML, sizeof(INDEX_HTML)-1);

}

void HanConfigAp::configurationHtml() {
	println("Serving /configuration.html over http...");

	configuration *config = new configuration();
	config->load();

	bool access = !config->hasConfig() || config->authSecurity == 0;
	if(config->authSecurity > 0 && server.hasHeader("Authorization")) {
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
		server.sendHeader("WWW-Authenticate", "Basic realm=\"Secure Area\"");
		server.send(401, "text/html", "");
		return;
	}
	String html = CONFIGURATION_HTML;

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");

	if(config->hasConfig()) {
		html.replace("${config.ssid}", config->ssid);
		html.replace("${config.ssidPassword}", config->ssidPassword);
		html.replace("${config.meterType}", String(config->fuseSize));
		for(int i = 0; i<3; i++) {
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
		for(int i = 0; i<2; i++) {
			html.replace("${config.authSecurity" + String(i) + "}", config->authSecurity == i ? "selected"  : "");
		}
		html.replace("${config.authPass}", config->authPass);
		html.replace("${config.fuseSize}", String(config->fuseSize));
		for(int i = 0; i<63; i++) {
			html.replace("${config.fuseSize" + String(i) + "}", config->fuseSize == i ? "selected"  : "");
		}
	} else {
		html.replace("${config.ssid}", "");
		html.replace("${config.ssidPassword}", "");
		html.replace("${config.meterType}", "");
		for(int i = 0; i<3; i++) {
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
		for(int i = 0; i<2; i++) {
			html.replace("${config.authSecurity" + String(i) + "}", i == 0 ? "selected"  : "");
		}
		html.replace("${config.authUser}", "");
		html.replace("${config.authPass}", "");
		html.replace("${config.fuseSize}", "");
		for(int i = 0; i<63; i++) {
			html.replace("${config.fuseSize" + String(i) + "}", i == 0 ? "selected"  : "");
		}
	}
	server.send(200, "text/html", html);
}

void HanConfigAp::bootstrapCss() {
	println("Serving /bootstrap.css over http...");

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	server.send(200, "text/html", BOOTSTRAP_CSS, sizeof(BOOTSTRAP_CSS)-1);

}

void HanConfigAp::applicationCss() {
	println("Serving /application.css over http...");

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	server.send(200, "text/html", APPLICATION_CSS, sizeof(APPLICATION_CSS)-1);

}

void HanConfigAp::jqueryJs() {
	println("Serving /jquery.js over http...");

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	server.send(200, "text/html", JQUERY_JS, sizeof(JQUERY_JS)-1);

}

void HanConfigAp::gaugemeterJs() {
	println("Serving /gaugemeter.js over http...");

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	server.send(200, "text/html", GAUEGMETER_JS, sizeof(GAUEGMETER_JS)-1);

}

void HanConfigAp::indexJs() {
	println("Serving /index.js over http...");

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	server.send(200, "text/html", INDEX_JS, sizeof(INDEX_JS)-1);

}

size_t HanConfigAp::print(const char* text)
{
	if (debugger) debugger->print(text);
}
size_t HanConfigAp::println(const char* text)
{
	if (debugger) debugger->println(text);
}
size_t HanConfigAp::print(const Printable& data)
{
	if (debugger) debugger->print(data);
}
size_t HanConfigAp::println(const Printable& data)
{
	if (debugger) debugger->println(data);
}
