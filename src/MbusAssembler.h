#ifndef _MBUS_ASSEMBLER_H
#define _MBUS_ASSEMBLER_H

#include <stdint.h>

class MbusAssembler {
public:
    MbusAssembler();
    uint8_t append(const uint8_t* d, int length);
    uint16_t write(const uint8_t* d);

private:
    uint16_t pos = 0;
    uint8_t *buf;
    uint8_t lastSequenceNumber = -1;
};

#endif
