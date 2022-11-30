# Initial flash Instructions

Use [esptool.py](https://github.com/espressif/esptool) for flashing. Substitute the command with appropriate COM port.

##ESP8266
esptool.py --chip esp8266 --port <port> --baud 115200 --before default_reset --after hard_reset write_flash 0x0 firmware.bin Substitute <port> with correct COM port your device is connected to. Ex /dev/ttyUSB0 for Linux or COM1 for Windows

# Erase flash
If the device stops responding or unpredictable behaviour, a flash erase might be needed before flashing. Read more here
  
## Erase flash Command:
esptool.py --port <port> erase_flash
  
Substitute <port> with correct COM port your device is connected to. Ex /dev/ttyUSB0 for Linux or COM1 for Windows
