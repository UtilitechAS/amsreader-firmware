/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "WiFiAccessPointConnectionHandler.h"

#if defined(AMS_REMOTE_DEBUG)
WiFiAccessPointConnectionHandler::WiFiAccessPointConnectionHandler(RemoteDebug* debugger) {
#else
WiFiAccessPointConnectionHandler::WiFiAccessPointConnectionHandler(Stream* debugger) {
#endif
    this->debugger = debugger;
    this->mode = NETWORK_MODE_WIFI_AP;
}

bool WiFiAccessPointConnectionHandler::connect(NetworkConfig config, SystemConfig sys) {
		//wifi_softap_set_dhcps_offer_option(OFFER_ROUTER, 0); // Disable default gw

		WiFi.mode(WIFI_AP);
		WiFi.softAP(config.ssid, config.psk);

		dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
		dnsServer.start(53, PSTR("*"), WiFi.softAPIP());
        connected = true;

        return true;
}

void WiFiAccessPointConnectionHandler::disconnect(unsigned long reconnectDelay) {
	WiFi.disconnect(true);
	WiFi.softAPdisconnect(true);
	WiFi.enableAP(false);
	WiFi.mode(WIFI_OFF);
	yield();
}

bool WiFiAccessPointConnectionHandler::isConnected() {
    return connected;
}

#if defined(ESP32)
void WiFiAccessPointConnectionHandler::eventHandler(WiFiEvent_t event, WiFiEventInfo_t info) {
    uint8_t mac[6];
    IPAddress stationIP;
    switch(event) {
        case ARDUINO_EVENT_WIFI_AP_START:
            #if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::INFO))
#endif
debugger->printf_P(PSTR("WiFi access point started with SSID %s\n"), config.ssid);
            break;
	    case ARDUINO_EVENT_WIFI_AP_STOP:
            #if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::INFO))
#endif
debugger->printf_P(PSTR("WiFi access point stopped!\n"));
            break;
	    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            memcpy(mac, info.wifi_ap_staconnected.mac, 6);
            #if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::INFO))
#endif
debugger->printf_P(PSTR("Client connected to AP, client MAC: %02x:%02x:%02x:%02x:%02x:%02x\n"), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            break;
	    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            memcpy(mac, info.wifi_ap_staconnected.mac, 6);
            #if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::INFO))
#endif
debugger->printf_P(PSTR("Client disconnected from AP, client MAC: %02x:%02x:%02x:%02x:%02x:%02x\n"), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            break;
	    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
            stationIP = info.wifi_ap_staipassigned.ip.addr;
            #if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::INFO))
#endif
debugger->printf_P(PSTR("Client was assigned IP %s\n"), stationIP.toString().c_str());
            break;
    }
}
#endif

bool WiFiAccessPointConnectionHandler::isConfigChanged() {
	return configChanged;
}

void WiFiAccessPointConnectionHandler::getCurrentConfig(NetworkConfig& networkConfig) {
	networkConfig = this->config;
}

IPAddress WiFiAccessPointConnectionHandler::getIP() {
	return WiFi.softAPIP();
}

IPAddress WiFiAccessPointConnectionHandler::getSubnetMask() {
    #if defined(ESP32)
	return WiFi.softAPSubnetMask();
    #else
    return IPAddress(255,255,255,0);
    #endif
}

IPAddress WiFiAccessPointConnectionHandler::getGateway() {
	return WiFi.softAPIP();
}

IPAddress WiFiAccessPointConnectionHandler::getDns(uint8_t idx) {
	return WiFi.softAPIP();
}

#if defined(ESP32)
IPv6Address WiFiAccessPointConnectionHandler::getIPv6() {
    return IPv6Address();
}

IPv6Address WiFiAccessPointConnectionHandler::getDNSv6(uint8_t idx) {
    return IPv6Address();
}
#endif
