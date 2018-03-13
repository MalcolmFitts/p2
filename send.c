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
  /* TO SEND */
  char * hello = "Sup Bitch";
  int hello_size = sizeof(hello);

  char *node_hostname = "172.19.138.17";
  uint16_t node_port = 9002;
  struct sockaddr_in node_addr;
  struct in_addr addr;
  int node_addr_len = sizeof(node_addr);

  if (inet_aton(node_hostname, &addr) == 0) {
        fprintf(stderr, "Invalid address\n");
        exit(EXIT_FAILURE);
  }

  bzero((char *) &node_addr, node_addr_len);
  node_addr.sin_family = AF_INET;
  node_addr.sin_addr = addr;
  node_addr.sin_port = htons(node_port);

  int my_sockfd;
  struct sockaddr_in my_addr;
  uint16_t my_port = 8346;

  my_sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  /* we are using the Internet */
  my_addr.sin_family = AF_INET;
  /* accept reqs to any IP addr */
  my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  /* port to listen on */
  my_addr.sin_port = htons(my_port);

  if (bind(my_sockfd, (struct sockaddr *) &my_addr, sizeof(my_addr)) < 0)
    error("ERROR on binding front-end socket with port");

  //if (listen(sockfd_fe, 10) < 0) /* allow 10 requests to queue up */
    //error("ERROR on listen");

  int sent = sendto(my_sockfd, hello, hello_size, 0,
                   (struct sockaddr *) &node_addr, node_addr_len);

  if(sent < 0){
    error("ERROR on SEND.");
  } else {
    printf("SENT\n");
  }

  return 0;
}
