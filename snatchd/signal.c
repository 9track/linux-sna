/* signal.c: Signal handling.
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

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#include <paths.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>

#include "snatchd.h"

static sigset_t blockmask, emptymask;
static int blocked=0;

void sig_init(void)
{
        struct sigaction sa;

        sigemptyset(&emptymask);
        sigemptyset(&blockmask);
        sigaddset(&blockmask, SIGCHLD);
        sigaddset(&blockmask, SIGHUP);
        sigaddset(&blockmask, SIGALRM);

        memset(&sa, 0, sizeof(sa));
        sa.sa_mask = blockmask;
        sa.sa_handler = retry;
        sigaction(SIGALRM, &sa, NULL);
        sa.sa_handler = config;
        sigaction(SIGHUP, &sa, NULL);
        sa.sa_handler = reapchild;
        sigaction(SIGCHLD, &sa, NULL);
        sa.sa_handler = goaway;
        sigaction(SIGTERM, &sa, NULL);
        sa.sa_handler = goaway;
        sigaction(SIGINT, &sa,  NULL);
        sa.sa_handler = SIG_IGN;
        sigaction(SIGPIPE, &sa, NULL);
}

void sig_block(void)
{
        sigprocmask(SIG_BLOCK, &blockmask, NULL);
        if(blocked) 
	{
            syslog(LOG_ERR, "internal error - signals already blocked\n");
            syslog(LOG_ERR, "please report to jschlst@turbolinux.com\n");
        }
        blocked = 1;
}

void sig_unblock(void)
{
        sigprocmask(SIG_SETMASK, &emptymask, NULL);
        blocked = 0;
}

void sig_wait(void) 
{
        sigsuspend(&emptymask);
}

void sig_preexec(void)
{
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = SIG_DFL;
        sigaction(SIGPIPE, &sa, NULL);

        sig_unblock();
}
