# AMS Reader
This code is designed to decode data from electric smart meters installed in many countries in Europe these days. The data is presented in a graphical web interface and can also send the data to a MQTT broker which makes it suitable for home automation project. Originally it was only designed to work with Norwegian meters, but has since been adapter to read any IEC-62056-7-5 or IEC-62056-21 compliant meters.

Later development have added Energy usage graph for both day and month, as well as future energy price. The code can run on any ESP8266 or ESP32 hardware which you can read more about in the [WiKi](https://github.com/UtilitechAS/amsreader-firmware/wiki). If you don't have the knowledge to set up a ESP device yourself, or you would like to support our work, please have a look at our shop at [amsleser.no](https://amsleser.no/).


<img src="images/dashboard.png">

Go to the [WiKi](https://github.com/UtilitechAS/amsreader-firmware/wiki) for information on how to get your own device! And find the latest prebuilt firmware file at the [release section](https://github.com/UtilitechAS/amsreader-firmware/releases).

## OTA updates from GitHub releases

The firmware now supports downloading updates straight from GitHub Pages using a
lightweight manifest file. Each time you push a tag like `v1.2.3`, the
`.github/workflows/release.yml` pipeline will:

1. Build every supported PlatformIO environment and publish `.bin`/`.zip`
	assets on the release page (existing behaviour).
2. Run `scripts/package_firmware.py` to assemble a static structure under
	`dist/firmware/<chip>/<channel>/` with the firmware binary, an MD5 checksum,
	and a `manifest.json` pointing at the binary.
3. Deploy the contents of `dist/` to GitHub Pages, yielding public URLs such as
	`https://<your-user>.github.io/neas-amsreader-firmware-test/firmware/esp32s2/stable/manifest.json`.

> ℹ️ Make sure GitHub Pages for the repository is configured to "GitHub
> Actions" under *Settings → Pages* the first time you run the workflow.

To make the device follow those releases, set the default OTA endpoint before
flashing:

```cpp
#define FIRMWARE_UPDATE_BASE_URL "https://<your-user>.github.io/neas-amsreader-firmware-test"
#define FIRMWARE_UPDATE_CHANNEL  "stable"
```

You can override the defaults either by editing
`lib/AmsFirmwareUpdater/include/UpgradeDefaults.h` or by adding corresponding
`-D` flags in `platformio.ini`. When a release is available, the device fetches
`manifest.json`, compares the `version` against its current firmware, and then
downloads the referenced binary in chunks.

If you need parallel release tracks (for example `beta` versus `stable`), pass
`--channel beta` to `package_firmware.py` inside your automation and override
`FIRMWARE_UPDATE_CHANNEL` for the devices you want on that track.

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


## MQTT auto-provisioning defaults
If you want devices to connect to a known MQTT broker immediately after flashing, keep credentials in a local `.env` file rather than committing them:

1. Copy `.env.example` to `.env` and fill in the MQTT values (host, port, username/password, client ID, topics, etc.).
2. Commit the `.env.example` changes only—`.env` is ignored so secrets stay local.
3. Build the firmware; the PlatformIO pre-build hook injects these values so the device boots with your broker settings.

Any field you leave empty will fall back to the defaults in `lib/AmsConfiguration/include/MqttDefaults.h`, meaning the web UI will prompt for credentials during first-time setup.