#!/bin/bash

if [ $1 ]
then
    echo casting to $1
    gst-launch-0.10 avimux name=mux \
        ! filesink location=$1 \
        ximagesrc name=videosource use-damage=false startx=1 starty=1 endx=641 endy=481 \
        ! video/x-raw-rgb,framerate=10/1 \
        ! videorate \
        ! ffmpegcolorspace \
        ! videoscale method=1 \
        ! video/x-raw-yuv,width=640,height=480,framerate=10/1 \
        ! ffenc_huffyuv \
        ! queue \
        ! mux.
else
    echo But where to cast to?
fi
