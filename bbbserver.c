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
#include "frontend.h"

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


/* DEFINED IN backend.h -- struct to hold data for initial threads */
// struct thread_data {
//   struct sockaddr_in c_addr;  /* client address struct */
//   int connfd;                 /* connection fd */
//   int tid;                    /* thread id tag */
//   int num;                    /* DEBUG - overall connected num */
//   int listenfd_be;            /* back end listening socket */
//   int port_be;                /* back end port */
// };

/* function prototype */
void *serve_client_thread(void *ptr);

/* Global Variable(s): */
Node_Dir* node_dir;           /* Directory for node referencing   */
char lb[MAX_PRINT_LEN];       /* buffer for logging               */
int numthreads;               /* number of current threads        */
/* -- TODO/CHECK might want to distinguish front-end and back-end threads */

int main(int argc, char **argv) {
  /* front-end (client) vars */
  int listenfd_fe;                  /* listening socket */
  int connfd;                       /* connection socket */
  int portno_fe;                    /* client port to listen on */
  struct sockaddr_in *serveraddr_fe = malloc(sizeof(struct sockaddr_in)); /* server's front-end addr */
  
  /* back-end (node) vars */
  int listenfd_be;                  /* listening socket */
  int portno_be;                    /* back-end port to use */
  struct sockaddr_in *serveraddr_be = malloc(sizeof(struct sockaddr_in)); /* server's back-end addr */
  
  /* client vars */
  unsigned int clientlen;           /* byte size of client's address */
  struct sockaddr_in clientaddr;    /* client's addr */

  /* check command line args */
  if (argc != 3) {
    fprintf(stderr, "usage: %s <port> <port>\n", argv[0]);
    exit(1);
  }
  portno_fe = atoi(argv[1]);
  portno_be = atoi(argv[2]);

  /* initialize front-end and back-end data */
  listenfd_fe = init_frontend(serveraddr_fe, portno_fe);
  listenfd_be = init_backend(serveraddr_be, portno_be);

  /* create node directory */
  node_dir = create_node_dir(MAX_NODES);
  
  /* allocating memory for data for thread */
  Recv_t* ptr = malloc(sizeof(Recv_t));
  ptr->sockfd = listenfd_be;
  ptr->serveraddr = serveraddr_be;

  /* spin-off thread for listening on back-end port and serving content */
  pthread_t tid_be;
  pthread_create(&(tid_be), NULL, recieve_pkt, ptr);
  pthread_detach(tid_be);

  /* initializing some local vars */
  clientlen = sizeof(clientaddr);
  numthreads = 1;
  int ctr = 1;

  /* main loop: */
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
  struct hostent *hostp;          /* client host info */
  char *hostaddrp;                /* dotted decimal host addr string */
  char buf[BUFSIZE];              /* message buffer */
  char bufcopy[BUFSIZE];          /* copy of message buffer */
  int n;                          /* message byte size */

  /* unused vars: */
  // int sb, eb;
  // char path[MAXLINE];
  // struct stat *fStat;
  // char sbyte[MAX_RANGE_NUM_LEN];  /* holders for range request bytes */
  // char ebyte[MAX_RANGE_NUM_LEN];  /* holders for range request bytes */
  // int content_size;               /* CHECK - should this be bigger? */
  // char content_filepath[MAX_FILEPATH_LEN] = "./content";
  // time_t last_modified;

  // char lb[MAX_PRINT_LEN];         /* buffer for logging */

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

  /* checking type of received request */
  int rqt = check_request_type(bufcopy);

  if(rqt == RQT_INV) {
    /* Invalid request type */
    numthreads--;
    error("ERROR Invalid GET request");
  }

  int flag_be = 0;
  int flag_fe = 0; 

  /* TODO 
   * - implement peer_add_response
   * - flag responses for peer add, view, and rate requests 
   */
  if(rqt == RQT_P_ADD) {
    /* back-end: Peer add request */
    flag_be = peer_add_response(connfd, buf, ct, node_dir);
  }

  else if(rqt == RQT_P_VIEW) {
    /* back-end: Peer view content request */
    flag_be = peer_view_response(connfd, buf, ct, node_dir);
  }

  else if(rqt == RQT_P_RATE) {
    /* back-end: Set content rate request */
    flag_be = peer_rate_response(connfd, buf, ct);
  }

  else {
    /* front-end: HTTP client request */
    flag_fe = frontend_response(connfd, buf, ct);
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
