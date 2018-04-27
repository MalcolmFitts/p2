
#ifndef GOSSIP_H
#define GOSSIP_H

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


#include "frontend.h"
#include "parser.h"
#include "packet.h"
#include "node.h"
#include "configlib.h"
#include "neighbor.h"


typedef struct Search_Info {
  int max_recv_ttl;
  int active_timer;
  char* content;
  char* peers;
} S_Inf;

// search thread content
typedef struct search_tc{
  int sock;
  int ttl;
  char path[MAXLINE];
  char config_name[MAXLINE];
  char search_list[BUFSIZE];
} s_tc;

/*   Search_Info struct values:
 *
 *  content
 *      - keep track of what content was searched for
 *
 *  max_recv_ttl
 *      - keep track of the max received ttl:
 *         -- if recv request for same content with lower ttl, dont start another search
 *               b/c we already started one
 *         -- calculate when search will end based on max recvd ttl
 *
 *  peers
 *     - keep track of peers that have content
 *
 *  active_timer
 *     - keep track of how many gossip cycles passed
 */


typedef struct Search_Directory {
  int cur_search;
  int max_search;
  S_Inf* search_arr;
} S_Dir;


/*  handle_exchange_msg
 *
 *  - handles packets flagged as exchange messages
 *
 *  params:
 *
 *    pkt - exchange packet
 *    sockfd - socket to send response to
 *    sender_addr - sender of packet
 *
 */

int handle_exchange_msg(Pkt_t pkt, int sockfd,
                        struct sockaddr_in sender_addr, S_Dir* s_dir);

/*  parse_search_info
 *
 *  - creates search info struct
 *
 *  params:
 *
 *    pkt - exchange packet to be parsed
 *
 */

S_Inf* parse_search_info(Pkt_t pkt);


/*  check_search_dir
 *
 *  - checks search directory for records of matching search info
 *
 *  return values:
 *
 *    0 - did not find content from info in any element of dir
 *    1 - found inactive search inside of directory
 *    2 - found active search inside of directory
 *
 */

int check_search_dir(S_Dir* dir, S_Inf* info);


/*  sync_peer_info
 *
 *  - syncs peer info for search in directory matching parameter 'info'
 *
 *  return values:
 *
 *    NULL  - some error
 *    char* - merged peer list
 *
 */

char* sync_peer_info(S_Dir* dir, S_Inf* info);

/*  merge_peer_lists
 *
 *  - actually merges peer lists without duplicates
 *
 *  return values:
 *
 *    NULL  - some error
 *    char* - merged peer list
 *
 */

char* merge_peer_lists(char* p_list1, char* p_list2);


/*  add_uuid_to_list
 *
 *  - attempts to add a UUID to existing list of peer UUIDs
 *  - will not add if already exists in list
 *
 *  return values:
 *
 *    NULL  - some error
 *    char* - list (with UUID added if not already included in list)
 *
 */

char* add_uuid_to_list(char* list, char* uuid);



#endif
