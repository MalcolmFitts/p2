

#ifndef NODE_H
#define NODE_H

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
	Node *n_array;        /* array of pointers to nodes       */
} Node_Dir;



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
 *	~return values:
 *		- returns 1 on finding content in node
 *		- returns 0 on failure to find content
 *		- return -1 if node is null
 */
int check_node_content(Node* pn, char* filename);


/*
 *  check_node_host
 *		- checks peer_node for matching hostname
 *
 *	~param: hostname
 *		ex: hostname = "128.2.3.137"
 *
 *	~return values:
 *		- returns 1 on finding node has hostname
 *		- returns 0 on non-matching hostname
 *		- return -1 if node is null
 */
int check_node_host(Node* pn, char* hostname);


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



/*
 *  find_node_by_hostname
 *		- takes Node Directory and hostname and attempts to find matching node
 *
 *
 *	~return values:
 *		- returns pointer to node with hostname
 *		- returns NULL if no node has this hostname in directory
 *
 */
Node* find_node_by_hostname(Node_Dir* dir, char* hostname);




#endif
