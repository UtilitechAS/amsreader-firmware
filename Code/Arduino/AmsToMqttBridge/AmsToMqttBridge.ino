/*
 Name:		AmsToMqttBridge.ino
 Created:	3/13/2018 7:40:28 PM
 Author:	roarf
*/


#include <DallasTemperature.h>
#include <OneWire.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "configuration.h"
#include "accesspoint.h"
#include <HanReader.h>
#include <Kaifa.h>
#include <Kamstrup.h>

#define WIFI_CONNECTION_TIMEOUT 30000;
#define TEMP_SENSOR_PIN 5 // Temperature sensor connected to GPIO5

OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature tempSensor(&oneWire);
long lastTempDebug = 0;

accesspoint ap;
WiFiClient *client;
PubSubClient mqtt;
HardwareSerial* debugger;

// The HAN Port reader
HanReader hanReader;

// the setup function runs once when you press reset or power the board
void setup() 
{
	debugger = &Serial;
	
	// Setup serial port for debugging
	debugger->begin(2400, SERIAL_8E1);
	while (!&debugger);
	debugger->println("Started...");
	
	// Assign pin for boot as AP
	delay(1000);
	pinMode(0, INPUT_PULLUP);
	
	// Flash the blue LED, to indicate we can boot as AP now
	pinMode(2, OUTPUT);
	digitalWrite(2, LOW);
	
	// Initialize the AP
	ap.setup(0, Serial);
	
	// Turn off the blue LED
	digitalWrite(2, HIGH);

	if (!ap.isActivated)
	{
		setupWiFi();
		hanReader.setup(&Serial, 2400, SERIAL_8E1, debugger);
		hanReader.compensateFor09HeaderBug = (ap.config.meterType == 1); // To compensate for the known Kaifa bug
	}
}

void setupWiFi()
{
	if (ap.config.isSecure())
		client = new WiFiClientSecure();
	else
		client = new WiFiClient();

	mqtt = PubSubClient(*client);

	WiFi.enableAP(false);
	WiFi.begin(ap.config.ssid, ap.config.ssidPassword);
	mqtt.setServer(ap.config.mqtt, ap.config.mqttPort);

	//std::function<void(char*, unsigned char*, unsigned int)> vCallback = MakeDelegate(this, xnsClientClass::mqttMessageReceived);
	mqtt.setCallback(mqttMessageReceived);
	MQTT_connect();

	sendMqttData("Connected!");
}

void mqttMessageReceived(char* topic, unsigned char* payload, unsigned int length)
{
	// make it a null-terminated string
	char message[1000];
	for (int i = 0; i < length; i++)
		message[i] = payload[i];
	message[length] = 0;

	debugger->println("Incoming MQTT message:");
	debugger->print("[");
	debugger->print(topic);
	debugger->print("] ");
	debugger->println(message);
}

// the loop function runs over and over again until power down or reset
void loop() 
{
	// Only do normal stupp if we're not booted as AP
	if (!ap.loop())
	{
		mqtt.loop();
		delay(10); // <- fixes some issues with WiFi stability

		if (!mqtt.connected()) {
			MQTT_connect();
		}
		else
		{
			debugTemperature();
			readHanPort();
		}
	}
}
void debugTemperature()
{
	if (lastTempDebug + 5000 < millis())
	{
		lastTempDebug = millis();
		tempSensor.requestTemperatures();
		float temperature = tempSensor.getTempCByIndex(0);
		debugger->print("Temperature is ");
		debugger->print(temperature);
		debugger->println(" degrees");
	}
}
void readHanPort()
{
	switch (ap.config.meterType)
	{
	case 1: // Kaifa
		readHanPort_Kaifa();
		break;
	case 2: // Kamstrup
		readHanPort_Kamstrup();
		break;
	case 3: // Aidon
		readHanPort_Aidon();
		break;
	default:
		debugger->print("Meter type ");
		debugger->print(ap.config.meterType, HEX);
		debugger->println(" is unknown");
		delay(10000);
		break;
	}
}

void readHanPort_Aidon()
{
	debugger->println("Meter type Aidon is not yet implemented");
	delay(10000);
}

void readHanPort_Kamstrup()
{
	if (hanReader.read())
	{
		// Get the list identifier
		int listSize = hanReader.getListSize();

		// Only care for the ACtive Power Imported, which is found in the first list
		if (listSize == (int)Kamstrup::List1 || listSize == (int)Kamstrup::List2)
		{
			if (listSize == (int)Kamstrup::List1)
			{
				String id = hanReader.getString((int)Kamstrup_List1::ListVersionIdentifier);
				if (debugger) debugger->println(id);
			}
			else if (listSize == (int)Kamstrup::List2)
			{
				String id = hanReader.getString((int)Kamstrup_List2::ListVersionIdentifier);
				if (debugger) debugger->println(id);
			}

			// Get the timestamp (as unix time) from the package
			time_t time = hanReader.getPackageTime();
			if (debugger) debugger->print("Time of the package is: ");
			if (debugger) debugger->println(time);

			// Define a json object to keep the data
			StaticJsonBuffer<500> jsonBuffer;
			JsonObject& root = jsonBuffer.createObject();

			// Any generic useful info here
			root["id"] = WiFi.macAddress();
			root["up"] = millis();
			root["t"] = time;

			// Add a sub-structure to the json object, 
			// to keep the data from the meter itself
			JsonObject& data = root.createNestedObject("data");

			// Get the temperature too
			tempSensor.requestTemperatures();
			float temperature = tempSensor.getTempCByIndex(0);
			data["temp"] = temperature;

			// Based on the list number, get all details 
			// according to OBIS specifications for the meter
			if (listSize == (int)Kamstrup::List1)
			{
				data["lv"] = hanReader.getString((int)Kamstrup_List1::ListVersionIdentifier);
				data["id"] = hanReader.getString((int)Kamstrup_List1::MeterID);
				data["type"] = hanReader.getString((int)Kamstrup_List1::MeterType);
				data["P"] = hanReader.getInt((int)Kamstrup_List1::ActiveImportPower);
				data["Q"] = hanReader.getInt((int)Kamstrup_List1::ReactiveImportPower);
				data["I1"] = hanReader.getInt((int)Kamstrup_List1::CurrentL1);
				data["I2"] = hanReader.getInt((int)Kamstrup_List1::CurrentL2);
				data["I3"] = hanReader.getInt((int)Kamstrup_List1::CurrentL3);
				data["U1"] = hanReader.getInt((int)Kamstrup_List1::VoltageL1);
				data["U2"] = hanReader.getInt((int)Kamstrup_List1::VoltageL2);
				data["U3"] = hanReader.getInt((int)Kamstrup_List1::VoltageL3);
			}
			else if (listSize == (int)Kamstrup::List2)
			{
				data["lv"] = hanReader.getString((int)Kamstrup_List2::ListVersionIdentifier);;
				data["id"] = hanReader.getString((int)Kamstrup_List2::MeterID);
				data["type"] = hanReader.getString((int)Kamstrup_List2::MeterType);
				data["P"] = hanReader.getInt((int)Kamstrup_List2::ActiveImportPower);
				data["Q"] = hanReader.getInt((int)Kamstrup_List2::ReactiveImportPower);
				data["I1"] = hanReader.getInt((int)Kamstrup_List2::CurrentL1);
				data["I2"] = hanReader.getInt((int)Kamstrup_List2::CurrentL2);
				data["I3"] = hanReader.getInt((int)Kamstrup_List2::CurrentL3);
				data["U1"] = hanReader.getInt((int)Kamstrup_List2::VoltageL1);
				data["U2"] = hanReader.getInt((int)Kamstrup_List2::VoltageL2);
				data["U3"] = hanReader.getInt((int)Kamstrup_List2::VoltageL3);
				data["tPI"] = hanReader.getInt((int)Kamstrup_List2::CumulativeActiveImportEnergy);
				data["tPO"] = hanReader.getInt((int)Kamstrup_List2::CumulativeActiveExportEnergy);
				data["tQI"] = hanReader.getInt((int)Kamstrup_List2::CumulativeReactiveImportEnergy);
				data["tQO"] = hanReader.getInt((int)Kamstrup_List2::CumulativeReactiveExportEnergy);
			}

			// Write the json to the debug port
			root.printTo(Serial1);
			Serial1.println();

			// Publish the json to the MQTT server
			char msg[1024];
			root.printTo(msg, 1024);
			mqtt.publish(ap.config.mqttPublishTopic, msg);
		}
	}
}


void readHanPort_Kaifa() {
	if (hanReader.read())
	{
		// Get the list identifier
		int listSize = hanReader.getListSize();

		// Only care for the ACtive Power Imported, which is found in the first list
		if (listSize == (int)Kaifa::List1 || listSize == (int)Kaifa::List2 || listSize == (int)Kaifa::List3)
		{
			if (listSize == (int)Kaifa::List1)
			{
				if (debugger) debugger->println(" (list #1 has no ID)");
			}
			else
			{
				String id = hanReader.getString((int)Kaifa_List2::ListVersionIdentifier);
				if (debugger) debugger->println(id);
			}

			// Get the timestamp (as unix time) from the package
			time_t time = hanReader.getPackageTime();
			if (debugger) debugger->print("Time of the package is: ");
			if (debugger) debugger->println(time);

			// Define a json object to keep the data
			StaticJsonBuffer<500> jsonBuffer;
			JsonObject& root = jsonBuffer.createObject();

			// Any generic useful info here
			root["id"] = WiFi.macAddress();
			root["up"] = millis();
			root["t"] = time;

			// Add a sub-structure to the json object, 
			// to keep the data from the meter itself
			JsonObject& data = root.createNestedObject("data");

			// Get the temperature too
			tempSensor.requestTemperatures();
			float temperature = tempSensor.getTempCByIndex(0);
			data["temp"] = temperature;

			// Based on the list number, get all details 
			// according to OBIS specifications for the meter
			if (listSize == (int)Kaifa::List1)
			{
				data["P"] = hanReader.getInt((int)Kaifa_List1::ActivePowerImported);
			}
			else if (listSize == (int)Kaifa::List2)
			{
				data["lv"] = hanReader.getString((int)Kaifa_List2::ListVersionIdentifier);
				data["id"] = hanReader.getString((int)Kaifa_List2::MeterID);
				data["type"] = hanReader.getString((int)Kaifa_List2::MeterType);
				data["P"] = hanReader.getInt((int)Kaifa_List2::ActiveImportPower);
				data["Q"] = hanReader.getInt((int)Kaifa_List2::ReactiveImportPower);
				data["I1"] = hanReader.getInt((int)Kaifa_List2::CurrentL1);
				data["I2"] = hanReader.getInt((int)Kaifa_List2::CurrentL2);
				data["I3"] = hanReader.getInt((int)Kaifa_List2::CurrentL3);
				data["U1"] = hanReader.getInt((int)Kaifa_List2::VoltageL1);
				data["U2"] = hanReader.getInt((int)Kaifa_List2::VoltageL2);
				data["U3"] = hanReader.getInt((int)Kaifa_List2::VoltageL3);
			}
			else if (listSize == (int)Kaifa::List3)
			{
				data["lv"] = hanReader.getString((int)Kaifa_List3::ListVersionIdentifier);;
				data["id"] = hanReader.getString((int)Kaifa_List3::MeterID);
				data["type"] = hanReader.getString((int)Kaifa_List3::MeterType);
				data["P"] = hanReader.getInt((int)Kaifa_List3::ActiveImportPower);
				data["Q"] = hanReader.getInt((int)Kaifa_List3::ReactiveImportPower);
				data["I1"] = hanReader.getInt((int)Kaifa_List3::CurrentL1);
				data["I2"] = hanReader.getInt((int)Kaifa_List3::CurrentL2);
				data["I3"] = hanReader.getInt((int)Kaifa_List3::CurrentL3);
				data["U1"] = hanReader.getInt((int)Kaifa_List3::VoltageL1);
				data["U2"] = hanReader.getInt((int)Kaifa_List3::VoltageL2);
				data["U3"] = hanReader.getInt((int)Kaifa_List3::VoltageL3);
				data["tPI"] = hanReader.getInt((int)Kaifa_List3::CumulativeActiveImportEnergy);
				data["tPO"] = hanReader.getInt((int)Kaifa_List3::CumulativeActiveExportEnergy);
				data["tQI"] = hanReader.getInt((int)Kaifa_List3::CumulativeReactiveImportEnergy);
				data["tQO"] = hanReader.getInt((int)Kaifa_List3::CumulativeReactiveExportEnergy);
			}

			// Write the json to the debug port
			root.printTo(Serial1);
			Serial1.println();

			// Publish the json to the MQTT server
			char msg[1024];
			root.printTo(msg, 1024);
			mqtt.publish("sensors/out/espdebugger", msg);
		}
	}
}




// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
	// Connect to WiFi access point.
	if (debugger) debugger->println(); if (debugger) debugger->println();
	if (debugger) debugger->print("Connecting to WiFi network ");
	if (debugger) debugger->println(ap.config.ssid);

	long vTimeout = millis() + WIFI_CONNECTION_TIMEOUT;
	while (WiFi.status() != WL_CONNECTED) {
		delay(50);
		if (debugger) debugger->print(".");
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
	while (!mqtt.connected()) {
		if ((ap.config.mqttUser == 0 && mqtt.connect(ap.config.mqttClientID)) || 
			(ap.config.mqttUser != 0 && mqtt.connect(ap.config.mqttClientID, ap.config.mqttUser, ap.config.mqttPass)))
		{
			if (debugger) debugger->println("\nSuccessfully connected to MQTT!");
			if (ap.config.mqttSubscribeTopic != 0 && strlen(ap.config.mqttSubscribeTopic) > 0)
			{
				mqtt.subscribe(ap.config.mqttSubscribeTopic);
				if (debugger) debugger->printf("  Subscribing to [%s]\r\n", ap.config.mqttSubscribeTopic);
			}
		}
		else
		{
			if (debugger) debugger->print(".");
			if (debugger) debugger->print("failed, mqtt.state() = ");
			if (debugger) debugger->print(mqtt.state());
			if (debugger) debugger->println(" trying again in 5 seconds");
			// Wait 2 seconds before retrying
			mqtt.disconnect();
			delay(2000);
		}
		yield();
	}
}

void sendMqttData(String data)
{
	if (ap.config.mqttPublishTopic == 0 || strlen(ap.config.mqttPublishTopic) == 0)
		return;

	if (!client->connected() || !mqtt.connected()) {
		MQTT_connect();
	}

	StaticJsonBuffer<500> jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();
	json["id"] = WiFi.macAddress();
	json["up"] = millis() / 1000;
	json["data"] = data;

	String msg;
	json.printTo(msg);

	mqtt.publish(ap.config.mqttPublishTopic, msg.c_str());
}