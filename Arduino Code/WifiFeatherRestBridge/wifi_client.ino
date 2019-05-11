#include "wifi_client.h"
#include <SPI.h>
#include "arduino_secrets.h"


char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
char auth[] = AUTH_HEADER;


// TODO: s/Serial.print/if (debugger) debugger->print/
// TODO: Rename from .ino to .cpp


void printWiFiStatus() {
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}


void connect_wifi() {
    // attempt to connect to WiFi network:
    while (true) {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(ssid);
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        int status = WiFi.begin(ssid, pass);

        if (status == WL_CONNECTED) {
            break;
        }

        // wait 1 seconds for connection:
        Serial.print("Waiting 1 before trying again. status: ");
        Serial.println(status);
        delay(1000);
    }

    WiFi.lowPowerMode();
    //WiFi.setSleepMode(M2M_PS_H_AUTOMATIC, 1);

    Serial.println("Connected to wifi");
    printWiFiStatus();
}


bool send_data(WiFiClient *client, char* json_data) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Ehhh, not connected?");
        return false;
    }

    Serial.println("\nConnection to server...");
    if (client->connect("kanskje.de", 8181)) {
        Serial.println("connected to server");
        char content_length[] = "Content-Length: 1234567890";
        sprintf(content_length, "Content-Length: %d", strlen(json_data));

        // Make a HTTP request:
        client->println("POST /what-ever HTTP/1.1");
        client->println("Host: kanskje.de");
        client->print("Authorization: ");
        client->println(auth);
        client->println("Content-Type: application/json");
        client->println(content_length);
        client->println("Connection: close");
        client->println();
        client->println(json_data);
        return true;
    }

    return false;
}


