
#include "gossip.h"





int handle_exchange_msg(Pkt_t pkt, int sockfd,
                          struct sockaddr_in sender_addr, S_Dir* s_dir) {

  /* Local variables */
  Pkt_t response_pkt;
  P_Hdr p_hdr;
  S_Inf* recv_search;
  S_Inf* info_ptr;
  s_tc* s_thd_info;
  pthread_t search_tid;

  char* merged_peers;
  char* self_uuid;
  char* formatted_uuid;
  char filename[MAX_FILEPATH_LEN];

  int dir_check_flag;
  int n_sent;
  socklen_t sender_addr_len;

  
  /* Ensuring this is exchange packet */
  if(get_packet_type(pkt) != PKT_FLAG_EXC) {
    return -1;
  }

  /* Initializing some locals */
  sender_addr_len = sizeof(sender_addr);
  p_hdr = pkt.header;

  /* Parse packet for search info */
  recv_search = parse_search_info(pkt);

  printf("Server parsed search request info.\n");

  /* Check search directory for status of received search */
  dir_check_flag = check_search_dir(s_dir, recv_search);

  printf("Server checking status of search request...\n");

  /* Casing on search results: */

  /* 2 --> Server is already running search for content */
  if(dir_check_flag == 2) {
    printf("Server recognized search as: Currently Running\n");
    printf("Server updating peers for search...\n");

    /* Merge the received peer list and the list in the search dir*/
    merged_peers = sync_peer_info(s_dir, recv_search);

    /* Create and send response packet to sender of exchange message */
    response_pkt = create_exchange_packet(p_hdr.source_port, 
                        p_hdr.dest_port, p_hdr.seq_num, 
                        recv_search->content, p_hdr.com_buf, merged_peers);

    n_sent = sendto(sockfd, &response_pkt, sizeof(response_pkt), 0,
     (struct sockaddr *) &sender_addr, sender_addr_len);
  }

  /* 1 --> Start running new search for content since old search expired */
  else if(dir_check_flag == 1) {
    /* Merge the received peer list and the list in the search dir*/
    merged_peers = sync_peer_info(s_dir, recv_search);

    /* RESET MATCHING SEARCH INFO STRUCT IN DIRECTORY */
    reset_search_dir_info(s_dir, recv_search, merged_peers);

    /* SEND RESPONSE MESSAGE WITH UPDATED PEER INFO */
    response_pkt = create_exchange_packet(p_hdr.source_port, 
                        p_hdr.dest_port, p_hdr.seq_num, 
                        recv_search->content, p_hdr.com_buf, merged_peers);

    n_sent = sendto(sockfd, &response_pkt, sizeof(response_pkt), 0,
     (struct sockaddr *) &sender_addr, sender_addr_len);

    /* START NEW SEARCH PROTOCOL FOR REQUESTED CONTENT */
    s_thd_info = malloc(sizeof(struct search_tc));

    s_thd_info->sock = sockfd;
    s_thd_info->ttl = p_hdr.seq_num;
    sprintf(s_thd_info->path, "%s", recv_search->content);
    sprintf(s_thd_info->config_name, "%s", config_filename_global);
    sprintf(s_thd_info->search_list, "%s", merged_peers);

    pthread_create(&(search_tid), NULL, start_search, s_thd_info);
  }

  /* 0 --> Document this search in Search Directory and start search protocol */
  else {
    /* FILL NEW (next empty) SEARCH INFO STRUCT IN DIRECTORY */
    int index = s_dir->cur_search;

    info_ptr = (S_Inf*) (&(s_dir->search_arr[index]));

    if(!info_ptr) {
      return 0;
    }

    bzero(info_ptr, sizeof(S_Inf));

    info_ptr->max_recv_ttl = recv_search->max_recv_ttl;
    info_ptr->active_timer = recv_search->active_timer;
    info_ptr->content = recv_search->content;

    sprintf(filename, "content/%s", recv_search->content);

    /* If content was found locally */
    if(check_file(filename)) {
      /* Get own UUID to add to exchange's peer list and format for use */
      self_uuid = get_config_field(config_filename_global, CF_TAG_UUID, 0);

      formatted_uuid = malloc(sizeof(char) * (CF_UUID_STR_LEN * 2));
      bzero(formatted_uuid, CF_UUID_STR_LEN * 2);
      sprintf(formatted_uuid, "{%s}", self_uuid);

      /* Store new merged list of peers and self */
      merged_peers = merge_peer_lists(recv_search->peers, formatted_uuid);
    }

    /* If content was not found locally */
    else {
      merged_peers = recv_search->peers;
    }

    info_ptr->peers = merged_peers;

    /* SEND RESPONSE MESSAGE WITH UPDATED PEER INFO */
    response_pkt = create_exchange_packet(p_hdr.source_port, 
                        p_hdr.dest_port, p_hdr.seq_num, 
                        recv_search->content, p_hdr.com_buf, merged_peers);

    n_sent = sendto(sockfd, &response_pkt, sizeof(response_pkt), 0,
     (struct sockaddr *) &sender_addr, sender_addr_len);

    /* START NEW SEARCH PROTOCOL FOR REQUESTED CONTENT */
    s_thd_info = malloc(sizeof(struct search_tc));

    s_thd_info->sock = sockfd;
    s_thd_info->ttl = p_hdr.seq_num;
    sprintf(s_thd_info->path, "%s", recv_search->content);
    sprintf(s_thd_info->config_name, "%s", config_filename_global);
    sprintf(s_thd_info->search_list, "%s", merged_peers);

    pthread_create(&(search_tid), NULL, start_search, s_thd_info);
  }

  return 1;
}




S_Inf* parse_search_info(Pkt_t pkt) {
  S_Inf* info;

  char* filename = malloc(MAX_FILEPATH_LEN);
  char* peers = malloc(BUFSIZE);
  int ttl;

  int n_scan;

  if (get_packet_type(pkt) != PKT_FLAG_EXC) {
    /* Not an exchange packet */
    return NULL;
  }

  /* Scan packet for relevant data */
  n_scan = sscanf(pkt.buf, "Content: %s\nPeers: %[^\n]\n", 
    filename, peers);

  if (n_scan != 2) {
    /* Buffer did not contain info OR was read wrong */
    return NULL;
  }

  /* Allocate memory for struct and fill with data */
  info = malloc(sizeof(struct Search_Info));

  info->max_recv_ttl = pkt.header.seq_num;
  info->active_timer = pkt.header.seq_num;
  info->content = filename;
  info->peers = peers;

  return info;
}





int check_search_dir(S_Dir* dir, S_Inf* info) {

  int max = dir->cur_search;

  /* Iterate through directory */
  for(int i = 0; i < max; i++) {
    S_Inf cur_info = dir->search_arr[i];

    /* Checking for searches for same content */
    if(strcmp(info->content, cur_info.content) == 0) {

      if(cur_info.active_timer > 0) {
        /* Found an active search */
        return 2;
      }

      else {
        /* Found an inactive search */
        return 1;
      }
    }
  }

  /* Did not find any searches for the same content */
  return 0;
}



char* sync_peer_info(S_Dir* dir, S_Inf* info) {
  int max = dir->cur_search;
  char* new_peer_list;

  /* Iterate through directory */
  for(int i = 0; i < max; i++) {
    S_Inf cur_info = dir->search_arr[i];

    /* Checking for searches for same content */
    if(strcmp(info->content, cur_info.content) == 0) {
      new_peer_list = merge_peer_lists(info->peers, cur_info.peers);

      /* Update Search's Peer List */
      sprintf((dir->search_arr[i]).peers, "%s", new_peer_list);

      return new_peer_list;
    }
  }

  return NULL;
}

void add_uuid_to_list(char* list, char* uuid) {
  char read_uuid[CF_UUID_STR_LEN + 2];
  char list_uuid[CF_UUID_STR_LEN];
  char* res_ptr;
  char* ptr;
  char* end_ptr;

  int f_flag;

  if(strcmp(list, "") == 0){
    sprintf(list, "{%s}", uuid);
    return;
  }

  /* Initializing flag to 1 --> assumes uuid not in list */
  f_flag = 1;

  ptr = strstr(list, "{");
  /* Iterating through all UUIDs in list */
  while(ptr) {
    /* Store pointer to end of found UUID for easier iteration */
    end_ptr = strstr(ptr, "}");

    memcpy(read_uuid, ptr, CF_UUID_STR_LEN + 2);
    read_uuid[38] = '\0';
    strncpy(list_uuid, read_uuid + 1, CF_UUID_STR_LEN);
    list_uuid[CF_UUID_STR_LEN] = '\0';

    /* Comparing found UUID with parameter */
    if(strcmp(uuid, list_uuid) == 0){
      /* Parameter UUID already exists in list;
       * --> set flag and stop iterating */
      f_flag = 0;
      break;
    }

    ptr = strstr(end_ptr, "{");
  }

  char* formatted_uuid = malloc(sizeof(char) * (CF_UUID_STR_LEN * 2));
  bzero(formatted_uuid, CF_UUID_STR_LEN * 2);

  /* If flag = 1 then add UUID to list and return */
  if(f_flag) {

      /* List is not emptny so add a comma for formatting */
      sprintf(formatted_uuid, ", {%s}", uuid);
      strcat(list, formatted_uuid);
  }
}

char* merge_peer_lists(char* p_list1, char* p_list2) {
  char* res_list = malloc(sizeof(char) * BUFSIZE);
  char* res_formatted = malloc(sizeof(char) * BUFSIZE);
  char* read_uuid = malloc(sizeof(char) * CF_UUID_STR_LEN + 2);
  char* uuid = malloc(sizeof(char) * CF_UUID_STR_LEN);
  char* ptr;
  char* end_ptr;

  bzero(res_list, BUFSIZE);
  bzero(res_formatted, BUFSIZE);
  bzero(read_uuid, CF_UUID_STR_LEN);

  if(!strcmp(p_list1, "[]")){
    return p_list2;
  }
  if(!strcmp(p_list2, "[]")){
    return p_list1;
  }

  /* Search first peer list for beginning of first peer uuid */
  ptr = strstr(p_list1, "{");

  while(ptr) {
    /* Store pointer to end of found UUID */
    end_ptr = strstr(ptr, "}");

    memcpy(read_uuid, ptr, CF_UUID_STR_LEN + 2);
    read_uuid[38] = '\0';
    strncpy(uuid, read_uuid + 1, CF_UUID_STR_LEN);
    uuid[CF_UUID_STR_LEN] = '\0';

    add_uuid_to_list(res_list, uuid);

    /* Search for next UUID */
    bzero(read_uuid, CF_UUID_STR_LEN);
    ptr = strstr(end_ptr, "{");
  }

  /* Repeat same process with second list */
  ptr = strstr(p_list2, "{");

  while(ptr) {
    end_ptr = strstr(ptr, "}");

    memcpy(read_uuid, ptr, CF_UUID_STR_LEN + 2);
    read_uuid[38] = '\0';
    strncpy(uuid, read_uuid + 1, CF_UUID_STR_LEN);
    uuid[CF_UUID_STR_LEN] = '\0';

    add_uuid_to_list(res_list, uuid);

    /* Search for next UUID */
    bzero(read_uuid, CF_UUID_STR_LEN);
    ptr = strstr(end_ptr, "{");
  }

  sprintf(res_formatted, "[%s]", res_list);

  return res_formatted;
}






int reset_search_dir_info(S_Dir* dir, S_Inf* info, char* new_peers) {
  /* NULL checks */
  if(!dir || !info || !new_peers) {
    return -1;
  }

  S_Inf* info_ptr;
  int max = dir->cur_search;

  /* Iterating through all searches in directory */
  for(int i = 0; i < max; i++) {
    S_Inf cur_info = dir->search_arr[i];

    /* Finding matching search based on content field */
    if(strcmp(cur_info.content, info->content) == 0) {
      /* Creating memory reference to directory's search struct */
      info_ptr = (S_Inf*) (&(dir->search_arr[i]));

      info_ptr->max_recv_ttl = info->max_recv_ttl;
      info_ptr->active_timer = info->active_timer;
      info_ptr->content = info->content;
      info_ptr->peers = new_peers;

      return 1;
    }
  }

  return 0;
}


void* start_search(void* ptr){
  s_tc* p = (s_tc*) ptr;
  int sockfd = p->sock;
  char* path = p->path;
  char* fname = p->config_name;
  char* search_list = p->search_list;
  int TTL = p->ttl;

  char* my_uuid;
  int my_port;
  char* content_path = malloc(sizeof(char) * MAXLINE);

  int search_interval;

  char json_content[BUFSIZE];
  // Neighbor Info
  int num_neighbors, n, n_port;
  char* n_info, n_host;
  struct sockaddr_in n_addr;
  struct hostent *n_server;
  socklen_t n_addr_len;

  char* BUF = malloc(sizeof(char) * BUFSIZE);
  char* gossip_buf = malloc(sizeof(char) * BUFSIZE);

  my_uuid = get_config_field(fname, CF_TAG_UUID, 0);
  my_port = atoi(get_config_field(fname, CF_TAG_BE_PORT, 0));

  search_interval = atoi(get_config_field(fname, CF_TAG_SEARCH_INT, 0));

  // Get the number of current neighbors
  num_neighbors = atoi(get_config_field(fname, CF_TAG_PEER_COUNT, 0));

  Pkt_t packet;
  n = 0;

  // Wait for TTL to go to 0
  while(TTL > 0){

    // Check the gossip buffer for updates
    bzero(BUF, BUFSIZE);

    pthread_mutex_lock(&mutex);
    strcpy(BUF, gossip_buf);
    memset(gossip_buf, '\0', BUFSIZE);
    pthread_mutex_unlock(&mutex);

    if (BUF[0] != '\0') {
      /* BUF has updated list */
      strcpy(search_list, merge_peer_lists(BUF, search_list));
    }

    if(n <= num_neighbors){
      n_info = get_config_field(fname, CF_TAG_PEER_INFO, n);
      n_port = atoi(parse_peer_info(n_info, BE_PORT));
      n_host = parse_peer_info(n_info, HOSTNAME);

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

      sendto(sockfd, &packet, sizeof(packet), 0,
            (struct sockaddr *) &n_addr, n_addr_len);
      n ++;
    }
    sleep(search_interval);
  }
}

