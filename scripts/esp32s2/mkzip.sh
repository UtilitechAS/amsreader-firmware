#!/bin/sh

env="esp32s2"
build_dir=".pio/build/$env/"

if [ ! -d $build_dir ];then
    echo "No build directory"
    exit 1
fi

cp ~/.platformio/packages/framework-arduinoespressif32/tools/sdk/$env/bin/bootloader_qio_40m.bin $build_dir
cp ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin $build_dir
chmod +x scripts/$env/flash.sh
zip -j $env.zip $build_dir/*.bin scripts/$env/flash.sh
