#include "Arduino.h"
#include "GBTAssembler.h"
#include "ams/crc.h"

GBTAssembler::GBTAssembler() {
    buf = (uint8_t *)malloc((size_t)1024); // TODO find out from first package ?
}

void GBTAssembler::init(const uint8_t* d, HDLCContext* context) {
    memcpy(buf, d, context->headersize);
    pos = headersize = context->headersize;
    buf[pos++] = 0x00; // HCS
    buf[pos++] = 0x00; // HCS
    buf[pos++] = 0xE6;
    buf[pos++] = 0xE7;
    buf[pos++] = 0x00;
    lastSequenceNumber = 0;
}

int GBTAssembler::append(HDLCContext* context, const uint8_t* d, int length, Print* debugger) {
    GBTHeader* h = (GBTHeader*) (d+context->apduStart);
    h->sequence = ntohs(h->sequence);
    h->sequenceAck = ntohs(h->sequenceAck);

    //debugger->printf("F: %02X, C: %02X, S: %d, A: %d, L: %d, X: %d\n", h->flag, h->control, h->sequence, h->sequenceAck, h->size, lastSequenceNumber);
    if(h->flag != 0xE0) return -9;

    if(h->sequence == 1) {
        init(d, context);
    } else if(lastSequenceNumber != h->sequence-1) {
        return -1;
    }

    uint8_t* ptr = (uint8_t*) &h[1];
    memcpy(buf + pos, ptr, h->size);
    pos += h->size;
    lastSequenceNumber = h->sequence;
    return 0;
}

uint16_t GBTAssembler::write(const uint8_t* d) {
    uint16_t head = (0xA000) | pos+1;
    buf[1] = (head>>8) & 0xFF;
    buf[2] = head & 0xFF;
    uint16_t hcs = crc16_x25(buf+1, headersize-1);
    buf[headersize] = (hcs>>8) & 0xFF;
    buf[headersize+1] = hcs & 0xFF;

    uint16_t fcs = crc16_x25(buf+1, pos-1);
    buf[pos++] = (fcs>>8) & 0xFF;
    buf[pos++] = fcs & 0xFF;
    buf[pos++] = HDLC_FLAG;
    memcpy((uint8_t *) d, buf, pos);
    return pos;
}
