/* libappc.c: APPC call mapping for Linux-SNA
 *
 * Author:
 * Jay Schulist         <jschlst@turbolinux.com>
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

/* Notes:
 * - Full Duplex, Half Duples, and Mapped Conversation verbs are all
 *   accessed through the same function, but with different flags.
 */

#include <config.h>
#include <syscall_pic.h>
#include <stdio.h>
#include <stdlib.h>

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
