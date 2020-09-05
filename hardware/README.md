# Hardware options

There are currently two possible hardware options for this project, both in need of external power supply. A self powered board is under development by a member of the community and is currently being tested. 

## Hardware v1 by [@roarfred](https://github.com/roarfred)
Composed from a ESP12E (or F) chip, this ESP8266 based board is designed specifically for this project with an on board M-bus chip.

Building this project will require some skills in ordering and assembling electronic circuits as well as programming. No detailed instructions are available.

![The HAN Reader Hardware](v1/img/HanReaderInEnclosure.PNG)

*The completed board mounted in a [3D printed enclosure](/Enclosure)*

## HAN reader 2.0 by [@dakarym](https://github.com/dakarym)
A board that does not require external power source. This have only been successfully tested on Aidon as far as I know. It draws too much power for Kamstrup, but it may work with Kaifa. The design is almost 
completely built with SMD components, so advanced soldering skills are required to make this one.

[View his design here](https://github.com/dakarym/AmsToMqttBridge/tree/master/PCB)

## Assembly of readily available modules
You can also use a ESP based development board and combine this with a M-Bus module. Here are a few boards that have been tested, each one has a dedicated firmware file in the releases section. 

### ESP8266 based boards

[Wemos D1 mini](https://docs.wemos.cc/en/latest/d1/d1_mini.html)
- M-Bus connected to GPIO5 (D1)
- Jump GPIO4 (D2) to GND to force AP mode during boot
- Dallas temp sensor connected to GPIO14 (D5)

### ESP32 based boards

[Wemos Lolin D32](https://docs.wemos.cc/en/latest/d32/d32.html) 
- M-Bus connected to GPIO16
- Jump GPIO4 to GND to force AP mode during boot
- Dallas temp sensor connected to GPIO14

[Adafruit HUZZAH32](https://www.adafruit.com/product/3405) 
- M-Bus connected to GPIO16


Combine one of above board with an M-Bus module. Connect 3.3v and GND together between the boards and connect the TX pin from the M-Bus board to the dedicated M-Bus pin on the ESP board.

[TSS721 M-BUS module board](https://www.aliexpress.com/item/TSS721/32751482255.html)

![FeatherMbus](img/feather_3010-00_mbus_slave.jpg)
