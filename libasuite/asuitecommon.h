/* asuite - APPC asuite common functions for Linux-SNA
 *
 * Author: Michael Madore <mmadore@turbolinux.com>
 *
 * Copyright (C) 2000 TurboLinux, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <syslog.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <dirent.h>
#include <stdarg.h>
#include <sys/stat.h>
#include "wildcard.h"
#if HAVE_SNA_CMC_H
#include <sna/cmc.h>
#else
#include <linux/cpic.h>
#endif
#include <glib.h>

#define MAX_BUFFER_SIZE 32763
#define CWD_CHUNK 64

#define ASUITE_STATUS_RECORD         1

#define ASUITE_STATUS_RETURN_CODE    2
#define ASUITE_STATUS_CATEGORY_INDEX 4
#define ASUITE_STATUS_PRIMARY_MSG    5
#define ASUITE_STATUS_SECONDARY_MSG  6
#define ASUITE_STATUS_INFO_MSG       7
#define ASUITE_STATUS_LOG_MSG        8

typedef unsigned long       ASUITE_RETURN_CODE_TYPE;

#define ASUITE_RC_OK                      ((ASUITE_RETURN_CODE_TYPE)0)
#define ASUITE_RC_COMM_FAIL_NO_RETRY      ((ASUITE_RETURN_CODE_TYPE)1)
#define ASUITE_RC_COMM_FAIL_RETRY         ((ASUITE_RETURN_CODE_TYPE)2)
#define ASUITE_RC_COMM_CONFIG_LOCAL       ((ASUITE_RETURN_CODE_TYPE)3)
#define ASUITE_RC_COMM_CONFIG_REMOTE      ((ASUITE_RETURN_CODE_TYPE)4)
#define ASUITE_RC_SECURITY_NOT_VALID      ((ASUITE_RETURN_CODE_TYPE)5)
#define ASUITE_RC_FAIL_INPUT_ERROR        ((ASUITE_RETURN_CODE_TYPE)6)
#define ASUITE_RC_FAIL_RETRY              ((ASUITE_RETURN_CODE_TYPE)7)
#define ASUITE_RC_FAIL_NO_RETRY           ((ASUITE_RETURN_CODE_TYPE)8)
#define ASUITE_RC_FAIL_FATAL              ((ASUITE_RETURN_CODE_TYPE)9)
#define ASUITE_RC_PROGRAM_INTERNAL_ERROR  ((ASUITE_RETURN_CODE_TYPE)10)
#define ASUITE_RC_PARAMETER_CHECK         ((ASUITE_RETURN_CODE_TYPE)11)
#define ASUITE_RC_HANDLE_NOT_VALID        ((ASUITE_RETURN_CODE_TYPE)12)
#define ASUITE_RC_STATE_CHECK             ((ASUITE_RETURN_CODE_TYPE)13)
#define ASUITE_RC_BUFFER_TOO_SMALL        ((ASUITE_RETURN_CODE_TYPE)14)

/* Error Catagories */
#define A_ERR_UNKNOWN_PARAMETER    128
#define A_ERR_OUT_OF_MEMORY        136
#define A_ERR_ACTION_NOT_PERFORMED 601
#define A_ERR_FILE_NOT_FOUND       602
#define A_ERR_ACCESS_DENIED        603
#define A_ERR_FILE_EXISTS          606
#define A_ERR_INVALID_PATH         607
#define A_ERR_CROSS_DEVICES        612
#define A_ERR_NO_FILES_FOUND       614

/* Idea shamelessly stolen from GTK+ */
#define asuite_new(type,count) (type *)malloc (sizeof (type) * (count))

struct _asuiterecord {
  int position;
  unsigned char buffer[MAX_BUFFER_SIZE];
};

typedef struct _asuiterecord asuiterecord;

unsigned int get_cat_index(asuiterecord * record, int curpos);
int get_major_code(asuiterecord * record);
int get_received_len(asuiterecord * record); 
int get_key(asuiterecord * record, int curpos);
unsigned int get_param_len(asuiterecord * record, int curpos);
unsigned int get_int_code(asuiterecord * record, int curpos);
asuiterecord * asuiterecord_new(void);
void asuiterecord_clear(asuiterecord * record);
void asuiterecord_settype(asuiterecord * record, int rectype);
int asuiterecord_append(asuiterecord * record, unsigned char key, 
		       unsigned char * data, int length);
int asuiterecord_append_char(asuiterecord * record, unsigned char key, 
			    unsigned char data);
int asuiterecord_append_fourbyte(asuiterecord * record, unsigned char key, 
				unsigned long data);
int asuiterecord_append_twobyte(asuiterecord * record, unsigned char key,
			       int count, ...);
int asuiterecord_append_bare_twobyte(asuiterecord * record, 
				     unsigned long data);
unsigned char asuiterecord_get_char(asuiterecord * data, 
				    unsigned int position);
int asuiterecord_destroy(asuiterecord * record);

int send_asuiterecord(unsigned char * conv_id, asuiterecord * record);
int send_asuiterecord_file(unsigned char * conv_id, asuiterecord * record);
int get_asuiterecord(unsigned char * conv_id, asuiterecord * record);

int get_local_file_list(char * directory, char * filespec, GSList ** results,
			const int sort);
void free_single_list(GSList * filelist);
void send_simple_response(unsigned char * conv_id, unsigned int errcode,
			  unsigned int category, unsigned int index);
char * safe_get_cwd();


