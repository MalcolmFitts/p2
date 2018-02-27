/*  (Version: p2)
 *  serverlog.c 
 *    - created for use with "bbbserver.c"
 *    - implementations of server logging functions
 *    - documentation/usage notes in "serverlog.h"
 *    - constants defined in "serverlog.h"
 */

#include "serverlog.h"

void log_msg(char* msg) {
  printf("%s\n", msg);
}

void log_main(char* msg, int c) {
  char buf[MAX_PRINT_LEN];
  bzero(buf, MAX_PRINT_LEN);
  sprintf(buf, "(m:%d)", c);
  server_log(msg, buf);
}

void log_thr(char* msg, int c, int t) {
  char buf[MAX_PRINT_LEN];
  bzero(buf, MAX_PRINT_LEN);
  sprintf(buf, "(m:%d)(t:%d)", c, t);
  server_log(msg, buf);
}

void log_req(char* msg, int c, int t, int r) {
  char buf[MAX_PRINT_LEN];
  bzero(buf, MAX_PRINT_LEN);
  sprintf(buf, "(m:%d)(t:%d)(r:%d)", c, t, r);
  server_log(msg, buf);
}

void server_log(char* message, char* db) {
  
  if(DB_LOG == 2) {
    char buf[MAX_PRINT_LEN];
    bzero(buf, MAX_PRINT_LEN);
    sprintf(buf, "db_%s: %s", db, message);
    printf("%s\n", buf);
  }
  if(DB_LOG == 1) {
    //sprintf(buf, "%s ", message);
    printf("%s\n", message);
  }
  //bzero(message, sizeof(message));
}