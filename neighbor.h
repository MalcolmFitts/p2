
#ifndef NEIGHBOR_H
#define NEIGHBOR_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#include "configlib.h"
#include "parser.h"

#define MAX_NBR_MAP_LEN 8192

struct Neighbor {
  char* uuid;
  char* nbr_metrics;
  int seq_num;
  int num_missed;
  int active;
};


typedef struct Neighbor_Directory {
  int cur_nbrs;
  int max_nbrs;
  Neighbor* nbr_list;
} N_Dir;



N_Dir* create_neighbor_dir(int max);


int add_neighbor(N_Dir* n_dir, char* uuid, char* new_neighbors, int s_num);


int update_neighbor(N_Dir* n_dir, char* uuid, 
                      char* new_neighbors, int s_num, int received);


char* get_map(N_Dir n_dir, char* conf_file);







#endif