
#include <string.h> /* for memset () */
#include <gst/gst.h>
#include "lo/lo.h"
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#define OSC 1

#define PLAYBACK "file:///old/screen-1302870084.avi"

static GMainLoop *loop;

GstElement *screenpipe;
GstElement *webcampipe;
#ifdef PLAYBACK
GstElement *playbackpipe;
#endif

#ifdef OSC
void error(int num, const char *msg, const char *path) {
    printf("liblo server error %d in path %s: %s\n", num, path, msg);
}

int start_handler(const char *path, const char *types, lo_arg **argv,
                  int argc, void *data, void *user_data) {
  static int played = 0;
  if (!played) {
    gst_element_set_state(screenpipe, GST_STATE_PLAYING);
    gst_element_set_state(webcampipe, GST_STATE_PLAYING);
#ifdef PLAYBACK
    usleep(50000);
    gst_element_set_state(playbackpipe, GST_STATE_PLAYING);
#endif
    played = 1;
  }
  return 0;
}

int stop_handler(const char *path, const char *types, lo_arg **argv,
                  int argc, void *data, void *user_data) {
  g_message("End-of-stream");
  g_main_loop_quit(loop);
  return 0;
}

#endif


static gboolean bus_call(GstBus *bus, GstMessage *msg, void *user_data)
{
	switch (GST_MESSAGE_TYPE(msg)) {
	case GST_MESSAGE_EOS: {
		g_message("End-of-stream");
		g_main_loop_quit(loop);
		break;
	}
	case GST_MESSAGE_ERROR: {
		GError *err;
		gst_message_parse_error(msg, &err, NULL);
		g_error("%s", err->message);
		g_error_free(err);

		g_main_loop_quit(loop);
		break;
	}
	default:
		break;
	}

	return 1;
}

GstElement *playback_pipeline(char *fn) {
  GstElement
    *pipeline,
    *playbin2,
    *audiorate,
    *fltaudio,
    *jackaudiosink;
  
  pipeline = gst_pipeline_new(fn);
  playbin2 = gst_element_factory_make("playbin2", "playbin2");
  g_assert(playbin2);
  audiorate = gst_element_factory_make("audiorate", "audiorate");
  fltaudio = gst_element_factory_make("fltaudio", "fltaudio");
  jackaudiosink = gst_element_factory_make("jackaudiosink", "jackaudiosink");
  g_assert(jackaudiosink);
  
  g_object_set(G_OBJECT (fltaudio), "caps",
               gst_caps_new_simple ("audio/x-raw-float",
                                    "rate", G_TYPE_INT, 44100,
                                    "channels", G_TYPE_INT, 2,
                                    "depth", G_TYPE_INT, 16,
                                    NULL), 
               NULL);

  gst_bin_add_many(GST_BIN (pipeline), 
                   playbin2,
                   //                   audiorate,
                   //                   jackaudiosink,
                   NULL
                   );
  /*  gst_element_link_many(fltaudio,
                        audiorate,
                        jackaudiosink,
                        NULL
                        );
  */
  g_object_set(G_OBJECT(playbin2),
               "uri", fn,
               "audio-sink", jackaudiosink,
               NULL);

  return(pipeline);
}

GstElement *webcam_pipeline() {
  GstElement 
    *pipeline,
    *v4l2src,
    *fltyuv,
    *ffenc_huffyuv,
    *fltrate,
    *videorate,
    *mux,
    *sink;

  char fn[256];
  struct timeval tv;

  pipeline = gst_pipeline_new("webcampipeline");
  v4l2src = gst_element_factory_make("v4l2src", "v4l2src");
  fltyuv = gst_element_factory_make("capsfilter", "fltyuv");
  fltrate = gst_element_factory_make("capsfilter", "fltrate");
  videorate = gst_element_factory_make("videorate", "videorate");
  mux = gst_element_factory_make ("avimux", "mux");
  sink = gst_element_factory_make ("filesink", "sink");

  g_object_set(G_OBJECT(fltyuv), "caps",
               gst_caps_new_simple ("video/x-raw-yuv",
                                    "framerate", GST_TYPE_FRACTION, 15, 1,
                                    "width", G_TYPE_INT, 640,
                                    "height", G_TYPE_INT, 480,
                                    NULL), 
               NULL);

  g_object_set(G_OBJECT(fltrate), "caps",
               gst_caps_new_simple ("video/x-raw-yuv",
                                    "framerate", GST_TYPE_FRACTION, 2, 1,
                                    "width", G_TYPE_INT, 640,
                                    "height", G_TYPE_INT, 480,
                                    NULL), 
               NULL);

  gettimeofday(&tv, NULL);
  sprintf(fn, "/old/webcam-%d.avi", (int) tv.tv_sec);
  g_object_set(G_OBJECT(sink),
               "location", fn,
               NULL);

  gst_bin_add_many(GST_BIN (pipeline), 
                   v4l2src,
                   fltyuv,
                   videorate,
                   fltrate,
                   mux,
                   sink,
                   NULL
                   );
  
  gst_element_link_many (
                         v4l2src,
                         fltyuv,
                         videorate,
                         fltrate,
                         mux,
                         sink,
                         NULL
                         );
  return(pipeline);
}

GstElement *screencast_pipeline() {
  GstElement 
    *pipeline, 

    *mux,
    *sink,
    
    *ximagesrc,
    *fltrgb,
    *fltyuv,
    *videorate,
    *ffmpegconv,
    *videoscale,
    *ffenc_huffyuv,
    *videoqueue,

    *jackaudiosrc,
    *audiorate,
    *fltaudio,
    *audioconvert,
    *audioqueue
    ;
  char fn[256];
  struct timeval tv;

  pipeline = gst_pipeline_new("screenpipeline");

  ximagesrc = gst_element_factory_make("ximagesrc", "source");
  fltrgb = gst_element_factory_make("capsfilter", "fltrgb");
  videorate = gst_element_factory_make("videorate", "videorate");
  ffmpegconv = gst_element_factory_make("ffmpegcolorspace", "ffmpegconv");
  videoscale = gst_element_factory_make("videoscale", "videoscale");
  fltyuv = gst_element_factory_make("capsfilter", "fltyuv");
  ffenc_huffyuv = gst_element_factory_make("ffenc_huffyuv", "ffenc_huffyuv");
  videoqueue = gst_element_factory_make("queue", "videoqueue");

  jackaudiosrc = gst_element_factory_make("jackaudiosrc", "jackaudiosrc");
  audiorate = gst_element_factory_make("audiorate", "audiorate");
  fltaudio = gst_element_factory_make("capsfilter", "fltaudio");
  audioconvert = gst_element_factory_make("audioconvert", "audioconvert");
  audioqueue = gst_element_factory_make("queue", "audioqueue");

  mux = gst_element_factory_make ("avimux", "mux");
  sink = gst_element_factory_make ("filesink", "sink");
  
  //configure
  gettimeofday(&tv, NULL);
  sprintf(fn, "/old/screen-%d.avi", (int) tv.tv_sec);
  g_object_set(G_OBJECT (sink),
               "location", fn,
               NULL);

  g_object_set(G_OBJECT (fltrgb), "caps",
               gst_caps_new_simple ("video/x-raw-rgb",
                                    "framerate", GST_TYPE_FRACTION, 10, 1,
                                    NULL), 
               NULL);

  g_object_set(G_OBJECT (videoscale),
               "method", 1,
               NULL);

  g_object_set(G_OBJECT (fltyuv), "caps",
               gst_caps_new_simple ("video/x-raw-yuv",
                                    "framerate", GST_TYPE_FRACTION, 10, 1,
                                    "width", G_TYPE_INT, 640,
                                    "height", G_TYPE_INT, 480,
                                    NULL), 
               NULL);

  g_object_set(G_OBJECT (ximagesrc),
               "use-damage", FALSE,
               "startx", 1,
               "starty", 1,
               "endx", 640,
               "endy", 480,
               NULL);

  g_object_set(G_OBJECT (jackaudiosrc),
               "connect", 0,
               "name", "screensave",
               NULL);

  g_object_set(G_OBJECT (fltaudio), "caps",
               gst_caps_new_simple ("audio/x-raw-float",
                                    "rate", G_TYPE_INT, 44100,
                                    "channels", G_TYPE_INT, 2,
                                    "depth", G_TYPE_INT, 16,
                                    NULL), 
               NULL);

  
  gst_bin_add_many(GST_BIN (pipeline), 
                   ximagesrc,
                   fltrgb,
                   videorate,
                   ffmpegconv,
                   //videoscale,
                   //fltyuv,
                   ffenc_huffyuv,
                   videoqueue,

                   jackaudiosrc,
                   audiorate,
                   fltaudio,
                   audioconvert,
                   audioqueue,

                   mux,
                   sink,

                   NULL
                   );
  
  gst_element_link_many (
                         ximagesrc,
                         fltrgb,
                         videorate,
                         ffmpegconv,
                         //videoscale,
                         //fltyuv,
                         ffenc_huffyuv,
                         videoqueue,
                         mux,
                         sink,
                         NULL
                         );

  gst_element_link_many(jackaudiosrc,
                        audiorate,
                        fltaudio,
                        audioconvert,
                        audioqueue,
                        mux,
                        NULL
                        );
  return(pipeline);
}


gint
main (gint   argc,
      gchar *argv[])
{
  GstBus *bus;

  gst_init(&argc, &argv);
  loop = g_main_loop_new (NULL, FALSE);

  screenpipe = screencast_pipeline();
  webcampipe = webcam_pipeline();
#ifdef PLAYBACK
  playbackpipe = playback_pipeline(PLAYBACK);
#endif
  
  bus = gst_pipeline_get_bus(GST_PIPELINE(screenpipe));
  gst_bus_add_watch(bus, bus_call, NULL);
  gst_object_unref(bus);

  bus = gst_pipeline_get_bus(GST_PIPELINE(webcampipe));
  gst_bus_add_watch(bus, bus_call, NULL);
  gst_object_unref(bus);
  
#ifdef OSC
  lo_server_thread osc = osc = lo_server_thread_new("7777", error);
  lo_server_thread_add_method(osc, "/screensave/start", "", start_handler, NULL);
  lo_server_thread_add_method(osc, "/screensave/stop", "", stop_handler, NULL);
  lo_server_thread_start(osc);

  gst_element_set_state(screenpipe, GST_STATE_PAUSED);
  gst_element_set_state(webcampipe, GST_STATE_PAUSED);
#ifdef PLAYBACK
  gst_element_set_state(playbackpipe, GST_STATE_PAUSED);
#endif
#else
  gst_element_set_state(screenpipe, GST_STATE_PLAYING);
  gst_element_set_state(webcampipe, GST_STATE_PLAYING);
#ifdef PLAYBACK
  gst_element_set_state(playbackpipe, GST_STATE_PLAYING);
#endif
#endif

  g_main_loop_run(loop);
  printf("cleaning\n");
  /* clean up */
  gst_element_set_state(screenpipe, GST_STATE_NULL);
  gst_object_unref(GST_OBJECT (screenpipe));

  gst_element_set_state(webcampipe, GST_STATE_NULL);
  gst_object_unref(GST_OBJECT (webcampipe));
   
  return 0;
}
