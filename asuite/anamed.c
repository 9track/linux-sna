/* anamed - APPC Name server for Linux-SNA
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

#include "anamed.h"

unsigned int curpos;
unsigned char conv_id[8];            
asuiterecord * record;
static TDB_CONTEXT *db;
char origin[MAX_FQPLU_NAME];

void
a_status()
{
  printf("ANAME_STATUS_RECORD\n");
}

void
a_register()
{
  unsigned int param_len;
  unsigned int key;
  char namebuf[256];
  TDB_DATA dbrec;
  int rc;

  syslog(LOG_INFO, "ANAME_REGISTER_REQUEST_RECORD\n");

  if(strcmp(origin, &record->buffer[curpos+3])) {
    syslog(LOG_INFO, "FQLU mismatch registering record.\n");

    send_simple_response(conv_id, ASUITE_RC_FAIL_INPUT_ERROR,
			 0, 709);

    return;
  }

  memset(namebuf, ' ', sizeof(namebuf));

  dbrec.dptr = namebuf;
  dbrec.dsize = sizeof(namerec);

  do {
    param_len = get_param_len(record, curpos);
	  
    key = get_key(record, curpos);
	    
    switch(key) 
      {
      case ANAME_USER_NAME:
	printf("ANAME_USER_NAME\n");
	memcpy(((namerec *)namebuf)->username, &record->buffer[curpos+3], 
	       param_len-4);
	((namerec *)namebuf)->username[param_len-4] = '\0';
	printf("%s\n", ((namerec *)namebuf)->username);
	break;
      case ANAME_FQLU_NAME:
	printf("ANAME_FQLU_NAME\n");
	memcpy(((namerec *)namebuf)->fqluname, &record->buffer[curpos+3],
	       param_len-4);
	((namerec *)namebuf)->fqluname[param_len-4] = '\0';
	printf("%s\n", ((namerec *)namebuf)->fqluname);
	 break;
      case ANAME_GROUP_NAME:
	printf("ANAME_GROUP_NAME\n");
	memcpy(((namerec *)namebuf)->grpname, &record->buffer[curpos+3],
	       param_len-4);
	((namerec *)namebuf)->grpname[param_len-4] = '\0';
	printf("%s\n", ((namerec *)namebuf)->grpname);
	 break;
      case ANAME_TP_NAME:
	printf("ANAME_TP_NAME\n");
	memcpy(((namerec *)namebuf)->tpname, &record->buffer[curpos+3],
	       param_len-4);
	((namerec *)namebuf)->tpname[param_len-4] = '\0';
	printf("%s\n", ((namerec *)namebuf)->tpname);
	break;
      case ANAME_BLANK_FLAG:
	printf("ANAME_BLANK_FLAG\n");
	break;
      case ANAME_DUPLICATE_FLAG:
	printf("ANAME_DUPLICATE_FLAG\n");
	break;
      default:
	fprintf(stderr, "Unknown parameter %d\n", key);
      }
    curpos += param_len;
  } while(curpos < record->position);

  rc = tdb_store(db, dbrec, dbrec, TDB_REPLACE);

  printf("tdb_store rc = %d\n", rc);

  asuiterecord_clear(record);
  asuiterecord_settype(record, ANAME_STATUS_RECORD);

  if(rc == 0) {
    asuiterecord_append_char(record, ANAME_STATUS_RETURN_CODE, ASUITE_RC_OK);
  } else {
    asuiterecord_append_char(record, ANAME_STATUS_RETURN_CODE, 
			     3);
  }

  send_asuiterecord(conv_id, record);
}

void
a_delete()
{
  unsigned int param_len;
  unsigned int key;
  char namebuf[256];
  TDB_DATA dbkey;
  CM_INT32 rc;
  int del_rc;

  printf("ANAME_DELETE_REQUEST_RECORD\n");

  memset(namebuf, ' ', sizeof(namebuf));

  do {
    param_len = get_param_len(record, curpos);
	  
    key = get_key(record, curpos);
	
    switch(key) 
      {
      case ANAME_USER_NAME:
	printf("ANAME_USER_NAME\n");
	memcpy(((namerec *)namebuf)->username, &record->buffer[curpos+3], 
	       param_len-4);
	((namerec *)namebuf)->username[param_len-4] = '\0';
	break;
      case ANAME_FQLU_NAME:
	printf("ANAME_FQLU_NAME\n");
	memcpy(((namerec *)namebuf)->fqluname, &record->buffer[curpos+3],
	       param_len-4);
	((namerec *)namebuf)->fqluname[param_len-4] = '\0';
	break;
      case ANAME_GROUP_NAME:
	printf("ANAME_GROUP_NAME\n");
	memcpy(((namerec *)namebuf)->grpname, &record->buffer[curpos+3],
	       param_len-4);
	((namerec *)namebuf)->grpname[param_len-4] = '\0';
	break;
      case ANAME_TP_NAME:
	printf("ANAME_TP_NAME\n");
	memcpy(((namerec *)namebuf)->tpname, &record->buffer[curpos+3],
	       param_len-4);
	((namerec *)namebuf)->tpname[param_len-4] = '\0';
	break;
      case ANAME_BLANK_FLAG:
	printf("ANAME_BLANK_FLAG\n");
	break;
      case ANAME_DUPLICATE_FLAG:
	printf("ANAME_DUPLICATE_FLAG\n");
	break;
      default:
	fprintf(stderr, "Unknown parameter %d\n", key);
      }
    curpos += param_len;
  } while(curpos < record->position);

  dbkey.dptr = namebuf;
  dbkey.dsize = sizeof(namerec);

  del_rc = tdb_delete(db, dbkey);

  if(del_rc == 0) {
    printf("Record deleted\n");

    asuiterecord_clear(record);
    asuiterecord_settype(record, ANAME_STATUS_RECORD);

    
    asuiterecord_append_char(record, ANAME_STATUS_RETURN_CODE,
			     ASUITE_RC_OK);


    rc = send_asuiterecord(conv_id, record);

    printf("rc = %lu\n", rc);

  } else {
    asuiterecord_clear(record);
    asuiterecord_settype(record, ANAME_STATUS_RECORD);
    asuiterecord_append_char(record, ANAME_STATUS_RETURN_CODE,
			     3);
  }

}

void
a_query()
{
  unsigned int param_len;
  unsigned int key;
  char namebuf[256];
  TDB_DATA dbrec;
  TDB_DATA dbkey;
  CM_INT32 rc;

  printf("ANAME_QUERY_REQUEST_RECORD\n");

  memset(namebuf, ' ', sizeof(namebuf));

  do {
    param_len = get_param_len(record, curpos);
	  
    key = get_key(record, curpos);
	
    switch(key) 
      {
      case ANAME_USER_NAME:
	printf("ANAME_USER_NAME\n");
	memcpy(((namerec *)namebuf)->username, &record->buffer[curpos+3], 
	       param_len-4);
	((namerec *)namebuf)->username[param_len-4] = '\0';
	break;
      case ANAME_FQLU_NAME:
	printf("ANAME_FQLU_NAME\n");
	memcpy(((namerec *)namebuf)->fqluname, &record->buffer[curpos+3],
	       param_len-4);
	((namerec *)namebuf)->fqluname[param_len-4] = '\0';
	break;
      case ANAME_GROUP_NAME:
	printf("ANAME_GROUP_NAME\n");
	memcpy(((namerec *)namebuf)->grpname, &record->buffer[curpos+3],
	       param_len-4);
	((namerec *)namebuf)->grpname[param_len-4] = '\0';
	break;
      case ANAME_TP_NAME:
	printf("ANAME_TP_NAME\n");
	memcpy(((namerec *)namebuf)->tpname, &record->buffer[curpos+3],
	       param_len-4);
	((namerec *)namebuf)->tpname[param_len-4] = '\0';
	break;
      case ANAME_BLANK_FLAG:
	printf("ANAME_BLANK_FLAG\n");
	break;
      case ANAME_DUPLICATE_FLAG:
	printf("ANAME_DUPLICATE_FLAG\n");
	break;
      default:
	fprintf(stderr, "Unknown parameter %d\n", key);
      }
    curpos += param_len;
  } while(curpos < record->position);

  dbkey.dptr = namebuf;
  dbkey.dsize = sizeof(namerec);

  dbrec = tdb_fetch(db, dbkey);

  if(dbrec.dptr != NULL) {
    printf("user name = %s\n", ((namerec *)dbrec.dptr)->username);

    asuiterecord_clear(record);
    asuiterecord_settype(record, ANAME_STATUS_RECORD);

    
    asuiterecord_append_char(record, ANAME_STATUS_RETURN_CODE,
			     ASUITE_RC_OK);


    rc = send_asuiterecord(conv_id, record);

    printf("rc = %lu\n", rc);

    asuiterecord_clear(record);
    asuiterecord_settype(record, ANAME_RESPONSE_RECORD);

    asuiterecord_append(record, ANAME_USER_NAME, 
			((namerec *)dbrec.dptr)->username,
			strlen( ((namerec *)dbrec.dptr)->username)+1 );

    asuiterecord_append(record, ANAME_FQLU_NAME,
			((namerec *)dbrec.dptr)->fqluname,
		        strlen( ((namerec *)dbrec.dptr)->fqluname)+1 );
    
    asuiterecord_append(record, ANAME_GROUP_NAME,
			((namerec *)dbrec.dptr)->grpname,
			strlen( ((namerec *)dbrec.dptr)->grpname)+1 );
    
    asuiterecord_append(record, ANAME_TP_NAME,
			((namerec *)dbrec.dptr)->tpname,
			strlen( ((namerec *)dbrec.dptr)->tpname)+1 );

    rc = send_asuiterecord(conv_id, record);

    printf("rc = %lu\n", rc);

    asuiterecord_clear(record);
    asuiterecord_settype(record, ANAME_STATUS_RECORD);

    
    asuiterecord_append_char(record, ANAME_STATUS_RETURN_CODE,
			     ASUITE_RC_OK);


    rc = send_asuiterecord(conv_id, record);

    printf("rc = %lu\n", rc);

  } else {
    asuiterecord_clear(record);
    asuiterecord_settype(record, ANAME_STATUS_RECORD);
    asuiterecord_append_char(record, ANAME_STATUS_RETURN_CODE,
			     3);

  }

}

void
a_response()
{
  printf("ANAME_RESPONSE_RECORD\n");
}

int 
main(void)
{

  CM_INT32 rc;
  CM_INT32 returned_length;
  unsigned int major_code;

  openlog("anamed", LOG_PID, LOG_DAEMON);
  syslog(LOG_INFO, "INVOKED\n");
  

  db = tdb_open(NULL, 0, 0, O_RDWR | O_CREAT | O_TRUNC, 0600);
 
  if (db == NULL) {
    printf("Error opening database.\n");
  }

  record = asuiterecord_new();

  do {
    asuiterecord_clear(record);

    cmaccp(conv_id, &rc);

    if (rc == CM_OK) {
      fprintf(stderr, "Accepted conversation.\n");

      cmepln(conv_id, origin, &returned_length, &rc);

      origin[returned_length] = '\0';
  
      if (rc == CM_OK) {
    
	printf("Partner LU = %s\n", origin); 

	rc = get_asuiterecord(conv_id, record);
    
	if(rc == ANAME_RC_OK) {
	
	  returned_length = get_received_len(record);
	  major_code = get_major_code(record);
	  curpos = 2;

	  switch(major_code) 
	    {
	    case ANAME_STATUS_RECORD:
	      a_status();
	      break;
	    case ANAME_REGISTER_REQUEST_RECORD:
	      a_register();
	      break;
	    case ANAME_DELETE_REQUEST_RECORD:
	      a_delete();
	      break;
	    case ANAME_QUERY_REQUEST_RECORD:
	      a_query();
	      break;
	    case ANAME_RESPONSE_RECORD:
	      a_response();
	      break;
	    default:
	      fprintf(stderr, "Unknown major code %d\n", major_code);
	    }
	  
	} else {
	  fprintf(stderr, "Could not get aname record.\n");
	} /* End get aname record */

      } else {
	fprintf(stderr, "Could not extract partner LU name.\n");
      } /* End extract partner LU */

      cmdeal(conv_id, &rc);
    
      if (rc != CM_OK) {
	fprintf(stderr, "Failed to deallocate session.\n");
      }

    } else {
      fprintf(stderr, "Accept converstation failed.\n");
    } /* End accept conversation */
  } while(rc == CM_OK);

  printf("rc = %lu\n", rc);

  tdb_close(db);

  return(0);
}








