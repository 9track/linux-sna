/* liblar.c: Linux-SNA LAR wraper library. 
 *
 * Notes:
 * - Hopefully this will someday go away, but until Linus lets
 *   me add new system calls into the kernel we will be stuck
 *   wrapping socket calls within the SNA calls for ever.
 * - The getsockopt() call will block if necessary and we don't care what
 *   the user things about us blocking.
 *
 * Author:
 * Jay Schulist		<jschlst@samba.org>
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
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#define __KERNEL__
#include <linux/socket.h>
#undef __KERNEL__

#include <linux/if_ether.h>
#include <linux/sna.h>

#ifdef NOT
int lar_find_member(int sk, )
{
	int err, size;

        err = getsockopt(sk, SOL_SNA_LAR, SNA_LAR_FIND_MEMBER,
                , (size_t *)&size);
        if(err < 0)
        {
                perror("lar_find_member");
                return (err);
        }

        return (0);
}

int lar_find(int sk, )
{
	int err, size;

        err = getsockopt(sk, SOL_SNA_LAR, SNA_LAR_FIND,
                , (size_t *)&size);
        if(err < 0)
        {
                perror("lar_find");
                return (err);
        }

        return (0);
}
#endif

int lar_search(int sk, char *nnet, char *name, unsigned long rtcap)
{
	int numreqs = 60;
	struct larconf lc;
	struct larreq *lr;
	int err, size, n;

	lc.larc_buf = NULL;
	memcpy(&lc.larc_net, nnet, 8);
        memcpy(&lc.larc_group, name, 8);
        lc.larc_rtcap = rtcap;
	for(;;)
	{
		lc.larc_len = sizeof(struct larreq) * numreqs;
		lc.larc_buf = (char *)realloc(lc.larc_buf, lc.larc_len);

		err = ioctl(sk, SIOCGLAR, &lc);
        	if(err < 0)
        	{
        	        perror("lar_search");
        	        goto out;
        	}

		if(lc.larc_len == sizeof(struct larreq) * numreqs)
		{
			numreqs += 20;
			continue;
		}
		break;
	}

	printf("Dynamic APPN node discovery results:\n");
	printf("%-18s%-9s%-13s%-5s\n",
                "netid.node", "group", "mac_address", "lsap");

	lr = lc.larc_req;
	for(n = 0; n < lc.larc_len; n += sizeof(struct larreq))
	{
		char netname[40];
		char buf[200];
		sprintf(netname, "%s.%s", lr->netid, lr->name);
		sprintf(buf, "%-18s%-9s%02X%02X%02X%02X%02X%02X %02X\n",
			netname, lr->group, lr->mac[0], lr->mac[1], lr->mac[2],
			lr->mac[3], lr->mac[4], lr->mac[5], lr->lsap);
		printf("%s", buf);
		lr++;
	}
	err = 0;

out:
	free(lc.larc_buf);
        return (err);
}

#ifdef NOT

int lar_record(int sk, )
{
	int err;

        err = setsockopt(sk, SOL_SNA_LAR, SNA_LAR_RECORD,
                , sizeof());
        if(err < 0)
        {
                perror("lar_record");
                return (err);
        }

        return (0);
}

int lar_erase(int sk, )
{
	int err;

        err = setsockopt(sk, SOL_SNA_LAR, SNA_LAR_ERASE,
                , sizeof());
        if(err < 0)
        {
                perror("lar_erase");
                return (err);
        }

        return (0);
}
#endif
