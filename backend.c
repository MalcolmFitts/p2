/*  (Version: p2)
 * backend.c
 *    - created for use with "bbbserver.c"
 *	  - implementations of server back-end functions
 *	  - functions and data structures defined in "backend.h"
 */

#include "backend.h"

void* recieve_pkt(void* ptr) {
  int sockfd = *(int*)ptr;

  struct sockaddr_in sender_addr;
  socklen_t sender_addr_len = sizeof(sender_addr);

  Pkt_t packet;
  int sent_status;
  int recv_status;

  //printf("%d\n", sockfd);

  while(1) {
    printf("Waiting for Packet\n");
    /* receiving packet and writing into p_buf */
    recv_status = recvfrom(sockfd, &packet, sizeof(packet), 0,
		      (struct sockaddr *) &sender_addr, &sender_addr_len);

    printf("Recieved Packet!\n");

    if(recv_status < 0) {
      /* TODO - Error on receive packet */
    }

    int type = get_packet_type(packet);

    /* CHECK accepted types (SYN and ACK)*/
    /* TODO add FIN implimentation */
    if (type == PKT_FLAG_UNKNOWN || type == PKT_FLAG_CORRUPT) {
      /* TODO corrupted packet */
    }

    else {
      sent_status = serve_content(packet, sockfd, sender_addr, type);

      if(sent_status < 0){
	       /* TODO error on send */
      }
    }
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

int init_backend(short port_be) {
  int sockfd_be;
  int optval_be = 1;

  sockfd_be = socket(AF_INET, SOCK_DGRAM, 0);
  if(sockfd_be < 0)
    error("ERROR opening back-end socket");

  /* setsockopt: Handy debugging trick that lets
   * us rerun the server immediately after we kill it;
   * otherwise we have to wait about 20 secs.
   * Eliminates "ERROR on binding: Address already in use" error.
   */
  setsockopt(sockfd_be, SOL_SOCKET, SO_REUSEADDR,
    (const void *)&optval_be, sizeof(int));

  /* build the server's back end internet address */
  struct sockaddr_in self_addr;

  /* CHECK - was not zeroing memory */
  bzero((char *) &self_addr, sizeof(self_addr));

  self_addr.sin_family = AF_INET; /* we are using the Internet */
  self_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* accept reqs to any IP addr */
  self_addr.sin_port = htons(port_be); /* port to listen on */


  /* bind: associate the listening socket with a port */
  if (bind(sockfd_be, (struct sockaddr *) &self_addr, sizeof(self_addr)) < 0)
    error("ERROR on binding back-end socket with port");

  // /* CHECK - debugging - debugging - debugging - debugging */
  // struct hostent *hostp; /* client host info */
  // char *hostaddrp;
  // printf("shits bad.\n");
  // hostp = gethostbyaddr((const char *)&self_addr.sin_addr.s_addr, 
  //       sizeof(self_addr.sin_addr.s_addr), AF_INET);
  // hostaddrp = inet_ntoa(self_addr.sin_addr);
  // printf("{be} Server address %s (%s)\n", 
  //    hostp->h_name, hostaddrp);
  // /* CHECK - debugging - debugging - debugging - debugging */

  // printf("not that bad\n");

  return sockfd_be;
}

Node* create_node(char* path, char* name, int port, int rate) {
  /* allocate mem for struct */
  Node* pn = malloc(sizeof(Node));

  /* checking for successful mem allocation */
  if(!pn) return NULL;

  /* cast port to correct type - uint16_t  */
  pn->port = (uint16_t) port;

  /* assign other fields directly */
  pn->content_path = path;
  pn->ip_hostname = name;
  pn->content_rate = rate;

  return pn;
}

int check_node_content(Node* pn, char* filename) {
  /* checking for null pointer */
  if(!pn) return -1;

  /* CHECK TODO - might have to check for /content/ start and whatnot */
  if(strcmp(filename, pn->content_path) == 0) {
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
  nd->n_array = malloc(max * sizeof(Node *));
  int i;
  for(i = 0; i < max; i++) {
    nd->n_array[i] = *((Node*) malloc(sizeof(Node)));
  }

  return nd;
}

int add_node(Node_Dir* nd, Node* node) {
  if((nd->cur_nodes) < (nd->max_nodes)) {
    /* directory not full - add the node */

    int n = (nd->cur_nodes);

    (nd->n_array[n]).content_path = node->content_path;
    (nd->n_array[n]).ip_hostname = node->ip_hostname;
    (nd->n_array[n]).port = node->port;
    (nd->n_array[n]).content_rate = node->content_rate;

    nd->cur_nodes = n + 1;
    return 1;
  }


  /* max nbr of nodes in directory already reached */
  return 0;
}

Node* check_content(Node_Dir* dir, char* filename) {
  int max = dir->cur_nodes;
  int found = 0;
  Node ref;

  /* looping through directory and checking nodes */
  int i;
  for(i = 0; i < max; i++) {
    Node t = dir->n_array[i];

    /* checking node content -- CHECK TODO - check for content rate too?  */
    if(check_node_content(&t, filename) == 1 && !found) {
      ref = dir->n_array[i];
      found = 1;
    }
  }

  /* did not find content */
  if(!found) return NULL;

  Node* res = create_node(ref.content_path, ref.ip_hostname, ref.port, ref.content_rate);
  return res;
}

char* sync_node(Node* node, uint16_t s_port, int sockfd) {
  char buf[BUFSIZE];
  int n_sent;
  int n_recv;
  uint16_t d_port = node->port;

  unsigned int peer_ip = (unsigned int) parse_str_2_int(node->ip_hostname);
  short peer_port = (short) node->port;
  struct sockaddr_in peer_addr = get_sockaddr_in(peer_ip, peer_port);
  socklen_t peer_addr_len = sizeof(peer_addr);

  Pkt_t syn_packet;
  Pkt_t syn_ack_packet;

  /* create SYN packet to send to node */
  syn_packet = create_packet(d_port, s_port, 0, node->content_path,
                                   PKT_FLAG_SYN);

  printf("Sending packet!\n");
  /* sending SYN packet */
  n_sent = sendto(sockfd, &syn_packet, sizeof(syn_packet), 0,
                 (struct sockaddr*) &peer_addr, peer_addr_len);

  printf("Packet sent!\n");
  if(n_sent < 0) {
    /* TODO buffer info: "send SYN fail" */
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

  /* CHECK - not sure i should discard these packets here */
  //discard_packet(syn_packet);
  //discard_packet(syn_ack_packet);

  char* ref = buf;
  return ref;
}

char* request_content(Node* node, uint16_t s_port, int sockfd,
  struct sockaddr_in serveraddr, uint32_t seq_ack_num) {
  /* init local vars */
  char buf[MAX_DATA_SIZE] = {0};
  int n_sent;
  int n_recv;
  int d_host = parse_str_2_int(node->ip_hostname);
  short d_port = node->port;
  Pkt_t data_pkt;
  Pkt_t ack_pkt;

  /* creating ACK packet */
  ack_pkt = create_packet(d_port, s_port, seq_ack_num,
    node->content_path, PKT_FLAG_ACK);

  /* creating sockaddr for peer node */
  struct sockaddr_in peer_addr = get_sockaddr_in(d_host, d_port);
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

  printf("parsed peer add successfully.\n");

  /* parsing string reps to ints and freeing memory */
  port = parse_str_2_int(port_c);
  rate = parse_str_2_int(rate_c);
  free(port_c);
  free(rate_c);

  /* creating node with parsed data */
  Node* n = create_node(filepath, hostname, port, rate);

  if(!add_node(node_dir, n)){
    /* 500 Code  --> Failure to add node to directory
     * TODO      --> flag (return) val: add node fail */
    write_status_header(connfd, SC_SERVER_ERROR, ST_SERVER_ERROR);
    return 0;
  }

  printf("created & added node successfully. \n");

  /* 200 Code  --> Success!
   * TODO      --> flag (return) val: success */
  write_status_header(connfd, SC_OK, ST_OK);

  printf("wrote headers!\n");
  return 1;
}

/*  TODO - replace writing headers/data with returning flag values
 */
int peer_view_response(int connfd, char*BUF, struct thread_data *ct, Node_Dir* node_dir){
  char buf[BUFSIZE];
  char* filepath = malloc(sizeof(char) * MAXLINE);
  char path[MAXLINE];
  char* file_type = malloc(sizeof(char) * MINLINE);
  Node* node;
  int len;
  int res;

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

  printf("trying to get peer file: %s\n", filepath);

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

  bzero(buf, BUFSIZE);
  strcpy(buf, BUF);

  /* finding node with requested content */
  node = check_content(node_dir, filepath);

  if(!node){
    /* 500 Error --> Failure to find content in node directory
     * TODO      --> flag (return) val: no node found */
    write_status_header(connfd, SC_NOT_FOUND, ST_NOT_FOUND);
    return 0;
  }

  printf("Node with content: %s:%d\n", node->ip_hostname, node->port);

  bzero(buf, BUFSIZE);

  /* TODO - format return value in sync_node */
  /* initializing connection with node that should have requested content */
  char* b = sync_node(node, ct->port_be, ct->listenfd_be);

  /* TODO check if fails */

  printf("synced node!\n");

  /* TODO - will need to parse this differently once chanced sync_node {658} */
  len = parse_str_2_int(b);

  write_status_header(connfd, SC_OK, ST_OK);
  write_content_length_header(connfd, len);
  write_content_type_header(connfd, file_type);

  printf("wrote headers!\n");

  bzero(buf, BUFSIZE);
  /* CHECK will not be full len after CP */
  char* b2 = request_content(node, ct->port_be, ct->listenfd_be, ct->c_addr,
                             len);

  write(connfd, b2, strlen(buf));

  printf("wrote data!!@@\n");

  return 1;
}

/*  TODO - replace writing headers with returning flag values
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

/* filler end line */

struct sockaddr_in get_sockaddr_in(unsigned int ip, short port){
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ip;
  addr.sin_port = htons(port);
  return addr;
}
