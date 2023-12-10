/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "EthernetConnectionHandler.h"

#if defined(ESP32)
#include <ETH.h>
#endif

EthernetConnectionHandler::EthernetConnectionHandler(RemoteDebug* debugger) {
    this->debugger = debugger;
    this->mode = NETWORK_MODE_ETH_CLIENT;
}

bool EthernetConnectionHandler::connect(NetworkConfig config, SystemConfig sys) {
	if(lastRetry > 0 && (millis() - lastRetry) < timeout) {
		delay(50);
		return false;
	}
	lastRetry = millis();

    #if defined(ESP32)
	if (!connected) {
        eth_phy_type_t ethType = ETH_PHY_LAN8720;
        eth_clock_mode_t ethClkMode = ETH_CLOCK_GPIO0_IN;
        uint8_t ethAddr = 0;
        uint8_t ethMdc = 0;
        uint8_t ethMdio = 0;

        if(sys.boardType == 241) {
			if (debugger->isActive(RemoteDebug::DEBUG)) debugger->printf_P(PSTR("LilyGO T-ETH-POE\n"));
            ethType = ETH_PHY_LAN8720;
            ethEnablePin = -1;
            ethAddr = 0;
            ethClkMode = ETH_CLOCK_GPIO17_OUT;
            ethPowerPin = 5;
            ethMdc = 23;
            ethMdio = 18;
        } else if(sys.boardType == 242) {
			if (debugger->isActive(RemoteDebug::DEBUG)) debugger->printf_P(PSTR("M5 PoESP32\n"));
            ethType = ETH_PHY_IP101;
            ethEnablePin = -1;
            ethAddr = 1;
            ethClkMode = ETH_CLOCK_GPIO0_IN;
            ethPowerPin = 5;
            ethMdc = 23;
            ethMdio = 18;
        } else if(sys.boardType == 243) {
			if (debugger->isActive(RemoteDebug::DEBUG)) debugger->printf_P(PSTR("WT32-ETH01\n"));
            ethType = ETH_PHY_LAN8720;
            ethEnablePin = -1;
            ethAddr = 1;
            ethClkMode = ETH_CLOCK_GPIO17_OUT;
            ethPowerPin = 16;
            ethMdc = 23;
            ethMdio = 18;
        } else {
			if (debugger->isActive(RemoteDebug::ERROR)) debugger->printf_P(PSTR("Board type %d incompatible with ETH\n"), sys.boardType);
            return false;
        }

        if(ethEnablePin > 0) {
            pinMode(ethEnablePin, OUTPUT);
            digitalWrite(ethEnablePin, 1);
        }

        if (debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("Connecting to Ethernet\n"));

        if(ETH.begin(ethAddr, ethPowerPin, ethMdc, ethMdio, ethType, ethClkMode)) {
            #if defined(ESP32)
                if(strlen(config.hostname) > 0) {
                    ETH.setHostname(config.hostname);
                }
            #endif

            if(strlen(config.ip) > 0) {
                IPAddress ip, gw, sn(255,255,255,0), dns1, dns2;
                ip.fromString(config.ip);
                gw.fromString(config.gateway);
                sn.fromString(config.subnet);
                if(strlen(config.dns1) > 0) {
                    dns1.fromString(config.dns1);
                } else if(strlen(config.gateway) > 0) {
                    dns1.fromString(config.gateway); // If no DNS, set gateway by default
                }
                if(strlen(config.dns2) > 0) {
                    dns2.fromString(config.dns2);
                } else if(dns1.toString().isEmpty()) {
                    dns2.fromString(F("208.67.220.220")); // Add OpenDNS as second by default if nothing is configured
                }
                if(!ETH.config(ip, gw, sn, dns1, dns2)) {
                    debugger->printf_P(PSTR("Static IP configuration is invalid, not using\n"));
                }
            }
        } else {
			if (debugger->isActive(RemoteDebug::ERROR)) debugger->printf_P(PSTR("Unable to start Ethernet\n"));
        }
    }
    #endif
    return false;
}

void EthernetConnectionHandler::disconnect(unsigned long reconnectDelay) {
	if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf_P(PSTR("Disconnecting!\n"));
}

bool EthernetConnectionHandler::isConnected() {
    return connected;
}

#if defined(ESP32)
void EthernetConnectionHandler::eventHandler(WiFiEvent_t event, WiFiEventInfo_t info) {
    switch(event) {
        case ARDUINO_EVENT_ETH_CONNECTED:
            connected = true;
            if(debugger->isActive(RemoteDebug::INFO)) {
                debugger->printf_P(PSTR("Successfully connected to Ethernet!\n"));
            }
            break;
        case ARDUINO_EVENT_ETH_GOT_IP:
            if(debugger->isActive(RemoteDebug::INFO)) {
                debugger->printf_P(PSTR("IP:  %s\n"), ETH.localIP().toString().c_str());
                debugger->printf_P(PSTR("GW:  %s\n"), ETH.gatewayIP().toString().c_str());
                debugger->printf_P(PSTR("DNS: %s\n"), ETH.dnsIP().toString().c_str());
            }
            break;
        case ARDUINO_EVENT_ETH_DISCONNECTED:
            connected = false;
            if(debugger->isActive(RemoteDebug::WARNING)) {
                debugger->printf_P(PSTR("Ethernet was disconnected!\n"));
            }
            break;
    }
}
#endif

bool EthernetConnectionHandler::isConfigChanged() {
	return configChanged;
}

void EthernetConnectionHandler::getCurrentConfig(NetworkConfig& networkConfig) {
	networkConfig = this->config;
}

IPAddress EthernetConnectionHandler::getIP() {
	return ETH.localIP();
}

IPAddress EthernetConnectionHandler::getSubnetMask() {
	return ETH.subnetMask();
}

IPAddress EthernetConnectionHandler::getGateway() {
	return ETH.gatewayIP();
}

IPAddress EthernetConnectionHandler::getDns(uint8_t idx) {
	return ETH.dnsIP(idx);
}
