/* display.c: functions to display sna node information.
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
#include <linux/sna.h>
#include <linux/cpic.h>
#include <version.h>
#include <nof.h>
#include <lar.h>

#include "snaconfig.h"
#include "paths.h"

extern int sna_sk;
extern int sna_debug;
extern int opt_a;

extern struct wordmap mode_types[];
extern struct wordmap on_types[];
extern struct wordmap prop_types[];
extern struct wordmap tx_types[];
extern struct wordmap security_types[];

static struct sna_all_info *sna_list = NULL;

char *sna_pr_state(unsigned short s)
{
	static char buff[20];

	if(s & SNA_DOWN) 
		sprintf(buff, "%s ", "DOWN");
	if(s & SNA_UP) 
		sprintf(buff, "%s ", "UP");
	if(s & SNA_STOPPED) 
		sprintf(buff, "%s ", "STOPPED");
	if(s & SNA_RUNNING) 
		sprintf(buff, "%s ", "RUNNING");

	return (buff);
}

/* Print one entry from data in 'info'. The data has already been parsed and
 * verified. All data will be there. Just make it look nice.
 */
int se_print(struct sna_all_info *sna)
{
        struct dlcreq 	*dl;
        struct portreq 	*port;
        struct lsreq 	*ls;
        struct modereq 	*mode;
        struct lureq 	*lu;
        struct plureq 	*plu;
	struct cpicsreq *cpics;
	struct cosreq 	*cos;

        if(sna_debug > 5)
                printf("se_print\n");

        printf("%-s.%-s  ", sna->net, sna->name);
        printf("nodeid:0x%s  ", sna_pr_nodeid(&sna->nodeid));
        printf("nodetype:");
        if(sna->type & SNA_LEN_END_NODE) printf("Len  ");
        if(sna->type & SNA_APPN_END_NODE) printf("Appn_En  ");
        if(sna->type & SNA_APPN_NET_NODE) printf("Appn_Nn  ");
	printf("%s ", sna_pr_state(sna->node_status));
        printf("\n");

	if(opt_a)
	{
		printf("          ");
        	printf("maxlus:%ld ", sna->max_lus);
        	printf("curlus:%d ", 0);
        	printf("luseg:%s ", sna_pr_word(on_types, sna->lu_seg));
        	printf("bindseg:%s ", sna_pr_word(on_types, sna->bind_seg));
        	printf("\n");
	}

        for(dl = sna->dl; dl != NULL; dl = dl->next)
        {
                printf("     ");
                printf("datalink:%s  ", dl->devname);
                printf("type:");
                if(!strncmp(dl->devname, "eth", 3)
                        || !strncmp(dl->devname, "tr", 2))
                        printf("llc2  ");
                else
                {
                        if(!strncmp(dl->devname, "lo", 2))
                                printf("loopback  ");
                        else
                                printf("unknown  ");
                }
                printf("numports:%ld  ", dl->port_qlen);
		printf("%s ", sna_pr_state(dl->flags));
                printf("\n");

                for(port = dl->port; port != NULL; port = port->next)
                {
                        printf("     ");
                        printf("port:0x%02X  ", port->saddr[0]);
                        printf("role:");
                        if(port->role & SNA_PORT_ROLE_PRI)
                                printf("pri  ");
                        if(port->role & SNA_PORT_ROLE_SEC)
                                printf("sec  ");
                        if(port->role & SNA_PORT_ROLE_NEG)
                                printf("neg  ");
                        printf("numls:%ld  ", port->ls_qlen);
                        printf("mia:%ld  ", port->mia);
                        printf("moa:%ld  ", port->moa);
                        printf("mtu:%ld  ", port->btu);
			printf("%s ", sna_pr_state(port->flags));
                        printf("\n");

                        for(ls = port->ls; ls != NULL; ls = ls->next)
                        {
                                printf("     ");
                                printf("linkstation:%s  ", sna_pr_ether(ls->dname));
                                printf("port:0x%02X  ", ls->daddr[0]);
                                printf("dev:%s  ", ls->devname);
				printf("%s ", sna_pr_state(ls->flags));
                                printf("\n");

				if(!opt_a) 
                        		continue;

				printf("          ");
                                printf("byteswap:%s ", 
					sna_pr_word(on_types, ls->byteswap));
                                printf("aact:%s ",
					sna_pr_word(on_types, ls->auto_act));
                                printf("adeact:%s ",
					sna_pr_word(on_types, ls->auto_deact));
				printf("\n");
                        }
                }
        }

        for(lu = sna->lu; lu != NULL; lu = lu->next)
        {
                printf("     ");
                printf("lclu:%s  ", lu->name);
                printf("sync_point:%s  ",sna_pr_word(on_types, lu->sync_point));
                printf("limit:%ld  ", lu->lu_sess_limit);
		printf("%s ", sna_pr_state(lu->flags));
                printf("\n");
        }

	/* remote_lu:LNXSNA.IBM
	 */
        for(plu = sna->plu; plu != NULL; plu = plu->next)
        {
                printf("     ");
                printf("rtlu:%s  ", sna_pr_netid(&plu->plu_name));
                printf("parallel_sessions:%s  ", 
			sna_pr_word(on_types, plu->parallel_ss));
                printf("cnv_security:%s  ",
			sna_pr_word(on_types, plu->cnv_security));
		printf("%s ", sna_pr_state(plu->flags));
                printf("\n");
        }

	if(!opt_a)
		return (0);

	/* mode:BLANK  cos:#CONNECT  plu:LNXSNA.EHEAD
	 * UP RUNNING  Crypto:off Max_sessions:8192 Auto_activation:off
	 * TX pacing:3 MTU:1024 RX pacing:3 MRU:1024 MCW:4096 MCL:2048
	 * ACTIVE  sessions:10 conwinners:5 conlosers:5
	 * PENDING sessions:3 conwinners:1 conlosers:2
	 */
        for(mode = sna->mode; mode != NULL; mode = mode->next)
        {
                printf("     ");
                printf("mode:%-8s  ", mode->mode_name);
		printf("cos:%-8s  ", mode->cos_name);
		printf("plu:%s  ", sna_pr_netid(&mode->plu_name));
		printf("%s ", sna_pr_state(mode->flags));
		printf("\n");

		if(!opt_a)	/* We are only showing a short list */
			continue;

		printf("          ");
		printf("Crypto:%s ", sna_pr_word(on_types, mode->crypto));
		printf("Max_sessions:%ld ", mode->max_sessions);
		printf("Auto_activation:%s ", 
			sna_pr_word(on_types, mode->auto_activation));
		printf("\n");

		printf("          ");
		printf("TX pacing:%ld ", mode->tx_pacing);
                printf("MTU:%ld ", mode->max_tx_ru);
		printf("Rx pacing:%ld ", mode->rx_pacing);
                printf("MRU:%ld ", mode->max_rx_ru);
		printf("MINCW:%ld ", mode->min_conwinners);
		printf("MINCL:%ld ", mode->min_conlosers);
                printf("\n");

		printf("          ");
		printf("ACTIVE  sessions:%ld ", mode->act_sessions);
		printf("conwinners:%ld ", mode->act_conwinners);
		printf("conlosers:%ld ", mode->act_conlosers);
                printf("\n");

		printf("          ");
		printf("PENDING sessions:%ld ", mode->pend_sessions);
		printf("conwinners:%ld ", mode->pend_conwinners);
		printf("conlosers:%ld ", mode->pend_conlosers);
		printf("\n");
        }

        for(cpics = sna->cpics; cpics != NULL; cpics = cpics->next)
        {
                printf("     ");
                printf("cpic:%-9s ", cpics->sym_dest_name);
                printf("plu:%-s  ", cpics->netid_plu);
                printf("mode:%s  ", cpics->mode_name);
                printf("tp:%s  ", cpics->tp_name);
                printf("\n");
        }

	for(cos = sna->cos; cos != NULL; cos = cos->next)
	{
		printf("     ");
		printf(" cos:%-9s ", cos->name);
		printf("weight:%-3d ", cos->weight);
		printf("tx_priority:%-8s ", 
			sna_pr_word(tx_types, cos->tx_priority));
		printf("dnull:%-4s ", 
			sna_pr_word(on_types, cos->default_cos_null));
		printf("dinvalid:%s ",
			sna_pr_word(on_types, cos->default_cos_invalid));
		printf("\n");
	}

        return (0);
}

/* Gather all the node information and stuff in *sna */
int sna_fetch(struct sna_all_info *sna)
{
        struct sna_qsna *n;
        struct sna_qdlc *d, *dlc;
        struct sna_qport *p, *port;
        struct sna_qls *l, *ls;
        struct sna_qmode *m, *mode;
        struct sna_qlu *ll, *lu;
        struct sna_qplu *pl, *plu;
        struct sna_qcpics *c, *cs;
	struct sna_qcos *cos, *coss;
	struct dlcreq *dl;

        if(sna_debug > 5)
                printf("sna_fetch\n");

        n = nof_query_node(sna_sk, sna->net, sna->name);
        if(!n)
                return (-ENOENT);

	sna->mode	= NULL;
	sna->dl		= NULL;
	sna->lu		= NULL;
	sna->plu	= NULL;
	sna->cpics	= NULL;
	sna->cos	= NULL;
        sna->type       = n->data.type;
        memcpy(&sna->nodeid, &n->data.nodeid, sizeof(struct sna_nodeid));
        sna->lu_seg     = n->data.lu_seg;
        sna->bind_seg   = n->data.bind_seg;
        sna->max_lus    = n->data.max_lus;
        sna->node_status= n->data.node_status;

        d = nof_query_dlc(sna_sk, sna->net, sna->name, "eth0");
        if(!d)
                return (0);

        for(dlc = d; dlc != NULL; dlc = dlc->next)
        {
                new(dl);
		memcpy(dl, &dlc->data, sizeof(struct dlcreq));
                dl->port 	= NULL;
                dl->next	= sna->dl;
                sna->dl         = dl;
        }

        /* Link each port with the correct DLC */
        p = nof_query_port(sna_sk, sna->net, sna->name, "*", "*");
        for(dl = sna->dl; dl != NULL; dl = dl->next)
        {
                for(port = p; port != NULL; port = port->next)
                {
                        if(!strcmp(dl->devname, port->data.devname))
                        {
				struct portreq *pt;
                                new(pt);
				memcpy(pt, &port->data,sizeof(struct portreq));
                                pt->ls          = NULL;
                                pt->next        = dl->port;
                                dl->port        = pt;
                        }
                }
        }

        l = nof_query_ls(sna_sk, sna->net, sna->name, "*", "*", "*");
        for(dl = sna->dl; dl != NULL; dl = dl->next)
        {
		struct portreq *pt;

                for(pt = dl->port; pt != NULL; pt = pt->next)
                {
                        for(ls = l; ls != NULL; ls = ls->next)
                        {
				struct lsreq *lk;

                                if(strcmp(ls->data.portname,pt->portname)
                                        && strcmp(ls->data.devname,dl->devname))
                                        break;
                                new(lk);
				memcpy(lk, &ls->data, sizeof(struct lsreq));
                                lk->next        = pt->ls;
                                pt->ls          = lk;
                        }
                }
        }

        /* Get Local LUs */
        ll = nof_query_lu(sna_sk, "*", "*", "*");
        for(lu = ll; lu != NULL; lu = lu->next)
        {
		struct lureq *llu;
                new(llu);
		memcpy(llu, &lu->data, sizeof(struct lureq));
                llu->next       = sna->lu;
                sna->lu         = llu;
        }

        /* Get Remote LUs */
        pl = nof_query_plu(sna_sk);
        for(plu = pl; plu != NULL; plu = plu->next)
        {
		struct plureq *pplu;
                new(pplu);
		memcpy(pplu, &plu->data, sizeof(struct plureq));
                pplu->next      = sna->plu;
                sna->plu        = pplu;
        }

        /* Get Modes */
        m = nof_query_mode(sna_sk, "*", "*", "*");
        for(mode = m; mode != NULL; mode = mode->next)
        {
		struct modereq *md;
                new(md);
		memcpy(md, &mode->data, sizeof(struct modereq));
                md->next        = sna->mode;
                sna->mode       = md;
        }

        /* Get CPICS */
        c = nof_query_cpic_side_info(sna_sk, "*", "*", "*");
        for(cs = c; cs != NULL; cs = cs->next)
        {
		struct cpicsreq *ccs;
                new(ccs);
		memcpy(ccs, &cs->data, sizeof(struct cpicsreq));
                ccs->next       = sna->cpics;
                sna->cpics      = ccs;
        }

	/* Get COS */
	cos = nof_query_cos(sna_sk, "*");
	for(coss = cos; coss != NULL; coss = coss->next)
	{
		struct cosreq *css;
		new(css);
		memcpy(css, &coss->data, sizeof(struct cosreq));
		css->next	= sna->cos;
		sna->cos	= css;
	}

        return (0);
}

int do_sna_fetch(struct sna_all_info *sna)
{
        if(sna_debug > 5)
                printf("do_sna_fetch\n");

        if(sna_fetch(sna) < 0)
        {
                char *errmsg;

                /* FIXME: make libnof use errno properly!!
                 */

                if (errno == ENOENT)
                        errmsg = "Node not found";
                else
                        errmsg = "Node not found";
                //      errmsg = strerror(errno);

                fprintf(stderr, "snaconfig: Error fetching node information - %s %s.%s\n",
                        errmsg, sna->net, sna->name);
                return (-1);
        }
        return (0);
}

/* Future caching mechinism.. */
struct sna_all_info *lookup_sna(struct sna_netid *n)
{
        struct sna_all_info *sna = NULL;

        if(!sna)
        {
                new(sna);
                strncpy(sna->net, n->net, 8);
                strncpy(sna->name, n->name, 8);
        }

        return (sna);
}

int do_sna_print(struct sna_all_info *sna)
{
        int res;

        if(sna_debug > 5)
                printf("do_sna_print\n");

        res = do_sna_fetch(sna);
        if(res >= 0)
                se_print(sna);

        return (res);
}

void add_node(struct sna_all_info *n)
{
        struct sna_all_info *sna, **pp;

        pp = &sna_list;
        for(sna = sna_list; sna; pp = &sna->next, sna = sna->next)
        {
                if(strcmp(n->name, sna->name) > 0)
                        break;
        }
        n->next = (*pp);
        (*pp) = n;
}

int get_netid(char *net, char *name, char *p)
{
        char buf[SNA_FQCP_NAME_LEN];

        sscanf(p, "%s", buf);
        strcpy(name, strpbrk(buf, ".")+1);
        strcpy(net, strtok(buf , "."));

        return (0);
}

int sna_readlist(void)
{
        FILE *fh;
        char buf[512];
        struct sna_all_info *sna;
        int err;

        fh = fopen(VIRTUAL_NODES, "r");
        if(!fh)
        {
                printf("Cannot open %s\n", VIRTUAL_NODES);
                return (-1);
        }

        fgets(buf, sizeof(buf), fh);    /* eat one line */
        fgets(buf, sizeof(buf), fh);    /* eat another line */

        err = 0;
        while(fgets(buf, sizeof(buf), fh))
        {
                new(sna);
                get_netid(sna->net, sna->name, buf);
                add_node(sna);
        }

        return (err);
}

int for_all_nodes(int (*doit)(struct sna_all_info *))
{
        struct sna_all_info *sna;

        if(!sna_list && (sna_readlist() < 0))
                return (-1);
        for(sna = sna_list; sna; sna = sna->next)
        {
                int err = doit(sna);
                if(err)
                        return (err);
        }

        return (0);
}

/* Gather all the data available, parse it, format it, verifiy it, then
 * send it to be printed.
 */
int sna_print(struct sna_netid *n)
{
        int res;

        if(sna_debug > 5)
                printf("sna_print\n");

        if(!n)
                res = for_all_nodes(do_sna_print);
        else
        {
                struct sna_all_info *sna;

                sna = lookup_sna(n);
                res = do_sna_fetch(sna);
                if(res >= 0)
                        se_print(sna);
        }

        return (res);
}

/* The display functions below are more for debugging, but are made
 * available to anyone. Output all the information in a global struct.
 * useful for finding silly sna errors due to bad input.
 */
int sna_print_global(global *g)
{
	struct dlc *dlcs;
	struct link *links;
	struct llu *llus;
	struct rlu *rlus;
	struct mode *modes;
	struct cpic_side *cpics;
	struct cos *coss;

	printf("\n================ Global SNA Input Structure ================\n");

	if(g == NULL)
	{
		printf("Global data is NULL!\n");
		goto pr_done;
	}

	printf("fq_node_name: %s\n", g->fq_node_name);
	if(iscntrl(g->node_type[0]))
		printf("node_type: blank\n");
	else
		printf("node_type: %s\n", g->node_type);
	if(iscntrl(g->node_id[0]))
		printf("node_id: blank\n");
	else
		printf("node_id: %s\n", g->node_id);
	printf("debug_level: %d\n", g->debug_level);
	printf("command_mode: %s\n", sna_pr_word(mode_types, g->mode));

	/* DLC */
	for(dlcs = g->dlcs; dlcs != NULL; dlcs = dlcs->next)
	{
		printf("-------- Data link control --------\n");
		printf("interface: %s\n", dlcs->interface);
		printf("port: %s\n", dlcs->port);
		printf("role: %s\n", dlcs->role);
		printf("btu: %d\n", dlcs->btu);
		printf("mia: %d\n", dlcs->mia);
		printf("moa: %d\n", dlcs->moa);
		printf("mode: %d\n", dlcs->mode);
		printf("-----------------------------------\n");
	}

	/* Link */
	for(links = g->links; links != NULL; links = links->next)
	{
		printf("-------- Link station --------\n");
		printf("interface: %s\n", links->interface);
		printf("port: %s\n", links->port);
		printf("plu_name: %s\n", links->plu_name);
		printf("dstaddr: %s\n", links->dstaddr);
		printf("dstport: %s\n", links->dstport);
		printf("byteswap: %d\n", links->byteswap);
		printf("retry_on_fail: %d\n", links->retry_on_fail);
		printf("retry_times: %d\n", links->retry_times);
		printf("autoact: %d\n", links->autoact);
		printf("autodeact: %d\n", links->autodeact);
		printf("tg_number: %d\n", links->tg_number);
		printf("cost_per_byte: %d\n", links->cost_per_byte);
		printf("cost_per_connect_time: %d\n", links->cost_per_connect_time);
		printf("effective_capacity: %d\n", links->effective_capacity);
		printf("propagation_delay: %d\n", links->propagation_delay);
		printf("security: %d\n", links->security);
		printf("user1: %d\n", links->user1);
		printf("user2: %d\n", links->user2);
		printf("user3: %d\n", links->user3);
		printf("mode: %d\n", links->mode);
		printf("------------------------------\n");
	}

	/* llu */
	for(llus = g->llus; llus != NULL; llus = llus->next)
	{
		printf("-------- Local logical unit --------\n");
		printf("name: %s\n", llus->name);
		printf("syncpoint: %d\n", llus->syncpoint);
		printf("lu_sess_limit: %d\n", llus->lu_sess_limit);
		printf("mode: %d\n", llus->mode);
		printf("------------------------------------\n");
	}

	/* rlus */
	for(rlus = g->rlus; rlus != NULL; rlus = rlus->next)
	{
		printf("-------- Remote logical unit --------\n");
		printf("plu_name: %s\n", rlus->plu_name);
		printf("fqcp_name: %s\n", rlus->fqcp_name);
		printf("mode: %d\n", rlus->mode);
		printf("-------------------------------------\n");
	}

	/* modes */
	for(modes = g->modes; modes != NULL; modes = modes->next)
	{
		printf("-------- Mode --------\n");
		printf("name: %s\n", modes->name);
		printf("plu_name: %s\n", modes->plu_name);
		printf("cos_name: %s\n", modes->cos_name);
		printf("encryption: %d\n", modes->encryption);
		printf("tx_pacing: %d\n", modes->tx_pacing);
		printf("rx_pacing: %d\n", modes->rx_pacing);
		printf("max_tx_ru: %d\n", modes->max_tx_ru);
		printf("max_rx_ru: %d\n", modes->max_rx_ru);
		printf("max_sessions: %d\n", modes->max_sessions);
		printf("min_conwinners: %d\n", modes->min_conwinners);
		printf("min_conlosers: %d\n", modes->min_conlosers);
		printf("auto_activation: %d\n", modes->auto_activation);
		printf("mode: %d\n", modes->mode);
		printf("----------------------\n");
	}

	/* cpics */
	for(cpics = g->cpics; cpics != NULL; cpics = cpics->next)
	{
		printf("-------- CPI-C --------\n");
		printf("sym_dest_name: %s\n", cpics->sym_dest_name);
		printf("mode_name: %s\n", cpics->mode_name);
		printf("plu_name: %s\n", cpics->plu_name);
		printf("tp_name: %s\n", cpics->tp_name);
		printf("mode: %d\n", cpics->mode);
		printf("-----------------------\n");
	}

	/* coss */
	for(coss = g->coss; coss != NULL; coss = coss->next)
	{
		printf("-------- Cost of Service --------\n");
		printf("cos_name: %s\n", coss->cos_name);
		printf("weight: %d\n", coss->weight);
		printf("tx_priority: %d\n", coss->tx_priority);
		printf("default_cos_invalid: %d\n", coss->default_cos_invalid);
		printf("default_cos_null: %d\n", coss->default_cos_null);
		printf("min_cost_per_connect: %d\n", coss->min_cost_per_connect);
		printf("max_cost_per_connect: %d\n", coss->max_cost_per_connect);
		printf("min_cost_per_byte: %d\n", coss->min_cost_per_byte);
		printf("max_cost_per_byte: %d\n", coss->max_cost_per_byte);
		printf("min_security: %d\n", coss->min_security);
		printf("max_security: %d\n", coss->max_security);
		printf("min_propagation_delay: %d\n", coss->min_propagation_delay);
		printf("max_propagation_delay: %d\n", coss->max_propagation_delay);
		printf("min_effective_capacity: %d\n", coss->min_effective_capacity);
		printf("max_effective_capacity: %d\n", coss->max_effective_capacity);
		printf("min_user1: %d\n", coss->min_user1);
		printf("max_user1: %d\n", coss->max_user1);
		printf("min_user2: %d\n", coss->min_user2);
		printf("max_user2: %d\n", coss->max_user2);
		printf("min_user3: %d\n", coss->min_user3);
		printf("max_user3: %d\n", coss->max_user3);
		printf("min_route_resistance: %d\n", coss->min_route_resistance);
		printf("max_route_resistance: %d\n", coss->max_route_resistance);
		printf("min_node_congested: %d\n", coss->min_node_congested);
		printf("max_node_congested: %d\n", coss->max_node_congested);
		printf("mode: %d\n", coss->mode);
		printf("---------------------------------\n");
	}
	
pr_done:
	printf("=========================================================\n");
	return (0);
}
