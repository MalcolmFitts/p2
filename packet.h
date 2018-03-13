#ifndef PACKET_H
#define PACKET_H

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

/* Packet Size Constants (bytes) */
#define P_HDR_SIZE 16
#define MAX_DATA_SIZE 65519
#define MAX_PACKET_SIZE 65535


/* Packet type (creation) flags */
#define PKT_FLAG_UNKNOWN -1
#define PKT_FLAG_CORRUPT  0
#define PKT_FLAG_DATA     1
#define PKT_FLAG_ACK      2
#define PKT_FLAG_SYN 	    3
#define PKT_FLAG_SYN_ACK  4
#define PKT_FLAG_FIN      5


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
Pkt_t create_packet (uint16_t dest_port, uint16_t s_port, unsigned int s_num,
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
int get_packet_type (Pkt_t packet);

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
uint16_t calc_checksum(P_Hdr hdr);

#endif
