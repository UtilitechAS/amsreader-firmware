/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "WiFiClientConnectionHandler.h"
#if defined(ESP32)
#include <esp_wifi.h>
#include <lwip/dns.h>
#endif

WiFiClientConnectionHandler::WiFiClientConnectionHandler(RemoteDebug* debugger) {
    this->debugger = debugger;
    this->mode = NETWORK_MODE_WIFI_CLIENT;
}

bool WiFiClientConnectionHandler::connect(NetworkConfig config, SystemConfig sys) {
	if(lastRetry > 0 && (millis() - lastRetry) < timeout) {
		delay(50);
		return false;
	}
	lastRetry = millis();

	if (WiFi.status() != WL_CONNECTED) {
		if(config.mode != this->mode || strlen(config.ssid) == 0) {
			return false;
		}

		if(WiFi.getMode() != WIFI_OFF) {
			if (debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("Not connected to WiFi, closing resources\n"));

			disconnect(RECONNECT_TIMEOUT);
			return false;
		}
		timeout = CONNECTION_TIMEOUT;

		if (debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("Connecting to WiFi network: %s\n"), config.ssid);
		switch(sys.boardType) {
			case 2: // spenceme
			case 3: // Pow-K UART0
			case 4: // Pow-U UART0
			case 5: // Pow-K+
			case 6: // Pow-P1
			case 7: // Pow-U+
			case 8: // dbeinder: HAN mosquito
				busPowered = true;
				break;
			default:
				busPowered = false;
		}
		firstConnect = sys.dataCollectionConsent == 0;

		#if defined(ESP32)
			if(strlen(config.hostname) > 0) {
				WiFi.setHostname(config.hostname);
			}
		#endif
		WiFi.mode(WIFI_STA);

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
			if(!WiFi.config(ip, gw, sn, dns1, dns2)) {
				debugger->printf_P(PSTR("Static IP configuration is invalid, not using\n"));
			}
		}
		#if defined(ESP8266)
			if(strlen(config.hostname) > 0) {
				WiFi.hostname(config.hostname);
			}
			//wifi_set_phy_mode(PHY_MODE_11N);
			if(!config.use11b) {
				wifi_set_user_sup_rate(RATE_11G6M, RATE_11G54M);
				wifi_set_user_rate_limit(RC_LIMIT_11G, 0x00, RATE_11G_G54M, RATE_11G_G6M);
				wifi_set_user_rate_limit(RC_LIMIT_11N, 0x00, RATE_11N_MCS7S, RATE_11N_MCS0);
				wifi_set_user_limit_rate_mask(LIMIT_RATE_MASK_ALL);
			}
		#endif
		#if defined(ESP32)
			WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
			WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
		#endif
		WiFi.setAutoReconnect(true);
		if(WiFi.begin(config.ssid, config.psk)) {
			if(config.sleep <= 2) {
				switch(config.sleep) {
					case 0:
						WiFi.setSleep(WIFI_PS_NONE);
						break;
					case 1:
						WiFi.setSleep(WIFI_PS_MIN_MODEM);
						break;
					case 2:
						WiFi.setSleep(WIFI_PS_MAX_MODEM);
						break;
				}
			}
			yield();
		} else {
			if (debugger->isActive(RemoteDebug::ERROR)) debugger->printf_P(PSTR("Unable to start WiFi\n"));
		}
        this->config = config;
        return true;
  	}
    return false;
}

void WiFiClientConnectionHandler::disconnect(unsigned long reconnectDelay) {
	if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf_P(PSTR("Disconnecting!\n"));
	#if defined(ESP8266)
		WiFiClient::stopAll();
	#endif

	WiFi.disconnect(true);
	WiFi.softAPdisconnect(true);
	WiFi.enableAP(false);
	WiFi.mode(WIFI_OFF);
	yield();
	timeout = reconnectDelay;
}

bool WiFiClientConnectionHandler::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

#if defined(ESP32)
void WiFiClientConnectionHandler::eventHandler(WiFiEvent_t event, WiFiEventInfo_t info) {
    switch(event) {
		case ARDUINO_EVENT_WIFI_READY:
			if (!config.use11b) {
				esp_wifi_config_11b_rate(WIFI_IF_AP, true);
				esp_wifi_config_11b_rate(WIFI_IF_STA, true);
			}
			break;
		case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("Successfully connected to WiFi!\n"));
			if(config.ipv6 && !WiFi.enableIpV6()) {
				debugger->printf_P(PSTR("Unable to enable IPv6\n"));
			}
            #if defined(ESP32)
				if(firstConnect && config.use11b) {
					// If first boot and phyMode is better than 11b, disable 11b for BUS powered devices
					if(busPowered) {
						wifi_phy_mode_t phyMode;
						if(esp_wifi_sta_get_negotiated_phymode(&phyMode) == ESP_OK) {
							if(phyMode > WIFI_PHY_MODE_11B) {
								if (debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("WiFi supports better rates than 802.11b, disabling\n"));
								config.use11b = false;
								configChanged = true;
								return;
							}
						}
					}
				}

                if(config.power >= 195)
                    WiFi.setTxPower(WIFI_POWER_19_5dBm);
                else if(config.power >= 190)
                    WiFi.setTxPower(WIFI_POWER_19dBm);
                else if(config.power >= 185)
                    WiFi.setTxPower(WIFI_POWER_18_5dBm);
                else if(config.power >= 170)
                    WiFi.setTxPower(WIFI_POWER_17dBm);
                else if(config.power >= 150)
                    WiFi.setTxPower(WIFI_POWER_15dBm);
                else if(config.power >= 130)
                    WiFi.setTxPower(WIFI_POWER_13dBm);
                else if(config.power >= 110)
                    WiFi.setTxPower(WIFI_POWER_11dBm);
                else if(config.power >= 85)
                    WiFi.setTxPower(WIFI_POWER_8_5dBm);
                else if(config.power >= 70)
                    WiFi.setTxPower(WIFI_POWER_7dBm);
                else if(config.power >= 50)
                    WiFi.setTxPower(WIFI_POWER_5dBm);
                else if(config.power >= 20)
                    WiFi.setTxPower(WIFI_POWER_2dBm);
                else
                    WiFi.setTxPower(WIFI_POWER_MINUS_1dBm);
            #elif defined(ESP8266)
                WiFi.setOutputPower(config.power / 10.0);
            #endif
			break;
		case ARDUINO_EVENT_WIFI_STA_GOT_IP: {
            if(debugger->isActive(RemoteDebug::INFO)) {
                debugger->printf_P(PSTR("IP:  %s\n"), getIP().toString().c_str());
                debugger->printf_P(PSTR("GW:  %s\n"), getGateway().toString().c_str());
				for(uint8_t i = 0; i < 3; i++) {
					IPAddress dns4 = getDns(i);
					if(!dns4.isAny()) debugger->printf_P(PSTR("DNS: %s\n"), dns4.toString().c_str());
				}
            }
			break;
        }
		case ARDUINO_EVENT_WIFI_STA_GOT_IP6: {
            if(debugger->isActive(RemoteDebug::INFO)) {
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
		case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: {
			wifi_err_reason_t reason = (wifi_err_reason_t) info.wifi_sta_disconnected.reason;
			const char* descr = WiFi.disconnectReasonName(reason);
			switch(reason) {
				case WIFI_REASON_ASSOC_LEAVE:
					break;
				default:
					if(strlen(descr) > 0) {
						if(debugger->isActive(RemoteDebug::WARNING)) {
							debugger->printf_P(PSTR("WiFi disconnected, reason %s\n"), descr);
						}
						disconnect(RECONNECT_TIMEOUT);
					}
			}
			break;
		}
    }
}
#endif

bool WiFiClientConnectionHandler::isConfigChanged() {
	return configChanged;
}

void WiFiClientConnectionHandler::getCurrentConfig(NetworkConfig& networkConfig) {
	networkConfig = this->config;
}

IPAddress WiFiClientConnectionHandler::getIP() {
	return WiFi.localIP();
}

IPAddress WiFiClientConnectionHandler::getSubnetMask() {
	return WiFi.subnetMask();
}

IPAddress WiFiClientConnectionHandler::getGateway() {
	return WiFi.gatewayIP();
}

IPAddress WiFiClientConnectionHandler::getDns(uint8_t idx) {
	#if defined(ESP32)
	for(uint8_t i = 0; i < 3; i++) {
		const ip_addr_t * dns = dns_getserver(i);
		if(dns->type == IPADDR_TYPE_V4) {
			if(idx-- == 0) return IPAddress(dns->u_addr.ip4.addr);
		}
	}
	#else
		return WiFi.dnsIP(idx);
	#endif
	return IPAddress();
}

#if defined(ESP32)
IPv6Address WiFiClientConnectionHandler::getIPv6() {
	esp_ip6_addr_t addr;
	if(esp_netif_get_ip6_global(get_esp_interface_netif(ESP_IF_WIFI_STA), &addr) == ESP_OK) {
		return IPv6Address(addr.addr);
	}
	return IPv6Address();
}

IPv6Address WiFiClientConnectionHandler::getDNSv6(uint8_t idx) {
	for(uint8_t i = 0; i < 3; i++) {
		const ip_addr_t * dns = dns_getserver(i);
		if(dns->type == IPADDR_TYPE_V6) {
			if(idx-- == 0) return IPv6Address(dns->u_addr.ip6.addr);
		}
	}
	return IPv6Address();
}
#endif
