#ifndef _DNBCURRPARSER_H
#define _DNBCURRPARSER_H

#include "Stream.h"

class DnbCurrParser: public Stream {
public:
    float getValue();
    
    int available();
    int read();
    int peek();
    void flush();
    size_t write(const uint8_t *buffer, size_t size);
    size_t write(uint8_t);

private:
    uint8_t scale = 0;
    float value = 1.0;

    char buf[128];
    uint8_t pos = 0;
    uint8_t mode = 0;
};

#endif
