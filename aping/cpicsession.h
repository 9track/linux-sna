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

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#include <windows.h>
#include "wincpic.h"
#else
#include "cpic.h"
#endif

#define  MAX_TP_NAME       (64+1) 
#define  MAX_SYM_DEST_NAME (8+1)
#define  MAX_FQPLU_NAME    (17+1)
#define  MAX_MODE_NAME     (8+1)
#define  MAX_USERID        (8+1)
#define  MAX_PASSWORD      (8+1)

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

#if !defined(_WIN32)
void strupr (char * string);
#endif

#ifdef __cplusplus
}

#endif
#endif				/* CPIC_SESSION_H*/
