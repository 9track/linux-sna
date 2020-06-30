/* sna_dfc.c: Linux Systems Network Architecture implementation
 * - SNA LU 6.2 Data Flow Control (DFC)
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

static int sna_dfc_send_rsp_mu(struct sna_hs_cb *hs, struct sk_buff *skb,
	u_int8_t expedite, u_int8_t negative, u_int32_t sense);
static int sna_dfc_ct_update(struct sna_hs_cb *hs, struct sk_buff *skb);
static int sna_dfc_ok_to_reply(struct sna_hs_cb *hs, struct sk_buff *skb);
static int sna_dfc_fsm_bsm_fmp19_state_chk(struct sna_hs_cb *hs, struct sk_buff *skb,
	int signal, int output);
static int sna_dfc_fsm_bsm_fmp19(struct sna_hs_cb *hs, struct sk_buff *skb,
	int signal);
static int sna_dfc_fsm_chain_rcv_fmp19_state_chk(struct sna_hs_cb *hs,
	struct sk_buff *skb, int chain, int output);
static int sna_dfc_fsm_qri_chain_rcv_fmp19_state_chk(struct sna_hs_cb *hs,
	struct sk_buff *skb, int output);
static int sna_dfc_fsm_chain_send_fmp19(struct sna_hs_cb *hs,
	struct sk_buff *skb, int chain);
static int sna_dfc_fsm_chain_send_fmp19_state_chk(struct sna_hs_cb *hs,
	struct sk_buff *skb, int chain, int output);
static int sna_dfc_send_fsms(struct sna_hs_cb *hs, struct sk_buff *skb);
static int sna_dfc_fsm_rcv_purge_fmp19(struct sna_hs_cb *hs,
	struct sk_buff *skb, int purge);
static int sna_dfc_fsm_chain_rcv_fmp19(struct sna_hs_cb *hs,
	struct sk_buff *skb, int chain);
static int sna_dfc_fsm_qri_chain_rcv_fmp19(struct sna_hs_cb *hs,
	struct sk_buff *skb);

/**
 * send RM a positive reponse to a BID, and receive the HS_PS_CONNECTED
 * record that will result in this half-session being connected to a PS.
 *
 * @hs: information about the last chain sent, local.ct_send.
 * @skb: mu.
 *
 * bid_pos_rsp sent to rm, local.current_bracket_sqn.
 */
#ifdef NOT
static int sna_dfc_send_bid_pos_rsp(struct sna_hs_cb *hs, struct sk_buff *skb)
{
	sna_debug(5, "init\n");

	struct sna_hs_local *local = sna_hs_find_local(mu->hs_id);
	struct sna_lu_lu_cb *lulu = &local->lulu;
	struct sna_hs_ps_connected *hs_ps_connected = NULL;
	struct sna_bid_rsp *pos_bid;

	new(pos_bid, GFP_KERNEL);
	if (!pos_bid)
		return -ENOMEM;
	pos_bid->hs_id = local->hs_id;
	pos_bid->sense = 0;
	sna_send_to_rm(mu);

	/* Receive the hs_ps_connected record. */
	lulu->ps_id             = hs_ps_connected->ps_id;
	lulu->bracket_id        = hs_ps_connected->bracket_id;

	sna_fsm_bsm_fmp19(mu, SNA_DFC_FSM_INB);
	kfree(hs_ps_connected);

	lulu->current_bracket_sqn.number = 0;
	return 0;
}
#endif

/**
 * fill in mu->hs_to_ps_header based on the contents of mu.rh.
 *
 * @hs: information about the type of hs_to_ps header that needs to be built.
 * @skb: mu containing data that needs to be passed to ps.
 *
 * mu fields may be set properly to reflect the contents of mu.rh.
 */
static int sna_dfc_build_hs_to_ps_header(struct sna_hs_cb *hs,
	struct sk_buff *skb)
{
	sna_rh *rh = sna_transport_header(skb);
	struct sna_skb_cb *cb = SNA_SKB_CB(skb);

	sna_debug(5, "init\n");
	if (rh->fi == SNA_RH_FI_FMH) {
		if (rh->ru == SNA_RH_RU_FMD)
			cb->fmh = 1;
	}
	if (rh->eci == SNA_RH_ECI_EC) {
		if (SNA_DFC_RQE1(rh) && rh->cdi == SNA_RH_CDI_CD) {
			cb->hs_ps_type = SNA_CTRL_T_PREPARE_TO_RCV_FLUSH;
			goto out;
		}
		if ((SNA_DFC_RQD1(rh) || SNA_DFC_RQE1(rh))
			&& rh->cebi == SNA_RH_CEBI_CEB) {
			cb->hs_ps_type = SNA_CTRL_T_DEALLOCATE_FLUSH;
			goto out;
		}
		if ((SNA_DFC_RQD2(rh) || SNA_DFC_RQD3(rh))
			&& rh->cdi == SNA_RH_CDI_NO_CD
			&& rh->cebi == SNA_RH_CEBI_NO_CEB) {
			cb->hs_ps_type = SNA_CTRL_T_CONFIRM;
			goto out;
		}
		if ((SNA_DFC_RQD2(rh) || SNA_DFC_RQE2(rh)
			|| SNA_DFC_RQD3(rh) || SNA_DFC_RQE3(rh))
			&& rh->cdi == SNA_RH_CDI_CD) {
			cb->hs_ps_type = SNA_CTRL_T_PREPARE_TO_RCV_CONFIRM;
			goto out;
		}
		if ((SNA_DFC_RQD2(rh) || SNA_DFC_RQD3(rh))
			&& rh->cebi == SNA_RH_CEBI_CEB) {
			cb->hs_ps_type = SNA_CTRL_T_DEALLOCATE_CONFIRM;
			goto out;
		}
	} else {
		cb->hs_ps_type = SNA_CTRL_T_NOT_END_OF_DATA;
	}
out:    return 0;
}

/**
 * needs to put data right on the rx_queue of the local listener
 * corresponding with the bracket ID in the packet.
 *
 * @hs: the bracket id that identifies this converstation.
 * @skb: mu to send (may be null if record type is not mu).
 * @type: record type to send if skb is null.
 *
 * appropriate record or an mu is sent to ps.
 */
static int sna_dfc_send_to_ps(struct sna_hs_cb *hs, struct sk_buff *skb, int type)
{
	struct sna_lulu_cb *lulu;
	struct sna_tp_cb *tp;
	struct sna_rcb *rcb;
	struct sna_skb_cb *cb = SNA_SKB_CB(skb);
	int err = -EINVAL;

	sna_debug(5, "init\n");
	if (!skb || type != SNA_CTRL_T_REC_MU) {
		sna_debug(5, "FIXME: need to create specified record\n");
		goto error;
	}
	err = -ENOENT;
	lulu = sna_sm_lulu_get_by_index(hs->lulu_index);
	if (!lulu)
		goto error;
	tp = sna_rm_tp_get_by_index(lulu->tp_index);
	if (!tp)
		goto error;
	rcb = sna_rm_rcb_get_by_index(tp->rcb_index);
	if (!rcb)
		goto error;
	err = 0;
	cb->type = type;
	skb_queue_tail(&rcb->hs_to_ps_queue, skb);
	wake_up_interruptible(&rcb->sleep);
	sna_debug(5, "DATA QUEUED TO PS... OKAY!!\n");
	goto out;
error:	kfree_skb(skb);
out:    return err;
}

/**
 * process an ru and, based on the conent of the ru, send the appropriate
 * records to rm and ps.
 *
 * @hs: loca.shs_bb_register; local.phs_bb-register; local.half_session
 *      (indication that half-session is primary or secondary); possibly
 *      in addition, an hs_ps_connected record received from rm.
 * @skb: mu containing a normal-flow request.
 *
 * appropriate records sent to rm or ps; if an fmh-5 (attach) is present,
 * local.current_bracket_sqn is set.
 */
static int sna_dfc_process_ru_data(struct sna_hs_cb *hs, struct sk_buff *skb)
{
	sna_rh *rh = sna_transport_header(skb);

	sna_debug(5, "init\n");
	if (rh->fi == SNA_RH_FI_FMH && rh->ru == SNA_RH_RU_FMD) {
		sna_fmh *fh = (sna_fmh *)skb->data;
		switch (fh->type) {
			case SNA_FMH_TYPE_5:	/* attach. */
				sna_dfc_build_hs_to_ps_header(hs, skb);
				sna_rm_proc_attach(skb);
				sna_dfc_fsm_bsm_fmp19(hs, skb, SNA_DFC_FSM_BSM_IND_INB);
				if (hs->type == SNA_HS_TYPE_FSP)
					hs->current_bracket.num = hs->shs_bb_register.num;
				else
					hs->current_bracket.num = hs->phs_bb_register.num;
				break;
			case SNA_FMH_TYPE_7:	/* error. */
				sna_debug(5, "fmh error\n");
				sna_dfc_build_hs_to_ps_header(hs, skb);
				sna_dfc_send_to_ps(hs, skb, SNA_CTRL_T_REC_MU);
				break;
			case SNA_FMH_TYPE_12:	/* security. */
				sna_debug(5, "security\n");
				sna_dfc_build_hs_to_ps_header(hs, skb);
				sna_rm_proc_security(skb);
				break;
		}
		goto out;
	}
	if (rh->eci == SNA_RH_ECI_EC || skb->len) {
		sna_dfc_build_hs_to_ps_header(hs, skb);
		sna_dfc_send_to_ps(hs, skb, SNA_CTRL_T_REC_MU);
		goto out;
	}
	kfree_skb(skb);
out:	return 0;
}

/**
 * this procedure builds and sends records to rm or ps based on the
 * received response mu.
 *
 * @hs: indication that session is first speaker; information about
 *      the last sent request.
 * @skb: mu containing a response.
 *
 * the appropriate "response" record is sent to rm or ps.
 * local.current_bracket_sqn is set to the sequence number of the last
 * sent bb request. the id of the ps connected to this hs may be saved.
 */
static int sna_dfc_send_rsp_to_rm_or_ps(struct sna_hs_cb *hs,
	struct sk_buff *skb)
{
	sna_debug(5, "init\n");
	return 0;
}

/**
 * generate the appropriate records for rm and ps based on the passed
 * mu's content.
 *
 * @hs: information about the last request send, local.ct_send;
 *      possibly in addition, a bid_rsp or an rtr_rsp record from rm.
 * @skb: mu containing normal-flow request.
 *
 * appropriate records sent to rm and ps, local.current_bracket_sqn,
 * id of the ps connected to this hs.
 */
static int sna_dfc_generate_rm_ps_inputs(struct sna_hs_cb *hs,
	struct sk_buff *skb)
{
	sna_rh *rh = sna_transport_header(skb);

	sna_debug(5, "init\n");
	if (rh->bbi == SNA_RH_BBI_BB) {
		kfree_skb(skb);
		goto out;
	}
	if ((rh->ru == SNA_RH_RU_DFC && skb->data[0] == SNA_RU_RC_BIS)
		|| (rh->ru == SNA_RH_RU_DFC && skb->data[0] == SNA_RU_RC_RTR)
		|| (rh->ru == SNA_RH_RU_DFC && skb->data[0] == SNA_RU_RC_LUSTAT)) {
		kfree_skb(skb);
		goto out;
	}
	if (sna_dfc_ok_to_reply(hs, skb) && (SNA_DFC_RQE2(((sna_rh *)&hs->ct_send.rh))
		|| SNA_DFC_RQE3(((sna_rh *)&hs->ct_send.rh))))
		sna_dfc_send_to_ps(hs, skb, SNA_CTRL_T_REC_CONFIRMED);
	sna_dfc_process_ru_data(hs, skb);
out:	return 0;
}

/**
 * send a response to the passed MU if required.
 *
 * @hs: information about the last received request; indication that a
 *      response is owed; the type (positive or negative) response to a
 *      bb request or rtr request or negative response to the next chain;
 *      when a negative response is owed, the sense data (included in the
 *      response).
 * @skb: mu, containing a normal-flow request.
 *
 * response sent if required, indication that a response is owed.
 */
static int sna_dfc_send_rsp_if_required(struct sna_hs_cb *hs,
	struct sk_buff *skb)
{
	sna_debug(5, "init\n");
	sna_debug(5, "FIXME: finish me!\n");
#ifdef NOT
	if(SNA_DFC_RQD(req_h))
	{
		if(SNA_DFC_POS_RSP(req_h))
			sna_dfc_send_rsp_mu(mu, SNA_TH_EFI_NORM, SNA_DFC_POS, 0);
		else
		{
			if(SNA_DFC_RQE(req_h) && req_h->cebi == SNA_RH_CEBI_CEB)
				sna_debug(5, "error\n");
			else
				sna_send_rsp_mu(mu, SNA_TH_EFI_NORM,SNA_DFC_NEG,
					lulu->bb_rsp_sense);
		}

		lulu->bb_rsp_state = 0;
		lulu->bb_rsp_sense = 0;
	}

	if(SNA_DFC_RQD(req_h))
	{
		if(SNA_DFC_POS_RSP(req_h))
			sna_send_rsp_mu(mu, SNA_NORMAL, SNA_DFC_POS, 0);
		else
			sna_send_rsp_mu(mu, SNA_NORMAL, SNA_DFC_NEG,
				lulu->rtr_rsp_sense);

		lulu->rtr_rsp_state = 0;
	}

	if(SNA_DFC_NEG_RSP(req_h))
	{
		if((req_h->bci == SNA_RH_BCI_BC && req_h->ru == SNA_RH_RU_FMD)
			|| (req_h->ru == SNA_RH_RU_DFC
			&& req_h->bbi != SNA_RH_BBI_BB))
		{
			sna_send_rsp_mu(mu, SNA_NORMAL, SNA_DFC_NEG, 0x08460000);
			lulu->send_error_rsp_state = 0;
		}
	}
#endif

	if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_PEND_RSP) {
		/* and the last chain received was CEB, RQD1 */
		sna_dfc_send_rsp_mu(hs, skb, 0, 0, 0);
	}
	return 0;
}

/**
 * determine if sense data on a negative response is valid.
 *
 * @hs: information about the last chain sent, local.ct_send;
 *      first-speaker indicator, local.first_speaker.
 * @skb: mu containing negative response.
 *
 * true for invalid sense data; otherwise, false.
 */
static int sna_dfc_invalid_sense_code(struct sna_hs_cb *hs, struct sk_buff *skb)
{
	int err = 0;

	/* this function needs to be validated as the states of
	 * bb, rtr and bis rsp are not clear.
	 */
	sna_debug(5, "init\n");
	if (hs->bb_rsp_state == SNA_DFC_RSP_STATE_POS_OWED) {
		if (hs->type == SNA_HS_TYPE_FSP) {
			if (*((u_int32_t *)skb->data) != 0x08460000
				&& *((u_int32_t *)skb->data) != 0x88B0000) {
				err = 1;
				goto out;
			}
		} else {
			if (*((u_int32_t *)skb->data) != 0x08130000
				&& *((u_int32_t *)skb->data) != 0x08140000) {
				err = 1;
				goto out;
			}
		}
		goto out;
	}
	if (hs->bb_rsp_state == SNA_DFC_RSP_STATE_NEG_OWED) {
		if (hs->rtr_rsp_state == SNA_DFC_RSP_STATE_NEG_OWED) {
			if (*((u_int32_t *)skb->data) != 0x08190000) {
				err = 1;
				goto out;
			}
		} else {
			/* fixme: detect if rsp to BIS. */
			err = 1;
			goto out;
		}
		goto out;
	}
out: 	return err;
}

/**
 * perform state error checking on received RQ/RSP. The types of errors
 * found here are protocol violations by the sender of the RQ/RSP. These
 * checks are optional. None, some, or all of the checks may be made.
 *
 * @hs: indication of whether a response to a signal is
 *      expected, local.sig_rq_outstanding.
 * @skb: mu containing request or response.
 *
 * true if a state error was encountered; otherwise, false. if true,
 * local.sense_code is set to the appropriate sense data.
 */
static int sna_dfc_rcv_state_error(struct sna_hs_cb *hs, struct sk_buff *skb)
{
	sna_rh *rh = sna_transport_header(skb);
	sna_fid2 *fh = sna_network_header(skb);
	int err = 0;

	sna_debug(5, "init\n");

	/* normal flow request. */
	if (rh->rri == SNA_RH_RRI_REQ
		&& fh->efi == SNA_TH_EFI_EXP) {
		if (hs->fsm_bsm_fmp19_state == SNA_DFC_FSM_BSM_IND_BETB
			&& ((sna_fmh *)skb->data)->type != SNA_FMH_TYPE_5)
			sna_dfc_fsm_bsm_fmp19(hs, skb, SNA_DFC_FSM_BSM_IND_NONE);
		if (SNA_DFC_FSM_RQE(rh)
			&& rh->bbi == SNA_RH_BBI_BB
			&& rh->cebi == SNA_RH_CEBI_CEB) {
			hs->sense = 0x40040000;
			err = 1;
			goto out;
		}
		if (sna_dfc_fsm_bsm_fmp19_state_chk(hs, skb, SNA_DFC_FSM_BSM_IND_BETB, 0)) {
			sna_dfc_fsm_bsm_fmp19_state_chk(hs, skb, SNA_DFC_FSM_BSM_IND_BETB, 1);
			err = 1;
			goto out;
		}
		if (sna_dfc_fsm_chain_rcv_fmp19_state_chk(hs, skb, SNA_DFC_FSM_CHAIN_IND_NONE, 0)) {
			sna_dfc_fsm_chain_rcv_fmp19_state_chk(hs, skb, SNA_DFC_FSM_CHAIN_IND_NONE, 1);
			err = 1;
			goto out;
		}
		if (sna_dfc_fsm_qri_chain_rcv_fmp19_state_chk(hs, skb, 0)) {
			sna_dfc_fsm_qri_chain_rcv_fmp19_state_chk(hs, skb, 1);
			err = 1;
			goto out;
		}
		goto out;
	}

	/* normal flow response. */
	if (rh->rri == SNA_RH_RRI_RSP
		&& fh->efi == SNA_TH_EFI_NORM) {
		if (rh->ru != hs->ct_send.rh.ru) {
			hs->sense = 0x40110000;
			err = 1;
			goto out;
		}
		if (rh->ru == SNA_RH_RU_DFC
			&& hs->ct_send.rq_code != hs->ct_rcv.rq_code) {
			hs->sense = 0x40120000;
			err = 1;
			goto out;
		}
		if (rh->qri != hs->ct_send.rh.qri) {
			hs->sense = 0x40210000;
			err = 1;
			goto out;
		}
		if (rh->rti == SNA_RH_RTI_NEG
			&& sna_dfc_invalid_sense_code(hs, skb)) {
			hs->sense = 0x20120000;
			err = 1;
			goto out;
		}
		if (sna_dfc_fsm_chain_send_fmp19_state_chk(hs, skb, SNA_DFC_FSM_CHAIN_IND_NONE, 0)) {
			sna_dfc_fsm_chain_send_fmp19_state_chk(hs, skb, SNA_DFC_FSM_CHAIN_IND_NONE, 1);
			err = 1;
			goto out;
		}
		goto out;
	}

	/* expedited flow response. */
	if (rh->rri == SNA_RH_RRI_RSP
		&& fh->efi == SNA_TH_EFI_EXP) {
		if (!hs->sig_rq_outstanding) {
			hs->sense = 0x200E0000;
			err = 1;
			goto out;
		}
		goto out;
	}
out:	return err;
}

/**
 * enforce data flow control protocols for received request and responses.
 *
 * @hs: local information.
 * @skb: mu, containing either a response or a normal-flow request.
 *
 * the request or response is sent to rm or ps. data is recorded (from
 * mu to local) for later use before passing the mu to rm or ps.
 */
static int sna_dfc_rcv_fsms(struct sna_hs_cb *hs, struct sk_buff *skb)
{
	sna_rh *rh = sna_transport_header(skb);
	sna_fid2 *fh = sna_network_header(skb);

	sna_debug(5, "init\n");
	if (sna_dfc_rcv_state_error(hs, skb)) {
		kfree_skb(skb);
		goto out;
	}
	if (hs->last_mu)
		kfree_skb(hs->last_mu);
	hs->last_mu = skb_clone(skb, GFP_ATOMIC);
	if (!hs->last_mu) {
		kfree_skb(skb);
		goto out;
	}
	memcpy(&hs->ct_rcv.rh, rh, sizeof(sna_rh));

	/* normal flow request. */
	if (rh->rri == SNA_RH_RRI_REQ
		&& fh->efi == SNA_TH_EFI_NORM) {
		if (!hs->rqd_required_on_ceb) {
			hs->normal_flow_rq_cnt++;
			if (hs->normal_flow_rq_cnt > 16384)
				hs->rqd_required_on_ceb = 1;
		}
		sna_dfc_ct_update(hs, skb);
		if (rh->bbi == SNA_RH_BBI_BB) {
			if (hs->direction == SNA_DFC_DIR_OUTBOUND) {
				if (hs->type == SNA_HS_TYPE_FSP) {
					hs->phs_bb_register.num = ntohs(fh->snf);
					hs->ct_send.snf_who 	= SNA_HS_TYPE_FSP;
				} else {
					hs->shs_bb_register.num	= ntohs(fh->snf);
					hs->ct_send.snf_who	= SNA_HS_TYPE_BIDDER;
				}
			} else {
				if (hs->type == SNA_HS_TYPE_FSP) {
					hs->shs_bb_register.num	= ntohs(fh->snf);
					hs->ct_rcv.snf_who	= SNA_HS_TYPE_BIDDER;
				} else {
					hs->phs_bb_register.num	= ntohs(fh->snf);
					hs->ct_rcv.snf_who	= SNA_HS_TYPE_FSP;
				}
			}
		}
		if (hs->fsm_rcv_purge_fmp19_state
			!= SNA_DFC_FSM_RCV_PURGE_FMP19_STATE_PURGE)
			sna_dfc_generate_rm_ps_inputs(hs, skb);
		else
			kfree_skb(skb);

		sna_dfc_fsm_rcv_purge_fmp19(hs, hs->last_mu, 0);
		if (hs->fsm_chain_send_fmp19_state
			== SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_PEND_RCV_REPLY) {
			sna_dfc_fsm_chain_send_fmp19(hs, hs->last_mu,
				SNA_DFC_FSM_CHAIN_IND_NONE);
		}
		if (sna_transport_header(hs->last_mu)->bci == SNA_RH_BCI_BC)
			sna_dfc_fsm_chain_rcv_fmp19(hs, hs->last_mu, SNA_DFC_FSM_CHAIN_IND_BEGIN);
		if (sna_transport_header(hs->last_mu)->eci == SNA_RH_ECI_EC)
			sna_dfc_fsm_chain_rcv_fmp19(hs, hs->last_mu, SNA_DFC_FSM_CHAIN_IND_END);
		sna_dfc_fsm_qri_chain_rcv_fmp19(hs, hs->last_mu);
		sna_dfc_send_rsp_if_required(hs, hs->last_mu);
		goto out;
	}

	/* normal flow response. */
	if (rh->rri == SNA_RH_RRI_RSP
		&& fh->efi == SNA_TH_EFI_NORM) {
		sna_dfc_ct_update(hs, skb);
		sna_dfc_send_rsp_to_rm_or_ps(hs, skb);
		sna_dfc_fsm_chain_send_fmp19(hs, skb, SNA_DFC_FSM_CHAIN_IND_NONE);
		goto out;
	}

	/* expedited flow response. */
	if (rh->rri == SNA_RH_RRI_RSP
		&& fh->efi == SNA_TH_EFI_EXP) {
		hs->sig_rq_outstanding = 0;
		sna_dfc_send_rsp_to_rm_or_ps(hs, skb);
		goto out;
	}

	/* catch-all, shouldn't get here. */
	sna_debug(5, "caught a stray skb\n");
	kfree_skb(skb);
out:	return 0;
}

/**
 * create and send a response. The response is based on the request MU (if
 * passed by the caller) or on information about the last chain received
 * (if a null MU is passed).
 *
 * @hs: information about the last chain received is used, (local.ct_rcv),
 *      when the input request mu has a null value.
 * @skb: request mu (if any).
 * @expedite: flow type (expedited or normal).
 * @negative: response type (positive or negative).
 * @sense: sense data.
 *
 * a rsp_mu is built and sent to tc.
 *
 * note: if non-null mu is passed the call looses control of that buffer.
 */
static int sna_dfc_send_rsp_mu(struct sna_hs_cb *hs, struct sk_buff *skb,
	u_int8_t expedite, u_int8_t negative, u_int32_t sense)
{
	struct sk_buff *r_skb;
	sna_rh *rh;
	sna_fid2 *fh;
	u_int8_t s_dir;

	sna_debug(5, "init\n");
	if (!skb) {
		r_skb = sna_alloc_skb(150, GFP_ATOMIC);	/* static len for now. */
		if (!r_skb)
			return -ENOMEM;
		sna_dfc_init_th_rh(r_skb);
	} else {
		/* reuse the passed skb. */
		r_skb = skb;
		sna_dfc_init_th_rh(r_skb);
	}
	rh = sna_transport_header(r_skb);
	fh = sna_network_header(r_skb);
	rh->rri      			= SNA_RH_RRI_RSP;
	rh->bci      			= SNA_RH_BCI_BC;
	rh->eci			= SNA_RH_ECI_EC;
	if (negative) {
		rh->rti 		= SNA_RH_RTI_NEG;
		rh->sdi		= SNA_RH_SDI_SD;
		memcpy(skb_put(r_skb, sizeof(u_int32_t)), &sense, sizeof(u_int32_t));
	} else {
		rh->rti		= SNA_RH_RTI_POS;
	}
	if (fh->efi == SNA_TH_EFI_NORM) {
		if (!skb) {
			rh->ru		= hs->ct_rcv.rh.ru;
			rh->dr1i	= hs->ct_rcv.rh.dr1i;
			rh->dr2i	= hs->ct_rcv.rh.dr2i;
			rh->qri	= hs->ct_rcv.rh.qri;
			if (rh->ru == SNA_RH_RU_DFC) {
				sna_debug(5, "FIXME: add the correct rq_code header.\n");
				/* add the rq_code header.
				 *
				 * ?? set the last byte of the rsp_mu.ru to the rq_code
				 * from the correlation table.
				 */
			}
		} else {
			/* we expect the passed buffer to have valid fields, so
			 * there is not much for us to do here.
			 *
			 * ?? set the last byte of the rsp_mu.ru to the rq_code
			 * from the correlation table.
			 */
			sna_debug(5, "FIXME: nothing\n");
		}
	} else {
		sna_ru_sig *sig;
		fh->efi		= SNA_TH_EFI_EXP;
		rh->ru			= SNA_RH_RU_DFC;
		rh->dr1i		= SNA_RH_DR1I_DR1;
		rh->dr2i		= SNA_RH_DR2I_NO_DR2;
		sig = (sna_ru_sig *)skb_put(skb, sizeof(sna_ru_sig));
		sig->rq_code			= SNA_RU_RC_SIG;
		/* ?? set the last byte of the rsp_mu.ru to the rq_code
		 * from the correlation table.
		 */
	}
	if (rh->ru == SNA_RH_RU_DFC)
		rh->fi = SNA_RH_FI_FMH;
	s_dir 		= hs->direction;
	hs->direction 	= SNA_DFC_DIR_OUTBOUND;
	if (!sna_dfc_fsm_chain_rcv_fmp19_state_chk(hs, skb, SNA_DFC_FSM_CHAIN_IND_NONE, 0))
		sna_dfc_send_fsms(hs, skb);
	else
		kfree_skb(r_skb);
	hs->direction	= s_dir;
	return 0;
}

/**
 * determines if a response is stray. (a stray response is one that was sent
 * in a bracket (conversation) but recevied in a different (later bracket).
 *
 * @hs: information about the last request sent, local.current_bracket_sqn,
 *      local.common.rq_code.
 * @skb: mu containing a response.
 *
 * true if stray response; otherwise, false. if stray response represents
 * a response correlation error, local.sense_code is set and a stray-response
 * message is logged.
 */
static int sna_dfc_stray_rsp(struct sna_hs_cb *hs, struct sk_buff *skb)
{
	sna_rh *rh = sna_transport_header(skb);
	sna_fid2 *fh = sna_network_header(skb);
	int err = 0;

	/* hs->sig_rq_outstanding is wrong, but it is a place holder
	 * for us to change it with the proper indication for outstanding
	 * request chain.
	 */
	sna_debug(5, "init\n");
	if (hs->ct_rcv.rq_code == SNA_RU_RC_RTR && hs->sig_rq_outstanding
		&& hs->current_bracket.num != ntohs(fh->snf)) {
		hs->sense = 0x200E0000;
		err = 1;
		goto done;
	}
	if (hs->ct_rcv.rq_code == SNA_RU_RC_SIG
		&& hs->current_bracket.num != ntohs(fh->snf)) {
		err = 1;
		goto done;
	}
	if (hs->ct_rcv.rq_code == SNA_RU_RC_LUSTAT
		|| rh->ru == SNA_RH_RU_FMD) {
		if (!hs->sig_rq_outstanding) {
			err = 1;
			goto done;
		}
		/* if outstanding chain carried BB and the BB SNF does not
		 *  match that in the response.
		 *    Indicate that the response is stray.
		 */
		if (ntohs(fh->snf) != hs->current_bracket.num
			|| hs->fsm_bsm_fmp19_state == SNA_DFC_FSM_BSM_FMP19_STATE_BETB) {
			err = 1;
			goto done;
		}
	}
done:	if (err && rh->rti == SNA_RH_RTI_POS
		&& hs->ct_rcv.rq_code != SNA_RU_RC_SIG)
		hs->sense = 0x200E0000;
	return err;
}

/**
 * perform format checks on expedited-flow responses. These checks are
 * optional.
 *
 * @hs: local information.
 * @skb: mu containing an expedited-flow response.
 *
 * for an error, local.sense_code is set to the appropriate sense data.
 */
static int sna_dfc_format_error_exp_rsp(struct sna_hs_cb *hs, struct sk_buff *skb)
{
	sna_rh *rh = sna_transport_header(skb);

	sna_debug(5, "init\n");
	if (rh->ru != SNA_RH_RU_DFC) {
		hs->sense = 0x40110000;
		goto out;
	}
	if (rh->fi == SNA_RH_FI_NO_FMH) {
		hs->sense = 0x400F0000;
		goto out;
	}
	if ((rh->sdi == SNA_RH_SDI_SD && rh->rti == SNA_RH_RTI_POS)
		|| (rh->sdi == SNA_RH_SDI_NO_SD && rh->rti == SNA_RH_RTI_NEG)) {
		hs->sense = 0x40130000;
		goto out;
	}
	if (rh->bci == SNA_RH_BCI_NO_BC || rh->eci == SNA_RH_ECI_NO_EC) {
		hs->sense = 0x400B0000;
		goto out;
	}
	if (rh->qri == SNA_RH_QRI_QR) {
		hs->sense = 0x40150000;
		goto out;
	}
	if (hs->ct_rcv.rq_code != SNA_CT_RQ_CODE_SIG) {
		hs->sense = 0x40120000;
		goto out;
	}
	if (rh->rti == SNA_RH_RTI_NEG) {
		hs->sense = *((u_int32_t *)skb->data);
		goto out;
	}
out:	return 0;
}

/**
 * perform format checks on normal-flow responses. These checks are optional.
 *
 * @hs: local information.
 * @skb: mu containing a normal-flow response.
 *
 * for an error, local.sense_code is set to the appropriate sense data.
 */
static int sna_dfc_format_error_norm_rsp(struct sna_hs_cb *hs, struct sk_buff *skb)
{
	sna_rh *rh = sna_transport_header(skb);

	sna_debug(5, "init\n");
	if (rh->bci == SNA_RH_BCI_NO_BC || rh->eci == SNA_RH_ECI_NO_EC) {
		hs->sense = 0x400B0000;
		goto out;
	}
	if ((rh->sdi == SNA_RH_SDI_SD && rh->rti == SNA_RH_RTI_POS)
		|| (rh->sdi == SNA_RH_SDI_NO_SD && rh->rti == SNA_RH_RTI_NEG)) {
		hs->sense = 0x40130000;
		goto out;
	}
	if (rh->ru == SNA_RH_RU_DFC && rh->fi == SNA_RH_FI_NO_FMH) {
		hs->sense = 0x400F000;
		goto out;
	}
	if (rh->ru == SNA_RH_RU_FMD && rh->rti == SNA_RH_RTI_POS
		&& rh->fi == SNA_RH_FI_FMH) {
		hs->sense = 0x400F0000;
		goto out;
	}
	if (rh->rti == SNA_RH_RTI_NEG) {
		if (*((u_int32_t *)skb->data) != 0x08130000
			&& *((u_int32_t *)skb->data) != 0x08140000
			&& *((u_int32_t *)skb->data) != 0x08190000
			&& *((u_int32_t *)skb->data) != 0x08460000
			&& *((u_int32_t *)skb->data) != 0x088B0000) {
			hs->sense = *((u_int32_t *)skb->data);
			goto out;
		}
	}
out:	return 0;
}

/**
 * perform format checks on FM data (FMD) requests. The checks are optional.
 *
 * @hs: local information.
 * @skb: mu containing fmd request.
 *
 * for an error, local.sense_code is set to the appropriate sense data.
 */
static int sna_dfc_format_error_req_fmd(struct sna_hs_cb *hs, struct sk_buff *skb)
{
	sna_rh *rh = sna_transport_header(skb);
	sna_fid2 *fh = sna_network_header(skb);
	sna_fmh *fmh_stub;

	sna_debug(5, "init\n");
	fmh_stub = (sna_fmh *)skb->data;
	if (fh->efi == SNA_TH_EFI_EXP) {
		hs->sense = 0x40110000;
		goto out;
	}
	if (!SNA_DFC_FSM_RQD(rh) && !SNA_DFC_FSM_RQE(rh)) {
		hs->sense = 0x40140000;
		goto out;
	}
	if (SNA_DFC_FSM_RQD(rh) && rh->eci == SNA_RH_ECI_NO_EC) {
		hs->sense = 0x40070000;
		goto out;
	}
	if (rh->bbi == SNA_RH_BBI_BB && rh->bci == SNA_RH_BCI_NO_BC) {
		hs->sense = 0x40030000;
		goto out;
	}
	if(rh->bbi == SNA_RH_BBI_BB && rh->ru == SNA_RH_RU_FMD
		&& !(rh->fi == SNA_RH_FI_FMH && ((sna_fmh *)skb->data)->type == SNA_FMH_TYPE_5)) {
		hs->sense = 0x40003000;
		goto out;
	}
	if (rh->csi == SNA_RH_CSI_CODE1 ) {
		hs->sense = 0x40100000;
		goto out;
	}
	if (rh->ebi == SNA_RH_EBI_EB) {
		hs->sense = 0x40040000;
		goto out;
	}
	if (rh->cdi == SNA_RH_CDI_CD && rh->eci == SNA_RH_ECI_NO_EC) {
		hs->sense = 0x40090000;
		goto out;
	}
	if (rh->cdi == SNA_RH_CDI_CD && SNA_DFC_RQD1(rh)) {
		hs->sense = 0x40090000;
		goto out;
	}
	if (rh->cebi == SNA_RH_CEBI_CEB && rh->eci == SNA_RH_ECI_NO_EC) {
		hs->sense = 0x40040000;
		goto out;
	}
	if (rh->bci == SNA_RH_BCI_BC && ((rh->bbi == SNA_RH_BBI_BB
		&& rh->qri == SNA_RH_QRI_NO_QR) || (rh->bbi == SNA_RH_BBI_BB
		|| rh->qri == SNA_RH_QRI_QR))) {
		hs->sense = 0x40180000;
		goto out;
	}
	if (rh->cebi == SNA_RH_CEBI_CEB && rh->cdi == SNA_RH_CDI_CD) {
		hs->sense = 0x40090000;
		goto out;
	}
	if (rh->cebi == SNA_RH_CEBI_CEB
		&& (SNA_DFC_RQE2(rh) || SNA_DFC_RQE3(rh))) {
		hs->sense = 0x40040000;
		goto out;
	}
	if (rh->cebi == SNA_RH_CEBI_NO_CEB && rh->cdi == SNA_RH_CDI_NO_CD
		&& rh->eci == SNA_RH_ECI_EC && SNA_DFC_FSM_RQE(rh)) {
		hs->sense = 0x40190000;
		goto out;
	}
	if (rh->fi == SNA_RH_FI_FMH && rh->cebi == SNA_RH_CEBI_NO_CEB
		&& SNA_DFC_RQD1(rh)) {
		hs->sense = 0x40190000;
		goto out;
	}
	if (rh->bbi == SNA_RH_BBI_BB && rh->cebi == SNA_RH_CEBI_CEB
		&& SNA_DFC_RQE1(rh) && hs->type == SNA_HS_TYPE_FSP) {
		hs->sense = 0x40040000;
		goto out;
	}
#ifdef CONFIG_SNA_LU_STRICT
	/* enabling this check means we won't get fmh7 responses from winnt,
	 * probably others also.
	 */
	if (rh->fi == SNA_RH_FI_FMH && rh->cebi == SNA_RH_CEBI_CEB
		&& fmh_stub->type == SNA_FMH_TYPE_7
		&& rh->rti == SNA_RH_ERI_ON) {
		hs->sense = 0x40060000;
		goto out;
	}
#endif
	if (rh->fi == SNA_RH_FI_FMH && rh->ru == SNA_RH_RU_FMD
		&& (fmh_stub->type != SNA_FMH_TYPE_5
		&& fmh_stub->type != SNA_FMH_TYPE_7)) {
		if (fmh_stub->type == SNA_FMH_TYPE_12) {
			if (rh->eci == SNA_RH_ECI_EC
				&& rh->cebi == SNA_RH_CEBI_NO_CEB)
				hs->sense = 0x080F6051;
		} else
			hs->sense = 0x10084001;
		goto out;
	}
out:	return 0;
}

/**
 * perform format checks for data flow control (DFC) request. These checks
 * are optional.
 *
 * @hs: local information.
 * @skb: mu containing dfc request.
 *
 * if error, local.sense_code is set to the appropriate sense data.
 */
static int sna_dfc_format_error_req_dfc(struct sna_hs_cb *hs, struct sk_buff *skb)
{
	sna_rh *rh = sna_transport_header(skb);
	sna_fid2 *fh = sna_network_header(skb);

	sna_debug(5, "init\n");
	if (fh->efi == SNA_TH_EFI_NORM
		&& (hs->ct_rcv.rq_code != SNA_CT_RQ_CODE_BIS
		&& hs->ct_rcv.rq_code != SNA_CT_RQ_CODE_LUSTAT
		&& hs->ct_rcv.rq_code != SNA_CT_RQ_CODE_RTR)) {
		hs->sense = 0x10030000;
		goto out;
	}
	if (fh->efi == SNA_TH_EFI_EXP && hs->ct_rcv.rq_code != SNA_CT_RQ_CODE_SIG) {
		hs->sense = 0x10030000;
		goto out;
	}
	if (fh->efi == SNA_TH_EFI_EXP
		&& hs->ct_rcv.rq_code == SNA_CT_RQ_CODE_SIG
		&& ((skb->len < sizeof(sna_ru_sig)
		|| (((sna_ru_sig *)skb->data)->signal_code != htons(0x1)
		|| ((sna_ru_sig *)skb->data)->signal_ext != htons(0x1))))) {
		hs->sense = 0x10050000;
		goto out;
	}
	if (rh->fi != SNA_RH_FI_FMH) {
		hs->sense = 0x400F0000;
		goto out;
	}
	if (rh->bci == SNA_RH_BCI_NO_BC || rh->eci == SNA_RH_ECI_NO_EC) {
		hs->sense = 0x400B0000;
		goto out;
	}
	if (rh->csi == SNA_RH_CSI_CODE1) {
		hs->sense = 0x40100000;
		goto out;
	}
	if (rh->edi == SNA_RH_EDI_ED) {
		hs->sense = 0x40160000;
		goto out;
	}
	if (rh->pdi == SNA_RH_PDI_PD) {
		hs->sense = 0x40170000;
		goto out;
	}
	if (hs->ct_rcv.rq_code == SNA_CT_RQ_CODE_LUSTAT) {
		if (skb->len > sizeof(sna_ru_lustat))
			sna_dfc_format_error_req_fmd(hs, skb);
		else {
			if (skb->len < sizeof(sna_ru_lustat))
				hs->sense = 0x10050000;
		}
		goto out;
	} else {
		if ((hs->ct_rcv.rq_code == SNA_CT_RQ_CODE_BIS
			&& (SNA_DFC_RQD2(rh) || SNA_DFC_RQD3(rh)))
			|| (hs->ct_rcv.rq_code != SNA_CT_RQ_CODE_BIS
			&& !SNA_DFC_RQD1(rh))) {
			hs->sense = 0x40140000;
			goto out;
		}
		if (rh->qri == SNA_RH_QRI_QR) {
			hs->sense = 0x40150000;
			goto out;
		}
		if (rh->bbi == SNA_RH_BBI_BB || rh->ebi == SNA_RH_EBI_EB
			|| rh->cebi == SNA_RH_CEBI_CEB) {
			hs->sense = 0x400C0000;
			goto out;
		}
		if (rh->cdi == SNA_RH_CDI_CD) {
			hs->sense = 0x40090000;
			goto out;
		}
		goto out;
	}
out:	return 0;
}

/**
 * perform format checks on all requests and responses for LU-LU session.
 * These checks are optional. If an error is detected, the local->sense
 * is set to the appropriate sense data. None, some, or all of these checks
 * may be done.
 *
 * @hs:
 * @skb: mu containing a request or a response.
 *
 * true for format error detected; otherwise false.
 */
static int sna_dfc_format_error(struct sna_hs_cb *hs, struct sk_buff *skb)
{
	sna_rh *rh = sna_transport_header(skb);
	sna_fid2 *fh = sna_network_header(skb);
	int err = 0;

	sna_debug(5, "init %08X\n", hs->sense);
	if (rh->rri == SNA_RH_RRI_REQ) {
		if (rh->ru == SNA_RH_RU_FMD)
			sna_dfc_format_error_req_fmd(hs, skb);
		if (rh->ru == SNA_RH_RU_DFC)
			sna_dfc_format_error_req_dfc(hs, skb);
	} else {
		if (fh->efi == SNA_TH_EFI_NORM)
			sna_dfc_format_error_norm_rsp(hs, skb);
		if (fh->efi == SNA_TH_EFI_EXP)
			sna_dfc_format_error_exp_rsp(hs, skb);
	}
	if (hs->sense)
		err = 1;
	return err;
}

/**
 * process MUs received from TC. This procedure is called by TC.
 *
 * @hs: local.common_rq_code; indication whether the alternate code may
 *      be used, local.alternate_code.
 * @skb: mu, containing either a request (normal or expedited) or a
 *       response.
 *
 * local.sig_received is set if signal is received; the snf of the signal
 * is saved in local.sig_snf; local.direction is set.
 */
int sna_dfc_rx(struct sna_hs_cb *hs, struct sk_buff *skb)
{
	sna_rh *rh = sna_transport_header(skb);
	sna_fid2 *fh = sna_network_header(skb);

	sna_debug(5, "init\n");
	hs->direction = SNA_DFC_DIR_INBOUND;
	if (!hs->sense) {
		if (rh->ru == SNA_RH_RU_DFC) {
			hs->ct_rcv.rq_code = skb->data[0];
			sna_debug(5, "RQ_CODE=%02X len=%d\n", skb->data[0], skb->len);
			if (hs->ct_rcv.rq_code != SNA_RU_RC_CRV
				&& hs->ct_rcv.rq_code != SNA_RU_RC_BIS
				&& hs->ct_rcv.rq_code != SNA_RU_RC_LUSTAT
				&& hs->ct_rcv.rq_code != SNA_RU_RC_RTR
				&& hs->ct_rcv.rq_code != SNA_RU_RC_SIG)
				hs->ct_rcv.rq_code = SNA_CT_RQ_CODE_OTHER;
		} else
			hs->ct_rcv.rq_code = SNA_CT_RQ_CODE_OTHER;
	}
	if (sna_dfc_format_error(hs, skb)) {
		sna_debug(5, "format error found: 0x%08X\n", hs->sense);
		kfree_skb(skb);
		return -EINVAL;
	}
	if (rh->rri == SNA_RH_RRI_REQ) {
		if (fh->efi == SNA_TH_EFI_NORM)
			sna_dfc_rcv_fsms(hs, skb);
		else {
			hs->sig_received 	= 1;
			hs->sig_snf_sqn		= ntohs(fh->snf);
			sna_dfc_send_rsp_mu(hs, skb, 1, 0, 0);
		}
	} else {
		if (!sna_dfc_stray_rsp(hs, skb))
			sna_dfc_rcv_fsms(hs, skb);
		else
			kfree_skb(skb);
	}
	return 0;
}

/**
 * determine whether or not a request is a valid reply. a reply is a request
 * sent (or received) after receiving (or sending) an (RQE,CD) request.
 *
 * @hs: local.direction; local.current_bracket_sqn; information about the
 *      last chain sent, local.ct_send.
 * @skb: mu containing a normal-flow request.
 *
 * true if valid reply; otherwise, false.
 */
static int sna_dfc_ok_to_reply(struct sna_hs_cb *hs, struct sk_buff *skb)
{
	sna_rh *rh = sna_transport_header(skb);

	sna_debug(5, "init\n");
	if (skb->data[0] == SNA_RU_RC_BIS || skb->data[0] == SNA_RU_RC_RTR)
		return 0;
	if (rh->bbi == SNA_RH_BBI_BB && rh->bci == SNA_RH_BCI_NO_BC)
		return 0;
	if (hs->direction == SNA_DFC_DIR_OUTBOUND
		&& hs->fsm_chain_rcv_fmp19_state != SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_PEND_SEND_REPLY)
		return 0;
	if (hs->direction == SNA_DFC_DIR_INBOUND
		&& hs->fsm_chain_send_fmp19_state != SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_PEND_RCV_REPLY)
		return 0;
	if (hs->direction == SNA_DFC_DIR_INBOUND
		&& hs->fsm_bsm_fmp19_state == SNA_DFC_FSM_BSM_FMP19_STATE_INB
		&& hs->ct_send.rh.bbi == SNA_RH_BBI_BB
		&& hs->current_bracket.num != hs->ct_send.snf)
		return 0;
	return 1;
}

/**
 * indicate state-check. enfore the bracket protocol. state transitions are
 * forced via the input signals inb (go in brackets) and betb (go
 * between brackets). the inputs r, rq, ... are used for error checking only.
 * inb state means dfc (the half-session) is connected to a ps; betb state
 * means dfc is not connected to a ps.
 *
 * @hs: local information.
 * @skb: mu.
 * @signal: signal that the fsm should be set to the specified state.
 * @output: execute output function if state check is found.
 *
 * true if state-check is found, otherwise false. if an error is discoveredd,
 * and output is enabled local.sense_code is set.
 */
static int sna_dfc_fsm_bsm_fmp19_state_chk(struct sna_hs_cb *hs, struct sk_buff *skb,
	int signal, int output)
{
	sna_rh *rh = sna_transport_header(skb);
	int err = 0;

	if (hs->direction == SNA_DFC_DIR_INBOUND
		&& rh->rri == SNA_RH_RRI_REQ) {
		/* R, RQ, (FMD|LUSTAT), NOT_BID_REPLY, !FMH5, !FMH12, !CEB_UNCOND. */
		if ((rh->bci == SNA_RH_BBI_NO_BB
			&& (hs->ct_send.rh.bbi != SNA_RH_BBI_BB || !sna_dfc_ok_to_reply(hs, skb)))
			&& !(rh->cebi == SNA_RH_CEBI_CEB
			&& (SNA_DFC_RQD1(rh) || SNA_DFC_RQE1(rh)))) {
			if ((rh->fi == SNA_RH_FI_FMH
				&& ((sna_fmh *)skb->data)->type != SNA_FMH_TYPE_5
				&& ((sna_fmh *)skb->data)->type != SNA_FMH_TYPE_12)
				|| skb->data[0] == SNA_RU_RC_LUSTAT) {
				if (hs->fsm_bsm_fmp19_state == SNA_DFC_FSM_BSM_FMP19_STATE_BETB) {
					if (output)
						hs->sense = 0x20030000;
					err = 1;
					goto out;
				}
			}
			goto out;
		}
		/* R, RQ, FMD, NOT_BID_REPLY, !FMH5, !FMH12, CEB_UNCOND. */
		if (rh->fi == SNA_RH_FI_FMH && (rh->bci == SNA_RH_BBI_NO_BB
			&& (hs->ct_send.rh.bbi != SNA_RH_BBI_BB || !sna_dfc_ok_to_reply(hs, skb)))
			&& ((sna_fmh *)skb->data)->type != SNA_FMH_TYPE_5
			&& ((sna_fmh *)skb->data)->type != SNA_FMH_TYPE_12
			&& (rh->cebi == SNA_RH_CEBI_CEB
			&& (SNA_DFC_RQD1(rh) || SNA_DFC_RQE1(rh)))) {
			if (hs->fsm_bsm_fmp19_state == SNA_DFC_FSM_BSM_FMP19_STATE_BETB) {
				if (output)
					hs->sense = 0x20030000;
				err = 1;
				goto out;
			}
		}
		goto out;
	}
out:    return err;
}

/**
 * indicate state-check. enfore the bracket protocol. state transitions are
 * forced via the input signals inb (go in brackets) and betb (go
 * between brackets). the inputs r, rq, ... are used for error checking only.
 * inb state means dfc (the half-session) is connected to a ps; betb state
 * means dfc is not connected to a ps.
 *
 * @hs: local information.
 * @skb: mu.
 * @signal: signal that the fsm should be set to the specified state.
 *
 * if an error is discovered, local.sense_code is set.
 */
static int sna_dfc_fsm_bsm_fmp19(struct sna_hs_cb *hs, struct sk_buff *skb,
	int signal)
{
	sna_rh *rh = sna_transport_header(skb);

	/* SIGNAL(INB). */
	if (signal == SNA_DFC_FSM_BSM_IND_INB) {
		hs->fsm_bsm_fmp19_state = SNA_DFC_FSM_BSM_FMP19_STATE_INB;
		goto out;
	}
	/* SIGNAL(BETB). */
	if (signal == SNA_DFC_FSM_BSM_IND_BETB) {
		hs->fsm_bsm_fmp19_state = SNA_DFC_FSM_BSM_FMP19_STATE_BETB;
		goto out;
	}
	if (hs->direction == SNA_DFC_DIR_INBOUND
		&& rh->rri == SNA_RH_RRI_REQ) {
		/* R, RQ, (FMD|LUSTAT), NOT_BID_REPLY, !FMH5, !FMH12, !CEB_UNCOND. */
		if ((rh->bci == SNA_RH_BBI_NO_BB
			&& (hs->ct_send.rh.bbi != SNA_RH_BBI_BB || !sna_dfc_ok_to_reply(hs, skb)))
			&& !(rh->cebi == SNA_RH_CEBI_CEB
			&& (SNA_DFC_RQD1(rh) || SNA_DFC_RQE1(rh)))) {
			if ((rh->fi == SNA_RH_FI_FMH
				&& ((sna_fmh *)skb->data)->type != SNA_FMH_TYPE_5
				&& ((sna_fmh *)skb->data)->type != SNA_FMH_TYPE_12)
				|| skb->data[0] == SNA_RU_RC_LUSTAT) {
				if (hs->fsm_bsm_fmp19_state == SNA_DFC_FSM_BSM_FMP19_STATE_BETB) {
					hs->sense = 0x20030000;
					goto out;
				}
			}
			goto out;
		}
		/* R, RQ, FMD, NOT_BID_REPLY, !FMH5, !FMH12, CEB_UNCOND. */
		if (rh->fi == SNA_RH_FI_FMH && (rh->bci == SNA_RH_BBI_NO_BB
			&& (hs->ct_send.rh.bbi != SNA_RH_BBI_BB || !sna_dfc_ok_to_reply(hs, skb)))
			&& ((sna_fmh *)skb->data)->type != SNA_FMH_TYPE_5
			&& ((sna_fmh *)skb->data)->type != SNA_FMH_TYPE_12
			&& (rh->cebi == SNA_RH_CEBI_CEB
			&& (SNA_DFC_RQD1(rh) || SNA_DFC_RQE1(rh)))) {
			if (hs->fsm_bsm_fmp19_state == SNA_DFC_FSM_BSM_FMP19_STATE_BETB) {
				hs->sense = 0x20030000;
				goto out;
			}
		}
		goto out;
	}
out:	return 0;
}

static int sna_dfc_fsm_chain_rcv_fmp19_output_a(struct sna_hs_cb *hs,
	struct sk_buff *skb)
{
	/* this function could be more complete. */
	sna_debug(5, "init\n");
	if (hs->ct_rcv.rh.bbi == SNA_RH_BBI_BB)
		goto out;
	sna_dfc_fsm_bsm_fmp19(hs, skb, SNA_DFC_FSM_BSM_IND_BETB);
	/* hs->bracket_id = 0; */
	hs->ct_rcv.entry_present = 0;
out:	return 0;
}

static int sna_dfc_fsm_chain_rcv_fmp19_output_b(struct sna_hs_cb *hs,
	struct sk_buff *skb)
{
	sna_debug(5, "init\n");
	hs->ct_rcv.entry_present = 0;
	return 0;
}

static int sna_dfc_fsm_chain_rcv_fmp19_output_r1(struct sna_hs_cb *hs,
	struct sk_buff *skb)
{
	sna_debug(5, "init\n");
	hs->sense = 0x20020000;		/* chaining error. */
	return 0;
}

static int sna_dfc_fsm_chain_rcv_fmp19_output_r2(struct sna_hs_cb *hs,
	struct sk_buff *skb)
{
	sna_debug(5, "init\n");
	hs->sense = 0x200A0000;		/* immediate request mode error. */
	return 0;
}

static int sna_dfc_fsm_chain_rcv_fmp19_output_r3(struct sna_hs_cb *hs,
	struct sk_buff *skb)
{
	sna_debug(5, "init\n");
	hs->sense = 0x2004000;		/* half-duplex error. */
	return 0;
}

/**
 * enforce the chaining protocol for received chains. A chain is "complete"
 * when the end-of-chain (EC) request has been received and any required
 * associated response or reply has been sent. A reply is a request sent after
 * receiving an (RQE, CD) chain that has not been negatively responsed to. A
 * reply implies a positive response to the (RQE, CD) chain.
 *
 * @hs: information about the last received request.
 * @skb: mu.
 * @chain: chain_indicator (posible values are begin_chain, end_chain,
 *         and not_specified).
 * @output: execute output function if state check is found.
 *
 * true if state-check is found, otherwise false. if an error is discoveredd,
 * and output is enabled local.sense_code is set.
 */
static int sna_dfc_fsm_chain_rcv_fmp19_state_chk(struct sna_hs_cb *hs,
	struct sk_buff *skb, int chain, int output)
{
	sna_rh *rh = sna_transport_header(skb);
	int err = 0;

	sna_debug(5, "init\n");
	if (hs->direction == SNA_DFC_DIR_INBOUND) { /* R. */
		/* R, RQ, BEGIN_CHAIN. */
		if (rh->rri == SNA_RH_RRI_REQ
			&& chain == SNA_DFC_FSM_CHAIN_IND_BEGIN) {
			goto out;
		}
		/* R, RQ, END_CHAIN. */
		if (rh->rri == SNA_RH_RRI_REQ
			&& chain == SNA_DFC_FSM_CHAIN_IND_END) {
			/* RQD. */
			if (SNA_DFC_FSM_RQD(rh))
				goto out;
			/* RQE, CEB. */
			if (SNA_DFC_FSM_RQE(rh) && rh->cebi)
				goto out;
			/* RQE, CD. */
			if (SNA_DFC_FSM_RQE(rh) && rh->cdi)
				goto out;
			/* BIS. */
			if (skb->data[0] == SNA_RU_RC_BIS)
				goto out;
			goto out;
		}
		/* R, RQ, BC. */
		if (rh->rri == SNA_RH_RRI_REQ && rh->bci) {
			if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_INC) {
				if (output)
					sna_dfc_fsm_chain_rcv_fmp19_output_r1(hs, skb);
				err = 1;
				goto out;
			}
			if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_NEG_RSP_SENT) {
				if (output)
					sna_dfc_fsm_chain_rcv_fmp19_output_r1(hs, skb);
				err = 1;
				goto out;
			}
			if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_PEND_RSP) {
				if (output)
					sna_dfc_fsm_chain_rcv_fmp19_output_r2(hs, skb);
				err = 1;
				goto out;
			}
			if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_PEND_SEND_REPLY) {
				if (output)
					sna_dfc_fsm_chain_rcv_fmp19_output_r3(hs, skb);
				err = 1;
				goto out;
			}
			goto out;
		}
		/* R, RQ, !BC. */
		if (rh->rri == SNA_RH_RRI_REQ && rh->eci) {
			if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_BETC) {
				if (output)
					sna_dfc_fsm_chain_rcv_fmp19_output_r1(hs, skb);
				err = 1;
				goto out;
			}
			if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_PEND_RSP) {
				if (output)
					sna_dfc_fsm_chain_rcv_fmp19_output_r2(hs, skb);
				err = 1;
				goto out;
			}
			if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_PEND_SEND_REPLY) {
				if (output)
					sna_dfc_fsm_chain_rcv_fmp19_output_r1(hs, skb);
				err = 1;
				goto out;
			}
			goto out;
		}
	} else { /* S. */
		/* S, -RSP, (FMD|LUSTAT). */
		if (rh->rri == SNA_RH_RRI_RSP && rh->rti == SNA_RH_RTI_NEG
			&& (rh->fi == SNA_RH_FI_FMH || skb->data[0] == SNA_RU_RC_LUSTAT)) {
			if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_BETC) {
				err = 1;
				goto out;
			}
			if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_NEG_RSP_SENT) {
				err = 1;
				goto out;
			}
			goto out;
		}
		/* S, +RSP, (FMD|LUSTAT). */
		if (rh->rri == SNA_RH_RRI_RSP && rh->rti == SNA_RH_RTI_NEG
			&& (rh->fi == SNA_RH_FI_FMH || skb->data[0] == SNA_RU_RC_LUSTAT)) {
			if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_BETC) {
				err = 1;
				goto out;
			}
			if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_NEG_RSP_SENT) {
				err = 1;
				goto out;
			}
			goto out;
		}
		/* S, oRSP, RTR. */
		if (rh->rri == SNA_RH_RRI_RSP && skb->data[0] == SNA_RU_RC_RTR) {
			goto out;
		}
		/* S, RQ, REPLY. */
		if (rh->rri == SNA_RH_RRI_REQ && sna_dfc_ok_to_reply(hs, skb)) {
			goto out;
		}
	}
out:	return err;
}

/**
 * enforce the chaining protocol for received chains. A chain is "complete"
 * when the end-of-chain (EC) request has been received and any required
 * associated response or reply has been sent. A reply is a request sent after
 * receiving an (RQE, CD) chain that has not been negatively responsed to. A
 * reply implies a positive response to the (RQE, CD) chain.
 *
 * @hs: information about the last received request.
 * @skb: mu.
 * @chain: chain_indicator (posible values are begin_chain, end_chain,
 *         and not_specified).
 *
 * if the bracket was ended by the request; the hs will be disconnected from
 * ps; information recorded about the last received request may be erased;
 * local.sense_code may be set.
 */
static int sna_dfc_fsm_chain_rcv_fmp19(struct sna_hs_cb *hs,
	struct sk_buff *skb, int chain)
{
	sna_rh *rh = sna_transport_header(skb);

	sna_debug(5, "init\n");
	if (hs->direction == SNA_DFC_DIR_INBOUND) { /* R. */
		/* R, RQ, BEGIN_CHAIN. */
		if (rh->rri == SNA_RH_RRI_REQ
			&& chain == SNA_DFC_FSM_CHAIN_IND_BEGIN) {
			if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_BETC)
				hs->fsm_chain_rcv_fmp19_state = SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_INC;
			goto out;
		}
		/* R, RQ, END_CHAIN. */
		if (rh->rri == SNA_RH_RRI_REQ
			&& chain == SNA_DFC_FSM_CHAIN_IND_END) {
			/* RQD. */
			if (SNA_DFC_FSM_RQD(rh)) {
				if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_INC) {
					hs->fsm_chain_rcv_fmp19_state = SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_PEND_RSP;
					goto out;
				}
				if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_NEG_RSP_SENT) {
					hs->fsm_chain_rcv_fmp19_state = SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_BETC;
					sna_dfc_fsm_chain_rcv_fmp19_output_a(hs, skb);
					goto out;
				}
				goto out;
			}
			/* RQE, CEB. */
			if (SNA_DFC_FSM_RQE(rh) && rh->cebi) {
				if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_INC
					|| hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_NEG_RSP_SENT) {
					hs->fsm_chain_rcv_fmp19_state = SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_BETC;
					sna_dfc_fsm_chain_rcv_fmp19_output_a(hs, skb);
					goto out;
				}
				goto out;
			}
			/* RQE, CD. */
			if (SNA_DFC_FSM_RQE(rh) && rh->cdi) {
				if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_INC) {
					hs->fsm_chain_rcv_fmp19_state = SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_PEND_SEND_REPLY;
					goto out;
				}
				if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_NEG_RSP_SENT) {
					hs->fsm_chain_rcv_fmp19_state = SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_BETC;
					sna_dfc_fsm_chain_rcv_fmp19_output_b(hs, skb);
					goto out;
				}
				goto out;
			}

			/* BIS. */
			if (skb->data[0] == SNA_RU_RC_BIS) {
				if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_INC) {
					hs->fsm_chain_rcv_fmp19_state = SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_BETC;
					goto out;
				}
				goto out;
			}
			goto out;
		}
		/* R, RQ, BC. */
		if (rh->rri == SNA_RH_RRI_REQ && rh->bci) {
			if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_INC) {
				sna_dfc_fsm_chain_rcv_fmp19_output_r1(hs, skb);
				goto out;
			}
			if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_NEG_RSP_SENT) {
				sna_dfc_fsm_chain_rcv_fmp19_output_r1(hs, skb);
				goto out;
			}
			if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_PEND_RSP) {
				sna_dfc_fsm_chain_rcv_fmp19_output_r2(hs, skb);
				goto out;
			}
			if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_PEND_SEND_REPLY) {
				sna_dfc_fsm_chain_rcv_fmp19_output_r3(hs, skb);
				goto out;
			}
			goto out;
		}
		/* R, RQ, !BC. */
		if (rh->rri == SNA_RH_RRI_REQ && rh->eci) {
			if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_BETC) {
				sna_dfc_fsm_chain_rcv_fmp19_output_r1(hs, skb);
				goto out;
			}
			if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_PEND_RSP) {
				sna_dfc_fsm_chain_rcv_fmp19_output_r2(hs, skb);
				goto out;
			}
			if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_PEND_SEND_REPLY) {
				sna_dfc_fsm_chain_rcv_fmp19_output_r1(hs, skb);
				goto out;
			}
			goto out;
		}
	} else { /* S. */
		/* S, -RSP, (FMD|LUSTAT). */
		if (rh->rri == SNA_RH_RRI_RSP && rh->rti == SNA_RH_RTI_NEG
			&& (rh->fi == SNA_RH_FI_FMH || skb->data[0] == SNA_RU_RC_LUSTAT)) {
			if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_INC) {
				hs->fsm_chain_rcv_fmp19_state = SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_NEG_RSP_SENT;
				goto out;
			}
			if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_PEND_RSP) {
				hs->fsm_chain_rcv_fmp19_state = SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_BETC;
				sna_dfc_fsm_chain_rcv_fmp19_output_a(hs, skb);
				goto out;
			}
			if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_PEND_SEND_REPLY) {
				hs->fsm_chain_rcv_fmp19_state = SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_BETC;
				sna_dfc_fsm_chain_rcv_fmp19_output_a(hs, skb);
				goto out;
			}
			goto out;
		}
		/* S, +RSP, (FMD|LUSTAT). */
		if (rh->rri == SNA_RH_RRI_RSP && rh->rti == SNA_RH_RTI_NEG
			&& (rh->fi == SNA_RH_FI_FMH || skb->data[0] == SNA_RU_RC_LUSTAT)) {
			if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_PEND_RSP) {
				hs->fsm_chain_rcv_fmp19_state = SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_BETC;
				sna_dfc_fsm_chain_rcv_fmp19_output_a(hs, skb);
				goto out;
			}
			goto out;
		}
		/* S, oRSP, RTR. */
		if (rh->rri == SNA_RH_RRI_RSP && skb->data[0] == SNA_RU_RC_RTR) {
			if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_PEND_RSP) {
				hs->fsm_chain_rcv_fmp19_state = SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_BETC;
				goto out;
			}
			goto out;
		}
		/* S, RQ, REPLY. */
		if (rh->rri == SNA_RH_RRI_REQ && sna_dfc_ok_to_reply(hs, skb)) {
			if (hs->fsm_chain_rcv_fmp19_state == SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_PEND_SEND_REPLY) {
				hs->fsm_chain_rcv_fmp19_state = SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_BETC;
				goto out;
			}
			goto out;
		}
	}
out:	return 0;
}

static int sna_dfc_fsm_chain_send_fmp19_output_a(struct sna_hs_cb *hs,
	struct sk_buff *skb)
{
	/* this function could be more complete. */
	sna_debug(5, "init\n");
	if (hs->ct_send.rh.bbi == SNA_RH_BBI_BB)
		goto out;
	sna_dfc_fsm_bsm_fmp19(hs, skb, SNA_DFC_FSM_BSM_IND_BETB);
	/* hs->bracket_id = 0; */
	/* set correlation entry to indicate no request chain outstanding. */
	sna_debug(5, "FIXME: set correlation entry to indicate no request chain outstanding.\n");
out:	return 0;
}

static int sna_dfc_fsm_chain_send_fmp19_output_b(struct sna_hs_cb *hs,
	struct sk_buff *skb)
{
	sna_debug(5, "init\n");
	/* set correlation entry to indicate no request chain outstanding. */
	sna_debug(5, "FIXME: set correlation entry to indicate no request chain outstanding.\n");
	return 0;
}

static int sna_dfc_fsm_chain_send_fmp19_output_r(struct sna_hs_cb *hs,
	struct sk_buff *skb)
{
	sna_debug(5, "init\n");
	hs->sense = 0x200F0000;		/* response protocol error. */
	return 0;
}

/**
 * enforce the chaining protocol for sending chains. A chain is "complete"
 * when the end-of-chain (EC) request has been sent and any required associated
 * response or reply has been received. A reply is a request recevied after
 * sending an (RQE, CD) chain that has not received a negative response. A
 * reply implies a positive response to the (RQE, CD) chain.
 *
 * @hs: information about the last received request.
 * @skb: mu.
 * @chain: chain_indicator (possible values are begin_chain, end_chain
 *         and not_specified).
 * @output: execute output function if state check is found.
 *
 * true if state-check is found, otherwise false. if an error is discoveredd,
 * and output is enabled local.sense_code is set.
 */
static int sna_dfc_fsm_chain_send_fmp19_state_chk(struct sna_hs_cb *hs,
	struct sk_buff *skb, int chain, int output)
{
	sna_rh *rh = sna_transport_header(skb);
	int err = 0;

	sna_debug(5, "init\n");
	if (hs->direction == SNA_DFC_DIR_INBOUND) { /* R. */
		/* R, -RSP, (FMD|LUSTAT). */
		if (rh->rri == SNA_RH_RRI_RSP && rh->rti == SNA_RH_RTI_NEG
			&& (rh->fi == SNA_RH_FI_FMH || skb->data[0] == SNA_RU_RC_LUSTAT)) {
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_BETC) {
				if (output)
					sna_dfc_fsm_chain_send_fmp19_output_r(hs, skb);
				err = 1;
				goto out;
			}
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_NET_RSP_RCVD) {
				if (output)
					sna_dfc_fsm_chain_send_fmp19_output_r(hs, skb);
				err = 1;
				goto out;
			}
			goto out;
		}
		/* R, +RSP, (FMD|LUSTAT). */
		if (rh->rri == SNA_RH_RRI_RSP && rh->rti == SNA_RH_RTI_POS
			&& (rh->fi == SNA_RH_FI_FMH || skb->data[0] == SNA_RU_RC_LUSTAT)) {
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_BETC) {
				if (output)
					sna_dfc_fsm_chain_send_fmp19_output_r(hs, skb);
				err = 1;
				goto out;
			}
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_INC) {
				if (output)
					sna_dfc_fsm_chain_send_fmp19_output_r(hs, skb);
				err = 1;
				goto out;
			}
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_NET_RSP_RCVD) {
				if (output)
					sna_dfc_fsm_chain_send_fmp19_output_r(hs, skb);
				err = 1;
				goto out;
			}
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_PEND_RCV_REPLY) {
				if (output)
					sna_dfc_fsm_chain_send_fmp19_output_r(hs, skb);
				err = 1;
				goto out;
			}
			goto out;
		}
		/* R, oRSP, RTR. */
		if (rh->rri == SNA_RH_RRI_RSP && skb->data[0] == SNA_RU_RC_RTR) {
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_BETC) {
				if (output)
					sna_dfc_fsm_chain_send_fmp19_output_r(hs, skb);
				err = 1;
				goto out;
			}
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_INC) {
				if (output)
					sna_dfc_fsm_chain_send_fmp19_output_r(hs, skb);
				err = 1;
				goto out;
			}
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_NET_RSP_RCVD) {
				if (output)
					sna_dfc_fsm_chain_send_fmp19_output_r(hs, skb);
				err = 1;
				goto out;
			}
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_PEND_RCV_REPLY) {
				if (output)
					sna_dfc_fsm_chain_send_fmp19_output_r(hs, skb);
				err = 1;
				goto out;
			}
			goto out;
		}
	}
out:	return err;
}

/**
 * enforce the chaining protocol for sending chains. A chain is "complete"
 * when the end-of-chain (EC) request has been sent and any required associated
 * response or reply has been received. A reply is a request recevied after
 * sending an (RQE, CD) chain that has not received a negative response. A
 * reply implies a positive response to the (RQE, CD) chain.
 *
 * @hs: information about the last received request.
 * @skb: mu.
 * @chain: chain_indicator (possible values are begin_chain, end_chain
 *         and not_specified).
 *
 * if the bracket was ended by the request, the hs will be disconnected from
 * ps; information recorded about the last received request may be erased;
 * local.sense_code may be set.
 */
static int sna_dfc_fsm_chain_send_fmp19(struct sna_hs_cb *hs,
	struct sk_buff *skb, int chain)
{
	sna_rh *rh = sna_transport_header(skb);

	sna_debug(5, "init: direction:%d state:%d\n", hs->direction,
		hs->fsm_chain_send_fmp19_state);
	if (hs->direction == SNA_DFC_DIR_INBOUND) { /* R. */
		/* R, -RSP, (FMD|LUSTAT). */
		if (rh->rri == SNA_RH_RRI_RSP && rh->rti == SNA_RH_RTI_NEG
			&& (rh->fi == SNA_RH_FI_FMH || skb->data[0] == SNA_RU_RC_LUSTAT)) {
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_BETC) {
				sna_dfc_fsm_chain_send_fmp19_output_r(hs, skb);
				goto out;
			}
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_INC) {
				hs->fsm_chain_send_fmp19_state = SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_NET_RSP_RCVD;
				goto out;
			}
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_NET_RSP_RCVD) {
				sna_dfc_fsm_chain_send_fmp19_output_r(hs, skb);
				goto out;
			}
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_PEND_RSP) {
				hs->fsm_chain_send_fmp19_state = SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_BETC;
				sna_dfc_fsm_chain_send_fmp19_output_a(hs, skb);
				goto out;
			}
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_PEND_RCV_REPLY) {
				hs->fsm_chain_send_fmp19_state = SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_BETC;
				sna_dfc_fsm_chain_send_fmp19_output_a(hs, skb);
				goto out;
			}
			goto out;
		}
		/* R, +RSP, (FMD|LUSTAT). */
		if (rh->rri == SNA_RH_RRI_RSP && rh->rti == SNA_RH_RTI_POS
			&& (rh->fi == SNA_RH_FI_FMH || skb->data[0] == SNA_RU_RC_LUSTAT)) {
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_BETC) {
				sna_dfc_fsm_chain_send_fmp19_output_r(hs, skb);
				goto out;
			}
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_INC) {
				sna_dfc_fsm_chain_send_fmp19_output_r(hs, skb);
				goto out;
			}
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_NET_RSP_RCVD) {
				sna_dfc_fsm_chain_send_fmp19_output_r(hs, skb);
				goto out;
			}
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_PEND_RSP) {
				hs->fsm_chain_send_fmp19_state = SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_BETC;
				sna_dfc_fsm_chain_send_fmp19_output_a(hs, skb);
				goto out;
			}
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_PEND_RCV_REPLY) {
				sna_dfc_fsm_chain_send_fmp19_output_r(hs, skb);
				goto out;
			}
			goto out;
		}
		/* R, oRSP, RTR. */
		if (rh->rri == SNA_RH_RRI_RSP && skb->data[0] == SNA_RU_RC_RTR) {
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_BETC) {
				sna_dfc_fsm_chain_send_fmp19_output_r(hs, skb);
				goto out;
			}
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_INC) {
				sna_dfc_fsm_chain_send_fmp19_output_r(hs, skb);
				goto out;
			}
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_NET_RSP_RCVD) {
				sna_dfc_fsm_chain_send_fmp19_output_r(hs, skb);
				goto out;
			}
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_PEND_RSP) {
				hs->fsm_chain_send_fmp19_state = SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_BETC;
				sna_dfc_fsm_chain_send_fmp19_output_b(hs, skb);
				goto out;
			}
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_PEND_RCV_REPLY) {
				sna_dfc_fsm_chain_send_fmp19_output_r(hs, skb);
				goto out;
			}
			goto out;
		}
		/* R, RQ, REPLY. */
		if (rh->rri == SNA_RH_RRI_REQ && sna_dfc_ok_to_reply(hs, skb)) {
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_PEND_RCV_REPLY) {
				hs->fsm_chain_send_fmp19_state = SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_BETC;
				sna_dfc_fsm_chain_send_fmp19_output_b(hs, skb);
				goto out;
			}
			goto out;
		}
	} else { /* S. */
		if (rh->rri == SNA_RH_RRI_RSP)
			goto out;
		/* S, RQ, BEGIN_CHAIN. */
		if (chain == SNA_DFC_FSM_CHAIN_IND_BEGIN) {
			if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_BETC) {
				hs->fsm_chain_send_fmp19_state = SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_INC;
				goto out;
			}
			goto out;
		}
		if (chain == SNA_DFC_FSM_CHAIN_IND_END) {
			/* S, RQ, END_CHAIN, RQD. */
			if (SNA_DFC_FSM_RQD(rh)) {
				if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_INC) {
					hs->fsm_chain_send_fmp19_state = SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_PEND_RSP;
					goto out;
				}
				if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_NET_RSP_RCVD) {
					hs->fsm_chain_send_fmp19_state = SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_BETC;
					sna_dfc_fsm_chain_send_fmp19_output_a(hs, skb);
					goto out;
				}
				goto out;
			}
			/* S, RQ, END_CHAIN, RQE, CEB. */
			if (SNA_DFC_FSM_RQE(rh) && rh->cebi) {
				if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_INC) {
					hs->fsm_chain_send_fmp19_state = SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_BETC;
					sna_dfc_fsm_chain_send_fmp19_output_a(hs, skb);
					goto out;
				}
				if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_NET_RSP_RCVD) {
					hs->fsm_chain_send_fmp19_state = SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_BETC;
					sna_dfc_fsm_chain_send_fmp19_output_a(hs, skb);
					goto out;
				}
				goto out;
			}
			/* S, RQ, END_CHAIN, RQE, CD. */
			if (SNA_DFC_FSM_RQE(rh) && rh->cdi) {
				if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_INC) {
					hs->fsm_chain_send_fmp19_state = SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_PEND_RCV_REPLY;
					goto out;
				}
				if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_NET_RSP_RCVD) {
					hs->fsm_chain_send_fmp19_state = SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_BETC;
					sna_dfc_fsm_chain_send_fmp19_output_b(hs, skb);
					goto out;
				}
				goto out;
			}
			/* S, RQ, END_CHAIN, BIS. */
			if (skb->data[0] == SNA_RU_RC_BIS) {
				if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_INC) {
					hs->fsm_chain_send_fmp19_state = SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_BETC;
					sna_dfc_fsm_chain_send_fmp19_output_b(hs, skb);
					goto out;
				}
				goto out;
			}
			goto out;
		}
	}
out:	sna_debug(5, "fini: state:%d\n", hs->fsm_chain_send_fmp19_state);
	return 0;
}

static int sna_dfc_fsm_qri_chain_rcv_fmp19_output_r(struct sna_hs_cb *hs,
	struct sk_buff *skb)
{
	sna_debug(5, "init\n");
	hs->sense = 0x200B0000;		/* QRI state error. */
	return 0;
}

/**
 * enforce the setting of the QRI indicator in the RH. This indicator is
 * set the same for all MUs in a chain; ie. all MUs in a chain have QRI = QR
 * or have QRI = -QR.
 *
 * @hs: information about the last received request.
 * @skb: mu.
 * @output: execute output function if state check is found.
 *
 * true if state-check is found, otherwise false. if an error is discoveredd,
 * and output is enabled local.sense_code is set.
 */
static int sna_dfc_fsm_qri_chain_rcv_fmp19_state_chk(struct sna_hs_cb *hs,
	struct sk_buff *skb, int output)
{
	sna_rh *rh = sna_transport_header(skb);
	int err = 0;

	sna_debug(5, "init\n");
	if (hs->direction != SNA_DFC_DIR_INBOUND
		|| rh->rri != SNA_RH_RRI_REQ)
		goto out;
	if (rh->qri == SNA_RH_QRI_QR) {
		if (rh->eci == SNA_RH_ECI_EC) { /* R, RQ, QR, EC. */
			if (hs->fsm_qri_chain_rcv_fmp19_state == SNA_DFC_FSM_QRI_CHAIN_RCV_FMP19_STATE_INC_NOT_QR) {
				if (output)
					sna_dfc_fsm_qri_chain_rcv_fmp19_output_r(hs, skb);
				err = 1;
				goto out;
			}
		} else { /* R, RQ, QR, !EC. */
			if (hs->fsm_qri_chain_rcv_fmp19_state == SNA_DFC_FSM_QRI_CHAIN_RCV_FMP19_STATE_INC_NOT_QR) {
				if (output)
					sna_dfc_fsm_qri_chain_rcv_fmp19_output_r(hs, skb);
				err = 1;
				goto out;
			}
		}
		goto out;
	} else {
		if (rh->eci == SNA_RH_ECI_EC) { /* R, RQ, !QR, EC. */
			if (hs->fsm_qri_chain_rcv_fmp19_state == SNA_DFC_FSM_QRI_CHAIN_RCV_FMP19_STATE_INC_QR) {
				if (output)
					sna_dfc_fsm_qri_chain_rcv_fmp19_output_r(hs, skb);
				err = 1;
				goto out;
			}
		} else { /* R, RQ, !QR, !EC. */
			if (hs->fsm_qri_chain_rcv_fmp19_state == SNA_DFC_FSM_QRI_CHAIN_RCV_FMP19_STATE_INC_QR) {
				if (output)
					sna_dfc_fsm_qri_chain_rcv_fmp19_output_r(hs, skb);
				err = 1;
				goto out;
			}
		}
		goto out;
	}
out:    return 0;
}

/**
 * enforce the setting of the QRI indicator in the RH. This indicator is
 * set the same for all MUs in a chain; ie. all MUs in a chain have QRI = QR
 * or have QRI = -QR.
 *
 * @hs: information about the last received request.
 * @skb: mu.
 *
 * if a qri state error is detected, local.sense_code is set.
 */
static int sna_dfc_fsm_qri_chain_rcv_fmp19(struct sna_hs_cb *hs,
	struct sk_buff *skb)
{
	sna_rh *rh = sna_transport_header(skb);

	sna_debug(5, "init\n");
	if (hs->direction != SNA_DFC_DIR_INBOUND
		|| rh->rri != SNA_RH_RRI_REQ)
		goto out;
	if (rh->qri == SNA_RH_QRI_QR) {
		if (rh->eci == SNA_RH_ECI_EC) { /* R, RQ, QR, EC. */
			if (hs->fsm_qri_chain_rcv_fmp19_state == SNA_DFC_FSM_QRI_CHAIN_RCV_FMP19_STATE_INC_QR) {
				hs->fsm_qri_chain_rcv_fmp19_state = SNA_DFC_FSM_QRI_CHAIN_RCV_FMP19_STATE_RESET;
				goto out;
			}
			if (hs->fsm_qri_chain_rcv_fmp19_state == SNA_DFC_FSM_QRI_CHAIN_RCV_FMP19_STATE_INC_NOT_QR) {
				sna_dfc_fsm_qri_chain_rcv_fmp19_output_r(hs, skb);
				goto out;
			}
		} else { /* R, RQ, QR, !EC. */
			if (hs->fsm_qri_chain_rcv_fmp19_state == SNA_DFC_FSM_QRI_CHAIN_RCV_FMP19_STATE_RESET) {
				hs->fsm_qri_chain_rcv_fmp19_state = SNA_DFC_FSM_QRI_CHAIN_RCV_FMP19_STATE_INC_QR;
				goto out;
			}
			if (hs->fsm_qri_chain_rcv_fmp19_state == SNA_DFC_FSM_QRI_CHAIN_RCV_FMP19_STATE_INC_NOT_QR) {
				sna_dfc_fsm_qri_chain_rcv_fmp19_output_r(hs, skb);
				goto out;
			}
		}
		goto out;
	} else {
		if (rh->eci == SNA_RH_ECI_EC) { /* R, RQ, !QR, EC. */
			if (hs->fsm_qri_chain_rcv_fmp19_state == SNA_DFC_FSM_QRI_CHAIN_RCV_FMP19_STATE_INC_QR) {
				sna_dfc_fsm_qri_chain_rcv_fmp19_output_r(hs, skb);
				goto out;
			}
			if (hs->fsm_qri_chain_rcv_fmp19_state == SNA_DFC_FSM_QRI_CHAIN_RCV_FMP19_STATE_INC_NOT_QR) {
				hs->fsm_qri_chain_rcv_fmp19_state = SNA_DFC_FSM_QRI_CHAIN_RCV_FMP19_STATE_RESET;
				goto out;
			}
		} else { /* R, RQ, !QR, !EC. */
			if (hs->fsm_qri_chain_rcv_fmp19_state == SNA_DFC_FSM_QRI_CHAIN_RCV_FMP19_STATE_RESET) {
				hs->fsm_qri_chain_rcv_fmp19_state = SNA_DFC_FSM_QRI_CHAIN_RCV_FMP19_STATE_INC_NOT_QR;
				goto out;
			}
			if (hs->fsm_qri_chain_rcv_fmp19_state == SNA_DFC_FSM_QRI_CHAIN_RCV_FMP19_STATE_INC_QR) {
				sna_dfc_fsm_qri_chain_rcv_fmp19_output_r(hs, skb);
				goto out;
			}
		}
		goto out;
	}
out:	return 0;
}

/* Maintain a purging state for received BB chains that have been negatively
 * responded to indicating a bracket error (0813, 0814, 088B). It is called
 * with a PURGE signal when the negative response is sent and reset when
 * the end-of-chain (EC) RU is received. When in the purging state, no records
 * are generated for PS or RM as a result of receiving a request RU in the
 * BB chain (ie. the remainder of the BB chain is purged).
 */
static int sna_dfc_fsm_rcv_purge_fmp19(struct sna_hs_cb *hs, struct sk_buff *skb, int purge)
{
	sna_rh *rh = sna_transport_header(skb);

	sna_debug(5, "init\n");
	if (hs->direction == SNA_DFC_DIR_INBOUND
		&& rh->eci == SNA_RH_ECI_EC) {
		if (hs->fsm_rcv_purge_fmp19_state == SNA_DFC_FSM_RCV_PURGE_FMP19_STATE_PURGE)
			hs->fsm_rcv_purge_fmp19_state = SNA_DFC_FSM_RCV_PURGE_FMP19_STATE_RESET;
		goto out;
	}
	if (purge) {
		if (hs->fsm_rcv_purge_fmp19_state == SNA_DFC_FSM_RCV_PURGE_FMP19_STATE_RESET)
			hs->fsm_rcv_purge_fmp19_state = SNA_DFC_FSM_RCV_PURGE_FMP19_STATE_PURGE;
		goto out;
	}
out:	return 0;
}

/**
 * record information about the last chain sent or received. This is done by
 * updating the correlation table entry.
 *
 * @hs: ct that may be updated, either ct_send or ct_rcv.
 * @skb: mu containing the information to save about a sent or received chain.
 *
 * information from the input mu added to the information that was saved
 * from the earlier ru's of the chain, this information is saved in the
 * input correlation table (ct_rcv or ct_send).
 */
static int sna_dfc_ct_update(struct sna_hs_cb *hs, struct sk_buff *skb)
{
	sna_rh *rh = sna_transport_header(skb);
	sna_fid2 *fh = sna_network_header(skb);
	struct sna_c_table *ct;

	sna_debug(5, "init\n");
	if (hs->direction)
		ct = &hs->ct_rcv;
	else
		ct = &hs->ct_send;
	if (rh->rri == SNA_RH_RRI_RSP) {
		if (rh->sdi)
			ct->neg_rsp_sense = 0x1; /* save the sense data from skb. */
		goto out;
	}

	/* request. */
	if (rh->bci) {
		ct->entry_present	= 1;
		ct->snf			= ntohs(fh->snf);
		ct->neg_rsp_sense	= 0;
		ct->rh.dr1i             = rh->dr1i;
		ct->rh.dr2i             = rh->dr2i;
		ct->rh.rti              = rh->rti;	/* eri */
		ct->rh.cebi             = rh->cebi;
		ct->rh.cdi              = rh->cdi;
		ct->rq_code		= SNA_CT_RQ_CODE_OTHER;	/* this seems to need more. */
		goto out;
	}
	if (rh->eci) {
		ct->rh.dr1i		= rh->dr1i;
		ct->rh.dr2i		= rh->dr2i;
		ct->rh.rti		= rh->rti;	/* eri */
		ct->rh.cebi		= rh->cebi;
		ct->rh.cdi		= rh->cdi;
		goto out;
	}
out:	return 0;
}

/**
 * maintain states by invoking the appropriate FSM while sending requests
 * and responses.
 *
 * @hs: alternate code allowed indicator, local.alternate_code.
 * @skb: mu, containing request or response.
 *
 * request/response to tc; possible update of the following fields:
 * local.direction; local.sig_rq_outstanding; sequence number for request
 * or response, local.sqn_send_cnt.
 */
static int sna_dfc_send_fsms(struct sna_hs_cb *hs, struct sk_buff *skb)
{
	sna_rh *rh = sna_transport_header(skb);
	sna_fid2 *fh = sna_network_header(skb);
	int err;

	sna_debug(5, "init\n");
	hs->direction	= 0;	/* outbound. */
	fh->snf = htons(++hs->sqn_tx_cnt);
	if (rh->rri == SNA_RH_RRI_REQ &&
		fh->efi == SNA_TH_EFI_EXP) {
		hs->sig_rq_outstanding = 1;
		goto done;
	}
	if (rh->rri == SNA_RH_RRI_RSP
		&& fh->efi == SNA_TH_EFI_NORM) {
		sna_dfc_ct_update(hs, skb);
		sna_dfc_fsm_chain_rcv_fmp19(hs, skb, SNA_DFC_FSM_CHAIN_IND_NONE);
		goto done;
	}

	/* normal flow request. */
	if (!hs->rqd_required_on_ceb) {
		hs->normal_flow_rq_cnt++;
		if (hs->normal_flow_rq_cnt > 16384)
			hs->rqd_required_on_ceb = 1;
	}
	sna_dfc_ct_update(hs, skb);
	if (rh->cebi == SNA_RH_CEBI_CEB) {
		sna_debug(5, "FIXME: here\n");
#ifdef NOT
		if (req_h->cebi == SNA_RH_CEBI_CEB) {
			if (SNA_DFC_RQE1(req_h))
				SNA_DFC_SET_RQD1(req_h);
			if (SNA_DFC_RQD(req_h)) {
				lulu->rqd_required_on_ceb = SNA_DFC_NO;
				lulu->normal_flow_rq_cnt = 0;
			}
			if (rq = SNA_DEALLOCATE_ABEND)
				SNA_DFC_SET_RQD1(req_h);
		}
#endif
	}
	if (hs->fsm_chain_rcv_fmp19_state
		== SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_PEND_SEND_REPLY)
		sna_dfc_fsm_chain_rcv_fmp19(hs, skb, SNA_DFC_FSM_CHAIN_IND_NONE);
	if (rh->bbi == SNA_RH_BBI_BB) {
		/* need to know if the hs_cb is in the send state? */
		if (hs->direction == 0) {
			if (hs->type == SNA_HS_TYPE_FSP) {
				hs->phs_bb_register.num = ntohs(fh->snf);
				hs->ct_send.snf_who	= SNA_HS_TYPE_FSP;
			} else {
				hs->shs_bb_register.num	= ntohs(fh->snf);
				hs->ct_send.snf_who     = SNA_HS_TYPE_BIDDER;
			}
		} else {
			if (hs->type == SNA_HS_TYPE_FSP) {
				hs->shs_bb_register.num	= ntohs(fh->snf);
				hs->ct_rcv.snf_who	= SNA_HS_TYPE_BIDDER;
			} else {
				hs->phs_bb_register.num	= ntohs(fh->snf);
				hs->ct_rcv.snf_who	= SNA_HS_TYPE_FSP;
			}
		}
	}
	if (rh->bci == SNA_RH_BCI_BC)
		sna_dfc_fsm_chain_send_fmp19(hs, skb, SNA_DFC_FSM_CHAIN_IND_BEGIN);
	if (rh->eci == SNA_RH_ECI_EC)
		sna_dfc_fsm_chain_send_fmp19(hs, skb, SNA_DFC_FSM_CHAIN_IND_END);
done:	err = sna_tc_send_mu(hs, skb);
	if (err < 0)
		sna_debug(5, "sna_tc_send_mu failed `%d'.\n", err);
	return err;
}

/**
 * initialize the th and rh fields of an mu record.
 *
 * @skb: a newly created mu.
 *
 * initialized rh and selected th bits, local.common.rq_code.
 */
int sna_dfc_init_th_rh(struct sk_buff *skb)
{
	sna_fid2 *fid2;
	sna_rh *rh;
	int len;

	sna_debug(5, "init\n");

	/* set request header. */
	len = sizeof(sna_rh);
	skb_reset_transport_header(skb);
	rh = (sna_rh *)skb_push(skb, sizeof(sna_rh));
	memset(rh, 0, sizeof(sna_rh));
	rh->rri         = SNA_RH_RRI_REQ;
	rh->ru          = SNA_RH_RU_FMD;
	rh->fi          = SNA_RH_FI_NO_FMH;
	rh->sdi         = SNA_RH_SDI_NO_SD;
	rh->bci         = SNA_RH_BCI_NO_BC;
	rh->eci         = SNA_RH_ECI_NO_EC;

	rh->rti		= SNA_RH_RTI_POS;
	SNA_DFC_SET_RQE1(rh);
	rh->llci        = SNA_RH_LLCI_NO_LLC;
	rh->rlwi        = SNA_RH_RLWI_NO_RLW;
	rh->qri         = SNA_RH_QRI_NO_QR;
	rh->pi          = SNA_RH_PI_NO_PAC;

	rh->bbi         = SNA_RH_BBI_NO_BB;
	rh->ebi         = SNA_RH_EBI_NO_EB;
	rh->cdi         = SNA_RH_CDI_NO_CD;
	rh->csi         = SNA_RH_CSI_CODE0;
	rh->edi         = SNA_RH_EDI_NO_ED;
	rh->pdi         = SNA_RH_PDI_NO_PD;
	rh->cebi        = SNA_RH_CEBI_NO_CEB;

	/* set transmission header. */
	len += sizeof(sna_fid2);
	skb_reset_network_header(skb);
	fid2 = (sna_fid2 *)skb_push(skb, sizeof(sna_fid2));
	memset(fid2, 0, sizeof(sna_fid2));
	fid2->format    = SNA_TH_FID2;
	fid2->mpf       = SNA_TH_MPF_WHOLE_BIU;
	fid2->efi       = SNA_TH_EFI_NORM;
	fid2->odai      = 0;
	fid2->daf       = 0;
	fid2->oaf       = 0;
	fid2->snf       = 0;

	return len;
}

static struct sk_buff *sna_dfc_create_and_init_mu(struct sna_rcb *rcb)
{
	struct sna_port_cb *port;
	struct sna_hs_cb *hs;
	struct sna_pc_cb *pc;
	struct sna_ls_cb *ls;
	struct sk_buff *skb;

	sna_debug(5, "init\n");
	hs = sna_hs_get_by_index(rcb->hs_index);
	if (!hs)
		return NULL;
	pc = sna_pc_get_by_index(hs->pc_index);
	if (!pc)
		return NULL;
	port = sna_cs_port_get_by_index(pc->port_index);
	if (!port)
		return NULL;
	ls = sna_cs_ls_get_by_index(port, pc->ls_index);
	if (!ls)
		return NULL;
	skb = sna_alloc_skb(rcb->tx_max_btu, GFP_ATOMIC);
	if (!skb)
		return NULL;
	sna_dlc_data_reserve(ls, skb);
	skb_reserve(skb, sizeof(sna_fid2));
	skb_reserve(skb, sizeof(sna_rh));
	return skb;
}

/**
 * send an MU according to passed instructions.
 *
 * @hs: local information.
 * @skb: mu, containing a ps_to_hs record (it informs this procedure how to
 *       set the bbi, fi, betc, bci, eci, eri, dr1i, and dr2i bits in the rh).
 *
 * the mu is created and initialized; mu.rh bits, local.common.rq_code,
 * mu.ru are set (according to the ps_to_hs record); and the mu is
 * sent to ps, betc.
 */
static int sna_dfc_send_fmd_mu(struct sna_hs_cb *hs, struct sk_buff *skb,
	struct sna_rcb *rcb)
{
	struct sk_buff *s_mu;
	sna_rh *s_rh;
	struct sna_skb_cb *cb = SNA_SKB_CB(skb);
	int err = 0, flush = 0;

	sna_debug(5, "init\n");
	if (!hs->send_mu) {
		hs->send_mu = sna_dfc_create_and_init_mu(rcb);
		if (!hs->send_mu)
			return -ENOMEM;
		sna_dfc_init_th_rh(hs->send_mu);
	}
	s_mu = hs->send_mu;
	s_rh = sna_transport_header(s_mu);

	if (cb->fmh) {
		s_rh->fi = SNA_RH_FI_FMH;
		if (cb->type == SNA_CTRL_T_REC_DEALLOCATE_ABEND)
			hs->deallocate_abend = 1;
	}
	if (hs->fsm_chain_send_fmp19_state == SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_BETC)
		s_rh->bci = SNA_RH_BCI_BC;
	switch (cb->type) {
		case SNA_CTRL_T_REC_ALLOCATE:
			// s_rh->bbi = SNA_RH_BBI_BB;
			s_rh->cdi = SNA_RH_CDI_CD;

			/* XXX: temporary. */
			s_rh->pi = 1;
			break;
		case SNA_CTRL_T_REC_FLUSH:
			s_rh->eci = SNA_RH_ECI_NO_EC;
			SNA_DFC_SET_RQE1(s_rh);
			break;
		case SNA_CTRL_T_CONFIRM:
			s_rh->eci = SNA_RH_ECI_EC;
			SNA_DFC_SET_RQD3(s_rh);
			break;
		case SNA_CTRL_T_PREPARE_TO_RCV_CONFIRM_SHORT:
			s_rh->eci = SNA_RH_ECI_EC;
			s_rh->cdi = SNA_RH_CDI_CD;
			SNA_DFC_SET_RQD3(s_rh);
			break;
		case SNA_CTRL_T_PREPARE_TO_RCV_CONFIRM_LONG:
			s_rh->eci = SNA_RH_ECI_EC;
			s_rh->cdi = SNA_RH_CDI_CD;
			SNA_DFC_SET_RQE3(s_rh);
			break;
		case SNA_CTRL_T_PREPARE_TO_RCV_FLUSH:
			s_rh->eci = SNA_RH_ECI_EC;
			s_rh->cdi = SNA_RH_CDI_CD;
			SNA_DFC_SET_RQE1(s_rh);
			flush = 1;
			break;
		case SNA_CTRL_T_DEALLOCATE_CONFIRM:
			s_rh->eci  = SNA_RH_ECI_EC;
			s_rh->cebi = SNA_RH_CEBI_CEB;
			SNA_DFC_SET_RQD3(s_rh);
			break;
		case SNA_CTRL_T_DEALLOCATE_FLUSH:
			s_rh->eci  = SNA_RH_ECI_EC;
			s_rh->cebi = SNA_RH_CEBI_CEB;
			break;
	}

	/* copy data to send_mu buffer.
	 * FIXME: we only do whole ru's at the moment.
	 */
	if (skb->len)
		memcpy(skb_put(s_mu, skb->len), skb->data, skb->len);
	kfree_skb(skb);

	/* send buffer if it is full or being flushed.
	 * FIXME: we need to enforce chaining protocol if buffer is
	 *        too big to send in one whole ru.
	 */
	if (flush) {
		/* pull any data sitting on the rcb send_mu queue. */
		if (rcb->send_mu) {
			sna_debug(5, "pulling %d bytes from rcb->send_mu buffer.\n",
				rcb->send_mu->len);
			memcpy(skb_put(s_mu, rcb->send_mu->len),
				rcb->send_mu->data, rcb->send_mu->len);
			kfree_skb(rcb->send_mu);
			rcb->send_mu = NULL;
		}

		hs->send_mu = NULL;
		err = sna_dfc_send_fsms(hs, s_mu);
		if (err < 0)
			sna_debug(5, "sna_dfc_send_fsms failed `%d'.\n", err);
	}

#ifdef OLD
	sna_dfc_init_th_rh(skb);
	if (cb->fmh) {
		rh->fi = SNA_RH_FI_FMH;
		if (cb->type == SNA_CTRL_T_REC_DEALLOCATE_ABEND)
			hs->deallocate_abend = 1;
	}
	if (hs->betc == 0) {	/* new chain, use something better... */
		rh->bci = SNA_RH_BCI_BC;
	}
	if (cb->type != SNA_CTRL_T_REC_FLUSH) {
		switch (cb->type) {
			case SNA_CTRL_T_REC_ALLOCATE:
				rh->bbi = SNA_RH_BBI_BB;
				rh->cdi = SNA_RH_CDI_CD;

				/* not sure how best to set these. */
				rh->bci = 1;
				rh->eci = 1;
				break;
			case SNA_CTRL_T_REC_FLUSH:
				rh->eci = SNA_RH_ECI_NO_EC;
				SNA_DFC_SET_RQE1(rh);
				break;
			case SNA_CTRL_T_CONFIRM:
				rh->eci = SNA_RH_ECI_EC;
				SNA_DFC_SET_RQD3(rh);
				break;
			case SNA_CTRL_T_PREPARE_TO_RCV_CONFIRM_SHORT:
				rh->eci = SNA_RH_ECI_EC;
				rh->cdi = SNA_RH_CDI_CD;
				SNA_DFC_SET_RQD3(rh);
				break;
			case SNA_CTRL_T_PREPARE_TO_RCV_CONFIRM_LONG:
				rh->eci = SNA_RH_ECI_EC;
				rh->cdi = SNA_RH_CDI_CD;
				SNA_DFC_SET_RQE3(rh);
				break;
			case SNA_CTRL_T_PREPARE_TO_RCV_FLUSH:
				rh->eci = SNA_RH_ECI_EC;
				rh->cdi = SNA_RH_CDI_CD;
				SNA_DFC_SET_RQE1(rh);
				break;
			case SNA_CTRL_T_DEALLOCATE_CONFIRM:
				rh->eci  = SNA_RH_ECI_EC;
				rh->cebi = SNA_RH_CEBI_CEB;
				SNA_DFC_SET_RQD3(rh);
				break;
			case SNA_CTRL_T_DEALLOCATE_FLUSH:
				rh->eci  = SNA_RH_ECI_EC;
				rh->cebi = SNA_RH_CEBI_CEB;
				break;
			/* dummy type for now. */
			case SNA_CTRL_T_REC_SEND_DATA:
				rh->bci = 1;
				rh->eci = 1;
				rh->cdi = 1;
				break;
		}
		hs->betc = 1;
	}

#ifdef NOT
	if (req_h->bci && req_h->eci && mu->biu->ru.ru.raw == NULL) {
		new(mu->biu->ru.ru.lustat, GFP_ATOMIC);
		if (!mu->biu->ru.ru.lustat)
			return -ENOMEM;
	}
#endif

	err = sna_dfc_send_fsms(hs, skb);
	if (err < 0)
		sna_debug(5, "sna_dfc_send_fsms failed `%d'.\n", err);
#endif
	return err;
}

int sna_dfc_send_from_ps_data(struct sk_buff *skb, struct sna_rcb *rcb)
{
	struct sna_hs_cb *hs;
	int err;

	sna_debug(5, "init\n");
	hs = sna_hs_get_by_index(rcb->hs_index);
	if (!hs)
		return -ENOENT;
	err = sna_dfc_send_fmd_mu(hs, skb, rcb);
	if (err < 0)
		sna_debug(5, "sna_dfc_send_fmd_mu failed `%d'.\n", err);
	return err;
}

int sna_dfc_send_from_ps_req(struct sk_buff *skb, struct sna_rcb *rcb)
{
	struct sna_hs_cb *hs;
	int err;

	sna_debug(5, "init\n");
	hs = sna_hs_get_by_index(rcb->hs_index);
	if (!hs)
		return -ENOENT;
	sna_dfc_init_th_rh(skb);
	err = sna_dfc_send_fsms(hs, skb);
	if (err < 0)
		sna_debug(5, "sna_dfc_send_fsms failed `%d'.\n", err);
	return err;
}

/**
 * this procedure initializes fields in the half-session's local storage
 * (for process data) that are used by dfc.
 *
 * @hs: half-session information.
 *
 * dfc local process data fields initialized.
 */
int sna_dfc_init(struct sna_hs_cb *hs)
{
	sna_debug(5, "init\n");
	hs->sqn_tx_cnt			= 0;
	hs->current_bracket.who		= SNA_HS_TYPE_FSP;
	hs->current_bracket.num		= 0;
	hs->phs_bb_register.who		= SNA_HS_TYPE_FSP;
	hs->phs_bb_register.num		= 0;
	hs->shs_bb_register.who		= SNA_HS_TYPE_BIDDER;
	hs->shs_bb_register.num		= 0;
	hs->rqd_required_on_ceb		= 0;
	hs->normal_flow_rq_cnt		= 0;
	hs->sig_received		= 0;
	hs->sig_snf_sqn			= 0;
	hs->betc			= 1;
	hs->sig_rq_outstanding		= 0;
	hs->send_error_rsp_state	= SNA_DFC_RSP_STATE_RESET;
	hs->bb_rsp_state		= SNA_DFC_RSP_STATE_RESET;
	hs->rtr_rsp_state		= SNA_DFC_RSP_STATE_RESET;
	hs->sig_rq_outstanding		= SNA_DFC_RSP_STATE_RESET;

	hs->last_mu			= NULL;
	hs->send_mu			= NULL;

	/* intialize the fsms. */
	hs->fsm_bsm_fmp19_state		= SNA_DFC_FSM_BSM_FMP19_STATE_BETB;
	hs->fsm_chain_rcv_fmp19_state	= SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_BETC;
	hs->fsm_chain_send_fmp19_state	= SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_BETC;
	hs->fsm_qri_chain_rcv_fmp19_state = SNA_DFC_FSM_QRI_CHAIN_RCV_FMP19_STATE_RESET;
	hs->fsm_rcv_purge_fmp19_state	= SNA_DFC_FSM_RCV_PURGE_FMP19_STATE_RESET;
	return 0;
}

#ifdef NOT
/* Process records received from the resource manager (RM). This procedure
 * is called by the half session router.
 */
int sna_dfc_send_from_rm(struct sna_mu *mu)
{
	struct sna_hs_local *local = sna_hs_find_local(mu->hs_id);
	struct sna_lu_lu_cb *lulu = &local->lulu;
	struct sna_hs_ps_connected *hs_ps_connected = NULL;
	struct sna_ru *ru = &mu->biu->ru;

	switch(mu->record_type)
	{
		case (SNA_REC_BID_WITHOUT_ATTACH):
			mu = sna_get_buffer(SNA_BM_TYPE_PERM,
				local->perm_buf_pool_id, 0, SNA_BM_NO_WAIT);
			sna_init_th_rh(mu);
			new(ru->ru.lustat, GFP_ATOMIC);
			sna_dfc_send_fsms(mu);
			break;

		case (SNA_REC_BIS_REPLY):
			mu = sna_get_buffer(SNA_BM_TYPE_PERM,
				local->perm_buf_pool_id, 0, SNA_BM_NO_WAIT);
			sna_init_th_rh(mu);
			new(ru->ru.bis, GFP_ATOMIC);
			sna_dfc_send_fsms(mu);
			break;

		case (SNA_REC_BIS_RQ):
			mu = sna_get_buffer(SNA_BM_TYPE_PERM,
				local->perm_buf_pool_id, 0, SNA_BM_NO_WAIT);
			sna_init_th_rh(mu);
			sna_dfc_send_fsms(mu);
			break;

		case (SNA_REC_BRACKET_FREED):
			sna_free_buffer(mu);
			break;

		case (SNA_REC_HS_PS_CONNECTED):
			lulu->ps_id = hs_ps_connected->ps_id;
			local->lulu.bracket_id = hs_ps_connected->bracket_id;

			sna_fsm_bsm_fmp19(mu, SNA_DFC_FSM_INB);
			if(lulu->session_just_started == SNA_DFC_YES)
				lulu->session_just_started = SNA_DFC_NO;
			else
			{
				lulu->current_bracket_sqn.number
					= lulu->sqn_send_cnt.number++;
				lulu->current_bracket_sqn.bracket_started_by
					= local->half_session;
				if(local->half_session == SNA_HS_PRI)
					lulu->phs_bb_register.number
						= lulu->current_bracket_sqn.number;
				else
					lulu->shs_bb_register.number
						= lulu->current_bracket_sqn.number;
			}
			break;

		case (SNA_REC_RTR_RQ):
			mu = sna_get_buffer(SNA_BM_TYPE_PERM,
				local->perm_buf_pool_id, 0, SNA_BM_NO_WAIT);
			sna_init_th_rh(mu);
			new(ru->ru.rtr, GFP_ATOMIC);
			local->rq_code = SNA_LOCAL_RTR;
			sna_dfc_send_fsms(mu);
			break;

		case (SNA_REC_YIELD_SESSION):
			if(lulu->session_just_started == SNA_DFC_YES)
				lulu->session_just_started = SNA_DFC_NO;
			mu = sna_get_buffer(SNA_BM_TYPE_PERM,
				local->perm_buf_pool_id, 0, SNA_BM_NO_WAIT);
			sna_init_th_rh(mu);
			new(ru->ru.lustat, GFP_ATOMIC);
			local->rq_code = SNA_LOCAL_LUSTAT;
			sna_dfc_send_fsms(mu);
			break;

		case (SNA_REC_SECURITY_REPLY_2):
			if(ru->ru.security_reply_2.send_parm.type
				== SNA_MU_DEALLOCATE_FLUSH)
			{
				lulu->session_just_started = SNA_DFC_NO;
			}

			mu = sna_get_buffer(SNA_BM_TYPE_PERM,
				local->perm_buf_pool_id, 0, SNA_BM_NO_WAIT);
			new(ru->ru.security_reply_2, GFP_ATOMIC);
			sna_send_fmd_mu(mu);
			break;

		default:
			break;
	}

	return (0);
}

/* This procedure determines if a REQUEST_TO_SEND record should be sent
 * to PS to indicate a SIGNAL has been received. This procedure is called
 * by the half-session router.
 */
static int sna_try_to_rcv_signal(struct sna_mu *mu)
{
	struct sna_hs_local *local = sna_hs_find_local(mu->hs_id);
	struct sna_lu_lu_cb *lulu = &local->lulu;

	if(sna_fsm_bsm_fmp19(0, SNA_DFC_FSM_CURRENT) == SNA_DFC_FSM_INB
		&& lulu->sig_received == SNA_DFC_YES)
	{
		switch(sna_signal_status(mu))
		{
			case (SNA_DFC_SIG_CURRENT):
				sna_dfc_send_to_ps(NULL);
				lulu->sig_received = SNA_DFC_NO;
				break;

			case (SNA_DFC_SIG_STRAY):
				lulu->sig_received = SNA_DFC_NO;
				break;

			case (SNA_DFC_SIG_FUTURE):
				break;
		}
	}

	return (0);
}

/**
 * determine if a normal-flow request is a reply to a BID request. A reply
 * is a request sent (or received) immediately after receving (or sending) a
 * request carrying (RQE, CD. A reply implies a positive response to the
 * (RQE, CD) request.
 *
 * @hs: information about the last chain sent, local.ct_send.
 * @skb: mu containing a normal-flow request.
 *
 * true if mu is reply to a bid; false, otherwise.
 */
static int sna_dfc_reply_to_bid(struct sna_hs_cb *hs, struct sk_buff *skb)
{
//	struct sna_hs_local *local = sna_hs_find_local(mu->hs_id);

	if(sna_ok_to_reply(mu))
		return (SNA_DFC_TRUE);
	else
		return (SNA_DFC_FALSE);
}

/**
 * determine if a SIGNAL is for a past, current, or future bracket. the
 * in-bracket (INB) state exists when this procedure is called.
 *
 * @hs: local.sig_snf, local.current_bracket_sqn, local.phs_bb_register,
 *      local.shs_bb_register.
 * @skb:
 *
 * either current, future, or stray return code is set.
 */
static int sna_dfc_signal_status(struct sna_hs_cb *hs, struct sk_buff *skb)
{
	struct sna_hs_local *local = sna_hs_find_local(mu->hs_id);
	struct sna_lu_lu_cb *lulu = &local->lulu;
	int result, reg;

	if(lulu->current_bracket_sqn.number
		== lulu->current_bracket_sqn.number)
		return (SNA_DFC_SIG_CURRENT);

	if(SNA_HS_PRI)
		reg = lulu->phs_bb_register.number;
	else
		reg = lulu->shs_bb_register.number;

	result = (lulu->sig_snf.number -  reg) % 25;
	if(result < 0)
	{
		result += 25;
		if(result == 0)
			return (SNA_DFC_SIG_STRAY);
		if(result == 24)
			return (SNA_DFC_SIG_FUTURE);
		if(result > 24)
			return (SNA_DFC_SIG_STRAY);
	}

	return (0);
}
#endif
