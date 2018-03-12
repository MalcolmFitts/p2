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

#include "parser.h"
#include "serverlog.h"
#include "datawriter.h"
#include "packet.h"

/* Peer Node Constant(s) */
#define MAX_NODES 50

/* Peer Node struct */
typedef struct Peer_Node {
	int content_rate;	  /* avg content bit rate (kbps) 	  */
	short port;		  	  /* back-end port num 				  */
	char* content_path;   /* filepath to content in peer node */
	char* ip_hostname;	  /* ip/hostname of peer node 		  */
	struct sockaddr_in node_addr; /* node's address */
} Node;


/* Peer Node Directory struct */
typedef struct Node_Directory {
	int cur_nodes;        /* current nbr of nodes             */
	int max_nodes;        /* max possible nbr of nodes        */
	Node *n_array;     /* array of pointers to nodes       */
} Node_Dir;

struct thread_data {
  struct sockaddr_in c_addr;  /* client address struct */
  int connfd;                 /* connection fd */
  int tid;                    /* thread id tag */
  int num;                    /* DEBUG - overall connected num */
  int listenfd_be;            /* back end listening socket */
  int port_be;                /* back end port */
  Node_Dir* node_dir;          /* Directory for node referencing   */
};

/*
 *  recieve_pkt
 *		- main threaded function to listen on back end port and serve content
 *
 *
 */
void* recieve_pkt(void* ptr);

/*
 *  serve_content
 *		- function to
 *
 *
 */
int serve_content(Pkt_t packet, int sockfd, struct sockaddr_in server_addr,
									int flag);


/* TODO make sure this works (serveraddr_be prob should be a pointer)
 *
 *  init_backend
 *		- initalizes the backend port for the server node
 *
 */
int init_backend(short port_be, struct sockaddr_in* self_addr);

/*
 *  create_node
 *		- allocates memory for and returns Node with values assigned
 *
 *	~param: path
 *		- path should start with "content/"
 *		ex: path = "content/rest/of/path.ogg"
 *
 *	~return values:
 *		- pointer to allocated node on success
 *		- NULL pointer on fail
 */
Node* create_node(char* path, char* name, int port, int rate);


/*
 *  check_node_content
 *		- checks peer_node for content defined by filename
 *
 *	~param: filename
 *		- filename should start with "content/"
 *		ex: filename = "content/rest/of/path.ogg"
 *
 *	~return values:f
 *		- returns 1 on finding content in node
 *		- returns 0 on failure to find content
 *		- return -1 if node is null
 */
int check_node_content(Node* pn, char* filename);


/*
 *  create_node_dir
 *		- allocates memory for and returns pointer to Node Directory
 *			with 0 initial Nodes
 *
 *	~param: max
 *		- attempts to define max number of Nodes stored in directory
 *		- if max > MAX_NODES, max will be overwritten with MAX_NODES.
 *
 *	~return values:
 *		- returns pointer to allocated node dir on success
 *		- returns NULL pointer on fail
 */
Node_Dir* create_node_dir(int max);





/*
 *  add_node
 *		- adds node to node directory, given directory is not full
 *
 *	~return values:
 *		- returns 1 on success (added node to directory)
 *		- returns 0 on fail
 */
int add_node(Node_Dir* nd, Node* node);

/*
 *  check_content
 *		- takes Node Directory and filename and attempts to find content
 *
 *
 *	~return values:
 *		- returns pointer to node that should have content
 *		- returns NULL if no node has content in directory
 *
 *	~front-end interaction:
 *		- front-end should use returned Node for sync_node
 */
Node* check_content(Node_Dir* dir, char* filename);


/*  TODO  --  CHECK
 *
 *  sync_node
 *		- attempts to initiate connection with node and receive acknowledgement
 *
 *	~return values:
 *		- returns pointer to Node's SYN-ACK buffer on success
 *		- (TODO) returns buffer indicating failure on fail
 *			-- buffer should have info on failure in this case
 *
 *	~front-end interaction:
 *		- front-end should parse returned buffer
 *		- will parse info for headers on successful sync
 *
 */
 char* sync_node(Node* node, uint16_t s_port, int sockfd);

/*  TODO  --  CHECK
 *
 *  request_content
 *    - attempts to request content from synced node
 *
 *  ~return values:
 *    - returns pointer to buffer w/ content on success
 *    - returns buffer indicating failure on fail
 *      -- buffer should have info on failure in this case
 *
 *  ~front-end interaction:
 *    - front-end should use returned buffer for HTTP response
 *    - will parse info for headers on successful sync
 *
 */
char* request_content(Node* node, uint16_t s_port, int sockfd,
  struct sockaddr_in serveraddr, uint32_t seq_ack_num);

/*  TODO
 *  add_response_be
 */
int peer_add_response(int connfd, char* BUF, struct thread_data *ct, Node_Dir* node_dir);

/*  TODO
 *  view_response_be
 */
int peer_view_response(int connfd, char*BUF, struct thread_data *ct, Node_Dir* node_dir);

/*  TODO
 *  rate_response_be
 */
int peer_rate_response(int connfd, char* BUF, struct thread_data *ct);


struct sockaddr_in get_sockaddr_in(char* hostname, short port);
/* filler end line */


#endif
