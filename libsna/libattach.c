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
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/sna.h>
#include <linux/attach.h>

#include "attach.h"

/**
 * @afd: attach file descriptor.
 */
int tp_register(int afd, struct tp_info *tp)
{
	int opcode = ATTACH_TP_REGISTER;
	attach_args *args;
	int err, ret;

	aargo3(args, &opcode, &ret, tp);
	err = ioctl(afd, 0, args);
	free(args);
	if (err == 0)
		err = ret;
	return err;
}

/**
 * @afd: attach file descriptor.
 */
int tp_unregister(int afd, int tp_index)
{
	int opcode = ATTACH_TP_UNREGISTER;
	attach_args *args;
	int err, ret;

	aargo3(args, &opcode, &err, &tp_index);
	err = ioctl(afd, 0, args);
	free(args);
	if (err == 0)
		err = ret;
	return err;
}

/**
 * @afd: attach file descriptor.
 */
int tp_correlate(int afd, pid_t pid, unsigned long tcb_id, char *tp_name)
{
	int opcode = ATTACH_TP_CORRELATE;
	attach_args *args;
	int err, ret;

	aargo5(args, &opcode, &err, &pid, &tcb_id, tp_name);
	err = ioctl(afd, 0, args);
	free(args);
	if (err == 0)
		err = ret;
	return 0;
}

int attach_open(void)
{
	return socket(PF_SNA, SOCK_DGRAM, SNAPROTO_ATTACH);
}

/**
 * @afd: attach file descriptor.
 */
int attach_listen(int afd, void *buf, int len, unsigned int flags)
{
	return recv(afd, buf, len, flags);
}

/**
 * @afd: attach file descriptor.
 */
int attach_close(int afd)
{
	return close(afd);
}
