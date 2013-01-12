
#include <string.h> /* for memset () */
#include <gst/gst.h>
#include "lo/lo.h"
#include <stdio.h>


GstElement *irc;

void error(int num, const char *msg, const char *path)
{
    printf("liblo server error %d in path %s: %s\n", num, path, msg);
}


int irc_handler(const char *path, const char *types, lo_arg **argv,
		     int argc, void *data, void *user_data) {
  char *str = strdup((char *) argv[0]);
  printf("got: %s\n", str);
  g_object_set (G_OBJECT (irc),
		"text", str,
		NULL);
  return 0;
}


gint
main (gint   argc,
      gchar *argv[])
{
  GstElement *pipeline, 
    *xsrc,
    *xcap,
    *videorate,
    *videoscale,
    *scalecap,
    *flt, *ffmpegconv, *oggmux, *shout, 
    *theoraenc, *vidqueue,
    *audiosource, *audiorate, *audioflt, *audioconv, *audioenc, *audioqueue
    ;

  GMainLoop *loop;

  gst_init (&argc, &argv);
  loop = g_main_loop_new (NULL, FALSE);

  pipeline = gst_pipeline_new ("pipeline");

  //video bit
  xsrc = gst_element_factory_make("ximagesrc", "xsrc");
  xcap = gst_element_factory_make ("capsfilter", "xcap");
  videorate = gst_element_factory_make ("videorate", "videorate");
  

  videoscale = gst_element_factory_make ("videoscale", "videoscale");
  irc = gst_element_factory_make ("textoverlay", "irc");
  scalecap = gst_element_factory_make ("capsfilter", "scalecap");

  ffmpegconv = gst_element_factory_make ("ffmpegcolorspace", "conv");
  flt = gst_element_factory_make ("capsfilter", "flt");
  theoraenc = gst_element_factory_make ("theoraenc", "theoraenc");
  vidqueue = gst_element_factory_make ("queue", "vidqueue");

  //audio bit
  audiosource = gst_element_factory_make ("jackaudiosrc", "audiosrc");
  audiorate = gst_element_factory_make("audiorate", "audiorate");
  audioflt = gst_element_factory_make ("capsfilter", "audioflt");
  audioconv = gst_element_factory_make ("audioconvert", "audioconv");
  audioenc = gst_element_factory_make ("vorbisenc", "audioenc");
  audioqueue = gst_element_factory_make ("queue", "audioqueue");
  
  //multiplexing and sending bit
  oggmux = gst_element_factory_make ("oggmux", "mux");
  shout = gst_element_factory_make ("shout2send", "shout");
  
  //configure

  g_object_set(G_OBJECT (audiosource),
               "connect", 0,
               "name", "lurkcast",
               NULL);
  
  g_object_set (G_OBJECT (xsrc),
		"use-damage", FALSE,
                "startx", 1,
                "starty", 1,
		"endx", 600,
		"endy", 480,
		NULL);

  g_object_set (G_OBJECT (xcap), "caps",
  		gst_caps_new_simple ("video/x-raw-rgb",
				     "framerate", GST_TYPE_FRACTION, 10, 1,
				     NULL), NULL);

  g_object_set (G_OBJECT (flt), "caps",
  		gst_caps_new_simple ("video/x-raw-yuv",
				     "framerate", GST_TYPE_FRACTION, 8, 1,
				     "width", G_TYPE_INT, 600,
				     "height", G_TYPE_INT, 480,
				     NULL), NULL);
  
  g_object_set (G_OBJECT (scalecap), "caps",
  		gst_caps_new_simple ("video/x-raw-yuv",
				     "width", G_TYPE_INT, 600,
				     "height", G_TYPE_INT, 480,
				     NULL), NULL);

  g_object_set (G_OBJECT (audioflt), "caps",
  		gst_caps_new_simple ("audio/x-raw-float",
				     "rate", G_TYPE_INT, 44100,
				     "channels", G_TYPE_INT, 2,
				     "depth", G_TYPE_INT, 16,
				     NULL), NULL);
  g_object_set (G_OBJECT (shout),
		"ip", "178.77.72.138",
		"password", "xxx",
		"mount", "/lurk.ogg",
		NULL);
  g_object_set (G_OBJECT (audioenc),
		"quality", 0.9,
		NULL);


  // add and link together
  gst_bin_add_many (GST_BIN (pipeline), 
		    xsrc, 
                    xcap,
                    videorate,
		    videoscale,
		    scalecap,
		    irc,
		    ffmpegconv, flt, theoraenc, 
		    vidqueue, oggmux, shout, 
		    audiosource, audiorate, audioflt, audioconv, audioenc, audioqueue,
		    NULL);
  gst_element_link_many (
			 xsrc, 
                         xcap,
			 ffmpegconv, 
                         videorate,
                         irc,
                         //videoscale,
                         //flt,
                         theoraenc, 
			 vidqueue, oggmux, shout, 
			 NULL);
   
  gst_element_link_many (audiosource, audiorate, audioflt, audioconv, audioenc, 
    			 audioqueue, oggmux,
                         NULL);
   

  /* play */
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  lo_server_thread osc = osc = lo_server_thread_new("7777", error);
  lo_server_thread_add_method(osc, "/text/irc", "s", irc_handler, NULL);
  lo_server_thread_start(osc);

  g_main_loop_run (loop);
   
  /* clean up */
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (pipeline));
   
  return 0;
}
