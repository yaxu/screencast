CFLAGS=`pkg-config --cflags --libs gstreamer-0.10`

screensave: screensave.c
	gcc -g -Wall -llo $(CFLAGS) screensave.c -o screensave

screencast: screencast.c
	gcc -g -Wall -llo $(CFLAGS) screencast.c -o screencast

placast: placast.c
	gcc -g -Wall -llo $(CFLAGS) placast.c -o placast

