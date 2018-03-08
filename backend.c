/*  (Version: p2)
 * backend.c
 *    - created for use with "bbbserver.c"
 *	  - implementations of server back-end functions
 *	  - functions and data structures defined in "backend.h"
 */

#include "backend.h"

void* recieve_pkt(void* ptr) {
  Recv_t* rec = ptr;

  int sockfd = rec->sockfd;
  struct sockaddr_in* sender_addr = NULL;
  socklen_t addr_len = 10;

  char p_buf[MAX_PACKET_SIZE] = {0};
  // char* content_path = node->content_path;
  int n_sent;
  int n_recv;

  // uint16_t s_port = node->port;


  while(1) {
    printf("Waiting for Packet\n");
    /* receiving packet and writing into p_buf */
    n_recv = recvfrom(sockfd, p_buf, MAX_PACKET_SIZE, 0,
		      (struct sockaddr *) sender_addr, &addr_len);

    printf("Recieved Packet!\n");

    if(n_recv < 0) {
      /* TODO - Error on receive packet */
    }

    /* parsing received buffer and getting packet type */
    Pkt_t* recv_pkt = parse_packet(p_buf);
    int type = get_packet_type(recv_pkt);

    /* CHECK accepted types (SYN and ACK)*/
    /* TODO add FIN implimentation */
    if (type == PKT_FLAG_UNKNOWN || type == PKT_FLAG_CORRUPT){
      /* TODO corrupted packet */
    }

    else{
      n_sent = serve_content(recv_pkt, sockfd, sender_addr, type);

      if(n_sent < 0){
	       /* TODO error on send */
      }
    }
  }
}

int serve_content(Pkt_t* packet, int sockfd, struct sockaddr_in* serveraddr, int flag){
  /* parsing data from packet */
  P_Hdr hdr = packet->header;

  /* flipping ports to send packet back */
  uint16_t s_port = hdr.dest_port;
  uint16_t d_port = hdr.source_port;

  /* CHECK s_num */
  uint32_t s_num = hdr.seq_num;

  /* local vars */
  char filename[MAX_DATA_SIZE];
  socklen_t server_len;
  int n_set;
  Pkt_t* data_pkt;

  /* Create the packet to be sent */
  if (flag == PKT_FLAG_ACK) {
    /* parsing filename from ACK packet buffer */
    sscanf(packet->buf, "Ready to send: %s\n", filename);

    /* respond to ACK packet with data */
    data_pkt = create_packet(d_port, s_port, s_num, filename, PKT_FLAG_DATA);
  }

  else {
    /* parsing filename from SYN packet buffer */
    sscanf(packet->buf, "Request: %s\n", filename);

    /* respond to SYN packet with SYN-ACK */
    data_pkt = create_packet(d_port, s_port, s_num, filename, PKT_FLAG_SYN_ACK);
  }

  /* CHECK - p_size */
  int p_size = sizeof(*data_pkt);
  char* data_pkt_wr = writeable_packet(data_pkt);

  server_len = sizeof(*serveraddr);
  n_set = sendto(sockfd, data_pkt_wr, p_size, 0,
		 (struct sockaddr *) serveraddr, server_len);

  return n_set;
}

int init_backend(int port_be){
  int listenfd_be;
  int optval_be;

  listenfd_be = socket(AF_INET, SOCK_DGRAM, 0);
  if(listenfd_be < 0)
    error("ERROR opening back-end socket");

  /* setsockopt: Handy debugging trick that lets
   * us rerun the server immediately after we kill it;
   * otherwise we have to wait about 20 secs.
   * Eliminates "ERROR on binding: Address already in use" error.
   */
  optval_be = 1;

  setsockopt(listenfd_be, SOL_SOCKET, SO_REUSEADDR,
    (const void *)&optval_be, sizeof(int));

  /* build the server's back end internet address */
  struct sockaddr_in self_addr = NULL;
  self_addr.sin_family = AF_INET; /* we are using the Internet */
  self_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* accept reqs to any IP addr */
  self_addr.sin_port = htons((short)port_be); /* port to listen on */


  /* bind: associate the listening socket with a port */
  if (bind(listenfd_be, (struct sockaddr *) &self_addr, sizeof(self_addr) < 0)
    error("ERROR on binding back-end socket with port");

  return listenfd_be;
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

Pkt_t* create_packet (uint16_t dest_port, uint16_t s_port, unsigned int s_num,
  char* filename, int flag) {

  Pkt_t *packet = malloc(sizeof(Pkt_t));
  P_Hdr *hdr = malloc(sizeof(P_Hdr));

  if(!packet || !hdr) return NULL;

  /* CHECK - fixing file buffer size to max size */
  char f_buf[MAX_DATA_SIZE] = {0};

  hdr->source_port = s_port;
  hdr->dest_port = dest_port;
  hdr->seq_num = s_num;

  /* CHECK - fixing size of packets to max size for now */
  hdr->length = MAX_PACKET_SIZE;

  if(flag == PKT_FLAG_DATA) {
    /* data packet */

    /* set flag to DATA */
    hdr->flag = (1 << PKT_FLAG_DATA);

    /* checking file contents */
    FILE* fp = fopen(filename, "r");

    /* CHECK - might want to return info (in buf) on not finding file */
    if(!fp) return NULL;
    rewind(fp);

    /* storing file contents in f_buf and closing file */
    /* CHECK - might want to save this value */
    fread(f_buf, 1, MAX_DATA_SIZE, fp);
    fclose(fp);
  }

  else if(flag == PKT_FLAG_ACK) {
    /* ACK packet */

    /* set flag to ACK */
    hdr->flag = (1 << PKT_FLAG_ACK);

    /*
     *  CHECK - not sure how to send ACK info - hdr->ACK_NUM (need to impelment)
     *  NOT in buffer here
     */
    sprintf(f_buf, "Ready to send: %s\n", filename);
  }
  else if(flag == PKT_FLAG_SYN) {
    /* SYN packet */

    /* set flag to SYN */
    hdr->flag =  (1 << PKT_FLAG_SYN);

    /* CHECK - not sure how to send SYN info - maybe in buffer */
    /* TODO - Data offset here to pass options in the data buffer (hdr->data_offset) */
    sprintf(f_buf, "Request: %s\n", filename);
  }
  else if(flag == PKT_FLAG_SYN_ACK) {
    /* SYN-ACK packet */

    /* set flag to SYN-ACK */
    hdr->flag =  (1 << PKT_FLAG_SYN_ACK);

    /* CHECK - not sure how to send SYN-ACK info - maybe in buffer */
    /* TODO - Data offset here to pass options in the data buffer */

    /* attempting to find file */
    FILE* fp = fopen(filename, "r");
    if(fp) {
      /* found file --> get file info */
      struct stat *fStat;
      fStat = malloc(sizeof(struct stat));
      stat(filename, fStat);
      int f_size = fStat->st_size;

      /* storing info in buffer */
      sprintf(f_buf, "File: %s\nContent Size: %d\n", filename, f_size);

      /* freeing mem and closing file */
      free(fStat);
      fclose(fp);
    }
    else if(flag == PKT_FLAG_FIN) {

      /* set flag to FIN */
      hdr->flag = (1 << PKT_FLAG_FIN);

    }
    else{
      /* info on no file found in buffer */
      sprintf(f_buf, "File: Not Found\nContent Size: -1\n");
    }

  }

  else if (flag == PKT_FLAG_FIN){

    /* set flag to FIN */
    hdr->flag = PKT_FIN_MASK;

  }
  else {
    /* invalid file creation flag */
    free(hdr);
    free(packet);
    return NULL;
  }

  strcpy(packet->buf, f_buf);

  /* TODO CHECK - calculate checksum value */
  uint16_t c = calc_checksum(hdr);
  hdr->checksum = c;

  packet->header = *hdr;

  return packet;
}

void discard_packet(Pkt_t *packet) {
  P_Hdr* ref = &(packet->header);
  free(ref);
  free(packet);
}

uint16_t calc_checksum(P_Hdr* hdr) {
  /* parsing source_port, dest_port and length directly */
  uint16_t s_p = hdr->source_port;
  uint16_t d_p = hdr->dest_port;
  uint16_t len = hdr->length;
  uint16_t flag = hdr->flag;

  /* splitting up both seq num and ack into two 16-bit nums */
  /* CHECK - might have to & with S_INT_MASK, but dont think so cause unsigned int */
  uint16_t seq_num1 = (uint16_t) ((hdr->seq_num) >> 16);
  uint16_t seq_num2 = (uint16_t) (hdr->seq_num);

  /* binary addition of relevant values */
  uint16_t x = ((((s_p + d_p) + len) + flag) + seq_num1) + seq_num2;

  /* one's complement of addition */
  uint16_t res = ~x;
  return res;
}

Pkt_t* parse_packet(char* buf) {
  /* allocating mem for structs */
  Pkt_t *pkt = malloc(sizeof(Pkt_t));
  P_Hdr *hdr = malloc(sizeof(P_Hdr));

  /* creating vars to directly parse into */
  uint16_t sp;     /* buf {0, 1}        */
  uint16_t dp;     /* buf {2, 3}        */
  uint16_t len;    /* buf {4, 5}        */
  uint16_t csum;   /* buf {6, 7}        */
  uint32_t snum;   /* buf {8, 9, 10, 11} */
  uint16_t flag;   /* buf {12, 13}      */
  uint32_t off;    /* buf {14, 15}      */

  /* parsing header data from buf into header*/
  char* b_ptr = buf;
  memcpy(&sp, b_ptr, 2);
  hdr->source_port = sp;

  b_ptr = &(buf[2]);
  memcpy(&dp, b_ptr, 2);
  hdr->dest_port = dp;

  b_ptr = &(buf[4]);
  memcpy(&len, b_ptr, 2);
  hdr->length = len;

  b_ptr = &(buf[6]);
  memcpy(&csum, b_ptr, 2);
  hdr->checksum = csum;

  b_ptr = &(buf[8]);
  memcpy(&snum, b_ptr, 4);
  hdr->seq_num = snum;

  b_ptr = &(buf[12]);
  memcpy(&flag, b_ptr, 2);
  hdr->flag = flag;

  b_ptr = &(buf[14]);
  memcpy(&off, b_ptr, 4);
  hdr->data_offset = off;

  /* assigning header */
  pkt->header = *hdr;

  /* copying packet's buffer data */
  b_ptr = &(buf[16]);
  /* CHECK - this might copy too much data OR data from outside packet */
  memcpy(pkt->buf, b_ptr, MAX_DATA_SIZE);

  return pkt;
}

char* writeable_packet(Pkt_t* packet) {
  int p_size = sizeof(*packet);

  /* creating buffer and copying all packet data into it (header + buf) */
  char* p_buf = malloc(p_size * sizeof(char));
  memcpy(p_buf, packet, p_size);

  return p_buf;
}

int get_packet_type(Pkt_t *packet) {
  P_Hdr *hdr = &(packet->header);
  uint16_t flag = hdr->flag;

  /* validating via checksum */
  uint16_t c = calc_checksum(hdr);
  if(c != hdr->checksum) return 0;

  if ((flag & PKT_SYN_ACK_MASK) > 0) {
    return PKT_FLAG_SYN_ACK;    /* SYN-ACK packet */
  }
  else if ((flag & PKT_SYN_MASK) > 0) {
    return PKT_FLAG_SYN;        /* SYN packet     */
  }
  else if ((flag & PKT_ACK_MASK) > 0) {
    return PKT_FLAG_ACK;        /* ACK packet     */
  }
  else if ((flag & PKT_DATA_MASK) > 0){
    return PKT_FLAG_DATA;       /* DATA packet    */
  }

  else if ((flag & PKT_FIN_MASK) > 0){
    return PKT_FLAG_FIN;        /* FIN packet */
  }

  /* should not happen - not sure what kind of packet this is */
  return -1;
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

char* sync_node(Node* node, uint16_t s_port, int sockfd,
  struct sockaddr_in serveraddr) {
  /* init local vars */
  char p_buf[MAX_PACKET_SIZE] = {0};
  char buf[MAX_DATA_SIZE] = {0};
  socklen_t serverlen;
  int n_sent;
  int n_recv;
  uint16_t d_port = node->port;

  /* create SYN packet to send to node */
  Pkt_t* syn_packet = create_packet(d_port, s_port, 0, node->content_path,
   PKT_FLAG_SYN);

  /* creating writeable version of packet */
  char* pack_write = writeable_packet(syn_packet);

  printf("Sending packet!\n");
  /* sending SYN packet */
  serverlen = sizeof(serveraddr);
  n_sent = sendto(sockfd, pack_write, strlen(pack_write), 0,
    (struct sockaddr*) &serveraddr, sizeof(serveraddr));

  printf("Packet sent!\n");
  if(n_sent < 0) {
    /* TODO buffer info: "send SYN fail" */
  }

  /* receive SYN-ACK packet */

  n_recv = recvfrom(sockfd, p_buf, MAX_PACKET_SIZE, 0,
    (struct sockaddr *) &serveraddr, &serverlen);

  if(n_recv < 0) {
    /* TODO buffer info: "recv ACK fail" */
  }

  /* parse buffer into SYN-ACK packet form */
  Pkt_t* syn_ack_packet = parse_packet(p_buf);
  if(!syn_ack_packet) {
    /* TODO buffer info: "parse packet fail" */
  }
  if(get_packet_type(syn_ack_packet) != PKT_FLAG_SYN_ACK) {
    /* TODO buffer info: "corrupted packet OR wrong packet type" */
  }

  /* storing buf for later return to caller & discarding local packets */
  /* CHECK - memcpy */
  size_t b_len = strlen(syn_ack_packet->buf);
  memcpy(buf, syn_ack_packet->buf, b_len);

  /* CHECK - not sure i should discard these packets here */
  //discard_packet(syn_packet);
  //discard_packet(syn_ack_packet);

  char* ref = buf;
  return ref;
}

char* request_content(Node* node, uint16_t s_port, int sockfd,
  struct sockaddr_in serveraddr, uint32_t seq_ack_num) {
  /* init local vars */
  char p_buf[MAX_PACKET_SIZE] = {0};
  char buf[MAX_DATA_SIZE] = {0};
  socklen_t serverlen;
  int n_sent;
  int n_recv;
  uint16_t d_port = node->port;

  /* creating ACK packet */
  Pkt_t* ack_packet = create_packet(d_port, s_port, seq_ack_num,
    node->content_path, PKT_FLAG_ACK);

  /* creating writeable version of packet */
  char* pack_write = writeable_packet(ack_packet);

  /* sending ACK packet */
  serverlen = sizeof(serveraddr);
  n_sent = sendto(sockfd, pack_write, strlen(pack_write), 0,
    (struct sockaddr *) &serveraddr, serverlen);

  if(n_sent < 0) {
    /* TODO deal with fail on sending ACK */
  }

  /* CHECK - removing cast of serveraddr to (struct sockaddr *) */
  n_recv = recvfrom(sockfd, p_buf, MAX_PACKET_SIZE, 0,
    (struct sockaddr *) &serveraddr, &serverlen);

  if(n_recv < 0) {
    /* TODO deal with fail on receiving data */
  }

  /* parse buffer into data packet form */
  Pkt_t* dat_packet = parse_packet(p_buf);
  if(!dat_packet) {
    /* TODO deal with parse packet fail */
  }
  if(get_packet_type(dat_packet) != PKT_FLAG_DATA) {
    /* TODO deal with corrupted packet */
  }

  /* storing buf for later return to caller & discarding local packets */
  /* CHECK - memcpy */
  size_t b_len = strlen(dat_packet->buf);
  memcpy(buf, dat_packet->buf, b_len);

  /* CHECK - not sure i should discard these packets here */
  //discard_packet(ack_packet);
  //discard_packet(dat_packet);

  char* ref = buf;
  return ref;
}

/*  TODO - replace writing headers with returning flag values
 *
 */

int peer_add_response(int connfd, char* BUF, struct thread_data *ct, Node_Dir* node_dir) {
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

  unsigned int peer_ip = (unsigned int) parse_str_2_int(node->ip_hostname);
  short peer_port = (short) node->port;

  /* TODO - format return value in sync_node */
  /* initializing connection with node that should have requested content */
  struct sockaddr_in peeraddr = get_sockaddr_in(peer_ip, peer_port);
  char* b = sync_node(node, ct->port_be, ct->listenfd_be, peeraddr);

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
  char* b2 = request_content(node, ct->port_be, ct->listenfd_be, ct->c_addr, len);

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
