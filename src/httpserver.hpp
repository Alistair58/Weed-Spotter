#ifndef HTTPSERVER_HPP
#define HTTPSERVER_HPP

#include <civetweb.h>

int findHighestPhotoId(void);
int takePhotoHandler(struct mg_connection *conn, void *);
void startHttpServer(int pipefd[2]);

#endif
