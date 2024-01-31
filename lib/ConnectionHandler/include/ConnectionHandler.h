/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _CONNECTIONHANDLER_H
#define _CONNECTIONHANDLER_H

#include "AmsConfiguration.h"
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
esp_netif_t* get_esp_interface_netif(esp_interface_t interface);
#endif

#define NETWORK_MODE_WIFI_CLIENT 1
#define NETWORK_MODE_WIFI_AP 2
#define NETWORK_MODE_ETH_CLIENT 3

class ConnectionHandler {
public:
    virtual ~ConnectionHandler() {};
    virtual bool connect(NetworkConfig config, SystemConfig sys);
    virtual void disconnect(unsigned long reconnectDelay);
    virtual bool isConnected();
    virtual bool isConfigChanged();
    virtual void getCurrentConfig(NetworkConfig& networkConfig);
    virtual IPAddress getIP();
    virtual IPAddress getSubnetMask();
    virtual IPAddress getGateway();
    virtual IPAddress getDns(uint8_t idx);
    #if defined(ESP32)
    virtual IPv6Address getIPv6();
    virtual IPv6Address getDNSv6(uint8_t idx);
    virtual void eventHandler(WiFiEvent_t event, WiFiEventInfo_t info);
    #endif

    uint8_t getMode() {
        return this->mode;
    }

protected:
    uint8_t mode;
};

#endif
