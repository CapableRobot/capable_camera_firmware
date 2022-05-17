#!/bin/bash

trap "i2cset -y 1 0x14 0xc 0x00 b" EXIT

while :  
do  
sleep 1
value=$(i2cget -y 1 0x14 0xf b)
if [ "$value" == "0xff" ]  
then
    echo $value
    echo "GPS Locked"
    break
fi
done  

i2cset -y 1 0x14 0xc 0xFF b
setarch linux32 ./libcamera-bridge --codec mjpeg --segment 0 -o sck:///tmp/bridge.sock --width 4056 --height 2016 --framerate 10 \
--awb normal --awbgains 1.0e+00,1.0e+00 --brightness 0 --contrast 0 --exposure normal --ev 0 --gain 0 --metering centre --saturation 0 --sharpness 0 --tuning-file imx477.json --timeout 0;