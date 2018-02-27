/*  (Version: p2)
 * backend.c
 *    - created for use with "bbbserver.c"
 *	  - implementations of server back-end functions
 *	  - functions and data structures defined in "backend.h"
 */

#include "backend.h"


void recieve_pkt(Node* node, int sockfd,
		 struct sockaddr_in serveraddr){

  char p_buf[MAX_PACKET_SIZE] = {0};
  char* content_path = node->content_path;
  socklen_t serverlen;
  int n_sent;
  int n_recv[MAX_PACKET_SIZE] = {0};
  uint16_t s_port = node->port;
  

  while(1){
    n_recv = recvfrom(sockfd, p_buf, MAX_PACKET_SIZE, 0,
		      (struct sockaddr *) &serveraddr, &serverlen);
    if(n_recv < 0) {
      /* TODO  */
    }

    Pkt_t* recv_pkt = parse_packet(p_buf);

    int type = get_packet_type(recv_pkt);

    /* CHECK accepted types (SYN and ACK)*/
    /* TODO add FIN implimentation */
    if (type != PKT_FLAG_SYN || type != PKT_FLAG_ACK){
      /* TODO corrupted packet */
    }

    else{
      d_port = recv_pkt->header->source_port;
      s_num = recv_pkt->header->s_num;
      /* TODO s_num */
      n_sent = serve_content(d_port, s_port, 0, content_path, sockfd,
			     serveraddr, type);

      if(n_sent < 0){
	/* TODO error on send */
      }
    }
  }
}

int serve_content(uint16_t d_port, uint16_t s_port, unsigned int s_num,
		  char* filename, int sockfd,
		  struct sockaddr_in serveraddr, int flag){
  
  socklen_t server_len;
  int n_set;
  Pkt_t* data_pkt;

  if (flag == PKT_FLAG_ACK){
    /* Create the packet to be sent */
    /* CHECK s_num */ 
    data_pkt = create_packet(d_port, s_port, s_num, filename, PKT_FLAG_DATA);
  }

  else /* flag == PKT_FLAG_SYN */ {
    /* CHECK s_num */ 
    data_pkt = create_packet(d_port, s_port, s_num, filename, PKT_FLAG_SYN_ACK);
  }
  
  char* data_pkt_wr = writeable_packet(Pkt_t* data_pkt);
  
  server_len = sizeof(serveraddr);
  n_set = sendto(sockfd, data_pkt_wr, str_len(data_pkt_wr), 0,
		 (struct sockaddr *) &serveraddr, serverlen);
  
  return n_set;
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
  for(int i = 0; i < max; i++) {
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
  uint16_t x = (((((s_p + d_p) + len) + syn)+ ack) + seq_num1) + seq_num2;

  /* one's complement of addition */
  uint16_t res = ~x;
  return res;
}


Pkt_t* parse_packet(char* buf) {
  /* allocating mem for structs */
  Pkt_t *pkt = malloc(sizeof(Pkt_t));
  P_Hdr *hdr = malloc(sizeof(P_Hdr));

  /* creating vars to directly parse into */
  uint16_t sp;     /* buf { 0,  1} */
  uint16_t dp;     /* buf { 2,  3} */
  uint16_t len;    /* buf { 4,  5} */
  uint16_t csum;   /* buf { 6,  7} */
  uint16_t ack;    /* buf { 8,  9} */
  uint16_t syn;    /* buf {10, 11} */
  uint32_t snum;   /* buf {12, 15} */  

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
  memcpy(&ack, b_ptr, 2);
  hdr->ack = ack;

  b_ptr = &(buf[10]);
  memcpy(&syn, b_ptr, 2);
  hdr->syn = syn;

  b_ptr = &(buf[12]);
  memcpy(&snum, b_ptr, 4);
  hdr->seq_num = snum;
  
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

  /* sending SYN packet */
  serverlen = sizeof(serveraddr);
  n_sent = sendto(sockfd, pack_write, strlen(pack_write), 0,
    (struct sockaddr *) &serveraddr, serverlen);

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







/* filler end line */
