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

void error(char *msg);

struct sockaddr_in get_sockaddr_in(char* hostname, short port);

/**  error - wrapper for perror*/
void error(char *msg) {
  perror(msg);
  exit(1);
}

struct sockaddr_in get_sockaddr_in(char* hostname, short port){
  struct hostent *server;
  struct sockaddr_in addr;

  /* resolve host */
  server = gethostbyname(hostname);
  if (!server) {
    printf("ERROR, no such host as %s\n", hostname);
    exit(0);
  }

  /* build node's address */
  bzero((char *) &addr, sizeof(addr));
  addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&addr.sin_addr.s_addr,
    server->h_length);
  addr.sin_port = htons(port);

  return addr;
}

int main(){
  int optval;
  /* TO SEND */
  char buf[BUFSIZE];

  char *node_hostname = "172.21.67.91";
  uint16_t node_port = 8436;
  struct sockaddr_in node_addr;
  int node_addr_len;

  node_addr = get_sockaddr_in(node_hostname, node_port);
  node_addr_len = sizeof(node_addr);

  int my_sockfd;
  struct sockaddr_in my_addr;
  uint16_t my_port = 8346;

  my_sockfd = socket(AF_INET, SOCK_DGRAM, 0);

 /* setsockopt: Handy debugging trick that lets
  *  us rerun the server immediately after we kill it;
  *  otherwise we have to wait about 20 secs.
  *  Eliminates "ERROR on binding: Address already in use" error.
  */

  optval = 1;
  setsockopt(my_sockfd, SOL_SOCKET, SO_REUSEADDR,
            (const void *)&optval , sizeof(int));

  /* we are using the Internet */
  my_addr.sin_family = AF_INET;
  /* accept reqs to any IP addr */
  my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  /* port to listen on */
  my_addr.sin_port = htons(my_port);

  if (bind(my_sockfd, (struct sockaddr *) &my_addr, sizeof(my_addr)) < 0)
    error("ERROR on binding front-end socket with port");

  /* get a message from the user */
  bzero(buf, BUFSIZE);
  printf("Please enter msg: ");
  fgets(buf, BUFSIZE, stdin);

  int sent = sendto(my_sockfd, buf, BUFSIZE, 0,
                   (struct sockaddr *) &node_addr, node_addr_len);

  if(sent < 0){
    error("ERROR on SEND.");
  } else {
    printf("SENT\n");
  }

  return 0;
}
