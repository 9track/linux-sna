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

#include <config.h>
#include "atell.h"

#define  ATELL_MAJOR (2)
#define  ATELL_MINOR (37)

#define MAX_DEST_USERID (11)

void receive_message (void);

void display_message(char * destination, char * userid, char * message);

int
main(void)
{
  openlog("atelld", LOG_PID, LOG_DAEMON);
  receive_message();
  closelog();
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

   cmaccp(conv_id,
          &rc);

   if (rc != CM_OK) {
      syslog(LOG_INFO, "Failed accept.\n");
      exit(EXIT_FAILURE);
   }

   get_client_version(conv_id,
		     (unsigned char)ATELL_MAJOR,
		     (unsigned char)ATELL_MINOR,
                     (unsigned char *)&tp_major,
                     (unsigned char *)&tp_minor,
                     os_name,
                     strlen(os_name),
                     &rc);

   syslog(LOG_INFO, "Client ATELL version is %d.%d\n", tp_major, tp_minor);
   syslog(LOG_INFO, "Client is running on %s\n", os_name);

   cmepln(conv_id,
          (unsigned char CM_PTR)origin,
          &length,
          &rc);

   if (rc != CM_OK) {
      syslog(LOG_INFO, "Extract partner LU name failed.\n");
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
         syslog(LOG_INFO, "Receive failed.\n");
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
               syslog(LOG_INFO, "Too many receives!\n");
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
      syslog(LOG_INFO, "Failed confirm.\n");
      exit(EXIT_FAILURE);
   }

}

void
display_message(char * origin, char * dest_userid, char * message)
{
   char timestamp[TIME_LEN];
   struct tm * newtime;
   time_t ltime;
   FILE * who_file;
   FILE * ttyhandle;
   char who_list[120];
   char tty_name[120];
   char user[80];
   char tty[80];


   time(&ltime);
   newtime = localtime(&ltime);
   strcpy(timestamp, asctime(newtime));

   timestamp[strlen(timestamp)-1] = '\0';

   if(dest_userid[0] != '\0') {
     who_file = popen("who", "r");
     if(who_file == NULL) {
       syslog(LOG_INFO, "Could not run who command\n");
       tty_name[0] = '\0';
     } else {
       tty_name[0] = '\0';

       while(fgets(who_list, sizeof(who_list)-1, who_file) != NULL) {
	 sscanf(who_list, "%s %s\n", user, tty);
	 if(strcmp(user, dest_userid) == 0) {
	   strcpy(tty_name, "/dev/");
	   strcat(tty_name, tty);
	   break;
	 }
       }
       fclose(who_file);
     }

   } else {
     strcpy(tty_name, "/dev/pts/1");
   }

   if(strlen(tty_name)) {
     ttyhandle = fopen(tty_name, "w");
   } else {
     ttyhandle = NULL;
     strcpy(tty_name, "User not found\n");
   }

   if(ttyhandle == NULL) {
     syslog(LOG_INFO, "Could not open TTY: %s\n", tty_name);
     ttyhandle = stderr;
   }

   fprintf(ttyhandle, "\n\nATELLD msg from %s\n", origin);
   if(dest_userid[0] != '\0') {
     fprintf(ttyhandle, "to user %s ", dest_userid);
   }
   fprintf(ttyhandle, "on %s:", timestamp);
   fprintf(ttyhandle, "\n\n     %s\n\n", message);

}
