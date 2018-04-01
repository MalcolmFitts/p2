/*  (Version: p2)
 *  serverlog.h 
 *    - created for use with "bbbserver.c"
 *    - prototypes of server logging functions
 *    - contains documentation/usage notes and constant definitions
 */

#ifndef SERVERLOG_H
#define SERVERLOG_H

#include <string.h>
#include <stdio.h>

/* 
 *  debug flag for server log (DB_LOG)
 *    - set value to 2 for debug log
 *    - set value to 1 for regular log
 *	  - ser value to 0 for no log
 */
#define DB_LOG 1

#define MAX_PRINT_LEN 1024

/*
 *	log_msg
 *    - created for use in bbbserver.c main fn
 *	  - logs msg with no extra debug data
 *    - internally calls server_log fn
 */
void log_msg(char* msg);

/*
 *	log_main
 *    - created for use in bbbserver.c main fn
 *	  - logs msg and uses c as debug data
 *    - internally calls server_log fn
 */
void log_main(char* msg, int c);

/*
 *	log_thr
 *    - created for use in bbbserver.c serve_client_thread fn
 *	  - logs msg and uses c and t as debug data
 *    - internally calls server_log fn
 */
void log_thr(char* msg, int c, int t);

/*
 *	log_req
 *    - created for use in bbbserver.c serve_client_thread fn,
 *        when handling multiple requests from client
 *	  - logs msg and uses c, t and r as debug data
 *    - internally calls server_log fn
 */
void log_req(char* msg, int c, int t, int r);

/*
 *	server_log
 *    - created for use in bbbserver.c
 *	  - logs message and debug info depending on defined flags (DB_LOG and LOG)
 *    - DB_LOG = 2 --> logs concatenation of db and message
 *	  - DB_LOG = 1 --> logs message
 *	  - DB_LOG = 0 --> logs nothing 
 */
void server_log(char* message, char* db);

#endif
