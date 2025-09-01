#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include "httpsserver.hpp"


void startHttpsServer(int pipefd[2]){
	close(pipefd[1]);
	pid_t rpicamPid;
	read(pipefd[0],&rpicamPid,sizeof(pid_t));
	close(pipefd[0]);

	httplib::SSLServer server("./certif/server.crt","./certif/server.key");

	server.Post("/take_photo", [](const httplib::Request& req, httplib::Response& res){
		std::cout << "Received POST request to /take_photo" << std::endl;
		if(rpicamPid>0){
			kill(rpicamPid,SIGUSR1); //Oi, take a photo
			std::cout << "Sent SIGUSR1 to PID " << rpicamPid << std::endl;
			res.set_content("Take photo triggered","text/plain");
		}
		else{
			std::cerr << "rpicam-vid PID is invalid" << std::endl;
			res.status = 500; //internal server error
			res.set_content("Server error: rpicam-vid PID not set","text/plain");
		}
	});

	std::cout << "HTTPS server listening on port 8080" << std::endl;
	server.listen("0.0.0.0",8080);
}
