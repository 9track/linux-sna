/* libattach.c: Linux-SNA Attach Manager library.
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

#include <stdlib.h>
#include <syscall_pic.h>
#include <linux/unistd.h>
#include <linux/attach.h>

#include "attach.h"

_syscall2_pic(int, tp_register, int, a, struct tp_info *, tp);
_syscall2_pic(int, tp_unregister, int, a, int, b);
_syscall4_pic(int, tp_correlate, int, s, pid_t, pid, unsigned long, tcb_id, char *, tp_name);
_syscall0(int, attach_open);
_syscall4_pic(int, attach_listen,int,s,void *,buf,int,len,unsigned int,flags);
_syscall1_pic(int, attach_close, int, s);
