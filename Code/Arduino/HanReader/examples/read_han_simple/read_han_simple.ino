/*
 * Simple sketch to read MBus data from electrical meter
 * As the protocol requires "Even" parity, and this is
 * only supported on the hardware port of the ESP8266,
 * we'll have to use Serial1 for debugging.
 * 
 * This means you'll have to program the ESP using the 
 * regular RX/TX port, and then you must remove the FTDI
 * and connect the MBus signal from the meter to the
 * RS pin. The FTDI/RX can be moved to Pin2 for debugging
 * 
 * Created 14. september 2017 by Roar Fredriksen
 */

#include "HanReader.h"

// The HAN Port reader
HanReader hanReader;

void setup() {
	setupDebugPort();
  
	// initialize the HanReader
	// (passing Serial as the HAN port and Serial1 for debugging)
	hanReader.setup(&Serial, &Serial1);
}

void setupDebugPort()
{
	// Initialize the Serial1 port for debugging
	// (This port is fixed to Pin2 of the ESP8266)
	Serial1.begin(115200);
	while (!Serial1) {}
	Serial1.setDebugOutput(true);
	Serial1.println("Serial1");
	Serial1.println("Serial debugging port initialized");
}

void loop() {
	// Read one byte from the port, and see if we got a full package
	if (hanReader.read())
	{
		// Get the list identifier
		List list = hanReader.getList();
		
		Serial1.println("");
		Serial1.print("List #");
		Serial1.print((byte)list, HEX);
		Serial1.print(": ");

		// Only care for the ACtive Power Imported, which is found in the first list
		if (list == List::List1)
		{
			int power = hanReader.getInt(List1_ObisObjects::ActivePowerImported);
      Serial1.print("Power consumtion is right now: ");
      Serial1.print(power);
      Serial1.println(" W");
		}
	}
}
