#include <SoftwareSerial.h>

#define RX_PIN 10
#define TX_PIN 11

SoftwareSerial mySerial(RX_PIN, TX_PIN);

// https://en.wikipedia.org/wiki/Binary_Gray_sequence
static unsigned int binary_to_gray(unsigned int num)
{
    return num ^ (num >> 1);
}

void setup() {
	mySerial.begin(9600);
}

void loop() {
	static unsigned int i = 0;
	static unsigned int ch = 0;

	// Change the character more seldom than each loop entry.
	if (i++ > 20) {
		i = 0;
		// 127 => leave the last bit sent always zero.
		if (++ch > 127) {
			ch = 0;
		}
	}

	mySerial.write(binary_to_gray(ch));
	
	// 9600 bps => 8 bit = 8/9600s ~ 0.8ms
	delay(50);
}

