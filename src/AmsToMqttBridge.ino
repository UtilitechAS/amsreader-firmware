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

#define DEBUG_ESP_HTTP_CLIENT 1
#define DEBUG_ESP_PORT Serial

HwTools hw;

DNSServer dnsServer;

AmsConfiguration config;

RemoteDebug Debug;

AmsWebServer ws(&Debug, &hw);

MQTTClient mqtt(512);

HanReader hanReader;

Stream *hanSerial;

void setup() {
	if(config.hasConfig()) {
		config.load();
	}

	if(!config.hasConfig() || config.getConfigVersion() < 81) {
		debugI("Setting default hostname");
		uint16_t chipId;
		#if defined(ESP32)
			chipId = ESP.getEfuseMac();
		#else
			chipId = ESP.getChipId();
		#endif
		config.setWifiHostname((String("ams-") + String(chipId, HEX)).c_str());
	}

	if(!config.hasConfig() || config.getConfigVersion() < 82) {
		config.setVccMultiplier(1.0);
		config.setVccBootLimit(0);
		#if HW_ROARFRED
			config.setHanPin(3);
			config.setApPin(0);
			config.setLedPin(2);
			config.setLedInverted(true);
			config.setTempSensorPin(5);
		#elif defined(ARDUINO_ESP8266_WEMOS_D1MINI)
			config.setHanPin(5);
			config.setApPin(4);
			config.setLedPin(2);
			config.setLedInverted(true);
			config.setTempSensorPin(14);
			config.setVccMultiplier(1.1);
		#elif defined(ARDUINO_LOLIN_D32)
			config.setHanPin(16);
			config.setLedPin(5);
			config.setLedInverted(true);
			config.setTempSensorPin(14);
			config.setVccPin(35);
			config.setVccMultiplier(2.25);
		#elif defined(ARDUINO_FEATHER_ESP32)
			config.setHanPin(16);
			config.setLedPin(2);
			config.setTempSensorPin(14);
		#elif defined(ARDUINO_ESP32_DEV)
			config.setHanPin(16);
			config.setLedPin(2);
			config.setLedInverted(false);
		#elif defined(ESP8266)
			config.setHanPin(3);
			config.setLedPin(2);
			config.setLedInverted(true);
		#elif defined(ESP32)
			config.setHanPin(16);
			config.setLedPin(2);
			config.setLedInverted(true);
			config.setTempSensorPin(14);
		#endif
	}
	delay(1);

	hw.setLed(config.getLedPin(), config.isLedInverted());
	hw.setLedRgb(config.getLedPinRed(), config.getLedPinGreen(), config.getLedPinBlue(), config.isLedRgbInverted());
	hw.setTempSensorPin(config.getTempSensorPin());
	hw.setTempAnalogSensorPin(config.getTempAnalogSensorPin());
	hw.setVccPin(config.getVccPin());
	hw.setVccMultiplier(config.getVccMultiplier());
	hw.setVccOffset(config.getVccOffset());
	hw.ledBlink(LED_INTERNAL, 1);
	hw.ledBlink(LED_RED, 1);
	hw.ledBlink(LED_YELLOW, 1);
	hw.ledBlink(LED_GREEN, 1);
	hw.ledBlink(LED_BLUE, 1);

	if(config.getHanPin() == 3) {
		switch(config.getMeterType()) {
			case METER_TYPE_KAMSTRUP:
			case METER_TYPE_OMNIPOWER:
				Serial.begin(2400, SERIAL_8N1);
				break;
			default:
				Serial.begin(2400, SERIAL_8E1);
				break;
		}
	} else {
		Serial.begin(115200);
	}

	if(config.hasConfig() && config.isDebugSerial()) {
		Debug.setSerialEnabled(config.isDebugSerial());
	} else {
		#if DEBUG_MODE
			Debug.setSerialEnabled(true);
		#endif
	}
	delay(1);

	uint8_t c = config.getTempSensorCount();
	for(int i = 0; i < c; i++) {
		TempSensorConfig* tsc = config.getTempSensorConfig(i);
		hw.confTempSensor(tsc->address, tsc->name, tsc->common);
	}

	double vcc = hw.getVcc();

	if (Debug.isActive(RemoteDebug::INFO)) {
		debugI("AMS bridge started");
		debugI("Voltage: %.2fV", vcc);
	}

	double vccBootLimit = config.getVccBootLimit();
	if(vccBootLimit > 0 && (config.getApPin() == 0xFF || digitalRead(config.getApPin()) == HIGH)) { // Skip if user is holding AP button while booting (HIGH = button is released)
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
		if(config.isNtpEnable()) {
			configTime(config.getNtpOffset(), config.getNtpSummerOffset(), config.getNtpServer());
			sntp_servermode_dhcp(config.isNtpDhcp() ? 1 : 0);
		}
	} else {
		if(Debug.isActive(RemoteDebug::INFO)) {
			debugI("No configuration, booting AP");
		}
		swapWifiMode();
	}

	ws.setup(&config, &mqtt);
}


int buttonTimer = 0;
bool buttonActive = false;
unsigned long longPressTime = 5000;
bool longPressActive = false;

bool wifiConnected = false;

unsigned long lastTemperatureRead = 0;
float temperatures[32];

unsigned long lastRead = 0;
unsigned long lastSuccessfulRead = 0;

unsigned long lastErrorBlink = 0; 
int lastError = 0;

// domoticz energy init
double energy = -1.0;

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
	if(config.getApPin() != 0xFF) {
		if (digitalRead(config.getApPin()) == LOW) {
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
		hw.updateTemperatures();
		lastTemperatureRead = now;

		uint8_t c = hw.getTempSensorCount();

		if(strlen(config.getMqttHost()) > 0) {
			bool anyChanged = false;
			for(int i = 0; i < c; i++) {
				bool changed = false;
				TempSensorData* data = hw.getTempSensorData(i);
				if(data->lastValidRead > -85) {
					changed = data->lastValidRead != temperatures[i];
					temperatures[i] = data->lastValidRead;
				}

				if((changed && config.getMqttPayloadFormat() == 1) || config.getMqttPayloadFormat() == 2) {
					mqtt.publish(String(config.getMqttPublishTopic()) + "/temperature/" + toHex(data->address), String(temperatures[i], 2));
				}

				anyChanged |= changed;
			}

			if(anyChanged && config.getMqttPayloadFormat() == 0) {
				StaticJsonDocument<512> json;
				JsonObject temps = json.createNestedObject("temperatures");
				for(int i = 0; i < c; i++) {
				TempSensorData* data = hw.getTempSensorData(i);
					JsonObject obj = temps.createNestedObject(toHex(data->address));
					obj["name"] = data->name;
					obj["value"] = serialized(String(temperatures[i], 2));
				}
				String msg;
				serializeJson(json, msg);
				mqtt.publish(config.getMqttPublishTopic(), msg.c_str());
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
			if(!wifiConnected) {
				wifiConnected = true;
				if(config.getAuthSecurity() > 0) {
					Debug.setPassword(config.getAuthPassword());
				}
				Debug.begin(config.getWifiHostname(), (uint8_t) config.getDebugLevel());
				Debug.setSerialEnabled(config.isDebugSerial());
				if(!config.isDebugTelnet()) {
					Debug.stop();
				}
				if(Debug.isActive(RemoteDebug::INFO)) {
					debugI("Successfully connected to WiFi!");
					debugI("IP: %s", WiFi.localIP().toString().c_str());
				}
				if(strlen(config.getWifiHostname()) > 0 && config.isMdnsEnable()) {
					debugD("mDNS is enabled, using host: %s", config.getWifiHostname());
					if(MDNS.begin(config.getWifiHostname())) {
						MDNS.addService("http", "tcp", 80);
					} else {
						debugE("Failed to set up mDNS!");
					}
				}
			}
			if(config.isNtpChanged()) {
				if(config.isNtpEnable()) {
					configTime(config.getNtpOffset(), config.getNtpSummerOffset(), config.getNtpServer());
					sntp_servermode_dhcp(config.isNtpDhcp() ? 1 : 0);
				}
				config.ackNtpChange();
			}
			#if defined ESP8266
			MDNS.update();
			#endif

			if(now > 10000 && now - lastErrorBlink > 3000) {
				errorBlink();
			}

			if (strlen(config.getMqttHost()) > 0) {
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
		if (now / 50 % 64 == 0) {
			if(!hw.ledBlink(LED_YELLOW, 1)) {
				hw.ledBlink(LED_INTERNAL, 1);
			}
		}
	}

	if(config.isMeterChanged()) {
		setupHanPort(config.getHanPin(), config.getMeterType());
		config.ackMeterChanged();
	}

	if(now - lastRead > 100) {
		yield();
		readHanPort();
		lastRead = now;
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
	hanReader.setEncryptionKey(config.getMeterEncryptionKey());
	hanReader.setAuthenticationKey(config.getMeterAuthenticationKey());

	// Compensate for the known Kaifa bug
	hanReader.compensateFor09HeaderBug = (newMeterType == 1);

	// Empty buffer before starting
	while (hanSerial->available() > 0) {
		hanSerial->read();
	}

	if(config.hasConfig() && config.isDebugSerial()) {
		if(WiFi.status() == WL_CONNECTED) {
			Debug.begin(config.getWifiHostname(), (uint8_t) config.getDebugLevel());
		}
		Debug.setSerialEnabled(config.isDebugSerial());
		if(!config.isDebugTelnet()) {
			Debug.stop();
		}
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
				if(strlen(config.getMqttHost()) > 0 && mqtt.lastError() != 0) {
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
	dnsServer.stop();
	WiFi.disconnect(true);
	WiFi.softAPdisconnect(true);
	WiFi.mode(WIFI_OFF);
	yield();

	if (mode != WIFI_AP || !config.hasConfig()) {
		if(Debug.isActive(RemoteDebug::INFO)) debugI("Swapping to AP mode");
		WiFi.softAP("AMS2MQTT");
		WiFi.mode(WIFI_AP);

		dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
		dnsServer.start(53, "*", WiFi.softAPIP());
	} else {
		if(Debug.isActive(RemoteDebug::INFO)) debugI("Swapping to STA mode");
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

int currentMeterType = 0;
AmsData lastMqttData;
void readHanPort() {
	if (hanReader.read()) {
		// Empty serial buffer. For some reason this seems to make a difference. Some garbage on the wire after package?
		while(hanSerial->available()) {
			hanSerial->read();
		}

		lastSuccessfulRead = millis();

		if(config.getMeterType() > 0) {
			if(!hw.ledBlink(LED_GREEN, 1))
				hw.ledBlink(LED_INTERNAL, 1);

			AmsData data(config.getMeterType(), config.isSubstituteMissing(), hanReader);
			if(data.getListType() > 0) {
				ws.setData(data);

				if(strlen(config.getMqttHost()) > 0 && strlen(config.getMqttPublishTopic()) > 0) {
					if(config.getMqttPayloadFormat() == 0) {
						StaticJsonDocument<512> json;
						hanToJson(json, data, hw, hw.getTemperature(), config.getMqttClientId());
						if (Debug.isActive(RemoteDebug::INFO)) {
							debugI("Sending data to MQTT");
							if (Debug.isActive(RemoteDebug::DEBUG)) {
								serializeJsonPretty(json, Debug);
							}
						}

						String msg;
						serializeJson(json, msg);
						mqtt.publish(config.getMqttPublishTopic(), msg.c_str());
					// 
					// Start DOMOTICZ
					//
					} else if(config.getMqttPayloadFormat() == 3) {
						debugI("Sending data to MQTT");
						//
						// Special MQTT messages for DOMOTIZ (https://www.domoticz.com/wiki/MQTT)
						// -All messages should be published to topic "domoticz/in"
						//
						//  message msg_PE : send active power and and cumulative energy consuption to  virtual meter "Electricity (instant and counter)"
						//
						//      /json.htm?type=command&param=udevice&idx=IDX&nvalue=0&svalue=POWER;ENERGY
						//
						//       MQTT sample message:    {"command": "udevice",  "idx" : IDX , "nvalue" : 0, "svalue" : "POWER;ENERGY"}   
						//         IDX = id of your device (This number can be found in the devices tab in the column "IDX")
						//         POWER = current power (Watt)
						//         ENERGY = cumulative energy in Watt-hours (Wh) This is an incrementing counter.
						//               (if you choose as type "Energy read : Computed", this is just a "dummy" counter, not updatable because it's the result of DomoticZ calculs from POWER)
						//
						//  message msg_V1 : send Voltage of L1 to virtual Voltage meter
						//
						//	      /json.htm?type=command&param=udevice&idx=IDX&nvalue=0&svalue=VOLTAGE
						//
						//       MQTT sample message:    {"command": "udevice",  "idx" : IDX , "nvalue" : 0, "svalue" : "VOLTAGE"}   
						//         IDX = id of your device (This number can be found in the devices tab in the column "IDX")
						//         VOLTAGE = Voltage (V)
						//  
			
						int idx1 = config.getDomoELIDX();
						if (idx1 > 0) {
							String PowerEnergy;
							int p;
							// double energy = config.getDomoEnergy();
							double tmp_energy;
							StaticJsonDocument<200> json_PE;
							p = data.getActiveImportPower();
							// cumulative energy is given only once pr hour. check if value is different from 0 and store last valid value on global variable.
							tmp_energy = data.getActiveImportCounter();
							if (tmp_energy > 1.0) energy = tmp_energy;		
							//  power_unit: watt, energy_unit: watt*h. Stored as kwh, need watth
							PowerEnergy = String((double) p/1.0) + ";" + String((double) energy*1000.0) ;
							json_PE["command"] = "udevice";
							json_PE["idx"] = idx1;
							json_PE["nvalue"] = 0;
							json_PE["svalue"] = PowerEnergy;
							// Stringify the json
							String msg_PE;
							serializeJson(json_PE, msg_PE);
							// publish power data directly to domoticz/in, but only after first reading of total power, once an hour... . (otherwise total consumtion will be wrong.)
							if (energy > 0.0 ) mqtt.publish("domoticz/in", msg_PE.c_str());
						}
						int idxu1 =config.getDomoVL1IDX();
						if (idxu1 > 0){				
							StaticJsonDocument<200> json_u1;
							double u1;
							//
							// prepare message msg_u1 for virtual Voltage meter"
							//
							u1 = data.getL1Voltage();
							if (u1 > 0.1){ 
								json_u1["command"] = "udevice";
								json_u1["idx"] = idxu1;
								json_u1["nvalue"] = 0;
								json_u1["svalue"] =  String(u1);
								// Stringify the json
								String msg_u1;
								serializeJson(json_u1, msg_u1);
								// publish power data directly to domoticz/in
								mqtt.publish("domoticz/in", msg_u1.c_str());
							}
						}
						int idxu2 =config.getDomoVL2IDX();
						if (idxu2 > 0){				
							StaticJsonDocument<200> json_u2;
							double u2;
							//
							// prepare message msg_u2 for virtual Voltage meter"
							//
							u2 = data.getL2Voltage();
							if (u2 > 0.1){ 
								json_u2["command"] = "udevice";
								json_u2["idx"] = idxu2;
								json_u2["nvalue"] = 0;
								json_u2["svalue"] =  String(u2);
								// Stringify the json
								String msg_u2;
								serializeJson(json_u2, msg_u2);
								// publish power data directly to domoticz/in
								mqtt.publish("domoticz/in", msg_u2.c_str());
							}
						}
						int idxu3 =config.getDomoVL3IDX();
						if (idxu3 > 0){				
							StaticJsonDocument<200> json_u3;
							double u3;
							//
							// prepare message msg_u3 for virtual Voltage meter"
							//
							u3 = data.getL3Voltage();
							if (u3 > 0.1){ 
								json_u3["command"] = "udevice";
								json_u3["idx"] = idxu3;
								json_u3["nvalue"] = 0;
								json_u3["svalue"] =  String(u3);
								// Stringify the json
								String msg_u3;
								serializeJson(json_u3, msg_u3);
								// publish power data directly to domoticz/in
								mqtt.publish("domoticz/in", msg_u3.c_str());
							}
						}
				
						int idxi1 =config.getDomoCL1IDX();
						if (idxi1 > 0){				
							StaticJsonDocument<200> json_i1;
							double i1, i2, i3;
							String Ampere3;
							//
							// prepare message msg_i1 for virtual Current/Ampere 3phase mater"
							//
							i1 = data.getL1Current();
							i2 = data.getL2Current();
							i3 = data.getL3Current();
							Ampere3 = String(i1) + ";" + String(i2) + ";" + String(i3) ;
							json_i1["command"] = "udevice";
							json_i1["idx"] = idxi1;
							json_i1["nvalue"] = 0;
							json_i1["svalue"] =  Ampere3;
							// Stringify the json
							String msg_i1;
							serializeJson(json_i1, msg_i1);
							// publish power data directly to domoticz/in
							if (i1 > 0.0) mqtt.publish("domoticz/in", msg_i1.c_str());
						}			
						//
						// End DOMOTICZ
						//
					} else if(config.getMqttPayloadFormat() == 1 || config.getMqttPayloadFormat() == 2) {
						mqtt.publish(String(config.getMqttPublishTopic()) + "/meter/dlms/timestamp", String(data.getPackageTimestamp()));
						switch(data.getListType()) {
							case 3:
								// ID and type belongs to List 2, but I see no need to send that every 10s
								mqtt.publish(String(config.getMqttPublishTopic()) + "/meter/id", data.getMeterId());
								mqtt.publish(String(config.getMqttPublishTopic()) + "/meter/type", data.getMeterType());
								mqtt.publish(String(config.getMqttPublishTopic()) + "/meter/clock", String(data.getMeterTimestamp()));
								mqtt.publish(String(config.getMqttPublishTopic()) + "/meter/import/reactive/accumulated", String(data.getReactiveImportCounter(), 2));
								mqtt.publish(String(config.getMqttPublishTopic()) + "/meter/import/active/accumulated", String(data.getActiveImportCounter(), 2));
								mqtt.publish(String(config.getMqttPublishTopic()) + "/meter/export/reactive/accumulated", String(data.getReactiveExportCounter(), 2));
								mqtt.publish(String(config.getMqttPublishTopic()) + "/meter/export/active/accumulated", String(data.getActiveExportCounter(), 2));
							case 2:
								// Only send data if changed. ID and Type is sent on the 10s interval only if changed
								if(lastMqttData.getMeterId() != data.getMeterId() || config.getMqttPayloadFormat() == 2) {
									mqtt.publish(String(config.getMqttPublishTopic()) + "/meter/id", data.getMeterId());
								}
								if(lastMqttData.getMeterType() != data.getMeterType() || config.getMqttPayloadFormat() == 2) {
									mqtt.publish(String(config.getMqttPublishTopic()) + "/meter/type", data.getMeterType());
								}
								if(lastMqttData.getL1Current() != data.getL1Current() || config.getMqttPayloadFormat() == 2) {
									mqtt.publish(String(config.getMqttPublishTopic()) + "/meter/l1/current", String(data.getL1Current(), 2));
								}
								if(lastMqttData.getL1Voltage() != data.getL1Voltage() || config.getMqttPayloadFormat() == 2) {
									mqtt.publish(String(config.getMqttPublishTopic()) + "/meter/l1/voltage", String(data.getL1Voltage(), 2));
								}
								if(lastMqttData.getL2Current() != data.getL2Current() || config.getMqttPayloadFormat() == 2) {
									mqtt.publish(String(config.getMqttPublishTopic()) + "/meter/l2/current", String(data.getL2Current(), 2));
								}
								if(lastMqttData.getL2Voltage() != data.getL2Voltage() || config.getMqttPayloadFormat() == 2) {
									mqtt.publish(String(config.getMqttPublishTopic()) + "/meter/l2/voltage", String(data.getL2Voltage(), 2));
								}
								if(lastMqttData.getL3Current() != data.getL3Current() || config.getMqttPayloadFormat() == 2) {
									mqtt.publish(String(config.getMqttPublishTopic()) + "/meter/l3/current", String(data.getL3Current(), 2));
								}
								if(lastMqttData.getL3Voltage() != data.getL3Voltage() || config.getMqttPayloadFormat() == 2) {
									mqtt.publish(String(config.getMqttPublishTopic()) + "/meter/l3/voltage", String(data.getL3Voltage(), 2));
								}
								if(lastMqttData.getReactiveExportPower() != data.getReactiveExportPower() || config.getMqttPayloadFormat() == 2) {
									mqtt.publish(String(config.getMqttPublishTopic()) + "/meter/export/reactive", String(data.getReactiveExportPower()));
								}
								if(lastMqttData.getActiveExportPower() != data.getActiveExportPower() || config.getMqttPayloadFormat() == 2) {
									mqtt.publish(String(config.getMqttPublishTopic()) + "/meter/export/active", String(data.getActiveExportPower()));
								}
								if(lastMqttData.getReactiveImportPower() != data.getReactiveImportPower() || config.getMqttPayloadFormat() == 2) {
									mqtt.publish(String(config.getMqttPublishTopic()) + "/meter/import/reactive", String(data.getReactiveImportPower()));
								}
							case 1:
								if(lastMqttData.getActiveImportPower() != data.getActiveImportPower() || config.getMqttPayloadFormat() == 2) {
									mqtt.publish(String(config.getMqttPublishTopic()) + "/meter/import/active", String(data.getActiveImportPower()));
								}
						}
					}
					lastMqttData.apply(data);
					mqtt.loop();
					delay(10);
				}
			} else {
				if(config.isSendUnknown() && strlen(config.getMqttHost()) > 0 && strlen(config.getMqttPublishTopic()) > 0) {
					byte buf[512];
					int length = hanReader.getBuffer(buf);
					String hexstring = "";

					for(int i = 0; i < length; i++) {
						if(buf[i] < 0x10) {
						hexstring += '0';
						}

						hexstring += String(buf[i], HEX);
					}
					mqtt.publish(String(config.getMqttPublishTopic()), hexstring);
				}
			}
		} else {
			// Auto detect meter if not set
			for(int i = 1; i <= 4; i++) {
				String list;
				switch(i) {
					case METER_TYPE_KAIFA:
						list = hanReader.getString((int) Kaifa_List1Phase::ListVersionIdentifier);
						break;
					case METER_TYPE_AIDON:
						list = hanReader.getString((int) Aidon_List1Phase::ListVersionIdentifier);
						break;
					case METER_TYPE_KAMSTRUP:
						list = hanReader.getString((int) Kamstrup_List1Phase::ListVersionIdentifier);
						break;
					case METER_TYPE_OMNIPOWER:
						list = hanReader.getString((int) Omnipower_DLMS::ListVersionIdentifier);
						break;
				}
				if(!list.isEmpty()) {
					list.toLowerCase();
					if(list.startsWith("kfm")) {
						config.setMeterType(METER_TYPE_KAIFA);
						if(Debug.isActive(RemoteDebug::INFO)) debugI("Detected Kaifa meter");
					} else if(list.startsWith("aidon")) {
						config.setMeterType(METER_TYPE_AIDON);
						if(Debug.isActive(RemoteDebug::INFO)) debugI("Detected Aidon meter");
					} else if(list.startsWith("kamstrup")) {
						switch(i) {
							case METER_TYPE_KAMSTRUP:
								config.setMeterType(METER_TYPE_KAMSTRUP);
								if(Debug.isActive(RemoteDebug::INFO)) debugI("Detected Kamstrup meter");
								break;
							case METER_TYPE_OMNIPOWER:
								config.setMeterType(METER_TYPE_OMNIPOWER);
								if(Debug.isActive(RemoteDebug::INFO)) debugI("Detected Kamstrup meter");
								break;
						}
					}
				}
			}
			hanReader.compensateFor09HeaderBug = (config.getMeterType() == 1);
		}
	}

	// Switch parity if meter is still not detected
	if(config.getMeterType() == 0 && millis() - lastSuccessfulRead > 10000) {
		lastSuccessfulRead = millis();
		debugD("No data for current setting, switching parity");
		Serial.flush();
		if(++currentMeterType == 4) currentMeterType = 1;
		setupHanPort(config.getHanPin(), currentMeterType);
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

	if (Debug.isActive(RemoteDebug::INFO)) debugI("Connecting to WiFi network: %s", config.getWifiSsid());

	if (WiFi.status() != WL_CONNECTED) {
		MDNS.end();
		WiFi.disconnect();
		yield();

		WiFi.enableAP(false);
		WiFi.mode(WIFI_STA);
		if(strlen(config.getWifiIp()) > 0) {
			IPAddress ip, gw, sn(255,255,255,0), dns1, dns2;
			ip.fromString(config.getWifiIp());
			gw.fromString(config.getWifiGw());
			sn.fromString(config.getWifiSubnet());
			dns1.fromString(config.getWifiDns1());
			dns2.fromString(config.getWifiDns2());
			WiFi.config(ip, gw, sn, dns1, dns2);
		} else {
#if defined(ESP32)
			WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE); // Workaround to make DHCP hostname work for ESP32. See: https://github.com/espressif/arduino-esp32/issues/2537
#endif
		}
		if(strlen(config.getWifiHostname()) > 0) {
#if defined(ESP8266)
			WiFi.hostname(config.getWifiHostname());
#elif defined(ESP32)
			WiFi.setHostname(config.getWifiHostname());
#endif
		}
		WiFi.begin(config.getWifiSsid(), config.getWifiPassword());
		yield();
	}
}

unsigned long lastMqttRetry = -10000;
void MQTT_connect() {
	if(strlen(config.getMqttHost()) == 0) {
		if(Debug.isActive(RemoteDebug::WARNING)) debugW("No MQTT config");
		return;
	}
	if(millis() - lastMqttRetry < (mqtt.lastError() == 0 ? 5000 : 60000)) {
		yield();
		return;
	}
	lastMqttRetry = millis();
	if(Debug.isActive(RemoteDebug::INFO)) {
		debugD("Disconnecting MQTT before connecting");
	}

	mqtt.disconnect();
	yield();

	WiFiClientSecure *secureClient = NULL;
	Client *client = NULL;
	if(config.isMqttSsl()) {
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
		debugI("Connecting to MQTT %s:%d", config.getMqttHost(), config.getMqttPort());
	}
	mqtt.begin(config.getMqttHost(), config.getMqttPort(), *client);

#if defined(ESP8266)
	if(secureClient) {
		time_t epoch = time(nullptr);
		debugD("Setting NTP time %i for secure MQTT connection", epoch);
 		secureClient->setX509Time(epoch);
	}
#endif

	// Connect to a unsecure or secure MQTT server
	if ((strlen(config.getMqttUser()) == 0 && mqtt.connect(config.getMqttClientId())) ||
		(strlen(config.getMqttUser()) > 0 && mqtt.connect(config.getMqttClientId(), config.getMqttUser(), config.getMqttPassword()))) {
		if (Debug.isActive(RemoteDebug::INFO)) debugI("Successfully connected to MQTT!");
		config.ackMqttChange();

		// Subscribe to the chosen MQTT topic, if set in configuration
		if (strlen(config.getMqttSubscribeTopic()) > 0) {
			mqtt.subscribe(config.getMqttSubscribeTopic());
			if (Debug.isActive(RemoteDebug::INFO)) debugI("  Subscribing to [%s]\r\n", config.getMqttSubscribeTopic());
		}
		
		if(config.getMqttPayloadFormat() == 0) {
			sendMqttData("Connected!");
		} else if(config.getMqttPayloadFormat() == 1) {
			sendSystemStatusToMqtt();
		}
	} else {
		if (Debug.isActive(RemoteDebug::ERROR)) {
			debugE("Failed to connect to MQTT");
#if defined(ESP8266)
			if(secureClient) {
				char buf[256];
  				secureClient->getLastSSLError(buf,256);
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
	if (strlen(config.getMqttPublishTopic()) == 0)
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

	if (Debug.isActive(RemoteDebug::INFO)) debugI("Sending MQTT data");
	if (Debug.isActive(RemoteDebug::DEBUG)) debugD("[%s]", data.c_str());
}

unsigned long lastSystemDataSent = -10000;
void sendSystemStatusToMqtt() {
	if (strlen(config.getMqttPublishTopic()) == 0)
		return;
	if(millis() - lastSystemDataSent < 10000)
		return;
	lastSystemDataSent = millis();

	mqtt.publish(String(config.getMqttPublishTopic()) + "/id", WiFi.macAddress());
	mqtt.publish(String(config.getMqttPublishTopic()) + "/uptime", String((unsigned long) millis64()/1000));
	double vcc = hw.getVcc();
	if(vcc > 0) {
		mqtt.publish(String(config.getMqttPublishTopic()) + "/vcc", String(vcc, 2));
	}
	mqtt.publish(String(config.getMqttPublishTopic()) + "/rssi", String(hw.getWifiRssi()));
    if(hw.getTemperature() > -85) {
		mqtt.publish(String(config.getMqttPublishTopic()) + "/temperature", String(hw.getTemperature(), 2));
    }
}
