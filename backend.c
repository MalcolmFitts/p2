/*  (Version: p2)
 * backend.c
 *    - created for use with "bbbserver.c"
 *	  - implementations of server back-end functions
 *	  - functions and data structures defined in "backend.h"
 */

#include "backend.h"

/* Global - Directory for node referencing   */
Node_Dir* node_dir;

void* handle_be(void* ptr) {

  int sockfd = *(int*)ptr;        /* parsing sockfd from pointer arg */

  struct sockaddr_in sender_addr; /* packet sender's address */
  socklen_t sender_addr_len = sizeof(sender_addr);

  Pkt_t packet;                   /* packet struct */

  int sent_status;                /* sent bytes */
  int recv_status;                /* received bytes */


  while(1) {
    printf("Waiting for packet on backend...\n");

    /* receiving packet and writing into p_buf */
    bzero(&packet, sizeof(packet));
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
    if(type == PKT_FLAG_UNKNOWN) {
      printf("Server received packet with unknown flag.\n");
      printf("{temp} ignoring this packet for now.\n\n");
    }
    else if(type == PKT_FLAG_CORRUPT) {
      printf("Server received packet with corrupt flag.\n");
      printf("{temp} ignoring this packet for now.\n\n");
    }

    /* responses for SYN, ACK, SYN-ACK and DATA packets */
    else {
      sent_status = serve_content(packet, sockfd, sender_addr, type);

      if(sent_status < 0) {
	       printf("Error: something went wrong with serving content after the fact.\n");
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
  struct hostent *hostp;          /* client host info */
  char *hostaddrp;                /* dotted decimal host addr string */

  /* Parsing data about packet sender */
  hostp = gethostbyaddr((const char *)&server_addr.sin_addr.s_addr,
        sizeof(server_addr.sin_addr.s_addr), AF_INET);
  hostaddrp = inet_ntoa(server_addr.sin_addr);

  printf("Packet sender: %s (%s)\n", hostp->h_name, hostaddrp);

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

    /* parsing filename, content size and num packets from SYN-ACK packet buffer */
    sscanf(packet.buf, "File: %s\nContent Size: %d\nRequired Packets: %d\n", filename, &c_size, &n_packs);

    printf("Confirmed file: %s\nFile size: %d\nRequired Num Packets: %d\n", filename, c_size, n_packs);

    /* sending info to front-end */
    send_hdr_to_fe(hdr.com_buf, c_size);

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

    /* CHECK 1 - node_dir might be error */
    if(!node_dir) {
      printf("Node dir null.\n");
      return -1;
    }
    else{
      printf("Node_dir is fine.\n");
    }

    /* Finding filename for response packets */
    Node* n = find_node_by_hostname(node_dir, hostaddrp);

    /* CHECK 2 - node might be error */

    if(!n) {
      printf("Node is null.\n");
      return -1;
    }
    else{
      printf("found node in dir.\n");
    }

    strncpy(filename, n->content_path, strlen(n->content_path));

    if((flag & PKT_FIN_MASK) > 0) {
      /* Terminating data packet (last packet); Respond with FIN packet */
      printf("Received packet type: DATA-FIN\n");

      /* write buf data to frontend */
      send_data_to_fe(hdr.com_buf, packet.buf, 1);

      /* respond to DATA-FIN packet with FIN packet */
      data_pkt = create_packet(d_port, s_port, s_num, filename, PKT_FLAG_FIN, hdr.com_buf);
      printf("Sending FIN packet...\n");
    }

    else {
      /* Non-terminating data packet; Respond with ACK packet */
      /* SEND TO FE: "1 {data}\n" */
      printf("Received packet type: DATA\n");

      /* write buf data to frontend */
      send_data_to_fe(hdr.com_buf, packet.buf, 0);

      /* respond to DATA packet with ACK packet */
      data_pkt = create_packet(d_port, s_port, s_num + 1,
        filename, PKT_FLAG_ACK, hdr.com_buf);
      printf("Sending DATA packet...\n");
    }
  }

  /* Responding to FIN packets: (peer node) */
  else if(flag == PKT_FLAG_FIN) {
    printf("Received packet type: FIN\n");
    printf("Finished sending.\n");
    return 0;
  }

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

  return sockfd_be;
}

/*
 *  TODO - replace writing headers with returning flag values
 */
int peer_add_response(int connfd, char* BUF, struct thread_data *ct) {
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
    return -1;
  }

  /* parsing string reps to ints and freeing memory */
  port = parse_str_2_int(port_c);
  rate = parse_str_2_int(rate_c);
  free(port_c);
  free(rate_c);

  /* creating node with parsed data */
  Node* n = create_node(filepath, hostname, port, rate);
  if(!add_node(node_dir, n)){
    return -1;
  }

  /* CHECK - debug printing */
  printf("Node hostname: %s\nNode port: %d\n", n->ip_hostname, n->port);
  printf("Node content: %s\nNode rate: %d\n\n", n->content_path, n->content_rate);

  /* TODO      --> write in front-end */
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
  write_empty_header(connfd);

  printf("Wrote server info to client.\n");

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

void send_hdr_to_fe(char* com_buf, int file_size){
  char buf[COM_BUFSIZE];
  sprintf(buf, "%d %d\n", 2, file_size);
  pthread_mutex_lock(&mutex);
  memcpy(com_buf, buf, strlen(buf));
  pthread_mutex_unlock(&mutex);
}

void send_data_to_fe(char* com_buf, char* data, int fin_flag){
  char buf[COM_BUFSIZE];
  sprintf(buf, "%d %s\n", 1, data);
  pthread_mutex_lock(&mutex);
  memcpy(com_buf, buf, strlen(buf));
  pthread_mutex_unlock(&mutex);
}
