/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "PassthroughMqttHandler.h"
#include "hexutils.h"

bool PassthroughMqttHandler::publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea, PriceService* ps) {
    return false;
}

bool PassthroughMqttHandler::publishTemperatures(AmsConfiguration*, HwTools*) {
    return false;
}

bool PassthroughMqttHandler::publishPrices(PriceService*) {
    return false;
}

bool PassthroughMqttHandler::publishSystem(HwTools* hw, PriceService* ps, EnergyAccounting* ea) {
    return false;
}

bool PassthroughMqttHandler::publishBytes(uint8_t* buf, uint16_t len) {
    mqtt.publish(topic.c_str(), toHex(buf, len));
    bool ret = mqtt.loop();
    delay(10);
    return ret;
}

bool PassthroughMqttHandler::publishString(char* str) {
    mqtt.publish(topic.c_str(), str);
    bool ret = mqtt.loop();
    delay(10);
    return ret;
}

uint8_t PassthroughMqttHandler::getFormat() {
    return 255;
}