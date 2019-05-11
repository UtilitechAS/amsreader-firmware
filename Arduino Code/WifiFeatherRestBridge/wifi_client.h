#ifndef _WIFI_CLIENT_H
#define _WIFI_CLIENT_H

#include <WiFi101.h>


void connect_wifi();
bool send_data(WiFiClient *client, char* json_data);

#endif//_WIFI_CLIENT_H
