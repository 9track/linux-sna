/* aftpd - APPC ftp server for Linux-SNA
 *
 * Author: Michael Madore <mmadore@turbolinux.com>
 *
 * Copyright (c) 1999-2002 by Jay Schulist <jschlst@linux-sna.org>
 *
 * This program can be redistributed or modified under the terms of the
 * GNU General Public License as published by the Free Software Foundation.
 * This program is distributed without any warranty or implied warranty
 * of merchantability or fitness for a particular purpose.
 *
 * See the GNU General Public License for more details.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "aftpd.h"

unsigned char conv_id[8];            
asuiterecord * record;
CM_INT32 returned_length;
unsigned int curpos;
int dest_file;

void
a_tower_request(void)
{
  CM_INT32 rc;

  asuiterecord_clear(record);
  asuiterecord_settype(record, AFTP_TOWER_RESPONSE);
  asuiterecord_append_twobyte(record, AFTP_TOWER_LIST, 0);

  rc = send_asuiterecord(conv_id, record);

}

void
a_query_system(void)
{
  CM_INT32 rc;
  unsigned char * system_string = "AFTPD version 0.01 for Linux-SNA\n";

  asuiterecord_clear(record);
  asuiterecord_settype(record, AFTP_SIMPLE_RESPONSE);
  asuiterecord_append_char(record, AFTP_STATUS_RETURN_CODE, 0);
  asuiterecord_append(record, AFTP_SYSTEM_INFO, system_string, 
		      strlen(system_string)+1);

  rc = send_asuiterecord(conv_id, record);

}

void
a_query_dir(void)
{
  CM_INT32 rc;
  char * curdir;
  
  curdir = safe_get_cwd();
  
  if(curdir != NULL) {
    asuiterecord_clear(record);
    asuiterecord_settype(record, AFTP_SIMPLE_RESPONSE);

    asuiterecord_append_char(record, AFTP_STATUS_RETURN_CODE, AFTP_RC_OK);
    asuiterecord_append(record, AFTP_FILESPEC, curdir, strlen(curdir)+1);
    rc = send_asuiterecord(conv_id, record);
    free(curdir);
  } else {
    send_simple_response(conv_id, AFTP_RC_PROGRAM_INTERNAL_ERROR, 
			 0, A_ERR_OUT_OF_MEMORY);
  }
}

GString *
get_long_format (const struct stat f)
{
  char modebuf[11];
  time_t when;
  time_t current_time;
  struct passwd *userinfo;
  struct group *groupinfo;
  GString * result;
  const char *fmt;
  struct tm *when_local;
  time_t s;
  char * newbuf;
  int maxbuf;

  mode_string (f.st_mode, modebuf);

  modebuf[10] = '\0';

  result = g_string_new(modebuf);

  when = f.st_ctime;

  current_time = time ((time_t *) 0);

  if (current_time > when + 6L * 30L * 24L * 60L * 60L	
      || current_time < when - 60L * 60L)     
    {
      fmt = "%b %e  %Y";
    }
  else
    {
      fmt = "%b %e %H:%M";
    }

  /* The space between the mode and the number of links is the POSIX
     "optional alternate access method flag". */

  g_string_sprintfa(result, " %3u ", (unsigned int) f.st_nlink);

  userinfo = getpwuid(f.st_uid); 
  
  if (userinfo) {
    g_string_sprintfa(result, "%-8.8s ", userinfo->pw_name);
  } else {
    g_string_sprintfa(result, "%-8u ", (unsigned int) f.st_uid);
  }
  
  groupinfo = getgrgid(f.st_gid);

  if (groupinfo) {
    g_string_sprintfa(result, "%-8.8s ", groupinfo->gr_name);
  } else {
    g_string_sprintfa(result, "%-8u ", (unsigned int) f.st_gid);
  }
  
  if (S_ISCHR (f.st_mode) || S_ISBLK (f.st_mode)) {
    g_string_sprintfa(result, "%3u, %3u ", (unsigned) major (f.st_rdev),
		      (unsigned) minor (f.st_rdev));
  } else {
    g_string_sprintfa(result, "%12ld ", f.st_size);
  }

  /* Use strftime rather than ctime, because the former can produce
     locale-dependent names for the weekday (% and month (%b).  */

  maxbuf = TIME_BUF_SIZE;

  if ((when_local = localtime (&when))) {
    newbuf = g_malloc(maxbuf);
    s = strftime(newbuf, maxbuf, fmt, when_local);

    while(!s) {
      newbuf = g_realloc(newbuf, maxbuf+TIME_BUF_SIZE);
      maxbuf += TIME_BUF_SIZE;
      s = strftime(newbuf,maxbuf, fmt, when_local);
    };

    g_string_sprintfa(result, " %s ", newbuf);

    free(newbuf);
  }

  return(result);

}

void
a_list_request()
{
  GSList * filelist;
  GSList * curelem;
  GString * longfile;
  unsigned int param_len;
  unsigned int key;
  char * filespec;
  char * filestring;
  AFTP_FILE_TYPE_TYPE filetype = AFTP_ALL_FILES;
  AFTP_INFO_LEVEL_TYPE infolevel;
  struct stat filestats;
  int isdir;
  int rc;
  char * directory;
  char * filename;

  longfile = NULL;
  
  do {
    param_len = get_param_len(record, curpos);
    key = get_key(record, curpos);
	    
    switch(key) 
      {
      case AFTP_FILESPEC:
	filespec = malloc(strlen(&record->buffer[curpos+3])+1);
	strncpy(filespec, &record->buffer[curpos+3], 
		strlen(&record->buffer[curpos+3])+1);
	break;
      case AFTP_FILE_TYPE:
	filetype = record->buffer[curpos+3];
	break;
      case AFTP_INFO_LEVEL:
	infolevel = record->buffer[curpos+3];
	break;
      default:
	syslog(LOG_INFO, "Unhandled/illegalkey %d\n", key);
	;
      }
    curpos += param_len;

  } while(curpos < returned_length);

  directory = g_dirname(filespec);
  filename = g_basename(filespec);

  if( !strcmp(directory, ".") ) {
    g_free(directory);
    directory = safe_get_cwd();
  }

  rc = lstat(filespec, &filestats);

  if(rc < 0) {
    rc = lstat(directory, &filestats);
    if(rc < 0) {
      send_simple_response(conv_id, AFTP_RC_FAIL_INPUT_ERROR, 0,
			   A_ERR_INVALID_PATH);
      g_free(directory);
      return;
    } else {
      if( S_ISDIR(filestats.st_mode) ) {
	rc = get_local_file_list(directory, filename, &filelist, 1);
      } else {
	send_simple_response(conv_id, AFTP_RC_FAIL_INPUT_ERROR, 0,
			     A_ERR_INVALID_PATH);
	g_free(directory);
	return;
      }
    }
  } else {
    if( S_ISDIR(filestats.st_mode) ) {
      rc = get_local_file_list(filespec, "*", &filelist, 1);
    } else {
      rc = get_local_file_list(directory, filename, &filelist, 1);
    }
  }

  if(filelist == NULL) {
    send_simple_response(conv_id, AFTP_RC_FAIL_INPUT_ERROR, 0,
			 A_ERR_NO_FILES_FOUND);
    g_free(directory);
    return;
  } else {

    asuiterecord_clear(record);
    asuiterecord_settype(record, AFTP_SIMPLE_RESPONSE);
  
    asuiterecord_append_char(record, AFTP_STATUS_RETURN_CODE, 0);
    asuiterecord_append(record, AFTP_FILESPEC, directory, strlen(directory)+1);

    send_asuiterecord(conv_id, record);

    curelem = filelist;
    
    while(curelem != NULL) {
      syslog(LOG_INFO, "data = %s\n", (char *)curelem->data);
      asuiterecord_clear(record);
      asuiterecord_settype(record, AFTP_LIST_ENTRY);
      stat(curelem->data, &filestats);
      isdir = S_ISDIR(filestats.st_mode);
    
      if(infolevel == AFTP_NATIVE_ATTRIBUTES) {
	longfile = get_long_format(filestats);
	g_string_sprintfa(longfile, " %s", (char *)curelem->data);
	filestring = longfile->str; 
      } else {
	filestring = curelem->data;
      }
    
      if(filetype == AFTP_FILE) {
	if(!isdir) {
	  asuiterecord_append(record, AFTP_FILESPEC, filestring,
			      strlen(filestring)+1);
	}
      } else if(filetype == AFTP_DIRECTORY) {
	if(isdir) {
	  asuiterecord_append(record, AFTP_FILESPEC, filestring,
			      strlen(filestring)+1);
	}
      } else {
	asuiterecord_append(record, AFTP_FILESPEC, filestring,
			    strlen(filestring)+1);
      }
      send_asuiterecord(conv_id, record);
      g_string_free(longfile, 1); 
      curelem = g_slist_next(curelem);
    }
    asuiterecord_clear(record);
    asuiterecord_settype(record, AFTP_LIST_COMPLETE);
    send_asuiterecord(conv_id, record);
  }    

  free_single_list(filelist); 
  g_free(directory);
}

void
a_change_dir()
{
  CM_INT32 rc;
  int chrc; 
  char * filespec;
  unsigned int param_len;
  unsigned int key;
  unsigned int errorcode;

  do {
    param_len = get_param_len(record, curpos);
    key = get_key(record, curpos);
	    
    switch(key) 
      {
      case AFTP_FILESPEC:
	filespec = malloc(strlen(&record->buffer[curpos+3])+1);
	strncpy(filespec, &record->buffer[curpos+3], 
		strlen(&record->buffer[curpos+3])+1);
	
	chrc = chdir(filespec);

	if(chrc < 0) {
	  switch(errno) {
	  case ENOENT:
	    errorcode = A_ERR_FILE_NOT_FOUND;
	      break;
	  default:
	    errorcode = A_ERR_ACTION_NOT_PERFORMED;
	  }
	  send_simple_response(conv_id, AFTP_RC_FAIL_INPUT_ERROR,
			       0, errorcode);
	  return;
	}
	break;

      default:
	syslog(LOG_INFO, "Unhandled/illegal key %d\n", key);
	;
      }
    curpos += param_len;
  } while(curpos < returned_length);

  asuiterecord_clear(record);
  asuiterecord_settype(record, AFTP_SIMPLE_RESPONSE);
  asuiterecord_append_char(record, AFTP_STATUS_RETURN_CODE, 0);

  rc = send_asuiterecord(conv_id, record);

}

void
a_request_file()
{
  char * filespec;
  char * newfilespec;
  char * filename;
  int source_file;
  unsigned int key;
  unsigned int param_len;
  CM_INT32 ftp_rc;
  int bytes_read;
  unsigned int data_type = AFTP_ASCII;
  int errorcode;

  do {
    param_len = get_param_len(record, curpos);
    key = get_key(record, curpos);
	    
    switch(key) 
      {
      case AFTP_FILESPEC:
	filespec = malloc(strlen(&record->buffer[curpos+3])+1); 
	if(filespec != NULL) {
	  strncpy(filespec, &record->buffer[curpos+3], 
		  strlen(&record->buffer[curpos+3])+1);
	} else {
	  send_simple_response(conv_id, ASUITE_RC_PROGRAM_INTERNAL_ERROR,
			       0, A_ERR_OUT_OF_MEMORY);
	  return;
	}
	break;
      case AFTP_NEW_FILESPEC:
	newfilespec = malloc(strlen(&record->buffer[curpos+3])+1);
	if(newfilespec != NULL) {
	  strncpy(newfilespec, &record->buffer[curpos+3],
		  strlen(&record->buffer[curpos+3])+1);
	} else {
	  send_simple_response(conv_id, ASUITE_RC_PROGRAM_INTERNAL_ERROR,
			       0, A_ERR_OUT_OF_MEMORY);
	  return;
	}
	break;
      case AFTP_DATA_TYPE:
	data_type = asuiterecord_get_char(record, curpos+3);
	break;
      case AFTP_WRITE_MODE:
	break;
      case AFTP_DATE_MODE:
	break;
      case AFTP_MOD_TIME:
	break;
      case AFTP_RECORD_LENGTH:
	break;
      case AFTP_RECORD_FORMAT:
	break;
      case AFTP_BLOCK_SIZE:
	break;
      case AFTP_ALLOCATION_SIZE:
	break;
      default:
	send_simple_response(conv_id, ASUITE_RC_PROGRAM_INTERNAL_ERROR,
			     0, A_ERR_UNKNOWN_PARAMETER);
	return;
	;
      }
    curpos += param_len;
  } while(curpos < returned_length);
  
  source_file = open(filespec, O_RDONLY);
  
  if(source_file < 0) {
    switch(errno) {
    case ENOENT:
      errorcode = A_ERR_FILE_NOT_FOUND;
      break;
    case EACCES:
      errorcode = A_ERR_ACCESS_DENIED;
      break;
    default:
      
    }
    send_simple_response(conv_id, AFTP_RC_FAIL_INPUT_ERROR, 0, 
			 errorcode);
    return;
  }

  filename = g_basename(filespec);

  asuiterecord_clear(record);
  asuiterecord_settype(record, AFTP_SEND_FILE);
  asuiterecord_append(record, AFTP_FILESPEC, filename, 
		      strlen(filespec)+1);

  asuiterecord_append_char(record, AFTP_DATA_TYPE, data_type);

  ftp_rc = send_asuiterecord(conv_id, record);

  if(ftp_rc != AFTP_RC_OK) {

  } 

  asuiterecord_clear(record);
  asuiterecord_settype(record, AFTP_FILE_DATA);

  bytes_read = read(source_file, 
		    &record->buffer[record->position],
		    MAX_BUFFER_SIZE - record->position);

  record->position = record->position + bytes_read;

  while(bytes_read > 0) {

    ftp_rc = send_asuiterecord(conv_id, record);
      
    if(ftp_rc != AFTP_RC_OK) {
      syslog(LOG_INFO, "ftp_rc = %lu", ftp_rc);
      exit(1);
    } 

    asuiterecord_clear(record);

    bytes_read = read(source_file, 
		      &record->buffer[record->position],
		      MAX_BUFFER_SIZE - record->position);

    if(bytes_read > 0) {
      record->position = record->position + bytes_read;
    }
  }

  close(source_file);

  asuiterecord_clear(record);
  asuiterecord_settype(record, AFTP_FILE_COMPLETE);

  ftp_rc = send_asuiterecord(conv_id, record);

  if(ftp_rc != AFTP_RC_OK) {
    syslog(LOG_INFO, "ftp_rc = %lu\n", ftp_rc);
    exit(1);
  }
  free(filespec);
  free(newfilespec);
}

void
a_send_file()
{
  char * filespec;
  char * newfilespec;
  unsigned int key;
  unsigned int param_len;
  int errorcode;

  do {
    param_len = get_param_len(record, curpos);
    key = get_key(record, curpos);
	    
    switch(key) 
      {
      case AFTP_FILESPEC:
	filespec = malloc(strlen(&record->buffer[curpos+3])+1); 
	if(filespec != NULL) {
	  strncpy(filespec, &record->buffer[curpos+3], 
		  strlen(&record->buffer[curpos+3])+1);
	} else {
	  send_simple_response(conv_id, ASUITE_RC_PROGRAM_INTERNAL_ERROR,
			       0, A_ERR_OUT_OF_MEMORY);
	  return;
	}
	syslog(LOG_INFO, "filespec = %s\n", filespec);
	break;
      case AFTP_NEW_FILESPEC:
	newfilespec = malloc(strlen(&record->buffer[curpos+3])+1);
	if(newfilespec != NULL) {
	  strncpy(newfilespec, &record->buffer[curpos+3],
		  strlen(&record->buffer[curpos+3])+1);
	} else {
	  send_simple_response(conv_id, ASUITE_RC_PROGRAM_INTERNAL_ERROR,
			       0, A_ERR_OUT_OF_MEMORY);
	  return;
	}
	break;
      case AFTP_DATA_TYPE:
	break;
      case AFTP_WRITE_MODE:
	break;
      case AFTP_DATE_MODE:
	break;
      case AFTP_MOD_TIME:
	break;
      case AFTP_RECORD_LENGTH:
	break;
      case AFTP_RECORD_FORMAT:
	break;
      case AFTP_BLOCK_SIZE:
	break;
      case AFTP_ALLOCATION_SIZE:
	break;
      default:
	send_simple_response(conv_id, ASUITE_RC_PROGRAM_INTERNAL_ERROR,
			     0, A_ERR_UNKNOWN_PARAMETER);
	return;
	;
      }
    curpos += param_len;
  } while(curpos < returned_length);

  syslog(LOG_INFO, "a_send_file() %s\n", filespec);

  dest_file = open(filespec, O_CREAT | O_WRONLY);

  if(dest_file < 0) {
    switch(errno) {
    case EPERM:
    case EACCES:
      errorcode = A_ERR_ACCESS_DENIED;
      break;
    default:
      errorcode = A_ERR_ACTION_NOT_PERFORMED;
      syslog(LOG_INFO, "a_send_file - errno = %s\n", strerror(errno));
    }
  
    send_simple_response(conv_id, AFTP_RC_FAIL_INPUT_ERROR, 0, 
			       errorcode);
  }  
  free(filespec);
  free(newfilespec);
}

void
a_file_data()
{
  int bytes_written;
  int errorcode;

  do {

    bytes_written = write(dest_file, &record->buffer[curpos], 
			  record->position-curpos);

    syslog(LOG_INFO, "bytes_written = %d\n", bytes_written);

    if(bytes_written < 0) {
      switch(errno) {
      default:
	errorcode = A_ERR_ACTION_NOT_PERFORMED;
	syslog(LOG_INFO, "a_file_data - errno = %s\n", strerror(errno));
      }
      send_simple_response(conv_id, AFTP_RC_FAIL_INPUT_ERROR, 0, 
			       errorcode);
      return;
    } else {
      curpos += bytes_written;
    }
  } while(curpos < record->position);

}

void 
a_file_complete()
{
  int rc;
  int errorcode;

  rc = close(dest_file);

  if(rc < 0) {
    switch(errno) {
    default:
      errorcode = A_ERR_ACTION_NOT_PERFORMED;
    }
    send_simple_response(conv_id, AFTP_RC_FAIL_INPUT_ERROR, 0, errorcode);
    return;

  } else {
    send_simple_response(conv_id, AFTP_RC_OK, 0, 0);
  }

}

void
a_delete()
{
  char * filespec;
  int rc;
  unsigned int key;
  unsigned int param_len;
  int errorcode;
  
  do {
    param_len = get_param_len(record, curpos);
    key = get_key(record, curpos);
	    
    switch(key) 
      {
      case AFTP_FILESPEC:
	filespec = malloc(strlen(&record->buffer[curpos+3])+1); 
	if(filespec != NULL) {
	  strncpy(filespec, &record->buffer[curpos+3], 
		  strlen(&record->buffer[curpos+3])+1);
	} else {
	  send_simple_response(conv_id, ASUITE_RC_PROGRAM_INTERNAL_ERROR,
			       0, A_ERR_OUT_OF_MEMORY);
	  return;
	}
	break;
      default:
	send_simple_response(conv_id, ASUITE_RC_PROGRAM_INTERNAL_ERROR,
			     0, A_ERR_UNKNOWN_PARAMETER);
	return;
	;
      }
    curpos += param_len;
  } while(curpos < returned_length);
  

  rc = unlink(filespec);

  if(rc < 0) {
    syslog(LOG_INFO, "a_delete() errno = %s\n", strerror(errno));
    switch(errno) {
    case ENOENT:
      errorcode = A_ERR_FILE_NOT_FOUND;
      break;
    case EPERM:
    case EACCES:
      errorcode = A_ERR_ACCESS_DENIED;
      break;
    default:
      errorcode = A_ERR_ACTION_NOT_PERFORMED;
    }

    send_simple_response(conv_id, AFTP_RC_FAIL_INPUT_ERROR, 0, 
			 errorcode);
    return;
  } else {
    send_simple_response(conv_id, AFTP_RC_OK, 0, 0);
  }

  free(filespec);
}

void
a_create_dir(void)
{
  char * filespec;
  int rc;
  unsigned int key;
  unsigned int param_len;
  int errorcode;
  
  do {
    param_len = get_param_len(record, curpos);
    key = get_key(record, curpos);
	    
    switch(key) 
      {
      case AFTP_FILESPEC:
	filespec = malloc(strlen(&record->buffer[curpos+3])+1); 
	if(filespec != NULL) {
	  strncpy(filespec, &record->buffer[curpos+3], 
		  strlen(&record->buffer[curpos+3])+1);
	} else {
	  send_simple_response(conv_id, ASUITE_RC_PROGRAM_INTERNAL_ERROR,
			       0, A_ERR_OUT_OF_MEMORY);
	  return;
	}
	break;
      default:
	send_simple_response(conv_id, ASUITE_RC_PROGRAM_INTERNAL_ERROR,
			     0, A_ERR_UNKNOWN_PARAMETER);
	return;
	;
      }
    curpos += param_len;
  } while(curpos < returned_length);
  

  rc = mkdir(filespec, S_IRWXU);

  if(rc < 0) {
    switch(errno) {
    case EEXIST:
      errorcode = A_ERR_FILE_EXISTS;
      break;
    case EACCES:
      errorcode = A_ERR_ACCESS_DENIED;
      break;
    default:
      errorcode = A_ERR_ACTION_NOT_PERFORMED;
    }

    send_simple_response(conv_id, AFTP_RC_FAIL_INPUT_ERROR, 0, 
			 errorcode);
    return;
  } else {
    send_simple_response(conv_id, AFTP_RC_OK, 0, 0);
  }

  free(filespec);

}

void
a_remove_dir(void)
{
  char * filespec;
  int rc;
  unsigned int key;
  unsigned int param_len;
  int errorcode;
  
  do {
    param_len = get_param_len(record, curpos);
    key = get_key(record, curpos);
	    
    switch(key) 
      {
      case AFTP_FILESPEC:
	filespec = malloc(strlen(&record->buffer[curpos+3])+1); 
	if(filespec != NULL) {
	  strncpy(filespec, &record->buffer[curpos+3], 
		  strlen(&record->buffer[curpos+3])+1);
	} else {
	  send_simple_response(conv_id, ASUITE_RC_PROGRAM_INTERNAL_ERROR,
			       0, A_ERR_OUT_OF_MEMORY);
	  return;
	}
	break;
      default:
	send_simple_response(conv_id, ASUITE_RC_PROGRAM_INTERNAL_ERROR,
			     0, A_ERR_UNKNOWN_PARAMETER);
	return;
	;
      }
    curpos += param_len;
  } while(curpos < returned_length);
  

  rc = rmdir(filespec);

  if(rc < 0) {
    switch(errno) {
    case EEXIST:
      errorcode = A_ERR_FILE_EXISTS;
      break;
    case ENOENT:
      errorcode = A_ERR_FILE_NOT_FOUND;
      break;
    case EPERM:
    case EACCES:
      errorcode = A_ERR_ACCESS_DENIED;
      break;
    default:
      errorcode = A_ERR_ACTION_NOT_PERFORMED;
      syslog(LOG_INFO, "a_remove_dir() - errno = %d\n", errno);
    }

    send_simple_response(conv_id, AFTP_RC_FAIL_INPUT_ERROR, 0, 
			 errorcode);
    return;
  } else {
    send_simple_response(conv_id, AFTP_RC_OK, 0, 0);
  }

  free(filespec);

}

void
a_rename()
{
  char * filespec;
  char * newfilespec;
  int rc;
  unsigned int key;
  unsigned int param_len;
  int errorcode;
  
  do {
    param_len = get_param_len(record, curpos);
    key = get_key(record, curpos);
	    
    switch(key) 
      {
      case AFTP_FILESPEC:
	filespec = malloc(strlen(&record->buffer[curpos+3])+1); 
	if(filespec != NULL) {
	  strncpy(filespec, &record->buffer[curpos+3], 
		  strlen(&record->buffer[curpos+3])+1);
	} else {
	  send_simple_response(conv_id, ASUITE_RC_PROGRAM_INTERNAL_ERROR,
			       0, A_ERR_OUT_OF_MEMORY);
	  return;
	}
	break;
      case AFTP_NEW_FILESPEC:
	newfilespec = malloc(strlen(&record->buffer[curpos+3])+1);
	if(newfilespec != NULL) {
	  strncpy(newfilespec, &record->buffer[curpos+3],
		  strlen(&record->buffer[curpos+3])+1);
	} else {
	  send_simple_response(conv_id, ASUITE_RC_PROGRAM_INTERNAL_ERROR,
			       0, A_ERR_OUT_OF_MEMORY);
	  return;
	}
	break;
      default:
	send_simple_response(conv_id, ASUITE_RC_PROGRAM_INTERNAL_ERROR,
			     0, A_ERR_UNKNOWN_PARAMETER);
	return;
	;
      }
    curpos += param_len;
  } while(curpos < returned_length);
  
  rc = rename(filespec, newfilespec);

  if(rc < 0) {
    switch(errno) {
    case EEXIST:
      errorcode = A_ERR_FILE_EXISTS;
      break;
    case EXDEV:
      errorcode = A_ERR_CROSS_DEVICES;
      break;
    case ENOENT:
      errorcode = A_ERR_FILE_NOT_FOUND;
      break;
    case EPERM:
    case EACCES:
      errorcode = A_ERR_ACCESS_DENIED;
      break;
    default:
      errorcode = A_ERR_ACTION_NOT_PERFORMED;
      syslog(LOG_INFO, "a_rename - errno = %d\n", errno);
    }

    send_simple_response(conv_id, AFTP_RC_FAIL_INPUT_ERROR, 0, 
			 errorcode);
    return;
  } else {
    send_simple_response(conv_id, AFTP_RC_OK, 0, 0);
  }

  free(filespec);
  free(newfilespec);
}

int 
main(void)
{

  CM_INT32 rc;
  CM_INT32 mainrc;
  char origin[MAX_FQPLU_NAME];
  unsigned int major_code;
  int done;

  openlog("aftpd", LOG_PID, LOG_DAEMON);

  record = asuiterecord_new();

  do {
    asuiterecord_clear(record);

    cmaccp(conv_id, &mainrc);
    
    if (mainrc == CM_OK) {

      cmepln(conv_id, origin, &returned_length, &rc);

      origin[returned_length] = '\0';
  
      if (mainrc == CM_OK) {
    
	syslog(LOG_INFO, "Accepted conversation from %s.\n", origin);

	done = 0;
	do {

	  rc = get_asuiterecord(conv_id, record);
    
	  if(rc == ASUITE_RC_OK) {
	
	    returned_length = get_received_len(record);
	    major_code = get_major_code(record);
	    curpos = 2;

	    syslog(LOG_INFO, "major_code = %d\n", major_code);

	    switch(major_code) 
	      {
	      case AFTP_TOWER_REQUEST:
		a_tower_request();
		break;
	      case AFTP_QUERY_SYSTEM:
		a_query_system();
		break;
	      case AFTP_QUERY_DIR:
		a_query_dir();
		break;
	      case AFTP_LIST_REQUEST:
		a_list_request();
		break;
	      case AFTP_CHANGE_DIR:
		a_change_dir();
		break;
	      case AFTP_REQUEST_FILE:
		a_request_file();
		break;
	      case AFTP_SEND_FILE:
		a_send_file();
		break;
	      case AFTP_FILE_DATA:
		a_file_data();
		break;
	      case AFTP_FILE_COMPLETE:
		a_file_complete();
		break;
	      case AFTP_DELETE:
		a_delete();
		break;
	      case AFTP_CREATE_DIR:
		a_create_dir();
		break;
	      case AFTP_REMOVE_DIR:
		a_remove_dir();
		break;
	      case AFTP_RENAME:
		a_rename();
		break;
	      case AFTP_CLOSE:
		done = 1;
		break;
	      default:
		syslog(LOG_INFO, "Unknown major code %d\n", major_code);
		done = 1;
	      }
	  } else if (rc == CM_DEALLOCATED_NORMAL) {
	    syslog(LOG_INFO, "Partner deallocated conversation.\n");
	    done = 1;
	  } else {
	    syslog(LOG_INFO, "Could not get aftp record %lu\n", rc);
	    done = 1;
	  } /* End get aftp record */

	} while(!done);
      } else {
	syslog(LOG_INFO, "Could not extract partner LU name\n");
      } /* End extract partner LU */

      cmdeal(conv_id, &rc);
    
    } else {
      syslog(LOG_INFO, "Accept conversation failed.\n");
    } /* End accept conversation */
  } while(mainrc == CM_OK);

  closelog();

  return(0);
}








