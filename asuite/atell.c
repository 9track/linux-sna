/* atell - APPC tell program for Linux-SNA
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
#include "atell.h"
#include <version.h>

#define MAX_DEST_USERID (11)

#define DEFAULT_SYM_DEST  "ATELLD"
#define DEFAULT_MODE_NAME "#INTER"
#define DEFAULT_TP_NAME   "ATELLD"

#define ATELL_MAJOR (2)
#define ATELL_MINOR (37)

#define  USERID_MAJOR (2)
#define  USERID_MINOR (2)

char message[MAX_MESSAGE_LEN];
CM_INT32 message_length;
char dest_userid[MAX_DEST_USERID];

void process_arguments(int argc,
                       char *argv[],
                       cpic_session * cpicinit);

void usage(void);

int
main(int argc, char *argv[])
{
   cpic_session * atellsession;

   unsigned char conv_id[8];            
   CM_INT32 rc;
   CM_INT32 rts_received;               /* request to send received */

   char tp_major;            
   char tp_minor;            
   char os_name[64];

   CM_SYNC_LEVEL sync_level = CM_CONFIRM;
   CM_DEALLOCATE_TYPE deallocate_type = CM_DEALLOCATE_SYNC_LEVEL;

   /* Don't buffer output to stdout */
   setbuf(stdout, NULL);

   atellsession = cpic_session_new();
   cpic_session_set_tp_name(atellsession, DEFAULT_TP_NAME);
   cpic_session_set_mode_name(atellsession, DEFAULT_MODE_NAME);
   cpic_session_set_symbolic_name(atellsession, DEFAULT_SYM_DEST);

   process_arguments(argc, argv, atellsession);

   cpic_session_init(atellsession, conv_id, &rc);

   cmssl(conv_id,                        
         &sync_level,
         &rc);

   cmallc(conv_id,                       
          &rc);
          
   if (rc != CM_OK) {
      fprintf(stderr, "Failed to allocate session.\n");
      exit(EXIT_FAILURE);
   }

   get_server_version(conv_id,
		      (unsigned char)ATELL_MAJOR,
		      (unsigned char)ATELL_MINOR,
		      (unsigned char *)&tp_major,
		      (unsigned char *)&tp_minor,
		      os_name,
		      sizeof(os_name),
		      &rc);

   if (strlen(os_name) != 0) {
       printf("\nConnected to atell TP running on: %s\n",
               os_name);
   }

   if (tp_major != ATELL_MAJOR || tp_minor != ATELL_MINOR ) {
      fprintf(stderr,"\nClient/Server version mismatch.\n");
      fprintf(stderr,"Client version: %u.%02u\n", ATELL_MAJOR, ATELL_MINOR);
      fprintf(stderr,"Server version: %u.%02u\n", tp_major, tp_minor);
   }

   cmsend(conv_id,
          (unsigned char *)message,
          &message_length,
          &rts_received,
          &rc);
          
   if (rc != CM_OK) {
      fprintf(stderr, "Failed to send.\n");
      exit(EXIT_FAILURE);
   }

   if (dest_userid[0] != '\0') {
      if (tp_major > USERID_MAJOR || 
          (tp_major == USERID_MAJOR && tp_minor >= USERID_MINOR)) {
         /* send the userid */
         CM_INT32 length;
         length = strlen(dest_userid);
         cmsend(conv_id,
                (unsigned char *)dest_userid,
                &length,
                &rts_received,
                &rc);
                
         if (rc != CM_OK) {
            fprintf(stderr, "Failed to send.\n");
            exit(EXIT_FAILURE);
         }
      } else {
         fprintf(stderr, "Receipt of userid not supported by atell TP.\n");
      } 
   } 
      
   cmsdt(conv_id,
         &deallocate_type,
         &rc);
         
   if (rc != CM_OK) {
      fprintf(stderr, "Failed to deallocate session.\n");
      exit(EXIT_FAILURE);
   }

   cpic_session_destroy(atellsession);

   cmdeal(conv_id,
          &rc);
          
   if (rc != CM_OK) {
      fprintf(stderr, "Failed to deallocate session.\n");
      exit(EXIT_FAILURE);
   }

   printf("\nMessage sent successfull to: %s\n",
           atellsession->destination);

   return(EXIT_SUCCESS);
}

void
process_arguments(int argc,
                  char *argv[],
                  cpic_session * atellsession)
{

   int c;                     

   while((c = getopt(argc, argv, "t:m:u:p:nv")) != EOF) 
      switch (c) {
      case 'm':
         cpic_session_set_mode_name(atellsession, optarg);
         break;
      case 't':
         cpic_session_set_tp_name(atellsession, optarg);
         break;
      case 'u':
         cpic_session_set_userid(atellsession, optarg);
         break;
      case 'p':
         cpic_session_set_password(atellsession, optarg);
         break;
      case 'n':
         cpic_session_set_security_type(atellsession, CM_SECURITY_NONE);
         break;
      case 'v':
	 printf("atell v%s\n%s\n", ToolsVersion, ToolsMaintain);
	 exit (0);
      default:
         usage();
      }

      argc -= optind;
      argv += optind;

      if(argc != 2) {
         usage();
      } else {
         char * temp_destination;
         optarg = argv[0];

         temp_destination = strchr(optarg, '@');
         if (temp_destination == NULL) {
            /* No userid was specified */
            if(strchr(optarg,'.')) {
               cpic_session_set_destination(atellsession, optarg);
            } else {
               cpic_session_set_symbolic_name(atellsession, optarg);
            }
         } else {
            *temp_destination = '\0';
            if (strlen(optarg) < sizeof(dest_userid)) {
               strcpy(dest_userid, optarg);
               cpic_session_set_userid(atellsession, dest_userid);
            } else {
               fprintf(stderr, "Userid too long.\n"
                       "Maximum userid length is %u bytes.\n"
                       "Message sent without userid.\n",
                       sizeof(dest_userid)-1);
                       
            } 
            temp_destination++;
            if(strchr(temp_destination,'.')) {
               cpic_session_set_destination(atellsession, temp_destination);
            } else {
               cpic_session_set_symbolic_name(atellsession, temp_destination);
            }

         }      
         
      optarg = argv[1];
      message_length = strlen(argv[optind])+1;
      if(message_length < sizeof(message)) {
         strcpy(message, argv[optind]);
      } else {
         fprintf(stderr,
                 "Message exceeds maximum message length of %d\n"
                 "Message will be truncated.\n", sizeof(message-1));   
         strncpy(message, argv[optind], sizeof(message-1));
      }
   }  
}

void usage()
{
   fprintf(stderr,
      "usage: atell [-v] [-m mode_name] [-t tp_name] [userid]@destination message\n");
   exit(EXIT_FAILURE);
}
