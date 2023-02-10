#include "HwTools.h"

void HwTools::setup(GpioConfig* config, AmsConfiguration* amsConf) {
    this->config = config;
    this->amsConf = amsConf;
    this->tempSensorInit = false;
    if(sensorApi != NULL)
        delete sensorApi;
    if(oneWire != NULL)
        delete oneWire;
    if(config->tempSensorPin > 0 && config->tempSensorPin < 40) {
        pinMode(config->tempSensorPin, INPUT);
    } else {
        config->tempSensorPin = 0xFF;
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
    } else {
        voltAdc.unit = 0xFF;
        voltAdc.channel = 0xFF;
        config->vccPin = 0xFF;
    }

    if(config->tempAnalogSensorPin > 0 && config->tempAnalogSensorPin < 40) {
        pinMode(config->tempAnalogSensorPin, INPUT);
    } else {
        config->tempAnalogSensorPin = 0xFF;
    }

    if(config->ledPin > 0 && config->ledPin < 40) {
        pinMode(config->ledPin, OUTPUT);
        ledOff(LED_INTERNAL);
    } else {
        config->ledPin = 0xFF;
    }

    if(config->ledPinRed > 0 && config->ledPinRed < 40) {
        pinMode(config->ledPinRed, OUTPUT);
        ledOff(LED_RED);
    } else {
        config->ledPinRed = 0xFF;
    }

    if(config->ledPinGreen > 0 && config->ledPinGreen < 40) {
        pinMode(config->ledPinGreen, OUTPUT);
        ledOff(LED_GREEN);
    } else {
        config->ledPinGreen = 0xFF;
    }

    if(config->ledPinBlue > 0 && config->ledPinBlue < 40) {
        pinMode(config->ledPinBlue, OUTPUT);
        ledOff(LED_BLUE);
    } else {
        config->ledPinBlue = 0xFF;
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

double HwTools::getVcc() {
    double volts = 0.0;
    if(config->vccPin != 0xFF) {
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
                    x += analogRead(config->vccPin);
                }
                volts = (x * 3.3) / 10.0 / analogRange;
            }
        #else
            uint32_t x = 0;
            for (int i = 0; i < 10; i++) {
                x += analogRead(config->vccPin);
            }
            volts = (x * 3.3) / 10.0 / analogRange;
        #endif
    } else {
        #if defined(ESP8266)
            volts = ESP.getVcc() / 1024.0;
        #endif
    }
    if(volts == 0.0) return 0.0;

    if(config->vccResistorGnd > 0 && config->vccResistorVcc > 0) {
        volts *= ((double) (config->vccResistorGnd + config->vccResistorVcc) / config->vccResistorGnd);
    }


    float vccOffset = config->vccOffset / 100.0;
    float vccMultiplier = config->vccMultiplier / 1000.0;
    return vccOffset + (volts > 0.0 ? volts * vccMultiplier : 0.0);
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
    if(config->tempSensorPin != 0xFF) {
        if(!tempSensorInit) {
            oneWire = new OneWire(config->tempSensorPin);
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
                delay(10);
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

double HwTools::getTemperature() {
    uint8_t c = 0;
    double ret = 0;
    double analogTemp = getTemperatureAnalog();
    if(analogTemp != DEVICE_DISCONNECTED_C) {
        ret += analogTemp;
        c++;
    }
    for(int x = 0; x < sensorCount; x++) {
        TempSensorData data = *tempSensors[x];
        TempSensorConfig* conf = amsConf->getTempSensorConfig(data.address);
        if((conf == NULL || conf->common) && data.lastValidRead > -85) {
            ret += data.lastValidRead;
            c++;
        }
    }
    return c == 0 ? DEVICE_DISCONNECTED_C : ret/c;
}
double HwTools::getTemperatureAnalog() {
    if(config->tempAnalogSensorPin != 0xFF) {
        float adcCalibrationFactor = 1.06587;
        int volts = ((double) analogRead(config->tempAnalogSensorPin) / analogRange) * 3.3;
        return ((volts * adcCalibrationFactor) - 0.4) / 0.0195;
    }
    return DEVICE_DISCONNECTED_C;
}

int HwTools::getWifiRssi() {
    int rssi = WiFi.RSSI();
    return isnan(rssi) ? -100.0 : rssi;
}

bool HwTools::ledOn(uint8_t color) {
    if(color == LED_INTERNAL) {
        return writeLedPin(color, config->ledInverted ? LOW : HIGH);
    } else {
        return writeLedPin(color, config->ledRgbInverted ? LOW : HIGH);
    }
}

bool HwTools::ledOff(uint8_t color) {
    if(color == LED_INTERNAL) {
        return writeLedPin(color, config->ledInverted ? HIGH : LOW);
    } else {
        return writeLedPin(color, config->ledRgbInverted ? HIGH : LOW);
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
            if(config->ledPin != 0xFF) {
                digitalWrite(config->ledPin, state);
                return true;
            } else {
                return false;
            }
            break;
        }
        case LED_RED: {
            if(config->ledPinRed != 0xFF) {
                digitalWrite(config->ledPinRed, state);
                return true;
            } else {
                return false;
            }
            break;
        }
        case LED_GREEN: {
            if(config->ledPinGreen != 0xFF) {
                digitalWrite(config->ledPinGreen, state);
                return true;
            } else {
                return false;
            }
            break;
        }
        case LED_BLUE: {
            if(config->ledPinBlue != 0xFF) {
                digitalWrite(config->ledPinBlue, state);
                return true;
            } else {
                return false;
            }
            break;
        }
        case LED_YELLOW: {
            if(config->ledPinRed != 0xFF && config->ledPinGreen != 0xFF) {
                digitalWrite(config->ledPinRed, state);
                digitalWrite(config->ledPinGreen, state);
                return true;
            } else {
                return false;
            }
            break;
        }
    }
    return false;
}
