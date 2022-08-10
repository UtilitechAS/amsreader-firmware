#include "PricesFromHubStream.h"

int PricesFromHubStream::available() {
    return 0;
}

int PricesFromHubStream::read() {
    return 0;
}

int PricesFromHubStream::peek() {
    return 0;
}

void PricesFromHubStream::flush() {

}

size_t PricesFromHubStream::write(const uint8_t *buffer, size_t size) {
    for(int i = 0; i < size; i++) {
        write(buffer[i]);
    }
    return size;
}

size_t PricesFromHubStream::write(uint8_t b) {
    buf[pos++] = b;
    return 1;
}
