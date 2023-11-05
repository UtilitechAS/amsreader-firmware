#include "EntsoeA44Parser.h"
#include "HardwareSerial.h"

EntsoeA44Parser::EntsoeA44Parser() {
    for(int i = 0; i < 25; i++) points[i] = ENTSOE_NO_VALUE;
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
    if(position >= 25) return ENTSOE_NO_VALUE;
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
    memset(container, 0, sizeof(*container));

    strcpy(container->currency, currency);
    strcpy(container->measurementUnit, measurementUnit);
    strcpy(container->source, "EOE");

    for(uint8_t i = 0; i < 25; i++) {
        container->points[i] = points[i] == ENTSOE_NO_VALUE ? ENTSOE_NO_VALUE : points[i] * 10000;
    }
}