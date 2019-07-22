## Arduino Code

FW and libraries for running on various HW

### AmsToMqttBridge

The original FW by roarfred, runs a Wifi access point until configured. Uploads data as json to a MQTT server.

### WifiFeatherRestBridge

_Note: This project is still under development._

Firmware for runninw AMS decoding with off-the-shelf components, so no PCB etching or soldering necessary. Specifially,
the sketch runs on the [Adafruit Feather M0 WiFi w/ATWINC1500](https://www.adafruit.com/product/3010) card and uses the
[TSS721 M-BUS module board](https://www.aliexpress.com/item/TSS721/32751482255.html) available from Aliexpress.

![FeatherMbus](/Debugging/Documentation/feather_3010-00_mbus_slave.jpg)


### Arduino Libraries/HanReader

The shared library to read from a serial port and decode the data into packets.

### Arduino Libraries/HanToJson

Shared library to create a json structure from the read HAN packets (for the different meter types).

