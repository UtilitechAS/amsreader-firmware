# AMS Reader
This code is designed to decode data from electric smart meters installed in many countries in Europe these days. The data is presented in a graphical web interface and can also send the data to a MQTT broker which makes it suitable for home automation project. Originally it was only designed to work with Norwegian meters, but has since been adapter to read any IEC-62056-7-5 or IEC-62056-21 compliant meters.

Later development have added Energy usage graph for both day and month, as well as future energy price. The code can run on any ESP8266 or ESP32 hardware which you can read more about in the [Wiki](https://wiki.amsleser.no/). If you don't have the knowledge to set up a ESP device yourself, or you would like to support our work, please have a look at our shop at [amsleser.no](https://www.amsleser.no/).


<img src="images/dashboard.png">

## Ready-made hardware

Pre-assembled readers running this firmware are available from [amsleser.no](https://www.amsleser.no/):

- **[Pow-K](https://www.amsleser.no/home/1-pow-k-han-port-reader-for-kamstrup-omnipower-meters.html)** – for Kamstrup Omnipower and 382M
- **[Pow-U](https://www.amsleser.no/home/7-pow-u-han-port-reader-for-m-bus-power-meters.html)** – for M-bus meters (HAN-NVE RJ45 in Norway/Sweden, RJ12 elsewhere)
- **[Pow-P1](https://www.amsleser.no/home/10-pow-p1-reader-for-power-meter-with-p1-port.html)** – for meters with P1 companion (DSMR) port

Not sure which one fits your meter? See the [Selection guide](https://www.amsleser.no/content/10-selection-guide).

## Installing pre-built firmware
If you have a device already running this firmware and you for some reason need to upgrade via USB port, you can use a [this web-based tool](https://www.amsleser.cloud/flasher)

If you are using a development board and want to flash a pre-built firmware manually, get the necessary files from the [release](https://github.com/UtilitechAS/amsreader-firmware/releases) section and visit the [WiKi](https://github.com/UtilitechAS/amsreader-firmware/wiki) and have a look at the [Flashing](   https://github.com/UtilitechAS/amsreader-firmware/wiki/flashing) section

## Building this project with PlatformIO
To build this project, you need [PlatformIO](https://platformio.org/) installed.

It is recommended to use Visual Studio Code with the PlatformIO plugin for development.

[Visual Studio Code](https://code.visualstudio.com/download)

[PlatformIO vscode plugin](https://platformio.org/install/ide?install=vscode)

For development purposes, copy the ```platformio-user.ini-example``` to ```platformio-user.ini``` and customize to your preference. The code will adapt to the platform and board set in your profile.

## Licensing
Initially, this project began as a hobby, consuming countless hours of our spare time. However, the time required to support this project has expanded beyond the scope of a hobby. As a result, we established ‘Utilitech’, a company dedicated to maintaining the software and hardware for this project as part of our regular work.

To ensure the sustainability of our venture, we have opted to license our software under the [Fair Source License] (https://fair.io). This approach allows the software to remain free for personal use, while also ensuring full transparency of our code’s inner workings. It also prevents competitors from exploiting our work without contributing to the maintenance of the code or providing technical support to end users.

For more information, please refer to our [LICENSE](/LICENSE) file.

If your usage falls outside the scope of this license and you require a separate license, please contact us at [post@utilitech.no](mailto:post@utilitech.no) for further details.
