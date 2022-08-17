#!/bin/bash

if [ ! -d "/tmp/recording" ] 
then
    echo "Creating recording directory" 
    mkdir /tmp/recording
fi

./libcamera-bridge --config config.json --segment 0  --timeout 0 --tuning-file imx477.json --quality 70
