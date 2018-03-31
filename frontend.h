#ifndef FRONTEND_H
#define FRONTEND_H

#include "datawriter.h"
#include "parser.h"
#include "serverlog.h"
#include "backend.h"
<<<<<<< HEAD
#include "bbbserver.h"
=======
#include "node.h"
>>>>>>> 59bd87cf571f0b7a6145da3b1ba960aeaeb718aa

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

#define COM_BUF_DATA 1
#define COM_BUF_HDR 2
#define COM_BUF_FIN 3
#define COM_BUFSIZE 1500

int init_frontend(short port_fe, struct sockaddr_in* self_addr);

/* Writes the frontend response to the client for a normal GET request */
int frontend_response(int connfd, char* BUF, struct thread_data *ct);

void handle_be_response(char* COM_BUF, int connfd, char* content_type);

#endif
