/* atell - APPC tell client/TP for Linux-SNA
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
#if defined(_WIN32)
#include "getopt.h"
#include <windows.h>
#include "wincpic.h"
#define OS_NAME "Windows"
#else
#define OS_NAME "Linux"
#include "cpic.h"
#include <unistd.h>
#endif
#include "cpicsession.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>


#define MAX_MESSAGE_LEN (300+1)
#define TIME_LEN 26

#define RC_OK 0
#define EXCHANGE_VERSION 1
#define EXCHANGE_OS_NAME 2
#define EXCHANGE_BUFFER_SIZE 128

/* min and max macros */
#ifndef max
#define max(a,b)        (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)        (((a) < (b)) ? (a) : (b))
#endif
