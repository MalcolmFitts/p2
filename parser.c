/*  (Version: p2)
 *  parser.c 
 *    - created for use with "bbbserver.c"
 *    - implementations of string parsing functions
 *    - documentation/usage notes in "parser.h"
 *    - constants defined in "parser.h"
 */

#include "parser.h"

int check_request_type(char* buf) {
  char b1[MINLINE], b2[MINLINE], b3[MINLINE];
  char b4[MINLINE], b5[MINLINE], b6[MINLINE];
  char b7[MINLINE], b8[MINLINE], b9[MINLINE];

  if(parse_get_request(buf, b1)) {
    if(parse_peer_add(buf, b2, b3, b4, b5)) {
      return RQT_P_ADD;     /*          Peer node ADD Request */
    }
    else if(parse_peer_view_content(buf, b6)) {
      return RQT_P_VIEW;    /*         Peer node VIEW Request */
    }
    else if(parse_peer_config_rate(buf, b7)) {
      return RQT_P_RATE;    /*  Peer node CONFIG RATE Request */
    }
    else if(parse_range_request(buf, b8, b9)) {
      return RQT_C_RNG;     /*           Client Range Request */
    }
    else{
      return RQT_GET;       /*              Valid GET Request */
    }
  }
  return RQT_INV;           /*            Invalid GET Request */
}


int parse_get_request(char* buf, char *path) {
  char extrabuf[MAXLINE];
  char bufcopy[MAXLINE];
  strcpy(bufcopy, buf);

  int numscanned = sscanf (buf, "GET %s HTTP/1.1%s", path, extrabuf);
  if (numscanned != 2) {
    printf("{500} Server has recieved a malformed request.\n");
    return 0;
  }

  int len = sprintf (bufcopy, "GET %s HTTP/1.1\r\n", path);
  if(len >= MAXLINE) {
    printf("{500} Server has recieved a malformed request.\n");
    return 0;
  }
  return 1;
}


int parse_file_type(char* filepath, char* buf) {
  char garbage[MAXLINE];
  if(sscanf(filepath, "%[^.].%s", garbage, buf) != 2) {
    return 0;
  }
  return 1;
}


int parse_range_request(char* buf, char* start_bytes, char* end_bytes) {
  char *rangeptr;
  
  char bufcopy[MAXLINE];
  strcpy(bufcopy, buf);

  rangeptr = strstr(bufcopy, "Range:");

  if(rangeptr) {
    char extrabuf[MAXLINE];
    int sb = -1;
    int eb = -1;
    int scanned = sscanf(rangeptr, "Range: bytes=%d-%d%s", &sb, &eb, extrabuf);

    sprintf(start_bytes, "%d", sb);
    sprintf(end_bytes, "%d", eb);

    if((scanned == 2 || scanned == 1) && eb == -1) return 1;
    
    if(scanned == 0) return 0;
  }
  else {
    return 0;
  }

  return 2;
}


int parse_peer_add(char* buf, char* fp, char* ip_hostname, char* port, char* rate) {
  char extrabuf[MAXLINE];

  int n = sscanf(buf, "GET /peer/add?path=%[^&]&host=%[^&]&port=%[^&]&rate=%s %s",
    fp, ip_hostname, port, rate, extrabuf);

  /* not a peer add request */
  if(n != 5) return 0;
  
  return 1;
}


int parse_peer_view_content(char* buf, char* filepath) {
  char extrabuf[MAXLINE];

  if(sscanf(buf, "GET /peer/view/%s %s", filepath, extrabuf) != 2) {
    return 0;
  }
  return 1;
}


int parse_peer_config_rate(char* buf, char* rate) {
  char extrabuf[MAXLINE];

  if(sscanf(buf, "GET /peer/config?rate=%s %s", rate, extrabuf) != 2) {
    return 0;
  }
  return 1;
}

int parse_str_2_int(char* str){
  int i = 0;
  int d = 0;
  int x = 0;
  int neg = 1;
  char c = str[i];

  if (!isdigit(c) && c != '-'){
    return -1;
  }

  if(c == '-'){
    neg = -1;
    i ++;
    c = str[i];
  }

  while(isdigit(c)){
    d = c - '0';
    x = (x * 10) + d;
    i ++;
    c = str[i];
  }

  return x * neg;
}
