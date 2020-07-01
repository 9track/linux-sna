/* sna_tdm.c: Linux Systems Network Architecture implementation
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
 * Bugs:
 * - process TDUs
 * - Garbage collection
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/sna.h>

#include "sna_common.h"

static LIST_HEAD(node_list);
static LIST_HEAD(ancb_list);

struct sna_tdm_node_cb *sna_tdm_node_get_by_netid(sna_netid *netid)
{
	struct sna_tdm_node_cb *n;
	struct list_head *le;

	sna_debug(5, "init: %s\n", sna_pr_netid(netid));
	list_for_each(le, &node_list) {
		n = list_entry(le, struct sna_tdm_node_cb, list);
		sna_debug(5, "n=%s\n", sna_pr_netid(&n->netid));
		if (!strncmp(n->netid.net, netid->net, SNA_NETWORK_NAME_LEN)
			&& !strncmp(n->netid.name, netid->name, SNA_RESOURCE_NAME_LEN))
			return n;
	}
	return NULL;
}

#ifdef NOT
struct sna_tg_cb *sna_tdm_find_tg_by_mac(char *mac)
{
	struct sna_tdm_node_cb *n;
	struct list_head *le, *se;
	struct sna_tg_cb *t;

	sna_debug(5, "init: (%s)\n", sna_pr_ether(mac));
	list_for_each(le, &node_list) {
		n = list_entry(le, struct sna_tdm_node_cb, list);
		list_for_each(se, &n->tg_list) {
			t = list_entry(se, struct sna_tg_cb, list);
			if (!memcmp(t->tg_vector.desc.dlc.mac, mac, IFHWADDRLEN))
				return t;
		}
	}
	return NULL;
}
#endif

struct sna_tg_cb *sna_tdm_tg_get_by_number(struct sna_tdm_node_cb *node,
	u_int8_t tg_number)
{
	struct list_head *le;
	struct sna_tg_cb *tg;

	sna_debug(5, "init\n");
	list_for_each(le, &node->tg_list) {
		tg = list_entry(le, struct sna_tg_cb, list);
		if (tg->tg_number == tg_number)
			return tg;
	}
	return NULL;
}

int sna_tdm_cp_status(void)
{
	return 0;
}

int sna_tdm_tg_update(sna_netid *name, struct sna_tg_update *update)
{
	struct sna_tdm_node_cb *node;
	struct sna_tg_cb *tg;
	int err = -ENOENT;

	sna_debug(5, "init\n");
	node = sna_tdm_node_get_by_netid(name);
	if (!node)
		goto out;
	tg = sna_tdm_tg_get_by_number(node, update->tg_number);
	if (!tg) {
		new(tg, GFP_ATOMIC);
		if (!tg) {
			err = -ENOMEM;
			goto out;
		}
		INIT_LIST_HEAD(&tg->cos_list);
		list_add_tail(&tg->list, &node->tg_list);
	}
	err = 0;
	do_gettimeofday(&tg->updated);
	memcpy(&tg->plu_name, &update->plu_name, sizeof(sna_netid));
	tg->tg_number			= update->tg_number;
	tg->ls_index			= update->ls_index;
	tg->port_index			= update->port_index;
	tg->dlc_index			= update->dlc_index;
	tg->l_node_type			= update->l_node_type;
	tg->r_node_type			= update->r_node_type;
	tg->routing			= update->routing;
//	sna_rss_resource_updates();
out:	return err;
}

int sna_tdm_define_node_chars(struct sna_nof_node *n)
{
	struct sna_tdm_node_cb *node;
	int err = -ENOMEM;

	sna_debug(5, "init\n");
	node = sna_tdm_node_get_by_netid(&n->netid);
	if (!node) {
		new(node, GFP_ATOMIC);
		if (!node)
			goto out;
		memcpy(&node->netid, &n->netid, sizeof(sna_netid));
		INIT_LIST_HEAD(&node->tg_list);
		INIT_LIST_HEAD(&node->cos_list);
		list_add_tail(&node->list, &node_list);
	}
	err = 0;
	do_gettimeofday(&node->updated);
	memcpy(&node->cp_name, &n->netid, sizeof(sna_netid));
	node->route_resistance 		= n->route_resistance;
	node->cn			= 0;
	node->inter_route_depleted	= 0;
	node->garbage			= 0;
	node->quiescing			= n->quiescing;
	node->gateway			= 0;
	node->cds			= 0;
	node->inter_route		= 0;
	node->peripheral		= 0;
	node->interchange		= 0;
	node->extended			= 0;
	node->hpr			= 0;
//	sna_rss_resource_updates();
out:	return err;
}

int sna_tdm_node_congenstion(void)
{
	return 0;
}

int sna_tdm_query_cpname(void)
{
	return 0;
}

/* Returned last FRSN recieved for cp_name specified */
u_int32_t sna_tdm_request_last_frsn(sna_netid *cp_name)
{
	struct sna_tdm_node_cb *node;

	node = sna_tdm_node_get_by_netid(cp_name);
	if (!node)
		return 0;	/* error */
	return node->frsn;
}

/* simply return the first tg_cb for this node, we will do more later.
 */
struct sna_tg_cb *sna_tdm_request_tg_vectors(sna_netid *remote_name)
{
	struct sna_tdm_node_cb *node;
	struct sna_tg_cb *tg;
	struct list_head *le;

	sna_debug(5, "init\n");
	node = sna_tdm_node_get_by_netid(remote_name);
	if (!node)
		return NULL;
	list_for_each(le, &node->tg_list) {
		tg = list_entry(le, struct sna_tg_cb, list);
		return tg;
	}
	return NULL;
}

#ifdef NOT
	cb = sna_tdm_node_get_by_netid(&v->org_cp_name);
	if (!cb)
		return -ENOENT;
	list_for_each(le, &cb->tg_list) {
		tg = list_entry(le, struct sna_tg_cb, list);
		break;
	}
	if (!tg)
		return -ENOENT;
	new(v->tg_vectors, GFP_ATOMIC);
	if (!v->tg_vectors)
		return -ENOMEM;
	memcpy(v->tg_vectors, &tg->tg_vector, sizeof(struct sna_tg_vector));
//	memcpy(v->tg_vectors, &cb->tg_list->tg_vector,
//		sizeof(struct sna_tg_vector));
	new_s(v->tg_vectors->desc.id.pcp_name,
		v->tg_vectors->desc.id.pcp_len + 1, GFP_ATOMIC);
	if (!v->tg_vectors->desc.id.pcp_name)
		return -ENOMEM;
	strcpy(v->tg_vectors->desc.id.pcp_name, tg->tg_vector.desc.id.pcp_name);
//	strcpy(v->tg_vectors->desc.id.pcp_name,
//		cb->tg_list->tg_vector.desc.id.pcp_name);
#endif

int sna_tdm_garbage_collection(void)
{
	return 0;
}

int sna_tdm_tdu_chk_errors(void)
{
	return 0;
}

#ifdef CONFIG_PROC_FS
int sna_tdm_get_info(struct seq_file *m, void *v)
{
	struct sna_tdm_node_cb *c;
	struct list_head *le;

	seq_printf(m, "%-18s\n", "NetID.Node");
	list_for_each(le, &node_list) {
		c = list_entry(le, struct sna_tdm_node_cb, list);
		seq_printf(m, "%-18s\n",sna_pr_netid(&c->netid));
	}

	return 0;
}

int sna_tdm_get_info_tg(struct seq_file *m, void *v)
{
	struct sna_tdm_node_cb *c;
	struct list_head *le, *se;
	struct sna_tg_cb *t;

	seq_printf(m, "%-18s%5s%4s%7s%8s%10s%5s%5s%4s%4s%9s%18s%6s%6s%6s\n",
		"NetID.Node", "type", "tgn", "status", "garbage",
		"quiescing", "cpcp", "ecap", "cpc",
		"cpb", "security", "propagation_delay",
		"user1", "user2", "user3");
	list_for_each(le, &node_list) {
		c = list_entry(le, struct sna_tdm_node_cb, list);
		list_for_each(se, &c->tg_list) {
			t = list_entry(se, struct sna_tg_cb, list);
			seq_printf(m, "%-18s\n",
				sna_pr_netid(&c->netid));
#ifdef NOT
			seq_printf(m, "%5d%4d%7d%8d%10d%5d"
				"%5d%4d%4d%9d%18d%6d%6d%6d\n",
				t->tg_vector.desc.id.type,
				t->tg_vector.desc.id.tg_number,
				t->tg_vector.chars.status,
				t->tg_vector.chars.garbage,
				t->tg_vector.chars.quiescing,
				t->tg_vector.chars.cpcp_session,
				t->tg_vector.chars.effective_capacity,
				t->tg_vector.chars.cost_per_connect,
				t->tg_vector.chars.cost_per_byte,
				t->tg_vector.chars.security,
				t->tg_vector.chars.propagation_delay,
				t->tg_vector.chars.user1,
				t->tg_vector.chars.user2,
				t->tg_vector.chars.user3);
#endif
		}
	}

	return 0;
}
#endif

int sna_tdm_create(struct sna_nof_node *start)
{
	return 0;
}

int sna_tdm_destroy(struct sna_nof_node *delete)
{
	struct sna_tdm_node_cb *cb;
	struct list_head *le, *se, *ae, *be;
	struct sna_tg_cb *t;

	list_for_each_safe(le, se, &node_list) {
		cb = list_entry(le, struct sna_tdm_node_cb, list);
		list_for_each_safe(ae, be, &cb->tg_list) {
			t = list_entry(ae, struct sna_tg_cb, list);
			list_del(&t->list);
			kfree(t);
		}
		list_del(&cb->list);
		kfree(cb);
	}
	return 0;
}
