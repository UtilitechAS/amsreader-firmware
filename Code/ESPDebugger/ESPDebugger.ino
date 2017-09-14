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


byte buffer[512];
int bytesRead;

void setup() {
  // Initialize the Serial1 port for debugging
  // (This port is fixed to Pin2 of the ESP8266)
  Serial1.begin(115200);
  while (!Serial1) {}
  Serial1.setDebugOutput(true);
  Serial1.println("Serial1");
  Serial1.println("Serial debugging port initialized");

  // Initialize H/W serial port for MBus communication
  Serial.begin(2400, SERIAL_8E1);
  while (!Serial) {}
  Serial1.println("MBUS serial setup complete");

  bytesRead = 0;
}

void loop() {
  if (Serial.available())
  {
    byte newByte = Serial.read();
    buffer[bytesRead++] = newByte;
    bool completed = bytesRead > 1 && newByte == 0x7E;
        
    if (completed)
      writeAndEmptyBuffer();
  }
}


void writeAndEmptyBuffer()
{
  for (int i = 0; i < bytesRead; i++)
  {
    if (buffer[i] < 0x10) Serial1.print("0");
    Serial1.print(buffer[i], HEX);
    Serial1.print(" ");
  }
  Serial1.println();
  bytesRead = 0;
}
