#include "streamer.hpp"


Streamer::Streamer(int *argcPtr,char ***argvPtr){
	gst_init(argcPtr,argvPtr);
	//Set up the pipe
	int pipefd[2];
	if(pipe(pipefd) == -1){
		throw std::runtime_error("Could not pipe");
	}
	pid_t pid = fork();
	if(pid == 0){
		//rpicam-vid writes to the pipe
		//close read end
		close(pipefd[0]);
		//Set the output to stdout where gstreamer will read it from
		dup2(pipefd[1],STDOUT_FILENO);
		//We don't need it anymore as we've set it to stdout
		close(pipefd[1]);
		execlp(
			"rpicam-vid","rpicam-vid", 
			"-t","0",
			"--inline",
			"--nopreview",
			"--width","640",
			"--height","480",
			"--framerate","25",
			"--codec","h264",
			"-o","-",
			(char *)NULL
		);
		throw std::runtime_error("execlp failed");
	}
	//We don't write anything
	close(pipefd[1]);

	GstRTSPServer *server = gst_rtsp_server_new();
	GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);

	std::string pipeline =
				"( fdsrc fd="+std::to_string(pipefd[0])+
				" ! queue ! h264parse ! video/x-h264,stream-format=avc,alignment=au ! rtph264pay name=pay0 config-interval=1 pt=96 )";
	GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();
	gst_rtsp_media_factory_set_launch(factory,pipeline.c_str());
	gst_rtsp_media_factory_set_shared(factory,TRUE);
	
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
	GstIterator *it = gst_bin_iterate_sources(GST_BIN(element));
	GValue item = G_VALUE_INIT;
	gboolean done = FALSE;

	while(!done){
		switch(gst_iterator_next(it,&item)){
			case GST_ITERATOR_OK: {
				GstElement *src = GST_ELEMENT(g_value_get_object(&item));
				if(g_str_has_prefix(GST_ELEMENT_NAME(src),"fdsrc")){
					g_object_set(src,"is-live",TRUE,NULL);
				}
				g_value_unset(&item);
				break;
			}
			case GST_ITERATOR_DONE:
			default:
				done = TRUE;
				break;
		}
	}
	gst_iterator_free(it);
}



