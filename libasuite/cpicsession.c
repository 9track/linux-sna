/* cpicsession -- helper functions for Linux-SNA cpic programming
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

#include "cpicsession.h"

#include <ctype.h>

/****f* cpic_session_new
 * NAME
 *    cpic_session_new
 * SYNOPSIS
 *    ret = cpic_session_new ();
 * INPUTS
 *    None
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
cpic_session *cpic_session_new()
{
   cpic_session *This;

   This = cpic_new(cpic_session, 1);
   if (This == NULL)
      return NULL;

   return This;
}

/****f* cpic_session_destroy
 * NAME
 *    cpic_session_destroy
 * SYNOPSIS
 *    cpic_session_destroy (This);
 * INPUTS
 *    cpic_session *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void cpic_session_destroy(cpic_session * This)
{
   free (This);
}

int cpic_session_set_tp_name(cpic_session * This, char * tp_name)
{
    int length;
    int rc;

    length = strlen(tp_name);

    if (length < (CM_INT32)sizeof(This->tp_name)) {
        rc = 0;
        memcpy(This->tp_name, tp_name, length);
        This->tp_name[length] = '\0';
    } else {
        rc = 1;
    }
    return rc;
}

int cpic_session_set_mode_name(cpic_session * This, char * mode_name)
{
    int length;
    int rc;

    length = strlen(mode_name);

    if (length < (CM_INT32)sizeof(This->mode_name)) {
        rc = 0;
        memcpy(This->mode_name, mode_name, length);
        This->mode_name[length] = '\0';
        strupr(This->mode_name);
    } else {
        rc = 1;
    }

    return rc;
}

int cpic_session_set_symbolic_name(cpic_session * This, char * symbolic_name)
{
    int length;
    int rc;

    length = strlen(symbolic_name);

    This->symbolic = 1;
    if (length < (CM_INT32)sizeof(This->sym_dest_name)) {
        rc = 0;
        memset(This->sym_dest_name, ' ', sizeof(This->sym_dest_name));
        memcpy(This->sym_dest_name, symbolic_name, length);
        This->sym_dest_name[MAX_SYM_DEST_NAME] = '\0';
        strupr(This->sym_dest_name);
    } else {
        rc = 1;
    }

    return rc;
}

int cpic_session_set_destination(cpic_session * This, char * destination)
{
    int length;
    int rc;

    length = strlen(destination);

    This->symbolic = 0;

    if (length < (CM_INT32)sizeof(This->destination)) {
        rc = 0;
        memset(This->destination, ' ', sizeof(This->destination));
        memcpy(This->destination, destination, length);
        This->destination[length] = '\0';
        strupr(This->destination);
    } else {
        rc = 1;
    }

    return rc;
}

int cpic_session_set_userid(cpic_session * This, char * userid)
{
    int length;
    int rc;

    length = strlen(userid);

    if (length < (CM_INT32)sizeof(This->userid)) {
        rc = 0;
        memcpy(This->userid, userid, length);
        This->userid[length] = '\0';
        This->security_type = CM_SECURITY_PROGRAM;
    } else {
        rc = 1;
    }

    return rc;
}

int cpic_session_set_password(cpic_session * This, char * password)
{
    int length;
    int rc;

    length = strlen(password);

    if (length < (CM_INT32)sizeof(This->password)) {
        rc = 0;
        memcpy(This->password, password, length);
        This->password[length] = '\0';
        This->security_type = CM_SECURITY_PROGRAM;
    } else {
        rc = 1;
    }

    return rc;
}

void cpic_session_set_security_type(cpic_session * This, unsigned long security_type)
{
    This->security_type = security_type;
}

int cpic_session_init(cpic_session * This, unsigned char * cm_conv_id, CM_INT32 * cpicrc)
{

   CM_INT32 cm_rc = CM_OK;
   CM_INT32 length;
   char sym_dest_name[MAX_SYM_DEST_NAME];
           
   if (This->symbolic) {

      cminit(cm_conv_id,
             (unsigned char *)This->sym_dest_name,
              &cm_rc);
   }
   else {
      /* Use special symbolic destination name of all blank.  Must set partner LU, etc... */
      memset(sym_dest_name, ' ', sizeof(sym_dest_name));
      cminit(cm_conv_id,
             (unsigned char *)sym_dest_name,
             &cm_rc);
             
      if(cm_rc != CM_OK)
         goto errout;

      /* Set partner LU name */
      length = strlen(This->destination);
      cmspln(cm_conv_id,                  
             (unsigned char *)This->destination,
             &length,
             &cm_rc);
             
      if(cm_rc != CM_OK)
         goto errout;

      length = strlen(This->tp_name);
    
      /* Set TP name */
      cmstpn(cm_conv_id,                      
             (unsigned char *)This->tp_name,
              &length,
              &cm_rc);
              
      if(cm_rc != CM_OK)
         goto errout;        
              
      /* Set mode name */
      length = strlen(This->mode_name);
      cmsmn(cm_conv_id,                   
            (unsigned char *)This->mode_name,
             &length,
             &cm_rc);
   }

errout:

   *cpicrc = cm_rc;
   
   return(0);
}

/*
  Exchanges version and operating system information with the TP
*/
int get_server_version(unsigned char * conv_id,
		       unsigned char client_major,
		       unsigned char client_minor,
		       unsigned char * tp_major,
		       unsigned char * tp_minor,
		       char * os_name,
		       unsigned int os_name_len,
		       CM_INT32 * rc)
{
  CM_SEND_TYPE send_type = CM_BUFFER_DATA;
  CM_PREPARE_TO_RECEIVE_TYPE prep_to_receive = CM_PREP_TO_RECEIVE_FLUSH;
  CM_INT32 length;                           
  CM_INT32 rts_received;                      /* request to send received */
  CM_INT32 max_receive_len;                   
  CM_INT32 what_received;                    
  CM_INT32 received_len;                       
  CM_INT32 status_received;                   
  unsigned char buffer[EXCHANGE_BUFFER_SIZE]; /* Receive data buffer      */
  unsigned int string_length;
  
  *tp_major = 0;  /* Major version of TP */
  *tp_minor = 0;  /* Minor version of TP */

  /* Set prepare to receive type */
  cmsptr(conv_id, &prep_to_receive, rc);
           
  if (*rc != CM_OK) 
    goto errout;
 
  /* Set send type */    
  cmsst(conv_id, &send_type, rc);
          
  if (*rc != CM_OK) 
    goto errout;

  /* Get version of TP */
  buffer[0] = EXCHANGE_VERSION;
  buffer[1] = client_major;
  buffer[2] = client_minor;
  length = 3;
    
  cmsend(conv_id, buffer, &length, &rts_received, rc);

  if (*rc != CM_OK)
    goto errout;    

  max_receive_len = sizeof(buffer);
  cmrcv (conv_id, buffer, &max_receive_len, &what_received, &received_len, 
	 &status_received,
	 &rts_received, rc);
          
  if (*rc == CM_OK) {
    if (what_received != CM_NO_DATA_RECEIVED) {
      if (received_len > 2 && buffer[0]==EXCHANGE_VERSION) {
	*tp_major = buffer[1];
	*tp_minor = buffer[2];
      }
          
      if (os_name != NULL && os_name_len > 1) {
	if (received_len > 4 && buffer[3]==EXCHANGE_OS_NAME) {
	  if (received_len < (CM_INT32)sizeof(buffer)) {
	    buffer[(unsigned int)received_len] = '\0';
	  }
	  else {
	    buffer[sizeof(buffer)] = '\0';
	  }

	  string_length = strlen((char*)&buffer[4]);

	  memcpy(os_name, &buffer[4],
		 min(string_length, os_name_len-1));

	  os_name[min(string_length, os_name_len-1)] = '\0';

	}
	else {
	  os_name[0] = '\0';
	}
      }

      switch (status_received) {
      case CM_CONFIRM_RECEIVED:
	cmcfmd(conv_id, rc);
	if (*rc != CM_OK) 
	  goto errout;
	break;
      case CM_SEND_RECEIVED:
	break;
      default:
	;
      }
    } else {
      return(1);
    }
  }
  
 errout:
    
  return RC_OK;
   
}

/*
  Exchanges version and operating system information with the TP
*/
int get_client_version(unsigned char * conv_id,
		       unsigned char tp_major,
		       unsigned char tp_minor,
		       unsigned char * client_major,
		       unsigned char * client_minor,
		       char * os_name,
		       unsigned int os_name_len,
		       CM_INT32 * rc)
     
{
   CM_SEND_TYPE send_type;          
   CM_PREPARE_TO_RECEIVE_TYPE prep_to_receive;
   CM_INT32 length;             
   CM_INT32 rts_received;       
   CM_INT32 max_receive_len;     
   CM_INT32 what_received;       
   CM_INT32 received_len;        
   CM_INT32 status_received;     
   unsigned char buffer[EXCHANGE_BUFFER_SIZE];
   unsigned int string_length;
   char * local_os_name;
   unsigned int local_os_name_len;

   *client_major = 0;
   *client_minor = 0;

   /* Set prepare to receive type */
   prep_to_receive = CM_PREP_TO_RECEIVE_FLUSH;
   cmsptr(conv_id,               
          &prep_to_receive,
          rc);

   if (*rc != CM_OK) {
      fprintf(stderr, "Set prepare to receive failed.\n");
      goto errout;
   }

   /* Set send type */
   send_type = CM_BUFFER_DATA;
   cmsst(conv_id,                
         &send_type,
         rc);
         
   if (*rc != CM_OK) {
      fprintf(stderr, "Set send type failed.\n");
      goto errout;
   }

   max_receive_len = sizeof(buffer);
   cmrcv (conv_id,
          buffer,
          &max_receive_len,
          &what_received,
          &received_len,
          &status_received,
          &rts_received,
          rc);
          
   if (*rc == CM_OK) {
      if (what_received != CM_NO_DATA_RECEIVED) {
         if (received_len > 2 && buffer[0] == EXCHANGE_VERSION) {
            *client_major = buffer[1];
            *client_minor = buffer[2];
         }
         if (os_name != NULL && os_name_len > 1) {
            if (received_len > 4 && buffer[3]==EXCHANGE_OS_NAME) {
               if (received_len < (CM_INT32)sizeof(buffer)) {
                  buffer[(unsigned int)received_len] = '\0';
               }
               else {
                  buffer[sizeof(buffer)] = '\0';
               }

               string_length = strlen((char*)&buffer[4]);

               memcpy(os_name, &buffer[4],
                      min(string_length, os_name_len-1));

               os_name[min(string_length, os_name_len-1)] = '\0';


            }
            else {
               os_name[0] = '\0';
            }
         }

      }
      
      switch (status_received) {
         case CM_CONFIRM_RECEIVED:
            cmcfmd(conv_id,
                   rc);
                   
            if (*rc != CM_OK)
               return (1);
            break;
         case CM_SEND_RECEIVED:
            break;
         default:
                ;
      }
   } else {
      return (1);
   }

   if (status_received == CM_SEND_RECEIVED) {

      buffer[0] = EXCHANGE_VERSION;
      buffer[1] = tp_major;
      buffer[2] = tp_minor;
      local_os_name = OS_NAME;
      local_os_name_len = strlen(local_os_name);
      if ((local_os_name_len + 5) < sizeof(buffer)) {
         buffer[3] = EXCHANGE_OS_NAME;
         memcpy(&buffer[4], local_os_name, local_os_name_len);
         buffer[4+local_os_name_len] ='\0';
         length = local_os_name_len + 5;
      }
      else {
         length = 3;
      }

      cmsend(conv_id,
             buffer,
             &length,
             &rts_received,
             rc);

      if (*rc != CM_OK) {
          fprintf(stderr, "Send failed.\n");
          goto errout;
      }

      cmptr(conv_id,
            rc);
              
      if (*rc != CM_OK) {
         fprintf(stderr, "Prepare to recieve failed.\n");
         goto errout;
      }
   }

errout:
   
   return RC_OK;
}

/* Convert a string to upper case */
void strupr (char * string) 
{ 
  for (;*string;++string)
    *string=toupper(*string);
} 
