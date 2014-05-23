#!/bin/bash

if [ $1 ]
then
    echo casting to $1
    gst-launch-0.10 \
        ximagesrc name=videosource use-damage=false startx=1 starty=1 endx=801 endy=601 \
        ! videorate ! videoscale ! video/x-raw-rgb,width=800,height=600,framerate=10/1 \
        ! queue \
        ! ffmpegcolorspace \
        ! theoraenc quality=60 \
        ! queue \
        ! oggmux name=mux \
        ! filesink location=$1 \
	jackaudiosrc connect=0 client-name=grabber \
	! audio/x-raw-float,rate=44100,channels=2 \
	! audioconvert ! audiorate ! vorbisenc quality=0.9 \
	! queue ! mux.
else
    echo But where to cast to?
fi
