/* load.c: functions to handle the loading, independent of parsers
 *  - All input is already verified and clean.
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

int sna_load_config(global *ginfo)
{
        struct dlc *d;
        struct link *l;
        struct llu *lu_local;
        struct rlu *lu_remote;
        struct mode *m;
        struct cpic_side *cp;
        struct cos *cos;
        struct sna_netid *b, *c;
	struct sna_nodeid *n;
        FILE *dblvl;
        int err;

	if(sna_debug > 5)
		sna_print_global(ginfo);

        if(ginfo == NULL)
                return (-1);

	b = sna_char_to_netid(ginfo->fq_node_name);

	/* define or start node */
	if(ginfo->mode == 0 || ginfo->mode == 1)
	{
		struct sna_start_node *snode;
        	struct sna_define_node_chars *dnode;

		dblvl = fopen(DEBUG_LEVEL, "r+");
        	if(dblvl == NULL)
                	return (-1);
        	fprintf(dblvl, "%d", ginfo->debug_level);
        	fclose(dblvl);

	        new(snode);
	        memcpy(&snode->netid, b, sizeof(struct sna_netid));

	        if(!strcmp("nn", ginfo->node_type))
	                snode->type = SNA_APPN_NET_NODE;
	        else
	        {
	                if(!strcmp("appn", ginfo->node_type))
	                        snode->type = SNA_APPN_END_NODE;
	                else
	                {
	                        if(!strcmp("len", ginfo->node_type))
	                                snode->type = SNA_LEN_END_NODE;
	                        else
	                                return (-1);
        	        }
        	}

		n = sna_char_to_nodeid(ginfo->node_id);
		memcpy(&snode->nodeid, n, sizeof(struct sna_nodeid));

	        snode->max_lus  = 0;
	        snode->lu_seg   = 0;
	        snode->bind_seg = 0;

	        new(dnode);
	        memcpy(&dnode->cp_name, b, sizeof(struct sna_netid));
	        err = nof_define_node_chars(sna_sk, dnode);
	        if(err < 0)
	        {
	                perror("nof_define_node_chars");
	                return (err);
	        }
		free(dnode);

	        err = nof_start_node(sna_sk, snode);
	        if(err < 0)
	        {
	                perror("nof_start_node");
	                return (err);
	        }
		free(snode);
	}

	/* stop node */
	if(ginfo->mode == 2)
	{
		struct sna_stop_node *snode;

		new(snode);
		memcpy(&snode->netid, b, sizeof(struct sna_netid));
		err = nof_stop_node(sna_sk, snode);
                if(err < 0)
                {
                	perror("nof_stop_node");
                        exit (1);
                }
		free(snode);
	}

	/* delete mode */
	if(ginfo->mode == 3)
	{
		struct sna_delete_node *dnode;

		new(dnode);
		memcpy(&dnode->netid, b, sizeof(struct sna_netid));
                err = nof_delete_node(sna_sk, dnode);
                if(err < 0)
                {
                	perror("nof_delete_node");
                        exit (1);
                }
		free(dnode);
	}

        for(d = ginfo->dlcs; d != NULL; d = d->next)
        {
		char    name[SNA_RESOURCE_NAME_LEN];
        	char    saddr[SNA_PORT_ADDR_LEN];
		unsigned long port;
		int err;

                strcpy(name, d->interface);
                port = strtoul(d->port, NULL, 0);
                memcpy(saddr, &port, sizeof(saddr));

		if(d->mode == 0)	/* define */
		{
			struct sna_define_port *dport;

			new(dport);
			memcpy(&dport->netid, b, sizeof(struct sna_netid));
			memcpy(&dport->name, name, sizeof(dport->name));
			memcpy(&dport->saddr, saddr, sizeof(dport->saddr));

			/* Get port type. */
                	if(!strcmp(d->role, "pri"))
                        	dport->role = SNA_PORT_ROLE_PRI;
                	else
                	{
                        	if(!strcmp(d->role, "sec"))
                                	dport->role = SNA_PORT_ROLE_SEC;
                        	else
                        	{
                                	if(!strcmp(d->role, "neg"))
                                        	dport->role = SNA_PORT_ROLE_NEG;
                        	}
                	}

                	dport->btu = d->btu;
                	dport->mia = d->mia;
                	dport->moa = d->moa;

                	err = nof_define_port(sna_sk, dport);
                	if(err < 0)
                	{
                        	perror("nof_define_port");
                        	return (err);
                	}

			free(dport);
		}

		if(d->mode == 1)	/* start */
		{
			struct sna_start_port *sport;

			new(sport);
			memcpy(&sport->netid, b, sizeof(struct sna_netid));
			memcpy(&sport->name, name, sizeof(sport->name));
                        memcpy(&sport->saddr, saddr, sizeof(sport->saddr));

			err = nof_start_port(sna_sk, sport);
                        if(err < 0)
                        {
                                perror("nof_start_port");
                                return (err);
                        }

			free(sport);
		}

		if(d->mode == 2)	/* stop */
		{
			struct sna_stop_port *sport;

			new(sport);
			memcpy(&sport->netid, b, sizeof(struct sna_netid));
			memcpy(&sport->name, name, sizeof(sport->name));
                        memcpy(&sport->saddr, saddr, sizeof(sport->saddr));

			err = nof_stop_port(sna_sk, sport);
                        if(err < 0)
                        {
                                perror("nof_stop_port");
                                return (err);
                        }

			free(sport);
		}

		if(d->mode == 3)	/* delete */
		{
			struct sna_delete_port *dport;

			new(dport);
			memcpy(&dport->netid, b, sizeof(struct sna_netid));
			memcpy(&dport->name, name, sizeof(dport->name));
                        memcpy(&dport->saddr, saddr, sizeof(dport->saddr));

			err = nof_delete_port(sna_sk, dport);
                        if(err < 0)
                        {
                                perror("nof_delete_port");
                                return (err);
                        }

			free(dport);
		}
        }

        for(lu_local = ginfo->llus; lu_local != NULL; lu_local = lu_local->next)        {
		unsigned char lu_name[SNA_RESOURCE_NAME_LEN];
		int err, i;

		strcpy(lu_name, lu_local->name);
                for(i = 0; i < SNA_RESOURCE_NAME_LEN; i++)
                	lu_name[i] = toupper(lu_name[i]);

		if(lu_local->mode == 0)		/* define */
		{
			struct sna_define_local_lu *dllu;

			new(dllu);
                	memcpy(&dllu->netid, b, sizeof(struct sna_netid));
			memcpy(&dllu->lu_name, lu_name, sizeof(dllu->lu_name));
                	dllu->sync_point	= lu_local->syncpoint;
                	dllu->lu_sess_limit 	= lu_local->lu_sess_limit;

                	err = nof_define_local_lu(sna_sk, dllu);
                	if(err < 0)
                	{
                        	perror("nof_define_local_lu");
                        	return (err);
                	}
	
			free(dllu);
		}

		if(lu_local->mode == 3)		/* delete */
		{
			struct sna_delete_local_lu *dllu;

			new(dllu);
			memcpy(&dllu->netid, b, sizeof(struct sna_netid));
                        memcpy(&dllu->lu_name, lu_name, sizeof(dllu->lu_name));

			err = nof_delete_local_lu(sna_sk, dllu);
                        if(err < 0)
                        {
                                perror("nof_delete_local_lu");
                                return (err);
                        }

			free(dllu);
		}
        }

        for(lu_remote = ginfo->rlus; lu_remote != NULL;
                lu_remote = lu_remote->next)
        {
		int err;

		c = sna_char_to_netid(lu_remote->plu_name);
		if(lu_remote->mode == 0)	/* define */
		{
			struct sna_define_partner_lu *dplu;

			new(dplu);

			memcpy(&dplu->netid, b, sizeof(struct sna_netid));
                	memcpy(&dplu->netid_plu, c, sizeof(struct sna_netid));
                	memcpy(&dplu->netid_fqcp, c, sizeof(struct sna_netid));

                	err = nof_define_partner_lu(sna_sk, dplu);
                	if(err < 0)
                	{
                	        perror("nof_define_partner_lu");
                	        return (err);
                	}

			free(dplu);
		}

		if(lu_remote->mode == 3)	/* delete */
		{
			struct sna_delete_partner_lu *dplu;

			new(dplu);
			memcpy(&dplu->netid, b, sizeof(struct sna_netid));
                        memcpy(&dplu->netid_plu, c, sizeof(struct sna_netid));
			err = nof_delete_partner_lu(sna_sk, dplu);
                        if(err < 0)
                        {
                                perror("nof_delete_partner_lu");
                                return (err);
                        }

			free(dplu);
		}
	}

        for(l = ginfo->links; l != NULL; l = l->next)
        {
		struct sna_netid netid_plu;
        	char name[SNA_RESOURCE_NAME_LEN];
        	char saddr[SNA_PORT_ADDR_LEN];
        	char dname[SNA_FQCP_NAME_LEN];
        	char daddr[SNA_PORT_ADDR_LEN];
		unsigned long port;
                int err;

		/* load up the info into local structs */
		strcpy(name, l->interface);
		in_ether(l->dstaddr, dname);
		port = strtoul(l->port, NULL, 0);
		memcpy(saddr, &port, sizeof(unsigned long));
		port = strtoul(l->dstport, NULL, 0);
		memcpy(daddr, &port, sizeof(unsigned long));
		memcpy(&netid_plu, sna_char_to_netid(l->plu_name),
	                sizeof(struct sna_netid));

		if(l->mode == 0)	/* define */
		{
			struct sna_define_link_station *dlink;

			new(dlink);
			memcpy(&dlink->netid, b, sizeof(struct sna_netid));
                	strcpy(dlink->name, name);
                	memcpy(dlink->saddr, saddr, sizeof(dlink->saddr));
                	memcpy(&dlink->plu_name, &netid_plu,
                        	sizeof(struct sna_netid));
			memcpy(&dlink->dname, dname, sizeof(dlink->dname));
                	memcpy(dlink->daddr, daddr, sizeof(dlink->daddr));

                	dlink->byteswap                 = l->byteswap;
                	dlink->retry_on_fail            = l->retry_on_fail;
                	dlink->retry_times              = l->retry_times;
                	dlink->autoact                  = l->autoact;
			dlink->autodeact                = l->autodeact;
                	dlink->tg_number                = l->tg_number;
                	dlink->cost_per_byte            = l->cost_per_byte;
                	dlink->cost_per_connect_time    = l->cost_per_connect_time;
                	dlink->effective_capacity       = l->effective_capacity;
                	dlink->propagation_delay        = l->propagation_delay;
                	dlink->security                 = l->security;
                	dlink->user1                    = l->user1;
                	dlink->user2                    = l->user2;
                	dlink->user3                    = l->user3;

	                err = nof_define_link_station(sna_sk, dlink);
	                if(err < 0)
	                {
	                        perror("nof_define_link_station");
	                        return (err);
	                }
			free(dlink);
		}

		if(l->mode == 1)	/* start */
		{
                	struct sna_start_link_station *slink;

			new(slink);
			memcpy(&slink->netid, b, sizeof(struct sna_netid));
                        strcpy(slink->name, name);
                        memcpy(slink->saddr, saddr, sizeof(slink->saddr));
                        memcpy(&slink->netid_plu, &netid_plu,
                                sizeof(struct sna_netid));
                        memcpy(&slink->dname, dname, sizeof(slink->dname));
                        memcpy(slink->daddr, daddr, sizeof(slink->daddr));

                        err = nof_start_link_station(sna_sk, slink);
                        if(err < 0)
                        {
                                perror("nof_start_link_station");
                                return (err);
                        }
			free(slink);
		}

		if(l->mode == 2)	/* stop */
		{
			struct sna_stop_link_station *slink;

			new(slink);
			memcpy(&slink->netid, b, sizeof(struct sna_netid));
                        strcpy(slink->name, name);
                        memcpy(slink->saddr, saddr, sizeof(slink->saddr));
                        memcpy(&slink->netid_plu, &netid_plu,
                                sizeof(struct sna_netid));
                        memcpy(&slink->dname, dname, sizeof(slink->dname));
                        memcpy(slink->daddr, daddr, sizeof(slink->daddr));
                        err = nof_stop_link_station(sna_sk, slink);
                        if(err < 0)
                        {
                                perror("nof_stop_link_station");
                                return (err);
                        }
			free(slink);
		}

		if(l->mode == 3)	/* delete */
		{
			struct sna_delete_link_station *dlink;

			new(dlink);
			memcpy(&dlink->netid, b, sizeof(struct sna_netid));
                        strcpy(dlink->name, name);
                        memcpy(dlink->saddr, saddr, sizeof(dlink->saddr));
                        memcpy(&dlink->dname, dname, sizeof(dlink->dname));
                        memcpy(dlink->daddr, daddr, sizeof(dlink->daddr));

                        err = nof_delete_link_station(sna_sk, dlink);
                        if(err < 0)
                        {
                                perror("nof_delete_link_station");
                                return (err);
                        }
			free(dlink);
		}
        }

        for(m = ginfo->modes; m != NULL; m = m->next)
        {
		int err, i;
		unsigned char mode_name[SNA_RESOURCE_NAME_LEN];

                strncpy(mode_name, m->name, SNA_RESOURCE_NAME_LEN);
                for(i = 0; i < SNA_RESOURCE_NAME_LEN; i++)
                        mode_name[i] = toupper(mode_name[i]);
		c = sna_char_to_netid(m->plu_name);

		if(m->mode == 0)	/* define */
		{
			struct sna_define_mode *dmode;

			new(dmode);
			memcpy(&dmode->netid, b, sizeof(struct sna_netid));
			memcpy(&dmode->netid_plu, c, sizeof(struct sna_netid));
			memcpy(&dmode->mode_name, mode_name, 
				sizeof(dmode->mode_name));

                	strncpy(dmode->cos_name, m->cos_name, 
				SNA_RESOURCE_NAME_LEN);
                	for(i = 0; i < SNA_RESOURCE_NAME_LEN; i++)
                        	dmode->cos_name[i]=toupper(dmode->cos_name[i]);

                	dmode->crypto           = m->encryption;
                	dmode->tx_pacing        = m->tx_pacing;
                	dmode->rx_pacing        = m->rx_pacing;
                	dmode->max_tx_ru        = m->max_tx_ru;
                	dmode->max_rx_ru        = m->max_rx_ru;
                	dmode->min_conwinners   = m->min_conwinners;
                	dmode->min_conlosers    = m->min_conlosers;
                	dmode->max_sessions     = m->max_sessions;
                	dmode->auto_activation  = m->auto_activation;

			err = nof_define_mode(sna_sk, dmode);
                	if(err < 0)
                	{
                        	perror("nof_define_mode");
                        	return (err);
                	}

			free(dmode);
		}

		if(m->mode == 3)	/* delete */
		{
			struct sna_delete_mode *dmode;

			new(dmode);
			memcpy(&dmode->netid, b, sizeof(struct sna_netid));
                        memcpy(&dmode->netid_plu, c, sizeof(struct sna_netid));
                        memcpy(&dmode->mode_name, mode_name,
                                sizeof(dmode->mode_name));

			err = nof_delete_mode(sna_sk, dmode);
                	if(err < 0)
                	{
                        	perror("nof_delete_mode");
                        	return (err);
                	}

			free(dmode);
		}
	}

        for(cos = ginfo->coss; cos != NULL; cos = cos->next)
        {
		unsigned char name[SNA_RESOURCE_NAME_LEN];
		int err, i;

		strncpy(name, cos->cos_name, SNA_RESOURCE_NAME_LEN);
                for(i = 0; i < SNA_RESOURCE_NAME_LEN; i++)
                        name[i] = toupper(name[i]);

		if(cos->mode == 0)	/* define */
		{
			struct sna_define_cos *dc;

			new(dc);
			memcpy(&dc->name, name, sizeof(dc->name));
			dc->weight                      = cos->weight;
                	dc->tx_priority                 = cos->tx_priority;
                	dc->default_cos_invalid		= cos->default_cos_invalid;
                	dc->default_cos_null            = cos->default_cos_null;
                	dc->min_cost_per_connect        = cos->min_cost_per_connect;
                	dc->max_cost_per_connect        = cos->max_cost_per_connect;
                	dc->min_cost_per_byte           = cos->min_cost_per_byte;
                	dc->max_cost_per_byte           = cos->max_cost_per_byte;
                	dc->min_security                = cos->min_security;
                	dc->max_security                = cos->max_security;
                	dc->min_propagation_delay       = cos->min_propagation_delay;
                	dc->max_propagation_delay       = cos->max_propagation_delay;
                	dc->min_effective_capacity      = cos->min_effective_capacity;
                	dc->max_effective_capacity      = cos->max_effective_capacity;
                	dc->min_user1                   = cos->min_user1;
                	dc->max_user1                   = cos->max_user1;
                	dc->min_user2                   = cos->min_user2;
                	dc->max_user2                   = cos->max_user2;
               		dc->min_user3                   = cos->min_user3;
                	dc->max_user3                   = cos->max_user3;
                	dc->min_route_resistance        = cos->min_route_resistance;
                	dc->max_route_resistance        = cos->max_route_resistance;
                	dc->min_node_congested          = cos->min_node_congested;
                	dc->max_node_congested          = cos->max_node_congested;
			err = nof_define_class_of_service(sna_sk, dc);
                	if(err < 0)
                	{
                        	perror("nof_define_class_of_service");
                        	return (err);
                	}

			free(dc);
		}

		if(cos->mode == 3)	/* delete */
		{
			struct sna_delete_cos *dc;

			new(dc);
			memcpy(&dc->name, name, sizeof(dc->name));
			err = nof_delete_class_of_service(sna_sk, dc);
                	if(err < 0)
                	{
                        	perror("nof_delete_cos");
                        	return (err);
                	}
			free(dc);
		}
        }

        for(cp = ginfo->cpics; cp != NULL; cp = cp->next)
        {
		int err, i;
		unsigned char netid[SNA_FQCP_NAME_LEN];
        	unsigned char netid_plu[SNA_FQCP_NAME_LEN];
        	unsigned char sym_dest_name[SNA_RESOURCE_NAME_LEN];
		unsigned char mode_name[SNA_RESOURCE_NAME_LEN];

		sna_netid_to_char(b, netid);
		sna_netid_to_char(sna_char_to_netid(cp->plu_name), netid_plu);
		strcpy(sym_dest_name, cp->sym_dest_name);
                for(i = 0; i < SNA_RESOURCE_NAME_LEN; i++)
                        sym_dest_name[i] = toupper(sym_dest_name[i]);
		strcpy(mode_name, cp->mode_name);
                for(i = 0; i < SNA_RESOURCE_NAME_LEN; i++)
                	mode_name[i] = toupper(mode_name[i]);

		if(cp->mode == 0)	/* define */
		{
			struct cpic_define_side_info *dcpic;

			new(dcpic);
			memcpy(&dcpic->netid, netid, sizeof(dcpic->netid));
			memcpy(&dcpic->netid_plu, netid_plu, 
				sizeof(dcpic->netid_plu));
			strcpy(dcpic->sym_dest_name, sym_dest_name);
			strcpy(dcpic->mode_name, mode_name);
	                strcpy(dcpic->tp_name, cp->tp_name);

			err = nof_define_cpic_side_info(sna_sk, dcpic);
                	if(err < 0)
                	{
                        	perror("nof_define_cpic_side_info");
                        	return (err);
                	}
			free(dcpic);
		}

		if(cp->mode == 3)	/* delete */
		{
			struct cpic_delete_side_info *dcpic;

			new(dcpic);
			memcpy(&dcpic->netid, netid, sizeof(dcpic->netid));
                        memcpy(&dcpic->netid_plu, netid_plu,
                                sizeof(dcpic->netid_plu));
			memcpy(&dcpic->sym_dest_name, sym_dest_name,
                                sizeof(dcpic->sym_dest_name));

			err = nof_delete_cpic_side_info(sna_sk, dcpic);
                	if(err < 0)
                	{
                        	perror("nof_delete_cpic_side_info");
                        	return (err);
                	}
			free(dcpic);
		}
        }

        return (0);
}
