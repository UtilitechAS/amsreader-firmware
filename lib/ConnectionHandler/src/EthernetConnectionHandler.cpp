/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "EthernetConnectionHandler.h"

#if defined(ESP32)
#include <ETH.h>
#include <esp_wifi.h>
#include <lwip/dns.h>
#endif

#if defined(AMS_REMOTE_DEBUG)
EthernetConnectionHandler::EthernetConnectionHandler(RemoteDebug* debugger) {
#else
EthernetConnectionHandler::EthernetConnectionHandler(Stream* debugger) {
#endif
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

        if(sys.boardType == 241) { // LilyGO T-ETH-POE
            ethType = ETH_PHY_LAN8720;
            ethEnablePin = -1;
            ethAddr = 0;
            ethClkMode = ETH_CLOCK_GPIO17_OUT;
            ethPowerPin = 5;
            ethMdc = 23;
            ethMdio = 18;
        } else if(sys.boardType == 242) { // M5 PoESP32
            ethType = ETH_PHY_IP101;
            ethEnablePin = -1;
            ethAddr = 1;
            ethClkMode = ETH_CLOCK_GPIO0_IN;
            ethPowerPin = 5;
            ethMdc = 23;
            ethMdio = 18;
        } else if(sys.boardType == 243) { // WT32-ETH01
            ethType = ETH_PHY_LAN8720;
            ethEnablePin = -1;
            ethAddr = 1;
            ethClkMode = ETH_CLOCK_GPIO17_OUT;
            ethPowerPin = 16;
            ethMdc = 23;
            ethMdio = 18;
        } else if (sys.boardType == 245) { // wESP32
            ethType = ETH_PHY_RTL8201;
            ethEnablePin = -1;
            ethAddr = 0;
            ethClkMode = ETH_CLOCK_GPIO0_IN;
            ethPowerPin = -1;
            ethMdc = 16;
            ethMdio = 17;
        } else if(sys.boardType == 244) {
            return false; // TODO
        } else {
			#if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::ERROR))
            #endif
            debugger->printf_P(PSTR("Board type %d incompatible with ETH\n"), sys.boardType);
            return false;
        }

        if(ethEnablePin > 0) {
            pinMode(ethEnablePin, OUTPUT);
            digitalWrite(ethEnablePin, 1);
        }

        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::INFO))
        #endif
        debugger->printf_P(PSTR("Connecting to Ethernet\n"));

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
			#if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::ERROR))
            #endif
            debugger->printf_P(PSTR("Unable to start Ethernet\n"));
        }
    }
    #endif
    return false;
}

void EthernetConnectionHandler::disconnect(unsigned long reconnectDelay) {
	#if defined(AMS_REMOTE_DEBUG)
    if (debugger->isActive(RemoteDebug::ERROR))
    #endif
    debugger->printf_P(PSTR("Disconnecting!\n"));
}

bool EthernetConnectionHandler::isConnected() {
    return connected;
}

#if defined(ESP32)
void EthernetConnectionHandler::eventHandler(WiFiEvent_t event, WiFiEventInfo_t info) {
    switch(event) {
        case ARDUINO_EVENT_ETH_CONNECTED:
            connected = true;
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::INFO))
            #endif
            {
                debugger->printf_P(PSTR("Successfully connected to Ethernet!\n"));
            }
            break;
        case ARDUINO_EVENT_ETH_GOT_IP:
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::INFO))
            #endif
            {
                debugger->printf_P(PSTR("IP:  %s\n"), getIP().toString().c_str());
                debugger->printf_P(PSTR("GW:  %s\n"), getGateway().toString().c_str());
				for(uint8_t i = 0; i < 3; i++) {
					IPAddress dns4 = getDns(i);
					if(dns4 == IPAddress()) {
						// No IP
					} else {
						debugger->printf_P(PSTR("DNS: %s\n"), dns4.toString().c_str());
					}
				}
            }
			break;
		case ARDUINO_EVENT_ETH_GOT_IP6: {
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::INFO))
            #endif
            {
				IPv6Address ipv6 = getIPv6();
				if(ipv6 == IPv6Address()) {
					// No IP
				} else {
					debugger->printf_P(PSTR("IPv6:  %s\n"), ipv6.toString().c_str());
				}

				for(uint8_t i = 0; i < 3; i++) {
					IPv6Address dns6 = getDNSv6(i);
					if(dns6 == IPv6Address()) {
						// No IP
					} else {
						debugger->printf_P(PSTR("DNSv6: %s\n"), dns6.toString().c_str());
					}
				}
            }
			break;
		}
        case ARDUINO_EVENT_ETH_DISCONNECTED:
            connected = false;
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::WARNING))
            #endif
            {
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
    #if defined(ESP32)
	return ETH.localIP();
    #else
    return NULL;
    #endif
}

IPAddress EthernetConnectionHandler::getSubnetMask() {
    #if defined(ESP32)
	return ETH.subnetMask();
    #else
    return NULL;
    #endif
}

IPAddress EthernetConnectionHandler::getGateway() {
    #if defined(ESP32)
	return ETH.gatewayIP();
    #else
    return NULL;
    #endif
}

IPAddress EthernetConnectionHandler::getDns(uint8_t idx) {
    #if defined(ESP32)
	return ETH.dnsIP(idx);
    #else
    return NULL;
    #endif
}

#if defined(ESP32)
IPv6Address EthernetConnectionHandler::getIPv6() {
	esp_ip6_addr_t addr;
	if(esp_netif_get_ip6_global(get_esp_interface_netif(ESP_IF_ETH), &addr) == ESP_OK) {
		return IPv6Address(addr.addr);
	}
	return IPv6Address();
}

IPv6Address EthernetConnectionHandler::getDNSv6(uint8_t idx) {
	for(uint8_t i = 0; i < 3; i++) {
		const ip_addr_t * dns = dns_getserver(i);
		if(dns->type == IPADDR_TYPE_V6) {
			if(idx-- == 0) return IPv6Address(dns->u_addr.ip6.addr);
		}
	}
	return IPv6Address();
}
#endif