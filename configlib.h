/*
 *  configlib.h
 *			- library fns for config file handling
 */
#ifndef CONFIGLIB_H
#define CONFIGLIB_H

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>

#include </usr/include/uuid/uuid.h>

#include "parser.h"

/* default config file: filename */
#define CF_DEFAULT_FILENAME "node.conf"

/* default config file: front-end port */
#define CF_DEFAULT_FE_PORT 9001

/* default config file: back-end port */
#define CF_DEFAULT_BE_PORT 9002
/* default config file: content directory */
#define CF_DEFAULT_CONTENT_DIR "content/"
/* default config file: peer count */
#define CF_DEFAULT_PEER_COUNT 0


/* valid config file fields: */
#define CF_TAG_UUID "uuid"
#define CF_TAG_NAME "name"
#define CF_TAG_FE_PORT "frontend_port"
#define CF_TAG_BE_PORT "backend_port"
#define CF_TAG_CONTENT_DIR "content_dir"
#define CF_TAG_PEER_COUNT "peer_count"
#define CF_TAG_PEER_INFO "peer_"

/* Length of char[] rep. of UUID */
#define CF_UUID_STR_LEN 37
/* Max line length of config file */
#define CF_MAX_LINE_LEN 512






/*
 *  get_config_field
 *    - searches for certain info in config file
 *    - stores requested info in char buffer
 *    - NOTE: this fn will open and close file once its done so as to prevent 
 *              bad scary things
 *
 *    params:
 *      filename - config file name; use CF_DEFAULT_FILENAME for default file
 *      field_tag - one of the CF_TAG_<tag> constants defined above
 *      peer_num - only used when field_tag = CF_TAG_PEER_INFO and is used
 *                  to search for the specific peer number, i.e. peer_1 or peer_2
 *
 *    return values:
 *        NULL --> fail on finding file
 *
 *        non-null results are based on field_tag:
 *          CF_TAG_UUID         --> res = "<uuid string>"
 *          CF_TAG_NAME         --> res = "<name string>"
 *          CF_TAG_FE_PORT      --> res = "<port num>"
 *          CF_TAG_BE_PORT      --> res = "<port num>"
 *          CF_TAG_CONTENT_DIR  --> res = "<content dir filepath>"
 *          CF_TAG_PEER_COUNT   --> res = "<num peers>"
 *
 *          CF_TAG_PEER_INFO    --> res = "<peer info string>"
 *            *result also depends on peer_num*
 *
 *
 */
char* get_config_field(char* filename, char* field_tag, int peer_num);

/*
 *  validate_config_file
 *
 *    - searches for config file UUID tag using get_config_field
 *
 *    - NOTE: this fn will open and close file once its done so as to prevent 
 *              bad scary things
 *
 *    return values:
 *        -1 --> fail on finding file
 *         0 --> fail on finding UUI
 *         1 --> found UUID tag in file
 */
int validate_config_file(char* filename);


/*
 *  check_default_config_file
 *		- searches for config file matching defined default config file name
 *		- if default file is not found, it generates one for this peer
 *    - NOTE: this fn will open and close file once its done so as to prevent
 *              bad scary things
 *
 *		return values:
 *        0 --> fail on both finding default config file and
 *                creating a default file
 *        1 --> did not find existing default config but
 *                created one successfully
 *        2 --> found existing default config file
 */
int check_default_config_file();


/*
 *  check_file
 *    - searches in own directory for file matching given filename
 *    - NOTE: this fn will open file if it finds it and close the file once
 *              its done so as to prevent bad scary things
 *
 *    return values:
 *        0 --> failure to find (config) file
 *        1 --> success on finding (config) file
 */
int check_file(char* filename);


/*
 *  update_config_file
 *    - attempts to update given config file with new value
 *    - will add the field and value if file does not have it yet
 *    - NOTE: this fn will open file if it finds it and close the file once
 *              its done so as to prevent bad scary things
 *
 *    params:
 *      filename - config file name; use CF_DEFAULT_FILENAME for default file
 *      field_tag - one of the CF_TAG_<tag> constants defined above
 *      peer_num - only used when field_tag = CF_TAG_PEER_INFO and is used
 *                  to search for the specific peer number, i.e. peer_1 or peer_2
 *      new_value - new value to be written to config file's <field_tag> field
 *      old_buf - if not NULL, will store old value in this pointer
 *
 *    return values:
 *       -1 --> failure to find file "filename"
 *        0 --> failure to find valid config file
 *        1 --> added tag and value to file
 *        2 --> updated tag and value
 */
int update_config_file(char* filename, char* field_tag, int peer_num,
                        char* new_value, char* old_buf);







/*
 *  create_config_file
 *    - attempts to create config file with given values
 *    - NOTE: this fn will open file if it finds it and close the file once
 *              its done so as to prevent bad scary things
 *
 *    return values:
 *        0 --> failure to create config file or file with same name exists
 *        1 --> created file
 */

int create_config_file(char* filename, char* node_name, 
                        int fe_port, int be_port, char* content_dir);




#endif
