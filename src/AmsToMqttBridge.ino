/*
 Name:		AmsToMqttBridge.ino
 Created:	3/13/2018 7:40:28 PM
 Author:	roarf
*/

ADC_MODE(ADC_VCC);    

#include <ArduinoJson.h>
#include <MQTT.h>

#if HAS_DALLAS_TEMP_SENSOR
#include <DallasTemperature.h>
#include <OneWire.h>
#endif

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

#include "AmsWebServer.h"
#include "HanConfigAp.h"
#include "HanReader.h"
#include "HanToJson.h"

#define WIFI_CONNECTION_TIMEOUT 30000;

#if IS_CUSTOM_AMS_BOARD
#define LED_PIN 2 // The blue on-board LED of the ESP8266 custom AMS board
#define LED_ACTIVE_HIGH 0
#define AP_BUTTON_PIN 0
#else
#define LED_PIN LED_BUILTIN
#define LED_ACTIVE_HIGH 1
#define AP_BUTTON_PIN INVALID_BUTTON_PIN
#endif

#if HAS_DALLAS_TEMP_SENSOR
#define TEMP_SENSOR_PIN 5 // Temperature sensor connected to GPIO5

OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature tempSensor(&oneWire);
#endif

// Object used to boot as Access Point
HanConfigAp ap;

AmsWebServer ws;

// WiFi client and MQTT client
WiFiClient *client;
MQTTClient mqtt(384);

// Object used for debugging
HardwareSerial* debugger = NULL;

// The HAN Port reader, used to read serial data and decode DLMS
HanReader hanReader;

// the setup function runs once when you press reset or power the board
void setup() {

#if DEBUG_MODE
	debugger = &Serial;
#endif

	if (debugger) {
		// Setup serial port for debugging
		debugger->begin(2400, SERIAL_8E1);
		//debugger->begin(115200);
		while (!debugger);
		debugger->println("");
		debugger->println("Started...");
		debugger->print("Voltage: ");
		debugger->print(ESP.getVcc());
		debugger->println("mV");
	}

	if (ESP.getVcc() < 3300) {
		if(debugger) {
			debugger->print("Voltage is too low: ");
			debugger->print(ESP.getVcc());
			debugger->println("mV");
		}
		ESP.deepSleep(5000000);    //Deep sleep for 5 seconds to allow output cap to charge up
	}  

	// Flash the LED, to indicate we can boot as AP now
	pinMode(LED_PIN, OUTPUT);
	led_on();

	delay(1000);

	// Initialize the AP
	ap.setup(AP_BUTTON_PIN, debugger);

	led_off();

	if (!ap.isActivated)
	{
		setupWiFi();
		// Configure uart for AMS data
		if(ap.config.meterType == 3) {
			Serial.begin(2400, SERIAL_8N1);
		} else {
			Serial.begin(2400, SERIAL_8E1);
		}
		while (!Serial);

		hanReader.setup(&Serial, debugger);

		// Compensate for the known Kaifa bug
		hanReader.compensateFor09HeaderBug = (ap.config.meterType == 1);
	}

	ws.setup(&ap.config, debugger);
}

// the loop function runs over and over again until power down or reset
void loop()
{
	// Only do normal stuff if we're not booted as AP
	if (!ap.loop())
	{
		// Turn off the LED
		led_off();

		// allow the MQTT client some resources
		mqtt.loop();
		delay(10); // <- fixes some issues with WiFi stability

		// Reconnect to WiFi and MQTT as needed
		if (!mqtt.connected()) {
			MQTT_connect();
		} else {
			readHanPort();
		}
	}
	else
	{
		// Continously flash the LED when AP mode
		if (millis() / 50 % 64 == 0)   led_on();
		else							led_off();

		// Make sure there is enough power to run
		delay(max(10, 3500-ESP.getVcc()));
	}
	ws.loop();
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
	WiFi.begin(ap.config.ssid, ap.config.ssidPassword);

	// Wait for WiFi connection
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
	if (hanReader.read() && ap.config.hasConfig())
	{
		// Flash LED on, this shows us that data is received
		led_on();

		// Get the timestamp (as unix time) from the package
		time_t time = hanReader.getPackageTime();
		if (debugger) debugger->print("Time of the package is: ");
		if (debugger) debugger->println(time);

		// Define a json object to keep the data
		StaticJsonDocument<500> json;

		// Any generic useful info here
		json["id"] = WiFi.macAddress();
		json["up"] = millis();
		json["t"] = time;
		json["vcc"] = ((double) ESP.getVcc()) / 1000;

		// Add a sub-structure to the json object,
		// to keep the data from the meter itself
		JsonObject data = json.createNestedObject("data");

#if HAS_DALLAS_TEMP_SENSOR
		// Get the temperature too
		tempSensor.requestTemperatures();
		data["temp"] = tempSensor.getTempCByIndex(0);
#endif

		hanToJson(data, ap.config.meterType, hanReader);

		if(ap.config.mqtt != 0 && strlen(ap.config.mqtt) != 0 && ap.config.mqttPublishTopic != 0 && strlen(ap.config.mqttPublishTopic) != 0) {
			// Write the json to the debug port
			if (debugger) {
				debugger->print("Sending data to MQTT: ");
				serializeJsonPretty(json, *debugger);
				debugger->println();
			}

			// Publish the json to the MQTT server
			String msg;
			serializeJson(json, msg);

			mqtt.publish(ap.config.mqttPublishTopic, msg.c_str());
			mqtt.loop();
		}
		ws.setJson(json);

		// Flash LED off
		led_off();
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
	json["vcc"] = ((double) ESP.getVcc()) / 1000;

	// Stringify the json
	String msg;
	serializeJson(json, msg);

	// Send the json over MQTT
	mqtt.publish(ap.config.mqttPublishTopic, msg.c_str());

	if (debugger) debugger->print("sendMqttData: ");
	if (debugger) debugger->println(data);
}
