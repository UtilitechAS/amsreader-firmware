#include "DnbCurrParser.h"
#include "Arduino.h"

float DnbCurrParser::getValue() {
    return value;
}

int DnbCurrParser::available() {
    return 0;
}

int DnbCurrParser::read() {
    return 0;
}

int DnbCurrParser::peek() {
    return 0;
}

void DnbCurrParser::flush() {

}

size_t DnbCurrParser::write(const uint8_t *buffer, size_t size) {
    for(size_t i = 0; i < size; i++) {
        write(buffer[i]);
    }
    return size;
}

size_t DnbCurrParser::write(uint8_t byte) {
    if(pos >= 128) pos = 0;
    if(pos == 0) {
        if(byte == '<') {
            buf[pos++] = byte;
        }
    } else if(byte == '>') {
        buf[pos++] = byte;
        buf[pos++] = '\0';
        if(strncmp(buf, "<Series", 7) == 0) {
            for(int i = 0; i < pos; i++) {
                if(strncmp(buf+i, "UNIT_MULT=\"", 11) == 0) {
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
            scale = String(buf+pos).toInt();
        } else if(strncmp(buf, "<Obs", 4) == 0) {
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
            value = String(buf+pos).toFloat() / pow(10, scale);
        }
        pos = 0;
    } else {
        buf[pos++] = byte;
    }

    return 1;
}
