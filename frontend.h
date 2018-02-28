#ifndef FRONTEND_H
#define FRONTEND_H

#include "datawriter.h"
#include "parser.h"
#include "serverlog.h"
#include "backend.h"

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define MAX_FILEPATH_LEN 512
#define MAXLINE 8192
#define BUFSIZE 1024
#define MAX_RANGE_NUM_LEN 32
#define SERVER_NAME "BBBserver"

int init_frontend(struct sockaddr_in serveraddr_fe, int port_fe);

/* Writes the frontend response to the client for a normal GET request */
int frontend_response(int connfd, char* BUF, struct thread_data *ct);

#endif
