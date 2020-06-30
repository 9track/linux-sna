/* sna_asm.c: Linux Systems Network Architecture implementation
 * - Linux-SNA Address Space Manager (Connect HS to PC).
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

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <net/datalink.h>
#include <net/sock.h>
#include <linux/list.h>

#include <linux/sna.h>

static LIST_HEAD(asm_list);

/* Display a Path Control ID */
char *sna_asm_pr_pcid(unsigned char *ptr)
{
	static char buff[64];

	sprintf(buff, "%02X%02X%02X%02X%02X%02X%02X%02X",
		(ptr[0] & 0377), (ptr[1] & 0377), (ptr[2] & 0377),
		(ptr[3] & 0377), (ptr[4] & 0377), (ptr[5] & 0377),
		(ptr[6] & 0377), (ptr[7] & 0377));
	return buff;
}

struct sna_asm_cb *sna_asm_get_by_index(u_int32_t index)
{
	struct sna_asm_cb *a;
	struct list_head *le;

	sna_debug(5, "init: %d\n", index);
	list_for_each(le, &asm_list) {
		a = list_entry(le, struct sna_asm_cb, list);
		sna_debug(5, "a->index=%d\n", a->index);
		if (a->index == index)
			return a;
	}
	return NULL;
}

struct sna_asm_cb *sna_asm_get_by_lfsid(struct sna_lfsid *lfsid)
{
	struct sna_lfsid_block *l;
	struct list_head *le, *be;
	struct sna_asm_cb *a;
	int i;

	sna_debug(5, "init\n");
	list_for_each(le, &asm_list) {
		a = list_entry(le, struct sna_asm_cb, list);
		list_for_each(be, &a->blk_list) {
			l = list_entry(be, struct sna_lfsid_block, list);
			for (i = 0; i < 256; i++) {
				if (l->l[i].odai == lfsid->odai
					&& l->l[i].sidh == lfsid->sidh
					&& l->l[i].sidl == lfsid->sidl)
					return a;
			}
		}
	}
	return NULL;
}

struct sna_lfsid *sna_asm_lfsid_get_by_sidhl(u_int8_t odai, u_int8_t sidh,
	u_int8_t sidl)
{
	struct sna_lfsid_block *l;
	struct list_head *le, *se;
	struct sna_asm_cb *a;
	int i;

	sna_debug(5, "init: odai:%d sidh:%d sidl:%d\n", odai, sidh, sidl);
	list_for_each(le, &asm_list) {
		a = list_entry(le, struct sna_asm_cb, list);
		list_for_each(se, &a->blk_list) {
			l = list_entry(se, struct sna_lfsid_block, list);
			for (i = 0; i < 256; i++) {
				sna_debug(5, "l->odai=%d l->sidh=%d l->sidl=%d\n",
					l->l[i].odai, l->l[i].sidh, l->l[i].sidl);
				if (l->l[i].odai == odai
					&& l->l[i].sidh == sidh
					&& l->l[i].sidl == sidl)
					return &l->l[i];
			}
		}
	}
	return NULL;
}

int sna_asm_activate_as(struct sna_asm_cb *as)
{
	sna_debug(5, "init\n");
	if (sna_asm_get_by_index(as->index))
		return -EEXIST;
	INIT_LIST_HEAD(&as->blk_list);
	list_add_tail(&as->list, &asm_list);
	return 0;
}

int sna_asm_deactivate_as(u_int32_t index)
{
	struct list_head *le, *se;
	struct sna_asm_cb *a;

	sna_debug(5, "init\n");
	list_for_each_safe(le, se, &asm_list) {
		a = list_entry(le, struct sna_asm_cb, list);
		if (a->index == index) {
			list_del(&a->list);
			kfree(a);
			return 0;
		}
	}
	return -ENOENT;
}

static int sna_asm_set_sidhl(struct sna_asm_cb *a, struct sna_lfsid *lf)
{
	struct sna_lfsid_block *l;
	u_int8_t sidh_s = 0x01; // 0x02;
	u_int8_t sidl_s = 0x00;
	struct list_head *le;
	int i;

	sna_debug(5, "init\n");
	/* locate a unique sidh. */
	list_for_each(le, &a->blk_list) {
		l = list_entry(le, struct sna_lfsid_block, list);
		for (i = 0; i < 256; i++) {
			if (l->l[i].sidh == sidh_s) {
				sidh_s++;
				if (sidh_s > 0xFE)
					return -EUSERS;
				continue;
			}
		}
	}

	/* locate a unique sidl. */
	list_for_each(le, &a->blk_list) {
		l = list_entry(le, struct sna_lfsid_block, list);
		for (i = 0; i < 256; i++) {
			if (l->l[i].sidl == sidl_s) {
				sidl_s++;
				if (sidl_s > 0xFF)
					return -EUSERS;
				continue;
			}
		}
	}

	lf->sidh = sidh_s;
	lf->sidl = sidl_s;
	return 0;
}

/* Locate address space, look for a free lfsid, if not in any of the
 * existing blocks, then allocate a new block, return a fresh lfsid.
 */
struct sna_lfsid *sna_asm_assign_lfsid(u_int32_t pc_index, u_int32_t sm_index)
{
	struct sna_lfsid_block *blk;
	struct sna_asm_cb *a;
	struct list_head *le;
	int i, blk_num = 0;

	sna_debug(5, "init\n");
	a = sna_asm_get_by_index(pc_index);
	if (!a) {
		sna_debug(5, "unable to locate address space\n");
		return NULL;
	}
rstart:	/* search for a free lfsid */
	list_for_each(le, &a->blk_list) {
		blk = list_entry(le, struct sna_lfsid_block, list);
		blk_num++;
		for (i = 0; i < 256; i++) {
			if (blk->l[i].active)	/* lfsid is in use */
				continue;

			/* found a non-active lfsid */
			blk->l[i].active 	= 1;
			blk->l[i].sm_index	= sm_index;
			blk->l[i].odai		= a->odai;
			if (sna_asm_set_sidhl(a, &blk->l[i]) < 0)
				return NULL;
			return &blk->l[i];
		}
	}

	/* could not find any non-active lfsids in already alloc'd blocks */
	new(blk, GFP_ATOMIC);
	if (!blk)
		return NULL;
	list_add_tail(&blk->list, &a->blk_list);
	a->blk_count++;
	goto rstart;	/* just added 256 addresses, lets try it again */
}

int sna_asm_free_lfsid(u_int32_t pc_index, u_int32_t sm_index,
	struct sna_lfsid *lf)
{
	struct sna_lfsid_block *l;
	struct sna_asm_cb *a;
	struct list_head *le;
	int i;

	sna_debug(5, "init\n");
	a = sna_asm_get_by_index(pc_index);
	if (!a)
		return -ENOENT;

	/* locate the lfsid */
	list_for_each(le, &a->blk_list) {
		l = list_entry(le, struct sna_lfsid_block, list);
		for (i = 0; i < 256; i++) {
			if (l->l[i].active && l->l[i].sm_index == sm_index
				&& l->l[i].odai == lf->odai
				&& l->l[i].sidh == lf->sidh
				&& l->l[i].sidl == lf->sidl) {
				memset(&l->l[i], 0, sizeof(struct sna_lfsid));
				return 0;
			}
		}
	}
	return -ENOENT;
}

/* zero terminated list of ru request codes we support. */
u_int8_t sna_asm_ru_supported[] = {
	SNA_RU_RC_BIND,
	SNA_RU_RC_UNBIND,
	0
};

/* asm bind error checking - 2.6.5 (appnarch.boo)
 * return negative errno if failure, otherwise return the mu request code.
 */
int sna_asm_rx_mu_chk_err(struct sk_buff *skb)
{
	u_int8_t rc;

	/* right now we just check to make sure it is an mu we
	 * know how to process.
	 */
	sna_debug(5, "init\n");
	for (rc = 0; sna_asm_ru_supported[rc]; rc++) {
		if (skb->data[0] == sna_asm_ru_supported[rc])
			return sna_asm_ru_supported[rc];
	}
	return -EINVAL;
}

/* asm bind processing - 2.6.2.3 (appnarch.boo)
 */
int sna_asm_rx(struct sna_asm_cb *as, struct sna_lfsid *lf, struct sk_buff *skb)
{
	int err;

	sna_debug(5, "init\n");
	err = sna_asm_rx_mu_chk_err(skb);
	if (err < 0) {
		/* need to send proper bind rsp. */
		sna_debug(5, "mu_chk_err\n");
		goto toss;
	}

	/* route the mu, we need to handle bind pacing. */
	switch (err) {
		case SNA_RU_RC_BIND:
			if (sna_transport_header(skb)->rri) {
				err = sna_sm_proc_bind_rsp(lf, skb);
			} else {
				err = sna_sm_proc_bind_req(lf, skb);
			}
			goto out;
		case SNA_RU_RC_UNBIND:
			if (sna_transport_header(skb)->rri) {
				err = 0;	/* discard unbind/rsp */
			} else {
				err = sna_sm_proc_unbind_req(lf, skb);
			}
			goto out;
		default:
			sna_debug(5, "tossing bind\n");
			err = -EINVAL;
			goto toss;
	}
toss:	kfree_skb(skb);
out:	return err;
}

/* sm exclusive user MU(UN/BIND_RQ/RSP_SEND).
 * asm-pc boundary - 2.6.4.1.3 (appnarch.boo)
 */
int sna_asm_tx_bind(struct sna_lfsid *lf, u_int32_t pc_index, struct sk_buff *skb)
{
	struct sna_pc_cb *pc;

	sna_debug(5, "init\n");
	pc = sna_pc_get_by_index(pc_index);
	if (!pc)
		return -ENOENT;
	sna_pc_tx_mu(pc, skb, NULL);
	return 0;
}

int sna_asm_create(struct sna_nof_node *node)
{
	sna_debug(5, "init\n");
	return 0;
}

int sna_asm_destroy(struct sna_nof_node *delete)
{
	sna_debug(5, "init\n");
	return 0;
}

#ifdef CONFIG_PROC_FS
int sna_asm_get_info(struct seq_file *m, void *v)
{
	struct sna_asm_cb *a;
	struct list_head *le;

	seq_printf(m, "%-9s%-8s%-6s%-5s%-6s%-9s%-8s%-9s\n",
		"pc_id", "max_btu", "intra", "odai", "godai",
		"dep_lulu", "bpacing", "dbpacing");

	list_for_each(le, &asm_list) {
		a = list_entry(le, struct sna_asm_cb, list);
		seq_printf(m, "%-9d%-8d%-6d%-5d%-6d%-9d%-8d%-9d\n",
			a->index, a->rx_max_btu, a->intranode,
			a->odai, a->gen_odai_usage_opt_set, a->dlus_lu_reg,
			a->tx_adaptive_bind_pacing, a->adptv_bind_pacing);
	}

	return 0;
}

int sna_asm_get_active_lfsids(struct seq_file *m, void *v)
{
	struct sna_lfsid_block *l;
	struct list_head *le, *se;
	struct sna_asm_cb *a;
	int i;

	/* output the asm data for the /proc filesystem. */
	seq_printf(m, "%-9s%-6s%-6s%-5s%-6s%-5s\n",
		"pc_id", "intra", "sm_id", "odai", "sidh", "sidl");
	list_for_each(le, &asm_list) {
		a = list_entry(le, struct sna_asm_cb, list);
		list_for_each(se, &a->blk_list) {
			l = list_entry(se, struct sna_lfsid_block, list);
			for (i = 0; i < 256; i++) {
				if (l->l[i].active) {
					seq_printf(m,
						"%-9d%-6d%-6d%-5d%-6d%-5d\n",
						a->index,
						a->intranode,
						l->l[i].sm_index,
						l->l[i].odai,
						l->l[i].sidh,
						l->l[i].sidl);
				}
			}
		}
	}

	return 0;
}
#endif
