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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include <linux/unistd.h>
#include <sys/syscall.h>

#include <linux/attach.h>
#include "attach.h"

/**
 * @afd: attach file descriptor.
 */
int tp_register(int afd, struct tp_info *tp)
{
	int opcode = ATTACH_TP_REGISTER;
	attach_args *args;
	int err;
	
	aargo4(args, &opcode, &err, &afd, tp);
	syscall(__NR_attachcall, args);
	free(args);
	return err;
}

/**
 * @afd: attach file descriptor.
 */
int tp_unregister(int afd, int tp_index)
{
	int opcode = ATTACH_TP_UNREGISTER;
	attach_args *args;
	int err;
	
	aargo4(args, &opcode, &err, &afd, &tp_index);
	syscall(__NR_attachcall, args);
	free(args);
	return err;
}

/**
 * @afd: attach file descriptor.
 */
int tp_correlate(int afd, pid_t pid, unsigned long tcb_id, char *tp_name)
{
	int opcode = ATTACH_TP_CORRELATE;
	attach_args *args;
	int err;
	
	aargo6(args, &opcode, &err, &afd, &pid, &tcb_id, tp_name);
	syscall(__NR_attachcall, args);
	free(args);
	return 0;
}

int attach_open(void)
{
	int opcode = ATTACH_OPEN;
	attach_args *args;
	int err;
	
	aargo2(args, &opcode, &err);
	syscall(__NR_attachcall, args);
	free(args);
	return err;
}

/**
 * @afd: attach file descriptor.
 */
int attach_listen(int afd, void *buf, int len, unsigned int flags)
{
	int opcode = ATTACH_LISTEN;
	attach_args *args;
        int err;

	aargo6(args, &opcode, &err, &afd, buf, &len, &flags);
	syscall(__NR_attachcall, args);
	free(args);
	return err;
}

/**
 * @afd: attach file descriptor.
 */
int attach_close(int afd)
{
	int opcode = ATTACH_CLOSE;
	attach_args *args;
        int err;

	aargo3(args, &opcode, &err, &afd);
	syscall(__NR_attachcall, args);
	free(args);
	return err;
}
