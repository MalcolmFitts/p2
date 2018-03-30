/*  (Version: p2)
 * backend.c
 *    - created for use with "bbbserver.c"
 *	  - implementations of server back-end functions
 *	  - functions and data structures defined in "backend.h"
 */

#include "backend.h"

void* handle_be(void* ptr) {
  int sockfd = *(int *) ptr;
  int connfd;

  struct sockaddr_in sender_addr;
  socklen_t sender_addr_len;
  sender_addr_len = sizeof(sender_addr);

  Pkt_t packet;
  int recv_flag = 0;

  while(1) {
    pthread_mutex_lock(&stdout_lock);
    printf("Waiting for Packet\n");
    pthread_mutex_unlock(&stdout_lock);
    /* receiving packet and writing into p_buf */
    connfd = accept(sockfd, (struct sockaddr *) &sender_addr, &sender_addr_len);
    bzero(&packet, sizeof(packet));
    recv_flag = recvfrom(connfd, &packet, sizeof(packet), 0,
                        (struct sockaddr *) &sender_addr, &sender_addr_len);

    if(recv_flag == -1){
      error("Error on recvfrom():30");
    }
    //int type = get_packet_type(packet);

    /* CHECK accepted types (SYN and ACK)*/
    /*
     * TODO add FIN implimentation
     */
    //if (type == PKT_FLAG_UNKNOWN || type == PKT_FLAG_CORRUPT) {
      /* TODO corrupted packet */
    //}

    //else {
      //sent_status = serve_content(packet, sockfd, sender_addr, type);

      //if(sent_status < 0){
	       /* TODO error on send */
      //}
    //}
    //*/
  }
}

int serve_content(Pkt_t packet, int sockfd, struct sockaddr_in server_addr,
                  int flag) {

  /* parsing data from packet */
  P_Hdr hdr = packet.header;

  /* flipping ports to send packet back */
  uint16_t s_port = hdr.dest_port;
  uint16_t d_port = hdr.source_port;

  /* CHECK s_num */
  uint32_t s_num = hdr.seq_num;

  /* local vars */
  char filename[MAX_DATA_SIZE];
  socklen_t server_addr_len = sizeof(server_addr);
  int n_set;
  Pkt_t data_pkt;

  /* Create the packet to be sent */
  if (flag == PKT_FLAG_ACK) {
    /* parsing filename from ACK packet buffer */
    sscanf(packet.buf, "Ready to send: %s\n", filename);

    /* respond to ACK packet with data */
    data_pkt = create_packet(d_port, s_port, s_num, filename, PKT_FLAG_DATA);
  }

  else {
    /* parsing filename from SYN packet buffer */
    sscanf(packet.buf, "Request: %s\n", filename);

    /* respond to SYN packet with SYN-ACK */
    data_pkt = create_packet(d_port, s_port, s_num, filename, PKT_FLAG_SYN_ACK);
  }

  n_set = sendto(sockfd, &data_pkt, sizeof(data_pkt), 0,
		 (struct sockaddr *) &server_addr, server_addr_len);

  return n_set;
}

Node create_node(char* path, char* name, int port, int rate) {
  Node n;
  /* cast port to correct type - uint16_t  */
  n.port = (uint16_t) port;

  /* assign other fields directly */
  n.content_path = path;
  n.ip_hostname = name;
  n.content_rate = rate;

  /* creating node address */
  struct hostent* node_host;
  node_host = gethostbyname(name);
  if(!node_host) {
    printf("Error: No Host Found For %s\n", name);
    n.content_rate = -1;
  }

  struct sockaddr_in addr;
  bzero((char *) &addr, sizeof(addr));
  addr.sin_family = AF_INET;
  bcopy((char *)node_host->h_addr, (char *)&addr.sin_addr.s_addr,
    node_host->h_length);
  addr.sin_port = htons((unsigned short) port);

  n.node_addr = addr;

  return n;
}

int check_node_content(Node pn, char* filename) {

  /* CHECK TODO - might have to check for /content/ start and whatnot */
  if(strcmp(filename, pn.content_path) == 0) {
    return 1;
  }
  return 0;
}

Node_Dir* create_node_dir(int max) {
  /* check for creation of dir larger than defined max */
  if(max > MAX_NODES) max = MAX_NODES;

  /* allocate mem for struct */
  Node_Dir* nd = malloc(sizeof(Node_Dir));

  /* checking for successful mem allocation */
  if(!nd) return NULL;

  nd->cur_nodes = 0;
  nd->max_nodes = max;

  /* allocate mem for node array */
  nd->n_array = malloc(max * sizeof(Node));
  int i;
  for(i = 0; i < max; i++) {
    nd->n_array[i] = *((Node*) malloc(sizeof(Node)));
  }

  return nd;
}

int add_node(Node_Dir* nd, Node node) {
  if((nd->cur_nodes) < (nd->max_nodes)) {
    /* directory not full - add the node */

    int n = (nd->cur_nodes);

    nd->n_array[n] = node;

    nd->cur_nodes = n + 1;
    return 1;
  }

  /* max nbr of nodes in directory already reached */
  return 0;
}

Node check_content(Node_Dir* dir, char* filename) {
  int max = dir->cur_nodes;
  int found = 0;
  int index = -1;
  Node n;

  /* looping through directory and checking nodes */
  int i;
  for(i = 0; i < max; i++) {
    n = dir->n_array[i];

    /* checking node content -- CHECK TODO - check for content rate too?  */
    if(check_node_content(n, filename) == 1 && !found) {
      found = 1;
      index = i;
    }
  }

  /* did not find content */
  if(!found || index == -1){
    n.content_rate = -1;
    return n;
  };

  Node res = create_node(n.content_path, n.ip_hostname, n.port,
                          n.content_rate);
  return res;
}

void sync_node(Node node, uint16_t s_port, int sockfd) {
  pthread_mutex_lock(&stdout_lock);
  printf("Made it to sync...\n");
  pthread_mutex_unlock(&stdout_lock);
  char buf[BUFSIZE];
  int n_sent;
  int n_recv;
  uint16_t node_port;
  node_port = node.port;

  pthread_mutex_lock(&stdout_lock);
  printf("Made it to backend.c:255\n");
  pthread_mutex_unlock(&stdout_lock);
  struct sockaddr_in peer_addr = node.node_addr;
  socklen_t peer_addr_len = sizeof(peer_addr);

  Pkt_t syn_packet;
  Pkt_t syn_ack_packet;

  printf("Made it to backend.c:262\n");

  /* create SYN packet to send to node */
  syn_packet = create_packet(node_port, s_port, 0, node.content_path,
                                   PKT_FLAG_SYN);

  printf("Made it to backend.c:268\n");

  printf("Sending packet!\n");
  /* sending SYN packet */
  n_sent = sendto(sockfd, &syn_packet, sizeof(syn_packet), 0,
                 (struct sockaddr*) &peer_addr, peer_addr_len);

  printf("Packet sent!\nBytes sent: %d\n", n_sent);

  if(n_sent < 0) {
    /* TODO buffer info: "send SYN fail" */
    printf("Error on sending SYN packet.\n");
    exit(0);
  }

  /* receive SYN-ACK packet */
  n_recv = recvfrom(sockfd, &syn_ack_packet, sizeof(syn_ack_packet), 0,
                   (struct sockaddr *) &peer_addr, &peer_addr_len);

  if(n_recv < 0) {
    /* TODO buffer info: "recv ACK fail" */
  }

  if(get_packet_type(syn_ack_packet) != PKT_FLAG_SYN_ACK) {
    /* TODO buffer info: "corrupted packet OR wrong packet type" */
  }

  /* storing buf for later return to caller & discarding local packets */
  /* CHECK - memcpy */
  size_t b_len = strlen(syn_ack_packet.buf);
  memcpy(buf, syn_ack_packet.buf, b_len);
  return;
}

char* request_content(Node node, uint16_t s_port, int sockfd,
  struct sockaddr_in serveraddr, uint32_t seq_ack_num) {
  /* init local vars */
  char buf[MAX_DATA_SIZE] = {0};
  int n_sent;
  int n_recv;
  /* CHECK - maybe this is parsed wrong */
  //int d_host = parse_str_2_int(node->ip_hostname);
  short d_port = node.port;

  Pkt_t data_pkt;
  Pkt_t ack_pkt;

  /* creating ACK packet */
  ack_pkt = create_packet(d_port, s_port, seq_ack_num,
    node.content_path, PKT_FLAG_ACK);

  /* creating sockaddr for peer node */
  struct sockaddr_in peer_addr = node.node_addr;
  socklen_t peer_addr_len = sizeof(peer_addr);

  /* sending ACK to peer node */
  n_sent = sendto(sockfd, &ack_pkt, sizeof(ack_pkt), 0,
                 (struct sockaddr *) &peer_addr, peer_addr_len);

  if(n_sent < 0) {
    /* TODO deal with fail on sending ACK */
  }

  n_recv = recvfrom(sockfd, &data_pkt, sizeof(data_pkt), 0,
                   (struct sockaddr *) &peer_addr, &peer_addr_len);

  if(n_recv < 0) {
    /* TODO deal with fail on receiving data */
  }

  if(get_packet_type(data_pkt) != PKT_FLAG_DATA) {
    /* TODO deal with corrupted packet */
  }

  /* storing buf for later return to caller & discarding local packets */
  /* CHECK - memcpy */
  size_t b_len = strlen(data_pkt.buf);
  memcpy(buf, data_pkt.buf, b_len);

  /* CHECK - not sure i should discard these packets here */
  //discard_packet(ack_packet);
  //discard_packet(dat_packet);

  char* ref = buf;
  return ref;
}

/*
 *  TODO - replace writing headers with returning flag values
 */
int peer_add_response(int connfd, char* BUF, struct thread_data *ct,
                      Node_Dir* node_dir) {
  /* initilizing local buf and arrays for use in parsing */
  char buf[BUFSIZE];
  char* filepath = malloc(sizeof(char) * MAXLINE);
  char* hostname = malloc(sizeof(char) * MAXLINE);
  char* port_c = malloc(sizeof(char) * MAXLINE);
  char* rate_c = malloc(sizeof(char) * MAXLINE);

  int port;
  int rate;

  /* creating local copy of buf */
  bzero(buf, BUFSIZE);
  strcpy(buf, BUF);

  /* CHECK - why using BUF when we have buf (copy of BUF) */
  int add_res = parse_peer_add(BUF, filepath, hostname, port_c, rate_c);
  if(!add_res) {
    /* 500 Code  --> Failure to parse peer add request
     * TODO      --> flag (return) val: parse fail */
    write_status_header(connfd, SC_SERVER_ERROR, ST_SERVER_ERROR);
    return 0;
  }

  pthread_mutex_lock(&stdout_lock);
  printf("parsed peer add successfully.\n");
  pthread_mutex_unlock(&stdout_lock);

  /* parsing string reps to ints and freeing memory */
  port = parse_str_2_int(port_c);
  rate = parse_str_2_int(rate_c);
  free(port_c);
  free(rate_c);

  /* creating node with parsed data */
  Node n = create_node(filepath, hostname, port, rate);

  if(n.content_rate == -1){
    error("ERROR: on create_node");
  }

  if(!add_node(node_dir, n)){
    /* 500 Code  --> Failure to add node to directory
     * TODO      --> flag (return) val: add node fail */
    write_status_header(connfd, SC_SERVER_ERROR, ST_SERVER_ERROR);
    return 0;
  }

  pthread_mutex_lock(&stdout_lock);
  printf("created & added node successfully. \n");

  /* CHECK - debug printing */
  printf("Node hostname: %s\nNode port: %d\n", n.ip_hostname, n.port);
  printf("Node content: %s\nNode rate: %d\n", n.content_path, n.content_rate);
  pthread_mutex_unlock(&stdout_lock);

  /* 200 Code  --> Success!
   * TODO      --> flag (return) val: success */
  write_status_header(connfd, SC_OK, ST_OK);

  pthread_mutex_lock(&stdout_lock);
  printf("wrote headers!\n");
  pthread_mutex_unlock(&stdout_lock);
  return 1;
}

/*
 *  TODO - replace writing headers/data with returning flag values
 */
int peer_view_response(int connfd, char*BUF, struct thread_data *ct,
                       Node_Dir* node_dir){
  char* filepath = malloc(sizeof(char) * MAXLINE);
  char path[MAXLINE];
  char* file_type = malloc(sizeof(char) * MINLINE);
  int res;

  char* buf = "SUH";

  struct sockaddr_in node_addr;
  socklen_t node_addr_len;

  /* return value - will be set bitwise */
  // int ret_flag = 0;

  /* parsing content filepath from original request */
  res = parse_peer_view_content(BUF, filepath);
  if(!res) {
    /* 500 Error --> Failure to parse peer view request
     * TODO      --> flag (return) val: parse fail */
    write_status_header(connfd, SC_SERVER_ERROR, ST_SERVER_ERROR);
    return 0;
  }

  pthread_mutex_lock(&stdout_lock);
  printf("trying to get peer file: %s\n", filepath);
  pthread_mutex_unlock(&stdout_lock);

  bzero(path, MAXLINE);
  strcpy(path, filepath);

  /* parsing file type from filepath */
  res = parse_file_type(path, file_type);
  if(!res) {
    /* 500 Error --> Failure to parse file type
     * TODO      --> flag (return) val: parse fail */
    write_status_header(connfd, SC_SERVER_ERROR, ST_SERVER_ERROR);
    return 0;
  }

  /* finding node with requested content */
  Node node = check_content(node_dir, filepath);

  if(node.content_rate == -1){
    /* 500 Error --> Failure to find content in node directory
     * TODO      --> flag (return) val: no node found */
    write_status_header(connfd, SC_NOT_FOUND, ST_NOT_FOUND);
    return 0;
  }

  char *node_hostname = node.ip_hostname;
  uint16_t node_port = node.port;

  pthread_mutex_lock(&stdout_lock);
  printf("Node with content: %s:%d\n", node.ip_hostname, node.port);
  pthread_mutex_unlock(&stdout_lock);

  node_addr = get_sockaddr_in(node_hostname, (short) node_port);
  node_addr_len = sizeof(node_addr);

  int my_sockfd;
  my_sockfd = ct->listenfd_be;

  int sent = sendto(my_sockfd, buf, sizeof(buf), 0,
                   (struct sockaddr *) &node_addr, node_addr_len);

  if(sent < 0){
    error("ERROR on SEND.");
  } else {
    printf("SENT\n");
  }

  return 0;
}

/*
 *  TODO - replace writing headers with returning flag values
 */
int peer_rate_response(int connfd, char* BUF, struct thread_data *ct){
  char buf[MAXLINE];
  //int rate;
  char* rate_c = malloc(sizeof(char) * MAXLINE);

  bzero(buf, BUFSIZE);
  strcpy(buf, BUF);

  /* TODO this returns something */
  parse_peer_config_rate(BUF, rate_c);

  //rate = parse_str_2_int(rate_c);
  free(rate_c);

  // CHECK 200 OK response on config_rate
  write_status_header(connfd, SC_OK, ST_OK);

  return 0;
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
