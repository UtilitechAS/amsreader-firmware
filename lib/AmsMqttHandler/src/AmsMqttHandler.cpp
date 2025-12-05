/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "AmsMqttHandler.h"
#include "FirmwareVersion.h"
#include "AmsStorage.h"
#include "LittleFS.h"
#include "Uptime.h"

void AmsMqttHandler::setCaVerification(bool caVerification) {
	this->caVerification = caVerification;
}

void AmsMqttHandler::setConfig(MqttConfig& mqttConfig) {
	this->mqttConfig = mqttConfig;
	this->mqttConfigChanged = true;
}

bool AmsMqttHandler::connect() {
	if(millis() - lastMqttRetry < 10000) {
		yield();
		return false;
	}
	lastMqttRetry = millis();

	time_t epoch = time(nullptr);
	
	WiFiClient *actualClient = NULL;

	if(mqttConfig.ssl) {
		if(epoch < FirmwareVersion::BuildEpoch) {
			return false;
		}
		
		bool applySslConfiguration = mqttConfigChanged;
		if(mqttSecureClient == NULL) {
			mqttSecureClient = new WiFiClientSecure();
			#if defined(ESP8266)
				mqttSecureClient->setBufferSizes(512, 512);
				return false;
			#endif
			applySslConfiguration = true;
		}

		if(applySslConfiguration) {
			if(caVerification && LittleFS.begin()) {
				File file;

				if(LittleFS.exists(FILE_MQTT_CA)) {
					file = LittleFS.open(FILE_MQTT_CA, (char*) "r");
					#if defined(ESP8266)
						BearSSL::X509List *serverTrustedCA = new BearSSL::X509List(file);
						mqttSecureClient->setTrustAnchors(serverTrustedCA);
					#elif defined(ESP32)
						if(!mqttSecureClient->loadCACert(file, file.size())) {
							return false;
						}
					#endif
					file.close();
				} else {
					mqttSecureClient->setInsecure();
				}

				#if defined(ESP8266)
					if(LittleFS.exists(FILE_MQTT_CERT) && LittleFS.exists(FILE_MQTT_KEY)) {
						file = LittleFS.open(FILE_MQTT_CERT, (char*) "r");
						BearSSL::X509List *serverCertList = new BearSSL::X509List(file);
						file.close();

						file = LittleFS.open(FILE_MQTT_KEY, (char*) "r");
						BearSSL::PrivateKey *serverPrivKey = new BearSSL::PrivateKey(file);
						file.close();

						mqttSecureClient->setClientRSACert(serverCertList, serverPrivKey);
					}
				#endif

				#if defined(ESP32)
					if(LittleFS.exists(FILE_MQTT_CERT)) {
						file = LittleFS.open(FILE_MQTT_CERT, (char*) "r");
						mqttSecureClient->loadCertificate(file, file.size());
						file.close();
					}
				
					if(LittleFS.exists(FILE_MQTT_KEY)) {
						file = LittleFS.open(FILE_MQTT_KEY, (char*) "r");
						mqttSecureClient->loadPrivateKey(file, file.size());
						file.close();
					}
				#endif
			} else {
				mqttSecureClient->setInsecure();
			}
		}
		actualClient = mqttSecureClient;
	} else {
		if(mqttClient == NULL) {
			mqttClient = new WiFiClient();
		}
		actualClient = mqttClient;
	}

	// This section helps with power saving on ESP32 devices by reducing timeouts
	// The timeout is multiplied by 10 because WiFiClient is retrying 10 times internally
	// Power drain for this timeout is too great when using the default 3s timeout
	// On ESP8266 the timeout is used differently and the following code causes MQTT instability
	#if defined(ESP32)
		int clientTimeout = mqttConfig.timeout / 1000;
		if(clientTimeout > 3) clientTimeout = 3; // 3000ms is default, see WiFiClient.cpp WIFI_CLIENT_DEF_CONN_TIMEOUT_MS
		actualClient->setTimeout(clientTimeout);
		// Why can't we set number of retries for write here? WiFiClient defaults to 10 (10*3s == 30s)
	#endif

	mqttConfigChanged = false;
	mqtt.setTimeout(mqttConfig.timeout);
	mqtt.setKeepAlive(mqttConfig.keepalive);
	mqtt.begin(mqttConfig.host, mqttConfig.port, *actualClient);
	String statusTopic = String(mqttConfig.publishTopic) + "/status";
	mqtt.setWill(statusTopic.c_str(), "offline", true, 0);

	#if defined(ESP8266)
		if(mqttSecureClient) {
			time_t epoch = time(nullptr);
			mqttSecureClient->setX509Time(epoch);
		}
	#endif

	// Connect to a unsecure or secure MQTT server
	if ((strlen(mqttConfig.username) == 0 && mqtt.connect(mqttConfig.clientId)) ||
		(strlen(mqttConfig.username) > 0 && mqtt.connect(mqttConfig.clientId, mqttConfig.username, mqttConfig.password))) {
		#if defined(AMS_REMOTE_DEBUG)
		if (debugger->isActive(RemoteDebug::INFO))
		#endif
		debugger->printf_P(PSTR("Successfully connected to MQTT\n"));
		mqtt.onMessage(std::bind(&AmsMqttHandler::onMessage, this, std::placeholders::_1, std::placeholders::_2));
		_connected = mqtt.publish(statusTopic, "online", true, 0);
        mqtt.loop();
		defaultSubscribe();
		postConnect();
        return true;
	} else {
		#if defined(AMS_REMOTE_DEBUG)
		if (debugger->isActive(RemoteDebug::ERROR))
		#endif
		{
			debugger->printf_P(PSTR("Failed to connect to MQTT: %d\n"), mqtt.lastError());
			#if defined(ESP8266)
				if(mqttSecureClient) {
					mqttSecureClient->getLastSSLError((char*) json, BufferSize);
					debugger->println((char*) json);
				}
			#endif
		}
        return false;
	}
}

bool AmsMqttHandler::defaultSubscribe() {
	bool ret = true;
	if(!subTopic.isEmpty()) {
        if(mqtt.subscribe(subTopic)) {
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::ERROR))
            #endif
            debugger->printf_P(PSTR("  Subscribed to [%s]\n"), subTopic.c_str());
        } else {
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::ERROR))
            #endif
            debugger->printf_P(PSTR("  Unable to subscribe to [%s]\n"), subTopic.c_str());
            ret = false;
        }
    }
	return ret;
}

void AmsMqttHandler::disconnect() {
    mqtt.disconnect();
    mqtt.loop();
	_connected = false;
    delay(10);
    yield();
}

lwmqtt_err_t AmsMqttHandler::lastError() {
    return mqtt.lastError();
}

bool AmsMqttHandler::connected() {
	return _connected && mqtt.connected();
}

bool AmsMqttHandler::loop() {
	uint64_t now = millis64();
    bool ret = connected() && mqtt.loop();
	if(ret) {
		lastSuccessfulLoop = now;
	} else if(mqttConfig.rebootMinutes > 0) {
		if(now - lastSuccessfulLoop > (uint64_t) mqttConfig.rebootMinutes * 60000) {
			// Reboot the device if the MQTT connection is lost for too long
			#if defined(AMS_REMOTE_DEBUG)
			if (debugger->isActive(RemoteDebug::WARNING))
			#endif
			debugger->printf_P(PSTR("MQTT connection lost for over %d minutes, rebooting device\n"), mqttConfig.rebootMinutes);
			rebootSuggested = true;
		}
	}
	delay(10); // Needed to preserve power. After adding this, the voltage is super smooth on a HAN powered device
    yield();
	#if defined(ESP32)
		esp_task_wdt_reset();
	#elif defined(ESP8266)
		ESP.wdtFeed();
	#endif
    return ret;
}

bool AmsMqttHandler::isRebootSuggested() {
	return rebootSuggested;
}