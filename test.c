#include "datawriter.h"
#include "parser.h"
#include "serverlog.h"
#include "backend.h"
#include "frontend.h"

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

int main(){
  Pkt_t* test_pkt = create_packet( 2222, 4444, 10, "test.test", PKT_FLAG_FIN );
  char* test_pkt_s = writeable_packet(test_pkt);
  Pkt_t* test_pkt_2 = parse_packet(test_pkt_s);
  uint16_t dest_port = test_pkt_2->header.dest_port;
  uint16_t source_port = test_pkt_2->header.source_port;
  uint32_t snum = test_pkt_2->header.seq_num;
  uint32_t flag = test_pkt_2->header.flag;
  int flag_test = (flag & PKT_FIN_MASK) >> PKT_FLAG_FIN;
  int pkt_type = get_packet_type(test_pkt_2);

  printf("dest_port: %hu\n", dest_port);
  printf("src_port: %hu\n", source_port);
  printf("snum: %d\n", snum);
  printf("flag_check: %d\n", flag_test);
  printf("get_packet_type: %d\n", pkt_type);
  printf("checksum: %d\n", calc_checksum( &(test_pkt_2->header) ));

  return 0;
}
