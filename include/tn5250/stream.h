/* TN5250 - An implementation of the 5250 telnet protocol.
 * Copyright (C) 1997 Michael Madore
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
 * If you do not wish that, delete this exception notice. */
#ifndef STREAM5250_H
#define STREAM5250_H

#if WIN32
# include <winsock.h>  /* Need for SOCKET type.  GJS 3/3/2000 */
#endif 

#ifdef __cplusplus
extern "C" {
#endif

#define TN3270_STREAM 0
#define TN5250_STREAM 1

struct _Tn5250Config;

/****s* lib5250/Tn5250Stream
 * NAME
 *    Tn5250Stream
 * SYNOPSIS
 *    Tn5250Stream *str = tn5250_stream_open ("telnet:my.as400.com");
 *    Tn5250Record *rec;
 *    tn5250_stream_send_packet (str, 0, TN5250_RECORD_FLOW_DISPLAY, 
 *	 TN5250_RECORD_H_NONE, TN5250_RECORD_OPCODE_PUT_GET, NULL);
 *    rec = tn5250_stream_get_record (str);
 *    tn5250_stream_disconnect (str);
 *    tn5250_stream_destroy (str);
 * DESCRIPTION
 *    Tn5250Stream is 'abstract', implementations currently reside in
 *    the telnetstr.c and debug.c source files.  A stream object
 *    manages the communications transport, such as TCP/IP.
 * SOURCE
 */
struct _Tn5250Stream {
   int (* connect) (struct _Tn5250Stream *This, const char *to);
  int (* accept) (struct _Tn5250Stream *This, SOCKET_TYPE masterSock);
   void (* disconnect) (struct _Tn5250Stream *This);
   int (* handle_receive) (struct _Tn5250Stream *This);
   void (* send_packet) (struct _Tn5250Stream *This, int length, int flowtype, unsigned char flags,
	 unsigned char opcode, unsigned char *data);
   void (/*@null@*/ * destroy) (struct _Tn5250Stream /*@only@*/ *This);

   struct _Tn5250Config *config; 

   Tn5250Record /*@null@*/ *records;
   Tn5250Record /*@dependent@*/ /*@null@*/ *current_record;
   int record_count;

   Tn5250Buffer sb_buf;

   SOCKET_TYPE sockfd;
   int status;
   int state;
  int streamtype;
  long msec_wait;

#ifndef NDEBUG
   FILE *debugfile;
#endif
};

typedef struct _Tn5250Stream Tn5250Stream;
/******/

extern Tn5250Stream /*@only@*/ /*@null@*/ *tn5250_stream_open (const char *to);
extern int tn5250_stream_config (Tn5250Stream *This, struct _Tn5250Config *config);
extern void tn5250_stream_destroy(Tn5250Stream /*@only@*/ * This);
extern Tn5250Record /*@only@*/ *tn5250_stream_get_record(Tn5250Stream * This);
extern Tn5250Record /*@only@*/ *tn3270_stream_get_record(Tn5250Stream * This);
extern Tn5250Stream *tn5250_stream_host(SOCKET_TYPE masterSock, long timeout,
					int streamtype);
#define tn5250_stream_connect(This,to) \
   (* (This->connect)) ((This),(to))
#define tn5250_stream_disconnect(This) \
   (* (This->disconnect)) ((This))
#define tn5250_stream_handle_receive(This) \
   (* (This->handle_receive)) ((This))
#define tn5250_stream_send_packet(This,len,flow,flags,opcode,data) \
   (* (This->send_packet)) ((This),(len),(flow),(flags),(opcode),(data))

/* This should be a more flexible replacement for different NEW_ENVIRON
 * strings. */
extern void tn5250_stream_setenv(Tn5250Stream * This, const char *name,
				 const char /*@null@*/ *value);
extern void tn5250_stream_unsetenv(Tn5250Stream * This, const char *name);
extern /*@observer@*/ /*@null@*/ const char *tn5250_stream_getenv(Tn5250Stream * This, const char *name);

#define tn5250_stream_record_count(This) ((This)->record_count)
#define tn5250_stream_socket_handle(This) ((This)->sockfd)

#ifdef __cplusplus
}

#endif
#endif

/* vi:set cindent sts=3 sw=3: */
