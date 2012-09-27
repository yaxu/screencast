#!/bin/bash

# Takes four videos and arranges them in a four-way split screen.  Also
# takes stereo audio from two of the videos and does a stereo mix of them
# Output is in webm format. 
# Was used for this: http://www.youtube.com/watch?v=QOoj2a14Zeg

gst-launch-0.10 --gst-debug-level=2 \
    webmmux name=mux ! filesink location=newfile.webm \
    videomixer name=mix sink_1::xpos=614 sink_1::ypos=480 sink_2::xpos=640 sink_2::ypos=0 sink_3::xpos=0 sink_3::ypos=480 ! ffmpegcolorspace ! vp8enc ! queue ! mux.video_0 \
    \
    filesrc location=screena.avi ! decodebin2 name=screenb ! autoconvert ! videobox right=-640 bottom=-480 border-alpha=0 ! queue ! mix. \
    filesrc location=screenb.avi ! decodebin2 name=screena ! autoconvert ! queue ! mix. \
    \
    filesrc location=webcama.avi ! decodebin2  ! autoconvert ! videoflip method=horizontal-flip ! queue ! mix. \
    filesrc location=webcamb.avi ! decodebin2 ! autoconvert ! queue ! mix. \
    adder name=addtome ! vorbisenc ! queue ! mux.audio_0 \
    screenb. ! audioconvert ! audioresample ! audiorate ! addtome. \
    screena. ! audioconvert ! audioresample ! audiorate ! addtome.
