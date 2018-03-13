// 
// 
// 

#include "accesspoint.h"

ESP8266WebServer accesspoint::server(80);

void accesspoint::setup(int accessPointButtonPin, Stream& debugger) 
{
	this->debugger = &debugger;

	// Test if we're missing configuration
	if (!config.hasConfig())
	{
		this->print("No config. We're booting as AP. Look for SSID ");
		this->println(this->AP_SSID);
		isActivated = true;
	}
	else
	{
		// Test if we're holding down the AP pin, over 5 seconds
		int time = millis() + 5000;
		this->print("Press the AP button now to boot as access point");
		while (millis() < time)
		{
			this->print(".");
			if (digitalRead(accessPointButtonPin) == LOW)
			{
				this->println("");
				this->println("AP button was pressed. Booting as access point now");
				isActivated = true;
				break;
			}
			delay(100);
		}
	}

	if (isActivated)
	{
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
		server.begin(); // Web server start

		this->print("Web server is ready for config at http://");
		this->print(WiFi.softAPIP());
		this->println("/");
	}
}

/** Handle root or redirect to captive portal */
void accesspoint::handleRoot() {
	Serial.println("Serving / over http...");

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
	server.sendContent(
		"<html><head></head><body>"
		"<h1>HELLO WORLD!!</h1>"
	);
	server.sendContent(
		"<p>You may want to <a href='/wifi'>config the wifi connection</a>.</p>"
		"</body></html>"
	);
	server.client().stop(); // Stop is needed because we sent no content length
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


size_t accesspoint::print(const char* text)
{
	if (this->debugger) this->debugger->print(text);
}
size_t accesspoint::println(const char* text)
{
	if (this->debugger) this->debugger->println(text);
}
size_t accesspoint::print(const Printable& data)
{
	if (this->debugger) this->debugger->print(data);
}
size_t accesspoint::println(const Printable& data)
{
	if (this->debugger) this->debugger->println(data);
}
