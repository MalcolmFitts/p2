

#include "neighbor.h"


N_Dir* create_neighbor_dir(int max) {
  N_Dir* n_dir = malloc(sizeof(N_Dir));

  n_dir->cur_nbrs = 0;
  n_dir->max_nbrs = max;

  n_dir->nbr_list = malloc(max * sizeof(Neighbor *));
  int i;
  for(i = 0; i < max; i++) {
    n_dir->nbr_list[i] = *((Neighbor *) malloc(sizeof(Neighbor)));
  }

  return n_dir;
}



int add_neighbor(N_Dir* n_dir, char* uuid, 
                    char* new_neighbors, int s_num) {
  if(n_dir->cur_nbrs >= n_dir->max_nbrs) {
    return 0;
  }

  int i = n_dir->cur_nbrs;

  (n_dir->nbr_list[i]).uuid = uuid;
  (n_dir->nbr_list[i]).nbr_metrics = new_neighbors;
  (n_dir->nbr_list[i]).seq_num = s_num;
  (n_dir->nbr_list[i]).num_missed = 0;
  (n_dir->nbr_list[i]).active = 1;
  (n_dir->nbr_list[i]).hostname = NULL;

  /* init port to default backend port */
  (n_dir->nbr_list[i]).port_be = CF_DEFAULT_PORT_BE;

  n_dir->cur_nbrs = i + 1;

  return 1;
}



int update_neighbor(N_Dir* n_dir, char* uuid, 
                      char* new_neighbors, int s_num, int received) {
  Neighbor *n;
  int i;
  int num = n_dir->cur_nbrs;

  for(i = 0; i < num; i++) {
    n = (Neighbor *)(&(n_dir->nbr_list[i]));

    if(strcmp(uuid, n->uuid) == 0 ) {

      if(!received) {

        int n_miss = n->num_missed;
        n->num_missed = n_miss + 1;

        if(n->num_missed == 3) {
          n->active = 0;
        }

      }
      else {

        if(n->seq_num < s_num) {
          n->nbr_metrics = new_neighbors;
          n->seq_num = s_num;
        }

        n->num_missed = 0;
        n->active = 1;
      }
      
      
    }
  }
}



char* get_map(N_Dir n_dir, char* conf_file) {

  char* metric_ptr;
  char* uuid_ptr;
  char* buf = malloc(MAX_NBR_MAP_LEN);
  char* res = malloc(MAX_NBR_MAP_LEN);
  int num = n_dir->cur_nbrs;

  sprintf(res, "{ ");

  int n_own_nbs;

  char* neighb_cnt = malloc(100);
  char* own_neighb = malloc(CF_MAX_LINE_LEN);
  char* n_uuid = malloc(CF_MAX_LINE_LEN);
  char* n_metric = malloc(CF_MAX_LINE_LEN);
  char* n_a = malloc(CF_MAX_LINE_LEN);
  char* n_b = malloc(CF_MAX_LINE_LEN);
  char* n_c = malloc(CF_MAX_LINE_LEN);

  char* my_uuid = NULL;

  neighb_cnt = get_config_field(conf_file, CF_TAG_PEER_COUNT, 0);
  if(neighb_cnt) {
    n_own_nbs = atoi(neighb_cnt);
  }

  if(n_own_nbs > 0) {
    my_uuid = get_config_field(conf_file, CF_TAG_UUID, 0);
    sprintf(buf, "\"%s\":{", my_uuid);
    strcat(res, buf);
  }
  

  int j;
  for(j = 0; j < n_own_nbs; j++) {
    own_neighb = get_config_field(conf_file, CF_TAG_PEER_INFO, j);
    bzero(buf, CF_MAX_LINE_LEN);
    if(own_neighb) {
      parse_neighbor_info(own_neighb, n_uuid, n_a, n_b, n_c, n_metric);
      sprintf(buf, "\"%s\":%s,", n_uuid, n_metric);
      strcat(res, buf);
    }
  }

  if(n_own_nbs > 0) {
    strcat(res, "}, ");
  }


  int i;
  for(i = 0; i < num; i++) {

    if((n_dir->nbr_list[i]).active) {

      uuid_ptr = (n_dir->nbr_list[i]).uuid;
      metric_ptr = (n_dir->nbr_list[i]).nbr_metrics;

      sprintf(buf, "\"%s\":%s", uuid_ptr, metric_ptr);
      strcat(res, buf);

      if(i < num - 1) {
        strcat(res, ",");
      }

    }
    
  }

  strcat(res, " }");

  return res;
}

