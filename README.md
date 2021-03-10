# AMS <-> MQTT Bridge
Orignally designed and coded by [@roarfred](https://github.com/roarfred), see the original repo at [roarfred/AmsToMqttBridge](https://github.com/roarfred/AmsToMqttBridge)

This repository contains the code and schematics necessary to build a device to receive and convert data from AMS electrical meters installed in Norway. The code can be used on both ESP8266 and ESP32, both as custom build devices or built from readily available development modules. It reads data from the HAN port of the meter and sends this to a configured MQTT bus.

There is a web interface available on runtime, showing meter data in real time.

<img src="webui.jpg">

## Hardware options
Look in [hardware section](/hardware) for more details about supported hardware

## Setting up your device
Go to the [WiKi](https://github.com/gskjold/AmsToMqttBridge/wiki) for information on how to set up your own device.

## Building this project with PlatformIO
To build this project, you need [PlatformIO](https://platformio.org/) installed.

It is recommended to use Visual Studio Code with the PlatformIO plugin for development.

[Visual Studio Code](https://code.visualstudio.com/download)

[PlatformIO vscode plugin](https://platformio.org/install/ide?install=vscode)

Copy the ```platformio-user.ini-example``` to ```platformio-user.ini``` and customize to your preference. The code will adapt to the platform and board set in your profile.
