#ifndef _ENTSOEA44PARSER_H
#define _ENTSOEA44PARSER_H

#include "Stream.h"
#include "PricesContainer.h"

#define DOCPOS_SEEK 0
#define DOCPOS_CURRENCY 1
#define DOCPOS_MEASUREMENTUNIT 2
#define DOCPOS_POSITION 3
#define DOCPOS_AMOUNT 4

#define ENTSOE_NO_VALUE -127

class EntsoeA44Parser: public Stream {
public:
    EntsoeA44Parser();
    virtual ~EntsoeA44Parser();

    char* getCurrency();
    char* getMeasurementUnit();
    float getPoint(uint8_t position);
    
    int available();
    int read();
    int peek();
    void flush();
    size_t write(const uint8_t *buffer, size_t size);
    size_t write(uint8_t);
    void get(PricesContainer*);

private:
    char currency[4];
    char measurementUnit[4];
    float points[25];

    char buf[64];
    uint8_t pos = 0;
    uint8_t docPos = 0;
    uint8_t pointNum = 0;
};

#endif
