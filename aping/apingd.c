/* apingd - APPC ping  for Linux-SNA
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
#include "aping.h"

#if defined(_WIN32)
#include <windows.h>
#include "wincpic.h"
#include "getopt.h"
#else
#include "cpic.h"
#include "unistd.h"
#endif

#define  MAXPACKET (32763) 

#define  PING_MAJOR (2)
#define  PING_MINOR (40)

#define  MAX_DESTINATION (18)

char * audit_file_name = NULL;
CM_PREPARE_TO_RECEIVE_TYPE prep_to_receive = CM_PREP_TO_RECEIVE_FLUSH;
CM_SEND_TYPE send_type;

int main(int argc, char *argv[])
{
   unsigned char conv_id[8];     
   CM_INT32 rc;               
   CM_INT32 length;              
   CM_INT32 rts_received;        
   CM_INT32 max_receive_len;     
   CM_INT32 data_received;       
   CM_INT32 received_len;        
   CM_INT32 status_received;     
    
   char os_name[64];
   
   FILE * file;
   CM_INT32 pln_length;
   unsigned long count; /* number of concurrent receives */

   /* Data buffer for send and receive */
   unsigned char CM_PTR buffer;    

   char destination[MAX_DESTINATION]; 
   unsigned int max_size;             
   int c;                             

   char tp_major; /* Major version of the aping client */
   char tp_minor; /* Minor version of the aping client */

   fprintf(stderr, "apingd - APPC ping daemon\n");

   while ((c = getopt(argc, argv, "l:")) != EOF) {
      switch (c) {
         case 'l':
            fprintf(stderr, "Logging to file: %s\n", optarg);
            audit_file_name = optarg;
            break;
      } 
   }

   argc -= optind;
   argv += optind;

   /* Accept Conversation */
   cmaccp(conv_id,               
          &rc);

   if (rc != CM_OK) {
      fprintf(stderr, "apingd: accept call failed.\n");
      exit(EXIT_FAILURE);
   } else {
      /* Extract the partner LU name and display it. */
      cmepln(conv_id,
             (unsigned char *)destination,
              &pln_length,
              &rc );

      destination[(int)pln_length] = '\0';

      fprintf(stderr, "Contacted by partner: %s\n", destination);

      if (audit_file_name != NULL) {
         file = fopen(audit_file_name, "a");
         if (file != NULL) {
            fprintf(file, "Contacted by partner: %s", destination);
            fclose(file);
         }
      }

   }

   /* Set prepare to receive type  */
   cmsptr(conv_id,                
          &prep_to_receive,
          &rc);
          
   if (rc != CM_OK) {
      fprintf(stderr, "apingd: set prepare to receive failed.\n");
      exit(EXIT_FAILURE);
   }
    
   max_receive_len = max_size = MAXPACKET;

   buffer = (unsigned char CM_PTR)malloc((unsigned int)max_size);

   get_aping_version(conv_id,
                     (unsigned char *)&tp_major,
                     (unsigned char *)&tp_minor,
                     os_name,
                     strlen(os_name),
                     &rc);

   do {
      count = 0;                
      /* loop until we get permission to send data or until error */
      do {
         cmrcv (conv_id,       
                buffer,            /* Data buffer for receive */
                &max_receive_len,  /* Size of Data Buffer     */
                &data_received,    
                &received_len,     
                &status_received,  
                &rts_received,     
                &rc);

         if (data_received != CM_NO_DATA_RECEIVED) {
             count++;           
         }
      } while ( (status_received != CM_SEND_RECEIVED) &&
                (status_received != CM_CONFIRM_RECEIVED) && !rc);

      if (rc != CM_OK) {
         if (rc == CM_DEALLOCATED_NORMAL) {
            exit(EXIT_SUCCESS);
         } else {
            fprintf(stderr, "apingd: failed message recieve.\n");
            exit(EXIT_FAILURE);
         }
      }

      /* Send back the same number and size of data blocks */
      if (status_received != CM_CONFIRM_RECEIVED) {
         send_type = CM_BUFFER_DATA;
         cmsst(conv_id,
               &send_type,
               &rc);
         if (rc != CM_OK) {
             fprintf(stderr, "apingd: set send type failed.\n");
             exit(EXIT_FAILURE);
         }

         /* send back the same number except for one */
         for (count--; count && !rc; count--) {
            length = received_len;
            cmsend(conv_id,
                   buffer,
                   &length,
                   &rts_received,
                   &rc);
            if (rc != CM_OK) {
                fprintf(stderr, "apingd: send failed.\n");
            }
         }

         /* Send last block */
         send_type = CM_SEND_AND_PREP_TO_RECEIVE;
         cmsst(conv_id,
               &send_type,
               &rc);
         if (rc != CM_OK) {
            fprintf(stderr, "apingd: failed set send type.\n");
            exit(EXIT_FAILURE);
         }

         length = received_len;
         cmsend(conv_id,
                buffer,
                &length,
                &rts_received,
                &rc);
         if (rc != CM_OK) {
            fprintf(stderr, "apingd: failed send.\n");
            exit(EXIT_FAILURE);
         }
      } else {
         /* For one-way data transfer just confirm. */
         cmcfmd(conv_id,
                &rc);
         if (rc != CM_OK) {
            fprintf(stderr, "apingd: failed confirm.\n");
            exit(EXIT_FAILURE);
         }
      }
   } while (rc == CM_OK);

   exit(EXIT_SUCCESS);
}

/*
  Exchanges version and operating system information with the aping TP
*/
int get_aping_version(unsigned char * conv_id,
                      unsigned char * tp_major,
                      unsigned char * tp_minor,
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

   *tp_major = 0;
   *tp_minor = 0;

   /* Set prepare to receive type */
   prep_to_receive = CM_PREP_TO_RECEIVE_FLUSH;
   cmsptr(conv_id,               
          &prep_to_receive,
          rc);

   if (*rc != CM_OK) {
      fprintf(stderr, "apingd: set prepare to receive failed.\n");
      goto errout;
   }

   /* Set send type */
   send_type = CM_BUFFER_DATA;
   cmsst(conv_id,                
         &send_type,
         rc);
         
   if (*rc != CM_OK) {
      fprintf(stderr, "apingd: set send type failed.\n");
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
               convert_from_ascii(&buffer[4], string_length);

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
      buffer[1] = PING_MAJOR;
      buffer[2] = PING_MINOR;
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
          fprintf(stderr, "apingd: send failed.\n");
          goto errout;
      }

      cmptr(conv_id,
            rc);
              
      if (*rc != CM_OK) {
         fprintf(stderr, "apingd: prepare to recieve failed.\n");
         goto errout;
      }
   }

errout:
   
   return RC_OK;
}
