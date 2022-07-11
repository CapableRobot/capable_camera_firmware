#!/bin/bash

setarch linux32 ./libcamera-bridge --codec mjpeg --segment 0 -o /tmp/recording/pic_ --width 4056 --height 2160 --framerate 10 --tuning-file imx477.json --timeout 0
