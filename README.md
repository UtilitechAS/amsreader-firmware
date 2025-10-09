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

The repository now defaults to the primary ESP32 firmware when you run `pio run`. If you need to build one of the alternative targets (ESP8266, ESP32-C3, ESP32-S2, ESP32-S3, etc.), pass the environment explicitly, for example `pio run -e esp8266` or `pio run -e esp32c3`.

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

### Shipping credentials with GitHub releases (without committing secrets)

The OTA manifest generated by `scripts/package_firmware.py` now carries an
optional `mqtt` block. If the build machine provides values for
`MQTT_DEFAULT_*` (through environment variables or a `.env` file), the script
embeds those defaults alongside the firmware checksum. Devices that upgrade via
GitHub Pages will download the manifest, detect the `mqtt` section, and apply
the broker settings automatically—unless the installer has already customised
the device through the web UI.

To keep secrets out of source control while still provisioning releases:

1. Store your broker credentials as GitHub Action secrets (for example
	`MQTT_DEFAULT_USERNAME`, `MQTT_DEFAULT_PASSWORD`, etc.).
2. In the release workflow, write a temporary `.env` file before invoking the
	PlatformIO build:

	```yaml
	- name: Write MQTT defaults
	  run: |
		 cat <<'EOF' > .env
		 MQTT_DEFAULT_HOST=${{ secrets.MQTT_DEFAULT_HOST }}
		 MQTT_DEFAULT_PORT=${{ secrets.MQTT_DEFAULT_PORT }}
		 MQTT_DEFAULT_USERNAME=${{ secrets.MQTT_DEFAULT_USERNAME }}
		 MQTT_DEFAULT_PASSWORD=${{ secrets.MQTT_DEFAULT_PASSWORD }}
		 MQTT_DEFAULT_CLIENT_ID=${{ secrets.MQTT_DEFAULT_CLIENT_ID }}
		 MQTT_DEFAULT_PUBLISH_TOPIC=${{ secrets.MQTT_DEFAULT_PUBLISH_TOPIC }}
		 MQTT_DEFAULT_SUBSCRIBE_TOPIC=${{ secrets.MQTT_DEFAULT_SUBSCRIBE_TOPIC }}
		 EOF
	```

3. Build the firmware and run `scripts/package_firmware.py` as usual; the
	generated `manifest.json` will include the broker defaults.
4. Upload `dist/` to GitHub Pages (the existing release workflow already covers
	this), so devices retrieving the manifest can bootstrap the MQTT connection
	immediately after flashing.

Because the `.env` file is created on-the-fly inside CI and never committed,
your credentials remain private while every release published to GitHub ships
with working MQTT settings out of the box.


# How to wipe bricked board?

To wipe the board you need to set it in USB mode. Connect the board to a usb board on you computor, hold AP/Prog button and shortly click reset before letting go of AP/Prog. To check if device is in usb mode you can check connections on this [site](https://www.amsleser.cloud/flasher)

When you have confirmed that board is in USB mode you need to figure out which port its connected to, then change to that port in the platformio.ini file under the function [env:esp32s2dev]

When that is complete, run 
```
pio run -t erase -e esp32s2dev
```

Then run:
```
pio run -t upload -e esp32s2
```

To finish it off, go to the flash-updater site [here](https://www.amsleser.cloud/flasher) and update to latest version to ensure a new firmware download

**Doing this will result in all data being deleted, and needing to setup the board from scratch**
