/* acopy - APPC remote file copy program for Linux-SNA
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

#include "util.h"
#include "acopy.h"
#include <version.h>

static char const * dest_lu;
static char const * source_filespec;
static char const * dest_filespec;

int sending;

AFTP_DATA_TYPE_TYPE transfer_mode = AFTP_ASCII;
unsigned char tp_name[CM_TPN_SIZE+1];
unsigned char mode_name[CM_MN_SIZE+1];
int mode_name_set = 0;
int tp_name_set = 0;

AFTP_HANDLE_TYPE session;
AFTP_RETURN_CODE_TYPE ftprc;

int 
main(int argc, char * argv[])
{
  process_arguments(argc, argv);

  aftp_create(session, &ftprc);

  if(ftprc != AFTP_RC_OK) 
    {
      fprintf(stderr, "Could not create ftp session\n");
      exit(EXIT_FAILURE);
    }

  aftp_set_destination(session, (unsigned char *)dest_lu, strlen(dest_lu), 
		       &ftprc);

  if(ftprc != AFTP_RC_OK) 
    {
      fprintf(stderr, "Could not set destination LU %s\n", dest_lu);
      exit(EXIT_FAILURE);
    }

  if(tp_name_set) {
    aftp_set_tp_name(session, tp_name, strlen(tp_name), &ftprc);
    if(ftprc != AFTP_RC_OK) 
      {
	fprintf(stderr, "Could not set TP name %s\n", tp_name);
	exit(EXIT_FAILURE);
      }
  }

  if(mode_name_set) {
    aftp_set_mode_name(session, mode_name, strlen(mode_name), &ftprc);
    if(ftprc != AFTP_RC_OK) 
      {
	fprintf(stderr, "Could not set mode name %s\n", mode_name);
	exit(EXIT_FAILURE);
      }
  }

  aftp_connect(session, &ftprc);

  if(ftprc != AFTP_RC_OK) 
    {
      fprintf(stderr, "Could not connect to destination LU %s\n", dest_lu);
      exit(EXIT_FAILURE);
    }

  aftp_set_data_type(session, transfer_mode, &ftprc);

  if(ftprc != AFTP_RC_OK) 
    {
      fprintf(stderr, "Could not set transfer mode\n");
      exit(EXIT_FAILURE);
    }

  if(sending) {
    aftp_send_file(session, (unsigned char *) source_filespec,
		   strlen(source_filespec), (unsigned char *)dest_filespec,
		   strlen(dest_filespec), &ftprc);

    if(ftprc != AFTP_RC_OK) 
      {
	fprintf(stderr, "Error sending file %s rc = %lu\n", 
		source_filespec, ftprc);
	exit(EXIT_FAILURE);
      }

  } else {
    aftp_receive_file(session, (unsigned char *) dest_filespec, 
		      strlen(dest_filespec), (unsigned char *)source_filespec,
		      strlen(source_filespec), &ftprc);

    if(ftprc != AFTP_RC_OK) 
      {
	fprintf(stderr, "Error retrieving file %s rc = %lu\n", 
		source_filespec, ftprc);
      }
  }

  aftp_close(session, &ftprc);

  if(ftprc != AFTP_RC_OK) 
    {
      fprintf(stderr, "Error closing connection\n");
      exit(EXIT_FAILURE);
    }

  aftp_destroy(session, &ftprc);

  if(ftprc != AFTP_RC_OK) 
    {
      fprintf(stderr, "Error destroying session\n");
      exit(EXIT_FAILURE);
    }

  return(0);
}

void
process_arguments(int argc, char *argv[])
{
  unsigned char data_type;
  unsigned char * divider;
  int c;                     

  while((c = getopt(argc, argv, "M:m:t:v")) != EOF) 
    switch (c) {
    case 'M':
      data_type = optarg[0];
      switch(data_type) {
      case 'a':
	transfer_mode = AFTP_ASCII;
	break;
      case 'b':
	transfer_mode = AFTP_BINARY;
	break;
      case 'e':
	transfer_mode = AFTP_EBCDIC;
	break;
      default:
	usage();
	break;
      }
      break;
    case 'v':
      printf("acopy v%s\n%s\n", ToolsVersion, ToolsMaintain);
      exit(0);
    case 'm':
      if(strlen(optarg) > CM_MN_SIZE) {
	fprintf(stderr, "Mode name is too long\n");
	exit(EXIT_FAILURE);
      } else {
	mode_name_set = 1;
	strcpy(mode_name, optarg);
      }
      break;
    case 't':
      if(strlen(optarg) > CM_TPN_SIZE) {
	fprintf(stderr, "TP name is too long\n");
	exit(EXIT_FAILURE);
      } else {
	tp_name_set = 1;
	strcpy(tp_name, optarg);
      }
      break;
    default:
      usage();
    }

  argc -= optind; 
  argv += optind; 

  if(argc != 2) {
    usage();
  } else {
    if( (divider = strchr(argv[0], ':')) )  {
      if(strchr(argv[1], ':')) {
	usage();
      } else {
	*divider = '\0';
      }
      sending = 0;
      dest_lu = savestr(argv[0]);
      source_filespec = savestr(++divider);
      dest_filespec = savestr(argv[1]);
    } else if( (divider = strchr(argv[1], ':')) ) {
      sending = 1;
      *divider = '\0';
      dest_lu = savestr(argv[1]);
      dest_filespec = savestr(++divider);
      source_filespec = savestr(argv[0]);
    } else {
      usage();
    }
  }  
}


void 
usage(void)
{
  printf("usage: acopy -v [-M (a|e|b)] [-m mode_name] [-t tp_name]\n\t"
         "[(-n | -u userid [-p password])]\n\t"
	 "(source_lu:sourcefile destfile | sourcefile dest_lu:destfile)\n");
  exit(EXIT_FAILURE);
}
