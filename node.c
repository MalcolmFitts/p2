
#include "node.h"

Node* create_node(char* path, char* name, int port, int rate) {
  /* allocate mem for struct */
  Node* pn;
  pn = malloc(sizeof(Node));

  /* checking for successful mem allocation */
  if(!pn) return NULL;

  /* cast port to correct type - uint16_t  */
  pn->port = (uint16_t) port;

  /* assign other fields directly */
  pn->content_path = path;
  pn->ip_hostname = name;
  pn->content_rate = rate;

  /* creating node address */
  struct hostent* node_host;
  node_host = gethostbyname(name);
  if(!node_host) {
    printf("Error: No Host Found For %s\n", name);
    return NULL;
  }

  struct sockaddr_in addr;
  bzero((char *) &addr, sizeof(addr));

  addr.sin_family = AF_INET;
  bcopy((char *)node_host->h_addr, (char *)&addr.sin_addr.s_addr,
    node_host->h_length);
  addr.sin_port = htons((unsigned short) port);

  pn->node_addr = addr;

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
    (nd->n_array[n]).node_addr = node->node_addr;
    (nd->n_array[n]).state = 0;

    nd->cur_nodes = n + 1;
    return 1;
  }

  /* max nbr of nodes in directory already reached */
  return 0;
}


Node* check_content(Node_Dir* dir, char* filename) {
  int max = dir->cur_nodes;
  int found = 0;
  int index = -1;
  Node ref;

  /* looping through directory and checking nodes */
  int i;
  for(i = 0; i < max; i++) {
    Node t = dir->n_array[i];

    /* checking node content -- CHECK TODO - check for content rate too?  */
    if(check_node_content(&t, filename) == 1 && !found) {
      found = 1;
      index = i;
    }
  }

  /* did not find content */
  if(!found || index == -1) return NULL;

  ref = dir->n_array[index];

  Node* res = create_node(ref.content_path, ref.ip_hostname, ref.port, ref.content_rate);
  return res;
}
