#include "EntsoeA44Parser.h"
#include "HardwareSerial.h"

EntsoeA44Parser::EntsoeA44Parser() {
    for(int i = 0; i < 24; i++) points[i] = ENTSOE_NO_VALUE;
}

EntsoeA44Parser::~EntsoeA44Parser() {

}

char* EntsoeA44Parser::getCurrency() {
    return currency;
}

char* EntsoeA44Parser::getMeasurementUnit() {
    return measurementUnit;
}

float EntsoeA44Parser::getPoint(uint8_t position) {
    if(position >= 24) return ENTSOE_NO_VALUE;
    return points[position];
}

int EntsoeA44Parser::available() {
    return 0;
}

int EntsoeA44Parser::read() {
    return 0;
}

int EntsoeA44Parser::peek() {
    return 0;
}

void EntsoeA44Parser::flush() {

}

size_t EntsoeA44Parser::write(const uint8_t *buffer, size_t size) {
    for(size_t i = 0; i < size; i++) {
        write(buffer[i]);
    }
    return size;
}

size_t EntsoeA44Parser::write(uint8_t byte) {
    if(pos >= 64) pos = 0;
    if(docPos == DOCPOS_CURRENCY) {
        buf[pos++] = byte;
        if(pos == 3) {
            buf[pos++] = '\0';
            memcpy(currency, buf, pos);
            docPos = DOCPOS_SEEK;
            pos = 0;
        }
    } else if(docPos == DOCPOS_MEASUREMENTUNIT) {
        buf[pos++] = byte;
        if(pos == 3) {
            buf[pos++] = '\0';
            memcpy(measurementUnit, buf, pos);
            docPos = DOCPOS_SEEK;
            pos = 0;
        }
    } else if(docPos == DOCPOS_POSITION) {
        if(byte == '<') {
            buf[pos] = '\0';
            pointNum = String(buf).toInt() - 1;
            docPos = DOCPOS_SEEK;
            pos = 0;
        } else {
            buf[pos++] = byte;
        }
    } else if(docPos == DOCPOS_AMOUNT) {
        if(byte == '<') {
            buf[pos] = '\0';
            points[pointNum] = String(buf).toFloat();
            docPos = DOCPOS_SEEK;
            pos = 0;
        } else {
            buf[pos++] = byte;
        }
    } else {
        if(pos == 0) {
            if(byte == '<') {
                buf[pos++] = byte;
            }
        } else if(byte == '>') {
            buf[pos++] = byte;
            buf[pos] = '\0';
            if(strcmp(buf, "<currency_Unit.name>") == 0) {
                docPos = DOCPOS_CURRENCY;
            } else if(strcmp(buf, "<price_Measure_Unit.name>") == 0) {
                docPos = DOCPOS_MEASUREMENTUNIT;
            } else if(strcmp(buf, "<position>") == 0) {
                docPos = DOCPOS_POSITION;
                pointNum = 0xFF;
            } else if(strcmp(buf, "<price.amount>") == 0) {
                docPos = DOCPOS_AMOUNT;
            }
            pos = 0;
        } else {
            buf[pos++] = byte;
        }
    }
    return 1;
}

void EntsoeA44Parser::get(PricesContainer* container) {
    strcpy(container->currency, currency);
    strcpy(container->measurementUnit, measurementUnit);

    container->points[0] = points[0] == ENTSOE_NO_VALUE ? ENTSOE_NO_VALUE : points[0] * 10000;
    container->points[1] = points[1] == ENTSOE_NO_VALUE ? ENTSOE_NO_VALUE : points[1] * 10000;
    container->points[2] = points[2] == ENTSOE_NO_VALUE ? ENTSOE_NO_VALUE : points[2] * 10000;
    container->points[3] = points[3] == ENTSOE_NO_VALUE ? ENTSOE_NO_VALUE : points[3] * 10000;
    container->points[4] = points[4] == ENTSOE_NO_VALUE ? ENTSOE_NO_VALUE : points[4] * 10000;
    container->points[5] = points[5] == ENTSOE_NO_VALUE ? ENTSOE_NO_VALUE : points[5] * 10000;
    container->points[6] = points[6] == ENTSOE_NO_VALUE ? ENTSOE_NO_VALUE : points[6] * 10000;
    container->points[7] = points[7] == ENTSOE_NO_VALUE ? ENTSOE_NO_VALUE : points[7] * 10000;
    container->points[8] = points[8] == ENTSOE_NO_VALUE ? ENTSOE_NO_VALUE : points[8] * 10000;
    container->points[9] = points[9] == ENTSOE_NO_VALUE ? ENTSOE_NO_VALUE : points[9] * 10000;

    container->points[10] = points[10] == ENTSOE_NO_VALUE ? ENTSOE_NO_VALUE : points[10] * 10000;
    container->points[11] = points[11] == ENTSOE_NO_VALUE ? ENTSOE_NO_VALUE : points[11] * 10000;
    container->points[12] = points[12] == ENTSOE_NO_VALUE ? ENTSOE_NO_VALUE : points[12] * 10000;
    container->points[13] = points[13] == ENTSOE_NO_VALUE ? ENTSOE_NO_VALUE : points[13] * 10000;
    container->points[14] = points[14] == ENTSOE_NO_VALUE ? ENTSOE_NO_VALUE : points[14] * 10000;
    container->points[15] = points[15] == ENTSOE_NO_VALUE ? ENTSOE_NO_VALUE : points[15] * 10000;
    container->points[16] = points[16] == ENTSOE_NO_VALUE ? ENTSOE_NO_VALUE : points[16] * 10000;
    container->points[17] = points[17] == ENTSOE_NO_VALUE ? ENTSOE_NO_VALUE : points[17] * 10000;
    container->points[18] = points[18] == ENTSOE_NO_VALUE ? ENTSOE_NO_VALUE : points[18] * 10000;
    container->points[19] = points[19] == ENTSOE_NO_VALUE ? ENTSOE_NO_VALUE : points[19] * 10000;

    container->points[20] = points[20] == ENTSOE_NO_VALUE ? ENTSOE_NO_VALUE : points[20] * 10000;
    container->points[21] = points[21] == ENTSOE_NO_VALUE ? ENTSOE_NO_VALUE : points[21] * 10000;
    container->points[22] = points[22] == ENTSOE_NO_VALUE ? ENTSOE_NO_VALUE : points[22] * 10000;
    container->points[23] = points[23] == ENTSOE_NO_VALUE ? ENTSOE_NO_VALUE : points[23] * 10000;
}