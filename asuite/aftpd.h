/* aftpd - APPC ftp server for Linux-SNA
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

#include <cpicsession.h>
#include <libaftp.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>

#if HAVE_SNA_CMC_H
#include <sna/cmc.h>
#include <port.h>
#else
#include <linux/cpic.h>
#endif

#include <filemode.h>

/* Since major is a function on SVR4, we can't use `ifndef major'.  */
#ifdef MAJOR_IN_MKDEV
# include <sys/mkdev.h>
# define HAVE_MAJOR
#endif
#ifdef MAJOR_IN_SYSMACROS
# include <sys/sysmacros.h>
# define HAVE_MAJOR
#endif
#ifdef major			/* Might be defined in sys/types.h.  */
# define HAVE_MAJOR
#endif

#ifndef HAVE_MAJOR
# define major(dev)  (((dev) >> 8) & 0xff)
# define minor(dev)  ((dev) & 0xff)
# define makedev(maj, min)  (((maj) << 8) | (min))
#endif
#undef HAVE_MAJOR

#define TIME_BUF_SIZE 32

void a_tower_request(void);
void a_query_system(void);
void a_query_dir(void);
void a_list_request(void);
void a_change_dir(void);
void a_request_file(void);
void a_delete(void);
void a_create_dir(void);
void a_send_file(void);
void a_file_data(void);
void a_file_complete(void);

char *getgroup ();
char *getuser ();



