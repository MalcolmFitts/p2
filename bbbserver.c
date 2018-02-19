/*
 * bbbserver.c - The Server For Big Ballers
 * usage: ./bbbserver <port>
 * Authors: Malcolm Fitts (mfitts) & Sam Adams (sjadams)
 * Version: Project 2
 */

#include "datawriter.h"
#include "parser.h"

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

 
#define MAX_FILEPATH_LEN 512
#define MAXLINE 8192
#define BUFSIZE 1024
#define MAX_RANGE_NUM_LEN 64
#define SERVER_NAME "BBBserver"

/* debug flags for server log (LOG) and debugging log (DB_LOG)
 *    -set value 0 to turn off flag, 1 to turn on
 *    -DB_LOG overrides LOG flag 
 */
#define DB_LOG 1
#define LOG 1

struct cthread_data {
  struct sockaddr_in c_addr;  /* client address struct */
  int connfd;                 /* connection fd */
  int tid;                    /* thread id tag */
  int num;                    /* DEBUG - overall connected num */
};

/* Function prototype(s): */
void error(char *msg);
void server_error(char *msg, int connfd);
void* serve_client_thread(void *ptr);
void write_partial_content(int connfd, FILE* fp, char* fileExt, 
	int sb, int eb, int full_content_size, time_t last_modified);
void write_full_content(int connfd, FILE* fp, char* fileExt, 
	int content_size, time_t last_modified);


/* Global Variable(s): */
int numthreads; 


int main(int argc, char **argv) {
  int listenfd; /* listening socket */
  int connfd; /* connection socket */
  int portno; /* port to listen on */
  unsigned int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client's addr */
  int optval; /* flag value for setsockopt */

  /* check command line args */
  if (argc != 3) {
    fprintf(stderr, "usage: %s <port> <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /* socket: create a socket */
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd < 0)
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets
   * us rerun the server immediately after we kill it;
   * otherwise we have to wait about 20 secs.
   * Eliminates "ERROR on binding: Address already in use" error.
   */
  optval = 1;
  setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

  /* build the server's internet address */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET; /* we are using the Internet */
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); /* accept reqs to any IP addr */
  serveraddr.sin_port = htons((unsigned short)portno); /* port to listen on */

  /* bind: associate the listening socket with a port */
  if (bind(listenfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
    error("ERROR on binding");

  /* listen: make it a listening socket ready to accept connection requests */
  if (listen(listenfd, 10000) < 0) /* allow 5 requests to queue up */
    error("ERROR on listen");


  clientlen = sizeof(clientaddr);
  numthreads = 0;

  /* main loop: */
  int ctr = 1;
  while (1) {
    pthread_t tid;
    struct cthread_data *ct = malloc(sizeof(struct cthread_data));

    /* accept: wait for a connection request */
    if(DB_LOG) printf("db_(m:%d):  Waiting for a connection request... \n", ctr);
    else if(LOG) printf("Waiting for a connection request... \n");

    connfd = accept(listenfd, (struct sockaddr *) &clientaddr, &clientlen);

    if(DB_LOG) printf("db_(m:%d):  Establishing connection...\n", ctr);
    else if(LOG) printf("Establishing connection...\n");

    if (connfd < 0) { error("ERROR on accept"); }
      
    /* storing info in struct for use in thread */
    ct->c_addr = clientaddr;
    ct->connfd = connfd;
    ct->tid = numthreads + 1;
    ct->num = ctr;

    /* spin off thread */
    pthread_create(&(tid), NULL, serve_client_thread, ct);
    ctr++;
  }
}
     
  
void *serve_client_thread(void *ptr) {
  /* increment num threads */
  numthreads++;

  /* parse data back out of the ptr*/
  struct cthread_data *ct = ptr;
  struct sockaddr_in clientaddr = ct->c_addr;
  int connfd = ct->connfd;
  int tid = ct->tid;
  
  /* defining local vars */
  char content_filepath[MAX_FILEPATH_LEN] = "./content";
  char path[MAXLINE];
  struct hostent *hostp; /* client host info */
  char *hostaddrp; /* dotted decimal host addr string */
  char buf[BUFSIZE]; /* message buffer */
  char bufcopy[BUFSIZE]; /* copy of message buffer */
  int n; /* message byte size */
  struct stat *fStat;
  int content_size;
  time_t last_modified;

  char sbyte[MAX_RANGE_NUM_LEN]; /* holders for range request bytes */
  char ebyte[MAX_RANGE_NUM_LEN]; /* holders for range request bytes */
  int sb, eb;

  /* think this ensures good memory usage ... 
     look into pthread_detach further */
  pthread_detach(pthread_self());

  /* gethostbyaddr: determine who sent the message */
  hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
        sizeof(clientaddr.sin_addr.s_addr), AF_INET);
  if (hostp == NULL) { 
    numthreads--;
    server_error("ERROR on gethostbyaddr", connfd); 
  }
  
  hostaddrp = inet_ntoa(clientaddr.sin_addr);
  if (hostaddrp == NULL) { 
    numthreads--;
    server_error("ERROR on inet_ntoa\n", connfd); 
  }
    
  if(DB_LOG) {
    printf("db_(m:%d)(t:%d):  Server established connection with %s (%s)\n", 
      ct->num, tid, hostp->h_name, hostaddrp);
    printf("db_(m:%d)(t:%d):  Connected client id: %d\n", ct->num, tid, tid);
  }
  else if(LOG) { 
    printf("Server established connection with %s (%s)\n", hostp->h_name, hostaddrp);
    printf("Connected client id: %d\n", tid);
  }

  /* read: read input string from the client */
  bzero(buf, BUFSIZE);
  n = read(connfd, buf, BUFSIZE);
  if (n < 0) { 
    numthreads--;
    server_error("ERROR reading from socket", connfd); 
  }

  if(DB_LOG) {
    printf("db_(m:%d)(t:%d)(r:%d):  Server received %d Bytes\n", 
      ct->num, tid, reqnum, n);
    printf("db_(m:%d)(t:%d)(r:%d):  GET Request Raw Headers:\n%s", 
      ct->num, tid, reqnum, buf);
  }
  else if(LOG) {
    printf("Server received %d Bytes\n", n);
    printf("GET Request Raw Headers:\n%s", buf);
  }

  /* making copy of buffer to check for range requests */
  bzero(bufcopy, BUFSIZE);
  strcpy(bufcopy, buf);
  
  /* parsing get request */
  bzero(path, MAXLINE);
  if(parse_get_request(buf, path) == 0) {
    error("ERROR Parsing get request");
  }

  /* getting image file and checking for existence */
  char* fname = strcat(content_filepath, path);
  FILE *fp = fopen(fname, "r");

  if(fp == NULL) { 
    /* this is a 404 status code */
    printf("File (%s) not found\n", fname);
    write_headers_404(connfd, SERVER_NAME);
  }
  else{
    if(DB_LOG)
      printf("db_(m:%d)(t:%d)(r:%d):  Found file: %s\n",
        ct->num, tid, reqnum, fname);
    else if(LOG)
      printf("Found file: %s\n", fname);

    /* getting file size and last modified time */
    fStat = malloc(sizeof(struct stat));
    stat(fname, fStat);
    content_size = fStat->st_size;
    last_modified = fStat->st_mtime;
    free(fStat);

    /* getting file type (extension) */
    char fileExt[MAXLINE] = {0};
    if(parse_file_type(path, fileExt) == 0) {
      error("File path error");
    }

    /* range_flag value (parse_range_request return val):
     *    2 --> valid range request, the end byte was sent with range request
     *	  1 --> range request only has start byte, the end byte was NOT sent with request
     *            so default range to EOF 
     *    0 --> not a valid range request */
    int range_flag = parse_range_request(bufcopy, sbyte, ebyte);

  	if(range_flag > 0) {
      /* this is a 206 status code */
  	  if(DB_LOG)
        printf("db_(m:%d)(t:%d)(r:%d):  Sending partial content to {Client %d}...\n",
          ct->num, tid, reqnum, tid);
      else if(LOG)
        printf("Sending partial content to {Client %d}...\n", tid);
  	  
      /* assigning values for start and end bytes based on range flag */
      sb = atoi(sbyte);
      if(range_flag == 2) { eb = atoi(ebyte); }
  	  else                { eb = content_size - 1; }

  	  write_partial_content(connfd, fp, fileExt, sb, eb,
  	  	content_size, last_modified);

  	  int sent = eb - sb + 1;
  	  if(DB_LOG)
        printf("db_(m:%d)(t:%d)(r:%d):  Sent %d bytes to {Client %d}!\n", 
          ct->num, tid, reqnum, sent, tid);
      else if(LOG)
        printf("Sent %d bytes to {Client %d}!\n", sent, tid);
  	}
  	else {
      /* this is a 200 status code */
  	  if(DB_LOG)
        printf("db_(m:%d)(t:%d)(r:%d):  Sending content to {Client %d}...\n", 
          ct->num, tid, reqnum, tid);
      else if(LOG)
        printf("Sending content to {Client %d}...\n", tid);
  	  
  	  write_full_content(connfd, fp, fileExt,
  	  	content_size, last_modified);

  	  if(DB_LOG)
        printf("db_(m:%d)(t:%d)(r:%d):  Sent %d bytes to {Client %d}!\n", 
          ct->num, tid, reqnum, content_size, tid);
      else if(LOG)
        printf("Sent %d bytes to {Client %d}!\n", content_size, tid);
  	}
    
    /* closing file*/
    fclose(fp);
  }

  /* closing client connection and freeing struct */
  close(connfd);
  free(ct);
  if(DB_LOG)
    printf("db_(m:%d)(t:%d):  Connection with {Client %d} closed.\n",
      ct->num, tid, tid);
  else if(LOG) 
    printf("Connection with {Client %d} closed.\n", tid);
  
  /* decrement num threads and return */
  numthreads--;
  return NULL;
}

/*
 * error - wrapper for perror 
 *       - where we will handle 500 error codes
 */

void error(char *msg) {
  perror(msg);
  exit(1);
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

void write_partial_content(int connfd, FILE* fp, char* fileExt, 
	int sb, int eb, int full_content_size, time_t last_modified) {

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




