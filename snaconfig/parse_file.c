/* parse_file.c: load an sna server configuration from a file.
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

extern FILE *OpenConfFile(char *FileName);
extern int Parse(FILE *InFile, int (*sfunc)(char *),
        int (*pfunc)(char *, char *));

static int iTopic = 0;

int in_client;

static global *ginfo = NULL;
char dlc_name[SNA_RESOURCE_NAME_LEN];
char link_name[SNA_RESOURCE_NAME_LEN];
char llu_name[SNA_RESOURCE_NAME_LEN];
char rlu_name[SNA_FQCP_NAME_LEN];
char mode_name[SNA_RESOURCE_NAME_LEN];
char cpic_name[SNA_RESOURCE_NAME_LEN];
char cos_name[SNA_RESOURCE_NAME_LEN];

struct dlc *find_dlc(char *iface)
{
        struct dlc *dl;

        for(dl = ginfo->dlcs; dl != NULL; dl = dl->next)
                if(!strcmp(dl->interface, iface))
                        return(dl);
        return (NULL);
}

struct link *find_link(char *iface)
{
        struct link *lk;

        for(lk = ginfo->links; lk != NULL; lk = lk->next)
                if(!strcmp(lk->interface, iface))
                        return (lk);

        return (NULL);
}

struct llu *find_llu(char *name)
{
        struct llu *lu;

        for(lu = ginfo->llus; lu != NULL; lu = lu->next)
                if(!strcmp(lu->name, name))
                        return (lu);

        return (NULL);
}

struct rlu *find_rlu(char *name)
{
        struct rlu *lu;

        for(lu = ginfo->rlus; lu != NULL; lu = lu->next)
                if(!strcmp(lu->plu_name, name))
                        return (lu);

        return (NULL);
}

struct mode *find_mode(char *name)
{
        struct mode *m;

        for(m = ginfo->modes; m != NULL; m = m->next)
                if(!strcmp(m->name, name))
                        return (m);

        return (NULL);
}

struct cos *find_cos(char *name)
{
        struct cos *c;

        for(c = ginfo->coss; c != NULL; c = c->next)
                if(!strcmp(c->cos_name, name))
                        return (c);

        return (NULL);
}

struct cpic_side *find_cpic(char *name)
{
        struct cpic_side *c;

        for(c = ginfo->cpics; c != NULL; c = c->next)
                if(!strcmp(c->sym_dest_name, name))
                        return (c);

        return (NULL);
}

/* Check the section and make sure it is valid.
 * 0 = no valid
 * 1 = valid
 */
int sfunk(char *sname)
{
        if(!strcmp("global", sname))
        {       iTopic = GLOBAL;
                return (1);
        }
        if(!strcmp("dlc", sname))
        {
                iTopic = DLC;
                return (1);
        }
        if(!strcmp("link", sname))
        {
                iTopic = LINK;
                return (1);
        }
        if(!strcmp("lu_local", sname))
        {
                iTopic = LU_LOCAL;
                return (1);
        }
        if(!strcmp("lu_remote", sname))
        {
                iTopic = LU_REMOTE;
                return (1);
        }
        if(!strcmp("cos", sname))
        {
                iTopic = COS;
                return (1);
        }
        if(!strcmp("mode", sname))
        {
                iTopic = MODE;
                return (1);
        }
        if(!strcmp("cpic", sname))
        {
                iTopic = CPIC;
                return (1);
        }

        return (0);
}

/* Process parameters for each section. */
int pfunk(char *a, char *b)
{
        int err = 0;

        switch(iTopic)
        {
                case (GLOBAL):
                        if(ginfo == NULL)
                                new(ginfo);
                        if(!strcmp("fq_node_name", a))
                        {
                                strncpy(ginfo->fq_node_name, b, 17);
                                err = 1;
                                break;
                        }

                        if(!strcmp("node_type", a))
                        {
                                strncpy(ginfo->node_type, b, 8);
                                err = 1;
                                break;
                        }

                        if(!strcmp("node_id", a))
                        {
				strncpy(ginfo->node_id, b, 8);
                                err = 1;
                                break;
                        }

                        if(!strcmp("debug_level", a))
                        {
                                ginfo->debug_level = atoi(b);
                                err = 1;
                                break;
                        }

                        break;

                case (DLC):
                {
                        struct dlc *dl;
                        if(!strcmp("interface", a))
                        {
                                if(strcmp(dlc_name, b))
                                        strcpy(dlc_name, b);
                        }

                        dl = find_dlc(dlc_name);
                        if(dl == NULL)
                        {
                                new(dl);
                                strcpy(dl->interface, b);
                                dl->next        = ginfo->dlcs;
                                ginfo->dlcs     = dl;
                                err = 1;
                        }

                        if(!strcmp("port", a))
                        {
                                strncpy(dl->port, b, 8);
                                err = 1;
                                break;
                        }

                        if(!strcmp("role", a))
                        {
                                strncpy(dl->role, b, 8);
                                err = 1;
                                break;
                        }

                        if(!strcmp("btu", a))
                        {
                                dl->btu = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("mia", a))
                        {
                                dl->mia = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("moa", a))
                        {
                                dl->moa = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("mode", a))
                        {
                                dl->mode = map_word(mode_types, b);
                                err = 1;
                                break;
                        }

                        break;
                }

                case (LINK):
                {
                        struct link *lk;
                        if(!strcmp("interface", a))
                                if(strcmp(link_name, b))
                                        strcpy(link_name, b);

                        lk = find_link(link_name);
                        if(lk == NULL)
                        {
                                new(lk);
                                strcpy(lk->interface, b);
                                lk->next        = ginfo->links;
                                ginfo->links    = lk;
                                err = 1;
                        }

                        if(!strcmp("port", a))
                        {
                                strncpy(lk->port, b, 8);
                                err = 1;
                                break;
                        }

                        if(!strcmp("byteswap", a))
                        {
                                lk->byteswap = map_word(on_types, b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("plu_name", a))
                        {
                                strcpy(lk->plu_name, b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("dstaddr", a))
                        {
                                strncpy(lk->dstaddr, b, 16);
                                err = 1;
                                break;
                        }

                        if(!strcmp("dstport", a))
                        {
                                strncpy(lk->dstport, b, 8);
                                err = 1;
                                break;
                        }

                        if(!strcmp("autoact", a))
                        {
                                lk->autoact = map_word(on_types, b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("autodeact", a))
                        {
                                lk->autodeact = map_word(on_types, b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("retry_on_fail", a))
                        {
                                lk->retry_on_fail = map_word(on_types, b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("retry_times", a))
                        {
                                lk->retry_times = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("tg_number", a))
                        {
                                lk->tg_number = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("cost_per_byte", a))
                        {
                                lk->cost_per_byte = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("cost_per_connect_time", a))
                        {
                                lk->cost_per_connect_time = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("effective_capacity", a))
                        {
                                lk->effective_capacity = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("propagation_delay", a))
                        {
                                lk->propagation_delay = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("security", a))
                        {
                                lk->security = map_word(security_types, b);;
                                err = 1;
                                break;
                        }

                        if(!strcmp("user1", a))
                        {
                                lk->user1 = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("user2", a))
                        {
                                lk->user2 = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("user3", a))
                        {
                                lk->user3 = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("mode", a))
                        {
                                lk->mode = map_word(mode_types, b);
                                err = 1;
                                break;
                        }

                        break;
                }

                case (LU_LOCAL):
                {
                        struct llu *lu;
                        if(!strcmp("name", a))
                                if(strcmp(llu_name, b))
                                        strcpy(llu_name, b);

                        lu = find_llu(llu_name);
                        if(lu == NULL)
                        {
                                new(lu);
                                strcpy(lu->name, b);
                                lu->next        = ginfo->llus;
                                ginfo->llus     = lu;
                                err = 1;
                        }

                        if(!strcmp("syncpoint", a))
                        {
                                lu->syncpoint = map_word(on_types, b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("lu_sess_limit", a))
                        {
                                lu->lu_sess_limit = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("mode", a))
                        {
                                lu->mode = map_word(mode_types, b);
                                err = 1;
                                break;
                        }

                        break;
                }

                case (LU_REMOTE):
                {
                        struct rlu *lu;
                        if(!strcmp("plu_name", a))
                                if(strcmp(rlu_name, b))
                                        strcpy(rlu_name, b);

                        lu = find_rlu(rlu_name);
                        if(lu == NULL)
                        {
                                new(lu);
                                strncpy(lu->plu_name, b, 17);
                                lu->next        = ginfo->rlus;
                                ginfo->rlus     = lu;
                                err = 1;
                        }

                        if(!strcmp("fqcp_name", a))
                        {
                                strncpy(lu->fqcp_name, b, 17);
                                err = 1;
                                break;
                        }

                        if(!strcmp("mode", a))
                        {
                                lu->mode = map_word(mode_types, b);
                                err = 1;
                                break;
                        }

                        break;
                }

                case (MODE):
                {
                        struct mode *m;
                        if(!strcmp("name", a))
                        {
                                if(strcmp(mode_name, b))
                                        strcpy(mode_name, b);
                        }

                        m = find_mode(mode_name);
                        if(m == NULL)
                        {
                                new(m);
                                strncpy(m->name, b, 8);
                                m->next        = ginfo->modes;
                                ginfo->modes     = m;
                                err = 1;
                        }

                        if(!strcmp("plu_name", a))
                        {
                                strncpy(m->plu_name, b, 17);
                                err = 1;
                                break;
                        }

                        if(!strcmp("cos_name", a))
                        {
                                strncpy(m->cos_name, b, 9);
                                err = 1;
                                break;
                        }

                        if(!strcmp("encryption", a))
                        {
                                m->encryption = map_word(on_types, b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("min_conwinners", a))
                        {
                                m->min_conwinners = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("min_conlosers", a))
                        {
                                m->min_conlosers = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("max_sessions", a))
                        {
                                m->max_sessions = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("auto_activation", a))
                        {
                                m->auto_activation = map_word(on_types, b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("tx_pacing", a))
                        {
                                m->tx_pacing = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("rx_pacing", a))
                        {
                                m->rx_pacing = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("max_tx_ru", a))
                        {
                                m->max_tx_ru = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("max_rx_ru", a))
                        {
                                m->max_rx_ru = atoi(b);
                                err = 1;
                                break;
                        }

/*
                        if(!strcmp("mode", a))
                        {
                                m->mode = map_word(mode_types, b);
                                err = 1;
                                break;
                        }
*/

                        break;
                }

                case (COS):
                {
                        struct cos *c;
                        if(!strcmp("name", a))
                                if(strcmp(cos_name, b))
                                        strcpy(cos_name, b);

                        c = find_cos(cos_name);
                        if(c == NULL)
                        {
                                new(c);
                                strcpy(c->cos_name, b);
                                c->next        = ginfo->coss;
                                ginfo->coss     = c;
                                err = 1;

                                break;
                        }

                        if(!strcmp("weight", a))
                        {
                                c->weight = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("tx_priority", a))
                        {
                                c->tx_priority = map_word(tx_types, b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("default_cos_invalid", a))
                        {
                                c->default_cos_invalid = map_word(on_types, b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("default_cos_null", a))
                        {
                                c->default_cos_null = map_word(on_types, b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("min_cost_per_connect", a))
                        {
                                c->min_cost_per_connect = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("max_cost_per_connect", a))
                        {
                                c->max_cost_per_connect = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("min_cost_per_byte", a))
                        {
                                c->min_cost_per_byte = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("max_cost_per_byte", a))
                        {
                                c->max_cost_per_byte = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("min_security", a))
                        {
                                c->min_security = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("max_security", a))
                        {
                                c->max_security = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("min_propagation_delay", a))
                        {
                                c->min_propagation_delay = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("max_propagation_delay", a))
                        {
                                c->max_propagation_delay = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("min_effective_capacity", a))
                        {
                                c->min_effective_capacity = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("max_effective_capacity", a))
                        {
                                c->max_effective_capacity = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("min_user1", a))
                        {
                                c->min_user1 = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("max_user1", a))
                        {
                                c->max_user1 = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("min_user2", a))
                        {
                                c->min_user2 = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("max_user2", a))
                        {
                                c->max_user2 = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("min_user3", a))
                        {
                                c->min_user3 = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("max_user3", a))
                        {
                                c->max_user3 = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("min_route_resistance", a))
                        {
                                c->min_route_resistance = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("max_route_resistance", a))
                        {
                                c->max_route_resistance = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("min_node_congested", a))
                        {
                                c->min_node_congested = atoi(b);
                                err = 1;
                                break;
                        }

                        if(!strcmp("max_node_congested", a))
                        {
                                c->max_node_congested = atoi(b);
                                err = 1;
                                break;
                        }

                        break;
                }

                case (CPIC):
                {
                        struct cpic_side *c;
                        if(!strcmp("sym_dest_name", a))
                                if(strcmp(cpic_name, b))
                                        strcpy(cpic_name, b);

                        c = find_cpic(cpic_name);
                        if(c == NULL)
                        {
                                new(c);
                                strcpy(c->sym_dest_name, b);
                                c->next        = ginfo->cpics;
                                ginfo->cpics     = c;
                                err = 1;
                        }

                        if(!strcmp("mode_name", a))
                        {
                                strncpy(c->mode_name, b, 8);
                                err = 1;
                                break;
                        }

                        if(!strcmp("plu_name", a))
                        {
                                strncpy(c->plu_name, b, 17);
                                err = 1;
                                break;
                        }

                        if(!strcmp("tp_name", a))
                        {
                                strncpy(c->tp_name, b, 8);
                                err = 1;
                                break;
                        }

                        if(!strcmp("mode", a))
                        {
                                c->mode = map_word(mode_types, b);
                                err = 1;
                                break;
                        }

                        break;
                }
        }

        return (err);
}

/* entrance for loading the standard sna.cfg file. */
int sna_load_cfg_file(char *cfile)
{
        FILE *InFile;
        int err = 0;

        InFile = OpenConfFile(cfile);
        if(InFile == NULL)
                return (-1);

        err = Parse(InFile, sfunk, pfunk);
        if(err < 0)
                return (err);

        err = sna_load_config(ginfo);
        if(err < 0)
                return (err);

        return (err);
}
