/*
 * Simple sketch to read MBus data from electrical meter
 * As the protocol requires "Even" parity, and this is
 * only supported on the hardware port of the ESP8266,
 * we'll have to use Serial1 for debugging.
 * 
 * This means you'll have to program the ESP using the 
 * regular RX/TX port, and then you must remove the FTDI
 * and connect the MBus signal from the meter to the
 * RS pin. The FTDI/RX can be moved to Pin2 for debugging
 * 
 * Created 14. september 2017 by Roar Fredriksen
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "HanReader.h"
#include "Kaifa.h"

// The HAN Port reader
HanReader hanReader;

// WiFi and MQTT endpoints
const char* ssid     = "Roar_Etne";
const char* password = "**********";
const char* mqtt_server = "192.168.10.203";

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
	setupDebugPort();
	setupWiFi();
	setupMqtt();
  
	// initialize the HanReader
	// (passing Serial as the HAN port and Serial1 for debugging)
	hanReader.setup(&Serial, &Serial1);
}

void setupMqtt()
{
	client.setServer(mqtt_server, 1883);
}

void setupDebugPort()
{
	// Initialize the Serial1 port for debugging
	// (This port is fixed to Pin2 of the ESP8266)
	Serial1.begin(115200);
	while (!Serial1) {}
	Serial1.setDebugOutput(true);
	Serial1.println("Serial1");
	Serial1.println("Serial debugging port initialized");
}

void setupWiFi()
{
	// Initialize wifi
	Serial1.print("Connecting to ");
	Serial1.println(ssid);
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial1.print(".");
	}

	Serial1.println("");
	Serial1.println("WiFi connected");
	Serial1.println("IP address: ");
	Serial1.println(WiFi.localIP());
}

void loop() {
	loopMqtt();

	// Read one byt from the port, and see if we got a full package
	if (hanReader.read())
	{
    // Get the list identifier
    int listSize = hanReader.getListSize();

    Serial1.println("");
    Serial1.print("List size: ");
    Serial1.print(listSize);
    Serial1.print(": ");

    // Only care for the ACtive Power Imported, which is found in the first list
    if (listSize == (int)Kaifa::List1 || listSize == (int)Kaifa::List2 || listSize == (int)Kaifa::List3)
    {
      if (listSize == (int)Kaifa::List1)
      {
        Serial1.println(" (list #1 has no ID)");
      }
      else
      {
        String id = hanReader.getString((int)Kaifa_List2::ListVersionIdentifier);
        Serial1.println(id);
      }

  		// Get the timestamp (as unix time) from the package
  		time_t time = hanReader.getPackageTime();
      Serial.print("Time of the package is: ");
      Serial.println(time);
  
  		// Define a json object to keep the data
  		StaticJsonBuffer<500> jsonBuffer;
  		JsonObject& root = jsonBuffer.createObject();
  		
  		// Any generic useful info here
  		root["id"] = "espdebugger";
  		root["up"] = millis();
  		root["t"] = time;
  
  		// Add a sub-structure to the json object, 
  		// to keep the data from the meter itself
  		JsonObject& data = root.createNestedObject("data");
  		
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
  		client.publish("sensors/out/espdebugger", msg);
  	}
	}
}

// Ensure the MQTT lirary gets some attention too
void loopMqtt()
{
	if (!client.connected()) {
		reconnectMqtt();
	}
	client.loop();
}

void reconnectMqtt() {
	// Loop until we're reconnected
	while (!client.connected()) {
		Serial1.print("Attempting MQTT connection...");
		// Attempt to connect
		if (client.connect("ESP8266Client")) {
			Serial1.println("connected");
			// Once connected, publish an announcement...
			// client.publish("sensors", "hello world");
			// ... and resubscribe
			// client.subscribe("inTopic");
		}
		else {
			Serial1.print("failed, rc=");
			Serial1.print(client.state());
			Serial1.println(" try again in 5 seconds");
			// Wait 5 seconds before retrying
			delay(5000);
		}
	}
}
