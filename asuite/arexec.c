/* arexec - APPC arexec program for Linux-SNA
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

#include <config.h>
#include "arexec.h"
#include <version.h>

#define  DEFAULT_TP_NAME   "AREXECD"
#define  DEFAULT_MODE_NAME "#INTER"
#define  DEFAULT_SYM_DEST  "AREXECD"

#define AREXEC_MAJOR (2)
#define AREXEC_MINOR (37)

void 
process_arguments(int argc,
		  char *argv[],
		  cpic_session * arexecsession);

void 
usage(void);

char command[MAX_COMMAND_LEN];
CM_INT32 command_len;

CM_SYNC_LEVEL sync_level = CM_CONFIRM;

int
main( int argc, char *argv[])
{
  unsigned char conv_id[8];
  CM_INT32 cm_rc;
  CM_INT32 rts_received;
  CM_INT32 max_receive_len;
  CM_INT32 what_received;
  CM_INT32 received_len;
  CM_INT32 status_received;

  cpic_session * arexecsession;
    
  char tp_major;
  char tp_minor;
  char os_name[64];

  /* Don't buffer output to stdout */
  setbuf(stdout, NULL); 

  /* Create and populate cpic_session object */
  arexecsession = cpic_session_new();
  cpic_session_set_tp_name(arexecsession, DEFAULT_TP_NAME);
  cpic_session_set_mode_name(arexecsession, DEFAULT_MODE_NAME);
  cpic_session_set_symbolic_name(arexecsession, DEFAULT_SYM_DEST);

  process_arguments(argc, argv, arexecsession);

  cpic_session_init(arexecsession, conv_id, &cm_rc);

  /* Set sync level */    
  cmssl(conv_id, &sync_level, &cm_rc);

  /* Allocate the conversation */
  cmallc(conv_id, &cm_rc);

  if (cm_rc != CM_OK) {
    fprintf(stderr, "Failed allocate call: %lu\n", cm_rc);
    exit(EXIT_FAILURE);
  }

  get_server_version(conv_id, 
		     (unsigned char)AREXEC_MAJOR,
		     (unsigned char)AREXEC_MINOR,
		     &tp_major, 
		     &tp_minor, 
		     os_name, 
		     sizeof(os_name), 
		     &cm_rc);

  if (strlen(os_name) != 0) {
    printf("\nConnected to an arexec TP running on: %s\n", os_name);
  }

  cmsend(conv_id, (unsigned char *)command, &command_len, &rts_received, &cm_rc);

  if (cm_rc != CM_OK) {
    fprintf(stderr, "Failed send call.\n");
    exit(EXIT_FAILURE);
  }

  max_receive_len = sizeof(command);
  do {
    cmrcv(conv_id, (unsigned char *)command, &max_receive_len, 
	  &what_received, &received_len, &status_received, &rts_received,
	  &cm_rc);
    
    if (what_received != CM_NO_DATA_RECEIVED &&
	(cm_rc == CM_OK || cm_rc == CM_DEALLOCATED_NORMAL)) {
      fwrite(command, 1, (unsigned int)received_len, stdout);
    }

  } while ( !cm_rc );

  if (cm_rc != CM_DEALLOCATED_NORMAL) {
    fprintf(stderr, "Error during receive: %lu\n", cm_rc);
    exit(EXIT_FAILURE);
  }

  cpic_session_destroy(arexecsession);

  return(EXIT_SUCCESS);
}

void
process_arguments(int argc,
                  char *argv[],
                  cpic_session * arexecsession)
{
  int set_destination = 0;
  int c;                         
  
  command_len = 0;
  command[0] = '\0';

  while (optind != argc) {
    c = getopt(argc, argv, "t:m:u:p:nv");
    switch (c) {
    case EOF:
      if (set_destination == 0) {
	set_destination = 1;
	optarg = argv[optind];
	if (optarg[0] == '?') {
	  exit(EXIT_FAILURE);
	}
	optind++;
	cpic_session_set_destination(arexecsession, optarg);
      } else {
	command[0] = '\0';
	for ( ; optind<argc ; optind++ ) {
	  command_len += strlen(argv[optind])+1;
	  if (command_len < MAX_COMMAND_LEN) {
	    strcat(command, argv[optind]);
	    strcat(command, " ");
	  } else {
	    fprintf(stderr,
		    "Command length execeeds max allowed (%d).\n"
		    "No command will be sent.\n",
		    MAX_COMMAND_LEN-1);
	    exit(EXIT_FAILURE);
	  }
	}
      }
      break;
    case 'v':
      printf("arexec v%s\n%s\n", ToolsVersion, ToolsMaintain);
      exit (0);
    case 'm':
      cpic_session_set_mode_name(arexecsession, optarg);
      break;
    case 't':
      cpic_session_set_tp_name(arexecsession,optarg);
      break;
    case 'u':
      cpic_session_set_userid(arexecsession, optarg);
      break;
    case 'p':
      cpic_session_set_password(arexecsession, optarg);
      break;
    case 'n':
      cpic_session_set_security_type(arexecsession, CM_SECURITY_NONE);
      break;
    default:
      usage();
    }
  }
  if (!(set_destination && command_len)) {
    usage();
  }
}

void 
usage(void)
{
  fprintf(stderr,
	  "usage: arexec [-v] [-t tp_name] [-m mode_name] destination command \n");
  exit(EXIT_FAILURE);   

}





