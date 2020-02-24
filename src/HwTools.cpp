#include "HwTools.h"

double HwTools::getVcc() {
#if defined(ARDUINO_ESP8266_WEMOS_D1MINI)
	return (((double) ESP.getVcc()) / 900); // This board has a voltage divider on VCC. Yes, 900 is correct
#elif defined(ESP8266)
    #if defined(ESP_VCC_CALIB_FACTOR)
        return ((double) ESP.getVcc()) / 1024 * ESP_VCC_CALIB_FACTOR;
    #else
	    return ((double) ESP.getVcc()) / 1024;
    #endif
#endif
	return -1;
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