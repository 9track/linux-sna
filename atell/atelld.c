/* atelld - APPC tell TP for Linux-SNA
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

#define  ATELL_MAJOR (2)
#define  ATELL_MINOR (37)

#define MAX_DEST_USERID (11)

void receive_message (void);

void display_message(char * destination, char * userid, char * message);

int
main(void)
{
   receive_message();
   return(EXIT_SUCCESS);
}

void
receive_message(void)
{
    unsigned char conv_id[8];            
    CM_INT32 rc;                      
    CM_INT32 length;                     
    CM_INT32 rts_received;               
    CM_INT32 max_receive_len;            
    CM_INT32 data_received;             
    CM_INT32 received_len;               
    CM_INT32 status_received;            
    char     tp_major;      
    char     tp_minor;     

    char buffer[MAX_MESSAGE_LEN];        
    char message[MAX_MESSAGE_LEN];      
    char origin[MAX_FQPLU_NAME];
    char dest_userid[MAX_DEST_USERID];
    int  done_receive_loop = 0;      

   char os_name[64];

   fprintf(stderr, "atelld\n\n");

   cmaccp(conv_id,
          &rc);

   if (rc != CM_OK) {
      fprintf(stderr, "Failed accept.\n");
      exit(EXIT_FAILURE);
   }

   get_atell_version(conv_id,
                     (unsigned char *)&tp_major,
                     (unsigned char *)&tp_minor,
                     os_name,
                     strlen(os_name),
                     &rc);


   cmepln(conv_id,
          (unsigned char CM_PTR)origin,
          &length,
          &rc);

   if (rc != CM_OK) {
      fprintf(stderr, "Extract partner LU name failed.\n");
      exit(EXIT_FAILURE);
   }
   origin[(int)length] = '\0';

   message[0] = '\0';
   dest_userid[0] = '\0';                      

   do {
      max_receive_len = sizeof(buffer);

      cmrcv(conv_id,                   
            (unsigned char *)buffer,      
             &max_receive_len,           
             &data_received,              
             &received_len,               
             &status_received,            
             &rts_received,            
             &rc);
              
      if (rc != CM_OK) {
         fprintf(stderr, "Receive failed.\n");
         exit(EXIT_FAILURE);
         done_receive_loop = 1;
      }

      if (data_received != CM_NO_DATA_RECEIVED) {

         buffer[(int)received_len] = '\0';

         if (message[0] == '\0') {
            strncpy(message, buffer, MAX_MESSAGE_LEN-1);
            message[MAX_MESSAGE_LEN-1] = '\0';
         } else {
            if (dest_userid[0] == '\0') {
               strncpy(dest_userid, buffer, MAX_DEST_USERID-1);
               dest_userid[MAX_DEST_USERID-1] = '\0';
            } else {
               fprintf(stderr, "Too many receives!\n");
            }
         }
      }

      if (status_received == CM_CONFIRM_DEALLOC_RECEIVED) {
         display_message(origin, dest_userid, message);
         done_receive_loop = 1;
      }

   } while (!done_receive_loop);

   cmcfmd(conv_id,
          &rc);

   if (rc != CM_OK) {
      fprintf(stderr, "Failed confirm.\n");
      exit(EXIT_FAILURE);
   }

#if defined(_WIN32)
    printf(stderr, "Press enter to continue:");
    getchar();
#endif

}
 
/*
  Exchanges version and operating system information with the atell TP
*/
int get_atell_version(unsigned char * conv_id,
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
      fprintf(stderr, "atelld: set prepare to receive failed.\n");
      goto errout;
   }

   /* Set send type */
   send_type = CM_BUFFER_DATA;
   cmsst(conv_id,                
         &send_type,
         rc);
         
   if (*rc != CM_OK) {
      fprintf(stderr, "atelld: set send type failed.\n");
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
      buffer[1] = ATELL_MAJOR;
      buffer[2] = ATELL_MINOR;
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
          fprintf(stderr, "atelld: send failed.\n");
          goto errout;
      }

      cmptr(conv_id,
            rc);
              
      if (*rc != CM_OK) {
         fprintf(stderr, "atelld: prepare to recieve failed.\n");
         goto errout;
      }
   }

errout:
   
   return RC_OK;
}

void
display_message(char * origin, char * dest_userid, char * message)
{
   char timestamp[TIME_LEN];
   struct tm * newtime;
   time_t ltime;

   time(&ltime);
   newtime = localtime(&ltime);
   strcpy(timestamp, asctime(newtime));

   timestamp[strlen(timestamp)-1] = '\0';

   printf("\n\n  msg from %s ", origin);
   if (dest_userid[0] != '\0') {
      printf("to user %s ", dest_userid);
   }
   printf("on %s:", timestamp);
   printf("\n\n     %s\n\n",message);
}
