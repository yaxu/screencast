
#include <string.h> /* for memset () */
#include <gst/gst.h>
#include <gst/audio/multichannel.h>
#include <gst/audio/audio-enumtypes.h>
#include "lo/lo.h"
#include <stdio.h>

#define CHANNELS 8
/*GstAudioChannelPosition CHANNEL_POSITIONS[CHANNELS] = {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
        GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
        GST_AUDIO_CHANNEL_POSITION_LFE,
        GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT,
        GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT,
	GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_LEFT,
	GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_RIGHT,
	GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_CENTER,
	GST_AUDIO_CHANNEL_POSITION_TOP_CENTER,
	GST_AUDIO_CHANNEL_POSITION_TOP_REAR_LEFT,
	GST_AUDIO_CHANNEL_POSITION_TOP_REAR_RIGHT,
	GST_AUDIO_CHANNEL_POSITION_TOP_SIDE_LEFT,
	GST_AUDIO_CHANNEL_POSITION_TOP_SIDE_RIGHT
};
*/
#define FILE "test.ogg"

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
    *flt, *ffmpegconv, *oggmux, *out, 
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
#ifdef FILE
  out = gst_element_factory_make ("filesink", "filesink");
#else
  out = gst_element_factory_make ("shout2send", "shout");
#endif
  //configure

  g_object_set(G_OBJECT (audiosource),
               "connect", 0,
               "name", "lurkcast",
               NULL);
  
  g_object_set (G_OBJECT (xsrc),
		"use-damage", FALSE,
                "startx", 1,
                "starty", 31,
		"endx", 600,
		"endy", 510,
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

  /*
  GValue val;
  memset(&val, 0, sizeof(val));
  
  GValueArray *arr; // for channel position layout
  arr = g_value_array_new(CHANNELS);
  
  g_value_init(&val, GST_TYPE_AUDIO_CHANNEL_POSITION);
  
  for (int i = 0; i < CHANNELS; i++) {
    g_value_set_enum(&val, GST_AUDIO_CHANNEL_POSITION_NONE);
    g_value_array_append(arr, &val);
    g_value_reset(&val);
  }

  g_value_unset(&val);

  */

  g_object_set (G_OBJECT (audioflt), "caps",
  		gst_caps_new_simple ("audio/x-raw-float",
				     "rate", G_TYPE_INT, 44100,
				     "channels", G_TYPE_INT, CHANNELS,
				     "depth", G_TYPE_INT, 16,
				     //"channel-positions", arr,
				     NULL), NULL);
#ifdef FILE
  g_object_set (G_OBJECT (out),
		"location", FILE,
		NULL);
#else
  g_object_set (G_OBJECT (out),
		"ip", "lurk.org",
		"password", "xxx",
		"mount", "/lurk.ogg",
		NULL);
#endif

  g_object_set (G_OBJECT (audioenc),
		"quality", 0.3,
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
		    vidqueue, oggmux, out, 
		    audiosource, audiorate, audioflt, audioconv, audioenc, 
		    audioqueue,
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
			 vidqueue, oggmux, out, 
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
