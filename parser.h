#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define MAXLINE 8192

int parse_get_request(char* buf, char *path);
int parse_file_type(char* filepath, char* buf);
int parse_range_request(char* buf, char* start_bytes, 
  char* end_bytes);
int parse_peer_add(char* buf, char* filepath, char* ip_hostname, 
  int* port, int* rate);
int parse_peer_view_content(char* buf, char* filepath);
int parse_peer_config_rate(char* buf, int* rate);
