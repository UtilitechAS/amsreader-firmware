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

#include <Arduino.h>

#if defined(ESP8266)
ADC_MODE(ADC_VCC);
#endif

#if defined(ESP32)
#include <esp_task_wdt.h>
#include <lwip/dns.h>
#endif
#define WDT_TIMEOUT 60

#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32C3)
#include <driver/uart.h>
#endif

#include "FirmwareVersion.h"
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

#define debugV_P(x, ...)	if (Debug.isActive(Debug.VERBOSE)) 	{Debug.printf_P(x,  ##__VA_ARGS__);Debug.println();}
#define debugD_P(x, ...)	if (Debug.isActive(Debug.DEBUG))	{Debug.printf_P(x, ##__VA_ARGS__);Debug.println();}
#define debugI_P(x, ...)	if (Debug.isActive(Debug.INFO)) 	{Debug.printf_P(x, ##__VA_ARGS__);Debug.println();}
#define debugW_P(x, ...)	if (Debug.isActive(Debug.WARNING)) 	{Debug.printf_P(x, ##__VA_ARGS__);Debug.println();}
#define debugE_P(x, ...)	if (Debug.isActive(Debug.ERROR)) 	{Debug.printf_P(x, ##__VA_ARGS__);Debug.println();}
#define debugA_P(x, ...)	if (Debug.isActive(Debug.ANY)) 		{Debug.printf_P(x, ##__VA_ARGS__);Debug.println();}


#define BUF_SIZE_COMMON (2048)
#define BUF_SIZE_HAN (1280)

#include "IEC6205621.h"
#include "IEC6205675.h"
#include "LNG.h"
#include "LNG2.h"

#include "DataParsers.h"
#include "Timezones.h"

uint8_t commonBuffer[BUF_SIZE_COMMON];
uint8_t hanBuffer[BUF_SIZE_HAN];

HwTools hw;

DNSServer* dnsServer = NULL;

AmsConfiguration config;

RemoteDebug Debug;

EntsoeApi* eapi = NULL;

Timezone* tz = NULL;

AmsWebServer ws(commonBuffer, &Debug, &hw);

MQTTClient *mqtt = NULL;
WiFiClient *mqttClient = NULL;
WiFiClientSecure *mqttSecureClient = NULL;
AmsMqttHandler* mqttHandler = NULL;

Stream *hanSerial;
SoftwareSerial *swSerial = NULL;
HardwareSerial *hwSerial = NULL;
size_t rxBufferSize = 128;
uint8_t rxBufferErrors = 0;

GpioConfig gpioConfig;
MeterConfig meterConfig;
bool mqttEnabled = false;
String topic = "ams";
AmsData meterState;
bool ntpEnabled = false;

bool mdnsEnabled = false;

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


void configFileParse();
void swapWifiMode();
void WiFi_connect();
void WiFi_post_connect();
void MQTT_connect();
void handleNtpChange();
void handleDataSuccess(AmsData* data);
void handleTemperature(unsigned long now);
void handleSystem(unsigned long now);
void handleAutodetect(unsigned long now);
void handleButton(unsigned long now);
void handlePriceApi(unsigned long now);
void handleClear(unsigned long now);
void handleEnergyAccountingChanged();
bool readHanPort();
void setupHanPort(GpioConfig& gpioConfig, uint32_t baud, uint8_t parityOrdinal, bool invert);
void rxerr(int err);
int16_t unwrapData(uint8_t *buf, DataParserContext &context);
void errorBlink();
void printHanReadError(int pos);
void debugPrint(byte *buffer, int start, int length);


#if defined(ESP32)
uint8_t dnsState = 0;
ip_addr_t dns0;
void WiFiEvent(WiFiEvent_t event) {
	switch(event) {
		case ARDUINO_EVENT_WIFI_STA_GOT_IP:
			const ip_addr_t* dns = dns_getserver(0);
			memcpy(&dns0, dns, sizeof(dns0));

			IPAddress res;
			int ret = WiFi.hostByName("hub.amsleser.no", res);
			if(ret == 0) {
				dnsState = 2;
				debugI_P(PSTR("No DNS, probably a closed network"));
			} else {
				debugI_P(PSTR("DNS is present and working, monitoring"));
				dnsState = 1;
			}
			break;
	}
}
#endif


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
	ws.setPriceSettings(entsoe.area, entsoe.currency);
	ea.setFixedPrice(entsoe.fixedPrice / 1000.0, entsoe.currency);
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
	yield();

	float vcc = hw.getVcc();

	if (Debug.isActive(RemoteDebug::INFO)) {
		debugI_P(PSTR("AMS bridge started"));
		debugI_P(PSTR("Voltage: %.2fV"), vcc);
	}

	float vccBootLimit = gpioConfig.vccBootLimit == 0 ? 0 : min(3.29, gpioConfig.vccBootLimit / 10.0); // Make sure it is never above 3.3v
	if(vccBootLimit > 2.5 && vccBootLimit < 3.3 && (gpioConfig.apPin == 0xFF || digitalRead(gpioConfig.apPin) == HIGH)) { // Skip if user is holding AP button while booting (HIGH = button is released)
		if (vcc < vccBootLimit) {
			if(Debug.isActive(RemoteDebug::INFO)) {
				Debug.printf_P(PSTR("(setup) Voltage is too low (%.2f < %.2f), sleeping\n"), vcc, vccBootLimit);
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
	debugD_P(PSTR("ESP32 LittleFS"));
	hasFs = LittleFS.begin(true);
	debugD_P(PSTR(" size: %d"), LittleFS.totalBytes());
#else
	debugD_P(PSTR("ESP8266 LittleFS"));
	hasFs = LittleFS.begin();
#endif
	yield();

	if(hasFs) {
		#if defined(ESP8266)
			LittleFS.gc();
			if(!LittleFS.check()) {
				debugW_P(PSTR("LittleFS filesystem error"));
				if(!LittleFS.format()) {
					debugE_P(PSTR("Unable to format broken filesystem"));
				}
			}
		#endif
		bool flashed = false;
		if(LittleFS.exists(FILE_FIRMWARE)) {
			if (!config.hasConfig()) {
				debugI_P(PSTR("Device has no config, yet a firmware file exists, deleting file."));
			} else if (gpioConfig.apPin == 0xFF || digitalRead(gpioConfig.apPin) == HIGH) {
				if(Debug.isActive(RemoteDebug::INFO)) debugI_P(PSTR("Found firmware"));
				#if defined(ESP8266)
					WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
					WiFi.forceSleepBegin();
				#endif
				int i = 0;
				while(hw.getVcc() > 1.0 && hw.getVcc() < 3.2 && i < 3) {
					if(Debug.isActive(RemoteDebug::INFO)) debugI_P(PSTR(" vcc not optimal, light sleep 10s"));
					#if defined(ESP8266)
						delay(10000);
					#elif defined(ESP32)
						esp_sleep_enable_timer_wakeup(10000000);
						esp_light_sleep_start();
					#endif
					i++;
				}

				debugI_P(PSTR(" flashing"));
				File firmwareFile = LittleFS.open(FILE_FIRMWARE, (char*) "r");
				debugD_P(PSTR(" firmware size: %d"), firmwareFile.size());
				uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
				debugD_P(PSTR(" available: %d"), maxSketchSpace);
				if (!Update.begin(maxSketchSpace, U_FLASH)) {
					if(Debug.isActive(RemoteDebug::ERROR)) {
						debugE_P(PSTR("Unable to start firmware update"));
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
				config.setUpgradeInformation(flashed ? 2 : 0, 0xFF, FirmwareVersion::VersionString, "");
				firmwareFile.close();
			} else {
				debugW_P(PSTR("AP button pressed, skipping firmware update and deleting firmware file."));
			}
			LittleFS.remove(FILE_FIRMWARE);
		} else if(LittleFS.exists(FILE_CFG)) {
			if(Debug.isActive(RemoteDebug::INFO)) debugI_P(PSTR("Found config"));
			configFileParse();
			flashed = true;
		}
		if(flashed) {
			LittleFS.end();
			if(Debug.isActive(RemoteDebug::INFO)) {
				debugI_P(PSTR("Firmware update complete, restarting"));
				Debug.flush();
			}
			delay(250);
			ESP.restart();
			return;
		}
	}
	LittleFS.end();
	yield();

	if(config.hasConfig()) {
		if(Debug.isActive(RemoteDebug::INFO)) config.print(&Debug);
		WiFi_connect();
		handleNtpChange();
		ds.load();
	} else {
		if(Debug.isActive(RemoteDebug::INFO)) {
			debugI_P(PSTR("No configuration, booting AP"));
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
	unsigned long now = millis();
	unsigned long start = now;
	Debug.handle();
	unsigned long end = millis();
	if(end - start > 1000) {
		debugW_P(PSTR("Used %dms to handle debug"), millis()-start);
	}

	handleButton(now);

	if(now > 10000 && now - lastErrorBlink > 3000) {
		errorBlink();
	}

	if(hwSerial != NULL) {
		#if defined ESP8266
		if(hwSerial->hasRxError()) {
			debugE_P(PSTR("Serial RX error"));
			meterState.setLastError(METER_ERROR_RX);
		}
		if(hwSerial->hasOverrun()) {
			rxerr(2);
		}
		#endif
	} else if(swSerial != NULL) {
		if(swSerial->overflow()) {
			rxerr(2);
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
				WiFi_post_connect();
			}
			if(config.isNtpChanged()) {
				handleNtpChange();
			}
			#if defined ESP8266
			if(mdnsEnabled) {
				start = millis();
				MDNS.update();
				end = millis();
				if(end - start > 1000) {
					debugW_P(PSTR("Used %dms to update mDNS"), millis()-start);
				}
			}
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
				handlePriceApi(now);
			} catch(const std::exception& e) {
				debugE_P(PSTR("Exception in ENTSO-E loop (%s)"), e.what());
			}
			start = millis();
			ws.loop();
			end = millis();
			if(end - start > 1000) {
				debugW_P(PSTR("Used %dms to handle web"), millis()-start);
			}
		}
		if(mqtt != NULL) {
			start = millis();
			mqtt->loop();
			delay(10); // Needed to preserve power. After adding this, the voltage is super smooth on a HAN powered device
			end = millis();
			if(end - start > 1000) {
				debugW_P(PSTR("Used %dms to handle mqtt"), millis()-start);
			}
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
		setupHanPort(gpioConfig, meterConfig.baud, meterConfig.parity, meterConfig.invert);
		config.ackMeterChanged();
		if(gcmParser != NULL) {
			delete gcmParser;
			gcmParser = NULL;
		}
	}

	if(config.isEnergyAccountingChanged()) {
		handleEnergyAccountingChanged();
	}
	try {
		start = millis();
		if(readHanPort() || now - meterState.getLastUpdateMillis() > 30000) {
			end = millis();
			if(end - start > 1000) {
				debugW_P(PSTR("Used %dms to read HAN port (true)"), millis()-start);
			}
			handleTemperature(now);
			handleSystem(now);
		} else {
			end = millis();
			if(end - start > 1000) {
				debugW_P(PSTR("Used %dms to read HAN port (false)"), millis()-start);
			}
		}
		if(millis() - meterState.getLastUpdateMillis() > 1800000 && !ds.isHappy()) {
			handleClear(now);
		}
	} catch(const std::exception& e) {
		debugE_P(PSTR("Exception in readHanPort (%s)"), e.what());
		meterState.setLastError(METER_ERROR_EXCEPTION);
	}
	try {
		handleAutodetect(now);
	} catch(const std::exception& e) {
		debugE_P(PSTR("Exception in meter autodetect (%s)"), e.what());
		meterState.setLastError(METER_ERROR_AUTODETECT);
	}

	delay(10); // Needed for auto modem sleep
	start = millis();
	#if defined(ESP32)
		esp_task_wdt_reset();
	#elif defined(ESP8266)
		ESP.wdtFeed();
	#endif
	yield();

	end = millis();
	if(end-start > 1000) {
		debugW_P(PSTR("Used %dms to feed WDT"), end-start);
	}

	if(end-now > 2000) {
		debugW_P(PSTR("loop() used %dms"), end-now);
	}
}

void handleClear(unsigned long now) {
	tmElements_t tm;
	breakTime(time(nullptr), tm);
	if(tm.Minute == 0) {
		AmsData nullData;
		debugI_P(PSTR("Clearing data that have not been updated"));
		ds.update(&nullData);
	}
}

void handleEnergyAccountingChanged() {
	EnergyAccountingConfig *eac = ea.getConfig();
	config.getEnergyAccountingConfig(*eac);
	ea.setup(&ds, eac);
	config.ackEnergyAccountingChange();
}

char ntpServerName[64] = "";

void handleNtpChange() {
	NtpConfig ntp;
	if(config.getNtpConfig(ntp)) {
		tz = resolveTimezone(ntp.timezone);
		if(ntp.enable && strlen(ntp.server) > 0) {
			strcpy(ntpServerName, ntp.server);
		} else if(ntp.enable) {
			strcpy(ntpServerName, "pool.ntp.org");
		} else {
			memset(ntpServerName, 0, 64);
		}
		configTime(tz->toLocal(0), tz->toLocal(JULY1970)-JULY1970, ntpServerName, "", "");
		sntp_servermode_dhcp(ntp.enable && ntp.dhcp ? 1 : 0); // Not implemented on ESP32?
		ntpEnabled = ntp.enable;

		ws.setTimezone(tz);
		ds.setTimezone(tz);
		ea.setTimezone(tz);
	}

	config.ackNtpChange();
}

void handleSystem(unsigned long now) {
	unsigned long start, end;
	if(now - lastSysupdate > 60000) {
		start = millis();
		if(mqtt != NULL && mqttHandler != NULL && WiFi.getMode() != WIFI_AP && WiFi.status() == WL_CONNECTED && mqtt->connected() && !topic.isEmpty()) {
			mqttHandler->publishSystem(&hw, eapi, &ea);
		}
		lastSysupdate = now;
		end = millis();
		if(end - start > 1000) {
			debugW_P(PSTR("Used %dms to send system update to MQTT"), millis()-start);
		}

		#if defined(ESP32)
		if(dnsState == 1) {
			const ip_addr_t* dns = dns_getserver(0);
			if(memcmp(&dns0, dns, sizeof(dns0)) != 0) {
					dns_setserver(0, &dns0);
					debugI_P(PSTR("Had to reset DNS server"));
			}
		}
		#endif
	}
}

void handleTemperature(unsigned long now) {
	unsigned long start, end;
	if(now - lastTemperatureRead > 15000) {
		start = millis();
		if(hw.updateTemperatures()) {
			lastTemperatureRead = now;

			if(mqtt != NULL && mqttHandler != NULL && WiFi.getMode() != WIFI_AP && WiFi.status() == WL_CONNECTED && mqtt->connected() && !topic.isEmpty()) {
				mqttHandler->publishTemperatures(&config, &hw);
			}
		}
		end = millis();
		if(end - start > 1000) {
			debugW_P(PSTR("Used %dms to update temperature"), millis()-start);
		}
	}
}

void handlePriceApi(unsigned long now) {
	unsigned long start, end;
	if(eapi != NULL && ntpEnabled) {
		start = millis();
		if(eapi->loop() && mqtt != NULL && mqttHandler != NULL && mqtt->connected()) {
			end = millis();
			if(end - start > 1000) {
				debugW_P(PSTR("Used %dms to update prices"), millis()-start);
			}

			start = millis();
			mqttHandler->publishPrices(eapi);
			end = millis();
			if(end - start > 1000) {
				debugW_P(PSTR("Used %dms to publish prices to MQTT"), millis()-start);
			}
		} else {
			end = millis();
			if(end - start > 1000) {
				debugW_P(PSTR("Used %dms to handle price API"), millis()-start);
			}
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
		ws.setPriceSettings(entsoe.area, entsoe.currency);
		config.ackEntsoeChange();
		ea.setFixedPrice(entsoe.fixedPrice / 1000.0, entsoe.currency);
	}
}

void handleAutodetect(unsigned long now) {
	if(meterState.getListType() == 0) {
		if(now - meterAutodetectLastChange > 20000 && (meterConfig.baud == 0 || meterConfig.parity == 0)) {
			meterAutodetect = true;
			meterAutoIndex++; // Default is to try the first one in setup()
			debugI_P(PSTR("Meter serial autodetect, swapping to: %d, %d, %s"), bauds[meterAutoIndex], parities[meterAutoIndex], inverts[meterAutoIndex] ? "true" : "false");
			if(meterAutoIndex >= 4) meterAutoIndex = 0;
			setupHanPort(gpioConfig, bauds[meterAutoIndex], parities[meterAutoIndex], inverts[meterAutoIndex]);
			meterAutodetectLastChange = now;
		}
	} else if(meterAutodetect) {
		debugI_P(PSTR("Meter serial autodetected, saving: %d, %d, %s"), bauds[meterAutoIndex], parities[meterAutoIndex], inverts[meterAutoIndex] ? "true" : "false");
		meterAutodetect = false;
		meterConfig.baud = bauds[meterAutoIndex];
		meterConfig.parity = parities[meterAutoIndex];
		meterConfig.invert = inverts[meterAutoIndex];
		config.setMeterConfig(meterConfig);
		setupHanPort(gpioConfig, meterConfig.baud, meterConfig.parity, meterConfig.invert);
	}
}

void handleButton(unsigned long now) {
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
					debugD_P(PSTR("Button was clicked, no action configured"));
				}
				buttonActive = false;
			}
		}
	}
}

void rxerr(int err) {
	if(err == 0) return;
	switch(err) {
		case 1:
			debugD_P(PSTR("Serial break"));
			return;
		case 2:
			debugE_P(PSTR("Serial buffer overflow"));
			rxBufferErrors++;
			if(rxBufferErrors > 3 && rxBufferSize < MAX_RX_BUFFER_SIZE) {
				rxBufferSize += 128;
				debugI_P(PSTR("Increasing RX buffer to %d bytes"), rxBufferSize);
				config.setMeterChanged();
				rxBufferErrors = 0;
			}
			break;
		case 3:
			debugE_P(PSTR("Serial FIFO overflow"));
			break;
		case 4:
			debugW_P(PSTR("Serial frame error"));
			break;
		case 5:
			debugW_P(PSTR("Serial parity error"));
			break;
	}
	meterState.setLastError(90+err);
}

void setupHanPort(GpioConfig& gpioConfig, uint32_t baud, uint8_t parityOrdinal, bool invert) {
	uint8_t pin = gpioConfig.hanPin;

	if(Debug.isActive(RemoteDebug::INFO)) Debug.printf_P(PSTR("(setupHanPort) Setting up HAN on pin %d with baud %d and parity %d\n"), pin, baud, parityOrdinal);

	if(baud == 0) {
		baud = bauds[meterAutoIndex];
		parityOrdinal = parities[meterAutoIndex];
		invert = inverts[meterAutoIndex];
	}
	if(parityOrdinal == 0) {
		parityOrdinal = 3; // 8N1
	}

	SystemConfig sys;
	config.getSystemConfig(sys);
	switch(sys.boardType) {
		case 8: // HAN mosquito: has inverting level shifter
			invert = !invert;
			break;
	}

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
		#elif defined(CONFIG_IDF_TARGET_ESP32S2) ||  defined(CONFIG_IDF_TARGET_ESP32C3)
			hwSerial = &Serial1;
		#endif
	#endif

	if(pin == 0) {
		debugE_P(PSTR("Invalid GPIO configured for HAN"));
		return;
	}

	if(hwSerial != NULL) {
		debugD_P(PSTR("Hardware serial"));
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
		if(rxBufferSize < 256) rxBufferSize = 256; // 64 is default for software serial, 256 for hardware

		hwSerial->setRxBufferSize(rxBufferSize);
		#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32C3)
			hwSerial->begin(baud, serialConfig, -1, -1, invert);
			uart_set_pin(UART_NUM_1, UART_PIN_NO_CHANGE, pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
		#elif defined(ESP32)
			hwSerial->begin(baud, serialConfig, -1, -1, invert);
		#else
			hwSerial->begin(baud, serialConfig, SERIAL_FULL, 1, invert);
		#endif
		
		#if defined(ESP8266)
			if(pin == 3) {
				debugI_P(PSTR("Switching UART0 to pin 1 & 3"));
				Serial.pins(1,3);
			} else if(pin == 113) {
				debugI_P(PSTR("Switching UART0 to pin 15 & 13"));
				Serial.pins(15,13);
			}
		#endif

		#if defined(ESP32)
		hwSerial->onReceiveError(rxerr);
		#endif
		hanSerial = hwSerial;
		if(swSerial != NULL) {
			swSerial->end();
			delete swSerial;
			swSerial = NULL;
		}
	} else {
		debugD_P(PSTR("Software serial"));
		Serial.flush();
		
		if(swSerial == NULL) {
			swSerial = new SoftwareSerial(pin, -1, invert);
		} else {
			swSerial->end();
		}

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

		swSerial->begin(baud, serialConfig, pin, -1, invert, rxBufferSize);
		hanSerial = swSerial;

		Serial.end();
		Serial.begin(115200);
		hwSerial = NULL;
	}

	// The library automatically sets the pullup in Serial.begin()
	if(!gpioConfig.hanPinPullup) {
		debugI_P(PSTR("HAN pin pullup disabled"));
		pinMode(gpioConfig.hanPin, INPUT);
	}

	hanSerial->setTimeout(250);

	// Empty buffer before starting
	while (hanSerial->available() > 0) {
		hanSerial->read();
	}
	#if defined(ESP8266)
	if(hwSerial != NULL) {
		hwSerial->hasOverrun();
	} else if(swSerial != NULL) {
		swSerial->overflow();
	}
	#endif
}

void errorBlink() {
	if(lastError == 3)
		lastError = 0;
	lastErrorBlink = millis();
	while(lastError < 3) {
		switch(lastError++) {
			case 0:
				if(lastErrorBlink - meterState.getLastUpdateMillis() > 30000) {
					debugW_P(PSTR("No HAN data received last 30s, single blink"));
					hw.ledBlink(LED_RED, 1); // If no message received from AMS in 30 sec, blink once
					if(meterState.getLastError() == 0) meterState.setLastError(METER_ERROR_NO_DATA);
					return;
				}
				break;
			case 1:
				if(mqttEnabled && mqtt != NULL && mqtt->lastError() != 0) {
					debugW_P(PSTR("MQTT connection not available, double blink"));
					hw.ledBlink(LED_RED, 2); // If MQTT error, blink twice
					return;
				}
				break;
			case 2:
				if(WiFi.getMode() != WIFI_AP && WiFi.status() != WL_CONNECTED) {
					debugW_P(PSTR("WiFi not connected, tripe blink"));
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
	delay(10);
	yield();

	if (mode != WIFI_AP || !config.hasConfig()) {
		if(Debug.isActive(RemoteDebug::INFO)) debugI_P(PSTR("Swapping to AP mode"));

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
			Debug.begin(F("192.168.4.1"), 23, RemoteDebug::VERBOSE);
		#endif
	} else {
		if(Debug.isActive(RemoteDebug::INFO)) debugI_P(PSTR("Swapping to STA mode"));
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
	unsigned long start, end;
	if(!hanSerial->available()) {
		return false;
	}

	// Before reading, empty serial buffer to increase chance of getting first byte of a data transfer
	if(!serialInit) {
		hanSerial->readBytes(hanBuffer, BUF_SIZE_HAN);
		serialInit = true;
		return false;
	}

	DataParserContext ctx = {0,0,0,0};
	strcpy_P((char*) ctx.system_title, PSTR(""));
	int pos = DATA_PARSE_INCOMPLETE;
	// For each byte received, check if we have a complete frame we can handle
	start = millis();
	while(hanSerial->available() && pos == DATA_PARSE_INCOMPLETE) {
		// If buffer was overflowed, reset
		if(len >= BUF_SIZE_HAN) {
			hanSerial->readBytes(hanBuffer, BUF_SIZE_HAN);
			len = 0;
			debugI_P(PSTR("Buffer overflow, resetting"));
			return false;
		}
		hanBuffer[len++] = hanSerial->read();
		ctx.length = len;
		pos = unwrapData((uint8_t *) hanBuffer, ctx);
		if(ctx.type > 0 && pos >= 0) {
			if(ctx.type == DATA_TAG_DLMS) {
				debugD_P(PSTR("Received valid DLMS at %d"), pos);
			} else if(ctx.type == DATA_TAG_DSMR) {
				debugD_P(PSTR("Received valid DSMR at %d"), pos);
			} else {
				// TODO: Move this so that payload is sent to MQTT
				debugE_P(PSTR("Unknown tag %02X at pos %d"), ctx.type, pos);
				len = 0;
				return false;
			}
		}
		yield();
	}
	end = millis();
	if(end-start > 1000) {
		debugW_P(PSTR("Used %dms to unwrap HAN data"), end-start);
	}

	if(pos == DATA_PARSE_INCOMPLETE) {
		return false;
	} else if(pos == DATA_PARSE_UNKNOWN_DATA) {
		debugW_P(PSTR("Unknown data received"));
		meterState.setLastError(pos);
		len = len + hanSerial->readBytes(hanBuffer+len, BUF_SIZE_HAN-len);
		if(Debug.isActive(RemoteDebug::VERBOSE)) {
			debugV_P(PSTR("  payload:"));
			debugPrint(hanBuffer, 0, len);
		}
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
			delay(10);
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

	AmsData* data = NULL;
	char* payload = ((char *) (hanBuffer)) + pos;
	if(ctx.type == DATA_TAG_DLMS) {
		// If MQTT bytestream payload is selected (mqttHandler == NULL), send the payload to MQTT
		if(mqttEnabled && mqtt != NULL && mqttHandler == NULL) {
			mqtt->publish(topic.c_str(), toHex((byte*) payload, ctx.length));
			mqtt->loop();
			delay(10);
		}

		debugV_P(PSTR("Using application data:"));
		if(Debug.isActive(RemoteDebug::VERBOSE)) debugPrint((byte*) payload, 0, ctx.length);

		// Rudimentary detector for L&G proprietary format, this is terrible code... Fix later
		if(payload[0] == CosemTypeStructure && payload[2] == CosemTypeArray && payload[1] == payload[3]) {
			debugV_P(PSTR("LNG"));
			data = new LNG(payload, meterState.getMeterType(), &meterConfig, ctx, &Debug);
		} else if(payload[0] == CosemTypeStructure && 
			payload[2] == CosemTypeLongUnsigned && 
			payload[5] == CosemTypeLongUnsigned && 
			payload[8] == CosemTypeLongUnsigned && 
			payload[11] == CosemTypeLongUnsigned && 
			payload[14] == CosemTypeLongUnsigned && 
			payload[17] == CosemTypeLongUnsigned
		) {
			debugV_P(PSTR("LNG2"));
			data = new LNG2(payload, meterState.getMeterType(), &meterConfig, ctx, &Debug);
		} else {
			debugV_P(PSTR("DLMS"));
			// TODO: Split IEC6205675 into DataParserKaifa and DataParserObis. This way we can add other means of parsing, for those other proprietary formats
			data = new IEC6205675(payload, meterState.getMeterType(), &meterConfig, ctx);
		}
	} else if(ctx.type == DATA_TAG_DSMR) {
		data = new IEC6205621(payload, tz);
	}
	len = 0;

	if(data != NULL) {
		if(data->getListType() > 0) {
			handleDataSuccess(data);
		}
		delete data;
	}
	yield();
	return true;
}

void handleDataSuccess(AmsData* data) {
	if(rxBufferErrors > 0) rxBufferErrors--;
	if(!hw.ledBlink(LED_GREEN, 1))
		hw.ledBlink(LED_INTERNAL, 1);

	if(mqttEnabled && mqttHandler != NULL && mqtt != NULL) {
		#if defined(ESP32)
			esp_task_wdt_reset();
		#elif defined(ESP8266)
			ESP.wdtFeed();
		#endif
		yield();
		if(mqttHandler->publish(data, &meterState, &ea, eapi)) {
			mqtt->loop();
			delay(10);
		}
	}

	time_t now = time(nullptr);
	if(now < FirmwareVersion::BuildEpoch && data->getListType() >= 3) {
		if(data->getMeterTimestamp() > FirmwareVersion::BuildEpoch) {
			debugI_P(PSTR("Using timestamp from meter"));
			now = data->getMeterTimestamp();
		} else if(data->getPackageTimestamp() > FirmwareVersion::BuildEpoch) {
			debugI_P(PSTR("Using timestamp from meter (DLMS)"));
			now = data->getPackageTimestamp();
		}
		if(now > FirmwareVersion::BuildEpoch) {
			timeval tv { now, 0};
			settimeofday(&tv, nullptr);
		}
	}

	meterState.apply(*data);

	bool saveData = false;
	if(!ds.isHappy() && now > FirmwareVersion::BuildEpoch) { // Must use "isHappy()" in case day state gets reset and lastTimestamp is "now"
		debugD_P(PSTR("Its time to update data storage"));
		tmElements_t tm;
		breakTime(now, tm);
		if(tm.Minute == 0 && data->getListType() >= 3) {
			debugV_P(PSTR(" using actual data"));
			saveData = ds.update(data);
		} else if(tm.Minute == 1 && meterState.getListType() >= 3) {
			debugV_P(PSTR(" using estimated data"));
			saveData = ds.update(&meterState);
		}
		if(saveData) {
			debugI_P(PSTR("Saving data"));
			ds.save();
		}
	}

	if(ea.update(data)) {
		debugI_P(PSTR("Saving energy accounting"));
		ea.save();
		saveData = true; // Trigger LittleFS.end
	}
	if(saveData) {
		LittleFS.end();
	}
}

void printHanReadError(int pos) {
	if(Debug.isActive(RemoteDebug::WARNING)) {
		switch(pos) {
			case DATA_PARSE_BOUNDRY_FLAG_MISSING:
				debugW_P(PSTR("Boundry flag missing"));
				break;
			case DATA_PARSE_HEADER_CHECKSUM_ERROR:
				debugW_P(PSTR("Header checksum error"));
				break;
			case DATA_PARSE_FOOTER_CHECKSUM_ERROR:
				debugW_P(PSTR("Frame checksum error"));
				break;
			case DATA_PARSE_INCOMPLETE:
				debugW_P(PSTR("Received frame is incomplete"));
				break;
			case GCM_AUTH_FAILED:
				debugW_P(PSTR("Decrypt authentication failed"));
				break;
			case GCM_ENCRYPTION_KEY_FAILED:
				debugW_P(PSTR("Setting decryption key failed"));
				break;
			case GCM_DECRYPT_FAILED:
				debugW_P(PSTR("Decryption failed"));
				break;
			case MBUS_FRAME_LENGTH_NOT_EQUAL:
				debugW_P(PSTR("Frame length mismatch"));
				break;
			case DATA_PARSE_INTERMEDIATE_SEGMENT:
				debugI_P(PSTR("Intermediate segment received"));
				break;
			case DATA_PARSE_UNKNOWN_DATA:
				debugW_P(PSTR("Unknown data format %02X"), hanBuffer[0]);
				break;
			default:
				debugW_P(PSTR("Unspecified error while reading data: %d"), pos);
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
			Debug.println(F(""));
		else if ((i - start + 1) % 4 == 0)
			Debug.print(F(" "));

		yield(); // Let other get some resources too
	}
	Debug.println(F(""));
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
			if (Debug.isActive(RemoteDebug::INFO)) debugI_P(PSTR("Not connected to WiFi, closing resources"));
			if(mqtt != NULL) {
				mqtt->disconnect();
				mqtt->loop();
				delay(10);
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

		if (Debug.isActive(RemoteDebug::INFO)) debugI_P(PSTR("Connecting to WiFi network: %s"), wifi.ssid);

		wifiReconnectCount++;

		#if defined(ESP32)
			if(strlen(wifi.hostname) > 0) {
				WiFi.setHostname(wifi.hostname);
			}	
			WiFi.onEvent(WiFiEvent);
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
				debugE_P(PSTR("Static IP configuration is invalid, not using"));
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
			if (Debug.isActive(RemoteDebug::ERROR)) debugI_P(PSTR("Unable to start WiFi"));
		}
  	}
}

void WiFi_post_connect() {
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
			debugI_P(PSTR("Successfully connected to WiFi!"));
			debugI_P(PSTR("IP:  %s"), WiFi.localIP().toString().c_str());
			debugI_P(PSTR("GW:  %s"), WiFi.gatewayIP().toString().c_str());
			debugI_P(PSTR("DNS: %s"), WiFi.dnsIP().toString().c_str());
		}
		mdnsEnabled = false;
		if(strlen(wifi.hostname) > 0 && wifi.mdns) {
			debugD_P(PSTR("mDNS is enabled, using host: %s"), wifi.hostname);
			if(MDNS.begin(wifi.hostname)) {
				mdnsEnabled = true;
				MDNS.addService(F("http"), F("tcp"), 80);
			} else {
				debugE_P(PSTR("Failed to set up mDNS!"));
			}
		}
	}

	MqttConfig mqttConfig;
	if(config.getMqttConfig(mqttConfig)) {
		mqttEnabled = strlen(mqttConfig.host) > 0;
		ws.setMqttEnabled(mqttEnabled);
	}
}

void mqttMessageReceived(String &topic, String &payload) {
    debugI_P(PSTR("Received message for topic %s"), topic.c_str() );
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
				debugE_P(PSTR("Ended up in default case while unwrapping...(tag %02X)"), tag);
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
					debugV_P(PSTR("HDLC frame:"));
					// If MQTT bytestream payload is selected (mqttHandler == NULL), send the payload to MQTT
					if(mqttEnabled && mqtt != NULL && mqttHandler == NULL) {
						mqtt->publish(topic.c_str(), toHex(buf, curLen));
						mqtt->loop();
						delay(10);
					}
					break;
				case DATA_TAG_MBUS:
					debugV_P(PSTR("MBUS frame:"));
					// If MQTT bytestream payload is selected (mqttHandler == NULL), send the payload to MQTT
					if(mqttEnabled && mqtt != NULL && mqttHandler == NULL) {
						mqtt->publish(topic.c_str(), toHex(buf, curLen));
						mqtt->loop();
						delay(10);
					}
					break;
				case DATA_TAG_GBT:
					debugV_P(PSTR("GBT frame:"));
					break;
				case DATA_TAG_GCM:
					debugV_P(PSTR("GCM frame:"));
					break;
				case DATA_TAG_LLC:
					debugV_P(PSTR("LLC frame:"));
					break;
				case DATA_TAG_DLMS:
					debugV_P(PSTR("DLMS frame:"));
					break;
				case DATA_TAG_DSMR:
					debugV_P(PSTR("DSMR frame:"));
					if(mqttEnabled && mqtt != NULL && mqttHandler == NULL) {
						mqtt->publish(topic.c_str(), (char*) buf);
						mqtt->loop();
						delay(10);
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
	debugE_P(PSTR("Got to end of unwrap method..."));
	return DATA_PARSE_UNKNOWN_DATA;
}

unsigned long lastMqttRetry = -10000;
void MQTT_connect() {
	MqttConfig mqttConfig;
	if(!config.getMqttConfig(mqttConfig) || strlen(mqttConfig.host) == 0) {
		if(Debug.isActive(RemoteDebug::WARNING)) debugW_P(PSTR("No MQTT config"));
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
			debugD_P(PSTR("Disconnecting MQTT before connecting"));
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
		mqtt = new MQTTClient(128);
		mqtt->dropOverflow(true);
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
			HomeAssistantConfig haconf;
			SystemConfig sys;
			config.getHomeAssistantConfig(haconf);
			config.getSystemConfig(sys);
			mqttHandler = new HomeAssistantMqttHandler(mqtt, (char*) commonBuffer, mqttConfig.clientId, mqttConfig.publishTopic, sys.boardType, haconf, &hw);
			break;
	}

	time_t epoch = time(nullptr);
	if(mqttConfig.ssl) {
		if(epoch < FirmwareVersion::BuildEpoch) {
			debugI_P(PSTR("NTP not ready for MQTT SSL"));
			return;
		}
		debugI_P(PSTR("MQTT SSL is configured (%dkb free heap)"), ESP.getFreeHeap());
		if(mqttSecureClient == NULL) {
			mqttSecureClient = new WiFiClientSecure();
			#if defined(ESP8266)
				mqttSecureClient->setBufferSizes(512, 512);
				debugD_P(PSTR("ESP8266 firmware does not have enough memory..."));
				return;
			#endif
		
			if(LittleFS.begin()) {
				File file;

				if(LittleFS.exists(FILE_MQTT_CA)) {
					debugI_P(PSTR("Found MQTT CA file (%dkb free heap)"), ESP.getFreeHeap());
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
							debugI_P(PSTR("Found MQTT certificate file (%dkb free heap)"), ESP.getFreeHeap());
							file = LittleFS.open(FILE_MQTT_CERT, (char*) "r");
							BearSSL::X509List *serverCertList = new BearSSL::X509List(file);
							file.close();

							debugI_P(PSTR("Found MQTT key file (%dkb free heap)"), ESP.getFreeHeap());
							file = LittleFS.open(FILE_MQTT_KEY, (char*) "r");
							BearSSL::PrivateKey *serverPrivKey = new BearSSL::PrivateKey(file);
							file.close();

							debugD_P(PSTR("Setting client certificates (%dkb free heap)"), ESP.getFreeHeap());
							mqttSecureClient->setClientRSACert(serverCertList, serverPrivKey);
						#elif defined(ESP32)
							debugI_P(PSTR("Found MQTT certificate file (%dkb free heap)"), ESP.getFreeHeap());
							file = LittleFS.open(FILE_MQTT_CERT, (char*) "r");
							mqttSecureClient->loadCertificate(file, file.size());
							file.close();

							debugI_P(PSTR("Found MQTT key file (%dkb free heap)"), ESP.getFreeHeap());
							file = LittleFS.open(FILE_MQTT_KEY, (char*) "r");
							mqttSecureClient->loadPrivateKey(file, file.size());
							file.close();
						#endif
						mqttClient = mqttSecureClient;
					}
				}

				LittleFS.end();
				debugD_P(PSTR("MQTT SSL setup complete (%dkb free heap)"), ESP.getFreeHeap());
			}
		}
	}
	
	if(mqttClient == NULL) {
		mqttClient = new WiFiClient();
	}

	if(Debug.isActive(RemoteDebug::INFO)) {
		debugI_P(PSTR("Connecting to MQTT %s:%d"), mqttConfig.host, mqttConfig.port);
	}
	
	mqtt->begin(mqttConfig.host, mqttConfig.port, *mqttClient);

	#if defined(ESP8266)
		if(mqttSecureClient) {
			time_t epoch = time(nullptr);
			debugD_P(PSTR("Setting NTP time %lu for secure MQTT connection"), epoch);
			mqttSecureClient->setX509Time(epoch);
		}
	#endif

	// Connect to a unsecure or secure MQTT server
	if ((strlen(mqttConfig.username) == 0 && mqtt->connect(mqttConfig.clientId)) ||
		(strlen(mqttConfig.username) > 0 && mqtt->connect(mqttConfig.clientId, mqttConfig.username, mqttConfig.password))) {
		if (Debug.isActive(RemoteDebug::INFO)) debugI_P(PSTR("Successfully connected to MQTT!"));
		
		if(mqttHandler != NULL) {
			mqttHandler->publishSystem(&hw, eapi, &ea);
		}

		// Subscribe to the chosen MQTT topic, if set in configuration
		if (strlen(mqttConfig.subscribeTopic) > 0) {
            mqtt->onMessage(mqttMessageReceived);
			mqtt->subscribe(String(mqttConfig.subscribeTopic) + "/#");
			debugI_P(PSTR("  Subscribing to [%s]\n"), mqttConfig.subscribeTopic);
		}
	} else {
		if (Debug.isActive(RemoteDebug::ERROR)) {
			debugE_P(PSTR("Failed to connect to MQTT: %d"), mqtt->lastError());
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
	debugD_P(PSTR("Parsing config file"));

	if(!LittleFS.exists(FILE_CFG)) {
		debugW_P(PSTR("Config file does not exist"));
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
	bool lHa = false;
	bool lNtp = false;
	bool lEntsoe = false;
	bool lEac = false;
	bool sEa = false;
	bool sDs = false;

	ds.load();

	SystemConfig sys;
	WiFiConfig wifi;
	MqttConfig mqtt;
	WebConfig web;
	MeterConfig meter;
	GpioConfig gpio;
	DomoticzConfig domo;
	HomeAssistantConfig haconf;
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
				debugD_P(PSTR("Found non-ascii, shortening line from %d to %d"), size, i);
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
			strcpy(web.password, buf+12);
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
		} else if(strncmp_P(buf, PSTR("gpioHanPinPullup "), 17) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.hanPinPullup = String(buf+17).toInt() == 1;
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
			gpio.vccOffset = String(buf+14).toFloat() * 100;
		} else if(strncmp_P(buf, PSTR("gpioVccMultiplier "), 18) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.vccMultiplier = String(buf+18).toFloat() * 1000;
		} else if(strncmp_P(buf, PSTR("gpioVccBootLimit "), 17) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.vccBootLimit = String(buf+17).toFloat() * 10;
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
		} else if(strncmp_P(buf, PSTR("homeAssistantDiscoveryPrefix "), 29) == 0) {
			if(!lHa) { config.getHomeAssistantConfig(haconf); lHa = true; };
			strcpy(haconf.discoveryPrefix, buf+29);
		} else if(strncmp_P(buf, PSTR("homeAssistantDiscoveryHostname "), 31) == 0) {
			if(!lHa) { config.getHomeAssistantConfig(haconf); lHa = true; };
			strcpy(haconf.discoveryHostname, buf+31);
		} else if(strncmp_P(buf, PSTR("homeAssistantDiscoveryNameTag "), 30) == 0) {
			if(!lHa) { config.getHomeAssistantConfig(haconf); lHa = true; };
			strcpy(haconf.discoveryNameTag, buf+30);
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
			entsoe.multiplier = String(buf+17).toFloat() * 1000;
		} else if(strncmp_P(buf, PSTR("entsoeFixedPrice "), 17) == 0) {
			if(!lEntsoe) { config.getEntsoeConfig(entsoe); lEntsoe = true; };
			entsoe.fixedPrice = String(buf+17).toFloat() * 1000;
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
                0, 0, 0, // Last month import, export and accuracy
                0, 0, // Peak 1
                0, 0, // Peak 2
                0, 0, // Peak 3
                0, 0, // Peak 4
                0, 0 // Peak 5
            };
			uint8_t peak = 0;
			uint64_t totalImport = 0, totalExport = 0;
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
						float val = String(pch).toFloat();
						if(val > 0.0) {
							ead.peaks[0] = { 1, (uint16_t) (val*100) };
						}
					} else if(i == 3) {
						float val = String(pch).toFloat();
						ead.costYesterday = val * 100;
					} else if(i == 4) {
						float val = String(pch).toFloat();
						ead.costThisMonth = val * 100;
					} else if(i == 5) {
						float val = String(pch).toFloat();
						ead.costLastMonth = val * 100;
					} else if(i >= 6 && i < 18) {
						uint8_t hour = i-6;					
						{			
							long val = String(pch).toInt();
							ead.peaks[peak].day = val;
						} 
						pch = strtok (NULL, " ");
						i++;
						{
							float val = String(pch).toFloat();
							ead.peaks[peak].value = val * 100;
						}
						peak++;
					}
				} else {
					if(i == 1) {
						long val = String(pch).toInt();
						ead.month = val;
					} else if(i == 2) {
						float val = String(pch).toFloat();
						ead.costYesterday = val * 100;
					} else if(i == 3) {
						float val = String(pch).toFloat();
						ead.costThisMonth = val * 100;
					} else if(i == 4) {
						float val = String(pch).toFloat();
						ead.costLastMonth = val * 100;
					} else if(i == 5) {
						float val = String(pch).toFloat();
						ead.incomeYesterday= val * 100;
					} else if(i == 6) {
						float val = String(pch).toFloat();
						ead.incomeThisMonth = val * 100;
					} else if(i == 7) {
						float val = String(pch).toFloat();
						ead.incomeLastMonth = val * 100;
					} else if(i >= 8 && i < 18) {
						uint8_t hour = i-8;		
						{			
							long val = String(pch).toInt();
							ead.peaks[peak].day = val;
						} 
						pch = strtok (NULL, " ");
						i++;
						{
							float val = String(pch).toFloat();
							ead.peaks[peak].value = val * 100;
						}
						peak++;
					} else if(i == 18) {
						float val = String(pch).toFloat();
						totalImport = val * 1000;
					} else if(i == 19) {
						float val = String(pch).toFloat();
						totalExport = val * 1000;
					}
				}
				pch = strtok (NULL, " ");
				i++;
			}
            uint8_t accuracy = 0;
            uint64_t importUpdate = totalImport, exportUpdate = totalExport;
			while(importUpdate > UINT32_MAX || exportUpdate > UINT32_MAX) {
                accuracy++;
                importUpdate = totalImport / pow(10, accuracy);
                exportUpdate = totalExport / pow(10, accuracy);
            }
            ead.lastMonthImport = importUpdate;
            ead.lastMonthExport = exportUpdate;

			ead.version = 6;
			ea.setData(ead);
			sEa = true;
		}
		memset(buf, 0, 1024);
	}

	debugD_P(PSTR("Deleting config file"));
	file.close();
	if(!LittleFS.remove(FILE_CFG)) {
		debugW_P(PSTR("Unable to remove config file, formatting filesystem"));
		if(!sDs) {
			ds.load();
			sDs = true;
		}
		if(!sEa) {
			ea.load();
			sEa = true;
		}
		if(!LittleFS.format()) {
			debugE_P(PSTR("Unable to format broken filesystem"));
		}
	}

	debugI_P(PSTR("Saving configuration now..."));
	Serial.flush();
	if(lSys) config.setSystemConfig(sys);
	if(lWiFi) config.setWiFiConfig(wifi);
	if(lMqtt) config.setMqttConfig(mqtt);
	if(lWeb) config.setWebConfig(web);
	if(lMeter) config.setMeterConfig(meter);
	if(lGpio) config.setGpioConfig(gpio);
	if(lDomo) config.setDomoticzConfig(domo);
	if(lHa) config.setHomeAssistantConfig(haconf);
	if(lNtp) config.setNtpConfig(ntp);
	if(lEntsoe) config.setEntsoeConfig(entsoe);
	if(lEac) config.setEnergyAccountingConfig(eac);
	if(sDs) ds.save();
	if(sEa) ea.save();
	config.save();
	LittleFS.end();
}
