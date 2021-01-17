#include "HwTools.h"

void HwTools::setup(GpioConfig* config, AmsConfiguration* amsConf) {
    this->config = config;
    this->amsConf = amsConf;
    this->tempSensorInit = false;
    if(this->tempSensors == NULL)
        this->tempSensors = new TempSensorData*[32];
    if(sensorApi != NULL)
        delete sensorApi;
    if(oneWire != NULL)
        delete oneWire;
    if(config->tempSensorPin > 0 && config->tempSensorPin < 40) {
        pinMode(config->tempSensorPin, INPUT);
    } else {
        config->tempSensorPin = 0xFF;
    }

    if(config->tempAnalogSensorPin > 0 && config->tempAnalogSensorPin < 40) {
        pinMode(config->tempAnalogSensorPin, INPUT);
    } else {
        config->tempAnalogSensorPin = 0xFF;
    }

    if(config->vccPin > 0 && config->vccPin < 40) {
        pinMode(config->vccPin, INPUT);
    } else {
        config->vccPin = 0xFF;
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

double HwTools::getVcc() {
    double volts = 0.0;
    if(config->vccPin != 0xFF) {
        #if defined(ESP8266)
            volts = (analogRead(config->vccPin) / 1024.0) * 3.3;
        #elif defined(ESP32)
            volts = (analogRead(config->vccPin) / 4095.0) * 3.3;
        #endif
    } else {
        #if defined(ESP8266)
            volts = ((double) ESP.getVcc()) / 1024.0;
        #endif
    }

    float vccOffset = config->vccOffset / 100.0;
    float vccMultiplier = config->vccMultiplier / 1000.0;
    return vccOffset + (volts > 0.0 ? volts * vccMultiplier : 0.0);
}

uint8_t HwTools::getTempSensorCount() {
    return sensorCount;
}

TempSensorData* HwTools::getTempSensorData(uint8_t i) {
    return tempSensors[i];
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
        int volts;
        #if defined(ESP8266)
            volts = (analogRead(config->tempAnalogSensorPin) / 1024.0) * 3.3;
        #elif defined(ESP32)
            volts = (analogRead(config->tempAnalogSensorPin) / 4095.0) * 3.3;
        #endif
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
        delay(50);
        ledOff(color);
        if(i != blink)
            delay(50);
    }
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
