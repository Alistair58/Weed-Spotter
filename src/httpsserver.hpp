#ifndef HTTPSSERVER_HPP
#define HTTPSSERVER_HPP

#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <sys/types.h>

void startHttpsServer(int pipefd[2]);

#endif
