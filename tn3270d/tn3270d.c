/* tn3270d - tn3270 -> SNA (LU-2) gateway implementation.
 * Copyright (C) 2000 TurboLinux
 * Written by Michael Madore <mmadore@turbolinux.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA
 * 
 * As a special exception, the Free Software Foundation gives permission
 * for additional uses of the text contained in its release of TN5250.
 * 
 * The exception is that, if you link the TN5250 library with other files
 * to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public License.
 * Your use of that executable is in no way restricted on account of
 * linking the TN5250 library code into it.
 * 
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the GNU General Public License.
 * 
 * If you write modifications of your own for TN5250, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice. 
 */

#include <config.h>
#include "tn5250.h"
#include "host3270.h"
#include "tn3270d.h"
#include "codes3270.h"
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <syslog.h>
#include <lua_c.h>

#define DATASIZE 4096  /* FIXME: Should we use bid?   */
#define BETB     1     /* Between brackets            */
#define SEND     2     /* In bracket and can send     */
#define RECV     3     /* In bracket, but cannot send */

void other_done(LUA_VERB_RECORD *verb);
int do_select(int check_socket, int sockfd);
void send_response(unsigned long *sense, LUA_VERB_RECORD * verb);
void client_response(unsigned long sense, int sequence);
void response_done(LUA_VERB_RECORD * verb);
void close_session();
void send_reinit();
void reinit_done(LUA_VERB_RECORD * verb);
void send_read();
void send_write(int streamtype, unsigned char * data, int len);
int send_verb(unsigned int type);
void reload_config();
void sig_hup(int signum);

LUA_VERB_RECORD read_verb;         /* RUI read verb                     */
LUA_VERB_RECORD reinit_verb;       /* RUI init, write or term verb      */
LUA_VERB_RECORD other_verb;        /* RUI init, write or term verb      */
LUA_VERB_RECORD purge_verb;        /* RUI purge                         */
LUA_VERB_RECORD client_verb;

const char * poolname;             /* LU Pool                           */
int semaphore;                          
int sid;                           /* Session ID                        */
int sna_fd;                        /* SNA file descriptor for select()  */
char * lu_name[9];                 /* Saved LU name for session         */
unsigned char read_data[DATASIZE]; /* Outbound RU                       */
int send_state;                    /* BETB, SEND or RECV                */
int lu_session;                    /* LU or SSCP session?               */ 
int terminating;                   /* Are we in the process of quitting */
int saved_retry_reinit;            /* total # of times to retry REINIT  */
int retry_reinit;                  /* How many times to try reinit      */
Tn5250Stream * hoststream;         /* Telnet stream to client           */
Tn5250Host * host;                 /* Telnet session object             */
Tn5250Buffer tbuf;                 /* Telnet packet buffer              */

Tn5250Config * config = NULL;      /* Configuration file object         */
GSList * addrlist;                 /* Address list from config file     */
GSList * permitlist;               /* List of allowable hosts           */

int reconfig= 0;                   /* SIGHUP sets this to true          */

/****f* tn3270d/response_type
 * NAME
 *    response_type
 * SYNOPSIS
 *    type = response_type(verb)
 * INPUTS
 *    LUA_VERB_RECORD * verb - verb to get response type from
 * DESCRIPTION
 *    Examines the values of DR1, DR2 and ERI to determine what type of 
 * response the host wants (No response, Definite response or exception 
 * response).  This is then converted to the equivalent TN3270E value and
 * returned.
 *
 *****/
unsigned char
response_type(LUA_VERB_RECORD * verb)
{
  unsigned char eri;
  unsigned char dr1i;
  unsigned char dr2i;

  eri = verb->common.lua_rh.ri;
  dr1i = verb->common.lua_rh.dr1i;
  dr2i = verb->common.lua_rh.dr2i;
  
  if( eri == 0 && dr1i == 0 && dr2i == 0) 
    return TN3270E_NO_RESPONSE;
  
  if( (dr1i == 1 || dr2i == 1) && eri == 0)
    return TN3270E_ALWAYS_RESPONSE;

  if( (dr1i == 1 || dr2i == 1) && eri == 1)
    return TN3270E_ERROR_RESPONSE;

  return TN3270E_NO_RESPONSE;
}

/****f* tn3270d/send_purges
 * NAME 
 *    send_purges
 * SYNOPSIS
 *    send_purges(void)
 * INPUTS
 *    NONE     
 * DESCRIPTION
 *    Purges any outstanding verbs.
 *****/
void 
send_purges(void)
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
    {
      other_done(&purge_verb);
    }
}

/****f* tn3270d/do_select
 * NAME 
 *    do_select
 * SYNOPSIS
 *    ret = do_select(check, sockfd)
 * INPUTS
 *    int check  - Check for data on telnet socket?
 *    int sockfd - Telnet socket file descriptor
 * DESCRIPTION
 *    Waits (indefinately) for data on the SNA file descriptor.  Optionally,
 * it will also wait for data on the telnet socket provided by the caller.
 * If there is data ready on the telnet file descriptor, 1 is returned, 
 * otherwise the function returns 0.
 *****/
int 
do_select(int check_socket, int sockfd)
{
  int rc;
  fd_set read_table;
  fd_set write_table;
  fd_set except_table;


  while(semaphore || check_socket)
    {
      sna_fd = SNA_GET_FD();

      if (sna_fd == -1) 
	{
	  syslog(LOG_INFO, 
		 "Internal error - failed to get file descriptor.");
	  _exit(-1);
	}

      FD_ZERO(&read_table);
      FD_ZERO(&write_table);
      FD_ZERO(&except_table);

      FD_SET(sna_fd, &except_table);
      FD_SET(sna_fd, &read_table);
    
    if(check_socket) 
      {
	FD_SET(sockfd, &read_table);
      }

    rc = select(FD_SETSIZE,
                &read_table,
                &write_table,
                &except_table,
                NULL);

    if ((rc == 1) || (rc == 2))
      {
	if (FD_ISSET(sna_fd, &except_table) || FD_ISSET(sna_fd, &read_table)) 
	  {
	    SNA_EVENT_FD();
	  } 

	if (FD_ISSET(sockfd, &read_table)) 
	  {
	    return(1);
	  }

      }
    else
      {
	syslog(LOG_INFO, 
	       "Internal error - select call failed, rc = %d. Exiting.", rc);
	_exit(-1);
      }
    }
  return (0);
}

/****f* tn3270d/read_finished
 * NAME
 *    read_finished
 * SYNOPSIS
 *    read_finished()
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
 *****/
void 
read_finished(LUA_VERB_RECORD * verb)
{
  unsigned char *data;
  int action = 0; 
  unsigned long sense = 0l;
  int reinit_sent = 0;
  StreamHeader header;

  syslog(LOG_INFO, "read finished");
  if (verb->common.lua_prim_rc == LUA_OK)
    {
      if ((verb->common.lua_message_type == LUA_MESSAGE_TYPE_LU_DATA)   ||
	  (verb->common.lua_message_type == LUA_MESSAGE_TYPE_SSCP_DATA))
	{
	  if (verb->common.lua_data_length > 0)
	    {
	      data = read_data;

	      tn5250_buffer_append_data(&tbuf, data, 
					verb->common.lua_data_length);
	    }

	  header.h3270.data_type     = TN3270E_TYPE_3270_DATA;
	  header.h3270.request_flag  = 0x00;
	  if(hoststream->options && TN3270E_RESPONSES)
	    {
	      header.h3270.response_flag = response_type(verb);
	      header.h3270.sequence = verb->common.lua_th.snf[0] << 8 
		| (verb->common.lua_th.snf[1] & 0x00ff);
	    }
	  else
	    {
	      header.h3270.response_flag = TN3270E_NO_RESPONSE;
	      header.h3270.sequence      = 0x0000;
	    }
	    
	  syslog(LOG_INFO, "ebi = %d", verb->common.lua_rh.ebi);
	  if (verb->common.lua_rh.ebi)
	    {
	      /* End of bracket. Send data to client */
	      syslog(LOG_INFO, "ebi present");
	      tn5250_stream_send_packet(hoststream,
					tn5250_buffer_length(&tbuf),
					header,
					tn5250_buffer_data(&tbuf));
	      tn5250_buffer_init(&tbuf);
	      send_state = BETB;
	    }
	  else if (verb->common.lua_rh.cdi ||
		   verb->common.lua_rh.ri == 0)
	    {
	      /* We are now in send state. Send data to client. */
	      tn5250_stream_send_packet(hoststream,
					tn5250_buffer_length(&tbuf),
					header,
					tn5250_buffer_data(&tbuf));
	      tn5250_buffer_init(&tbuf);
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
		  header.h3270.data_type     = TN3270E_TYPE_BIND_IMAGE;
		  header.h3270.request_flag  = 0x00;
		  if(hoststream->options && TN3270E_RESPONSES)
		    header.h3270.response_flag = response_type(verb);
		  else
		    header.h3270.response_flag = TN3270E_NO_RESPONSE;
		  header.h3270.sequence      = 0x0000;

		  tn5250_stream_send_packet(hoststream, 
					    verb->common.lua_data_length,
					    header,read_data);
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
	      header.h3270.data_type = TN3270E_TYPE_UNBIND;
	      header.h3270.request_flag = 0x00;
	      if(hoststream->options && TN3270E_RESPONSES)
		header.h3270.response_flag = response_type(verb);
	      else
		header.h3270.response_flag = TN3270E_NO_RESPONSE;
	      header.h3270.sequence = 0x0000;
	      tn5250_stream_send_packet(hoststream, 1, header, &reason);
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
	  /*	  action = 1;*/
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
}

/****f* tn3270d/send_response
 * NAME
 *    send_response
 * SYNOPSIS
 *    send_response(sense, verb)
 * INPUT
 *    unsigned long *sense - sense code
 *    LUA_VERB_RECORD verb - verb we use for response
 * DESCRIPTION
 *    This function sends an SNA reponse to the host.  sense will either 
 * contain 0 (no error) , or the appropriate sense code.  The verb is passed
 * as a parameter because it already has most of the fields set appropriately.
 * We write the response on the same flows that are set in the original verb.
 ****/
void 
send_response(unsigned long *sense, LUA_VERB_RECORD * verb)
{

  verb->common.lua_opcode           = LUA_OPCODE_RUI_WRITE;
  verb->common.lua_correlator       = __LINE__;
  verb->common.lua_max_length       = 0;
  verb->common.lua_post_handle      = (unsigned long) response_done;
  verb->common.lua_rh.rri           = 1;
  verb->common.lua_flag1.lu_norm    = 0;
  verb->common.lua_flag1.lu_exp     = 0;
  verb->common.lua_flag1.sscp_norm  = 0;
  verb->common.lua_flag1.sscp_exp   = 0;
  verb->common.lua_flag1.bid_enable = 0;
  verb->common.lua_flag1.nowait     = 0;
  verb->common.lua_flag2.bid_enable = 0;

  if (*sense != 0l) 
    {
      verb->common.lua_data_length = 4;
      memcpy(read_data, sense, 4);
      verb->common.lua_rh.ri = 1;
    } 
  else 
    {
      verb->common.lua_data_length = 0;
      verb->common.lua_rh.ri = 0;
    }

  if (verb->common.lua_flag2.lu_norm) 
    {
      verb->common.lua_flag1.lu_norm = 1;
    } 
  else if (verb->common.lua_flag2.lu_exp) 
    {
      verb->common.lua_flag1.lu_exp = 1;
    } 
  else if (verb->common.lua_flag2.sscp_norm) 
    {
      verb->common.lua_flag1.sscp_norm = 1;
    } 
  else if (verb->common.lua_flag2.sscp_exp) 
    {
      verb->common.lua_flag1.sscp_exp = 1;
    }

  RUI(verb);

  if (verb->common.lua_flag2.async == 0)
    {
      response_done(verb);
    }
} 

void 
client_response(unsigned long sense, int sequence)
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

  if (sense != 0l) 
    {
      client_verb.common.lua_data_length = 4;
      memcpy(read_data, &sense, 4);
      client_verb.common.lua_rh.ri = 1;
    } 
  else 
    {
      client_verb.common.lua_data_length = 0;
      client_verb.common.lua_rh.ri = 0;
    }

  RUI(&client_verb);

  if (client_verb.common.lua_flag2.async == 0)
    {
      response_done(&client_verb);
    }
} 

/****f* tn3270d/response_done
 * NAME
 *    response_done
 * SYNOPSIS
 *    response_done(verb)
 * INPUT
 *    LUA_VERB_RECORD verb - Verb used when sending response
 * DESCRIPTION
 *    Callback function for send_response.  If the response write failed due
 * to a session failure, we try to reinitialize the session, otherwise, we 
 * shutdown.
 *****/
void 
response_done(LUA_VERB_RECORD * verb)
{

  if (verb->common.lua_prim_rc != LUA_OK)
    {

      syslog(LOG_INFO, "WRITE for response failed, (%4.4x, %8.8x)",
	     verb->common.lua_prim_rc, verb->common.lua_sec_rc);
      if (verb->common.lua_prim_rc != LUA_SESSION_FAILURE)
	{
	  close_session();
	}
      else
	{
	  send_reinit();
	}
    }
  else
    {
      send_read();
    }
} 

/****f* tn3270d/close_session()
 * NAME
 *    close_session
 * SYNOPSIS
 *    close_session()
 * INPUT
 *    NONE
 * DESCRIPTION
 *    Send the TERM verb to close down the session.  The function first tries
 * to purge any outstanding verbs.
 *****/
void 
close_session()
{
  /* Purge outstanding verbs. */
  send_purges();

  if (terminating == 0)
  {
    terminating = 1;
    syslog(LOG_INFO, "Terminating session.");
    if (send_verb((unsigned int) LUA_OPCODE_RUI_TERM))
      {
	_exit(1);
      }
    _exit(0);
  }
} 

/****f* tn3270d/send_reinit()
 * NAME
 *    send_reinit
 * SYNOPSIS
 *    send_reinit()
 * INPUT
 *    NONE
 * DESCRIPTION
 *    Sends the REINIT verb.
 *****/
void 
send_reinit()
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
    {
      reinit_done(&reinit_verb);
    }
} 

/****f* tn3270d/reinit_done
 * NAME
 *    reinit_done
 * SYNOPSIS
 *    reinit_done(verb)
 * INPUT
 *    LUA_VERB_RECORD * verb - verb from send_reinit
 * DESCRIPTION
 *    This is the callback function for send_reinit.  If the send_reinit was
 * successful, the function restarts the read cycle.  Otherwise, it will call
 * send_reinit until our maximum number of retries is met. 
 *****/
void 
reinit_done(LUA_VERB_RECORD * verb)
{

  if (verb->common.lua_prim_rc == LUA_OK)
    {
      /* Reinit was successful, start the read cycle again. */
      retry_reinit = saved_retry_reinit;
      syslog(LOG_INFO, "LU RE active");
      send_read();
    }
  else
    {
      /* Otherwise retry until we reach our retry limit */
      syslog(LOG_INFO, "REINIT failed, (%4.4x, %8.8x)\n",
	     verb->common.lua_prim_rc, verb->common.lua_sec_rc);

      if (retry_reinit != 0)
	{
	  if (retry_reinit > 0) 
	    retry_reinit--;
	  send_reinit();
	}
      else
	{
	  _exit(1);
	}
    }
} 

/****f* tn3270d/send_verb
 * NAME
 *    send_verb
 * SYNOPSIS
 *    int send_verb(type)
 * INPUT
 *    unsigned int type - the type of verb to send.
 * DESCRIPTION
 *    Generic verb sending function.  Sends the verb type passed in the type 
 * parameter.  The INIT verb is handled specially in order to specify the
 * LU pool name to use.  The function returns the SNA return code.
 *****/
int 
send_verb(unsigned int type)
{

  memset(&other_verb, 0, sizeof(other_verb));
  other_verb.common.lua_verb        = LUA_VERB_RUI;
  other_verb.common.lua_verb_length = sizeof(other_verb);
  other_verb.common.lua_opcode      = type;
  other_verb.common.lua_correlator  = __LINE__;

  if (type == LUA_OPCODE_RUI_INIT) 
    {
      memset(other_verb.common.lua_luname, ' ', 8);
      memcpy(other_verb.common.lua_luname, poolname, strlen(poolname) );
    }

  other_verb.common.lua_post_handle = (unsigned long) other_done;
  other_verb.common.lua_flag2.async = 1;
  semaphore = 1;

  RUI(&other_verb);

  if (other_verb.common.lua_flag2.async) 
    {
      /* executing asynchronously.  Wait for result. */
	 do_select(0, 0);
    } 
  else 
    {
    /* Finished immediately.  Execute the callback function. */
       other_done(&other_verb);
    }

  return(other_verb.common.lua_prim_rc != LUA_OK);

} 

/****f* tn3270d/other_done
 * NAME
 *    other_done
 * SYNOPSIS
 *    other_done(verb)
 * INPUT
 *    LUA_VERB_RECORD * verb - verb used during generic verb send
 * DESCRIPTION
 *    This is the callback function for the generic send_verb function.
 *****/
void 
other_done(LUA_VERB_RECORD * verb)
{

  syslog(LOG_INFO, "Other done");
  switch (verb->common.lua_opcode)
    {
    case LUA_OPCODE_RUI_INIT:
      /* INIT verb.  Save the lu name. */
      if (verb->common.lua_prim_rc == LUA_OK)
	{
	  sid = other_verb.common.lua_sid;
	  memcpy(lu_name, other_verb.common.lua_luname, 8);
	  lu_name[8] = "/0";
	  syslog(LOG_INFO, "LU active on %s\n", lu_name);
	}
      else
	{
	  syslog(LOG_INFO, "INIT failed, (%4.4x, %8.8x)\n",
		 verb->common.lua_prim_rc, verb->common.lua_sec_rc);
	}
      break;
      
    case LUA_OPCODE_RUI_PURGE:
      /* PURGE verb. */
      if (verb->common.lua_prim_rc != LUA_OK)
	{
	  syslog(LOG_INFO, "PURGE failed, (%4.4x, %8.8x)\n",
		 verb->common.lua_prim_rc, verb->common.lua_sec_rc);
	}
      break;
      
    case LUA_OPCODE_RUI_WRITE:
      /* WRITE verb. */
      if (verb->common.lua_prim_rc != LUA_OK)
	{
	  syslog(LOG_INFO, "WRITE failed, (%4.4x, %8.8x)\n",
		 verb->common.lua_prim_rc, verb->common.lua_sec_rc);
	}
      break;
      
    case LUA_OPCODE_RUI_BID:
      /* BID verb.  If successful, start the read cycle. */
      if (verb->common.lua_prim_rc == LUA_OK)
	{
	  send_read(); 
	}
      else
	{
	  syslog(LOG_INFO, "BID failed, (%4.4x, %8.8x)\n",
		 verb->common.lua_prim_rc, verb->common.lua_sec_rc);
	  _exit(0);
	}
      break;
      
    case LUA_OPCODE_RUI_READ:
      /* READ verb.  If successful execute the read callback */
      if (verb->common.lua_prim_rc == LUA_OK)
	{
	  read_finished(verb); 
	}
      break;
      
    case LUA_OPCODE_RUI_TERM:
      /* TERM verb.*/
      syslog(LOG_INFO, "Terminated\n");
      break;
      
    }

  semaphore = 0;

}

/****f* tn3270d/send_read
 * NAME
 *    send_read
 * SYNOPSIS
 *    send_read()
 * INPUT
 *    NONE
 * DESCRIPTION
 *    Sends a read verb.
 *****/
void 
send_read()
{


  memset(&read_verb, 0, sizeof(read_verb));
  memset(read_data, 0, DATASIZE);
  read_verb.common.lua_verb             = LUA_VERB_RUI;
  read_verb.common.lua_verb_length      = sizeof(read_verb);
  read_verb.common.lua_opcode           = LUA_OPCODE_RUI_READ;
  read_verb.common.lua_correlator       = __LINE__;
  read_verb.common.lua_sid              = sid;
  read_verb.common.lua_max_length       = DATASIZE;
  read_verb.common.lua_data_ptr         = (char *) read_data;
  read_verb.common.lua_post_handle      = (unsigned long) read_finished;
  read_verb.common.lua_flag1.lu_norm    = 1;
  read_verb.common.lua_flag1.lu_exp     = 1;
  read_verb.common.lua_flag1.sscp_norm  = 1;
  read_verb.common.lua_flag1.sscp_exp   = 1;


  RUI(&read_verb);

  if (read_verb.common.lua_flag2.async == 0)
    {
      read_finished(&read_verb);
    }
} 

/****f* tn3270d/send_write
 * NAME
 *    send_write
 * SYNOPSIS
 *    send_write(record)
 * INPUTS
 *    Tn5250Record * record - record containing the 3270 data stream to send.
 * DESCRIPTION
 *    This function sends a write verb.  The data stream is extracted from
 * the input record.
 *****/
void send_write(int streamtype, unsigned char * data, int len)
{

  int ok;

  if(streamtype == TN3270E_STREAM)
    {
      data += 5;

      len = len-5;
    } 

  ok  = 1;
  memset(&other_verb, 0, sizeof(other_verb));
  other_verb.common.lua_verb        = LUA_VERB_RUI;
  other_verb.common.lua_verb_length = sizeof(other_verb);
  other_verb.common.lua_opcode      = LUA_OPCODE_RUI_WRITE;
  other_verb.common.lua_correlator  = __LINE__;
  other_verb.common.lua_sid         = sid;
  other_verb.common.lua_data_length = len;
  other_verb.common.lua_data_ptr    = (char *)data;
  other_verb.common.lua_post_handle = (unsigned long) other_done;
  other_verb.common.lua_rh.bci      = 1;
  other_verb.common.lua_rh.eci      = 1;
  other_verb.common.lua_rh.dr1i     = 1;
	    
  if (lu_session) 
    {
      /* In the lu flow */
      other_verb.common.lua_flag1.lu_norm  = 1;
      other_verb.common.lua_rh.ri          = 1;
      if (send_state == BETB) 
	{
	  other_verb.common.lua_rh.bbi = 1;
	  other_verb.common.lua_rh.cdi = 1;
	  send_state = RECV;
	} 
      else if (send_state == SEND) 
	{
	  other_verb.common.lua_rh.cdi = 1;
	  send_state = RECV;
	} 
      else 
	{
	  syslog(LOG_INFO, "Wait");
	  ok = 0;
	}
    } 
  else 
    {
      /* In the SSCP flow */
      other_verb.common.lua_flag1.sscp_norm  = 1;
    }
  
  /* If it's ok to send */
  if (ok) 
    {
      semaphore = 1;
      RUI(&other_verb);
      if (other_verb.common.lua_flag2.async) 
	{
	  /* Executing asynchronously.  Wait for the result. */
	  do_select(0, 0);
	} 
      else 
	{
	  /* Finished immediately.  Execute the callback */
	  other_done(&other_verb);
	}
    }
  
}

/****f* tn3270d/process_client
 * NAME
 *    process_client
 * SYNOPSIS
 *    process_client(sockfd)
 * INPUTS
 *    int sockfd - file descriptor for the client socket.
 * DESCRIPTION
 *    Processes a new tn3270 client.  This function is invoked once for each
 * client tn3270 connection.  Once the telnet negotiation is successful, it
 * attempts to establish an LU-2 session to the host.  It then loops, acting
 * as a gateway and passing 3270 datastream packets between the telnet and LU-2
 * sides.
 *****/
void
process_client(int sockfd)
{

  Tn5250Record * record;

  /* Perform the telnet negotiation */
  hoststream = tn5250_stream_host(sockfd, 0, TN3270_STREAM);

  if(hoststream != NULL) 
    {
      /* Create a new host object for this session. */
      host = tn5250_host_new(hoststream);
      syslog(LOG_INFO, "Successful connection\n");
      
      /* Use application schedualing */
      SNA_USE_FD_SCHED();

      /* Initialize an empty 5250 buffer to hold 3270 packets */
      tn5250_buffer_init(&tbuf);

      /* Attempt to connect to the host via LU-2 */
      if (send_verb((unsigned int) LUA_OPCODE_RUI_INIT)) 
	{
	  syslog(LOG_INFO, "Session init failed");
	  _exit(1);
	}

      syslog(LOG_INFO, "SNA Session established\n");

      /* Start the SNA read cycle */
      send_read();

      while (terminating == 0) 
	{

	  /* Check for data on both the telnet and SNA sockets */
	  if (do_select(1, sockfd) == 1) 
	    {

	      if (!(tn5250_stream_handle_receive(hoststream))) 
		{
		  /* We got disconnected or something. */
		  syslog(LOG_INFO, "Client disconnected");
		  close_session();
		  _exit(1);
		}


	      /* Pass the data from the telnet session to the host */
	      if (hoststream->record_count > 0) 
		{
		  unsigned char * data;
		  int len;
		  int response;
		  int response_flag;
		  unsigned long sense = 0l;

		  sense = tn3270e_response(data);
		  record = tn5250_stream_get_record(hoststream);
		  data = tn5250_record_data(record);
		  len = tn5250_record_length(record);

		  if(hoststream->streamtype == TN3270E_STREAM)
		    {
		      switch(tn3270e_type(data))
			{
			case TN3270E_TYPE_3270_DATA:
			  syslog(LOG_INFO, "Sending write");
			  send_write(hoststream->streamtype, data, len);
			  break;
			case TN3270E_TYPE_RESPONSE:
			  response_flag = tn3270e_response_flag(data);
			  response = tn3270e_response(data);
			  
			  if(response_flag == TN3270E_POSITIVE_RESPONSE)
			    {
			      sense = tn3270e_response(data);
			    }
			  else
			    {
			      switch(response)
				{
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
		    }
		  else
		    {
		      send_write(hoststream->streamtype, data, len);
		    }

		}
	    }
	}      
      tn5250_host_destroy(host);
      _exit(0);
    } 
  else 
    {
      syslog(LOG_INFO, "Connection failed.\n");
      _exit(1);
    }
}

/****f*
 * NAME
 *    process_client_test
 * SYNOPSIS
 *    process_client_test(sockfd)
 * INPUTS
 *    int sockfd - client telnet socket
 * DESCRIPTION
 *    This function implements a test 'host'.  tn3270 clients can connect to
 * the daemon when this function is used and receive a test screen.  This 
 * function is currently broken for Hummingbird, although it works for x3270.
 *****/
void
process_client_test(int sockfd)
{
  Tn5250Stream * hoststream;
  Tn5250Host * host;
  int aidkey;
  
  /* Perform the telnet negotiation */
  hoststream = tn5250_stream_host(sockfd, 0, TN3270_STREAM);
  
  if(hoststream != NULL) 
    {
      /* Create a new host object for this session */
      host = tn5250_host_new(hoststream);
      syslog(LOG_INFO, "Successful connection\n");
      
      /* Send a test screen and wait for an AID key to be pressed */
      aidkey = SendTestScreen(host); 

      tn5250_host_destroy(host);
      syslog(LOG_INFO, "aidkey = %d\n", aidkey); 
      _exit(0);
    } 
  else 
    {
      syslog(LOG_INFO, "Connection failed.\n");
      _exit(1);
    }
}

/*
  Signal handler for SIGHUP.  We simply set a flag that indicating that the
  configuration file should be reread at the next opportunity.
*/
void
sig_hup(int signum)
{
  reconfig = 1;

  return;
}

void
reload_config()
{
  GSList * iter;
  int rc;

  syslog(LOG_INFO, "Reloading configuration file.");

  tn5250_config_unref(config);

  config = tn5250_config_new();

  /* Read in application defaults */
		
  rc = tn5250_config_load(config, "/home/mmadore/tn3270d.conf");

  if (rc > 0) 
    {
      syslog(LOG_INFO, "%s", strerror(rc));
      syslog(LOG_INFO, "Could not read configuration file.");
      tn5250_config_unref(config);
      exit(1);
    }
  
  poolname = tn5250_config_get(config, "poolname");

  if(poolname == NULL) {
    syslog(LOG_INFO, "No LU pool name specified.  Terminating.");
    exit(1); 
  }

  /* Build a list of allowed clients.  This list belongs to tn5250_config!  */
  addrlist = (GSList *)tn5250_config_get(config, "allowed");

  iter = permitlist;

  while(iter != NULL)
    {
      g_free(iter->data);
      iter = g_slist_next(iter);
    }
  g_slist_free(permitlist);
  
  permitlist = build_addr_list(addrlist);
  
}

int
main(void)
{
  fd_set rset;                  /* File handle set to listen on  */
  int childpid;                 /* Process id of client handler  */
  struct sockaddr_in sn = { AF_INET }; 
  int clientport;               /* tn3270 port                   */
  int manageport;               /* Management port               */
  int emulate_sysreq;           /* False to use USS              */
  int sock;                     /* Main telnet socket (parent)   */
  int mgr_sock;                 /* Management port for parent    */
  int sockfd;                   /* Child telnet socket           */
  size_t snsize;                /* Size of sockaddr structure    */
  
  struct sigaction sa;
  

  sa.sa_handler = sig_hup;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  sigaction(SIGHUP, &sa, NULL);
  
  /* Create a new configuration object */
  config = tn5250_config_new();

  /* Read in application defaults */
  if (tn5250_config_load(config, "tn3270d.conf") == -1) 
    {
      tn5250_config_unref(config);
      exit(1);
    }

  /* If not specified in the configuration file, set default ports */

  clientport = tn5250_config_get_int(config, "clientport");
  if(clientport == 0) 
    {
      clientport = DEFAULT_CLIENT_PORT;
    }

  manageport = tn5250_config_get_int(config, "manageport");
  if(manageport == 0) 
    {
      manageport = DEFAULT_MANAGE_PORT;
    }

  poolname = tn5250_config_get(config, "poolname");

  if(poolname == NULL) {
    printf("No LU pool name specified.  Terminating.");
    exit(1);
  }

  emulate_sysreq = tn5250_config_get_int(config, "emulate_sysreq");

  /* Build a list of allowed clients */
  addrlist = (GSList *)tn5250_config_get(config, "allowed");

  permitlist = build_addr_list(addrlist);

  printf("Starting tn3270d server...\n");
  
  printf("Client port     = %d\n", clientport);
  printf("Management port = %d\n", manageport);
  printf("LU Pool = %s\n", poolname);
  if(emulate_sysreq)
    {
      printf("Emulating sysrequest\n");
    }
  else
    {
      printf("Using real sysrequest\n");
    }

  /* Become a daemon */
  tn5250_daemon(0,0,1);

  sock = tn5250_make_socket (clientport);
  mgr_sock = tn5250_make_socket(manageport);

  /* Create the client socket and set it up to accept connections. */
  if (listen (sock, 1) < 0)
    {
      syslog(LOG_INFO, "listen (CLIENT_PORT): %s\n", strerror(errno));
      exit (EXIT_FAILURE);
    }

  /* Create the manager socket and set it up to accept connections. */
  if (listen (mgr_sock, 1) < 0)
    {
      syslog(LOG_INFO, "listen (MANAGER_PORT): %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }

  syslog(LOG_INFO, "tn3270d server started\n");
  
  /* Process client connections */
  while(1) 
    {
      FD_ZERO(&rset);
      FD_SET(sock,&rset);
      
      if( select(sock+1, &rset, NULL, NULL, NULL) < 0 ) 
	{
	  if( errno == EINTR ) 
	    {
	      continue;
	    } 
	  else 
	    {
	      syslog(LOG_INFO, "select: %s\n", strerror(errno));
	      exit(1);
	    }
	}

      snsize = sizeof(sn);
      
      sockfd = accept(sock, (struct sockaddr *)&sn, &snsize);

      if(sockfd < 0) 
	{
	switch(errno) 
	  {
	  case EINTR:
	  case EWOULDBLOCK:
	  case ECONNABORTED:
	    continue;
	  default:
	    syslog(LOG_INFO, "accept: %s\n", strerror(errno));
	    exit(1);
	  }
	} 

      if(reconfig) 
	{
	  reload_config();
	}

      if(valid_client(permitlist, sn.sin_addr.s_addr)) 
	{
	  syslog(LOG_INFO, "Accepting connection from %s\n", 
		 inet_ntoa(sn.sin_addr));
	} 
      else 
	{
	  syslog(LOG_INFO, "Rejecting connection from %s\n",
		 inet_ntoa(sn.sin_addr));
	  close(sockfd);
	  continue;
	}

      if( (childpid = fork()) < 0) 
	{
	  perror("fork");
	} 
      else if(childpid > 0) 
	{
	  close(sockfd);

	} 
      else 
	{
	  close(sock);
	  process_client(sockfd);
	}
    }
  
}

