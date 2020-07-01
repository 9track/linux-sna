/* sna_sm.c: Linux Systems Network Architecture implementation
 * - SNA LU 6.2 Session Manager (SM)
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
#include <linux/time.h>
#include <linux/list.h>
#include <linux/sna.h>

#ifdef CONFIG_SNA_LLC
#include <net/llc_if.h>
#include <net/llc_sap.h>
#include <net/llc_pdu.h>
#include <net/llc_conn.h>
#include <linux/llc.h>
#endif  /* CONFIG_SNA_LLC */

#include "sna_common.h"

static LIST_HEAD(lulu_list);
static u_int32_t sna_lulu_system_index = 0;

struct sna_lulu_cb *sna_sm_lulu_get_by_index(u_int32_t index)
{
	struct sna_lulu_cb *lulu;
	struct list_head *le;

	sna_debug(5, "init\n");
	list_for_each(le, &lulu_list) {
		lulu = list_entry(le, struct sna_lulu_cb, list);
		if (lulu->index == index)
			return lulu;
	}
	return NULL;
}

static u_int32_t sna_sm_lulu_new_index(void)
{
	for (;;) {
		if (++sna_lulu_system_index <= 0)
			sna_lulu_system_index = 1;
		if (sna_sm_lulu_get_by_index(sna_lulu_system_index) == NULL)
			return sna_lulu_system_index;
	}
	return 0;
}

struct sna_lulu_cb *sna_sm_lulu_get_by_fqpcid(sna_fqpcid *fqpcid)
{
	struct sna_lulu_cb *lulu;
	struct list_head *le;

	sna_debug(5, "init\n");
	list_for_each(le, &lulu_list) {
		lulu = list_entry(le, struct sna_lulu_cb, list);
		if (!memcmp(lulu->fqpcid, fqpcid, sizeof(sna_fqpcid)))
			return lulu;
	}
	return NULL;
}

#ifdef SNA_UNUSED
static int sna_sm_build_bind_cv(__u8 type, __u8 *cv_start)
{
	int len = 0;
	__u8 s, l;
	unsigned char name[17];

	sna_debug(5, "init\n");
	switch (type) {
		case CV_ROUTE_SEL:
			s = CV_ROUTE_SEL;
			memcpy(cv_start + len, &s, sizeof(__u8));
			len += 1;
			l = 19;
			memcpy(cv_start + len, &l, sizeof(__u8));
			len += 1;
			s = 0x01;
			memcpy(cv_start + len, &s, sizeof(__u8));
			len += 1;
			s = 0x01;
			memcpy(cv_start + len, &s, sizeof(__u8));
			len += 1;
			l = 17;
			memcpy(cv_start + len, &l, sizeof(__u8));
			len += 1;
			s = 0x46;
			memcpy(cv_start + len, &s, sizeof(__u8));
			len += 1;
			l = 15;
			memcpy(cv_start + len, &l, sizeof(__u8));
			len += 1;
			s = 0x80;
			memcpy(cv_start + len, &s, sizeof(__u8));
			len += 1;
			s = 0x00;	/* TG Number */
			memcpy(cv_start + len, &s, sizeof(__u8));
			len += 1;
			l = 10;
			memcpy(cv_start + len, &l, sizeof(__u8));
			len += 1;
			atoe_strncpy(name, "LNXSNA.IBM", 10);
			memcpy(cv_start + len, name, 10);
			len += 10;
			s = 0x00;
			memcpy(cv_start + len, &s, sizeof(__u8));
			len += 1;
			break;
	}

	return len;
}
#endif

/* Build +RSP(BIND). */
#ifdef CONFIG_SNA_LLC
#ifdef OLD_LLC
static int sna_sm_build_bind_rsp_pos(struct sk_buff *skb, int t)
{
	sna_debug(5, "sna_sm_build_bind_rsp\n");

	struct ethhdr *eth_hdr = skb->mac.ethernet;
	struct net_device *dev = skb->dev;
	struct sna_rcb *rcb = NULL;
	struct sk_buff *newskb;
	struct sna_bind *bind;
	int size, len = 0;

	/* Device + Datalink + XID */
	size = dev->hard_header_len + LLC_TYPE2_SIZE + 250;
	newskb = alloc_skb(size, GFP_ATOMIC);

	skb_reserve(newskb, 200);
	skb_reserve(newskb, sizeof(struct snarhdr));
	skb_reserve(newskb, sizeof(struct sna_fid2));

	skb_reserve(newskb, LLC_TYPE2_SIZE);
	skb_reserve(newskb, dev->hard_header_len);
	newskb->dev = dev;

	if (t == BIND_S_BIND || t == BIND_S_PR_BIND
		|| t == BIND_S_PR_BIND_W_AF) {
		if (t == BIND_S_BIND_W_AF)
			bind = (struct sna_bind *)skb_push(newskb,
				sizeof(struct sna_bind) + 61 + 45 + 21);
		else
			bind = (struct sna_bind *)skb_push(newskb,
				sizeof(struct sna_bind) + 61 + 45);

		memset(bind, 0, sizeof(struct sna_bind));
		bind->request_code	= BIND_RQ;
		bind->fm_profile	= FMH_19;
		bind->ts_profile	= TSH_7;
		bind->pri_flags		= 0xB0;
		bind->sec_flags		= 0xB0;
		bind->cm1_flags		= 0x50;
		bind->cm2_flags		= 0xB3;
		if (t == BIND_S_PR_BIND) {
			bind->sec_tx_win_size	= 0;
			bind->sec_rx_win_size	= 0x80;
		} else {
//			if (t == BIND_S_PR_BIND_W_AF) {
//				bind->sec_tx_win_size   = 0x01;
 //                       	bind->sec_rx_win_size   = 0x81;
//			} else {
				bind->sec_tx_win_size 	= 0x07;
				bind->sec_rx_win_size	= 0x87;
//			}
		}

		if (t == BIND_S_PR_BIND_W_AF) {
			bind->shs_max_ru_size   = 0x86;
			bind->phs_max_ru_size   = 0x86;
		} else {
			bind->shs_max_ru_size	= 0x87;
			bind->phs_max_ru_size	= 0x87;
		}

		if (t == BIND_S_PR_BIND) {
			bind->pri_tx_win_size	= 0x80;
			bind->pri_rx_win_size	= 0;
		} else {
//			if (t == BIND_S_PR_BIND_W_AF) {
//				bind->pri_tx_win_size   = 0x81;
  //                              bind->pri_rx_win_size   = 0x01;
//			} else {
				bind->pri_tx_win_size	= 0x87;
				bind->pri_rx_win_size	= 0x07;
//			}
		}
		bind->lu_type		= 0x6;
		bind->lu6_level		= 0x02;
		bind->ps1_flags		= 0x16;
		bind->ps2_flags		= 0x23;

		len += sna_sm_build_bind_user_data(&bind->raw);

		if (t == BIND_S_PR_BIND_W_AF)
			len += sna_sm_build_bind_cv(CV_ROUTE_SEL,
				&bind->raw + len); /* +20 */
		len += sna_sm_build_bind_cv(CV_COS_TPF, &bind->raw + len); /* +11 */
		len += sna_sm_build_bind_cv(CV_FQ_PCID, &bind->raw + len); /* +21 */
		sna_dfc_init_th_rh(newskb, rcb);

		if (t == BIND_S_PR_BIND) {
			newskb->h.raw[0] = 0xEB;
			newskb->h.raw[1] = 0x80;
			newskb->h.raw[2] = 0x00;
		}

		if (t == BIND_S_PR_BIND_W_AF) {
			newskb->h.raw[0] = 0xEB;
			newskb->h.raw[1] = 0x80;
			newskb->h.raw[2] = 0x00;
			newskb->nh.raw[2] = 0x02;
			newskb->nh.raw[3] = 0x01;
		}
	} else {
		skb_push(newskb, 3);
		newskb->data[0] = 0;
		newskb->data[1] = 0;
		newskb->data[2] = 1;

		sna_dfc_init_th_rh(newskb, rcb);

		newskb->h.raw[0] = 0x83;
		newskb->h.raw[1] = 0x01;
		newskb->h.raw[2] = 0x00;
		newskb->nh.raw[0] = 0x2D;
		newskb->nh.raw[1] = 0x00;
		if (t == BIND_S_PR_FMD_W_AF) {
			newskb->nh.raw[2] = 0x00; // 0x02;
			newskb->nh.raw[3] = 0x01;
		} else {
			newskb->nh.raw[2] = 0x01;
			newskb->nh.raw[3] = 0x00;
		}
		newskb->nh.raw[4] = 0x00;
		newskb->nh.raw[5] = 0x00;
	}

	llc_data(0x04, 0x04, eth_hdr->h_source, newskb, dev);

	return 0;
}
#endif	/* OLD_LLC */
#endif	/* CONFIG_SNA_LLC */

static int sna_sm_bind_rsp_state_err(struct sna_lulu_cb *lulu, struct sk_buff *skb)
{
	sna_debug(5, "init\n");
	return 0;
}

#ifdef SNA_UNUSED
static int sna_sm_bind_req_state_err(struct sna_lulu_cb *lulu, struct sk_buff *skb)
{
	sna_debug(5, "init\n");
	return 0;
}
#endif

/* Check if the received RSP(BIND) correlates with a previously sent BIND. */
static struct sna_lulu_cb *sna_sm_correlate_bind_rsp(struct sna_lfsid *lf, struct sk_buff *skb)
{
	struct sna_lulu_cb *lulu;
/*	sna_fqpcid *fqpcid;*/

	sna_debug(5, "init\n");
	lulu = sna_sm_lulu_get_by_index(lf->sm_index);
	if (!lulu)
		return NULL;
	/* validate bind by extracting the fqpcid control vector,
	 * this function should really be.
	 *
	 * fqpcid = sna_sm_bind_extract_fqpcid(skb);
	 * if (!fqpcid)
	 * 	return NULL;
	 * lulu = sna_sm_lulu_get_by_fqpcid(fqpcid);
	 * if (!lulu)
	 * 	return NULL;
	 * if (lf->sm_index != lulu->index)
	 * 	return NULL;
	 */
	return lulu;
}

/* Check if the received RSP(UNBIND) correlates with a known session. */
#ifdef SNA_UNUSED
static struct sna_lulu_cb *sna_sm_correlate_unbind_req(struct sna_lfsid *lf, struct sk_buff *skb)
{
	struct sna_lulu_cb *lulu;
/*	sna_fqpcid *fqpcid;*/

	sna_debug(5, "init\n");
	lulu = sna_sm_lulu_get_by_index(lf->sm_index);
	if (!lulu)
		return NULL;
	/* validate bind by extracting the fqpcid control vector.
	 * fqpcid = sna_sm_bind_extract_fqpcid(skb);
	 * if (!fqpcid)
	 *      return NULL;
	 * lulu = sna_sm_lulu_get_by_fqpcid(fqpcid);
	 * if (!lulu)
	 *      return NULL;
	 * if (!lf->sm_index != lulu->index)
	 * 	return NULL;
	 */
	return lulu;
}
#endif

/* Check BIND for semantic and state errors, create a half-session process,
 * reserve required buffers. If no errors occur, build and send a +RSP(BIND),
 * update and save active session parameters, and initialize the half-session.
 */
int sna_sm_proc_bind_req(struct sna_lfsid *lf, struct sk_buff *skb)
{
	sna_debug(5, "init\n");
#ifdef NOT
	if (bstate == BIND_RESET) {
		sna_sm_build_bind_rsp_pos(skb, BIND_S_BIND);
		bstate = BIND_S_BIND;
		kfree_skb(skb);
		return 0;
	}

	if (bstate == BIND_S_BIND) {
		/* Send +RSP BIND */
		sna_sm_build_bind_rsp_pos(skb, BIND_S_PR_BIND);
		bstate = BIND_S_PR_BIND;
		kfree_skb(skb);
		return 0;
	}

	if (bstate == BIND_S_PR_BIND) {
		/* Send +RSP FMD BIND */
		sna_sm_build_bind_rsp_pos(skb, BIND_S_PR_FMD);
		bstate = BIND_S_PR_FMD;
		kfree_skb(skb);
		return 0;
	}

	if (bstate == BIND_S_PR_FMD) {
		/* Send BIND w/ real oaf/daf */
		sna_sm_build_bind_rsp_pos(skb, BIND_S_PR_BIND_W_AF);
		bstate = BIND_S_PR_BIND_W_AF;
		sna_sm_build_bind_rsp_pos(skb, BIND_S_PR_FMD_W_AF);
		bstate = BIND_S_PR_FMD_W_AF;
		kfree_skb(skb);
		return 0;
	}

	if (bstate == BIND_S_PR_BIND_W_AF) {
		sna_sm_build_bind_rsp_pos(skb, BIND_S_PR_FMD_W_AF);
		bstate = BIND_S_PR_FMD_W_AF;
		return 0;
	}

	if (bstate == BIND_S_PR_BIND_W_AF) {
		sna_sm_build_bind_rsp_pos(skb, BIND_S_PR_FMD_W_AF);
		bstate = BIND_S_PR_FMD_W_AF;
		return 0;
	}
	kfree_skb(skb);

	struct sna_local *local;
	struct sna_mu *mu_new;
	struct sna_lu_lu_cb *lulu_cb;
	struct sna_partner_lu *plu;

	err = sna_check_semantic(mu);
	if (err < 0) {
		local->sense = err;
		return -1;
	}

	err = sna_bind_rq_state_error(mu);
	if (err < 0) {
		local->sense = err;
		return (-1);
	}

	plu->active_session_params.parallel = bind_rq_rcv->parallel;
	new(lulu_cb, GFP_ATOMIC);
	if (!lulu_cb)
		return -ENOMEM;
	sna_init_lulu_cb_bind(mu, lulu_cb);
	lulu_cb->hs_id = unique_hs_id();
	sna_build_bind_rsp_pos(mu, lulu_cb, mu_new);
	err = sna_reserve_constant_buffers(lulu_cb);
	if (!err)
		err = sna_reserver_variable_buffers(lulu_cb, bind_rq_rcv);
	if (!err) {
		lulu_cb->session_id = sna_get_session_id();
		sna_build_and_send_init_hs(lulu_cb, bind_image);
	}

	if (!err) {
		send_to_asm(mu_new);
		fsm_status(mu_new, lulu_cb);
	} else {
		if (mu->bind_ru.fqpcid != NULL)
			sna_build_and_send_unbind_rq(mu,CLEANUP,local->sense);
		else {
			if (buffer_err == 0)
				bm(FREE, mu_new);
			sna_build_and_send_bind_rsp_neg(mu);
		}

		if (lulu_cb != NULL)
			sna_cleanup_lu_lu_session(lulu_cb);
	}
#endif
	return 0;
}

/* Check if the received RSP(BIND) correlator with the previously sent BIND. If
 * it does, delete pending random data used in LU-LU verification for the
 * session (if present) and after additional processing (in case of a positive
 * response) call the FSM. If it does not correlate, the RSP(BIND) is
 * considered to be a stray one and is ignored (no action taken).
 */
int sna_sm_proc_bind_rsp(struct sna_lfsid *lf, struct sk_buff *skb)
{
	struct sna_remote_lu_cb *remote_lu;
	struct sna_lulu_cb *lulu;
	int err = -EINVAL;

	sna_debug(5, "init\n");
	lulu = sna_sm_correlate_bind_rsp(lf, skb);
	if (!lulu) {
		sna_debug(5, "unable to correlate bind rsp\n");
		kfree_skb(skb);
		goto out;
	}
	if (sna_transport_header(skb)->rti == SNA_RH_RTI_NEG) {
		sna_debug(5, "negative bind response\n");
		lulu->input = SNA_SM_FSM_INPUT_NEG_BIND_RSP;
		goto done;
	}
	err = sna_sm_bind_rsp_state_err(lulu, skb);
	if (err < 0) {
		sna_debug(5, "state error found\n");
		lulu->input = SNA_SM_FSM_INPUT_POS_BIND_RSP_NG;
		goto done;
	}
	remote_lu = sna_rm_remote_lu_get_by_index(lulu->remote_lu_index);
	if (!remote_lu) {
		sna_debug(5, "lost remote lu\n");
		lulu->input = SNA_SM_FSM_INPUT_POS_BIND_RSP_NG;
		goto done;
	}
	err = sna_hs_init_finish(lulu->hs_index, lulu->pc_index, lf, skb);
	if (err < 0) {
		sna_debug(5, "unable to save bind image\n");
		lulu->input = SNA_SM_FSM_INPUT_POS_BIND_RSP_NG;
		goto done;
	}
	remote_lu->sessions++;
	lulu->input = SNA_SM_FSM_INPUT_POS_BIND_RSP_OK;
done:	err = sna_sm_fsm_status(lulu);
	if (err < 0)
		sna_debug(5, "fsm_status error `%d'.\n", err);
out:    kfree_skb(skb);
	return err;
}

/* Process a received UNBIND. SM always receives the entire UNBIND MU, since
 * the PIU is not longer than 99 bytes and thus no reassembly by the ASM in
 * needed. If a received UNBIND correlates to one of the active or pending
 * active sessions, the FSM is called to clean up the session.
 */
int sna_sm_proc_unbind_req(struct sna_lfsid *lf, struct sk_buff *skb)
{
	sna_debug(5, "init\n");
#ifdef NOT
	struct sna_lu_lu_cb *lulu_cb;

	correlate = sna_correlator_unbind_rq(mu);
	sna_build_and_send_unbind_rsp(mu);
	if (correlate)
		fsm_status(mu, lulu_cb);
#endif
	return 0;
}

#ifdef SNA_UNUSED
static u_int32_t sna_sm_bind_ru_unfold(u_int8_t a, u_int8_t b)
{
	u_int32_t i, c;
	for (i = 1, c = 2; i < b; i++)
		c = c * 2;
	return a * c;
}
#endif

static u_int8_t sna_sm_bind_ru_fold(u_int32_t n)
{
	u_int8_t b;
	if (n < 8)
		return 0x80;
	if (n >= 491520)
		return 0xFF;
	for (b = 0; n > 15; b++)
		n = n / 2;
	return (((u_int8_t)n) << 4) | (((u_int8_t)b));
}

static int sna_sm_user_data_nonce(struct sk_buff *skb)
{
	u_int8_t n_data[8];
	u_int8_t *len, *key, *rsv;
	struct timeval t;

	sna_debug(5, "init\n");
	len = (u_int8_t *)skb_put(skb, 1);
	key = (u_int8_t *)skb_put(skb, 1);
	rsv = (u_int8_t *)skb_put(skb, 1);
	do_gettimeofday(&t);
	memcpy(n_data, &t, 8);
	fatoe_strncpy(n_data, n_data, 8);
	memcpy(skb_put(skb, 8), n_data, 8);
	*key = SNA_USER_DATA_NONCE;
	*len = 2 + 8;
	return *len + 1;
}

static int sna_sm_user_data_network_name(sna_netid *name, int slu, struct sk_buff *skb)
{
	u_int8_t name_len, name_flat[17];
	u_int8_t *len, *key;

	sna_debug(5, "init\n");
	len = (u_int8_t *)skb_put(skb, 1);
	key = (u_int8_t *)skb_put(skb, 1);
	name_len = sna_netid_to_char(name, name_flat);
	fatoe_strncpy(name_flat, name_flat, name_len);
	memcpy(skb_put(skb, name_len), name_flat, name_len);
	if (slu)
		*key = SNA_USER_DATA_SLU_NETWORK_NAME;
	else
		*key = SNA_USER_DATA_PLU_NETWORK_NAME;
	*len = name_len + 1;
	return *len + 1;
}

static int sna_sm_user_data_session_instance(struct sna_lulu_cb *lulu,
	struct sk_buff *skb)
{
	u_int8_t *len, *key, *format;
	u_int8_t data_len = 4;
	u_int32_t data;

	sna_debug(5, "init\n");
	len 	= (u_int8_t *)skb_put(skb, 1);
	key 	= (u_int8_t *)skb_put(skb, 1);
	format	= (u_int8_t *)skb_put(skb, 1);
	data = htonl(lulu->hs_index);
	atoe_strncpy((u_int8_t *)&data, (u_int8_t *)&data, data_len);
	memcpy(skb_put(skb, data_len), &data, data_len);
	*format	= 0xF0;
	*key 	= SNA_USER_DATA_SESSION_INSTANCE;
	*len 	= data_len + 2;
	return *len + 1;
}

static int sna_sm_user_data_mode(u_int8_t *name, struct sk_buff *skb)
{
	u_int8_t e_name[SNA_RESOURCE_NAME_LEN];
	u_int8_t *len, *key;

	sna_debug(5, "init\n");
	len = (u_int8_t *)skb_put(skb, 1);
	key = (u_int8_t *)skb_put(skb, 1);
	atoe_strncpy(e_name, name, strlen(name));
	memcpy(skb_put(skb, strlen(name)), e_name, strlen(name));
	*key = SNA_USER_DATA_MODE;
	*len = strlen(name) + 1;
	return *len + 1;
}

static int sna_sm_bind_pkt_append_user_data(struct sna_lulu_cb *lulu,
	struct sk_buff *skb)
{
	u_int8_t *user_len, *user_key;
	struct sna_mode_cb *mode;
	u_int8_t len = 0;

	sna_debug(5, "init\n");
	mode = sna_rm_mode_get_by_index(lulu->mode_index);
	if (!mode)
		return -ENOENT;
	user_len = (u_int8_t *)skb_put(skb, 1);
	user_key = (u_int8_t *)skb_put(skb, 1);
	len += sna_sm_user_data_mode(mode->mode_name, skb);
	len += sna_sm_user_data_session_instance(lulu, skb);
	len += sna_sm_user_data_network_name(&lulu->local_name, 0, skb);
	len += sna_sm_user_data_nonce(skb);
	*user_key = 0;
	*user_len = len + 1;
	return len + 2;
}

static int sna_sm_bind_pkt_append_name(sna_netid *name, struct sk_buff *skb)
{
	u_int8_t *name_len, name_flat[17];
	int len;

	sna_debug(5, "init\n");
	len = sna_netid_to_char(name, name_flat);
	fatoe_strncpy(name_flat, name_flat, len);
	name_len  = (u_int8_t *)skb_put(skb, sizeof(u_int8_t));
	*name_len = len;
	memcpy(skb_put(skb, len), name_flat, len);
	return len;
}

static int sna_sm_bind_pkt_append_urc(struct sk_buff *skb)
{
	u_int8_t *urc;
	int len = 1;

	sna_debug(5, "init\n");
	urc = (u_int8_t *)skb_put(skb, 1);
	*urc = 0;
	return len;
}

static int sna_sm_bind_pkt_init(struct sna_lulu_cb *lulu, struct sk_buff *skb)
{
	struct sna_mode_cb *mode;
	sna_ru_bind *bind;
	sna_fid2 *fid2;
	sna_rh *rh;
	u_int16_t len;

	sna_debug(5, "init\n");
	mode = sna_rm_mode_get_by_index(lulu->mode_index);
	if (!mode)
		return -ENOENT;

	/* set transmission header. */
	len = sizeof(sna_fid2);
	fid2 = (sna_fid2 *)skb_put(skb, sizeof(sna_fid2));
	memset(fid2, 0, sizeof(sna_fid2));
	fid2->format	= SNA_TH_FID2;
	fid2->mpf	= SNA_TH_MPF_WHOLE_BIU;
	fid2->efi	= SNA_TH_EFI_EXP;
	fid2->odai	= lulu->lfsid.odai;
	fid2->daf	= lulu->lfsid.sidl;
	fid2->oaf	= lulu->lfsid.sidh;
	fid2->snf	= 0; // htons(SET_SNF_SENDER(1) | SET_SNF_REQUEST(2));

	/* set request header. */
	len += sizeof(sna_rh);
	rh = (sna_rh *)skb_put(skb, sizeof(sna_rh));
	memset(rh, 0, sizeof(sna_rh));
	rh->rri		= SNA_RH_RRI_REQ;
	rh->ru		= SNA_RH_RU_SC;
	rh->fi		= SNA_RH_FI_FMH;
	rh->sdi		= SNA_RH_SDI_NO_SD;
	rh->bci		= SNA_RH_BCI_BC;
	rh->eci		= SNA_RH_ECI_EC;
	rh->dr1i	= SNA_RH_DR1I_DR1;
	rh->llci	= SNA_RH_LLCI_NO_LLC;
	rh->dr2i	= SNA_RH_DR2I_NO_DR2;
	rh->rti		= SNA_RH_RTI_POS;
	rh->rlwi	= SNA_RH_RLWI_NO_RLW;
	rh->qri		= SNA_RH_QRI_NO_QR;
	rh->pi		= SNA_RH_PI_NO_PAC;	// SNA_RH_PI_PAC;
	rh->bbi		= SNA_RH_BBI_NO_BB;
	rh->ebi		= SNA_RH_EBI_NO_EB;
	rh->cdi		= SNA_RH_CDI_NO_CD;
	rh->csi		= SNA_RH_CSI_CODE0;
	rh->edi		= SNA_RH_EDI_NO_ED;
	rh->pdi		= SNA_RH_PDI_NO_PD;
	rh->cebi	= SNA_RH_CEBI_NO_CEB;

	/* set bind header. */
	len += sizeof(sna_ru_bind);
	bind = (sna_ru_bind *)skb_put(skb, sizeof(sna_ru_bind));
	memset(bind, 0, sizeof(sna_ru_bind));
	bind->rc		= SNA_RU_RC_BIND;
	bind->format		= 0;
	bind->type		= SNA_BIND_TYPE_NEG;
	bind->fm_profile	= SNA_FM_PROFILE_19;
	bind->ts_profile	= SNA_TS_PROFILE_7;

	/* primary flags. */
	bind->p_chain_use	= SNA_CHAIN_USE_MULTI;
	bind->p_req_mode	= SNA_REQ_MODE_IMMEDIATE;
	bind->p_chain_rsp	= SNA_CHAIN_RSP_BOTH;
	bind->p_compression	= 0;
	bind->p_tx_end_bracket	= SNA_TX_END_BRACKET_NO;

	/* secondary flags. */
	bind->s_chain_use	= SNA_CHAIN_USE_MULTI;
	bind->s_req_mode	= SNA_REQ_MODE_IMMEDIATE;
	bind->s_chain_rsp	= SNA_CHAIN_RSP_BOTH;
	bind->s_compression	= 0;
	bind->s_tx_end_bracket	= SNA_TX_END_BRACKET_NO;

	/* common flags. */
	bind->whole_biu		= SNA_WHOLE_BIU_SEG;		/* non-static. */
	bind->fm_header		= SNA_FM_HEADER_ALLOWED;
	bind->brackets		= 0;
	bind->bracket_term	= 1;
	bind->alt_code_set	= 0;				/* non-static. */
	bind->bind_queue	= 0;				/* non-static. */

	bind->flow_mode		= SNA_FLOW_MODE_HALF_DUPLEX_FF;	/* non-static. */
	bind->recovery		= 1;
	bind->contention	= 0; // 1;				/* non-static. */
	bind->alt_code_proc_id	= 0; // 1;
	bind->ctrl_vectors	= 0; // 1;				/* non-static. */
	bind->hdx_ff_reset	= 1;

	/* ts usage. */
	bind->sec_stagi		= 0;				/* non-static. */
	bind->sec_tx_win_size	= mode->tx_pacing;
	bind->adaptive_pacing	= 1;				/* non-static. */
	bind->sec_rx_win_size	= mode->rx_pacing;
	bind->sec_max_ru_size	= sna_sm_bind_ru_fold(mode->rx_max_ru);
	bind->pri_max_ru_size	= sna_sm_bind_ru_fold(mode->tx_max_ru);
	bind->pri_stagi		= 1; // 0;			/* non-static. */
	bind->pri_tx_win_size	= mode->tx_pacing;
	bind->pri_rx_win_size	= mode->rx_pacing;
	bind->ps_usage		= 0;
	bind->lu_type		= 6;
	bind->lu6_level		= 2;

	/* ps flags 1. */
	bind->xt_security	= 0;				/* non-static. */
	bind->xt_security_sense = 0;				/* non-static. */
	bind->access_security	= 1;				/* non-static. */
	bind->lulu_verification = 0;				/* non-static. */
	bind->password_sub	= 1;				/* non-static. */
	bind->already_verified	= 1; // 0;				/* non-static. */
	bind->persist_verification = 0;				/* non-static. */

	/* ps flags 2. */
	bind->sync_level	= 1;				/* non-static. */
	bind->session_reinit	= 0;				/* non-static. */
	bind->parallel_session	= 0; // 1;				/* non-static. */
	bind->chg_num_sessions_gds = 0; // 1;				/* non-static. */
	bind->limited_resource	= 0;				/* non-static. */
	bind->lcc		= 0;				/* non-static. */

	/* crypto flags. */
	bind->crypto_supp	= 0;				/* non-static. */
	bind->crypto_len	= 0;				/* non-static. */

	/* setup local name. */
	len += sna_sm_bind_pkt_append_name(&lulu->local_name, skb);

	/* setup user data. */
	len += sna_sm_bind_pkt_append_user_data(lulu, skb);

	/* setup user request correlation data. */
	len += sna_sm_bind_pkt_append_urc(skb);

	/* setup remote name. */
	len += sna_sm_bind_pkt_append_name(&lulu->remote_name, skb);

	/* setup control vectors. */
	if (bind->ctrl_vectors) {
		len += sna_vector_cos_tpf(mode->cos_name, 1, 0, skb);
		len += sna_vector_fqpcid(&lulu->local_name, &lulu->fqpcid, skb);
	}
	return len;
}

/* Build and send a BIND. */
static int sna_sm_build_and_send_bind_req(struct sna_lulu_cb *lulu)
{
	struct sna_port_cb *port;
	struct sna_ls_cb *ls;
	struct sna_pc_cb *pc;
	struct sk_buff *skb;
	int err, size;

	sna_debug(5, "init\n");
	pc = sna_pc_get_by_index(lulu->pc_index);
	if (!pc)
		return -ENOENT;
	port = sna_cs_port_get_by_index(pc->port_index);
	if (!port)
		return -ENOENT;
	ls = sna_cs_ls_get_by_index(port, pc->ls_index);
	if (!ls)
		return -ENOENT;

	size = sna_dlc_data_min_len(ls) + sizeof(sna_rh) + 150;	/* static max for now. */
	skb = sna_alloc_skb(size, GFP_ATOMIC);
	if (!skb)
		return -ENOMEM;
	sna_dlc_data_reserve(ls, skb);
	sna_sm_bind_pkt_init(lulu, skb);
	err = sna_asm_tx_bind(&lulu->lfsid, lulu->pc_index, skb);
	if (err < 0) {
		sna_debug(5, "tx_bind failed `%d'.\n", err);
		kfree_skb(skb);
	}
	return err;
}

/* Get the address (LFSID structure) for the session. Create a half-session
 * process. Reserve buffers for the session.
 *
 * @lulu: lulu control block.
 */
static int sna_sm_prepare_to_send_bind(struct sna_lulu_cb *lulu)
{
	struct sna_lfsid *lf;
	int err = -EINVAL;

	sna_debug(5, "init\n");
	lf = sna_asm_assign_lfsid(lulu->pc_index, lulu->index);
	if (!lf)
		goto out;
	memcpy(&lulu->lfsid, lf, sizeof(struct sna_lfsid));
	lulu->hs_index = sna_hs_init(lulu, &err);
	if (err < 0)
		sna_debug(5, "hs_init failed `%d'.\n", err);
out:	return err;
}

/* Process a receieved CINIT_SIGNAL record. First, this signal must be
 * correlated with a previously sent INIT_SIGNAL record. The correlation is
 * based on the value of FQPCID. If the correlation fails, the session has
 * already been brought down by the RM and a SESSEND_SIGNAL record is built
 * and sent to SS.
 *
 * @lulu_index: unique index to locate a lulu control block.
 * @pc_index: unique index to locate a path control instance.
 */
int sna_sm_proc_cinit_sig_rsp(u_int32_t lulu_index, u_int32_t pc_index)
{
	struct sna_remote_lu_cb *remote_lu;
	struct sna_lulu_cb *lulu;
	struct sna_mode_cb *mode;
	struct sna_pc_cb *pc;
	int err;

	sna_debug(5, "init\n");
	lulu = sna_sm_lulu_get_by_index(lulu_index);
	if (!lulu) {
		sna_debug(5, "init lulu_get_by_index enoent\n");
		err = -ENOENT;
		goto out;
	}
	lulu->pc_index = pc_index;
	remote_lu = sna_rm_remote_lu_get_by_index(lulu->remote_lu_index);
	if (!remote_lu) {
		lulu->input = SNA_SM_FSM_INPUT_CINIT_SIGNAL_NG;
		goto done;
	}
	mode = sna_rm_mode_get_by_index(lulu->mode_index);
	if (!mode) {
		lulu->input = SNA_SM_FSM_INPUT_CINIT_SIGNAL_NG;
		goto done;
	}
	if (sna_sm_lu_mode_session_limit(remote_lu, mode,
		lulu->type, SNA_SM_S_STATE_BIND_SENT)) {
		lulu->input = SNA_SM_FSM_INPUT_CINIT_SIGNAL_NG;
		goto done;
	}
	pc = sna_pc_get_by_index(pc_index);
	if (!pc) {
		lulu->input = SNA_SM_FSM_INPUT_CINIT_SIGNAL_NG;
		goto done;
	}
	if (pc->tx_max_btu < mode->tx_max_ru
		|| pc->rx_max_btu < mode->rx_max_ru) {
		lulu->input = SNA_SM_FSM_INPUT_CINIT_SIGNAL_NG;
		goto done;
	}
	/* need to check if there is already an active session
	 * to this remote_lu and if the remote_lu supports parallel
	 * sessions.
	 */
	err = sna_sm_prepare_to_send_bind(lulu);
	if (err < 0) {
		lulu->input = SNA_SM_FSM_INPUT_CINIT_SIGNAL_NG;
		goto done;
	}
	lulu->input = SNA_SM_FSM_INPUT_CINIT_SIGNAL_OK;
done:	err = sna_sm_fsm_status(lulu);
	if (err < 0)
		sna_debug(5, "fsm_status error `%d'.\n", err);
out:	return err;
}

/* Process a received INIT_SIGNAL_NEG_RSP record from the SS component of
 * the control point.
 *
 * @lulu_index: unique index to locate an lulu control block.
 */
int sna_sm_proc_init_sig_neg_rsp(u_int32_t lulu_index)
{
	struct sna_lulu_cb *lulu;
	int err;

	sna_debug(5, "init\n");
	lulu = sna_sm_lulu_get_by_index(lulu_index);
	if (!lulu)
		return -ENOENT;
	lulu->input = SNA_SM_FSM_INPUT_INIT_SIGNAL_NEG_RSP;
	err = sna_sm_fsm_status(lulu);
	if (err < 0)
		sna_debug(5, "fsm_status error `%d'.\n", err);
	return err;
}

/* Build and send an INIT_SIGNAL record to the control point.
 *
 * @lulu: lulu control block.
 */
static int sna_sm_build_and_send_init_sig(struct sna_lulu_cb *lulu)
{
	struct sna_init_signal init;
	struct sna_mode_cb *mode;

	sna_debug(5, "init\n");
	mode = sna_rm_mode_get_by_index(lulu->mode_index);
	if (!mode)
		return -ENOENT;
	memset(&init, 0, sizeof(init));
	init.index = lulu->index;
	memcpy(&init.local_name, &lulu->local_name, sizeof(sna_netid));
	memcpy(&init.remote_name, &lulu->remote_name, sizeof(sna_netid));
	memcpy(&init.mode_name, &mode->mode_name, SNA_RESOURCE_NAME_LEN);
	return sna_ss_proc_init_sig(&init);
}

/* Determine whether or not session limits associated with a given (LU,
 * mode name) pair are exceeded for the given state condition (FSM_STATUS for
 * this session).
 *
 * @remote_lu: remote lu control block.
 * @mode: mode control block session is using.
 * @type: first speaker (conwinner) or bidder (conloser).
 * @state: state of the activation (enum sna_sm_session_state).
 */
int sna_sm_lu_mode_session_limit(struct sna_remote_lu_cb *remote_lu,
	struct sna_mode_cb *mode, int type, int state)
{
	int bidder_limit, fsp_limit, total_limit;
	int bidder_cnt, fsp_cnt;
	int err = 0;

	sna_debug(5, "init\n");
	switch (state) {
		case SNA_SM_S_STATE_ACTIVE:
			bidder_cnt 	= mode->active.conlosers;
			fsp_cnt		= mode->active.conwinners;
			break;
		case SNA_SM_S_STATE_INIT_SENT:
		case SNA_SM_S_STATE_BIND_SENT:
			bidder_cnt	= mode->pending.conlosers;
			fsp_cnt		= mode->pending.conwinners;
			break;
		default:
			return -EINVAL;
	}
	total_limit     = mode->user_max.sessions;
	fsp_limit       = mode->user_max.conwinners;
	bidder_limit    = mode->user_max.conlosers;
	if (fsp_cnt + bidder_cnt > total_limit) {
		err = 1;	/* 0x08050000 */
		goto out;
	}
	if (total_limit - bidder_limit == fsp_cnt
		&& type == SNA_SM_S_TYPE_FSP && remote_lu->parallel) {
		err = 1;	/* 0x08050001 */
		goto out;
	}
	if (total_limit - fsp_limit == bidder_cnt
		&& type == SNA_SM_S_TYPE_BIDDER) {
		err = 1;	/* 0x08050001 */
		goto out;
	}
out:	return err;
}

/* Get the fully-qualified procedure correlation identifier (FQPCID) from the
 * session services (SS) component of the control point. Repeat requests if
 * a duplicate FQPCID was received. An FQPCID is considered duplicate if its
 * PCID matches that for another active or pending-active session at this LU.
 *
 * @lulu: lulu control block.
 */
static int sna_sm_get_fqpcid(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return sna_ss_assign_pcid(lulu);
}

static int sna_sm_fsm_output_a(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return sna_sm_build_and_send_init_sig(lulu);
}

static int sna_sm_fsm_output_b(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return 0;
}

static int sna_sm_fsm_output_c(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return 0;
}

static int sna_sm_fsm_output_d(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return 0;
}

static int sna_sm_fsm_output_e(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return 0;
}

static int sna_sm_fsm_output_f(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return 0;
}

static int sna_sm_fsm_output_g(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return 0;
}

static int sna_sm_fsm_output_h(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return 0;
}

static int sna_sm_fsm_output_i(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return 0;
}

/* call build_and_send_sesst_sig(lulu);
 * call build_and_send_sess_activated(lulu);
 */
static int sna_sm_fsm_output_j(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return 0;
}

static int sna_sm_fsm_output_k(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return 0;
}

static int sna_sm_fsm_output_l(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return 0;
}

static int sna_sm_fsm_output_m(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return 0;
}

static int sna_sm_fsm_output_n(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return 0;
}

static int sna_sm_fsm_output_p(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return 0;
}

static int sna_sm_fsm_output_q(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return 0;
}

static int sna_sm_fsm_output_r(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return 0;
}

/* call build_and_send_act_sess_rsp_pos(lulu).
 */
static int sna_sm_fsm_output_s(struct sna_lulu_cb *lulu)
{
	int err;

	sna_debug(5, "init\n");
	err = sna_rm_session_activated_proc(lulu);
	if (err < 0)
		sna_debug(5, "session activated failed `%d'.\n", err);
	return err;
}

static int sna_sm_fsm_output_t(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return 0;
}

static int sna_sm_fsm_output_u(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return 0;
}

static int sna_sm_fsm_output_v(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return 0;
}

/* this is a linux addition to the standard state machine.
 * nothing special just finish processing a cinit_signal_ok
 * signal.
 */
static int sna_sm_fsm_output_x(struct sna_lulu_cb *lulu)
{
	int err;

	sna_debug(5, "init\n");
	err = sna_sm_build_and_send_bind_req(lulu);
	if (err < 0)
		sna_debug(5, "build and send failed\n");
	return err;
}

static int sna_sm_fsm_output(struct sna_lulu_cb *lulu, int action)
{
	int err = -EINVAL;

	sna_debug(5, "init\n");
	switch (action) {
		case SNA_SM_FSM_OUTPUT_A:
			err = sna_sm_fsm_output_a(lulu);
			break;
		case SNA_SM_FSM_OUTPUT_B:
			err = sna_sm_fsm_output_b(lulu);
			break;
		case SNA_SM_FSM_OUTPUT_C:
			err = sna_sm_fsm_output_c(lulu);
			break;
		case SNA_SM_FSM_OUTPUT_D:
			err = sna_sm_fsm_output_d(lulu);
			break;
		case SNA_SM_FSM_OUTPUT_E:
			err = sna_sm_fsm_output_e(lulu);
			break;
		case SNA_SM_FSM_OUTPUT_F:
			err = sna_sm_fsm_output_f(lulu);
			break;
		case SNA_SM_FSM_OUTPUT_G:
			err = sna_sm_fsm_output_g(lulu);
			break;
		case SNA_SM_FSM_OUTPUT_H:
			err = sna_sm_fsm_output_h(lulu);
			break;
		case SNA_SM_FSM_OUTPUT_I:
			err = sna_sm_fsm_output_i(lulu);
			break;
		case SNA_SM_FSM_OUTPUT_J:
			err = sna_sm_fsm_output_j(lulu);
			break;
		case SNA_SM_FSM_OUTPUT_K:
			err = sna_sm_fsm_output_k(lulu);
			break;
		case SNA_SM_FSM_OUTPUT_L:
			err = sna_sm_fsm_output_l(lulu);
			break;
		case SNA_SM_FSM_OUTPUT_M:
			err = sna_sm_fsm_output_m(lulu);
			break;
		case SNA_SM_FSM_OUTPUT_N:
			err = sna_sm_fsm_output_n(lulu);
			break;
		case SNA_SM_FSM_OUTPUT_P:
			err = sna_sm_fsm_output_p(lulu);
			break;
		case SNA_SM_FSM_OUTPUT_Q:
			err = sna_sm_fsm_output_q(lulu);
			break;
		case SNA_SM_FSM_OUTPUT_R:
			err = sna_sm_fsm_output_r(lulu);
			break;
		case SNA_SM_FSM_OUTPUT_S:
			err = sna_sm_fsm_output_s(lulu);
			break;
		case SNA_SM_FSM_OUTPUT_T:
			err = sna_sm_fsm_output_t(lulu);
			break;
		case SNA_SM_FSM_OUTPUT_U:
			err = sna_sm_fsm_output_u(lulu);
			break;
		case SNA_SM_FSM_OUTPUT_V:
			err = sna_sm_fsm_output_v(lulu);
			break;
		case SNA_SM_FSM_OUTPUT_X:
			err = sna_sm_fsm_output_x(lulu);
			break;
	}
	return err;
}

static int sna_sm_fsm_input_activate_session(struct sna_lulu_cb *lulu)
{
	int err = -EINVAL;

	sna_debug(5, "init\n");
	if (lulu->state != SNA_SM_FSM_STATE_RES)
		goto out;
	lulu->state = SNA_SM_FSM_STATE_PND_CIN;
	err = sna_sm_fsm_output(lulu, SNA_SM_FSM_OUTPUT_A);
out:	return err;
}

static int sna_sm_fsm_input_init_signal_neg_rsp(struct sna_lulu_cb *lulu)
{
	int err = EINVAL;

	sna_debug(5, "init\n");
	if (lulu->state != SNA_SM_FSM_STATE_PND_CIN)
		goto out;
	lulu->state = SNA_SM_FSM_STATE_RES;
	err = sna_sm_fsm_output(lulu, SNA_SM_FSM_OUTPUT_B);
out:	return err;
}

static int sna_sm_fsm_input_cinit_signal_ok(struct sna_lulu_cb *lulu)
{
	int err;

	sna_debug(5, "init\n");
	if (lulu->state != SNA_SM_FSM_STATE_PND_CIN)
		return -EINVAL;
	lulu->state = SNA_SM_FSM_STATE_PND_BIN_RSP;
	err = sna_sm_fsm_output(lulu, SNA_SM_FSM_OUTPUT_X);
	return err;
}

static int sna_sm_fsm_input_cinit_signal_ng(struct sna_lulu_cb *lulu)
{
	int err = -EINVAL;

	sna_debug(5, "init\n");
	if (lulu->state != SNA_SM_FSM_STATE_PND_CIN)
		goto out;
	lulu->state = SNA_SM_FSM_STATE_RES;
	err = sna_sm_fsm_output(lulu, SNA_SM_FSM_OUTPUT_L);
out:	return err;
}

static int sna_sm_fsm_input_pos_bind_rsp_ok(struct sna_lulu_cb *lulu)
{
	int err = -EINVAL;

	sna_debug(5, "init\n");
	if (lulu->state != SNA_SM_FSM_STATE_PND_BIN_RSP)
		goto out;
	lulu->state = SNA_SM_FSM_STATE_PND_INI_HS_RSP_PLU;

	/* if we get here our init_hs_rsp was positive, so we simply
	 * make the state change.
	 */
	lulu->input = SNA_SM_FSM_INPUT_POS_INIT_HS_RSP;
	err = sna_sm_fsm_status(lulu);
	if (err < 0)
		sna_debug(5, "fsm_status error `%d'.\n", err);
out:	return err;
}

static int sna_sm_fsm_input_pos_bind_rsp_ng(struct sna_lulu_cb *lulu)
{
	int err = -EINVAL;

	sna_debug(5, "init\n");
	if (lulu->state != SNA_SM_FSM_STATE_PND_BIN_RSP)
		goto out;
	lulu->state = SNA_SM_FSM_STATE_RES;
	err = sna_sm_fsm_output(lulu, SNA_SM_FSM_OUTPUT_R);
out:	return err;
}

static int sna_sm_fsm_input_neg_bind_rsp(struct sna_lulu_cb *lulu)
{
	int err = -EINVAL;

	sna_debug(5, "init\n");
	if (lulu->state != SNA_SM_FSM_STATE_PND_BIN_RSP)
		goto out;
	lulu->state = SNA_SM_FSM_STATE_RES;
	err = sna_sm_fsm_output(lulu, SNA_SM_FSM_OUTPUT_E);
out:	return err;
}

static int sna_sm_fsm_input_bind(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	if (lulu->state != SNA_SM_FSM_STATE_RES)
		return -EINVAL;
	lulu->state = SNA_SM_FSM_STATE_PND_INI_HS_RSP_SLU;
	return 0;
}

static int sna_sm_fsm_input_pos_init_hs_rsp(struct sna_lulu_cb *lulu)
{
	int err = -EINVAL;

	sna_debug(5, "init\n");
	switch (lulu->state) {
		case SNA_SM_FSM_STATE_PND_INI_HS_RSP_PLU:
			lulu->state = SNA_SM_FSM_STATE_ACT;
			err = sna_sm_fsm_output(lulu, SNA_SM_FSM_OUTPUT_S);
			break;
		case SNA_SM_FSM_STATE_PND_INI_HS_RSP_SLU:
			lulu->state = SNA_SM_FSM_STATE_ACT;
			err = sna_sm_fsm_output(lulu, SNA_SM_FSM_OUTPUT_J);
			break;
	}
	return err;
}

static int sna_sm_fsm_input_neg_init_hs_rsp(struct sna_lulu_cb *lulu)
{
	int err = -EINVAL;

	sna_debug(5, "init\n");
	switch (lulu->state) {
		case SNA_SM_FSM_STATE_PND_INI_HS_RSP_PLU:
			lulu->state = SNA_SM_FSM_STATE_RES;
			err = sna_sm_fsm_output(lulu, SNA_SM_FSM_OUTPUT_H);
			break;
		case SNA_SM_FSM_STATE_PND_INI_HS_RSP_SLU:
			lulu->state = SNA_SM_FSM_STATE_RES;
			err = sna_sm_fsm_output(lulu, SNA_SM_FSM_OUTPUT_N);
			break;
	}
	return err;
}

static int sna_sm_fsm_input_deactivate_session(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return 0;
}

static int sna_sm_fsm_input_unbind(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return 0;
}

static int sna_sm_fsm_input_session_route_inop(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return 0;
}

static int sna_sm_fsm_input_abort_hs(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return 0;
}

static int sna_sm_fsm_input_rm_abend(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return 0;
}

static int sna_sm_fsm_input_hs_abend(struct sna_lulu_cb *lulu)
{
	sna_debug(5, "init\n");
	return 0;
}

int sna_sm_fsm_status(struct sna_lulu_cb *lulu)
{
	int err = -EINVAL;

	sna_debug(5, "init\n");
	switch (lulu->input) {
		case SNA_SM_FSM_INPUT_ACTIVATE_SESSION:
			err = sna_sm_fsm_input_activate_session(lulu);
			break;
		case SNA_SM_FSM_INPUT_INIT_SIGNAL_NEG_RSP:
			err = sna_sm_fsm_input_init_signal_neg_rsp(lulu);
			break;
		case SNA_SM_FSM_INPUT_CINIT_SIGNAL_OK:
			err = sna_sm_fsm_input_cinit_signal_ok(lulu);
			break;
		case SNA_SM_FSM_INPUT_CINIT_SIGNAL_NG:
			err = sna_sm_fsm_input_cinit_signal_ng(lulu);
			break;
		case SNA_SM_FSM_INPUT_POS_BIND_RSP_OK:
			err = sna_sm_fsm_input_pos_bind_rsp_ok(lulu);
			break;
		case SNA_SM_FSM_INPUT_POS_BIND_RSP_NG:
			err = sna_sm_fsm_input_pos_bind_rsp_ng(lulu);
			break;
		case SNA_SM_FSM_INPUT_NEG_BIND_RSP:
			err = sna_sm_fsm_input_neg_bind_rsp(lulu);
			break;
		case SNA_SM_FSM_INPUT_BIND:
			err = sna_sm_fsm_input_bind(lulu);
			break;
		case SNA_SM_FSM_INPUT_POS_INIT_HS_RSP:
			err = sna_sm_fsm_input_pos_init_hs_rsp(lulu);
			break;
		case SNA_SM_FSM_INPUT_NEG_INIT_HS_RSP:
			err = sna_sm_fsm_input_neg_init_hs_rsp(lulu);
			break;
		case SNA_SM_FSM_INPUT_DEACTIVATE_SESSION:
			err = sna_sm_fsm_input_deactivate_session(lulu);
			break;
		case SNA_SM_FSM_INPUT_UNBIND:
			err = sna_sm_fsm_input_unbind(lulu);
			break;
		case SNA_SM_FSM_INPUT_SESSION_ROUTE_INOP:
			err = sna_sm_fsm_input_session_route_inop(lulu);
			break;
		case SNA_SM_FSM_INPUT_ABORT_HS:
			err = sna_sm_fsm_input_abort_hs(lulu);
			break;
		case SNA_SM_FSM_INPUT_RM_ABEND:
			err = sna_sm_fsm_input_rm_abend(lulu);
			break;
		case SNA_SM_FSM_INPUT_HS_ABEND:
			err = sna_sm_fsm_input_hs_abend(lulu);
			break;
	}
	return err;
}

/* Process an ACTIVATE_SESSION record received from RM. That includes checking
 * for a session limit to be exceeded (since RM does not know whether the
 * session limit is exceeded when it sends ACTIVATE_SESSION to SM), creating
 * and initializing of the LULU_CB control block, getting an FQPCID for the
 * session from SS, and sending an INIT_SIGNAL record to SS.
 *
 *
 * @remote_name: remote lu name.
 * @mode_name: mode name session needs to use.
 * @type: first speaker (conwinner) or bidder (conloser).
 *
 * 3.5.15 - (lu62peer.boo)
 */
int sna_sm_proc_activate_session(struct sna_mode_cb *mode,
	struct sna_remote_lu_cb *remote_lu, u_int32_t tp_index, int type)
{
	struct sna_lulu_cb *lulu;
	int err;

	sna_debug(5, "init\n");
	if (sna_sm_lu_mode_session_limit(remote_lu, mode,
		type, SNA_SM_S_STATE_INIT_SENT))
		return -EUSERS;
	new(lulu, GFP_ATOMIC);
	if (!lulu)
		return -ENOMEM;
	memcpy(&lulu->local_name, &remote_lu->netid, sizeof(sna_netid));
	memcpy(&lulu->remote_name, &remote_lu->netid_plu, sizeof(sna_netid));
	lulu->index 		= sna_sm_lulu_new_index();
	lulu->remote_lu_index	= remote_lu->index;
	lulu->mode_index	= mode->index;
	lulu->tp_index		= tp_index;
	lulu->type		= type;
	lulu->state		= SNA_SM_FSM_STATE_RES;
	err = sna_sm_get_fqpcid(lulu);
	if (err < 0) {
		sna_debug(5, "get_fqpcid failed `%d'\n", err);
		return err;
	}
	list_add(&lulu->list, &lulu_list);
	lulu->input = SNA_SM_FSM_INPUT_ACTIVATE_SESSION;
	err = sna_sm_fsm_status(lulu);
	if (err < 0)
		sna_debug(5, "fsm_status error `%d'.\n", err);
	return err;
}

/* LU session manager (SM) is responsible for creating the RM process and for
 * activating and deactivating sessions between this LU and another LU. There
 * is one SM process per LU in the node, and it is created and destroyed when
 * the LU is created and destroyed. SM receives records from the resource
 * manager (RM), the half-session (HS), the address space manager (ASM), and
 * the session services (SS) processes. When the records are received, they
 * are routed to the appropriate procedures when they are processed. SM uses
 * process data (called LOCAL) that can be accessed by any procedure in the
 * SM process.
 */
int sna_sm_create(struct sna_nof_node *node)
{
	sna_debug(5, "init\n");
	sna_rm_create(node);
	return 0;
}

int sna_sm_destroy(struct sna_nof_node *node)
{
	sna_debug(5, "init\n");
	sna_rm_destroy(node);
	return 0;
}

#ifdef NOT
/* Determine if there is a state error on receipt of a BIND. */
static int sna_bind_rq_state_err(struct sna_mu *mu)
{
	struct sna_parnter_lu *plu;
	struct sna_mode *mode;
	struct sna_bind_ru *bind_ru;
	struct sna_lucb *lucb;
	struct sna_local *local;

	plu = search_plu(mu->plu_name);
	if(plu != NULL)
	{
		local->sense = (0x0835 | offset_to_plu_name);
		return (TRUE);
	}

	if(plu->lu_name != lucb->lu_name)
	{
		local->sense = 0x083B0001;
		return (TRUE);
	}

	if(lucb->security_select != plu->security_select)
	{
		local->sense = 0x080F6051;
		return (TRUE);
	}

	mode = search_mode(mu->mode_name);
	if(mode == NULL)
	{
		local->sense = 0x0835xxxx;
		return (TRUE);
	}

	if(plu->parallel != TRUE && mode->min_conwinners == 1)
		local->session_type = FIRST_SPEAKER;
	else /* Use value in bind */
	{
		if(mu->first_speaker == TRUE)
			local->session_type = FIRST_SPEAKER;
		else
			local->session_type = BIDDER;
	}

	limit = sna_bind_session_limit_exceeded(plu->fqlu_name, mode, local->session_type);
	if(limit == EXCEEDED)
		return (TRUE);

	if(plu->parallel != TRUE && another_pending_req() == TRUE)
	{
		/* Bind winner is the one with the longer LU name ;) */
	}

	/*
	 * Consistency checks on PS usage fields
	 */

	err = check_sync_levels(plu, mode);
	if(err < 0)
	{
		local->sense = 0x08350000;
		return (TRUE);
	}

	err = check_parallel_level(plu, mode);
	if(err < 0)
	{
		local->sense = 0x08350000;
		return (TRUE);
	}

	err = check_cnos();
	if(err < 0)
	{
		local->sense = 0x08350000;
		return (TRUE);
	}

	err = check_conv_security();
	if(err < 0)
	{
		local->sense = 0x080F6051;
		return (TRUE);
	}

	err = check_verif_security();
	if(err < 0)
	{
		local->sense = 0x080F6051;
		return (TRUE);
	}

	if(mu->bind.conv_secure == FALSE && mu->bind.alverified == TRUE)
	{
		local->sense = 0x0835001A;
		return (TRUE);
	}

	if(mu->bind.crypto == TRUE && mu->bind.cryptopts == NULL)
	{
		local->sense = 0x08480000;
		return (TRUE);
	}

	/*
	 * One whole If-Else pair here
	 */

	err = check_session_id();
	if(err < 0)
	{
		local->sense = 0x08520001;
		return (TRUE);
	}

	if(plu->segments == FALSE && send_ru_size_lower_bound)
	{
		local->sense = 0x0877002A;
		return (TRUE);
	}

	if(lucb->segments == FALSE && send_ru_size_lower_bound)
	{
		local->sense = 0x0877002A;
		return (TRUE);
	}

	if(lucb->seg_reassm != TRUE && send_ru_size_lower_bound)
	{
		local->sense = 0x0877002B;
		return (TRUE);
	}

	return (FALSE);
}

/* Perform state error checking on a received +RSP(BIND). */
static int sna_bind_rsp_state_err(struct sna_mu *mu, struct sna_lu_lu_cb *lulucb)
{
	struct sna_local *local;
	struct sna_partner_lu *plu;
	struct sna_bind_ru *bind_ru;
	struct sna_mode *mode;

	/* Pacing and Max. RU size checks */
	if(bind->pace != rsp->pace)
	{
		local->sense = 0x08350008 or 0x0835000C;
		return (TRUE);
	}

	if(rsp->adaptive_pace != TRUE)
	{
		if(rsp->second.send_window != bind->second.send_window)
		{
			local->sense = 0x08350008;
			return (TRUE);
		}

		/* 0 is infinately large */
		if(rsp->second.recv_window > bind->second.recv_window)
		{
			local->sense = 0x08350008;
			return (TRUE);
		}

		if(rsp->primary.send_window > bind->primary.send_window)
		{
			local->sense = 0x0835000C;
			return (TRUE);
		}

		if(rsp->primary.recv_window != bind->primary.recv_window)
		{
			local->sense = 0x0835000C;
			return (TRUE);
		}
	}

	/* Determine if within bounds for send/recv wins */
	err = check_ru_bounds();
	if(err < 0)
	{
		if(secondary_out_bounds)
			local->sense = 0x0835000A;
		else
			local->sense = 0x0835000B;

		return (TRUE);
	}

	/* PS usage checks */
	if(other_active_sessions(plu) && conv_security != plu->conv_security)
	{
		local->sense = 0x080F6051;
		return (TRUE);
	}

	if(other_active_sessions(plu) && alrdy_verified != plu->alrdy_verified)
	{
		local->sense = 0x080F6051;
		return (TRUE);
	}

	if(other_active_sessions(plu) && sync != plu->sync)
	{
		local->sense = 0x08350018;
		return (TRUE);
	}
	else
	{
		if(rsp->sync == (CONFIRM|SYNCPOINT|BACKOUT)
			&& bind->sync == (CONFIRM))
		{
			local->sense = 0x08350018;
			return (TRUE);
		}
	}

	if(rsp->parallel != TRUE)
	{
		if(rsp->sess_reinit == NONOPCTRL
			&& bind->sess_reinit == OPCTRL)
		{
			local->sense = 0x08350018;
			return (TRUE);
		}

		if(rsp->sess_reinit == SECONDARY
			&& bind->sess_reinit == PRIMARY)
		{
			local->sense = 0x08350018;
			return (TRUE);
		}

		if(rsp->sess_reinit == PRIMARY
			&& bind->sess_reinit == SECONDARY)
		{
			local->sense = 0x08350018;
			return (TRUE);
		}
	}

	if(rsp->parallel_opts != bind->parallel_opts
		&& rsp->chg_sess != bind->chg_sess)
	{
		local->sense = 0x08350018;
		return (TRUE);
	}

	/* Contention winner checks */
	if(rsp->parallel == TRUE)
	{
		if(rsp->contention_winner != bind->contention_winner)
		{
			local->sense = 0x08035007;
			return (TRUE);
		}
	}
	else
	{
		if(rsp->contention_winner == PRIMARY
			&& bind->contention_winner == SECONDARY)
		{
			local->sense = 0x08350007;
			return (TRUE);
		}
	}


	if(rsp->contention_winner == PRIMARY)
		local->session_type = FIRST_SPEAKER;
	else
		local->session_type = BIDDER;

	limit = sna_mode_session_limit_exceeded(plu->fqlu_name, mode, local->session_type, active);
	if(limit == EXCEEDED)
	{
		return(TRUE);
	}

	/* Crypto checks */
	if(rsp->cryptopts != bind->cryptopts)
	{
		local->sense = 0x0835xxxx;
		return (TRUE);
	}

	/* User data subfield checks */
	if(rsp->mode_name != bind->mode->name)
	{
		local->sense = 0x0835xxxx;
		return (TRUE);
	}

	if(lulu_cb->random != NULL)
	{
		if(rsp->security == NULL || is_incorrect)
		{
			local->sense = 0x080F6051;
			return (TRUE);
		}
	}

	/* User data subfield - session_id checks */
	if(rsp->session_id != 0x02)
	{
		if(rsp->session_id != 0x00 || rsp->session_id != 0xF0
			|| rsp->session_id != bind->session_id)
		{
			local->sense = 0x0835xxxx;
			return (TRUE);
		}
	}
	else
	{
		if(plu->fq_pcid != TRUE)
		{
			local->sense = 0x0835xxxx;
			return (TRUE);
		}
	}

	if(rsp->session_id != NULL)
	{
		if(rsp->sessiond_id == 0x02)
			lulu_cb->session_id = fq_pcid->pcid;
		else
			lulu_cb->session_id = rsp->session_id;

		if(sna_id_unique(SESSION, lulu_cb->session_id) < 0)
		{
			local->sense = 0x8520001;
			return (TRUE);
		}
	}

	/* URC Checks */
	if(rsp->urc != bind->urc)
		return (TRUE);

	return (FALSE);
}

/* Betermine whether or not session limits are exceeded for a received BIND. */
static int sna_bind_session_limit_exceeded(unsigned char *plu_fqlu_name,
	struct sna_mode *mode, int type)
{
	struct sna_local *local;

	if(mode->cnos_negotiation_in_progress == TRUE
		&& prop_slimit > curr_slimit)
	{
		if(mode->active.sessions ? proposed.slimit)
		{
			local->sense = 0x08050000;
			return (TRUE);
		}
		else
		{
			if((mode->active.sessions + mode->pending.sessions)
				? proposed.slimit)
			{
				local->sense = 0x08050000;
				return (TRUE);
			}
		}
	}
	else
	{
		limit = sna_lu_mode_session_limit_exceeded(plu_fqlu_name, mode, type, ACTIVE);
		if(limit == EXCEEDED)
		{
			local->rcode = TRUE;
		}
		else
		{
			limit = sna_lu_mode_session_limit_exceeded(plu_fqlu_name, mode, type, AT_LEAST_BIND_SENT);
			if(limit == EXCEEDED)
				local->check_winner_flag = TRUE;
		}
	}

	/* Check for BIND race conditions */
	if(local->check_winner_flag == TRUE)
	{
		/* Fake for now */
		if(session->lu_name > plu_fq_lu_name)
			local->rcode = TRUE;
		else
		{
			local->sense = 0x00000000;
			local->rcode = FALSE;
		}
	}

	return (local->rcode);
}

/* Build and send ACTIVATE_SESSION_RSP (negative) to RM. */
static int sna_build_and_send_act_sess_rsp_neg(__u8 correlator, int err)
{
	struct sna_activate_session_rsp *act_rsp;

	new(act_rsp, GFP_ATOMIC);
	act_rsp->correlator = correlator;
	act_rsp->type = NEG;
	act_rsp->err_type = err;

	send_to_rm(act_rsp);

	return (0);
}

/* Build and send ACTIVATE_SESSION_RSP (positive) to RM. This completes
 * (from the SM's standpoint) the session initiation activity triggered by
 * the ACTIVATE_SESSION record received by SM from RM.
 */
static int sna_build_and_send_act_sess_rsp_pos(struct sna_lulu_cb *lulu_cb)
{
	struct sna_actiavte_session_rsp *act_rsp;

	new(act_rsp, GFP_ATOMIC);
	act_rsp->correlator = lulu_cb->correlator;
	act_rsp->type = POS;

	act_rsp->session_information.hs_id = lulu_cb->hs_id;
	act_rsp->session_information.hs_type = PRI;
	act_rsp->session_information.bracket_type = lulu_cb->session_type;

	act_rsp->session_information.send_ru_size = neg_max_send_ru_size;
	act_rsp->session_information.perm_buf_pool_id = perm_buf_pool_id;
	act_rsp->session_information.limit_buf_pool_id = limit_buf_pool_id;

	act_rsp->session_informtaion.session_id = lulu_cb->session_id;

	act_rsp->session_information.random_data = lulu_cb->random_data;
	act_rsp->session_information.limit_resource = lulu_cb->limit_resource;

	send_to_rm(act_rsp);

	return (0);
}

/* Build and send a -RSP(BIND). */
static int sna_build_and_send_bind_rsp_neg(unsigned char *buf)
{
	struct sna_mu *mu;

	mu->header_type = BIND_RSP_SEND;
	mu->bind_rsp_send.sender.id = lu_id;
	mu->bind_rsp_send.sener.type = SM;
	mu->bind_rsp_send.lfsid = bind->lfsid;
	mu->bind_rsp_send.pc_id = bind->pc_id;
	mu->bind_rsp_send.tx_priority = LOW;
	mu->bind_rsp_send.free_lfsid = YES;
	mu->bind_rsp_send.hs_id = NULL;

	/* Set TH and RH fields to default 3.5.9 SNA formats */

	/* Set RU */

	mu->dcf = (RH->size + RU->size);

	send_to_asm(mu);

	return (0);
}

/* Build and send a FREE_LFSID record to the control point. This is
 * necessary when SM asked ASM to give SM an LFSID for a session, and SM
 * received ASSIGN_LFSID_RSP, but could not send a BIND (because, for
 * example, SM cannot get a buffer for it). In this case, SM explicity asks
 * ASM to free the LFSID by sending the FREE_LFSID record to it. If SM sends
 * a BIND successfully, it later sends an UNBIND or a RSP(UNBIND) to ASM
 * and sets the FREE_LFSID variable to YES in them.
 */
static int sna_build_and_send_free_lfsid(struct sna_lulu_cb *lulu_cb)
{
	struct sna_free_lfsid *free_lfsid;

	new(free_lfsid, GFP_ATOMIC);
	if (!free_lfsid)
		return -ENOMEM;
	free_lfsid->pc_id = lulu_cb->pc_id;
	free_lfsid->lfsid = lulu_cb->lfsid;

	send_to_asm(free_lfsid);

	return (0);
}

/* Build and send a PC_HS_DISCONNECT record to ASM. This is done only after
 * a PLU receives a -RSP(BIND). If, instead, SM receives an UNBIND, it sends
 * a RSP(UNBIND), asking ASM to free LFSID, thus disconnecting PC and HS.
 */
static int sna_build_and_send_pc_hs_disconnect(struct sna_lulu_cb *lulu_cb)
{
	struct sna_msg_queue *msg = NULL;

	msg->cmd 	= SNA_PC_HS_DISCONNECT;
	msg->pc_id	= lulu_cb->pc_id;
	msg->lfsid	= lulu_cb->lfsid;

	sna_asm(msg);

	return (0);
}

/* Build and send SESSION_ACTIVATED to RM to indicate that a new session
 * has become active and to give RM the information about this session.
 */
static int sna_build_and_send_sess_activated(struct sna_lulu_cb *lulu_cb)
{
	struct sna_session_activated *activated;

	new(activated, GFP_ATOMIC);

	activated->session_information.hs_id = lulu_cb->hs_id;
	activated->session_information.hs_type = SEC;
	activated->session_information.bracket_type = lulu_cb->session_type;

	activated->session_information.send_ru_size = neg_max_send_ru_size;
	activated->session_information.perm_buf_pool_id = lulu_cb->perm_pool_id;
	actiavted->session_information.limit_buf_pool_id = lulu_cb->dem_lim_pool_id;
	activated->session_information.session_id = lulu_cb->session_id;

	/* Send to RM for FMH-12 info */

	activated->session_information.random_data = lulu_cb->random;
	activated->lu_name = lulu_cb->local_partner_lu_name;
	activated->mode_name = lulu_cb->mode_name;
	activated->session_information.limit_resource = lulu_cb->limit_resource;

	err = send_to_rm(activated);
	if(err < 0)
		destroy(activated);

	return (0);
}

/* Build and send SESSION_DEACTIVATED to RM to indicate that an active session
 * has been deactivated.
 */
static int sna_build_and_send_sess_deactivated(__u8 hs_id, __u8 reason,
	__u8 sense)
{
	struct sna_session_deactivated *deactivated;

	new(deactivated, GFP_ATOMIC);
	deactivated->hs_id 	= hs_id;
	deactivated->reason 	= reason;
	if(reason != NORMAL)
		deactivated->sense = sense;

	err = send_to_rm(deactivated);
	if(err < 0)
		destroy(deactivated);

	return (0);
}

/* Build and send a SESSEND_SIGNAL record to the control point. This record
 * can be sent by both PLU and SLU when the session is brought down. The PLU
 * sends it, however, only if it has previously received a CINIT_SIGNAL
 * record. The SLU sends it only if it has already sent a SESSST_SIGNAL
 * record to SS.
 */
static int sna_build_and_send_sessend_sig(struct sna_lulu_cb *lulu_cb,
	__u8 sense)
{
	struct sna_sessend_signal *sessend_signal;

	new(sessend_signal, GFP_ATOMIC);

	sessend_signal->sense = sense;
	sessend_signal->fqpcid = lulu_cb->fqpcid;
	sessend_signal->pc_id = lulu_cb->pc_id;

	send_to_ss(sessend_signal);

	return (0);
}

/* Build and send a SESSST_SIGNAL record to the control point. This record is
 * sent by the SLU when it receives the INIT_HS_RSP record from the half-session
 * process. The PLU does not need to send it, since its local SS sends a
 * CINIT_SIGNAL to SM and assumes that the session will be activated.
 */
static int sna_build_and_send_sessst_sig(struct sna_lu_lu_cb *lulu_cb)
{
	struct sna_sessst_signal *sessst_signal;

	new(sesst_signal, GFP_ATOMIC);

	sessst_signal->pc_id = lulu_cd->pc_id;

	send_to_ss(sessst_signal);

	return (0);
}

/* Build and send an UNBIND. */
static int sna_build_and_send_unbind_rq(unsigned char *buf, __u8 cleanup,
	__u8 sense)
{
	struct sna_mu *mu;
	struct sna_local *local;
	struct sna_lu_lu_cb *lulu_cb;

	mu->header_type = UNBIND_RQ_SEND;
	mu->unbind_rq_send.sender.id = local->lu_id;
	mu->unbind_rq_send.sender.type = SM;
	mu->unbind_rq_send.lfsid = lulu_cb->lfsid;
	mu->unbind_rq_send.pc_id = lulu_cd->pc_id;
	mu->unbind_rq_send.tx_priority = lulu_cd->tx_priority;
	mu->unbind_rq_send.free_lfsid = YES;
	mu->unbind_rq_send.hs_id = lulu_cd->hs_id;

	/* Set TH and RH fields to default */

	/* Set RU of unbind mu to defaults, using sense and type passed */

	mu->dcf = (RH->size + RU->size);

	send_to_asm(mu);

	return (0);
}

/* Build and send a RSP(UNBIND). */
static int sna_build_and_send_unbind_rsp(struct sna_mu *mu)
{
	struct sna_lu_lu_cb *lulu_cb
	struct sna_mu *mu_new;
	int unbind_type;

	if(determine_how_unbind_was_recieved() == EXR || length_err)
		unbind_type = NEG;
	else
		unbind_type = POS;

	mu_new = bm(GET_BUFFER, demand, size, no_wait);
	if(mu != NULL)
	{
		mu_new->header_type = UNBIND_RSP_SEND;
		mu_new->unbind_rsp_send.lu_id = lulu_cb->lu_id;
		mu_new->unbind_rsp_send.sender.type = SM;

		if(unbind correlated to a specific session)
		{
			mu_new->unbind_rsp_send.hs_id = lulu_cb->hs_id;
			mu_new->unbind_rsp_send.tx_priority = lulu_cb->tx_priority;
		}
		else
		{
			mu_new->unbind_rsp_send.hs_id = NULL;
			mu_new->unbind_rsp_send.tx_priority = LOW;
		}

		mu_new->unbind_rsp_send.free_lfsid = YES;
		mu_new->pc_id = mu->pc_id;
		mu_new->lfsid = mu->lfsid;
		mu_new->th.snf = mu->th.snf;

		/* Set TH and RH to defaults */

		if(type == POS)
			mu_new->ru.type = POS;
		else
			mu_new->ru.type = NEG;

		mu->dcf = (RH->size + RU->size);

		send_to_asm(mu);
	}
	else
		/* IBM.. Duh do nothing */

	return (0);
}

/* Clean up LU-LU session. */
static int sna_cleanup_lu_lu_session(struct sna_lu_lu_cb *lulu_cb)
{
	if(sesst_signal was sent || cinit_signal was recieved)
		sna_build_and_send_sessend_sig(lulu_cb);

	sna_unreserve_buffers(lulu_cb);
	hs = sna_hs_unlink(lulu_cb->hs_id);
	sna_hs_destroy(hs);

	random = sna_random_unlink(lulu_cb->random);
	sna_random_destroy(random);

	lulu_cb = sna_lulu_cb_unlink(lulu_cb);
	sna_lulu_cb_destroy(lulu_cb);

	return (0);
}

/* Process an abend notification record from a child process (RM or HS). */
static int sna_process_abend_notification(struct sna_abend_notification *abend)
{
	struct sna_local *local;
	struct sna_lu_lu_cb *lulu_cb;

	switch(abend->abend_process)
	{
		case (RM_PROCESS_VARIABLE):
			for(lu = local->lulu_cb_list; lu != NULL; lu = lu->next)
			{
				fsm_status(abend, lulu_cb);
			}
			break;

		case (HS_PROCESS_VARIABLE):
			lu = search_lulu_cb(abend->hs_id);
			if(lu != NULL)
				fsm_status(abend, lulu_cb);
			break;

		default:
			sna_debug(5, "unknown\n");
	}

	return (0);
}

/* Process an ABORT_HS record received from LU-LU half-session. */
static int sna_process_abort_hs(struct sna_abort_hs *abort_hs)
{
	struct sna_local *local;
	struct sna_lu_lu_cb *lulu_cb;

	lulu_cb = search_lulu_cb(local->hs_id);
	if(lulu_cb != NULL)
		fsm_status(abort_hs, lulu_cb);

	return (0);
}

/* Process a DEACTIVATION_SESSION record received from RM. */
static int sna_process_deactivation_session(struct sna_deactivate_session *deactivate)
{
	struct sna_lu_lu_cb *lulu_cb;

	if(deactivate->status == PENDING)
		lulu_cb = search_lulu_cb(deactivate->correlator);
	else
		lulu_cb = search_lulu_cb(deactivate->hs_id);

	if(lulu_cb)
		fsm_status(deactivate, lulu_cb);

	return (0);
}

/* Process a received LFSID_IN_USE record. This record is sent to SM by ASM
 * so that ASM will know whether a given (LFSID, PC_ID) pair is
 * currently in use. ASM must know before it sends a BIND to an appropriate
 * LU. If the pair is in use, ASM will hold the BIND in order to avoid
 * certain race conditions.
 */
static int sna_process_lfsid_in_use(struct sna_lfsid_in_use *lfsid_in_use)
{
	struct sna_lfsid_in_use_rsp *lfsid_in_use_rsp;

	/* find activate or pending session ?? */

	new(lfsid_in_use_rsp, GFP_ATOMIC);

	lfsid_in_use_rsp->pc_id = lfsid_in_use->pc_id;
	lfsid_in_use_rsp->lfsid = lfsid_in_use->lfsid;

	if(sna_find_session(PEND_ACTIVE, lfsid, pc_id))
		lfsid_in_use_rsp->answer = YES;
	else
		lfsid_in_use_rsp->answer = NO;

	send_to_asm(lfsid_in_use_rsp);

	return (0);
}

/* Process a SESSION_ROUTE_INOP record received from ASM. */
static int sna_process_session_route_inop(struct sna_session_route_inop *inop)
{
	struct sna_lu_lu_cb *lulu_cb;

	for(lulu_cb = lulu_cb_list; lulu_cb != NULL; lulu_cb = lulu_cb->next)
	{
		if(inop->pc_id != lulu_cb->pc_id)
			continue;

		fsm_status(inop, lulu_cb);
	}

	return (0);
}
#endif
