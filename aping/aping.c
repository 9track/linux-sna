/* aping - APPC ping program for Linux-SNA
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
#include "getopt.h"

/*
  Little hack to allow us to compile the same code on Linux and WIN32
*/
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
   SYSTEMTIME systime;

   GetSystemTime((LPSYSTEMTIME)&systime);

   tv->tv_sec  = systime.wHour*360+systime.wMinute*60+systime.wSecond;
   tv->tv_usec = systime.wMilliseconds*1000;

   (void *)tz;

   return(0);
}

#else
#include<sys/time.h>
#include <unistd.h>
#endif

#define  DEFDATALEN 100     /* Default size of a packet */
#define  MAXPACKET  (32763) /* Maximum size of a packet accepted by cpi-c */

/* Defaults if the user doesn't override them */
#define  TP_NAME   "APINGD"
#define  MODE_NAME "#INTER"
#define  SYM_DEST  "APING   "

/* Version of the aping protocol we support */
#define  PING_MAJOR (2)
#define  PING_MINOR (40)

/* Minimum version of the aping protocol to support one-way data */
#define  ONEWAY_MAJOR (2)
#define  ONEWAY_MINOR (2)

int datalen = DEFDATALEN;
int iterations = 2; /* Default number of iterations */
int concurrent = 1; /* Default number of concurrent packets */
int one_way = 2;    /* Do two way by default */

cpic_session * pingsession;

int main(int argc, char *argv[])
{
    do_aping(argc, argv);
    return(0);
}

void usage(void)
{
   fprintf(stderr,
      "usage: aping [-1] [-c count] [-i iterations] [-s size] [-t tp_name] \n\t[-m mode_name]  destination\n");
   exit(EXIT_FAILURE);   
}

void do_aping(int argc, char *argv[])
{
   unsigned char conv_id[8];     
   CM_INT32 length;              
   CM_INT32 rts_received;        /* request to send received */
   CM_INT32 max_receive_len;    
   CM_INT32 data_received;       
   CM_INT32 received_len;        
   CM_INT32 status_received;    

   /* Data buffer for send and receive */
   unsigned char CM_PTR buffer;

   unsigned long start_time = 0;    
   unsigned long end_time;          
   unsigned long elapsed_time;     

   char tp_major;    /* Major version of the aping TP */
   char tp_minor;    /* Minor version of the aping TP */
   char os_name[64]; /* Operating system of the aping TP */

   long curr_iteration;          /* which iteration is active     */
   long curr_concurrent;         /* which send/recv is active     */
   unsigned long total_time = 0; /* used to calculate averages    */

   unsigned long min_time = 0xFFFFFFFF; /* used for min elapsed time */
   unsigned long max_time = 0;          /* used for max elapsed time */

   double rate; /* Used for calculating throughput */

   CM_SYNC_LEVEL sync_level = CM_CONFIRM;
   CM_PREPARE_TO_RECEIVE_TYPE prep_to_receive = CM_PREP_TO_RECEIVE_FLUSH;
   CM_SEND_TYPE send_type;
   CM_DEALLOCATE_TYPE deallocate_type = CM_DEALLOCATE_FLUSH;

   CM_INT32 rc;

   /* No buffering for output to stdout */
   setbuf(stdout, NULL);

   pingsession = cpic_session_new();
   cpic_session_set_tp_name(pingsession, TP_NAME);
   cpic_session_set_mode_name(pingsession, MODE_NAME);
   cpic_session_set_symbolic_name(pingsession, SYM_DEST);

   /* Scan command line arguments */
   process_arguments(argc, argv);

   /* allocate a buffer for use in sending */
   buffer = (unsigned char CM_PTR)malloc((unsigned int)datalen);

   if (buffer == NULL) {
      fprintf(stderr, "aping: out of memory.\n");
      exit(EXIT_FAILURE);
   }

   /* Set buffer to zeros */
   memset((char *)buffer, 0, (unsigned int)datalen);

   /* Initialize the session object */
   cpic_session_init(pingsession, conv_id, &rc);

   if(rc != CM_OK) {
      fprintf(stderr, "aping: failed session initialization: %lu\n", rc);
      exit(EXIT_FAILURE);
   }

   /* Set sync level */
   cmssl(conv_id,                       
         &sync_level,
         &rc);

   if (rc != CM_OK) {
      fprintf(stderr, "aping: failed set sync level: %lu", rc);
      exit(EXIT_FAILURE);
   }
   
   /* Set prepare to receive type */
   cmsptr(conv_id,               
          &prep_to_receive,
          &rc);

   if (rc != CM_OK) {
      fprintf(stderr, "Failed prepare to recieve set: %lu\n", rc);
      exit(EXIT_FAILURE);
   }

   start_time = get_time(); /* Time allocate */

   cmallc(conv_id,
          &rc);

   end_time = get_time(); /* Allocate finished */

   if (rc != CM_OK) {
      fprintf(stderr, "Failed to allocate: %lu\n", rc);
      exit(EXIT_FAILURE);
   }         
    
   printf("Allocate duration: %8lu\n\n", end_time - start_time);

   start_time = get_time(); /* Measure startup time */

   get_aping_version(conv_id,
                     (unsigned char *)&tp_major,
                     (unsigned char *)&tp_minor,
                     (char *)os_name,
                     sizeof(os_name),
                     &rc);

   if(rc != CM_OK) {
      fprintf(stderr, "Could not get version of aping TP: %lu\n", rc);
      exit(EXIT_FAILURE);
   }

   end_time = get_time(); /* Startup finished */

   /* Display operating system of the aping TP */
   if (strlen(os_name) != 0) {
       printf("Connected to a partner running on: %s\n\n", os_name);
   }

   /* Check to see if the aping TP can support one way data */
   if ((one_way == 1)  &&
       !(tp_major > ONEWAY_MAJOR ||
       (tp_major == ONEWAY_MAJOR &&
       tp_minor >= ONEWAY_MINOR))) {

      printf("One way data transfer is not supported by partner.\n"
             "Partner will echo data.\n");
         
      one_way = 2;
   }

   /* Show the startup time */
   printf("Program startup and Confirm duration: %8lu\n\n", 
          end_time - start_time);

   print_header();
    
   /* Set the amount of data we will send on each send call. */
   length = datalen;

   /* 
    -i (iterations) option determines how many times through this outer loop 
    (default 2).  Each time through the outer loop we give control over to 
    partner, or ask the partner to perform a confirm.  Each loop is a 
    transaction of sorts.
   */
   for (curr_iteration = 0; curr_iteration < iterations; curr_iteration++ ) {
         
      /* Set the send type to buffered mode */
      send_type = CM_BUFFER_DATA; 
      cmsst(conv_id,
            &send_type,
            &rc);
     
      if (rc != CM_OK) {
         fprintf(stderr, "Failed send: %lu\n", rc);
         exit(EXIT_FAILURE);
      }
        
      start_time = get_time();

      /*
       -i option determines how many times through this inner loop.  
        In other words, how many sends we perform before giving control 
        over to the partner.  All sends are part of the same transaction.
      */
      for (curr_concurrent = 1; curr_concurrent < concurrent; 
           curr_concurrent++ ) {

         cmsend(conv_id,
                buffer,
                &length,
                &rts_received,
                &rc);

         if (rc != CM_OK) {
            fprintf(stderr, "Failed send: %lu\n", rc);
            exit(EXIT_FAILURE);
         }

      }
        
      /* 
        if we are in two-way mode, we send a final packet that tells the 
        partner they now have permission to send.

        else we are in one-way mode we make a confirm call so we will 
        know when all the packets have made it two our partner.
      */
      if (one_way != 1) {
         send_type = CM_SEND_AND_PREP_TO_RECEIVE;
      } else {
         send_type = CM_SEND_AND_CONFIRM;
      }

      cmsst(conv_id,
            &send_type,
            &rc);

      if (rc != CM_OK) {
         fprintf(stderr, "Failed set send type: %lu\n", rc);
         exit(EXIT_FAILURE);
      }

      cmsend(conv_id,
             buffer,
             &length,
             &rts_received,
             &rc);
               
      if (rc != CM_OK) {
         fprintf(stderr, "Failed send: %lu\n", rc);
         exit(EXIT_FAILURE);
      }

      /* Two-way mode.  We need to receive data back from the partner */
      if (one_way != 1) {
         max_receive_len = datalen;
         /*
           Repeat the receive loop until we have received permission to send
         */
         do {

            /* Recieve a block of data */
            cmrcv (conv_id,      
                   buffer,          
                   &max_receive_len,
                   &data_received,  
                   &received_len,   
                   &status_received,
                   &rts_received,   /* Request to send */
                   &rc);
                       
            if (rc != CM_OK) {
               fprintf(stderr, "Failed recieve: %lu\n", rc);
               exit(EXIT_FAILURE);
            }

            if (data_received != CM_NO_DATA_RECEIVED) {
               curr_concurrent--;
            }

         } while ((status_received !=  CM_SEND_RECEIVED));

         /* Oops.  We didn't recieve back all the blocks we sent. */
         if (curr_concurrent != 0) {
            printf("ERROR:  Partner did not send the expected number of records.\n");
         }
      }

      end_time = get_time();       /* Send/Recieve complete */

      elapsed_time = end_time - start_time; /* Send/Recieve elapsed time */

      /* Print transfer stats for current transaction */
      printf("%16ld", elapsed_time);
      printf("%17u", datalen * concurrent * one_way);

      if (elapsed_time) {
         rate = ( ( (datalen * concurrent) / 1024) * 1000 * one_way )
                / ( elapsed_time/10) / 10;

      printf("%17.1f", rate);
      printf("%17.3f\n", (rate * 8) / 1000);

      } else {
         printf("\n");      
      }

      total_time += elapsed_time;  

      max_time = max(max_time, elapsed_time);
      min_time = min(min_time, elapsed_time);

   }

   cmsdt(conv_id,
         &deallocate_type,
         &rc);
          
   if (rc != CM_OK) {
      fprintf(stderr, "Failed set deallocate type: %lu\n", rc); 
      exit(EXIT_FAILURE);
   }
    
   cmdeal(conv_id,
          &rc);
           
   if (rc != CM_OK) {
      fprintf(stderr, "Failed deallocate: %lu\n", rc);
      exit(EXIT_FAILURE);
   }

   cpic_session_destroy(pingsession);

   /* Print out totals */
   if (total_time > 10) {

       rate =                 
         ( (datalen * concurrent / 1024) 
         * curr_iteration * 1000 * one_way)
         / ( total_time/10) / 10;

       printf("Totals:%9lu", total_time);
       printf("%17lu", datalen * concurrent * curr_iteration * one_way);
       printf("%17.1f", rate);
       printf("%17.3f\n", (rate * 8) / 1000);
   } else {
       printf("Totals:%9lu", total_time);
       printf("%17lu\n", datalen * concurrent * curr_iteration * one_way);
   }
   if (curr_iteration > 0) {
       printf("Duration statistics:  Min = %lu   Avg = %lu   Max = %lu",
              min_time, total_time/curr_iteration, max_time);
   }

   exit(EXIT_SUCCESS);

}

/*
  Processes command line arguments
*/
void process_arguments(int argc, char *argv[])
{
   int ch;

   while ((ch = getopt(argc, argv, "1fnc:t:m:i:s:u:p:")) != EOF)
      switch (ch) {
         case 'f':
            /* Add code to set buffering/non-buffering operation here */
            break; 
         case 'm':
            cpic_session_set_mode_name(pingsession, optarg);
            break;
         case 't':
            cpic_session_set_tp_name(pingsession,optarg);
            break;
         case 'u':
            cpic_session_set_userid(pingsession, optarg);
            break;
         case 'p':
            cpic_session_set_password(pingsession, optarg);
            break;
         case 'n':
            cpic_session_set_security_type(pingsession, CM_SECURITY_NONE);
            break;
         case 's':
	    datalen = atoi(optarg);
	    if (datalen > MAXPACKET) {
	       fprintf(stderr, "aping: packet size too large.\n");
	       exit(EXIT_FAILURE);
	   }
	   if (datalen <= 0) {
	       fprintf(stderr, "aping: illegal packet size.\n");
	       exit(EXIT_FAILURE);
	   }
	   break;
         case 'i':
            iterations = atoi(optarg);
            break;
         case 'c':
            concurrent = atoi(optarg);
            break;
         case '1':
            one_way = 1;
            break;
         default:
            usage();
        }
	argc -= optind;
	argv += optind;
	
	if (argc != 1) {
           usage();
        } else {
           if(strchr(*argv,'.')) {
              cpic_session_set_destination(pingsession, *argv);
           } else {
              cpic_session_set_symbolic_name(pingsession, *argv);
           }
      }
        
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

   *tp_major = 0;  /* Major version of aping TP */
   *tp_minor = 0;  /* Minor version of aping TP */

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

   /* Get version of aping TP */
   buffer[0] = EXCHANGE_VERSION;
   buffer[1] = PING_MAJOR;
   buffer[2] = PING_MINOR;
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

/*
 Prints the column headers for the transfer statistics.
*/
void print_header()
{
    printf(
    "        Duration        Data Sent        "
                                        "Data Rate        Data Rate\n"
    "        (msec)          (bytes)          "
                                        "(KB/s)           (Mb/s)   \n"
    "        --------        ---------       "
                                       " ---------        ---------\n");
}

/* 
  Returns the System Time in milliseconds 
*/
unsigned long get_time(void)
{
   struct timeval curtime;
   
   (void)gettimeofday(&curtime, (struct timezone *)NULL );
   
   return( (1000 * curtime.tv_sec) + (curtime.tv_usec/1000));
}
