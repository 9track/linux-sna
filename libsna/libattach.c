/* libattach.c: Linux-SNA Attach Manager library.
 *
 * Author:
 * Jay Schulist         <jschlst@samba.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * None of the authors or maintainers or their employers admit
 * liability nor provide warranty for any of this software.
 * This material is provided "as is" and at no charge.
 */

#include <config.h>
#include <syscall_pic.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/unistd.h>
#include <linux/attach.h>

#include "attach.h"

_syscall2_pic(int, tp_register, int, a, struct tp_info *, tp);
_syscall2_pic(int, tp_unregister, int, a, int, b);
_syscall4_pic(int, tp_correlate, int, s, pid_t, pid, unsigned long, tcb_id, char *, tp_name);
_syscall0(int, attach_open);
_syscall4_pic(int, attach_listen,int,s,void *,buf,int,len,unsigned int,flags);
_syscall1_pic(int, attach_close, int, s);
