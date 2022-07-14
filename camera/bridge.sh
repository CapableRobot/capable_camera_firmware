#!/bin/bash

if [ ! -d "/tmp/recording" ] 
then
    echo "Creating recording directory" 
    mkdir /tmp/recording
fi

./libcamera-bridge --maxusedspace 2147483648 --minfreespace 268435456 --codec mjpeg --segment 0 -o /tmp/recording/ --width 4056 --height 2160 --framerate 10 --tuning-file imx477.json --timeout 0
