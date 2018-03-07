/*  (Version: p2)
 *  parser.h 
 *    - created for use with "bbbserver.c"
 *    - prototypes of string parsing functions
 *    - contains documentation/usage notes and constant definitions
 */
#ifndef PARSER_H
#define PARSER_H

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

#define MINLINE 128
#define MAXLINE 8192

/* request type flags for check request type func */
#define RQT_INV 0		/* Invalid GET request 			*/
#define RQT_GET 1		/* Valid GET request 		        */
#define RQT_C_RNG 2		/* Client GET request w/ Range req 	*/
#define RQT_P_ADD 3		/* Peer node ADD request 		*/
#define RQT_P_VIEW 4	        /* Peer node VIEW request 		*/
#define RQT_P_RATE 5	        /* Peer node CONFIG RATE request 	*/

/*
 *	check_request_type
 *    - checks the type of request stored in buf
 *	  - returns one of above flags indicating type of request
 */
int check_request_type(char* buf);

/* 
 * parse_get_request
 *	  - parses requested file path from HTTP/1.1 GET request
 *	  - stores file path in path ptr
 *	  - returns 1 on success, 0 on fail 		
 */
int parse_get_request(char* buf, char *path);

/*
 * parse_file_type - parses file type from a full file path
 *    - stores file type in buf
 *    - returns 1 on success, 0 on fail
 */
int parse_file_type(char* filepath, char* buf);

/*
 * parse_range_request 
 *    - parses range request from full HTTP/1.1 GET request buffer
 *    - stores start and end bytes in char pointers
 *    - returns 2 on full success, i.e. parsed start and end bytes
 *    - returns 1 on partial success, i.e. parsed start bytes
 *    - returns 0 on fail
 */
int parse_range_request(char* buf, char* start_bytes, char* end_bytes);

/*                    
 *  parse_peer_add
 *      - parse peer add requests for back-end functionality
 *      - stores filename, ip/hostname, port and rate in ptrs 
 *      - return 1 on success, 0 on fail
 */
int parse_peer_add(char* buf, char* fp, char* ip_hostname,
	char* port, char* rate);


/*                    
 *  parse_peer_view_content
 *      - parse peer view requests for back-end functionality
 *      - stores filepath in ptr
 *      - returns 1 on success, 0 on fail
 */
int parse_peer_view_content(char* buf, char* filepath);

/*                    
 *  parse_peer_config_rate
 *      - parse configure back-end transfer rate requests for peer nodes
 *      - stores rate in ptr
 *      - returns 1 on success, 0 on fail
 */
int parse_peer_config_rate(char* buf, char* rate);

/*
 *  str_2_int
 *       - Converts type char* to type int;
 */
int parse_str_2_int(char* str);

#endif
