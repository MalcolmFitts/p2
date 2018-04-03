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
//   int sockfd_be;              /* back end listening socket */
//   int port_be;                /* back end port */
// };

pthread_mutex_t mutex;

/* function prototype */
void *serve_client_thread(void *ptr);

int init_port(unsigned short port, int flag);

char lb[MAX_PRINT_LEN];       /* buffer for logging               */
int numthreads;               /* number of current threads        */

int main(int argc, char **argv) {
  /* front-end (client) vars */
  int sockfd_fe;                    /* listening socket */
  short port_fe;                    /* client port to listen on */
  struct sockaddr_in self_addr_fe; /* front end address */
  int connfd;                       /* connection socket */

  /* back-end (node) vars */
  int sockfd_be;                    /* listening socket */
  short port_be;                    /* back-end port to use */
  struct sockaddr_in self_addr_be;;  /* back-end address */

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

  printf("<BBBServer start-up info>\n");
  // printf("front-end port: %d\nback-end port: %d\n", port_fe, port_be);

  pthread_mutex_init(&mutex, NULL);

  /* initialize front-end and back-end data */
  sockfd_fe = init_frontend(port_fe, &self_addr_fe);
  sockfd_be = init_backend(port_be, &self_addr_be);

  printf("\n");

  int* be_sockfd_ptr = &sockfd_be;

  /* spin-off thread for listening on back-end port and serving content */
  pthread_t tid_be;
  pthread_create(&(tid_be), NULL, handle_be, be_sockfd_ptr);

  /* initializing some local vars */
  numthreads = 0;
  int ctr = 1;

  /* main loop: */
  while (1) {
    printf("Waiting for a client connection request...\n");
    // sprintf(lb, "Waiting for a connection request...");
    // log_main(lb, ctr);

    /* accept: wait for a connection request */
    connfd = accept(sockfd_fe, (struct sockaddr *) &clientaddr, &clientlen);

    printf("Establishing connection...\n");
    // sprintf(lb, "Establishing connection...");
    // log_main(lb, ctr);

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

  char* filepath;
  char* file_type;

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

  printf("Server established connection with %s (%s)\n",
          hostp->h_name, hostaddrp);
  printf("Connected client id: %d.\n", tid);

  // sprintf(lb, "Server established connection with %s (%s)",
  //         hostp->h_name, hostaddrp);
  // log_thr(lb, ct->num, tid);
  // sprintf(lb, "Connected client id: %d.", tid);
  // log_thr(lb, ct->num, tid);

  /* read: read input string from the client */
  bzero(buf, BUFSIZE);
  n = read(connfd, buf, BUFSIZE);
  if (n < 0) {
    numthreads--;
    server_error("ERROR reading from socket", connfd);
  }

  /* print formatting stuff - not important */
  printf("Server received %d Bytes.\n", n);
  printf("Get Request Raw Headers:\n    <start>\n");
  bzero(bufcopy, BUFSIZE);
  strncpy(bufcopy, buf, n - 2);
  printf("%s    <end>\n\n", bufcopy);

  // sprintf(lb, "Server received %d Bytes", n);
  // log_thr(lb, ct->num, tid);
  // sprintf(lb, "Get Request Raw Headers:");
  // log_thr(lb, ct->num, tid);
  // log_msg(buf);

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
      printf("Server recognized request type: peer add\n");
      flag_be = peer_add_response(connfd, buf, ct);

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
      printf("Server recognized request type: peer view\n");
      filepath = malloc(sizeof(char) * MAXLINE);
      file_type = malloc(sizeof(char) * MINLINE);
      uint16_t port_be = ct->port_be;
      int sockfd_be = ct->listenfd_be;
      char* COM_BUF = malloc(sizeof(char) * BUFSIZE);

      memset(COM_BUF, '\0', BUFSIZE);

      if((!parse_peer_view_content(bufcopy, filepath)) ||
         (!parse_file_type(filepath, file_type))) {
        /* 500 Error --> Failure to parse file type
         * TODO      --> flag (return) val: parse fail */
        write_status_header(connfd, SC_SERVER_ERROR, ST_SERVER_ERROR);
        write_empty_header(connfd);
        return 0;
      }

      flag_be = peer_view_response(filepath, file_type, port_be, sockfd_be, (COM_BUF));

      /* couldn't find content */
      if(flag_be == 0) {
        write_status_header(connfd, SC_NOT_FOUND, ST_NOT_FOUND);
        write_empty_header(connfd);
      }
      else if(flag_be == -1) {
        /* ERROR on sendto (resend?)*/
      }
      handle_be_response(COM_BUF, connfd, file_type);
      break;

    case RQT_P_RATE:
      printf("Server recognized request type: peer rate\n");
      /* This goes to backend */
      flag_be = peer_rate_response(connfd, buf, ct);
      break;
    case RQT_INV:
      numthreads--;
      error("ERROR Invalid GET request");
      break;

    default:
      /* Standard FE response */
      printf("Server recognized request type: front-end request\n");
      flag_fe = frontend_response(connfd, buf, ct);
      break;
  }

  /* TODO: closing client connection and freeing struct */

  log_thr(lb, ct->num, tid);

  /* decrement num threads and return */
  numthreads--;
  return NULL;
}

int init_port(unsigned short port, int flag) {
  int optval;
  int sockfd;
  struct sockaddr_in addr;
  socklen_t lenaddr;

  if(!flag){
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  } else {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
  }
  if (sockfd < 0)
    error("ERROR opening socket");

 /*
  *  setsockopt: Handy debugging trick that lets
  *              us rerun the server immediately after we kill it;
  *              otherwise we have to wait about 20 secs.
  *              Eliminates "ERROR on binding: Address already in use" error.
  */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
            (const void *)&optval , sizeof(int));

  bzero((char *) &addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);
  lenaddr = sizeof(addr);

  if (bind(sockfd, (struct sockaddr *) &addr, lenaddr) < 0)
    error("ERROR biding");

  if (flag){
    if (listen(sockfd, 10) < 0) /* allow 10 requests to queue up */
      error("ERROR on listen");
  }

  return sockfd;
}
