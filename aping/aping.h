/* aping - APPC ping client for Linux-SNA
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

/*
 * Define the operating system string.
 */

#if defined(_WIN32)
#include <windows.h>
#include "wincpic.h"

#define OS_NAME    "Windows"

#define popen(x,y)      _popen(x,y)

struct timezone {
   int  tz_minuteswest; /* minutes W of Greenwich */
   int  tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timezone *tz);

#else
#include "cpic.h"

#define OS_NAME    "Linux"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "cpicsession.h"

#define EXCHANGE_BUFFER_SIZE 128
#define EXCHANGE_VERSION 1
#define EXCHANGE_OS_NAME 2
#define RC_OK 0

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

#define convert_to_ascii(buffer, len)
#define convert_from_ascii(buffer, len)

unsigned long  get_time(void);

void do_aping(int argc, char *argv[]);
              
void process_arguments(int argc, char *argv[]);
                       
void usage(void);

int get_aping_version(unsigned char * conv_id,
                      unsigned char * tp_major,
                      unsigned char * tp_minor,
                      char * os_name,
                      unsigned int os_name_len,
                      CM_INT32 * rc);

void print_header();