/**
 * @brief ESP8266 based program to receive data from AMS electric meters and send to MQTT
 * 
 * @details Originally developed by Roar Fredriksen, this program was created to receive data from
 * AMS electric meters via M-Bus, decode and send to a MQTT broker. The data packet structure 
 * supported by this software is specific to Norwegian meters, but may also support data from
 * electricity providers in other countries. It was originally based on ESP8266, but have also been 
 * adapted to work with ESP32.
 * 
 * @author Roar Fredriksen (@roarfred)
 * The original developer for this project
 * https://github.com/roarfred/AmsToMqttBridge
 * 
 * @author Gunnar Skjold (@gskjold)
 * Maintainer of current code
 * https://github.com/gskjold/AmsToMqttBridge
 */

#include "AmsToMqttBridge.h"
#include "AmsStorage.h"
#define ARDUINOJSON_POSITIVE_EXPONENTIATION_THRESHOLD 1e9
#include <ArduinoJson.h>
#include <MQTT.h>
#include <DNSServer.h>
#include <lwip/apps/sntp.h>

#if defined(ESP8266)
ADC_MODE(ADC_VCC);  
#endif   

#include "HwTools.h"
#include "entsoe/EntsoeApi.h"

#include "web/AmsWebServer.h"
#include "AmsConfiguration.h"
#include "HanReader.h"
#include "HanToJson.h"

#include "Aidon.h"
#include "Kaifa.h"
#include "Kamstrup.h"
#include "Omnipower.h"

#include "Uptime.h"

#define WEBSOCKET_DISABLED true
#include "RemoteDebug.h"

HwTools hw;

DNSServer* dnsServer = NULL;

AmsConfiguration config;

RemoteDebug Debug;

EntsoeApi eapi(&Debug);

Timezone* tz;

AmsWebServer ws(&Debug, &hw, &eapi);

MQTTClient mqtt(512);

HanReader hanReader;

Stream *hanSerial;

GpioConfig gpioConfig;
MeterConfig meterConfig;
DomoticzConfig* domoConfig = NULL;
float energy = 0.0;
bool mqttEnabled = false;
String clientId = "ams-reader";
uint8_t payloadFormat = 0;
String topic = "ams";
AmsData meterState;

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
			gpioConfig.vccPin = 35;
			gpioConfig.vccMultiplier = 2250;
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

	hw.setup(&gpioConfig, &config);
	hw.ledBlink(LED_INTERNAL, 1);
	hw.ledBlink(LED_RED, 1);
	hw.ledBlink(LED_YELLOW, 1);
	hw.ledBlink(LED_GREEN, 1);
	hw.ledBlink(LED_BLUE, 1);

	EntsoeConfig entsoe;
	if(config.getEntsoeConfig(entsoe)) {
		eapi.setup(entsoe);
	}

	bool shared = false;
	config.getMeterConfig(meterConfig);
	if(gpioConfig.hanPin == 3) {
		shared = true;
		switch(meterConfig.type) {
			case METER_TYPE_KAMSTRUP:
			case METER_TYPE_OMNIPOWER:
				Serial.begin(2400, SERIAL_8N1);
				break;
			default:
				Serial.begin(2400, SERIAL_8E1);
				break;
		}
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
	if(vccBootLimit > 0 && (gpioConfig.apPin == 0xFF || digitalRead(gpioConfig.apPin) == HIGH)) { // Skip if user is holding AP button while booting (HIGH = button is released)
		if (vcc < vccBootLimit) {
			if(Debug.isActive(RemoteDebug::INFO)) {
				debugI("Voltage is too low, sleeping");
				Serial.flush();
			}
			ESP.deepSleep(10000000);    //Deep sleep to allow output cap to charge up
		}  
	}

	WiFi.disconnect(true);
	WiFi.softAPdisconnect(true);
	WiFi.mode(WIFI_OFF);

	bool spiffs = false;
#if defined(ESP32)
	debugD("ESP32 SPIFFS");
	spiffs = SPIFFS.begin(true);
#else
	debugD("ESP8266 SPIFFS");
	spiffs = SPIFFS.begin();
#endif
	delay(1);

	if(spiffs) {
		bool flashed = false;
		if(SPIFFS.exists(FILE_FIRMWARE)) {
			if(Debug.isActive(RemoteDebug::INFO)) debugI("Found firmware");
#if defined(ESP8266)
			WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
			WiFi.forceSleepBegin();
#endif
			int i = 0;
			while(hw.getVcc() < 3.2 && i < 3) {
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
			File firmwareFile = SPIFFS.open(FILE_FIRMWARE, "r");
			debugD(" firmware size: %d", firmwareFile.size());
			uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
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
			SPIFFS.remove(FILE_FIRMWARE);
		}
		SPIFFS.end();
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
	delay(1);

	if(config.hasConfig()) {
		if(Debug.isActive(RemoteDebug::INFO)) config.print(&Debug);
		WiFi_connect();
		
		NtpConfig ntp;
		if(config.getNtpConfig(ntp)) {
			if(ntp.enable) {
				configTime(ntp.offset*10, ntp.summerOffset*10, ntp.server);
				sntp_servermode_dhcp(ntp.dhcp ? 1 : 0);
			}
			TimeChangeRule std = {"STD", Last, Sun, Oct, 3, ntp.offset / 6};
			TimeChangeRule dst = {"DST", Last, Sun, Mar, 2, (ntp.offset + ntp.summerOffset) / 6};
			tz = new Timezone(dst, std);
			ws.setTimezone(tz);
		}
	} else {
		if(Debug.isActive(RemoteDebug::INFO)) {
			debugI("No configuration, booting AP");
		}
		swapWifiMode();
	}

	ws.setup(&config, &gpioConfig, &meterConfig, &meterState, &mqtt);
}

int buttonTimer = 0;
bool buttonActive = false;
unsigned long longPressTime = 5000;
bool longPressActive = false;

bool wifiConnected = false;

unsigned long lastTemperatureRead = 0;
unsigned long lastSuccessfulRead = 0;
unsigned long lastErrorBlink = 0; 
int lastError = 0;

String toHex(uint8_t* in) {
	String hex;
	for(int i = 0; i < sizeof(in)*2; i++) {
		if(in[i] < 0x10) {
			hex += '0';
		}
		hex += String(in[i], HEX);
	}
	hex.toUpperCase();
	return hex;
}

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

	if(now - lastTemperatureRead > 15000) {
		unsigned long start = millis();
		hw.updateTemperatures();
		lastTemperatureRead = now;

		uint8_t c = hw.getTempSensorCount();

		if(WiFi.getMode() != WIFI_AP && WiFi.status() == WL_CONNECTED && mqtt.connected() && !topic.isEmpty()) {
			if(payloadFormat == 1 || payloadFormat == 2) {
				for(int i = 0; i < c; i++) {
					TempSensorData* data = hw.getTempSensorData(i);
					if(data->lastValidRead > -85) {
						if(data->changed || payloadFormat == 2) {
							mqtt.publish(topic + "/temperature/" + toHex(data->address), String(data->lastValidRead, 2));
							data->changed = false;
						}
					}
				}
			} else if(payloadFormat == 0) {
				StaticJsonDocument<512> json;
				JsonObject temps = json.createNestedObject("temperatures");
				for(int i = 0; i < c; i++) {
					TempSensorData* data = hw.getTempSensorData(i);
					if(data->lastValidRead > -85) {
						TempSensorConfig* conf = config.getTempSensorConfig(data->address);
						JsonObject obj = temps.createNestedObject(toHex(data->address));
						if(conf != NULL) {
							obj["name"] = conf->name;
						}
						obj["value"] = serialized(String(data->lastValidRead, 2));
					}
					data->changed = false;
				}
				String msg;
				serializeJson(json, msg);
				mqtt.publish(topic, msg.c_str());
			}
		}
		debugD("Used %d ms to update temperature", millis()-start);
	}

	// Only do normal stuff if we're not booted as AP
	if (WiFi.getMode() != WIFI_AP) {
		if (WiFi.status() != WL_CONNECTED) {
			wifiConnected = false;
			Debug.stop();
			WiFi_connect();
		} else {
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
				}
			}
			if(config.isNtpChanged()) {
				NtpConfig ntp;
				if(config.getNtpConfig(ntp) && ntp.enable && strlen(ntp.server) > 0) {
					configTime(ntp.offset*10, ntp.summerOffset*10, ntp.server);
					sntp_servermode_dhcp(ntp.dhcp ? 1 : 0);
				}

				if(tz != NULL) delete tz;
				TimeChangeRule std = {"STD", Last, Sun, Oct, 3, ntp.offset / 6};
				TimeChangeRule dst = {"DST", Last, Sun, Mar, 2, (ntp.offset + ntp.summerOffset) / 6};
				tz = new Timezone(dst, std);
				ws.setTimezone(tz);

				config.ackNtpChange();
			}
			#if defined ESP8266
			MDNS.update();
			#endif

			if(now > 10000 && now - lastErrorBlink > 3000) {
				errorBlink();
			}

			if (mqttEnabled || config.isMqttChanged()) {
				mqtt.loop();
				delay(10); // Needed to preserve power. After adding this, the voltage is super smooth on a HAN powered device
				if(!mqtt.connected() || config.isMqttChanged()) {
					MQTT_connect();
				}
				if(payloadFormat == 1) {
					sendSystemStatusToMqtt();
				}
			} else if(mqtt.connected()) {
				mqtt.disconnect();
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
	}

	if(config.isMeterChanged()) {
		config.getMeterConfig(meterConfig);
		setupHanPort(gpioConfig.hanPin, meterConfig.type);
		config.ackMeterChanged();
	}
	delay(1);
	readHanPort();
	if(WiFi.status() == WL_CONNECTED) {
		if(eapi.loop()) {
			sendPricesToMqtt();
		}
	}
	ws.loop();
	delay(1); // Needed for auto modem sleep
}

void setupHanPort(int pin, int newMeterType) {
	debugI("Setting up HAN on pin %d for meter type %d", pin, newMeterType);

	HardwareSerial *hwSerial = NULL;
	if(pin == 3) {
		hwSerial = &Serial;
	}
	#if defined(ESP32)
		if(pin == 9) {
			hwSerial = &Serial1;
		}
		if(pin == 16) {
			hwSerial = &Serial2;
		}
	#endif

	if(pin == 0) {
		debugE("Invalid GPIO configured for HAN");
		return;
	}

	if(hwSerial != NULL) {
		debugD("Hardware serial");
		Serial.flush();
		switch(newMeterType) {
			case METER_TYPE_KAMSTRUP:
			case METER_TYPE_OMNIPOWER:
				hwSerial->begin(2400, SERIAL_8N1);
				break;
			default:
				hwSerial->begin(2400, SERIAL_8E1);
				break;
		}
		hanSerial = hwSerial;
	} else {
		debugD("Software serial");
		Serial.flush();
		SoftwareSerial *swSerial = new SoftwareSerial(pin);

		switch(newMeterType) {
			case METER_TYPE_KAMSTRUP:
			case METER_TYPE_OMNIPOWER:
				swSerial->begin(2400, SWSERIAL_8N1);
				break;
			default:
				swSerial->begin(2400, SWSERIAL_8E1);
				break;
		}
		hanSerial = swSerial;

		Serial.begin(115200);
	}

	hanReader.setup(hanSerial, &Debug);
	hanReader.setEncryptionKey(meterConfig.encryptionKey);
	hanReader.setAuthenticationKey(meterConfig.authenticationKey);

	// Compensate for the known Kaifa bug
	hanReader.compensateFor09HeaderBug = (newMeterType == 1);

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
				if(lastErrorBlink - lastSuccessfulRead > 30000) {
					hw.ledBlink(LED_RED, 1); // If no message received from AMS in 30 sec, blink once
					return;
				}
				break;
			case 1:
				if(mqttEnabled && mqtt.lastError() != 0) {
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

void mqttMessageReceived(String &topic, String &payload)
{

	if (Debug.isActive(RemoteDebug::DEBUG)) {
		debugD("Incoming MQTT message: [%s] %s", topic.c_str(), payload.c_str());
	}

	// Do whatever needed here...
	// Ideas could be to query for values or to initiate OTA firmware update
}

void sendPricesToMqtt() {
	if(!mqttEnabled || topic.isEmpty() || !mqtt.connected())
		return;
	if(strcmp(eapi.getToken(), "") != 0)
		return;

	time_t now = time(nullptr);

	float min1hr, min3hr, min6hr;
	uint8_t min1hrIdx = -1, min3hrIdx = -1, min6hrIdx = -1;
	float min = INT16_MAX, max = INT16_MIN;
	float values[24] = {0};
	for(uint8_t i = 0; i < 24; i++) {
		float val = eapi.getValueForHour(now, i);
		values[i] = val;

		if(val == ENTSOE_NO_VALUE) break;
		
		if(val < min) min = val;
		if(val > max) max = val;

		if(min1hrIdx == -1 || min1hr > val) {
			min1hr = val;
			min1hrIdx = i;
		}

		if(i >= 2) {
			i -= 2;
			float val1 = values[i++];
			float val2 = values[i++];
			float val3 = val;
			if(val1 == ENTSOE_NO_VALUE || val2 == ENTSOE_NO_VALUE || val3 == ENTSOE_NO_VALUE) continue;
			float val3hr = val1+val2+val3;
			if(min3hrIdx == -1 || min3hr > val3hr) {
				min3hr = val3hr;
				min3hrIdx = i-2;
			}
		}

		if(i >= 5) {
			i -= 5;
			float val1 = values[i++];
			float val2 = values[i++];
			float val3 = values[i++];
			float val4 = values[i++];
			float val5 = values[i++];
			float val6 = val;
			if(val1 == ENTSOE_NO_VALUE || val2 == ENTSOE_NO_VALUE || val3 == ENTSOE_NO_VALUE || val4 == ENTSOE_NO_VALUE || val5 == ENTSOE_NO_VALUE || val6 == ENTSOE_NO_VALUE) continue;
			float val6hr = val1+val2+val3+val4+val5+val6;
			if(min6hrIdx == -1 || min6hr > val6hr) {
				min6hr = val6hr;
				min6hrIdx = i-5;
			}
		}

	}

	char ts1hr[21];
	if(min1hrIdx != -1) {
		tmElements_t tm;
        breakTime(now + (SECS_PER_HOUR * min1hrIdx), tm);
		sprintf(ts1hr, "%04d-%02d-%02dT%02d:00:00Z", tm.Year+1970, tm.Month, tm.Day, tm.Hour);
	}
	char ts3hr[21];
	if(min3hrIdx != -1) {
		tmElements_t tm;
        breakTime(now + (SECS_PER_HOUR * min3hrIdx), tm);
		sprintf(ts3hr, "%04d-%02d-%02dT%02d:00:00Z", tm.Year+1970, tm.Month, tm.Day, tm.Hour);
	}
	char ts6hr[21];
	if(min6hrIdx != -1) {
		tmElements_t tm;
        breakTime(now + (SECS_PER_HOUR * min6hrIdx), tm);
		sprintf(ts6hr, "%04d-%02d-%02dT%02d:00:00Z", tm.Year+1970, tm.Month, tm.Day, tm.Hour);
	}

	switch(payloadFormat) {
		case 0: // JSON
		{
			StaticJsonDocument<384> json;
			json["id"] = WiFi.macAddress();
		    JsonObject jp = json.createNestedObject("prices");
			for(int i = 0; i < 24; i++) {
				float val = values[i];
				if(val == ENTSOE_NO_VALUE) break;
				jp[String(i)] = serialized(String(val, 4));
			}
			if(min != INT16_MAX) {
				jp["min"] = serialized(String(min, 4));
			}
			if(max != INT16_MIN) {
				jp["max"] = serialized(String(max, 4));
			}

			if(min1hrIdx != -1) {
				jp["cheapest1hr"] = String(ts1hr);
			}
			if(min3hrIdx != -1) {
				jp["cheapest3hr"] = String(ts3hr);
			}
			if(min6hrIdx != -1) {
				jp["cheapest6hr"] = String(ts6hr);
			}

			String msg;
			serializeJson(json, msg);
			mqtt.publish(topic, msg.c_str());
			break;
		}
		case 1: // RAW
		case 2:
			{
				for(int i = 0; i < 24; i++) {
					float val = values[i];
					if(val == ENTSOE_NO_VALUE) {
						mqtt.publish(topic + "/price/" + String(i), "");
						break;
					} else {
						mqtt.publish(topic + "/price/" + String(i), String(val, 4));
					}
					mqtt.loop();
					delay(10);
				}
				if(min != INT16_MAX) {
					mqtt.publish(topic + "/price/min", String(min, 4));
				}
				if(max != INT16_MIN) {
					mqtt.publish(topic + "/price/max", String(max, 4));
				}

				if(min1hrIdx != -1) {
					mqtt.publish(topic + "/price/cheapest/1hr", String(ts1hr));
				}
				if(min3hrIdx != -1) {
					mqtt.publish(topic + "/price/cheapest/3hr", String(ts3hr));
				}
				if(min6hrIdx != -1) {
					mqtt.publish(topic + "/price/cheapest/6hr", String(ts6hr));
				}
			}
			break;
	}
}

int currentMeterType = 0;
void readHanPort() {
	if (hanReader.read()) {
		lastSuccessfulRead = millis();

		if(meterConfig.type > 0) {
			if(!hw.ledBlink(LED_GREEN, 1))
				hw.ledBlink(LED_INTERNAL, 1);

			AmsData data(meterConfig.type, meterConfig.substituteMissing, hanReader);
			if(data.getListType() > 0) {
				if(mqttEnabled && !topic.isEmpty()) {
					if(payloadFormat == 0) {
						StaticJsonDocument<512> json;
						hanToJson(json, data, hw, hw.getTemperature(), clientId);
						if (Debug.isActive(RemoteDebug::INFO)) {
							debugI("Sending data to MQTT");
							if (Debug.isActive(RemoteDebug::DEBUG)) {
								serializeJsonPretty(json, Debug);
							}
						}

						String msg;
						serializeJson(json, msg);
						mqtt.publish(topic, msg.c_str());
					// 
					// Start DOMOTICZ
					//
					} else if(payloadFormat == 3) {
						debugI("Sending data to MQTT");
						if(config.isDomoChanged()) {
							if(domoConfig == NULL)
								domoConfig = new DomoticzConfig();
							if(config.getDomoticzConfig(*domoConfig)) {
								config.ackDomoChange();
							}
						}
			
						if (domoConfig->elidx > 0) {
							if(data.getActiveImportCounter() > 1.0) {
								energy = data.getActiveImportCounter();
							}
							if(energy > 0.0) {
								String PowerEnergy;
								int p;
								StaticJsonDocument<200> json_PE;
								p = data.getActiveImportPower();
								PowerEnergy = String((double) p/1.0) + ";" + String(energy*1000.0, 0);
								json_PE["command"] = "udevice";
								json_PE["idx"] = domoConfig->elidx;
								json_PE["nvalue"] = 0;
								json_PE["svalue"] = PowerEnergy;
								String msg_PE;
								serializeJson(json_PE, msg_PE);
								mqtt.publish("domoticz/in", msg_PE.c_str());
							}
						}

						if (domoConfig->vl1idx > 0){				
							if (data.getL1Voltage() > 0.1){ 
								StaticJsonDocument<200> json_u1;
								json_u1["command"] = "udevice";
								json_u1["idx"] = domoConfig->vl1idx;
								json_u1["nvalue"] = 0;
								json_u1["svalue"] =  String(data.getL1Voltage(), 2);
								String msg_u1;
								serializeJson(json_u1, msg_u1);
								mqtt.publish("domoticz/in", msg_u1.c_str());
							}
						}

						if (domoConfig->vl2idx > 0){				
							if (data.getL2Voltage() > 0.1){ 
								StaticJsonDocument<200> json_u2;
								json_u2["command"] = "udevice";
								json_u2["idx"] = domoConfig->vl2idx;
								json_u2["nvalue"] = 0;
								json_u2["svalue"] =  String(data.getL2Voltage(), 2);
								String msg_u2;
								serializeJson(json_u2, msg_u2);
								mqtt.publish("domoticz/in", msg_u2.c_str());
							}
						}

						if (domoConfig->vl3idx > 0){				
							if (data.getL3Voltage() > 0.1){ 
								StaticJsonDocument<200> json_u3;
								json_u3["command"] = "udevice";
								json_u3["idx"] = domoConfig->vl3idx;
								json_u3["nvalue"] = 0;
								json_u3["svalue"] =  String(data.getL3Voltage());
								String msg_u3;
								serializeJson(json_u3, msg_u3);
								mqtt.publish("domoticz/in", msg_u3.c_str());
							}
						}
				
						if (domoConfig->cl1idx > 0){				
							if(data.getL1Current() > 0.0) {
								StaticJsonDocument<200> json_i1;
								String Ampere3;
								Ampere3 = String(data.getL1Current(), 1) + ";" + String(data.getL2Current(), 1) + ";" + String(data.getL3Current(), 1) ;
								json_i1["command"] = "udevice";
								json_i1["idx"] = domoConfig->cl1idx;
								json_i1["nvalue"] = 0;
								json_i1["svalue"] =  Ampere3;
								String msg_i1;
								serializeJson(json_i1, msg_i1);
								mqtt.publish("domoticz/in", msg_i1.c_str());
							}
						}			
						//
						// End DOMOTICZ
						//
					} else if(payloadFormat == 1 || payloadFormat == 2) {
						if(data.getPackageTimestamp() > 0) {
							mqtt.publish(topic + "/meter/dlms/timestamp", String(data.getPackageTimestamp()));
						}
						switch(data.getListType()) {
							case 3:
								// ID and type belongs to List 2, but I see no need to send that every 10s
								mqtt.publish(topic + "/meter/id", data.getMeterId());
								mqtt.publish(topic + "/meter/type", data.getMeterType());
								mqtt.publish(topic + "/meter/clock", String(data.getMeterTimestamp()));
								mqtt.publish(topic + "/meter/import/reactive/accumulated", String(data.getReactiveImportCounter(), 2));
								mqtt.publish(topic + "/meter/import/active/accumulated", String(data.getActiveImportCounter(), 2));
								mqtt.publish(topic + "/meter/export/reactive/accumulated", String(data.getReactiveExportCounter(), 2));
								mqtt.publish(topic + "/meter/export/active/accumulated", String(data.getActiveExportCounter(), 2));
								sendPricesToMqtt();
							case 2:
								// Only send data if changed. ID and Type is sent on the 10s interval only if changed
								if(meterState.getMeterId() != data.getMeterId() || payloadFormat == 2) {
									mqtt.publish(topic + "/meter/id", data.getMeterId());
								}
								if(meterState.getMeterType() != data.getMeterType() || payloadFormat == 2) {
									mqtt.publish(topic + "/meter/type", data.getMeterType());
								}
								if(meterState.getL1Current() != data.getL1Current() || payloadFormat == 2) {
									mqtt.publish(topic + "/meter/l1/current", String(data.getL1Current(), 2));
								}
								if(meterState.getL1Voltage() != data.getL1Voltage() || payloadFormat == 2) {
									mqtt.publish(topic + "/meter/l1/voltage", String(data.getL1Voltage(), 2));
								}
								if(meterState.getL2Current() != data.getL2Current() || payloadFormat == 2) {
									mqtt.publish(topic + "/meter/l2/current", String(data.getL2Current(), 2));
								}
								if(meterState.getL2Voltage() != data.getL2Voltage() || payloadFormat == 2) {
									mqtt.publish(topic + "/meter/l2/voltage", String(data.getL2Voltage(), 2));
								}
								if(meterState.getL3Current() != data.getL3Current() || payloadFormat == 2) {
									mqtt.publish(topic + "/meter/l3/current", String(data.getL3Current(), 2));
								}
								if(meterState.getL3Voltage() != data.getL3Voltage() || payloadFormat == 2) {
									mqtt.publish(topic + "/meter/l3/voltage", String(data.getL3Voltage(), 2));
								}
								if(meterState.getReactiveExportPower() != data.getReactiveExportPower() || payloadFormat == 2) {
									mqtt.publish(topic + "/meter/export/reactive", String(data.getReactiveExportPower()));
								}
								if(meterState.getActiveExportPower() != data.getActiveExportPower() || payloadFormat == 2) {
									mqtt.publish(topic + "/meter/export/active", String(data.getActiveExportPower()));
								}
								if(meterState.getReactiveImportPower() != data.getReactiveImportPower() || payloadFormat == 2) {
									mqtt.publish(topic + "/meter/import/reactive", String(data.getReactiveImportPower()));
								}
							case 1:
								if(meterState.getActiveImportPower() != data.getActiveImportPower() || payloadFormat == 2) {
									mqtt.publish(topic + "/meter/import/active", String(data.getActiveImportPower()));
								}
						}
					}
					mqtt.loop();
					delay(10);
				}
				meterState.apply(data);
			}
		} else {
			// Auto detect meter if not set
			for(int i = 1; i <= 3; i++) {
				String list;
				switch(i) {
					case 1:
						list = hanReader.getString((int) Kaifa_List1Phase::ListVersionIdentifier);
						break;
					case 2:
						list = hanReader.getString((int) Aidon_List1Phase::ListVersionIdentifier);
						break;
					case 3:
						list = hanReader.getString((int) Kamstrup_List1Phase::ListVersionIdentifier);
						break;
				}
				if(!list.isEmpty()) {
					list.toLowerCase();
					if(list.startsWith("kfm")) {
						meterConfig.type = 1;
						config.setMeterConfig(meterConfig);
						if(Debug.isActive(RemoteDebug::INFO)) debugI("Detected Kaifa meter");
						break;
					} else if(list.startsWith("aidon")) {
						meterConfig.type = 2;
						config.setMeterConfig(meterConfig);
						if(Debug.isActive(RemoteDebug::INFO)) debugI("Detected Aidon meter");
						break;
					} else if(list.startsWith("kamstrup")) {
						meterConfig.type = 3;
						config.setMeterConfig(meterConfig);
						if(Debug.isActive(RemoteDebug::INFO)) debugI("Detected Kamstrup meter");
						break;
					}
				}
			}
			hanReader.compensateFor09HeaderBug = (meterConfig.type == 1);
		}
	}

	// Switch parity if meter is still not detected
	if(meterConfig.type == 0 && millis() - lastSuccessfulRead > 10000) {
		lastSuccessfulRead = millis();
		debugD("No data for current setting, switching parity");
		Serial.flush();
		if(++currentMeterType == 4) currentMeterType = 1;
		setupHanPort(gpioConfig.hanPin, currentMeterType);
	}
	delay(1);
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

		if (Debug.isActive(RemoteDebug::INFO)) debugI("Connecting to WiFi network: %s", wifi.ssid);

		MDNS.end();
		WiFi.disconnect();
		yield();

		WiFi.enableAP(false);
		WiFi.mode(WIFI_STA);
		if(strlen(wifi.ip) > 0) {
			IPAddress ip, gw, sn(255,255,255,0), dns1, dns2;
			ip.fromString(wifi.ip);
			gw.fromString(wifi.gateway);
			sn.fromString(wifi.subnet);
			dns1.fromString(wifi.dns1);
			dns2.fromString(wifi.dns2);
			WiFi.config(ip, gw, sn, dns1, dns2);
		} else {
			#if defined(ESP32)
			WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE); // Workaround to make DHCP hostname work for ESP32. See: https://github.com/espressif/arduino-esp32/issues/2537
			#endif
		}
		if(strlen(wifi.hostname) > 0) {
			#if defined(ESP8266)
			WiFi.hostname(wifi.hostname);
			#elif defined(ESP32)
			WiFi.setHostname(wifi.hostname);
			#endif
		}
		WiFi.begin(wifi.ssid, wifi.psk);
		yield();
	}
}

unsigned long lastMqttRetry = -10000;
void MQTT_connect() {
	MqttConfig mqttConfig;
	if(!config.getMqttConfig(mqttConfig) || strlen(mqttConfig.host) == 0) {
		if(Debug.isActive(RemoteDebug::WARNING)) debugW("No MQTT config");
		mqttEnabled = false;
		config.ackMqttChange();
		return;
	}
	if(millis() - lastMqttRetry < (mqtt.lastError() == 0 || config.isMqttChanged() ? 5000 : 30000)) {
		yield();
		return;
	}
	lastMqttRetry = millis();

	clientId = String(mqttConfig.clientId);
	mqttEnabled = true;
	payloadFormat = mqttConfig.payloadFormat;
	topic = String(mqttConfig.publishTopic);

	if(Debug.isActive(RemoteDebug::INFO)) {
		debugD("Disconnecting MQTT before connecting");
	}

	mqtt.disconnect();
	yield();

	WiFiClientSecure *secureClient = NULL;
	Client *client = NULL;
	if(mqttConfig.ssl) {
		debugI("MQTT SSL is configured");

		secureClient = new WiFiClientSecure();
		#if defined(ESP8266)
		secureClient->setBufferSizes(512, 512);
		#endif

		if(SPIFFS.begin()) {
			char *ca = NULL;
			char *cert = NULL;
			char *key = NULL;

			if(SPIFFS.exists(FILE_MQTT_CA)) {
				debugI("Found MQTT CA file");
				File file = SPIFFS.open(FILE_MQTT_CA, "r");
				secureClient->loadCACert(file, file.size());
			}
			if(SPIFFS.exists(FILE_MQTT_CERT)) {
				debugI("Found MQTT certificate file");
				File file = SPIFFS.open(FILE_MQTT_CERT, "r");
				secureClient->loadCertificate(file, file.size());
			}
			if(SPIFFS.exists(FILE_MQTT_KEY)) {
				debugI("Found MQTT key file");
				File file = SPIFFS.open(FILE_MQTT_KEY, "r");
				secureClient->loadPrivateKey(file, file.size());
			}
			SPIFFS.end();
		}
		client = secureClient;
	} else {
		client = new WiFiClient();
	}

	if(Debug.isActive(RemoteDebug::INFO)) {
		debugI("Connecting to MQTT %s:%d", mqttConfig.host, mqttConfig.port);
	}
	mqtt.begin(mqttConfig.host, mqttConfig.port, *client);

	#if defined(ESP8266)
	if(secureClient) {
		time_t epoch = time(nullptr);
		debugD("Setting NTP time %i for secure MQTT connection", epoch);
 		secureClient->setX509Time(epoch);
	}
	#endif

	// Connect to a unsecure or secure MQTT server
	if ((strlen(mqttConfig.username) == 0 && mqtt.connect(mqttConfig.clientId)) ||
		(strlen(mqttConfig.username) > 0 && mqtt.connect(mqttConfig.clientId, mqttConfig.username, mqttConfig.password))) {
		if (Debug.isActive(RemoteDebug::INFO)) debugI("Successfully connected to MQTT!");
		config.ackMqttChange();

		// Subscribe to the chosen MQTT topic, if set in configuration
		if (strlen(mqttConfig.subscribeTopic) > 0) {
			mqtt.subscribe(mqttConfig.subscribeTopic);
			if (Debug.isActive(RemoteDebug::INFO)) debugI("  Subscribing to [%s]\r\n", mqttConfig.subscribeTopic);
		}
		
		if(payloadFormat == 0) {
			sendMqttData("Connected!");
		} else if(payloadFormat == 1 || payloadFormat == 2) {
			sendSystemStatusToMqtt();
		}
	} else {
		if (Debug.isActive(RemoteDebug::ERROR)) {
			debugE("Failed to connect to MQTT");
#if defined(ESP8266)
			if(secureClient) {
				char buf[64];
  				secureClient->getLastSSLError(buf,64);
				Debug.println(buf);
			}
#endif
		}
	}
	yield();
}

// Send a simple string embedded in json over MQTT
void sendMqttData(String data)
{
	// Make sure we have configured a publish topic
	if (topic.isEmpty())
		return;

	// Build a json with the message in a "data" attribute
	StaticJsonDocument<128> json;
	json["id"] = WiFi.macAddress();
	json["up"] = millis64()/1000;
	json["data"] = data;
	double vcc = hw.getVcc();
	if(vcc > 0) {
		json["vcc"] = vcc;
	}
	json["rssi"] = hw.getWifiRssi();

	// Stringify the json
	String msg;
	serializeJson(json, msg);

	// Send the json over MQTT
	mqtt.publish(topic, msg.c_str());

	if (Debug.isActive(RemoteDebug::INFO)) debugI("Sending MQTT data");
	if (Debug.isActive(RemoteDebug::DEBUG)) debugD("[%s]", data.c_str());
}

unsigned long lastSystemDataSent = -10000;
void sendSystemStatusToMqtt() {
	if (topic.isEmpty())
		return;
	if(millis() - lastSystemDataSent < 10000)
		return;
	lastSystemDataSent = millis();

	mqtt.publish(topic + "/id", WiFi.macAddress());
	mqtt.publish(topic + "/uptime", String((unsigned long) millis64()/1000));
	double vcc = hw.getVcc();
	if(vcc > 0) {
		mqtt.publish(topic + "/vcc", String(vcc, 2));
	}
	mqtt.publish(topic + "/rssi", String(hw.getWifiRssi()));
    if(hw.getTemperature() > -85) {
		mqtt.publish(topic + "/temperature", String(hw.getTemperature(), 2));
    }
}
