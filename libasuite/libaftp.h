/* libaftp - AFTP API library for Linux-SNA
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

#if HAVE_SNA_CMC_H
#include <sna/cmc.h>
#include <port.h>
#else
#include <linux/cpic.h>
#endif

#define OS_NAME "Linux"

#include <netinet/in.h>
#include <unistd.h>

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <asuitecommon.h>
#include <appfftp.h>

#define MSG_BUFFER_LENGTH 512
#define MAX_BUFFER_SIZE 32763
#define MAX_FILE_NAME   128
#define MAX_STRING 32
#define MAX_SYS_INFO    64
#define MAJOR_VERSION   0
#define MINOR_VERSION   1

/* Major codes */
#define AFTP_SIMPLE_RESPONSE 1
#define AFTP_QUERY_SYSTEM    2
#define AFTP_LIST_REQUEST    3
#define AFTP_LIST_ENTRY      4
#define AFTP_LIST_COMPLETE   5
#define AFTP_CHANGE_DIR      6
#define AFTP_QUERY_DIR       7
#define AFTP_CREATE_DIR      8
#define AFTP_REMOVE_DIR      9
#define AFTP_DELETE          10
#define AFTP_RENAME          11
#define AFTP_SEND_FILE       12
#define AFTP_REQUEST_FILE    13
#define AFTP_FILE_DATA       14
#define AFTP_FILE_COMPLETE   15
#define AFTP_TOWER_REQUEST   16
#define AFTP_TOWER_RESPONSE  17
#define AFTP_CLOSE           18
#define AFTP_TRACE           19

/* Key codes */
#define AFTP_STATUS_RETURN_CODE    2
#define AFTP_STATUS_CATEGORY_INDEX 4
#define AFTP_STATUS_PRIMARY_MSG    5
#define AFTP_STATUS_SECONDARY_MSG  6
#define AFTP_STATUS_INFO_MSG       7
#define AFTP_STATUS_LOG_MSG        8
#define AFTP_TOWER_LIST            16
#define AFTP_FILESPEC              17
#define AFTP_NEW_FILESPEC          18
#define AFTP_FILE_TYPE             19
#define AFTP_INFO_LEVEL            20
#define AFTP_MOD_TIME              21
#define AFTP_SERVER_BLOCK          22
#define AFTP_SYSTEM_INFO           23
#define AFTP_DATA_TYPE             24
#define AFTP_DATE_MODE             25
#define AFTP_WRITE_MODE            26
#define AFTP_RECORD_FORMAT         27
#define AFTP_RECORD_LENGTH         28
#define AFTP_ALLOCATION_SIZE       29
#define AFTP_BLOCK_SIZE            30
#define AFTP_TRACE_LEVEL           33
#define AFTP_RESERVED              0xFF

/* Idea shamelessly stolen from GTK+ */
#define aftp_new(type,count) (type *)malloc (sizeof (type) * (count))

struct _codemap {
  unsigned long code;
  unsigned char string[MAX_STRING];
};

typedef struct _codemap codemap;

struct _ftpsession {
  AFTP_HANDLE_TYPE session_handle;
  int symbolic;
  unsigned char destination[CM_PLN_SIZE+1];
  unsigned char mode_name[CM_MN_SIZE+1];
  unsigned char tp_name[CM_TPN_SIZE+1];
  unsigned char conversation_ID[8];
  unsigned char password[AFTP_PASSWORD_SIZE+1];
  unsigned char userid[AFTP_PASSWORD_SIZE+1];
  unsigned char sys_info[MAX_SYS_INFO];
  asuiterecord * record;
  AFTP_ALLOCATION_SIZE_TYPE alloc_size;
  AFTP_BLOCK_SIZE_TYPE block_size;
  AFTP_DATA_TYPE_TYPE data_type;
  AFTP_DATE_MODE_TYPE date_mode;
  AFTP_RECORD_FORMAT_TYPE record_format;
  AFTP_RECORD_LENGTH_TYPE record_length;
  AFTP_SECURITY_TYPE security_type;
  AFTP_WRITE_MODE_TYPE write_mode;
  int status;
  unsigned char primary_msg[MSG_BUFFER_LENGTH+1];
  int msg_category;
  int msg_index;
  char * current_directory;
};

typedef struct _ftpsession ftpsession;

void set_primary_msg(ftpsession * session, int curpos);
void get_code_string(codemap * map,
		     unsigned long code,
		     unsigned char AFTP_PTR string,
		     AFTP_LENGTH_TYPE string_size,
		     AFTP_LENGTH_TYPE AFTP_PTR returned_length,
		     AFTP_RETURN_CODE_TYPE AFTP_PTR rc);
