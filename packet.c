#include "packet.h"

Pkt_t create_packet (uint16_t dest_port, uint16_t s_port, unsigned int s_num,
  char* filename, int flag, char* COM_BUF) {

  Pkt_t *packet = malloc(sizeof(struct Packet));
  P_Hdr *hdr = malloc(sizeof(struct Packet_Header));

  hdr->source_port = s_port;
  hdr->dest_port = dest_port;
  hdr->seq_num = s_num;

  /* initializing length of packet's buffer to max size */
  hdr->length = MAX_PACKET_SIZE;

  hdr->com_buf = COM_BUF;

  int len;

  if(flag == PKT_FLAG_DATA) {
    /* set flag to DATA */
    hdr->flag = (1 << PKT_FLAG_DATA);

    /* checking file contents */
    FILE* fp = fopen(filename, "r");

    if(!fp){
      /* TODO fail on finding file */
      sprintf(packet->buf, "File: Not Found\nContent Size: -1\n");
      hdr->length = 0;
    }
    else{
      /* finds correct starting point in file based on seq */
      int file_start = s_num * MAX_DATA_SIZE;
      fseek(fp, file_start, SEEK_SET);

      /* CHECK - might want to save this value for FIN flag*/
      len = fread(packet->buf, 1, MAX_DATA_SIZE, fp);
      hdr->length = len;
      fclose(fp);
    }

  }

  /* ACK packet */
  else if(flag == PKT_FLAG_ACK) {
    /* set flag to ACK and fill buffer*/
    hdr->flag = (1 << PKT_FLAG_ACK);

    len = sprintf(packet->buf, "Ready to send: %s\n", filename);
    hdr->length = len;
  }

  /* SYN packet */
  else if(flag == PKT_FLAG_SYN) {
    /* set flag to SYN and fill buffer*/
    hdr->flag =  (1 << PKT_FLAG_SYN);
    /* TODO - Data offset here to pass options in the data buffer (hdr->data_offset) */
    
    len = sprintf(packet->buf, "Request: %s\n", filename);
    hdr->length = len;
  }

  /* SYN-ACK packet */
  else if(flag == PKT_FLAG_SYN_ACK) {
    /* set flag to SYN-ACK */
    hdr->flag =  (1 << PKT_FLAG_SYN_ACK);

    /* attempting to find file */
    FILE* fp = fopen(filename, "r");
    if(fp) {
      /* getting file info */
      struct stat *fStat;
      fStat = malloc(sizeof(struct stat));
      stat(filename, fStat);

      /* size of requested file */
      int f_size = fStat->st_size;

      /* total number of packets for file transfer */
      int n_packs = (f_size / MAX_DATA_SIZE) + 1;

      /* storing info in buffer */
      len = sprintf(packet->buf,
        "File: %s\nContent Size: %d\nRequired Packets: %d\n", filename, f_size, n_packs);
      hdr->length = len;

      /* freeing mem and closing file */
      free(fStat);
      fclose(fp);
    }

    else {
      /* TODO fail on finding file */
      sprintf(packet->buf, "File: Not Found\nContent Size: -1\n");
      hdr->length = 0;
    }
  }

  /* FIN packet */
  else if(flag == PKT_FLAG_FIN) {
    /* set flag to FIN and store FIN info in buffer*/
    hdr->flag = (1 << PKT_FLAG_FIN);
    hdr->length = sprintf(packet->buf, "FIN: %s\n", filename);
  }

  /* invalid file creation flag */
  else {
    hdr->flag = 0;
    hdr->length = 0;
  }

  /* calculate checksum value */
  uint16_t c = calc_checksum(*hdr);
  hdr->checksum = c;

  /* assign header */
  packet->header = (*hdr);

  return *packet;
}

void discard_packet(Pkt_t *packet) {
  P_Hdr* ref = &(packet->header);
  free(ref);
  free(packet);
}

Pkt_t* parse_packet(char* buf) {
  /* allocating mem for structs */
  Pkt_t *pkt = malloc(sizeof(Pkt_t));
  P_Hdr *hdr = malloc(sizeof(P_Hdr));

  /* creating vars to directly parse into */
  uint16_t sp;     /* buf {0, 1}           */
  uint16_t dp;     /* buf {2, 3}           */
  uint16_t len;    /* buf {4, 5}           */
  uint16_t csum;   /* buf {6, 7}           */
  uint32_t snum;   /* buf {8, 9, 10, 11}   */
  uint16_t flag;   /* buf {12, 13}         */
  uint32_t off;    /* buf {14, 15}         */
  char* COM_BUF;   /* buf {16, 17, 18, 19} */

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
  memcpy(&off, b_ptr, 2);
  hdr->data_offset = off;

  b_ptr = &(buf[16]);
  memcpy(&COM_BUF, b_ptr, 4);
  hdr->com_buf = COM_BUF;

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



int get_packet_type(Pkt_t packet) {
  P_Hdr hdr = packet.header;
  uint16_t flag = hdr.flag;

  /* validating via checksum */
  uint16_t c = calc_checksum(hdr);
  if(c != hdr.checksum) return 0;

  if ((flag & PKT_SYN_ACK_MASK) > 0) {
    return PKT_FLAG_SYN_ACK;    /* SYN-ACK packet */
  }
  else if ((flag & PKT_SYN_MASK) > 0) {
    return PKT_FLAG_SYN;        /* SYN packet     */
  }
  else if ((flag & PKT_ACK_MASK) > 0) {
    return PKT_FLAG_ACK;        /* ACK packet     */
  }
  else if ((flag & PKT_DATA_MASK) > 0) {
    return PKT_FLAG_DATA;       /* DATA packet    */
  }

  else if ((flag & PKT_FIN_MASK) > 0){
    return PKT_FLAG_FIN;        /* FIN packet     */
  }

  /* should not happen - not sure what kind of packet this is */
  return -1;
}


/* TODO: fix checksum for new packets */
uint16_t calc_checksum(P_Hdr hdr) {
  /* parsing source_port, dest_port and length directly */
  uint16_t s_p = hdr.source_port;
  uint16_t d_p = hdr.dest_port;
  uint16_t len = hdr.length;
  uint16_t flag = hdr.flag;

  /* splitting up both seq num and ack into two 16-bit nums */
  /* CHECK - might have to & with S_INT_MASK, but dont think so cause unsigned int */
  uint16_t seq_num1 = (uint16_t) ((hdr.seq_num) >> 16);
  uint16_t seq_num2 = (uint16_t) (hdr.seq_num);

  /* binary addition of relevant values */
  uint16_t x = ((((s_p + d_p) + len) + flag) + seq_num1) + seq_num2;

  /* one's complement of addition */
  uint16_t res = ~x;
  return res;
}
