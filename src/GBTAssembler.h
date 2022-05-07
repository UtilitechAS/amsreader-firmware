#ifndef _GBT_ASSEMBLER_H
#define _GBT_ASSEMBLER_H

#include <stdint.h>
#include "ams/hdlc.h"

typedef struct GBTHeader {
	uint8_t flag;
	uint8_t control;
	uint16_t sequence;
	uint16_t sequenceAck;
    uint8_t size;
} __attribute__((packed)) GBTHeader;

class GBTAssembler {
public:
    GBTAssembler();
    void init(const uint8_t* d, HDLCContext* context);
    int append(const uint8_t* d, int length, Print* debugger);
    uint16_t write(const uint8_t* d);

private:
    uint16_t pos = 0;
    uint8_t headersize = 0;
    uint8_t *buf;
    uint8_t lastSequenceNumber = 0;
};

#endif
