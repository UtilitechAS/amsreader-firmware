// 
// 
// 

#include "accesspoint.h"

ESP8266WebServer accesspoint::server(80);
Stream* accesspoint::debugger;

bool accesspoint::hasConfig() {
	return config.hasConfig();
}

void accesspoint::setup(int accessPointButtonPin, Stream& debugger) 
{
	this->debugger = &debugger;

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
		if (this->debugger) config.print(debugger);

		// Test if we're holding down the AP pin, over 5 seconds
		int time = millis() + 5000;
		print("Press the AP button now to boot as access point");
		while (millis() < time)
		{
			print(".");
			if (digitalRead(accessPointButtonPin) == LOW)
			{
				println("");
				print("AP button was pressed. Booting as access point now. Look for SSID ");
				println(this->AP_SSID);
				isActivated = true;
				break;
			}
			delay(100);
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

		server.on("/", handleRoot);
		server.on("/save", handleSave);
		server.begin(); // Web server start

		print("Web server is ready for config at http://");
		print(WiFi.softAPIP());
		println("/");
	}
}

bool accesspoint::loop() {
	if (isActivated)
	{
		//DNS
		dnsServer.processNextRequest();
		//HTTP
		server.handleClient();
		return true;
	}
	else
	{
		return false;
	}
}

/** Handle root or redirect to captive portal */
void accesspoint::handleRoot() {
	println("Serving / over http...");

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
	String html = String("<html>\r\n\r\n<head>\r\n\t<style type=\"text/css\">\r\n\t\tbody,\r\n\t\tdiv,\r\n\t\tinput {\r\n\t\t\tfont-family: \"Roboto\", Arial, Lucida Grande;\r\n\t\t}\r\n\r\n\t\t.wrapper {\r\n\t\t\twidth: 50%;\r\n\t\t\tposition: absolute;\r\n\t\t\tpadding: 30px;\r\n\t\t\tbackground-color: #FFF;\r\n\t\t\tborder-radius: 1px;\r\n\t\t\tcolor: #333;\r\n\t\t\tborder-color: rgba(0, 0, 0, 0.03);\r\n\t\t\tbox-shadow: 0 2px 2px rgba(0, 0, 0, .24), 0 0 2px rgba(0, 0, 0, .12);\r\n\t\t\tmargin-left: 20px;\r\n\t\t\tmargin-top: 20px;\r\n\t\t}\r\n\r\n\t\tdiv {\r\n\t\t\tpadding-bottom: 5px;\r\n\t\t}\r\n\r\n\t\tinput {\r\n\t\t\tfont-family: \"Roboto\", \"Helvetica Neue\", sans-serif;\r\n\t\t\tfont-size: 14px;\r\n\t\t\tline-height: 16px;\r\n\t\t\tbottom: 30px;\r\n\t\t\tborder: none;\r\n\t\t\tborder-bottom: 1px solid #d4d4d4;\r\n\t\t\tpadding: 10px;\r\n\t\t\twidth: 80%;\r\n\t\t\tbackground: transparent;\r\n\t\t\ttransition: all .25s ease;\r\n\t\t}\r\n\r\n\t\tinput[type=number] {\r\n\t\t\twidth: 70px;\r\n\t\t\tmargin-left: 5px;\r\n\t\t}\r\n\r\n\t\tinput:focus {\r\n\t\t\toutline: none;\r\n\t\t\tborder-bottom: 1px solid #3f51b5;\r\n\t\t}\r\n\r\n\t\th2 {\r\n\t\t\ttext-align: left;\r\n\t\t\tfont-size: 20px;\r\n\t\t\tfont-weight: bold;\r\n\t\t\tletter-spacing: 3px;\r\n\t\t\tline-height: 28px;\r\n\t\t}\r\n\r\n\t\t.submit-button {\r\n\t\t\tposition: absolute;\r\n\t\t\ttext-align: right;\r\n\t\t\tborder-radius: 20px;\r\n\t\t\tborder-bottom-right-radius: 0;\r\n\t\t\tborder-top-right-radius: 0;\r\n\t\t\tbackground-color: #3f51b5;\r\n\t\t\tcolor: #FFF;\r\n\t\t\tpadding: 12px 25px;\r\n\t\t\tdisplay: inline-block;\r\n\t\t\tfont-size: 12px;\r\n\t\t\tfont-weight: bold;\r\n\t\t\tletter-spacing: 2px;\r\n\t\t\tright: 0px;\r\n\t\t\tbottom: 10px;\r\n\t\t\tcursor: pointer;\r\n\t\t\ttransition: all .25s ease;\r\n\t\t\tbox-shadow: 0 2px 2px rgba(0, 0, 0, .24), 0 0 2px rgba(0, 0, 0, .12);\r\n\t\t\twidth: 100px;\r\n\t\t}\r\n\r\n\t\t.select-style {\r\n\t\t\tborder-top: 10px solid white;\r\n\t\t\tborder-bottom: 1px solid #d4d4d4;\r\n\t\t\tcolor: #ffffff;\r\n\t\t\tcursor: pointer;\r\n\t\t\tdisplay: block;\r\n\t\t\tfont-family: Roboto, \"Helvetica Neue\", sans-serif;\r\n\t\t\tfont-size: 14px;\r\n\t\t\tfont-weight: 400;\r\n\t\t\theight: 16px;\r\n\t\t\tline-height: 14px;\r\n\t\t\tmin-width: 200px;\r\n\t\t\tpadding-bottom: 7px;\r\n\t\t\tpadding-left: 0px;\r\n\t\t\tpadding-right: 0px;\r\n\t\t\tposition: relative;\r\n\t\t\ttext-align: left;\r\n\t\t\twidth: 80%;\r\n\t\t\t-webkit-box-direction: normal;\r\n\t\t\toverflow: hidden;\r\n\t\t\tbackground: #ffffff url(\"data:image/png;base64,R0lGODlhDwAUAIABAAAAAP///yH5BAEAAAEALAAAAAAPABQAAAIXjI+py+0Po5wH2HsXzmw//lHiSJZmUAAAOw==\") no-repeat 98% 50%;\r\n\t\t}\r\n\r\n\t\t.disabled-option {\r\n\t\t\tcolor: #d4d4d4;\r\n\r\n\t\t}\r\n\r\n\t\t.select-style select {\r\n\t\t\tpadding: 5px 8px;\r\n\t\t\twidth: 100%;\r\n\t\t\tborder: none;\r\n\r\n\t\t\tbox-shadow: none;\r\n\t\t\tbackground: transparent;\r\n\t\t\tbackground-image: none;\r\n\t\t\t-webkit-appearance: none;\r\n\t\t}\r\n\r\n\t\t.select-style select:focus {\r\n\t\t\toutline: none;\r\n\t\t\tborder: none;\r\n\t\t}\r\n\r\n\t\t@media only screen and (max-width: 1000px) {\r\n\t\t\t.wrapper {\r\n\t\t\t\twidth: 80%;\r\n\r\n\r\n\t\t\t}\r\n\t\t}\r\n\r\n\t\t@media only screen and (max-width: 300px) {\r\n\t\t\t.wrapper {\r\n\t\t\t\twidth: 75%;\r\n\t\t\t}\r\n\t\t}\r\n\r\n\t\t@media only screen and (max-width: 600px) {\r\n\t\t\t.wrapper {\r\n\t\t\t\twidth: 80%;\r\n\t\t\t\tmargin-left: 0px;\r\n\t\t\t\tmargin-top: 0px;\r\n\t\t\t}\r\n\t\t\t.submit-button {\r\n\t\t\t\tbottom: 0px;\r\n\t\t\t\twidth: 70px;\r\n\t\t\t}\r\n\t\t\tinput {\r\n\t\t\t\twidth: 100%;\r\n\t\t\t}\r\n\t\t}\r\n\t</style>\r\n</head>\r\n\r\n<body>\r\n\t<form method='post' action='/save'>\r\n\r\n\t\t<div class=\"wrapper\">\r\n\r\n\t\t\t<div class=\"inner-wrapper\">\r\n\t\t\t\t<div>\r\n\t\t\t\t\t<h2>WiFi</h2>\r\n\t\t\t\t</div>\r\n\t\t\t\t<div>\r\n\t\t\t\t\t<input type='text' name='ssid' placeholder=\"SSID\">\r\n\t\t\t\t</div>\r\n\t\t\t\t<div>\r\n\t\t\t\t\t<input type='password' name='ssidPassword' placeholder=\"Password\">\r\n\t\t\t\t</div>\r\n\t\t\t</div>\r\n\t\t\t<div class=\"inner-wrapper\">\r\n\t\t\t\t<div>\r\n\t\t\t\t\t<h2>Meter Type</h2>\r\n\t\t\t\t</div>\r\n\t\t\t\t<div class=\"select-style\">\r\n\t\t\t\t\t<select name=\"meterType\">\r\n\t\t\t\t\t\t<option value=\"0\" disabled=\"true\" selected class=\"disabled-option\"> SELECT TYPE </option>\r\n\t\t\t\t\t\t<option value=\"1\">Kaifa</option>\r\n\t\t\t\t\t\t<option value=\"2\">Aidon</option>\r\n\t\t\t\t\t\t<option value=\"3\">Kamstrup</option>\r\n\t\t\t\t\t</select>\r\n\t\t\t\t</div>\r\n\t\t\t</div>\r\n\t\t\t<div class=\"inner-wrapper\">\r\n\t\t\t\t<div>\r\n\t\t\t\t\t<h2>MQTT</h2>\r\n\t\t\t\t</div>\r\n\t\t\t\t<div>\r\n\t\t\t\t\t<input type='text' name='mqtt' placeholder=\"Server\">\r\n\t\t\t\t\t<input type='number' name='mqttPort' value=\"1883\" placeholder=\"port\">\r\n\t\t\t\t</div>\r\n\t\t\t\t<div>\r\n\t\t\t\t\t<input type='text' name='mqttClientID' placeholder=\"Client ID\">\r\n\t\t\t\t</div>\r\n\t\t\t\t<div>\r\n\t\t\t\t\t<input type='text' name='mqttPublishTopic' placeholder=\"Publish Topic\">\r\n\t\t\t\t</div>\r\n\t\t\t\t<div>\r\n\t\t\t\t\t<input type='text' name='mqttSubscribeTopic' placeholder=\"Subscribe Topic\">\r\n\t\t\t\t</div>\r\n\t\t\t\t<div>\r\n\t\t\t\t\t<input type='text' name='mqttUser' placeholder=\"Username (leave blank for unsecure)\">\r\n\t\t\t\t</div>\r\n\t\t\t\t<div>\r\n\t\t\t\t\t<input type='password' name='mqttPass' placeholder=\"Password\">\r\n\t\t\t\t</div>\r\n\t\t\t\t<div>\r\n\t\t\t\t\t<input class=\"submit-button\" type='submit' value='save'>\r\n\t\t\t\t</div>\r\n\t\t\t</div>\r\n\t\t</div>\r\n\t</form>\r\n\r\n\t<body>\r\n\r\n</html>");
	server.sendContent(html);
	server.sendContent(""); // (End 1/2) Sending a zero length chunk to terminate session
	server.client().stop(); // (End 2/2) Stop is needed because we sent no content length
}


void accesspoint::handleSave() {
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

	if (accesspoint::debugger) config->print(*accesspoint::debugger);
	if (config->save())
	{
		println("Successfully saved. Will roboot now.");
		String html = "<html><body><h1>Successfully Saved!</h1><h3>Device is restarting now...</h3></form>";
		server.send(200, "text/html", html);
		ESP.reset();
	}
	else
	{
		println("Error saving configuration");
		String html = "<html><body><h1>Error saving configuration!</h1></form>";
		server.send(500, "text/html", html);
	}
}

size_t accesspoint::print(const char* text)
{
	if (debugger) debugger->print(text);
}
size_t accesspoint::println(const char* text)
{
	if (debugger) debugger->println(text);
}
size_t accesspoint::print(const Printable& data)
{
	if (debugger) debugger->print(data);
}
size_t accesspoint::println(const Printable& data)
{
	if (debugger) debugger->println(data);
}
