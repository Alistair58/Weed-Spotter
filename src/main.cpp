#include "cameraimage.hpp"
#include "cameraaccess.hpp"
#include "streamer.hpp"
#include "httpsserver.hpp"
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
	startHttpsServer(pipefd);
	return 0;
}
