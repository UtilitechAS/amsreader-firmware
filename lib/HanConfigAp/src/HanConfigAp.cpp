#include "HanConfigAp.h"

Stream* HanConfigAp::debugger;

bool HanConfigAp::hasConfig() {
	return config->hasConfig();
}

void HanConfigAp::setup(int accessPointButtonPin, configuration* config, Stream* debugger)
{
	this->debugger = debugger;
	this->config = config;

	// Test if we're missing configuration
	if (!config->hasConfig())
	{
		print("No config. We're booting as AP. Look for SSID ");
		println(this->AP_SSID);
		isActivated = true;
	}
	else
	{
		// Load the configuration
		if (this->debugger) config->print(this->debugger);

		if (accessPointButtonPin != INVALID_BUTTON_PIN)
		{
			// Assign pin for boot as AP
			pinMode(accessPointButtonPin, INPUT_PULLUP);

			// Test if we're holding down the AP pin, over 1 second
			int time = millis() + 1000;
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

		WiFi.mode(WIFI_AP);
		WiFi.setOutputPower(0);
		WiFi.softAP(AP_SSID);

		/* Setup the DNS server redirecting all the domains to this IP */
		dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
		dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
	}
}

bool HanConfigAp::loop() {
	if(isActivated) {
		//DNS
		dnsServer.processNextRequest();
	}

	return isActivated;
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
