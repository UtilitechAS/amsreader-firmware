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
#include <ArduinoJson.h>
#include <MQTT.h>
#include <DNSServer.h>

#if defined(ESP8266)
ADC_MODE(ADC_VCC);  
#endif   

#include "HwTools.h"

#include "web/AmsWebServer.h"
#include "AmsConfiguration.h"
#include "HanReader.h"
#include "HanToJson.h"

#include "Aidon.h"
#include "Kaifa.h"
#include "Kamstrup.h"

#include "Uptime.h"

HwTools hw;

DNSServer dnsServer;

AmsConfiguration config;

AmsWebServer ws;

WiFiClient *client;
MQTTClient mqtt(384);

Stream* debugger = NULL;

HanReader hanReader;

void setup() {
	if(config.hasConfig()) {
		config.load();
	}

#if DEBUG_MODE
#if HW_ROARFRED
#if SOFTWARE_SERIAL
	SoftwareSerial *ser = new SoftwareSerial(-1, 1);
	ser->begin(115200, SWSERIAL_8N1);
	debugger = ser;
#else
	HardwareSerial *ser = &Serial;
	if(config.getMeterType() == 3) {
		ser->begin(2400, SERIAL_8N1);
	} else {
		ser->begin(2400, SERIAL_8E1);
	}
#endif
#else
	HardwareSerial *ser = &Serial;
	ser->begin(115200, SERIAL_8N1);
#endif
	debugger = ser;
#endif

	double vcc = hw.getVcc();

	if (debugger) {
		debugger->println("");
		debugger->println("Started...");
		debugger->print("Voltage: ");
		debugger->print(vcc);
		debugger->println("V");
	}

	if (vcc > 0 && vcc < 3.1) {
		if(debugger) {
			debugger->println("Voltage is too low, sleeping");
			debugger->flush();
		}
		ESP.deepSleep(10000000);    //Deep sleep to allow output cap to charge up
	}  

	#if HAS_RGB_LED
		// Initialize RGB LED pins
		pinMode(LEDPIN_RGB_GREEN, OUTPUT);	
		pinMode(LEDPIN_RGB_RED, OUTPUT);
	#endif

	pinMode(LED_PIN, OUTPUT);
	pinMode(AP_BUTTON_PIN, INPUT_PULLUP);

	led_off();

	WiFi.disconnect(true);
	WiFi.softAPdisconnect(true);
	WiFi.mode(WIFI_OFF);

	if(SPIFFS.begin()) {
		bool flashed = false;
		if(SPIFFS.exists("/firmware.bin")) {
			if(debugger) debugger->println("Found firmware");
#if defined(ESP8266)
			WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
			WiFi.forceSleepBegin();
#endif
			int i = 0;
			while(hw.getVcc() < 3.3 && i < 3) {
				if(debugger) debugger->println(" vcc not optimal, light sleep 10s");
#if defined(ESP8266)
				delay(10000);
#elif defined(ESP32)
			    esp_sleep_enable_timer_wakeup(10000000);
			    esp_light_sleep_start();
#endif
				i++;
			}

			if(debugger) debugger->println(" flashing");
			File firmwareFile = SPIFFS.open("/firmware.bin", "r");
			uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
			if (!Update.begin(maxSketchSpace, U_FLASH)) {
				if(debugger) {
					debugger->println("Unable to start firmware update");
					Update.printError(*debugger);
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
			SPIFFS.remove("/firmware.bin");
		}
		SPIFFS.end();
		if(flashed) {
			if(debugger) {
				debugger->println("Firmware update complete, restarting");
				debugger->flush();
			}
#if defined(ESP8266)
			ESP.reset();
#elif defined(ESP32)
			ESP.restart();
#endif
			return;
		}
	}

	if(config.hasConfig()) {
		if(debugger) config.print(debugger);
		WiFi_connect();
		client = new WiFiClient();
	} else {
		if(debugger) {
			debugger->println("No configuration, booting AP");
		}
		swapWifiMode();
	}

#if SOFTWARE_SERIAL
	if(debugger) debugger->println("HAN has software serial");
	if(config.getMeterType() == 3) {
		hanSerial->begin(2400, SWSERIAL_8N1);
	} else {
		hanSerial->begin(2400, SWSERIAL_8E1);
	}
#else
	if(debugger) { 
		debugger->println("HAN has hardware serial");
		debugger->flush();
	}
	if(config.getMeterType() == 3) {
		hanSerial->begin(2400, SERIAL_8N1);
	} else {
		hanSerial->begin(2400, SERIAL_8E1);
	}
#if UART2
	hanSerial->swap();
#endif
#endif

	hanReader.setup(hanSerial, debugger);

	// Compensate for the known Kaifa bug
	hanReader.compensateFor09HeaderBug = (config.getMeterType() == 1);

	// Empty buffer before starting
	while (hanSerial->available() > 0) {
    	hanSerial->read();
	}

	ws.setup(&config, debugger, &mqtt);

#if HAS_RGB_LED
	//Signal startup by blinking red / green / yellow
	rgb_led(RGB_RED, 2);
	delay(250);
	rgb_led(RGB_GREEN, 2);
	delay(250);
	rgb_led(RGB_YELLOW, 2);
#endif
}

int buttonTimer = 0;
bool buttonActive = false;
unsigned long longPressTime = 5000;
bool longPressActive = false;

bool wifiConnected = false;

unsigned long lastTemperatureRead = 0;
double temperature = -127;

bool even = true;
unsigned long lastRead = 0;
unsigned long lastSuccessfulRead = 0;

unsigned long lastErrorBlink = 0; 
int lastError = 0;

void loop() {
	unsigned long now = millis();
	if(AP_BUTTON_PIN != INVALID_BUTTON_PIN) {
		if (digitalRead(AP_BUTTON_PIN) == LOW) {
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
	
	if(now - lastTemperatureRead > 5000) {
		temperature = hw.getTemperature();
		lastTemperatureRead = now;
	}

	if(now > 10000 && now - lastErrorBlink > 3000) {
		errorBlink();
	}

	// Only do normal stuff if we're not booted as AP
	if (WiFi.getMode() != WIFI_AP) {
		led_off();

		if (WiFi.status() != WL_CONNECTED) {
			wifiConnected = false;
			WiFi_connect();
		} else {
			if(!wifiConnected) {
				wifiConnected = true;
				if(debugger) {
					debugger->println("Successfully connected to WiFi!");
					debugger->println(WiFi.localIP());
				}
			}
			if (!config.getMqttHost().isEmpty()) {
				mqtt.loop();
				delay(10); // Needed to preserve power. After adding this, the voltage is super smooth on a HAN powered device
				if(!mqtt.connected() || config.isMqttChanged()) {
					MQTT_connect();
				}
				if(config.getMqttPayloadFormat() == 1) {
					sendSystemStatusToMqtt();
				}
			} else if(mqtt.connected()) {
				mqtt.disconnect();
			}
		}
	} else {
		dnsServer.processNextRequest();
		// Continously flash the LED when AP mode
		if (now / 50 % 64 == 0)   led_on();
		else					  led_off();

	}
	if(now - lastRead > 100) {
		yield();
		readHanPort();
		lastRead = now;
	}
	ws.loop();
	delay(1); // Needed for auto modem sleep
}


void led_on()
{
#if LED_ACTIVE_HIGH
	digitalWrite(LED_PIN, HIGH);
#else
	digitalWrite(LED_PIN, LOW);
#endif
}


void led_off()
{
#if LED_ACTIVE_HIGH
	digitalWrite(LED_PIN, LOW);
#else
	digitalWrite(LED_PIN, HIGH);
#endif
}

void errorBlink() {
	if(lastError == 3)
		lastError = 0;
	lastErrorBlink = millis();
	for(;lastError < 3;lastError++) {
		switch(lastError) {
			case 0:
				if(lastErrorBlink - lastSuccessfulRead > 30000) {
					rgb_led(1, 2); // If no message received from AMS in 30 sec, blink once
					return;
				}
				break;
			case 1:
				if(!config.getMqttHost().isEmpty() && mqtt.lastError() != 0) {
					rgb_led(1, 3); // If MQTT error, blink twice
					return;
				}
				break;
			case 2:
				if(WiFi.getMode() != WIFI_AP && WiFi.status() != WL_CONNECTED) {
					rgb_led(1, 4); // If WiFi not connected, blink three times
					return;
				}
				break;
		}
	}
}

void swapWifiMode() {
	led_on();
	WiFiMode_t mode = WiFi.getMode();
	dnsServer.stop();
	WiFi.disconnect(true);
	WiFi.softAPdisconnect(true);
	WiFi.mode(WIFI_OFF);
	yield();

	if (mode != WIFI_AP || !config.hasConfig()) {
		if(debugger) debugger->println("Swapping to AP mode");
		WiFi.softAP("AMS2MQTT");
		WiFi.mode(WIFI_AP);

		dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
		dnsServer.start(53, "*", WiFi.softAPIP());
	} else {
		if(debugger) debugger->println("Swapping to STA mode");
		WiFi_connect();
	}
	delay(500);
	led_off();
}

void mqttMessageReceived(String &topic, String &payload)
{

	if (debugger) {
		debugger->println("Incoming MQTT message:");
		debugger->print("[");
		debugger->print(topic);
		debugger->print("] ");
		debugger->println(payload);
	}

	// Do whatever needed here...
	// Ideas could be to query for values or to initiate OTA firmware update
}

AmsData lastMqttData;
void readHanPort() {
	if (hanReader.read()) {
		// Empty serial buffer. For some reason this seems to make a difference. Some garbage on the wire after package?
		while(hanSerial->available()) {
			hanSerial->read();
		}

		lastSuccessfulRead = millis();

		if(config.getMeterType() > 0) {
			#if HAS_RGB_LED
				rgb_led(RGB_GREEN, 1);
			#else
				led_on();
			#endif

			AmsData data(config.getMeterType(), hanReader);
			ws.setData(data);

			if(!config.getMqttHost().isEmpty() && !config.getMqttPublishTopic().isEmpty()) {
				if(config.getMqttPayloadFormat() == 0) {
					StaticJsonDocument<512> json;
					hanToJson(json, data, hw, temperature);
					if (debugger) {
						debugger->print("Sending data to MQTT: ");
						serializeJsonPretty(json, *debugger);
						debugger->println();
					}

					String msg;
					serializeJson(json, msg);
					mqtt.publish(config.getMqttPublishTopic(), msg.c_str());
				} else if(config.getMqttPayloadFormat() == 1) {
					mqtt.publish(config.getMqttPublishTopic() + "/meter/dlms/timestamp", String(data.getPackageTimestamp()));
					switch(data.getListType()) {
						case 3:
							// ID and type belongs to List 2, but I see no need to send that every 10s
							mqtt.publish(config.getMqttPublishTopic() + "/meter/id", data.getMeterId());
							mqtt.publish(config.getMqttPublishTopic() + "/meter/type", data.getMeterType());
							mqtt.publish(config.getMqttPublishTopic() + "/meter/clock", String(data.getMeterTimestamp()));
							mqtt.publish(config.getMqttPublishTopic() + "/meter/import/reactive/accumulated", String(data.getReactiveImportCounter(), 2));
							mqtt.publish(config.getMqttPublishTopic() + "/meter/import/active/accumulated", String(data.getActiveImportCounter(), 2));
							mqtt.publish(config.getMqttPublishTopic() + "/meter/export/reactive/accumulated", String(data.getReactiveExportCounter(), 2));
							mqtt.publish(config.getMqttPublishTopic() + "/meter/export/active/accumulated", String(data.getActiveExportCounter(), 2));
						case 2:
							// Only send data if changed. ID and Type is sent on the 10s interval only if changed
							if(lastMqttData.getMeterId() != data.getMeterId()) {
								mqtt.publish(config.getMqttPublishTopic() + "/meter/id", data.getMeterId());
							}
							if(lastMqttData.getMeterType() != data.getMeterType()) {
								mqtt.publish(config.getMqttPublishTopic() + "/meter/type", data.getMeterType());
							}
							if(lastMqttData.getL1Current() != data.getL1Current()) {
								mqtt.publish(config.getMqttPublishTopic() + "/meter/l1/current", String(data.getL1Current(), 2));
							}
							if(lastMqttData.getL1Voltage() != data.getL1Voltage()) {
								mqtt.publish(config.getMqttPublishTopic() + "/meter/l1/voltage", String(data.getL1Voltage(), 2));
							}
							if(lastMqttData.getL2Current() != data.getL2Current()) {
								mqtt.publish(config.getMqttPublishTopic() + "/meter/l2/current", String(data.getL2Current(), 2));
							}
							if(lastMqttData.getL2Voltage() != data.getL2Voltage()) {
								mqtt.publish(config.getMqttPublishTopic() + "/meter/l2/voltage", String(data.getL2Voltage(), 2));
							}
							if(lastMqttData.getL3Current() != data.getL3Current()) {
								mqtt.publish(config.getMqttPublishTopic() + "/meter/l3/current", String(data.getL3Current(), 2));
							}
							if(lastMqttData.getL3Voltage() != data.getL3Voltage()) {
								mqtt.publish(config.getMqttPublishTopic() + "/meter/l3/voltage", String(data.getL3Voltage(), 2));
							}
							if(lastMqttData.getReactiveExportPower() != data.getReactiveExportPower()) {
								mqtt.publish(config.getMqttPublishTopic() + "/meter/export/reactive", String(data.getReactiveExportPower()));
							}
							if(lastMqttData.getActiveExportPower() != data.getActiveExportPower()) {
								mqtt.publish(config.getMqttPublishTopic() + "/meter/export/active", String(data.getActiveExportPower()));
							}
							if(lastMqttData.getReactiveImportPower() != data.getReactiveImportPower()) {
								mqtt.publish(config.getMqttPublishTopic() + "/meter/import/reactive", String(data.getReactiveImportPower()));
							}
						case 1:
							if(lastMqttData.getActiveImportPower() != data.getActiveImportPower()) {
								mqtt.publish(config.getMqttPublishTopic() + "/meter/import/active", String(data.getActiveImportPower()));
							}
					}
				}
				lastMqttData.apply(data);
				mqtt.loop();
				delay(10);
			}

			#if HAS_RGB_LED
				rgb_led(RGB_GREEN, 0);
			#else
				led_off();
			#endif
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
						config.setMeterType(1);
						if(debugger) debugger->println("Detected Kaifa meter");
						break;
					} else if(list.startsWith("aidon")) {
						config.setMeterType(2);
						if(debugger) debugger->println("Detected Aidon meter");
						break;
					} else if(list.startsWith("kamstrup")) {
						config.setMeterType(3);
						if(debugger) debugger->println("Detected Kamstrup meter");
						break;
					}
				}
			}
			hanReader.compensateFor09HeaderBug = (config.getMeterType() == 1);
		}
	}

	// Switch parity if meter is still not detected
	if(config.getMeterType() == 0 && millis() - lastSuccessfulRead > 10000) {
		lastSuccessfulRead = millis();
		if(debugger) debugger->println("No data for current setting, switching parity");
#if SOFTWARE_SERIAL
			if(even) {
				hanSerial->begin(2400, SWSERIAL_8N1);
			} else {
				hanSerial->begin(2400, SWSERIAL_8E1);
			}
#else
			if(even) {
				hanSerial->begin(2400, SERIAL_8N1);
			} else {
				hanSerial->begin(2400, SERIAL_8E1);
			}
#endif
		even = !even;
	}
}

unsigned long wifiTimeout = WIFI_CONNECTION_TIMEOUT;
unsigned long lastWifiRetry = -WIFI_CONNECTION_TIMEOUT;
void WiFi_connect() {
	if(millis() - lastWifiRetry < wifiTimeout) {
		delay(50);
		return;
	}
	lastWifiRetry = millis();

	if (debugger) {
		debugger->println();
		debugger->println();
		debugger->print("Connecting to WiFi network ");
		debugger->println(config.getWifiSsid());
	}

	if (WiFi.status() != WL_CONNECTED) {
		WiFi.disconnect();
		yield();

		WiFi.enableAP(false);
		WiFi.mode(WIFI_STA);
		if(!config.getWifiIp().isEmpty()) {
			IPAddress ip, gw, sn(255,255,255,0);
			ip.fromString(config.getWifiIp());
			gw.fromString(config.getWifiGw());
			sn.fromString(config.getWifiSubnet());
			WiFi.config(ip, gw, sn);
		}
		WiFi.begin(config.getWifiSsid().c_str(), config.getWifiPassword().c_str());
		yield();
	}
}

unsigned long lastMqttRetry = -10000;
void MQTT_connect() {
	if(config.getMqttHost().isEmpty()) {
		if(debugger) debugger->println("No MQTT config");
		return;
	}
	if(millis() - lastMqttRetry < 5000) {
		yield();
		return;
	}
	lastMqttRetry = millis();
	if(debugger) {
		debugger->print("Connecting to MQTT: ");
		debugger->print(config.getMqttHost());
		debugger->print(", port: ");
		debugger->print(config.getMqttPort());
		debugger->println();
	}

	mqtt.disconnect();
	yield();

	mqtt.begin(config.getMqttHost().c_str(), config.getMqttPort(), *client);

	// Connect to a unsecure or secure MQTT server
	if ((config.getMqttUser().isEmpty() && mqtt.connect(config.getMqttClientId().c_str())) ||
		(!config.getMqttUser().isEmpty() && mqtt.connect(config.getMqttClientId().c_str(), config.getMqttUser().c_str(), config.getMqttPassword().c_str()))) {
		if (debugger) debugger->println("\nSuccessfully connected to MQTT!");
		config.ackMqttChange();

		// Subscribe to the chosen MQTT topic, if set in configuration
		if (!config.getMqttSubscribeTopic().isEmpty()) {
			mqtt.subscribe(config.getMqttSubscribeTopic());
			if (debugger) debugger->printf("  Subscribing to [%s]\r\n", config.getMqttSubscribeTopic().c_str());
		}
		
		if(config.getMqttPayloadFormat() == 0) {
			sendMqttData("Connected!");
		} else if(config.getMqttPayloadFormat() == 1) {
			sendSystemStatusToMqtt();
		}
	} else {
		if (debugger) {
			debugger->print(" failed, ");
			debugger->println(" trying again in 5 seconds");
		}
	}
	yield();
}

// Send a simple string embedded in json over MQTT
void sendMqttData(String data)
{
	// Make sure we have configured a publish topic
	if (config.getMqttPublishTopic().isEmpty())
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
	mqtt.publish(config.getMqttPublishTopic(), msg.c_str());

	if (debugger) debugger->print("sendMqttData: ");
	if (debugger) debugger->println(data);
}

unsigned long lastSystemDataSent = -10000;
void sendSystemStatusToMqtt() {
	if (config.getMqttPublishTopic().isEmpty())
		return;
	if(millis() - lastSystemDataSent < 10000)
		return;
	lastSystemDataSent = millis();

	mqtt.publish(config.getMqttPublishTopic() + "/id", WiFi.macAddress());
	mqtt.publish(config.getMqttPublishTopic() + "/uptime", String((unsigned long) millis64()/1000));
	double vcc = hw.getVcc();
	if(vcc > 0) {
		mqtt.publish(config.getMqttPublishTopic() + "/vcc", String(vcc, 2));
	}
	mqtt.publish(config.getMqttPublishTopic() + "/rssi", String(hw.getWifiRssi()));
    if(temperature != DEVICE_DISCONNECTED_C) {
		mqtt.publish(config.getMqttPublishTopic() + "/vcc", String(temperature, 2));
    }
}

void rgb_led(int color, int mode) {
// Activate red and green LEDs if RGB LED is present (HAS_RGB_LED=1)
// If no RGB LED present (HAS_RGB_LED=0 or not defined), all output goes to ESP onboard LED
// color: 1=red, 2=green, 3=yellow
// mode: 0=OFF, 1=ON, >=2 -> Short blink(s), number of blinks: (mode - 1)
#ifndef  HAS_RGB_LED
#define LEDPIN_RGB_RED LED_PIN
#define LEDPIN_RGB_GREEN LED_PIN
#endif
	int blinkduration = 50;	// milliseconds
	switch (mode) {
		case 0:	//OFF
			digitalWrite(LEDPIN_RGB_RED, HIGH);
			digitalWrite(LEDPIN_RGB_GREEN, HIGH);
			break;
		case 1: //ON
			switch (color) {
				case 1:	//Red
					digitalWrite(LEDPIN_RGB_RED, LOW);
					digitalWrite(LEDPIN_RGB_GREEN, HIGH);
					break;
				case 2:	//Green
					digitalWrite(LEDPIN_RGB_RED, HIGH);
					digitalWrite(LEDPIN_RGB_GREEN, LOW);
					break;
				case 3:	//Yellow
					digitalWrite(LEDPIN_RGB_RED, LOW);
					digitalWrite(LEDPIN_RGB_GREEN, LOW);
					break;
				}
			break;
		default: // Blink
			for(int i = 1; i < mode; i++) {
				rgb_led(color, 1);
				delay(blinkduration);
				rgb_led(color, 0);
				if(i != mode)
					delay(blinkduration);
			}
			break;
	}
}
