/*
 Name:		AmsToMqttBridge.ino
 Created:	3/13/2018 7:40:28 PM
 Author:	roarf
*/


#include "configuration.h"
#include "accesspoint.h"
#include <HanReader.h>

accesspoint ap;


// the setup function runs once when you press reset or power the board
void setup() 
{
	// Setup serial port for debugging
	Serial.begin(115200);
	while (!Serial);
	Serial.println("Started...");
	
	// Assign pin for boot as AP
	delay(1000);
	pinMode(0, INPUT_PULLUP);
	
	// Flash the blue LED, to indicate we can boot as AP now
	pinMode(2, OUTPUT);
	digitalWrite(2, LOW);
	
	// Initialize the AP
	ap.setup(0, Serial);
	
	// Turn off the blue LED
	digitalWrite(2, HIGH);
}

// the loop function runs over and over again until power down or reset
void loop() 
{
	if (!ap.loop())
	{
		// Only do normal stupp if we're not booted as AP
	}
}
