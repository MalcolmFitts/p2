
#include "gossip.h"


//  PSEUDO - handle exchange message

/*
  
  // 1. parse search info from packet
  recv_search = parse_exchange_pkt(pkt)

  // 2. check if content was seen (search struct exists)
  check_searches(recv_search)

  // 3. case - this is not a new search
  if(search struct exists for this content && 
      struct's active timer != 0) {
    
    // 3.1 - update search structs record of peers
    update_search_peers(recv_search peers, search struct)

    update_own_neighbors?

    // 3.2 - respond to message with synced list of peers
    respond_to_exchange(...)

  }

  // 4. case - this is new search
  else  {
    
    // 4.1 - if struct exists but search had ended, reset the struct
    if(struct's active timer = 0) {
    

      reset_search(search struct, recvd info);

    }


    // 4.2 - create new struct
    else {
    
      create_new_search(...)

    }

    // 4.3 - respond with synced peer list
    respond_to_exchange(...)

    // 4.4 - start new search for content
    start_search(...)


  }

*/



int handle_exchange_msg(Pkt_t pkt, int sockfd, 
                          struct sockaddr_in sender_addr, S_Dir* s_dir) {

  /* Local variables */
  S_Inf* recv_search;
  char* merged_peers;
  int dir_check_flag;


  /* Ensuring this is exchange packet */
  if(get_packet_type(pkt) != PKT_FLAG_EXC) {
    return -1;
  }

  /* Parse packet for search info */
  recv_search = parse_search_info(pkt);

  /* Check search directory for status of received search */
  dir_check_flag = check_search_dir(s_dir, recv_search);

  /* Casing on search results: */

  /* 2 --> Server is already running search for content */
  if(dir_check_flag == 2) {
    /* Merge the received peer list and the list in the search dir*/
    merged_peers = sync_peer_info(s_dir, recv_search);

    /* RESPONSE MESSAGE WITH UPDATED PEER INFO */
  }

  /* 1 --> Start running new search for content since old search expired */
  else if(dir_check_flag == 1) {
    /* Merge the received peer list and the list in the search dir*/
    merged_peers = sync_peer_info(s_dir, recv_search);

    /* RESPONSE MESSAGE WITH UPDATED PEER INFO */

    /* START NEW SEARCH PROTOCOL FOR REQUESTED CONTENT */
  }

  /* 0 --> Document this search in Search Directory and start search protocol */
  else {

  }
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
  n_scan = sscanf(packet.buf, "Content: %s\nPeers: [%s]\n", 
    filename, peers);

  if (n_scan != 2) {
    /* Buffer did not contain info OR was read wrong */
    return NULL;
  }

  /* Allocate memory for struct and fill with data */
  info = malloc(sizeof(struct Search_Info));

  info->max_recv_ttl = packet.header.seq_num;
  info->active_timer = packet.header.seq_num;
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
      cur_info.peers = new_peer_list;

      return new_peer_list;
    }
  }

  return NULL;
}




char* merge_peer_lists(char* p_list1, char* p_list2) {
  char* res_list = malloc(sizeof(char) * BUFSIZE);
  char* res_formatted = malloc(sizeof(char) * BUFSIZE);
  char* read_uuid = malloc(sizeof(char) * CF_UUID_STR_LEN);
  char* ptr;
  char* end_ptr;
  int scan_flag;

  bzero(res_list, BUFSIZE);
  bzero(res_formatted, BUFSIZE);
  bzero(read_uuid, CF_UUID_STR_LEN);

  /* Search first peer list for beginning of first peer uuid */
  ptr = strstr(p_list1, '{');

  while(ptr) {
    /* Store pointer to end of found UUID */
    end_ptr = strstr(ptr, '}');

    /* Scanning UUID into read_uuid var */
    if(sscanf(ptr, "{%s}", read_uuid)) {
      add_uuid_to_list(res_list, read_uuid);
    }
    
    /* Search for next UUID */
    bzero(read_uuid, CF_UUID_STR_LEN);
    ptr = strstr(end_ptr, '{');
  }

  /* Repeat same process with second list */
  ptr = strstr(p_list2, '{');

  while(ptr) {
    end_ptr = strstr(ptr, '}');

    if(sscanf(ptr, "{%s}", read_uuid)) {
      add_uuid_to_list(res_list, read_uuid);
    }

    bzero(read_uuid, CF_UUID_STR_LEN);
    ptr = strstr(end_ptr, '{');
  }

  sprintf(res_formatted, "[%s]", res_list);

  return res_formatted;
}



char* add_uuid_to_list(char* list, char* uuid) {
  char* read_uuid = malloc(sizeof(char) * CF_UUID_STR_LEN);
  char* res_ptr;
  char* ptr;
  char* end_ptr;

  int f_flag;
  int s_flag;

  bzero(read_uuid, CF_UUID_STR_LEN);
  
  /* Initializing flag to 1 --> assumes uuid not in list */
  f_flag = 1;

  ptr = strstr(list, '{');

  /* Iterating through all UUIDs in list */
  while(ptr) {
    /* Store pointer to end of found UUID for easier iteration */
    end_ptr = strstr(ptr, '}');

    s_flag = sscanf(ptr, "{%s}", read_uuid);
    
    /* Comparing found UUID with parameter */
    if((s_flag == 1) && (strcmp(read_uuid, uuid) == 0)) {
      /* Parameter UUID already exists in list;
       * --> set flag and stop iterating */
      f_flag = 0;
      break;
    }

    /* Searching for beginning of next UUID */
    bzero(read_uuid, CF_UUID_STR_LEN);
    ptr = strstr(end_ptr, '{');
  }

  char* formatted_uuid = malloc(sizeof(char) * (CF_UUID_STR_LEN * 2));
  bzero(formatted_uuid, CF_UUID_STR_LEN * 2);

  /* If flag = 1 then add UUID to list and return */
  if(f_flag) {

    if(strlen(list) > 0) {
      /* List is not empty so add a comma for formatting */
      sprintf(formatted_uuid, ", {%s}", uuid);
      res_ptr = strcat(list, formatted_uuid);
    }

    else {
      /* List is empty so just return formatted uuid as the new list */
      sprintf(formatted_uuid, "{%s}", uuid);
      res_ptr = formatted_uuid;
    }
    
  }

  /* Else UUID was in list so just return original list */
  else {
    res_ptr = list;
  }

  return res_ptr;
}






















