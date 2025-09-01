#ifndef HTTPSSERVER_HPP
#define HTTPSSERVER_HPP

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <sys/types.h>

void startHttpsServer(int pipefd[2]);

#endif
