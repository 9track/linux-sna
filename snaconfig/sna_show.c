/* sna_show.c:
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
#include <linux/if_arp.h>
#include <linux/sna.h>
#include <linux/cpic.h>
#include <nof.h>

/* our stuff. */
#include "sna_list.h"
#include "snaconfig.h"
#include "sna_xwords.h"

extern char     version_s[];
extern char     name_s[];
extern char     desc_s[];
extern char     maintainer_s[];
extern char     company_s[];
extern char     web_s[];
extern int      sna_debug_level;
extern global_info *sna_config_info;

static struct sna_all_info *sna_list = NULL;
static int sna_show_all_flag = 0;

char *sna_pr_state(unsigned short s)
{
        static char buff[20];

        if (s & SNA_DOWN)
                sprintf(buff, "%s", "DOWN");
        if (s & SNA_UP)
                sprintf(buff, "%s", "UP");
        if (s & SNA_STOPPED)
                sprintf(buff, "%s", "INACTIVE");
        if (s & SNA_RUNNING)
                sprintf(buff, "%s", "ACTIVE");
        return buff;
}

char *sna_pr_link_type(unsigned short type)
{
	static char buff[20];

	switch (type) {
		case ARPHRD_ETHER:
			sprintf(buff, "%s", "ether");
			break;
		case ARPHRD_LOOPBACK:
			sprintf(buff, "%s", "loop");
			break;
	}
	return buff;	
}

char *sna_pr_ls_status(struct lsreq *ls)
{
	static char buff[20];

	if (ls->state == SNA_LS_STATE_DEFINED) {
		sprintf(buff, "%s", "OFFLINE");
		goto out;
	}

	if (ls->state == SNA_LS_STATE_ACTIVATED) {
		if (ls->co_status == CO_FAIL)
			sprintf(buff, "%s", "FAILED");
		else {
			if (ls->direction == SNA_LS_DIR_IN)
				sprintf(buff, "%s", "INCOMING");
			else
				sprintf(buff, "%s", "PENDING");
		}
		goto out;
	}

	if (ls->state == SNA_LS_STATE_ACTIVE) {
		sprintf(buff, "%s", "ONLINE");
		goto out;
	}
out:	return buff;
}

static int sna_show_print(struct sna_all_info *sna)
{
        struct dlcreq   *dl;
        struct portreq  *port;
        struct lsreq    *ls;
        struct modereq  *mode;
        struct lureq    *lu;
        struct plureq   *plu;
        struct cpicsreq *cpics;
        struct cosreq   *cos;

	sna_debug(5, "init\n");
	printf("%-s.%-s ", sna->net, sna->name);
	printf("<%s> ", sna_pr_state(sna->node_status));
        printf("nodeid %s ", sna_pr_nodeid(sna->nodeid));
        printf("nodetype/");
        if (sna->type == SNA_LEN_END_NODE) printf("Len ");
        if (sna->type == SNA_APPN_END_NODE) printf("Appn_En ");
        if (sna->type == SNA_APPN_NET_NODE) printf("Appn_Nn ");
        printf("\n");

        printf("  ");
        printf("maxlus:%ld ", sna->max_lus);
        printf("curlus:%d ", 0);
        printf("luseg:%s ", xword_pr_word(on_types, sna->lu_seg));
        printf("bindseg:%s ", xword_pr_word(on_types, sna->bind_seg));
        printf("\n");

        for (dl = sna->dl; dl != NULL; dl = dl->next) {
                printf("  ");
                printf("device:%s ", dl->dev_name);
		printf("<%s> ", sna_pr_state(dl->flags));
		printf("mtu %d ", dl->mtu);
		printf("link/%s %s ", sna_pr_link_type(dl->type), 
			sna_pr_ether(dl->dev_addr));
                printf("\n");

                for (port = dl->port; port != NULL; port = port->next) {
                        printf("  ");
			printf("dlc:%s ", port->use_name);
			printf("<%s> ", sna_pr_state(port->flags));
			printf("%s@0x%02X ", port->dev_name, port->saddr[0]);
			printf("mia %d ", port->mia);
                        printf("moa %d ", port->moa);
                        printf("btu %d ", port->btu);
                        printf("links/%d ", port->ls_qlen);
                        printf("\n");

                        for (ls = port->ls; ls != NULL; ls = ls->next) {
                                printf("  ");
				printf("link:%s ", ls->use_name);
				printf("<%s|%s> ", sna_pr_state(ls->flags), sna_pr_ls_status(ls));
				printf("role %s/%s ", xword_pr_word(role_types, ls->role),
					xword_pr_word(role_types, ls->effective_role));
				printf("tg %d/%d ", ls->tg_number, ls->effective_tg);
				printf("btu %d/%d ", ls->tx_max_btu, ls->rx_max_btu);
				printf("win %d/%d ", ls->tx_window, ls->rx_window);
				printf("\n");
				
				printf("    ");
				printf("plu/%s ", sna_pr_netid(&ls->plu_name));
				printf("nodeid %s ", sna_pr_nodeid(ls->plu_node_id));
				printf("%s@0x%02X ", sna_pr_ether(ls->plu_mac_addr),
					ls->plu_port);
				printf("\n");

				printf("    ");
				printf("retry/%s ", xword_pr_word(on_types, ls->retry_on_fail));
				printf("retries %d/%d ", ls->retries, ls->retry_times);
				printf("xid %d ", ls->xid_count);
				printf("xid_init %d ", ls->xid_init_method);
				printf("autoact/%s ", xword_pr_word(on_types, ls->autoact));
				printf("autodeact/%s ", xword_pr_word(on_types, ls->autodeact));
				printf("\n");

				printf("    ");
				printf("byteswap/%s ", xword_pr_word(on_types, ls->byteswap));
				printf("security/%s ", xword_pr_word(security_types, ls->security));
				printf("cpb %d ", ls->cost_per_byte);
				printf("cpc %d ", ls->cost_per_connect_time);
				printf("propagation_delay %d ", ls->propagation_delay);
				printf("\n");
                        }
                }
        }

        for (lu = sna->lu; lu != NULL; lu = lu->next) {
                printf("  ");
                printf("lclu:%s  ", lu->name);
                printf("sync_point:%s  ", xword_pr_word(on_types, lu->sync_point));
                printf("limit:%ld  ", lu->lu_sess_limit);
                printf("%s ", sna_pr_state(lu->flags));
                printf("\n");
        }

        for (plu = sna->plu; plu != NULL; plu = plu->next) {
                printf("  ");
                printf("rtlu:%s  ", sna_pr_netid(&plu->plu_name));
                printf("parallel_sessions:%s  ",
                        xword_pr_word(on_types, plu->parallel_ss));
                printf("cnv_security:%s  ",
                        xword_pr_word(on_types, plu->cnv_security));
                printf("%s ", sna_pr_state(plu->flags));
                printf("\n");
        }

	if (!sna_show_all_flag)
		return 0;
	
        /* mode:BLANK  cos:#CONNECT  plu:LNXSNA.EHEAD
         * UP RUNNING  Crypto:off Max_sessions:8192 Auto_activation:off
         * TX pacing:3 MTU:1024 RX pacing:3 MRU:1024 MCW:4096 MCL:2048
         * ACTIVE  sessions:10 conwinners:5 conlosers:5
         * PENDING sessions:3 conwinners:1 conlosers:2
         */
        for (mode = sna->mode; mode != NULL; mode = mode->next) {
                printf("     ");
                printf("mode:%-8s  ", mode->mode_name);
                printf("cos:%-8s  ", mode->cos_name);
                printf("plu:%s  ", sna_pr_netid(&mode->plu_name));
                printf("%s ", sna_pr_state(mode->flags));
                printf("\n");


                printf("          ");
                printf("Crypto:%s ", xword_pr_word(on_types, mode->crypto));
                printf("Max_sessions:%ld ", mode->max_sessions);
                printf("Auto_activation:%s ",
                        xword_pr_word(on_types, mode->auto_activation));
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

        for (cpics = sna->cpics; cpics != NULL; cpics = cpics->next) {
                printf("     ");
                printf("cpic:%-9s ", cpics->sym_dest_name);
                printf("plu:%-s  ", cpics->netid_plu);
                printf("mode:%s  ", cpics->mode_name);
                printf("tp:%s  ", cpics->tp_name);
                printf("\n");
        }

        for (cos = sna->cos; cos != NULL; cos = cos->next) {
                printf("     ");
                printf(" cos:%-9s ", cos->name);
                printf("weight:%-3d ", cos->weight);
                printf("tx_priority:%-8s ",
                        xword_pr_word(tx_types, cos->tx_priority));
                printf("dnull:%-4s ",
                        xword_pr_word(on_types, cos->default_cos_null));
                printf("dinvalid:%s ",
                        xword_pr_word(on_types, cos->default_cos_invalid));
                printf("\n");
        }
        return 0;
}

static int sna_show_gather_info(struct sna_all_info *sna)
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
	int sna_sk;

	sna_debug(5, "init\n");
	sna_sk = sna_nof_connect();
        if (sna_sk < 0)
                return sna_sk;
        n = nof_query_node(sna_sk, sna->net, sna->name);
        if (!n)
                return -ENOENT;
        sna->mode       = NULL;
        sna->dl         = NULL;
        sna->lu         = NULL;
        sna->plu        = NULL;
        sna->cpics      = NULL;
        sna->cos        = NULL;
        sna->type       = n->data.type;
        sna->nodeid     = n->data.nodeid;
        sna->lu_seg     = n->data.lu_seg;
        sna->bind_seg   = n->data.bind_seg;
        sna->max_lus    = n->data.max_lus;
        sna->node_status= n->data.node_status;

        d = nof_query_dlc(sna_sk, sna->net, sna->name, "eth0");
        if (!d)
                return 0;
        for (dlc = d; dlc != NULL; dlc = dlc->next) {
                new(dl);
                memcpy(dl, &dlc->data, sizeof(struct dlcreq));
                dl->port        = NULL;
                dl->next        = sna->dl;
                sna->dl         = dl;
        }

        /* link each port with the correct dlc. */
        p = nof_query_port(sna_sk, sna->net, sna->name, "*", "*");
        for (dl = sna->dl; dl != NULL; dl = dl->next) {
                for (port = p; port != NULL; port = port->next) {
                        if (!strcmp(dl->dev_name, port->data.dev_name)) {
                                struct portreq *pt;
                                new(pt);
                                memcpy(pt, &port->data, sizeof(struct portreq));
                                pt->ls          = NULL;
                                pt->next        = dl->port;
                                dl->port        = pt;
                        }
                }
        }

	/* link each ls with the correct port. */
        l = nof_query_ls(sna_sk, sna->net, sna->name, "*", "*", "*");
        for (dl = sna->dl; dl != NULL; dl = dl->next) {
                struct portreq *pt;

                for (pt = dl->port; pt != NULL; pt = pt->next) {
                        for (ls = l; ls != NULL; ls = ls->next) {
                                struct lsreq *lk;

                                if (strcmp(ls->data.port_name, pt->use_name)
                                        && strcmp(ls->data.dev_name, dl->dev_name))
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
        for (lu = ll; lu != NULL; lu = lu->next) {
                struct lureq *llu;
                new(llu);
                memcpy(llu, &lu->data, sizeof(struct lureq));
                llu->next       = sna->lu;
                sna->lu         = llu;
        }

        /* Get Remote LUs */
        pl = nof_query_plu(sna_sk);
        for (plu = pl; plu != NULL; plu = plu->next) {
                struct plureq *pplu;
                new(pplu);
                memcpy(pplu, &plu->data, sizeof(struct plureq));
                pplu->next      = sna->plu;
                sna->plu        = pplu;
        }

        /* Get Modes */
        m = nof_query_mode(sna_sk, "*", "*", "*");
        for (mode = m; mode != NULL; mode = mode->next) {
                struct modereq *md;
                new(md);
                memcpy(md, &mode->data, sizeof(struct modereq));
                md->next        = sna->mode;
                sna->mode       = md;
        }

        /* Get CPICS */
        c = nof_query_cpic_side_info(sna_sk, "*", "*", "*");
        for (cs = c; cs != NULL; cs = cs->next) {
                struct cpicsreq *ccs;
                new(ccs);
                memcpy(ccs, &cs->data, sizeof(struct cpicsreq));
                ccs->next       = sna->cpics;
                sna->cpics      = ccs;
        }

        /* Get COS */
        cos = nof_query_cos(sna_sk, "*");
        for (coss = cos; coss != NULL; coss = coss->next) {
                struct cosreq *css;
                new(css);
                memcpy(css, &coss->data, sizeof(struct cosreq));
                css->next       = sna->cos;
                sna->cos        = css;
        }
	sna_nof_disconnect(sna_sk);
        return 0;
}

static void sna_show_add_node(struct sna_all_info *n)
{
        struct sna_all_info *sna, **pp;

        pp = &sna_list;
        for (sna = sna_list; sna; pp = &sna->next, sna = sna->next)
                if (strcmp(n->name, sna->name) > 0)
                        break;
        n->next = (*pp);
        (*pp) = n;
	return;
}

static int sna_show_get_netid(char *net, char *name, char *p)
{
        char buf[SNA_FQCP_NAME_LEN];

        sscanf(p, "%s", buf);
        strcpy(name, strpbrk(buf, ".")+1);
        strcpy(net, strtok(buf , "."));
        return 0;
}

static int sna_show_read_nodes(void)
{
        struct sna_all_info *sna;
	char buf[512];
        int err = 0;
	FILE *fh;

        fh = fopen(_PATH_SNA_VIRTUAL_NODE, "r");
        if (!fh) {
		sna_debug(1, "unable to open `%s'.\n", _PATH_SNA_VIRTUAL_NODE);
		return -EINVAL;
	}
        fgets(buf, sizeof(buf), fh);    /* eat another line */
        while (fgets(buf, sizeof(buf), fh)) {
                if (!new(sna))
			return -ENOMEM;
                sna_show_get_netid(sna->net, sna->name, buf);
                sna_show_add_node(sna);
        }
        return err;
}

static int sna_show_all(void)
{
	struct sna_all_info *sna;

        if (!sna_list && (sna_show_read_nodes() < 0))
                return -EINVAL;
        for (sna = sna_list; sna; sna = sna->next) {
                int err;
		err = sna_show_gather_info(sna);
		if (err)
			return err;
		err = sna_show_print(sna);
                if (err)
                        return err;
        }
	return 0;
}

static int sna_show_help(void)
{
        printf("Usage: %s show [<service> <name>]\n", name_s);
        exit(1);
}

int sna_show(int argc, char **argv)
{
	if (!argc)
		return sna_show_all();
	if (argc == 1 && !matches(*argv, "all")) {
		sna_show_all_flag = 1;
		return sna_show_all();
	}
	if (argc < 2)
		sna_show_help();
	if (!matches(*argv, "node")) {
		next_arg_fail(argv, argc, sna_show_help);
                sna_debug(1, "feature currently not supported.\n");
                return 0;
        }
        if (!matches(*argv, "dlc")) {
                next_arg_fail(argv, argc, sna_show_help);
		sna_debug(1, "feature currently not supported.\n");
                return 0;
        }
        if (!matches(*argv, "link")) {
                next_arg_fail(argv, argc, sna_show_help);
		sna_debug(1, "feature currently not supported.\n");
                return 0;
        }
	sna_show_help();
	return 0;
}
