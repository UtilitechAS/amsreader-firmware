/*
 Name:		AmsToMqttBridge.ino
 Created:	3/13/2018 7:40:28 PM
 Author:	roarf
*/

#include "AmsToMqttBridge.h"
#include <ArduinoJson.h>
#include <MQTT.h>
#include <DNSServer.h>

#if defined(ESP8266)
ADC_MODE(ADC_VCC);  
#endif   

#include "HwTools.h"

#include "web/AmsWebServer.h"
#include "AmsConfiguration.h"
#include "HanReader.h"
#include "HanToJson.h"

#include "Aidon.h"
#include "Kaifa.h"
#include "Kamstrup.h"

HwTools hw;

DNSServer dnsServer;

// Configuration
AmsConfiguration config;

// Web server
AmsWebServer ws;

// WiFi client and MQTT client
WiFiClient *client;
MQTTClient mqtt(512);

// Object used for debugging
Stream* debugger = NULL;

// The HAN Port reader, used to read serial data and decode DLMS
HanReader hanReader;

// the setup function runs once when you press reset or power the board
void setup() {
	if(config.hasConfig()) {
		config.load();
	}

#if DEBUG_MODE
#if HW_ROARFRED
#if SOFTWARE_SERIAL
	SoftwareSerial *ser = new SoftwareSerial(-1, 1);
	ser->begin(115200, SWSERIAL_8N1);
	debugger = ser;
#else
	HardwareSerial *ser = &Serial;
	if(config.getMeterType() == 3) {
		ser->begin(2400, SERIAL_8N1);
	} else {
		ser->begin(2400, SERIAL_8E1);
	}
#endif
#else
	HardwareSerial *ser = &Serial;
	ser->begin(115200, SERIAL_8N1);
#endif
	debugger = ser;
#endif

	double vcc = hw.getVcc();

	if (debugger) {
		debugger->println("");
		debugger->println("Started...");
		debugger->print("Voltage: ");
		debugger->print(vcc);
		debugger->println("mV");
	}

	if (vcc > 0 && vcc < 3.1) {
		if(debugger) {
			debugger->println("Voltage is too low, sleeping");
			debugger->flush();
		}
		ESP.deepSleep(10000000);    //Deep sleep to allow output cap to charge up
	}  

	// Flash the LED, to indicate we can boot as AP now
	pinMode(LED_PIN, OUTPUT);
	pinMode(AP_BUTTON_PIN, INPUT_PULLUP);

	led_off();

	WiFi.disconnect(true);
	WiFi.softAPdisconnect(true);
	WiFi.mode(WIFI_OFF);

	if(config.hasConfig()) {
		if(debugger) config.print(debugger);
		WiFi_connect();
		client = new WiFiClient();
	} else {
		if(debugger) {
			debugger->println("No configuration, booting AP");
		}
		swapWifiMode();
	}

#if defined SOFTWARE_SERIAL
	if(config.getMeterType() == 3) {
		hanSerial->begin(2400, SWSERIAL_8N1);
	} else {
		hanSerial->begin(2400, SWSERIAL_8E1);
	}
#else
	if(config.getMeterType() == 3) {
		hanSerial->begin(2400, SERIAL_8N1);
	} else {
		hanSerial->begin(2400, SERIAL_8E1);
	}
#if defined UART2
	hanSerial->swap();
#endif
#endif

	hanReader.setup(hanSerial, 0);

	// Compensate for the known Kaifa bug
	hanReader.compensateFor09HeaderBug = (config.getMeterType() == 1);

	ws.setup(&config, debugger, &mqtt);
}

int buttonTimer = 0;
bool buttonActive = false;
unsigned long longPressTime = 5000;
bool longPressActive = false;

bool wifiConnected = false;

unsigned long lastTemperatureRead = 0;
double temperature = -127;

void loop() {
	unsigned long now = millis();
	if (digitalRead(AP_BUTTON_PIN) == LOW) {
		if (buttonActive == false) {
			buttonActive = true;
			buttonTimer = now;
		}

		if ((now - buttonTimer > longPressTime) && (longPressActive == false)) {
			longPressActive = true;
			swapWifiMode();
		}
	} else {
		if (buttonActive == true) {
			if (longPressActive == true) {
				longPressActive = false;
			} else {
				// Single press action
			}
			buttonActive = false;
		}
	}

	if(now - lastTemperatureRead > 10000) {
		temperature = hw.getTemperature();
		lastTemperatureRead = now;
	}

	// Only do normal stuff if we're not booted as AP
	if (WiFi.getMode() != WIFI_AP) {
		// Turn off the LED
		led_off();

		// Reconnect to WiFi and MQTT as needed
		if (WiFi.status() != WL_CONNECTED) {
			wifiConnected = false;
			WiFi_connect();
		} else {
			if(!wifiConnected) {
				wifiConnected = true;
				if(debugger) debugger->println("Successfully connected to WiFi!");
			}
			if (!config.getMqttHost().isEmpty()) {
				mqtt.loop();
				delay(10);
				if(!mqtt.connected() || config.isMqttChanged()) {
					MQTT_connect();
				}
			} else if(mqtt.connected()) {
				mqtt.disconnect();
			}
		}
	} else {
		dnsServer.processNextRequest();
		// Continously flash the LED when AP mode
		if (now / 50 % 64 == 0)   led_on();
		else							led_off();

	}
	readHanPort();
	ws.loop();
	delay(1); // Needed for auto modem sleep
}


void led_on()
{
#if LED_ACTIVE_HIGH
	digitalWrite(LED_PIN, HIGH);
#else
	digitalWrite(LED_PIN, LOW);
#endif
}


void led_off()
{
#if LED_ACTIVE_HIGH
	digitalWrite(LED_PIN, LOW);
#else
	digitalWrite(LED_PIN, HIGH);
#endif
}

void swapWifiMode() {
	led_on();
	WiFiMode_t mode = WiFi.getMode();
	dnsServer.stop();
	WiFi.disconnect(true);
	WiFi.softAPdisconnect(true);
	WiFi.mode(WIFI_OFF);
	yield();

	if (mode != WIFI_AP || !config.hasConfig()) {
		if(debugger) debugger->println("Swapping to AP mode");
		WiFi.softAP("AMS2MQTT");
		WiFi.mode(WIFI_AP);

		dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
		dnsServer.start(53, "*", WiFi.softAPIP());
	} else {
		if(debugger) debugger->println("Swapping to STA mode");
		WiFi_connect();
	}
	delay(500);
	led_off();
}

void mqttMessageReceived(String &topic, String &payload)
{

	if (debugger) {
		debugger->println("Incoming MQTT message:");
		debugger->print("[");
		debugger->print(topic);
		debugger->print("] ");
		debugger->println(payload);
	}

	// Do whatever needed here...
	// Ideas could be to query for values or to initiate OTA firmware update
}

bool even = true;
unsigned long lastSuccessfulRead = 0;
void readHanPort() {
	if (hanReader.read()) {
		lastSuccessfulRead = millis();

		if(config.getMeterType() > 0) {
			// Flash LED on, this shows us that data is received
			led_on();

			// Get the timestamp (as unix time) from the package
			time_t time = hanReader.getPackageTime();
			if (debugger) debugger->print("Time of the package is: ");
			if (debugger) debugger->println(time);

			// Define a json object to keep the data
			StaticJsonDocument<1024> json;

			// Any generic useful info here
			json["id"] = WiFi.macAddress();
			json["up"] = millis();
			json["t"] = time;
			double vcc = hw.getVcc();
			if(vcc > 0) {
				json["vcc"] = vcc;
			}
			float rssi = WiFi.RSSI();
			rssi = isnan(rssi) ? -100.0 : rssi;
			json["rssi"] = rssi;
			if(temperature != -127) {
				json["temp"] = temperature;
			}

			// Add a sub-structure to the json object,
			// to keep the data from the meter itself
			JsonObject data = json.createNestedObject("data");

			hanToJson(data, config.getMeterType(), hanReader);

			if(!config.getMqttHost().isEmpty() && !config.getMqttPublishTopic().isEmpty()) {
				// Write the json to the debug port
				if (debugger) {
					debugger->print("Sending data to MQTT: ");
					serializeJsonPretty(json, *debugger);
					debugger->println();
				}

				// Publish the json to the MQTT server
				String msg;
				serializeJson(json, msg);

				mqtt.publish(config.getMqttPublishTopic(), msg.c_str());
				mqtt.loop();
			}
			ws.setJson(json);

			// Flash LED off
			led_off();
		} else {
			for(int i = 1; i <= 3; i++) {
				String list;
				switch(i) {
					case 1:
						list = hanReader.getString((int) Kaifa_List1Phase::ListVersionIdentifier);
						break;
					case 2:
						list = hanReader.getString((int) Aidon_List1Phase::ListVersionIdentifier);
						break;
					case 3:
						list = hanReader.getString((int) Kamstrup_List1Phase::ListVersionIdentifier);
						break;
				}
				if(!list.isEmpty()) {
					list.toLowerCase();
					if(list.startsWith("kfm")) {
						config.setMeterType(1);
						if(debugger) debugger->println("Detected Kaifa meter");
						break;
					} else if(list.startsWith("aidon")) {
						config.setMeterType(2);
						if(debugger) debugger->println("Detected Aidon meter");
						break;
					} else if(list.startsWith("kamstrup")) {
						config.setMeterType(3);
						if(debugger) debugger->println("Detected Kamstrup meter");
						break;
					}
				}
			}
			hanReader.compensateFor09HeaderBug = (config.getMeterType() == 1);
		}
	}

	if(config.getMeterType() == 0 && millis() - lastSuccessfulRead > 10000) {
		lastSuccessfulRead = millis();
		if(debugger) debugger->println("No data for current setting, switching parity");
#if defined SOFTWARE_SERIAL
			if(even) {
				hanSerial->begin(2400, SWSERIAL_8N1);
			} else {
				hanSerial->begin(2400, SWSERIAL_8E1);
			}
#else
			if(even) {
				hanSerial->begin(2400, SERIAL_8N1);
			} else {
				hanSerial->begin(2400, SERIAL_8E1);
			}
#endif
		even = !even;
	}
}

unsigned long wifiTimeout = WIFI_CONNECTION_TIMEOUT;
unsigned long lastWifiRetry = -WIFI_CONNECTION_TIMEOUT;
void WiFi_connect() {
	if(millis() - lastWifiRetry < wifiTimeout) {
		delay(50);
		return;
	}
	lastWifiRetry = millis();

	if (debugger)
	{
		debugger->println();
		debugger->println();
		debugger->print("Connecting to WiFi network ");
		debugger->println(config.getWifiSsid());
	}

	if (WiFi.status() != WL_CONNECTED) {
		WiFi.disconnect();
		yield();

		WiFi.enableAP(false);
		WiFi.mode(WIFI_STA);
//		WiFi.setOutputPower(0);
		if(!config.getWifiIp().isEmpty()) {
			IPAddress ip, gw, sn(255,255,255,0);
			ip.fromString(config.getWifiIp());
			gw.fromString(config.getWifiGw());
			sn.fromString(config.getWifiSubnet());
			WiFi.config(ip, gw, sn);
		}
		WiFi.begin(config.getWifiSsid().c_str(), config.getWifiPassword().c_str());
		yield();
	}
}

unsigned long lastMqttRetry = -10000;
void MQTT_connect() {
	if(config.getMqttHost().isEmpty()) {
		if(debugger) debugger->println("No MQTT config");
		return;
	}
	if(millis() - lastMqttRetry < 5000) {
		yield();
		return;
	}
	lastMqttRetry = millis();
	if(debugger) {
		debugger->print("Connecting to MQTT: ");
		debugger->print(config.getMqttHost());
		debugger->print(", port: ");
		debugger->print(config.getMqttPort());
		debugger->println();
	}

	mqtt.disconnect();
	yield();

	mqtt.begin(config.getMqttHost().c_str(), config.getMqttPort(), *client);

	// Connect to a unsecure or secure MQTT server
	if ((config.getMqttUser().isEmpty() && mqtt.connect(config.getMqttClientId().c_str())) ||
		(!config.getMqttUser().isEmpty() && mqtt.connect(config.getMqttClientId().c_str(), config.getMqttUser().c_str(), config.getMqttPassword().c_str()))) {
		if (debugger) debugger->println("\nSuccessfully connected to MQTT!");
		config.ackMqttChange();

		// Subscribe to the chosen MQTT topic, if set in configuration
		if (!config.getMqttSubscribeTopic().isEmpty()) {
			mqtt.subscribe(config.getMqttSubscribeTopic());
			if (debugger) debugger->printf("  Subscribing to [%s]\r\n", config.getMqttSubscribeTopic().c_str());
		}

		sendMqttData("Connected!");
	} else {
		if (debugger) {
			debugger->print(" failed, ");
			debugger->println(" trying again in 5 seconds");
		}
	}
	yield();
}

// Send a simple string embedded in json over MQTT
void sendMqttData(String data)
{
	// Make sure we have configured a publish topic
	if (config.getMqttPublishTopic().isEmpty())
		return;

	// Build a json with the message in a "data" attribute
	StaticJsonDocument<500> json;
	json["id"] = WiFi.macAddress();
	json["up"] = millis();
	json["data"] = data;
	double vcc = hw.getVcc();
	if(vcc > 0) {
		json["vcc"] = vcc;
	}
	float rssi = WiFi.RSSI();
	rssi = isnan(rssi) ? -100.0 : rssi;
	json["rssi"] = rssi;

	// Stringify the json
	String msg;
	serializeJson(json, msg);

	// Send the json over MQTT
	mqtt.publish(config.getMqttPublishTopic(), msg.c_str());

	if (debugger) debugger->print("sendMqttData: ");
	if (debugger) debugger->println(data);
}


