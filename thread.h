
#ifndef THREAD_H
#define THREAD_H

struct thread_data {
  struct sockaddr_in c_addr;  /* client address struct */
  int connfd;                 /* connection fd */
  int tid;                    /* thread id tag */
  int num;                    /* DEBUG - overall connected num */
  int listenfd_be;            /* back end listening socket */
  int port_be;                /* back end port */
  char* config_fn;            /* config filename */
};

#endif
