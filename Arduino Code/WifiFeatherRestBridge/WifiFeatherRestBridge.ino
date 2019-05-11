#include <ArduinoJson.h>
#include <SPI.h>
#include <WiFi101.h>
#include "Kaifa.h"
#include "Kamstrup.h"
#include "HanReader.h"
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
    Serial.begin(9600);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }

    // Uncomment to debug over uart
    debugger = &Serial;

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

    //connect_wifi();

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

        // Get the list identifier
        int listSize = hanReader.getListSize();

        switch (meterType)
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
                if (debugger) {
                    debugger->print("Meter type ");
                    debugger->print(meterType, HEX);
                    debugger->println(" is unknown");
                }
                delay(10000); // TODO: Why sleep?
                break;
        }

        // Flash LED off
        digitalWrite(LED_PIN, LOW);
    }
}

void readHanPort_Aidon(int listSize)
{
    if (debugger) debugger->println("Meter type Aidon is not yet implemented");
    delay(1000);
}

void readHanPort_Kamstrup(int listSize)
{
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
        StaticJsonDocument<500> doc;

        // Any generic useful info here
        //doc["id"] = WiFi.macAddress(); // TODO: Fix?
        doc["up"] = millis();
        doc["t"] = time;

        // Add a sub-structure to the json object, 
        // to keep the data from the meter itself
        JsonObject data = doc.createNestedObject("data");

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
        if (debugger) {
            debugger->print("Sending data to MQTT: ");
            serializeJsonPretty(doc, *debugger);
            debugger->println();
        }

        // TODO: Post data
        //// Make sure we have configured a publish topic
        //if (ap.config.mqttPublishTopic == 0 || strlen(ap.config.mqttPublishTopic) == 0)
        //	return;

        //// Publish the json to the MQTT server
        //char msg[1024];
        //serializeJsonPretty(doc, msg, 1024);
        //mqtt.publish(ap.config.mqttPublishTopic, msg);

        //if (send_data(&client)) {
        //    state = 1;
        //}
    }
}


void readHanPort_Kaifa(int listSize)
{
    // Only care for the ACtive Power Imported, which is found in the first list
    if (listSize == (int)Kaifa::List1 || listSize == (int)Kaifa::List2 || listSize == (int)Kaifa::List3 || listSize == (int)Kaifa::List4)
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
        //StaticJsonDocument<500> doc;
        DynamicJsonDocument doc(500); // TODO: Too small?

        // Any generic useful info here
        //doc["id"] = WiFi.macAddress(); // TODO: Fix?
        doc["up"] = millis();
        doc["t"] = time;

        // Add a sub-structure to the json object,
        // to keep the data from the meter itself
        JsonObject data = doc.createNestedObject("data");

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
        else if (listSize == (int)Kaifa::List4)
        {
            data["lv"] = hanReader.getString((int)Kaifa_List3::ListVersionIdentifier);;
            data["id"] = hanReader.getString((int)Kaifa_List3::MeterID);
            data["type"] = hanReader.getString((int)Kaifa_List3::MeterType);
            data["P"] = hanReader.getInt((int)Kaifa_List3::ActiveImportPower);
            data["Q"] = hanReader.getInt((int)Kaifa_List3::ReactiveImportPower);
            data["I1"] = hanReader.getInt((int)Kaifa_List3::CurrentL1);
            data["I2"] = hanReader.getInt((int)Kaifa_List3::CurrentL2);
        }

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
    }
}

