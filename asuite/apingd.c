/* apingd - APPC ping for Linux-SNA
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

#include "aping.h"

#if HAVE_SNA_CMC_H
#include <sna/cmc.h>
#else
#include <linux/cpic.h>
#endif

#include <unistd.h>

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

   openlog("apingd", LOG_PID, LOG_DAEMON);
   syslog(LOG_INFO, "apingd - APPC ping daemon\n");

   while ((c = getopt(argc, argv, "l:")) != EOF) {
      switch (c) {
         case 'l':
            syslog(LOG_INFO, "Logging to file: %s\n", optarg);
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
      syslog(LOG_INFO, "apingd: accept call failed.\n");
      exit(EXIT_FAILURE);
   } else {
      /* Extract the partner LU name and display it. */
      cmepln(conv_id,
             (unsigned char *)destination,
              &pln_length,
              &rc );

      destination[(int)pln_length] = '\0';

      syslog(LOG_INFO, "Contacted by partner: %s\n", destination);

      if (audit_file_name != NULL) {
         file = fopen(audit_file_name, "a");
         if (file != NULL) {
            syslog(LOG_INFO, "Contacted by partner: %s", destination);
            fclose(file);
         }
      }

   }

   /* Set prepare to receive type  */
   cmsptr(conv_id,                
          &prep_to_receive,
          &rc);
          
   if (rc != CM_OK) {
      syslog(LOG_INFO, "Set prepare to receive failed.\n");
      exit(EXIT_FAILURE);
   }
    
   max_receive_len = max_size = MAXPACKET;

   buffer = (unsigned char CM_PTR)malloc((unsigned int)max_size);

   get_client_version(conv_id,
		      (unsigned char)PING_MAJOR,
		      (unsigned char)PING_MINOR,
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
            syslog(LOG_INFO, "apingd: failed message recieve.\n");
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
             syslog(LOG_INFO, "apingd: set send type failed.\n");
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
                syslog(LOG_INFO, "apingd: send failed.\n");
            }
         }

         /* Send last block */
         send_type = CM_SEND_AND_PREP_TO_RECEIVE;
         cmsst(conv_id,
               &send_type,
               &rc);
         if (rc != CM_OK) {
            syslog(LOG_INFO, "apingd: failed set send type.\n");
            exit(EXIT_FAILURE);
         }

         length = received_len;
         cmsend(conv_id,
                buffer,
                &length,
                &rts_received,
                &rc);
         if (rc != CM_OK) {
            syslog(LOG_INFO, "apingd: failed send.\n");
            exit(EXIT_FAILURE);
         }
      } else {
         /* For one-way data transfer just confirm. */
         cmcfmd(conv_id,
                &rc);
         if (rc != CM_OK) {
            syslog(LOG_INFO, "apingd: failed confirm.\n");
            exit(EXIT_FAILURE);
         }
      }
   } while (rc == CM_OK);

   exit(EXIT_SUCCESS);
}

