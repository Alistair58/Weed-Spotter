#include "streamer.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

Streamer::Streamer(int *argcPtr,char ***argvPtr,int parentPipefd[2]){
	close(parentPipefd[0]);
	gst_init(argcPtr,argvPtr);
	//Set up the streaming pipe
	int streamPipefd[2];
	if(pipe(streamPipefd) == -1){
		throw std::runtime_error("Could not pipe");
	}
	pid_t pid = fork();
	if(pid == 0){
		//rpicam-vid writes to the pipe
		//close read end
		close(streamPipefd[0]);
		//Set the output to stdout where gstreamer will read it from
		dup2(streamPipefd[1],STDOUT_FILENO);
		//We don't need it anymore as we've set it to stdout
		close(streamPipefd[1]);
		execlp(
			"rpicam-vid","rpicam-vid", 
			"-t","0",
			"--inline",
			"--intra","30",
			"--nopreview",
			"--width","640",
			"--height","480",
			"--framerate","30",
			"--codec","h264",
			"-o","-", //Output to stdout
			(char *)NULL
		);
		throw std::runtime_error("execlp failed");
	}
	//Give the child PID to our parent (the grandparent)
	write(parentPipefd[1],&pid,sizeof(pid_t));
	close(parentPipefd[1]);

	//We don't write anything
	close(streamPipefd[1]);

	GstRTSPServer *server = gst_rtsp_server_new();
	GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);
	
	const char *sprop_param_sets = "Z0LADdkBQfsBEAAAAwAQAAADAyDxgxqw,aM48gA=="; //For 640x480
	std::string pipeline =
				"( fdsrc fd="+std::to_string(streamPipefd[0])+" name=picamsrc "+
				" ! queue "+
				" ! h264parse config-interval=-1 "+
				" ! video/x-h264,stream-format=avc,alignment=au "+
				" ! rtph264pay name=pay0 config-interval=1 pt=96 "+
				" sprop-parameter-sets=\""+ sprop_param_sets +"\" )";
	GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();
	gst_rtsp_media_factory_set_launch(factory,pipeline.c_str());
	gst_rtsp_media_factory_set_shared(factory,TRUE);
	gst_rtsp_media_factory_set_latency(factory,0);
	
	g_signal_connect(factory, "media-configure", (GCallback) Streamer::onMediaConfigure,NULL);

	gst_rtsp_mount_points_add_factory(mounts,"/stream",factory);
	g_object_unref(mounts);

	gst_rtsp_server_attach(server,NULL);
	std::cout << "RTSP stream ready at rtsp://<pi-ip>:8554/stream" << std::endl;
	GMainLoop *loop = g_main_loop_new(NULL,FALSE);
	g_main_loop_run(loop);
}

void Streamer::onMediaConfigure(GstRTSPMediaFactory *factory,
				GstRTSPMedia *media,
				gpointer user_data){
	GstElement *element = gst_rtsp_media_get_element(media);
	if(!element) return;
	GstElement *fd = gst_bin_get_by_name_recurse_up(GST_BIN(element),"picamsrc");
	if(fd){
		//is-live doesn't exist in here apparently - you get a warning at runtime
		g_object_set(fd,"do-timestamp",TRUE,NULL);
		gst_object_unref(fd);
	}
	gst_object_unref(element);
}
