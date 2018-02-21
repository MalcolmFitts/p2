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

#define MAX_NODES 20

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

/* Packet Header struct */
typedef struct Packet_Header {
	uint16_t source_port;		/* source port 	- 2 bytes */
	uint16_t dest_port;			/* dest port 	- 2 bytes */

	uint16_t length;			/* length		- 2 bytes */
	uint16_t checksum;			/* checksum		- 2 bytes */

	int seq_num;				/* sequence num - 4 bytes */

	int ack;					/* acknowledge  - 4 bytes */
} P_Hdr;



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
 *		- attempts to define max nbr of Nodes stored in dir
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


















/* filler end line */