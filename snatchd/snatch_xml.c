/* snatch_xml.c: generic xml functions.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <glib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* xml stuff. */
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

/* our stuff. */
#include "snatch_list.h"
#include "snatchd.h"
#include "snatch_xwords.h"

extern int snatch_debug_level;
extern struct list_head tp_list;

int snatch_print_config(struct list_head *list)
{
	struct list_head *le;
	local_tp_info *tp;

	list_for_each(le, list) {
		tp = list_entry(le, local_tp_info, list);
		printf("---------------------- tp ----------------------\n");
		printf("name: %s\n", tp->name);
		printf("type: %s\n", xword_pr_word(conversation_types, tp->type));
		printf("sync: %s\n", xword_pr_word(sync_levels, tp->sync_level));
		printf("queued: %s\n", xword_pr_word(on_types, tp->queued));
		printf("limit: %d\n", tp->limit);
		printf("user: %s\n", tp->user);
		printf("path: %s\n", tp->path);
#ifdef ARGS
		printf("args: %s\n", tp->args);
#endif
		printf("------------------------------------------------\n");
	}
	return 0;
}

static local_tp_info *snatch_parse_tp(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	local_tp_info *tp;

	if (!new(tp))
		return NULL;
	for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
		if (cur->ns != ns)
                        continue;
		if (!strcmp(cur->name, "name")) {
			xword_strcpy(doc, cur, tp->name);
		}
		if (!strcmp(cur->name, "type")) {
			tp->type = xword_conversation_type(doc, cur);
		}
		if (!strcmp(cur->name, "sync")) {
			tp->sync_level = xword_sync_level(doc, cur);
		}
		if (!strcmp(cur->name, "queued")) {
			tp->queued = xword_on_off(doc, cur);
		}
		if (!strcmp(cur->name, "limit")) {
			tp->limit = xword_int(doc, cur);
		}
		if (!strcmp(cur->name, "user")) {
			xword_strcpy(doc, cur, tp->user);
		}
		if (!strcmp(cur->name, "path")) {
			xword_strcpy(doc, cur, tp->path);
		}
#ifdef ARGS
		if (!strcmp(cur->name, "args")) {
			xword_args(doc, cur, tp->args);
		}
#endif
	}
	return tp;
}

int snatch_read_config_file(char *cfile)
{
	xmlNodePtr cur;
        xmlDocPtr doc;
	xmlNsPtr ns;

        /* COMPAT: Do not generate nodes for formatting spaces */
        LIBXML_TEST_VERSION
        xmlKeepBlanksDefault(0);

        /* build an XML tree from a the file. */
        doc = xmlParseFile(cfile);
        if (!doc)
                return -1;

        /* check the document is of the right kind. */
        cur = xmlDocGetRootElement(doc);
        if (!cur) {
                snatch_debug(1, "file (%s) is an empty document.\n", cfile);
                xmlFreeDoc(doc);
                return -1;
        }
        ns = xmlSearchNsByHref(doc, cur, _PATH_SNATCHD_XML_HREF);
        if (!ns) {
                snatch_debug(1, "file (%s) is of the wrong type,"
                        " %s namespace not found.\n", cfile,
                        _PATH_SNATCHD_XML_HREF);
                xmlFreeDoc(doc);
                return -1;
        }
	if (strcmp(cur->name, "Helping")) {
                fprintf(stderr, "file (%s) is of the wrong type,"
                        " root node != Helping.\n", cfile);
                xmlFreeDoc(doc);
                return -1;
        }

        /* now we walk the xml tree. */
        cur = cur->xmlChildrenNode;
        while (cur && xmlIsBlankNode(cur))
                cur = cur->next;
        if (!cur)
                return -1;
	while (cur && strcmp(cur->name, _PATH_SNATCHD_XML_TOP))
		cur = cur->next;
	if (!cur)
		return -1;

        /* now we walk the xml tree. */
        for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
		if (cur->ns != ns)
			continue;
		if (!strcmp(cur->name, "tp")) {
			local_tp_info *tp;

			tp = snatch_parse_tp(doc, ns, cur);
			if (!tp)
				continue;
			list_add_tail(&tp->list, &tp_list);
		}
        }
        return 0;
}
