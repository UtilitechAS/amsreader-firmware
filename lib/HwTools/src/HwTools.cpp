/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "HwTools.h"

bool HwTools::applyBoardConfig(uint8_t boardType, GpioConfig& gpioConfig, MeterConfig& meterConfig, uint8_t hanPin) {
    #if defined(CONFIG_IDF_TARGET_ESP32S2)
        switch(boardType) {
            case 5: // Pow-K+
                meterConfig.txPin = 9;
            case 7: // Pow-U+
            case 6: // Pow-P1
                meterConfig.rxPin = 16;
                gpioConfig.apPin = 0;
                gpioConfig.ledPinRed = 13;
                gpioConfig.ledPinGreen = 14;
                gpioConfig.ledRgbInverted = true;
                gpioConfig.vccPin = 10;
                gpioConfig.vccResistorGnd = 22;
                gpioConfig.vccResistorVcc = 33;
                gpioConfig.ledDisablePin = 6;
                return true;
            case 51: // Wemos S2 mini
                gpioConfig.ledPin = 15;
                gpioConfig.ledInverted = false;
                gpioConfig.apPin = 0;
                meterConfig.rxPin = hanPin > 0 ? hanPin : 18;
                if(meterConfig.rxPin != 18) {
                    gpioConfig.vccPin = 18;
                    gpioConfig.vccResistorGnd = 45;
                    gpioConfig.vccResistorVcc = 10;
                }
                return true;
            case 50: // Generic ESP32-S2
                meterConfig.rxPin = hanPin > 0 ? hanPin : 18;
                return true;
        }
    #elif defined(CONFIG_IDF_TARGET_ESP32C3)
        switch(boardType) {
            case 8: // dbeinder: HAN mosquito
                meterConfig.rxPin = 7;
                meterConfig.rxPinPullup = false;
                gpioConfig.apPin = 9;
                gpioConfig.ledRgbInverted = true;
                gpioConfig.ledPinRed = 5;
                gpioConfig.ledPinGreen = 6;
                gpioConfig.ledPinBlue = 4;
                return true;
            case 71: // ESP32-C3-DevKitM-1
                gpioConfig.apPin = 9;
            case 70: // Generic ESP32-C3
                meterConfig.rxPin = hanPin > 0 ? hanPin : 7;
                return true;
        }
    #elif defined(CONFIG_IDF_TARGET_ESP32S3)
        switch(boardType) {
            case 80: // Generic ESP32-S3
                meterConfig.rxPin = hanPin > 0 ? hanPin : 18;
                return true;
        }
    #elif defined(ESP32)
        switch(boardType) {
            case 241: // LilyGO T-ETH-POE
                gpioConfig.apPin = 0;
                meterConfig.rxPin = hanPin > 0 ? hanPin : 39;
                gpioConfig.ledPin = 2;
                gpioConfig.ledInverted = true;
                return true;
            case 242: // M5 PoESP32
                meterConfig.rxPin = hanPin > 0 ? hanPin : 16;
                return true;
            case 243: // WT32-ETH01
                meterConfig.rxPin = hanPin > 0 ? hanPin : 39;
                return true;
            case 245: // wESP32
                gpioConfig.apPin = 0;
                meterConfig.rxPin = hanPin > 0 ? hanPin : 39;
            case 201: // D32
                meterConfig.rxPin = hanPin > 0 ? hanPin : 16;
                gpioConfig.apPin = 4;
                gpioConfig.ledPin = 5;
                gpioConfig.ledInverted = true;
                return true;
            case 202: // Feather
            case 203: // DevKitC
            case 200: // ESP32
                meterConfig.rxPin = hanPin > 0 ? hanPin : 16;
                gpioConfig.ledPin = 2;
                gpioConfig.ledInverted = false;
                return true;
        }
    #elif defined(ESP8266)
        switch(boardType) {
            case 2: // spenceme
                gpioConfig.vccBootLimit = 32;
                meterConfig.rxPin = 3;
                gpioConfig.apPin = 0;
                gpioConfig.ledPin = 2;
                gpioConfig.ledInverted = true;
                gpioConfig.tempSensorPin = 5;
                return true;
            case 0: // roarfred
                meterConfig.rxPin = 3;
                gpioConfig.apPin = 0;
                gpioConfig.ledPin = 2;
                gpioConfig.ledInverted = true;
                gpioConfig.tempSensorPin = 5;
                return true;
            case 1: // Arnio Kamstrup
            case 3: // Pow-K UART0
            case 4: // Pow-U UART0
                meterConfig.rxPin = 3;
                gpioConfig.apPin = 0;
                gpioConfig.ledPin = 2;
                gpioConfig.ledInverted = true;
                gpioConfig.ledPinRed = 13;
                gpioConfig.ledPinGreen = 14;
                gpioConfig.ledRgbInverted = true;
                return true;
            case 5: // Pow-K GPIO12
            case 7: // Pow-U GPIO12
                meterConfig.rxPin = 12;
                gpioConfig.apPin = 0;
                gpioConfig.ledPin = 2;
                gpioConfig.ledInverted = true;
                gpioConfig.ledPinRed = 13;
                gpioConfig.ledPinGreen = 14;
                gpioConfig.ledRgbInverted = true;
                return true;
            case 101: // D1
                meterConfig.rxPin = hanPin > 0 ? hanPin : 5;
                gpioConfig.apPin = 4;
                gpioConfig.ledPin = 2;
                gpioConfig.ledInverted = true;
                gpioConfig.vccMultiplier = 1100;
                return true;
            case 100: // ESP8266
                meterConfig.rxPin = hanPin > 0 ? hanPin : 3;
                gpioConfig.ledPin = 2;
                gpioConfig.ledInverted = true;
                return true;
        }
    #endif
    return false;
}

void HwTools::setup(SystemConfig* sys, GpioConfig* config) {
    this->boardType = sys->boardType;
    this->tempSensorInit = false;
    if(sensorApi != NULL)
        delete sensorApi;
    if(oneWire != NULL)
        delete oneWire;
    if(config->tempSensorPin > 0 && config->tempSensorPin < 40) {
        pinMode(config->tempSensorPin, INPUT);
        tempPin = config->tempSensorPin;
    } else {
        tempPin = config->tempSensorPin = 0xFF;
    }

    #if defined(CONFIG_IDF_TARGET_ESP32S2)
        analogReadResolution(13);
        analogRange = 8192;
        analogSetAttenuation(ADC_11db);
    #elif defined(ESP32)
        analogReadResolution(12);
        analogRange = 4096;
        analogSetAttenuation(ADC_6db);
    #endif
    if(config->vccPin > 0 && config->vccPin < 40) {
        #if defined(CONFIG_IDF_TARGET_ESP32S2)
            getAdcChannel(config->vccPin, voltAdc);
            if(voltAdc.unit != 0xFF) {
                if(voltAdc.unit == ADC_UNIT_1) {
                    voltAdcChar = (esp_adc_cal_characteristics_t*) calloc(1, sizeof(esp_adc_cal_characteristics_t));
                    esp_adc_cal_value_t adcVal = esp_adc_cal_characterize((adc_unit_t) voltAdc.unit, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_13, 1100, voltAdcChar);
                    adc1_config_channel_atten((adc1_channel_t) voltAdc.channel, ADC_ATTEN_DB_11);
                } else if(voltAdc.unit == ADC_UNIT_2) {
                    voltAdcChar = (esp_adc_cal_characteristics_t*) calloc(1, sizeof(esp_adc_cal_characteristics_t));
                    esp_adc_cal_value_t adcVal = esp_adc_cal_characterize((adc_unit_t) voltAdc.unit, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_13, 1100, voltAdcChar);
                    adc2_config_channel_atten((adc2_channel_t) voltAdc.channel, ADC_ATTEN_DB_11);
                }
            }
        #elif defined(ESP32)
            getAdcChannel(config->vccPin, voltAdc);
            if(voltAdc.unit != 0xFF) {
                if(voltAdc.unit == ADC_UNIT_1) {
                    voltAdcChar = (esp_adc_cal_characteristics_t*) calloc(1, sizeof(esp_adc_cal_characteristics_t));
                    esp_adc_cal_value_t adcVal = esp_adc_cal_characterize((adc_unit_t) voltAdc.unit, ADC_ATTEN_DB_6, ADC_WIDTH_BIT_12, 1100, voltAdcChar);
                    adc1_config_channel_atten((adc1_channel_t) voltAdc.channel, ADC_ATTEN_DB_6);
                } else if(voltAdc.unit == ADC_UNIT_2) {
                    voltAdcChar = (esp_adc_cal_characteristics_t*) calloc(1, sizeof(esp_adc_cal_characteristics_t));
                    esp_adc_cal_value_t adcVal = esp_adc_cal_characterize((adc_unit_t) voltAdc.unit, ADC_ATTEN_DB_6, ADC_WIDTH_BIT_12, 1100, voltAdcChar);
                    adc2_config_channel_atten((adc2_channel_t) voltAdc.channel, ADC_ATTEN_DB_6);
                }
            }
        #else
            pinMode(config->vccPin, INPUT);
        #endif
        vccPin = config->vccPin;
    } else {
        voltAdc.unit = 0xFF;
        voltAdc.channel = 0xFF;
        vccPin = config->vccPin = 0xFF;
    }
    vccOffset = config->vccOffset / 100.0;
    vccMultiplier = config->vccMultiplier / 1000.0;
    vccGnd_r = config->vccResistorGnd;
    vccVcc_r = config->vccResistorVcc;

    if(config->tempAnalogSensorPin > 0 && config->tempAnalogSensorPin < 40) {
        pinMode(config->tempAnalogSensorPin, INPUT);
        atempPin = config->tempAnalogSensorPin;
    } else {
        atempPin = config->tempAnalogSensorPin = 0xFF;
    }

    if(config->ledPin > 0 && config->ledPin < 40) {
        pinMode(config->ledPin, OUTPUT);
        ledPin = config->ledPin;
        ledInvert = config->ledInverted;
        ledOff(LED_INTERNAL);
    } else {
        ledPin = config->ledPin = 0xFF;
    }

    if(config->ledPinRed > 0 && config->ledPinRed < 40) {
        pinMode(config->ledPinRed, OUTPUT);
        redPin = config->ledPinRed;
        ledOff(LED_RED);
    } else {
        redPin = config->ledPinRed = 0xFF;
    }

    if(config->ledPinGreen > 0 && config->ledPinGreen < 40) {
        pinMode(config->ledPinGreen, OUTPUT);
        greenPin = config->ledPinGreen;
        ledOff(LED_GREEN);
    } else {
        greenPin = config->ledPinGreen = 0xFF;
    }

    if(config->ledPinBlue > 0 && config->ledPinBlue < 40) {
        pinMode(config->ledPinBlue, OUTPUT);
        bluePin = config->ledPinBlue;
        ledOff(LED_BLUE);
    } else {
        bluePin = config->ledPinBlue = 0xFF;
    }

    rgbInvert = config->ledRgbInverted;

    if(config->ledDisablePin > 0 && config->ledDisablePin < 40) {
        pinMode(config->ledDisablePin, OUTPUT_OPEN_DRAIN);
        ledDisablePin = config->ledDisablePin;
        ledBehaviour = config->ledBehaviour;
        setBootSuccessful(false);
    }
}

void HwTools::getAdcChannel(uint8_t pin, AdcConfig& config) {
    config.unit = 0xFF;
    config.channel = 0xFF;
    #if defined(ESP32)
        switch(pin) {
            case ADC1_CHANNEL_0_GPIO_NUM:
                config.unit = ADC_UNIT_1;
                config.channel = ADC1_CHANNEL_0;
                break;
            case ADC1_CHANNEL_1_GPIO_NUM:
                config.unit = ADC_UNIT_1;
                config.channel = ADC1_CHANNEL_1;
                break;
            case ADC1_CHANNEL_2_GPIO_NUM:
                config.unit = ADC_UNIT_1;
                config.channel = ADC1_CHANNEL_2;
                break;
            case ADC1_CHANNEL_3_GPIO_NUM:
                config.unit = ADC_UNIT_1;
                config.channel = ADC1_CHANNEL_3;
                break;
            case ADC1_CHANNEL_4_GPIO_NUM:
                config.unit = ADC_UNIT_1;
                config.channel = ADC1_CHANNEL_4;
                break;
            #if defined(ADC1_CHANNEL_5_GPIO_NUM)
            case ADC1_CHANNEL_5_GPIO_NUM:
                config.unit = ADC_UNIT_1;
                config.channel = ADC1_CHANNEL_5;
                break;
            #endif
            #if defined(ADC1_CHANNEL_6_GPIO_NUM)
            case ADC1_CHANNEL_6_GPIO_NUM:
                config.unit = ADC_UNIT_1;
                config.channel = ADC1_CHANNEL_6;
                break;
            #endif
            #if defined(ADC1_CHANNEL_7_GPIO_NUM)
            case ADC1_CHANNEL_7_GPIO_NUM:
                config.unit = ADC_UNIT_1;
                config.channel = ADC1_CHANNEL_7;
                break;
            #endif
            #if defined(ADC1_CHANNEL_8_GPIO_NUM)
            case ADC1_CHANNEL_8_GPIO_NUM:
                config.unit = ADC_UNIT_1;
                config.channel = ADC1_CHANNEL_8;
                break;
            #endif
            #if defined(ADC1_CHANNEL_9_GPIO_NUM)
            case ADC1_CHANNEL_9_GPIO_NUM:
                config.unit = ADC_UNIT_1;
                config.channel = ADC1_CHANNEL_9;
                break;
            #endif
            #if defined(ADC2_CHANNEL_0_GPIO_NUM)
            case ADC2_CHANNEL_0_GPIO_NUM:
                config.unit = ADC_UNIT_2;
                config.channel = ADC2_CHANNEL_0;
                break;
            #endif
            #if defined(ADC2_CHANNEL_1_GPIO_NUM)
            case ADC2_CHANNEL_1_GPIO_NUM:
                config.unit = ADC_UNIT_2;
                config.channel = ADC2_CHANNEL_1;
                break;
            #endif
            #if defined(ADC2_CHANNEL_2_GPIO_NUM)
            case ADC2_CHANNEL_2_GPIO_NUM:
                config.unit = ADC_UNIT_2;
                config.channel = ADC2_CHANNEL_2;
                break;
            #endif
            #if defined(ADC2_CHANNEL_3_GPIO_NUM)
            case ADC2_CHANNEL_3_GPIO_NUM:
                config.unit = ADC_UNIT_2;
                config.channel = ADC2_CHANNEL_3;
                break;
            #endif
            #if defined(ADC2_CHANNEL_4_GPIO_NUM)
            case ADC2_CHANNEL_4_GPIO_NUM:
                config.unit = ADC_UNIT_2;
                config.channel = ADC2_CHANNEL_4;
                break;
            #endif
            #if defined(ADC2_CHANNEL_5_GPIO_NUM)
            case ADC2_CHANNEL_5_GPIO_NUM:
                config.unit = ADC_UNIT_2;
                config.channel = ADC2_CHANNEL_5;
                break;
            #endif
            #if defined(ADC2_CHANNEL_6_GPIO_NUM)
            case ADC2_CHANNEL_6_GPIO_NUM:
                config.unit = ADC_UNIT_2;
                config.channel = ADC2_CHANNEL_6;
                break;
            #endif
            #if defined(ADC2_CHANNEL_7_GPIO_NUM)
            case ADC2_CHANNEL_7_GPIO_NUM:
                config.unit = ADC_UNIT_2;
                config.channel = ADC2_CHANNEL_7;
                break;
            #endif
            #if defined(ADC2_CHANNEL_8_GPIO_NUM)
            case ADC2_CHANNEL_8_GPIO_NUM:
                config.unit = ADC_UNIT_2;
                config.channel = ADC2_CHANNEL_8;
                break;
            #endif
            #if defined(ADC2_CHANNEL_9_GPIO_NUM)
            case ADC2_CHANNEL_9_GPIO_NUM:
                config.unit = ADC_UNIT_2;
                config.channel = ADC2_CHANNEL_9;
                break;
            #endif
        }
    #endif
}

float HwTools::getVcc() {
    float volts = 0.0;
    if(vccPin != 0xFF) {
        #if defined(ESP32)
            if(voltAdc.unit != 0xFF) {
                uint32_t x = 0;
                for (int i = 0; i < 10; i++) {
                    if(voltAdc.unit == ADC_UNIT_1) {
                        x +=  adc1_get_raw((adc1_channel_t) voltAdc.channel);
                    } else if(voltAdc.unit == ADC_UNIT_2) {
                        int v = 0;
                        #if defined(CONFIG_IDF_TARGET_ESP32S2)
                        adc2_get_raw((adc2_channel_t) voltAdc.channel, ADC_WIDTH_BIT_13, &v);
                        #elif defined(CONFIG_IDF_TARGET_ESP32)
                        adc2_get_raw((adc2_channel_t) voltAdc.channel, ADC_WIDTH_BIT_12, &v);
                        #endif
                        x += v;
                    }
                }
                x = x / 10;
                uint32_t voltage = esp_adc_cal_raw_to_voltage(x, voltAdcChar);
                volts = voltage / 1000.0;
            } else {
                uint32_t x = 0;
                for (int i = 0; i < 10; i++) {
                    x += analogRead(vccPin);
                }
                volts = (x * 3.3) / 10.0 / analogRange;
            }
        #else
            uint32_t x = 0;
            for (int i = 0; i < 10; i++) {
                x += analogRead(vccPin);
            }
            volts = (x * 3.3) / 10.0 / analogRange;
        #endif
    } else {
    }
    if(volts == 0.0) {
        #if defined(ESP8266)
            volts = ESP.getVcc() / 1024.0;
        #else
            return 0.0;
        #endif
    }

    if(vccGnd_r > 0 && vccVcc_r > 0)
        volts *= ((float) (vccGnd_r + vccVcc_r) / vccGnd_r);
    if(vccOffset != 0.0)
        volts += vccOffset;
    if(vccMultiplier != 0.0)
        volts *= vccMultiplier;
    
    return volts;
}

uint8_t HwTools::getTempSensorCount() {
    return sensorCount;
}

TempSensorData* HwTools::getTempSensorData(uint8_t i) {
    if(i < sensorCount) {
        return tempSensors[i];
    }
    return NULL;
}

bool HwTools::updateTemperatures() {
    if(tempPin != 0xFF) {
        if(!tempSensorInit) {
            oneWire = new OneWire(tempPin);
            sensorApi = new DallasTemperature(this->oneWire);
            sensorApi->begin();
            delay(100);
            tempSensorInit = true;

            DeviceAddress addr;
            sensorApi->requestTemperatures();
            int c = sensorApi->getDeviceCount();
            if(this->tempSensors != NULL) {
                delete this->tempSensors;
            }
            this->tempSensors = new TempSensorData*[c];
            for(int i = 0; i < c; i++) {
                bool found = false;
                sensorApi->getAddress(addr, i);
                float t = sensorApi->getTempC(addr);
                for(int x = 0; x < sensorCount; x++) {
                    TempSensorData *data = tempSensors[x];
                    if(isSensorAddressEqual(data->address, addr)) {
                        found = true;
                        data->lastRead = t;
                        if(t > -85) {
                            data->changed = data->lastValidRead != t;
                            data->lastValidRead = t;
                        }
                    }
                }
                if(!found) {
                    TempSensorData *data = new TempSensorData();
                    memcpy(data->address, addr, 8);
                    data->lastRead = t;
                    if(t > -85) {
                        data->changed = data->lastValidRead != t;
                        data->lastValidRead = t;
                    }
                    tempSensors[sensorCount++] = data;
                }
                yield();
            }
        } else {
            if(sensorCount > 0) {
                sensorApi->requestTemperatures();

                for(int x = 0; x < sensorCount; x++) {
                    TempSensorData *data = tempSensors[x];
                    float t = sensorApi->getTempC(data->address);
                    data->lastRead = t;
                    if(t > -85) {
                        data->changed = data->lastValidRead != t;
                        data->lastValidRead = t;
                    }
                }
            }
        }

        return true;
    }
    return false;
}

bool HwTools::isSensorAddressEqual(uint8_t a[8], uint8_t b[8]) {
    for(int i = 0; i < 8; i++) {
        if(a[i] != b[i]) return false;
    }
    return true;
}

float HwTools::getTemperature() {
    uint8_t c = 0;
    float ret = 0;
    float analogTemp = getTemperatureAnalog();
    if(analogTemp != DEVICE_DISCONNECTED_C) {
        ret += analogTemp;
        c++;
    }
    for(int x = 0; x < sensorCount; x++) {
        TempSensorData data = *tempSensors[x];
        if(data.lastValidRead > -85) {
            ret += data.lastValidRead;
            c++;
        }
    }
    return c == 0 ? DEVICE_DISCONNECTED_C : ret/c;
}
float HwTools::getTemperatureAnalog() {
    if(atempPin != 0xFF) {
        float adcCalibrationFactor = 1.06587;
        int volts = ((float) analogRead(atempPin) / analogRange) * 3.3;
        return ((volts * adcCalibrationFactor) - 0.4) / 0.0195;
    }
    return DEVICE_DISCONNECTED_C;
}

int HwTools::getWifiRssi() {
    int rssi = WiFi.RSSI();
    return isnan(rssi) ? -100.0 : rssi;
}

void HwTools::setBootSuccessful(bool value) {
    if(bootSuccessful && value) return;
    bootSuccessful = value;
    if(ledDisablePin > 0 && ledDisablePin < 40) {
        switch(ledBehaviour) {
            case LED_BEHAVIOUR_ERROR_ONLY:
            case LED_BEHAVIOUR_OFF:
                digitalWrite(ledDisablePin, LOW);
                break;
            case LED_BEHAVIOUR_BOOT:
                if(bootSuccessful) {
                    digitalWrite(ledDisablePin, LOW);
                } else {
                    digitalWrite(ledDisablePin, HIGH);
                }
                break;
            default:
                digitalWrite(ledDisablePin, HIGH);
        }
    }
}

bool HwTools::ledOn(uint8_t color) {
    if(ledBehaviour == LED_BEHAVIOUR_OFF) return false;
    if(ledBehaviour == LED_BEHAVIOUR_ERROR_ONLY && color != LED_RED) return false;
    if(ledBehaviour == LED_BEHAVIOUR_BOOT && color != LED_RED && bootSuccessful) return false;

    if(color == LED_INTERNAL) {
        return writeLedPin(color, ledInvert ? LOW : HIGH);
    } else {
        return writeLedPin(color, rgbInvert ? LOW : HIGH);
    }
}

bool HwTools::ledOff(uint8_t color) {
    if(color == LED_INTERNAL) {
        return writeLedPin(color, ledInvert ? HIGH : LOW);
    } else {
        return writeLedPin(color, rgbInvert ? HIGH : LOW);
    }
}

bool HwTools::ledBlink(uint8_t color, uint8_t blink) {
    for(int i = 0; i < blink; i++) {
        if(!ledOn(color)) return false;
        delay(75);
        ledOff(color);
        if(i != blink) delay(75);
    }
    return true;
}

bool HwTools::writeLedPin(uint8_t color, uint8_t state) {
    switch(color) {
        case LED_INTERNAL: {
            if(ledPin != 0xFF) {
                digitalWrite(ledPin, state);
                return true;
            } else {
                return false;
            }
            break;
        }
        case LED_RED: {
            if(redPin != 0xFF) {
                digitalWrite(redPin, state);
                return true;
            } else {
                return false;
            }
            break;
        }
        case LED_GREEN: {
            if(greenPin != 0xFF) {
                digitalWrite(greenPin, state);
                return true;
            } else {
                return false;
            }
            break;
        }
        case LED_BLUE: {
            if(bluePin != 0xFF) {
                digitalWrite(bluePin, state);
                return true;
            } else {
                return false;
            }
            break;
        }
        case LED_YELLOW: {
            if(redPin != 0xFF && greenPin != 0xFF) {
                digitalWrite(redPin, state);
                digitalWrite(greenPin, state);
                return true;
            } else {
                return false;
            }
            break;
        }
    }
    return false;
}

bool HwTools::isVoltageOptimal(float range) {
	if(boardType >= 5 && boardType <= 7 && maxVcc > 2.8) { // Pow-*
		float vcc = getVcc();
		if(vcc > 3.4 || vcc < 2.8) {
			maxVcc = 0; // Voltage is outside the operating range, we have to assume voltage is OK
		} else if(vcc > maxVcc) {
			maxVcc = vcc;
		} else {
			float diff = min(maxVcc, (float) 3.3) - vcc;
            return diff < range;
		}
	}
	return true;
}

uint8_t HwTools::getBoardType() {
    return boardType;
}