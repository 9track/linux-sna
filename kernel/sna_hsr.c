/* sna_hsr.c: Linux Systems Network Architecture implementation
 * - SNA LU 6.2 Half Session Router (HSR)
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
#include <linux/sna.h>

static LIST_HEAD(hs_list);
static u_int32_t sna_hs_system_index = 0;

struct sna_hs_cb *sna_hs_get_by_lfsid(struct sna_lfsid *lf)
{
	struct sna_hs_cb *hs;
	struct list_head *le;

	sna_debug(5, "init\n");
	list_for_each(le, &hs_list) {
		hs = list_entry(le, struct sna_hs_cb, list);
		if (hs->lfsid.odai == lf->odai
			&& hs->lfsid.sidh == lf->sidh
			&& hs->lfsid.sidl == lf->sidl)
			return hs;
	}
	return NULL;
}

struct sna_hs_cb *sna_hs_get_by_index(u_int32_t index)
{
	struct sna_hs_cb *hs;
	struct list_head *le;

	sna_debug(5, "init\n");
	list_for_each(le, &hs_list) {
		hs = list_entry(le, struct sna_hs_cb, list);
		if (hs->index == index)
			return hs;
	}
	return NULL;
}

static u_int32_t sna_hs_new_index(void)
{
	for (;;) {
		if (++sna_hs_system_index <= 0)
			sna_hs_system_index = 1;
		if (sna_hs_get_by_index(sna_hs_system_index) == NULL)
			return sna_hs_system_index;
	}
	return 0;
}

int sna_hs_init_finish(u_int32_t index, u_int32_t pc_index,
	struct sna_lfsid *lf, struct sk_buff *skb)
{
	struct sna_hs_cb *hs;

	sna_debug(5, "init\n");
	hs = sna_hs_get_by_index(index);
	if (!hs)
		return -ENOENT;
	if (hs->rx_bind)
		kfree_skb(hs->rx_bind);
	hs->rx_bind = skb_copy(skb, GFP_ATOMIC);
	if (!hs->rx_bind)
		return -ENOMEM;
	hs->pc_index = pc_index;
	memcpy(&hs->lfsid, lf, sizeof(struct sna_lfsid));
	sna_tc_init(hs);
	sna_dfc_init(hs);
	return 0;
}

/* create an hs_cb, init basic parameters and most importantly get an hs_index.
 *
 * @lulu: lulu control block.
 * @err: returned error code.
 */
u_int32_t sna_hs_init(struct sna_lulu_cb *lulu, int *err)
{
	struct sna_hs_cb *hs;
	u_int32_t hs_index = 0;

	sna_debug(5, "init\n");
	new(hs, GFP_ATOMIC);
	if (!hs) {
		*err = -ENOMEM;
		goto out;
	}
	*err = 0;
	hs->index = hs_index	= sna_hs_new_index();
	hs->lulu_index		= lulu->index;
	hs->type		= lulu->type;
	list_add(&hs->list, &hs_list);
out:	return hs_index;
}

int sna_hs_destroy(u_int32_t index)
{
	struct sna_hs_cb *hs;

	sna_debug(5, "init\n");
	hs = sna_hs_get_by_index(index);
	if (!hs)
		return 0;
	list_del(&hs->list);
	kfree(hs);
	return 0;
}

int sna_hs_ps_connected(struct sna_hs_cb *hs, u_int32_t bracket_index, u_int32_t tp_index)
{
	sna_debug(5, "init\n");
	/* sna_dfc_hs_ps_connected(hs, bracket_index, tp_index);
	 */
	return 0;
}

int sna_hs_tx_ps_mu_data(struct sna_rcb *rcb, struct sk_buff *skb)
{
	int err;

	sna_debug(5, "init\n");
	err = sna_dfc_send_from_ps_data(skb, rcb);
	if (err < 0)
		sna_debug(5, "dfc send from ps failed `%d'.\n", err);
	return err;
}

int sna_hs_tx_ps_mu_req(struct sna_rcb *rcb, struct sk_buff *skb)
{
	int err;

	sna_debug(5, "init\n");
	err = sna_dfc_send_from_ps_req(skb, rcb);
	if (err < 0)
		sna_debug(5, "dfc send from ps failed `%d'.\n", err);
	return err;
}

int sna_hs_rx(struct sna_lfsid *lf, struct sk_buff *skb)
{
	int err;

	sna_debug(5, "init\n");
	err = sna_tc_rx(lf, skb);
	if (err < 0) {
		/* this should cause the session to be deactivated. */
		sna_debug(5, "sna_tx_rx failed `%d'.\n", err);
	}
	return err;
}

/* Does processing for the half-session (FM profile 19). Message units
 * received from RM and PS are routed to DFC. Message units received from
 * PC are routed to TC. THe half-session continues to operate until and error
 * condition occurs or the half session process is destroyed. If an error
 * condition occurs, local->sense is set (by DFC or TC) with the sense
 * data indicating what kind of error occured. When this field is set, the
 * half-session sends an ABORT message to SM. This causes SM to send an
 * UNBIND(protocol error) for this session. HS recevies BUFFERS_RESERVED
 * signals from buffer manager and builds and sends the appropriate pacing
 * response.
 *
 * sna_process_lu_lu_session();
 */
int sna_hs_process_lu_lu_session(int who, struct sk_buff *skb,
	struct sna_rcb *rcb)
{
	sna_debug(5, "init\n");
#ifdef NOT
	switch (who) {
		case SNA_PS:
			sna_dfc_send_from_ps(skb, rcb);
			break;

		case SNA_RM:
			sna_dfc_send_from_rm(skb);
			break;

		case SNA_PC:
			sna_tc_rcv(skb);
			break;
		default:
			return -1;
	}
#endif
	return 0;
}
