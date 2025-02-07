/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _WIFIACCESSPOINTCONNECTIONHANDLER_H
#define _WIFIACCESSPOINTCONNECTIONHANDLER_H

#include "ConnectionHandler.h"
#include <Arduino.h>
#if defined(AMS_REMOTE_DEBUG)
#include "RemoteDebug.h"
#endif
#include <DNSServer.h>

class WiFiAccessPointConnectionHandler : public ConnectionHandler {
public:
    #if defined(AMS_REMOTE_DEBUG)
    WiFiAccessPointConnectionHandler(RemoteDebug* debugger);
    #else
    WiFiAccessPointConnectionHandler(Stream* debugger);
    #endif

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
    IPv6Address getIPv6();
    IPv6Address getDNSv6(uint8_t idx);
    void eventHandler(WiFiEvent_t event, WiFiEventInfo_t info);
    #endif

private:
    #if defined(AMS_REMOTE_DEBUG)
    RemoteDebug* debugger;
    #else
    Stream* debugger;
    #endif
    NetworkConfig config;

    DNSServer dnsServer;

    bool connected = false;
    bool configChanged = false;
};

#endif
