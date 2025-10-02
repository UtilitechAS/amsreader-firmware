/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _ENTSOEA44PARSER_H
#define _ENTSOEA44PARSER_H

#include "Stream.h"
#include "PricesContainer.h"

#define DOCPOS_SEEK 0
#define DOCPOS_CURRENCY 1
#define DOCPOS_MEASUREMENTUNIT 2
#define DOCPOS_POSITION 3
#define DOCPOS_AMOUNT 4
#define DOCPOS_RESOLUTION 5

class EntsoeA44Parser: public Stream {
public:
    EntsoeA44Parser(PricesContainer *container);
    virtual ~EntsoeA44Parser();

    int available();
    int read();
    int peek();
    void flush();
    size_t write(const uint8_t *buffer, size_t size);
    size_t write(uint8_t);

private:
    PricesContainer *container;
    float multiplier = 1.0;

    char buf[64];
    uint8_t pos = 0;
    uint8_t docPos = 0;
    uint8_t pointNum = 0;
};

#endif
