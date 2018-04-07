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
#include "configlib.h"

#define COM_BUFSIZE 1500
#define JSON_BUFSIZE 4096

extern pthread_mutex_t mutex;

struct thread_data {
  struct sockaddr_in c_addr;  /* client address struct */
  int connfd;                 /* connection fd */
  int tid;                    /* thread id tag */
  int num;                    /* DEBUG - overall connected num */
  int listenfd_be;            /* back end listening socket */
  int port_be;                /* back end port */
  char* config_fn;            /* config filename */
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

int init_backend(short port_be, struct sockaddr_in* self_addr);

/*  params:
 *  	connfd - socket to use for writing - might not need after using resp_buf
 *		BUF - stores raw GET request
 *		ct - stores info - not needed?
 *		resp_buf - used to recover data to write to client for peer add responses
 */
int peer_add_response(char* BUF);

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


void send_hdr_to_fe(char* com_buf, int file_size);


/*
 *  fin_flag:
 *		0 - DATA
 *		1 - DATA-FIN
 *		2 - FIN
 *
 */
void send_data_to_fe(char* com_buf, char* data, int fin_flag);

/*
 *  handle_uuid_rqt -> Handles: "GET /peer/uuid"
 *                  -> Sends: "{"uuid":"<uuid_node>"}" to client
 */
void handle_uuid_rqt(int connfd);


/*
 *  handle_neighbors_rqt -> Handles: "GET /peer/neighbors"
 *                       -> Sends: JSON list of neighbors and data
 */
void handle_neighbors_rqt(int connfd);


int handle_add_uuid_rqt(char* buf);

void handle_add_neighbor_rqt(char* buf);

void* advertise(void* ptr);

/* filler end line */


#endif
