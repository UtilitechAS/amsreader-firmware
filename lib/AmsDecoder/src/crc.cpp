/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "crc.h"

uint16_t crc16_x25(const uint8_t* p, int len)
{
	uint16_t crc = UINT16_MAX;

	while(len--)
		for (uint16_t i = 0, d = 0xff & *p++; i < 8; i++, d >>= 1)
			crc = ((crc & 1) ^ (d & 1)) ? (crc >> 1) ^ 0x8408 : (crc >> 1);

	return (~crc << 8) | (~crc >> 8 & 0xff);
}

uint16_t crc16 (const uint8_t *p, int len) {
    uint16_t crc = 0;

    while (len--) {
		uint8_t i;
		crc ^= *p++;
		for (i = 0 ; i < 8 ; ++i) {
			if (crc & 1)
				crc = (crc >> 1) ^ 0xa001;
			else
				crc = (crc >> 1);
		}    			
    }
    
    return crc;
}

uint16_t crc16_1021(const uint8_t *p, int len)
{
	uint16_t crc = 0x0000;
	for(uint16_t i = 0; i < len; i++) {
		uint16_t mask = 0x80;
		while(mask > 0) {
			crc <<= 1;
			if (p[i] & mask) {
				crc |= 1;
			}
			mask>>=1;
			if (crc & 0x10000) {
				crc &= 0xffff;
				crc ^= 0x1021;
			}
		}
	}
	return crc;

}
