/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _WIFICLIENTCONNECTIONHANDLER_H
#define _WIFICLIENTCONNECTIONHANDLER_H

#include "ConnectionHandler.h"
#include <Arduino.h>
#include "RemoteDebug.h"

#define CONNECTION_TIMEOUT 30000
#define RECONNECT_TIMEOUT 5000

class WiFiClientConnectionHandler : public ConnectionHandler {
public:
    WiFiClientConnectionHandler(RemoteDebug* debugger);

    bool connect(NetworkConfig config, SystemConfig sys);
    void disconnect(unsigned long reconnectDelay);
    bool isConnected();
    bool isConfigChanged();
    void getCurrentConfig(NetworkConfig& networkConfig);
    IPAddress getIP();
    IPAddress getSubnetMask();
    IPAddress getGateway();
    IPAddress getDns(uint8_t idx);

    #if defined(ESP32)
    void eventHandler(WiFiEvent_t event, WiFiEventInfo_t info);
    #endif

private:
    RemoteDebug* debugger;
    NetworkConfig config;
    bool busPowered = false;
    bool firstConnect = true;
    bool configChanged = false;

    unsigned long timeout = CONNECTION_TIMEOUT;
    unsigned long lastRetry = 0;
};

#endif
