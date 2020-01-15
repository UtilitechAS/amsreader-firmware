#include "HanConfigAp.h"
#include "config_html.h"
#include "style_css.h"

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
	server.on("/", handleRoot);
	server.on("/style.css", handleStyle);
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

/** Handle root or redirect to captive portal */
void HanConfigAp::handleRoot() {
	println("Serving / over http...");

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	String html = CONFIG_HTML;

	configuration *config = new configuration();
	config->load();

	if(config->hasConfig()) {
		html.replace("${config.ssid}", config->ssid);
		html.replace("${config.ssidPassword}", config->ssidPassword);
		switch (config->meterType) {
			case 1:
				html.replace("${config.meterType0}", "");
				html.replace("${config.meterType1}", "selected");
				html.replace("${config.meterType2}", "");
				html.replace("${config.meterType3}", "");
				break;
			case 2:
				html.replace("${config.meterType0}", "");
				html.replace("${config.meterType1}", "");
				html.replace("${config.meterType2}", "selected");
				html.replace("${config.meterType3}", "");
				break;
			case 3:
				html.replace("${config.meterType0}", "");
				html.replace("${config.meterType1}", "");
				html.replace("${config.meterType2}", "");
				html.replace("${config.meterType3}", "selected");
				break;
			default:
				html.replace("${config.meterType0}", "selected");
				html.replace("${config.meterType1}", "");
				html.replace("${config.meterType2}", "");
				html.replace("${config.meterType3}", "");
		}
		html.replace("${config.mqtt}", config->mqtt);
		html.replace("${config.mqttPort}", String(config->mqttPort));
		html.replace("${config.mqttClientID}", config->mqttClientID);
		html.replace("${config.mqttPublishTopic}", config->mqttPublishTopic);
		html.replace("${config.mqttSubscribeTopic}", config->mqttSubscribeTopic);
		html.replace("${config.mqttUser}", config->mqttUser);
		html.replace("${config.mqttPass}", config->mqttPass);
	} else {
		html.replace("${config.ssid}", "");
		html.replace("${config.ssidPassword}", "");
		html.replace("${config.meterType0}", "selected");
		html.replace("${config.meterType1}", "");
		html.replace("${config.meterType2}", "");
		html.replace("${config.meterType3}", "");
		html.replace("${config.mqtt}", "");
		html.replace("${config.mqttPort}", "1883");
		html.replace("${config.mqttClientID}", "");
		html.replace("${config.mqttPublishTopic}", "");
		html.replace("${config.mqttSubscribeTopic}", "");
		html.replace("${config.mqttUser}", "");
		html.replace("${config.mqttPass}", "");
	}
	server.send(200, "text/html", html);
}

void HanConfigAp::handleStyle() {
	println("Serving /style.css over http...");

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	server.send(200, "text/html", STYLE_CSS, sizeof(STYLE_CSS));
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

	println("Saving configuration now...");

	if (HanConfigAp::debugger) config->print(HanConfigAp::debugger);
	if (config->save())
	{
		println("Successfully saved. Will reboot now.");
		String html = "<html><body><h1>Successfully Saved!</h1><h3>Device is restarting now...</h3></form>";
		server.send(200, "text/html", html);
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
