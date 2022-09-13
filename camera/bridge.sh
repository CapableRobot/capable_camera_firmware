#!/bin/bash

if [ ! -d "/tmp/recording/preview" ]
then
  mkdir /tmp/recording/pic
fi

if [ ! -d "/tmp/recording/pic" ]
then
  mkdir /tmp/recording/pic
fi

if [ ! -d "/mnt/data/pic" ]
then
  mkdir /mnt/data/pic
fi

./libcamera-bridge --config config.json --segment 0  --timeout 0 --tuning-file imx477.json --quality 70
