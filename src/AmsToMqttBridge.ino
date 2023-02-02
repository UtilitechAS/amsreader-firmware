/**
 *  Copyright (C) 2022  Gunnar Skjold
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * @author Gunnar Skjold (@gskjold) gunnar.skjold@gmail.com
 * 
 * @brief Program for ESP32 and ESP8266 to receive data from AMS electric meters and send to MQTT
 * 
 * @details This program was created to receive data from AMS electric meters via M-Bus, decode 
 * and send to a MQTT broker. The data packet structure supported by this software is specific 
 * to Norwegian meters, but may also support data from electricity providers in other countries. 
 */

#if defined(ESP8266)
ADC_MODE(ADC_VCC);
#endif

#if defined(ESP32)
#include <esp_task_wdt.h>
#endif
#define WDT_TIMEOUT 60

#if defined(CONFIG_IDF_TARGET_ESP32S2)
#include <driver/uart.h>
#endif

#include "version.h"

#include "AmsToMqttBridge.h"
#include "AmsStorage.h"
#include "AmsDataStorage.h"
#include "EnergyAccounting.h"
#include <MQTT.h>
#include <DNSServer.h>
#include <lwip/apps/sntp.h>

#include "hexutils.h"
#include "HwTools.h"

#include "EntsoeApi.h"

#include "AmsWebServer.h"
#include "AmsConfiguration.h"

#include "AmsMqttHandler.h"
#include "JsonMqttHandler.h"
#include "RawMqttHandler.h"
#include "DomoticzMqttHandler.h"
#include "HomeAssistantMqttHandler.h"

#include "Uptime.h"

#include "RemoteDebug.h"

#define BUF_SIZE_COMMON (2048)
#define BUF_SIZE_HAN (1280)

#include "IEC6205621.h"
#include "IEC6205675.h"
#include "LNG.h"

#include "DataParsers.h"
#include "Timezones.h"

uint8_t commonBuffer[BUF_SIZE_COMMON];
uint8_t hanBuffer[BUF_SIZE_HAN];

HwTools hw;

DNSServer* dnsServer = NULL;

AmsConfiguration config;

RemoteDebug Debug;

EntsoeApi* eapi = NULL;

Timezone* tz;

AmsWebServer ws(commonBuffer, &Debug, &hw);

MQTTClient *mqtt = NULL;
WiFiClient *mqttClient = NULL;
WiFiClientSecure *mqttSecureClient = NULL;
AmsMqttHandler* mqttHandler = NULL;

Stream *hanSerial;
SoftwareSerial *swSerial = NULL;

GpioConfig gpioConfig;
MeterConfig meterConfig;
bool mqttEnabled = false;
String topic = "ams";
AmsData meterState;
bool ntpEnabled = false;

AmsDataStorage ds(&Debug);
EnergyAccounting ea(&Debug);

uint8_t wifiReconnectCount = 0;

HDLCParser *hdlcParser = NULL;
MBUSParser *mbusParser = NULL;
GBTParser *gbtParser = NULL;
GCMParser *gcmParser = NULL;
LLCParser *llcParser = NULL;
DLMSParser *dlmsParser = NULL;
DSMRParser *dsmrParser = NULL;

void setup() {
	Serial.begin(115200);

	config.hasConfig(); // Need to run this to make sure all configuration have been migrated before we load GPIO config

	if(!config.getGpioConfig(gpioConfig)) {
		config.clearGpio(gpioConfig);
	}

	delay(1);
	config.loadTempSensors();
	hw.setup(&gpioConfig, &config);

	if(gpioConfig.apPin >= 0) {
		pinMode(gpioConfig.apPin, INPUT_PULLUP);

		if(!hw.ledOn(LED_GREEN)) {
			hw.ledOn(LED_INTERNAL);
		}
		delay(1000);
		if(digitalRead(gpioConfig.apPin) == LOW) {
			if(!hw.ledOn(LED_RED)) {
				hw.ledBlink(LED_INTERNAL, 4);
			}
			delay(2000);
			if(digitalRead(gpioConfig.apPin) == LOW) {
				if(!hw.ledOff(LED_GREEN)) {
					hw.ledOn(LED_INTERNAL);
				}
				delay(2000);
				if(digitalRead(gpioConfig.apPin) == HIGH) {
					config.clear();
					if(!hw.ledBlink(LED_RED, 6)) {
						hw.ledBlink(LED_INTERNAL, 6);
					}
				}
			}
		}
	}

	hw.ledBlink(LED_INTERNAL, 1);
	hw.ledBlink(LED_RED, 1);
	hw.ledBlink(LED_YELLOW, 1);
	hw.ledBlink(LED_GREEN, 1);
	hw.ledBlink(LED_BLUE, 1);

	EntsoeConfig entsoe;
	if(config.getEntsoeConfig(entsoe) && entsoe.enabled && strlen(entsoe.area) > 0) {
		eapi = new EntsoeApi(&Debug);
		eapi->setup(entsoe);
		ws.setEntsoeApi(eapi);
	}
	ws.setPriceRegion(entsoe.area);
	bool shared = false;
	config.getMeterConfig(meterConfig);
	Serial.flush();
	Serial.end();
	if(gpioConfig.hanPin == 3) {
		shared = true;
		#if defined(ESP8266)
			SerialConfig serialConfig;
		#elif defined(ESP32)
			uint32_t serialConfig;
		#endif
		switch(meterConfig.parity) {
			case 2:
				serialConfig = SERIAL_7N1;
				break;
			case 3:
				serialConfig = SERIAL_8N1;
				break;
			case 10:
				serialConfig = SERIAL_7E1;
				break;
			default:
				serialConfig = SERIAL_8E1;
				break;
		}
		#if defined(ESP32)
			Serial.begin(meterConfig.baud == 0 ? 2400 : meterConfig.baud, serialConfig, -1, -1, meterConfig.invert);
		#else
			Serial.begin(meterConfig.baud == 0 ? 2400 : meterConfig.baud, serialConfig, SERIAL_FULL, 1, meterConfig.invert);
		#endif
	}

 	if(!shared) {
		Serial.begin(115200);
	}

	Debug.setSerialEnabled(true);
	delay(1);

	float vcc = hw.getVcc();

	if (Debug.isActive(RemoteDebug::INFO)) {
		debugI("AMS bridge started");
		debugI("Voltage: %.2fV", vcc);
	}

	float vccBootLimit = gpioConfig.vccBootLimit == 0 ? 0 : min(3.29, gpioConfig.vccBootLimit / 10.0); // Make sure it is never above 3.3v
	if(vccBootLimit > 2.5 && vccBootLimit < 3.3 && (gpioConfig.apPin == 0xFF || digitalRead(gpioConfig.apPin) == HIGH)) { // Skip if user is holding AP button while booting (HIGH = button is released)
		if (vcc < vccBootLimit) {
			if(Debug.isActive(RemoteDebug::INFO)) {
				Debug.printf("(setup) Voltage is too low (%.2f < %.2f), sleeping\n", vcc, vccBootLimit);
				Serial.flush();
			}
			ESP.deepSleep(10000000);    //Deep sleep to allow output cap to charge up
		}  
	}

	WiFi.disconnect(true);
	WiFi.softAPdisconnect(true);
	WiFi.mode(WIFI_OFF);

	bool hasFs = false;
#if defined(ESP32)
	debugD("ESP32 LittleFS");
	hasFs = LittleFS.begin(true);
	debugD(" size: %d", LittleFS.totalBytes());
#else
	debugD("ESP8266 LittleFS");
	hasFs = LittleFS.begin();
#endif
	delay(1);

	if(hasFs) {
		#if defined(ESP8266)
			LittleFS.gc();
		#endif
		bool flashed = false;
		if(LittleFS.exists(FILE_FIRMWARE)) {
			if (!config.hasConfig()) {
				debugI("Device has no config, yet a firmware file exists, deleting file.");
			} else if (gpioConfig.apPin == 0xFF || digitalRead(gpioConfig.apPin) == HIGH) {
				if(Debug.isActive(RemoteDebug::INFO)) debugI("Found firmware");
				#if defined(ESP8266)
					WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
					WiFi.forceSleepBegin();
				#endif
				int i = 0;
				while(hw.getVcc() > 1.0 && hw.getVcc() < 3.2 && i < 3) {
					if(Debug.isActive(RemoteDebug::INFO)) debugI(" vcc not optimal, light sleep 10s");
					#if defined(ESP8266)
						delay(10000);
					#elif defined(ESP32)
						esp_sleep_enable_timer_wakeup(10000000);
						esp_light_sleep_start();
					#endif
					i++;
				}

				debugI(" flashing");
				File firmwareFile = LittleFS.open(FILE_FIRMWARE, (char*) "r");
				debugD(" firmware size: %d", firmwareFile.size());
				uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
				debugD(" available: %d", maxSketchSpace);
				if (!Update.begin(maxSketchSpace, U_FLASH)) {
					if(Debug.isActive(RemoteDebug::ERROR)) {
						debugE("Unable to start firmware update");
						Update.printError(Serial);
					}
				} else {
					while (firmwareFile.available()) {
						uint8_t ibuffer[128];
						firmwareFile.read((uint8_t *)ibuffer, 128);
						Update.write(ibuffer, sizeof(ibuffer));
					}
					flashed = Update.end(true);
				}
				firmwareFile.close();
			} else {
				debugW("AP button pressed, skipping firmware update and deleting firmware file.");
			}
			LittleFS.remove(FILE_FIRMWARE);
		} else if(LittleFS.exists(FILE_CFG)) {
			if(Debug.isActive(RemoteDebug::INFO)) debugI("Found config");
			configFileParse();
			flashed = true;
		}
		LittleFS.end();
		if(flashed) {
			if(Debug.isActive(RemoteDebug::INFO)) {
				debugI("Firmware update complete, restarting");
				Serial.flush();
			}
			delay(250);
#if defined(ESP8266)
			ESP.reset();
#elif defined(ESP32)
			ESP.restart();
#endif
			return;
		}
	}
	LittleFS.end();
	delay(1);

	if(config.hasConfig()) {
		if(Debug.isActive(RemoteDebug::INFO)) config.print(&Debug);
		WiFi_connect();
		
		NtpConfig ntp;
		if(config.getNtpConfig(ntp)) {
			tz = resolveTimezone(ntp.timezone);
			configTime(tz->toLocal(0), tz->toLocal(JULY1970)-JULY1970, ntp.enable ? strlen(ntp.server) > 0 ? ntp.server : (char*) F("pool.ntp.org") : (char*) F("")); // Add NTP server by default if none is configured
			sntp_servermode_dhcp(ntp.enable && ntp.dhcp ? 1 : 0);
			ntpEnabled = ntp.enable;
			ws.setTimezone(tz);
			ds.setTimezone(tz);
			ea.setTimezone(tz);
		}

		ds.load();
	} else {
		if(Debug.isActive(RemoteDebug::INFO)) {
			debugI("No configuration, booting AP");
		}
		swapWifiMode();
	}

	EnergyAccountingConfig *eac = new EnergyAccountingConfig();
	if(!config.getEnergyAccountingConfig(*eac)) {
		config.clearEnergyAccountingConfig(*eac);
		config.setEnergyAccountingConfig(*eac);
		config.ackEnergyAccountingChange();
	}
	ea.setup(&ds, eac);
	ea.load();
	ea.setEapi(eapi);
	ws.setup(&config, &gpioConfig, &meterConfig, &meterState, &ds, &ea);

	#if defined(ESP32)
		esp_task_wdt_init(WDT_TIMEOUT, true);
 		esp_task_wdt_add(NULL);
    #elif defined(ESP8266)
		ESP.wdtEnable(WDT_TIMEOUT * 1000);
	#endif
}

int buttonTimer = 0;
bool buttonActive = false;
unsigned long longPressTime = 5000;
bool longPressActive = false;

bool wifiConnected = false;

unsigned long lastTemperatureRead = 0;
unsigned long lastSysupdate = 0;
unsigned long lastErrorBlink = 0; 
int lastError = 0;

bool meterAutodetect = false;
unsigned long meterAutodetectLastChange = 0;
uint8_t meterAutoIndex = 0;
uint32_t bauds[] = { 2400, 2400, 115200, 115200 };
uint8_t parities[] = { 11, 3, 3, 3 };
bool inverts[] = { false, false, false, true };

void loop() {
	Debug.handle();
	unsigned long now = millis();
	if(gpioConfig.apPin != 0xFF) {
		if (digitalRead(gpioConfig.apPin) == LOW) {
			if (buttonActive == false) {
				buttonActive = true;
				buttonTimer = now;
			}

			if ((now - buttonTimer > longPressTime) && (longPressActive == false)) {
				longPressActive = true;
				swapWifiMode();
			}
		} else {
			if (buttonActive == true) {
				if (longPressActive == true) {
					longPressActive = false;
				} else {
					// Single press action
					debugD("Button was clicked, no action configured");
				}
				buttonActive = false;
			}
		}
	}

	if(now > 10000 && now - lastErrorBlink > 3000) {
		errorBlink();
	}

	// Only do normal stuff if we're not booted as AP
	if (WiFi.getMode() != WIFI_AP) {
		if (WiFi.status() != WL_CONNECTED) {
			wifiConnected = false;
			Debug.stop();
			WiFi_connect();
		} else {
			wifiReconnectCount = 0;
			if(!wifiConnected) {
				wifiConnected = true;

				WiFiConfig wifi;
				if(config.getWiFiConfig(wifi)) {
					#if defined(ESP32)
						if(wifi.power >= 195)
							WiFi.setTxPower(WIFI_POWER_19_5dBm);
						else if(wifi.power >= 190)
							WiFi.setTxPower(WIFI_POWER_19dBm);
						else if(wifi.power >= 185)
							WiFi.setTxPower(WIFI_POWER_18_5dBm);
						else if(wifi.power >= 170)
							WiFi.setTxPower(WIFI_POWER_17dBm);
						else if(wifi.power >= 150)
							WiFi.setTxPower(WIFI_POWER_15dBm);
						else if(wifi.power >= 130)
							WiFi.setTxPower(WIFI_POWER_13dBm);
						else if(wifi.power >= 110)
							WiFi.setTxPower(WIFI_POWER_11dBm);
						else if(wifi.power >= 85)
							WiFi.setTxPower(WIFI_POWER_8_5dBm);
						else if(wifi.power >= 70)
							WiFi.setTxPower(WIFI_POWER_7dBm);
						else if(wifi.power >= 50)
							WiFi.setTxPower(WIFI_POWER_5dBm);
						else if(wifi.power >= 20)
							WiFi.setTxPower(WIFI_POWER_2dBm);
						else
							WiFi.setTxPower(WIFI_POWER_MINUS_1dBm);
					#elif defined(ESP8266)
						WiFi.setOutputPower(wifi.power / 10.0);
					#endif
					

					WebConfig web;
					if(config.getWebConfig(web) && web.security > 0) {
						Debug.setPassword(web.password);
					}
					DebugConfig debug;
					if(config.getDebugConfig(debug)) {
						Debug.begin(wifi.hostname, debug.serial || debug.telnet ? (uint8_t) debug.level : RemoteDebug::WARNING); // I don't know why, but ESP8266 stops working after a while if ERROR level is set
						if(!debug.telnet) {
							Debug.stop();
						}
					} else {
						Debug.stop();
					}
					if(Debug.isActive(RemoteDebug::INFO)) {
						debugI("Successfully connected to WiFi!");
						debugI("IP:  %s", WiFi.localIP().toString().c_str());
						debugI("GW:  %s", WiFi.gatewayIP().toString().c_str());
						debugI("DNS: %s", WiFi.dnsIP().toString().c_str());
					}
					if(strlen(wifi.hostname) > 0 && wifi.mdns) {
						debugD("mDNS is enabled, using host: %s", wifi.hostname);
						if(MDNS.begin(wifi.hostname)) {
							MDNS.addService(F("http"), F("tcp"), 80);
						} else {
							debugE("Failed to set up mDNS!");
						}
					}
				}

				MqttConfig mqttConfig;
				if(config.getMqttConfig(mqttConfig)) {
					mqttEnabled = strlen(mqttConfig.host) > 0;
					ws.setMqttEnabled(mqttEnabled);
				}
			}
			if(config.isNtpChanged()) {
				NtpConfig ntp;
				if(config.getNtpConfig(ntp)) {
					tz = resolveTimezone(ntp.timezone);
					configTime(tz->toLocal(0), tz->toLocal(JULY1970)-JULY1970, ntp.enable ? strlen(ntp.server) > 0 ? ntp.server : (char*) F("pool.ntp.org") : (char*) F("")); // Add NTP server by default if none is configured
					sntp_servermode_dhcp(ntp.enable && ntp.dhcp ? 1 : 0);
					ntpEnabled = ntp.enable;

					ws.setTimezone(tz);
					ds.setTimezone(tz);
					ea.setTimezone(tz);
				}

				config.ackNtpChange();
			}
			#if defined ESP8266
			MDNS.update();
			#endif

			if (mqttEnabled || config.isMqttChanged()) {
				if(mqtt == NULL || !mqtt->connected() || config.isMqttChanged()) {
					MQTT_connect();
					config.ackMqttChange();
				}
			} else if(mqtt != NULL && mqtt->connected()) {
				mqttClient->stop();
				mqtt->disconnect();
			}

			try {
				if(eapi != NULL && ntpEnabled) {
					if(eapi->loop() && mqtt != NULL && mqttHandler != NULL && mqtt->connected()) {
						mqttHandler->publishPrices(eapi);
					}
				}
				
				if(config.isEntsoeChanged()) {
					EntsoeConfig entsoe;
					if(config.getEntsoeConfig(entsoe) && entsoe.enabled && strlen(entsoe.area) > 0) {
						if(eapi == NULL) {
							eapi = new EntsoeApi(&Debug);
							ea.setEapi(eapi);
							ws.setEntsoeApi(eapi);
						}
						eapi->setup(entsoe);
					} else if(eapi != NULL) {
						delete eapi;
						eapi = NULL;
						ws.setEntsoeApi(NULL);
					}
					ws.setPriceRegion(entsoe.area);
					config.ackEntsoeChange();
				}
			} catch(const std::exception& e) {
				debugE("Exception in ENTSO-E loop (%s)", e.what());
			}
			ws.loop();
		}
		if(mqtt != NULL) {
			mqtt->loop();
			delay(10); // Needed to preserve power. After adding this, the voltage is super smooth on a HAN powered device
		}
	} else {
		if(dnsServer != NULL) {
			dnsServer->processNextRequest();
		}
		// Continously flash the LED when AP mode
		if (now / 50 % 64 == 0) {
			if(!hw.ledBlink(LED_YELLOW, 1)) {
				hw.ledBlink(LED_INTERNAL, 1);
			}
		}
		ws.loop();
	}

	if(config.isMeterChanged()) {
		config.getMeterConfig(meterConfig);
		setupHanPort(gpioConfig.hanPin, meterConfig.baud, meterConfig.parity, meterConfig.invert);
		config.ackMeterChanged();
		delete gcmParser;
		gcmParser = NULL;
	}

	if(config.isEnergyAccountingChanged()) {
		EnergyAccountingConfig *eac = ea.getConfig();
		config.getEnergyAccountingConfig(*eac);
		ea.setup(&ds, eac);
		config.ackEnergyAccountingChange();
	}
	try {
		if(readHanPort() || now - meterState.getLastUpdateMillis() > 30000) {
			if(now - lastTemperatureRead > 15000) {
				unsigned long start = millis();
				if(hw.updateTemperatures()) {
					lastTemperatureRead = now;

					if(mqtt != NULL && mqttHandler != NULL && WiFi.getMode() != WIFI_AP && WiFi.status() == WL_CONNECTED && mqtt->connected() && !topic.isEmpty()) {
						mqttHandler->publishTemperatures(&config, &hw);
					}
					debugD("Used %ld ms to update temperature", millis()-start);
				}
			}
			if(now - lastSysupdate > 60000) {
				if(mqtt != NULL && mqttHandler != NULL && WiFi.getMode() != WIFI_AP && WiFi.status() == WL_CONNECTED && mqtt->connected() && !topic.isEmpty()) {
					mqttHandler->publishSystem(&hw, eapi, &ea);
				}
				lastSysupdate = now;
			}
		}
	} catch(const std::exception& e) {
		debugE("Exception in readHanPort (%s)", e.what());
		meterState.setLastError(98);
	}
	try {
		if(meterState.getListType() == 0) {
			if(now - meterAutodetectLastChange > 20000 && (meterConfig.baud == 0 || meterConfig.parity == 0)) {
				meterAutodetect = true;
				meterAutoIndex++; // Default is to try the first one in setup()
				debugI("Meter serial autodetect, swapping to: %d, %d, %s", bauds[meterAutoIndex], parities[meterAutoIndex], inverts[meterAutoIndex] ? "true" : "false");
				if(meterAutoIndex >= 4) meterAutoIndex = 0;
				setupHanPort(gpioConfig.hanPin, bauds[meterAutoIndex], parities[meterAutoIndex], inverts[meterAutoIndex]);
				meterAutodetectLastChange = now;
			}
		} else if(meterAutodetect) {
			debugI("Meter serial autodetected, saving: %d, %d, %s", bauds[meterAutoIndex], parities[meterAutoIndex], inverts[meterAutoIndex] ? "true" : "false");
			meterAutodetect = false;
			meterConfig.baud = bauds[meterAutoIndex];
			meterConfig.parity = parities[meterAutoIndex];
			meterConfig.invert = inverts[meterAutoIndex];
			config.setMeterConfig(meterConfig);
			setupHanPort(gpioConfig.hanPin, meterConfig.baud, meterConfig.parity, meterConfig.invert);
		}
	} catch(const std::exception& e) {
		debugE("Exception in meter autodetect (%s)", e.what());
		meterState.setLastError(99);
	}

	delay(1); // Needed for auto modem sleep
	#if defined(ESP32)
		esp_task_wdt_reset();
	#elif defined(ESP8266)
		ESP.wdtFeed();
	#endif
}

void setupHanPort(uint8_t pin, uint32_t baud, uint8_t parityOrdinal, bool invert) {
	if(Debug.isActive(RemoteDebug::INFO)) Debug.printf((char*) F("(setupHanPort) Setting up HAN on pin %d with baud %d and parity %d\n"), pin, baud, parityOrdinal);

	if(baud == 0) {
		baud = bauds[meterAutoIndex];
		parityOrdinal = parities[meterAutoIndex];
		invert = inverts[meterAutoIndex];
	}
	if(parityOrdinal == 0) {
		parityOrdinal = 3; // 8N1
	}

	HardwareSerial *hwSerial = NULL;
	if(pin == 3 || pin == 113) {
		hwSerial = &Serial;
	}

	#if defined(ESP32)
		if(pin == 9) {
			hwSerial = &Serial1;
		}
		#if defined(CONFIG_IDF_TARGET_ESP32)
			if(pin == 16) {
				hwSerial = &Serial2;
			}
		#elif defined(CONFIG_IDF_TARGET_ESP32S2)
			hwSerial = &Serial1;
		#elif defined(CONFIG_IDF_TARGET_ESP32C3)
		#endif
	#endif

	if(pin == 0) {
		debugE("Invalid GPIO configured for HAN");
		return;
	}

	if(hwSerial != NULL) {
		debugD("Hardware serial");
		Serial.flush();
		#if defined(ESP8266)
			SerialConfig serialConfig;
		#elif defined(ESP32)
			uint32_t serialConfig;
		#endif
		switch(parityOrdinal) {
			case 2:
				serialConfig = SERIAL_7N1;
				break;
			case 3:
				serialConfig = SERIAL_8N1;
				break;
			case 10:
				serialConfig = SERIAL_7E1;
				break;
			default:
				serialConfig = SERIAL_8E1;
				break;
		}

		#if defined(CONFIG_IDF_TARGET_ESP32S2)
			hwSerial->begin(baud, serialConfig, -1, -1, invert);
			uart_set_pin(UART_NUM_1, UART_PIN_NO_CHANGE, pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
		#elif defined(ESP32)
			hwSerial->begin(baud, serialConfig, -1, -1, invert);
		#else
			hwSerial->begin(baud, serialConfig, SERIAL_FULL, 1, invert);
		#endif
		
		#if defined(ESP8266)
			if(pin == 3) {
				debugI("Switching UART0 to pin 1 & 3");
				Serial.pins(1,3);
			} else if(pin == 113) {
				debugI("Switching UART0 to pin 15 & 13");
				Serial.pins(15,13);
			}
		#endif

		hanSerial = hwSerial;
	} else {
		debugD("Software serial");
		Serial.flush();
		
		if(swSerial != NULL) {
			swSerial->end();
			delete swSerial;
		}
		swSerial = new SoftwareSerial(pin);

		SoftwareSerialConfig serialConfig;
		switch(parityOrdinal) {
			case 2:
				serialConfig = SWSERIAL_7N1;
				break;
			case 3:
				serialConfig = SWSERIAL_8N1;
				break;
			case 10:
				serialConfig = SWSERIAL_7E1;
				break;
			default:
				serialConfig = SWSERIAL_8E1;
				break;
		}

		SoftwareSerial *swSerial = new SoftwareSerial(pin, -1, invert);
		swSerial->begin(baud, serialConfig);
		hanSerial = swSerial;

		Serial.end();
		Serial.begin(115200);
	}

	// Empty buffer before starting
	while (hanSerial->available() > 0) {
		hanSerial->read();
	}
}

void errorBlink() {
	if(lastError == 3)
		lastError = 0;
	lastErrorBlink = millis();
	while(lastError < 3) {
		switch(lastError++) {
			case 0:
				if(lastErrorBlink - meterState.getLastUpdateMillis() > 30000) {
					debugW("No HAN data received last 30s, single blink");
					hw.ledBlink(LED_RED, 1); // If no message received from AMS in 30 sec, blink once
					if(meterState.getLastError() == 0) meterState.setLastError(90);
					return;
				}
				break;
			case 1:
				if(mqttEnabled && mqtt != NULL && mqtt->lastError() != 0) {
					debugW("MQTT connection not available, double blink");
					hw.ledBlink(LED_RED, 2); // If MQTT error, blink twice
					return;
				}
				break;
			case 2:
				if(WiFi.getMode() != WIFI_AP && WiFi.status() != WL_CONNECTED) {
					debugW("WiFi not connected, tripe blink");
					hw.ledBlink(LED_RED, 3); // If WiFi not connected, blink three times
					return;
				}
				break;
		}
	}
}

void swapWifiMode() {
	if(!hw.ledOn(LED_YELLOW)) {
		hw.ledOn(LED_INTERNAL);
	}
	WiFiMode_t mode = WiFi.getMode();
	if(dnsServer != NULL) {
		dnsServer->stop();
	}
	WiFi.disconnect(true);
	WiFi.softAPdisconnect(true);
	WiFi.mode(WIFI_OFF);
	yield();

	if (mode != WIFI_AP || !config.hasConfig()) {
		if(Debug.isActive(RemoteDebug::INFO)) debugI("Swapping to AP mode");

		//wifi_softap_set_dhcps_offer_option(OFFER_ROUTER, 0); // Disable default gw

		/* Example code to set captive portal option in DHCP
		auto& server = WiFi.softAPDhcpServer();
		server.onSendOptions([](const DhcpServer& server, auto& options) {
			// Captive Portal URI
			const IPAddress gateway = netif_ip4_addr(server.getNetif());
			const String captive = F("http://") + gateway.toString();
			options.add(114, captive.c_str(), captive.length());
		});
		*/

		WiFi.softAP(PSTR("AMS2MQTT"));
		WiFi.mode(WIFI_AP);

		if(dnsServer == NULL) {
			dnsServer = new DNSServer();
		}
		dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
		dnsServer->start(53, PSTR("*"), WiFi.softAPIP());
		#if defined(DEBUG_MODE)
			Debug.setSerialEnabled(true);
			Debug.begin("192.168.4.1", 23, RemoteDebug::VERBOSE);
		#endif
	} else {
		if(Debug.isActive(RemoteDebug::INFO)) debugI("Swapping to STA mode");
		if(dnsServer != NULL) {
			delete dnsServer;
			dnsServer = NULL;
		}
		WiFi_connect();
	}
	delay(500);
	if(!hw.ledOff(LED_YELLOW)) {
		hw.ledOff(LED_INTERNAL);
	}
}

int len = 0;
bool serialInit = false;
bool readHanPort() {
	if(!hanSerial->available()) return false;

	// Before reading, empty serial buffer to increase chance of getting first byte of a data transfer
	if(!serialInit) {
		hanSerial->readBytes(hanBuffer, BUF_SIZE_HAN);
		serialInit = true;
		return false;
	}

	DataParserContext ctx = {0};
	int pos = DATA_PARSE_INCOMPLETE;
	// For each byte received, check if we have a complete frame we can handle
	while(hanSerial->available() && pos == DATA_PARSE_INCOMPLETE) {
		// If buffer was overflowed, reset
		if(len >= BUF_SIZE_HAN) {
			hanSerial->readBytes(hanBuffer, BUF_SIZE_HAN);
			len = 0;
			debugI("Buffer overflow, resetting");
			return false;
		}
		hanBuffer[len++] = hanSerial->read();
		ctx.length = len;
		pos = unwrapData((uint8_t *) hanBuffer, ctx);
		if(ctx.type > 0 && pos >= 0) {
			if(ctx.type == DATA_TAG_DLMS) {
				debugD("Received valid DLMS at %d", pos);
			} else if(ctx.type == DATA_TAG_DSMR) {
				debugD("Received valid DSMR at %d", pos);
			} else {
				// TODO: Move this so that payload is sent to MQTT
				debugE("Unknown tag %02X at pos %d", ctx.type, pos);
				len = 0;
				return false;
			}
		}
	}
	if(pos == DATA_PARSE_INCOMPLETE) {
		return false;
	} else if(pos == DATA_PARSE_UNKNOWN_DATA) {
		meterState.setLastError(pos);
		debugV("Unknown data payload:");
		len = len + hanSerial->readBytes(hanBuffer+len, BUF_SIZE_HAN-len);
		if(Debug.isActive(RemoteDebug::VERBOSE)) debugPrint(hanBuffer, 0, len);
		len = 0;
		return false;
	}

	if(pos == DATA_PARSE_INTERMEDIATE_SEGMENT) {
		len = 0;
		return false;
	} else if(pos < 0) {
		meterState.setLastError(pos);
		printHanReadError(pos);
		len += hanSerial->readBytes(hanBuffer+len, BUF_SIZE_HAN-len);
		if(mqttEnabled && mqtt != NULL && mqttHandler == NULL) {
			mqtt->publish(topic.c_str(), toHex(hanBuffer+pos, len));
			mqtt->loop();
		}
		while(hanSerial->available()) hanSerial->read(); // Make sure it is all empty, in case we overflowed buffer above
		len = 0;
		return false;
	}

	// Data is valid, clear the rest of the buffer to avoid tainted parsing
	for(int i = pos+ctx.length; i<BUF_SIZE_HAN; i++) {
		hanBuffer[i] = 0x00;
	}
	meterState.setLastError(DATA_PARSE_OK);

	AmsData data;
	char* payload = ((char *) (hanBuffer)) + pos;
	if(ctx.type == DATA_TAG_DLMS) {
		// If MQTT bytestream payload is selected (mqttHandler == NULL), send the payload to MQTT
		if(mqttEnabled && mqtt != NULL && mqttHandler == NULL) {
			mqtt->publish(topic.c_str(), toHex((byte*) payload, ctx.length));
			mqtt->loop();
		}

		debugV("Using application data:");
		if(Debug.isActive(RemoteDebug::VERBOSE)) debugPrint((byte*) payload, 0, ctx.length);

		// Rudimentary detector for L&G proprietary format
		if(payload[0] == CosemTypeStructure && payload[2] == CosemTypeArray && payload[1] == payload[3]) {
			data = LNG(payload, meterState.getMeterType(), &meterConfig, ctx, &Debug);
		} else {
			// TODO: Split IEC6205675 into DataParserKaifa and DataParserObis. This way we can add other means of parsing, for those other proprietary formats
			data = IEC6205675(payload, meterState.getMeterType(), &meterConfig, ctx);
		}
	} else if(ctx.type == DATA_TAG_DSMR) {
		data = IEC6205621(payload);
	}
	len = 0;

	if(data.getListType() > 0) {
		if(!hw.ledBlink(LED_GREEN, 1))
			hw.ledBlink(LED_INTERNAL, 1);
		if(mqttEnabled && mqttHandler != NULL && mqtt != NULL) {
			if(mqttHandler->publish(&data, &meterState, &ea)) {
				mqtt->loop();
				delay(10);
			}
		}

		time_t now = time(nullptr);
		if(now < BUILD_EPOCH && data.getListType() >= 3) {
			if(data.getMeterTimestamp() > BUILD_EPOCH) {
				debugI("Using timestamp from meter");
				now = data.getMeterTimestamp();
			} else if(data.getPackageTimestamp() > BUILD_EPOCH) {
				debugI("Using timestamp from meter (DLMS)");
				now = data.getPackageTimestamp();
			}
			if(now > BUILD_EPOCH) {
				timeval tv { now, 0};
				settimeofday(&tv, nullptr);
			}
		}
		if(meterState.getListType() < 3 && now > BUILD_EPOCH) {
			// TODO: Load an estimated value from dayplot
		}

		meterState.apply(data);

		bool saveData = false;
		if(!ds.isHappy() && now > BUILD_EPOCH) {
			debugD("Its time to update data storage");
			tmElements_t tm;
			breakTime(now, tm);
			if(tm.Minute == 0) {
				debugV(" using actual data");
				saveData = ds.update(&data);
			} else if(meterState.getListType() >= 3) {
				debugV(" using estimated data");
				saveData = ds.update(&meterState);
			}
			if(saveData) {
				debugI("Saving data");
				ds.save();
			}
		}

		if(ea.update(&data)) {
			debugI("Saving energy accounting");
			ea.save();
		}
	}
	delay(1);
	return true;
}

void printHanReadError(int pos) {
	if(Debug.isActive(RemoteDebug::WARNING)) {
		switch(pos) {
			case DATA_PARSE_BOUNDRY_FLAG_MISSING:
				debugW("Boundry flag missing");
				break;
			case DATA_PARSE_HEADER_CHECKSUM_ERROR:
				debugW("Header checksum error");
				break;
			case DATA_PARSE_FOOTER_CHECKSUM_ERROR:
				debugW("Frame checksum error");
				break;
			case DATA_PARSE_INCOMPLETE:
				debugW("Received frame is incomplete");
				break;
			case GCM_AUTH_FAILED:
				debugW("Decrypt authentication failed");
				break;
			case GCM_ENCRYPTION_KEY_FAILED:
				debugW("Setting decryption key failed");
				break;
			case GCM_DECRYPT_FAILED:
				debugW("Decryption failed");
				break;
			case MBUS_FRAME_LENGTH_NOT_EQUAL:
				debugW("Frame length mismatch");
				break;
			case DATA_PARSE_INTERMEDIATE_SEGMENT:
				debugI("Intermediate segment received");
				break;
			case DATA_PARSE_UNKNOWN_DATA:
				debugW("Unknown data format %02X", hanBuffer[0]);
				break;
			default:
				debugW("Unspecified error while reading data: %d", pos);
		}
	}
}

void debugPrint(byte *buffer, int start, int length) {
	for (int i = start; i < start + length; i++) {
		if (buffer[i] < 0x10)
			Debug.print(F("0"));
		Debug.print(buffer[i], HEX);
		Debug.print(F(" "));
		if ((i - start + 1) % 16 == 0)
			Debug.println("");
		else if ((i - start + 1) % 4 == 0)
			Debug.print(F(" "));

		yield(); // Let other get some resources too
	}
	Debug.println("");
}

unsigned long wifiTimeout = WIFI_CONNECTION_TIMEOUT;
unsigned long lastWifiRetry = -WIFI_CONNECTION_TIMEOUT;
void WiFi_connect() {
	if(millis() - lastWifiRetry < wifiTimeout) {
		delay(50);
		return;
	}
	lastWifiRetry = millis();

	if (WiFi.status() != WL_CONNECTED) {
		WiFiConfig wifi;
		if(!config.getWiFiConfig(wifi) || strlen(wifi.ssid) == 0) {
			swapWifiMode();
			return;
		}

		if(WiFi.getMode() != WIFI_OFF) {
			if(wifiReconnectCount > 3 && wifi.autoreboot) {
				ESP.restart();
				return;
			}
			if (Debug.isActive(RemoteDebug::INFO)) debugI("Not connected to WiFi, closing resources");
			if(mqtt != NULL) {
				mqtt->disconnect();
				mqtt->loop();
				yield();
				delete mqtt;
				mqtt = NULL;
				ws.setMqtt(NULL);
			}

			if(mqttClient != NULL) {
				mqttClient->stop();
				delete mqttClient;
				mqttClient = NULL;
				if(mqttSecureClient != NULL) {
					mqttSecureClient = NULL;
				}
			}

			#if defined(ESP8266)
				WiFiClient::stopAll();
			#endif

			MDNS.end();
			WiFi.disconnect(true);
			WiFi.softAPdisconnect(true);
			WiFi.enableAP(false);
			WiFi.mode(WIFI_OFF);
			yield();
			wifiTimeout = 5000;
			return;
		}
		wifiTimeout = WIFI_CONNECTION_TIMEOUT;

		if (Debug.isActive(RemoteDebug::INFO)) debugI("Connecting to WiFi network: %s", wifi.ssid);

		wifiReconnectCount++;

		#if defined(ESP32)
			if(strlen(wifi.hostname) > 0) {
				WiFi.setHostname(wifi.hostname);
			}	
		#endif
		WiFi.mode(WIFI_STA);

		if(strlen(wifi.ip) > 0) {
			IPAddress ip, gw, sn(255,255,255,0), dns1, dns2;
			ip.fromString(wifi.ip);
			gw.fromString(wifi.gateway);
			sn.fromString(wifi.subnet);
			if(strlen(wifi.dns1) > 0) {
				dns1.fromString(wifi.dns1);
			} else if(strlen(wifi.gateway) > 0) {
				dns1.fromString(wifi.gateway); // If no DNS, set gateway by default
			}
			if(strlen(wifi.dns2) > 0) {
				dns2.fromString(wifi.dns2);
			} else if(dns1.toString().isEmpty()) {
				dns2.fromString(F("208.67.220.220")); // Add OpenDNS as second by default if nothing is configured
			}
			if(!WiFi.config(ip, gw, sn, dns1, dns2)) {
				debugE("Static IP configuration is invalid, not using");
			}
		} else {
			#if defined(ESP32)
			// This trick does not work anymore...
			// WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE); // Workaround to make DHCP hostname work for ESP32. See: https://github.com/espressif/arduino-esp32/issues/2537
			#endif
		}
		#if defined(ESP8266)
			if(strlen(wifi.hostname) > 0) {
				WiFi.hostname(wifi.hostname);
			}	
		#endif
		#if defined(ESP32)
			WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
			WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
		#endif
		WiFi.setAutoReconnect(true);
		if(WiFi.begin(wifi.ssid, wifi.psk)) {
			if(wifi.sleep <= 2) {
				switch(wifi.sleep) {
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
			if (Debug.isActive(RemoteDebug::ERROR)) debugI("Unable to start WiFi");
		}
  	}
}

void mqttMessageReceived(String &topic, String &payload) {
    debugI("Received message for topic %s", topic.c_str() );
	//if(meterConfig.source == METER_SOURCE_MQTT) {
		//DataParserContext ctx = {static_cast<uint8_t>(payload.length()/2)};
		//fromHex(hanBuffer, payload, ctx.length);
		//uint16_t pos = unwrapData(hanBuffer, ctx);
		// TODO: Run through DLMS/DMSR parser and apply AmsData
	//}
}

int16_t unwrapData(uint8_t *buf, DataParserContext &context) {
	int16_t ret = 0;
	bool doRet = false;
	uint16_t end = BUF_SIZE_HAN;
	uint8_t tag = (*buf);
	uint8_t lastTag = DATA_TAG_NONE;
	while(tag != DATA_TAG_NONE) {
		int16_t curLen = context.length;
		int8_t res = 0;
		switch(tag) {
			case DATA_TAG_HDLC:
				if(hdlcParser == NULL) hdlcParser = new HDLCParser();
				res = hdlcParser->parse(buf, context);
				break;
			case DATA_TAG_MBUS:
				if(mbusParser == NULL) mbusParser =  new MBUSParser();
				res = mbusParser->parse(buf, context);
				break;
			case DATA_TAG_GBT:
				if(gbtParser == NULL) gbtParser = new GBTParser();
				res = gbtParser->parse(buf, context);
				break;
			case DATA_TAG_GCM:
				if(gcmParser == NULL) gcmParser = new GCMParser(meterConfig.encryptionKey, meterConfig.authenticationKey);
				res = gcmParser->parse(buf, context);
				break;
			case DATA_TAG_LLC:
				if(llcParser == NULL) llcParser = new LLCParser();
				res = llcParser->parse(buf, context);
				break;
			case DATA_TAG_DLMS:
				if(dlmsParser == NULL) dlmsParser = new DLMSParser();
				res = dlmsParser->parse(buf, context);
				if(res >= 0) doRet = true;
				break;
			case DATA_TAG_DSMR:
				if(dsmrParser == NULL) dsmrParser = new DSMRParser();
				res = dsmrParser->parse(buf, context, lastTag != DATA_TAG_NONE);
				if(res >= 0) doRet = true;
				break;
			default:
				debugE("Ended up in default case while unwrapping...(tag %02X)", tag);
				return DATA_PARSE_UNKNOWN_DATA;
		}
		lastTag = tag;
		if(res == DATA_PARSE_INCOMPLETE) {
			return res;
		}
		if(context.length > end) return false;
		if(Debug.isActive(RemoteDebug::VERBOSE)) {
			switch(tag) {
				case DATA_TAG_HDLC:
					debugV("HDLC frame:");
					// If MQTT bytestream payload is selected (mqttHandler == NULL), send the payload to MQTT
					if(mqttEnabled && mqtt != NULL && mqttHandler == NULL) {
						mqtt->publish(topic.c_str(), toHex(buf, curLen));
						mqtt->loop();
					}
					break;
				case DATA_TAG_MBUS:
					debugV("MBUS frame:");
					// If MQTT bytestream payload is selected (mqttHandler == NULL), send the payload to MQTT
					if(mqttEnabled && mqtt != NULL && mqttHandler == NULL) {
						mqtt->publish(topic.c_str(), toHex(buf, curLen));
						mqtt->loop();
					}
					break;
				case DATA_TAG_GBT:
					debugV("GBT frame:");
					break;
				case DATA_TAG_GCM:
					debugV("GCM frame:");
					break;
				case DATA_TAG_LLC:
					debugV("LLC frame:");
					break;
				case DATA_TAG_DLMS:
					debugV("DLMS frame:");
					break;
				case DATA_TAG_DSMR:
					debugV("DSMR frame:");
					if(mqttEnabled && mqtt != NULL && mqttHandler == NULL) {
						mqtt->publish(topic.c_str(), (char*) buf);
						mqtt->loop();
					}
					break;
			}
			debugPrint(buf, 0, curLen);
		}
		if(res == DATA_PARSE_FINAL_SEGMENT) {
			if(tag == DATA_TAG_MBUS) {
				res = mbusParser->write(buf, context);
			}
		}

		if(res < 0) {
			return res;
		}
		buf += res;
		end -= res;
		ret += res;

		// If we are ready to return, do that
		if(doRet) {
			context.type = tag;
			return ret;
		}

		// Use start byte of new buffer position as tag for next round in loop
		tag = (*buf);
	}
	debugE("Got to end of unwrap method...");
	return DATA_PARSE_UNKNOWN_DATA;
}

unsigned long lastMqttRetry = -10000;
void MQTT_connect() {
	MqttConfig mqttConfig;
	if(!config.getMqttConfig(mqttConfig) || strlen(mqttConfig.host) == 0) {
		if(Debug.isActive(RemoteDebug::WARNING)) debugW("No MQTT config");
		mqttEnabled = false;
		ws.setMqttEnabled(false);
		return;
	}
	if(mqtt != NULL) {
		if(millis() - lastMqttRetry < (mqtt->lastError() == 0 || config.isMqttChanged() ? 5000 : 30000)) {
			yield();
			return;
		}
		lastMqttRetry = millis();

		if(Debug.isActive(RemoteDebug::INFO)) {
			debugD("Disconnecting MQTT before connecting");
		}

		mqtt->disconnect();
		if(config.isMqttChanged()) {
			if(mqttSecureClient != NULL) {
				mqttSecureClient->stop();
				delete mqttSecureClient;
				mqttSecureClient = NULL;
			} else {
				mqttClient->stop();
			}
			mqttClient = NULL;
		}
		yield();
	} else {
		mqtt = new MQTTClient(1024);
		ws.setMqtt(mqtt);
	}

	mqttEnabled = true;
	ws.setMqttEnabled(true);
	topic = String(mqttConfig.publishTopic);

	if(mqttHandler != NULL) {
		delete mqttHandler;
		mqttHandler = NULL;
	}

	switch(mqttConfig.payloadFormat) {
		case 0:
			mqttHandler = new JsonMqttHandler(mqtt, (char*) commonBuffer, mqttConfig.clientId, mqttConfig.publishTopic, &hw);
			break;
		case 1:
		case 2:
			mqttHandler = new RawMqttHandler(mqtt, (char*) commonBuffer, mqttConfig.publishTopic, mqttConfig.payloadFormat == 2);
			break;
		case 3:
			DomoticzConfig domo;
			config.getDomoticzConfig(domo);
			mqttHandler = new DomoticzMqttHandler(mqtt, (char*) commonBuffer, domo);
			break;
		case 4:
			mqttHandler = new HomeAssistantMqttHandler(mqtt, (char*) commonBuffer, mqttConfig.clientId, mqttConfig.publishTopic, &hw);
			break;
	}

	time_t epoch = time(nullptr);
	if(mqttConfig.ssl) {
		if(epoch < BUILD_EPOCH) {
			debugI("NTP not ready for MQTT SSL");
			return;
		}
		debugI("MQTT SSL is configured (%dkb free heap)", ESP.getFreeHeap());
		if(mqttSecureClient == NULL) {
			mqttSecureClient = new WiFiClientSecure();
			#if defined(ESP8266)
				mqttSecureClient->setBufferSizes(512, 512);
				debugD("ESP8266 firmware does not have enough memory...");
				return;
			#endif
		
			if(LittleFS.begin()) {
				File file;

				if(LittleFS.exists(FILE_MQTT_CA)) {
					debugI("Found MQTT CA file (%dkb free heap)", ESP.getFreeHeap());
					file = LittleFS.open(FILE_MQTT_CA, (char*) "r");
					#if defined(ESP8266)
						BearSSL::X509List *serverTrustedCA = new BearSSL::X509List(file);
						mqttSecureClient->setTrustAnchors(serverTrustedCA);
					#elif defined(ESP32)
						mqttSecureClient->loadCACert(file, file.size());
					#endif
					file.close();

					if(LittleFS.exists(FILE_MQTT_CERT) && LittleFS.exists(FILE_MQTT_KEY)) {
						#if defined(ESP8266)
							debugI("Found MQTT certificate file (%dkb free heap)", ESP.getFreeHeap());
							file = LittleFS.open(FILE_MQTT_CERT, (char*) "r");
							BearSSL::X509List *serverCertList = new BearSSL::X509List(file);
							file.close();

							debugI("Found MQTT key file (%dkb free heap)", ESP.getFreeHeap());
							file = LittleFS.open(FILE_MQTT_KEY, (char*) "r");
							BearSSL::PrivateKey *serverPrivKey = new BearSSL::PrivateKey(file);
							file.close();

							debugD("Setting client certificates (%dkb free heap)", ESP.getFreeHeap());
							mqttSecureClient->setClientRSACert(serverCertList, serverPrivKey);
						#elif defined(ESP32)
							debugI("Found MQTT certificate file (%dkb free heap)", ESP.getFreeHeap());
							file = LittleFS.open(FILE_MQTT_CERT, (char*) "r");
							mqttSecureClient->loadCertificate(file, file.size());
							file.close();

							debugI("Found MQTT key file (%dkb free heap)", ESP.getFreeHeap());
							file = LittleFS.open(FILE_MQTT_KEY, (char*) "r");
							mqttSecureClient->loadPrivateKey(file, file.size());
							file.close();
						#endif
						mqttClient = mqttSecureClient;
					}
				}

				LittleFS.end();
				debugD("MQTT SSL setup complete (%dkb free heap)", ESP.getFreeHeap());
			}
		}
	}
	
	if(mqttClient == NULL) {
		mqttClient = new WiFiClient();
	}

	if(Debug.isActive(RemoteDebug::INFO)) {
		debugI("Connecting to MQTT %s:%d", mqttConfig.host, mqttConfig.port);
	}
	
	mqtt->begin(mqttConfig.host, mqttConfig.port, *mqttClient);

	#if defined(ESP8266)
		if(mqttSecureClient) {
			time_t epoch = time(nullptr);
			debugD("Setting NTP time %lu for secure MQTT connection", epoch);
			mqttSecureClient->setX509Time(epoch);
		}
	#endif

	// Connect to a unsecure or secure MQTT server
	if ((strlen(mqttConfig.username) == 0 && mqtt->connect(mqttConfig.clientId)) ||
		(strlen(mqttConfig.username) > 0 && mqtt->connect(mqttConfig.clientId, mqttConfig.username, mqttConfig.password))) {
		if (Debug.isActive(RemoteDebug::INFO)) debugI("Successfully connected to MQTT!");
		
		if(mqttHandler != NULL) {
			mqttHandler->publishSystem(&hw, eapi, &ea);
		}

		// Subscribe to the chosen MQTT topic, if set in configuration
		if (strlen(mqttConfig.subscribeTopic) > 0) {
            mqtt->onMessage(mqttMessageReceived);
			mqtt->subscribe(String(mqttConfig.subscribeTopic) + "/#");
			debugI("  Subscribing to [%s]\n", mqttConfig.subscribeTopic);
		}
	} else {
		if (Debug.isActive(RemoteDebug::ERROR)) {
			debugE("Failed to connect to MQTT: %d", mqtt->lastError());
			#if defined(ESP8266)
				if(mqttSecureClient) {
					mqttSecureClient->getLastSSLError((char*) commonBuffer, BUF_SIZE_COMMON);
					Debug.println((char*) commonBuffer);
				}
			#endif
		}
	}
	yield();
}

void configFileParse() {
	debugD("Parsing config file");

	if(!LittleFS.exists(FILE_CFG)) {
		debugW("Config file does not exist");
		return;
	}

	File file = LittleFS.open(FILE_CFG, (char*) "r");

	bool lSys = false;
	bool lWiFi = false;
	bool lMqtt = false;
	bool lWeb = false;
	bool lMeter = false;
	bool lGpio = false;
	bool lDomo = false;
	bool lNtp = false;
	bool lEntsoe = false;
	bool lEac = false;
	bool sEa = false;
	bool sDs = false;

	SystemConfig sys;
	WiFiConfig wifi;
	MqttConfig mqtt;
	WebConfig web;
	MeterConfig meter;
	GpioConfig gpio;
	DomoticzConfig domo;
	NtpConfig ntp;
	EntsoeConfig entsoe;
	EnergyAccountingConfig eac;

	size_t size;
	char* buf = (char*) commonBuffer;
	memset(buf, 0, 1024);
	while((size = file.readBytesUntil('\n', buf, 1024)) > 0) {
		for(uint16_t i = 0; i < size; i++) {
			if(buf[i] < 32 || buf[i] > 126) {
				memset(buf+i, 0, size-i);
				debugD("Found non-ascii, shortening line from %d to %d", size, i);
				size = i;
				break;
			}
		}
		if(strncmp_P(buf, PSTR("boardType "), 10) == 0) {
			if(!lSys) { config.getSystemConfig(sys); lSys = true; };
			sys.boardType = String(buf+10).toInt();
		} else if(strncmp_P(buf, PSTR("ssid "), 5) == 0) {
			if(!lWiFi) { config.getWiFiConfig(wifi); lWiFi = true; };
			strcpy(wifi.ssid, buf+5);
		} else if(strncmp_P(buf, PSTR("psk "), 4) == 0) {
			if(!lWiFi) { config.getWiFiConfig(wifi); lWiFi = true; };
			strcpy(wifi.psk, buf+4);
		} else if(strncmp_P(buf, PSTR("ip "), 3) == 0) {
			if(!lWiFi) { config.getWiFiConfig(wifi); lWiFi = true; };
			strcpy(wifi.ip, buf+3);
		} else if(strncmp_P(buf, PSTR("gateway "), 8) == 0) {
			if(!lWiFi) { config.getWiFiConfig(wifi); lWiFi = true; };
			strcpy(wifi.gateway, buf+8);
		} else if(strncmp_P(buf, PSTR("subnet "), 7) == 0) {
			if(!lWiFi) { config.getWiFiConfig(wifi); lWiFi = true; };
			strcpy(wifi.subnet, buf+7);
		} else if(strncmp_P(buf, PSTR("dns1 "), 5) == 0) {
			if(!lWiFi) { config.getWiFiConfig(wifi); lWiFi = true; };
			strcpy(wifi.dns1, buf+5);
		} else if(strncmp_P(buf, PSTR("dns2 "), 5) == 0) {
			if(!lWiFi) { config.getWiFiConfig(wifi); lWiFi = true; };
			strcpy(wifi.dns2, buf+5);
		} else if(strncmp_P(buf, PSTR("hostname "), 9) == 0) {
			if(!lWiFi) { config.getWiFiConfig(wifi); lWiFi = true; };
			strcpy(wifi.hostname, buf+9);
		} else if(strncmp_P(buf, PSTR("mdns "), 5) == 0) {
			if(!lWiFi) { config.getWiFiConfig(wifi); lWiFi = true; };
			wifi.mdns = String(buf+5).toInt() == 1;;
		} else if(strncmp_P(buf, PSTR("mqttHost "), 9) == 0) {
			if(!lMqtt) { config.getMqttConfig(mqtt); lMqtt = true; };
			strcpy(mqtt.host, buf+9);
		} else if(strncmp_P(buf, PSTR("mqttPort "), 9) == 0) {
			if(!lMqtt) { config.getMqttConfig(mqtt); lMqtt = true; };
			mqtt.port = String(buf+9).toInt();
		} else if(strncmp_P(buf, PSTR("mqttClientId "), 13) == 0) {
			if(!lMqtt) { config.getMqttConfig(mqtt); lMqtt = true; };
			strcpy(mqtt.clientId, buf+13);
		} else if(strncmp_P(buf, PSTR("mqttPublishTopic "), 17) == 0) {
			if(!lMqtt) { config.getMqttConfig(mqtt); lMqtt = true; };
			strcpy(mqtt.publishTopic, buf+17);
		} else if(strncmp_P(buf, PSTR("mqttUsername "), 13) == 0) {
			if(!lMqtt) { config.getMqttConfig(mqtt); lMqtt = true; };
			strcpy(mqtt.username, buf+13);
		} else if(strncmp_P(buf, PSTR("mqttPassword "), 13) == 0) {
			if(!lMqtt) { config.getMqttConfig(mqtt); lMqtt = true; };
			strcpy(mqtt.password, buf+13);
		} else if(strncmp_P(buf, PSTR("mqttPayloadFormat "), 18) == 0) {
			if(!lMqtt) { config.getMqttConfig(mqtt); lMqtt = true; };
			mqtt.payloadFormat = String(buf+18).toInt();
		} else if(strncmp_P(buf, PSTR("mqttSsl "), 8) == 0) {
			if(!lMqtt) { config.getMqttConfig(mqtt); lMqtt = true; };
			mqtt.ssl = String(buf+8).toInt() == 1;;
		} else if(strncmp_P(buf, PSTR("webSecurity "), 12) == 0) {
			if(!lWeb) { config.getWebConfig(web); lWeb = true; };
			web.security = String(buf+12).toInt();
		} else if(strncmp_P(buf, PSTR("webUsername "), 12) == 0) {
			if(!lWeb) { config.getWebConfig(web); lWeb = true; };
			strcpy(web.username, buf+12);
		} else if(strncmp_P(buf, PSTR("webPassword "), 12) == 0) {
			if(!lWeb) { config.getWebConfig(web); lWeb = true; };
			strcpy(web.username, buf+12);
		} else if(strncmp_P(buf, PSTR("meterBaud "), 10) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			meter.baud = String(buf+10).toInt();
		} else if(strncmp_P(buf, PSTR("meterParity "), 12) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			if(strncmp_P(buf+12, PSTR("7N1"), 3) == 0) meter.parity = 2;
			if(strncmp_P(buf+12, PSTR("8N1"), 3) == 0) meter.parity = 3;
			if(strncmp_P(buf+12, PSTR("7E1"), 3) == 0) meter.parity = 10;
			if(strncmp_P(buf+12, PSTR("8E1"), 3) == 0) meter.parity = 11;
		} else if(strncmp_P(buf, PSTR("meterInvert "), 12) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			meter.invert = String(buf+12).toInt() == 1;;
		} else if(strncmp_P(buf, PSTR("meterDistributionSystem "), 24) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			meter.distributionSystem = String(buf+24).toInt();
		} else if(strncmp_P(buf, PSTR("meterMainFuse "), 14) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			meter.mainFuse = String(buf+14).toInt();
		} else if(strncmp_P(buf, PSTR("meterProductionCapacity "), 24) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			meter.productionCapacity = String(buf+24).toInt();
		} else if(strncmp_P(buf, PSTR("meterEncryptionKey "), 19) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			fromHex(meter.encryptionKey, String(buf+19), 16);
		} else if(strncmp_P(buf, PSTR("meterAuthenticationKey "), 23) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			fromHex(meter.authenticationKey, String(buf+23), 16);
		} else if(strncmp_P(buf, PSTR("gpioHanPin "), 11) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.hanPin = String(buf+11).toInt();
		} else if(strncmp_P(buf, PSTR("gpioApPin "), 10) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.apPin = String(buf+10).toInt();
		} else if(strncmp_P(buf, PSTR("gpioLedPin "), 11) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.ledPin = String(buf+11).toInt();
		} else if(strncmp_P(buf, PSTR("gpioLedInverted "), 16) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.ledInverted = String(buf+16).toInt() == 1;
		} else if(strncmp_P(buf, PSTR("gpioLedPinRed "), 14) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.ledPinRed = String(buf+14).toInt();
		} else if(strncmp_P(buf, PSTR("gpioLedPinGreen "), 16) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.ledPinGreen = String(buf+16).toInt();
		} else if(strncmp_P(buf, PSTR("gpioLedPinBlue "), 15) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.ledPinBlue = String(buf+15).toInt();
		} else if(strncmp_P(buf, PSTR("gpioLedRgbInverted "), 19) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.ledRgbInverted = String(buf+19).toInt() == 1;
		} else if(strncmp_P(buf, PSTR("gpioTempSensorPin "), 18) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.tempSensorPin = String(buf+18).toInt();
		} else if(strncmp_P(buf, PSTR("gpioTempAnalogSensorPin "), 24) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.tempAnalogSensorPin = String(buf+24).toInt();
		} else if(strncmp_P(buf, PSTR("gpioVccPin "), 11) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.vccPin = String(buf+11).toInt();
		} else if(strncmp_P(buf, PSTR("gpioVccOffset "), 14) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.vccOffset = String(buf+14).toDouble() * 100;
		} else if(strncmp_P(buf, PSTR("gpioVccMultiplier "), 18) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.vccMultiplier = String(buf+18).toDouble() * 1000;
		} else if(strncmp_P(buf, PSTR("gpioVccBootLimit "), 17) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.vccBootLimit = String(buf+17).toDouble() * 10;
		} else if(strncmp_P(buf, PSTR("gpioVccResistorGnd "), 19) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.vccResistorGnd = String(buf+19).toInt();
		} else if(strncmp_P(buf, PSTR("gpioVccResistorVcc "), 19) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.vccResistorVcc = String(buf+19).toInt();
		} else if(strncmp_P(buf, PSTR("domoticzElidx "), 14) == 0) {
			if(!lDomo) { config.getDomoticzConfig(domo); lDomo = true; };
			domo.elidx = String(buf+14).toInt();
		} else if(strncmp_P(buf, PSTR("domoticzVl1idx "), 15) == 0) {
			if(!lDomo) { config.getDomoticzConfig(domo); lDomo = true; };
			domo.vl1idx = String(buf+15).toInt();
		} else if(strncmp_P(buf, PSTR("domoticzVl2idx "), 15) == 0) {
			if(!lDomo) { config.getDomoticzConfig(domo); lDomo = true; };
			domo.vl2idx = String(buf+15).toInt();
		} else if(strncmp_P(buf, PSTR("domoticzVl3idx "), 15) == 0) {
			if(!lDomo) { config.getDomoticzConfig(domo); lDomo = true; };
			domo.vl3idx = String(buf+15).toInt();
		} else if(strncmp_P(buf, PSTR("domoticzCl1idx "), 15) == 0) {
			if(!lDomo) { config.getDomoticzConfig(domo); lDomo = true; };
			domo.cl1idx = String(buf+15).toInt();
		} else if(strncmp_P(buf, PSTR("ntpEnable "), 10) == 0) {
			if(!lNtp) { config.getNtpConfig(ntp); lNtp = true; };
			ntp.enable = String(buf+10).toInt() == 1;
		} else if(strncmp_P(buf, PSTR("ntpDhcp "), 8) == 0) {
			if(!lNtp) { config.getNtpConfig(ntp); lNtp = true; };
			ntp.dhcp = String(buf+8).toInt() == 1;
		} else if(strncmp_P(buf, PSTR("ntpServer "), 10) == 0) {
			if(!lNtp) { config.getNtpConfig(ntp); lNtp = true; };
			strcpy(ntp.server, buf+10);
		} else if(strncmp_P(buf, PSTR("ntpTimezone "), 12) == 0) {
			if(!lNtp) { config.getNtpConfig(ntp); lNtp = true; };
			strcpy(ntp.timezone, buf+12);
		} else if(strncmp_P(buf, PSTR("entsoeToken "), 12) == 0) {
			if(!lEntsoe) { config.getEntsoeConfig(entsoe); lEntsoe = true; };
			strcpy(entsoe.token, buf+12);
		} else if(strncmp_P(buf, PSTR("entsoeArea "), 11) == 0) {
			if(!lEntsoe) { config.getEntsoeConfig(entsoe); lEntsoe = true; };
			strcpy(entsoe.area, buf+11);
		} else if(strncmp_P(buf, PSTR("entsoeCurrency "), 15) == 0) {
			if(!lEntsoe) { config.getEntsoeConfig(entsoe); lEntsoe = true; };
			strcpy(entsoe.currency, buf+15);
		} else if(strncmp_P(buf, PSTR("entsoeMultiplier "), 17) == 0) {
			if(!lEntsoe) { config.getEntsoeConfig(entsoe); lEntsoe = true; };
			entsoe.multiplier = String(buf+17).toDouble() * 1000;
		} else if(strncmp_P(buf, PSTR("thresholds "), 11) == 0) {
			if(!lEac) { config.getEnergyAccountingConfig(eac); lEac = true; };
			int i = 0;
			char * pch = strtok (buf+11," ");
			while (pch != NULL && i < 10) {
				eac.thresholds[i++] = String(pch).toInt();
				pch = strtok (NULL, " ");
			}
			eac.hours = String(pch).toInt();
		} else if(strncmp_P(buf, PSTR("dayplot "), 8) == 0) {
			int i = 0;
			DayDataPoints day = { 0 };
			char * pch = strtok (buf+8," ");
			while (pch != NULL) {
				int64_t val = String(pch).toInt();
				if(day.version < 5) {
					if(i == 0) {
						day.version = val;
					} else if(i == 1) {
						day.lastMeterReadTime = val;
					} else if(i == 2) {
						day.activeImport = val;
					} else if(i > 2 && i < 27) {
						day.hImport[i-3] = val / 10;
					} else if(i == 27) {
						day.activeExport = val;
					} else if(i > 27 && i < 52) {
						day.hExport[i-28] = val / 10;
					}
				} else {
					if(i == 1) {
						day.lastMeterReadTime = val;
					} else if(i == 2) {
						day.activeImport = val;
					} else if(i == 3) {
						day.accuracy = val;
					} else if(i > 3 && i < 28) {
						day.hImport[i-4] = val / pow(10, day.accuracy);
					} else if(i == 28) {
						day.activeExport = val;
					} else if(i > 28 && i < 53) {
						day.hExport[i-29] = val / pow(10, day.accuracy);
					}
				}

				pch = strtok (NULL, " ");
				i++;
			}
			ds.setDayData(day);
			sDs = true;
		} else if(strncmp_P(buf, PSTR("monthplot "), 10) == 0) {
			int i = 0;
			MonthDataPoints month = { 0 };
			char * pch = strtok (buf+10," ");
			while (pch != NULL) {
				int64_t val = String(pch).toInt();
				if(month.version < 6) {
					if(i == 0) {
						month.version = val;
					} else if(i == 1) {
						month.lastMeterReadTime = val;
					} else if(i == 2) {
						month.activeImport = val;
					} else if(i > 2 && i < 34) {
						month.dImport[i-3] = val / 10;
					} else if(i == 34) {
						month.activeExport = val;
					} else if(i > 34 && i < 66) {
						month.dExport[i-35] = val / 10;
					}
				} else {
					if(i == 1) {
						month.lastMeterReadTime = val;
					} else if(i == 2) {
						month.activeImport = val;
					} else if(i == 3) {
						month.accuracy = val;
					} else if(i > 3 && i < 35) {
						month.dImport[i-4] = val / pow(10, month.accuracy);
					} else if(i == 35) {
						month.activeExport = val;
					} else if(i > 35 && i < 67) {
						month.dExport[i-36] = val / pow(10, month.accuracy);
					}
				}
				pch = strtok (NULL, " ");
				i++;
			}
			ds.setMonthData(month);
			sDs = true;
		} else if(strncmp_P(buf, PSTR("energyaccounting "), 17) == 0) {
			uint8_t i = 0;
			EnergyAccountingData ead = { 0, 0, 
                0, 0, 0, // Cost
                0, 0, 0, // Income
                0, 0, // Peak 1
                0, 0, // Peak 2
                0, 0, // Peak 3
                0, 0, // Peak 4
                0, 0 // Peak 5
            };
			uint8_t peak = 0;
			char * pch = strtok (buf+17," ");
			while (pch != NULL) {
				if(ead.version < 5) {
					if(i == 0) {
						long val = String(pch).toInt();
						ead.version = val;
					} else if(i == 1) {
						long val = String(pch).toInt();
						ead.month = val;
					} else if(i == 2) {
						double val = String(pch).toDouble();
						if(val > 0.0) {
							ead.peaks[0] = { 1, (uint16_t) (val*100) };
						}
					} else if(i == 3) {
						double val = String(pch).toDouble();
						ead.costYesterday = val * 10;
					} else if(i == 4) {
						double val = String(pch).toDouble();
						ead.costThisMonth = val;
					} else if(i == 5) {
						double val = String(pch).toDouble();
						ead.costLastMonth = val;
					} else if(i >= 6 && i < 18) {
						uint8_t hour = i-6;					
						{			
							long val = String(pch).toInt();
							ead.peaks[peak].day = val;
						} 
						pch = strtok (NULL, " ");
						i++;
						{
							double val = String(pch).toDouble();
							ead.peaks[peak].value = val * 100;
						}
						peak++;
					}
				} else {
					if(i == 1) {
						long val = String(pch).toInt();
						ead.month = val;
					} else if(i == 2) {
						double val = String(pch).toDouble();
						ead.costYesterday = val * 10;
					} else if(i == 3) {
						double val = String(pch).toDouble();
						ead.costThisMonth = val;
					} else if(i == 4) {
						double val = String(pch).toDouble();
						ead.costLastMonth = val;
					} else if(i == 5) {
						double val = String(pch).toDouble();
						ead.incomeYesterday= val * 10;
					} else if(i == 6) {
						double val = String(pch).toDouble();
						ead.incomeThisMonth = val;
					} else if(i == 7) {
						double val = String(pch).toDouble();
						ead.incomeLastMonth = val;
					} else if(i >= 8 && i < 20) {
						uint8_t hour = i-8;		
						{			
							long val = String(pch).toInt();
							ead.peaks[peak].day = val;
						} 
						pch = strtok (NULL, " ");
						i++;
						{
							double val = String(pch).toDouble();
							ead.peaks[peak].value = val * 100;
						}
						peak++;
					}
				}
				pch = strtok (NULL, " ");
				i++;
			}
			ead.version = 5;
			ea.setData(ead);
			sEa = true;
		}
		memset(buf, 0, 1024);
	}

	debugD("Deleting config file");
	file.close();
	LittleFS.remove(FILE_CFG);

	debugI("Saving configuration now...");
	Serial.flush();
	if(lSys) config.setSystemConfig(sys);
	if(lWiFi) config.setWiFiConfig(wifi);
	if(lMqtt) config.setMqttConfig(mqtt);
	if(lWeb) config.setWebConfig(web);
	if(lMeter) config.setMeterConfig(meter);
	if(lGpio) config.setGpioConfig(gpio);
	if(lDomo) config.setDomoticzConfig(domo);
	if(lNtp) config.setNtpConfig(ntp);
	if(lEntsoe) config.setEntsoeConfig(entsoe);
	if(lEac) config.setEnergyAccountingConfig(eac);
	if(sDs) ds.save();
	if(sEa) ea.save();
	config.save();
}