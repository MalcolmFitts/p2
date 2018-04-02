
#include "frontend.h"

int init_frontend(short port_fe, struct sockaddr_in* self_addr){
  int optval_fe = 1;
  int sockfd_fe;

  /* socket: create front and back end sockets */
  sockfd_fe = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd_fe < 0)
    error("ERROR opening front-end socket");

  /* setsockopt: Handy debugging trick that lets
   * us rerun the server immediately after we kill it;
   * otherwise we have to wait about 20 secs.
   * Eliminates "ERROR on binding: Address already in use" error.
   */
  setsockopt(sockfd_fe, SOL_SOCKET, SO_REUSEADDR,
	     (const void *)&optval_fe, sizeof(int));

  /* build the server's front end internet address */

  /* CHECK - was not zeroing memory */
  bzero((char *) self_addr, sizeof(*self_addr));
  self_addr->sin_family = AF_INET; /* we are using the Internet */
  self_addr->sin_addr.s_addr = htonl(INADDR_ANY); /* accept reqs to any IP addr */
  self_addr->sin_port = htons((unsigned short) port_fe); /* port to listen on */

  /* bind: associate the listening socket with a port */
  if (bind(sockfd_fe, (struct sockaddr *) self_addr, sizeof(*self_addr)) < 0)
    error("ERROR on binding front-end socket with port");

  /* listen: make it a listening socket ready to accept connection requests
   *         - only need to listen for front-end
   */
  if (listen(sockfd_fe, 10) < 0) /* allow 10 requests to queue up */
    error("ERROR on listen");

  return sockfd_fe;
}

int frontend_response(int connfd, char* BUF, struct thread_data *ct) {
  char content_filepath[MAX_FILEPATH_LEN] = "./content";
  char buf[BUFSIZE];              /* message buffer */
  char path[MAXLINE];
  char lb[MAX_PRINT_LEN];         /* buffer for logging */

  int content_size;               /* CHECK - should this be bigger? */
  time_t last_modified;

  char sbyte[MAX_RANGE_NUM_LEN];  /* holders for range request bytes */
  char ebyte[MAX_RANGE_NUM_LEN];  /* holders for range request bytes */
  int sb, eb;
  struct stat *fStat;

  char bufcopy[BUFSIZE];

  int tid = ct->tid;

  bzero(buf, BUFSIZE);
  strcpy(buf, BUF);

  /* parsing get request */
  bzero(path, MAXLINE);
  if(parse_get_request(buf, path) == 0) {
    /* CHECK to see if this works with threading */
    //CHECK numthreads--;
    error("ERROR Parsing get request");
    return 0;
  }

  /* getting image file and checking for existence */
  char* fname = strcat(content_filepath, path);
  FILE *fp = fopen(fname, "r");

  if(fp == NULL) {
    /* this is a 404 status code */
    sprintf(lb, "File (%s) not found", fname);
    log_thr(lb, ct->num, tid);

    write_headers_404(connfd, SERVER_NAME);
    //CHECK
    return 1;
  }
  else{
    sprintf(lb, "Found file: %s", fname);
    log_thr(lb, ct->num, tid);

    /* getting file size and last modified time */
    fStat = malloc(sizeof(struct stat));
    stat(fname, fStat);
    content_size = fStat->st_size;
    last_modified = fStat->st_mtime;
    free(fStat);

    /* getting file type (extension) */
    char fileExt[MAXLINE] = {0};
    if(parse_file_type(path, fileExt) == 0) {
      //CHECK
      error("File path error");
      return 0;
    }

    /* making copy of buffer to check for range requests */
    bzero(bufcopy, BUFSIZE);
    strcpy(bufcopy, buf);

    /*
     *    range_flag value (parse_range_request return val):
     *    2 --> valid range request, the end byte was sent with range request
     *	  1 --> range request only has start byte,the end byte was NOT sent
     *          with request so default range to EOF
     *    0 --> not a valid range request
     */
    int range_flag = parse_range_request(bufcopy, sbyte, ebyte);

    if(range_flag > 0) {
      /* this is a 206 status code */
      sprintf(lb, "Sending partial content to {Client %d}...", tid);
      log_thr(lb, ct->num, tid);

      /* assigning values for start and end bytes based on range flag */
      sb = atoi(sbyte);
      if(range_flag == 2) { eb = atoi(ebyte); }
      else                { eb = content_size - 1; }

      write_partial_content(connfd, fp, fileExt, sb, eb,
			    content_size, last_modified);

      int sent = eb - sb + 1;

      sprintf(lb, "Sent %d bytes to {Client %d}!", sent, tid);
      log_thr(lb, ct->num, tid);
    }
    else {
      /* this is a 200 status code */
      sprintf(lb, "Sending content to {Client %d}...", tid);
      log_thr(lb, ct->num, tid);

      write_full_content(connfd, fp, fileExt, content_size, last_modified);

      sprintf(lb, "Sent %d bytes to {Client %d}!", content_size, tid);
      log_thr(lb, ct->num, tid);
    }

    /* closing file*/
    fclose(fp);
  }
  return 1;
}

void handle_be_response(char* COM_BUF, int connfd, char* content_type){
  char* BUF = malloc(sizeof(char) * BUFSIZE);
  char* info = NULL;
  char* data = NULL;
  int type;
  int n_scan = 0;
  int content_len;
  //char* content = NULL;

  /* CHECK - seeing if this is running */
  printf("{*debug*} Front-end attempting to parse info from back-end.\n");

  while(1) {
    /* Locking BE-FE communication buffer to safely copy data */
    pthread_mutex_lock(&mutex);
    memcpy(BUF, COM_BUF, BUFSIZE);
    memset(COM_BUF, '\0', BUFSIZE);
    pthread_mutex_unlock(&mutex);

    if (BUF[0] != '\0') {
      /* BUF has info for FE; parse type of response and data */
      printf("{*debug*} Front-end received info from back-end!\n");
      // printf("{*debug*} BUF:\n%s\n", BUF);

      if(atoi(&BUF[0]) == COM_BUF_HDR) {
        n_scan = sscanf(BUF, "%d %d\n", &type, (int*)data);
      }
      else{
        n_scan = sscanf(BUF, "%d %s\n", &type, data);
      }

      printf("{*debug*} n_scan: %d\n",n_scan);
      printf("{*debug*} type: %d\n",type);
      printf("{*debug*} data: %s\n",data);

      if (n_scan != 2) {
        printf("FE DOESNT UNDERSTAND BE\n");
      }

      switch(type){
        case COM_BUF_HDR:
          printf("{*debug*} Front-end sending headers from back-end response.\n");
          n_scan = sscanf(data, "%d", &content_len);
          if (n_scan != 1) {
            /* SERVER_ERROR */
            write_status_header(connfd, SC_SERVER_ERROR, ST_SERVER_ERROR);
            write_empty_header(connfd);
            return;
          }
          write_status_header(connfd, SC_OK, ST_OK);
          write_content_length_header(connfd, content_len);
          write_content_type_header(connfd, content_type);
          write_empty_header(connfd);
          break;

        case COM_BUF_DATA:
          printf("{*debug*} Front-end sending data from back-end response.\n");
          /* CHECK - this was scanning info when it was still null */
          n_scan = sscanf(data, "%s", info);
          write(connfd, info, strlen(info));
          break;

        case COM_BUF_FIN:
          write_empty_header(connfd);
          break;

        default:
          printf("FE DOESN'T UNDERSTAND BE\n");
          break;
       }
     }
   }
   return;
}
