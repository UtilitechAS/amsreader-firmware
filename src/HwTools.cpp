#include "HwTools.h"

void HwTools::setTempSensorPin(int tempSensorPin) {
    if(tempSensorPin != this->tempSensorPin) {
        this->tempSensorInit = false;
        if(tempSensor)
            delete tempSensor;
        if(oneWire)
            delete oneWire;
        if(tempSensorPin > 0 && tempSensorPin < 40) {
            this->tempSensorPin = tempSensorPin;
            pinMode(tempSensorPin, INPUT);
        } else {
            this->tempSensorPin = 0xFF;
        }
    }
}

void HwTools::setVccPin(int vccPin) {
    if(vccPin > 0 && vccPin < 40) {
        pinMode(vccPin, INPUT);
        this->vccPin = vccPin;
    } else {
        this->vccPin = 0xFF;
    }
}

void HwTools::setVccMultiplier(double vccMultiplier) {
    this->vccMultiplier = vccMultiplier;
}

double HwTools::getVcc() {
    double volts = 0.0;
    if(vccPin != 0xFF) {
        #if defined(ESP8266)
            volts = (analogRead(vccPin) / 1024.0) * 3.3;
        #elif defined(ESP32)
            volts = (analogRead(vccPin) / 4095.0) * 3.3;
        #endif
    } else {
        #if defined(ESP8266)
            volts = ((double) ESP.getVcc()) / 1024.0;
        #endif
    }

    return volts > 0.0 ? volts * vccMultiplier : 0.0;
}

double HwTools::getTemperature() {
    if(tempSensorPin != 0xFF) {
        if(!tempSensorInit) {
            oneWire = new OneWire(tempSensorPin);
            tempSensor = new DallasTemperature(this->oneWire);
            tempSensor->begin();
            delay(50);
            tempSensor->requestTemperatures();
            hasTempSensor = tempSensor->getTempCByIndex(0) != DEVICE_DISCONNECTED_C;
            tempSensorInit = true;
        }

        if(hasTempSensor) {
            tempSensor->requestTemperatures();
            return tempSensor->getTempCByIndex(0);
        } else {
            return DEVICE_DISCONNECTED_C;
        }
    }

    return DEVICE_DISCONNECTED_C;
}

int HwTools::getWifiRssi() {
    int rssi = WiFi.RSSI();
    return isnan(rssi) ? -100.0 : rssi;
}

void HwTools::setLed(uint8_t ledPin, bool ledInverted) {
    if(ledPin > 0 && ledPin < 40) {
        this->ledPin = ledPin;
        this->ledInverted = ledInverted;
        pinMode(ledPin, OUTPUT);
        ledOff(LED_INTERNAL);
    } else {
        this->ledPin = 0xFF;
    }
}

void HwTools::setLedRgb(uint8_t ledPinRed, uint8_t ledPinGreen, uint8_t ledPinBlue, bool ledRgbInverted) {
    this->ledRgbInverted = ledRgbInverted;
    if(ledPinRed > 0 && ledPinRed < 40) {
        this->ledPinRed = ledPinRed;
        pinMode(ledPinRed, OUTPUT);
        ledOff(LED_RED);
    } else {
        this->ledPinRed = 0xFF;
    }
    if(ledPinGreen > 0 && ledPinGreen < 40) {
        this->ledPinGreen = ledPinGreen;
        pinMode(ledPinGreen, OUTPUT);
        ledOff(LED_GREEN);
    } else {
        this->ledPinGreen = 0xFF;
    }
    if(ledPinBlue > 0 && ledPinBlue < 40) {
        this->ledPinBlue = ledPinBlue;
        pinMode(ledPinBlue, OUTPUT);
        ledOff(LED_BLUE);
    } else {
        this->ledPinBlue = 0xFF;
    }
}

bool HwTools::ledOn(uint8_t color) {
    if(color == LED_INTERNAL) {
        return writeLedPin(color, ledInverted ? LOW : HIGH);
    } else {
        return writeLedPin(color, ledRgbInverted ? LOW : HIGH);
    }
}

bool HwTools::ledOff(uint8_t color) {
    if(color == LED_INTERNAL) {
        return writeLedPin(color, ledInverted ? HIGH : LOW);
    } else {
        return writeLedPin(color, ledRgbInverted ? HIGH : LOW);
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
        case LED_INTERNAL:
            if(ledPin != 0xFF) {
                digitalWrite(ledPin, state);
                return true;
            } else {
                return false;
            }
            break;
        case LED_RED:
            if(ledPinRed != 0xFF) {
                digitalWrite(ledPinRed, state);
                return true;
            } else {
                return false;
            }
            break;
        case LED_GREEN:
            if(ledPinGreen != 0xFF) {
                digitalWrite(ledPinGreen, state);
                return true;
            } else {
                return false;
            }
            break;
        case LED_BLUE:
            if(ledPinBlue != 0xFF) {
                digitalWrite(ledPinBlue, state);
                return true;
            } else {
                return false;
            }
            break;
        case LED_YELLOW:
            if(ledPinRed != 0xFF && ledPinGreen != 0xFF) {
                digitalWrite(ledPinRed, state);
                digitalWrite(ledPinGreen, state);
                return true;
            } else {
                return false;
            }
            break;
    }
    return false;
}
