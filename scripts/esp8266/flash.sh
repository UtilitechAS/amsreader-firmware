#!/bin/sh

if [ -z "$1" ];then
    echo "Usage: "
    echo "  Flashing first time            :   $0 flash /dev/ttyUSB0"
    echo "  When upgrading to new version  :   $0 upgrade /dev/ttyUSB0"
    echo "    NOTE: Replace /dev/ttyUSB0 with correct port"
    exit 1
fi

if [ -z "$2" ];then
    echo "Please specify port"
    exit 1
fi

esptool=`which esptool`
if [ -z "$esptool" ];then
    esptool=`which esptool.py`
fi
if [ -z "$esptool" ];then
    if [ -f esptool.py ];then
        esptool="esptool.py"
    fi
fi
if [ -z "$esptool" ];then
    echo "esptool.py not available to run following command: "
    esptool="echo esptool.py"
fi

$esptool --before default_reset --after hard_reset --chip esp8266 --port $2 --baud 115200 write_flash 0x0 firmware.bin
exit $?
