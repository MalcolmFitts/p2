/*  (Version: p2)
 * backend.h
 *    - created for use with "bbbserver.c"
 *
 *
 */

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

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
	Node *(*n_array);     /* array of pointers to nodes       */
} Node_Dir;


/* Packet Size Constants (bytes) */
#define P_HDR_SIZE 16
#define MAX_DATA_SIZE 65519
#define MAX_PACKET_SIZE 65535 

/* Packet constants and definitions */
#define SHORT_INT_MASK 0x0000FFFF

/* ACK field default values */
#define PKT_DAT 0xAAAAAAAA
#define PKT_ACK 0x8888FFFF

/* Packet type (creation) flags */
#define PKT_FLAG_DATA 1
#define PKT_FLAG_ACK  2

/* intial value for creating ACK packet "ack" value */
#define ACK_MASK_INIT 0xFFFFAAAA

/* value of ack in ACK packet that didnt lose any files */
#define ACK_NO_LOSS 0x8888FFFF

/*	possible future flag(s):
#define PKT_FLAG_FRAG 3
*/



/* Packet Header struct */
typedef struct Packet_Header {
	uint16_t source_port;		/* source port 	- 2 bytes */
	uint16_t dest_port;			/* dest port 	- 2 bytes */

	uint16_t length;			/* length		- 2 bytes */
	uint16_t checksum;			/* checksum		- 2 bytes */

	unsigned int seq_num;		/* sequence num - 4 bytes */

	unsigned int ack;			/* acknowledge  - 4 bytes */
} P_Hdr;

/* Packet struct */
typedef struct Packet {
	P_Hdr header;				/* Packet Header */
	char buf[MAX_DATA_SIZE];	/* Packet Data   */
} Pkt_t;


/*
 *  TODO 3.) write parse_packet
 *
 * Pkt_t* parse_packet (char* buf)
 *		- creates packet struct by parsing input buffer
 *
 *	(things i need)
 *		- parse header data
 *			-- source port --> buf { 0,  1}
 *			-- dest port   --> buf { 2,  3}
 *			-- length      --> buf { 4,  5}
 *			-- check sum   --> buf { 6,  7}
 *			-- seq num     --> buf { 8, 11}
 *			-- ack    	   --> buf {12, 15}
 *
 *		- data (rest)
 *
 *
 *
 *
 *  TODO 4.) write request_content
 *
 * Pkt_t* request_content (Node* n)
 *		- attempts to request and receive data packet from node
 *
 *	TODO 5.) deal with receiving requests
 *
 *
 */


/*
 * create_node
 *		- allocates memory for and returns Node with values assigned
 *
 *	~param: path
 *		- path should start with /content/
 *		ex: path = "/content/rest/of/path.ogg"
 *
 *	~return values:
 *		- returns pointer to allocated node on success, NULL pointer on fail
 */
Node* create_node(char* path, char* name, int port, int rate);


/*
 * check_node_content
 *		- checks peer_node for content defined by filename
 *
 *	~param: filename
 *		- filename should start with /content/
 *		ex: filename = "/content/rest/of/path.ogg"
 *
 *	~return values:
 *		- returns 1 on finding content in node
 *		- returns 0 on failure to find content
 *		- return -1 if node is null
 */
int check_node_content(Node* pn, char* filename);

/*
 * create_node_dir
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
 * add_node
 *		- adds node to node directory, given directory is not full
 *
 *	~return values:
 *		- returns 1 on success (added node to directory)
 *		- returns 0 on fail
 */
int add_node(Node_Dir* nd, Node* node);


/*  TODO  --  CHECK
 *
 * create_packet 
 *		- creates packet struct to be sent/received by nodes
 *  
 *  ~param: ack_mask
 *		- if this is an acknowledgement, this should indicate which packets
 *			were received
 *		- acknowledge 16 packets at a time
 *		- normal value: 
 *			0xFFFFAAAA (last 16 bits alternate)
 *		- bad packet: 
 *			bit representing which packet was bad is flipped (in last 16 bits)
 *
 *  ~param: flag
 *		= 1 (PKT_FLAG_DATA) --> create a data packet
 *		= 2 (PKT_FLAG_ACK)  --> create an acknowledge packet
 *
 *	~return values:
 *		- returns packet with correct content on success
 *		- returns NULL pointer on fail
 *
 */


Pkt_t* create_packet (Node* n, uint16_t s_port, unsigned int s_num, 
  char* filename, int flag, unsigned int ack_mask);



/*
 *  calc_checksum 
 *		- calculates checksum value of a packet header struct
 *		- does this independently of header's checksum value
 *
 *	~notes:
 *		- header consists of 7 16-bit values
 *		- checksum will be 8th
 *		- vars:
 *			from hdr: {source_port, dest_port, length}
 *			seq_num1 = (uint16_t) ((seq_num >> 16) & 0xFFFF)
 *			seq_num2 = (uint16_t) ((seq_num & 0xFFFF))
 *			ack1 = (uint16_t) ((ack >> 16) & 0xFFFF)
 *			ack2 = (uint16_t) (ack & 0xFFFF)
 *
 *		- calculation:
 *			x = [(s_p) + (d_p) + (l) + (seq_num1) + (seq_num2)
 *					+ (ack1) + (ack2)]
 *			checksum = ~ x;
 *
 *
 */

uint16_t calc_checksum(P_Hdr* hdr);






/* filler end line */