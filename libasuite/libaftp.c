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

#include "libaftp.h"

/*
 * These are the defaults to be used if the user does not provide
 * arguments to override these values.
*/
#define  DEFAULT_TP_NAME   "AFTPD"
#define  DEFAULT_MODE_NAME "#BATCH"
#define  DEFAULT_SYM_DEST  "AFTPD"

#define MAX_FTP_SESSIONS 20

static unsigned char ftp_handle_counter = 0;
static unsigned char global_trace_file[MAX_FILE_NAME];
static AFTP_TRACE_LEVEL_TYPE global_trace_level;

static ftpsession * sessions[MAX_FTP_SESSIONS];

/* Maps numeric code to a text string using a user defined code map. */ 
void
get_code_string(codemap * map,
	       unsigned long code,
	       unsigned char AFTP_PTR string,
	       AFTP_LENGTH_TYPE string_size,
	       AFTP_LENGTH_TYPE AFTP_PTR returned_length,
	       AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  int curindex;

  for(curindex = 0; map[curindex].code >= 0; curindex++)
    {
      if(map[curindex].code == code) {
	if(strlen(map[curindex].string) <= string_size) {
	  strcpy(string, map[curindex].string);
	  *returned_length = strlen(string);
	  *rc = AFTP_RC_OK;
	  return;
	} else {
	  *rc = AFTP_RC_PARAMETER_CHECK;
	}
      } 
    }
  *rc = AFTP_RC_PARAMETER_CHECK;
}

void
set_primary_msg(ftpsession * session, int curpos)
{
  strncpy(session->primary_msg, 
	  &session->record->buffer[curpos+3], 
	  MSG_BUFFER_LENGTH);
}

/*
  Changes the current directory on the AFTPD server.
*/
AFTP_ENTRY
aftp_change_dir(unsigned char AFTP_PTR connection_id,
		unsigned char AFTP_PTR directory,
		AFTP_LENGTH_TYPE length,
		AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  AFTP_RETURN_CODE_TYPE ftp_rc;
  unsigned char index = connection_id[0];
  unsigned int major_code;
  unsigned int receive_length;
  unsigned int curpos;
  unsigned int param_len;
  unsigned int key;


  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL) ){
    asuiterecord_clear(sessions[index]->record);
    asuiterecord_settype(sessions[index]->record, AFTP_CHANGE_DIR);
    asuiterecord_append(sessions[index]->record, AFTP_FILESPEC, directory, 
			length+1);
    ftp_rc = send_asuiterecord(sessions[index]->conversation_ID, 
			       sessions[index]->record);
    
    if(ftp_rc != AFTP_RC_OK) {
      *rc = ftp_rc;
      return;
    }

    ftp_rc = get_asuiterecord(sessions[index]->conversation_ID,
			      sessions[index]->record);
    
    if(ftp_rc != AFTP_RC_OK) {
      *rc = ftp_rc;
      return;
    }
	
    receive_length = get_received_len(sessions[index]->record);
    major_code = get_major_code(sessions[index]->record);
    curpos = 2;
    *rc = AFTP_RC_OK;
    switch(major_code) 
      {
      case AFTP_SIMPLE_RESPONSE: 
	do {
	  param_len = get_param_len(sessions[index]->record, curpos);
	  
	  key = get_key(sessions[index]->record, curpos);
	    
	  switch(key) 
	    {
	    case AFTP_STATUS_RETURN_CODE:
	      break;
	    case AFTP_STATUS_CATEGORY_INDEX:
	      *rc = AFTP_RC_FAIL_FATAL;
	      sessions[index]->msg_category = 
		get_cat_index(sessions[index]->record, curpos+3);
	      sessions[index]->msg_index = 
		get_cat_index(sessions[index]->record, curpos+5);
	    case AFTP_STATUS_PRIMARY_MSG:
	      set_primary_msg(sessions[index], curpos);
	      break;
	    case AFTP_SERVER_BLOCK:
	      break;
	    default:
	      fprintf(stderr, "Unknown parameter %d\n", key);
	    }
	  curpos += param_len;
	} while(curpos < length);
	break;
      default:
	fprintf(stderr, "Unknown major code %d\n", major_code);
      }
      
  } else {
    *rc = AFTP_RC_HANDLE_NOT_VALID;
  }
  
}

/*
  Disconnects from the AFTPD server.
*/
AFTP_ENTRY
aftp_close(unsigned char AFTP_PTR connection_id,
	   AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{

  AFTP_RETURN_CODE_TYPE ftp_rc;
  unsigned char index = connection_id[0];

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL) ){

    asuiterecord_clear(sessions[index]->record);
    asuiterecord_settype(sessions[index]->record, AFTP_CLOSE);
    ftp_rc = send_asuiterecord(sessions[index]->conversation_ID, 
			       sessions[index]->record);

    if(ftp_rc != AFTP_RC_OK) {
      *rc = ftp_rc;
      return;
    } 

    cmdeal(sessions[index]->conversation_ID, &ftp_rc);

    if(ftp_rc != CM_OK) {
      *rc = AFTP_RC_FAIL_FATAL;
    } else {
      *rc = AFTP_RC_OK;
    }

  } else {
    *rc = AFTP_RC_HANDLE_NOT_VALID;
  }
}

AFTP_ENTRY
aftp_connect(unsigned char AFTP_PTR connection_id,
	     AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  CM_INT32 cm_rc;
  unsigned char sym_dest[CM_SDN_SIZE];
  CM_INT32 length;
  unsigned char conv_id[8];
  AFTP_RETURN_CODE_TYPE ftp_rc;

  unsigned char index = connection_id[0];

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL)) {
      
    /* Destination was never set. */
    if(strlen(sessions[index]->destination) == 0) {
      *rc = AFTP_RC_STATE_CHECK;
      return;
    }

    if(sessions[index]->symbolic) {
      cminit(conv_id, sessions[index]->destination, &cm_rc);
    } else {
      memset(sym_dest, ' ', CM_SDN_SIZE);
      cminit(conv_id, sym_dest, &cm_rc);
    }

    if(cm_rc == CM_OK) {
      memcpy(sessions[index]->conversation_ID, conv_id, sizeof(conv_id));
    } else {
      *rc = AFTP_RC_COMM_CONFIG_LOCAL;
      return;
    }

    if(!sessions[index]->symbolic) {
      length = strlen(sessions[index]->destination);
      cmspln(conv_id, sessions[index]->destination, &length, &cm_rc);
      
      if(cm_rc != CM_OK) {
	*rc = AFTP_RC_PARAMETER_CHECK;
	return;
      }

      length = strlen(sessions[index]->mode_name);
      cmsmn(conv_id, sessions[index]->mode_name, &length, &cm_rc);

      if(cm_rc != CM_OK) {
	*rc = AFTP_RC_PARAMETER_CHECK;
	return;
      }

      length = strlen(sessions[index]->tp_name);
      cmstpn(conv_id, sessions[index]->tp_name, &length, &cm_rc);

      if(cm_rc != CM_OK) {
	*rc = AFTP_RC_PARAMETER_CHECK;
	return;
      }
    }
    
    cmallc(conv_id, &cm_rc);

    if(cm_rc != CM_OK) {
      *rc = AFTP_RC_COMM_FAIL_NO_RETRY;
      return;
    } else {
      asuiterecord_settype(sessions[index]->record, AFTP_TOWER_REQUEST);
      ftp_rc = send_asuiterecord(sessions[index]->conversation_ID,
				 sessions[index]->record);
      if(ftp_rc != AFTP_RC_OK) {
	*rc = ftp_rc;
	return;
      }
	    
      ftp_rc = get_asuiterecord(sessions[index]->conversation_ID,
				sessions[index]->record);
      if(ftp_rc != AFTP_RC_OK) {
	*rc = ftp_rc;
	return;
      }
      
      *rc = AFTP_RC_OK;
    }
  
  } else {
    *rc = AFTP_RC_HANDLE_NOT_VALID;
  }
  
}

AFTP_ENTRY
aftp_create(unsigned char AFTP_PTR rconnection_id,
	    AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  ftpsession * new_session;
  asuiterecord  * new_record;

  new_session = aftp_new(ftpsession,1);

  if(new_session != NULL) {
    sessions[ftp_handle_counter] = new_session;
    new_session->destination[0] = '\0';
    strcpy(new_session->mode_name, DEFAULT_MODE_NAME);
    strcpy(new_session->tp_name, DEFAULT_TP_NAME);
    strcpy(new_session->sys_info, OS_NAME);
    new_session->symbolic = 1;
    rconnection_id[0] = ftp_handle_counter;
    new_session->alloc_size = 0;
    new_session->block_size = 0;
    new_session->data_type  = AFTP_ASCII;
    new_session->date_mode  = AFTP_OLDDATE;
    new_session->record_format = AFTP_DEFAULT_RECORD_FORMAT;
    new_session->record_length = 0;
    new_session->security_type = AFTP_SECURITY_NONE;
    new_session->write_mode = AFTP_REPLACE;

    new_record = asuiterecord_new();
    if(new_record == NULL) {
      free(new_session);
      *rc = AFTP_RC_FAIL_FATAL;
    } else {
      new_session->record = new_record;
      ftp_handle_counter++;
      *rc = AFTP_RC_OK;      
    }

    new_session->current_directory = safe_get_cwd();
    if(new_session->current_directory == NULL) {
      free(new_session->record);
      free(new_session);
    }
      
  } else {
    *rc = AFTP_RC_FAIL_FATAL;
  }  
}

AFTP_ENTRY
aftp_create_dir(unsigned char AFTP_PTR connection_id,
		unsigned char AFTP_PTR directory,
		AFTP_LENGTH_TYPE length,
		AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  AFTP_RETURN_CODE_TYPE ftp_rc;
  unsigned char index = connection_id[0];
  unsigned int major_code;
  unsigned int receive_length;
  unsigned int curpos;
  unsigned int param_len;
  unsigned int key;

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL)) {
    asuiterecord_clear(sessions[index]->record);
    asuiterecord_settype(sessions[index]->record, AFTP_CREATE_DIR);
    asuiterecord_append(sessions[index]->record, AFTP_FILESPEC, directory, 
			length+1);
    ftp_rc = send_asuiterecord(sessions[index]->conversation_ID,
			       sessions[index]->record);

    if(ftp_rc != AFTP_RC_OK) {
      *rc = ftp_rc;
      return;
    }

    ftp_rc = get_asuiterecord(sessions[index]->conversation_ID,
			      sessions[index]->record);

    if(ftp_rc != AFTP_RC_OK) {
      *rc = ftp_rc;
      return;
    }
	
    receive_length = get_received_len(sessions[index]->record);
    major_code = get_major_code(sessions[index]->record);
    curpos = 2;
    *rc = AFTP_RC_OK;
    switch(major_code) 
      {
      case AFTP_SIMPLE_RESPONSE: 
	do {
	  param_len = get_param_len(sessions[index]->record, curpos);
	  key = get_key(sessions[index]->record, curpos);
	    
	  switch(key) 
	    {
	    case AFTP_STATUS_RETURN_CODE:
	      break;
	    case AFTP_STATUS_CATEGORY_INDEX:
	      *rc = AFTP_RC_FAIL_FATAL;
	      sessions[index]->msg_category = 
		get_cat_index(sessions[index]->record, curpos+3);
	      sessions[index]->msg_index = 
		get_cat_index(sessions[index]->record, curpos+5);
	      break;
	    case AFTP_STATUS_PRIMARY_MSG:
	      set_primary_msg(sessions[index], curpos);
	      break;
	    default:
	      ;
	    }
	  curpos += param_len;
	} while(curpos < length);
	break;
      default:
	;
      }
      
  } else {
    *rc = AFTP_RC_HANDLE_NOT_VALID;
  }
}

AFTP_ENTRY
aftp_delete(unsigned char AFTP_PTR connection_id,
	    unsigned char AFTP_PTR filename,
	    AFTP_LENGTH_TYPE length,
	    AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  AFTP_RETURN_CODE_TYPE ftp_rc;
  unsigned char index = connection_id[0];
  unsigned int major_code;
  unsigned int receive_length;
  unsigned int curpos;
  unsigned int param_len;
  unsigned int key;

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL)) {
    asuiterecord_clear(sessions[index]->record);
    asuiterecord_settype(sessions[index]->record, AFTP_DELETE);
    asuiterecord_append(sessions[index]->record, AFTP_FILESPEC, filename, 
			length+1);
    ftp_rc = send_asuiterecord(sessions[index]->conversation_ID,
			       sessions[index]->record);

    if(ftp_rc != AFTP_RC_OK) {
      *rc = ftp_rc;
      return;
    }

    ftp_rc = get_asuiterecord(sessions[index]->conversation_ID,
			      sessions[index]->record);

    if(ftp_rc != AFTP_RC_OK) {
      *rc = ftp_rc;
      return;
    }
	
    receive_length = get_received_len(sessions[index]->record);
    major_code = get_major_code(sessions[index]->record);
    curpos = 2;
    *rc = AFTP_RC_OK;
    switch(major_code) 
      {
      case AFTP_SIMPLE_RESPONSE: 
	do {
	  param_len = get_param_len(sessions[index]->record, curpos);
	  key = get_key(sessions[index]->record, curpos);
	  
	  switch(key) 
	    {
	    case AFTP_STATUS_RETURN_CODE:
	      break;
	    default:
	      ;
	    }
	  curpos += param_len;
	} while(curpos < receive_length);
	break;
      default:
	;
      }
    *rc = ftp_rc;
      
  } else {
    *rc = AFTP_RC_HANDLE_NOT_VALID;
  }

}

AFTP_ENTRY
aftp_destroy(unsigned char AFTP_PTR connection_id,
	     AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL)) {
    free(sessions[index]->record);
    free(sessions[index]->current_directory);
    free(sessions[index]);
    *rc = AFTP_RC_OK;

  } else {
    *rc = AFTP_RC_HANDLE_NOT_VALID;
  }
}

AFTP_ENTRY
aftp_dir_close(unsigned char AFTP_PTR connection_id,
	       AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  *rc = AFTP_RC_OK;
}

AFTP_ENTRY
aftp_dir_open(unsigned char AFTP_PTR connection_id,
	      unsigned char AFTP_PTR filespec,
	      AFTP_LENGTH_TYPE length,
	      AFTP_FILE_TYPE_TYPE file_type,
	      AFTP_INFO_LEVEL_TYPE info_level,
	      unsigned char AFTP_PTR path,
	      AFTP_LENGTH_TYPE path_buffer_length,
	      AFTP_LENGTH_TYPE AFTP_PTR path_returned_length,
	      AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  AFTP_RETURN_CODE_TYPE ftp_rc;
  unsigned char index = connection_id[0];
  unsigned int major_code;
  unsigned int receive_length;
  unsigned int curpos;
  unsigned int param_len;
  unsigned int key;

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL)) {
    asuiterecord_clear(sessions[index]->record);
    asuiterecord_settype(sessions[index]->record, AFTP_LIST_REQUEST);
    asuiterecord_append(sessions[index]->record, AFTP_FILESPEC, filespec, 
			length+1);
    asuiterecord_append_char(sessions[index]->record, AFTP_FILE_TYPE, 
			     (unsigned char)file_type);
    asuiterecord_append_char(sessions[index]->record, AFTP_INFO_LEVEL, 
			     (unsigned char)info_level);
    ftp_rc = send_asuiterecord(sessions[index]->conversation_ID,
			       sessions[index]->record);

    if(ftp_rc != AFTP_RC_OK) {
      *rc = ftp_rc;
      return;
    }

    ftp_rc = get_asuiterecord(sessions[index]->conversation_ID,
			      sessions[index]->record);

    if(ftp_rc != AFTP_RC_OK) {
      *rc = ftp_rc;
      return;
    }
	
    receive_length = get_received_len(sessions[index]->record);
    major_code = get_major_code(sessions[index]->record);
    curpos = 2;
    *rc = AFTP_RC_OK;
    switch(major_code) 
      {
      case AFTP_SIMPLE_RESPONSE: 
	do {
	  param_len = get_param_len(sessions[index]->record, curpos);
	  key = get_key(sessions[index]->record, curpos);
	    
	  switch(key) 
	    {
	    case AFTP_STATUS_RETURN_CODE:
	      sessions[index]->status = 
		sessions[index]->record->buffer[curpos+3];
	      break;
	    case AFTP_FILESPEC:
	      strncpy(path, &sessions[index]->record->buffer[curpos+3],
		      path_buffer_length);
	      *path_returned_length = strlen(path);
	      break;
	    case AFTP_STATUS_CATEGORY_INDEX:
	      *rc = AFTP_RC_FAIL_FATAL;
	      sessions[index]->msg_category = 
		get_cat_index(sessions[index]->record, curpos+3);
	      sessions[index]->msg_index = 
		get_cat_index(sessions[index]->record, curpos+5);
	      break;
	    case AFTP_STATUS_PRIMARY_MSG:
	      set_primary_msg(sessions[index], curpos);
	      break;
	    default:
	      ;
	    }
	  curpos += param_len;
	} while(curpos < receive_length);
	break;
      default:
	;
      }
      
  } else {
    *rc = AFTP_RC_FAIL_FATAL;
  }
  
}

AFTP_ENTRY
aftp_dir_read(unsigned char AFTP_PTR connection_id,
	      unsigned char AFTP_PTR dir_entry,
	      AFTP_LENGTH_TYPE dir_entry_size,
	      AFTP_LENGTH_TYPE AFTP_PTR returned_length,
	      AFTP_BOOLEAN_TYPE AFTP_PTR no_more_entries,
	      AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  AFTP_RETURN_CODE_TYPE ftp_rc;
  unsigned char index = connection_id[0];
  unsigned int major_code;
  unsigned int receive_length;
  unsigned int curpos;
  unsigned int param_len;
  unsigned int key;

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL) ) {
    ftp_rc = get_asuiterecord(sessions[index]->conversation_ID,
			      sessions[index]->record);

    if(ftp_rc != AFTP_RC_OK) {
      *rc = ftp_rc;
      return;
    }
	
    receive_length = get_received_len(sessions[index]->record);
    major_code = get_major_code(sessions[index]->record);
    curpos = 2;
    *rc = AFTP_RC_OK;
    switch(major_code) 
      {
      case AFTP_LIST_ENTRY: 
	do {
	  param_len = get_param_len(sessions[index]->record, curpos);
	  key = get_key(sessions[index]->record, curpos);
	    
	  switch(key) 
	    {
	    case AFTP_STATUS_RETURN_CODE:
	      break;
	    case AFTP_FILESPEC:
	      strncpy(dir_entry, &sessions[index]->record->buffer[curpos+3],
		      dir_entry_size);
	      *returned_length = strlen(dir_entry);
	      *no_more_entries = 0;
	      break;
	    case AFTP_STATUS_CATEGORY_INDEX:
	      *rc = AFTP_RC_FAIL_FATAL;
	      sessions[index]->msg_category = 
		get_cat_index(sessions[index]->record, curpos+3);
	      sessions[index]->msg_index = 
		get_cat_index(sessions[index]->record, curpos+5);
	      break;
	    case AFTP_STATUS_PRIMARY_MSG:
	      set_primary_msg(sessions[index], curpos);
	      break;
	    default:
	      ;
	    }
	  curpos += param_len;
	} while(curpos < receive_length);
	break;
      case AFTP_LIST_COMPLETE:
	*no_more_entries = 1;
      default:
	;
      }
      
  } else {
    *rc = AFTP_RC_HANDLE_NOT_VALID;
  }

}

AFTP_ENTRY
aftp_extract_allocation_size(unsigned char AFTP_PTR connection_id,
			     AFTP_ALLOCATION_SIZE_TYPE AFTP_PTR allocation_size,
			     AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  *allocation_size = sessions[index]->alloc_size;
  *rc = AFTP_RC_OK;
}

AFTP_ENTRY
aftp_extract_block_size(unsigned char AFTP_PTR connection_id,
			AFTP_BLOCK_SIZE_TYPE AFTP_PTR block_size,
			AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  *block_size = sessions[index]->block_size;
  *rc = AFTP_RC_OK;
}

AFTP_ENTRY
aftp_extract_data_type(unsigned char AFTP_PTR connection_id,
		       AFTP_DATA_TYPE_TYPE AFTP_PTR data_type,
		       AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  *data_type = sessions[index]->data_type;

  *rc = AFTP_RC_OK;
}

AFTP_ENTRY
aftp_extract_date_mode(unsigned char AFTP_PTR connection_id,
		       AFTP_DATE_MODE_TYPE AFTP_PTR date_mode,
		       AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  *date_mode = sessions[index]->date_mode;
  *rc = AFTP_RC_OK;
}

AFTP_ENTRY
aftp_extract_destination(unsigned char AFTP_PTR connection_id,
			 unsigned char AFTP_PTR destination,
			 AFTP_LENGTH_TYPE destination_size,
			 AFTP_LENGTH_TYPE AFTP_PTR returned_length,
			 AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  int len = strlen(sessions[index]->destination);

  if(len > 0) {
    if(len <= destination_size) {
      strncpy(destination, sessions[index]->destination, destination_size);
      *returned_length = strlen(destination);
      *rc = AFTP_RC_OK;
    } else {
      *rc = AFTP_RC_PARAMETER_CHECK;
    }
  } else {
    *rc = AFTP_RC_STATE_CHECK;
  }
}

AFTP_ENTRY
aftp_extract_mode_name(unsigned char AFTP_PTR connection_id,
		       unsigned char AFTP_PTR mode_name,
		       AFTP_LENGTH_TYPE mode_name_size,
		       AFTP_LENGTH_TYPE AFTP_PTR returned_length,
		       AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  int len = strlen(sessions[index]->mode_name);

  if(len > 0) {
    if(len <= mode_name_size) {
      strncpy(mode_name, sessions[index]->mode_name, mode_name_size);
      *returned_length = strlen(mode_name);
      *rc = AFTP_RC_OK;
    } else {
      *rc = AFTP_RC_PARAMETER_CHECK;
    }
  } else {
    *rc = AFTP_RC_STATE_CHECK;
  }

}

AFTP_ENTRY
aftp_extract_partner_LU_name(unsigned char AFTP_PTR connection_id,
			     unsigned char AFTP_PTR partner_LU_name,
			     AFTP_LENGTH_TYPE partner_LU_name_size,
			     AFTP_LENGTH_TYPE AFTP_PTR returned_length,
			     AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  int len = strlen(sessions[index]->destination);

  if(len > 0) {
    if(len <= partner_LU_name_size) {
      strncpy(partner_LU_name, sessions[index]->destination, partner_LU_name_size);
      *returned_length = strlen(partner_LU_name);
      *rc = AFTP_RC_OK;
    } else {
      *rc = AFTP_RC_PARAMETER_CHECK;
    }
  } else {
    *rc = AFTP_RC_STATE_CHECK;
  }

}

AFTP_ENTRY
aftp_extract_password(unsigned char AFTP_PTR connection_id,
		      unsigned char AFTP_PTR password,
		      AFTP_LENGTH_TYPE password_size,
		      AFTP_LENGTH_TYPE AFTP_PTR returned_length,
		      AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  int len = strlen(sessions[index]->password);

  if(len > 0) {
    if(len <= password_size) {
      strncpy(password, sessions[index]->password, password_size);
      *returned_length = strlen(password);
      *rc = AFTP_RC_OK;
    } else {
      *rc = AFTP_RC_PARAMETER_CHECK;
    }
  } else {
    *rc = AFTP_RC_STATE_CHECK;
  }

}

AFTP_ENTRY
aftp_extract_record_format(unsigned char AFTP_PTR connection_id,
			   AFTP_RECORD_FORMAT_TYPE AFTP_PTR record_format,
			   AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  *record_format = sessions[index]->record_format;
  *rc = AFTP_RC_OK;
  
}

AFTP_ENTRY
aftp_extract_record_length(unsigned char AFTP_PTR connection_id,
			   AFTP_RECORD_LENGTH_TYPE AFTP_PTR record_length,
			   AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  *record_length = sessions[index]->record_length;
  *rc = AFTP_RC_OK;

}

AFTP_ENTRY
aftp_extract_security_type(unsigned char AFTP_PTR connection_id,
			   AFTP_SECURITY_TYPE AFTP_PTR security_type,
			   AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  *security_type = sessions[index]->security_type;
  *rc = AFTP_RC_OK;
}

AFTP_ENTRY
aftp_extract_connection_info(unsigned char AFTP_PTR connection_id,
			     unsigned long AFTP_PTR connection_info,
			     AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  *rc = AFTP_RC_FAIL_FATAL;
}

AFTP_ENTRY
aftp_extract_tp_name(unsigned char AFTP_PTR connection_id,
		     unsigned char AFTP_PTR tp_name,
		     AFTP_LENGTH_TYPE tp_name_size,
		     AFTP_LENGTH_TYPE AFTP_PTR returned_length,
		     AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  int len = strlen(sessions[index]->tp_name);

  if(len > 0) {
    if(len <= tp_name_size) {
      strncpy(tp_name, sessions[index]->tp_name, tp_name_size);
      *returned_length = strlen(tp_name);
      *rc = AFTP_RC_OK;
    } else {
      *rc = AFTP_RC_PARAMETER_CHECK;
    }
  } else {
    *rc = AFTP_RC_STATE_CHECK;
  }

}

AFTP_ENTRY
aftp_extract_userid(unsigned char AFTP_PTR connection_id,
		    unsigned char AFTP_PTR userid,
		    AFTP_LENGTH_TYPE userid_size,
		    AFTP_LENGTH_TYPE AFTP_PTR returned_length,
		    AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  int len = strlen(sessions[index]->userid);

  if(len > 0) {
    if(len <= userid_size) {
      strncpy(userid, sessions[index]->userid, userid_size);
      *returned_length = strlen(userid);
      *rc = AFTP_RC_OK;
    } else {
      *rc = AFTP_RC_PARAMETER_CHECK;
    }
  } else {
    *rc = AFTP_RC_STATE_CHECK;
  }

}

AFTP_ENTRY
aftp_extract_write_mode(unsigned char AFTP_PTR connection_id,
			AFTP_WRITE_MODE_TYPE AFTP_PTR write_mode,
			AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  *write_mode = sessions[index]->write_mode;
  *rc = AFTP_RC_OK;

}

AFTP_ENTRY
aftp_get_data_type_string(AFTP_DATA_TYPE_TYPE data_type,
			  unsigned char AFTP_PTR data_type_string,
			  AFTP_LENGTH_TYPE data_type_size,
			  AFTP_LENGTH_TYPE AFTP_PTR returned_length,
			  AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{

  codemap map[] = { 
    {AFTP_ASCII, "ASCII"},
    {AFTP_BINARY, "BINARY"},
    {-1, ""}
  };

  get_code_string(map, data_type, data_type_string, data_type_size, returned_length, rc);

}

AFTP_ENTRY
aftp_get_date_mode_string(AFTP_DATE_MODE_TYPE date_mode,
			  unsigned char AFTP_PTR date_mode_string,
			  AFTP_LENGTH_TYPE date_mode_size,
			  AFTP_LENGTH_TYPE AFTP_PTR returned_length,
			  AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{

  codemap map[] = {
    {AFTP_OLDDATE, "OLD"},
    {AFTP_NEWDATE, "NEW"},
    {-1, ""}
  };

  get_code_string(map, date_mode, date_mode_string, date_mode_size, returned_length, rc);

}

AFTP_ENTRY
aftp_get_record_format_string(AFTP_RECORD_FORMAT_TYPE record_format,
			      unsigned char AFTP_PTR record_format_string,
			      AFTP_LENGTH_TYPE record_format_size,
			      AFTP_LENGTH_TYPE AFTP_PTR returned_length,
			      AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{

  codemap map[] = {
    {AFTP_DEFAULT_RECORD_FORMAT, "Default"},
    {AFTP_V                    , "V"},
    {AFTP_VA                   , "VA"},
    {AFTP_VB                   , "VB"},
    {AFTP_VM                   , "VM"},
    {AFTP_VS                   , "VS"},
    {AFTP_VBA                  , "VBA"},
    {AFTP_VBM                  , "VBM"},
    {AFTP_VBS                  , "VBS"},
    {AFTP_VSA                  , "VSA"},
    {AFTP_VSM                  , "VSM"},
    {AFTP_VBSA                 , "VBSA"},
    {AFTP_VBSM                 , "VBSM"},
    {AFTP_F                    , "F"},
    {AFTP_FA                   , "FA"},
    {AFTP_FB                   , "FB"},
    {AFTP_FM                   , "FM"},
    {AFTP_FBA                  , "FBA"},
    {AFTP_FBM                  , "FBM"},
    {AFTP_FBS                  , "FBS"},
    {AFTP_FBSM                 , "FBSM"},
    {AFTP_FBSA                 , "FBSA"},
    {AFTP_U                    , "U"},
    {AFTP_UA                   , "UA"},
    {AFTP_UM                   , "UM"},
    {-1, ""}
  };

  get_code_string(map, record_format, record_format_string, record_format_size, 
		  returned_length, rc);
  
}

AFTP_ENTRY
aftp_get_write_mode_string(AFTP_WRITE_MODE_TYPE write_mode,
			   unsigned char AFTP_PTR write_mode_string,
			   AFTP_LENGTH_TYPE write_mode_size,
			   AFTP_LENGTH_TYPE AFTP_PTR returned_length,
			   AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  codemap map[] = {
    {AFTP_REPLACE    , "REPLACE"},    
    {AFTP_APPEND     , "APPEND"},     
    {AFTP_NOREPLACE  , "NOREPLACE"},  
    {AFTP_STOREUNIQUE, "STOREUNIQUE"},
    {-1, ""}
  };

  get_code_string(map, write_mode, write_mode_string, write_mode_size, 
		  returned_length, rc);

}

AFTP_ENTRY
aftp_load_ini_file(unsigned char AFTP_PTR filename,
		   AFTP_LENGTH_TYPE filename_size,
		   unsigned char AFTP_PTR program_path,
		   AFTP_LENGTH_TYPE path_size,
		   unsigned char AFTP_PTR error_string,
		   AFTP_LENGTH_TYPE error_string_size,
		   AFTP_LENGTH_TYPE AFTP_PTR returned_length,
		   AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  *rc = AFTP_RC_PROGRAM_INTERNAL_ERROR;
}

AFTP_ENTRY
aftp_local_change_dir(unsigned char AFTP_PTR connection_id,
		      unsigned char AFTP_PTR directory,
		      AFTP_LENGTH_TYPE length,
		      AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];
  char * newbuf;

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL) ) {
    if(chdir(directory) == 0) {
      newbuf = getcwd(NULL,0);
      if(newbuf != NULL) {
	free(sessions[index]->current_directory);
	sessions[index]->current_directory = newbuf;
	*rc = AFTP_RC_OK;
      } else {
	*rc = AFTP_RC_FAIL_FATAL;
      }
    }
  } else {
    *rc = AFTP_RC_HANDLE_NOT_VALID;
  }
  
}

AFTP_ENTRY
aftp_local_dir_close(unsigned char AFTP_PTR connection_id,
		     AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  *rc = AFTP_RC_PROGRAM_INTERNAL_ERROR;
} 

AFTP_ENTRY
aftp_local_dir_open(unsigned char AFTP_PTR connection_id,
		    unsigned char AFTP_PTR filespec,
		    AFTP_LENGTH_TYPE length,
		    AFTP_FILE_TYPE_TYPE file_type,
		    AFTP_INFO_LEVEL_TYPE info_level,
		    unsigned char AFTP_PTR path,
		    AFTP_LENGTH_TYPE path_buffer_length,
		    AFTP_LENGTH_TYPE AFTP_PTR path_returned_length,
		    AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  *rc = AFTP_RC_PROGRAM_INTERNAL_ERROR;
}

AFTP_ENTRY
aftp_local_dir_read(unsigned char AFTP_PTR connection_id,
		    unsigned char AFTP_PTR dir_entry,
		    AFTP_LENGTH_TYPE dir_entry_size,
		    AFTP_LENGTH_TYPE AFTP_PTR returned_length,
		    AFTP_BOOLEAN_TYPE AFTP_PTR no_more_entries,
		    AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  *rc = AFTP_RC_PROGRAM_INTERNAL_ERROR;
}

AFTP_ENTRY
aftp_local_query_current_dir(unsigned char AFTP_PTR connection_id,
			     unsigned char AFTP_PTR directory,
			     AFTP_LENGTH_TYPE directory_size,
			     AFTP_LENGTH_TYPE AFTP_PTR returned_length,
			     AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL) ) {
    strncpy(directory, sessions[index]->current_directory, directory_size);
    *returned_length = strlen(directory);
    *rc = AFTP_RC_OK;
  } else {
    *rc = AFTP_RC_HANDLE_NOT_VALID;
  }
}

AFTP_ENTRY
aftp_query_bytes_transferred(unsigned char AFTP_PTR connection_id,
			     AFTP_LENGTH_TYPE AFTP_PTR bytes_transferred,
			     AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  *rc = AFTP_RC_PROGRAM_INTERNAL_ERROR;
}

AFTP_ENTRY
aftp_query_current_dir(unsigned char AFTP_PTR connection_id,
		       unsigned char AFTP_PTR directory,
		       AFTP_LENGTH_TYPE directory_size,
		       AFTP_LENGTH_TYPE AFTP_PTR returned_length,
		       AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  AFTP_RETURN_CODE_TYPE ftp_rc;
  unsigned int major_code;
  unsigned int length;
  unsigned int curpos;
  unsigned int param_len;
  unsigned int key;
  unsigned char index = connection_id[0];


  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL) ) {

    asuiterecord_clear(sessions[index]->record);
    asuiterecord_settype(sessions[index]->record, AFTP_QUERY_DIR);
    ftp_rc = send_asuiterecord(sessions[index]->conversation_ID,
			       sessions[index]->record);


    if(ftp_rc != AFTP_RC_OK) {
      *rc = ftp_rc;
      return;
    } 

    ftp_rc = get_asuiterecord(sessions[index]->conversation_ID,
			      sessions[index]->record);
  
    if(ftp_rc != AFTP_RC_OK) {
      *rc = ftp_rc;
      return;
    }

    length = get_received_len(sessions[index]->record);
    major_code = get_major_code(sessions[index]->record);
    curpos = 2;
    *rc = AFTP_RC_OK;
    switch(major_code) 
      {
      case AFTP_SIMPLE_RESPONSE: 
	do {
	  param_len = get_param_len(sessions[index]->record, curpos);
	  key = get_key(sessions[index]->record, curpos);
	  
	  switch(key) 
	    {
	    case AFTP_STATUS_RETURN_CODE:
	      break;
	    case AFTP_FILESPEC:
	      strncpy(directory, 
		      &sessions[index]->record->buffer[curpos+3], 
		      directory_size);
	      *returned_length = strlen(directory);
	      break;
	    default:
	      ;
	    }
	  curpos += param_len;
	} while(curpos < length);
	break;
      default:
	;
      }
    *rc = ftp_rc;
  } else {
    *rc = AFTP_RC_HANDLE_NOT_VALID;
  }
} 

AFTP_ENTRY
aftp_query_local_version(AFTP_VERSION_TYPE AFTP_PTR major_version,
			 AFTP_VERSION_TYPE AFTP_PTR minor_version,
			 AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  *major_version = MAJOR_VERSION;
  *minor_version = MINOR_VERSION;
  *rc = AFTP_RC_OK;
}

AFTP_ENTRY
aftp_query_local_system_info(unsigned char AFTP_PTR connection_id,
			     unsigned char AFTP_PTR system_info,
			     AFTP_LENGTH_TYPE system_info_size,
			     AFTP_LENGTH_TYPE AFTP_PTR returned_length,
			     AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];  

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL) ) {  
    strncpy(system_info, sessions[index]->sys_info, system_info_size);
    *returned_length = strlen(system_info);
  } else {
    *rc = AFTP_RC_HANDLE_NOT_VALID;
  }
}

AFTP_ENTRY
aftp_query_system_info(unsigned char AFTP_PTR connection_id,
		       unsigned char AFTP_PTR system_info,
		       AFTP_LENGTH_TYPE system_info_size,
		       AFTP_LENGTH_TYPE AFTP_PTR returned_length,
		       AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  AFTP_RETURN_CODE_TYPE ftp_rc;
  unsigned int major_code;
  unsigned int length;
  unsigned int curpos;
  unsigned int param_len;
  unsigned int key;

  unsigned char index = connection_id[0];

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL) ) {

    asuiterecord_clear(sessions[index]->record);
    asuiterecord_settype(sessions[index]->record, AFTP_QUERY_SYSTEM);
    ftp_rc = send_asuiterecord(sessions[index]->conversation_ID, 
			       sessions[index]->record);


    if(ftp_rc != AFTP_RC_OK) {
      *rc = ftp_rc;
      return;
    } 

    ftp_rc = get_asuiterecord(sessions[index]->conversation_ID,
			      sessions[index]->record);
  
    if(ftp_rc != AFTP_RC_OK) {
      *rc = ftp_rc;
      return;
    }

    length = get_received_len(sessions[index]->record);
    major_code = get_major_code(sessions[index]->record);
    curpos = 2;
    *rc = AFTP_RC_OK;
    switch(major_code) 
      {
      case AFTP_SIMPLE_RESPONSE: 
	do {
	  param_len = get_param_len(sessions[index]->record, curpos);
	  key = get_key(sessions[index]->record, curpos);

	  switch(key) 
	    {
	    case AFTP_STATUS_RETURN_CODE:
	      break;
	    case AFTP_SYSTEM_INFO:
	      strncpy(system_info, 
		      &sessions[index]->record->buffer[curpos+3], 
		      system_info_size);
	      *returned_length = strlen(system_info);
	      break;
	    default:
	      ;
	    }
	  curpos += param_len;
	} while(curpos < length);
	break;
      default:
	;
      }
    *rc = ftp_rc;
  } else {
    *rc = AFTP_RC_HANDLE_NOT_VALID;
  }
}

AFTP_ENTRY
aftp_receive_file(unsigned char AFTP_PTR connection_id,
		  unsigned char AFTP_PTR local_file,
		  AFTP_LENGTH_TYPE local_file_length,
		  unsigned char AFTP_PTR remote_file,
		  AFTP_LENGTH_TYPE remote_file_length,
		  AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  AFTP_RETURN_CODE_TYPE ftp_rc;
  unsigned char index = connection_id[0];
  unsigned int major_code;
  unsigned int receive_length;
  unsigned int curpos;
  unsigned int param_len;
  unsigned int key;
  int dest_file;
  int bytes_written;
  int more_data;

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL) ) {

    dest_file = open(local_file, O_CREAT | O_WRONLY);

    if(dest_file < 0) {
      *rc = AFTP_RC_FAIL_FATAL;
      return;
    }

    asuiterecord_clear(sessions[index]->record);
    asuiterecord_settype(sessions[index]->record, AFTP_REQUEST_FILE);
    asuiterecord_append(sessions[index]->record, AFTP_FILESPEC, remote_file, 
			remote_file_length+1);
    asuiterecord_append(sessions[index]->record, AFTP_NEW_FILESPEC, 
			local_file, local_file_length+1);
    asuiterecord_append_char(sessions[index]->record, AFTP_DATA_TYPE, 
			     sessions[index]->data_type);
    if(sessions[index]->block_size > 0) {
      asuiterecord_append_twobyte(sessions[index]->record, AFTP_BLOCK_SIZE,
				  sessions[index]->block_size);
    }
    ftp_rc = send_asuiterecord(sessions[index]->conversation_ID,
			       sessions[index]->record);
    
    if(ftp_rc != AFTP_RC_OK) {
      *rc = ftp_rc;
      return;
    }

    more_data = 1;
    do {
      asuiterecord_clear(sessions[index]->record);
      ftp_rc = get_asuiterecord(sessions[index]->conversation_ID,
				sessions[index]->record);
      
      if(ftp_rc != AFTP_RC_OK) {
	*rc = ftp_rc;
	return;
      }
	
      receive_length = get_received_len(sessions[index]->record);
      major_code = get_major_code(sessions[index]->record);
      curpos = 2;
      *rc = AFTP_RC_OK;
      switch(major_code) 
	{
	case AFTP_SIMPLE_RESPONSE: 
	  do {
	    param_len = get_param_len(sessions[index]->record, curpos);
	    key = get_key(sessions[index]->record, curpos);
	    
	    switch(key) 
	      {
	      case AFTP_STATUS_RETURN_CODE:
		break;
	      default:
		*rc = AFTP_RC_FAIL_INPUT_ERROR; 
		return;
	      }
	    curpos += param_len;
	  } while(curpos < receive_length);
	  break;
	case AFTP_SEND_FILE:
	  do {
	    param_len = get_param_len(sessions[index]->record, curpos);
	    key = get_key(sessions[index]->record, curpos);
	    
	    switch(key) 
	      {
	      case AFTP_STATUS_RETURN_CODE:
		  
		break;
	      default:
		;
	      }
	    curpos += param_len;
	  } while(curpos < receive_length);
	  break;
	case AFTP_FILE_DATA:

	  bytes_written = write(dest_file, 
				&sessions[index]->record->buffer[2],
				receive_length-2);

	  break;
	case AFTP_FILE_COMPLETE:
	  more_data = 0;
	  close(dest_file);

	default:
	  ;
	}
      *rc = ftp_rc;
    } while(more_data);

  } else {
    *rc = AFTP_RC_FAIL_FATAL;
  }

}

AFTP_ENTRY
aftp_remove_dir(unsigned char AFTP_PTR connection_id,
		unsigned char AFTP_PTR directory,
		AFTP_LENGTH_TYPE length,
		AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  AFTP_RETURN_CODE_TYPE ftp_rc;
  unsigned char index = connection_id[0];
  unsigned int major_code;
  unsigned int receive_length;
  unsigned int curpos;
  unsigned int param_len;
  unsigned int key;

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL) ) {
    asuiterecord_clear(sessions[index]->record);
    asuiterecord_settype(sessions[index]->record, AFTP_REMOVE_DIR);
    asuiterecord_append(sessions[index]->record, AFTP_FILESPEC, directory, 
			length+1);
    ftp_rc = send_asuiterecord(sessions[index]->conversation_ID,
			       sessions[index]->record);

    if(ftp_rc != AFTP_RC_OK) {
      *rc = ftp_rc;
      return;
    }

    ftp_rc = get_asuiterecord(sessions[index]->conversation_ID,
			      sessions[index]->record);

    if(ftp_rc != AFTP_RC_OK) {
      *rc = ftp_rc;
      return;
    }
	
    receive_length = get_received_len(sessions[index]->record);
    major_code = get_major_code(sessions[index]->record);
    curpos = 2;
    *rc = AFTP_RC_OK;
    switch(major_code) 
      {
      case AFTP_SIMPLE_RESPONSE: 
	do {
	  param_len = get_param_len(sessions[index]->record, curpos);
	  key = get_key(sessions[index]->record, curpos);
	    
	  switch(key) 
	    {
	    case AFTP_STATUS_RETURN_CODE:
	      sessions[index]->status = 
		sessions[index]->record->buffer[curpos+3];
	      break;
	    case AFTP_STATUS_CATEGORY_INDEX:
	      *rc = AFTP_RC_FAIL_FATAL;
	      sessions[index]->msg_category = 
		get_cat_index(sessions[index]->record, curpos+3);
	      sessions[index]->msg_index = 
		get_cat_index(sessions[index]->record, curpos+5);
	      break;
	    case AFTP_STATUS_PRIMARY_MSG:
	      set_primary_msg(sessions[index], curpos);
	      break;
	    default:
	      ;
	    }
	  curpos += param_len;
	} while(curpos < receive_length);
	break;
      default:
	;
      }
      
  } else {
    *rc = AFTP_RC_FAIL_FATAL;
  }

}

AFTP_ENTRY
aftp_rename(unsigned char AFTP_PTR connection_id,
	    unsigned char AFTP_PTR oldfile,
	    AFTP_LENGTH_TYPE oldlength,
	    unsigned char AFTP_PTR newfile,
	    AFTP_LENGTH_TYPE newlength,
	    AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  AFTP_RETURN_CODE_TYPE ftp_rc;
  unsigned char index = connection_id[0];
  unsigned int major_code;
  unsigned int receive_length;
  unsigned int curpos;
  unsigned int param_len;
  unsigned int key;

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL) ) {
    asuiterecord_clear(sessions[index]->record);
    asuiterecord_settype(sessions[index]->record, AFTP_RENAME);
    asuiterecord_append(sessions[index]->record, AFTP_FILESPEC, oldfile, 
			oldlength+1);
    asuiterecord_append(sessions[index]->record, AFTP_NEW_FILESPEC, newfile, 
			newlength+1);
    ftp_rc = send_asuiterecord(sessions[index]->conversation_ID,
			       sessions[index]->record);

    if(ftp_rc != AFTP_RC_OK) {
      *rc = ftp_rc;
      return;
    }

    ftp_rc = get_asuiterecord(sessions[index]->conversation_ID,
			      sessions[index]->record);

    if(ftp_rc != AFTP_RC_OK) {
      *rc = ftp_rc;
      return;
    }
	
    receive_length = get_received_len(sessions[index]->record);
    major_code = get_major_code(sessions[index]->record);
    curpos = 2;
    *rc = AFTP_RC_OK;
    switch(major_code) 
      {
      case AFTP_SIMPLE_RESPONSE: 
	do {
	  param_len = get_param_len(sessions[index]->record, curpos);
	  key = get_key(sessions[index]->record, curpos);
	  
	  switch(key) 
	    {
	    case AFTP_STATUS_RETURN_CODE:
	      sessions[index]->status = 
		sessions[index]->record->buffer[curpos+3];
	      break;
	    case AFTP_STATUS_CATEGORY_INDEX:
	      *rc = AFTP_RC_FAIL_FATAL;
	      sessions[index]->msg_category = 
		get_cat_index(sessions[index]->record, curpos+3);
	      sessions[index]->msg_index = 
		get_cat_index(sessions[index]->record, curpos+5);
	      break;
	    case AFTP_STATUS_PRIMARY_MSG:
	      set_primary_msg(sessions[index], curpos);
	      break;
	    default:
	      ;
	    }
	  curpos += param_len;
	} while(curpos < receive_length);
	break;
      default:
	;
      }
    
  } else {
    *rc = AFTP_RC_FAIL_FATAL;
  }
  
}

AFTP_ENTRY
aftp_send_file(unsigned char AFTP_PTR connection_id,
	       unsigned char AFTP_PTR local_file,
	       AFTP_LENGTH_TYPE local_file_length,
	       unsigned char AFTP_PTR remote_file,
	       AFTP_LENGTH_TYPE remote_file_length,
	       AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  AFTP_RETURN_CODE_TYPE ftp_rc;
  unsigned char index = connection_id[0];
  unsigned int major_code;
  unsigned int receive_length;
  unsigned int curpos;
  unsigned int param_len;
  unsigned int key;
  int source_file;
  int bytes_read;
  asuiterecord * record;

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL)) {


    source_file = open(local_file, O_RDONLY);

    if(source_file < 0) {
      *rc = AFTP_RC_FAIL_FATAL;
      return;
    }

    asuiterecord_clear(sessions[index]->record);
    asuiterecord_settype(sessions[index]->record, AFTP_SEND_FILE);
    asuiterecord_append(sessions[index]->record, AFTP_FILESPEC, local_file, 
			local_file_length+1);
    asuiterecord_append(sessions[index]->record, AFTP_NEW_FILESPEC, 
			remote_file, remote_file_length+1);
    asuiterecord_append_char(sessions[index]->record, AFTP_DATA_TYPE, 
			     sessions[index]->data_type);
    if(sessions[index]->alloc_size > 0) {
      asuiterecord_append_fourbyte(sessions[index]->record, 
				   AFTP_ALLOCATION_SIZE,
				   sessions[index]->alloc_size);
    }
    if(sessions[index]->block_size > 0) {
      asuiterecord_append_twobyte(sessions[index]->record, AFTP_BLOCK_SIZE,
				  sessions[index]->block_size);
    }
    if(sessions[index]->record_length > 0) {
      asuiterecord_append_twobyte(sessions[index]->record, AFTP_RECORD_LENGTH,
				  sessions[index]->record_length);
    }
    asuiterecord_append_char(sessions[index]->record, AFTP_DATE_MODE, 
			     sessions[index]->date_mode);
    ftp_rc = send_asuiterecord(sessions[index]->conversation_ID,
			       sessions[index]->record);

    if(ftp_rc != AFTP_RC_OK) {
      *rc = ftp_rc;
      return;
    }

    asuiterecord_clear(sessions[index]->record);
    asuiterecord_settype(sessions[index]->record, AFTP_FILE_DATA);

    record = sessions[index]->record;

    bytes_read = read(source_file, 
		      &record->buffer[record->position],
		      MAX_BUFFER_SIZE - record->position);

    record->position = record->position + bytes_read;
      
    while(bytes_read > 0) {

      ftp_rc = send_asuiterecord(sessions[index]->conversation_ID,
				 sessions[index]->record);

      if(ftp_rc != AFTP_RC_OK) {
	*rc = ftp_rc;
	return;
      }

      asuiterecord_clear(sessions[index]->record);

      bytes_read = read(source_file, 
			&record->buffer[record->position],
			MAX_BUFFER_SIZE - record->position);


      record->position = record->position + bytes_read;
    }

    close(source_file);

    asuiterecord_settype(sessions[index]->record, AFTP_FILE_COMPLETE);
    asuiterecord_clear(sessions[index]->record);

    ftp_rc = send_asuiterecord(sessions[index]->conversation_ID,
			       sessions[index]->record);

    if(ftp_rc != AFTP_RC_OK) {
      *rc = ftp_rc;
      return;
    }

    ftp_rc = get_asuiterecord(sessions[index]->conversation_ID,
			      sessions[index]->record);

    if(ftp_rc != AFTP_RC_OK) {
      *rc = ftp_rc;
      return;
    }
	
    receive_length = get_received_len(sessions[index]->record);
    major_code = get_major_code(sessions[index]->record);
    curpos = 2;
    *rc = AFTP_RC_OK;
    switch(major_code) 
      {
      case AFTP_SIMPLE_RESPONSE: 
	do {
	  param_len = get_param_len(sessions[index]->record, curpos);
	  key = get_key(sessions[index]->record, curpos);
	    
	  switch(key) 
	    {
	    case AFTP_STATUS_RETURN_CODE:
	      
	      break;
	    default:
	      ;
	    }
	  curpos += param_len;
	} while(curpos < receive_length);
	break;
      default:
	;
      }
    *rc = ftp_rc;
   
  } else {
    *rc = AFTP_RC_FAIL_FATAL;
  }

}

AFTP_ENTRY
aftp_set_allocation_size(unsigned char AFTP_PTR connection_id,
			 AFTP_ALLOCATION_SIZE_TYPE allocation_size,
			 AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL) )  {
    *rc = AFTP_RC_OK;
      sessions[index]->alloc_size = allocation_size;
  } else {
    *rc = AFTP_RC_HANDLE_NOT_VALID;
  }
  
} 

AFTP_ENTRY
aftp_set_block_size(unsigned char AFTP_PTR connection_id,
		    AFTP_BLOCK_SIZE_TYPE block_size,
		    AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL) )  {
    *rc = AFTP_RC_OK;
    sessions[index]->block_size = block_size;
  } else {
    *rc = AFTP_RC_HANDLE_NOT_VALID;
  }
  
}

AFTP_ENTRY
aftp_set_data_type(unsigned char AFTP_PTR connection_id,
		   AFTP_DATA_TYPE_TYPE data_type,
		   AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL) )  {
    sessions[index]->data_type = data_type;
    *rc = AFTP_RC_OK;
  } else {
    *rc = AFTP_RC_FAIL_FATAL;
  }

}

AFTP_ENTRY
aftp_set_connection_info(unsigned char AFTP_PTR connection_id,
			 unsigned long connection_info,
			 AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  *rc = AFTP_RC_PROGRAM_INTERNAL_ERROR;
}

AFTP_ENTRY
aftp_set_date_mode(unsigned char AFTP_PTR connection_id,
		   AFTP_DATE_MODE_TYPE date_mode,
		   AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL) )  {
    *rc = AFTP_RC_OK;
    sessions[index]->date_mode = date_mode;
  } else {
    *rc = AFTP_RC_FAIL_FATAL;
  }
  
}

AFTP_ENTRY
aftp_set_destination(unsigned char AFTP_PTR connection_id,
		     unsigned char AFTP_PTR destination,
		     AFTP_LENGTH_TYPE length,
		     AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{

  unsigned char index = connection_id[0];

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL) ) {
    if(strchr(destination, '.')) {
      if(length <= CM_PLN_SIZE) {
	memcpy(sessions[index]->destination, destination, length);
	sessions[index]->destination[length] = '\0';
	sessions[index]->symbolic = 0;
	*rc = AFTP_RC_OK;
      } else {
	fprintf(stderr, "Partner lu name too long.\n");
	*rc = AFTP_RC_PARAMETER_CHECK;
      }
    } else {
      if(length <= CM_SDN_SIZE) {
	memset(sessions[index]->destination, ' ', CM_SDN_SIZE);
	memcpy(sessions[index]->destination, destination, length);
	sessions[index]->destination[CM_SDN_SIZE] = '\0';
	sessions[index]->symbolic = 1;
	*rc = AFTP_RC_OK;
      } else {
	fprintf(stderr, "Symbolic destination name too long.\n");
	*rc = AFTP_RC_PARAMETER_CHECK;
      }      
    }
  } else {
    *rc = AFTP_RC_HANDLE_NOT_VALID;
  }

} 

AFTP_ENTRY
aftp_set_trace_level(AFTP_TRACE_LEVEL_TYPE trace_level,
		     AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{

    global_trace_level = trace_level;
    *rc = AFTP_RC_OK;
}

AFTP_ENTRY
aftp_extract_trace_level(AFTP_TRACE_LEVEL_TYPE AFTP_PTR trace_level,
			 AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
    *trace_level = global_trace_level;
    *rc = AFTP_RC_OK;
}

AFTP_ENTRY
aftp_set_trace_filename(unsigned char AFTP_PTR filename,
			AFTP_LENGTH_TYPE filename_length,
			AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{

  if(filename_length <= MAX_FILE_NAME-1) {
    strncpy(global_trace_file, filename, MAX_FILE_NAME-1);
    *rc = AFTP_RC_OK;
  } else {
    *rc = AFTP_RC_PARAMETER_CHECK;
  }

}

AFTP_ENTRY
aftp_set_mode_name(unsigned char AFTP_PTR connection_id,
		   unsigned char AFTP_PTR mode_name,
		   AFTP_LENGTH_TYPE length,
		   AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];
  
  printf("mode_name_size = %d\n", strlen(mode_name));
  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL) )  {
    if(length <= AFTP_MODE_NAME_SIZE) {
      strcpy(sessions[index]->mode_name, mode_name);
      *rc = AFTP_RC_OK;
    } else {
      *rc = AFTP_RC_PARAMETER_CHECK;
    }
  } else {
    *rc = AFTP_RC_HANDLE_NOT_VALID;
  }

}

AFTP_ENTRY
aftp_set_password(unsigned char AFTP_PTR connection_id,
		  unsigned char AFTP_PTR password,
		  AFTP_LENGTH_TYPE length,
		  AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL) )  {
    if(length <= AFTP_PASSWORD_SIZE) {
      strcpy(sessions[index]->password, password);
      *rc = AFTP_RC_OK;
    } else {
      *rc = AFTP_RC_PARAMETER_CHECK;
    }
  } else {
    *rc = AFTP_RC_HANDLE_NOT_VALID;
  }

}

AFTP_ENTRY
aftp_set_record_format(unsigned char AFTP_PTR connection_id,
		       AFTP_RECORD_FORMAT_TYPE record_format,
		       AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL) )  {
    *rc = AFTP_RC_OK;
    sessions[index]->record_format = record_format;
  } else {
    *rc = AFTP_RC_HANDLE_NOT_VALID;
  }

}

AFTP_ENTRY
aftp_set_record_length(unsigned char AFTP_PTR connection_id,
		       AFTP_RECORD_LENGTH_TYPE record_length,
		       AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL) )  {
    *rc = AFTP_RC_OK;
    sessions[index]->record_length = record_length;
  } else {
    *rc = AFTP_RC_HANDLE_NOT_VALID;
  }
}

AFTP_ENTRY
aftp_set_security_type(unsigned char AFTP_PTR connection_id,
		       AFTP_SECURITY_TYPE security_type,
		       AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL) )  {
    sessions[index]->security_type = security_type;
  } else {
    *rc = AFTP_RC_HANDLE_NOT_VALID;
  }

}

AFTP_ENTRY
aftp_set_tp_name(unsigned char AFTP_PTR connection_id,
		 unsigned char AFTP_PTR tp_name,
		 AFTP_LENGTH_TYPE length,
		 AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  printf("set_tp %s\n", tp_name);
  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL) )  {
    if(length <= AFTP_TP_NAME_SIZE) {
      strcpy(sessions[index]->tp_name, tp_name);
      *rc = AFTP_RC_OK;
    } else {
      *rc = AFTP_RC_PARAMETER_CHECK;
    }
  } else {
    *rc = AFTP_RC_HANDLE_NOT_VALID;
  }

}

AFTP_ENTRY
aftp_set_userid(unsigned char AFTP_PTR connection_id,
		unsigned char AFTP_PTR userid,
		AFTP_LENGTH_TYPE length,
		AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL) )  {
    if(length <= AFTP_USERID_SIZE) {
      strcpy(sessions[index]->userid, userid);
      *rc = AFTP_RC_OK;
    } else {
      *rc = AFTP_RC_PARAMETER_CHECK;
    }
  } else {
    *rc = AFTP_RC_HANDLE_NOT_VALID;
  }

}

AFTP_ENTRY
aftp_set_write_mode(unsigned char AFTP_PTR connection_id,
		    AFTP_WRITE_MODE_TYPE write_mode,
		    AFTP_RETURN_CODE_TYPE AFTP_PTR rc)
{
  unsigned char index = connection_id[0];

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL) )  {
    sessions[index]->write_mode = write_mode;
  } else {
    *rc = AFTP_RC_HANDLE_NOT_VALID;
  }
}

AFTP_ENTRY
aftp_format_error(
    unsigned char AFTP_PTR                  connection_id,
    AFTP_DETAIL_LEVEL_TYPE                  detail_level,
    unsigned char AFTP_PTR                  error_str,
    AFTP_LENGTH_TYPE                        error_str_size,
    AFTP_LENGTH_TYPE AFTP_PTR               returned_length,
    AFTP_RETURN_CODE_TYPE AFTP_PTR          rc)
{
  unsigned char index = connection_id[0];  
  unsigned char codes[10];
  int total;

  if( (index < MAX_FTP_SESSIONS) && (sessions[index] != NULL) )  {
    total = 0;
    switch(detail_level) {
    case AFTP_DETAIL_RC:
      sprintf(codes, "%d%d", sessions[index]->msg_category,
	      sessions[index]->msg_index);
      total = strlen(codes);
      total = total + strlen(sessions[index]->primary_msg);

      strcpy(error_str, codes);
      strcat(error_str, sessions[index]->primary_msg);
      *returned_length = strlen(error_str);
    default:
      break;
    }
  } else {
    *rc = AFTP_RC_HANDLE_NOT_VALID;
  }
  
}















