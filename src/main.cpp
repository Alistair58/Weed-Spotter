#include "cameraimage.hpp"
#include "cameraaccess.hpp"
#include "streamer.hpp"
#include "httpserver.hpp"
#include "sys/types.h"
#include <unistd.h>
#include "picoi2c.hpp"
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <iostream>
#include "cnn.hpp"


std::string currDir = "/home/users/alistair/Weed-Spotter";

Tensor uint8ToTensor(uint8_t *data,size_t dataSize,std::vector<int>& dimens);
d2 loadPixelStats();
void locateWeedsBlocking();

int main(int argc,char **argv){
	//Pipe to give the rpicam-vid PID to the server so it can take photos
	int pipefd[2];
	pipe(pipefd);

	pid_t streamerPid = fork();
	if(streamerPid==0){ //child
		Streamer streamer(&argc,&argv,pipefd);
	}
	pid_t httpServerPid = fork();
	if(httpServerPid==0){
		startHttpServer(pipefd);
	}
	pid_t picoI2cPid = fork();
	if(picoI2cPid==0){
		picoI2cListenBlocking();
	}
	locateWeedsBlocking();
	return 0;
}

void locateWeedsBlocking(){
	//TODO - just for testing
	const uint64_t usDelay = 1e6;

	d2 pixelStats = loadPixelStats();
	CNN cnn(pixelStats);

    gst_init(NULL, NULL);

	const std::vector<int> imageDimens = {3,480,640};
    std::string pipelineDesc =
        "rtspsrc location=rtsp://127.0.0.1:8554/stream ! decodebin ! "
        "videoconvert ! video/x-raw,format=RGB,width="+imageDimens[2]+
		",height="+imageDimens[1]+" ! appsink name=sink";

    GError *error = nullptr;
    GstElement *pipeline = gst_parse_launch(pipelineDesc.c_str(), &error);
    if (!pipeline) {
        std::cerr << "Pipeline error: " << error->message << std::endl;
        g_error_free(error);
        return;
    }

    GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

	while(1){
        //Pull one sample (blocking)
        GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
        if (!sample) {
            std::cerr << "No sample" << std::endl;
        }

        GstBuffer *buffer = gst_sample_get_buffer(sample);
        GstMapInfo map;
        if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
            //map.data is a pointer to the raw pixels
            //RGB height x width x channels
            const uint8_t *pixels = map.data;
            size_t size = map.size;
			Tensor inputImage = uint8ToTensor(map.data,map.size);

			const float hasWeedThreshold = 0.5f;
			std::vector result = cnn.forwards(inputImage);
			if(result[2] > hasWeedThreshold){
				std::cout << "Weed spotted at: ("+std::to_string(result[0])+","+
				std::to_string(result[1])+")" << std::endl;
			}
			else{
				std::cout << "No weeds present" << std::endl;
			}
            gst_buffer_unmap(buffer, &map);
        }
        gst_sample_unref(sample);

		sleep_us(usDelay);
	}	

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}

d2 loadPixelStats(){
	std::ifstream statsFile(currDir+"/res/stats.json");
    nlohmann::json jsonStats;
    statsFile >> jsonStats;
    statsFile.close();
    if(jsonStats.size()!=3){
        throw std::invalid_argument(ANSI_RED+"Stats file is not in the format {{mean1,..},{stdDev1,..},{count}}"+ANSI_RESET);
    }
	return jsonStats.get<d2>();
}

Tensor uint8ToTensor(uint8_t *data,size_t dataSize,std::vector<int>& dimens){
	//data is RGB, height x width x channels
	Tensor result(dimens);
	if(result.getTotalSize()!=dataSize){
		throw std::runtime_error("RTSP image data is not in the correct shape for the CNN");
	}
	if(dimens[0] != 3){
		throw std::runtime_error("RGB image has 3 channels. Tensor does not have 3 channels");
	}
	float *__restrict__ resultData = result.getData();
	for(int y=0;y<dimens[1];y++){
		int resultRow = y*dimens[2];
		for(int x=0;x<dimens[2];x++){
			int dataPixel = (y*dimens[2]+x)*3
			for(int c=0;c<3;c++){
				int resultIndex = c*childSizes[0] + resultRow + x;
				resultData[resultIndex] = data[dataPixel + c];
			}
		}
	}
	return result;
}