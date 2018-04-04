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
    // else if(type == PKT_FLAG_CORRUPT) {
    //   printf("Server received packet with corrupt flag.\n");
    //   printf("{temp} ignoring this packet for now.\n\n");
    // }

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

      printf("{*debug*} DATA-FIN-FIN BUF: \n%s\n", packet.buf);

      /* write buf data to frontend */
      /* SEND TO FE: "1 {data}\n" */
      printf("{*debug*} Sending DATA-FIN packet info to front-end for data.\n");
      send_data_to_fe(hdr.com_buf, packet.buf, 1);
      printf("{*debug*} Stored DATA-FIN packet buf info in com_buf.\n");

      /* respond to DATA-FIN packet with FIN packet */
      data_pkt = create_packet(d_port, s_port, s_num, filename, PKT_FLAG_FIN, hdr.com_buf);
      printf("Sending FIN packet...\n");
    }

    else {
      /* Non-terminating data packet; Respond with ACK packet */
      printf("Received packet type: DATA\n");

      printf("{*debug*} DATA BUF: \n%s\n", packet.buf);

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
int peer_add_response(char* BUF, char* resp_buf) {
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
    return 0;
  }

  /* parsing string reps to ints and freeing memory */
  port = parse_str_2_int(port_c);
  rate = parse_str_2_int(rate_c);
  free(port_c);
  free(rate_c);

  /* creating node with parsed data */
  Node* n = create_node(filepath, hostname, port, rate);
  if(!add_node(node_dir, n)){
    return 0;
  }

  /* Status printing */
  printf("Created peer node info:\n");
  printf("Node hostname: %s\nNode port: %d\n", n->ip_hostname, n->port);
  printf("Node content: %s\nNode rate: %d\n", n->content_path, n->content_rate);

  sprintf(resp_buf, "Peer Add Success!\nNode hostname: %s\nNode port: %d\nNode content: %s\nNode rate: %d\n",
    n->ip_hostname, n->port, n->content_path, n->content_rate);

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
