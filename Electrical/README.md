
# Overview

Here are links to the different board designs together with a short summary.

## HAN_ESP_TSS721 (The current design)

This is the current and most robust design for the [main board](HAN_ESP_TS7221). There's been great help in creating this, both here on GitHub, but also at the Norwegian home automation forum, [www.hjemmeautomasjon.no](https://www.hjemmeautomasjon.no/forums/topic/1982-lesing-av-ams-data-amshan-iot/)

The board

* uses TSS721 for MBus to TTL conversion
* holds ESP8266 for processing and transmit HAN data to WiFi / MQTT
* full schematics and PCB in editable [KiCad](http://www.kicad-pcb.org/) source files
* features a temperature sensor (DS18B20)
* uses a modular design, leaving these features optional
  * temperature sensor
  * M-bus TX
  * WiFi (Leave ESP, power supply and use for only M-bus to TTL)

### Status

At the time of this writing, this has never been tried as a whole, but all parts has been prototyped individually. First PCB prototypes are very soon in the order.

## HAN_ESP_Simple (Was: Board 1)

The project's original [board design](HAN_ESP_Simple). It

* is based on the ESP8266 chip
* is powered by USB
* uses a very simple voltage divider to demodulate the M-bus signal
* has shematic and pcb design only available as finished pdf/png files

### Status

Prototypes have been made and some people have started using them(?).

## HAN_TTL_TSS721 (Was: Board 2)

This [board design](HAN_TTL_TSS721) is a newer alternative to the original. It

* is an Arduino shield.
* uses the industry standard TSS721 chip to interface the M-bus.
* is optically isolate
* has shematic and pcb design available in editable [KiCad](http://www.kicad-pcb.org/) source files

### Status

Unfinished, just started.

## MBUS_Simulator (Was: Board 3)

This [board](MBUS_Simulator) is a M-bus master simulator to be able to develop and
test the other boards without being dependent on having and using a real AMS unit.

### Status

Implementation done.

Please also see [Getting started building or modifying](GETTING_STARTED.md)
