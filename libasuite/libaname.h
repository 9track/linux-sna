/* libaname - ANAME API library for Linux-SNA
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

#if HAVE_SNA_CMC_H
#include <sna/cmc.h>
#include <port.h>
#else
#include <linux/cpic.h>
#endif

#define OS_NAME "Linux"

#include <netinet/in.h>
#include <unistd.h>

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <anameapi.h>

/* Major Codes */
#define ANAME_STATUS_RECORD           1
#define ANAME_REGISTER_REQUEST_RECORD 2
#define ANAME_DELETE_REQUEST_RECORD   3
#define ANAME_QUERY_REQUEST_RECORD    4
#define ANAME_RESPONSE_RECORD         5

/* Key Codes */
#define ANAME_USER_NAME          100
#define ANAME_FQLU_NAME          101
#define ANAME_GROUP_NAME         102
#define ANAME_TP_NAME            103
#define ANAME_BLANK_FLAG         108
#define ANAME_DUPLICATE_FLAG     109
#define ANAME_STATUS_RETURN_CODE   2
#define ANAME_CATEGORY_INDEX       4
#define ANAME_STATUS_PRIMARY_MSG   5
#define ANAME_STATUS_SECONDARY_MSG 6
#define ANAME_STATUS_INFO_MSG      7
#define ANAME_STATUS_LOG_MSG       8
