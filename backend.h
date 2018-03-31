/*  (Version: p2)
 * backend.h
 *    - created for use with "bbbserver.c"
 *
 *
 */

#ifndef BACKEND_H
#define BACKEND_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <pthread.h>

#include "parser.h"
#include "serverlog.h"
#include "datawriter.h"
#include "packet.h"
#include "node.h"
#include "bbbserver.h"

struct thread_data {
  struct sockaddr_in c_addr;  /* client address struct */
  int connfd;                 /* connection fd */
  int tid;                    /* thread id tag */
  int num;                    /* DEBUG - overall connected num */
  int listenfd_be;            /* back end listening socket */
  int port_be;                /* back end port */
};

/*
 *	### FOR REFERENCE ###
 *
 *	struct sockaddr_in {
 *		short            sin_family;   // e.g. AF_INET
 *		unsigned short   sin_port;     // e.g. htons(3490)
 *		struct in_addr   sin_addr;     // see struct in_addr, below
 *		char             sin_zero[8];  // zero this if you want to
 *	};
 *
 *	struct in_addr {
 *		unsigned long s_addr;  // load with inet_aton()
 *	};
 */

/*
 *  recieve_pkt
 *		- main threaded function to listen on back end port and serve content
 */
void* handle_be(void* ptr);

/*
 *  serve_content
 *		- function to
 */
int serve_content(Pkt_t packet, int sockfd, struct sockaddr_in server_addr,
									int flag);

/*  TODO
 *  add_response_be
 */
int peer_add_response(int connfd, char* BUF, struct thread_data *ct);

/*  TODO
 *  view_response_be
 */
 int peer_view_response(char* filepath, char* file_type, uint16_t port_be,
                        int sockfd_be, char* COM_BUF);

/*  TODO
 *  rate_response_be
 */
int peer_rate_response(int connfd, char* BUF, struct thread_data *ct);


struct sockaddr_in get_sockaddr_in(char* hostname, short port);
/* filler end line */


#endif
