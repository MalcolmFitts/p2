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
//   int sockfd_be;            /* back end listening socket */
//   int port_be;                /* back end port */
// };

/* function prototype */
void *serve_client_thread(void *ptr);

/* Global Variable(s): */
Node_Dir* node_dir;           /* Directory for node referencing   */
char lb[MAX_PRINT_LEN];       /* buffer for logging               */
int numthreads;               /* number of current threads        */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char **argv) {
  /* front-end (client) vars */
  int sockfd_fe;                    /* listening socket */
  short port_fe;                    /* client port to listen on */
  struct sockaddr_in* self_addr_fe; /* front end address */
  int connfd;                       /* connection socket */

  /* back-end (node) vars */
  int sockfd_be;                    /* listening socket */
  short port_be;                    /* back-end port to use */
  struct sockaddr_in* self_addr_be  /* back-end address */

  /* client vars */
  struct sockaddr_in clientaddr;               /* client's addr */
  unsigned int clientlen = sizeof(clientaddr); /* size of client's address */


  /* check command line args */
  if (argc != 3) {
    fprintf(stderr, "usage: %s <port> <port>\n", argv[0]);
    exit(1);
  }

  port_fe = atoi(argv[1]);
  port_be = atoi(argv[2]);

  printf("front-end port: %d\nback-end port: %d\n", port_fe, port_be);

  /* initialize front-end and back-end data */
  sockfd_fe = init_frontend(port_fe, self_addr_fe);
  sockfd_be = init_backend(port_be, self_addr_be);

  /* create node directory */
  node_dir = create_node_dir(MAX_NODES);

  /* spin-off thread for listening on back-end port and serving content */
  int* sock_ptr = &sockfd_be;
  pthread_t tid_be;
  pthread_create(&(tid_be), NULL, recieve_pkt, sock_ptr);

  /* initializing some local vars */
  numthreads = 0;
  int ctr = 1;

  /* main loop: */
  while (1) {
    sprintf(lb, "Waiting for a connection request...");
    log_main(lb, ctr);

    /* accept: wait for a connection request */
    connfd = accept(sockfd_fe, (struct sockaddr *) &clientaddr, &clientlen);

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
    ct->listenfd_be = sockfd_be;
    ct->port_be = port_be;

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

  int flag_be = 0;
  int flag_fe = 0;

  /* TODO: flag responses for peer add, view, and rate requests */

  switch(rqt){

    case RQT_P_ADD:
      /* This goes to backend */
      flag_be = peer_add_response(connfd, buf, ct, node_dir);

      if(flag_be){
        /* 200 Code  --> Success! */
        write_status_header(connfd, SC_OK, ST_OK);
        write_date_header(connfd);
        write_conn_header(connfd, CONN_KEEP_HDR);
        write_keep_alive_header(connfd, 0, 100);
        write_empty_header(connfd);
      } else /* flag_be < 1 */ {
        /* 500 Code  --> Failure on peer_add request */
        write_status_header(connfd, SC_SERVER_ERROR, ST_SERVER_ERROR);
        write_empty_header(connfd);
      }
      break;

    case RQT_P_VIEW:
      /* This goes to backend */
      char* filepath = malloc(sizeof(char) * MAXLINE);
      char* file_type = malloc(sizeof(char) * MINLINE);
      uint16_t port_be = ct->port_be;
      int sockfd_be = ct->listenfd_be;
      struct sockaddr_in client_addr = ct->c_addr;
      char* COM_BUF = malloc(sizeof(char) * BUFSIZE);

      memset(COM_BUF, '\0', BUFSIZE);

      if((!parse_peer_view_content(BUF, filepath))
         (!parse_file_type(filepath, file_type)) ||{
        /* 500 Error --> Failure to parse file type
         * TODO      --> flag (return) val: parse fail */
        write_status_header(connfd, SC_SERVER_ERROR, ST_SERVER_ERROR);
        write_empty_header(connfd);
        return 0;
      }
      flag_be = peer_view_response(filepath, file_type, port_be, sockfd_be,
                                   node_dir);
      /* couldn't find content */
      if(flag_be == 0){
        write_status_header(connfd, SC_NOT_FOUND, ST_NOT_FOUND);
        write_empty_header(connfd);
      } else if(flag_be == -1){
        /* ERROR on sendto (resend?)*/
      }
      handle_be_response(COM_BUF, connfd, file_type);
      break;

    case RQT_P_RATE:
      /* This goes to backend */
      flag_be = peer_rate_response(connfd, buf, ct);
      break;
    case RQT_INV:
      numthreads--;
      error("ERROR Invalid GET request");
      break;

    default:
      /* Standard FE response */
      flag_fe = frontend_response(connfd, buf, ct);
      break;
  }

  /* TODO: closing client connection and freeing struct */

  sprintf(lb, "{Client %d} request served.", tid);
  log_thr(lb, ct->num, tid);

  /* decrement num threads and return */
  numthreads--;
  return NULL;
}
