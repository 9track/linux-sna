/* parse_stdin.c: load and sna server configuration from standard input.
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

extern struct wordmap mode_types[];
extern struct wordmap on_types[];
extern struct wordmap prop_types[];
extern struct wordmap tx_types[];
extern struct wordmap security_types[];

#define SNA_PORT_ROLE_PRI       0x1
#define SNA_PORT_ROLE_SEC       0x2
#define SNA_PORT_ROLE_NEG       0x4

int sna_load_cfg_stdin(char **argv)
{
	struct sna_netid *netid;
	global *g;

        /* Get the NetID.Node name */
        new(netid);
	netid = sna_char_to_netid(*argv);
        if(*argv == (char *)NULL)
        {
                sna_print(netid);
                close(sna_sk);
                exit (0);
        }

	/* start moving information to the new global struct */
	new(g);
	sna_netid_to_char(netid, g->fq_node_name);

	/* set the debug level right away. */
	g->debug_level = sna_debug;

        /* get any node options. */
        while(*argv != (char *)NULL)
        {
                if(!strcmp(*argv, "nodeid"))
                {
			if(*++argv == NULL) help();
			strncpy(g->node_id, *argv, SNA_RESOURCE_NAME_LEN);
                        continue;
                }

                if(!strcmp(*argv, "len"))
                {
			strcpy(g->node_type, "len");
                        (void)*argv++;
                        continue;
                }

                if(!strcmp(*argv, "appn"))
                {
			strcpy(g->node_type, "appn");
                        (void)*argv++;
                        continue;
                }

                if(!strcmp(*argv, "nn"))
                {
			strcpy(g->node_type, "nn");
                        (void)*argv++;
                        continue;
                }

                /* Datalink options. */
                if(!strcmp(*argv, "dlc"))
                {
			struct dlc *dl;

                        if(*++argv == NULL) help();
			new(dl);
			dl->next = g->dlcs;
			g->dlcs  = dl;

                        /* get device/interface name */
			strcpy(dl->interface, *argv);
                        if(*++argv == NULL) help();

                        /* Get port. */
			strcpy(dl->port, *argv);
                        if(*++argv == NULL) help();

                        /* Get port type. */
                        if(!strcmp(*argv, "pri"))
                        {
				strcpy(dl->role, "pri");
                                if(*++argv == NULL) goto define_dlc;
                        }
                        else
                        {
                                if(!strcmp(*argv, "sec"))
                                {
					strcpy(dl->role, "sec");
                                        if(*++argv == NULL) goto define_dlc;
                                }
                                else
                                {
                                        if(!strcmp(*argv, "neg"))
                                        {
						strcpy(dl->role, "neg");
                                                if(*++argv == NULL) goto define_dlc;
                                        }
                                }
                        }

                        if(!strcmp(*argv, "btu"))
                        {
                                if(*++argv == NULL) help();
				dl->btu = atoi(*argv);
                                if(*++argv == NULL) goto define_dlc;
                        }

                        if(!strcmp(*argv, "mia"))
                        {
                                if(*++argv == NULL) help();
				dl->mia = atoi(*argv);
                                if(*++argv == NULL) goto define_dlc;
                        }

                        if(!strcmp(*argv, "moa"))
                        {
                                if(*++argv == NULL) help();
				dl->moa = atoi(*argv);
                                if(*++argv == NULL) goto define_dlc;
                        }

                        /* Start port if needed */
                        if(!strcmp(*argv, "start"))
                        {
				dl->mode = map_word(mode_types, "start");
				g->mode = map_word(mode_types, "sub_opts");
                                (void)*argv++;
                                continue;
                        }

                        /* Stop DLC */
                        if(!strcmp(*argv, "stop"))
                        {
				dl->mode = map_word(mode_types, "stop");
				g->mode = map_word(mode_types, "sub_opts");
                                (void)*argv++;
                                continue;
                        }

                        if(!strcmp(*argv, "delete"))
                        {
				dl->mode = map_word(mode_types, "delete");
				g->mode = map_word(mode_types, "sub_opts");
                                (void)*argv++;
                                continue;
                        }

define_dlc:
			dl->mode = map_word(mode_types, "define");
			g->mode = map_word(mode_types, "sub_opts");
                        continue;
                }

                /* Link station options. */
                if(!strcmp(*argv, "link"))
                {
			struct link *lk;

                        if(*++argv == NULL) help();
			new(lk);
			lk->next = g->links;
			g->links = lk;

                        /* Get source device */
			strcpy(lk->interface, *argv);

                        /* Get source port */
                        if(*++argv == NULL) help();
			strcpy(lk->port, *argv);

                        /* Get partner LU name */
                        if(*++argv == NULL) help();
			strcpy(lk->plu_name, *argv);

                        /* Get link dest mac addr */
	                if(*++argv == NULL) help();
			strcpy(lk->dstaddr, *argv);

                        /* Get dest port. */
                        if(*++argv == NULL) help();
			strcpy(lk->dstport, *argv);
                        if(*++argv == NULL) goto define_ls;

                        if(!strcmp(*argv, "auto_act"))
                        {
				lk->autoact = map_word(on_types, "on");
                                if(*++argv == NULL) goto define_ls;
                        }

                        if(!strcmp(*argv, "-auto_act"))
                        {
				lk->autoact = map_word(on_types, "off");
                                if(*++argv == NULL) goto define_ls;
                        }

                        if(!strcmp(*argv, "auto_deact"))
                        {
				lk->autodeact = map_word(on_types, "on");
                                if(*++argv == NULL) goto define_ls;
                        }

                        if(!strcmp(*argv, "-auto_deact"))
                        {
				lk->autodeact = map_word(on_types, "off");
                                if(*++argv == NULL) goto define_ls;
                        }

                        if(!strcmp(*argv, "byte_swap"))
                        {
				lk->byteswap = map_word(on_types, "on");
                                if(*++argv == NULL) goto define_ls;
                        }

                        if(!strcmp(*argv, "-byte_swap"))
                        {
				lk->byteswap = map_word(on_types, "off");
                                if(*++argv == NULL) goto define_ls;
                        }

                        if(!strcmp(*argv, "auto_retry"))
                        {
				lk->retry_on_fail = map_word(on_types, "on");
                                if(*++argv == NULL) goto define_ls;
                        }

                        if(!strcmp(*argv, "retry"))
                        {
                                if(*++argv == NULL) help();
				lk->retry_times = atoi(*argv);
                                if(*++argv == NULL) goto define_ls;
                        }

                        if(!strcmp(*argv, "tg_number"))
                        {
                                if(*++argv == NULL) help();
				lk->tg_number = atoi(*argv);
                                if(*++argv == NULL) goto define_ls;
                        }

                        if(!strcmp(*argv, "cost_per_byte"))
                        {
                                if(*++argv == NULL) help();
				lk->cost_per_byte = atoi(*argv);
                                if(*++argv == NULL) goto define_ls;
                        }

                        if(!strcmp(*argv, "cost_per_connect"))
                        {
                                if(*++argv == NULL) help();
				lk->cost_per_connect_time = atoi(*argv);
                                if(*++argv == NULL) goto define_ls;
                        }

                        if(!strcmp(*argv, "effective_capacity"))
                        {
                                if(*++argv == NULL) help();
				lk->effective_capacity = atoi(*argv);
                                if(*++argv == NULL) goto define_ls;
                        }

                        if(!strcmp(*argv, "propagation_delay"))
                        {
                                if(*++argv == NULL) help();
				lk->propagation_delay = atoi(*argv);
                                if(*++argv == NULL) goto define_ls;
                        }

                        if(!strcmp(*argv, "security"))
                        {
                                if(*++argv == NULL) help();
				lk->security = map_word(security_types, *argv);
                                if(*++argv == NULL) goto define_ls;
                        }

                        if(!strcmp(*argv, "user1"))
                        {
                                if(*++argv == NULL) help();
				lk->user1 = atoi(*argv);
                                if(*++argv == NULL) goto define_ls;
                        }

                        if(!strcmp(*argv, "user2"))
                        {
                                if(*++argv == NULL) help();
				lk->user2 = atoi(*argv);
                                if(*++argv == NULL) goto define_ls;
                        }

                        if(!strcmp(*argv, "user3"))
                        {
                                if(*++argv == NULL) help();
				lk->user3 = atoi(*argv);
                                if(*++argv == NULL) goto define_ls;
                        }

                        if(!strcmp(*argv, "stop"))
                        {
				lk->mode = map_word(mode_types, "stop");
                                (void)*argv++;
				g->mode = map_word(mode_types, "sub_opts");
                                continue;
                        }

                        if(!strcmp(*argv, "start"))
                        {
				lk->mode = map_word(mode_types, "start");
                                (void)*argv++;
				g->mode = map_word(mode_types, "sub_opts");
                                continue;
                        }

                        if(!strcmp(*argv, "delete"))
                        {
				lk->mode = map_word(mode_types, "delete");
                                (void)*argv++;
				g->mode = map_word(mode_types, "sub_opts");
                                continue;
                        }

define_ls:
			lk->mode = map_word(mode_types, "define");
			g->mode = map_word(mode_types, "sub_opts");
                        continue;
                }

                /* LU Options */
                if(!strcmp(*argv, "lu"))
                {
                        if(*++argv == NULL) help();

                        if(!strcmp(*argv, "local"))
                        {
				struct llu *lu;

                                if(*++argv == NULL) help();
				new(lu);
				lu->next	= g->llus;
                                g->llus     	= lu;

				strcpy(lu->name, *argv);
                                if(*++argv == NULL) goto define_local_lu;

                                if(!strcmp(*argv, "-sync_point"))
                                {
					lu->syncpoint=map_word(on_types, "off");
                                        if(*++argv == NULL) goto define_local_lu;
                                }

                                if(!strcmp(*argv, "sync_point"))
                                {
					lu->syncpoint=map_word(on_types, "on");
                                        if(*++argv == NULL) goto define_local_lu;
                                }

                                if(!strcmp(*argv, "lu_sess_limit"))
                                {
                                        if(*++argv == NULL) help();
					lu->lu_sess_limit = atoi(*argv);
                                        if(*++argv == NULL) goto define_local_lu;
                                }

                                if(!strcmp(*argv, "delete"))
                                {
					lu->mode=map_word(mode_types, "delete");
					g->mode = map_word(mode_types, "sub_opts");
                                        if(*++argv == NULL) continue;
                                }

define_local_lu:
				lu->mode = map_word(mode_types, "define");
				g->mode = map_word(mode_types, "sub_opts");
                                continue;
                         }

                         if(!strcmp(*argv, "remote"))
                         {
				struct rlu *lu;

                                if(*++argv == NULL) help();
				new(lu);
				lu->next	= g->rlus;
                                g->rlus     	= lu;

				strcpy(lu->plu_name, *argv);
                                if(*++argv == NULL) help();
				strcpy(lu->fqcp_name, *argv);
                                if(*++argv == NULL) goto define_partner_lu;

                                if(!strcmp(*argv, "delete"))
                                {
					lu->mode=map_word(mode_types, "delete");
					g->mode = map_word(mode_types, "sub_opts");
                                        if(*++argv == NULL) continue;
                                }

define_partner_lu:
				lu->mode = map_word(mode_types, "define");
				g->mode = map_word(mode_types, "sub_opts");
                                continue;
                         }
                }

                /* Mode Options */
                if(!strcmp(*argv, "mode"))
                {
			struct mode *m;

                        if(*++argv == NULL) help();
                        new(m);
			m->next  = g->modes;
			g->modes = m;

			strcpy(m->name, *argv);

                        if(*++argv == NULL) help();
			strcpy(m->plu_name, *argv);
			if(*++argv == NULL) help();
                        strcpy(m->cos_name, *argv);
                        if(*++argv == NULL) goto define_mode;

                        if(!strcmp(*argv, "tx_pacing"))
                        {
                                if(*++argv == NULL) help();
                                printf("tx_pacing <%d>\n", atoi(*argv));
                                m->tx_pacing = atoi(*argv);
                                if(*++argv == NULL) goto define_mode;
                        }

                        if(!strcmp(*argv, "rx_pacing"))
                        {
                                if(*++argv == NULL) help();
                                printf("rx_pacing <%d>\n", atoi(*argv));
                                m->rx_pacing = atoi(*argv);
                                if(*++argv == NULL) goto define_mode;
                        }

                        if(!strcmp(*argv, "max_tx_ru"))
                        {
                                if(*++argv == NULL) help();
                                printf("max_tx_ru <%d>\n", atoi(*argv));
                                m->max_tx_ru = atoi(*argv);
                                if(*++argv == NULL) goto define_mode;
                        }

                        if(!strcmp(*argv, "max_rx_ru"))
                        {
                                if(*++argv == NULL) help();
                                printf("max_rx_ru <%d>\n", atoi(*argv));
                                m->max_rx_ru = atoi(*argv);
                                if(*++argv == NULL) goto define_mode;
                        }

                        if(!strcmp(*argv, "auto_activation"))
                        {
                                m->auto_activation = 1;
                                if(*++argv == NULL) goto define_mode;
                        }

                        if(!strcmp(*argv, "-auto_activation"))
                        {
                                m->auto_activation = 0;
                                if(*++argv == NULL) goto define_mode;
                        }

                        if(!strcmp(*argv, "min_conlosers"))
                        {
                                if(*++argv == NULL) help();
                                printf("min_conlosers <%d>\n", atoi(*argv));
                                m->min_conlosers = atoi(*argv);
                                if(*++argv == NULL) goto define_mode;
                        }

                        if(!strcmp(*argv, "min_conwinners"))
                        {
                                if(*++argv == NULL) help();
                                printf("min_conwinners <%d>\n", atoi(*argv));
                                m->min_conwinners = atoi(*argv);
                                if(*++argv == NULL) goto define_mode;
                        }

                        if(!strcmp(*argv, "max_sessions"))
                        {
                                if(*++argv == NULL) help();
                                printf("max_sessions <%d>\n", atoi(*argv));
                                m->max_sessions = atoi(*argv);
                                if(*++argv == NULL) goto define_mode;
                        }

                        if(!strcmp(*argv, "-encryption"))
                        {
                                printf("-encryption\n");
                                m->encryption = 0;
                                if(*++argv == NULL) goto define_mode;
                        }

                        if(!strcmp(*argv, "encryption"))
                        {
                                printf("encryption\n");
                                m->encryption = 0;
                                if(*++argv == NULL) goto define_mode;
                        }

                        if(!strcmp(*argv, "delete"))
                        {
				m->mode = map_word(mode_types, "delete");
				g->mode = map_word(mode_types, "sub_opts");
                                if(*++argv == NULL) continue;
                        }

define_mode:
			m->mode = map_word(mode_types, "define");
			g->mode = map_word(mode_types, "sub_opts");
                        continue;
                }

                /* Cost of service options */
                if(!strcmp(*argv, "cos"))
                {
			struct cos *dc;

                        if(*++argv == NULL) help();
                        new(dc);
			dc->next = g->coss;
			g->coss  = dc;

                        strcpy(dc->cos_name, *argv);
                        if(*++argv == NULL) goto define_cos;

                        if(!strcmp(*argv, "weight"))
                        {
                                if(*++argv == NULL) help();
                                dc->weight = atoi(*argv);
                                if(*++argv == NULL) goto define_cos;
                        }

                        if(!strcmp(*argv, "tx_priority"))
                        {
                                if(*++argv == NULL) help();
                                dc->tx_priority = atoi(*argv);
                                if(*++argv == NULL) goto define_cos;
                        }

                        if(!strcmp(*argv, "default_cos_invalid"))
                        {
                                dc->default_cos_invalid = 1;
                                if(*++argv == NULL) goto define_cos;
                        }

                        if(!strcmp(*argv, "-default_cos_invalid"))
                        {
                                dc->default_cos_invalid = 0;
                                if(*++argv == NULL) goto define_cos;
                        }

                        if(!strcmp(*argv, "default_cos_null"))
                        {
                                dc->default_cos_null = 1;
                                if(*++argv == NULL) goto define_cos;
                        }

                        if(!strcmp(*argv, "-default_cos_null"))
                        {
                                dc->default_cos_null = 0;
                                if(*++argv == NULL) goto define_cos;
                        }

                        if(!strcmp(*argv, "min_cost_per_connect"))
                        {
                                if(*++argv == NULL) help();
                                dc->min_cost_per_connect = atoi(*argv);
                                if(*++argv == NULL) goto define_cos;
                        }

                        if(!strcmp(*argv, "max_cost_per_connect"))
                        {
                                if(*++argv == NULL) help();
                                dc->max_cost_per_connect = atoi(*argv);
                                if(*++argv == NULL) goto define_cos;
                        }

                        if(!strcmp(*argv, "min_cost_per_byte"))
                        {
                                if(*++argv == NULL) help();
                                dc->min_cost_per_byte = atoi(*argv);
                                if(*++argv == NULL) goto define_cos;
                        }

                        if(!strcmp(*argv, "max_cost_per_byte"))
                        {
                                if(*++argv == NULL) help();
                                dc->max_cost_per_byte = atoi(*argv);
                                if(*++argv == NULL) goto define_cos;
                        }

                        if(!strcmp(*argv, "min_propagation_delay"))
                        {
                                if(*++argv == NULL) help();
                                dc->min_propagation_delay = atoi(*argv);
                                if(*++argv == NULL) goto define_cos;
                        }

                        if(!strcmp(*argv, "max_propagation_delay"))
                        {
                                if(*++argv == NULL) help();
                                dc->max_propagation_delay = atoi(*argv);
                                if(*++argv == NULL) goto define_cos;
                        }

                        if(!strcmp(*argv, "min_effective_capacity"))
                        {
                                if(*++argv == NULL) help();
                                dc->min_effective_capacity = atoi(*argv);
                                if(*++argv == NULL) goto define_cos;
                        }

                        if(!strcmp(*argv, "max_effective_capacity"))
                        {
                                if(*++argv == NULL) help();
                                dc->max_effective_capacity = atoi(*argv);
                                if(*++argv == NULL) goto define_cos;
                        }

                        if(!strcmp(*argv, "min_route_resistance"))
                        {
                                if(*++argv == NULL) help();
                                dc->min_route_resistance = atoi(*argv);
                                if(*++argv == NULL) goto define_cos;
                        }

                        if(!strcmp(*argv, "max_route_resistance"))
                        {
                                if(*++argv == NULL) help();
                                dc->max_route_resistance = atoi(*argv);
                                if(*++argv == NULL) goto define_cos;
                        }

                        if(!strcmp(*argv, "min_node_congested"))
                        {
                                if(*++argv == NULL) help();
                                dc->min_node_congested = atoi(*argv);
                                if(*++argv == NULL) goto define_cos;
                        }

                        if(!strcmp(*argv, "max_node_congested"))
                        {
                                if(*++argv == NULL) help();
                                dc->max_node_congested = atoi(*argv);
                                if(*++argv == NULL) goto define_cos;
                        }

                        if(!strcmp(*argv, "min_security"))
                        {
                                if(*++argv == NULL) help();
                                dc->min_security = atoi(*argv);
                                if(*++argv == NULL) goto define_cos;
                        }

                        if(!strcmp(*argv, "max_security"))
                        {
                                if(*++argv == NULL) help();
                                dc->max_security = atoi(*argv);
                                if(*++argv == NULL) goto define_cos;
                        }

                        if(!strcmp(*argv, "min_user1"))
                        {
                                if(*++argv == NULL) help();
                                dc->min_user1 = atoi(*argv);
                                if(*++argv == NULL) goto define_cos;
                        }

                        if(!strcmp(*argv, "max_user1"))
                        {
                                if(*++argv == NULL) help();
                                dc->max_user1 = atoi(*argv);
                                if(*++argv == NULL) goto define_cos;
                        }

                        if(!strcmp(*argv, "min_user2"))
                        {
                                if(*++argv == NULL) help();
                                dc->min_user2 = atoi(*argv);
                                if(*++argv == NULL) goto define_cos;
                        }

                        if(!strcmp(*argv, "max_user2"))
                        {
                                if(*++argv == NULL) help();
                                dc->max_user2 = atoi(*argv);
                                if(*++argv == NULL) goto define_cos;
                        }

                        if(!strcmp(*argv, "min_user3"))
                        {
                                if(*++argv == NULL) help();
                                dc->min_user3 = atoi(*argv);
                                if(*++argv == NULL) goto define_cos;
                        }

                        if(!strcmp(*argv, "max_user3"))
                        {
                                if(*++argv == NULL) help();
                                dc->max_user3 = atoi(*argv);
                                if(*++argv == NULL) goto define_cos;
                        }

			if(!strcmp(*argv, "delete"))
                        {
                                dc->mode = map_word(mode_types, "delete");
                                g->mode = map_word(mode_types, "sub_opts");
                                if(*++argv == NULL) continue;
                        }

define_cos:
			dc->mode = map_word(mode_types, "define");
			g->mode = map_word(mode_types, "sub_opts");
                        continue;
                }

                /* CPI-C Side Information Options */
                if(!strcmp(*argv, "cpic"))
                {
			struct cpic_side *side;

                        if(*++argv == NULL) help();
                        new(side);
			side->next = g->cpics;
			g->cpics   = side;

                        strcpy(side->sym_dest_name, *argv);
                        if(*++argv == NULL) help();
                        strcpy(side->mode_name, *argv);
                        if(*++argv == NULL) help();
			strcpy(side->plu_name, *argv);
                        if(*++argv == NULL) help();
                        strcpy(side->tp_name, *argv);

                        if(*++argv == NULL) goto define_cpic_side;

                        if(!strcmp(*argv, "delete"))
                        {
				side->mode = map_word(mode_types, "delete");
				g->mode = map_word(mode_types, "sub_opts");
                                if(*++argv == NULL) continue;
                        }

define_cpic_side:
			side->mode = map_word(mode_types, "define");
			g->mode = map_word(mode_types, "sub_opts");
                        continue;
                }

                if(!strcmp(*argv, "start"))
                {
			g->mode = map_word(mode_types, "start");
                        (void)*argv++;
                        continue;
                }

                if(!strcmp(*argv, "stop"))
                {
			g->mode = map_word(mode_types, "stop");
                        (void)*argv++;
                        continue;
                }

                if(!strcmp(*argv, "delete"))
                {
			g->mode = map_word(mode_types, "delete");
                        (void)*argv++;
                        continue;
                }

                /* Catch all for any commands that we don't do. */
                (void)*argv++;
        }

	/* actually load the configuration into the kernel */
	sna_load_config(g);

	return (0);
}
