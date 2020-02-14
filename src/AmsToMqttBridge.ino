/*
 Name:		AmsToMqttBridge.ino
 Created:	3/13/2018 7:40:28 PM
 Author:	roarf
*/

#include "AmsToMqttBridge.h"
#include <ArduinoJson.h>
#include <MQTT.h>

#include "web/AmsWebServer.h"
#include "HanConfigAp.h"
#include "HanReader.h"
#include "HanToJson.h"

// Configuration
configuration config;

// Object used to boot as Access Point
HanConfigAp ap;

// Web server
AmsWebServer ws;

// WiFi client and MQTT client
WiFiClient *client;
MQTTClient mqtt(512);

// Object used for debugging
HardwareSerial* debugger = NULL;

// The HAN Port reader, used to read serial data and decode DLMS
HanReader hanReader;

// the setup function runs once when you press reset or power the board
void setup() {
	if(config.hasConfig()) {
		config.load();
	}

#if DEBUG_MODE
	debugger = &Serial;
	#if SOFTWARE_SERIAL
		debugger->begin(115200, SERIAL_8N1);
	#else
		if(config.meterType == 3) {
			hanSerial->begin(2400, SERIAL_8N1);
		} else {
			hanSerial->begin(2400, SERIAL_8E1);
		}
	#endif
	while (!&debugger);
#endif

	if (debugger) {
		debugger->println("");
		debugger->println("Started...");
	}

	// Flash the LED, to indicate we can boot as AP now
	pinMode(LED_PIN, OUTPUT);
	led_on();

	delay(1000);

	// Initialize the AP
	ap.setup(AP_BUTTON_PIN, &config, debugger);

	led_off();

	if (!ap.isActivated)
	{
		setupWiFi();

		// Configure uart for AMS data
#if defined SOFTWARE_SERIAL
		if(config.meterType == 3) {
			hanSerial->begin(2400, SWSERIAL_8N1);
		} else {
			hanSerial->begin(2400, SWSERIAL_8E1);
		}
#else
		if(config.meterType == 3) {
			hanSerial->begin(2400, SERIAL_8N1);
		} else {
			hanSerial->begin(2400, SERIAL_8E1);
		}
#if defined UART2
		hanSerial->swap();
#endif
#endif
		while (!&hanSerial);

		hanReader.setup(hanSerial, debugger);

		// Compensate for the known Kaifa bug
		hanReader.compensateFor09HeaderBug = (config.meterType == 1);
	}

	ws.setup(&config, debugger);
}

// the loop function runs over and over again until power down or reset
void loop()
{
	// Only do normal stuff if we're not booted as AP
	if (!ap.loop())
	{
		// Turn off the LED
		led_off();

		// Reconnect to WiFi and MQTT as needed
		if (WiFi.status() != WL_CONNECTED) {
			WiFi_connect();
		} else {
			if (config.mqttHost) {
				mqtt.loop();
				yield();
				if(!mqtt.connected()) {
					MQTT_connect();
				}
			}
		}

		readHanPort();
	}
	else
	{
		// Continously flash the LED when AP mode
		if (millis() / 1000 % 2 == 0)   led_on();
		else							led_off();
	}
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


void setupWiFi()
{
	// Turn off AP
	WiFi.enableAP(false);

	// Connect to WiFi
	WiFi.mode(WIFI_STA);
	WiFi.begin(config.ssid, config.ssidPassword);

	// Wait for WiFi connection
	if (debugger) debugger->print("\nWaiting for WiFi to connect...");
	while (WiFi.status() != WL_CONNECTED) {
		if (debugger) debugger->print(".");
		delay(500);
	}
	if (debugger) debugger->println(" connected");

	client = new WiFiClient();
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

void readHanPort()
{
	if (hanReader.read() && config.hasConfig())
	{
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

		// Add a sub-structure to the json object,
		// to keep the data from the meter itself
		JsonObject data = json.createNestedObject("data");

#if defined TEMP_SENSOR_PIN
		// Get the temperature too
		tempSensor.requestTemperatures();
		data["temp"] = tempSensor.getTempCByIndex(0);
#endif

		hanToJson(data, config.meterType, hanReader);

		if(config.mqttHost != 0 && strlen(config.mqttHost) != 0 && config.mqttPublishTopic != 0 && strlen(config.mqttPublishTopic) != 0) {
			// Write the json to the debug port
			if (debugger) {
				debugger->print("Sending data to MQTT: ");
				serializeJsonPretty(json, *debugger);
				debugger->println();
			}

			// Publish the json to the MQTT server
			String msg;
			serializeJson(json, msg);

			mqtt.publish(config.mqttPublishTopic, msg.c_str());
			mqtt.loop();
		}
		ws.setJson(json);

		// Flash LED off
		led_off();
	}
}

void WiFi_connect() {
	// Connect to WiFi access point.
	if (debugger)
	{
		debugger->println();
		debugger->println();
		debugger->print("Connecting to WiFi network ");
		debugger->println(config.ssid);
	}

	if (WiFi.status() != WL_CONNECTED)
	{
		// Make one first attempt at connect, this seems to considerably speed up the first connection
		WiFi.disconnect();
		WiFi.begin(config.ssid, config.ssidPassword);
		delay(1000);
	}

	// Wait for the WiFi connection to complete
	long vTimeout = millis() + WIFI_CONNECTION_TIMEOUT;
	while (WiFi.status() != WL_CONNECTED) {
		delay(50);
		if (debugger) debugger->print(".");

		// If we timed out, disconnect and try again
		if (vTimeout < millis())
		{
			if (debugger)
			{
				debugger->print("Timout during connect. WiFi status is: ");
				debugger->println(WiFi.status());
			}
			WiFi.disconnect();
			WiFi.begin(config.ssid, config.ssidPassword);
			vTimeout = millis() + WIFI_CONNECTION_TIMEOUT;
		}
		yield();
	}

	if (debugger) {
		debugger->println();
		debugger->println("WiFi connected");
		debugger->println("IP address: ");
		debugger->println(WiFi.localIP());
	}
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.

unsigned long lastMqttRetry = -10000;
void MQTT_connect() {
	if(!config.mqttHost) {
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
		debugger->print(config.mqttHost);
		debugger->print(", port: ");
		debugger->print(config.mqttPort);
		debugger->println();
	}

	mqtt.disconnect();

	mqtt.begin(config.mqttHost, config.mqttPort, *client);

	// Connect to a unsecure or secure MQTT server
	if ((config.mqttUser == 0 && mqtt.connect(config.mqttClientID)) ||
		(config.mqttUser != 0 && mqtt.connect(config.mqttClientID, config.mqttUser, config.mqttPass))) {
		if (debugger) debugger->println("\nSuccessfully connected to MQTT!");

		// Subscribe to the chosen MQTT topic, if set in configuration
		if (config.mqttSubscribeTopic != 0 && strlen(config.mqttSubscribeTopic) > 0) {
			mqtt.subscribe(config.mqttSubscribeTopic);
			if (debugger) debugger->printf("  Subscribing to [%s]\r\n", config.mqttSubscribeTopic);
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
	if (config.mqttPublishTopic == 0 || strlen(config.mqttPublishTopic) == 0)
		return;

	// Build a json with the message in a "data" attribute
	StaticJsonDocument<500> json;
	json["id"] = WiFi.macAddress();
	json["up"] = millis();
	json["data"] = data;

	// Stringify the json
	String msg;
	serializeJson(json, msg);

	// Send the json over MQTT
	mqtt.publish(config.mqttPublishTopic, msg.c_str());

	if (debugger) debugger->print("sendMqttData: ");
	if (debugger) debugger->println(data);
}
