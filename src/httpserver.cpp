#include "httpserver.hpp"


static pid_t rpicamPid;
static int photoId = -1;

int takePhotoHandler(struct mg_connection *conn,void *){
	mg_printf(conn,
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/plain\r\n"
		"Connection: close\r\n\r\n"
	);
	if(rpicamPid>0){
		photoId++;
		pid_t pid = fork();
		if(pid==0){
			std::string locationStr = "location=/home/alistair/pictures/photo_"+std::to_string(photoId)+".jpg";
			execlp("timeout","timeout",
				"5", //Max wait of 5 seconds, the stream is more important than the photo
				"gst-launch-1.0",
				"-e",
				"rtspsrc","location=rtsp://127.0.0.1:8554/stream","latency=50", //wait 50ms for all rtsp packets
				"!","rtph264depay",
				"!","avdec_h264",
				"!","videoconvert",
				"!","jpegenc",
				"!","multifilesink",locationStr.c_str(),"max-files=1",
				(char*) NULL
			);
			throw std::runtime_error("Could not take photo");
		}
		std::cout << "Photo is being taken" << std::endl;
		mg_printf(conn,
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/plain\r\n"
			"Connection: close\r\n\r\n"
			"Take photo triggered\n"
		);
		return 200;
	}
	else{
		std::cerr << "rpicam-vid PID is invalid or not set" << std::endl;
		mg_printf(conn,
			"HTTP/1.1 500 Internal Server Error\r\n"
			"Content-Type: text/plain\r\n"
			"Connection: close\r\n\r\n"
			"Server error: PID not set");
		return 500;
	}
}

void startHttpServer(int pipefd[2]){
	close(pipefd[1]);
	read(pipefd[0],&rpicamPid,sizeof(pid_t));
	close(pipefd[0]);
	photoId = findHighestPhotoId();

	const char *options[] = {
		"listening_ports","8080",
		0
	};
	struct mg_callbacks callbacks;
	memset(&callbacks,0,sizeof(callbacks));
	struct mg_context *ctx = mg_start(&callbacks,nullptr,options);
	if(!ctx){
		throw std::runtime_error("Failed to start CivetWeb HTTP server");
	}
	mg_set_request_handler(ctx,"/take_photo",takePhotoHandler,nullptr);
	std::cout << "HTTP server listening on port 8080" << std::endl;

	while(true) sleep(1);
	mg_stop(ctx);
}

int findHighestPhotoId(void){
	const std::string folderPath = "/home/alistair/pictures";
	std::regex photoFnamePattern(R"(photo_(\d+)\.jpg)");
	//If we don't find any photos return -1 which will give the first photo as "photo_0.jpg"
	int maxIndex = -1;
	for(const auto& entry: std::filesystem::directory_iterator(folderPath)){
		if(entry.is_regular_file()){
			std::string fname = entry.path().filename().string();
			std::smatch match;
			if(std::regex_match(fname,match,photoFnamePattern)){
				int index = std::stoi(match[1].str());
				if(index>maxIndex) maxIndex = index;
			}
		}
	}
	return maxIndex;
}
