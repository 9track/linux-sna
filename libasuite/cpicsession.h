/* cpicsession -- helper functions for Linux-SNA cpic programming
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
#ifndef CPIC_SESSION_H
#define CPIC_SESSION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#if HAVE_SNA_CMC_H
#include <sna/cmc.h>
#else
#include <linux/cpic.h>
#endif
#define OS_NAME    "Linux"

#include <sys/time.h>
#include <unistd.h>

#define EXCHANGE_BUFFER_SIZE 128
#define EXCHANGE_VERSION 1
#define EXCHANGE_OS_NAME 2
#define RC_OK 0

#define  MAX_TP_NAME       (64+1) 
#define  MAX_SYM_DEST_NAME (8+1)
#define  MAX_FQPLU_NAME    (17+1)
#define  MAX_MODE_NAME     (8+1)
#define  MAX_USERID        (8+1)
#define  MAX_PASSWORD      (8+1)

/* min and max macros */
#ifndef max
#define max(a,b)        (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)        (((a) < (b)) ? (a) : (b))
#endif

#ifndef TRUE
#define  TRUE  (1)
#endif

#ifndef FALSE
#define  FALSE (0)
#endif

/* Idea shamelessly stolen from GTK+ */
#define cpic_new(type,count) (type *)malloc (sizeof (type) * (count))

/****s* lib5250/Tn5250Session
 * NAME~
 *    cpic_session
 * SYNOPSIS
 *    cpic_session *sess = cpic_session_new ();
 *    cpic_session_destroy(sess);
 * DESCRIPTION
 *
 * High level object oriented interface the the CPI-C library.
 *
 * SOURCE
 */
struct _cpic_session {
    int           symbolic;	  		        /* Destination name is symbolic  */
    char          tp_name[MAX_TP_NAME];                 /* Transaction Program name      */
    char          mode_name[MAX_MODE_NAME];             /* Mode name                     */
    char          destination[MAX_FQPLU_NAME];          /* Partner LU name               */
    char          sym_dest_name[MAX_SYM_DEST_NAME];     /* Symbolic destination name     */
    char          userid[MAX_USERID];                   /* Userid                        */
    char          password[MAX_PASSWORD];               /* Password                      */
    unsigned long security_type;                        /* Security type                 */
};

typedef struct _cpic_session cpic_session;

extern cpic_session *cpic_session_new(void);
extern void cpic_session_destroy(cpic_session * This);
extern int  cpic_session_set_tp_name(cpic_session * This, char * tp_name);
extern int  cpic_session_set_mode_name(cpic_session * This, char * mode_name);
extern int  cpic_session_set_symbolic_name(cpic_session * This, char * symbolic_name);
extern int  cpic_session_set_destination(cpic_session * This, char * destination);
extern int  cpic_session_set_userid(cpic_session * This, char * userid);
extern int  cpic_session_set_password(cpic_session * This, char * password);
extern void cpic_session_set_security_type(cpic_session * This, unsigned long security_type);
extern int  cpic_session_init(cpic_session * This, unsigned char * cm_conv_id, CM_INT32 * cpicrc);

int get_server_version(unsigned char * conv_id,
		       unsigned char client_major,
		       unsigned char client_minor,
		       unsigned char * tp_major,
		       unsigned char * tp_minor,
		       char * os_name,
		       unsigned int os_name_len,
		       CM_INT32 * rc);

int get_client_version(unsigned char * conv_id,
		       unsigned char tp_major,
		       unsigned char tp_minor,
		       unsigned char * client_major,
		       unsigned char * client_minor,
		       char * os_name,
		       unsigned int os_name_len,
		       CM_INT32 * rc);

void strupr (char * string);

#ifdef __cplusplus
}

#endif
#endif				/* CPIC_SESSION_H*/
