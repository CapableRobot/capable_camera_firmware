#!/bin/bash

emmcSpace=df | grep /mnt/data | awk '{print $2}'
minSpace=1000000
if (( $emmcSpace > $minSpace )) 
then
    echo "Using new emmc based config"
    if [ ! -d "/mnt/data/pic" ]
    then
      mkdir /mnt/data/pic
    fi
    cp emmc_config.json config.json
fi

./libcamera-bridge --config config.json --segment 0  --timeout 0 --tuning-file imx477.json --quality 70
