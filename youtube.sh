#!/bin/bash

if [ $# != 2 ] ; then
   echo "Please supply the window id (via xwininfo) and the youtube live login."
   exit
fi

youtube_auth1=$2
youtube_auth2=''

youtube_app='live2'
serverurl='rtmp://a.rtmp.youtube.com/'$youtube_app

width=854
height=480
audiorate=44100
channels=2
#framerate='30/1'
framerate='15/1'
vbitrate=1000
abitrate=128000
GST_DEBUG="--gst-debug=flvmux:0,rtmpsink:0"

###################### Settings ########################
########################################################
# THe following settings should not be changed
h264_level=4.1
h264_profile=main
h264_bframes=0
keyint=`echo "2 * $framerate" |bc`
datarate=`echo "$vbitrate + $abitrate / 1000" |bc`
flashver='FME/3.0%20(compatible;%20FMSc%201.0)'
akamai_flashver="flashver=FMLE/3.0(compatible;FMSc/1.0) playpath=I4Ckpath_12@44448"

########################################################
# This will detect gstreamer-1.0 over gstreamer-0.10
gstlaunch=`which gst-launch-1.0`
if 0; then # [ X$gstlaunch != X ] ; then
  VIDEOCONVERT=videoconvert
  VIDEO='video/x-raw, format=(string)BGRA, pixel-aspect-ratio=(fraction)1/1, interlace-mode=(string)progressive'
  AUDIO=audio/x-raw
  # VIDEO=video/x-raw-yuv
  vfid=string
  afid="format=(string)S16LE, "
else
  gstlaunch=`which gst-launch-0.10`
  if [ X$gstlaunch != X ] ; then
        VIDEOCONVERT=ffmpegcolorspace
        VIDEO='video/x-raw-rgb, bpp=(int)32, depth=(int)32, endianness=(int)4321, red_mask=(int)65280, green_mask=(int)16711680, blue_mask=(int)-16777216, pixel-aspect-ratio=(fraction)1/1, interlaced=(boolean)false'
        AUDIO=audio/x-raw-int
        vfid=fourcc
        afid=""
  else
        echo "Could not find gst-launch-1.0 or gst-launch-0.10. Stopping"
        exit
  fi
 fi

 auth="$youtube_auth1"
ENCAUDIOFORMAT='aacparse ! audio/mpeg,mpegversion=4,stream-format=raw'
videoencoder="x264enc bitrate=$vbitrate key-int-max=$keyint bframes=$h264_bframes byte-stream=false aud=true tune=zerolatency"
audioencoder="faac bitrate=$abitrate"
location=$serverurl'/x/'$auth'?videoKeyframeFrequency=1&totalDatarate='$datarate' app='$youtube_app' flashVer='$flashver' swfUrl='$serverurl

ENCVIDEOFORMAT='h264parse ! video/x-h264,level=(string)'$h264_level',profile='$h264_profile
VIDEOFORMAT=$VIDEO', framerate='$framerate', width='$width', height='$height
AUDIOFORMAT=$AUDIO', '$afid' endianness=(int)1234, signed=(boolean)true, width=(int)16, depth=(int)16, rate=(int)'$audiorate', channels=(int)'$channels
TIMEOLPARMS='halignment=left valignment=bottom text="Stream time:" shaded-background=true'
#VIDEOSRC="videotestsrc pattern=0 is-live=true ! timeoverlay $TIMEOLPARMS"
VIDEOSRC="ximagesrc name=videosource use-damage=false xid=$1 ! video/x-raw-rgb,framerate=15/1 ! videorate ! videoscale ! ffmpegcolorspace"
#AUDIOSRC="audiotestsrc is-live=true"
AUDIOSRC="jackaudiosrc connect=0 client-name=grabber ! audio/x-raw-float,rate=44100,channels=2 ! audioconvert ! audiorate"

    $gstlaunch -v $GST_DEBUG                         \
            $VIDEOSRC                               !\
            $VIDEOFORMAT                            !\
            queue                                   !\
            $VIDEOCONVERT                           !\
            $videoencoder                           !\
            $ENCVIDEOFORMAT                         !\
            queue                                   !\
            mux. $AUDIOSRC                          !\
            $AUDIOFORMAT                            !\
            queue                                   !\
            $audioencoder                           !\
            $ENCAUDIOFORMAT                         !\
            queue                                   !\
            flvmux streamable=true name=mux         !\
            queue                                   !\
            rtmpsink location="$location"

