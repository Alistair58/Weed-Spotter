#ifndef HTTPSERVER_HPP
#define HTTPSERVER_HPP

#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <sys/types.h>
#include <civetweb.h>
#include <cstring>
#include <string>
#include <regex>
#include <filesystem>
#include <climits>

int findHighestPhotoId(void);
int takePhotoHandler(struct mg_connection *conn, void *);
void startHttpServer(int pipefd[2]);

#endif
