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

/* Peer Node Constant(s) */
#define MAX_NODES 50

/* Peer Node struct */
typedef struct Peer_Node {
	int content_rate;	  /* avg content bit rate (kbps) 	  */
	uint16_t port;		  /* back-end port num 				  */
	char* content_path;   /* filepath to content in peer node */
	char* ip_hostname;	  /* ip/hostname of peer node 		  */
} Node;


/* Peer Node Directory struct */
typedef struct Node_Directory {
	int cur_nodes;        /* current nbr of nodes             */
	int max_nodes;        /* max possible nbr of nodes        */
	Node *n_array;     /* array of pointers to nodes       */
} Node_Dir;


/* Packet Size Constants (bytes) */
#define P_HDR_SIZE 16
#define MAX_DATA_SIZE 65519
#define MAX_PACKET_SIZE 65535


/* Packet type (creation) flags */
#define PKT_FLAG_UNKNOWN -1
#define PKT_FLAG_CORRUPT 0
#define PKT_FLAG_DATA    1
#define PKT_FLAG_ACK     2
#define PKT_FLAG_SYN 	 3
#define PKT_FLAG_SYN_ACK 4
#define PKT_FLAG_FIN     5


/* TODO add FIN implimentation */

/*	possible future flag(s):
#define PKT_FLAG_FRAG
*/

/*
 *  Masks for flag in packet header
 *
 *  flag = |  DATA  | SYN-ACK |  SYN  | ACK  |
 */
#define PKT_CORRUPT_MASK  0x0001    /* CORRUPT mask */
#define PKT_DATA_MASK     0x0002    /* DATA mask    */
#define PKT_ACK_MASK      0x0004
#define PKT_SYN_MASK      0x0008    /* SYN mask     */
#define PKT_SYN_ACK_MASK  0x0010    /* SYN-ACK mask */
#define PKT_FIN_MASK      0x0020    /* DATA mask    */



/* Packet Header Struct */
typedef struct Packet_Header {
  uint16_t source_port;		/* source port 	- 2 bytes */
  uint16_t dest_port;			/* dest port 	- 2 bytes */

  uint16_t length;			/* length		- 2 bytes */
  uint16_t checksum;			/* checksum		- 2 bytes */

  uint32_t seq_num;                     /* sequence num - 4 bytes */

  uint16_t flag;
  uint16_t data_offset;

} P_Hdr;

/* Packet Struct */
typedef struct Packet {
  P_Hdr header;				/* Packet Header */
  char buf[MAX_DATA_SIZE];	/* Packet Data   */
} Pkt_t;

/* Struct for threaded function for receiving packets */
typedef struct Receive_Struct {
  int sockfd;
} Recv_t;

struct thread_data {
  struct sockaddr_in c_addr;  /* client address struct */
  int connfd;                 /* connection fd */
  int tid;                    /* thread id tag */
  int num;                    /* DEBUG - overall connected num */
  int listenfd_be;            /* back end listening socket */
  int port_be;                /* back end port */
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
int serve_content(Pkt_t* packet, int sockfd, struct sockaddr_in* serveraddr, int flag);


/* TODO make sure this works (serveraddr_be prob should be a pointer)
 *
 *  init_backend
 *		- initalizes the backend port for the server node
 *
 */
int init_backend(struct sockaddr_in* serveraddr_be, int port_be);

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
 *  create_packet
 *		- creates packet struct to be sent/received by nodes
 *
 *  ~param: flag
 *		= 1 (PKT_FLAG_DATA) 	--> creates data packet
 *		= 2 (PKT_FLAG_ACK)  	--> creates ack (received) packet
 *		= 3 (PKT_FLAG_SYN) 		--> creates syn (request) packet
 *		= 4 (PKT_FLAG_SYN_ACK)  --> creates syn-ack (request accepted) packet
 *
 *	~return values:
 *		- returns packet with correct content on success
 *		- returns NULL pointer on fail
 *
 */
Pkt_t* create_packet (uint16_t dest_port, uint16_t s_port, unsigned int s_num,
  char* filename, int flag);


/*  TODO  --  CHECK
 *
 *  discard_packet
 *		- discards packet by freeing memory
 *
 *	~notes:
 *		- WARNING: packet data will be GONE FOREVER
 *		- should only be used when you DO NOT need packet contents
 */
void discard_packet(Pkt_t *packet);


/*
 *  calc_checksum
 *		- calculates checksum value of a packet header struct
 *		- does this independently of header's checksum value
 *
 *	~notes:
 *		- header consists of 7 16-bit values
 *		- checksum will be 8th
 *		- vars:
 *			from hdr: {source_port, dest_port, length, ack, syn}
 *			seq_num1 = (uint16_t) ((seq_num >> 16))
 *			seq_num2 = (uint16_t) (seq_num)
 *
 *		- calculation:
 *			x = [(s_p) + (d_p) + (l) + (seq_num1) + (seq_num2)
 *					+ (ack) + (syn)]
 *			checksum = ~ x;
 *
 *
 */
uint16_t calc_checksum(P_Hdr* hdr);


/*
 *  parse_packet
 *		- creates packet struct by parsing input buffer
 *
 *	~notes:
 *		- header data
 *			-- source port --> buf { 0,  1}
 *			-- dest port   --> buf { 2,  3}
 *			-- length      --> buf { 4,  5}
 *			-- check sum   --> buf { 6,  7}
 *			-- syn    	   --> buf { 8,  9}
 *			-- ack         --> buf {10, 11}
 *			-- seq num     --> buf {12, 15}
 *		- data (rest)
 */
Pkt_t* parse_packet (char* buf);


/*
 *  writeable_packet
 *		- takes packet and creates char array filled copy with packet byte data
 *
 *
 */
char* writeable_packet(Pkt_t* packet);


/*
 *  get_packet_type
 *		- takes packet and returns (expected) defined type
 *		- checks header's syn and ack flags
 *
 *	~return values:
 *		= -1 --> corrupted packet (undefined reason)
 *		=  0 --> corrupted packet (checksum)
 *		=  1 --> data packet
 *		=  2 --> ACK packet
 *		=  3 --> SYN packet
 *		=  4 --> SYN-ACK packet
 *
 */
int get_packet_type (Pkt_t* packet);




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
 char* sync_node(Node* node, uint16_t s_port, int sockfd,
   struct sockaddr_in serveraddr);

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


struct sockaddr_in get_sockaddr_in(unsigned int ip, short port);
/* filler end line */


#endif
