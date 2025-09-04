#include "cameraimage.hpp"
#include "cameraaccess.hpp"
#include "streamer.hpp"
#include "httpserver.hpp"
#include "sys/types.h"
#include <unistd.h>
#include "picoi2c.hpp"

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
	picoI2cListenBlocking();
	return 0;
}
