# HAN_ESP_TSS721

This is the current and most robust design for the [main board](HAN_ESP_TSS721). There's been great help in creating this, both here on GitHub, but also at the Norwegian home automation forum, [www.hjemmeautomasjon.no](https://www.hjemmeautomasjon.no/forums/topic/1982-lesing-av-ams-data-amshan-iot/)

The [design](./kicad) uses an [ESP8266](http://esp8266.net/) in an ESP12 module from [AI Thinker](https://www.ai-thinker.com) for micro processing and the [TSS721](http://www.ti.com/product/TSS721A) from Texas Instruments for level conversion from M-Bus to TTL (3.3V). An earlier design solved the level conversion using an op-amp, which might be an easier and more available solution, but also more error-prone.

The board

* uses TSS721 for MBus to TTL conversion
* holds ESP8266 for processing and transmit HAN data to WiFi / MQTT
* full schematics and PCB in editable [KiCad](http://www.kicad-pcb.org/) source files
* features a temperature sensor (DS18B20)
* uses a modular design, leaving these features optional
  * temperature sensor
  * M-bus TX
  * WiFi (Leave ESP, power supply and use for only M-bus to TTL)

Full schematics is available for this board in the kicad folder.

There is also a 3D printable enclosure in stl folder.

![The HAN Reader Hardware](img/HanReaderInEnclosure.PNG)

*The completed board mounted in a [3D printed enclosure](/Enclosure)*

![The HAN Reader Installed](img/HanReaderConnected.PNG)

*...installed with the electrical meter...*
