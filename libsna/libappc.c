/* libappc.c: APPC call mapping for Linux-SNA
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

#include <syscall_pic.h>
#include <linux/unistd.h>
#include <linux/appc.h>

/* Single APPC system call into the kernel. */
_syscall5_pic(void, appcall, unsigned short, opcode, unsigned char, opext,
        unsigned short, rcpri, unsigned long, rcsec, void *, uaddr);

void appc(unsigned short opcode, unsigned char opext,
        unsigned short rcpri, unsigned long rcsec, void *uaddr)
{
	appcall(opcode, opext, rcpri, rcsec, uaddr);
}
