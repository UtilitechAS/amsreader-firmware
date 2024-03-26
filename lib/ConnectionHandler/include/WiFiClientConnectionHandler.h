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

#if defined(ESP32)
esp_err_t set_esp_interface_ip(esp_interface_t interface, IPAddress local_ip=INADDR_NONE, IPAddress gateway=INADDR_NONE, IPAddress subnet=INADDR_NONE, IPAddress dhcp_lease_start=INADDR_NONE);
#endif

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
    IPv6Address getIPv6();
    IPv6Address getDNSv6(uint8_t idx);
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

    wl_status_t begin(const char* ssid, const char* psk);
    #if defined(ESP32)
    void wifi_sta_config(wifi_config_t * wifi_config, const char * ssid=NULL, const char * password=NULL, const uint8_t * bssid=NULL, uint8_t channel=0, wifi_auth_mode_t min_security=WIFI_AUTH_WPA2_PSK, wifi_scan_method_t scan_method=WIFI_ALL_CHANNEL_SCAN, wifi_sort_method_t sort_method=WIFI_CONNECT_AP_BY_SIGNAL, uint16_t listen_interval=0, bool pmf_required=false);
    #endif
};

#endif
