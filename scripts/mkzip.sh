#!/bin/sh

find -name firmware.bin | while read firmware;do
    dir=`dirname $firmware`
    env=`basename $dir`
    if [ -f scripts/$env/mkzip.sh ];then
        echo "Building zip for env '$env'"
        chmod +x scripts/$env/mkzip.sh 
        scripts/$env/mkzip.sh
    fi
done