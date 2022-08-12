#ifndef _PRICESFROMHUBSTREAM_H
#define _PRICESFROMHUBSTREAM_H

#include "Stream.h"
#include "PricesContainer.h"

class PricesFromHubStream: public Stream {
public:
    int available();
    int read();
    int peek();
    void flush();
    void get(PricesContainer*);
    size_t write(const uint8_t *buffer, size_t size);
    size_t write(uint8_t);

    int pos = 0;
    uint8_t buf[512];
};

#endif
