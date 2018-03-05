/*
 * Template sketch for a selv managed ESP8266 WiFi - MQTT setup.
 * 
 * The ESP will boot and try to connect to a WiFi network. 
 * If it's not able to connect, it will boot up as an Access Point (AP).
 * 
 * The user can then connect to the AP and browse to http://192.168.4.1
 * and fill inn SSID/password, MQTT IP and meter type.
 * 
 * The ESP will then store the information in EEPROM for later use.
 * It is possible to reset the ESP trough MQTT or pulling GPIO13 HIGH.
 * 
 * The sketch also supports Firmware OTA.
 * 
 * Created 01. march 2017 by Ruben Andreassen
 */
 
/**
 * EEPROM
 */
#include <EEPROM.h>

struct SettingsObject {
  char ssid[51];
  char password[51];
  char mqtt[16];
  int meter;
};
SettingsObject customSettings;

/**
 * WIFI
 */
#include <ESP8266WiFi.h>
WiFiClient espClient;

const char* ssid_ap = "ESP-AMS-MQTT";

/**
 * Access Point
 */
#include <ESP8266WebServer.h>
ESP8266WebServer server(80); //Server on port 80
bool apMode = false;

const char MAIN_page[] PROGMEM = R"=====(
<html>
  <head>
    <title>AMS - MQTT Bridge</title>
  </head>
  <body>
    <form method="post" action="/store">
      SSID <input type="text" name="ssid" /><br/>
      Password <input type="text" name="password" /><br/>
      MQTT IP <input type="text" name="mqtt" /><br/>
      Meter <select name="meter">
        <option value="0">Kamstrup</option>
        <option value="1">Kaifa</option>
      <!--  <option value="2">Aidon</option> -->
      </select><br/>
      <input type="submit" value="Submit" />
    </form>
  </body>
</html>
)=====";

/**
 * Firmware updater
 */
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

const int FW_VERSION = 1010;

/**
 * MQTT
 */
#include <PubSubClient.h>
PubSubClient client(espClient);

const char* mqtt_topic_fw_version = "esp/ams/fw/version";
const char* mqtt_topic_fw_update = "esp/ams/fw/update";
const char* mqtt_topic_fw_info = "esp/ams/fw/info";
const char* mqtt_topic_reset = "esp/ams/reset";

/**
 * AMS
 */

#include <HanReader.h>
#include <Kamstrup.h>
#include <Kaifa.h>

// The HAN Port reader
HanReader hanReader;

const char* mqtt_topic = "esp/ams";
const char* mqtt_topic_PackageTime = "esp/ams/packagetime";
const char* mqtt_topic_ListSize = "esp/ams/listsize";
const char* mqtt_topic_ListVersionIdentifier = "esp/ams/listversionidentifier";
const char* mqtt_topic_MeterID = "esp/ams/meterid";
const char* mqtt_topic_MeterType = "esp/ams/metertype";
const char* mqtt_topic_ActiveImportPower = "esp/ams/activeimportpower";
const char* mqtt_topic_ActiveExportPower = "esp/ams/activeExportpower";
const char* mqtt_topic_ReactiveImportPower = "esp/ams/reactiveimportpower";
const char* mqtt_topic_ReactiveExportPower = "esp/ams/reactiveexportpower";
const char* mqtt_topic_CurrentL1 = "esp/ams/currentl1";
const char* mqtt_topic_CurrentL2 = "esp/ams/currentl2";
const char* mqtt_topic_CurrentL3 = "esp/ams/currentl3";
const char* mqtt_topic_VoltageL1 = "esp/ams/voltagel1";
const char* mqtt_topic_VoltageL2 = "esp/ams/voltagel2";
const char* mqtt_topic_VoltageL3 = "esp/ams/voltagel3";
const char* mqtt_topic_MeterClock = "esp/ams/meterclock";
const char* mqtt_topic_CumulativeActiveImportEnergy = "esp/ams/cumulativeactiveimportenergy";
const char* mqtt_topic_CumulativeActiveExportEnergy = "esp/ams/cumulativeactiveexportenergy";
const char* mqtt_topic_CumulativeReactiveImportEnergy = "esp/ams/cumulativereactiveimportenergy";
const char* mqtt_topic_CumulativeReactiveExportEnergy = "esp/ams/cumulativereactiveexportenergy";

String last_PackageTime = "";
String last_ListVersionIdentifier = "";
String last_MeterID = "";
String last_MeterType = "";
int last_ActiveImportPower = 0;
int last_ActiveExportPower = 0;
int last_ReactiveImportPower = 0;
int last_ReactiveExportPower = 0;
int last_CurrentL1 = 0;
int last_CurrentL2 = 0;
int last_CurrentL3 = 0;
int last_VoltageL1 = 0;
int last_VoltageL2 = 0;
int last_VoltageL3 = 0;

/**
 * Setup and loop
 */

void setup() {
  Serial.begin(9600);
  pinMode(13, INPUT); //Reset PIN
  
  loadCredentials();
  if (!setupWiFi()) {
    setupAP();
  } else {
    setupMqtt();
    setupAms();
  }
}

void loop() {
  if (apMode) {
    server.handleClient(); //Handle client requests
  } else {
    if (digitalRead(13) == LOW) {
      clearCredentials();
    }
    
    loopMqtt();
    
    if (customSettings.meter == 0) {
      loopAmsKamstrup();
    } else if (customSettings.meter == 1) {
      loopAmsKaifa();
    } /*else if (customSettings.meter == 2) {
      Serial.println("Aidon selected");
      // Not yet supported
    }*/
  }
}

/**
 * EEPROM
 */

void clearCredentials() {
  EEPROM.begin(512);
  // write a 0 to all 512 bytes of the EEPROM
  for (int i = 0; i < 512; i++) {
    EEPROM.write(i, (char)0);
  }
  EEPROM.end();
  
  for( int i = 0; i < sizeof(customSettings.ssid);  ++i) {
    customSettings.ssid[i] = (char)0;
    customSettings.password[i] = (char)0;
  }
  for( int i = 0; i < sizeof(customSettings.mqtt);  ++i) {
    customSettings.mqtt[i] = (char)0;
  }
  customSettings.meter = -1;
  
  WiFi.disconnect(1);
}

/** Load WLAN credentials from EEPROM */
void loadCredentials() {
  EEPROM.begin(512);
  EEPROM.get(0, customSettings);

  customSettings.ssid[strlen(customSettings.ssid)] = '\0';
  customSettings.password[strlen(customSettings.password)] = '\0';

  EEPROM.end();
}

/** Store WLAN credentials to EEPROM */
void saveCredentials() {
  EEPROM.begin(512);
  EEPROM.put(0, customSettings);
  EEPROM.commit();
  EEPROM.end();
}

/**
 * WIFI
 */

bool setupWiFi()
{
  int retryCount = 0;
  delay(10);
  WiFi.mode(WIFI_STA);
  WiFi.begin((char*)customSettings.ssid, (char*)customSettings.password);

  while (WiFi.status() != WL_CONNECTED && retryCount < 20) {
    delay(500);
    Serial.println("Trying to connect to WiFi");
    Serial.println(WiFi.status());
    retryCount++;
  }

  Serial.println(WiFi.status());
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Could not connect");
    return false;
  } else {
    Serial.println("Connected!");
    return true;
  }
}

/**
 * Access Point
 */
void setupAP()
{
  WiFi.mode(WIFI_AP);           //Only Access point
  WiFi.softAP(ssid_ap);  //Start HOTspot removing password will disable security
 
  IPAddress myIP = WiFi.softAPIP(); //Get IP address
  Serial.print("HotSpt IP:");
  Serial.println(myIP);

  server.on("/", handleRoot);      //Which routine to handle at root location
  server.on("/store", handleSubmit);      //Which routine to handle at root location
  server.begin();                  //Start server

  apMode = true;
}

void handleRoot() {
  String s = MAIN_page;
  server.send(200, "text/html", s);
}

void handleSubmit() {
  clearCredentials();
  
  strncpy(customSettings.ssid, server.arg("ssid").c_str(), 51);
  strncpy(customSettings.password, server.arg("password").c_str(), 51);
  strncpy(customSettings.mqtt, server.arg("mqtt").c_str(), 16);
  //strncpy(customSettings.meter, server.arg("meter").c_str(), 1);
  customSettings.meter = server.arg("meter").toInt();
  
  saveCredentials();
  server.send(200, "text/plain", "Rebooting and connecting to your WiFi....");
  delay(2000);
  
  ESP.reset();
}

/**
 * Firmware updater
 */

void checkForUpdates(String fwURL) {
  String fwVersionURL = fwURL;
  fwVersionURL.concat( ".version" );

  client.publish(mqtt_topic_fw_info, ((String)"Checking for firmware updates.").c_str());
  client.publish(mqtt_topic_fw_info, fwVersionURL.c_str());

  HTTPClient httpClient;
  httpClient.begin( fwVersionURL );
  int httpCode = httpClient.GET();
  if( httpCode == 200 ) {
    String newFWVersion = httpClient.getString();

    client.publish(mqtt_topic_fw_info, ((String)"Current firmware version").c_str());
    client.publish(mqtt_topic_fw_info, ((String)FW_VERSION).c_str());
    client.publish(mqtt_topic_fw_info, ((String)"Available firmware version").c_str());
    client.publish(mqtt_topic_fw_info, newFWVersion.c_str());
    
    int newVersion = newFWVersion.toInt();

    if( newVersion > FW_VERSION ) {
      client.publish(mqtt_topic_fw_info, ((String)"Preparing to update").c_str());

      String fwImageURL = fwURL;
      fwImageURL.concat( ".bin" );

      client.publish(mqtt_topic_fw_info, ((String)"Downloading new firmware.").c_str());
      client.publish(mqtt_topic_fw_info, fwImageURL.c_str());
      
      t_httpUpdate_return ret = ESPhttpUpdate.update( fwImageURL );

      switch(ret) {
        case HTTP_UPDATE_FAILED:
          //Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          client.publish(mqtt_topic_fw_info, ((String)HTTP_UPDATE_FAILED).c_str());
          client.publish(mqtt_topic_fw_info, ((String)ESPhttpUpdate.getLastError()).c_str());
          client.publish(mqtt_topic_fw_info, ESPhttpUpdate.getLastErrorString().c_str());
          break;
        case HTTP_UPDATE_NO_UPDATES:
          client.publish(mqtt_topic_fw_info, ((String)HTTP_UPDATE_NO_UPDATES).c_str());
          break;
      }
    }
    else {
      client.publish(mqtt_topic_fw_info, ((String)"Already on latest version").c_str());
    }
  }
  else {
    client.publish(mqtt_topic_fw_info, ((String)"Firmware version check failed, got HTTP response code").c_str());
    client.publish(mqtt_topic_fw_info, ((String)httpCode).c_str());
  }
  httpClient.end();
}

/**
 * MQTT
 */

void setupMqtt()
{
  String input = (String)customSettings.mqtt;
  // Define number of pieces
  const int numberOfPieces = 4;
  String pieces[numberOfPieces];
  
  // Keep track of current position in array
  int counter = 0;
  
  // Keep track of the last comma so we know where to start the substring
  int lastIndex = 0;

  Serial.println(input);
  Serial.println(input.length());

  for (int i = 0; i < input.length(); i++) {
        // Loop through each character and check if it's a comma
        if (input.substring(i, i+1) == ".") {
          // Grab the piece from the last index up to the current position and store it
          pieces[counter] = input.substring(lastIndex, i);
          // Update the last position and add 1, so it starts from the next character
          lastIndex = i + 1;
          // Increase the position in the array that we store into
          counter++;
        }

        // If we're at the end of the string (no more commas to stop us)
        if (i == input.length() - 1) {
          // Grab the last part of the string from the lastIndex to the end
          pieces[counter] = input.substring(lastIndex, i+1);
        }
  }

  IPAddress server(pieces[0].toInt(), pieces[1].toInt(), pieces[2].toInt(), pieces[3].toInt());
  
  client.setServer(server, 1883);
  client.setCallback(callbackMqtt);
}

// Ensure the MQTT lirary gets some attention too
void loopMqtt()
{
  if (!client.connected()) {
    reconnectMqtt();
  }
  client.loop();
  delay(100);
}

void callbackMqtt(char* topic, byte* payload, unsigned int length) {
  /*for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }*/
  if (strcmp(topic, mqtt_topic_fw_update) == 0) {
    String fwURL = "";
    for (int i = 0; i < length; i++) {
      fwURL.concat( (char)payload[i] );
    }
  
    checkForUpdates(fwURL);
  } else if (strcmp(topic, mqtt_topic_reset) == 0) {
    clearCredentials();
    delay(1000);
    ESP.reset();
  }
}

void reconnectMqtt() {
  // Loop until we're reconnected
  while (!client.connected()) {
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);

    Serial.println("MQTT reconnect");
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      client.subscribe(mqtt_topic_reset);
      client.subscribe(mqtt_topic_fw_update);
      client.publish(mqtt_topic_fw_version, ((String)FW_VERSION).c_str());
    }
    else {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/**
 * AMS
 */

void setupAms() {
  // initialize the HanReader
  if (customSettings.meter == 0) {
    Serial.println("Kamstup selected");
    hanReader.setup(&Serial, 2400, SERIAL_8N1, NULL);
  }else if (customSettings.meter == 1) {
    Serial.println("Kaifa selected");
    hanReader.setup(&Serial);
  } /*else if (customSettings.meter == 2) {
    Serial.println("Aidon selected");
    // Not yet supported
  }*/  else {
    Serial.println("Unknown meter type");
  }
}

void loopAmsKamstrup() {
  // Read one byte from the port, and see if we got a full package
  if (hanReader.read())
  {
    // Get the list identifier
    int listSize = hanReader.getListSize();
    
    if (listSize == (int)Kamstrup::List1 || listSize == (int)Kamstrup::List2)
    {
      //Publish uptime to let the world know i'm alive
      client.publish(mqtt_topic, ((String)millis()).c_str());

      last_ListVersionIdentifier = publishNewValue(hanReader.getString((int)Kamstrup_List1::ListVersionIdentifier), last_ListVersionIdentifier, listSize, (int)Kamstrup::List2, mqtt_topic_ListVersionIdentifier);
      last_MeterID = publishNewValue(hanReader.getString((int)Kamstrup_List1::MeterID), last_MeterID, listSize, (int)Kamstrup::List2, mqtt_topic_MeterID);
      last_MeterType = publishNewValue(hanReader.getString((int)Kamstrup_List1::MeterType), last_MeterType, listSize, (int)Kamstrup::List2, mqtt_topic_MeterType);
      last_PackageTime = publishNewValue((String)hanReader.getPackageTime(), last_PackageTime, listSize, (int)Kamstrup::List2, mqtt_topic_PackageTime);
          
      last_ActiveImportPower = publishNewValue(hanReader.getInt((int)Kamstrup_List1::ActiveImportPower), last_ActiveImportPower, listSize, (int)Kamstrup::List2, mqtt_topic_ActiveImportPower); 
      last_ActiveExportPower = publishNewValue(hanReader.getInt((int)Kamstrup_List1::ActiveExportPower), last_ActiveExportPower, listSize, (int)Kamstrup::List2, mqtt_topic_ActiveExportPower); 
      last_ReactiveImportPower = publishNewValue(hanReader.getInt((int)Kamstrup_List1::ReactiveImportPower), last_ReactiveImportPower, listSize, (int)Kamstrup::List2, mqtt_topic_ReactiveImportPower); 
      last_ReactiveExportPower = publishNewValue(hanReader.getInt((int)Kamstrup_List1::ReactiveExportPower), last_ReactiveExportPower, listSize, (int)Kamstrup::List2, mqtt_topic_ReactiveExportPower); 
      
      last_CurrentL1 = publishNewValue(((float)hanReader.getInt((int)Kamstrup_List1::CurrentL1) / 100.0), last_CurrentL1, listSize, (int)Kamstrup::List2, mqtt_topic_CurrentL1);
      last_CurrentL2 = publishNewValue(((float)hanReader.getInt((int)Kamstrup_List1::CurrentL2) / 100.0), last_CurrentL2, listSize, (int)Kamstrup::List2, mqtt_topic_CurrentL2);
      last_CurrentL3 = publishNewValue(((float)hanReader.getInt((int)Kamstrup_List1::CurrentL3) / 100.0), last_CurrentL3, listSize, (int)Kamstrup::List2, mqtt_topic_CurrentL3);
      
      last_VoltageL1 = publishNewValue(hanReader.getInt((int)Kamstrup_List1::VoltageL1), last_VoltageL1, listSize, (int)Kamstrup::List2, mqtt_topic_VoltageL1);
      last_VoltageL2 = publishNewValue(hanReader.getInt((int)Kamstrup_List1::VoltageL2), last_VoltageL2, listSize, (int)Kamstrup::List2, mqtt_topic_VoltageL2);
      last_VoltageL3 = publishNewValue(hanReader.getInt((int)Kamstrup_List1::VoltageL3), last_VoltageL3, listSize, (int)Kamstrup::List2, mqtt_topic_VoltageL3);

      if (listSize == (int)Kamstrup::List2)
      {
        client.publish(mqtt_topic_MeterClock, ((String)hanReader.getTime((int)Kamstrup_List2::MeterClock)).c_str());
        client.publish(mqtt_topic_CumulativeActiveImportEnergy, ((String)hanReader.getInt((int)Kamstrup_List2::CumulativeActiveImportEnergy)).c_str());
        client.publish(mqtt_topic_CumulativeActiveExportEnergy, ((String)hanReader.getInt((int)Kamstrup_List2::CumulativeActiveExportEnergy)).c_str());
        client.publish(mqtt_topic_CumulativeReactiveImportEnergy, ((String)hanReader.getInt((int)Kamstrup_List2::CumulativeReactiveImportEnergy)).c_str());
        client.publish(mqtt_topic_CumulativeReactiveExportEnergy, ((String)hanReader.getInt((int)Kamstrup_List2::CumulativeReactiveExportEnergy)).c_str());
      }

    }
  }
}

void loopAmsKaifa() {
  //TODO
}

/*
 * Only publish new values or when a spesific list size occours
 */
int publishNewValue(int currentValue, int lastValue, int currentListSize, int publishListSize, const char* topic) {
  if (currentValue != lastValue || currentListSize == publishListSize) {
    client.publish(topic, ((String)currentValue).c_str());
  }
  return currentValue;
}

float publishNewValue(float currentValue, float lastValue, int currentListSize, int publishListSize, const char* topic) {
  if (currentValue != lastValue || currentListSize == publishListSize) {
    client.publish(topic, ((String)currentValue).c_str());
  }
  return currentValue;
}

String publishNewValue(String currentValue, String lastValue, int currentListSize, int publishListSize, const char* topic) {
  if (currentValue != lastValue || currentListSize == publishListSize) {
    client.publish(topic, currentValue.c_str());
  }
  return currentValue;
}

