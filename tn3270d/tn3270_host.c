/* host3270.c:
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

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <libtnX.h>
#include <tnX_codes.h>

extern int tn3270_debug_level;

/* our stuff. */
#include "tn3270_codes.h"
#include "tn3270_list.h"
#include "tn3270.h"
#include "tn3270_host.h"

static char ascii_banner[][560] = {
	"                                                                  #####        ",
	"                                                                 #######       ",
	"                    @                                            ##O#O##       ",
	"   ######          @@#                                           #VVVVV#       ",
	"     ##             #                                          ##  VVV  ##     ",
	"     ##         @@@   ### ####   ###    ###  ##### ######     #          ##    ",
	"     ##        @  @#   ###    ##  ##     ##    ###  ##       #            ##   ",
	"     ##       @   @#   ##     ##  ##     ##      ###         #            ###  ",
	"     ##          @@#   ##     ##  ##     ##      ###        QQ#           ##Q  ",
	"     ##       # @@#    ##     ##  ##     ##     ## ##     QQQQQQ#       #QQQQQQ",
	"     ##      ## @@# #  ##     ##  ###   ###    ##   ##    QQQQQQQ#     #QQQQQQQ",
	"   ############  ###  ####   ####   #### ### ##### ######   QQQQQ#######QQQQQ  ",
	"                                                                               ",
	"                                                                               ",
	"                            Linux-SNA TN3270 server                            ",
	"                       (Press an AID key to disconnect)                        "
};

static char *mapname = MAP_DEFAULT;    

tnXhost *tnX_host_new(tnXstream *This)
{
  	tnXhost *newHost = g_new(tnXhost, 1);
  
  	if (newHost != NULL) {
    		memset(newHost, 0, sizeof(tnXhost));
    		newHost->stream = This;
    		tnX_buffer_init(&newHost->buffer);
    		setXCharMap(newHost, mapname);
    		newHost->maxcol = 80;
  	}
  	return newHost;
}

void setXCharMap(tnXhost *This, const char *name)
{
  	tnXchar_map *map = tnX_char_map_new(name);
  	tnX_assert(map != NULL);
  	if (This->map != NULL)
    		tnX_char_map_destroy(This->map);
  	This->map = map;
  	return;
}

void writeToDisplay(tnXhost *This)
{
   	tnXbuff *buff;
   	unsigned char ctrlchar=0;

   	if (This->wtd_set)
      		return;
   	buff = &This->buffer;
   	if (This->inputInhibited && !This->inSysInterrupt) {
      		ctrlchar = TN3270_SESSION_CTL_KEYBOARD_RESTORE;
      		This->inputInhibited = FALSE;
   	}
   	tnX_buffer_append_byte(buff, CMD_3270_WRITE);
   	tnX_buffer_append_byte(buff, ctrlchar);
   	This->clearState 	= FALSE;
   	This->wtd_set 		= TRUE;
   	This->lastattr 		= -1;
	return;
}

void appendBlock2Ebcdic(tnXbuff *buff, 
	unsigned char *str, int len, tnXchar_map *map)
{
   	int i;
   	unsigned char uc;

   	for (uc=str[i=0]; i<len; uc=str[++i]) {
      		if (isprint(uc))
			tnX_buffer_append_byte(buff, 
				tnX_char_map_to_remote(map, uc));
   	}
	return;
}

void sendReadMDT(tnXstream *This, tnXbuff *buff)
{
  	syslog(LOG_INFO, "Sending Read Modified command.");

  	tnX_buffer_init(buff);
  	tnX_buffer_append_byte(buff, CMD_3270_READ_MODIFIED);
  	tnX_stream_send_packet(This, tnX_buffer_length(buff),
		0, 0, 0, tnX_buffer_data(buff));
	return;
}

void StartField(tnXhost *This, tnXbuff *buff, unsigned char attr)
{
  	syslog(LOG_INFO, "Sending Start Field order.");

  	tnX_buffer_append_byte(buff, ORDER_3270_SF);
  	tnX_buffer_append_byte(buff, attr);
	return;
}

void EraseWrite(tnXhost *This)
{
  	tnXbuff *buff;
  	unsigned char ctrlchar;

  	buff = &This->buffer;
  	This->inputInhibited = FALSE;
  	ctrlchar = TN3270_SESSION_CTL_KEYBOARD_RESTORE
    		| TN3270_SESSION_CTL_RESET_MDT
    		| TN3270_SESSION_CTL_RESET;

  	tnX_buffer_append_byte(buff, CMD_3270_ERASE_WRITE);
  	tnX_buffer_append_byte(buff, ctrlchar);
  	This->clearState = FALSE;
  	This->wtd_set = TRUE;
  	This->lastattr = -1;
	return;
}

void setBufferAddr(tnXhost *This, int row, int col)
{
  	unsigned char addr1;
  	unsigned char addr2;
  	unsigned short int address;

  	address = (row) * 80 + (col - 1);

  	addr1 = (address >> 6) | 0xC0;
  	addr2 = address & 0x3F;

   	writeToDisplay(This);
   	tnX_buffer_append_byte(&This->buffer, SBA);
   	tnX_buffer_append_byte(&This->buffer, addr1);
   	tnX_buffer_append_byte(&This->buffer, addr2);
	return;
}

void repeat2Addr(tnXhost *This, int row, int col, unsigned char uc)
{
   	writeToDisplay(This);
   	tnX_buffer_append_byte(&This->buffer, RA);
   	tnX_buffer_append_byte(&This->buffer, (unsigned char) row);
   	tnX_buffer_append_byte(&This->buffer, (unsigned char) col);
   	tnX_buffer_append_byte(&This->buffer, uc);
	return;
}

void setAttribute(tnXhost *This, unsigned short attr)
{
   	This->curattr = attr;
	return;
}

void sendWrite(tnXhost *This)
{
  	tnX_stream_send_packet(This->stream, tnX_buffer_length(&This->buffer),
	   	0, 0, 0, tnX_buffer_data(&This->buffer));
	return;
}

int readMDTfields_1(client_t *client, tnXhost *This, int sendRMF)
{
  	syslog(LOG_INFO, "Reading modified fields.");
  	if (sendRMF) {
    		unsigned char ctrlchar=0;
    		if (This->inputInhibited && !This->inSysInterrupt)
      			ctrlchar = TN3270_SESSION_CTL_KEYBOARD_RESTORE;
    		sendReadMDT(This->stream, &This->buffer);
  	}
  	This->wtd_set = FALSE;

	return 0;
}

int readMDTfields_2(client_t *client, tnXhost *This, int sendRMF)
{
        int hor;
        int ver;
        int aidCode=0;
	
	syslog(LOG_INFO, "readMDTfields_2\n");
	syslog(LOG_INFO, "ok 1\n");
    	if (!(tnX_stream_handle_receive(This->stream))) {
		syslog(LOG_INFO, "got disconnedted or something\n");
      		/* We got disconnected or something. */
      		This->disconnected = TRUE;
      		return -ECONNABORTED;
    	}
	syslog(LOG_INFO, "ok 2\n");
    	if (This->record) {
      		tnX_record_destroy(This->record);
      		This->record = NULL;
    	}
	syslog(LOG_INFO, "ok 3\n");
    	if (This->stream->record_count>0)
      		This->record = tnX_stream_get_record(This->stream);
    	else
		return 0;
	syslog(LOG_INFO, "ok 4\n");
    	ver = tnX_record_get_byte(This->record) - 1;
    	hor = tnX_record_get_byte(This->record) - 1;
    	This->cursorPos = ver*This->maxcol + hor;
    	aidCode = tnX_record_get_byte(This->record);
	if (aidCode)
		return -ECONNABORTED;
  	return aidCode;
}

int processFlags(tnXhost *This, unsigned char flags, unsigned char *buf)
{
   	char *msg;
   	short ecode;
   	tnXstream *myStream = This->stream;

   	switch (flags) {
      		case TN5250_RECORD_H_HLP:
			ecode = (short)buf[0]<<8 | (short)buf[1];
			if (!cancelInvite(myStream))
				return -1;
			msg = getErrMsg(ecode);
			sendWriteErrorCode(This, msg,
				TN5250_RECORD_OPCODE_OUTPUT_ONLY);
			break;
      		case TN5250_RECORD_H_ERR:
			/* Data stream output error */
			msg = processErr(buf);
			sendWriteErrorCode(This, msg,
				TN5250_RECORD_OPCODE_OUTPUT_ONLY);
			return -1;
			break;
      		case TN5250_RECORD_H_TRQ:
			/* Test Request Key */
			break;
      		case TN5250_RECORD_H_ATN:
      		case TN5250_RECORD_H_SRQ:
			if (This->inSysInterrupt)
		   		return 0;
			if (!cancelInvite(myStream))
		   		return -1;
			This->inSysInterrupt 	= TRUE;
			This->wtd_set 		= FALSE;
			This->clearState 	= FALSE;
			tnX_buffer_free(&This->buffer);
			clearScreen(This);
			processSRQ(myStream);
			This->wtd_set 		= FALSE;
			This->clearState 	= FALSE;
			This->inSysInterrupt 	= FALSE;
			tnX_buffer_free(&This->buffer);
			break;
      		default:
			break;
   	}
   	return 0;
}

int cancelInvite(tnXstream *This)
{
   	int statOK;

   	tnX_stream_send_packet(This, 0, TN5250_RECORD_FLOW_DISPLAY, 
		TN5250_RECORD_H_NONE, TN5250_RECORD_OPCODE_CANCEL_INVITE, NULL);
   	if (This->record_count) {
      		This->records = tnX_record_list_destroy(This->records);
      		This->record_count = 0;
   	}
   	/* Get Cancel Invite acknowlegement from client. */
   	do {
      		statOK = (int)(tnX_stream_handle_receive(This));
   	} while (statOK && !This->record_count);
   	if (This->record_count>0) { /* Zap the record(s) */
      		This->records = tnX_record_list_destroy(This->records);
      		This->record_count = 0;
   	}
   	return statOK;
}

char *getErrMsg(short ecode)
{
   	char *errmsg;
   	switch (ecode) {
      		case ERR_DONT_KNOW:
			errmsg = MSG_DONT_KNOW;
			break;
      		case ERR_BYPASS_FIELD:
			errmsg = MSG_BYPASS_FIELD;
			break;
      		case ERR_NO_FIELD:
			errmsg = MSG_NO_FIELD;
			break;
      		case ERR_INVALID_SYSREQ:
			errmsg = MSG_INVALID_SYSREQ;
			break;
      		case ERR_MANDATORY_ENTRY:
			errmsg = MSG_MANDATORY_ENTRY;
			break;
      		case ERR_ALPHA_ONLY:
			errmsg = MSG_ALPHA_ONLY;
			break;
      		case ERR_NUMERIC_ONLY:
			errmsg = MSG_NUMERIC_ONLY;
			break;
      		case ERR_DIGITS_ONLY:
			errmsg = MSG_DIGITS_ONLY;
			break;
      		case ERR_LAST_SIGNED:
			errmsg = MSG_LAST_SIGNED;
			break;
      		case ERR_NO_ROOM:
			errmsg = MSG_NO_ROOM;
			break;
      		case ERR_MANADATORY_FILL:
			errmsg = MSG_MANADATORY_FILL;
			break;
      		case ERR_CHECK_DIGIT:
			errmsg = MSG_CHECK_DIGIT;
			break;
      		case ERR_NOT_SIGNED:
			errmsg = MSG_NOT_SIGNED;
			break;
      		case ERR_EXIT_NOT_VALID:
			errmsg = MSG_EXIT_NOT_VALID;
			break;
      		case ERR_DUP_NOT_ENABLED:
			errmsg = MSG_DUP_NOT_ENABLED;
			break;
      		case ERR_NO_FIELD_EXIT:
			errmsg = MSG_NO_FIELD_EXIT;
			break;
      		case ERR_NO_INPUT:
			errmsg = MSG_NO_INPUT;
			break;
      		case ERR_BAD_CHAR:
			errmsg = MSG_BAD_CHAR;
			break;
#ifdef JAPAN
      		case ERR_DBCS_WRONG_TYPE:
			errmsg = MSG_DBCS_WRONG_TYPE;
			break;
      		case ERR_SBCS_WRONG_TYPE:
			errmsg = MSG_SBCS_WRONG_TYPE;
			break;
#endif
      		default:
			errmsg = MSG_NO_HELP;
			break;
   	}
   	return errmsg;
}

void sendWriteErrorCode(tnXhost *This, char *msg, unsigned char opcode)
{
   	tnXbuff tbuf;

	tnX_buffer_init(&tbuf);
   	tnX_buffer_append_byte(&tbuf, ESC);
   	tnX_buffer_append_byte(&tbuf, CMD_WRITE_ERROR_CODE);
  	hiliteString(&tbuf, msg, This->map);
   	if (opcode==TN5250_RECORD_OPCODE_OUTPUT_ONLY) {
      		tnX_stream_send_packet(This->stream, tnX_buffer_length(&tbuf),
			TN5250_RECORD_FLOW_DISPLAY, TN5250_RECORD_H_NONE, opcode,
			tnX_buffer_data(&tbuf));
      		tbuf.len = 0;
      		opcode = TN5250_RECORD_OPCODE_INVITE;
   	}
   	sendReadMDT(This->stream, &tbuf);
	return;
}

typedef struct {
   	int code;
   	char *msg;
} DSNRTABLE;

static DSNRTABLE dsnrMsgTable[]= {
	{ DSNR_RESEQ_ERR,	EMSG_RESEQ_ERR },
	{ DSNR_INVCURSPOS,	EMSG_INVCURSPOS },
	{ DSNR_RAB4WSA,		EMSG_RAB4WSA },
	{ DSNR_INVSFA,		EMSG_INVSFA },
	{ DSNR_FLDEOD,		EMSG_FLDEOD },
	{ DSNR_FMTOVF,		EMSG_FMTOVF },
	{ DSNR_WRTEOD,		EMSG_WRTEOD },
	{ DSNR_SOHLEN,		EMSG_SOHLEN },
	{ DSNR_ROLLPARM,	EMSG_ROLLPARM },
	{ DSNR_NO_ESC,		EMSG_NO_ESC },
	{ DSNR_INV_WECW,	EMSG_INV_WECW },
	{ DSNR_UNKNOWN,		NULL }
};

char *processErr(unsigned char *buf)
{
   	static char invCmd[]="Invalid command encountered in data stream.",
		unkfmt[]="Unknown data stream error: 0x%04X: %02X %02X",
		mbuf[80]="";
   	short catmod;
   	unsigned char ubyte1;
   	int dsnrCode, i=0;

   	catmod	 = (short)buf[0]<<8 | buf[1];
   	ubyte1 	 = buf[2];
   	dsnrCode = (int) buf[3];
   	if (catmod==0x1003 && ubyte1==1 && dsnrCode==1)
      		return invCmd;
   	if (catmod!=0x1005 || ubyte1!=1) {
      		sprintf(mbuf, unkfmt, catmod, ubyte1, dsnrCode);
      		return mbuf;
   	}
   	while (dsnrMsgTable[i].code!=DSNR_UNKNOWN &&
		dsnrMsgTable[i].code!=dsnrCode)
      		i++;

   	if (dsnrMsgTable[i].code==DSNR_UNKNOWN) {
      		sprintf(mbuf, unkfmt, catmod, ubyte1, dsnrCode);
      		return mbuf;
   	}
   	return dsnrMsgTable[i].msg;
}

void clearScreen(tnXhost * This)
{
   	if (This->clearState)
      		return;
   	if (This->wtd_set)
      		flushTN5250(This);
   	tnX_buffer_append_byte(&This->buffer, ESC);
   	tnX_buffer_append_byte(&This->buffer, CMD_CLEAR_UNIT);
   	This->inputInhibited = 1;
   	This->clearState = 1;
	return;
}

int processSRQ(tnXstream *This)
{
   	tnXrec *record;
   	tnXbuff tbuf;

   	record = saveScreen(This);
   	if (!record)
      		return -1;
   	raise(SIGINT);  /* Generate an interrupt. */
   	restoreScreen(This, &record->data);
   	tnX_record_destroy(record);
   	tnX_buffer_init(&tbuf);
   	sendReadMDT(This, &tbuf);
   	return 0;
}

void hiliteString(tnXbuff *buff, char *str, tnXchar_map *map)
{
   	tnX_buffer_append_byte(buff, ATTR_5250_WHITE);
   	appendBlock2Ebcdic(buff, (unsigned char *)str, strlen(str), map);
   	tnX_buffer_append_byte(buff, ATTR_5250_NORMAL);
	return;
}

void flushTN5250(tnXhost *This)
{
   	if (tnX_buffer_length(&This->buffer)>0) {
      		tnX_stream_send_packet(This->stream,
			tnX_buffer_length(&This->buffer),
			TN5250_RECORD_FLOW_DISPLAY, 
			TN5250_RECORD_H_NONE, TN5250_RECORD_OPCODE_PUT_GET,
			tnX_buffer_data(&This->buffer));
      		tnX_buffer_free(&This->buffer);
   	}
   	if (This->wtd_set)
      		This->wtd_set = 0;
	return;
}

tnXrec *saveScreen(tnXstream *This)
{
   	tnXbuff tbuf;
   	tnXrec *trec;
   	int statOK;

   	tnX_buffer_init(&tbuf);
   	tnX_buffer_append_byte(&tbuf, ESC);
   	tnX_buffer_append_byte(&tbuf, CMD_SAVE_SCREEN);
   	tnX_stream_send_packet(This, 2, TN5250_RECORD_FLOW_DISPLAY, 
		TN5250_RECORD_H_NONE, TN5250_RECORD_OPCODE_SAVE_SCR,
		tbuf.data);
   	tnX_buffer_free(&tbuf);
   	while (This->record_count>0) {
      		trec = tnX_stream_get_record(This);
      		tnX_record_destroy(trec);
   	}
   	do {
      		statOK = (int)(tnX_stream_handle_receive(This));
   	} while (statOK && !This->record_count);
   	if (statOK)
      		return tnX_stream_get_record(This);
   	else
      		return NULL;
}

void restoreScreen(tnXstream *This, tnXbuff *buff)
{
   	int len = tnX_buffer_length(buff);
   	unsigned char *bufp = tnX_buffer_data(buff);
   
   	tnX_assert(buff->data != NULL);
   	tnX_assert(len>10);
   	/* Skip the standard 10 byte header since the send_packet function
      	 * automatically generates/prefixes one for us.   GJS 3/20/2000 */
   	bufp += 10;
   	len  -= 10;

   	tnX_stream_send_packet(This, len,
		TN5250_RECORD_FLOW_DISPLAY, TN5250_RECORD_H_NONE,
		TN5250_RECORD_OPCODE_RESTORE_SCR, bufp);
	return;
}

void tnX_host_destroy(tnXhost *This)
{
   	tnXstream *myStream;

   	if (!This)
      		return;

   	tnX_buffer_free(&This->buffer);
   	if (This->map)
      		tnX_char_map_destroy(This->map);
   	if (This->record)
      		tnX_record_destroy(This->record);
   	if (This->screenRec)
      		tnX_record_destroy(This->screenRec);
   	if (!(myStream = This->stream))
      		return;
   	if ((myStream->sockfd >= 0) && myStream->disconnect) {
       		printf("Disconnecting...\n");
         	tnX_stream_disconnect(myStream);
     	}
   	tnX_stream_destroy(myStream);
	return;
}

int SendTestScreen(client_t *client)
{
  	int currow;

 	syslog(LOG_INFO, "Sending test screen."); 

 	EraseWrite(client->host);
 	for (currow = 0; currow < 16; currow++) {
     		setBufferAddr(client->host, currow+3, 1);
     		appendBlock2Ebcdic(&client->host->buffer, 
			(unsigned char *)ascii_banner[currow], 
			strlen(ascii_banner[currow]),
			client->host->map);
   	}
 	client->host->inputInhibited = client->host->inSysInterrupt = FALSE;
 
 	setBufferAddr(client->host, 1, 1);
 	StartField(client->host, &client->host->buffer, 0);
 	appendBlock2Ebcdic(&client->host->buffer, (unsigned char *)"Test", 4, 
		client->host->map);
 	sendWrite(client->host);
 	return readMDTfields_1(client, client->host, 0);
}
