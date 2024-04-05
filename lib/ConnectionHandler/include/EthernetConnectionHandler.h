/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _ETHERNETCONNECTIONHANDLER_H
#define _ETHERNETCONNECTIONHANDLER_H

#include "ConnectionHandler.h"
#include <Arduino.h>
#include "RemoteDebug.h"

#define CONNECTION_TIMEOUT 30000

class EthernetConnectionHandler : public ConnectionHandler {
public:
    EthernetConnectionHandler(RemoteDebug* debugger);

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
    RemoteDebug* debugger;
    NetworkConfig config;

    bool connected = false;
    bool configChanged = false;
    unsigned long timeout = CONNECTION_TIMEOUT;
    unsigned long lastRetry = 0;

    int8_t ethPowerPin = -1;
    uint8_t ethEnablePin = 0;

};

#endif
