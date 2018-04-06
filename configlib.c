/*
 *  configlib.c
 *			- implementations of fns for config file handling
 */

#include "configlib.h"

char* get_config_field(char* filename, char* field_tag, int peer_num) {
  FILE* fp;
  char *res_buf = malloc(sizeof(char) * CF_MAX_LINE_LEN);
  char *buf = malloc(CF_MAX_LINE_LEN);
  //char *long_buf = malloc(2 * CF_MAX_LINE_LEN);
  int num;

  bzero(res_buf, CF_MAX_LINE_LEN);
  bzero(buf, CF_MAX_LINE_LEN);
  

  if(check_file(filename)) {
    /* If file exists, open for reading */
    fp = fopen(filename, "r");

    /* UUID */ 
    if(strcmp(field_tag, CF_TAG_UUID) == 0) {
      /* read lines until we find uuid */
      while(fgets(buf, CF_MAX_LINE_LEN, (FILE*) fp)) {
        /* Searching for tag and value */
        if(sscanf(buf, "uuid = %s", res_buf) == 1) {
          /* Found tag and value; close file and return data */
          fclose(fp);
          return res_buf;
        }
      }
    }

    /* Name */
    else if(strcmp(field_tag, CF_TAG_NAME) == 0) {
      /* read lines until we find name */
      while(fgets(buf, CF_MAX_LINE_LEN, (FILE*) fp)) {
        /* Searching for tag and value */
        if(sscanf(buf, "name = %s", res_buf) == 1) {
          /* Found tag and value; close file and return data */
          fclose(fp);
          return res_buf;
        }
      }
    }

    /* Content Directory */
    else if(strcmp(field_tag, CF_TAG_CONTENT_DIR) == 0) {
      /* read lines until we find name */
      while(fgets(buf, CF_MAX_LINE_LEN, (FILE*) fp)) {
        /* Searching for tag and value */
        if(sscanf(buf, "content_dir = %s", res_buf) == 1) {
          /* Found tag and value; close file and return data */
          fclose(fp);
          return res_buf;
        }
      }
    }

    /* Front-end Port */
    else if(strcmp(field_tag, CF_TAG_FE_PORT) == 0) {
      /* read lines until we find name */
      while(fgets(buf, CF_MAX_LINE_LEN, (FILE*) fp)) {
        /* Searching for tag and value */
        if(sscanf(buf, "frontend_port = %d", &num) == 1) {
          /* Found tag and value; close file and return data */
          sprintf(res_buf, "%d", num);
          fclose(fp);
          return res_buf;
        }
      }
    }

    /* Back-end Port */
    else if(strcmp(field_tag, CF_TAG_BE_PORT) == 0) {
      /* read lines until we find name */
      while(fgets(buf, CF_MAX_LINE_LEN, (FILE*) fp)) {
        /* Searching for tag and value */
        if(sscanf(buf, "backend_port = %d", &num) == 1) {
          /* Found tag and value; close file and return data */
          sprintf(res_buf, "%d", num);
          fclose(fp);
          return res_buf;
        }
      }
    }

    /* Peer Count */
    else if(strcmp(field_tag, CF_TAG_PEER_COUNT) == 0) {
      /* read lines until we find name */
      while(fgets(buf, CF_MAX_LINE_LEN, (FILE*) fp)) {
        /* Searching for tag and value */
        if(sscanf(buf, "peer_count = %d", &num) == 1) {
          /* Found tag and value; close file and return data */
          sprintf(res_buf, "%d", num);
          fclose(fp);
          return res_buf;
        }
      }
    }

    /* Peer Info */
    else if((strcmp(field_tag, CF_TAG_PEER_INFO) == 0)) {
      /* read lines until we find name */
      while(fgets(buf, CF_MAX_LINE_LEN, (FILE*) fp)) {
        /* Searching for tag and value */
        if(sscanf(buf, "peer_%d = %s", &num, res_buf) == 2) {
          if(num == peer_num) {
            /* found matching peer */
            fclose(fp);
            return res_buf;
          }        
        }
      }
    }

    /* End of case statements */
  }

  /* Failed finding tag */
  return NULL;
}


  //   char* tag_line

  //   tag_line = ;

  //   /* read through the whole file */
  //   
  //     

  //     printf("IN WHILE LOOP\n");

  //     /* UUID, Name and Content Directory */
  //     if( (strcmp(field_tag, CF_TAG_UUID) == 0) ||
  //         (strcmp(field_tag, CF_TAG_NAME) == 0) ||
  //         (strcmp(field_tag, CF_TAG_CONTENT_DIR) == 0)) {

  //       printf("Scanning file for field: %s\n", field_tag);
  //       printf("buf: \n%s\n", buf);



  //       /* Searching for tag and value */
  //       if(sscanf(buf, "%s = %s", field_tag, res_buf) == 2) {
  //         /* Found tag and value; close file and return data */
  //         printf("Scanned success.{47}\n");
  //         fclose(fp);
  //         return res_buf;
  //       }
  //       printf("Failed if check.\n");
  //     }

  //     /* Front-end Ports, Back-end Ports and Peer Count */
  //     else if((strcmp(field_tag, CF_TAG_FE_PORT) == 0) ||
  //             (strcmp(field_tag, CF_TAG_BE_PORT) == 0) || 
  //             (strcmp(field_tag, CF_TAG_PEER_COUNT) == 0)) {

  //       printf("Scanned file's field: %s\n", field_tag);
  //       printf("buf: %s\n", buf);

  //       /* Searching for tag and value */
  //       if(sscanf(buf, "%s = %d", field_tag, &num) == 2) {
  //         /* Found tag and value; close file and return data */
  //         sprintf(res_buf, "%d", num);
  //         fclose(fp);
  //         return res_buf;
  //       }
  //     }

  //     /* Peer Info */
  //     else if((strcmp(field_tag, CF_TAG_PEER_INFO) == 0)) {
  //       /* Creating peer info string to search for using peer_num */
  //       sprintf(peer_num_tag, "%s%d =", CF_TAG_PEER_INFO, peer_num);

  //       printf("Scanned file's field: %s\n", peer_num_tag);
  //       printf("buf: %s\n", buf);

  //       /* Searching for tag and value */
  //       if(sscanf(buf, "%s = %s", peer_num_tag, res_buf) == 2) {
  //         /* Found tag and value; close file and return data */
  //         fclose(fp);
  //         return res_buf;
  //       }
  //     }

  //     /* Clear buffers for next iteration */
  //     bzero(buf, CF_MAX_LINE_LEN);
  //     bzero(peer_num_tag, CF_MAX_LINE_LEN);
  //     bzero(res_buf, CF_MAX_LINE_LEN);

  //     fgets(buf, CF_MAX_LINE_LEN, (FILE*)fp);
  //     read = strlen(buf);
  //   }
  //   fclose(fp);
  //   /* Read the whole file and didnt find tag */
  // }
  // return NULL;
  // }



int validate_config_file(char* filename) {
  /* Check for file and search for UUID tag */
  if(check_file(filename)) {
    if(get_config_field(filename, CF_TAG_UUID, 0)) {
      /* File has UUID tag - enough for now */
      return 1;
    }

    else {
      /* File has no UUID tag */
      return 0;
    }
  }
  /* Cannot find file */
  return -1;
}

int check_default_config_file() {
	FILE* cfp;
  /* Attempt to open existing config file */
	cfp = fopen(CF_DEFAULT_FILENAME, "r");
	if(!cfp) {
    /* default config file doesn't exist - create it */
    cfp = fopen(CF_DEFAULT_FILENAME, "w+");

    if(!cfp) {
      /* ERROR - could not create file */
      printf("Server Error: failed on creating config file\n\n");
      return 0;
    }

    uuid_t new_uuid;
    char uuid_arr[CF_UUID_STR_LEN];
    char buf[CF_MAX_LINE_LEN];
    int len;

    /* Generating new UUID for node and parsing to char[] */
    uuid_generate(new_uuid);
    uuid_unparse_upper(new_uuid, uuid_arr);

    /* writing UUID to created file */
    len = sprintf(buf, "%s = %s\n", CF_TAG_UUID, uuid_arr);
    fputs(buf, cfp);
    bzero(buf, CF_MAX_LINE_LEN);

    /* writing default front-end port to created file */
    len = sprintf(buf, "%s = %d\n", CF_TAG_FE_PORT, CF_DEFAULT_FE_PORT);
    fputs(buf, cfp);
    bzero(buf, CF_MAX_LINE_LEN);

    /* writing default back-end port to created file */
    len = sprintf(buf, "%s = %d\n", CF_TAG_BE_PORT, CF_DEFAULT_BE_PORT);
    fputs(buf, cfp);
    bzero(buf, CF_MAX_LINE_LEN);

    /* writing default content directory to created file */
    len = sprintf(buf, "%s = %s\n", CF_TAG_CONTENT_DIR, CF_DEFAULT_CONTENT_DIR);
    fputs(buf, cfp);
    bzero(buf, CF_MAX_LINE_LEN);

    /* writing default peer_count to created file */
    len = sprintf(buf, "%s = %d\n", CF_TAG_PEER_COUNT, CF_DEFAULT_PEER_COUNT);
    fputs(buf, cfp);
    bzero(buf, CF_MAX_LINE_LEN);

    /* Done creating new default config file */
    fclose(cfp);
    return 1;
	}

  /* Found existing default config file */
	fclose(cfp);
  return 2;
}


int check_file(char* filename) {
  FILE* cfp;
  cfp = fopen(filename, "r");

  if(!cfp) {    /* Could not find file */
    return 0;
  }

  /* Found file */
  fclose(cfp);
  return 1;
}