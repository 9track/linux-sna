/* arexecd - APPC arexec TP for Linux-SNA
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

#include "arexec.h"

CM_INT32 cpicerr;

#define AREXEC_MAJOR (2)
#define AREXEC_MINOR (37)

void
execute_and_send_output(char * command,
                        unsigned char * conv_id);

int
main()
{
  unsigned char conv_id[8];            
  CM_INT32 cm_rc;                      
  CM_INT32 length;                    
  CM_INT32 rts_received;          /* Request to send received */
  CM_INT32 max_receive_len;            
  CM_INT32 what_received;             
  CM_INT32 received_len;               
  CM_INT32 status_received;            
  
  CM_SEND_TYPE send_type = CM_SEND_AND_FLUSH;
  CM_DEALLOCATE_TYPE deallocate_type = CM_DEALLOCATE_FLUSH;
  
  char buffer[MAX_COMMAND_LEN]; 
  
  char tp_major;
  char tp_minor;
  char os_name[64];

  unsigned char destination[MAX_FQPLU_NAME];

  openlog("arexecd", LOG_PID, LOG_DAEMON);

  cmaccp(conv_id, &cm_rc);

  if (cm_rc != CM_OK) {
    syslog(LOG_INFO, "Error on accept: %lu\n", cm_rc);
    exit(EXIT_FAILURE);
  } else {
    length = 17;
    cmepln(conv_id, destination, &length, &cm_rc);
    destination[(unsigned int)length] = '\0';
    syslog(LOG_INFO, "\nContacted by partner: %s\n", destination);
  }

  get_client_version(conv_id, 
		     (unsigned char)AREXEC_MAJOR,
		     (unsigned char)AREXEC_MINOR,
		     (unsigned char *)&tp_major, 
		     (unsigned char *)&tp_minor,
		     os_name, 
		     strlen(os_name), 
		     &cpicerr);
  
  /* Set send type */    
  cmsst(conv_id, &send_type, &cm_rc);

  max_receive_len = MAX_COMMAND_LEN-1;

  cmrcv(conv_id, (unsigned char *) buffer, &max_receive_len, &what_received,
	&received_len, &status_received, &rts_received, &cm_rc);

  if (cm_rc != CM_OK) {
    syslog(LOG_INFO, "Error on receive: %lu\n", cm_rc);
    exit(EXIT_FAILURE);
  }

  buffer[(unsigned int)received_len] = '\0';

  syslog(LOG_INFO, "The command is:\n%s\n\n", buffer);

  execute_and_send_output(buffer, conv_id);

  cmsdt(conv_id, &deallocate_type, &cm_rc);

  cmdeal(conv_id, &cm_rc);
  if (cm_rc) {
    syslog(LOG_INFO, "Error during deallocate: %lu\n", cm_rc);
    exit(EXIT_FAILURE);
  }

  return(EXIT_SUCCESS);

}

void
execute_and_send_output(char * command, unsigned char * conv_id)
{
  FILE *   read_handle;
  CM_INT32 cm_rc;                      
  CM_INT32 length;                     
  CM_INT32 rts_received;              

  char buffer[800];

  int done = 0;

  int read_length;              

  read_handle = popen(command, "r");
   
  if (read_handle == NULL) {
    syslog(LOG_INFO, "popen failed\n");
    strcpy(buffer, "AREXECD ERROR: The command failed to execute.\n");
    length = strlen(buffer)+1;
    cmsend(conv_id, (unsigned char *) buffer, &length, &rts_received, &cm_rc);
    if (cm_rc) {
      syslog(LOG_INFO, "Error on Send: %lu\n", cm_rc);
      exit(EXIT_FAILURE);
    }
    done = 1;
  }
  
  while (!done) {
    read_length = fread(buffer, 1, 800, read_handle);
    if (!read_length) {
      done = 1;
    } else {
      length = (CM_INT32)read_length;
      if (length) {
	cmsend(conv_id, (unsigned char *) buffer, &length, &rts_received,
	       &cm_rc);
	if (cm_rc) {
	  syslog(LOG_INFO, "Error on Send: %lu\n", cm_rc);
	  exit(EXIT_FAILURE);
	}
      } else {
	done = 1;
      }
    }
  }
  
  fclose(read_handle);

}











