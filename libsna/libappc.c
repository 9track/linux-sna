/* libappc.c: APPC system call wrappers.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include <linux/unistd.h>
#include <sys/syscall.h>

#include <linux/appc.h>

void appc(unsigned short opcode, unsigned char opext,
        unsigned short rcpri, unsigned long rcsec, void *uaddr)
{
	syscall(__NR_appcall, opcode, opext, rcpri, rcsec, uaddr);
}
