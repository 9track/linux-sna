/* tn3270_xwords.c:
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
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <glib.h>

/* xml stuff. */
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

/* required for linux-SNA. */
#include <asm/byteorder.h>
#include <sys/socket.h>
#include <linux/netdevice.h>
#include <linux/sna.h>
#include <linux/cpic.h>
#include <nof.h>

#include <libtnX.h>

/* our stuff. */
#include "tn3270_list.h"
#include "tn3270.h"
#include "tn3270_xwords.h"

struct wordmap on_types[] = {
        { "off",        0               },
        { "on",         1               },
        { NULL,         -1              }
};

int map_word(struct wordmap *wm, const char *word)
{
        int i;
        for (i = 0; wm[i].word != NULL; i++)
                if (!strcmp(wm[i].word, word))
                        return wm[i].val;
        return -1;
}

char *xword_pr_word(struct wordmap *wm, const int v)
{
        int i;
        for (i = 0; wm[i].word != NULL; i++)
                if (wm[i].val == v)
                        return wm[i].word;
        return NULL;
}

int xword_on_off(xmlDocPtr doc, xmlNodePtr cur)
{
	return map_word(on_types, xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
}

int xword_int(xmlDocPtr doc, xmlNodePtr cur)
{
	return atoi(xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
}

int xword_strcpy(xmlDocPtr doc, xmlNodePtr cur, char *to)
{
	char *from = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	strcpy(to, from);
	return strlen(from);
}

int xword_strcpy_toupper(xmlDocPtr doc, xmlNodePtr cur, char *to)
{
        char *from = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	int i;
	
        strcpy(to, from);
	for (i = 0; i < strlen(from); i++)
		to[i] = toupper(to[i]);
        return strlen(from);
}

int xword_min_max(xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns, int *min, int *max)
{
        for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
                if (cur->ns != ns)
                        continue;
                if (!matches(cur->name, "min")) {
                        *min = xword_int(doc, cur);
                }
                if (!matches(cur->name, "max")) {
                        *max = xword_int(doc, cur);
                }
        }
        return 0;
}

allow_info *xword_ip(xmlDocPtr doc, xmlNodePtr cur)
{
	char ip_s[16], netmask_s[3];
	char ip_string[40];
	allow_info *allow;
	char *slash;
	int bits;
	
	if (!new(allow))
		return NULL;
	xword_strcpy(doc, cur, ip_string);
	slash = strchr(ip_string, '/');
	if (!slash)
		return NULL;
	memset(ip_s, '\0', 16);
	memset(netmask_s, '\0', 3);
	strncpy(ip_s, ip_string, strlen(ip_string) - strlen(slash));
	strncpy(netmask_s, ++slash, strlen(slash) - 1);
	bits = atoi(netmask_s);
	if (bits != 0)
		allow->netmask = htonl(0xFFFFFFFF << (32 - bits));
	else
		allow->netmask = 0;
	allow->ipaddr = inet_addr(ip_s);
	return allow;
}

int xword_ip_allow(xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns, server_info *server)
{
	for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
                if (cur->ns != ns)
                        continue;
		if (!matches(cur->name, "ip")) {
			allow_info *allow;

			allow = xword_ip(doc, cur);
			if (!allow)
				continue;
			list_add_tail(&allow->list, &server->allow_list);
		}
	}
	return 0;
}
