/* snatch_xwords.c:
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

/* our stuff. */
#include "snatch_list.h"
#include "snatchd.h"
#include "snatch_xwords.h"

struct wordmap conversation_types[] = {
        { "either",     0       },
        { "basic",      1       },
        { "mapped",     2       },
        { NULL,         -1      }
};

struct wordmap sync_levels[] = {
        { "any",        0       },
        { "none",       1       },
        { "confirm",    2       },
        { NULL,         -1      }
};

struct wordmap on_types[] = {
        { "off",        0               },
        { "on",         1               },
        { NULL,         -1              }
};

int matches(const char *cmd, char *pattern)
{
        int len = strlen(cmd);
        if (len > strlen(pattern))
                return -1;
        return memcmp(pattern, cmd, len);
}

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

int xword_conversation_type(xmlDocPtr doc, xmlNodePtr cur)
{
        return map_word(conversation_types, xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
}

int xword_sync_level(xmlDocPtr doc, xmlNodePtr cur)
{
        return map_word(sync_levels, xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
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
