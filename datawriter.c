
#include "datawriter.h"

/*  other headers to implement:

Last-Modified: Wed, 22 Jul 2009 19:15:56 GMT

*/

/*      Status code headers
 * ex: 
 *	HTTP/1.1 200 OK
 */
void write_status_header(int fd, char* rc, char* def){
  char buf[SHORT_HEADER_LEN];
  sprintf(buf, "HTTP/1.1 %s %s\r\n", rc, def);
  
  //sending to client
  write(fd, buf, strlen(buf));
}


/*	Date Headers
 * ex: 
 *      Date: Mon, 27 Jul 2009 12:28:53 GMT
 */
void write_date_header(int fd) {
  //getting GMT time
  time_t t = time(0);
  struct tm *timeptr = gmtime(&t);
  
  //formatting raw time
  char raw_date[SHORT_HEADER_LEN];
  strftime(raw_date, MAX_HEADER_LEN, "%a, %d %b %Y %H:%M:%S %Z", timeptr);
  
  //formatting header
  char buf[SHORT_HEADER_LEN];
  sprintf(buf, "Date: %s\r\n", raw_date);
  
  write(fd, buf, strlen(buf));
}


/* 	Server Name Headers
 * ex: 
 *	Server: Apache/2.2.14 (Win32)
 */
void write_server_name_header(int fd, char* sn) {
  char buf[SHORT_HEADER_LEN];
  sprintf(buf, "Server: %s\r\n", sn);
  
  write(fd, buf, strlen(buf));
}


/*	Connection Type Headers
 * ex: 
 *	Connection: Close
 */
void write_conn_header(int fd, char* conn_type) {
  char buf[SHORT_HEADER_LEN];
  sprintf(buf, "Connection: %s\r\n", conn_type);
  
  write(fd, buf, strlen(buf));
}

/* 	Connection Keep Alive Headers
 * ex: 
 *	Keep-Alive: timeout=3, max=100
 */
void write_keep_alive_header(int fd, int t, int m) {
  char buf[SHORT_HEADER_LEN];
  sprintf(buf, "Keep-Alive: timeout=%d, max=%d\r\n", t, m);
  
  write(fd, buf, strlen(buf));
}

/*	Empty Line Header
 */
void write_empty_header(int fd) {
  char *buf = "\r\n";
  write(fd, buf, strlen(buf));
}

/*      Content Length Header
 * ex:
 *      Content-Length: 88
 */

void write_content_length_header(int fd, int content_len) {
  char buf[SHORT_HEADER_LEN];
  sprintf(buf, "Content-Length: %d\r\n", content_len);

  write(fd, buf, strlen(buf));
}

/*      Content Type Header
 * ex:
 *      Content-Type: text/html
 */
    
void write_content_type_header(int fd, char *content_type){
  char buf[SHORT_HEADER_LEN];
  //mapping extension to header string and then writing header
  if(strcmp(content_type, "txt") == 0) {
    sprintf(buf, "Content-Type: %s\r\n", CONTENT_TYPE_TEXT_PLAIN);
  }
  else if(strcmp(content_type, "css") == 0) {
    sprintf(buf, "Content-Type: %s\r\n", CONTENT_TYPE_TEXT_CSS);
  }
  else if((strcmp(content_type, "htm") == 0) || 
          (strcmp(content_type, "html") == 0)) {
    sprintf(buf, "Content-Type: %s\r\n", CONTENT_TYPE_TEXT_HTML);
  }
  else if(strcmp(content_type, "gif") == 0) {
    sprintf(buf, "Content-Type: %s\r\n", CONTENT_TYPE_GIF);
  }
  else if((strcmp(content_type, "jpg") == 0) || 
          (strcmp(content_type, "jpeg") == 0)) {
    sprintf(buf, "Content-Type: %s\r\n", CONTENT_TYPE_JPEG);
  }
  else if(strcmp(content_type, "png") == 0) {
    sprintf(buf, "Content-Type: %s\r\n", CONTENT_TYPE_PNG);
  }
  else if(strcmp(content_type, "js") == 0) {
    sprintf(buf, "Content-Type: %s\r\n", CONTENT_TYPE_JS_APP);
  }
  else if(strcmp(content_type, "mp4") == 0) {
    sprintf(buf, "Content-Type: %s\r\n", CONTENT_TYPE_VIDEO_MP4);
  }
  else if(strcmp(content_type, "webm") == 0) {
    sprintf(buf, "Content-Type: %s\r\n", CONTENT_TYPE_VIDEO_WEBM);
  }
  else if(strcmp(content_type, "ogg") == 0) {
    sprintf(buf, "Content-Type: %s\r\n", CONTENT_TYPE_VIDEO_OGG);
  }
  else {
    sprintf(buf, "Content-Type: %s\r\n", CONTENT_TYPE_DEFAULT);
  }
  
  write(fd, buf, strlen(buf));
}


/*
 *        Last Modified Header
 * ex:
 *        Last-Modified: Wed, 15 Nov 1995 04:58:08 GMT
 */
void write_last_modified_header(int fd, time_t t){
  struct tm *timeptr = gmtime(&t);
  
  //formatting raw time
  char raw_date[SHORT_HEADER_LEN];
  strftime(raw_date, MAX_HEADER_LEN, "%a, %d %b %Y %H:%M:%S %Z", timeptr);
  
  //formatting header
  char buf[SHORT_HEADER_LEN];
  sprintf(buf, "Last-Modified: %s\r\n", raw_date);
  
  write(fd, buf, strlen(buf));
}

/*
 *        Content Range Header
 * ex:
 *        Content-Range: bytes 21010-47021/47022
 */
void write_content_range_header(int fd, int start_bytes, 
      int end_bytes, int content_length) {
  char buf[SHORT_HEADER_LEN];

  sprintf(buf, "Content-Range: bytes %d-%d/%d\r\n",
    start_bytes, end_bytes, content_length);
  
  write(fd, buf, strlen(buf));
}

/*
 *        Accept Ranges Header
 * ex:
 *       Accept-Ranges: bytes
 */
void write_accept_ranges_header(int fd) {
  char buf[SHORT_HEADER_LEN];
  sprintf(buf, "Accept-Ranges: bytes\r\n");
  
  write(fd, buf, strlen(buf));
}

/*
 * sending headers for "ok" status code (200)
 *
 */
void write_headers_200(int connfd, char* name, int content_length, char* content_type,
      time_t t) {
  write_status_header(connfd, SC_OK, ST_OK);
  write_date_header(connfd);
  write_server_name_header(connfd, name);
  write_conn_header(connfd, CONN_KEEP_HDR);
  write_keep_alive_header(connfd, 0, 100);
  write_last_modified_header(connfd, t);
  write_accept_ranges_header(connfd);
  write_content_length_header(connfd, content_length);
  write_content_type_header(connfd, content_type);
  write_empty_header(connfd);
}

/*
 * sending headers for "Not Found" status code (404)
 *
 */ 
void write_headers_404(int connfd, char* name) {
  write_status_header(connfd, SC_NOT_FOUND, ST_NOT_FOUND);
  write_date_header(connfd);
  write_server_name_header(connfd, name);
  write_conn_header(connfd, CONN_CLOSE_HDR);
  write_empty_header(connfd);
}

/*
 * sending headers for "Partial Content" status code (206)
 *
 */ 
void write_headers_206(int connfd, char* name, int full_length, char* content_type,
      time_t t, int sb, int eb, int content_length) {
  write_status_header(connfd, SC_PARTIAL, ST_PARTIAL);
  write_date_header(connfd);
  write_server_name_header(connfd, name);
  write_last_modified_header(connfd, t);
  write_content_range_header(connfd, sb, eb, full_length);
  write_content_type_header(connfd, content_type);
  write_content_length_header(connfd, content_length);
  write_empty_header(connfd);
}

/*
 * sending headers for "Internal Server Error" status code (500)
 *
 */ 
void write_headers_500(int connfd) {
  write_status_header(connfd, SC_SERVER_ERROR, ST_SERVER_ERROR);
  write_empty_header(connfd);
}

/*
 * writes data to client connected to connfd from
 * file pointed to by fp where file's size is content_size
 */
void write_data(int connfd, FILE* fp, int content_size, long start_byte){
  rewind(fp);
  fseek(fp, start_byte, SEEK_SET);

  /* Reading file into a buffer of size BUFSIZE */
  int sent = 0;
  int remain_data = content_size;
  char buf[BUFSIZE] = {0};
  fread(buf, 1, BUFSIZE, fp);

  /* Sending the buffer to the client */
  while((sent = send(connfd, buf, BUFSIZE, 0) > 0)
      && (remain_data > 0)) {
    remain_data -= BUFSIZE;
    /* Reading BUFSIZE into the buffer again to be sent */
    fread(buf, 1, BUFSIZE, fp);
  }
}

void write_partial_content(int connfd, FILE* fp, char* fileExt, 
			   int sb, int eb, int full_content_size,
			   time_t last_modified) {

  /* getting size of requested content */
  int content_size = eb - sb + 1;
    
  /* writing headers and data */
  write_headers_206(connfd, SERVER_NAME, full_content_size, fileExt, 
		    last_modified, sb, eb, content_size);
  write_data(connfd, fp, content_size, (long) sb);
}

void write_full_content(int connfd, FILE* fp, char* fileExt, 
			int content_size, time_t last_modified) { 

  /* writing headers and data */
  write_headers_200(connfd, SERVER_NAME, content_size, fileExt, last_modified);
  write_data(connfd, fp, content_size, 0L);
}

/*
 * server_error - server wrapper for perror
 */
void server_error(char *msg, int connfd) {
  perror(msg);
  write_status_header(connfd, SC_SERVER_ERROR, ST_SERVER_ERROR);
  write_empty_header(connfd);
  exit(1);
}

/*
 * error - wrapper for perror 
 *       - where we will handle 500 error codes
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

