# Setup

1. Copy AmsToMqttBridge\Code\Arduino\HanReader\src to Arduino\libraries
2. Download the following libraries and put them in Arduino\libraries
   - ESP8266WiFi
   - PubSubClient
   - ArduinoJson
   
The device will boot as an AP (Access Point) the first time it starts.
Connect to the AP (ESP-AMS-MQTT) and browse to the Web page http://192.168.4.1/

Fill inn the form on the Web page and hit 'Submit'
The ESP will reboot and connect to your network, publishing it's Firmware version to the 'esp/ams/fw/version' topic.

The ESP can be reset trough the topic 'esp/ams/reset' or by pulling GPIO 13 LOW.

# Firmware Over The Air upgrade

The ESP now supports FOTA upgrades. Compile your Arduino sketch to binary (Sketch > Export compiled binary)
Upload the .bin file to a Web Server and create a version file.

Example: 
If your .bin file is named 'ams.bin', create a 'ams.version' in the same folder and write a version number different from the
version published on the 'esp/ams/fw/version' topic.

Then send the URL of the file, exluding the file ending, to the 'esp/ams/fw/update' topic like this:
`mosquitto_pub -t 'esp/ams/fw/update' -m 'http://your.local.webserver/ams'` if the bin and version file is in the root folder.

The ESP will publish upgrade information to the 'esp/ams/fw/info' topic.

# Output

All output is published on `esp/ams/#`