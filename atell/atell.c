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

#include "atell.h"

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

int get_atell_version(unsigned char * conv_id,
                      unsigned char * tp_major,
                      unsigned char * tp_minor,
                      char * os_name,
                      unsigned int os_name_len,
                      CM_INT32 * rc);

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

   get_atell_version(conv_id,
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

   while((c = getopt(argc, argv, "t:m:u:p:n")) != EOF) 
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
                       "Maximum userid length is %d bytes.\n",
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
                 "Message exceeds maximum message length of %d\n",
                 "Message will be truncated.\n", sizeof(message-1));   
         strncpy(message, argv[optind], sizeof(message-1));
      }
   }  
}

void usage()
{
   fprintf(stderr,
      "usage: atell [-m mode_name] [-t tp_name] destination message\n");
   exit(EXIT_FAILURE);
}

/*
  Exchanges version and operating system information with the atell TP
*/
int get_atell_version(unsigned char * conv_id,
                      unsigned char * tp_major,
                      unsigned char * tp_minor,
                      char * os_name,
                      unsigned int os_name_len,
                      CM_INT32 *      rc)
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

   *tp_major = 0;  /* Major version of atell TP */
   *tp_minor = 0;  /* Minor version of atell TP */

   /* Set prepare to receive type */
   cmsptr(conv_id, 
          &prep_to_receive,
          rc);
           
   if (*rc != CM_OK) 
      goto errout;
 
   /* Set send type */    
   cmsst(conv_id,                
         &send_type,
         rc);
          
   if (*rc != CM_OK) 
      goto errout;

   /* Get version of atell TP */
   buffer[0] = EXCHANGE_VERSION;
   buffer[1] = ATELL_MAJOR;
   buffer[2] = ATELL_MINOR;
   length = 3;
    
   cmsend(conv_id,
          buffer,
          &length,
          &rts_received,
          rc);

   if (*rc != CM_OK)
      goto errout;    

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
               cmcfmd(conv_id,
                      rc);
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
