# Electrical Design

## Explained
This design uses the [Texas Instruments TS721](http://www.ti.com/product/TSS721A) circuit for the M-bus to TTL conversion. From here, the 3.3V TTL signal is taken to the [ESP-12](http://www.esp8266.com/wiki/doku.php?id=esp8266-module-family#esp-12) / [ESP8266](http://esp8266.net/) for further processing and reporting over WiFi to MQTT. Really, this hardware is agnostic to what you choose to report to, the HAN library will help you decode the serial signal into readable meter values.

An addition to this design is using a DS18B20 as a temperature sensor. Just a little added value in monitoring the temperature of your fuse box.

There is an on-board programming option for the ESP. In order to do programming, my experience is that you'll be better off disconnecting the RX/TX from the TSS721, so there's two jumpers for this.

Other than this, it's all about software. You can find the [Arduino code](../../Code) to decode the HAN data and report the values over WiFi to an MQTT server.


## Schematics
![Schematics](./images/schematics.png)

## PCB
![PCB](./images/PCB_3D.png)

### Layers
| Top Copper | Bottom Copper | Silk | Combined |
| ---------- | ------------- | ---- | -------- |
| ![Top Copper Layer](./images/HAN_ESP_TSS721-F.Cu.svg) | ![Bottom Copper Layer](./images/HAN_ESP_TSS721-B.Cu.svg) | ![Silk Layer](./images/HAN_ESP_TSS721-F.SilkS.svg) | ![Combined Layer](./images/HAN_ESP_TSS721-brd.svg) |


## Source Code
The full KiCad source for these design files are here in this folder. Download this repository, start KiCad, chose open project and select HAN_ESP_TSS721.pro to get going.
