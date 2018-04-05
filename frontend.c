
#include "frontend.h"

int init_frontend(short port_fe, struct sockaddr_in* self_addr){
  int optval_fe = 1;
  int sockfd_fe;

  /* socket: create front and back end sockets */
  sockfd_fe = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd_fe < 0)
    error("ERROR opening front-end socket");

  setsockopt(sockfd_fe, SOL_SOCKET, SO_REUSEADDR,
	     (const void *)&optval_fe, sizeof(int));

  /* build the server's front end internet address */
  bzero((char *) self_addr, sizeof(*self_addr));
  self_addr->sin_family = AF_INET;
  self_addr->sin_addr.s_addr = htonl(INADDR_ANY);   /* accept reqs to any IP */
  self_addr->sin_port = htons((unsigned short) port_fe);  /* port to listen */

  /* bind: associate the listening socket with a port */
  if (bind(sockfd_fe, (struct sockaddr *) self_addr, sizeof(*self_addr)) < 0)
    error("ERROR on binding front-end socket with port");

  /* listen: make it a listening socket ready to accept connection requests */
  if (listen(sockfd_fe, 10) < 0) /* allow 10 requests to queue up */
    error("ERROR on listen");

  return sockfd_fe;
}

void frontend_response(int connfd, char* BUF, struct thread_data *ct) {
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
    write_headers_500(connfd);
    return;
  }

  /* getting image file and checking for existence */
  char* fname = strcat(content_filepath, path);
  FILE *fp = fopen(fname, "r");

  if(fp == NULL) {
    write_headers_404(connfd, SERVER_NAME);
    return;
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
      write_headers_500(connfd);
      return;
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
  return;
}

void handle_be_response(char* COM_BUF, int connfd, char* content_type){
  char* BUF = malloc(sizeof(char) * COM_BUFSIZE);
  char* info = malloc(sizeof(char) * COM_BUFSIZE);
  char* data = malloc(sizeof(char) * COM_BUFSIZE);
  int type;
  int content_len;
  int done = 0;

  bzero(BUF, COM_BUFSIZE);
  bzero(info, COM_BUFSIZE);
  bzero(data, COM_BUFSIZE);

  while(!done) {
    /* Locking BE-FE communication buffer to safely copy data */
    pthread_mutex_lock(&mutex);
    memcpy(BUF, COM_BUF, COM_BUFSIZE);
    memset(COM_BUF, '\0', COM_BUFSIZE);
    pthread_mutex_unlock(&mutex);

    if (BUF[0] != '\0') {
      /* BUF has info for FE; parse type of response and data */
      type = atoi(&(BUF[0]));
      strncpy(data, BUF + 2, COM_BUFSIZE);

      switch(type){
        case COM_BUF_HDR:
          content_len = atoi(data);

          if (content_len <= 0) {
            write_headers_500(connfd); /* SERVER_ERROR */
          } else {
            write_status_header(connfd, SC_OK, ST_OK);
            write_date_header(connfd);
            write_server_name_header(connfd, SERVER_NAME);
            write_conn_header(connfd, CONN_KEEP_HDR);
            write_keep_alive_header(connfd, 0, 100);
            write_content_length_header(connfd, content_len);
            write_content_type_header(connfd, content_type);
            write_empty_header(connfd);
          }
          break;

        case COM_BUF_DATA:
          write(connfd, data, strlen(data));
          break;

        case COM_BUF_DATA_FIN:
          write(connfd, data, strlen(data));
          done = 1;
          break;

        case COM_BUF_FIN:
          write_empty_header(connfd);
          done = 1;
          break;

        default:
          printf("FE DOESN'T UNDERSTAND BE\n");
          break;
       }
     }
   }
   return;
}
