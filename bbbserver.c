/*
 * bbbserver.c - The Server For Big Ballers
 * usage: ./bbbserver <port>
 * Authors: Malcolm Fitts (mfitts) & Sam Adams (sjadams)
 * Version: Project 2
 *
 *  (notes)
 * 1. CHECK   
 *      - put in comments, search file for this keyword
 *      - used to indicate code that should be reviewed
 * 2. TODO   
 *      - put in comments, search file for this keyword
 *      - used to indicate things to do
 *
 * 3. Server log
 *      - check file "serverlog.h" for info on server log (printing) 
 *  
 */

#include "datawriter.h"
#include "parser.h"
#include "serverlog.h"
#include "backend.h"

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
#define MAX_RANGE_NUM_LEN 32
#define SERVER_NAME "BBBserver"


// TODO - make this thread_data struct
//      - add fields for back-end data
struct thread_data {
  struct sockaddr_in c_addr;  /* client address struct */
  int connfd;                 /* connection fd */
  int tid;                    /* thread id tag */
  int num;                    /* DEBUG - overall connected num */
  int listenfd_be;            /* back end listening socket */
  int port_be;                /* back end port */
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
  /* front-end (client) vars */
  int listenfd_fe;                  /* listening socket */
  int connfd;                       /* connection socket */
  int portno_fe;                    /* client port to listen on */
  unsigned int clientlen;           /* byte size of client's address */
  struct sockaddr_in serveraddr_fe; /* server's front-end addr */
  struct sockaddr_in clientaddr;    /* client's addr */
  int optval_fe;                    /* flag value for setsockopt */

  /* back-end (node) vars */
  int listenfd_be;                  /* listening socket */
  int portno_be;                    /* back-end port to use */
  struct sockaddr_in serveraddr_be; /* server's back-end addr */
  int optval_be;                    /* flag value for setsockopt */

  // CHECK
  //unsigned int nodelen;             /* byte size of client's address */
  //struct sockaddr_in nodeaddr;      /* node's addr */

  char lb[MAX_PRINT_LEN];           /* buffer for logging */
  

  /* check command line args */
  if (argc != 3) {
    fprintf(stderr, "usage: %s <port> <port>\n", argv[0]);
    exit(1);
  }
  portno_fe = atoi(argv[1]);
  portno_be = atoi(argv[2]);

  /* socket: create front and back end sockets */
  listenfd_fe = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd_fe < 0)
    error("ERROR opening front-end socket");

  listenfd_be = socket(AF_INET, SOCK_DGRAM, 0);
  if(listenfd_be < 0)
    error("ERROR opening back-end socket");

  /* setsockopt: Handy debugging trick that lets
   * us rerun the server immediately after we kill it;
   * otherwise we have to wait about 20 secs.
   * Eliminates "ERROR on binding: Address already in use" error.
   */
  optval_fe = 1;
  optval_be = 1;
  setsockopt(listenfd_fe, SOL_SOCKET, SO_REUSEADDR, 
    (const void *)&optval_fe, sizeof(int));
  setsockopt(listenfd_be, SOL_SOCKET, SO_REUSEADDR, 
    (const void *)&optval_be, sizeof(int));

  /* build the server's front end internet address */
  bzero((char *) &serveraddr_fe, sizeof(serveraddr_fe));
  serveraddr_fe.sin_family = AF_INET; /* we are using the Internet */
  serveraddr_fe.sin_addr.s_addr = htonl(INADDR_ANY); /* accept reqs to any IP addr */
  serveraddr_fe.sin_port = htons((unsigned short)portno_fe); /* port to listen on */

  /* build the server's back end internet address */
  bzero((char *) &serveraddr_be, sizeof(serveraddr_be));
  serveraddr_be.sin_family = AF_INET; /* we are using the Internet */
  serveraddr_be.sin_addr.s_addr = htonl(INADDR_ANY); /* accept reqs to any IP addr */
  serveraddr_be.sin_port = htons((unsigned short)portno_be); /* port to listen on */

  /* bind: associate the listening socket with a port */
  if (bind(listenfd_fe, (struct sockaddr *) &serveraddr_fe, sizeof(serveraddr_fe)) < 0)
    error("ERROR on binding front-end socket with port");
  if (bind(listenfd_be, (struct sockaddr *) &serveraddr_be, sizeof(serveraddr_be)) < 0)
    error("ERROR on binding back-end socket with port");

  /* listen: make it a listening socket ready to accept connection requests 
   *         - only need to listen for front-end
   */
  if (listen(listenfd_fe, 10) < 0) /* allow 10 requests to queue up */
    error("ERROR on listen");


  clientlen = sizeof(clientaddr);
  // CHECK
  // nodelen = sizeof(nodeaddr);
  numthreads = 0;

  /* main loop: */
  int ctr = 1;
  while (1) {
    sprintf(lb, "Waiting for a connection request...");
    log_main(lb, ctr);

    /* accept: wait for a connection request */
    connfd = accept(listenfd_fe, (struct sockaddr *) &clientaddr, &clientlen);

    sprintf(lb, "Establishing connection...");
    log_main(lb, ctr);

    if (connfd < 0) { error("ERROR on accept"); }
      
    /* storing info in struct for use in thread */
    pthread_t tid;
    struct thread_data *ct = malloc(sizeof(struct thread_data));
    ct->c_addr = clientaddr;
    ct->connfd = connfd;
    ct->tid = numthreads + 1;
    ct->num = ctr;
    ct->listenfd_be = listenfd_be;
    ct->port_be = portno_be;

    /* spin off thread */
    pthread_create(&(tid), NULL, serve_client_thread, ct);
    ctr++;
  }
}
     
  
void *serve_client_thread(void *ptr) {
  /* increment num threads */
  numthreads++;

  /* parse data back out of the ptr*/
  struct thread_data *ct = ptr;
  struct sockaddr_in clientaddr = ct->c_addr;
  int connfd = ct->connfd;
  int tid = ct->tid;
  
  /* defining local vars */
  char content_filepath[MAX_FILEPATH_LEN] = "./content";
  char path[MAXLINE];
  struct hostent *hostp;          /* client host info */
  char *hostaddrp;                /* dotted decimal host addr string */
  char buf[BUFSIZE];              /* message buffer */
  char bufcopy[BUFSIZE];          /* copy of message buffer */
  int n;                          /* message byte size */
  struct stat *fStat;
  int content_size;  /* CHECK - should this be bigger? */
  time_t last_modified;

  char lb[MAX_PRINT_LEN];         /* buffer for logging */

  char sbyte[MAX_RANGE_NUM_LEN];  /* holders for range request bytes */
  char ebyte[MAX_RANGE_NUM_LEN];  /* holders for range request bytes */
  int sb, eb;

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

  sprintf(lb, "Server established connection with %s (%s)",
    hostp->h_name, hostaddrp);
  log_thr(lb, ct->num, tid);

  sprintf(lb, "Connected client id: %d.", tid);
  log_thr(lb, ct->num, tid);

  /* read: read input string from the client */
  bzero(buf, BUFSIZE);
  n = read(connfd, buf, BUFSIZE);
  if (n < 0) { 
    numthreads--;
    server_error("ERROR reading from socket", connfd); 
  }

  sprintf(lb, "Server received %d Bytes", n);
  log_thr(lb, ct->num, tid);
  sprintf(lb, "Get Request Raw Headers:");
  log_thr(lb, ct->num, tid);
  log_msg(buf);

  /* making copy of buffer to check for range requests */
  bzero(bufcopy, BUFSIZE);
  strcpy(bufcopy, buf);

  int rqt = check_request_type(bufcopy);

  if(rqt == RQT_INV) {
    numthreads--;
    error("ERROR Invalid GET request");
  }

  //  TODO
  if(rqt == RQT_P_ADD || rqt == RQT_P_VIEW || rqt == RQT_P_RATE) {
    /* back-end request
     * not yet implemented */
  }

  /* else this is a regular client request */

  bzero(bufcopy, BUFSIZE);
  strcpy(bufcopy, buf);
  
  /* parsing get request */
  bzero(path, MAXLINE);
  if(parse_get_request(buf, path) == 0) {
    numthreads--;
    error("ERROR Parsing get request");
  }

  /* getting image file and checking for existence */
  char* fname = strcat(content_filepath, path);
  FILE *fp = fopen(fname, "r");

  if(fp == NULL) { 
    /* this is a 404 status code */
    sprintf(lb, "File (%s) not found", fname);
    log_thr(lb, ct->num, tid);

    write_headers_404(connfd, SERVER_NAME);
  }
  else{
    sprintf(lb, "Found file: %s", fname);
    log_thr(lb, ct->num, tid);

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
      sprintf(lb, "Sending partial content to {Client %d}...", tid);
      log_thr(lb, ct->num, tid);
  	  
      /* assigning values for start and end bytes based on range flag */
      sb = atoi(sbyte);
      if(range_flag == 2) { eb = atoi(ebyte); }
  	  else                { eb = content_size - 1; }

  	  write_partial_content(connfd, fp, fileExt, sb, eb,
  	  	content_size, last_modified);

  	  int sent = eb - sb + 1;

      sprintf(lb, "Sent %d bytes to {Client %d}!", sent, tid);
      log_thr(lb, ct->num, tid);
  	}
  	else {
      /* this is a 200 status code */
      sprintf(lb, "Sending content to {Client %d}...", tid);
      log_thr(lb, ct->num, tid);
  	  
  	  write_full_content(connfd, fp, fileExt,
  	  	content_size, last_modified);

      sprintf(lb, "Sent %d bytes to {Client %d}!", content_size, tid);
      log_thr(lb, ct->num, tid);
  	}
    
    /* closing file*/
    fclose(fp);
  }

  /* closing client connection and freeing struct */
  close(connfd);
  free(ct);

  sprintf(lb, "Connection with {Client %d} closed.", tid);
  log_thr(lb, ct->num, tid);
  
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




