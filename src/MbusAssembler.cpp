#include "Arduino.h"
#include "MbusAssembler.h"
#include "ams/hdlc.h"

MbusAssembler::MbusAssembler() {
    buf = (uint8_t *)malloc((size_t)1024); // TODO find out from first package ?
}

uint8_t MbusAssembler::append(const uint8_t* d, int length) {
    MbusHeader* h = (MbusHeader*) d;
    uint8_t* ptr = (uint8_t*) &h[1];

    uint8_t len = h->len1;
    
    uint8_t control = *ptr;
    ptr++; len--;
    
    uint8_t address = *ptr;
    ptr++; len--;
    
    uint8_t ci = *ptr;
    ptr++; len--;
    
    uint8_t stsap = *ptr;
    ptr++; len--;
    
    uint8_t dtsap = *ptr;
    ptr++; len--;

    uint8_t sequenceNumber = ci & 0x0F;
    if(sequenceNumber == 0) {
        memcpy(buf, d, length - 2); // Do not include FCS and MBUS_STOP
        buf[6] = 0x10; // Mark that this is a single, complete frame
        pos = length - 2;
        lastSequenceNumber = 0;
        return 0;
    } else if(pos + len > 1024 || sequenceNumber != (lastSequenceNumber + 1)) { // TODO return error
        pos = 0;
        lastSequenceNumber = -1;
        return -1;
    } else {
        if(len > length) return -1;
        memcpy(buf + pos, ptr, len);
        pos += len;
        lastSequenceNumber = sequenceNumber;
        return 0;
    }
    return -2;
}

uint16_t MbusAssembler::write(const uint8_t* d) {
    buf[1] = buf[2] = 0x00;
    buf[pos++] = mbusChecksum(buf+4, pos-4);
    buf[pos++] = MBUS_END;
    memcpy((uint8_t *) d, buf, pos);
    return pos;
}
