/*
* Simple sketch to simulate reading data from a Kamstrup
* AMS Meter.
*
* Created 24. October 2017 by Roar Fredriksen
* Modified 06. November 2017 by Ruben Andreassen
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include <HanReader.h>
#include <Kamstrup.h>

// The HAN Port reader
HanReader hanReader;

// WiFi and MQTT endpoints
const char* ssid     = "ssid";
const char* password = "password";
const char* mqtt_server = "ip or dns";
const char* mqtt_topic = "sensors/out/espams";
const char* device_name = "espams";

bool enableDebug = false;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
	//setupDebugPort(); //Comment out this line if you dont need debugging on Serial1
  setupWiFi();
  setupMqtt();

	// initialize the HanReader
	// (passing no han port, as we are feeding data manually, but provide Serial for debugging)
  if (enableDebug) {
	  hanReader.setup(&Serial, 2400, SERIAL_8N1, &Serial1);  
  } else {
    hanReader.setup(&Serial, 2400, SERIAL_8N1, NULL);  
  }
}

void setupMqtt()
{
  client.setServer(mqtt_server, 1883);
}

void setupDebugPort()
{
  enableDebug = true;
	// Initialize the Serial port for debugging
	Serial1.begin(115200);
	while (!Serial1) {}
	Serial1.setDebugOutput(true);
	Serial1.println("Serial1");
	Serial1.println("Serial debugging port initialized");
}


void setupWiFi()
{
  // Initialize wifi
  if (enableDebug) {
    Serial1.print("Connecting to ");
    Serial1.println(ssid);
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (enableDebug) Serial1.print(".");
  }

  if (enableDebug) {
    Serial1.println("");
    Serial1.println("WiFi connected");
    Serial1.println("IP address: ");
    Serial1.println(WiFi.localIP());
  }
}

void loop() {
  loopMqtt();

	// Read one byte from the port, and see if we got a full package
	if (hanReader.read())
	{
		// Get the list identifier
		int listSize = hanReader.getListSize();

    if (enableDebug) {
  		Serial1.println("");
  		Serial1.print("List size: ");
  		Serial1.print(listSize);
  		Serial1.print(": ");
    }
    
		// Only care for the ACtive Power Imported, which is found in the first list
		if (listSize == (int)Kamstrup::List1 || listSize == (int)Kamstrup::List2)
		{
      // Define a json object to keep the data
      StaticJsonBuffer<MQTT_MAX_PACKET_SIZE> jsonBuffer;
      JsonObject& root = jsonBuffer.createObject();
      
      // Any generic useful info here
      root["dn"] = device_name;
      root["up"] = millis();
      
      // Add a sub-structure to the json object, 
      // to keep the data from the meter itself
      JsonObject& data = root.createNestedObject("data");
      
      data["ls"] = listSize;
      
			data["lvi"] = hanReader.getString((int)Kamstrup_List1::ListVersionIdentifier);
      data["mid"] = hanReader.getString((int)Kamstrup_List1::MeterID);
      data["mt"] = hanReader.getString((int)Kamstrup_List1::MeterType);
			data["t"] = hanReader.getPackageTime();
      
			data["aip"] = hanReader.getInt((int)Kamstrup_List1::ActiveImportPower); //power
      data["aep"] = hanReader.getInt((int)Kamstrup_List1::ActiveExportPower);
      data["rip"] = hanReader.getInt((int)Kamstrup_List1::ReactiveImportPower);
      data["rep"] = hanReader.getInt((int)Kamstrup_List1::ReactiveExportPower);
			
			data["al1"] = (float)hanReader.getInt((int)Kamstrup_List1::CurrentL1) / 100.0;
			data["al2"] = (float)hanReader.getInt((int)Kamstrup_List1::CurrentL2) / 100.0;
			data["al3"] = (float)hanReader.getInt((int)Kamstrup_List1::CurrentL3) / 100.0;

			data["vl1"] = hanReader.getInt((int)Kamstrup_List1::VoltageL1);
			data["vl2"] = hanReader.getInt((int)Kamstrup_List1::VoltageL2);
			data["vl3"] = hanReader.getInt((int)Kamstrup_List1::VoltageL3);

			if (listSize == (int)Kamstrup::List2)
			{
				data["cl"] = hanReader.getTime((int)Kamstrup_List2::MeterClock);
        data["caie"] = hanReader.getInt((int)Kamstrup_List2::CumulativeActiveImportEnergy);
        data["caee"] = hanReader.getInt((int)Kamstrup_List2::CumulativeActiveExportEnergy);
        data["crie"] = hanReader.getInt((int)Kamstrup_List2::CumulativeReactiveImportEnergy);
        data["cree"] = hanReader.getInt((int)Kamstrup_List2::CumulativeReactiveExportEnergy);
			}

      if (enableDebug) {
        root.printTo(Serial1);
        Serial1.println("JSON length");
        Serial1.println(root.measureLength());
        Serial1.println("");
      }
      
      // Publish the json to the MQTT server
      char msg[MQTT_MAX_PACKET_SIZE];
      root.printTo(msg, MQTT_MAX_PACKET_SIZE);
      bool result = client.publish(mqtt_topic, msg);
      
      if (enableDebug) {
        Serial1.println("MQTT publish result:");
        Serial1.println(result);
      }
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
    if (enableDebug) Serial1.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      if (enableDebug) Serial1.println("connected");
      // Once connected, publish an announcement...
      // client.publish("sensors", "hello world");
      // ... and resubscribe
      // client.subscribe("inTopic");
    }
    else {
      if (enableDebug) {
        Serial1.print("failed, rc=");
        Serial1.print(client.state());
        Serial1.println(" try again in 5 seconds");
      }
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
