/*
 Name:		AmsToMqttBridge.ino
 Created:	3/13/2018 7:40:28 PM
 Author:	roarf
*/


#include <ArduinoJson.h>
#include <DallasTemperature.h>
#include <MQTT.h>
#include <OneWire.h>
#include <ESP8266WiFi.h>
#include "HanConfigAp.h"
#include "HanReader.h"
#include "HanToJson.h"

#define WIFI_CONNECTION_TIMEOUT 30000;
#define TEMP_SENSOR_PIN 5 // Temperature sensor connected to GPIO5
#define LED_PIN 2 // The blue on-board LED of the ESP

OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature tempSensor(&oneWire);
long lastTempDebug = 0;

// Object used to boot as Access Point
HanConfigAp ap;

// WiFi client and MQTT client
WiFiClient *client;
MQTTClient mqtt(384);

// Object used for debugging
HardwareSerial* debugger = NULL;

// The HAN Port reader, used to read serial data and decode DLMS
HanReader hanReader;

// the setup function runs once when you press reset or power the board
void setup() 
{
	// Uncomment to debug over the same port as used for HAN communication
	debugger = &Serial;
	
	if (debugger) {
		// Setup serial port for debugging
		debugger->begin(2400, SERIAL_8E1);
		while (!&debugger);
		debugger->println("");
		debugger->println("Started...");
	}

	// Assign pin for boot as AP
	delay(1000);
	pinMode(0, INPUT_PULLUP);
	
	// Flash the blue LED, to indicate we can boot as AP now
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, LOW);
	
	// Initialize the AP
	ap.setup(0, debugger);
	
	// Turn off the blue LED
	digitalWrite(LED_PIN, HIGH);

	if (!ap.isActivated)
	{
		setupWiFi();
		Serial.begin(2400, SERIAL_8E1);
		while (!Serial);

		hanReader.setup(&Serial, debugger);

		// Compensate for the known Kaifa bug
		hanReader.compensateFor09HeaderBug = (ap.config.meterType == 1);
	}
}

// the loop function runs over and over again until power down or reset
void loop()
{
	// Only do normal stuff if we're not booted as AP
	if (!ap.loop())
	{
		// turn off the blue LED
		digitalWrite(LED_PIN, HIGH);

		// allow the MQTT client some resources
		mqtt.loop();
		delay(10); // <- fixes some issues with WiFi stability

		// Reconnect to WiFi and MQTT as needed
		if (!mqtt.connected()) {
			MQTT_connect();
		}
		else
		{
			// Read data from the HAN port
			readHanPort();
		}
	}
	else
	{
		// Continously flash the blue LED when AP mode
		if (millis() / 1000 % 2 == 0)
			digitalWrite(LED_PIN, LOW);
		else
			digitalWrite(LED_PIN, HIGH);
	}
}

void setupWiFi()
{
	// Turn off AP
	WiFi.enableAP(false);
	
	// Connect to WiFi
  WiFi.mode(WIFI_STA);
	WiFi.begin(ap.config.ssid, ap.config.ssidPassword);
	
	if (debugger) debugger->print("\nWaiting for WiFi to connect...");
	while (WiFi.status() != WL_CONNECTED) {
		if (debugger) debugger->print(".");
		delay(500);
	}
	if (debugger) debugger->println(" connected");
	
  client = new WiFiClient();
	mqtt.begin(ap.config.mqtt, *client);

	// Direct incoming MQTT messages
	if (ap.config.mqttSubscribeTopic != 0 && strlen(ap.config.mqttSubscribeTopic) > 0) {
	mqtt.subscribe(ap.config.mqttSubscribeTopic);
	mqtt.onMessage(mqttMessageReceived);
	}

	// Notify everyone we're here!
	sendMqttData("Connected!");
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
	if (hanReader.read())
	{
		// Flash LED on, this shows us that data is received
		digitalWrite(LED_PIN, LOW);

		// Get the timestamp (as unix time) from the package
		time_t time = hanReader.getPackageTime();
		if (debugger) debugger->print("Time of the package is: ");
		if (debugger) debugger->println(time);

		// Get the temperature too
		tempSensor.requestTemperatures();
		float temperature = tempSensor.getTempCByIndex(0);

		// Define a json object to keep the data
		StaticJsonDocument<500> json;

		// Any generic useful info here
		json["id"] = WiFi.macAddress(); // TODO: Fix?
		json["up"] = millis();
		json["t"] = time;

		// Add a sub-structure to the json object,
		// to keep the data from the meter itself
		JsonObject data = json.createNestedObject("data");
		data["temp"] = temperature;

		hanToJson(data, ap.config.meterType, hanReader);

		// Write the json to the debug port
		if (debugger) {
			debugger->print("Sending data to MQTT: ");
			serializeJsonPretty(json, *debugger);
			debugger->println();
		}

		// Make sure we have configured a publish topic
		if (ap.config.mqttPublishTopic == 0 || strlen(ap.config.mqttPublishTopic) == 0)
		{
			return;
		}

		// Publish the json to the MQTT server
		String msg;
		serializeJson(json, msg);

		mqtt.publish(ap.config.mqttPublishTopic, msg.c_str());
		mqtt.loop();

		// Flash LED off
		digitalWrite(LED_PIN, HIGH);
	}
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() 
{
	// Connect to WiFi access point.
	if (debugger)
	{
		debugger->println(); 
		debugger->println();
		debugger->print("Connecting to WiFi network ");
		debugger->println(ap.config.ssid);
	}

	if (WiFi.status() != WL_CONNECTED)
	{
		// Make one first attempt at connect, this seems to considerably speed up the first connection
		WiFi.disconnect();
		WiFi.begin(ap.config.ssid, ap.config.ssidPassword);
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
			WiFi.begin(ap.config.ssid, ap.config.ssidPassword);
			vTimeout = millis() + WIFI_CONNECTION_TIMEOUT;
		}
		yield();
	}

	if (debugger) {
		debugger->println();
		debugger->println("WiFi connected");
		debugger->println("IP address: ");
		debugger->println(WiFi.localIP());
		debugger->print("\nconnecting to MQTT: ");
		debugger->print(ap.config.mqtt);
		debugger->print(", port: ");
		debugger->print(ap.config.mqttPort);
		debugger->println();
	}

	// Wait for the MQTT connection to complete
	while (!mqtt.connected()) {
		
		// Connect to a unsecure or secure MQTT server
		if ((ap.config.mqttUser == 0 && mqtt.connect(ap.config.mqttClientID)) || 
			(ap.config.mqttUser != 0 && mqtt.connect(ap.config.mqttClientID, ap.config.mqttUser, ap.config.mqttPass)))
		{
			if (debugger) debugger->println("\nSuccessfully connected to MQTT!");

			// Subscribe to the chosen MQTT topic, if set in configuration
			if (ap.config.mqttSubscribeTopic != 0 && strlen(ap.config.mqttSubscribeTopic) > 0)
			{
				mqtt.subscribe(ap.config.mqttSubscribeTopic);
				if (debugger) debugger->printf("  Subscribing to [%s]\r\n", ap.config.mqttSubscribeTopic);
			}
		}
		else
		{
			if (debugger)
			{
				debugger->print(".");
				debugger->print("failed, ");
				debugger->println(" trying again in 5 seconds");
			}

			// Wait 2 seconds before retrying
			mqtt.disconnect();

			delay(2000);
		}

		// Allow some resources for the WiFi connection
		yield();
	delay(2000);
	}
}

// Send a simple string embedded in json over MQTT
void sendMqttData(String data)
{
	// Make sure we have configured a publish topic
	if (ap.config.mqttPublishTopic == 0 || strlen(ap.config.mqttPublishTopic) == 0)
		return;

	// Make sure we're connected
	if (!client->connected() || !mqtt.connected()) {
		MQTT_connect();
	}

	// Build a json with the message in a "data" attribute
	StaticJsonDocument<500> json;
	json["id"] = WiFi.macAddress();
	json["up"] = millis();
	json["data"] = data;

	// Stringify the json
	String msg;
	serializeJson(json, msg);

	// Send the json over MQTT
	mqtt.publish(ap.config.mqttPublishTopic, msg.c_str());

	if (debugger) debugger->print("sendMqttData: ");
	if (debugger) debugger->println(data);
}
