#!/bin/bash

gst-launch-0.10 jackaudiosrc connect=0 name=audiosource \
    ! audio/x-raw-float,rate=44100,channels=4,depth=16 \
    ! audioconvert \
    ! vorbisenc quality=0.9 \
    ! queue \
    ! oggmux name=mux ! shout2send ip=178.77.72.138 password=beansontoastyum mount=/quadlive.ogg
