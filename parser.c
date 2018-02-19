#include "parser.h"

/*                WANT TO CHANGE RETURN VALUES 
 * parse_get_request
 *    - parses requested file path from HTTP/1.1 GET request
 *		- stores file path in path ptr
 *		- returns 0 on success, -1 on fail
 */
int parse_get_request(char* buf, char *path) {
  char extrabuf[MAXLINE];

  int numscanned = sscanf (buf, "GET %s HTTP/1.1%s", path, extrabuf);
  if (numscanned != 2) {
    printf("500: Malformed Request. Server has recieved a malformed request.\n");
    return -1;
  }

  int len = sprintf (buf, "GET %s HTTP/1.1\r\n", path);
  if(len >= MAXLINE) {
    printf("500: Malformed Request. Server has recieved a malformed request.\n");
    return -1;
  }
  return 0;
}



/*                WANT TO CHANGE RETURN VALUES 
 * parse_file_type - parses file type from a full file path
 *    - stores file type in buf
 *    - returns 0 on success, -1 on fail
 */
int parse_file_type(char* filepath, char* buf) {
  char garbage[MAXLINE];
  if(sscanf(filepath, "%[^.].%s", garbage, buf) != 2) {
    return -1;
  }
  return 0;
}

/*
 * parse_range_request 
 *    - parses range request from full HTTP/1.1 GET request buffer
 *    - stores start and end bytes in char pointers
 *    - returns 1 on full success, i.e. parsed start and end bytes
 *    - return 0 on partial success, i.e. parsed start bytes
 *    - return -1 on fail
 */
int parse_range_request(char* buf, char* start_bytes, char* end_bytes) {
  char *rangeptr;
  rangeptr = strstr(buf, "Range:");

  if(rangeptr) {
    char extrabuf[MAXLINE];
    int sb = -1;
    int eb = -1;
    int scanned = sscanf(rangeptr, "Range: bytes=%d-%d%s", &sb, &eb, extrabuf);

    sprintf(start_bytes, "%d", sb);
    sprintf(end_bytes, "%d", eb);

    if((scanned == 2 || scanned == 1) && eb == -1) return 0;
    
    if(scanned == 0) return -1;
  }
  else { return -1; }

  return 1;
}

/*                    UNTESTED!!
 *  parse_peer_add
 *      - parse peer add requests for back-end functionality
 *      - stores filename, ip/hostname, port and rate in ptrs 
 *      - return 1 on success, 0 on fail
 */
int parse_peer_add(char* buf, char* filepath, char* ip_hostname,
    int* port, int* rate) {

  char extrabuf[MAXLINE], portbuf[MAXLINE];
  int n = sscanf(buf, "GET /peer/add?path=%[^&]&host=%[^&]&port=%[^&]&rate=%d %s",
            filepath, ip_hostname, portbuf, *rate, extrabuf);

  if(n != 5) { /* not a peer add request */
    return 0;
  }

  *port = atoi(portbuf);
  return 1;
}

/*                    UNTESTED!!
 *  parse_peer_view_content
 *      - parse peer view requests for back-end functionality
 *      - stores filepath in ptr
 *      - returns 1 on success, 0 on fail
 */
int parse_peer_view_content(char* buf, char* filepath) {
  char extrabuf[MAXLINE];
  if(sscanf(buf, "GET /peer/view/%s %s", filepath, extrabuf) != 2) {
    return 0;
  }
  return 1;
}

/*                    UNTESTED!!
 *  parse_peer_config_rate
 *      - parse configure back-end transfer rate requests for peer nodes
 *      - stores rate in ptr
 *      - returns 1 on success, 0 on fail
 */
int parse_peer_config_rate(char* buf, int* rate) {
  char extrabuf[MAXLINE];
  if(sscanf(buf, "GET /peer/config?rate=%d %s", *rate, extrabuf) != 2) {
    return 0;
  }
  return 1;
}