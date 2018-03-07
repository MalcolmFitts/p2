/**
 *     File: datawriters.h
 *
 *    Brief: Functions used to write from the server to the client.
 *
 *  Authors: Malcolm Fitts {mfitts@andrew.cmu.edu}
 *           Samuel Adams  {sjadams@andrew.cmu.edu}
 **/

#ifndef DATAWRITER_H
#define DATAWRITER_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define SHORT_HEADER_LEN 128
#define MAX_HEADER_LEN 1024
#define BUFSIZE 1024

#define SERVER_NAME "BBBserver"

#define SC_OK "200"
#define ST_OK "OK"

#define SC_PARTIAL "206"
#define ST_PARTIAL "Partial Content"

#define SC_NOT_FOUND "404"
#define ST_NOT_FOUND "Not Found"

#define SC_SERVER_ERROR "500"
#define ST_SERVER_ERROR "Internal Server Error"

#define CONN_KEEP_HDR "Keep-Alive"
#define CONN_CLOSE_HDR "Close"

#define CONTENT_TYPE_DEFAULT "application/octet-stream"
#define CONTENT_TYPE_TEXT_PLAIN "text/plain"
#define CONTENT_TYPE_TEXT_CSS "text/css"
#define CONTENT_TYPE_TEXT_HTML "text/html"
#define CONTENT_TYPE_GIF "image/gif"
#define CONTENT_TYPE_JPEG "image/jpeg"
#define CONTENT_TYPE_PNG "image/png"
#define CONTENT_TYPE_JS_APP "application/javascript"
#define CONTENT_TYPE_VIDEO_WEBM "video/webm"
#define CONTENT_TYPE_VIDEO_MP4 "video/mp4"
#define CONTENT_TYPE_VIDEO_OGG "video/ogg"

/*	  Status Headers
 *     
 * write_status_header( {fd} , {STATUS_CODE} , {DESCRIPTION} );
 * 
 * Output:  "HTTP/1.1 {STATUS_CODE} {DESCRIPTION}\r\n"
 */
void write_status_header(int fd, char* rc, char* def);


/*	  Date Headers
 *
 * write_date_header( {fd} );
 *
 * output:  "Date: Mon, 27 Jul 2009 12:28:53 GMT\r\n"
 */
void write_date_header(int fd);


/* 	  Server Name Headers
 *
 * write_server_name_header( {fd} , {SERVER_NAME} );
 * 
 * output: "Server: {SERVER_NAME}\r\n"
 */
void write_server_name_header(int fd, char* sn);


/*	  Connection Type Headers
 *
 * write_conn_header( {fd} , {CONN_TYPE} );
 * 
 * output: "Connection: {CONN_TYPE}\r\n"
 */
void write_conn_header(int fd, char* conn_type);

/* 	  Connection Keep Alive Headers
 *
 * write_conn_header( {fd} , {TIMEOUT} , {MAX} ); 
 *	
 * output: "Keep-Alive: timeout={TIMEOUT}, max={MAX}\r\n
 */
void write_keep_alive_header(int fd, int t, int m);


/*        Empty Line Header
 *
 * write_empty_header( {fd} ); 
 *
 * output: "\r\n"
 */
void write_empty_header(int fd);


/*        Content Length Header
 * 
 * write_content_length_header( {fd} , {CONTENT_LEN} );
 *
 * output: "Content-Length: {CONTENT_LEN}\r\n"
 */
void write_content_length_header(int fd, int content_len);


/*
 *        Last Modified Header
 *
 * write_last_modified_header( {fd} , {TIME} );
 *
 * output: "Last-Modified: Wed, 21 Oct 2015 07:28:00 GMT\r\n"
 */

void write_last_modified_header(int fd, time_t t);


/*        Content Type Header
 * 
 * write_content_type_header( {fd}, {CONTENT_TYPE} );
 * 
 * output: "Content-Type: "{CONTENT_TYPE}\r\n"
 */
void write_content_type_header(int fd, char* content_type);

/*        Content Range Header
 * 
 * write_content_range_header( {fd}, {start_bytes}, {end_bytes}, {content_length});
 * 
 * output: "Content-Range: bytes {start_bytes}-{end_bytes}/{content_length}\r\n"
 */
void write_content_range_header(int fd, int start_bytes, 
      int end_bytes, int content_length);

/*        Accept Ranges Header
 * 
 * write_accept_ranges_header( {fd} );
 * 
 * output: "Accept-Ranges: bytes\r\n"
 */
void write_accept_ranges_header(int fd);

/*
 *        Write headers to the client with "200 OK" Status Code
 */
void write_headers_200(int connfd, char* name, int content_length,
		       char* content_type, time_t t);

/*
 *        Write headers to the client with "404 Not Found" Status Code
 */
void write_headers_404(int connfd, char *name);

/*
 *        Write headers to the client with "206 Partial Content" Status Code
 * 	note:
 *			{full_length} is full size of content
 *			{content_length} is requested content size
 */
void write_headers_206(int connfd, char* name, int full_length, 
	char* content_type, time_t t, int sb, int eb, int content_length);

/*
 *        Write the data requested to the client.
 *	note:
 *			{start_byte} indicates where in the file this will start writing from
 */
void write_data(int connfd, FILE* fp, int content_size, long start_byte);


void write_partial_content(int connfd, FILE* fp, char* fileExt, 
			   int sb, int eb, int full_content_size,
			   time_t last_modified);

void write_full_content(int connfd, FILE* fp, char* fileExt, 
			int content_size, time_t last_modified);

/*
 * server_error - server wrapper for perror
 */
void server_error(char *msg, int connfd);

/*
 * error - wrapper for perror 
 *       - where we will handle 500 error codes
 */
void error(char *msg);

#endif
