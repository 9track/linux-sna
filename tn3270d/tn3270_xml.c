/* tnX_xml.c: generic tn xml functions.
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
#include <gnome-xml/xmlmemory.h>
#include <gnome-xml/parser.h>

#include <libtnX.h>

/* our stuff. */
#include "tn3270_list.h"
#include "tn3270.h"
#include "tn3270_xwords.h"

extern int tn3270_debug_level;
extern struct list_head server_list;

int tn3270_print_config(struct list_head *list)
{
	struct list_head *le, *se;
	server_info *server;
	allow_info *allow;

	list_for_each(le, list) {
		server = list_entry(le, server_info, list);
		printf("---------------------- server ----------------------\n");
		printf("use_name: %s\n", server->use_name);
		printf("debug_level: %d\n", server->debug_level);
		printf("client_port: %d\n", server->client_port);
		printf("manage_port: %d\n", server->manage_port);
		printf("test: %s\n", xword_pr_word(on_types, server->test));
		printf("limit: %d\n", server->limit);
		printf("sysreq: %s\n", xword_pr_word(on_types, server->sysreq));
		printf("pool: %s\n", server->pool);
		printf("--------------- allow ---------------\n");
		list_for_each(se, &server->allow_list) {
			struct in_addr in;
			allow = list_entry(se, allow_info, list);

			in.s_addr = allow->ipaddr;
			printf("ipaddr: %s\n", inet_ntoa(in));
			in.s_addr = allow->netmask;
			printf("netmask: %s\n", inet_ntoa(in));
		}
		printf("-------------------------------------\n");
		printf("----------------------------------------------------\n");
	}
	return 0;
}

static server_info *tn3270_parse_server(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	server_info *server;

	if (!new(server))
		return NULL;
	list_init_head(&server->allow_list);
	for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
		if (cur->ns != ns)
                        continue;
		if (!strcmp(cur->name, "debug")) {
			server->debug_level = xword_int(doc, cur);
		}
		if (!strcmp(cur->name, "use_name")) {
			xword_strcpy(doc, cur, server->use_name);
		}
		if (!strcmp(cur->name, "client_port")) {
			server->client_port = xword_int(doc, cur);
		}
		if (!strcmp(cur->name, "manage_port")) {
			server->manage_port = xword_int(doc, cur);
		}
		if (!strcmp(cur->name, "test")) {
			server->test = xword_on_off(doc, cur);
		}
		if (!strcmp(cur->name, "limit")) {
			server->limit = xword_int(doc, cur);
		}
		if (!strcmp(cur->name, "sysreq")) {
			server->sysreq = xword_on_off(doc, cur);
		}
		if (!strcmp(cur->name, "pool")) {
			xword_strcpy(doc, cur, server->pool);
		}
		if (!strcmp(cur->name, "allow")) {
			xword_ip_allow(doc, cur, ns, server);
		}
	}
	return server;
}

int tn3270_read_config_file(char *cfile)
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
                tn3270_debug(1, "file (%s) is an empty document.\n", cfile);
                xmlFreeDoc(doc);
                return -1;
        }
        ns = xmlSearchNsByHref(doc, cur, _PATH_TN3270D_XML_HREF);
        if (!ns) {
                tn3270_debug(1, "file (%s) is of the wrong type,"
                        " %s namespace not found.\n", cfile,
                        _PATH_TN3270D_XML_HREF);
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
	while (cur && strcmp(cur->name, _PATH_TN3270D_XML_TOP))
		cur = cur->next;
	if (!cur)
		return -1;

        /* now we walk the xml tree. */
        for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
		if (cur->ns != ns)
			continue;
		if (!strcmp(cur->name, "server")) {
			server_info *server;

			server = tn3270_parse_server(doc, ns, cur);
			if (!server)
				continue;
			list_add_tail(&server->list, &server_list);
		}
        }
        return 0;
}
