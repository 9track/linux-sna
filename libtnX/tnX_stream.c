/* tnX_stream.c: generic tn stream functions.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <glib.h>
#include <netinet/in.h>

/* our stuff. */
#include "libtnX.h"

#define LAST_ERROR              (errno)
#define ERR_INTR                EINTR
#define ERR_AGAIN               EAGAIN
#define WAS_ERROR_RET(r)        ((r) < 0)
#define WAS_INVAL_SOCK(r)       ((r) < 0)

#define SEND    1
#define IS      0
#define INFO    2
#define VALUE   1
#define VAR     0
#define VALUE   1
#define USERVAR 3

#define TERMINAL 1
#define BINARY   2
#define RECORD   4
#define DONE     7
#define HOST     8

#define TRANSMIT_BINARY 0
#define END_OF_RECORD   25
#define TERMINAL_TYPE   24
#define TIMING_MARK     6
#define NEW_ENVIRON     39

#define TN3270E         40

/* Sub-Options for TN3270E negotiation */
#define TN3270E_ASSOCIATE   0
#define TN3270E_CONNECT     1
#define TN3270E_DEVICE_TYPE 2
#define TN3270E_FUNCTIONS   3
#define TN3270E_IS          4
#define TN3270E_REASON      5
#define TN3270E_REJECT      6
#define TN3270E_REQUEST     7
#define TN3270E_SEND        8

/* Reason codes for TN3270E negotiation */
#define TN3270E_CONN_PARTNER    0
#define TN3270E_DEVICE_IN_USE   1
#define TN3270E_INV_ASSOCIATE   2
#define TN3270E_INV_NAME        3
#define TN3270E_INV_DEVICE_TYPE 4
#define TN3270E_TYPE_NAME_ERROR 5
#define TN3270E_UNKNOWN_ERROR   6
#define TN3270E_UNSUPPORTED_REQ 7

/* Function names for TN3270E FUNCTIONS sub-option */
#define TN3270E_BIND_IMAGE      0
#define TN3270E_DATA_STREAM_CTL 1
#define TN3270E_RESPONSES       2
#define TN3270E_SCS_CTL_CODES   3
#define TN3270E_SYSREQ          4

#define EOR  239
#define SE   240
#define SB   250
#define WILL 251
#define WONT 252
#define DO   253
#define DONT 254
#define IAC  255

#define TN5250_STREAM_STATE_NO_DATA     0       /* Dummy state */
#define TN5250_STREAM_STATE_DATA        1
#define TN5250_STREAM_STATE_HAVE_IAC    2
#define TN5250_STREAM_STATE_HAVE_VERB   3       /* e.g. DO, DONT, WILL, WONT */
#define TN5250_STREAM_STATE_HAVE_SB     4       /* SB data */
#define TN5250_STREAM_STATE_HAVE_SB_IAC 5

/* Internal Telnet option settings (bit-wise flags) */
#define RECV_BINARY     1
#define SEND_BINARY     2
#define RECV_EOR        4
#define SEND_EOR        8

static const unsigned char SB_Str_NewEnv[]={IAC, SB, NEW_ENVIRON, SEND, USERVAR,
        'I','B','M','R','S','E','E','D', 0,1,2,3,4,5,6,7,
        VAR, USERVAR, IAC, SE};
static const unsigned char SB_Str_TermType[]={IAC, SB, TERMINAL_TYPE, SEND, IAC, SE};

static unsigned char hostInitStr[] 	= { IAC,DO,NEW_ENVIRON,IAC,DO,TERMINAL_TYPE };
static unsigned char hostDoEOR[] 	= { IAC,DO,END_OF_RECORD };
static unsigned char hostDoBinary[] 	= { IAC,DO,TRANSMIT_BINARY };

typedef struct doTable_t {
   unsigned char       *cmd;
   unsigned     len;
} DOTABLE;

static const DOTABLE hostDoTable[] = {
        { hostInitStr,    sizeof(hostInitStr) 	}, 
        { hostDoEOR,      sizeof(hostDoEOR)	},
        { hostDoBinary,   sizeof(hostDoBinary)	},
        { NULL,           0			}
};

void tnX_stream_setenv(tnXstream *This, const char *name, const char *value)
{
   	char *name_buf;
   	if (This->config == NULL) {
      		This->config = tnX_config_new();
      		tnX_assert(This->config != NULL);
   	}
   	name_buf = (char*)g_malloc (strlen (name) + 10);
   	strcpy(name_buf, "env.");
   	strcat(name_buf, name);
   	tnX_config_set(This->config, name_buf, CONFIG_STRING, (gpointer)value);
   	g_free(name_buf);
	return;
}

const char *tnX_stream_getenv(tnXstream *This, const char *name)
{
   	char *name_buf;
   	const char *val;

   	if (This->config == NULL)
      		return NULL;
   	name_buf = (char*)g_malloc(strlen(name) + 10);
   	strcpy (name_buf, "env.");
   	strcat (name_buf, name);
   	val = tnX_config_get(This->config, name_buf);
   	g_free (name_buf);
   	return val;
}

static void telnet_stream_write(tnXstream *This, unsigned char *data, int size)
{
   	int r;
   	int last_error = 0;
   	fd_set fdw;

   	/* There was an easier way to do this, but I can't remember.  This
   	 * just makes sure that non blocking writes that don't have enough
   	 * buffer space get completed anyway. 
   	 */
   	do {
      		FD_ZERO(&fdw);
      		FD_SET(This->sockfd, &fdw);
      		r = select(This->sockfd + 1, NULL, &fdw, NULL, NULL);
      		if (WAS_ERROR_RET(r)) {
         		last_error = LAST_ERROR;
         		switch (last_error) {
         			case ERR_INTR:
         			case ERR_AGAIN:
            				r = 0;
            				continue;
         			default:
            				perror("select");
            				exit(5);
         		}
      		}
      		if (FD_ISSET(This->sockfd, &fdw)) {
         		r = send(This->sockfd, (char *) data, size, 0);
         		if (WAS_ERROR_RET(r)) {
            			last_error = LAST_ERROR;
            			if (last_error != ERR_AGAIN) {
               				perror("Error writing to socket");
               				exit(5);
            			}
         		}
         		if (r > 0) {
            			data += r;
            			size -= r;
         		}
      		}
   	} while (size && (r >= 0 || last_error == ERR_AGAIN));
	return;
}

static int telnet_stream_get_next(tnXstream *This)
{
   	unsigned char curchar;
   	int rc;
   	fd_set fdr;
   	struct timeval tv;

   	FD_ZERO(&fdr);
   	FD_SET(This->sockfd, &fdr);
   	tv.tv_sec  = This->msec_wait/1000;
   	tv.tv_usec = (This->msec_wait%1000)*1000;
   	select(This->sockfd + 1, &fdr, NULL, NULL, &tv);
   	if (!FD_ISSET(This->sockfd, &fdr))
      		return -1;                /* No data on socket. */

   	rc = recv(This->sockfd, (char *) &curchar, 1, 0);
   	if (WAS_ERROR_RET(rc)) {
      		if (LAST_ERROR != ERR_AGAIN && LAST_ERROR != ERR_INTR) {
         		printf("Error reading from socket: %s\n", strerror(LAST_ERROR));
         		return -2;
      		} else
         		return -1;
   	}
   	/* If the socket was readable, but there is no data, that means that we
   	 * have been disconnected. 
   	 */
   	if (rc == 0)
      		return -2;
   	return (int)curchar;
}

static int sendWill(int sock, unsigned char what)
{
   	static unsigned char buff[3] = { IAC, WILL };

   	buff[2] = what;
   	return send(sock, buff, 3, 0);
}

static int telnet_stream_host_verb(int sock, unsigned char verb,
	unsigned char what)
{
   	int len, option=0, retval=0;

   	// IACVERB_LOG("GotVerb(1)",verb,what);
   	switch (verb) {
      		case DO:
        		switch (what) {
           			case END_OF_RECORD:
                			option = SEND_EOR;
                			break;
           			case TRANSMIT_BINARY:
                			option = SEND_BINARY;
                			break;
           			default:
                			break;
        		} 
        		break;
      		case DONT:
      		case WONT:
        		break;
      		case WILL:
        		switch (what) {
           			case NEW_ENVIRON:
                			len = sizeof(SB_Str_NewEnv);
                			tnX_log(("Sending SB NewEnv..\n"));
                			retval = send(sock, SB_Str_NewEnv, len, 0);
                			break;
           			case TERMINAL_TYPE:
                			len = sizeof(SB_Str_TermType);
                			tnX_log(("Sending SB TermType..\n"));
                			retval = send(sock, SB_Str_TermType, len, 0);
                			break;
           			case END_OF_RECORD:
                			option = RECV_EOR;
                			retval = sendWill(sock, what);
                			break;
           			case TRANSMIT_BINARY:
                			option = RECV_BINARY;
                			retval = sendWill(sock, what);
                			break;
           			default:
                			break;
        		}
        		break;
      		default:
        		break;
   	}

   	return WAS_ERROR_RET(retval) ? retval : option;
}

static void telnet_stream_do_verb(tnXstream *This, unsigned char verb, unsigned char what)
{
   	unsigned char reply[3];
   	int ret;

   	// IACVERB_LOG("GotVerb(2)", verb, what);
   	reply[0] = IAC;
   	reply[2] = what;
   	switch (verb) {
   		case DO:
      			switch (what) {
      				case TERMINAL_TYPE:
      				case END_OF_RECORD:
      				case TRANSMIT_BINARY:
      				case NEW_ENVIRON:
         				reply[1] = WILL;
         				break;
      				default:
         				reply[1] = WONT;
         				break;
      			}
      			break;
   		case DONT:
      			break;
   		case WILL:
      			switch (what) {
      				case TERMINAL_TYPE:
      				case END_OF_RECORD:
      				case TRANSMIT_BINARY:
      				case NEW_ENVIRON:
         				reply[1] = DO;
         				break;
      				case TIMING_MARK:
         				tnX_log(("do_verb: IAC WILL TIMING_MARK received.\n"));
      				default:
         				reply[1] = DONT;
         				break;
      			}
      			break;
   		case WONT:
      			break;
   	}

   	/* We should really keep track of states here, but the code has been
    	 * like this for some time, and no complaints.  
    	 *
    	 * Actually, I don't even remember what that comment means -JMF 
    	 */
   	// IACVERB_LOG("GotVerb(3)",verb,what);
   	ret = send(This->sockfd, (char *)reply, 3, 0);
   	if (WAS_ERROR_RET(ret)) {
      		printf("Error writing to socket: %s\n", strerror(LAST_ERROR));
      		exit(5);
   	}
	return;
}

static void telnet_stream_host_sb(tnXstream *This, unsigned char *sb_buf, int sb_len)
{
   	int i, sbType;
   	tnXbuff tbuf;

   	if (sb_len <= 0)
      		return;
   	tnX_log(("GotSB:<IAC><SB>"));
//   	tnSB_log(sb_buf, sb_len);
   	tnX_log(("<IAC><SE>\n"));
   	sbType = sb_buf[0];
   	sb_buf += 2;  /* Assume IS follows SB option type. */
   	sb_len -= 2;
   	switch (sbType) {
      		case TERMINAL_TYPE:
                	tnX_buffer_init(&tbuf);
                	for (i=0; i<sb_len && sb_buf[i]!=IAC; i++)
                   		tnX_buffer_append_byte(&tbuf, sb_buf[i]);
                	tnX_buffer_append_byte(&tbuf, 0);
                	tnX_stream_setenv(This, "TERM", (char *)tbuf.data);
                	tnX_buffer_free(&tbuf);
                	break;
      		case NEW_ENVIRON:
                	/* TODO:
                 	 * setNewEnvVars(This, sb_buf, sb_len);
                 	 */
                	break;
      		default:
                	break;
   	}
	return;
}

static void telnet_stream_sb_var_value(tnXbuff *buf, unsigned char *var, unsigned char *value)
{
   	tnX_buffer_append_byte(buf, VAR);
   	tnX_buffer_append_data(buf, var, strlen((char *) var));
   	tnX_buffer_append_byte(buf, VALUE);
   	tnX_buffer_append_data(buf, value, strlen((char *) value));
}

static void telnet_stream_sb(tnXstream *This, unsigned char *sb_buf, int sb_len)
{
   	tnXbuff out_buf;
   	int ret;

   	tnX_log(("GotSB:<IAC><SB>"));
   	// TNSB_LOG(sb_buf,sb_len);
   	tnX_log(("<IAC><SE>\n"));

	syslog(LOG_INFO, "stream_sb\n");
   	tnX_buffer_init(&out_buf);
   	if (sb_len <= 0)
      		return;
   	if (sb_buf[0] == TERMINAL_TYPE) {
      		unsigned char *termtype;

      		if (sb_buf[1] != SEND)
         		return;
      		termtype = (unsigned char *)tnX_stream_getenv(This, "TERM");
      		tnX_buffer_append_byte(&out_buf, IAC);
      		tnX_buffer_append_byte(&out_buf, SB);
      		tnX_buffer_append_byte(&out_buf, TERMINAL_TYPE);
      		tnX_buffer_append_byte(&out_buf, IS);
      		tnX_buffer_append_data(&out_buf, termtype, strlen((char *) termtype));
      		tnX_buffer_append_byte(&out_buf, IAC);
      		tnX_buffer_append_byte(&out_buf, SE);

      		ret = send(This->sockfd, (char *)tnX_buffer_data(&out_buf),
                 	tnX_buffer_length(&out_buf), 0);
      		if (WAS_ERROR_RET(ret)) {
         		printf("Error writing to socket: %s\n", strerror(LAST_ERROR));
         		exit(5);
      		}
      		tnX_log(("SentSB:<IAC><SB><TERMTYPE><IS>%s<IAC><SE>\n"));

      		This->status = This->status | TERMINAL;
   	} else {
		if (sb_buf[0] == NEW_ENVIRON) {
     			GSList * iter;
     			tnXconfig_str *data;
     			tnX_buffer_append_byte(&out_buf, IAC);
     			tnX_buffer_append_byte(&out_buf, SB);
     			tnX_buffer_append_byte(&out_buf, NEW_ENVIRON);
     			tnX_buffer_append_byte(&out_buf, IS);
      			if (This->config != NULL) {
         			if ((iter = This->config->vars) != NULL) {
            				do {
              					data = (tnXconfig_str *)iter->data;
               					if (strlen (data->name) > 4 
							&& !memcmp (data->name, "env.", 4)) {
                  					telnet_stream_sb_var_value(&out_buf,
                        					(unsigned char *)data->name + 4,
                        					(unsigned char *)data->value);
               					}
               					iter = g_slist_next(iter);
            				} while (iter != NULL);
         			}
      			}
      			tnX_buffer_append_byte(&out_buf, IAC);
      			tnX_buffer_append_byte(&out_buf, SE);

      			ret = send(This->sockfd, (char *)tnX_buffer_data(&out_buf),
                 		tnX_buffer_length(&out_buf), 0);
      			if (WAS_ERROR_RET(ret)) {
         			printf("Error writing to socket: %s\n", strerror(LAST_ERROR));
         			exit(5);
      			}
      			tnX_log(("SentSB:<IAC><SB>"));
      			// TNSB_LOG(&out_buf.data[2], out_buf.len-4);
      			tnX_log(("<IAC><SE>\n"));
   		}
   		tnX_buffer_free(&out_buf);
	}
	return;
}

static int telnet_stream_get_byte(tnXstream *This)
{
   	int temp;
   	unsigned char verb;

   	do {
      		if (This->state == TN5250_STREAM_STATE_NO_DATA)
         		This->state = TN5250_STREAM_STATE_DATA;
      		temp = telnet_stream_get_next(This);
      		if (temp < 0)
         		return temp;
      		switch (This->state) {
      			case TN5250_STREAM_STATE_DATA:
         			if (temp == IAC)
            				This->state = TN5250_STREAM_STATE_HAVE_IAC;
         			break;
      			case TN5250_STREAM_STATE_HAVE_IAC:
         			switch (temp) {
         				case IAC:
            					This->state = TN5250_STREAM_STATE_DATA;
            					break;
         				case DO:
         				case DONT:
         				case WILL:
         				case WONT:
            					verb = temp;
            					This->state = TN5250_STREAM_STATE_HAVE_VERB;
            					break;
         				case SB:
            					This->state = TN5250_STREAM_STATE_HAVE_SB;
            					tnX_buffer_free(&(This->sb_buf));
            					break;
         				case EOR:
            					This->state = TN5250_STREAM_STATE_DATA;
            					return -END_OF_RECORD;
         				default:
            					tnX_log(("GetByte: unknown escape 0x%02x in telnet stream.\n", temp));
            					This->state = TN5250_STREAM_STATE_NO_DATA;  /* Hopefully a good recovery. */
         			}
         			break;
      			case TN5250_STREAM_STATE_HAVE_VERB:
        			tnX_log(("HOST, This->status  = %d %d\n", HOST, This->status));
         			if (This->status&HOST) {
            				temp = telnet_stream_host_verb(This->sockfd, verb, (unsigned char)temp);
            				if (WAS_ERROR_RET(temp)) {
               					// LOGERROR("send", LAST_ERROR);
               					return -2;
            				}
            				/* Implement later...
            				 *             This->options |= temp;
           				 */
         			} else
            				telnet_stream_do_verb(This, verb, (unsigned char)temp);
         			This->state = TN5250_STREAM_STATE_NO_DATA;
         			break;
      			case TN5250_STREAM_STATE_HAVE_SB:
         			if (temp == IAC)
            				This->state = TN5250_STREAM_STATE_HAVE_SB_IAC;
         			else
           				tnX_buffer_append_byte(&(This->sb_buf), (unsigned char)temp);
         			break;
      			case TN5250_STREAM_STATE_HAVE_SB_IAC:
         			switch (temp) {
         				case IAC:
            					tnX_buffer_append_byte(&(This->sb_buf), IAC);
            					/* Since the IAC code was escaped, shouldn't we be resetting the
						* state as in the following statement?  Please verify and
						* uncomment if applicable.  GJS 2/25/2000 */
			            		/* This->state = TN5250_STREAM_STATE_HAVE_SB; */
            					break;
         				case SE:
            					if (This->status&HOST)
               						telnet_stream_host_sb(This, tnX_buffer_data(&This->sb_buf),
                        					tnX_buffer_length(&This->sb_buf));
            					else
               						telnet_stream_sb(This, tnX_buffer_data(&(This->sb_buf)),
                        					tnX_buffer_length(&(This->sb_buf)));
            					tnX_buffer_free(&(This->sb_buf));
            					This->state = TN5250_STREAM_STATE_NO_DATA;
            					break;

         				default: /* Should never happen -- server error */
            					tnX_log(("GetByte: huh? Got IAC SB 0x%02X.\n", temp));
            					This->state = TN5250_STREAM_STATE_HAVE_SB;
            					break;
         			}
         			break;

      			default:
         			tnX_log(("GetByte: huh? Invalid state %d.\n", This->state));
         			tnX_assert(0);
         			break;
      		}
   	} while (This->state != TN5250_STREAM_STATE_DATA);
   	return (int)temp;
}

static int telnet_stream_handle_receive(tnXstream *This)
{
   	int c;

	syslog(LOG_INFO, "handle_receive\n");
   	/* -1 = no more data, -2 = we've been disconnected */
   	while ((c = telnet_stream_get_byte(This)) != -1 && c != -2) {
		syslog(LOG_INFO, "hr 1\n");
      		if (c == -END_OF_RECORD && This->current_record != NULL) {
			syslog(LOG_INFO, "hr 2\n");
         		/* End of current packet. */
         		tnX_record_dump(This->current_record);
         		This->records = tnX_record_list_add(This->records, This->current_record);
         		This->current_record = NULL;
         		This->record_count++;
         		continue;
      		}
      		if (This->current_record == NULL) {
			syslog(LOG_INFO, "hr 4\n");
         		/* Start of new packet. */
         		This->current_record = tnX_record_new();
      		}
      		tnX_record_append_byte(This->current_record, (unsigned char)c);
   	}
	syslog(LOG_INFO, "hr 56 = %d\n", c);
   	return (c != -2);
}

static void telnet_stream_escape(tnXbuff *in)
{
   	tnXbuff out;
   	register unsigned char c;
   	int n;

   	tnX_buffer_init(&out);
   	for (n = 0; n < tnX_buffer_length(in); n++) {
      		c = tnX_buffer_data(in)[n];
      		tnX_buffer_append_byte(&out, c);
      		if (c == IAC)
         		tnX_buffer_append_byte(&out, IAC);
   	}
   	tnX_buffer_free(in);
   	memcpy(in, &out, sizeof(tnXbuff));
	return;
}

static void tn3270_stream_send_packet(tnXstream *This, int length, int flowtype,
	unsigned char flags, unsigned char opcode, unsigned char *data)
{
	tnXbuff out_buf;

   	tnX_buffer_init(&out_buf);

   	tnX_buffer_append_data(&out_buf, data, length);

   	telnet_stream_escape(&out_buf);

   	tnX_buffer_append_byte(&out_buf, IAC);
   	tnX_buffer_append_byte(&out_buf, EOR);

   	telnet_stream_write(This, tnX_buffer_data(&out_buf),
		tnX_buffer_length(&out_buf));

   	tnX_buffer_free(&out_buf);
	return;
}

static int telnet_stream_accept(tnXstream *This, int masterfd)
{
   	int i, len, retCode;
   	struct sockaddr_in serv_addr;
   	fd_set fdr;
   	struct timeval tv;
   	u_long ioctlarg=1L;

   	/*
   	 * len = sizeof(serv_addr);
   	 * This->sockfd = accept(masterSock, (struct sockaddr *) &serv_addr, &len);
   	 * if (WAS_INVAL_SOCK(This->sockfd)) {
   	 * 	return LAST_ERROR;
   	 * }
   	 */
   	syslog(LOG_INFO, "This->sockfd = %d\n", masterfd);
   	This->sockfd = masterfd;

   	/* Set socket to non-blocking mode. */
   	ioctl(This->sockfd, FIONBIO, &ioctlarg);

   	This->state = TN5250_STREAM_STATE_DATA;
   	This->status = HOST;

   	/* Commence TN5250 negotiations...
   	 * Send DO options (New Environment, Terminal Type, etc.) 
   	 */

   	for (i=0; hostDoTable[i].cmd; i++) {
      		retCode = send(This->sockfd, hostDoTable[i].cmd, hostDoTable[i].len, 0);
      		if (WAS_ERROR_RET(retCode)) {
        		perror("telnetstr");
         		return LAST_ERROR;
      		}

      		FD_ZERO(&fdr);
      		FD_SET(This->sockfd, &fdr);
      		tv.tv_sec = 5;
      		tv.tv_usec = 0;
      		select(This->sockfd + 1, &fdr, NULL, NULL, &tv);
      		if (FD_ISSET(This->sockfd, &fdr)) {
        		if (!telnet_stream_handle_receive(This)) {
        	  		retCode = LAST_ERROR;
        	  		return retCode ? retCode : -1;
        		}
      		} else {
        		return -1;
      		}
   	}
   	return 0;
}

static void telnet_stream_disconnect(tnXstream *This)
{
	printf("Closing...\n");
   	close(This->sockfd);
	return;
}

static void telnet_stream_destroy(tnXstream *This)
{
   	/* noop */
	return;
}

static int tn5250_telnet_stream_init(tnXstream *This)
{
   	This->disconnect 	= telnet_stream_disconnect;
   	This->handle_receive 	= telnet_stream_handle_receive;
//   	This->send_packet 	= telnet_stream_send_packet;
   	This->destroy 		= telnet_stream_destroy;
   	This->streamtype 	= TN5250_STREAM;
   	return 0; 
}

static int tn3270_telnet_stream_init(tnXstream *This)
{
	This->accept		= telnet_stream_accept;
   	This->disconnect 	= telnet_stream_disconnect;
   	This->handle_receive 	= telnet_stream_handle_receive;
   	This->send_packet 	= tn3270_stream_send_packet;
   	This->destroy 		= telnet_stream_destroy;
   	This->streamtype 	= TN3270_STREAM;
   	return 0; 
}

static void streamInit(tnXstream *This, long timeout)
{
   	This->status 		= 0;
   	This->config 		= NULL;
   	This->connect 		= NULL;
   	This->disconnect 	= NULL;
   	This->handle_receive 	= NULL;
   	This->send_packet 	= NULL;
   	This->destroy 		= NULL;
   	This->record_count 	= 0;
   	This->records 		= This->current_record = NULL;
   	This->sockfd 		= (int) - 1;
   	This->msec_wait 	= timeout;
   	This->streamtype 	= TN5250_STREAM;
   	tnX_buffer_init(&(This->sb_buf));
	return;
}

void tnX_stream_destroy(tnXstream *This)
{
   	/* Call particular stream type's destroy handler. */
   	if (This->destroy)
      		(*(This->destroy)) (This);

   	/* Free the environment. */
   	if (This->config != NULL)
      		tnX_config_unref(This->config);
   	tnX_buffer_free(&(This->sb_buf));
   	tnX_record_list_destroy(This->records);
   	g_free(This);
	return;
}

tnXstream *tnX_stream_host(int masterfd, long timeout, int streamtype)
{
   	tnXstream *This = tnX_new(tnXstream, 1);
   	tnXstream_type *iter;
   	const char *postfix;
   	int ret;

   	if (This != NULL) {
		syslog(LOG_INFO, "t1\n");
      		streamInit(This, timeout);
		syslog(LOG_INFO, "t2\n");
      		if(streamtype == TN5250_STREAM) {
          		/* Assume telnet stream type. */
          		ret = tn5250_telnet_stream_init(This);
        	} else {
          		ret = tn3270_telnet_stream_init(This);
        	}
      		if (ret != 0) {
			syslog(LOG_INFO, "t3\n");
         		tnX_stream_destroy(This);
         		return NULL;
      		}
      		/* Accept */
      		syslog(LOG_INFO, "masterfd = %d\n", masterfd);
      		ret = (*(This->accept))(This, masterfd);
      		if (ret == 0)
         		return This;
      		tnX_stream_destroy(This);
   	}
   	return NULL;
}

tnXrec *tnX_stream_get_record(tnXstream *This)
{
   	tnXrec *record;
   	int offset;

   	record = This->records;
   	tnX_assert(This->record_count >= 1);
   	tnX_assert(record != NULL);

   	This->records = tnX_record_list_remove(This->records, record);
   	This->record_count--;

   	if(This->streamtype == TN5250_STREAM) {
       		tnX_assert(tnX_record_length(record)>= 10);
       		offset = 6 + tnX_record_data(record)[6];
     	} else {
       		offset = 0;
     	}

   	tnX_log(("tnX_stream_get_record: offset = %d\n", offset));
   	tnX_record_set_cur_pos(record, offset);
   	return record;
}
