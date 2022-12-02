#include "crc.h"

uint16_t crc16_x25(const uint8_t* p, int len)
{
	uint16_t crc = UINT16_MAX;

	while(len--)
		for (uint16_t i = 0, d = 0xff & *p++; i < 8; i++, d >>= 1)
			crc = ((crc & 1) ^ (d & 1)) ? (crc >> 1) ^ 0x8408 : (crc >> 1);

	return (~crc << 8) | (~crc >> 8 & 0xff);
}
