/* sna_utility.c: snaconfig utility functions.
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
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if_arp.h>
#include <net/ethernet.h>

#include <asm/byteorder.h>
#include <linux/sna.h>

/* our stuff. */
#include "sna_list.h"
#include "snaconfig.h"

int sna_netid_to_char(sna_netid *n, unsigned char *c)
{
        int len = 0, i;

        for (i = 0; i < 8 && (n->net[i] != 0x20); i++)
                /* Nothing */ ;
        len = i;
        strncpy(c, n->net, i);
        len = i + 1;
        strncpy(c + i, ".", 1);
        for (i = 0; i < 8 && (n->name[i] != 0x20); i++)
                /* Nothing */ ;
        strncpy(c + len, n->name, i);
        len += i;
        strncpy(c + len, "\0", 1);
        return len;
}

sna_netid *sna_char_to_netid(unsigned char *b)
{
        sna_netid *n;
        unsigned char c[40];
        int i;

        strcpy(c, b);   /* always use protection */
	new(n);
	if (!n)
		return NULL;
        strcpy(n->name, strpbrk(c, ".")+1);
        for (i = 0; i < 8; i++)
                n->name[i] = toupper(n->name[i]);
        for (i = strlen(n->name); i < 8; i++)
                n->name[i] = 0x20;
        strcpy(n->net, strtok(c, "."));
        for (i = 0; i < 8; i++)
                n->net[i] = toupper(n->net[i]);
        for (i = strlen(n->net); i < 8; i++)
                n->net[i] = 0x20;
        return n;
}

/* display a padded netid correctly */
char *sna_pr_netid(sna_netid *n)
{
        sna_netid p;
        static char buff[20];
        int i;

        memcpy(&p, n, sizeof(sna_netid));
        for (i = 0; p.net[i] != 0x20; i++); p.net[i] = 0;
        for (i = 0; p.name[i] != 0x20; i++); p.name[i] = 0;
        sprintf(buff, "%s.%s", p.net, p.name);
        return buff;
}

/* Display an Ethernet address in readable format. */
char *sna_pr_ether(unsigned char *ptr)
{
  	static char buff[64];

  	snprintf(buff, sizeof(buff), "%02X:%02X:%02X:%02X:%02X:%02X",
        	(ptr[0] & 0377), (ptr[1] & 0377), (ptr[2] & 0377),
        	(ptr[3] & 0377), (ptr[4] & 0377), (ptr[5] & 0377));
  	return buff;
}

unsigned long sna_char_to_nodeid(char *c)
{
	unsigned char d[40];
	unsigned long nodeid;
	
	strcpy(d, c);
	nodeid = strtol(d, NULL, 16);
	return nodeid;
}

char *sna_pr_nodeid(unsigned long nodeid)
{
        static char buff[20];

        sprintf(buff, "%03X.%05X", xid_get_block_num(nodeid),
		xid_get_pu_num(nodeid));
        return buff;
}

#define nibble(v, w)    ((v >> (w * 4)) & 0x0F)

static unsigned char rbits[16] = {
        0x00, 0x08, 0x04, 0x0C, 0x02, 0x0A, 0x06, 0x0E,
        0x01, 0x09, 0x05, 0x0D, 0x03, 0x0B, 0x07, 0x0F
};

unsigned char flip_nibble(unsigned char v)
{
        return (rbits[v & 0x0F]);
}

unsigned char flip_byte(unsigned char v)
{
        return ((flip_nibble(nibble(v, 0)) << 4) | flip_nibble(nibble(v, 1)));
}

/* Input an Ethernet address and convert to binary. */
int sna_char_to_ether(char *bufp, char *bmac)
{
        unsigned char *ptr;
        char c, *orig;
        int i;
        unsigned val;

        ptr = bmac;

        i = 0;
        orig = bufp;
        while ((*bufp != '\0') && (i < ETH_ALEN)) {
                val = 0;
                c = *bufp++;
                if (isdigit(c)) val = c - '0';
                else if (c >= 'a' && c <= 'f') val = c - 'a' + 10;
                else if (c >= 'A' && c <= 'F') val = c - 'A' + 10;
                else {
                        errno = EINVAL;
                        return -1;
                }
                val <<= 4;
                c = *bufp;
                if (isdigit(c)) val |= c - '0';
                else if (c >= 'a' && c <= 'f') val |= c - 'a' + 10;
                else if (c >= 'A' && c <= 'F') val |= c - 'A' + 10;
                else if (c == ':' || c == 0) val >>= 4;
                else {
                        errno = EINVAL;
                        return -1;
                }
                if (c != 0) bufp++;
                *ptr++ = (unsigned char) (val & 0377);
                i++;

                /* We might get a semicolon here - not required. */
                if (*bufp == ':') {
                        if (i == ETH_ALEN) {
                                                ; /* nothing */
                        }
                        bufp++;
                }
        }
        return 0;
}
