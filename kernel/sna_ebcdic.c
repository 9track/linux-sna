/* sna_ebcdic.c: Linux-SNA EBCDIC/ASCII converter.
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
 
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/bitops.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/in.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/if_ether.h>
#include <linux/notifier.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/route.h>
#include <linux/inet.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <net/datalink.h>
#include <net/sock.h>
#include <linux/proc_fs.h>

#include <linux/sna.h>

unsigned char etor(unsigned char a)
{
        return ebcdic_to_rotated[a];
}

char *etor_strncpy(char *dest, char *src, size_t count)
{
        char *tmp = dest;

        while (count-- && (*dest++ = etor(*src++)) != '\0')
                /* Nothing */;
        return tmp;
}

/* Convert a single charater from EBCDIC to ASCII */
unsigned char etoa(unsigned char a)
{
        return ebcdic_to_ascii_sna[a];
}

/* Convert a single character from ASCII to EBCDIC */
unsigned char atoe(unsigned char a)
{
	return ascii_to_ebcdic_sna[a];
}

char *atoe_strcpy(char *dest, char *src)
{
        char *tmp = dest;

        while ((*dest++ = atoe(*src++)) != '\0')
                /* Nothing */;
        return tmp;
}

char *etoa_strcpy(char *dest, char *src)
{
        char *tmp = dest;

        while ((*dest++ = etoa(*src++)) != '\0')
                /* Nothing */;
        return tmp;
}

char *atoe_strncpy(char *dest, char *src, size_t count)
{
        char *tmp = dest;

        while (count-- && (*dest++ = atoe(*src++)) != '\0')
                /* Nothing */;
        return tmp;
}

char *fatoe_strncpy(char *dest, char *src, size_t count)
{
        char *tmp = dest;

        while (count--)
		(*dest++ = atoe(*src++));
	return tmp;
}

char *etoa_strncpy(char *dest, char *src, size_t count)
{
        char *tmp = dest;

        while (count-- && (*dest++ = etoa(*src++)) != '\0')
                /* Nothing */;
        return tmp;
}

int atoe_strcmp(const char *acs, const char *ect)
{
        register signed char __res;

        while (1) {
                if ((__res = atoe(*acs) - *ect++) != 0 || !*acs++)
                        break;
        }
        return __res;
}

int etoa_strcmp(const char *ecs, const char *act)
{
        register signed char __res;

        while (1) {
                if ((__res = etoa(*ecs) - *act++) != 0 || !*ecs++)
                        break;
        }
        return __res;
}

int atoe_strncmp(const char *acs, const char *ect, size_t count)
{
        register signed char __res = 0;

        while (count) {
                if ((__res = atoe(*acs) - *ect++) != 0 || !*acs++)
                        break;
                count--;
        }
        return __res;
}

int etoa_strncmp(const char *ecs, const char *act, size_t count)
{
        register signed char __res = 0;

        while (count) {
                if ((__res = etoa(*ecs) - *act++) != 0 || !*ecs++)
                        break;
                count--;
        }
        return __res;
}

#define nibble(v, w)    ((v >> (w * 4)) & 0x0F)

static unsigned char rbits[16] = {
        0x00, 0x08, 0x04, 0x0C, 0x02, 0x0A, 0x06, 0x0E,
        0x01, 0x09, 0x05, 0x0D, 0x03, 0x0B, 0x07, 0x0F
};

unsigned char flip_nibble(unsigned char v)
{
        return rbits[v & 0x0F];
}

unsigned char flip_byte(unsigned char v)
{
        return (flip_nibble(nibble(v, 0)) << 4) | flip_nibble(nibble(v, 1));
}
