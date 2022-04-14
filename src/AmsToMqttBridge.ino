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

#include "entsoe/EntsoeApi.h"

#include "web/AmsWebServer.h"
#include "AmsConfiguration.h"

#include "mqtt/AmsMqttHandler.h"
#include "mqtt/JsonMqttHandler.h"
#include "mqtt/RawMqttHandler.h"
#include "mqtt/DomoticzMqttHandler.h"
#include "mqtt/HomeAssistantMqttHandler.h"

#include "Uptime.h"

#include "RemoteDebug.h"

#define BUF_SIZE_COMMON (2048)
#define BUF_SIZE_HAN (1024)
#include "ams/hdlc.h"
#include "MbusAssembler.h"

#include "IEC6205621.h"
#include "IEC6205675.h"

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
WiFiClient *mqttClient = new WiFiClient();
WiFiClientSecure *mqttSecureClient = NULL;
AmsMqttHandler* mqttHandler = NULL;

Stream *hanSerial;
SoftwareSerial *swSerial = NULL;
HDLCConfig* hc = NULL;

GpioConfig gpioConfig;
MeterConfig meterConfig;
bool mqttEnabled = false;
String topic = "ams";
AmsData meterState;
bool ntpEnabled = false;

AmsDataStorage ds(&Debug);
EnergyAccounting ea(&Debug);

uint8_t wifiReconnectCount = 0;

void setup() {
	WiFiConfig wifi;
	Serial.begin(115200);

	if(!config.getGpioConfig(gpioConfig)) {
		#if HW_ROARFRED
			gpioConfig.hanPin = 3;
			gpioConfig.apPin = 0;
			gpioConfig.ledPin = 2;
			gpioConfig.ledInverted = true;
			gpioConfig.tempSensorPin = 5;
		#elif defined(ARDUINO_ESP8266_WEMOS_D1MINI)
			gpioConfig.hanPin = 5;
			gpioConfig.apPin = 4;
			gpioConfig.ledPin = 2;
			gpioConfig.ledInverted = true;
			gpioConfig.tempSensorPin = 14;
			gpioConfig.vccMultiplier = 1100;
		#elif defined(ARDUINO_LOLIN_D32)
			gpioConfig.hanPin = 16;
			gpioConfig.ledPin = 5;
			gpioConfig.ledInverted = true;
			gpioConfig.tempSensorPin = 14;
		#elif defined(ARDUINO_FEATHER_ESP32)
			gpioConfig.hanPin = 16;
			gpioConfig.ledPin = 2;
			gpioConfig.tempSensorPin = 14;
		#elif defined(ARDUINO_ESP32_DEV)
			gpioConfig.hanPin = 16;
			gpioConfig.ledPin = 2;
			gpioConfig.ledInverted = true;
		#elif defined(ESP8266)
			gpioConfig.hanPin = 3;
			gpioConfig.ledPin = 2;
			gpioConfig.ledInverted = true;
		#elif defined(ESP32)
			gpioConfig.hanPin = 16;
			gpioConfig.ledPin = 2;
			gpioConfig.ledInverted = true;
			gpioConfig.tempSensorPin = 14;
		#endif
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

	#if defined(ESP32)
	EntsoeConfig entsoe;
	if(config.getEntsoeConfig(entsoe) && strlen(entsoe.token) > 0) {
		eapi = new EntsoeApi(&Debug);
		eapi->setup(entsoe);
		ws.setEntsoeApi(eapi);
	}
	#endif

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
			Serial.begin(meterConfig.baud, serialConfig, -1, -1, meterConfig.invert);
		#else
			Serial.begin(meterConfig.baud, serialConfig, SERIAL_FULL, 1, meterConfig.invert);
		#endif
	}

 	if(!shared) {
		Serial.begin(115200);
	}

	Debug.setSerialEnabled(true);
	DebugConfig debug;
	delay(1);

	float vcc = hw.getVcc();

	if (Debug.isActive(RemoteDebug::INFO)) {
		debugI("AMS bridge started");
		debugI("Voltage: %.2fV", vcc);
	}

	float vccBootLimit = gpioConfig.vccBootLimit == 0 ? 0 : gpioConfig.vccBootLimit / 10.0;
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
				File firmwareFile = LittleFS.open(FILE_FIRMWARE, "r");
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
			configTime(ntp.offset*10, ntp.summerOffset*10, ntp.enable ? strlen(ntp.server) > 0 ? ntp.server : "pool.ntp.org" : ""); // Add NTP server by default if none is configured
			sntp_servermode_dhcp(ntp.enable && ntp.dhcp ? 1 : 0);
			ntpEnabled = ntp.enable;
			TimeChangeRule std = {"STD", Last, Sun, Oct, 3, ntp.offset / 6};
			TimeChangeRule dst = {"DST", Last, Sun, Mar, 2, (ntp.offset + ntp.summerOffset) / 6};
			tz = new Timezone(dst, std);
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
	ea.setup(&ds, eapi, eac);
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
							MDNS.addService("http", "tcp", 80);
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
					configTime(ntp.offset*10, ntp.summerOffset*10, ntp.enable ? ntp.server : "");
					sntp_servermode_dhcp(ntp.enable && ntp.dhcp ? 1 : 0);
					ntpEnabled = ntp.enable;

					if(tz != NULL) delete tz;
					TimeChangeRule std = {"STD", Last, Sun, Oct, 3, ntp.offset / 6};
					TimeChangeRule dst = {"DST", Last, Sun, Mar, 2, (ntp.offset + ntp.summerOffset) / 6};
					tz = new Timezone(dst, std);
					ws.setTimezone(tz);
					ds.setTimezone(tz);
					ea.setTimezone(tz);
				}

				config.ackNtpChange();
			}
			#if defined ESP8266
			MDNS.update();
			#endif

			if(now > 10000 && now - lastErrorBlink > 3000) {
				errorBlink();
			}

			if (mqttEnabled || config.isMqttChanged()) {
				if(mqtt == NULL || !mqtt->connected() || config.isMqttChanged()) {
					MQTT_connect();
				}
			} else if(mqtt != NULL && mqtt->connected()) {
				mqttClient->stop();
				mqtt->disconnect();
			}

			#if defined(ESP32)
			try {
				if(eapi != NULL && ntpEnabled) {
					if(eapi->loop() && mqtt != NULL && mqttHandler != NULL && mqtt->connected()) {
						mqttHandler->publishPrices(eapi);
					}
				}
				
				if(config.isEntsoeChanged()) {
					EntsoeConfig entsoe;
					if(config.getEntsoeConfig(entsoe) && strlen(entsoe.token) > 0) {
						if(eapi == NULL) {
							eapi = new EntsoeApi(&Debug);
							ws.setEntsoeApi(eapi);
						}
						eapi->setup(entsoe);
					} else if(eapi != NULL) {
						delete eapi;
						eapi = NULL;
						ws.setEntsoeApi(NULL);
					}
					config.ackEntsoeChange();
				}
			} catch(const std::exception& e) {
				debugE("Exception in ENTSO-E loop (%s)", e.what());
			}
			#endif
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
		delete hc;
		hc = NULL;
	}

	if(config.isEnergyAccountingChanged()) {
		EnergyAccountingConfig *eac = ea.getConfig();
		config.getEnergyAccountingConfig(*eac);
		ea.setup(&ds, eapi, eac);
		config.ackEnergyAccountingChange();
	}

	if(readHanPort() || now - meterState.getLastUpdateMillis() > 30000) {
		if(now - lastTemperatureRead > 15000) {
			unsigned long start = millis();
			hw.updateTemperatures();
			lastTemperatureRead = now;

			if(mqtt != NULL && mqttHandler != NULL && WiFi.getMode() != WIFI_AP && WiFi.status() == WL_CONNECTED && mqtt->connected() && !topic.isEmpty()) {
				mqttHandler->publishTemperatures(&config, &hw);
			}
			debugD("Used %d ms to update temperature", millis()-start);
		}
		if(now - lastSysupdate > 10000) {
			if(mqtt != NULL && mqttHandler != NULL && WiFi.getMode() != WIFI_AP && WiFi.status() == WL_CONNECTED && mqtt->connected() && !topic.isEmpty()) {
				mqttHandler->publishSystem(&hw);
			}
			lastSysupdate = now;
		}
	}
	delay(1); // Needed for auto modem sleep
	#if defined(ESP32)
		esp_task_wdt_reset();
	#elif defined(ESP8266)
		ESP.wdtFeed();
	#endif
}

void setupHanPort(uint8_t pin, uint32_t baud, uint8_t parityOrdinal, bool invert) {
	if(Debug.isActive(RemoteDebug::INFO)) Debug.printf("(setupHanPort) Setting up HAN on pin %d with baud %d and parity %d\n", pin, baud, parityOrdinal);

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

		#if defined(ESP32)
			hwSerial->begin(baud, serialConfig, -1, -1, invert);
			hwSerial->setRxBufferSize(768);
		#else
			hwSerial->begin(baud, serialConfig, SERIAL_FULL, 1, invert);
			hwSerial->setRxBufferSize(768);
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
	for(;lastError < 3;lastError++) {
		switch(lastError) {
			case 0:
				if(lastErrorBlink - meterState.getLastUpdateMillis() > 30000) {
					hw.ledBlink(LED_RED, 1); // If no message received from AMS in 30 sec, blink once
					return;
				}
				break;
			case 1:
				if(mqttEnabled && mqtt != NULL && mqtt->lastError() != 0) {
					hw.ledBlink(LED_RED, 2); // If MQTT error, blink twice
					return;
				}
				break;
			case 2:
				if(WiFi.getMode() != WIFI_AP && WiFi.status() != WL_CONNECTED) {
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
		WiFi.softAP("AMS2MQTT");
		WiFi.mode(WIFI_AP);

		if(dnsServer == NULL) {
			dnsServer = new DNSServer();
		}
		dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
		dnsServer->start(53, "*", WiFi.softAPIP());
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
MbusAssembler* ma = NULL;
int currentMeterType = -1;
bool readHanPort() {
	if(!hanSerial->available()) return false;

	if(currentMeterType == -1) {
		hanSerial->readBytes(hanBuffer, BUF_SIZE_HAN);
		currentMeterType = 0;
		return false;
	}
	if(currentMeterType == 0) {
		uint8_t flag = hanSerial->read();
		if(flag == 0x7E || flag == 0x68) {
			debugD("HDLC or MBUS");
			currentMeterType = 1;
		} else if(flag == 0xDB) {
			debugD("Encrypted DSMR");
			hc = new HDLCConfig();
			memcpy(hc->encryption_key, meterConfig.encryptionKey, 16);
			memcpy(hc->authentication_key, meterConfig.authenticationKey, 16);
			currentMeterType = 2;
		} else if(flag == 0x2F) {
			debugD("DSMR");
			currentMeterType = 2;
		} else {
			currentMeterType = -1;
		}
		hanSerial->readBytes(hanBuffer, BUF_SIZE_HAN);
		return false;
	}
	CosemDateTime timestamp = {0};
	AmsData data;
	if(currentMeterType == 1) {
		int pos = HDLC_FRAME_INCOMPLETE;
		while(hanSerial->available() && pos == HDLC_FRAME_INCOMPLETE) {
			hanBuffer[len++] = hanSerial->read();
			pos = HDLC_validate((uint8_t *) hanBuffer, len, hc, &timestamp);
		}
		if(len > 0) {
			if(len >= BUF_SIZE_HAN) {
				hanSerial->readBytes(hanBuffer, BUF_SIZE_HAN);
				len = 0;
				debugI("Buffer overflow, resetting");
				return false;
			}
			if(pos == MBUS_FRAME_INTERMEDIATE_SEGMENT) {
				debugI("Intermediate segment");
				if(ma == NULL) {
					ma = new MbusAssembler();
				}
				if(ma->append((uint8_t *) hanBuffer, len) < 0)
					pos = -77;
				if(Debug.isActive(RemoteDebug::VERBOSE)) {
					debugD("Frame dump (%db):", len);
					debugPrint(hanBuffer, 0, len);
				}
				len = 0;
				return false;
			} else if(pos == MBUS_FRAME_LAST_SEGMENT) {
				debugI("Final segment");
				if(Debug.isActive(RemoteDebug::VERBOSE)) {
					debugD("Frame dump (%db):", len);
					debugPrint(hanBuffer, 0, len);
				}
				if(ma->append((uint8_t *) hanBuffer, len) >= 0) {
					len = ma->write((uint8_t *) hanBuffer);
					pos = HDLC_validate((uint8_t *) hanBuffer, len, hc, &timestamp);
				} else {
					pos = -77;
				}
			}
			if(pos == HDLC_FRAME_INCOMPLETE) {
				return false;
			}
			for(int i = len; i<BUF_SIZE_HAN; i++) {
				hanBuffer[i] = 0x00;
			}
			if(pos == HDLC_ENCRYPTION_CONFIG_MISSING) {
				hc = new HDLCConfig();
				memcpy(hc->encryption_key, meterConfig.encryptionKey, 16);
				memcpy(hc->authentication_key, meterConfig.authenticationKey, 16);
			}
			if(Debug.isActive(RemoteDebug::VERBOSE)) {
				debugD("Frame dump (%db):", len);
				debugPrint(hanBuffer, 0, len);
			}
			if(hc != NULL && Debug.isActive(RemoteDebug::VERBOSE)) {
				debugD("System title:");
				debugPrint(hc->system_title, 0, 8);
				debugD("Initialization vector:");
				debugPrint(hc->initialization_vector, 0, 12);
				debugD("Additional authenticated data:");
				debugPrint(hc->additional_authenticated_data, 0, 17);
				debugD("Authentication tag:");
				debugPrint(hc->authentication_tag, 0, 12);
			}
			if(mqttEnabled && mqtt != NULL && mqttHandler == NULL) {
				mqtt->publish(topic.c_str(), toHex(hanBuffer, len));
			}
			len = 0;
			if(pos > 0) {
				debugD("Valid data, start at byte %d", pos);
				data = IEC6205675(((char *) (hanBuffer)) + pos, meterState.getMeterType(), &meterConfig, timestamp, hc);
			} else {
				printHanReadError(pos);
				return false;
			}
		} else {
			return false;
		}
	} else if(currentMeterType == 2) {
		int pos = HDLC_FRAME_INCOMPLETE;
		if(hc != NULL) {
			while(hanSerial->available() && pos == HDLC_FRAME_INCOMPLETE) {
				hanBuffer[len++] = hanSerial->read();
				pos = mbus_decrypt((uint8_t *) hanBuffer, len, hc);
			}
		} else {
			while(hanSerial->available()) {
				hanBuffer[len++] = hanSerial->read();
			}
			if(len > 10) {
				String end = String((char*) hanBuffer+len-5);
				if(end.startsWith("!")) pos = 0;
				while(hanSerial->available()) hanSerial->read();
			}
		}
		if(len == 0) return false;
		if(pos == HDLC_FRAME_INCOMPLETE) return false;
		if(len >= BUF_SIZE_HAN) {
			len = 0;
			debugI("Buffer overflow, resetting");
			return false;
		}
		if(pos < 0) {
			printHanReadError(pos);
			return false;
		}

		if(mqttEnabled && mqtt != NULL && mqttHandler == NULL) {
			mqtt->publish(topic.c_str(), (char*) hanBuffer);
		}
		len = 0;
		data = IEC6205621(((char *) (hanBuffer)) + pos);
		if(data.getListType() == 0) {
			currentMeterType = 0;
			return false;
		} else {
			if(Debug.isActive(RemoteDebug::DEBUG)) {
				debugD("Frame dump: %d", strlen((char*) (hanBuffer+pos)));
				debugD("%s", hanBuffer+pos);
			}
		}
		for(int i = len; i<BUF_SIZE_HAN; i++) hanBuffer[i] = 0x00;
	}

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
				debugD(" using actual data");
				saveData = ds.update(&data);
			} else if(meterState.getListType() >= 3) {
				debugD(" using estimated data");
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
			case HDLC_BOUNDRY_FLAG_MISSING:
				debugW("Boundry flag missing");
				break;
			case HDLC_HCS_ERROR:
				debugW("Header checksum error");
				break;
			case HDLC_FCS_ERROR:
				debugW("Frame checksum error");
				break;
			case HDLC_FRAME_INCOMPLETE:
				debugW("Received frame is incomplete");
				break;
			case HDLC_ENCRYPTION_CONFIG_MISSING:
				debugI("Encryption configuration requested, initializing");
				break;
			case HDLC_ENCRYPTION_AUTH_FAILED:
				debugW("Decrypt authentication failed");
				break;
			case HDLC_ENCRYPTION_KEY_FAILED:
				debugW("Setting decryption key failed");
				break;
			case HDLC_ENCRYPTION_DECRYPT_FAILED:
				debugW("Decryption failed");
				break;
			case MBUS_FRAME_LENGTH_NOT_EQUAL:
				debugW("Frame length mismatch");
				break;
			case MBUS_FRAME_INTERMEDIATE_SEGMENT:
			case MBUS_FRAME_LAST_SEGMENT:
				debugW("Partial frame dropped");
				break;
			case HDLC_TIMESTAMP_UNKNOWN:
				debugW("Frame timestamp is not correctly formatted");
				break;
			case HDLC_UNKNOWN_DATA:
				debugW("Unknown data format %02X", hanBuffer[0]);
				currentMeterType = 0;
				break;
			default:
				debugW("Unspecified error while reading data: %d", pos);
		}
	}
}

void debugPrint(byte *buffer, int start, int length) {
	for (int i = start; i < start + length; i++) {
		if (buffer[i] < 0x10)
			Debug.print("0");
		Debug.print(buffer[i], HEX);
		Debug.print(" ");
		if ((i - start + 1) % 16 == 0)
			Debug.println("");
		else if ((i - start + 1) % 4 == 0)
			Debug.print(" ");

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
		if(WiFi.getMode() != WIFI_OFF) {
			if(wifiReconnectCount > 3) {
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
			WiFi.persistent(false);
			WiFi.disconnect(true);
			WiFi.softAPdisconnect(true);
			WiFi.enableAP(false);
			WiFi.mode(WIFI_OFF);
			yield();
			wifiTimeout = 5000;
			return;
		}
		wifiTimeout = WIFI_CONNECTION_TIMEOUT;

		WiFiConfig wifi;
		if(!config.getWiFiConfig(wifi) || strlen(wifi.ssid) == 0) {
			swapWifiMode();
			return;
		}

		if (Debug.isActive(RemoteDebug::INFO)) debugI("Connecting to WiFi network: %s", wifi.ssid);

		wifiReconnectCount++;

		#if defined(ESP32)
			if(strlen(wifi.hostname) > 0) {
				WiFi.setHostname(wifi.hostname);
			}	
		#endif
		WiFi.mode(WIFI_STA);
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
		#elif defined(ESP8266)
			WiFi.setOutputPower(wifi.power / 10.0);
		#endif
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
				dns2.fromString("208.67.220.220"); // Add OpenDNS as second by default if nothing is configured
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
		WiFi.setAutoReconnect(true);
		WiFi.persistent(true);
		if(WiFi.begin(wifi.ssid, wifi.psk)) {
			yield();
		} else {
			if (Debug.isActive(RemoteDebug::ERROR)) debugI("Unable to start WiFi");
		}
  	}
}

unsigned long lastMqttRetry = -10000;
void MQTT_connect() {
	MqttConfig mqttConfig;
	if(!config.getMqttConfig(mqttConfig) || strlen(mqttConfig.host) == 0) {
		if(Debug.isActive(RemoteDebug::WARNING)) debugW("No MQTT config");
		mqttEnabled = false;
		ws.setMqttEnabled(false);
		config.ackMqttChange();
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
		yield();
	} else {
		uint16_t size = 128;
		switch(mqttConfig.payloadFormat) {
			case 0: // JSON
			case 4: // Home Assistant
				size = 768;
				break;
			case 3: // Domoticz
				size = 256;
				break;
			case 255: // Raw frame
				size = 1024;
				break;
		}

		mqtt = new MQTTClient(size);
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

	if(mqttConfig.ssl) {
		debugI("MQTT SSL is configured");
		if(mqttSecureClient == NULL) {
			mqttSecureClient = new WiFiClientSecure();
		}
		#if defined(ESP8266)
			mqttSecureClient->setBufferSizes(512, 512);
		#endif

		if(LittleFS.begin()) {
			char *ca = NULL;
			char *cert = NULL;
			char *key = NULL;
			File file;

			if(LittleFS.exists(FILE_MQTT_CA)) {
				debugI("Found MQTT CA file");
				file = LittleFS.open(FILE_MQTT_CA, "r");
				#if defined(ESP8266)
					BearSSL::X509List *serverTrustedCA = new BearSSL::X509List(file);
                    mqttSecureClient->setTrustAnchors(serverTrustedCA);
				#elif defined(ESP32)
					mqttSecureClient->loadCACert(file, file.size());
				#endif
			}

			if(LittleFS.exists(FILE_MQTT_CERT) && LittleFS.exists(FILE_MQTT_KEY)) {
				#if defined(ESP8266)
					file = LittleFS.open(FILE_MQTT_CERT, "r");
				 	BearSSL::X509List *serverCertList = new BearSSL::X509List(file);
					file = LittleFS.open(FILE_MQTT_KEY, "r");
  					BearSSL::PrivateKey *serverPrivKey = new BearSSL::PrivateKey(file);
					mqttSecureClient->setClientRSACert(serverCertList, serverPrivKey);
				#elif defined(ESP32)
					debugI("Found MQTT certificate file");
					file = LittleFS.open(FILE_MQTT_CERT, "r");
					mqttSecureClient->loadCertificate(file, file.size());

					debugI("Found MQTT key file");
					file = LittleFS.open(FILE_MQTT_KEY, "r");
					mqttSecureClient->loadPrivateKey(file, file.size());
				#endif
			}
			LittleFS.end();
		}
		mqttClient = mqttSecureClient;
	} else if(mqttClient == NULL) {
		mqttClient = new WiFiClient();
	}

	if(Debug.isActive(RemoteDebug::INFO)) {
		debugI("Connecting to MQTT %s:%d", mqttConfig.host, mqttConfig.port);
	}
	
	mqtt->begin(mqttConfig.host, mqttConfig.port, *mqttClient);

	#if defined(ESP8266)
		if(mqttSecureClient) {
			time_t epoch = time(nullptr);
			debugD("Setting NTP time %i for secure MQTT connection", epoch);
			mqttSecureClient->setX509Time(epoch);
		}
	#endif

	// Connect to a unsecure or secure MQTT server
	if ((strlen(mqttConfig.username) == 0 && mqtt->connect(mqttConfig.clientId)) ||
		(strlen(mqttConfig.username) > 0 && mqtt->connect(mqttConfig.clientId, mqttConfig.username, mqttConfig.password))) {
		if (Debug.isActive(RemoteDebug::INFO)) debugI("Successfully connected to MQTT!");
		config.ackMqttChange();
		
		if(mqttHandler != NULL) {
			mqttHandler->publishSystem(&hw);
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

	File file = LittleFS.open(FILE_CFG, "r");

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
		if(strncmp(buf, "boardType ", 10) == 0) {
			if(!lSys) { config.getSystemConfig(sys); lSys = true; };
			sys.boardType = String(buf+10).toInt();
		} else if(strncmp(buf, "ssid ", 5) == 0) {
			if(!lWiFi) { config.getWiFiConfig(wifi); lWiFi = true; };
			memcpy(wifi.ssid, buf+5, size-5);
		} else if(strncmp(buf, "psk ", 4) == 0) {
			if(!lWiFi) { config.getWiFiConfig(wifi); lWiFi = true; };
			memcpy(wifi.psk, buf+4, size-4);
		} else if(strncmp(buf, "ip ", 3) == 0) {
			if(!lWiFi) { config.getWiFiConfig(wifi); lWiFi = true; };
			memcpy(wifi.ip, buf+3, size-3);
		} else if(strncmp(buf, "gateway ", 8) == 0) {
			if(!lWiFi) { config.getWiFiConfig(wifi); lWiFi = true; };
			memcpy(wifi.gateway, buf+8, size-8);
		} else if(strncmp(buf, "subnet ", 7) == 0) {
			if(!lWiFi) { config.getWiFiConfig(wifi); lWiFi = true; };
			memcpy(wifi.subnet, buf+7, size-7);
		} else if(strncmp(buf, "dns1 ", 5) == 0) {
			if(!lWiFi) { config.getWiFiConfig(wifi); lWiFi = true; };
			memcpy(wifi.dns1, buf+5, size-5);
		} else if(strncmp(buf, "dns2 ", 5) == 0) {
			if(!lWiFi) { config.getWiFiConfig(wifi); lWiFi = true; };
			memcpy(wifi.dns2, buf+5, size-5);
		} else if(strncmp(buf, "hostname ", 9) == 0) {
			if(!lWiFi) { config.getWiFiConfig(wifi); lWiFi = true; };
			memcpy(wifi.hostname, buf+9, size-9);
		} else if(strncmp(buf, "mdns ", 5) == 0) {
			if(!lWiFi) { config.getWiFiConfig(wifi); lWiFi = true; };
			wifi.mdns = String(buf+5).toInt() == 1;;
		} else if(strncmp(buf, "mqttHost ", 9) == 0) {
			if(!lMqtt) { config.getMqttConfig(mqtt); lMqtt = true; };
			memcpy(mqtt.host, buf+9, size-9);
		} else if(strncmp(buf, "mqttPort ", 9) == 0) {
			if(!lMqtt) { config.getMqttConfig(mqtt); lMqtt = true; };
			mqtt.port = String(buf+9).toInt();
		} else if(strncmp(buf, "mqttClientId ", 13) == 0) {
			if(!lMqtt) { config.getMqttConfig(mqtt); lMqtt = true; };
			memcpy(mqtt.clientId, buf+13, size-13);
		} else if(strncmp(buf, "mqttPublishTopic ", 17) == 0) {
			if(!lMqtt) { config.getMqttConfig(mqtt); lMqtt = true; };
			memcpy(mqtt.publishTopic, buf+17, size-17);
		} else if(strncmp(buf, "mqttUsername ", 13) == 0) {
			if(!lMqtt) { config.getMqttConfig(mqtt); lMqtt = true; };
			memcpy(mqtt.username, buf+13, size-13);
		} else if(strncmp(buf, "mqttPassword ", 13) == 0) {
			if(!lMqtt) { config.getMqttConfig(mqtt); lMqtt = true; };
			memcpy(mqtt.password, buf+13, size-13);
		} else if(strncmp(buf, "mqttPayloadFormat ", 18) == 0) {
			if(!lMqtt) { config.getMqttConfig(mqtt); lMqtt = true; };
			mqtt.payloadFormat = String(buf+18).toInt();
		} else if(strncmp(buf, "mqttSsl ", 8) == 0) {
			if(!lMqtt) { config.getMqttConfig(mqtt); lMqtt = true; };
			mqtt.ssl = String(buf+8).toInt() == 1;;
		} else if(strncmp(buf, "webSecurity ", 12) == 0) {
			if(!lWeb) { config.getWebConfig(web); lWeb = true; };
			web.security = String(buf+12).toInt();
		} else if(strncmp(buf, "webUsername ", 12) == 0) {
			if(!lWeb) { config.getWebConfig(web); lWeb = true; };
			memcpy(web.username, buf+12, size-12);
		} else if(strncmp(buf, "webPassword ", 12) == 0) {
			if(!lWeb) { config.getWebConfig(web); lWeb = true; };
			memcpy(web.username, buf+12, size-12);
		} else if(strncmp(buf, "meterBaud ", 10) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			meter.baud = String(buf+10).toInt();
		} else if(strncmp(buf, "meterParity ", 12) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			if(strncmp(buf+12, "7N1", 3) == 0) meter.parity = 2;
			if(strncmp(buf+12, "8N1", 3) == 0) meter.parity = 3;
			if(strncmp(buf+12, "7E1", 3) == 0) meter.parity = 10;
			if(strncmp(buf+12, "8E1", 3) == 0) meter.parity = 11;
		} else if(strncmp(buf, "meterInvert ", 12) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			meter.invert = String(buf+12).toInt() == 1;;
		} else if(strncmp(buf, "meterDistributionSystem ", 24) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			meter.distributionSystem = String(buf+24).toInt();
		} else if(strncmp(buf, "meterMainFuse ", 14) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			meter.mainFuse = String(buf+14).toInt();
		} else if(strncmp(buf, "meterProductionCapacity ", 24) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			meter.productionCapacity = String(buf+24).toInt();
		} else if(strncmp(buf, "meterEncryptionKey ", 19) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			fromHex(meter.encryptionKey, String(buf+19), 16);
		} else if(strncmp(buf, "meterAuthenticationKey ", 23) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			fromHex(meter.authenticationKey, String(buf+19), 16);
		} else if(strncmp(buf, "gpioHanPin ", 11) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.hanPin = String(buf+11).toInt();
		} else if(strncmp(buf, "gpioApPin ", 10) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.apPin = String(buf+10).toInt();
		} else if(strncmp(buf, "gpioLedPin ", 11) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.ledPin = String(buf+11).toInt();
		} else if(strncmp(buf, "gpioLedInverted ", 16) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.ledInverted = String(buf+16).toInt() == 1;
		} else if(strncmp(buf, "gpioLedPinRed ", 14) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.ledPinRed = String(buf+14).toInt();
		} else if(strncmp(buf, "gpioLedPinGreen ", 16) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.ledPinGreen = String(buf+16).toInt();
		} else if(strncmp(buf, "gpioLedPinBlue ", 15) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.ledPinBlue = String(buf+15).toInt();
		} else if(strncmp(buf, "gpioLedRgbInverted ", 19) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.ledRgbInverted = String(buf+19).toInt() == 1;
		} else if(strncmp(buf, "gpioTempSensorPin ", 18) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.tempSensorPin = String(buf+18).toInt();
		} else if(strncmp(buf, "gpioTempAnalogSensorPin ", 24) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.tempAnalogSensorPin = String(buf+24).toInt();
		} else if(strncmp(buf, "gpioVccPin ", 11) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.vccPin = String(buf+11).toInt();
		} else if(strncmp(buf, "gpioVccOffset ", 14) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.vccOffset = String(buf+14).toDouble() * 100;
		} else if(strncmp(buf, "gpioVccMultiplier ", 18) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.vccMultiplier = String(buf+18).toDouble() * 1000;
		} else if(strncmp(buf, "gpioVccBootLimit ", 17) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.vccBootLimit = String(buf+17).toDouble() * 10;
		} else if(strncmp(buf, "gpioVccResistorGnd ", 19) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.vccResistorGnd = String(buf+19).toInt();
		} else if(strncmp(buf, "gpioVccResistorVcc ", 19) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.vccResistorVcc = String(buf+19).toInt();
		} else if(strncmp(buf, "domoticzElidx ", 14) == 0) {
			if(!lDomo) { config.getDomoticzConfig(domo); lDomo = true; };
			domo.elidx = String(buf+14).toInt();
		} else if(strncmp(buf, "domoticzVl1idx ", 15) == 0) {
			if(!lDomo) { config.getDomoticzConfig(domo); lDomo = true; };
			domo.vl1idx = String(buf+15).toInt();
		} else if(strncmp(buf, "domoticzVl2idx ", 15) == 0) {
			if(!lDomo) { config.getDomoticzConfig(domo); lDomo = true; };
			domo.vl2idx = String(buf+15).toInt();
		} else if(strncmp(buf, "domoticzVl3idx ", 15) == 0) {
			if(!lDomo) { config.getDomoticzConfig(domo); lDomo = true; };
			domo.vl3idx = String(buf+15).toInt();
		} else if(strncmp(buf, "domoticzCl1idx ", 15) == 0) {
			if(!lDomo) { config.getDomoticzConfig(domo); lDomo = true; };
			domo.cl1idx = String(buf+15).toInt();
		} else if(strncmp(buf, "ntpEnable ", 10) == 0) {
			if(!lNtp) { config.getNtpConfig(ntp); lNtp = true; };
			ntp.enable = String(buf+10).toInt() == 1;
		} else if(strncmp(buf, "ntpDhcp ", 8) == 0) {
			if(!lNtp) { config.getNtpConfig(ntp); lNtp = true; };
			ntp.dhcp = String(buf+8).toInt() == 1;
		} else if(strncmp(buf, "ntpOffset ", 10) == 0) {
			if(!lNtp) { config.getNtpConfig(ntp); lNtp = true; };
			ntp.offset = String(buf+10).toInt() / 10;
		} else if(strncmp(buf, "ntpSummerOffset ", 16) == 0) {
			if(!lNtp) { config.getNtpConfig(ntp); lNtp = true; };
			ntp.summerOffset = String(buf+16).toInt() / 10;
		} else if(strncmp(buf, "ntpServer ", 10) == 0) {
			if(!lNtp) { config.getNtpConfig(ntp); lNtp = true; };
			memcpy(ntp.server, buf+10, size-10);
		} else if(strncmp(buf, "entsoeToken ", 12) == 0) {
			if(!lEntsoe) { config.getEntsoeConfig(entsoe); lEntsoe = true; };
			memcpy(entsoe.token, buf+12, size-12);
		} else if(strncmp(buf, "entsoeArea ", 11) == 0) {
			if(!lEntsoe) { config.getEntsoeConfig(entsoe); lEntsoe = true; };
			memcpy(entsoe.area, buf+11, size-11);
		} else if(strncmp(buf, "entsoeCurrency ", 15) == 0) {
			if(!lEntsoe) { config.getEntsoeConfig(entsoe); lEntsoe = true; };
			memcpy(entsoe.currency, buf+15, size-15);
		} else if(strncmp(buf, "entsoeMultiplier ", 17) == 0) {
			if(!lEntsoe) { config.getEntsoeConfig(entsoe); lEntsoe = true; };
			entsoe.multiplier = String(buf+17).toDouble() * 1000;
		} else if(strncmp(buf, "thresholds ", 11) == 0) {
			if(!lEac) { config.getEnergyAccountingConfig(eac); lEac = true; };
			int i = 0;
			char * pch = strtok (buf+11," ");
			while (pch != NULL) {
				eac.thresholds[i++] = String(pch).toInt();
				pch = strtok (NULL, " ");
			}
		} else if(strncmp(buf, "dayplot ", 8) == 0) {
			int i = 0;
			DayDataPoints day = { 4 }; // Use a version we know the multiplier of the data points
			char * pch = strtok (buf+8," ");
			while (pch != NULL) {
				int64_t val = String(pch).toInt();
				if(i == 1) {
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

				pch = strtok (NULL, " ");
				i++;
			}
			ds.setDayData(day);
		} else if(strncmp(buf, "monthplot ", 10) == 0) {
			int i = 0;
			MonthDataPoints month = { 5 }; // Use a version we know the multiplier of the data points
			char * pch = strtok (buf+10," ");
			while (pch != NULL) {
				int64_t val = String(pch).toInt();
				if(i == 1) {
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

				pch = strtok (NULL, " ");
				i++;
			}
			ds.setMonthData(month);
		} else if(strncmp(buf, "energyaccounting ", 17) == 0) {
			int i = 0;
			EnergyAccountingData ead = { 1 };
			char * pch = strtok (buf+17," ");
			while (pch != NULL) {
				if(i == 1) {
					long val = String(pch).toInt();
					ead.month = val;
				} else if(i == 2) {
					double val = String(pch).toDouble();
					ead.maxHour = val * 100;
				} else if(i == 3) {
					double val = String(pch).toDouble();
					ead.costYesterday = val * 100;
				} else if(i == 4) {
					double val = String(pch).toDouble();
					ead.costThisMonth = val * 100;
				} else if(i == 5) {
					double val = String(pch).toDouble();
					ead.costLastMonth = val * 100;
				}
				pch = strtok (NULL, " ");
				i++;
			}
			ea.setData(ead);
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
	ds.save();
	ea.save();
	config.save();
}
