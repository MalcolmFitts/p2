/*  (Version: p2)
 * backend.c
 *    - created for use with "bbbserver.c"
 *	  - implementations of server back-end functions
 *	  - functions and data structures defined in "backend.h"
 */

#include "backend.h"

int init_backend(struct sockaddr_in serveraddr_be, int port_be){
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
  bzero((char *) &serveraddr_be, sizeof(serveraddr_be));
  serveraddr_be.sin_family = AF_INET; /* we are using the Internet */
  serveraddr_be.sin_addr.s_addr = htonl(INADDR_ANY); /* accept reqs to any IP addr */
  serveraddr_be.sin_port = htons((unsigned short)portno_be); /* port to listen on */

  /* bind: associate the listening socket with a port */
  if (bind(listenfd_be, (struct sockaddr *) &serveraddr_be, sizeof(serveraddr_be)) < 0)
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
  /* CHANGE --> backend.c */
  nd->n_array = malloc(max * sizeof(Node *));
  for(int i = 0; i < max; i++) {
    nd->n_array[i] = *((Node*) malloc(sizeof(Node)));
  }

  return nd;
}


int add_node(Node_Dir* nd, Node* node) {
  if((nd->cur_nodes) < (nd->max_nodes)) {
    /* directory not full - add the node */
    int n = (nd->cur_nodes);

    /* CHANGE --> backend.c */
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


Pkt_t* create_packet (Node* n, uint16_t s_port, unsigned int s_num, 
  char* filename, int flag) {

  Pkt_t *packet = malloc(sizeof(Pkt_t));
  P_Hdr *hdr = malloc(sizeof(P_Hdr));

  if(!packet || !hdr) return NULL;

  /* CHECK - fixing file buffer size to max size */
  char f_buf[MAX_DATA_SIZE] = {0};

  hdr->source_port = s_port;
  hdr->dest_port = n->port;
  hdr->seq_num = s_num;

  /* CHECK - fixing size of packets to max size for now */
  hdr->length = MAX_PACKET_SIZE;

  if(flag == PKT_FLAG_DATA) {  
    /* data packet */

    /* assign ACK & SYN fields */
    hdr->ack = (uint16_t) PKT_ACK_NAN;
    hdr->syn = (uint16_t) PKT_SYN_NAN;

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

    /* assign ACK & SYN fields */
    hdr->ack = (uint16_t) PKT_ACK;
    hdr->syn = (uint16_t) PKT_SYN_NAN;

    /* CHECK - not sure how to send ACK info - maybe in buffer */
  }
  else if(flag == PKT_FLAG_SYN) {
    /* SYN packet */

    /* assign ACK & SYN fields */
    hdr->ack = (uint16_t) PKT_ACK_NAN;
    hdr->syn = (uint16_t) PKT_SYN;

    /* CHECK - not sure how to send SYN info - maybe in buffer */
    sprintf(f_buf, "Request: %s\n", filename);
  }
  else if(flag == PKT_FLAG_SYN_ACK) {
    /* SYN-ACK packet */

    /* assign ACK & SYN fields */
    hdr->ack = (uint16_t) PKT_ACK;
    hdr->syn = (uint16_t) PKT_SYN;

    /* CHECK - not sure how to send SYN-ACK info - maybe in buffer */

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
    else{
      /* info on no file found in buffer */
      sprintf(f_buf, "File: Not Found\nContent Size: -1\n");
    }

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
  uint16_t syn = hdr->syn;
  uint16_t ack = hdr->ack;

  /* splitting up both seq num and ack into two 16-bit nums */
  /* CHECK - might have to & with S_INT_MASK, but dont think so cause unsigned int */
  uint16_t seq_num1 = (uint16_t) ((hdr->seq_num) >> 16);
  uint16_t seq_num2 = (uint16_t) (hdr->seq_num);

  /* binary addition of relevant values */
  uint16_t x = s_p + d_p + len + syn + ack + seq_num1 + seq_num2;

  /* one's complement of addition */
  uint16_t res = ~x;

  return res;
}


Pkt_t* parse_packet(char* buf) {
  /* ensuring min size of data */
  if(strlen(buf) < P_HDR_SIZE) return NULL;

  /* allocating mem for structs */
  Pkt_t *pkt = malloc(sizeof(Pkt_t));
  P_Hdr *hdr = malloc(sizeof(P_Hdr));

  char sp[2];     /* buf { 0,  1} */
  char dp[2];     /* buf { 2,  3} */
  char len[2];    /* buf { 4,  5} */
  char csum[2];   /* buf { 6,  7} */
  char ack[2];    /* buf { 8,  9} */
  char syn[2];    /* buf {10, 11} */
  char snum[4];   /* buf {12, 15} */

  sprintf(sp, "%c%c", buf[0], buf[1]);
  sprintf(dp, "%c%c", buf[2], buf[3]);
  sprintf(len, "%c%c", buf[4], buf[5]);
  sprintf(csum, "%c%c", buf[6], buf[7]);
  sprintf(ack, "%c%c", buf[8], buf[9]);
  sprintf(syn, "%c%c", buf[10], buf[11]);
  sprintf(snum, "%c%c%c%c", buf[12], buf[13], buf[14], buf[15]);

  /* CHECK - make sure this is parsing correctly */
  hdr->source_port  = (uint16_t) atoi(sp);
  hdr->dest_port    = (uint16_t) atoi(dp);
  hdr->length       = (uint16_t) atoi(len);
  hdr->checksum     = (uint16_t) atoi(csum);
  hdr->ack          = (uint16_t) atoi(ack);
  hdr->syn          = (uint16_t) atoi(syn);
  hdr->seq_num      = (uint32_t) atoi(snum);
  
  pkt->header = *hdr;

  /* CHECK - not sure im assigning this correctly */
  char *b_ptr = &(buf[16]);
  size_t b_len = sizeof(buf) - 16;
  memcpy(pkt->buf, b_ptr, b_len);

  return pkt;
}


char* writeable_packet(Pkt_t* packet) {
  /* CHECK - I really just threw this together */
  char p_buf[MAX_PACKET_SIZE];
  size_t p_size = sizeof(packet);
  memcpy(p_buf, packet, p_size);

  char* ref = p_buf;
  return ref;
}


int get_packet_type(Pkt_t *packet) {
  P_Hdr *hdr = &(packet->header);

  /* validating via checksum */
  uint16_t c = calc_checksum(hdr);
  if(c != hdr->checksum) return 0;

  uint16_t syn = (uint16_t) PKT_SYN;
  uint16_t ack = (uint16_t) PKT_ACK;
  uint16_t syn_nan = (uint16_t) PKT_SYN_NAN;
  uint16_t ack_nan = (uint16_t) PKT_ACK_NAN;

  if((hdr->syn) == syn && (hdr->ack) == ack) {
    return PKT_FLAG_SYN_ACK;    /* SYN-ACK packet */
  }
  else if((hdr->syn) == syn && (hdr->ack) == ack_nan)  {
    return PKT_FLAG_SYN;        /* SYN packet     */
  }
  else if((hdr->ack) == ack && (hdr->syn) == syn_nan) {
    return PKT_FLAG_ACK;        /* ACK packet     */
  }
  else if((hdr->ack) == ack_nan && (hdr->syn) == syn_nan) {
    return PKT_FLAG_DATA;       /* DATA packet    */
  }
  
  /* should not happen - not sure what kind of packet this is */
  return -1;
}


Node* check_content(Node_Dir* dir, char* filename) {
  int max = dir->cur_nodes;
  int found = 0;
  Node ref;

  /* looping through directory and checking nodes */
  for(int i = 0; i < max; i++) {
    /* CHANGE --> backend.c  */
    Node t = dir->n_array[i];

    /* CHECK TODO - check for content rate too? */
    /* checking node content */
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

  /* create SYN packet to send to node */
  Pkt_t* syn_packet = create_packet(node, s_port, 0, node->content_path,
   PKT_FLAG_SYN);

  /* creating writeable version of packet */
  char* pack_write = writeable_packet(syn_packet);

  /* sending SYN packet */
  serverlen = sizeof(serveraddr);
  n_sent = sendto(sockfd, pack_write, strlen(pack_write), 0, 
    (struct sockaddr *) &serveraddr, serverlen);

  if(n_sent < 0) {
    /* TODO buffer info: "send SYN fail" */
  }

  /* receive SYN-ACK packet */
  n_recv = recvfrom(sockfd, p_buf, MAX_PACKET_SIZE, 0, 
    (struct sockaddr *) &serveraddr, &serverlen);f
  
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

  /* creating ACK packet */
  Pkt_t* ack_packet = create_packet(node, s_port, seq_ack_num, 
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

/*  TODO
 *  add_response_be
 */ 
int peer_add_response(int connfd, char* BUF, struct thread_data *ct){
  char buf[MAXLINE];
  char* filepath = malloc(sizeof(char) * MAXLINE);
  char* hostname = malloc(sizeof(char) * MAXLINE);
  char* port_c = malloc(sizeof(char) * MAXLINE);
  char* rate_c = malloc(sizeof(char) * MAXLINE);

  int port;
  int rate;

  bzero(buf, BUFSIZE);
  strcpy(buf, BUF);

  /* TODO this returns something */
  parse_peer_add(BUF, filepath, hostname, port_c, rate_c);

  port = parse_str_2_int(port_c);
  rate = parse_str_2_int(rate_c);

  free(port_c);
  free(rate_c);

  Node* n = create_node(filepath, hostname, port, rate);

  if(!add_node(node_dir, n)){
    /* TODO error checking */
    write_status_header(connfd, SC_SERVER_ERROR, ST_SERVER_ERROR);
    return 0;
  }
  
  write_status_header(connfd, SC_OK, ST_OK);
  return 1;
}

/*  TODO
 *  view_response_be
 */
int peer_view_response(int connfd, char*BUF, struct thread_data *ct){
  char buf[MAXLINE];
  char filepath = malloc(sizeof(char) * MAXLINE);
  char path[MAXLINE];
  char* file_type = malloc(sizeof(char) * MINLINE);
  Node* node;
  int len;

  /* TODO this returns something */
  parse_peer_view_content(BUF, filepath);

  bzero(path, MAXLINE);
  strcpy(path, filepath);

  /* TODO this will return something */
  parse_file_type(path, file_type);
  
  bzero(buf, BUFSIZE);
  strcpy(buf, BUF);
  
  if ((node = check_content(node_dir, filepath)) == NULL){
    write_status_header(connfd, SC_NOT_FOUND, ST_NOT_FOUND);
    return 0;
  }

  bzero(buf, BUFSIZE);
  
  buf = sync_node(node, ct->port_be, ct->listenfd_be, ct->c_addr);

  /* TODO check if fails */

  len = parse_str_2_int(buf);

  write_status_header(connfd, SC_OK, ST_OK);
  write_content_length_header(connfd, len);
  write_content_type_header(connfd, file_type);
  
  bzero(buf, BUFSIZE);
  /* CHECK will not be full len after CP */
  buf = request_content(node, ct->port_be, ct->listenfd_be, ct->c_addr, len);

  write(connfd, buf, strlen(buf));

  return 1;
}

/*  TODO
 *  rate_response_be
 */
int peer_rate_response(int connfd, char* BUF, struct thread_data *ct){
  char buf[MAXLINE];
  int rate;
  char* rate_c = malloc(sizeof(char) * MAXLINE);
    
  bzero(buf, BUFSIZE);
  strcpy(buf, BUF);

  /* TODO this returns something */
  parse_peer_config_rate(BUF, rate);
  
  rate = parse_str_to_int(char* rate_c);
  free(rate_c);

  // CHECK 200 OK response on config_rate
  write_status_header(connfd, SC_OK, ST_OK);

  return 0;
}

/* filler end line */
