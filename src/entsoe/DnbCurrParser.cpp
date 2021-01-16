#include "DnbCurrParser.h"
#include "HardwareSerial.h"

float DnbCurrParser::getValue() {
    return value;
}

int DnbCurrParser::available() {

}

int DnbCurrParser::read() {

}

int DnbCurrParser::peek() {

}

void DnbCurrParser::flush() {

}

size_t DnbCurrParser::write(const uint8_t *buffer, size_t size) {
    for(int i = 0; i < size; i++) {
        write(buffer[i]);
    }
    return size;
}

size_t DnbCurrParser::write(uint8_t byte) {
    if(pos == 0) {
        if(byte == '<') {
            buf[pos++] = byte;
        }
    } else if(byte == '>') {
        buf[pos++] = byte;
        if(strncmp(buf, "<Obs", 4) == 0) {
            for(int i = 0; i < pos; i++) {
                if(strncmp(buf+i, "OBS_VALUE=\"", 11) == 0) {
                    pos = i + 11;
                    break;
                }
            }
            for(int i = 0; i < 16; i++) {
                uint8_t b = buf[pos+i];
                if(b == '"') {
                    buf[pos+i] = '\0';
                    break;
                }
            }
            value = String(buf+pos).toFloat();
        }
        pos = 0;
    } else {
        buf[pos++] = byte;
    }

    return 1;
}
