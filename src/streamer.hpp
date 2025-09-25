#ifndef STREAMER_HPP
#define STREAMER_HPP

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

class Streamer{
	public:
		Streamer(int *argcPtr,char ***argvPtr,int parentPipefd[2]);
		static void onMediaConfigure(   GstRTSPMediaFactory *factory,
						GstRTSPMedia *media,
						gpointer user_data);
};

#endif
