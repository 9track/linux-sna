/* sna_load.c:
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

extern char 	version_s[];
extern char 	name_s[];
extern char 	desc_s[];
extern char 	maintainer_s[];
extern char 	company_s[];
extern char 	web_s[];
extern int  	sna_debug_level;
extern global_info *sna_config_info;

static int sna_print_config(global_info *g)
{
	struct list_head *le;
	link_info *link;
	mode_info *mode;
	cpic_info *cpic;
	cos_info *cos;
	dlc_info *dlc;
	lu_info *lu;

	printf("\n================ Global Input Structure ================\n");
        if (!g) {
                printf("global data is NULL.\n");
                goto out;
        }
        printf("debug_level: %s\n", g->debug_level);
	printf("node_name: %s\n", sna_pr_netid(&g->node_name));
	printf("node_type: %d\n", g->node_type);
	printf("node_id: %s\n", sna_pr_nodeid(g->node_id));
	printf("max_lus: %d\n", g->max_lus);
	printf("lu_seg: %d\n", g->lu_seg);
	printf("bind_seg: %d\n", g->bind_seg);

        list_for_each(le, &g->dlc_list) {
		dlc = list_entry(le, dlc_info, list);
		printf("-------- Data link control --------\n");
		printf("use_name: %s\n", dlc->use_name);
		printf("iface: %s\n", dlc->interface);
                printf("port: 0x%02X\n", dlc->port[0]);
                printf("btu: %d\n", dlc->btu);
                printf("mia: %d\n", dlc->mia);
                printf("moa: %d\n", dlc->moa);
                printf("-----------------------------------\n");
	}

	list_for_each(le, &g->link_list) {
                link = list_entry(le, link_info, list);
		printf("-------- Link station --------\n");
		printf("use_name: %s\n", link->use_name);
		printf("role: %s\n", xword_pr_word(role_types, link->role));
		printf("direction: %s\n", xword_pr_word(direction_types,
			link->direction));
                printf("iface: %s\n", link->interface);
                printf("port: 0x%02X\n", link->port[0]);
                printf("plu_name: %s\n", sna_pr_netid(&link->plu_name));
		printf("plu_node_id: %s\n", sna_pr_nodeid(link->plu_node_id));
                printf("dstaddr: %s\n", sna_pr_ether(link->dstaddr));
                printf("dstport: 0x%02X\n", link->dstport[0]);
                printf("byteswap: %d\n", link->byteswap);
                printf("retry_on_fail: %d\n", link->retry_on_fail);
                printf("retry_times: %d\n", link->retry_times);
                printf("autoact: %d\n", link->autoact);
                printf("tg_number: %d\n", link->tg_number);
                printf("cost_per_byte: %d\n", link->cost_per_byte);
                printf("cost_per_connect_time: %d\n", link->cost_per_connect_time);
                printf("effective_capacity: %d\n", link->effective_capacity);
                printf("propagation_delay: %d\n", link->propagation_delay);
                printf("security: %d\n", link->security);
                printf("user1: %d\n", link->user1);
                printf("user2: %d\n", link->user2);
                printf("user3: %d\n", link->user3);
                printf("------------------------------\n");
	}

	list_for_each(le, &g->lu_list) {
                lu = list_entry(le, lu_info, list);
		printf("-------- Logical unit --------\n");
		printf("use_name: %s\n", lu->use_name);
		printf("type: %d\n", lu->type);
		if (!lu->type) {
			printf("name: %s\n", lu->name);
	                printf("syncpoint: %d\n", lu->syncpoint);
	                printf("lu_sess_limit: %d\n", lu->lu_sess_limit);
		} else {
			printf("plu_name: %s\n", sna_pr_netid(&lu->plu_name));
                	printf("fqcp_name: %s\n", sna_pr_netid(&lu->fqcp_name));
		}
		printf("-------------------------------------\n");
        }

	list_for_each(le, &g->mode_list) {
               	mode = list_entry(le, mode_info, list);
		printf("-------- Mode --------\n");
                printf("name: %s\n", mode->name);
                printf("plu_name: %s\n", sna_pr_netid(&mode->plu_name));
                printf("cos_name: %s\n", mode->cos_name);
                printf("encryption: %d\n", mode->encryption);
                printf("tx_pacing: %d\n", mode->tx_pacing);
                printf("rx_pacing: %d\n", mode->rx_pacing);
                printf("max_tx_ru: %d\n", mode->max_tx_ru);
                printf("max_rx_ru: %d\n", mode->max_rx_ru);
                printf("max_sessions: %d\n", mode->max_sessions);
                printf("min_conwinners: %d\n", mode->min_conwinners);
                printf("min_conlosers: %d\n", mode->min_conlosers);
                printf("auto_activation: %d\n", mode->auto_activation);
                printf("----------------------\n");
        }

	list_for_each(le, &g->cpic_list) {
                cpic = list_entry(le, cpic_info, list);
		printf("-------- CPI-C --------\n");
                printf("sym_dest_name: %s\n", cpic->sym_dest_name);
                printf("mode_name: %s\n", cpic->mode_name);
                printf("plu_name: %s\n", sna_pr_netid(&cpic->plu_name));
                printf("tp_name: %s\n", cpic->tp_name);
                printf("-----------------------\n");
        }

	list_for_each(le, &g->cos_list) {
                cos = list_entry(le, cos_info, list);
		printf("-------- Cost of Service --------\n");
                printf("cos_name: %s\n", cos->cos_name);
                printf("weight: %d\n", cos->weight);
                printf("tx_priority: %d\n", cos->tx_priority);
                printf("default_cos_invalid: %d\n", cos->default_cos_invalid);
                printf("default_cos_null: %d\n", cos->default_cos_null);
                printf("min_cost_per_connect: %d\n", cos->min_cost_per_connect);
		printf("max_cost_per_connect: %d\n", cos->max_cost_per_connect);
                printf("min_cost_per_byte: %d\n", cos->min_cost_per_byte);
		printf("max_cost_per_byte: %d\n", cos->max_cost_per_byte);
                printf("min_security: %d\n", cos->min_security);
                printf("max_security: %d\n", cos->max_security);
                printf("min_propagation_delay: %d\n", cos->min_propagation_delay);
                printf("max_propagation_delay: %d\n", cos->max_propagation_delay);
                printf("min_effective_capacity: %d\n", cos->min_effective_capacity);
                printf("max_effective_capacity: %d\n", cos->max_effective_capacity);
                printf("min_user1: %d\n", cos->min_user1);
                printf("max_user1: %d\n", cos->max_user1);
                printf("min_user2: %d\n", cos->min_user2);
                printf("max_user2: %d\n", cos->max_user2);
                printf("min_user3: %d\n", cos->min_user3);
                printf("max_user3: %d\n", cos->max_user3);
                printf("min_route_resistance: %d\n", cos->min_route_resistance);
                printf("max_route_resistance: %d\n", cos->max_route_resistance);
                printf("min_node_congested: %d\n", cos->min_node_congested);
                printf("max_node_congested: %d\n", cos->max_node_congested);
                printf("---------------------------------\n");
        }

out:	printf("=========================================================\n");
        return 0;
}

static cos_info *sna_parse_cos(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	cos_info *cos;

	if (!new(cos))
		return NULL;
	for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
		if (!matches(cur->name, "name")) {
			xword_strcpy_toupper(doc, cur, cos->cos_name);
                }
		if (!matches(cur->name, "weight")) {
			cos->weight = xword_int(doc, cur);
                }
		if (!matches(cur->name, "tx_priority")) {
			cos->tx_priority = xword_tx_types(doc, cur);
                }
		if (!matches(cur->name, "default_cos_invalid")) {
			cos->default_cos_invalid = xword_on_off(doc, cur);
                }
		if (!matches(cur->name, "default_cos_null")) {
			cos->default_cos_null = xword_on_off(doc, cur);
                }
		if (!matches(cur->name, "cost")) {
			xword_cost(doc, cur, ns, &cos->min_cost_per_connect,
				&cos->max_cost_per_connect,
				&cos->min_cost_per_byte, &cos->max_cost_per_byte);
                }
		if (!matches(cur->name, "security")) {
			xword_min_max(doc, cur, ns, &cos->min_security, 
				&cos->max_security);
                }
		if (!matches(cur->name, "propagation_delay")) {
			xword_min_max(doc, cur, ns, &cos->min_propagation_delay,
				&cos->max_propagation_delay);
                }
		if (!matches(cur->name, "effective_capacity")) {
			xword_min_max(doc, cur, ns, &cos->min_effective_capacity,
				&cos->max_effective_capacity);
                }
		if (!matches(cur->name, "route_resistance")) {
			xword_min_max(doc, cur, ns, &cos->min_route_resistance,
				&cos->max_route_resistance);
                }
		if (!matches(cur->name, "node_congested")) {
			xword_min_max(doc, cur, ns, &cos->min_node_congested,
				&cos->max_node_congested);
                }
		if (!matches(cur->name, "user1")) {
			xword_min_max(doc, cur, ns, &cos->min_user1, 
				&cos->max_user1);
                }
		if (!matches(cur->name, "user2")) {
			xword_min_max(doc, cur, ns, &cos->min_user2,
				&cos->max_user2);
                }
		if (!matches(cur->name, "user3")) {
			xword_min_max(doc, cur, ns, &cos->min_user3,
				&cos->max_user3);
                }
        }
	return cos;
}

static cpic_info *sna_parse_cpic(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	cpic_info *cpic;

	if (!new(cpic))
		return NULL;
	for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
		if (!matches(cur->name, "sym_dest_name")) {
			xword_strcpy_toupper(doc, cur, cpic->sym_dest_name);
                }
		if (!matches(cur->name, "mode_name")) {
			xword_strcpy_toupper(doc, cur, cpic->mode_name);
                }
		if (!matches(cur->name, "plu_name")) {
			xword_netid(doc, cur, &cpic->plu_name);
                }
		if (!matches(cur->name, "tp_name")) {
			xword_strcpy(doc, cur, cpic->tp_name);
                }
        }
	return cpic;
}

static mode_info *sna_parse_mode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	mode_info *mode;

	if (!new(mode))
		return NULL;
	for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
		if (!matches(cur->name, "name")) {
			xword_strcpy_toupper(doc, cur, mode->name);
                }
		if (!matches(cur->name, "cos_name")) {
			xword_strcpy_toupper(doc, cur, mode->cos_name);
                }
		if (!matches(cur->name, "plu_name")) {
			xword_netid(doc, cur, &mode->plu_name);
                }
		if (!matches(cur->name, "encryption")) {
			mode->encryption = xword_on_off(doc, cur);
                }
		if (!matches(cur->name, "tx_pacing")) {
			mode->tx_pacing = xword_int(doc, cur);
                }
		if (!matches(cur->name, "rx_pacing")) {
			mode->rx_pacing = xword_int(doc, cur);
                }
		if (!matches(cur->name, "max_tx_ru")) {
			mode->max_tx_ru = xword_int(doc, cur);
                }
		if (!matches(cur->name, "max_rx_ru")) {
			mode->max_rx_ru = xword_int(doc, cur);
                }
		if (!matches(cur->name, "min_conwinners")) {
			mode->min_conwinners = xword_int(doc, cur);
                }
		if (!matches(cur->name, "min_conlosers")) {
			mode->min_conlosers = xword_int(doc, cur);
                }
		if (!matches(cur->name, "max_sessions")) {
			mode->max_sessions = xword_int(doc, cur);
                }
		if (!matches(cur->name, "auto_activation")) {
			mode->auto_activation = xword_on_off(doc, cur);
                }
        }
	return mode;
}

static lu_info *sna_parse_lu(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	lu_info *lu;

	if (!new(lu))
		return NULL;
	for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
		if (!matches(cur->name, "use_name")) {
			xword_strcpy(doc, cur, lu->use_name);
		}
		if (!matches(cur->name, "where")) {
			lu->type = xword_lu_types(doc, cur);
		}
		if (!matches(cur->name, "syncpoint")) {
			lu->syncpoint = xword_on_off(doc, cur);
                }
		if (!matches(cur->name, "lu_sess_limit")) {
			lu->lu_sess_limit = xword_int(doc, cur);
                }
		if (!matches(cur->name, "plu_name")) {
			xword_netid(doc, cur, &lu->plu_name);
                }
		if (!matches(cur->name, "fqcp_name")) {
			xword_netid(doc, cur, &lu->fqcp_name);
                }
        }
	return lu;
}

static link_info *sna_parse_link(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	link_info *link;

	if (!new(link))
		return NULL;
	for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
		if (cur->ns != ns)
			continue;
		if (!matches(cur->name, "use_name")) {
			xword_strcpy(doc, cur, link->use_name);
		}
		if (!matches(cur->name, "role")) {
			link->role = xword_role_types(doc, cur);
                }
		if (!matches(cur->name, "direction")) {
			link->direction = xword_direction_types(doc, cur);
		}
		if (!matches(cur->name, "iface")) {
			xword_strcpy(doc, cur, link->interface);
                }
		if (!matches(cur->name, "port")) {
			link->port[0] = xword_lsap(doc, cur);
                }
		if (!matches(cur->name, "byteswap")) {
			link->byteswap = xword_on_off(doc, cur);
                }
		if (!matches(cur->name, "dstaddr")) {
			xword_mac_address(doc, cur, link->dstaddr);
                }
		if (!matches(cur->name, "dstport")) {
			link->dstport[0] = xword_lsap(doc, cur);
                }
		if (!matches(cur->name, "plu_name")) {
			xword_netid(doc, cur, &link->plu_name);
                }
		if (!matches(cur->name, "plu_node_id")) {
			link->plu_node_id = xword_nodeid(doc, cur);
		}
		if (!matches(cur->name, "retry_on_fail")) {
			link->retry_on_fail = xword_on_off(doc, cur);
                }
		if (!matches(cur->name, "retry_times")) {
			link->retry_times = xword_int(doc, cur);
                }
		if (!matches(cur->name, "autoact")) {
			link->autoact = xword_on_off(doc, cur);
                }
		if (!matches(cur->name, "tg_number")) {
			link->tg_number = xword_int(doc, cur);
                }
		if (!matches(cur->name, "cost_per_byte")) {
			link->cost_per_byte = xword_int(doc, cur);
                }
		if (!matches(cur->name, "cost_per_connect_time")) {
			link->cost_per_connect_time = xword_int(doc, cur);
                }
		if (!matches(cur->name, "effective_capacity")) {
			link->effective_capacity = xword_int(doc, cur);
                }
		if (!matches(cur->name, "propagation_delay")) {
			link->propagation_delay = xword_int(doc, cur);
                }
		if (!matches(cur->name, "security")) {
			link->security = xword_security(doc, cur);
                }
		if (!matches(cur->name, "user1")) {
			link->user1 = xword_int(doc, cur);
                }
		if (!matches(cur->name, "user2")) {
			link->user2 = xword_int(doc, cur);
                }
		if (!matches(cur->name, "user3")) {
			link->user3 = xword_int(doc, cur);
                }
        }
	return link;
}

static dlc_info *sna_parse_dlc(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	dlc_info *dlc;

	if (!new(dlc))
		return NULL;
	for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
		if (cur->ns != ns)
                        continue;
		if (!matches(cur->name, "use_name")) {
			xword_strcpy(doc, cur, dlc->use_name);
		}
		if (!matches(cur->name, "iface")) {
			xword_strcpy(doc, cur, dlc->interface);
                }
		if (!matches(cur->name, "port")) {
			dlc->port[0] = xword_lsap(doc, cur);
                }
		if (!matches(cur->name, "btu")) {
			dlc->btu = xword_int(doc, cur);
                }
		if (!matches(cur->name, "mia")) {
			dlc->mia = xword_int(doc, cur);
                }
		if (!matches(cur->name, "moa")) {
			dlc->moa = xword_int(doc, cur);
                }
        }
	return dlc;
}

static global_info *sna_parse_global(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	global_info *gi;

	if (!new(gi))
		return NULL;
	list_init_head(&gi->dlc_list);
	list_init_head(&gi->link_list);
	list_init_head(&gi->lu_list);
	list_init_head(&gi->mode_list);
	list_init_head(&gi->cpic_list);
	list_init_head(&gi->cos_list);
	for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
		if (cur->ns != ns)
                        continue;
		if (!matches(cur->name, "debuglevel")) {
			xword_strcpy(doc, cur, gi->debug_level);
		}
		if (!matches(cur->name, "node_name")) {
			xword_netid(doc, cur, &gi->node_name);
                }
		if (!matches(cur->name, "node_type")) {
			gi->node_type = xword_node_types(doc, cur);
                }
		if (!matches(cur->name, "node_id")) {
			gi->node_id = xword_nodeid(doc, cur);
                }
		if (!matches(cur->name, "max_lus")) {
			gi->max_lus = xword_int(doc, cur);
                }
		if (!matches(cur->name, "lu_seg")) {
			gi->lu_seg = xword_int(doc, cur);
                }
		if (!matches(cur->name, "bind_seg")) {
			gi->bind_seg = xword_int(doc, cur);
                }
	}
	return gi;
}

int sna_read_config_file(char *cfile)
{
	xmlNodePtr cur;
	xmlDocPtr doc;
	global_info *ginfo;
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
                sna_debug(1, "file (%s) is an empty document.\n", cfile);
                xmlFreeDoc(doc);
                return -1;
        }
        ns = xmlSearchNsByHref(doc, cur, _PATH_SNACFG_XML_HREF);
        if (!ns) {
                sna_debug(1, "file (%s) is of the wrong type,"
                        " %s namespace not found.\n", cfile, 
			_PATH_SNACFG_XML_HREF);
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

	/* first level is just 'node' */
        if ((strcmp(cur->name, _PATH_SNACFG_XML_TOP)) || (cur->ns != ns)) {
                fprintf(stderr, "file (%s) is of the wrong type, was '%s',"
                        " %s expected", cfile, cur->name, _PATH_SNACFG_XML_TOP);
                fprintf(stderr, "xmlDocDump follows.\n");
                xmlDocDump(stderr, doc);
                fprintf(stderr, "xmlDocDump finished.\n");
                xmlFreeDoc(doc);
                return -1;
        }

        /* now we walk the xml tree. */
        for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
                if ((!strcmp(cur->name, "global")) && (cur->ns == ns)) {
                        ginfo = sna_parse_global(doc, ns, cur);
                        if (!ginfo)
                                return -EINVAL;
                        continue;
                }
                if ((!strcmp(cur->name, "dlc")) && (cur->ns == ns)) {
			dlc_info *dlc;
                        dlc = sna_parse_dlc(doc, ns, cur);
                        if (!dlc)
                                continue;
			if (!ginfo) {
				free(dlc);
				continue;
			}
                        list_add_tail(&dlc->list, &ginfo->dlc_list);
                        continue;
                }
                if ((!strcmp(cur->name, "link")) && (cur->ns == ns)) {
			link_info *link;
                        link = sna_parse_link(doc, ns, cur);
                        if (!link)
                                continue;
                        if (!ginfo) {
                                free(link);
                                continue;
                        }
                        list_add_tail(&link->list, &ginfo->link_list);
                        continue;
                }
		if ((!strcmp(cur->name, "lu")) && (cur->ns == ns)) {
			lu_info *lu;
                        lu = sna_parse_lu(doc, ns, cur);
                        if (!lu) 
                                continue;
                        if (!ginfo) {
                                free(lu);
                                continue;
                        }
                        list_add_tail(&lu->list, &ginfo->lu_list);
                        continue;
                }
		if ((!strcmp(cur->name, "cos")) && (cur->ns == ns)) {
			cos_info *cos;
                        cos = sna_parse_cos(doc, ns, cur);
                        if (!cos) 
                                continue;
                        if (!ginfo) {
                                free(cos);
                                continue;
                        }
                        list_add_tail(&cos->list, &ginfo->cos_list);
                        continue;
                }
		if ((!strcmp(cur->name, "mode")) && (cur->ns == ns)) {
			mode_info *mode;
                        mode = sna_parse_mode(doc, ns, cur);
                        if (!mode) 
                                continue;
                        if (!ginfo) {
                                free(mode);
                                continue;
                        }
                        list_add_tail(&mode->list, &ginfo->mode_list);
                        continue;
                }
		if ((!strcmp(cur->name, "cpic")) && (cur->ns == ns)) {
			cpic_info *cpic;
                        cpic = sna_parse_cpic(doc, ns, cur);
                        if (!cpic) 
                                continue;
                        if (!ginfo) {
                                free(cpic);
                                continue;
                        }
                        list_add_tail(&cpic->list, &ginfo->cpic_list);
                        continue;
                }
        }
        sna_config_info = ginfo;
	return 0;
}

int sna_execute_config_info(int sna_sk, global_info *g)
{
	struct sna_nof_node *node_n;
	struct sna_nof_port *dlc_n;
	struct sna_nof_cpic *cpic_n;
	struct sna_nof_mode *mode_n;
	struct sna_nof_cos *cos_n;
	struct sna_nof_ls *link_n;
        struct sna_nof_local_lu *llu_n;
	struct sna_nof_remote_lu *plu_n;
	struct list_head *le;
	cpic_info *cpic_i;
	mode_info *mode_i;
	link_info *link_i;
	cos_info *cos_i;
	dlc_info *dlc_i;
	lu_info *lu_i;
	FILE *dblvl;
	int err;
	
	if (!g)
		return -EINVAL;

	/* set debug level. */
	dblvl = fopen(_PATH_SNA_DEBUG_LEVEL, "w");
        if (!dblvl)
		 return -1;
	fprintf(dblvl, "%s", g->debug_level);
	fclose(dblvl);
	
	/* define and start node. */
	if (!new(node_n))
		return -ENOMEM;
	memcpy(&node_n->netid, &g->node_name, sizeof(sna_netid));
	node_n->nodeid		= g->node_id;
	node_n->type		= g->node_type;
	node_n->max_lus		= g->max_lus;
	node_n->lu_seg		= g->lu_seg;
	node_n->bind_seg	= g->bind_seg;
	err = nof_define_node(sna_sk, node_n);
	if (err < 0) {
		sna_debug(1, "define node failed `%d: %s'.\n", err, strerror(errno));
		return err;
	}
	
#ifdef START_NODE_DEFAULT
	err = nof_start_node(sna_sk, node_n);
	free(node_n);
	if (err < 0) {
		sna_debug(1, "start node failed `%d: %s'.\n", err, strerror(errno));
		return err;
	}
#endif

	/* define dlc. */
	list_for_each(le, &g->dlc_list) {
		dlc_i = list_entry(le, dlc_info, list);
		if (!new(dlc_n))
			return -ENOMEM;
		memcpy(&dlc_n->use_name, &dlc_i->use_name, SNA_USE_NAME_LEN);
		memcpy(&dlc_n->netid, &g->node_name, sizeof(sna_netid));
		memcpy(&dlc_n->name, &dlc_i->interface, SNA_RESOURCE_NAME_LEN);
		memcpy(&dlc_n->saddr, &dlc_i->port, SNA_RESOURCE_NAME_LEN);
		dlc_n->btu	= dlc_i->btu;
		dlc_n->mia	= dlc_i->mia;
		dlc_n->moa	= dlc_i->moa;
		err = nof_define_port(sna_sk, dlc_n);
		free(dlc_n);
		if (err < 0) {
			sna_debug(1, "define port failed `%d: %s'.\n", err, strerror(errno));
			return err;
		}
	}

	/* define link. */
	list_for_each(le, &g->link_list) {
		link_i = list_entry(le, link_info, list);
		if (!new(link_n))
			return -ENOMEM;
		memcpy(&link_n->use_name, &link_i->use_name, SNA_USE_NAME_LEN);
		memcpy(&link_n->netid, &g->node_name, sizeof(sna_netid));
		memcpy(&link_n->plu_name, &link_i->plu_name, sizeof(sna_netid));
		memcpy(&link_n->dname, &link_i->dstaddr, MAX_ADDR_LEN);
		strcpy(link_n->name, link_i->interface);
		memcpy(&link_n->saddr, &link_i->port, SNA_RESOURCE_NAME_LEN);
		memcpy(&link_n->daddr, &link_i->dstport, SNA_RESOURCE_NAME_LEN);
		link_n->plu_node_id		= link_i->plu_node_id;
		link_n->role			= link_i->role;
		link_n->direction		= link_i->direction;
		link_n->byteswap		= link_i->byteswap;
		link_n->retry_on_fail		= link_i->retry_on_fail;
		link_n->retry_times		= link_i->retry_times;
		link_n->autoact			= link_i->autoact;
		link_n->tg_number		= link_i->tg_number;
		link_n->cost_per_byte		= link_i->cost_per_byte;
		link_n->cost_per_connect_time	= link_i->cost_per_connect_time;
		link_n->effective_capacity	= link_i->effective_capacity;
		link_n->propagation_delay	= link_i->propagation_delay;
		link_n->security		= link_i->security;
		link_n->user1			= link_i->user1;
		link_n->user2			= link_i->user2;
		link_n->user3			= link_i->user3;
		err = nof_define_link_station(sna_sk, link_n);
		free(link_n);
		if (err < 0) {
			sna_debug(1, "define ls failed `%d: %s'.\n", err, strerror(errno));
			return err;
		}
	}

	/* define cos. */
	list_for_each(le, &g->cos_list) {
		cos_i = list_entry(le, cos_info, list);
		if (!new(cos_n))
			return -ENOMEM;
		memcpy(&cos_n->name, &cos_i->cos_name, SNA_RESOURCE_NAME_LEN);
                cos_n->weight                   = cos_i->weight;
                cos_n->tx_priority              = cos_i->tx_priority;
                cos_n->default_cos_invalid      = cos_i->default_cos_invalid;
                cos_n->default_cos_null         = cos_i->default_cos_null;
                cos_n->min_cost_per_connect     = cos_i->min_cost_per_connect;
		cos_n->max_cost_per_connect	= cos_i->max_cost_per_connect;
                cos_n->min_cost_per_byte        = cos_i->min_cost_per_byte;
		cos_n->max_cost_per_byte	= cos_i->max_cost_per_byte;
                cos_n->min_security             = cos_i->min_security;
                cos_n->max_security             = cos_i->max_security;
                cos_n->min_propagation_delay    = cos_i->min_propagation_delay;
                cos_n->max_propagation_delay    = cos_i->max_propagation_delay;
                cos_n->min_effective_capacity   = cos_i->min_effective_capacity;
                cos_n->max_effective_capacity   = cos_i->max_effective_capacity;
                cos_n->min_user1                = cos_i->min_user1;
                cos_n->max_user1                = cos_i->max_user1;
                cos_n->min_user2                = cos_i->min_user2;
                cos_n->max_user2                = cos_i->max_user2;
                cos_n->min_user3                = cos_i->min_user3;
                cos_n->max_user3                = cos_i->max_user3;
                cos_n->min_route_resistance     = cos_i->min_route_resistance;
                cos_n->max_route_resistance     = cos_i->max_route_resistance;
                cos_n->min_node_congested       = cos_i->min_node_congested;
                cos_n->max_node_congested       = cos_i->max_node_congested;
		err = nof_define_class_of_service(sna_sk, cos_n);
                free(cos_n);
                if (err < 0) {
                        sna_debug(1, "define cos failed `%d: %s'.\n", err, strerror(errno));
                        return err;
                }
	}
	
	/* define mode. */
	list_for_each(le, &g->mode_list) {
		mode_i = list_entry(le, mode_info, list);
                if (!new(mode_n))
                        return -ENOMEM;
		memcpy(&mode_n->netid, &g->node_name, sizeof(sna_netid));
		memcpy(&mode_n->netid_plu, &mode_i->plu_name, sizeof(sna_netid));
		memcpy(mode_n->mode_name, &mode_i->name, SNA_RESOURCE_NAME_LEN);
		memcpy(mode_n->cos_name, mode_i->cos_name, SNA_RESOURCE_NAME_LEN);
		mode_n->crypto			= mode_i->encryption;
		mode_n->tx_pacing		= mode_i->tx_pacing;
		mode_n->rx_pacing		= mode_i->rx_pacing;
		mode_n->tx_max_ru		= mode_i->max_tx_ru;
		mode_n->rx_max_ru		= mode_i->max_rx_ru;
		mode_n->min_conwinners		= mode_i->min_conwinners;
		mode_n->min_conlosers		= mode_i->min_conlosers;
		mode_n->max_sessions		= mode_i->max_sessions;
		mode_n->auto_activation		= mode_i->auto_activation;
                err = nof_define_mode(sna_sk, mode_n);
                free(mode_n);
                if (err < 0) {
                        sna_debug(1, "define mode failed `%d: %s'.\n", err, strerror(errno));
                        return err;
                }
	}

	/* define lu. */
	list_for_each(le, &g->lu_list) {
		lu_i = list_entry(le, lu_info, list);
		if (lu_i->type) {
			if (!new(plu_n))
				return -ENOMEM;
			memcpy(&plu_n->use_name, &lu_i->use_name, SNA_USE_NAME_LEN);
			memcpy(&plu_n->netid, &g->node_name, sizeof(sna_netid));
			memcpy(&plu_n->netid_plu, &lu_i->plu_name, sizeof(sna_netid));
			memcpy(&plu_n->netid_fqcp, &lu_i->fqcp_name, sizeof(sna_netid));
			err = nof_define_partner_lu(sna_sk, plu_n);
			free(plu_n);
		} else {
                	if (!new(llu_n))
                	        return -ENOMEM;
			memcpy(&llu_n->use_name, &lu_i->use_name, SNA_USE_NAME_LEN);
			memcpy(&llu_n->netid, &g->node_name, sizeof(sna_netid));
			memcpy(llu_n->lu_name, lu_i->name, SNA_RESOURCE_NAME_LEN);
			llu_n->sync_point	= lu_i->syncpoint;
			llu_n->lu_sess_limit	= lu_i->lu_sess_limit;
                	err = nof_define_local_lu(sna_sk, llu_n);
                	free(llu_n);
		}
                if (err < 0) {
                        sna_debug(1, "define lu failed `%d: %s'.\n", err, strerror(errno));
                        return err;
                }
	}
	
	/* define cpic. */
	list_for_each(le, &g->cpic_list) {
		cpic_i = list_entry(le, cpic_info, list);
                if (!new(cpic_n))
                        return -ENOMEM;
		memcpy(&cpic_n->netid, &g->node_name, sizeof(sna_netid));
		memcpy(&cpic_n->netid_plu, &cpic_i->plu_name, sizeof(sna_netid));
		strcpy(cpic_n->sym_dest_name, cpic_i->sym_dest_name);
		strcpy(cpic_n->mode_name, cpic_i->mode_name);
		strcpy(cpic_n->tp_name, cpic_i->tp_name);
                err = nof_define_cpic_side_info(sna_sk, cpic_n);
                free(cpic_n);
                if (err < 0) {
                        sna_debug(1, "define cpic side info failed `%d: %s'.\n", err, strerror(errno));
                        return err;
                }
	}
	   
	return 0;
}

int sna_load_help(void)
{
	printf("Usage: %s load <config_file>\n", name_s);
	exit(1);
}

int sna_load(int argc, char **argv)
{
	char config_file[_PATH_SNACFG_CONF_MAX];
	int err, sk;
	
	if (argc != 1)
		sna_load_help();
	if (strlen(*argv) >= _PATH_SNACFG_CONF_MAX) {
		sna_debug(1, "path to configuration file `%s' is too long\n", 
			*argv);
		exit(1);
	}
	strcpy(config_file, *argv);
	err = sna_read_config_file(config_file);
	if (err < 0) {
		sna_debug(1, "error (%d) while reading config file `%s'\n",
			err, config_file);
		return err;
	}
	if (sna_debug_level >= 2)
		sna_print_config(sna_config_info);
	sk = sna_nof_connect();
	if (sk < 0)
		return sk;
	err = sna_execute_config_info(sk, sna_config_info);
	if (err < 0) {
		sna_print_config(sna_config_info);
		sna_debug(1, "error (%d) while executing configuration.\n", err);
	}
	sna_nof_disconnect(sk);
	return err;
}

int sna_reload(int argc, char **argv)
{

	return 0;
}

int sna_unload(int argc, char **argv)
{

	return 0;
}
