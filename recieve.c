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

/**  error - wrapper for perror*/
void error(char *msg) {
  perror(msg);
  exit(1);
}

int main(){
  int optval;
  int my_sockfd;
  struct sockaddr_in my_addr;
  uint16_t my_port = 8436;
  struct sockaddr_in sender_addr;
  unsigned int sender_len = sizeof(sender_addr);
  int flag;
  char recieved[BUFSIZE];

  my_sockfd = socket(AF_INET, SOCK_DGRAM, 0);

 /* setsockopt: Handy debugging trick that lets
  *  us rerun the server immediately after we kill it;
  *  otherwise we have to wait about 20 secs.
  *  Eliminates "ERROR on binding: Address already in use" error.
  */

  optval = 1;
  setsockopt(my_sockfd, SOL_SOCKET, SO_REUSEADDR,
            (const void *)&optval , sizeof(int));

  bzero((char *) &my_addr, sizeof(my_addr));
  my_addr.sin_family = AF_INET;
  my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  my_addr.sin_port = htons((unsigned short)my_port);

  if (bind(my_sockfd, (struct sockaddr *) &my_addr, sizeof(my_addr)) < 0)
    error("ERROR on binding front-end socket with port");

  printf("Waiting to recieve...\n");

  bzero(recieved, BUFSIZE);
  flag = recvfrom(my_sockfd, recieved, BUFSIZE, 0,
                  (struct sockaddr *) &sender_addr, &sender_len);

  if(flag < 0){
    printf("Error on recieve\n");
  }

  printf("RECIEVED: %s\n", recieved);

  return 0;
}
