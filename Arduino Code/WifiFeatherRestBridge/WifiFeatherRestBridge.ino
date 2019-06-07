#include <ArduinoJson.h>
#include <SPI.h>
#include <WiFi101.h>
#include "HanReader.h"
#include "HanToJson.h"
#include "wifi_client.h"


#define LED_PIN 13 // The red led on the WiFi Feather


int state  = 0;
WiFiClient client;

// Object used for debugging
Serial_* debugger = NULL;

// The HAN Port reader, used to read serial data and decode DLMS
HanReader hanReader;
byte meterType = 1; // Kaifa TODO: Read config, don't hard code

void setup() {
    // Enable red LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(100);

    // Initialize serial and wait for port to open:
    //Serial.begin(9600);
    //while (!Serial) ; // wait for serial port to connect. Needed for native USB port only

    // Uncomment to debug over uart
    //debugger = &Serial;

    if (debugger) debugger->print("Unwantingly wait 5"); // Allow programming during restart
    delay(1000);
    if (debugger) debugger->print(" 4");
    delay(1000);
    if (debugger) debugger->print(" 3");
    delay(1000);
    if (debugger) debugger->print(" 2");
    delay(1000);
    if (debugger) debugger->print(" 1");
    delay(1000);
    if (debugger) debugger->println(" 0");

    // Configure uart for AMS data
    Serial1.begin(2400, SERIAL_8E1);
    while (!Serial1);

    // Configure pins for Adafruit ATWINC1500 Feather
    WiFi.setPins(8,7,4,2);

    // Check for the presence of the shield:
    if (WiFi.status() == WL_NO_SHIELD) {
        if (debugger) debugger->println("WiFi shield not present");
        // don't continue:
        while (true);
    }

    connect_wifi();

    hanReader.setup(&Serial1, debugger);

    // Compensate for the known Kaifa bug
    hanReader.compensateFor09HeaderBug = (meterType == 1);
}

void loop() {
    // If there are incoming bytes available
    // from the server, read them and print them:
    while (client.available()) {
        state = 2;
        char c = client.read();
        Serial.write(c);
    }

    // Change state if last byte read from client
    if (state == 2 && !client.available()) {
        state = 0;

        if (debugger) debugger->println();
        if (debugger) debugger->println("Response data received");
    }

    readHanPort();

    //if (debugger) debugger->println("Wait for event ");
    __WFI();
}


void readHanPort()
{
    if (hanReader.read())
    {
        // Flash LED on, this shows us that data is received
        digitalWrite(LED_PIN, HIGH);

        // Get the timestamp (as unix time) from the package
        time_t time = hanReader.getPackageTime();
        if (debugger) debugger->print("Time of the package is: ");
        if (debugger) debugger->println(time);

        // Define a json object to keep the data
        StaticJsonDocument<500> doc;

        // Any generic useful info here
        //doc["id"] = WiFi.macAddress(); // TODO: Fix?
        doc["up"] = millis();
        doc["t"] = time;

        // Add a sub-structure to the json object,
        // to keep the data from the meter itself
        JsonObject data = doc.createNestedObject("data");

        hanToJson(data, meterType, hanReader);

        // Write the json to the debug port
        if (debugger) {
            debugger->print("Sending data to MQTT: ");
            serializeJsonPretty(doc, *debugger);
            debugger->println();
            debugger->print("data size: ");
            debugger->println(measureJson(doc));
        }

        // TODO: Post data
        //// Make sure we have configured a publish topic
        //if (ap.config.mqttPublishTopic == 0 || strlen(ap.config.mqttPublishTopic) == 0)
        //	return;

        //// Publish the json to the MQTT server
        char msg[1024];
        serializeJson(doc, msg, sizeof(msg));
        //mqtt.publish(ap.config.mqttPublishTopic, msg);

        if (send_data(&client, msg)) {
            state = 1;
        }

        // Flash LED off
        digitalWrite(LED_PIN, LOW);
    }
}

