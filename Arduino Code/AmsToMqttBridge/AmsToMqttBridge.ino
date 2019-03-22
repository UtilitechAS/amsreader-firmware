/*
 Name:		AmsToMqttBridge.ino
 Created:	3/13/2018 7:40:28 PM
 Author:	roarf
*/

#include <ESP8266WiFi.h>
#include <RemoteDebug.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <ArduinoJson.h>
#include <MQTT.h>
#include <HanReader.h>
#include <Aidon.h>
#include <Kaifa.h>
#include <Kamstrup.h>
#include "configuration.h"
#include "accesspoint.h"

#define WIFI_CONNECTION_TIMEOUT 30000;
#define TEMP_SENSOR_PIN 5 // Temperature sensor connected to GPIO5
#define LED_PIN 2 // The blue on-board LED of the ESP

OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature tempSensor(&oneWire);
long lastTempDebug = 0;

// Object used to boot as Access Point
accesspoint ap;

// WiFi client and MQTT client
WiFiClient *client;
MQTTClient mqtt(256);

// Object used for debugging
boolean debugEnabled = false;
RemoteDebug Debug;

// The HAN Port reader, used to read serial data and decode DLMS
HanReader hanReader;

// the setup function runs once when you press reset or power the board
void setup() 
{
	Serial.begin(2400, SERIAL_8E1);
	// Uncomment to debug over the same port as used for HAN communication
	if(debugEnabled) {
		Debug.begin("AMS2MQTT", Debug.WARNING);
		Debug.setSerialEnabled(true);
	}
	
	rdebugI("Started...");

	// Assign pin for boot as AP
	delay(1000);
	pinMode(0, INPUT_PULLUP);
	
	// Flash the blue LED, to indicate we can boot as AP now
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, LOW);
	
	// Initialize the AP
	ap.setup(0, Serial);
	
	// Turn off the blue LED
	digitalWrite(LED_PIN, HIGH);

	if (!ap.isActivated)
	{
		setupWiFi();
		hanReader.setup(&Serial, 2400, SERIAL_8E1, 0);
		
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

  Debug.handle();
}

void setupWiFi()
{
	// Turn off AP
	WiFi.enableAP(false);
	
	// Connect to WiFi
	WiFi.mode(WIFI_STA);
	WiFi.begin(ap.config.ssid, ap.config.ssidPassword);
	
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
	}
	rdebugI("Connected to WiFi");
	
	client = new WiFiClient();
	mqtt.begin(ap.config.mqtt, ap.config.mqttPort, *client);

	// Direct incoming MQTT messages
	if (ap.config.mqttSubscribeTopic != 0 && strlen(ap.config.mqttSubscribeTopic) > 0) {
		mqtt.subscribe(ap.config.mqttSubscribeTopic);
		mqtt.onMessage(mqttMessageReceived);
	}

	// Notify everyone we're here!
	sendMqttData("Connected!");
	rdebugI("Connected to MQTT");
}

void mqttMessageReceived(String &topic, String &payload)
{
	rdebugI("Incoming MQTT message: [%s] \n", &topic);
	rdebugI("%s\n", &payload); 

	// Do whatever needed here...
	// Ideas could be to query for values or to initiate OTA firmware update
}

void readHanPort()
{
	if (hanReader.read())
	{
		// Flash LED on, this shows us that data is received
		digitalWrite(LED_PIN, LOW);

		// Get the list identifier
		int listSize = hanReader.getListSize();
		rdebugD("Listsize: %d\n", listSize);

		switch (ap.config.meterType)
		{
		case 1: // Kaifa
			readHanPort_Kaifa(listSize);
			break;
		case 2: // Aidon
			readHanPort_Aidon(listSize);
			break;
		case 3: // Kamstrup
			readHanPort_Kamstrup(listSize);
			break;
		default:
			rdebugW("Meter type %X is unknown", ap.config.meterType);
			delay(1000);
			break;
		}

		// Flash LED off
		digitalWrite(LED_PIN, HIGH);
	}
}

void readHanPort_Aidon(int listSize)
{
  if (listSize == (int)Aidon::List1 || listSize == (int)Aidon::List2)
  {
    // Get the timestamp (as unix time) from the package
    time_t time = hanReader.getPackageTime();

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
    if (listSize == (int)Aidon::List1)
    {
      data["P"] = hanReader.getInt((int)Aidon_List1::ActiveImportPower);
    }
    else if (listSize == (int)Aidon::List2)
    {
      data["lv"] = hanReader.getString((int)Aidon_List2::ListVersionIdentifier);
      data["id"] = hanReader.getString((int)Aidon_List2::MeterID);
      data["type"] = hanReader.getString((int)Aidon_List2::MeterType);
      data["P"] = hanReader.getInt((int)Aidon_List2::ActiveImportPower);
      data["Q"] = hanReader.getInt((int)Aidon_List2::ReactiveExportPower);
      data["I1"] = ((double) hanReader.getInt((int)Aidon_List2::CurrentL1)) / 10;
      data["I2"] = ((double) hanReader.getInt((int)Aidon_List2::CurrentL2)) / 10;
      data["I3"] = ((double) hanReader.getInt((int)Aidon_List2::CurrentL3)) / 10;
      data["U1"] = ((double) hanReader.getInt((int)Aidon_List2::VoltageL1)) / 10;
      data["U2"] = ((double) hanReader.getInt((int)Aidon_List2::VoltageL2)) / 10; 
      data["U3"] = ((double) hanReader.getInt((int)Aidon_List2::VoltageL3)) / 10;
    }

    // Make sure we have configured a publish topic
    if (ap.config.mqttPublishTopic == 0 || strlen(ap.config.mqttPublishTopic) == 0)
      return;

    // Publish the json to the MQTT server
    char msg[1024];
    root.printTo(msg, 1024);
    rdebugI("Sending data to MQTT");
    rdebugD("Payload: %s", msg);
    mqtt.publish(ap.config.mqttPublishTopic, msg);
  } else {
    debugPrintData(hanReader.getBuffer(), 0, hanReader.getBytesRead());
  }
}

void readHanPort_Kamstrup(int listSize)
{
	// Only care for the ACtive Power Imported, which is found in the first list
	if (listSize == (int)Kamstrup::List1 || listSize == (int)Kamstrup::List2)
	{
		// Get the timestamp (as unix time) from the package
		time_t time = hanReader.getPackageTime();

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

		// Make sure we have configured a publish topic
		if (ap.config.mqttPublishTopic == 0 || strlen(ap.config.mqttPublishTopic) == 0)
			return;

		// Publish the json to the MQTT server
		char msg[1024];
		root.printTo(msg, 1024);
		rdebugI("Sending data to MQTT");
		rdebugD("Payload: %s", msg);
		mqtt.publish(ap.config.mqttPublishTopic, msg);
  } else {
		debugPrintData(hanReader.getBuffer(), 0, hanReader.getBytesRead());
	}
}


void readHanPort_Kaifa(int listSize) 
{
	// Only care for the ACtive Power Imported, which is found in the first list
	if (listSize == (int)Kaifa::List1 || listSize == (int)Kaifa::List2 || listSize == (int)Kaifa::List3)
	{
		// Get the timestamp (as unix time) from the package
		time_t time = hanReader.getPackageTime();

		// Define a json object to keep the data
		//StaticJsonBuffer<500> jsonBuffer;
		DynamicJsonBuffer jsonBuffer;
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
		data["temp"] = String(temperature);

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
			data["U1"] = ((double) hanReader.getInt((int)Kaifa_List2::VoltageL1)) / 10;
			data["U2"] = ((double) hanReader.getInt((int)Kaifa_List2::VoltageL2)) / 10;
			data["U3"] = ((double) hanReader.getInt((int)Kaifa_List2::VoltageL3)) / 10;
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
			data["U1"] = ((double) hanReader.getInt((int)Kaifa_List3::VoltageL1)) / 10;
			data["U2"] = ((double) hanReader.getInt((int)Kaifa_List3::VoltageL2)) / 10;
			data["U3"] = ((double) hanReader.getInt((int)Kaifa_List3::VoltageL3)) / 10;
			data["tPI"] = hanReader.getInt((int)Kaifa_List3::CumulativeActiveImportEnergy);
			data["tPO"] = hanReader.getInt((int)Kaifa_List3::CumulativeActiveExportEnergy);
			data["tQI"] = hanReader.getInt((int)Kaifa_List3::CumulativeReactiveImportEnergy);
			data["tQO"] = hanReader.getInt((int)Kaifa_List3::CumulativeReactiveExportEnergy);
		}

		// Make sure we have configured a publish topic
		if (ap.config.mqttPublishTopic == 0 || strlen(ap.config.mqttPublishTopic) == 0)
			return;

		// Publish the json to the MQTT server
		char msg[1024];
		root.printTo(msg, 1024);
		rdebugI("Sending data to MQTT");
		rdebugD("Payload: %s", msg);
		mqtt.publish(ap.config.mqttPublishTopic, msg);
	} else {
		debugPrintData(hanReader.getBuffer(), 0, hanReader.getBytesRead());
	}
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() 
{
	// Connect to WiFi access point.

	if (WiFi.status() != WL_CONNECTED)
	{
		rdebugI("Connecting to WiFi network %s", ap.config.ssid);
		// Make one first attempt at connect, this seems to considerably speed up the first connection
		WiFi.disconnect();
		WiFi.begin(ap.config.ssid, ap.config.ssidPassword);
		delay(1000);
	}

	// Wait for the WiFi connection to complete
	long vTimeout = millis() + WIFI_CONNECTION_TIMEOUT;
	while (WiFi.status() != WL_CONNECTED) {
		delay(50);
		
		// If we timed out, disconnect and try again
		if (vTimeout < millis())
		{
			rdebugW("Timeout during connect. WiFi status is: %d", WiFi.status());
			WiFi.disconnect();
			WiFi.begin(ap.config.ssid, ap.config.ssidPassword);
			vTimeout = millis() + WIFI_CONNECTION_TIMEOUT;
		}
		yield();
	}
	rdebugI("Wifi connected, IP: %s\n", WiFi.localIP().toString().c_str());
	rdebugI("Connecting to MQTT server: %s, port: %d\n", ap.config.mqtt, ap.config.mqttPort);

	// Wait for the MQTT connection to complete
	while (!mqtt.connected()) {
		
		// Connect to a unsecure or secure MQTT server
		if ((ap.config.mqttUser == 0 && mqtt.connect(ap.config.mqttClientID)) || 
			(ap.config.mqttUser != 0 && mqtt.connect(ap.config.mqttClientID, ap.config.mqttUser, ap.config.mqttPass)))
		{
			rdebugI("MQTT connected\n");

			// Subscribe to the chosen MQTT topic, if set in configuration
			if (ap.config.mqttSubscribeTopic != 0 && strlen(ap.config.mqttSubscribeTopic) > 0)
			{
				mqtt.subscribe(ap.config.mqttSubscribeTopic);
				mqtt.onMessage(mqttMessageReceived);
			}
		}
		else
		{
			rdebugE("MQTT connection failed, trying again in 5 seconds");

			// Wait 2 seconds before retrying
			mqtt.disconnect();

			delay(2000);
		}

		// Allow some resources for the WiFi connection
		yield();
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
	DynamicJsonBuffer jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();
	json["id"] = WiFi.macAddress();
	json["up"] = millis();
	json["data"] = data;

	// Stringify the json
	String msg;
	json.printTo(msg);

	// Send the json over MQTT
	mqtt.publish(ap.config.mqttPublishTopic, msg.c_str());
}

void debugPrintData(byte *buffer, int start, int length)
{
  for (int i = start; i < start + length; i++)
  {
    rdebugD("%01X ", buffer[i]);
    if ((i - start + 1) % 16 == 0)
      rdebugD("\n");
    else if ((i - start + 1) % 4 == 0)
      rdebugD(" ");
    
    yield(); // Let other get some resources too
  }
  rdebugD("\n");
}
