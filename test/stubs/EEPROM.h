/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 * Native in-memory shim for <EEPROM.h>.
 */
#ifndef _NATIVE_EEPROM_H
#define _NATIVE_EEPROM_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

class EEPROMClass {
public:
    bool begin(size_t size) { return size <= data.size(); }
    bool commit() { return true; }

    uint8_t read(int address) const {
        return data.at(static_cast<size_t>(address));
    }

    template<typename T>
    T& get(int address, T& value) const {
        std::memcpy(&value, data.data() + address, sizeof(T));
        return value;
    }

    template<typename T>
    const T& put(int address, const T& value) {
        std::memcpy(data.data() + address, &value, sizeof(T));
        return value;
    }

    void reset() { data.fill(0); }

private:
    std::array<uint8_t, 4096> data = {};
};

inline EEPROMClass EEPROM;

#endif
