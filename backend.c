/*  (Version: p2)
 * backend.c
 *    - created for use with "bbbserver.c"
 *	  - implementations of server back-end functions
 *	  - functions and data structures defined in "backend.h"
 */

#include "backend.h"

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
  nd->n_array = malloc(max * sizeof(Node*));

  /* checking for successful mem allocation */
  if(!(nd->n_array)) return NULL;

  return nd;
}


int add_node(Node_Dir* nd, Node* node) {
	if((nd->cur_nodes) < (nd->max_nodes)) {
		/* directory not full - add the node */
		int n = (nd->cur_nodes) + 1;
		nd->n_array[n] = node;

		nd->cur_nodes = n;
		return 1;
	}

	/* max nbr of nodes in directory already reached */
	return 0;
}


Pkt_t* create_packet (Node* n, uint16_t s_port, unsigned int s_num, 
  char* filename, int flag, unsigned int ack_mask) {

  Pkt_t *packet = malloc(sizeof(Pkt_t));
  P_Hdr *hdr = malloc(sizeof(P_Hdr));

  if(!packet || !hdr) return NULL;

  hdr->source_port = s_port;
  hdr->dest_port = n->port;
  hdr->seq_num = s_num;

  /* CHECK - fixing size of packets to max size for now */
  hdr->length = MAX_PACKET_SIZE;

  /* determining whether packet is data or acknowledgement */
  if(flag == PKT_FLAG_DATA) {
    /* assign ACK field */
    hdr->ack = (unsigned int) PKT_DAT;

    /* checking file contents */
    FILE* fp = fopen(filename, "r");
    if(!fp) return NULL;
    rewind(fp);

    /* CHECK - fixing file buffer size to max size */
    char f_buf[MAX_DATA_SIZE];

    /* storing file contents in f_buf */
    fread(f_buf, 1, MAX_DATA_SIZE, fp);  /* might want to save this value */
    strcpy(packet->buf, f_buf);
  }
  else if(flag == PKT_FLAG_ACK) {
    /* assign ACK field */
    hdr->ack = (unsigned int) (PKT_ACK & ack_mask);

    /* CHECK - fixing file buffer size to max size */
    char f_buf[MAX_DATA_SIZE] = {0};
    strcpy(packet->buf, f_buf);
  }
  else {
    return NULL;
  }

  /* TODO - calculate checksum value */
  uint16_t c = calc_checksum(hdr);
  hdr->checksum = c;

  packet->header = *hdr;

  return packet;
}



uint16_t calc_checksum(P_Hdr* hdr) {
  /* parsing source_port, dest_port and length directly */
  uint16_t s_p = hdr->source_port;
  uint16_t d_p = hdr->dest_port;
  uint16_t len = hdr->length;

  /* splitting up both seq num and ack into two 16-bit nums */
  uint16_t seq_num1 = (uint16_t) (((hdr->seq_num) >> 16) & SHORT_INT_MASK);
  uint16_t seq_num2 = (uint16_t) ((hdr->seq_num) & SHORT_INT_MASK);

  uint16_t ack1 = (uint16_t) ((hdr->ack >> 16) & SHORT_INT_MASK);
  uint16_t ack2 = (uint16_t) ((hdr->ack) & SHORT_INT_MASK);

  /* binary addition of relevant values */
  uint16_t x = s_p + d_p + len + seq_num1 + seq_num2 + ack1 + ack2;

  /* one's complement of addition */
  uint16_t res = ~x;

  return res;
}









/* filler end line */