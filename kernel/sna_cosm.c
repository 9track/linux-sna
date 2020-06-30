/* sna_cosm.c: Linux Systems Network Architecture implementation
 * - Class-of-Service Manager
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
 
/*
 * Jay Please Look 00
 *		  [==]
 *
 * o You probably are not handling the BLANK and NULL mode names properly.
 */

#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/bitops.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/socket.h>
#include <linux/sockios.h>
#include <linux/in.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/notifier.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/route.h>
#include <linux/inet.h>
#include <linux/skbuff.h>
#include <linux/proc_fs.h>
#include <net/datalink.h>
#include <net/sock.h>
#include <linux/list.h>

#include <linux/sna.h>

static LIST_HEAD(cosm_list);

struct sna_cosm_cb *sna_cosm_find(unsigned char *name)
{
	struct sna_cosm_cb *c;
	struct list_head *le;

	list_for_each(le, &cosm_list) {
		c = list_entry(le, struct sna_cosm_cb, list);
		if (!strncmp(c->name, name, SNA_RESOURCE_NAME_LEN))
			return c;
	}
	return NULL;
}

struct sna_cosm_tg_cb *sna_cosm_find_tg_weight(struct sna_cosm_cb *c,
	unsigned short rsn)
{
	struct sna_cosm_tg_cb *t;

	for (t = c->tg; t != NULL; t = t->next) {
		if (t->rsn == rsn)
			return t;
	}
	return NULL;
}

struct sna_cosm_node_cb *sna_cosm_find_node_weight(struct sna_cosm_cb *c,
        unsigned short rsn)
{
        struct sna_cosm_node_cb *n;

        for (n = c->node; n != NULL; n = n->next) {
                if (n->rsn == rsn)
                        return n;
	}
        return NULL;
}

/* Add a COS definition to the table, this function will create a new
 * entry or update and existing entry.
 */
int sna_cosm_define_cos(struct sna_nof_cos *cos)
{
	struct sna_cosm_cb *c;
	struct sna_cosm_node_cb *n;
	struct sna_cosm_tg_cb *t;

	sna_debug(5, "init: %s\n", cos->name);
	c = sna_cosm_find(cos->name);
	if (!c) {
		new(c, GFP_ATOMIC);
		if (!c)
			return -ENOMEM;
		strncpy(c->name, cos->name, SNA_RESOURCE_NAME_LEN);
		c->tg			= NULL;
		c->node			= NULL;
		list_add_tail(&c->list, &cosm_list);
	}
	c->weight               = cos->weight;
        c->tx_priority          = cos->tx_priority;
        c->default_cos_invalid  = cos->default_cos_invalid;
        c->default_cos_null     = cos->default_cos_null;

	/* now append tg cos records */
	t = sna_cosm_find_tg_weight(c, cos->tg_rsn);
	if (!t) {
		new(t, GFP_ATOMIC);
		if (!t)
			return -ENOMEM;
		t->prev = NULL;
		t->next	= c->tg;
		c->tg	= t;
	}
	t->rsn				= cos->tg_rsn;
        t->min_cost_per_connect		= cos->min_cost_per_connect;
        t->max_cost_per_connect		= cos->max_cost_per_connect;
        t->min_cost_per_byte		= cos->min_cost_per_byte;
        t->max_cost_per_byte		= cos->max_cost_per_byte;
        t->min_security			= cos->min_security;
        t->max_security			= cos->max_security;
        t->min_propagation_delay	= cos->min_propagation_delay;
        t->max_propagation_delay	= cos->max_propagation_delay;
        t->min_effective_capacity	= cos->min_effective_capacity;
        t->max_effective_capacity	= cos->max_effective_capacity;
        t->min_user1			= cos->min_user1;
        t->max_user1			= cos->max_user1;
        t->min_user2			= cos->min_user2;
        t->max_user2			= cos->max_user2;
        t->min_user3			= cos->min_user3;
        t->max_user3			= cos->max_user3;

	/* now append node cos records */
	n = sna_cosm_find_node_weight(c, cos->node_rsn);
	if (!n) {
		new(n, GFP_ATOMIC);
		if (!n)
			return -ENOMEM;
		n->prev = NULL;
		n->next	= c->node;
		c->node	= n;
	}
	n->rsn				= cos->node_rsn;
	n->route_resistance		= cos->max_route_resistance;
	n->node_congested		= cos->max_node_congested;
//	n->inter_routing_depleted	= cos->inter_routing_depleted;
	return 0;
}

int sna_cosm_delete_cos(struct sna_nof_cos *cos)
{
	struct list_head *le, *se;
	struct sna_cosm_cb *c;

	sna_debug(5, "init\n");
	list_for_each_safe(le, se, &cosm_list) {
		c = list_entry(le, struct sna_cosm_cb, list);
                if (!strcmp(c->name, cos->name)) {
			struct sna_cosm_tg_cb *t;
			struct sna_cosm_node_cb *n;

			for (t = c->tg; t != NULL; t = t->next)
				kfree(t);
			for (n = c->node; n != NULL; n = n->next)
				kfree(n);
			list_del(&c->list);
                        kfree(c);
                        return 0;
                }
        }
	return -ENOENT;
}

int sna_cosm_cos_tpf_vector(struct sna_cos_tpf_vector *cos)
{
	struct sna_cosm_cb *c;
	int i;

	sna_debug(5, "sna_cosm_cos_tpf_vector\n");
	c = sna_cosm_find(cos->mode_name);
	if (!c)
		return -ENOENT;
	memset(&cos->v, 0, sizeof(struct sna_cos));
	cos->v.type		= 0x2C;
	cos->v.tx_priority	= c->tx_priority;
	i = strlen(c->name);
	memcpy(cos->v.cos_name, c->name, i);
	for (i = strlen(cos->v.cos_name); i < 8; i++)
		cos->v.cos_name[i] = 0x40;
	cos->v.len = sizeof(struct sna_cos);
	return 0;
}

/* Slick little way to initialize all of the default COS records. */
struct sna_nof_cos cos_defaults[] = {
	{0, "CONNECT", 0, SNA_TP_MEDIUM   , 1, 1, 30, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x4, 0x7, 0xF, 0, 255, 0, 255, 0, 255, 5, 0, 31, 0, 0},
        {0, "CONNECT", 0, SNA_TP_MEDIUM   , 1, 1, 60, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x7, 0x4, 0xF, 0, 255, 0, 255, 0, 255, 10, 0, 63, 0, 0},
        {0, "CONNECT", 0, SNA_TP_MEDIUM   , 1, 1, 90, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x7, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 20, 0, 95, 0, 0},
        {0, "CONNECT", 0, SNA_TP_MEDIUM   , 1, 1, 120, 0, 0, 0, 0, 0x01, 0xFF,
	0x0, 0x7, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 40, 0, 127, 0, 0},
        {0, "CONNECT", 0, SNA_TP_MEDIUM   , 1, 1, 150, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x9, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 60, 0, 159, 0, 0},
        {0, "CONNECT", 0, SNA_TP_MEDIUM   , 1, 1, 180, 0, 128, 0, 128, 0x01, 0xFF,
        0x0, 0x9, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 80, 0, 191, 0, 0},
        {0, "CONNECT", 0, SNA_TP_MEDIUM   , 1, 1, 210, 0, 196, 0,196,0x01,0xFF,
        0x0, 0xF, 0x2, 0xF, 0, 255, 0, 255, 0, 255, 120, 0, 223, 0, 0},
        {0, "CONNECT", 0, SNA_TP_MEDIUM   , 1, 1, 240, 0, 255, 0,255,0x01,0xFF,
        0x0, 0xF, 0x0, 0xF, 0, 255, 0, 255, 0, 255, 160, 0, 255, 0, 1},
	{0, "#BATCH", 10, SNA_TP_LOW      , 0, 0, 30, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x4, 0x7, 0xF, 0, 255, 0, 255, 0, 255, 5, 0, 31, 0, 0},
        {0, "#BATCH", 10, SNA_TP_LOW      , 0, 0, 60, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x7, 0x4, 0xF, 0, 255, 0, 255, 0, 255, 10, 0, 63, 0, 0},
        {0, "#BATCH", 10, SNA_TP_LOW      , 0, 0, 90, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x7, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 20, 0, 95, 0, 0},
        {0, "#BATCH", 10, SNA_TP_LOW      , 0, 0, 120, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x7, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 40, 0, 127, 0, 0},
        {0, "#BATCH", 10, SNA_TP_LOW      , 0, 0, 150, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x9, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 60, 0, 159, 0, 0},
        {0, "#BATCH", 10, SNA_TP_LOW      , 0, 0, 180, 0, 128, 0,128,0x01,0xFF,
        0x0, 0x9, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 80, 0, 191, 0, 0},
        {0, "#BATCH", 10, SNA_TP_LOW      , 0, 0, 210, 0, 196, 0,196,0x01,0xFF,
        0x0, 0xF, 0x2, 0xF, 0, 255, 0, 255, 0, 255, 120, 0, 223, 0, 0},
        {0, "#BATCH", 10, SNA_TP_LOW      , 0, 0, 240, 0, 255, 0,255,0x01,0xFF,
        0x0, 0xF, 0x0, 0xF, 0, 255, 0, 255, 0, 255, 160, 0, 255, 0, 1},
	{0, "#INTER", 20, SNA_TP_HIGH     , 0, 0, 30, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x4, 0x7, 0xF, 0, 255, 0, 255, 0, 255, 5, 0, 31, 0, 0},
        {0, "#INTER", 20, SNA_TP_HIGH     , 0, 0, 60, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x7, 0x4, 0xF, 0, 255, 0, 255, 0, 255, 10, 0, 63, 0, 0},
        {0, "#INTER", 20, SNA_TP_HIGH     , 0, 0, 90, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x7, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 20, 0, 95, 0, 0},
        {0, "#INTER", 20, SNA_TP_HIGH     , 0, 0, 120, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x7, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 40, 0, 127, 0, 0},
        {0, "#INTER", 20, SNA_TP_HIGH     , 0, 0, 150, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x9, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 60, 0, 159, 0, 0},
        {0, "#INTER", 20, SNA_TP_HIGH     , 0, 0, 180, 0, 128, 0,128,0x01,0xFF,
        0x0, 0x9, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 80, 0, 191, 0, 0},
        {0, "#INTER", 20, SNA_TP_HIGH     , 0, 0, 210, 0, 196, 0,196,0x01,0xFF,
        0x0, 0xF, 0x2, 0xF, 0, 255, 0, 255, 0, 255, 120, 0, 223, 0, 0},
        {0, "#INTER", 20, SNA_TP_HIGH     , 0, 0, 240, 0, 255, 0,255,0x01,0xFF,
        0x0, 0xF, 0x0, 0xF, 0, 255, 0, 255, 0, 255, 160, 0, 255, 0, 1},
	{0, "#BATCHSC", 30, SNA_TP_LOW    , 0, 0, 30, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x4, 0x7, 0xF, 0, 255, 0, 255, 0, 255, 5, 0, 31, 0, 0},
        {0, "#BATCHSC", 30, SNA_TP_LOW    , 0, 0, 60, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x7, 0x4, 0xF, 0, 255, 0, 255, 0, 255, 10, 0, 63, 0, 0},
        {0, "#BATCHSC", 30, SNA_TP_LOW    , 0, 0, 90, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x7, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 20, 0, 95, 0, 0},
        {0, "#BATCHSC", 30, SNA_TP_LOW    , 0, 0, 120, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x7, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 40, 0, 127, 0, 0},
        {0, "#BATCHSC", 30, SNA_TP_LOW    , 0, 0, 150, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x9, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 60, 0, 159, 0, 0},
        {0, "#BATCHSC", 30, SNA_TP_LOW    , 0, 0, 180, 0, 128, 0,128,0x01,0xFF,
        0x0, 0x9, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 80, 0, 191, 0, 0},
        {0, "#BATCHSC", 30, SNA_TP_LOW    , 0, 0, 210, 0, 196, 0,196,0x01,0xFF,
        0x0, 0xF, 0x2, 0xF, 0, 255, 0, 255, 0, 255, 120, 0, 223, 0, 0},
        {0, "#BATCHSC", 30, SNA_TP_LOW    , 0, 0, 240, 0, 255, 0,255,0x01,0xFF,
        0x0, 0xF, 0x0, 0xF, 0, 255, 0, 255, 0, 255, 160, 0, 255, 0, 1},
        {0, "#INTERSC", 40, SNA_TP_HIGH   , 0, 0, 30, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x4, 0x7, 0xF, 0, 255, 0, 255, 0, 255, 5, 0, 31, 0, 0},
        {0, "#INTERSC", 40, SNA_TP_HIGH   , 0, 0, 60, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x7, 0x4, 0xF, 0, 255, 0, 255, 0, 255, 10, 0, 63, 0, 0},
        {0, "#INTERSC", 40, SNA_TP_HIGH   , 0, 0, 90, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x7, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 20, 0, 95, 0, 0},
        {0, "#INTERSC", 40, SNA_TP_HIGH   , 0, 0, 120, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x7, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 40, 0, 127, 0, 0},
        {0, "#INTERSC", 40, SNA_TP_HIGH   , 0, 0, 150, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x9, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 60, 0, 159, 0, 0},
        {0, "#INTERSC", 40, SNA_TP_HIGH   , 0, 0, 180, 0, 128, 0,128,0x01,0xFF,
        0x0, 0x9, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 80, 0, 191, 0, 0},
        {0, "#INTERSC", 40, SNA_TP_HIGH   , 0, 0, 210, 0, 196, 0,196,0x01,0xFF,
        0x0, 0xF, 0x2, 0xF, 0, 255, 0, 255, 0, 255, 120, 0, 223, 0, 0},
        {0, "#INTERSC", 40, SNA_TP_HIGH   , 0, 0, 240, 0, 255, 0,255,0x01,0xFF,
        0x0, 0xF, 0x0, 0xF, 0, 255, 0, 255, 0, 255, 160, 0, 255, 0, 1},
	{0, "CPSVCMG", 50, SNA_TP_NETWORK, 0, 0, 30, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x4, 0x7, 0xF, 0, 255, 0, 255, 0, 255, 5, 0, 31, 0, 0},
        {0, "CPSVCMG", 50, SNA_TP_NETWORK, 0, 0, 60, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x7, 0x4, 0xF, 0, 255, 0, 255, 0, 255, 10, 0, 63, 0, 0},
        {0, "CPSVCMG", 50, SNA_TP_NETWORK, 0, 0, 90, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x7, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 20, 0, 95, 0, 0},
        {0, "CPSVCMG", 50, SNA_TP_NETWORK, 0, 0, 120, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x7, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 40, 0, 127, 0, 0},
        {0, "CPSVCMG", 50, SNA_TP_NETWORK, 0, 0, 150, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x9, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 60, 0, 159, 0, 0},
        {0, "CPSVCMG", 50, SNA_TP_NETWORK, 0, 0, 180, 0, 128, 0,128,0x01,0xFF,
        0x0, 0x9, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 80, 0, 191, 0, 0},
        {0, "CPSVCMG", 50, SNA_TP_NETWORK, 0, 0, 210, 0, 196, 0,196,0x01,0xFF,
        0x0, 0xF, 0x2, 0xF, 0, 255, 0, 255, 0, 255, 120, 0, 223, 0, 0},
        {0, "CPSVCMG", 50, SNA_TP_NETWORK, 0, 0, 240, 0, 255, 0,255,0x01,0xFF,
        0x0, 0xF, 0x0, 0xF, 0, 255, 0, 255, 0, 255, 160, 0, 255, 0, 1},
	{0, "SNASVCMG", 60, SNA_TP_NETWORK, 0, 0, 30, 0, 0, 0, 0, 0x01, 0xFF, 
	0x0, 0x4, 0x7, 0xF, 0, 255, 0, 255, 0, 255, 5, 0, 31, 0, 0},
	{0, "SNASVCMG", 60, SNA_TP_NETWORK, 0, 0, 60, 0, 0, 0, 0, 0x01, 0xFF, 
	0x0, 0x7, 0x4, 0xF, 0, 255, 0, 255, 0, 255, 10, 0, 63, 0, 0},
	{0, "SNASVCMG", 60, SNA_TP_NETWORK, 0, 0, 90, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x7, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 20, 0, 95, 0, 0},
	{0, "SNASVCMG", 60, SNA_TP_NETWORK, 0, 0, 120, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x7, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 40, 0, 127, 0, 0},
	{0, "SNASVCMG", 60, SNA_TP_NETWORK, 0, 0, 150, 0, 0, 0, 0, 0x01, 0xFF,
        0x0, 0x9, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 60, 0, 159, 0, 0},
	{0, "SNASVCMG", 60, SNA_TP_NETWORK, 0, 0, 180, 0, 128, 0,128,0x01,0xFF,
        0x0, 0x9, 0x3, 0xF, 0, 255, 0, 255, 0, 255, 80, 0, 191, 0, 0},
	{0, "SNASVCMG", 60, SNA_TP_NETWORK, 0, 0, 210, 0, 196, 0,196,0x01,0xFF,
        0x0, 0xF, 0x2, 0xF, 0, 255, 0, 255, 0, 255, 120, 0, 223, 0, 0},
	{0, "SNASVCMG", 60, SNA_TP_NETWORK, 0, 0, 240, 0, 255, 0,255,0x01,0xFF,
        0x0, 0xF, 0x0, 0xF, 0, 255, 0, 255, 0, 255, 160, 0, 255, 0, 1}
};

int sna_cosm_init_default_cos(void)
{
	struct sna_cosm_cb *c;
	struct list_head *le;
	int i;

	sna_debug(5, "init\n");
	for (i = 0; i < 56; i++)
		sna_cosm_define_cos(&cos_defaults[i]);
	list_for_each(le, &cosm_list) {
		c = list_entry(le, struct sna_cosm_cb, list);
		sna_debug(5, "cos_name %s\n", c->name);
	}
	return 0;
}

int sna_cos_ginfo(struct sna_cosm_cb *cos, char *buf, int len)
{
	struct cosreq cr;
	int done = 0;

	sna_debug(10, "init\n");
	if (!buf) {
                done += sizeof(cr);
                return done;
        }
        if (len < (int)sizeof(cr))
                return done;
        memset(&cr, 0, sizeof(struct cosreq));

        /* Move the data here */
	strncpy(cr.name, cos->name, SNA_RESOURCE_NAME_LEN);
	cr.weight		= cos->weight;
	cr.tx_priority		= cos->tx_priority;
	cr.default_cos_invalid	= cos->default_cos_invalid;
	cr.default_cos_null	= cos->default_cos_null;

	if (copy_to_user(buf, &cr, sizeof(struct cosreq)))
                return -EFAULT;
        buf  += sizeof(struct cosreq);
        len  -= sizeof(struct cosreq);
        done += sizeof(struct cosreq);
        return done;
}

int sna_cosm_query_cos(char *arg)
{
	struct sna_cosm_cb *cos;
	struct list_head *le;
	int len, total, done;
	struct cosconf cc;
	char *pos;

	sna_debug(5, "init\n");
	if (copy_from_user(&cc, arg, sizeof(cc)))
                return -EFAULT;

        pos = cc.cosc_buf;
        len = cc.cos_len;

        /*
         * Get the data and put it into the structure
         */
        total = 0;
	list_for_each(le, &cosm_list) {
		cos = list_entry(le, struct sna_cosm_cb, list);
                if (pos == NULL)
                        done = sna_cos_ginfo(cos, NULL, 0);
                else
                        done = sna_cos_ginfo(cos, pos + total, len - total);
                if (done < 0)
                        return -EFAULT;
                total += done;
        }

        cc.cos_len = total;
	if (copy_to_user(arg, &cc, sizeof(cc)))
                return -EFAULT;
        return 0;
}

#ifdef CONFIG_PROC_FS
int sna_cosm_get_info_tg(char *buffer, char **start,
        off_t offset, int length)
{
	struct sna_cosm_cb *c;
        off_t pos = 0, begin = 0;
	struct list_head *le;
        int len = 0;

	/* Output the NOF data for the /proc filesystem. */
        len += sprintf(buffer, "%-9s%5s%5s"
                "%5s%5s%4s%4s%4s%4s%4s%4s%4s"
                "%4s%4s%4s%5s%6s\n",
                "name", "lcpc",
                "hcpc", "lcpb",
                "hcpb", "lpd",
                "hpd", "lec",
                "lec", "lu1", "hu1",
                "lu2", "hu2", "lu3", "hu3",
                "stat", "quisc");

	list_for_each(le, &cosm_list) {
		struct sna_cosm_tg_cb *t;

		c = list_entry(le, struct sna_cosm_cb, list);
		for (t = c->tg; t != NULL; t = t->next) {
			len += sprintf(buffer + len, "%-9s%5d%5d"
                	"%5d%5d%4d%4d%4d%4d%4d%4d%4d"
                	"%4d%4d%4d%5d%6d\n",
			c->name, t->min_cost_per_connect, 
			t->max_cost_per_connect, t->min_cost_per_byte,
			t->max_cost_per_byte, t->min_propagation_delay,
			t->max_propagation_delay, t->min_effective_capacity,
			t->max_effective_capacity, t->min_user1, t->max_user1,
			t->min_user2, t->max_user2, t->min_user3, t->max_user3,
			t->operational, t->quiescing);
		}

                /* Are we still dumping unwanted data then discard the record */
		pos = begin + len;
                if (pos < offset) {
                        len = 0;        /* Keep dumping into the buffer start */
			begin = pos;
                }
                if (pos > offset + length)       /* We have dumped enough */
                        break;
        }

        /* The data in question runs from begin to begin+len */
        *start = buffer + (offset - begin);     /* Start of wanted data */
        len -= (offset - begin);   /* Remove unwanted header data from length */
	if (len > length)
                len = length;      /* Remove unwanted tail data from length */
        if (len < 0)
                len = 0;
	return len;
}

int sna_cos_get_info_node(char *buffer, char **start,
        off_t offset, int length)
{
        struct sna_cosm_cb *c;
        off_t pos = 0, begin = 0;
	struct list_head *le;
        int len = 0;

        /* Output the NOF data for the /proc filesystem. */
        len += sprintf(buffer, "%-9s%4s%4s%4s%7s%8s%10s\n",
                "name", "rr", "nc",
		"ird", "quiesc", "gateway",
		"directory");

	list_for_each(le, &cosm_list) {
		struct sna_cosm_node_cb *n;

		c = list_entry(le, struct sna_cosm_cb, list);
		for (n = c->node; n != NULL; n = n->next) {
			len += sprintf(buffer + len, "%-9s%4d%4d%4d%7d%8d%10d\n",
			c->name, n->route_resistance, n->node_congested,
			n->inter_routing_depleted, n->quiescing, 
			n->gateway_support, n->central_directory);
		}

                /* Are we still dumping unwanted data then discard the record */
		pos = begin + len;
                if (pos < offset) {
                        len = 0;        /* Keep dumping into the buffer start */
			begin = pos;
                }
                if (pos > offset + length)       /* We have dumped enough */
                        break;
        }

        /* The data in question runs from begin to begin+len */
        *start = buffer + (offset - begin);     /* Start of wanted data */
        len -= (offset - begin);   /* Remove unwanted header data from length */
	if (len > length)
                len = length;      /* Remove unwanted tail data from length */
        if (len < 0)
                len = 0;
	return len;
}

int sna_cosm_get_info(char *buffer, char **start,
        off_t offset, int length)
{
	struct sna_cosm_cb *c;
        off_t pos = 0, begin = 0;
	struct list_head *le;
        int len = 0;

        /* Output the NOF data for the /proc filesystem. */
        len += sprintf(buffer, "%-9s%7s%12s%17s%20s\n",
		"name", "weight", "tx_priority", "default_cos_null",
		"default_cos_invalid");

	list_for_each(le, &cosm_list) {
		c = list_entry(le, struct sna_cosm_cb, list);
		len += sprintf(buffer+len, "%-9s%7d%12d%17d%20d\n",
		c->name, c->weight, c->tx_priority, c->default_cos_null,
		c->default_cos_invalid);

                /* Are we still dumping unwanted data then discard the record */
		pos = begin + len;
                if (pos < offset) {
                        len = 0;        /* Keep dumping into the buffer start */
			begin = pos;
                }
                if (pos > offset + length)       /* We have dumped enough */
                        break;
        }

        /* The data in question runs from begin to begin+len */
        *start = buffer + (offset - begin);     /* Start of wanted data */
        len -= (offset - begin);   /* Remove unwanted header data from length */
	if (len > length)
                len = length;      /* Remove unwanted tail data from length */
        if (len < 0)
                len = 0;
        return len;
}
#endif

int sna_cosm_create(struct sna_nof_node *start)
{
	sna_debug(5, "sna_cosm_create\n");
	sna_cosm_init_default_cos();
	return 0;
}

int sna_cosm_destroy(struct sna_nof_node *delete)
{
	struct list_head *le, *se;
	struct sna_cosm_cb *c;
	
	sna_debug(5, "init\n");
	list_for_each_safe(le, se, &cosm_list) {
		struct sna_cosm_tg_cb *t;
                struct sna_cosm_node_cb *n;

		c = list_entry(le, struct sna_cosm_cb, list);
                for (t = c->tg; t != NULL; t = t->next)
	                kfree(t);
                for (n = c->node; n != NULL; n = n->next)
                        kfree(n);
		list_del(&c->list);
		kfree(c);
	}
	return 0;
}
