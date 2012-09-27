
#include <string.h> /* for memset () */
#include <gst/gst.h>
#include "lo/lo.h"
#include <stdio.h>

#define OSC 1
//#define SILLY 1

GstElement *feedback;
GstElement *vocable;
GstElement *irc;

#ifdef OSC
void error(int num, const char *msg, const char *path)
{
    printf("liblo server error %d in path %s: %s\n", num, path, msg);
}


int feedback_handler(const char *path, const char *types, lo_arg **argv,
		     int argc, void *data, void *user_data) {
  char *str = strdup((char *) argv[0]);
  printf("got: %s\n", str);
  g_object_set (G_OBJECT (feedback),
		"text", str,
		NULL);
  return 0;
}

int vocable_handler(const char *path, const char *types, lo_arg **argv,
		     int argc, void *data, void *user_data) {
  char *str = strdup((char *) argv[0]);
  //printf("got: %s\n", str);
  g_object_set (G_OBJECT (vocable),
		"text", str,
		NULL);
  return 0;
}

int irc_handler(const char *path, const char *types, lo_arg **argv,
		     int argc, void *data, void *user_data) {
  char *str = strdup((char *) argv[0]);
  //printf("got: %s\n", str);
  g_object_set (G_OBJECT (irc),
		"text", str,
		NULL);
  return 0;
}

#endif

gint
main (gint   argc,
      gchar *argv[])
{
  GstElement *pipeline, 
#ifdef OSC
    *black,
    *videoscale,
    *scalecap,
#else
    *ximagesrc, 
#endif
    *flt, *ffmpegconv, *ffmpegconva, *oggmux, *shout, 
    *theoraenc, *vidqueue,
    *audiosource, *audioflt, *audioconv, *audioenc, *audioqueue
#ifdef SILLY
    , *effect
#endif
    ;

  GMainLoop *loop;

  gst_init (&argc, &argv);
  loop = g_main_loop_new (NULL, FALSE);

  pipeline = gst_pipeline_new ("pipeline");

  //video bit
#ifdef OSC
  black = gst_element_factory_make ("videotestsrc", "black");
  videoscale = gst_element_factory_make ("videoscale", "videoscale");
  feedback = gst_element_factory_make ("textoverlay", "feedback");
  vocable = gst_element_factory_make ("textoverlay", "vocable");
  irc = gst_element_factory_make ("textoverlay", "irc");
  scalecap = gst_element_factory_make ("capsfilter", "scalecap");
#ifdef SILLY
  effect = gst_element_factory_make ("shagadelictv", "shag");
#endif
#else
  ximagesrc = gst_element_factory_make ("ximagesrc", "source");
#endif

  ffmpegconv = gst_element_factory_make ("ffmpegcolorspace", "conv");
  ffmpegconva = gst_element_factory_make ("ffmpegcolorspace", "conva");
  flt = gst_element_factory_make ("capsfilter", "flt");
  theoraenc = gst_element_factory_make ("theoraenc", "theoraenc");
  vidqueue = gst_element_factory_make ("queue", "vidqueue");

  //audio bit
  audiosource = gst_element_factory_make ("alsasrc", "audiosrc");
  audioflt = gst_element_factory_make ("capsfilter", "audioflt");
  audioconv = gst_element_factory_make ("audioconvert", "audioconv");
  audioenc = gst_element_factory_make ("vorbisenc", "audioenc");
  audioqueue = gst_element_factory_make ("queue", "audioqueue");
  
  //multiplexing and sending bit
  oggmux = gst_element_factory_make ("oggmux", "mux");
  shout = gst_element_factory_make ("shout2send", "shout");
  
  //configure
  g_object_set (G_OBJECT (flt), "caps",
  		gst_caps_new_simple ("video/x-raw-yuv",
				     "framerate", GST_TYPE_FRACTION, 8, 1,
				     "width", G_TYPE_INT, 600,
				     "height", G_TYPE_INT, 450,
				     NULL), NULL);
  
#ifdef OSC
  g_object_set (G_OBJECT (black),
		"pattern", 2,
		NULL);
  g_object_set (G_OBJECT (scalecap), "caps",
  		gst_caps_new_simple ("video/x-raw-yuv",
				     "width", G_TYPE_INT, 400,
				     "height", G_TYPE_INT, 300,
				     NULL), NULL);

  g_object_set (G_OBJECT (feedback),
		"valign", "top",
		"halign", "left",
		"xpad", 5,
		"ypad", 35,
		"font-desc", "inconsolata,monospace 14",
		"line-alignment", 0,
		NULL);

  g_object_set (G_OBJECT (vocable),
		"valign", "top",
		"halign", "right",
		"xpad", 0,
		"ypad", 0,
		NULL);
#else
  g_object_set (G_OBJECT (ximagesrc),
		"use-damage", FALSE,
		"endx", 100,
		"endy", 100,
		NULL);
#endif
  
  g_object_set (G_OBJECT (audioflt), "caps",
  		gst_caps_new_simple ("audio/x-raw-int",
				     "rate", G_TYPE_INT, 22000,
				     "channels", G_TYPE_INT, 2,
				     "depth", G_TYPE_INT, 16,
				     NULL), NULL);
  g_object_set (G_OBJECT (shout),
		"ip", "94.228.66.94",
		"password", "xxx",
		"mount", "live",
		NULL);
  g_object_set (G_OBJECT (audioenc),
		"quality", 0.9,
		NULL);


  // add and link together
  gst_bin_add_many (GST_BIN (pipeline), 
#ifdef OSC
		    black, 
		    videoscale,
		    scalecap,
		    feedback,
		    vocable,
		    irc,
		    ffmpegconva,
#ifdef SILLY
		    effect,
#endif
#else
		    ximagesrc, 
#endif
		    
		    ffmpegconv, flt, theoraenc, 
		    vidqueue, oggmux, shout, 
		    audiosource, audioflt, audioconv, audioenc, audioqueue,
		    NULL);
  gst_element_link_many (
#ifdef OSC
			 black, 
			 feedback,
			 vocable,
			 irc,
			 ffmpegconva,
#ifdef SILLY
			 effect,
#endif
#else
			 ximagesrc, 
#endif
			 ffmpegconv, flt, theoraenc, 
			 vidqueue, oggmux, shout, 
			 NULL);
   
  gst_element_link_many (audiosource, audioflt, audioconv, audioenc, 
			 audioqueue, oggmux,
			 NULL);
   

  /* play */
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

#ifdef OSC
  lo_server_thread osc = osc = lo_server_thread_new("7777", error);
  lo_server_thread_add_method(osc, "/text/feedback", "s", feedback_handler, NULL);
  lo_server_thread_add_method(osc, "/text/vocable", "s", vocable_handler, NULL);
  lo_server_thread_add_method(osc, "/text/irc", "s", irc_handler, NULL);
  lo_server_thread_start(osc);
#endif

  g_main_loop_run (loop);
   
  /* clean up */
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (pipeline));
   
  return 0;
}
