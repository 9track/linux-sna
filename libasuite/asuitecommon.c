/* asuite - APPC suite common functions for Linux-SNA
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

#include "asuitecommon.h"

/*
  Builds a list of files in the specified directory that match the filespec.
  The list is contained in a dynarray, so the caller must release the 
  allocated memory by calling dynarray_destroy when they are finished with it.
*/

gint fname_compare(gconstpointer a, gconstpointer b)
{
  return(strcmp(a,b));
}

int get_local_file_list(char * directory, char * filespec, GSList ** results,
			const int sort)
{
  GSList * filelist;
  DIR * curdir;
  struct dirent * curfile;
  char * tempstr;

  syslog(LOG_INFO, "directory = %s\n", directory);

  curdir = opendir(directory);

  if(curdir == NULL) {
    return(-1);
  }

  filelist = NULL;

  curfile = readdir(curdir);

  while(curfile != NULL) {
    if(wc_match(curfile->d_name, filespec)) {
      tempstr = g_malloc(strlen(curfile->d_name)+1);
      strcpy(tempstr, curfile->d_name);
      filelist = g_slist_insert_sorted(filelist, tempstr, fname_compare);
    }
    curfile = readdir(curdir);
  }

  closedir(curdir);

  *results = filelist;

  return(0);
}

void
free_single_list(GSList * filelist)
{
  GSList * curelem;

  curelem = filelist;

  while(curelem != NULL)
    {
      g_free(curelem->data);
      curelem = g_slist_next(curelem);
    }

  if(filelist != NULL)
    {
      g_slist_free(filelist);
    }

}

/* 
   Returns the length of the parameter residing at position curpos in the 
   buffer 
*/
unsigned int
get_param_len(asuiterecord * record, int curpos)
{
  unsigned int len;

  len = record->buffer[curpos+1] << 8;
  len |= record->buffer[curpos];
  len = ntohs((u_int16_t) len);

  return(len);

}

/* 
   Returns the two byte code residing at position curpos in the 
   buffer
*/
unsigned int
get_cat_index(asuiterecord * record, int curpos)
{
  unsigned int code;

  code = record->buffer[curpos+1] << 8;
  code |= record->buffer[curpos];
  code = ntohs((u_int16_t) code);

  return(code);

}

/* Returns the major code from current buffer */
int 
get_major_code(asuiterecord * record)   
{
  return(record->buffer[1]);
}

/* Returns the length of the current buffer */
int
get_received_len(asuiterecord * record) 
{
  return(record->position);
}

/* Returns the parameter key residing at position curpos */
int
get_key(asuiterecord * record, int curpos)
{
  return(record->buffer[curpos+2]);
}

/*
  Creates a new record for ASUITE transactions
*/
asuiterecord * 
asuiterecord_new()
{
  asuiterecord * newrecord = asuite_new(asuiterecord,1);

  if(newrecord != NULL) {
    newrecord->position = 2;
  }
  return(newrecord);
}

/*
  Clears the record.
*/
void 
asuiterecord_clear(asuiterecord * record)
{

  if(record != NULL) {
    record->position = 2;  /* This is the first position after the major code */
  }
}

/*
  Sets the major code for the record.  The code is two bytes, and is stored in
  big endian order.
*/
void 
asuiterecord_settype(asuiterecord * record, int rectype) 
{
  u_int16_t temp = htons((u_int16_t)rectype);

  if(record != NULL) {
    record->buffer[0] = temp & 0x00FF;
    record->buffer[1] = (temp & 0xFF00) >> 8;
  }
}

/*
  Appends a parameter key and associated data to the record.  The parameter 
  length is a two byte integer and is stored in big endian format.
*/
int 
asuiterecord_append(asuiterecord * record, unsigned char key, 
		    unsigned char * data, int length)
{
  u_int16_t temp = (u_int16_t)(length+3);
  temp = htons(temp);

  if(record != NULL) {
    record->buffer[record->position] = temp & 0x00FF;
    record->position++;
    record->buffer[record->position] = temp >> 8;
    record->position++;
    record->buffer[record->position] = key;
    record->position++;
    strncpy(&record->buffer[record->position], data, length);
    record->position += length;
    return(0);
  } else {
    return(1);
  }
}

/*
  Appends a parameter key and associated data to the buffer.  The parameter 
  is a single unsigned character.
*/
int 
asuiterecord_append_char(asuiterecord * record, unsigned char key, 
			 unsigned char data)
{
  u_int16_t temp = (u_int16_t)(4);
  temp = htons(temp);

  if(record != NULL) {
    record->buffer[record->position] = temp & 0x00FF;
    record->position++;
    record->buffer[record->position] = temp >> 8;
    record->position++;
    record->buffer[record->position] = key;
    record->position++;
    record->buffer[record->position] = data;
    record->position++;
    return(0);
  } else {
    return(1);
  }
}

/*
  Appends a parameter key and associated data to the buffer.  The parameter 
  is a 4 byte integer.
*/
int 
asuiterecord_append_fourbyte(asuiterecord * record, unsigned char key, 
			     unsigned long data)
{
  u_int16_t temp = (u_int16_t)(4);
  u_int32_t temp_data;

  temp = htons(temp);

  temp_data = htonl(data);

  if(record != NULL) {
    record->buffer[record->position] = temp & 0x00FF;
    record->position++;
    record->buffer[record->position] = temp >> 8;
    record->position++;
    record->buffer[record->position] = key;
    record->position++;
    record->buffer[record->position] = temp_data & 0x000000FF;
    record->position++;
    record->buffer[record->position] = (temp_data & 0x0000FF00) >> 8;
    record->position++;
    record->buffer[record->position] = (temp_data & 0x00FF0000) >> 16;
    record->position++;
    record->buffer[record->position] = (temp_data & 0xFF000000) >> 24;
    record->position++;
    return(0);
  } else {
    return(1);
  }
}

/*
  Appends a parameter key and associated data to the buffer.  The parameter 
  is a 2 byte integer.
*/
int 
asuiterecord_append_twobyte(asuiterecord * record, unsigned char key, 
			    int count, ...)
{
  u_int16_t temp = (u_int16_t)(3);
  u_int16_t temp_data;
  va_list argp;
  unsigned int p;
  unsigned len_index = record->position;
  int argcnt;

  argcnt = 0;
  if(record != NULL) {
    record->position = record->position + 2;
    record->buffer[record->position] = key;
    record->position++;

    va_start(argp, count);
    p = va_arg(argp, unsigned int);

    for(argcnt = 0; argcnt < count; argcnt++) {
      printf("OK\n");
      temp_data = p;
      temp_data = htons(temp_data);
      record->buffer[record->position] = temp_data & 0x000000FF;
      record->position++;
      record->buffer[record->position] = (temp_data & 0x0000FF00) >> 8;
      record->position++;
      p = va_arg(argp, unsigned int);
    }
    va_end(argp);

    printf("two_byte = %d\n", argcnt);
    temp = temp + (u_int16_t)(2*count);
    printf("two_byte = %d\n", temp);

    temp = htons(temp);

    record->buffer[len_index] = temp & 0x00FF;
    record->buffer[len_index+1] = temp >> 8;

    return(0);
  } else {
    return(1);
  }
}

/*
  Appends a parameter key and associated data to the buffer.  The parameter 
  is a 2 byte integer.
*/
int 
asuiterecord_append_bare_twobyte(asuiterecord * record, 
				 unsigned long data)

{
  u_int16_t temp_data;

  temp_data = htons(data);

  if(record != NULL) {
    record->buffer[record->position] = temp_data & 0x000000FF;
    record->position++;
    record->buffer[record->position] = (temp_data & 0x0000FF00) >> 8;
    record->position++;
    return(0);
  } else {
    return(1);
  }
}

unsigned char asuiterecord_get_char(asuiterecord * record, 
				    unsigned int position)
{
  return(record->buffer[position]);
}

/*
  Deallocates the record.
*/
int 
asuiterecord_destroy(asuiterecord * record)
{

  if(record != NULL) {
    free(record);
    return(0);
  } else {
    return(1);
  }
}

/*
  Sends the record to the server.
*/
int 
send_asuiterecord(unsigned char * conv_id, asuiterecord * record)
{
  CM_INT32 rts_received;
  CM_INT32 cm_rc;

  CM_INT32 length = record->position;
  
  cmsend(conv_id, record->buffer, &length, 
	 &rts_received, &cm_rc);

  if(cm_rc != CM_OK) {
    return(cm_rc);
  } else {
    return(ASUITE_RC_OK);
  }

}

/*
  Retrieves the response record from the server.
*/
int 
get_asuiterecord(unsigned char * conv_id, asuiterecord * record)
{
  CM_INT32 cm_rc;
  CM_DATA_RECEIVED_TYPE data_received;
  CM_INT32 received_length;
  CM_STATUS_RECEIVED status_received;
  CM_REQUEST_TO_SEND_RECEIVED rts_received;
  CM_INT32 requested_length = MAX_BUFFER_SIZE;
  
  cmrcv(conv_id, record->buffer, &requested_length,
	&data_received, &received_length, &status_received, &rts_received, 
	&cm_rc);
  
  if(cm_rc == CM_OK) {
    record->position = received_length;
    return(ASUITE_RC_OK);
  } else {
    return(cm_rc);
  }
}

void
send_simple_response(unsigned char * conv_id, unsigned int errcode,
		     unsigned int category, unsigned int index) 
{
  CM_INT32 rc;
  asuiterecord * err_rec;

  err_rec = asuiterecord_new();

  asuiterecord_clear(err_rec);
  asuiterecord_settype(err_rec, ASUITE_STATUS_RECORD);
  asuiterecord_append_char(err_rec, ASUITE_STATUS_RETURN_CODE, errcode);
  if(errcode != 0) {
    asuiterecord_append_twobyte(err_rec, ASUITE_STATUS_CATEGORY_INDEX, 
				2, category, index);

    asuiterecord_append(err_rec, ASUITE_STATUS_PRIMARY_MSG,
			"ERROR", 6);

  }
  rc = send_asuiterecord(conv_id, err_rec);

  asuiterecord_destroy(err_rec);

}

char * 
safe_get_cwd()
{
  int done;
  char * result;
  int count;
  char * final;

  done = 0;
  count = 1;

  result = getcwd(NULL, CWD_CHUNK);

  do {
    if(result == NULL) {
      if(errno == ERANGE) {
	result = getcwd(NULL, count*CWD_CHUNK);
      } else {
	return(NULL);
      }
    } else {
      done = 1;
    }
  } while(!done);

  final = malloc(strlen(result)+2);

  if(final != NULL) {
    strcpy(final, result);
    strcat(final, "/");
    free(result);
  }

  return final;
}
