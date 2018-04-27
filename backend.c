/*  (Version: p2)
 * backend.c
 *    - created for use with "bbbserver.c"
 *	  - implementations of server back-end functions
 *	  - functions and data structures defined in "backend.h"
 */

#include "backend.h"

/* Global - Directory for node referencing   */
Node_Dir* node_dir;
N_Dir* neighbor_dir;

void* handle_be(void* ptr) {

  int sockfd = *(int*)ptr;        /* parsing sockfd from pointer arg */

  struct sockaddr_in sender_addr; /* packet sender's address */
  socklen_t sender_addr_len = sizeof(sender_addr);

  int sent_status;                /* sent bytes */
  int recv_status;                /* received bytes */

  while(1) {
    printf("Waiting for packet on backend...\n");

    /* receiving packet and writing into p_buf */
    Pkt_t packet;
    recv_status = recvfrom(sockfd, &packet, sizeof(packet), 0,
                        (struct sockaddr *) &sender_addr, &sender_addr_len);

    if(recv_status < 0) {
      /* Failed to receive packet - restart listening */
      printf("Error on receiving packet.\n");
      continue;
    }

    printf("Recieved Packet!\n");

    /* parsing out the type of packet that was received */
    int type = get_packet_type(packet);

    /* checking for corrupt or unknown flagged packets */
    if(type == PKT_FLAG_FIN) {
      printf("Server received FIN packet.\n");
      printf("Finished serving.\n\n");
      continue;
    }

    else if(type == PKT_FLAG_CORRUPT) {
       printf("Server received packet with corrupt flag.\n");
       continue;
    }

    /* responses for SYN, ACK, SYN-ACK and DATA packets */
    else {
      sent_status = serve_content(packet, sockfd, sender_addr, type);

      if(sent_status < 0) {
	       printf("Error: something went wrong with serving content.\n");
      }
    }
  }
}

int serve_content(Pkt_t packet, int sockfd, struct sockaddr_in server_addr,
                  int flag) {


  /* Packet struct for sending response */
  Pkt_t data_pkt;
  bzero(&data_pkt, sizeof(data_pkt));

  /* Response message length */
  int n_set;

  /* Content size from SYN-ACK packet buf */
  int c_size;
  int n_packs;

  /* Filename buffer */
  char filename[MAX_DATA_SIZE];

  /* Peer data variables */
  socklen_t server_addr_len = sizeof(server_addr);
  //struct hostent *hostp;          /* client host info */
  char *hostaddrp;                /* dotted decimal host addr string */

  /* Parsing data about packet sender */
  hostaddrp = inet_ntoa(server_addr.sin_addr);

  printf("Packet sender: %s\n", hostaddrp);

  /* Parsing header from packet */
  P_Hdr hdr = packet.header;

  /* Assigning ports for response */
  uint16_t s_port = hdr.dest_port;
  uint16_t d_port = hdr.source_port;

  /* Packet sequence number*/
  uint32_t s_num = hdr.seq_num;


  /* Responding to SYN packets: (peer node) */
  if (flag == PKT_FLAG_SYN) {
    printf("Received packet type: SYN\n");

    /* parsing filename from SYN packet buffer */
    if(sscanf(packet.buf, "Request: %s\n", filename) != 1) {
      printf("Error: could not parse filename from packet.\n");
      printf("Packet Buffer: \n%s\n", packet.buf);
      return -1;
    }

    printf("Requested file: %s\n", filename);

    /* respond to SYN packet with SYN-ACK packet */
    data_pkt = create_packet(d_port, s_port, s_num, filename, PKT_FLAG_SYN_ACK,
                             packet.header.com_buf);
    printf("Sending SYN-ACK packet...\n");
  }


  /* Responding to SYN-ACK packets: (master node) */
  else if (flag == PKT_FLAG_SYN_ACK) {
    printf("Received packet type: SYN-ACK\n");

    /* parsing filename, content size and num packets from SYN-ACK packet */
    sscanf(packet.buf, "File: %s\nContent Size: %d\nRequired Packets: %d\n",
           filename, &c_size, &n_packs);

    printf("Confirmed file: %s\nFile size: %d\nRequired Num Packets: %d\n",
           filename, c_size, n_packs);

    /* sending info to front-end */
    printf("{*debug*} Sending SYN-ACK packet info to front-end for headers.\n");
    send_hdr_to_fe(hdr.com_buf, c_size);
    printf("{*debug*} Stored SYN-ACK packet buf info in com_buf.\n");

    /* respond to SYN-ACK packet with ACK packet */
    /* CHECK s_num = 0 */
    data_pkt = create_packet(d_port, s_port, 0, filename, PKT_FLAG_ACK,
                            packet.header.com_buf);
    printf("Sending ACK packet...\n");
  }

  /* Responding to ACK packets: (peer node) */
  else if (flag == PKT_FLAG_ACK) {
    printf("Received packet type: ACK\n");

    /* parsing filename from ACK packet buffer */
    sscanf(packet.buf, "Ready to send: %s\n", filename);

    printf("Requested file: %s\n", filename);

    /* respond to ACK packet with DATA packet */
    data_pkt = create_packet(d_port, s_port, s_num, filename, PKT_FLAG_DATA,
                             packet.header.com_buf);
    printf("Sending DATA packet...\n");
  }

  /* Responding to DATA packets: (master node) */
  else if(flag == PKT_FLAG_DATA) {
    /* Finding filename for response packets */
    Node* n = find_node_by_hostname(node_dir, hostaddrp);
    strncpy(filename, n->content_path, strlen(n->content_path));

    if((hdr.flag & PKT_FIN_MASK) > 0) {
      /* Terminating data packet (last packet); Respond with FIN packet */
      printf("Received packet type: DATA-FIN\n");

      // printf("{*debug*} DATA-FIN BUF: \n%s\n", packet.buf);

      /* write buf data to frontend */
      /* SEND TO FE: "1 {data}\n" */
      printf("{*debug*} Sending DATA-FIN packet info to front-end for data.\n");
      send_data_to_fe(hdr.com_buf, packet.buf, 1);
      printf("{*debug*} Stored DATA-FIN packet buf info in com_buf.\n");

      /* respond to DATA-FIN packet with FIN packet */
      data_pkt = create_packet(d_port, s_port, s_num, filename, PKT_FLAG_FIN,
                               hdr.com_buf);
      printf("Sending FIN packet...\n");
    }

    else {
      /* Non-terminating data packet; Respond with ACK packet */
      printf("Received packet type: DATA\n");

      // printf("{*debug*} DATA BUF: \n%s\n", packet.buf);

      /* write buf data to frontend */
      /* SEND TO FE: "1 {data}\n" */
      printf("{*debug*} Sending DATA packet info to front-end for data.\n");
      send_data_to_fe(hdr.com_buf, packet.buf, 0);
      printf("{*debug*} Stored DATA packet buf info in com_buf.\n");

      /* respond to DATA packet with ACK packet */
      data_pkt = create_packet(d_port, s_port, s_num + 1,
        filename, PKT_FLAG_ACK, hdr.com_buf);
      printf("Sending ACK packet...\n");
    }
  }

  /* Responding to FIN packets: (peer node) */
  else if(flag == PKT_FLAG_FIN) {
    printf("Received packet type: FIN\n");
    printf("{*debug*} Sending FIN packet info to front-end for data.\n");
    send_data_to_fe(hdr.com_buf, packet.buf, 2);
    printf("{*debug*} Stored FIN packet buf info in com_buf.\n");

    printf("Finished sending.\n");
    return 0;
  }

  printf("Packet destination: %s\n", hostaddrp);

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


  bzero((char *) self_addr, sizeof(*self_addr));
  /* we are using the Internet */
  self_addr->sin_family = AF_INET;
  /* accept reqs to any IP addr */
  self_addr->sin_addr.s_addr = htonl(INADDR_ANY);
  /* port to listen on */
  self_addr->sin_port = htons((unsigned short) port_be);

  /* bind: associate the listening socket with a port */
  if (bind(sockfd_be, (struct sockaddr *) self_addr, sizeof(*self_addr)) < 0){
    error("ERROR on binding back-end socket with port");
  }

  /* create node directory */
  node_dir = create_node_dir(MAX_NODES);
  /* Created neighbor dir */
  neighbor_dir = create_neighbor_dir(100);

  return sockfd_be;
}

/*
 *  TODO - replace writing headers with returning flag values
 */
int peer_add_response(char* BUF) {
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
    return -2;
  }

  /* parsing string reps to ints and freeing memory */
  port = parse_str_2_int(port_c);
  rate = parse_str_2_int(rate_c);
  free(port_c);
  free(rate_c);

  /* creating node with parsed data */
  Node* n = create_node(filepath, hostname, port, rate);
  if(!add_node(node_dir, n)){
    return -2;
  }

  /* Status printing */
  printf("Created peer node info:\n");
  printf("Node hostname: %s\nNode port: %d\n", n->ip_hostname, n->port);
  printf("Node content: %s\nNode rate: %d\n",
         n->content_path, n->content_rate);

  return 1;
}

/*
 *  TODO - replace writing headers/data with returning flag values
 */
int peer_view_response(char* filepath, char* file_type, uint16_t port_be,
                       int sockfd_be, char* COM_BUF){
  Pkt_t syn_packet;
  int sent;
  struct sockaddr_in peer_addr;
  socklen_t peer_addr_len;

  /* finding node with requested content */
  Node *node = check_content(node_dir, filepath);

  if(!node) {
    /* 500 Error --> Failure to find content in node directory
     * TODO      --> flag (-1): no node found */
    return -1;
  }

  printf("Known node with content: %s:%d\n", node->ip_hostname, node->port);

  peer_addr = node->node_addr;
  peer_addr_len = sizeof(peer_addr);

  /* TODO: send a SYN to the peer node */
  syn_packet = create_packet(port_be, port_be, 0, node->content_path,
                             PKT_FLAG_SYN, COM_BUF);

  printf("Sending SYN packet...\n");

  sent = sendto(sockfd_be, &syn_packet, sizeof(syn_packet), 0,
                  (struct sockaddr*) &peer_addr, peer_addr_len);

  if(sent < 0) {
  /* TODO buffer info: "send SYN fail" */
    printf("Error on sending SYN packet.\n");
    return -2;
  }

  printf("Sent!\n\n");

  return 0;
}

/*
 *  TODO - replace writing headers with returning flag values
 */
int peer_rate_response(int connfd, char* BUF, struct thread_data *ct){
  char buf[MAXLINE];
  char* rate_c = malloc(sizeof(char) * MAXLINE);

  bzero(buf, BUFSIZE);
  strcpy(buf, BUF);

  /* TODO this returns something */
  parse_peer_config_rate(BUF, rate_c);

  free(rate_c);

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

void send_hdr_to_fe(char* com_buf, int file_size) {
  char buf[COM_BUFSIZE];
  sprintf(buf, "2 %d\n", file_size);
  pthread_mutex_lock(&mutex);
  memcpy(com_buf, buf, strlen(buf));
  pthread_mutex_unlock(&mutex);
}

void send_data_to_fe(char* com_buf, char* data, int fin_flag) {
  char buf[COM_BUFSIZE];
  if(fin_flag == 0) { sprintf(buf, "1 %s\n", data); }
  else if(fin_flag == 1) { sprintf(buf, "3 %s\n", data); }
  else if(fin_flag == 2) { sprintf(buf, "4 %s\n", data); }
  pthread_mutex_lock(&mutex);
  memcpy(com_buf, buf, strlen(buf));
  pthread_mutex_unlock(&mutex);
}

void handle_uuid_rqt(int connfd, char* fname){

  char* uuid;
  char json_content[BUFSIZE];
  bzero(json_content, BUFSIZE);

  uuid = get_config_field(fname, CF_TAG_UUID, 0);
  sprintf(json_content, "{\"uuid\":\"%s\"}", uuid);
  write_json_content(connfd, json_content);
}

int handle_add_uuid_rqt(char* buf, char* fname){

  Node* n;

  char* num_peers_c;
  int num_peers;
  int i = 0;

  char* filepath = malloc(sizeof(char) * MAXLINE);
  char* uuid = malloc(sizeof(char) * MAXLINE);
  char* rate = malloc(sizeof(char) * MAXLINE);

  char* node = malloc(sizeof(char) * MAXLINE);
  char* peer_uuid = malloc(sizeof(char) * MAXLINE);
  char* hostname = malloc(sizeof(char) * MAXLINE);
  char* fe_port = malloc(sizeof(char) * MAXLINE);
  char* be_port = malloc(sizeof(char) * MAXLINE);
  char* metric = malloc(sizeof(char) * MAXLINE);

  bzero(filepath, strlen(filepath));
  bzero(uuid, strlen(uuid));
  bzero(rate, strlen(rate));

  parse_peer_add_uuid(buf, filepath, uuid, rate);

  num_peers_c = get_config_field(fname, CF_TAG_PEER_COUNT, 0);
  if(num_peers_c) {
    num_peers = atoi(num_peers_c);
  }
  else {
    num_peers = 0;
  }

  bzero(node, strlen(node));
  bzero(peer_uuid, strlen(peer_uuid));
  bzero(hostname, strlen(hostname));
  bzero(fe_port, strlen(fe_port));
  bzero(be_port, strlen(be_port));
  bzero(metric, strlen(metric));

  node = get_config_field(fname, CF_TAG_PEER_INFO, i);

  parse_neighbor_info(node, peer_uuid, hostname, fe_port, be_port, metric);


  while(i < num_peers){

    bzero(node, strlen(node));
    bzero(peer_uuid, strlen(peer_uuid));
    bzero(hostname, strlen(hostname));
    bzero(fe_port, strlen(fe_port));
    bzero(be_port, strlen(be_port));
    bzero(metric, strlen(metric));

    node = get_config_field(fname, CF_TAG_PEER_INFO, i);

    parse_neighbor_info(node, peer_uuid, hostname, fe_port, be_port, metric);

    if(strcmp(peer_uuid, uuid) == 0){
      n = create_node(filepath, hostname, atoi(be_port), atoi(rate));
      free(peer_uuid);
      free(hostname);
      free(fe_port);
      free(be_port);
      free(metric);
      free(uuid);
      free(rate);

      if(!add_node(node_dir, n)) return -2;

      /* Status printing */
      printf("Created peer node info:\n");
      printf("Node hostname: %s\nNode port: %d\n", n->ip_hostname, n->port);
      printf("Node content: %s\nNode rate: %d\n",
             n->content_path, n->content_rate);
      return 1;
    }

    i ++;
  }

  return -2;
}

void handle_neighbors_rqt(int connfd, char* fname){
  int num_peers;
  char* num_peers_c;

  char json_content[JSON_BUFSIZE] = "\0";

  char* neighbor;
  char  node_name[BUFSIZE];
  char  neighbor_json[BUFSIZE];
  char* uuid = malloc(sizeof(char) * BUFSIZE);
  char* hostname = malloc(sizeof(char) * BUFSIZE);
  char* fe_port = malloc(sizeof(char) * BUFSIZE);
  char* be_port = malloc(sizeof(char) * BUFSIZE);
  char* metric = malloc(sizeof(char) * BUFSIZE);


  int i = 0;

  num_peers_c = get_config_field(fname, CF_TAG_PEER_COUNT, 0);
  /* NULL check! */
  if(num_peers_c) {
    num_peers = atoi(num_peers_c);
  }
  else {
    num_peers = 0;
  }

  strcat(json_content, "[");
  while(i < num_peers){

    bzero(neighbor, strlen(neighbor));
    bzero(neighbor_json, strlen(neighbor_json));
    bzero(uuid, strlen(uuid));
    bzero(node_name, strlen(node_name));
    bzero(hostname, strlen(hostname));
    bzero(fe_port, strlen(fe_port));
    bzero(be_port, strlen(be_port));
    bzero(metric, strlen(metric));

    neighbor = get_config_field(fname, CF_TAG_PEER_INFO, i);

    printf("%s\n", neighbor);
    parse_neighbor_info(neighbor, uuid, hostname, fe_port, be_port, metric);

    /* TODO: Find Node name */
    sprintf(node_name, "name_%d", i);
    sprintf(neighbor_json, "{\"uuid\":\"%s\", \"name\":\"%s\", \"host\":\"%s\", \"frontend\":%s, \"backend\":%s, \"metric\":%s}",
            uuid, node_name, hostname, fe_port, be_port, metric);

    strcat(json_content, neighbor_json);
    i ++;
    if(i < num_peers){
      strcat(json_content, ", ");
    }
  }
  strcat(json_content, "]");

  write_json_content(connfd, json_content);

  free(uuid);
  free(hostname);
  free(fe_port);
  free(be_port);
  free(metric);
}

void handle_add_neighbor_rqt(char* buf, char* fname){

  char* num_peers_c;
  int new_peer_num;

  int update_flag;

  char value[BUFSIZE];
  char* uuid = malloc(sizeof(char) * BUFSIZE);
  char* host = malloc(sizeof(char) * BUFSIZE);
  char* fe_port = malloc(sizeof(char) * BUFSIZE);
  char* be_port = malloc(sizeof(char) * BUFSIZE);
  char* metric = malloc(sizeof(char) * BUFSIZE);

  bzero(value, strlen(value));
  bzero(uuid, strlen(uuid));
  bzero(host, strlen(host));
  bzero(fe_port, strlen(fe_port));
  bzero(be_port, strlen(be_port));
  bzero(metric, strlen(metric));

  parse_peer_add_neighbor(buf, uuid, host, fe_port, be_port, metric);

  num_peers_c = get_config_field(fname, CF_TAG_PEER_COUNT, 0);
  if(num_peers_c) {
    new_peer_num = atoi(num_peers_c);
  }
  else {
    new_peer_num = 0;
  }

  sprintf(value, "%s,%s,%s,%s,%s", uuid, host, fe_port, be_port, metric);

  update_flag = update_config_file(fname, CF_TAG_PEER_INFO,
                                   new_peer_num, value, NULL);
  return;
}

void handle_search_rqt(int connfd, int sockfd, char* path, char* fname){
  char* my_uuid;
  char* content_path = malloc(sizeof(char) * MAXLINE);
  int my_port;
  char* filepath;
  char search_list[BUFSIZE];
  char json_content[MAX_DATA_SIZE];
  bzero(json_content, BUFSIZE);
  int num_neighbors, TTL, search_interval;

  int n, n_port;
  char* n_info, n_host;
  struct sockaddr_in n_addr;
  struct hostent *n_server;
  socklen_t n_addr_len;

  char* BUF = malloc(sizeof(char) * BUFSIZE);
  char* info = malloc(sizeof(char) * BUFSIZE);
  char* data = malloc(sizeof(char) * BUFSIZE);

  char* gossip_buf = malloc(sizeof(char) * BUFSIZE);
  my_uuid = get_config_field(fname, CF_TAG_UUID, 0);
  my_port = atoi(get_config_field(fname, CF_TAG_BE_PORT, 0));
  content_path = get_config_field(fname, CF_TAG_CONTENT_DIR, 0);
  // See if filepath is in current node
  filepath = strcat(content_path, path);
  FILE *fp = fopen(filepath, "r");
  bzero(search_list, BUFSIZE);
  if(fp != NULL){
    sprintf(search_list, "[\"%s\"]", my_uuid);
  }
  else{
    strcpy(search_list, "[]");
  }

  TTL = atoi(get_config_field(fname, CF_TAG_SEARCH_TTL, 0));
  search_interval = atoi(get_config_field(fname, CF_TAG_SEARCH_INT, 0));
  // Get the number of current neighbors
  num_neighbors = atoi(get_config_field(fname, CF_TAG_PEER_COUNT, 0));

  if(num_neighbors == 0){
    sprintf(json_content, "[{\“content\”:\“%s\”, \“peers\”:%s}]", path, search_list);
    write_json_content(connfd, json_content);
    return;
  }

  Pkt_t packet;
  n = 0;
  while(TTL > 0){
    bzero(BUF, CBUFSIZE);
    bzero(info, BUFSIZE);
    bzero(data, BUFSIZE);

    pthread_mutex_lock(&mutex);
    memcpy(BUF, gossip_buf);
    memset(gossip_buf, '\0', BUFSIZE);
    pthread_mutex_unlock(&mutex);

    if (BUF[0] != '\0') {
      /* BUF has info for FE; parse type of response and data */
      
    }

    if(n <= num_neighbors){
      printf("1\n");
      n_info = get_config_field(fname, CF_TAG_PEER_INFO, n);
      printf("2\n");
      n_port = atoi(parse_peer_info(n_info, BE_PORT));
      n_host = parse_peer_info(n_info, HOSTNAME);

      printf("%s\n", n_info);
      printf("%d\n", n_port);
      printf("%s\n", n_host);

      packet = create_exchange_packet(n_port, my_port, TTL, path, gossip_buf,
                                      search_list);

      /* build the neighbor's Internet address */
      n_server = gethostbyname(n_host);
      bzero((char *) &n_addr, sizeof(n_addr));
      n_addr.sin_family = AF_INET;
      bcopy((char *)n_server->h_addr,
              (char *)&n_addr.sin_addr.s_addr, n_server->h_length);
      n_addr.sin_port = htons(n_port);

      /* send the message to the neighbor */
      n_addr_len = sizeof(n_addr);

      printf("SENDING HERE\n");
      sendto(sockfd, &packet, sizeof(packet), 0,
            (struct sockaddr *) &n_addr, n_addr_len);
      n ++;
    }
    sleep(search_interval);
  }

  sprintf(json_content, "[{\“content\”:\“%s\”, \“peers\”:%s}]", path, search_list);
  write_json_content(connfd, json_content);
}

void* advertise(void* ptr){
  printf("About to advertise: \n");

  int sequence_num = 0;
  int sockfd = *(int*)ptr;        /* parsing sockfd from pointer arg */
  char buf[MAX_DATA_SIZE];
  int num_neighbors;
  struct Neighbor* neighbors;

  char* neighbor;
  char neighbor_json[BUFSIZE];
  char* uuid = malloc(sizeof(char) * 100);
  char* hostname = malloc(sizeof(char) * 100);
  char* fe_port = malloc(sizeof(char) * 100);
  char* be_port = malloc(sizeof(char) * 100);
  char* metric = malloc(sizeof(char) * 100);

  while(1){
     sleep(10);
     num_neighbors = neighbor_dir->cur_nbrs;
     neighbors = neighbor_dir->nbr_list;

     int i = 0;

     bzero(neighbor_json, strlen(neighbor_json));
     strcat(neighbor_json, "{");
     while(i < num_neighbors){

       bzero(neighbor, strlen(neighbor));
       bzero(uuid, strlen(uuid));
       bzero(hostname, strlen(hostname));
       bzero(be_port, strlen(be_port));
       bzero(metric, strlen(metric));

       neighbor = get_config_field(CF_DEFAULT_FILENAME, CF_TAG_PEER_INFO, i);

       parse_neighbor_info(neighbor, uuid, hostname, fe_port, be_port, metric);

       sprintf(neighbor, "\"%s\":%s", uuid, metric);

       strcat(neighbor_json, neighbor);
       i ++;
       if(i < num_neighbors){
         strcat(neighbor_json, ", ");
       }
     }
     strcat(neighbor_json, "}");

     bzero(buf, strlen(buf));

     sprintf(buf, "%s\n", neighbor_json);

     Pkt_t ad;
     struct sockaddr_in peeraddr;
     struct hostent *peer_server;
     socklen_t peer_addr_len;

     int peer_port;
     char* peer_host;
     int n;

     for(i = 0; i < num_neighbors; i ++){

       struct Neighbor peer = neighbors[i];

       peer_port = peer.port;
       peer_host = peer.hostname;

       ad = create_packet(peer_port, 0, sequence_num, buf, PKT_FLAG_AD,
                          NULL);

       peer_server = gethostbyname(peer_host);

       /* build the server's Internet address */
       bzero((char *) &peeraddr, sizeof(peeraddr));
       peeraddr.sin_family = AF_INET;
       bcopy((char *)peer_server->h_addr,
             (char *)&peeraddr.sin_addr.s_addr, peer_server->h_length);
       peeraddr.sin_port = htons(peer_port);

       /* send the message to the server */
       peer_addr_len = sizeof(peeraddr);

       n = sendto(sockfd, &ad, sizeof(ad), 0,
                  (struct sockaddr *) &peeraddr, peer_addr_len);
     }
     sequence_num ++;
  }
}
