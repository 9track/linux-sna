/* sna_xwords.c:
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

/* xml stuff. */
#include <gnome-xml/xmlmemory.h>
#include <gnome-xml/parser.h>

/* required for linux-SNA. */
#include <asm/byteorder.h>
#include <sys/socket.h>
#include <linux/netdevice.h>
#include <linux/sna.h>
#include <linux/cpic.h>
#include <nof.h>

/* our stuff. */
#include "sna_list.h"
#include "snaconfig.h"
#include "sna_xwords.h"

struct wordmap on_types[] = {
        { "off",        0               },
        { "on",         1               },
        { NULL,         -1              }
};

struct wordmap security_types[] = {
        { "none",       0               }
};

struct wordmap lu_types[] = {
	{ "local",	0		},
	{ "remote",	1		}
};

struct wordmap tx_types[] = {
        { "low",        0               },
        { "medium",     1               },
        { "high",       2               },
        { "network",    3               },
        { NULL,         -1              }
};

struct wordmap node_types[] = {
	{ "nn",		SNA_APPN_NET_NODE	},
	{ "appn",	SNA_APPN_END_NODE	},
	{ "len",	SNA_LEN_END_NODE	}
};

struct wordmap role_types[] = {
	{ "neg",	SNA_LS_ROLE_NEG	},
	{ "pri",	SNA_LS_ROLE_PRI },
	{ "sec",	SNA_LS_ROLE_SEC }
};

struct wordmap direction_types[] = {
	{ "in",		SNA_LS_DIR_IN 	},
	{ "out",	SNA_LS_DIR_OUT 	},
	{ "both",	SNA_LS_DIR_BOTH	}
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

int xword_direction_types(xmlDocPtr doc, xmlNodePtr cur)
{
        return map_word(direction_types, xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
}

int xword_role_types(xmlDocPtr doc, xmlNodePtr cur)
{
        return map_word(role_types, xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
}

int xword_node_types(xmlDocPtr doc, xmlNodePtr cur)
{
        return map_word(node_types, xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
}

int xword_tx_types(xmlDocPtr doc, xmlNodePtr cur)
{
        return map_word(tx_types, xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
}

int xword_lu_types(xmlDocPtr doc, xmlNodePtr cur)
{
        return map_word(lu_types, xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
}

int xword_security(xmlDocPtr doc, xmlNodePtr cur)
{
        return map_word(security_types, xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
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

unsigned char xword_lsap(xmlDocPtr doc, xmlNodePtr cur)
{
	return strtol(xmlNodeListGetString(doc,
		cur->xmlChildrenNode, 1), (char **)NULL, 0);
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

int xword_cost(xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns, int *min_connect, 
	int *max_connect, int *min_byte, int *max_byte)
{
        for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
                if (cur->ns != ns)
                        continue;
                if (!matches(cur->name, "connect")) {
			xword_min_max(doc, cur, ns, min_connect, max_connect);
                }
                if (!matches(cur->name, "byte")) {
			xword_min_max(doc, cur, ns, min_byte, max_byte);
                }
        }
        return 0;
}

int xword_netid(xmlDocPtr doc, xmlNodePtr cur, sna_netid *netid)
{
	sna_netid *n;
	
	if (!netid)
		return -EINVAL;
	n = sna_char_to_netid(xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
	if (!n)
		return -EINVAL;
	memcpy(netid, n, sizeof(*netid));
	free(n);
	return 0;
}

sna_nodeid xword_nodeid(xmlDocPtr doc, xmlNodePtr cur)
{
	return sna_char_to_nodeid(xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
}

int xword_mac_address(xmlDocPtr doc, xmlNodePtr cur, char *mac)
{
	return sna_char_to_ether(xmlNodeListGetString(doc, cur->xmlChildrenNode, 1), mac);
}
