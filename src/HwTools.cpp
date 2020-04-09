#include "HwTools.h"

double HwTools::getVcc() {
    double volts = 0.0;
#if defined(ARDUINO_ESP8266_WEMOS_D1MINI)
	volts = (((double) ESP.getVcc()) / 900.0); // This board has a voltage divider on VCC.
#elif defined(ARDUINO_LOLIN_D32)
    volts = (analogRead(GPIO_NUM_35) / 4095.0) * 3.3 * 2.25; // We are actually reading battery voltage here
#elif defined(ESP8266)
    volts = ((double) ESP.getVcc()) / 1024.0;
#endif

#if defined(ESP_VCC_CALIB_FACTOR)
    return volts * ESP_VCC_CALIB_FACTOR;
#else
    return volts;
#endif
}

double HwTools::getTemperature() {

#if defined TEMP_SENSOR_PIN
    if(!tempSensorInit) {
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
#endif
    return DEVICE_DISCONNECTED_C;
}

int HwTools::getWifiRssi() {
    int rssi = WiFi.RSSI();
    return isnan(rssi) ? -100.0 : rssi;
}