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


int update_config_file(char* filename, char* field_tag, int peer_num,
                        char* new_value, char* old_buf) {
  /* locals */
  int init_peer_count;
  //int upc_res_flag;  /* updated peer count result flag */
  int valid_flag;
  FILE* fp;
  FILE* fp_temp;

  char* read_buf = malloc(CF_MAX_LINE_LEN);
  char* buf = malloc(CF_MAX_LINE_LEN);
  char* temp_fname = malloc(CF_MAX_LINE_LEN);
  char* update_buf = malloc(CF_MAX_LINE_LEN);
  char* field_buf = malloc(CF_MAX_LINE_LEN);
  char* buf_ptr = NULL;
  char* peer_count_ptr = NULL;

  bzero(read_buf, CF_MAX_LINE_LEN);
  bzero(buf, CF_MAX_LINE_LEN);
  bzero(temp_fname, CF_MAX_LINE_LEN);
  bzero(update_buf, CF_MAX_LINE_LEN);
  bzero(field_buf, CF_MAX_LINE_LEN);
  sprintf(temp_fname, CF_DEFAULT_FILENAME);

  /* Checking status of given file */
  valid_flag = validate_config_file(filename);

  if(valid_flag == -1) {
    /* Could not find file "filename" */
    free(read_buf);
    free(buf);
    free(temp_fname);
    return valid_flag;
  }

  if(valid_flag == 0 && (strcmp(field_tag, CF_TAG_UUID) != 0)) {
    /* Could not find UUID field in "filename"
     * AND UUID field is not being added */
    free(read_buf);
    free(buf);
    free(temp_fname);
    return 0;
  }

  /* Searching file for "field_tag" and storing possible past value old_buf */
  buf_ptr = get_config_field(filename, field_tag, peer_num);
  peer_count_ptr = get_config_field(filename, CF_TAG_PEER_COUNT, peer_num);

  /* Did not find "field_tag" in file "filename" */
  if(!buf_ptr) {
    /* Checking if new tag being added is peer_info tag */
    if(strcmp(field_tag, CF_TAG_PEER_INFO) == 0) {
      if(!peer_count_ptr) {
        /* No peer count field found exists */
        sprintf(update_buf, "1");
      }
      else {
        /* Peer count field exists - getting incremented value */
        init_peer_count = atoi(peer_count_ptr) + 1;
        sprintf(update_buf, "%d", init_peer_count);
      }

      /* Updating config file's peer_count before adding peer
       *  so that peer_info lines will be below peer_count */
      update_config_file(filename, CF_TAG_PEER_COUNT,
                          peer_num, update_buf, NULL);
      /* CHECK - Might want to store this flag value */

      /* Creating string for writing peer_info based on peer_num */
      sprintf(field_buf, "peer_%d", peer_num);
    }

    /* For all other field tags, just use the tag (ignore peer_num) */
    else {
      sprintf(field_buf, "%s", field_tag);
    }

    /* Adding field and value to end of file */
    /* note: need to print new line before buf so this prints on next line */
    fp = fopen(filename, "a");
    sprintf(buf, "%s = %s", field_buf, new_value);
    fprintf(fp, "%s\n", buf);
    fclose(fp);

    /* Returning "added field" flag */
    return 1;
  }

  /* Storing existing field "field_tag" value in old_buf if not NULL */
  if(old_buf) sprintf(old_buf, "%s", buf_ptr);

  /* Opening file and temporary file to fill with */
  fp = fopen(filename, "r+");
  fp_temp = fopen(temp_fname, "w");
  if(!fp_temp) {
    printf("Error updating config file; Unable to create temp file.\n");
    fclose(fp);
    return 0;
  }


  if(strcmp(field_tag, CF_TAG_PEER_INFO) == 0) {
    /* Formatting field buf to be able to differentiate peer_info cases */
    sprintf(field_buf, "peer_%d", peer_num);
  }
  else {
    /* Scan for field buf as is */
    sprintf(field_buf, "%s", field_tag);
  }


  /* Iterating through file and storing contents in temp file */
  while(fgets(read_buf, CF_MAX_LINE_LEN, fp)) {
    bzero(buf, CF_MAX_LINE_LEN);
    if(strstr(read_buf, field_buf)) {
      /* Read the desired field to update, so write new value */
      sprintf(buf, "%s = %s", field_buf, new_value);
      fprintf(fp_temp, "%s\n", buf);
    }
    else {
      /* Did not read desired field, so re-write preexisting field and value */
      sprintf(buf, "%s", read_buf);
      fprintf(fp_temp, "%s", buf);

      /* Formatting - print new-line at EOF for future added vals */
      if(feof(fp)) { fputs("\n", fp_temp); }
    }
  }

  /* Closing files */
  fclose(fp);
  fclose(fp_temp);

  /* Renaming temp as "filename" and removing old "filename" */
  remove(filename);
  rename(temp_fname, filename);

  /* Returning "updated" flag */
  return 2;
}





int create_config_file(char* filename, char* node_name, 
                        int fe_port, int be_port, char* content_dir) {
  /* local vars */
  FILE* fp;
  uuid_t new_uuid;
  char uuid_arr[CF_UUID_STR_LEN];
  char buf[CF_MAX_LINE_LEN];

  if(check_file(filename)) {
    return 0;
  }

  fp = fopen(filename, "w+");
  if(!fp) {
    printf("Server Error: Failed to create config file: %s\n", filename);
    return 0;
  }

  /* Generating new UUID for node and parsing to char[] */
  uuid_generate(new_uuid);
  uuid_unparse_upper(new_uuid, uuid_arr);

  /* writing UUID to created file */
  sprintf(buf, "%s = %s", CF_TAG_UUID, uuid_arr);
  fprintf(fp, "%s\n", buf);
  bzero(buf, CF_MAX_LINE_LEN);

  /* writing default front-end port to created file */
  sprintf(buf, "%s = %d", CF_TAG_FE_PORT, fe_port);
  fprintf(fp, "%s\n", buf);
  bzero(buf, CF_MAX_LINE_LEN);

  /* writing default back-end port to created file */
  sprintf(buf, "%s = %d", CF_TAG_BE_PORT, be_port);
  fprintf(fp, "%s\n", buf);
  bzero(buf, CF_MAX_LINE_LEN);

  /* writing default content directory to created file */
  sprintf(buf, "%s = %s", CF_TAG_CONTENT_DIR, content_dir);
  fprintf(fp, "%s\n", buf);
  bzero(buf, CF_MAX_LINE_LEN);

  /* writing default peer_count to created file */
  sprintf(buf, "%s = %d", CF_TAG_PEER_COUNT, 0);
  fprintf(fp, "%s\n", buf);
  bzero(buf, CF_MAX_LINE_LEN);

  /* Done creating new default config file */
  fclose(fp);
  return 1;

}




