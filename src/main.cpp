#include "cameraimage.hpp"
#include "cameraaccess.hpp"
#include "streamer.hpp"
#include "httpserver.hpp"
#include "sys/types.h"
#include <unistd.h>

int main(int argc,char **argv){
	//Pipe to give the rpicam-vid PID to the server so it can take photos
	int pipefd[2];
	pipe(pipefd);

	pid_t pid = fork();
	if(pid==0){ //child
		Streamer streamer(&argc,&argv,pipefd);
	}
	startHttpServer(pipefd);
	return 0;
}
