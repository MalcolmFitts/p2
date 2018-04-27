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
#include "configlib.h"
#include "neighbor.h"

//#include "spcuuid/src/uuid.h"
#include <uuid/uuid.h>

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

pthread_mutex_t mutex;

char lb[MAX_PRINT_LEN];       /* buffer for logging               */
int numthreads;               /* number of current threads        */

/* DEFINED IN backend.h -- struct to hold data for initial threads */
// struct thread_data {
//   struct sockaddr_in c_addr;  /* client address struct */
//   int connfd;                 /* connection fd */
//   int tid;                    /* thread id tag */
//   int num;                    /* DEBUG - overall connected num */
//   int sockfd_be;              /* back end listening socket */
//   int port_be;                /* back end port */
// };

/* function prototype */
void *serve_client_thread(void *ptr);

int main(int argc, char **argv) {
  /* front-end (client) vars */
  int sockfd_fe;                    /* listening socket */
  short port_fe;                    /* client port to listen on */
  struct sockaddr_in self_addr_fe; /* front end address */
  int connfd;                       /* connection socket */

  /* back-end (node) vars */
  int sockfd_be;                    /* listening socket */
  short port_be;                    /* back-end port to use */
  struct sockaddr_in self_addr_be;  /* back-end address */

  /* client vars */
  struct sockaddr_in clientaddr;               /* client's addr */
  unsigned int clientlen = sizeof(clientaddr); /* size of client's address */

  /* config file vars */
  char* config_filename;
  int config_init_flag;

  char* cf_port_fe = NULL;
  char* cf_port_be = NULL;

  /* check command line args */
  if (argc != 1 && argc != 3) {
    fprintf(stderr, "usage: %s [-c] <config file name>\n", argv[0]);
    exit(1);
  }

  if(argc == 3) {
    config_filename = argv[2];
    /* Validating given config file */
    config_init_flag = validate_config_file(config_filename);
    if(config_init_flag != 1) {
      /* Checking default config file if given invalid filename */
      config_init_flag = check_default_config_file();
      if(config_init_flag == 0) {
        printf("Error creating config file.\n");
        exit(0);
      }
      /* Created/Found default config file */
      config_filename = malloc(MAXLINE);
      sprintf(config_filename, "%s", CF_DEFAULT_FILENAME);
    }
  }

  else {
    config_init_flag = check_default_config_file();
    /* Not given config file - find/create default */
    if(config_init_flag == 0) {
      printf("Error creating config file.\n");
      exit(0);
    }
    config_filename = malloc(MAXLINE);
    sprintf(config_filename, "%s", CF_DEFAULT_FILENAME);
  }

  /* assigning front and backend ports */
  cf_port_fe = get_config_field(config_filename, CF_TAG_FE_PORT, 0);
  cf_port_be = get_config_field(config_filename, CF_TAG_BE_PORT, 0);

  if(cf_port_fe)  { port_fe = atoi(cf_port_fe);   }
  else            { port_fe = CF_DEFAULT_FE_PORT; }

  if(cf_port_be)  { port_be = atoi(cf_port_be);   }
  else            { port_be = CF_DEFAULT_BE_PORT; }

  pthread_mutex_init(&mutex, NULL);

  /* initialize front-end and back-end data */
  sockfd_fe = init_frontend(port_fe, &self_addr_fe);
  sockfd_be = init_backend(port_be, &self_addr_be);

  //printf("<BBBServer start-up info>\n\n");

  int* be_sockfd_ptr = &sockfd_be;

  /* spin-off thread for listening on back-end port and serving content */
  pthread_t tid_be;
  pthread_create(&(tid_be), NULL, handle_be, be_sockfd_ptr);

  //pthread_t tid_ad;
  //pthread_create(&(tid_ad), NULL, advertise, be_sockfd_ptr);

  /* initializing some local vars */
  numthreads = 0;
  int ctr = 1;

  /* main loop: */
  while (1) {
    printf("Waiting for a client connection request...\n");

    /* accept: wait for a connection request */
    connfd = accept(sockfd_fe, (struct sockaddr *) &clientaddr, &clientlen);

    printf("Establishing connection...\n");

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
    ct->config_fn = config_filename;

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
  char* resp_buf;

  int shutdown_flag = 0;         /* Flag for shutdown of client conn */
  int flag_be = 0;               /* Flag coming from peer functions */
  int rqt;                       /* Request Type */

  char* path = NULL;

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

  /* making copy of buffer to check for range requests */
  bzero(bufcopy, BUFSIZE);
  strcpy(bufcopy, buf);

  /* checking type of received request */
  rqt = check_request_type(bufcopy);
  printf("%d\n", rqt);

  switch(rqt){
    case RQT_P_ADD:

      printf("Server recognized request type: peer add\n");
      resp_buf = malloc(sizeof(char) * BUFSIZE);
      flag_be = peer_add_response(buf);


      if(flag_be == SERVER_ERROR) {
        write_headers_500(connfd);
      } else /* 200 Code  --> Success! */ {
        write_status_header(connfd, SC_OK, ST_OK);
        write_server_name_header(connfd, SERVER_NAME);
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
      char* COM_BUF = malloc(sizeof(char) * COM_BUFSIZE);

      memset(COM_BUF, '\0', COM_BUFSIZE);

      /* 500 Error --> Failure to parse file type */
      if((!parse_peer_view_content(bufcopy, filepath)) ||
         (!parse_file_type(filepath, file_type))) {
        write_headers_500(connfd);
        return 0;
      }

      flag_be = peer_view_response(filepath, file_type, port_be, sockfd_be, (COM_BUF));

      if(flag_be == FILE_NOT_FOUND) {
        write_headers_404(connfd, SERVER_NAME);
      } else if(flag_be == SERVER_ERROR) {
        write_headers_500(connfd);
      }

      handle_be_response(COM_BUF, connfd, file_type);
      break;

    case RQT_P_RATE:
      printf("Server recognized request type: peer rate\n");

      flag_be = peer_rate_response(connfd, buf, ct);

      write_status_header(connfd, SC_OK, ST_OK);
      write_empty_header(connfd);
      break;

    case RQT_P_ADD_UUID:
      /* TODO: Handle peer ADD UUID request */
      printf("Handling ADD UUID\n");
      flag_be = handle_add_uuid_rqt(bufcopy, ct->config_fn);

      if(flag_be == SERVER_ERROR) {
        write_headers_500(connfd);
      } else /* 200 Code  --> Success! */ {
        write_status_header(connfd, SC_OK, ST_OK);
        write_server_name_header(connfd, SERVER_NAME);
        write_empty_header(connfd);
      }
      break;

    case RQT_P_KILL:
      /* Handle peer KILL request */
      exit(EXIT_SUCCESS);
      break;

    case RQT_P_UUID:
      /* Handle peer UUID request*/
      handle_uuid_rqt(connfd, ct->config_fn);
      break;

    case RQT_P_NEIGH:
      /* TODO: Handle peer NEIGHBORS request */
      printf("Handling: PEER / NEIGHBOR\n");
      handle_neighbors_rqt(connfd, ct->config_fn);
      break;

    case RQT_P_ADD_NEIGH:
      /* TODO: Handle peer ADD NEIGHBOR request */
      handle_add_neighbor_rqt(bufcopy, ct->config_fn);
      write_status_header(connfd, SC_OK, ST_OK);
      write_empty_header(connfd);
      break;

    case RQT_P_MAP:
      /* TODO: Handle peer MAP request */
      break;

    case RQT_P_RANK:
      /* TODO: Hanlde peer RANK request */
      break;

    case RQT_P_SEARCH:
      path = malloc(sizeof(char) * MAXLINE);
      parse_peer_search(buf, path);
      handle_search_rqt(connfd, path, ct->config_fn);
      free(path);
      break;

    case RQT_INV:
      numthreads--;
      error("ERROR Invalid GET request");
      break;

    default:
      /* Standard FE response */
      printf("Server recognized request type: front-end request\n");
      frontend_response(connfd, buf, ct);
      break;
  }

  /* TODO: Make sure the client closes the connection */

  log_thr(lb, ct->num, tid);

  shutdown_flag = shutdown(connfd, SHUT_RDWR);
  close(connfd);
  printf("Connection with client closed.\n\n");

  /* decrement num threads and return */
  numthreads--;
  return NULL;
}
