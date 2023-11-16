/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _WIFIACCESSPOINTCONNECTIONHANDLER_H
#define _WIFIACCESSPOINTCONNECTIONHANDLER_H

#include "ConnectionHandler.h"
#include <Arduino.h>
#include "RemoteDebug.h"
#include <DNSServer.h>

class WiFiAccessPointConnectionHandler : public ConnectionHandler {
public:
    WiFiAccessPointConnectionHandler(RemoteDebug* debugger);

    bool connect(NetworkConfig config, SystemConfig sys);
    void disconnect(unsigned long reconnectDelay);
    bool isConnected();
    bool isConfigChanged();
    void getCurrentConfig(NetworkConfig& networkConfig);

    #if defined(ESP32)
    void eventHandler(WiFiEvent_t event, WiFiEventInfo_t info);
    #endif

private:
    RemoteDebug* debugger;
    NetworkConfig config;

    DNSServer dnsServer;

    bool connected = false;
    bool configChanged = false;
};

#endif
