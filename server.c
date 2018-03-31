/*
 * server.c - A simple UDP echo server
 * usage: udpserver <port>
 */

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

#define BUFSIZE 1024

static pthread_mutex_t printf_mutex;

void* recv_fn(void* ptr);
/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char **argv) {
  int sockfd; /* socket */
  //int clientlen; /* byte size of client's address */

  struct sockaddr_in serveraddr; /* server's addr */
  // int fe_be_portno;
  int be_portno;

  //struct sockaddr_in peeraddr; /* client addr */
  //int peer_be_portno;

  //struct hostent *hostp; /* client host info */
  //char buf[BUFSIZE]; /* message buf */
  //char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  //int n; /* message byte size */

  /*
   * check command line arguments
   */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  //peer_hostname = argv[1];
  be_portno = atoi(argv[1]);


  pthread_mutex_init(&printf_mutex, NULL);
  /*
   * socket: create the parent socket
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets
   * us rerun the server immediately after we kill it;
   * otherwise we have to wait about 20 secs.
   * Eliminates "ERROR on binding: Address already in use" error.
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
	     (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)be_portno);

  /*
   * bind: associate the parent socket with a port
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr,
	   sizeof(serveraddr)) < 0)
    error("ERROR on binding");

  pthread_t tid_be;
  pthread_create(&(tid_be), NULL, recv_fn, (void *) &sockfd);

  char go[BUFSIZE];
  pthread_mutex_lock(&printf_mutex);
  printf("Tell me when to stop: \n");
  pthread_mutex_unlock(&printf_mutex);
  pthread_mutex_lock(&printf_mutex);
  fgets(go, BUFSIZE, stdin);
  pthread_mutex_unlock(&printf_mutex);
  int f;
  f = shutdown(sockfd, 0);
  pthread_mutex_lock(&printf_mutex);
  printf("Socket should be closed\n");
  pthread_mutex_unlock(&printf_mutex);
  while(1){
  }
}

void* recv_fn(void* ptr){
  int sockfd = *(int *) ptr;
  //int connfd;

  char buf[BUFSIZE];

  struct sockaddr_in peer_addr;
  socklen_t peer_addr_len;
  peer_addr_len = sizeof(peer_addr);

  while(1){
    int recv_flag;
    bzero(&buf, sizeof(buf));
    pthread_mutex_lock(&printf_mutex);
    printf("I am recieving\n");
    pthread_mutex_unlock(&printf_mutex);
    recv_flag = recvfrom(sockfd, &buf, sizeof(buf), 0,
                        (struct sockaddr *) &peer_addr, &peer_addr_len);

    if(recv_flag == -1){
      error("Error on recvfrom():30");
    }
    pthread_mutex_lock(&printf_mutex);
    printf("I am off\n");
    pthread_mutex_unlock(&printf_mutex);
  }
}
