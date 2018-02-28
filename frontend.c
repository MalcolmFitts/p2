
#include "frontend.h"

int init_frontend(struct sockaddr_in serveraddr_fe, int port_fe){
  int optval_fe = 1;
  int listenfd_fe;
  
  /* socket: create front and back end sockets */
  listenfd_fe = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd_fe < 0)
    error("ERROR opening front-end socket");

  /* setsockopt: Handy debugging trick that lets
   * us rerun the server immediately after we kill it;
   * otherwise we have to wait about 20 secs.
   * Eliminates "ERROR on binding: Address already in use" error.
   */
  setsockopt(listenfd_fe, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval_fe, sizeof(int));

  /* build the server's front end internet address */
  bzero((char *) &serveraddr_fe, sizeof(serveraddr_fe));
  serveraddr_fe.sin_family = AF_INET; /* we are using the Internet */
  serveraddr_fe.sin_addr.s_addr = htonl(INADDR_ANY); /* accept reqs to any IP addr */
  serveraddr_fe.sin_port = htons((unsigned short)port_fe); /* port to listen on */

  /* bind: associate the listening socket with a port */
  if (bind(listenfd_fe, (struct sockaddr *) &serveraddr_fe, sizeof(serveraddr_fe)) < 0)
    error("ERROR on binding front-end socket with port");

  /* listen: make it a listening socket ready to accept connection requests 
   *         - only need to listen for front-end
   */
  if (listen(listenfd_fe, 10) < 0) /* allow 10 requests to queue up */
    error("ERROR on listen");
  
  return listenfd_fe;
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
      
      write_full_content(connfd, fp, fileExt,
			 content_size, last_modified);
      
      sprintf(lb, "Sent %d bytes to {Client %d}!", content_size, tid);
      log_thr(lb, ct->num, tid);
    }
    
    /* closing file*/
    fclose(fp);
  }
  return 1;
}

