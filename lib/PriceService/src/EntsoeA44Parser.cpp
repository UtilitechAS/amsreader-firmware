/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "EntsoeA44Parser.h"
#include "HardwareSerial.h"

EntsoeA44Parser::EntsoeA44Parser(PricesContainer *container) {
    this->container = container;
}

EntsoeA44Parser::~EntsoeA44Parser() {

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
            container->setCurrency(buf);
            docPos = DOCPOS_SEEK;
            pos = 0;
        }
    } else if(docPos == DOCPOS_MEASUREMENTUNIT) {
        buf[pos++] = byte;
        if(pos == 3) {
            buf[pos++] = '\0';
            if(strcmp_P(buf, PSTR("MWH"))) multiplier = 0.001;
            docPos = DOCPOS_SEEK;
            pos = 0;
        }
    } else if(docPos == DOCPOS_POSITION) {
        if(byte == '<') {
            buf[pos] = '\0';
            long pn = String(buf).toInt() - 1;
            if(pn < container->getNumberOfPoints()) {
                pointNum = pn;
            }
            docPos = DOCPOS_SEEK;
            pos = 0;
        } else {
            buf[pos++] = byte;
        }
    } else if(docPos == DOCPOS_AMOUNT) {
        if(byte == '<') {
            buf[pos] = '\0';
            float val = String(buf).toFloat();
            for(uint8_t i = pointNum; i < container->getNumberOfPoints(); i++) {
                container->setPrice(i, val * multiplier);
            }
            docPos = DOCPOS_SEEK;
            pos = 0;
        } else {
            buf[pos++] = byte;
        }
    } else if(docPos == DOCPOS_RESOLUTION) {
        if(byte == '<') {
            buf[pos] = '\0';

            // This happens if there are two time series in the XML. We are only interrested in the first one, so we ignore the rest of the document
            if(container->hasPrice(0)) return 1;

            if(strcmp_P(buf, PSTR("PT15M"))) {
                container->setup(15, 24);
            } else if(strcmp_P(buf, PSTR("PT60M"))) {
                container->setup(60, 24);
            }
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
            if(strcmp_P(buf, PSTR("<currency_Unit.name>")) == 0) {
                docPos = DOCPOS_CURRENCY;
            } else if(strcmp(buf, PSTR("<price_Measure_Unit.name>")) == 0) {
                docPos = DOCPOS_MEASUREMENTUNIT;
            } else if(strcmp(buf, PSTR("<position>")) == 0) {
                docPos = DOCPOS_POSITION;
                pointNum = 0xFF;
            } else if(strcmp(buf, PSTR("<price.amount>")) == 0) {
                docPos = DOCPOS_AMOUNT;
            } else if(strcmp(buf, PSTR("<resolution>")) == 0) {
                docPos = DOCPOS_RESOLUTION;
            }
            pos = 0;
        } else {
            buf[pos++] = byte;
        }
    }
    return 1;
}
