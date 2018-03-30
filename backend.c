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

  struct hostent *hostp;          /* client host info */
  char *hostaddrp;                /* dotted decimal host addr string */

  //printf("%d\n", sockfd);

  while(1) {
    printf("Waiting for packet on backend...\n");
    /* receiving packet and writing into p_buf */
    recv_status = recvfrom(sockfd, &packet, sizeof(packet), 0,
		      (struct sockaddr *) &sender_addr, &sender_addr_len);

    printf("Recieved Packet!\n");

    if(recv_status < 0) {
      /* TODO - Error on receive packet */
    }

    hostp = gethostbyaddr((const char *)&sender_addr.sin_addr.s_addr,
        sizeof(sender_addr.sin_addr.s_addr), AF_INET);
    hostaddrp = inet_ntoa(sender_addr.sin_addr);

    printf("Packet sender: %s (%s)\n", hostp->h_name, hostaddrp);

    int type = get_packet_type(packet);

    /* CHECK accepted types (SYN and ACK)*/
    /* TODO add FIN implimentation */
    if(type == PKT_FLAG_UNKNOWN) {
      printf("Server received packet with unknown flag.\n");
      printf("{temp} ignoring this packet for now.\n\n");
    }
    
    else if(type == PKT_FLAG_CORRUPT) {
      printf("Server received packet with corrupt flag.\n");
      printf("{temp} ignoring this packet for now.\n\n");
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

  struct hostent *hostp;          /* client host info */
  char *hostaddrp;                /* dotted decimal host addr string */

  /* Create the packet to be sent */
  if (flag == PKT_FLAG_ACK) {
    printf("Received packet type: ACK\n");

    /* parsing filename from ACK packet buffer */
    sscanf(packet.buf, "Ready to send: %s\n", filename);

    printf("Requested file: %s\n", filename);

    /* respond to ACK packet with data */
    data_pkt = create_packet(d_port, s_port, s_num, filename, PKT_FLAG_DATA);
    printf("Sending DATA packet...\n");
  }

  else if (flag == PKT_FLAG_SYN) {
    printf("Received packet type: SYN\n");

    /* parsing filename from SYN packet buffer */
    sscanf(packet.buf, "Request: %s\n", filename);

    printf("Requested file: %s\n", filename);

    /* respond to SYN packet with SYN-ACK */
    data_pkt = create_packet(d_port, s_port, s_num, filename, PKT_FLAG_SYN_ACK);
    printf("Sending SYN-ACK packet...\n");
  }

  else {
    
    if(flag == PKT_FLAG_SYN_ACK) {
      printf("Server received packet with SYN-ACK flag.\n");
      printf("Should not have received this type of packet here.\n");
    }
    else if(flag == PKT_FLAG_DATA) {
      printf("Server received packet with DATA flag.\n");
      printf("Should not have received this type of packet here.\n");
    }
    
    return 0;
  }

  hostp = gethostbyaddr((const char *)&server_addr.sin_addr.s_addr,
        sizeof(server_addr.sin_addr.s_addr), AF_INET);
  hostaddrp = inet_ntoa(server_addr.sin_addr);

  printf("Packet destination: %s (%s)\n", hostp->h_name, hostaddrp);

  n_set = sendto(sockfd, &data_pkt, sizeof(data_pkt), 0,
		 (struct sockaddr *) &server_addr, server_addr_len);

  printf("Sent!\n\n");

  return n_set;
}

int init_backend(short port_be, struct sockaddr_in* self_addr) {
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
  //struct sockaddr_in self_addr;

  /* CHECK - was not zeroing memory */
  //bzero((char *) &self_addr, sizeof(self_addr));

  self_addr->sin_family = AF_INET; /* we are using the Internet */
  (self_addr->sin_addr).s_addr = htonl(INADDR_ANY); /* accept reqs to any IP addr */
  self_addr->sin_port = htons((unsigned short) port_be); /* port to listen on */


  /* bind: associate the listening socket with a port */
  if (bind(sockfd_be, (struct sockaddr *) self_addr, sizeof(*self_addr)) < 0)
    error("ERROR on binding back-end socket with port");


  return sockfd_be;
}



Node* create_node(char* path, char* name, int port, int rate) {
  /* allocate mem for struct */
  Node* pn;
  pn = malloc(sizeof(Node));

  /* checking for successful mem allocation */
  if(!pn) return NULL;

  /* cast port to correct type - uint16_t  */
  pn->port = (uint16_t) port;

  /* assign other fields directly */
  pn->content_path = path;
  pn->ip_hostname = name;
  pn->content_rate = rate;

  /* creating node address */
  struct hostent* node_host;
  node_host = gethostbyname(name);
  if(!node_host) {
    printf("Error: No Host Found For %s\n", name);
    return NULL;
  }

  struct sockaddr_in addr;
  bzero((char *) &addr, sizeof(addr));

  addr.sin_family = AF_INET;
  bcopy((char *)node_host->h_addr, (char *)&addr.sin_addr.s_addr, 
    node_host->h_length);
  addr.sin_port = htons((unsigned short) port);

  pn->node_addr = addr;

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
    (nd->n_array[n]).node_addr = node->node_addr;

    nd->cur_nodes = n + 1;
    return 1;
  }


  /* max nbr of nodes in directory already reached */
  return 0;
}

Node* check_content(Node_Dir* dir, char* filename) {
  int max = dir->cur_nodes;
  int found = 0;
  int index = -1;
  Node ref;

  /* looping through directory and checking nodes */
  int i;
  for(i = 0; i < max; i++) {
    Node t = dir->n_array[i];

    /* checking node content -- CHECK TODO - check for content rate too?  */
    if(check_node_content(&t, filename) == 1 && !found) {
      found = 1;
      index = i;
    }
  }

  /* did not find content */
  if(!found || index == -1) return NULL;

  ref = dir->n_array[index];

  Node* res = create_node(ref.content_path, ref.ip_hostname, ref.port, ref.content_rate);
  return res;
}

char* sync_node(Node* node, uint16_t s_port, int sockfd) {
  char buf[MAX_DATA_SIZE];
  int n_sent;
  int n_recv;
  uint16_t d_port = node->port;

  // unsigned int peer_ip = (unsigned int) parse_str_2_int(node->ip_hostname);
  // short peer_port = (short) node->port;
  struct sockaddr_in peer_addr = node->node_addr;
  socklen_t peer_addr_len = sizeof(peer_addr);

  // struct hostent *hostp;          /* client host info */
  // char *hostaddrp;                /* dotted decimal host addr string */

  Pkt_t syn_packet;
  Pkt_t syn_ack_packet;

  /* create SYN packet to send to node */
  syn_packet = create_packet(d_port, s_port, 0, node->content_path,
                                   PKT_FLAG_SYN);

  printf("Sending SYN packet to: %s:%d...\n", node->ip_hostname, node->port);
  /* sending SYN packet */
  n_sent = sendto(sockfd, &syn_packet, sizeof(syn_packet), 0,
                 (struct sockaddr*) &peer_addr, peer_addr_len);

  if(n_sent < 0) {
    /* TODO buffer info: "send SYN fail" */
    printf("Error on sending SYN packet.\n");
    exit(0);
  }

  printf("Packet sent!\nBytes sent: %d\n", n_sent);

  printf("Waiting to receive SYN_ACK packet...\n");

  /* receive SYN-ACK packet */
  n_recv = recvfrom(sockfd, &syn_ack_packet, sizeof(syn_ack_packet), 0,
                   (struct sockaddr *) &peer_addr, &peer_addr_len);

  if(n_recv < 0) {
    /* TODO buffer info: "recv ACK fail" */
    printf("Error on receiving SYN-ACK packet.\n");
    exit(0);
  }

  // hostp = gethostbyaddr((const char *)&peer_addr.sin_addr.s_addr,
  //       sizeof(peer_addr.sin_addr.s_addr), AF_INET);
  // hostaddrp = inet_ntoa(peer_addr.sin_addr);

  // printf("Packet sender: %s (%s)\n", hostp->h_name, hostaddrp);

  while(get_packet_type(syn_ack_packet) != PKT_FLAG_SYN_ACK) {
    /* receive SYN-ACK packet */
    n_recv = recvfrom(sockfd, &syn_ack_packet, sizeof(syn_ack_packet), 0,
                   (struct sockaddr *) &peer_addr, &peer_addr_len);

    if(n_recv < 0) {
      /* TODO buffer info: "recv ACK fail" */
      printf("Error on receiving SYN-ACK packet.\n");
      exit(0);
    }
  }

  printf("Received SYN-ACK packet from: %s:%d\n", node->ip_hostname, node->port);

  /* storing buf for later return to caller & discarding local packets */
  /* CHECK - memcpy */
  size_t b_len = strlen(syn_ack_packet.buf);
  strncpy(buf, syn_ack_packet.buf, b_len);

  /* CHECK - not sure i should discard these packets here */
  //discard_packet(syn_packet);
  //discard_packet(syn_ack_packet);
  char* res = buf;
  return res;
}

char* request_content(Node* node, uint16_t s_port, int sockfd,
  struct sockaddr_in serveraddr, uint32_t seq_ack_num) {
  /* init local vars */
  char buf[MAX_DATA_SIZE] = {0};
  int n_sent;
  int n_recv;
  /* CHECK - maybe this is parsed wrong */
  //int d_host = parse_str_2_int(node->ip_hostname);
  
  /* CHECK - both ports (source & dest) should be the same */
  //short d_port = node->port;

  Pkt_t data_pkt;
  Pkt_t ack_pkt;

  /* creating ACK packet */
  ack_pkt = create_packet(s_port, s_port, seq_ack_num,
    node->content_path, PKT_FLAG_ACK);

  /* creating sockaddr for peer node */
  struct sockaddr_in peer_addr = node->node_addr;
  socklen_t peer_addr_len = sizeof(peer_addr);

  printf("Sending ACK packet to: %s:%d...\n", node->ip_hostname, node->port);

  /* sending ACK to peer node */
  n_sent = sendto(sockfd, &ack_pkt, sizeof(ack_pkt), 0,
                 (struct sockaddr *) &peer_addr, peer_addr_len);

  if(n_sent < 0) {
    /* TODO deal with fail on sending ACK */
    printf("Error on sending ACK packet.\n");
    exit(0);
  }

  printf("Packet sent!\nBytes sent: %d\n", n_sent);
  printf("Waiting to receive DATA packet from %s:%d...\n", node->ip_hostname, node->port);


  n_recv = recvfrom(sockfd, &data_pkt, sizeof(data_pkt), 0,
                   (struct sockaddr *) &peer_addr, &peer_addr_len);

  if(n_recv < 0) {
    /* TODO deal with fail on receiving data */
    printf("Error on receiving data packet.\n");
    exit(0);
  }

  while(get_packet_type(data_pkt) != PKT_FLAG_DATA) {
    /* TODO deal with corrupted packet */
    n_recv = recvfrom(sockfd, &data_pkt, sizeof(data_pkt), 0,
                   (struct sockaddr *) &peer_addr, &peer_addr_len);

    if(n_recv < 0) {
      /* TODO deal with fail on receiving data */
      printf("Error on receiving DATA packet.\n");
      exit(0);
    }
  }

  printf("Received DATA packet from: %s:%d\n", node->ip_hostname, node->port);

  /* storing buf for later return to caller & discarding local packets */
  /* CHECK - memcpy */
  //size_t b_len = strlen(data_pkt.buf);

  //sets b_len to received bytes - packet header size = data size
  size_t b_len = n_recv - P_HDR_SIZE;
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
    write_empty_header(connfd);
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
    write_empty_header(connfd);
    return 0;
  }

  printf("created & added node successfully. \n");

  /* CHECK - debug printing */
  printf("Node hostname: %s\nNode port: %d\n", n->ip_hostname, n->port);
  printf("Node content: %s\nNode rate: %d\n\n", n->content_path, n->content_rate);

  /* 200 Code  --> Success!
   * TODO      --> flag (return) val: success */
  write_status_header(connfd, SC_OK, ST_OK);
  write_date_header(connfd);
  write_conn_header(connfd, CONN_KEEP_HDR);
  write_keep_alive_header(connfd, 0, 100);
  write_empty_header(connfd);

  printf("Wrote headers to client.\n");

  char client_resp[BUFSIZE] = {0};
  sprintf(client_resp, "Peer Add Success!\nNode hostname: %s\nNode port: %d\nNode content: %s\nNode rate: %d\n", 
    n->ip_hostname, n->port, n->content_path, n->content_rate);

  send(connfd, client_resp, strlen(client_resp), 0);

  printf("Wrote server info to client.\n");

  return 1;
}

/*  TODO - replace writing headers/data with returning flag values
 */
int peer_view_response(int connfd, char*BUF, struct thread_data *ct, Node_Dir* node_dir){
  char buf[BUFSIZE];
  char* filepath = malloc(sizeof(char) * MAXLINE);
  char path[MAXLINE];
  char* file_type = malloc(sizeof(char) * MINLINE);
  char* b;
  char* b2;

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
    write_empty_header(connfd);
    return 0;
  }

  printf("Requested peer file: %s\n", filepath);

  bzero(path, MAXLINE);
  strcpy(path, filepath);

  /* parsing file type from filepath */
  res = parse_file_type(path, file_type);
  if(!res) {
    /* 500 Error --> Failure to parse file type
     * TODO      --> flag (return) val: parse fail */
    write_status_header(connfd, SC_SERVER_ERROR, ST_SERVER_ERROR);
    write_empty_header(connfd);
    return 0;
  }

  bzero(buf, BUFSIZE);
  strcpy(buf, BUF);

  /* finding node with requested content */
  Node* node = check_content(node_dir, filepath);

  if(!node) {
    /* 500 Error --> Failure to find content in node directory
     * TODO      --> flag (return) val: no node found */
    write_status_header(connfd, SC_NOT_FOUND, ST_NOT_FOUND);
    write_empty_header(connfd);
    return 0;
  }

  printf("Known node with content: %s:%d\n", node->ip_hostname, node->port);

  bzero(buf, BUFSIZE);

  /* TODO - format return value in sync_node */
  /* initializing connection with node that should have requested content */

  /* DEBUG  */
  if(!ct) {
    printf("Thread data struct null.\n");
    return 0;
  }

  b = sync_node(node, (uint16_t) (ct->port_be), ct->listenfd_be);

  /* TODO check if fails */

  printf("Synced with node: %s:%d\n", node->ip_hostname, node->port);
  printf("received syn-ack buffer:\n%s\n", b);

  /* TODO - will need to parse this differently once chanced sync_node {658} */
  len = parse_str_2_int(b);
  

  bzero(buf, BUFSIZE);
  /* CHECK will not be full len after CP */
  b2 = request_content(node, ct->port_be, ct->listenfd_be, ct->c_addr,
                             len);

  write_status_header(connfd, SC_OK, ST_OK);
  write_date_header(connfd);
  write_conn_header(connfd, CONN_KEEP_HDR);
  write_keep_alive_header(connfd, 0, 100);
  write_content_length_header(connfd, len);
  write_content_type_header(connfd, file_type);

  printf("Wrote headers to client!\n");

  /* CHECK - this was writing 0 bytes - strlen(buf) = 0 */
  write_empty_header(connfd);
  write(connfd, b2, strlen(b2));

  printf("Written data:\n%s\n", b2);

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
  write_empty_header(connfd);

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




/* filler end line */
