#include "hexutils.h"

String toHex(uint8_t* in) {
	return toHex(in, sizeof(in)*2);
}

String toHex(uint8_t* in, uint16_t size) {
	String hex;
	for(int i = 0; i < size; i++) {
		if(in[i] < 0x10) {
			hex += '0';
		}
		hex += String(in[i], HEX);
	}
	hex.toUpperCase();
	return hex;
}

void fromHex(uint8_t *out, String in, uint16_t size) {
	for(int i = 0; i < size*2; i += 2) {
		out[i/2] = strtol(in.substring(i, i+2).c_str(), 0, 16);
	}
}

bool stripNonAscii(uint8_t* in, uint16_t size, bool extended) {
	bool ret = false;
	for(uint16_t i = 0; i < size; i++) {
		if(in[i] == 0) { // Clear the rest with null-terminator
			memset(in+i, 0, size-i);
			break;
		}
		if(extended && (in[i] < 32 || in[i] == 127 || in[i] == 129 || in[i] == 141 || in[i] == 143 || in[i] == 144 || in[i] == 157)) {
			memset(in+i, ' ', 1);
			ret = true;
		} else if(!extended && (in[i] < 32 || in[i] > 126)) {
			memset(in+i, ' ', 1);
			ret = true;
		}
	}
	memset(in+size-1, 0, 1); // Make sure the last character is null-terminator
	return ret;
}