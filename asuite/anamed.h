/* anamed - APPC Name server for Linux-SNA
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

#include <syslog.h>
#include <cpicsession.h>
#include <libaname.h>
#include <anameapi.h>
#include <asuitecommon.h>
#include <tdb.h>

#include <stdio.h>

struct _namerec {
  char username[ANAME_UN_SIZE+1];
  char fqluname[ANAME_FQ_SIZE+1];
  char grpname[ANAME_GN_SIZE+1];
  char tpname[ANAME_TP_SIZE+1];
};

typedef struct _namerec namerec;





