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

// WiFi and MQTT endpoints
const char* ssid     = "Roar_Etne";
const char* password = "**********";
const char* mqtt_server = "192.168.10.203";

WiFiClient espClient;
PubSubClient client(espClient);

#include "DlmsReader.h"
#include "Crc16.h"
#include "KaifaHan.h"

DlmsReader reader;
byte buffer[512];
int bytesRead;
KaifaHan kaifa;

void setup() {
  // Initialize the Serial1 port for debugging
  // (This port is fixed to Pin2 of the ESP8266)
  Serial1.begin(115200);
  while (!Serial1) {}
  Serial1.setDebugOutput(true);
  Serial1.println("Serial1");
  Serial1.println("Serial debugging port initialized");

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

  client.setServer(mqtt_server, 1883);
  
  // Initialize H/W serial port for MBus communication
  Serial.begin(2400, SERIAL_8E1);
  while (!Serial) {}
  Serial1.println("MBUS serial setup complete");

  bytesRead = 0;
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();  
  
  if (Serial.available())
  {
    byte newByte = Serial.read();
    //if (newByte < 0x10) Serial1.print("0");
    //Serial1.print(newByte, HEX);

    if (reader.Read(newByte))
    {
      bytesRead = reader.GetRawData(buffer, 0, 512);

      byte list = kaifa.GetListID(buffer, 0, bytesRead);
      Serial1.println("");
      Serial1.print("List #");
      Serial1.print(list, HEX);
      Serial1.print(": ");
      
      time_t time = kaifa.GetPackageTime(buffer, 0, bytesRead);

      StaticJsonBuffer<500> jsonBuffer;
      JsonObject& root = jsonBuffer.createObject();
      root["id"] = "espdebugger";
      root["up"] = millis();
      root["t"] = time;
      
      JsonObject& data = root.createNestedObject("data");
      
      if (list == (byte)List::List1)
      {
        data["P"] = kaifa.GetInt((int)List1_ObisObjects::ActivePowerImported, buffer, 0, bytesRead);
      }
      else if (list == (byte)List::List2)
      {
        data["lv"] = kaifa.GetString((int)List2_ObisObjects::ObisListVersionIdentifier, buffer, 0, bytesRead);
        data["id"] = kaifa.GetString((int)List2_ObisObjects::MeterID, buffer, 0, bytesRead);
        data["type"] = kaifa.GetString((int)List2_ObisObjects::MeterType, buffer, 0, bytesRead);
        data["P"] = kaifa.GetInt((int)List2_ObisObjects::ActivePowerImported, buffer, 0, bytesRead);
        data["Q"] = kaifa.GetInt((int)List2_ObisObjects::ReactivePowerImported, buffer, 0, bytesRead);
        data["I1"] = kaifa.GetInt((int)List2_ObisObjects::CurrentPhaseL1, buffer, 0, bytesRead);
        data["I2"] = kaifa.GetInt((int)List2_ObisObjects::CurrentPhaseL2, buffer, 0, bytesRead);
        data["I3"] = kaifa.GetInt((int)List2_ObisObjects::CurrentPhaseL3, buffer, 0, bytesRead);
        data["U1"] = kaifa.GetInt((int)List2_ObisObjects::VoltagePhaseL1, buffer, 0, bytesRead);
        data["U2"] = kaifa.GetInt((int)List2_ObisObjects::VoltagePhaseL1, buffer, 0, bytesRead);
        data["U3"] = kaifa.GetInt((int)List2_ObisObjects::VoltagePhaseL1, buffer, 0, bytesRead);
      }
      else if (list == (byte)List::List3)
      {
        data["lv"] = kaifa.GetString((int)List3_ObisObjects::ObisListVersionIdentifier, buffer, 0, bytesRead);
        data["id"] = kaifa.GetString((int)List3_ObisObjects::MeterID, buffer, 0, bytesRead);
        data["type"] = kaifa.GetString((int)List3_ObisObjects::MeterType, buffer, 0, bytesRead);
        data["P"] = kaifa.GetInt((int)List3_ObisObjects::ActivePowerImported, buffer, 0, bytesRead);
        data["Q"] = kaifa.GetInt((int)List3_ObisObjects::ReactivePowerImported, buffer, 0, bytesRead);
        data["I1"] = kaifa.GetInt((int)List3_ObisObjects::CurrentPhaseL1, buffer, 0, bytesRead);
        data["I2"] = kaifa.GetInt((int)List3_ObisObjects::CurrentPhaseL2, buffer, 0, bytesRead);
        data["I3"] = kaifa.GetInt((int)List3_ObisObjects::CurrentPhaseL3, buffer, 0, bytesRead);
        data["U1"] = kaifa.GetInt((int)List3_ObisObjects::VoltagePhaseL1, buffer, 0, bytesRead);
        data["U2"] = kaifa.GetInt((int)List3_ObisObjects::VoltagePhaseL1, buffer, 0, bytesRead);
        data["U3"] = kaifa.GetInt((int)List3_ObisObjects::VoltagePhaseL1, buffer, 0, bytesRead);
        data["tPI"] = kaifa.GetInt((int)List3_ObisObjects::TotalActiveEnergyImported, buffer, 0, bytesRead);
        data["tPO"] = kaifa.GetInt((int)List3_ObisObjects::TotalActiveEnergyExported, buffer, 0, bytesRead);
        data["tQI"] = kaifa.GetInt((int)List3_ObisObjects::TotalReactiveEnergyImported, buffer, 0, bytesRead);
        data["tQO"] = kaifa.GetInt((int)List3_ObisObjects::TotalReactiveEnergyExported, buffer, 0, bytesRead);
      }
      else
      {
        Serial1.println("Invalid list");
        return;
      }

      root.printTo(Serial1);
      Serial1.println();

      char msg[1024];
      root.printTo(msg, 1024);
      client.publish("sensors/out/espdebugger", msg);
    }
  }
}


void reconnect() {
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
    } else {
      Serial1.print("failed, rc=");
      Serial1.print(client.state());
      Serial1.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
