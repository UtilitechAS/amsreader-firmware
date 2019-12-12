# AMS <-> MQTT Bridge
Orignally designed and coded by [@roarfred](https://github.com/roarfred), see the original repo at [roarfred/AmsToMqttBridge](https://github.com/roarfred/AmsToMqttBridge)

This repository contains the code and schematics necessary to build a device to receive and convert data from AMS electrical meters installed in Norway. The code can be used on both ESP8266 and ESP32, both as custom build devices or built from readily available modules. It reads data from the HAN port of the meter and sends this to a configured MQTT bus.

## Release binaries

In the [Release section](https://github.com/gskjold/AmsToMqttBridge/releases) of this repository, you will find precompiled binaries for some common boards.

- _esp12e_ :: Compiled for a general ESP8266 board with 12E or 12F chip, ex NodeMCU board.
- _hw1esp12e_ :: Compiled for first version hardware with ESP 12E of 12F chip.

### Flashing binaries with [esptool.py](https://github.com/espressif/esptool)

Linux:
```esptool.py --port /dev/ttyUSB0 write_flash 0x0 binary-file.bin```

Windows:
```esptool.py --port COM1 write_flash 0x0 binary-file.bin```
