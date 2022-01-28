/**
 * @brief ESP8266 based program to receive data from AMS electric meters and send to MQTT
 * 
 * @details Originally developed by Roar Fredriksen, this program was created to receive data from
 * AMS electric meters via M-Bus, decode and send to a MQTT broker. The data packet structure 
 * supported by this software is specific to Norwegian meters, but may also support data from
 * electricity providers in other countries. It was originally based on ESP8266, but have also been 
 * adapted to work with ESP32.
 * 
 * @author Gunnar Skjold (@gskjold)
 * Maintainer of current code
 * https://github.com/gskjold/AmsToMqttBridge
 */
#if defined(ESP8266)
ADC_MODE(ADC_VCC);
#endif

#if defined(ESP32)
#include <esp_task_wdt.h>
#endif
#define WDT_TIMEOUT 30

#include "AmsToMqttBridge.h"
#include "AmsStorage.h"
#include "AmsDataStorage.h"
#include "EnergyAccounting.h"
#include <MQTT.h>
#include <DNSServer.h>
#include <lwip/apps/sntp.h>

#include "HwTools.h"
#include "entsoe/EntsoeApi.h"

#include "web/AmsWebServer.h"
#include "AmsConfiguration.h"

#include "mqtt/AmsMqttHandler.h"
#include "mqtt/JsonMqttHandler.h"
#include "mqtt/RawMqttHandler.h"
#include "mqtt/DomoticzMqttHandler.h"

#include "Uptime.h"

#include "RemoteDebug.h"

#define BUF_SIZE (1024)
#include "ams/hdlc.h"
#include "MbusAssembler.h"

#include "IEC6205621.h"
#include "IEC6205675.h"

HwTools hw;

DNSServer* dnsServer = NULL;

AmsConfiguration config;

RemoteDebug Debug;

EntsoeApi* eapi = NULL;

Timezone* tz;

AmsWebServer ws(&Debug, &hw);

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
	if(gpioConfig.apPin >= 0)
		pinMode(gpioConfig.apPin, INPUT_PULLUP);

	config.loadTempSensors();
	hw.setup(&gpioConfig, &config);
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

	DebugConfig debug;
	if(config.getDebugConfig(debug)) {
		Debug.setSerialEnabled(debug.serial);
	}
	#if DEBUG_MODE
		Debug.setSerialEnabled(true);
	#endif
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
		bool flashed = false;
		if(LittleFS.exists(FILE_FIRMWARE)) {
			if (gpioConfig.apPin == 0xFF || digitalRead(gpioConfig.apPin) == HIGH) {
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
		}

		ds.load();
	} else {
		if(Debug.isActive(RemoteDebug::INFO)) {
			debugI("No configuration, booting AP");
		}
		swapWifiMode();
	}

	ea.setup(&ds, eapi);
	ws.setup(&config, &gpioConfig, &meterConfig, &meterState, &ds, &ea);

	#if defined(ESP32)
		esp_task_wdt_init(WDT_TIMEOUT, true);
 		esp_task_wdt_add(NULL);
    #elif defined(ESP8266)
		ESP.wdtEnable(WDT_TIMEOUT);
	#endif
}

int buttonTimer = 0;
bool buttonActive = false;
unsigned long longPressTime = 5000;
bool longPressActive = false;

bool wifiConnected = false;

unsigned long lastTemperatureRead = 0;
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
						Debug.begin(wifi.hostname, (uint8_t) debug.level);
						Debug.setSerialEnabled(debug.serial);
						if(!debug.telnet) {
							Debug.stop();
						}
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
			#endif
			ws.loop();
		}
		if(mqtt != NULL) { // Run loop regardless, to let MQTT do its work.
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
uint8_t buf[BUF_SIZE];
MbusAssembler* ma = NULL;
int currentMeterType = -1;
bool readHanPort() {
	if(!hanSerial->available()) return false;

	if(currentMeterType == -1) {
		hanSerial->readBytes(buf, BUF_SIZE);
		currentMeterType = 0;
		return false;
	}
	if(currentMeterType == 0) {
		uint8_t flag = hanSerial->read();
		if(flag == 0x7E || flag == 0x68) currentMeterType = 1;
		else currentMeterType = 2;
		hanSerial->readBytes(buf, BUF_SIZE);
		return false;
	}
	CosemDateTime timestamp = {0};
	AmsData data;
	if(currentMeterType == 1) {
		int pos = HDLC_FRAME_INCOMPLETE;
		while(hanSerial->available() && pos == HDLC_FRAME_INCOMPLETE) {
			buf[len++] = hanSerial->read();
			pos = HDLC_validate((uint8_t *) buf, len, hc, &timestamp);
		}
		if(len > 0) {
			if(len >= BUF_SIZE) {
				hanSerial->readBytes(buf, BUF_SIZE);
				len = 0;
				debugI("Buffer overflow, resetting");
				return false;
			}
			if(pos == MBUS_FRAME_INTERMEDIATE_SEGMENT) {
				debugI("Intermediate segment");
				if(ma == NULL) {
					ma = new MbusAssembler();
				}
				if(ma->append((uint8_t *) buf, len) < 0)
					pos = -77;
				if(Debug.isActive(RemoteDebug::VERBOSE)) {
					debugD("Frame dump (%db):", len);
					debugPrint(buf, 0, len);
				}
				len = 0;
				return false;
			} else if(pos == MBUS_FRAME_LAST_SEGMENT) {
				debugI("Final segment");
				if(Debug.isActive(RemoteDebug::VERBOSE)) {
					debugD("Frame dump (%db):", len);
					debugPrint(buf, 0, len);
				}
				if(ma->append((uint8_t *) buf, len) >= 0) {
					len = ma->write((uint8_t *) buf);
					pos = HDLC_validate((uint8_t *) buf, len, hc, &timestamp);
				} else {
					pos = -77;
				}
			}
			if(pos == HDLC_FRAME_INCOMPLETE) {
				return false;
			}
			for(int i = len; i<BUF_SIZE; i++) {
				buf[i] = 0x00;
			}
			if(pos == HDLC_ENCRYPTION_CONFIG_MISSING) {
				hc = new HDLCConfig();
				memcpy(hc->encryption_key, meterConfig.encryptionKey, 16);
				memcpy(hc->authentication_key, meterConfig.authenticationKey, 16);
			}
			if(Debug.isActive(RemoteDebug::VERBOSE)) {
				debugD("Frame dump (%db):", len);
				debugPrint(buf, 0, len);
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
			len = 0;
			while(hanSerial->available()) hanSerial->read();
			if(pos > 0) {
				debugD("Valid data, start at byte %d", pos);
				data = IEC6205675(((char *) (buf)) + pos, meterState.getMeterType(), meterConfig.distributionSystem, timestamp, hc);
			} else {
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
							debugW("Unknown data format %02X", buf[0]);
							currentMeterType = 0;
							break;
						default:
							debugW("Unspecified error while reading data: %d", pos);
					}
				}
				return false;
			}
		} else {
			return false;
		}
	} else if(currentMeterType == 2) {
		String payload = hanSerial->readString();
		data = IEC6205621(payload);
		if(data.getListType() == 0) {
			currentMeterType = 1;
			return false;
		} else {
			if(Debug.isActive(RemoteDebug::DEBUG)) {
				debugD("Frame dump: %d", payload.length());
				debugD("%s", payload.c_str());
			}
		}
	}

	if(data.getListType() > 0) {
		if(!hw.ledBlink(LED_GREEN, 1))
			hw.ledBlink(LED_INTERNAL, 1);
		if(mqttEnabled && mqttHandler != NULL && mqtt != NULL) {
			if(mqttHandler->publish(&data, &meterState)) {
				if(data.getListType() == 3 && eapi != NULL) {
					mqttHandler->publishPrices(eapi);
				}
				if(data.getListType() >= 2) {
					mqttHandler->publishSystem(&hw);
				}
			}
			if(mqtt != NULL) {
				mqtt->loop();
				delay(10);
			}
		}

		time_t now = time(nullptr);
		if(now < EPOCH_2021_01_01 && data.getListType() == 3) {
			if(data.getMeterTimestamp() > EPOCH_2021_01_01) {
				debugI("Using timestamp from meter");
				now = data.getMeterTimestamp();
			} else if(data.getPackageTimestamp() > EPOCH_2021_01_01) {
				debugI("Using timestamp from meter (DLMS)");
				now = data.getPackageTimestamp();
			}
			if(now > EPOCH_2021_01_01) {
				timeval tv { now, 0};
				settimeofday(&tv, nullptr);
			}
		}
		if(meterState.getListType() < 3 && now > EPOCH_2021_01_01) {
			// TODO: Load an estimated value from dayplot
		}

		meterState.apply(data);

		if(ds.update(&data)) {
			debugI("Saving day plot");
			ds.save();
		}
		if(ea.update(&data)) {
			debugI("Saving energy accounting");
			ea.save();
		}
	}
	delay(1);
	return true;
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
		if(strlen(wifi.hostname) > 0) {
			#if defined(ESP8266)
			WiFi.hostname(wifi.hostname);
			#elif defined(ESP32)
			WiFi.setHostname(wifi.hostname);
			#endif
		}	
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
		mqtt = new MQTTClient(512);
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
			mqttHandler = new JsonMqttHandler(mqtt, mqttConfig.clientId, mqttConfig.publishTopic, &hw);
			break;
		case 1:
		case 2:
			mqttHandler = new RawMqttHandler(mqtt, mqttConfig.publishTopic, mqttConfig.payloadFormat == 2);
			break;
		case 3:
			DomoticzConfig domo;
			config.getDomoticzConfig(domo);
			mqttHandler = new DomoticzMqttHandler(mqtt, domo);
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
					char buf[64];
					mqttSecureClient->getLastSSLError(buf,64);
					Debug.println(buf);
				}
			#endif
		}
	}
	yield();
}
