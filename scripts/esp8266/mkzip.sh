#!/bin/sh

env="esp8266"
build_dir=".pio/build/$env/"

if [ ! -d $build_dir ];then
    echo "No build directory"
    exit 1
fi

chmod +x scripts/$env/flash.sh
zip -j $env.zip $build_dir/*.bin scripts/$env/flash.sh
