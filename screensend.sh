#!/bin/bash

gst-launch-0.10 jackaudiosrc connect=0 name=audiosource \
    ! audio/x-raw-float,rate=44100,channels=2,depth=16 \
    ! audioconvert \
    ! vorbisenc quality=0.9 \
    ! queue \
    ! oggmux name=mux ! shout2send ip=yaxu.org password=xxx mount=/lurk.ogg \
    ximagesrc name=videosource use-damage=false startx=1 starty=1 endx=601 endy=481 \
    ! videorate ! videoscale ! video/x-raw-rgb,width=600,height=480,framerate=10/1 \
    ! queue \
    ! ffmpegcolorspace \
    ! theoraenc quality=40 \
    ! queue ! mux.
