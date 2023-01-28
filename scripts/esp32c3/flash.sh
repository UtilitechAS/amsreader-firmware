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

if [ "$1" = "flash" ];then
    $esptool --chip esp32c3 --port $2 --baud 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect \
    0x1000 bootloader_qio_40m.bin \
    0x8000 partitions.bin \
    0xe000 boot_app0.bin \
    0x10000 firmware.bin
    exit $?
elif [ "$1" = "upgrade" ];then
    $esptool --chip esp32c3 --port $2 --baud 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 0x10000 firmware.bin
    exit $?
fi
