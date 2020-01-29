# 1 "/tmp/tmpfprbzre1"
#include <Arduino.h>
# 1 "/home/gunnar/src/AmsToMqttBridge/src/AmsToMqttBridge.ino"
# 11 "/home/gunnar/src/AmsToMqttBridge/src/AmsToMqttBridge.ino"
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
#define LED_PIN 2
#define LED_ACTIVE_HIGH 0
#define AP_BUTTON_PIN 0
#else
#define LED_PIN LED_BUILTIN
#define LED_ACTIVE_HIGH 1
#define AP_BUTTON_PIN INVALID_BUTTON_PIN
#endif

#if HAS_DALLAS_TEMP_SENSOR
#define TEMP_SENSOR_PIN 5

OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature tempSensor(&oneWire);
#endif


HanConfigAp ap;

AmsWebServer ws;


WiFiClient *client;
MQTTClient mqtt(384);


HardwareSerial* debugger = NULL;


HanReader hanReader;
void setup();
void loop();
void led_on();
void led_off();
void setupWiFi();
void mqttMessageReceived(String &topic, String &payload);
void readHanPort();
void MQTT_connect();
void sendMqttData(String data);
#line 65 "/home/gunnar/src/AmsToMqttBridge/src/AmsToMqttBridge.ino"
void setup() {

#if DEBUG_MODE
 debugger = &Serial;
#endif

 if (debugger) {

  debugger->begin(2400, SERIAL_8E1);

  while (!debugger);
  debugger->println("");
  debugger->println("Started...");
 }


 pinMode(LED_PIN, OUTPUT);
 led_on();

 delay(1000);


 ap.setup(AP_BUTTON_PIN, debugger);

 led_off();

 if (!ap.isActivated)
 {
  setupWiFi();

  if(ap.config.meterType == 3) {
   Serial.begin(2400, SERIAL_8N1);
  } else {
   Serial.begin(2400, SERIAL_8E1);
  }
  while (!Serial);

  hanReader.setup(&Serial, debugger);


  hanReader.compensateFor09HeaderBug = (ap.config.meterType == 1);
 }

 ws.setup(&ap.config, debugger);
}


void loop()
{

 if (!ap.loop())
 {

  led_off();


  mqtt.loop();
  delay(10);


  if (!mqtt.connected()) {
   MQTT_connect();
  } else {
   readHanPort();
  }
 }
 else
 {

  if (millis() / 1000 % 2 == 0) led_on();
  else led_off();
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

 WiFi.enableAP(false);


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


 if (ap.config.mqttSubscribeTopic != 0 && strlen(ap.config.mqttSubscribeTopic) > 0) {
  mqtt.subscribe(ap.config.mqttSubscribeTopic);
  mqtt.onMessage(mqttMessageReceived);
 }


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



}

void readHanPort()
{
 if (hanReader.read() && ap.config.hasConfig())
 {

  led_on();


  time_t time = hanReader.getPackageTime();
  if (debugger) debugger->print("Time of the package is: ");
  if (debugger) debugger->println(time);


  StaticJsonDocument<500> json;


  json["id"] = WiFi.macAddress();
  json["up"] = millis();
  json["t"] = time;



  JsonObject data = json.createNestedObject("data");

#if HAS_DALLAS_TEMP_SENSOR

  tempSensor.requestTemperatures();
  data["temp"] = tempSensor.getTempCByIndex(0);
#endif

  hanToJson(data, ap.config.meterType, hanReader);

  if(ap.config.mqtt != 0 && strlen(ap.config.mqtt) != 0 && ap.config.mqttPublishTopic != 0 && strlen(ap.config.mqttPublishTopic) != 0) {

   if (debugger) {
    debugger->print("Sending data to MQTT: ");
    serializeJsonPretty(json, *debugger);
    debugger->println();
   }


   String msg;
   serializeJson(json, msg);

   mqtt.publish(ap.config.mqttPublishTopic, msg.c_str());
   mqtt.loop();
  }
  ws.setJson(json);


  led_off();
 }
}




void MQTT_connect()
{

 if (debugger)
 {
  debugger->println();
  debugger->println();
  debugger->print("Connecting to WiFi network ");
  debugger->println(ap.config.ssid);
 }

 if (WiFi.status() != WL_CONNECTED)
 {

  WiFi.disconnect();
  WiFi.begin(ap.config.ssid, ap.config.ssidPassword);
  delay(1000);
 }


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
   if (debugger)
   {
    debugger->print(".");
    debugger->print("failed, ");
    debugger->println(" trying again in 5 seconds");
   }


   mqtt.disconnect();

   delay(2000);
  }


  yield();
  delay(2000);
 }
}


void sendMqttData(String data)
{

 if (ap.config.mqttPublishTopic == 0 || strlen(ap.config.mqttPublishTopic) == 0)
  return;


 if (!client->connected() || !mqtt.connected()) {
  MQTT_connect();
 }


 StaticJsonDocument<500> json;
 json["id"] = WiFi.macAddress();
 json["up"] = millis();
 json["data"] = data;


 String msg;
 serializeJson(json, msg);


 mqtt.publish(ap.config.mqttPublishTopic, msg.c_str());

 if (debugger) debugger->print("sendMqttData: ");
 if (debugger) debugger->println(data);
}