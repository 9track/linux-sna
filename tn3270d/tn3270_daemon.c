/* tn3270d.c: tn3270 -> SNA (LU-2) gateway implementation.
 *
 * Original author, Michael Madore <mmadore@turbolinux.com>
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
 
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <syslog.h>
#include <glib.h>

/* sna stack headers. */
#include <lua.h>
#include <libtnX.h>

/* our stuff. */
#include "tn3270_list.h"
#include "tn3270.h"
#include "tn3270_host.h"
#include "tn3270_codes.h"

char    version_s[]             = VERSION;
char    name_s[]                = "tn3270d";
char    desc_s[]                = "Linux-SNA TN3270 server";
char    maintainer_s[]          = "Jay Schulist <jschlst@linux-sna.org>";
char    company_s[]             = "linux-SNA.ORG";
char    web_s[]                 = "http://www.linux-sna.org";
int     tn3270_debug_level      = 1;
list_head(server_list);

static sigset_t blockmask, emptymask;
static int blocked = 0;

extern void sig_block(void);
extern void sig_unblock(void);

static void tn3270_server_count_and_set_fds(server_info *server, int fd)
{
        server->open_fds++;
        if (server->open_fds > server->wmark_fd)
                server->wmark_fd = server->open_fds;
        FD_SET(fd, &server->all_fds);
        if (fd > server->highest_fd)
                server->highest_fd = fd;
        return;
}

static void tn3270_server_count_and_clear_fds(server_info *server, int fd)
{
        server->open_fds--;
        FD_CLR(fd, &server->all_fds);
        return;
}

static client_t *tn3270_client_find_by_fd(struct list_head *list, int fd)
{
        client_t *l = NULL;
        struct list_head *le;

        list_for_each(le, list) {
                l = list_entry(le, client_t, list);
                if(l->fd == fd)
                        goto out;
                else
                        l = NULL;
        }
out:    return l;
}

static int tn3270_client_delete(struct list_head *list, client_t *clt)
{
        struct list_head *le, *se;
        client_t *l = NULL;

        list_for_each_safe(le, se, list) {
                l = list_entry(le, client_t, list);
                if (clt->from.sin_addr.s_addr == l->from.sin_addr.s_addr) {
                        list_del(&l->list);
                        free(l);
                        return 0;
                }
        }
        return -ENOENT;
}

static int tn3270_client_delete_list(struct list_head *list)
{
        struct list_head *le, *se;
        client_t *l;

        list_for_each_safe(le, se, list) {
                l = list_entry(le, client_t, list);
                list_del(&l->list);
                free(l);
        }
        return 0;
}

#define DATASIZE 4096  /* FIXME: Should we use bid?   */
#define BETB     1     /* Between brackets            */
#define SEND     2     /* In bracket and can send     */
#define RECV     3     /* In bracket, but cannot send */

void other_done(LUA_VERB_RECORD *verb);
int do_select(int check_socket, int sockfd);
void send_response(unsigned long *sense, LUA_VERB_RECORD * verb);
void client_response(unsigned long sense, int sequence);
void response_done(LUA_VERB_RECORD * verb);
void send_reinit();
void reinit_done(LUA_VERB_RECORD * verb);
void send_read();
void send_write(int streamtype, unsigned char * data, int len);
int send_verb(unsigned int type);

LUA_VERB_RECORD read_verb;         /* RUI read verb                     */
LUA_VERB_RECORD reinit_verb;       /* RUI init, write or term verb      */
LUA_VERB_RECORD other_verb;        /* RUI init, write or term verb      */
LUA_VERB_RECORD purge_verb;        /* RUI purge                         */
LUA_VERB_RECORD client_verb;

const char * poolname;             /* LU Pool                           */
int semaphore;                          
int sid;                           /* Session ID                        */
int sna_fd;                        /* SNA file descriptor for select()  */
char *lu_name[9];                 /* Saved LU name for session         */
unsigned char read_data[DATASIZE]; /* Outbound RU                       */
int send_state;                    /* BETB, SEND or RECV                */
int lu_session;                    /* LU or SSCP session?               */ 
int saved_retry_reinit;            /* total # of times to retry REINIT  */
int retry_reinit;                  /* How many times to try reinit      */

/* response_type
 * INPUTS
 *    LUA_VERB_RECORD * verb - verb to get response type from
 * DESCRIPTION
 *    Examines the values of DR1, DR2 and ERI to determine what type of 
 * response the host wants (No response, Definite response or exception 
 * response).  This is then converted to the equivalent TN3270E value and
 * returned.
 *
 */
unsigned char response_type(LUA_VERB_RECORD *verb)
{
  	unsigned char eri;
  	unsigned char dr1i;
  	unsigned char dr2i;

  	eri = verb->common.lua_rh.ri;
  	dr1i = verb->common.lua_rh.dr1i;
  	dr2i = verb->common.lua_rh.dr2i;
  
  	if (eri == 0 && dr1i == 0 && dr2i == 0) 
    		return TN3270E_NO_RESPONSE;
  	if ((dr1i == 1 || dr2i == 1) && eri == 0)
    		return TN3270E_ALWAYS_RESPONSE;
  	if ((dr1i == 1 || dr2i == 1) && eri == 1)
    		return TN3270E_ERROR_RESPONSE;
  	return TN3270E_NO_RESPONSE;
}

/* send_purges
 * INPUTS
 *    NONE     
 * DESCRIPTION
 *    Purges any outstanding verbs.
 */
void send_purges(void)
{
  	memset(&purge_verb, 0, sizeof(purge_verb));
  	purge_verb.common.lua_verb        = LUA_VERB_RUI;
  	purge_verb.common.lua_verb_length = sizeof(purge_verb);
  	purge_verb.common.lua_opcode      = LUA_OPCODE_RUI_PURGE;
  	purge_verb.common.lua_correlator  = __LINE__;
  
  	purge_verb.common.lua_sid = sid;

  	purge_verb.common.lua_data_ptr = (char*)&read_verb;

  	purge_verb.common.lua_post_handle = (unsigned long) other_done;
  
  	RUI(&purge_verb);

  	if (purge_verb.common.lua_flag2.async == 0)
      		other_done(&purge_verb);
	return;
}

/* do_select
 * INPUTS
 *    int check  - Check for data on telnet socket?
 *    int sockfd - Telnet socket file descriptor
 * DESCRIPTION
 *    Waits (indefinately) for data on the SNA file descriptor.  Optionally,
 * it will also wait for data on the telnet socket provided by the caller.
 * If there is data ready on the telnet file descriptor, 1 is returned, 
 * otherwise the function returns 0.
 */
int do_select(int check_socket, int sockfd)
{
  	int rc;
  	fd_set read_table;
  	fd_set write_table;
  	fd_set except_table;

  	while (semaphore || check_socket) {
      		sna_fd = SNA_GET_FD();

      		if (sna_fd == -1)  {
	  		syslog(LOG_INFO, 
		 		"Internal error - failed to get file descriptor.");
	  		_exit(-1);
		}
      		FD_ZERO(&read_table);
      		FD_ZERO(&write_table);
      		FD_ZERO(&except_table);

      		FD_SET(sna_fd, &except_table);
      		FD_SET(sna_fd, &read_table);
    
    		if (check_socket) 
			FD_SET(sockfd, &read_table);

    		rc = select(FD_SETSIZE,
                	&read_table,
                	&write_table,
                	&except_table,
                	NULL);

    		if ((rc == 1) || (rc == 2)) {
			if (FD_ISSET(sna_fd, &except_table) 
				|| FD_ISSET(sna_fd, &read_table)) {
	    			SNA_EVENT_FD();
	  		} 

			if (FD_ISSET(sockfd, &read_table)) 
	    			return 1;
      		} else {
			syslog(LOG_INFO, 
	       			"Internal error - select call failed, rc = %d. Exiting.", rc);
			_exit(-1);
      		}
    	}
  	return 0;
}

/* read_finished
 * INPUTS
 *    NONE
 * DESCRIPTION
 *    This is the callback routine for the read verb.  In the case of a DATA
 * message, the contents of the RU are appended to a 5250 buffer.  Multiple
 * RUs may be appended to one 5250 buffer. When the send state changes to BETB
 * or SEND, the buffer is sent to the client.  After sending, the buffer is 
 * cleared for the next use.  In the case of a BIND message, we check for
 * valid bind parameters and reject to bind if they don't meet our 
 * expectations.
 */
void read_finished(LUA_VERB_RECORD * verb)
{
#ifdef NOT
  unsigned char *data;
  int action = 0;
  unsigned long sense = 0l;
  int reinit_sent = 0;

  syslog(LOG_INFO, "read finished");
  if (verb->common.lua_prim_rc == LUA_OK)
    {
      if ((verb->common.lua_message_type == LUA_MESSAGE_TYPE_LU_DATA)   ||
          (verb->common.lua_message_type == LUA_MESSAGE_TYPE_SSCP_DATA))
        {
          if (verb->common.lua_data_length > 0)
            {
              data = read_data;

              tnX_buffer_append_data(&tbuf, data,
                                        verb->common.lua_data_length);
            }

          syslog(LOG_INFO, "ebi = %d", verb->common.lua_rh.ebi);
          if (verb->common.lua_rh.ebi)
            {
              /* End of bracket. Send data to client */
              syslog(LOG_INFO, "ebi present");
              tnX_stream_send_packet(hoststream,
                                        tnX_buffer_length(&tbuf), 0, 0, 0,
                                        tnX_buffer_data(&tbuf));
              tnX_buffer_init(&tbuf);
              send_state = BETB;
            }
          else if (verb->common.lua_rh.cdi ||
                   verb->common.lua_rh.ri == 0)
            {
              /* We are now in send state. Send data to client. */
              tnX_stream_send_packet(hoststream,
                                        tnX_buffer_length(&tbuf), 0, 0, 0,
                                        tnX_buffer_data(&tbuf));
              tnX_buffer_init(&tbuf);
              send_state = SEND;
            }
          else
            {
              /* We are still in receive state. Continue appending data. */
              send_state = RECV;
            }

        }
      else if (verb->common.lua_message_type == LUA_MESSAGE_TYPE_BIND)
        {
          /* Check the BIND parameters */
          if (((read_data[6] & 0x20) == 0x20)  &&  /* Brackets used and BETB */
              ((read_data[7] & 0xF0) == 0x80)  &&  /* HDX-FF, sec con winner */
              ( read_data[14]        == 0x02))     /* LU type 2              */
            {
              syslog(LOG_INFO, "BIND accepted");
              action = 1;
              send_state = BETB;
              lu_session = 1;

              if(hoststream->options && TN3270E_BIND_IMAGE)
                {
                  syslog(LOG_INFO, "TN3270E_STREAM");
                  tnX_stream_send_packet(hoststream,
                                            verb->common.lua_data_length, 0, 0, 0,
                                            read_data);
                }
            }
          else
            {
              syslog(LOG_INFO, "BIND rejected");
              sense = LUA_INVALID_SESSION_PARAMETERS;
            }
        }
      else if (verb->common.lua_message_type == LUA_MESSAGE_TYPE_UNBIND)
        {
          unsigned char reason;

          syslog(LOG_INFO, "UNBIND");
          if(hoststream->options == TN3270E_BIND_IMAGE)
            {
              reason = read_data[1];
              tnX_stream_send_packet(hoststream, 1, 0, 0, 0, &reason);
            }
          close_session();
        }
      else if(verb->common.lua_message_type == LUA_MESSAGE_TYPE_SDT)
        {
          syslog(LOG_INFO, "SDT");
          action = 1;
        }

      if ((verb->common.lua_message_type != LUA_MESSAGE_TYPE_RSP)  &&
          (verb->common.lua_rh.ri == 0)) /* definite response */
        {
          /*      action = 1;*/
          if(hoststream->streamtype == TN3270_STREAM)
            {
              /* Host wants a response. Set this flag for later. */
              action = 1;
            }
        }

    }
  else
    {
      /* We received an unexpected message */
      if (terminating == 0)
        {
          syslog(LOG_INFO,
                 "READ ERROR, primary rc = %4.4x, secondary rc = %8.8x\n",
                 verb->common.lua_prim_rc, verb->common.lua_sec_rc);
          if (verb->common.lua_prim_rc != LUA_SESSION_FAILURE)
            {
              /* The error was fatal */
              close_session();
            }
          else
            {
              /* Maybe we can get things restarted */
              send_reinit();
              reinit_sent = 1;
            }
        }
    }

  if (terminating == 0)
    {
    if (action)
      {
        /* Send the response to the host. */
        syslog(LOG_INFO, "Sending response.");
        send_response(&sense,verb);
      }
    else
      {
        syslog(LOG_INFO, "Send stat = %d", send_state);
        if (reinit_sent != 1 && send_state != SEND)
          {
            if(send_state == RECV)
              {
                /* Start the next read cycle */
                syslog(LOG_INFO, "Sending read");
                send_read();
              }
          }
      }
    }

#ifdef BROKEN
  	unsigned char *data;
  	int action = 0; 
  	unsigned long sense = 0l;
  	int reinit_sent = 0;
#ifdef OLD
  	tnXstream_hdr header;
#endif
	
  	syslog(LOG_INFO, "read finished");
  	if (verb->common.lua_prim_rc == LUA_OK) {
      		if ((verb->common.lua_message_type == LUA_MESSAGE_TYPE_LU_DATA)
	  		|| (verb->common.lua_message_type == LUA_MESSAGE_TYPE_SSCP_DATA)) {
	  		if (verb->common.lua_data_length > 0) {
	      			data = read_data;
	      			tnX_buffer_append_data(&tbuf, data, 
					verb->common.lua_data_length);
	    		}
#ifdef OLD
	  		header.h3270.data_type     = TN3270E_TYPE_3270_DATA;
	  		header.h3270.request_flag  = 0x00;
	  		if(hoststream->options && TN3270E_RESPONSES) {
	      			header.h3270.response_flag = response_type(verb);
	      			header.h3270.sequence = verb->common.lua_th.snf[0] << 8 
					| (verb->common.lua_th.snf[1] & 0x00ff);
	    		} else {
	      			header.h3270.response_flag = TN3270E_NO_RESPONSE;
	      			header.h3270.sequence      = 0x0000;
	    		}
#endif	    
	  		syslog(LOG_INFO, "ebi = %d", verb->common.lua_rh.ebi);
	  		if (verb->common.lua_rh.ebi) {
	      			/* End of bracket. Send data to client */
	      			syslog(LOG_INFO, "ebi present");
	      			tnX_stream_send_packet(hoststream,
					tnX_buffer_length(&tbuf), 0, 0, 0,
//					header,
					tnX_buffer_data(&tbuf));
	      			tnX_buffer_init(&tbuf);
	      			send_state = BETB;
	    		} else {
	  			if (verb->common.lua_rh.cdi ||
		   			verb->common.lua_rh.ri == 0) {
	      				/* We are now in send state. Send 
					 * data to client. 
					 */
	      				tnX_stream_send_packet(hoststream,
						tnX_buffer_length(&tbuf), 0, 0, 0,
//						header,
						tnX_buffer_data(&tbuf));
	      				tnX_buffer_init(&tbuf);
	      				send_state = SEND;
	    			} else {
	      				/* We are still in receive state. 
					 * Continue appending data. 
					 */
	      				send_state = RECV;
	    			}
			}
		} else {
      			if (verb->common.lua_message_type == LUA_MESSAGE_TYPE_BIND) {
	  			/* Check the BIND parameters */
	  			if (((read_data[6] & 0x20) == 0x20)  /* Brackets used and BETB */
	      				&& ((read_data[7] & 0xF0) == 0x80) /* HDX-FF, sec con winner */
	      				&& (read_data[14] == 0x02)) {     /* LU type 2              */
	      				syslog(LOG_INFO, "BIND accepted");
	      				action = 1;
	      				send_state = BETB;
	      				lu_session = 1;

	      				if (hoststream->options && TN3270E_BIND_IMAGE) {
		  				syslog(LOG_INFO, "TN3270E_STREAM");
#ifdef NOT
		  				header.h3270.data_type     = TN3270E_TYPE_BIND_IMAGE;
		  				header.h3270.request_flag  = 0x00;
		  				if(hoststream->options && TN3270E_RESPONSES)
		    					header.h3270.response_flag = response_type(verb);
		  				else
		    					header.h3270.response_flag = TN3270E_NO_RESPONSE;
		  				header.h3270.sequence      = 0x0000;
#endif
		  				tnX_stream_send_packet(hoststream, 
					    		verb->common.lua_data_length, 0, 0, 0,
					    		read_data);
					}
	    			} else {
	      				syslog(LOG_INFO, "BIND rejected");
	      				sense = LUA_INVALID_SESSION_PARAMETERS;
	    			}
			} else {
      				if (verb->common.lua_message_type == LUA_MESSAGE_TYPE_UNBIND) {
	  				unsigned char reason;

	  				syslog(LOG_INFO, "UNBIND");
	  				if(hoststream->options == TN3270E_BIND_IMAGE) {
	      					reason = read_data[1];
#ifdef NOT
	      					header.h3270.data_type = TN3270E_TYPE_UNBIND;
	      					header.h3270.request_flag = 0x00;
	      					if(hoststream->options && TN3270E_RESPONSES)
							header.h3270.response_flag = response_type(verb);
	      					else
							header.h3270.response_flag = TN3270E_NO_RESPONSE;
	      					header.h3270.sequence = 0x0000;
#endif
	      					tnX_stream_send_packet(hoststream, 1, 0, 0, 0, &reason);
	    				}
	  				close_session();
				} else {
      					if(verb->common.lua_message_type == LUA_MESSAGE_TYPE_SDT) {
	  					syslog(LOG_INFO, "SDT");
	  					action = 1;
					}
				}
			}
		}
	}

      	if ((verb->common.lua_message_type != LUA_MESSAGE_TYPE_RSP)  &&
	  	(verb->common.lua_rh.ri == 0)) { /* definite response */
	  	/*	  action = 1;*/
	  	if(hoststream->streamtype == TN3270_STREAM) {
	      		/* Host wants a response. Set this flag for later. */
	      		action = 1;
	    	}
	}

    } 
  else 
    {
      /* We received an unexpected message */
      if (terminating == 0) 
	{
	  syslog(LOG_INFO, 
		 "READ ERROR, primary rc = %4.4x, secondary rc = %8.8x\n",
		 verb->common.lua_prim_rc, verb->common.lua_sec_rc);
	  if (verb->common.lua_prim_rc != LUA_SESSION_FAILURE) 
	    {
	      /* The error was fatal */
	      close_session();
	    } 
	  else 
	    {
	      /* Maybe we can get things restarted */
	      send_reinit();
	      reinit_sent = 1;
	    }
	}
    }

  	if (terminating == 0) {
    		if (action) {
			/* Send the response to the host. */
			syslog(LOG_INFO, "Sending response.");
			send_response(&sense,verb);
      		} else {
			syslog(LOG_INFO, "Send stat = %d", send_state);
			if (reinit_sent != 1 && send_state != SEND) {
	    			if(send_state == RECV) {
					/* Start the next read cycle */
					syslog(LOG_INFO, "Sending read");
					send_read();
	      			}
	  		}
      		}
    	}
	return;
#endif
#endif
}

/* send_response
 * INPUT
 *    unsigned long *sense - sense code
 *    LUA_VERB_RECORD verb - verb we use for response
 * DESCRIPTION
 *    This function sends an SNA reponse to the host.  sense will either 
 * contain 0 (no error) , or the appropriate sense code.  The verb is passed
 * as a parameter because it already has most of the fields set appropriately.
 * We write the response on the same flows that are set in the original verb.
 ****/
void send_response(unsigned long *sense, LUA_VERB_RECORD * verb)
{
  	verb->common.lua_opcode           = LUA_OPCODE_RUI_WRITE;
  	verb->common.lua_correlator       = __LINE__;
  	verb->common.lua_max_length       = 0;
  	verb->common.lua_post_handle      = (unsigned long)response_done;
  	verb->common.lua_rh.rri           = 1;
  	verb->common.lua_flag1.lu_norm    = 0;
  	verb->common.lua_flag1.lu_exp     = 0;
  	verb->common.lua_flag1.sscp_norm  = 0;
  	verb->common.lua_flag1.sscp_exp   = 0;
  	verb->common.lua_flag1.bid_enable = 0;
  	verb->common.lua_flag1.nowait     = 0;
  	verb->common.lua_flag2.bid_enable = 0;

  	if (*sense != 0l) {
      		verb->common.lua_data_length = 4;
      		memcpy(read_data, sense, 4);
      		verb->common.lua_rh.ri = 1;
    	} else {
      		verb->common.lua_data_length = 0;
      		verb->common.lua_rh.ri = 0;
    	}

  	if (verb->common.lua_flag2.lu_norm) {
      		verb->common.lua_flag1.lu_norm = 1;
    	} else {
  		if (verb->common.lua_flag2.lu_exp) {
      			verb->common.lua_flag1.lu_exp = 1;
    		} else {
  			if (verb->common.lua_flag2.sscp_norm) {
      				verb->common.lua_flag1.sscp_norm = 1;
    			} else {
  				if (verb->common.lua_flag2.sscp_exp) {
      					verb->common.lua_flag1.sscp_exp = 1;
    				}
			}
		}
	}

  	RUI(verb);
  	if (verb->common.lua_flag2.async == 0)
      		response_done(verb);
	return;
} 

void client_response(unsigned long sense, int sequence)
{
  	memset(&client_verb, 0, sizeof(client_verb));
  	client_verb.common.lua_verb             = LUA_VERB_RUI;
  	client_verb.common.lua_verb_length      = sizeof(client_verb);
  	client_verb.common.lua_opcode           = LUA_OPCODE_RUI_WRITE;
  	client_verb.common.lua_correlator       = __LINE__;
  	client_verb.common.lua_sid              = sid;
  	client_verb.common.lua_post_handle      = (unsigned long) response_done;
  	client_verb.common.lua_th.snf[0]        = sequence >> 8;
  	client_verb.common.lua_th.snf[1]        = sequence & 0x00ff;
  	client_verb.common.lua_rh.rri           = 1;
  	client_verb.common.lua_flag1.lu_norm    = 1;

  	if (sense != 0l) {
      		client_verb.common.lua_data_length = 4;
      		memcpy(read_data, &sense, 4);
      		client_verb.common.lua_rh.ri = 1;
    	} else {
      		client_verb.common.lua_data_length = 0;
      		client_verb.common.lua_rh.ri = 0;
    	}

  	RUI(&client_verb);
  	if (client_verb.common.lua_flag2.async == 0)
      		response_done(&client_verb);
	return;
} 

/* response_done
 * INPUT
 *    LUA_VERB_RECORD verb - Verb used when sending response
 * DESCRIPTION
 *    Callback function for send_response.  If the response write failed due
 * to a session failure, we try to reinitialize the session, otherwise, we 
 * shutdown.
 */
void response_done(LUA_VERB_RECORD * verb)
{
#ifdef NOT
  	if (verb->common.lua_prim_rc != LUA_OK) {
      		syslog(LOG_INFO, "WRITE for response failed, (%4.4x, %8.8x)",
	     		verb->common.lua_prim_rc, verb->common.lua_sec_rc);
      		if (verb->common.lua_prim_rc != LUA_SESSION_FAILURE)
			close_session();
      		else
	  		send_reinit();
    	} else {
      		send_read();
    	}
#endif
	return;
} 

/* close_session
 * INPUT
 *    NONE
 * DESCRIPTION
 *    Send the TERM verb to close down the session.  The function first tries
 * to purge any outstanding verbs.
 */
int tn3270_close_session(server_info *server, client_t *client)
{
	tnX_host_destroy(client->host);

  	/* Purge outstanding verbs. */
  	send_purges();

  	if (client->terminated == 0) {
    		client->terminated = 1;
    		syslog(LOG_INFO, "Terminating session.");
    		send_verb((unsigned int)LUA_OPCODE_RUI_TERM);
  	}
	close(client->fd);
	tn3270_server_count_and_clear_fds(server, client->fd);
	list_del(&client->list);
	return 0;
} 

/* send_reinit
 * INPUT
 *    NONE
 * DESCRIPTION
 *    Sends the REINIT verb.
 */
void send_reinit()
{
  	lu_session = 0;

  	memset(&read_verb, 0, sizeof(read_verb));
  	memset(read_data, 0, DATASIZE);
  	reinit_verb.common.lua_verb        = LUA_VERB_RUI;
  	reinit_verb.common.lua_verb_length = sizeof(reinit_verb);
  	reinit_verb.common.lua_opcode      = LUA_OPCODE_RUI_REINIT;
  	reinit_verb.common.lua_correlator  = __LINE__;
  	reinit_verb.common.lua_sid         = sid;
  	reinit_verb.common.lua_post_handle = (unsigned long) reinit_done;

  	RUI(&reinit_verb);
  	if (reinit_verb.common.lua_flag2.async == 0)
      		reinit_done(&reinit_verb);
	return;
} 

/* reinit_done
 * INPUT
 *    LUA_VERB_RECORD * verb - verb from send_reinit
 * DESCRIPTION
 *    This is the callback function for send_reinit.  If the send_reinit was
 * successful, the function restarts the read cycle.  Otherwise, it will call
 * send_reinit until our maximum number of retries is met. 
 */
void reinit_done(LUA_VERB_RECORD * verb)
{
  	if (verb->common.lua_prim_rc == LUA_OK) {
      		/* Reinit was successful, start the read cycle again. */
      		retry_reinit = saved_retry_reinit;
      		syslog(LOG_INFO, "LU RE active");
      		send_read();
    	} else {
      		/* Otherwise retry until we reach our retry limit */
      		syslog(LOG_INFO, "REINIT failed, (%4.4x, %8.8x)\n",
	     		verb->common.lua_prim_rc, verb->common.lua_sec_rc);

      		if (retry_reinit != 0) {
	  		if (retry_reinit > 0) 
	    			retry_reinit--;
	  		send_reinit();
		} else {
	  		_exit(1);
		}
    	}
	return;
} 

/* send_verb
 * INPUT
 *    unsigned int type - the type of verb to send.
 * DESCRIPTION
 *    Generic verb sending function.  Sends the verb type passed in the type 
 * parameter.  The INIT verb is handled specially in order to specify the
 * LU pool name to use.  The function returns the SNA return code.
 */
int send_verb(unsigned int type)
{
  	memset(&other_verb, 0, sizeof(other_verb));
  	other_verb.common.lua_verb        = LUA_VERB_RUI;
  	other_verb.common.lua_verb_length = sizeof(other_verb);
  	other_verb.common.lua_opcode      = type;
  	other_verb.common.lua_correlator  = __LINE__;
  	if (type == LUA_OPCODE_RUI_INIT) {
      		memset(other_verb.common.lua_luname, ' ', 8);
      		memcpy(other_verb.common.lua_luname, poolname, strlen(poolname) );
    	}

  	other_verb.common.lua_post_handle = (unsigned long) other_done;
  	other_verb.common.lua_flag2.async = 1;
  	semaphore = 1;

  	RUI(&other_verb);
  	if (other_verb.common.lua_flag2.async) {
      		/* executing asynchronously.  Wait for result. */
	 	do_select(0, 0);
    	} else {
    		/* Finished immediately.  Execute the callback function. */
       		other_done(&other_verb);
    	}
  	return other_verb.common.lua_prim_rc != LUA_OK;
} 

/* other_done
 * SYNOPSIS
 *    other_done(verb)
 * INPUT
 *    LUA_VERB_RECORD * verb - verb used during generic verb send
 * DESCRIPTION
 *    This is the callback function for the generic send_verb function.
 */
void other_done(LUA_VERB_RECORD * verb)
{
  	syslog(LOG_INFO, "Other done");
  	switch (verb->common.lua_opcode) {
    		case LUA_OPCODE_RUI_INIT:
      			/* INIT verb.  Save the lu name. */
      			if (verb->common.lua_prim_rc == LUA_OK) {
	  			sid = other_verb.common.lua_sid;
	  			memcpy(lu_name, other_verb.common.lua_luname, 8);
	  			lu_name[8] = "/0";
	  			syslog(LOG_INFO, "LU active on %s\n", lu_name);
			} else {
	  			syslog(LOG_INFO, "INIT failed, (%4.4x, %8.8x)\n",
		 			verb->common.lua_prim_rc, verb->common.lua_sec_rc);
			}
      			break;
    		case LUA_OPCODE_RUI_PURGE:
      			/* PURGE verb. */
      			if (verb->common.lua_prim_rc != LUA_OK) {
	  			syslog(LOG_INFO, "PURGE failed, (%4.4x, %8.8x)\n",
		 		verb->common.lua_prim_rc, verb->common.lua_sec_rc);
			}
      			break;
    		case LUA_OPCODE_RUI_WRITE:
      			/* WRITE verb. */
      			if (verb->common.lua_prim_rc != LUA_OK) {
	  			syslog(LOG_INFO, "WRITE failed, (%4.4x, %8.8x)\n",
		 		verb->common.lua_prim_rc, verb->common.lua_sec_rc);
			}
      			break;
    		case LUA_OPCODE_RUI_BID:
      			/* BID verb.  If successful, start the read cycle. */
      			if (verb->common.lua_prim_rc == LUA_OK) {
	  			send_read(); 
			} else {
	  			syslog(LOG_INFO, "BID failed, (%4.4x, %8.8x)\n",
		 		verb->common.lua_prim_rc, verb->common.lua_sec_rc);
	  			_exit(0);
			}
      			break;
    		case LUA_OPCODE_RUI_READ:
      			/* READ verb.  If successful execute the read callback */
      			if (verb->common.lua_prim_rc == LUA_OK) {
	  			read_finished(verb); 
			}
      			break;
    		case LUA_OPCODE_RUI_TERM:
      			/* TERM verb.*/
      			syslog(LOG_INFO, "Terminated\n");
      			break;
    	}

  	semaphore = 0;
	return;
}

/* send_read
 * INPUT
 *    NONE
 * DESCRIPTION
 *    Sends a read verb.
 */
void send_read()
{
  	memset(&read_verb, 0, sizeof(read_verb));
  	memset(read_data, 0, DATASIZE);
  	read_verb.common.lua_verb             = LUA_VERB_RUI;
  	read_verb.common.lua_verb_length      = sizeof(read_verb);
  	read_verb.common.lua_opcode           = LUA_OPCODE_RUI_READ;
  	read_verb.common.lua_correlator       = __LINE__;
  	read_verb.common.lua_sid              = sid;
  	read_verb.common.lua_max_length       = DATASIZE;
  	read_verb.common.lua_data_ptr         = (char *)read_data;
  	read_verb.common.lua_post_handle      = (unsigned long)read_finished;
  	read_verb.common.lua_flag1.lu_norm    = 1;
  	read_verb.common.lua_flag1.lu_exp     = 1;
  	read_verb.common.lua_flag1.sscp_norm  = 1;
  	read_verb.common.lua_flag1.sscp_exp   = 1;

  	RUI(&read_verb);
  	if (read_verb.common.lua_flag2.async == 0)
      		read_finished(&read_verb);
	return;
} 

/* send_write
 * INPUTS
 *    Tn5250Record * record - record containing the 3270 data stream to send.
 * DESCRIPTION
 *    This function sends a write verb.  The data stream is extracted from
 * the input record.
 *****/
void send_write(int streamtype, unsigned char * data, int len)
{
  	int ok;

  	if (streamtype == TN3270E_STREAM) {
      		data += 5;
      		len  -= 5;
    	} 

  	ok = 1;
  	memset(&other_verb, 0, sizeof(other_verb));
  	other_verb.common.lua_verb        = LUA_VERB_RUI;
  	other_verb.common.lua_verb_length = sizeof(other_verb);
  	other_verb.common.lua_opcode      = LUA_OPCODE_RUI_WRITE;
  	other_verb.common.lua_correlator  = __LINE__;
  	other_verb.common.lua_sid         = sid;
  	other_verb.common.lua_data_length = len;
  	other_verb.common.lua_data_ptr    = (char *)data;
  	other_verb.common.lua_post_handle = (unsigned long)other_done;
  	other_verb.common.lua_rh.bci      = 1;
  	other_verb.common.lua_rh.eci      = 1;
  	other_verb.common.lua_rh.dr1i     = 1;
	    
  	if (lu_session) {
      		/* In the lu flow */
      		other_verb.common.lua_flag1.lu_norm  = 1;
      		other_verb.common.lua_rh.ri          = 1;
      		if (send_state == BETB) {
	  		other_verb.common.lua_rh.bbi = 1;
	  		other_verb.common.lua_rh.cdi = 1;
	  		send_state = RECV;
		} else {
      			if (send_state == SEND) {
	  			other_verb.common.lua_rh.cdi = 1;
	  			send_state = RECV;
			} else {
	  			syslog(LOG_INFO, "Wait");
	  			ok = 0;
			}
		}
	} else {
      		/* In the SSCP flow */
      		other_verb.common.lua_flag1.sscp_norm  = 1;
    	}
  
  	/* If it's ok to send */
  	if (ok) {
      		semaphore = 1;
      		RUI(&other_verb);
      		if (other_verb.common.lua_flag2.async) {
	  		/* Executing asynchronously.  Wait for the result. */
	  		do_select(0, 0);
		} else {
	  		/* Finished immediately.  Execute the callback */
	  		other_done(&other_verb);
		}
    	}
	return;
}

/* process_client
 * INPUTS
 *    int sockfd - file descriptor for the client socket.
 * DESCRIPTION
 *    Processes a new tn3270 client.  This function is invoked once for each
 * client tn3270 connection.  Once the telnet negotiation is successful, it
 * attempts to establish an LU-2 session to the host.  It then loops, acting
 * as a gateway and passing 3270 datastream packets between the telnet and LU-2
 * sides.
 */
int tn3270_client_reg_top_half(client_t *client)
{
	client->hoststream = tnX_stream_host(client->fd, 0, TN3270_STREAM);
	if (!client->hoststream)
		return -EINVAL;
	client->host = tnX_host_new(client->hoststream);
	if (!client->host)
		return -EINVAL;
	syslog(LOG_INFO, "Successful connection\n");

	/* Use application schedualing */
        SNA_USE_FD_SCHED();

	/* Initialize an empty 5250 buffer to hold 3270 packets */
        tnX_buffer_init(&client->tbuf);

	/* Attempt to connect to the host via LU-2 */
        if (send_verb((unsigned int) LUA_OPCODE_RUI_INIT)) {
		syslog(LOG_INFO, "Session init failed");
		return -EINVAL;
        }

	syslog(LOG_INFO, "SNA Session established\n");

	/* Start the SNA read cycle */
        send_read();

	return 0;
}

int tn3270_client_reg_bottom_half(client_t *client)
{
  	tnXrec *record;
#ifdef NOT
	/* Check for data on both the telnet and SNA sockets */
	if (do_select(1, sockfd) == 1) {
		if (!(tnX_stream_handle_receive(client->hoststream))) {
  			/* We got disconnected or something. */
  			syslog(LOG_INFO, "Client disconnected");
  			tn3270_close_session(client);
			return 0;
		}	

		/* Pass the data from the telnet session to the host */
		if (client->hoststream->record_count > 0) {
  			unsigned char *data;
  			int len;
  			int response;
 			int response_flag;
  			unsigned long sense = 0l;

  			sense 	= tn3270e_response(data);
  			record 	= tnX_stream_get_record(client->hoststream);
  			data 	= tnX_record_data(record);
  			len 	= tnX_record_length(record);

  			if (client->hoststream->streamtype == TN3270E_STREAM) {
      				switch (tn3270e_type(data)) {
					case TN3270E_TYPE_3270_DATA:
	  					syslog(LOG_INFO, "Sending write");
	  					send_write(client->hoststream->streamtype, data, len);
	  					break;
					case TN3270E_TYPE_RESPONSE:
	  					response_flag = tn3270e_response_flag(data);
	  					response = tn3270e_response(data);
	  
	  					if (response_flag == TN3270E_POSITIVE_RESPONSE)
	      						sense = tn3270e_response(data);
	  					else {
	      						switch (response) {
								case TN3270E_CLIENT_INVALID_COMMAND:
		  							sense = TN3270E_SERVER_INVALID_COMMAND;
		  							break;
								case TN3270E_CLIENT_PRINTER_NOT_READY:
		  							sense = TN3270E_SERVER_PRINTER_NOT_READY;
		  							break;
								case TN3270E_CLIENT_ILLEGAL_ADDRESS:
		  							sense = TN3270E_SERVER_ILLEGAL_ADDRESS;
		  							break;
								case TN3270E_CLIENT_PRINTER_OFF:
		  							sense = TN3270E_SERVER_PRINTER_OFF;
		  							break;
							}
	    					}
	  					client_response(sense, tn3270e_sequence(data)); 
	  					break;
					default:
	  					break;
				}
    			} else {
      				send_write(client->hoststream->streamtype, data, len);
    			}
		}
	}
#endif
	return 0;
}

/* process_client_test
 * INPUTS
 *    int sockfd - client telnet socket
 * DESCRIPTION
 *    This function implements a test 'host'.  tn3270 clients can connect to
 * the daemon when this function is used and receive a test screen.  This 
 * function is currently broken for Hummingbird, although it works for x3270.
 */
int tn3270_client_test_top_half(client_t *client)
{
	syslog(LOG_INFO, "here\n");
	
  	/* Perform the telnet negotiation */
  	client->hoststream = tnX_stream_host(client->fd, 0, TN3270_STREAM);
	if (!client->hoststream)
		return -EINVAL;
      	client->host = tnX_host_new(client->hoststream);
	if (!client->host)
		return -EINVAL;
      	syslog(LOG_INFO, "Successful connection\n");
    
      	/* Send a test screen and wait for an AID key to be pressed */
      	SendTestScreen(client); 
	return 0;
}

int tn3270_client_test_bottom_half(client_t *client)
{
	return readMDTfields_2(client, client->host, 0);
}

void tn3270_signal_child(int signum)
{
        int pid, status;

        while ((pid = waitpid(-1, &status, WNOHANG)) > 0);
        return;
}

void tn3270_signal_goaway(int signum)
{
        (void)signum;

        syslog(LOG_ERR, "Structured tear-down complete.");
        unlink(_PATH_TN3270D_PID);
        closelog();
        exit (0);
}

void tn3270_signal_flush(int signum)
{
        (void)signum;
        return;
}

void sig_block(void)
{
        sigprocmask(SIG_BLOCK, &blockmask, NULL);
        if (blocked) {
                syslog(LOG_ERR, "internal error - signals already blocked\n");
                syslog(LOG_ERR, "please report to %s\n", maintainer_s);
        }
        blocked = 1;
}

void sig_unblock(void)
{
        sigprocmask(SIG_SETMASK, &emptymask, NULL);
        blocked = 0;
}

void sig_init(void)
{
        struct sigaction sa;

        sigemptyset(&emptymask);
        sigemptyset(&blockmask);
        sigaddset(&blockmask, SIGCHLD);
        sigaddset(&blockmask, SIGHUP);
        sigaddset(&blockmask, SIGALRM);

        memset(&sa, 0, sizeof(sa));
        sa.sa_mask = blockmask;
        sa.sa_handler = tn3270_signal_flush;
        sigaction(SIGHUP, &sa, NULL);
        sa.sa_handler = tn3270_signal_goaway;
        sigaction(SIGTERM, &sa, NULL);
        sa.sa_handler = tn3270_signal_goaway;
        sigaction(SIGINT, &sa,  NULL);
        sa.sa_handler = SIG_IGN;
        sigaction(SIGPIPE, &sa, NULL);
	sa.sa_handler = tn3270_signal_child;
	sigaction(SIGCHLD, &sa, NULL);
	return;
}

static int tn3270_server_load_user_table(server_info *server)
{
	char path[400];
	int err;

	sprintf(path, "%s/%s.%s", _PATH_TN3270D_USER_TABLE, server->use_name, name_s);
	unlink(path);
        err = mkfifo(path, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if(err < 0 && errno != EEXIST)
                return err;
        return 0;
}

/* we could display some stuff like:
 * - time since connection was established.
 * - packets/bytes tx/rx.
 */
static int tn3270_server_user_table_check(server_info *server)
{
	struct list_head *le;
	char path[400];
        int blen = 8192;
        int fd, len = 0;
	char *buf;

	sprintf(path, "%s/%s.%s", _PATH_TN3270D_USER_TABLE, server->use_name, name_s);
        fd = open(path, O_WRONLY | O_NONBLOCK, 0);
        if (fd < 0)
                return fd;
        new_s(buf, blen);
        if (!buf)
                return -ENOMEM;

	len += sprintf(buf + len, "server: %s limit:%d used:%d\n",
		server->use_name, server->limit, server->client_num);

	len += sprintf(buf + len, "%-17s%-7s%s\n", "client", "port", "established");
        list_for_each(le, &server->client_list) {
                client_t *c = list_entry(le, client_t, list);
                if (blen - len < 1000) {
                        buf = realloc(buf, blen * 2);
                        blen = blen * 2;
                }
		len += sprintf(buf + len, "%-17s%-7d%s\n",
			inet_ntoa(c->from.sin_addr), c->from.sin_port,
			ctime(&c->start_time));
        }

        /* write the buffer to user. */
        write(fd, buf, len);
        free(buf);
        close(fd);
        usleep(1000);
        return 0;
}

static int tn3270_accept_new_client(server_info *server, int server_fd)
{
	client_t *client;
	int fromlen, err;

	if (!new(client))
		return -ENOMEM;
	fromlen = sizeof(client->from);
	client->fd = accept(server_fd, (struct sockaddr *)&client->from,
		&fromlen);
	if (client->fd < 0) {
		free(client);
		return -EINVAL;
	}

	/* validate that client is allowed to connect. */
#ifdef NOT      
        if (tnX_valid_client(permitlist, sn.sin_addr.s_addr)) {
		syslog(LOG_INFO, "Accepting connection from %s\n",
                                inet_ntoa(sn.sin_addr));
        } else {
                syslog(LOG_INFO, "Rejecting connection from %s\n",
			inet_ntoa(sn.sin_addr));
                close(sockfd);
                continue;
        }
#endif          

	if (server->client_num + 1 > server->limit) {
		syslog(LOG_INFO, "server (%s) reached maximum connections `%d'.",
			server->use_name, server->limit);
		syslog(LOG_INFO, "dropping client `%s' due to limit.",
			inet_ntoa(client->from.sin_addr));
		close(client->fd);
		free(client);
		return -EUSERS;
	}

	syslog(LOG_INFO, "client `%s' accepted on port:%d.",
		inet_ntoa(client->from.sin_addr), server_fd);

	client->start_time = time(NULL);
	list_add_tail(&client->list, &server->client_list);
	tn3270_server_count_and_set_fds(server, client->fd);
	server->client_num++;
	
	/* initiate processing of the client. */
	if (server->test)
		err = tn3270_client_test_top_half(client);
	else
		err = tn3270_client_reg_top_half(client);
	if (err < 0) {
		syslog(LOG_INFO, "final stage of client setup failed `%d'.\n", err);
		list_del(&client->list);
		close(client->fd);
		free(client);
	}
	return 0;
}

static int tn3270_make_socket(int port)
{
        int err, sock, on = 1;
        struct sockaddr_in name;
        u_long ioctlarg = 0;

        /* create the socket. */
        sock = socket(PF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
                syslog(LOG_INFO, "socket: %s\n", strerror(errno));
		return sock;
        }

        /* Give the socket a name. */
        name.sin_family         = AF_INET;
        name.sin_port           = htons(port);
        name.sin_addr.s_addr    = htonl(INADDR_ANY);
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
        tnX_ioctl(sock, FIONBIO, &ioctlarg);
        err = bind(sock, (struct sockaddr *)&name, sizeof(name));
	if (err < 0) {
                syslog(LOG_INFO, "bind: %s\n", strerror(errno));
		return err;
        }
        return sock;
}

static int tn3270_server_doit(server_info *server)
{
	struct timeval timeout;
	client_t *client;
	fd_set readable;
	int fd, err, i;

	/* initialize some runtime data. */
	list_init_head(&server->client_list);
	FD_ZERO(&server->all_fds);
	server->client_num = 0;

	/* setup signal handling */
        sig_init();

	/* dynamic client log file. */
	tn3270_server_load_user_table(server);

	/* open client and management sockets. */
	err = tn3270_make_socket(server->client_port);
	if (err < 0) {
		syslog(LOG_INFO, "failed to build client port `%d'\n", err);
		_exit(err);
	}
	server->client_sock = err;
        err = tn3270_make_socket(server->manage_port);
	if (err < 0) {
		syslog(LOG_INFO, "failed to build mgr port `%d'.\n", err);
		_exit(err);
	}
	server->manage_sock = err;

	/* put the sockets into a listen state to accept connections. */
	err = listen(server->client_sock, 1);
        if (err < 0) {
                syslog(LOG_INFO, "listen (CLIENT_PORT): %s\n", strerror(errno));
                _exit(err);
        }
	err = listen(server->manage_sock, 1);
        if (err < 0) {
                syslog(LOG_INFO, "listen (MANAGER_PORT): %s\n", strerror(errno));
                _exit(err);
        }
	tn3270_server_count_and_set_fds(server, server->client_sock);
	tn3270_server_count_and_set_fds(server, server->manage_sock);

        syslog(LOG_INFO, "TN3270 (%s) active @ port:%d mgr:%d with limit:%d.",
		server->use_name, server->client_port, server->manage_port,
		server->limit);

	/* handle all the client connections. */
	sig_block();
	for (;;) {
		readable = server->all_fds;

		memset(&timeout, 0, sizeof(timeout));
                timeout.tv_usec = TN3270D_DIR_TIMEOUT;

		sig_unblock();
		fd = select(server->highest_fd + 1, &readable,
			NULL, NULL, &timeout);
		sig_block();

		/* timer has gone off. */
		if (fd == 0) {
			tn3270_server_user_table_check(server);
			continue;
		}
		if (fd < 0) {   /* check for immediate errors. */
                        if (fd < 0 && errno != EINTR) {
                                syslog(LOG_ERR, "select failed: %s",
                                        strerror(errno));
                                sleep(1);
                        }
                        continue;
                }

		/* find which fd has an event for us. */
		for (i = 3; i <= server->highest_fd; i++) {
			if (!FD_ISSET(i, &readable))
				continue;

			if (i == server->client_sock) {
				/* process a new client. */
				tn3270_accept_new_client(server, i);
				continue;
			}

			if (i == server->manage_sock) {
				/* process a new management client. */
				syslog(LOG_INFO, "management port activated, ignoring!");
				continue;
			}

			client = tn3270_client_find_by_fd(&server->client_list, i);
			if (client) {
				/* process client data. */
				if (server->test)
					err = tn3270_client_test_bottom_half(client);
				else
					err = tn3270_client_test_bottom_half(client);

				/* this is the only place we cleanup happens
				 * for established connections. 
				 */
				if (err == -ECONNABORTED)
					tn3270_close_session(server, client);
				continue;
			}

			/* we have an unknown file description in the list. */
			syslog(LOG_INFO, "unknown file descriptor `%d'.\n", i);
		}
	}
	
	syslog(LOG_INFO, "TN3270 (%s) terminated.\n", server->use_name);
	_exit(0);
}

static int tn3270_spawn_server(server_info *server)
{
	int pid;

        if ((pid = fork()) < 0) {
		perror("fork");
		return pid;
        }
	if (pid == 0) {
		syslog(LOG_INFO, "spawned server\n");
	} else {
		tn3270_server_doit(server);
	}
	return 0;
}

static void logpid(char *path)
{
        FILE *fp;

        if ((fp = fopen(path, "w")) != NULL) {
                fprintf(fp, "%u\n", getpid());
                (void)fclose(fp);
        }
}

void version(void)
{
        printf("%s: %s %s\n%s %s\n%s\n", name_s, desc_s, version_s,
		company_s, maintainer_s, web_s);
        exit(1);
}

void help(void)
{
        printf("Usage: %s [-h] [-V] [-d level] [-f config]\n", name_s);
        exit(1);
}

int matches(const char *cmd, char *pattern)
{
        int len = strlen(cmd);
        if (len > strlen(pattern))
                return -1;
        return memcmp(pattern, cmd, len);
}

int main(int argc, char **argv)
{
	char config_file[_PATH_TN3270D_CONF_MAX];
	int nodaemon = 0, err, c;
	struct list_head *le;
	server_info *server;
	
	strcpy(config_file, _PATH_TN3270D_CONF);
	while ((c = getopt(argc, argv, "hvVf:d:")) != EOF) {
		switch (c) {
                        case 'd':       /* don't go into background. */
                                tn3270_debug_level = nodaemon = atoi(optarg);
                                break;

                        case 'f':       /* Configuration file. */
                                strcpy(config_file, optarg);
                                break;

                        case 'V':       /* Display author and version. */

                        case 'v':       /* Display author and version. */
                                version();
                                break;

                        case 'h':       /* Display useless help information. */
                                help();
                                break;
                }
	}
	
	/* read configuration information. */
	err = tn3270_read_config_file(config_file);
	if (err < 0) {
		tn3270_debug(5, "configuration file (%s) invalid `%d'.\n", 
			config_file, err);
		return err;
	}
	if (tn3270_debug_level >= 10)
		tn3270_print_config(&server_list);

	if (list_empty(&server_list)) {
		tn3270_debug(5, "no servers configured, exiting\n");
		return 0;
	}
	
	if (nodaemon == 0)
		daemon(0, 0);

	/* log our pid for scripts. */
	openlog(name_s, LOG_PID, LOG_DAEMON);
        syslog(LOG_INFO, "%s %s", desc_s, version_s);
        logpid(_PATH_TN3270D_PID);

	list_for_each(le, &server_list) {
		server = list_entry(le, server_info, list);
		tn3270_spawn_server(server);
	}

	/* setup signal handling */
        sig_init();

	/* handle and tasks for all the servers. */
	for (;;)
		sleep(1);
	return 0;
}
