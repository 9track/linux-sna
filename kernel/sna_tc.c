/* sna_tc.c: Linux Systems Network Architecture implementation
 * - SNA Transmission Control (TC)
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

#include <linux/config.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/bitops.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <linux/stat.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/proc_fs.h>

#ifdef CONFIG_SNA_LLC
#include <net/llc_if.h>
#include <net/llc_sap.h>
#include <net/llc_pdu.h>
#include <net/llc_conn.h>
#include <linux/llc.h>
#endif  /* CONFIG_SNA_LLC */

#include <linux/sna.h>

static int sna_tc_send_to_pc(struct sna_hs_cb *hs, struct sk_buff *skb);

/** 
 * this procedure updates pacing counts in the local->common_cb, sends
 * reset acknowledgments to unsolicited IPMs and sends MUs to path_control
 * if possible.
 *
 * @hs: local.common_cb, the common control block for the pacing routines.
 * @skb: mu.
 *
 * the pacing response is discarded after being processed and the mu is
 * freed; local.common_cb, the pacing counts, set_rlwi, 
 * unsolicited_ipm_outstanding, and adjust_for_ipm_ack_outstanding may
 * be updated; ipm ack may be sent to path control; mus from the pacing
 * queue sent to path control; the dynamic buffer pool and the limited
 * buffer pool sizes may be adjusted.
 */
static int sna_tc_rx_pacing_rsp(struct sna_hs_cb *hs, struct sk_buff *skb)
{
	sna_ipm *ipm = (sna_ipm *)skb->data;
	struct sk_buff *r_skb;
	int err = 0;
	
	sna_debug(5, "init\n");
	if (hs->tx_pacing.type == SNA_HS_PACING_TYPE_NONE) {
		err = -EINVAL;
		goto out;
	}
	if (hs->tx_pacing.type == SNA_HS_PACING_TYPE_FIXED) {
                hs->tx_pacing.nws = hs->first_ws;
                goto done;
        }
	if (hs->tx_pacing.type == SNA_HS_PACING_TYPE_ADAPTIVE) {
		if (ipm->type == SNA_IPM_TYPE_RESET) {
			hs->rx_pacing.rpc 		= 0;
			hs->rx_pacing.nws 		= hs->unsolicited_nws;
			hs->unsolicited_ipm_outstanding	= 0;
			/* spec makes mention to if we are reassembling,
			 * to no free buffer.. we will find out.
			 */
			goto done;
		}
		if (ipm->type == SNA_IPM_TYPE_SOLICIT) {
			if (skb_queue_empty(&hs->write_queue))
				goto done;
			hs->tx_pacing.rpc = 0;
			hs->tx_pacing.nws = ntohs(ipm->nws);
			goto done;
		}
		if (ipm->type == SNA_IPM_TYPE_UNSOLICIT) {
			r_skb = skb_copy(skb, GFP_ATOMIC);
			if (!r_skb)
				goto out;
			hs->tx_pacing.nws = ntohs(ipm->nws);
			hs->tx_pacing.rpc = 0;
			((sna_ipm *)skb->data)->type 	= SNA_IPM_TYPE_RESET;
			((sna_ipm *)skb->data)->rwi	= SNA_IPM_RWI_OFF;
			err = sna_tc_send_to_pc(hs, r_skb);
			goto done;
		}
	}
	
	/* remove the next mu from the pacing queue. */
done:	if (!skb_queue_empty(&hs->write_queue)) {
		r_skb = skb_peek(&hs->write_queue);		/* need to be careful. */
		if (r_skb->nh.fh->mpf != SNA_TH_MPF_BBIU 
			|| r_skb->h.rh->rri == SNA_RH_RRI_RSP
			|| (hs->tx_pacing.rpc + hs->tx_pacing.nws) > 0) {
			r_skb = skb_dequeue(&hs->write_queue);
			err = sna_tc_send_to_pc(hs, r_skb);
		}
	}
out:	kfree_skb(skb);
	return err;
}

/** 
 * this procedure performs receive checks on BIUs. 
 *
 * @hs: local information.
 * @skb: mu, containing a biu.
 *
 * local.sense_code is set to reflect the receive checks.
 */
static int sna_tc_rx_biu_chk(struct sna_hs_cb *hs, struct sk_buff *skb)
{
	int len = 0;
	
	sna_debug(5, "init\n");
	if (skb->h.rh->ru != SNA_RH_RU_FMD && skb->h.rh->ru != SNA_RH_RU_DFC)
		return -EINVAL;	/* 0x10070000 */
	if (skb->h.rh->sdi)
		len += 4;
	if (skb->h.rh->ru != SNA_RH_RU_DFC)
		len += 1;
	if (skb->len < (len + sizeof(sna_rh)))
		return -EINVAL;	/* 0x10020000 */
	if (skb->h.rh->sdi) {
		/* we need to extract the sense data and send it to the caller. */
		sna_debug(5, "biu contains sense data\n");
		return -EINVAL;
	}
	if (skb->h.rh->rri == SNA_RH_RRI_REQ 
		&& skb->nh.fh->efi == SNA_TH_EFI_NORM) {
		if (ntohs(skb->nh.fh->snf) != (hs->sqn_rcv_cnt + 1))
			return -EINVAL;	/* 0x20010000 */
		if (hs->crypto && skb->h.rh->ru == SNA_RH_RU_FMD) {
			if (skb->h.rh->edi != SNA_RH_EDI_ED)
				return -EINVAL;	/* 0x08090000 */
			if (skb->len % 8)	/* must be a multple of 8. */
				return -EINVAL;	/* 0x10010000 */
		}
	}	
        return 0;
}

static struct sk_buff *sna_tc_segment_reasm(struct sna_hs_cb *hs)
{
	struct sk_buff *skb, *s_skb;
	
	sna_debug(5, "init\n");
	
	/* we should use something more complete like 
	 * the ip_fragment handler. 
	 */
	hs->segment_in |= SNA_HS_SEGMENT_COMPLETE;

	/* handle the first segment special. */
	s_skb = skb_dequeue(&hs->segments);
        if (!s_skb)
                goto o_fail;
	
	/* allocate an skb large enough for all segments. */
	skb = skb_copy_expand(s_skb, 0, hs->segment_len, GFP_ATOMIC);
	if (!skb)
		goto o_mem;
	kfree_skb(s_skb);

	/* copy ru portion of all following segments. */
	while ((s_skb = skb_dequeue(&hs->segments)) != NULL) {
		memcpy(skb_put(skb, skb->len), skb->data, skb->len);
		kfree_skb(s_skb);
	}
	skb->nh.fh->mpf	= SNA_TH_MPF_WHOLE_BIU;
	hs->segment_num	= 0;
	hs->segment_len	= 0;
	hs->segment_in	= SNA_HS_SEGMENT_NONE;
	return skb;
	
o_mem:	sna_debug(5, "out of memory to build whole biu\n");
o_fail:	return NULL;	
}

/* segments are queued in the order they are received. 
 */
static int sna_tc_segment_queue(struct sna_hs_cb *hs, struct sk_buff *skb)
{
	sna_debug(5, "init\n");

	if (hs->segment_in & SNA_HS_SEGMENT_COMPLETE)
		goto error;
	if (skb->nh.fh->mpf == SNA_TH_MPF_BBIU) {		/* first segment. */
		hs->segment_in |= SNA_HS_SEGMENT_FIRST_IN;
		hs->segment_len += (skb->truesize - skb->len);	/* include mac + th + rh */
	}
	if (skb->nh.fh->mpf == SNA_TH_MPF_EBIU) {		/* last segment. */
		hs->segment_in |= SNA_HS_SEGMENT_LAST_IN;
	}
	skb_queue_tail(&hs->segments, skb);
	hs->segment_len += skb->len;
	hs->segment_num++;
	return 0;
	
error:	kfree_skb(skb);
	return -EINVAL;
}

/** 
 * this procedure copies normal-flow request MUs from link buffer to a
 * dynamic buffer. This procedure is called to reassemble segments into a
 * whole BIU.
 *
 * @hs: local information.
 * @skb: mu, the mu contains a segment that received from path control
 * @err: numeric result of function.
 *
 * when segment reassembly is complete, all segments (for the same biu) are
 * reassembled and stored in the dynamic buffer. the dynamic buffer address is
 * returned to the calling procedure. if there is an ipm reset acknowledgment
 * outstanding, dynamic buffer pool may be adjusted and 
 * adjust_for_ipm_ack_outstanding bit in the local.common_cb may be changed. 
 * if an error is detected, local.sense_code is set to indicate the error.
 */
static struct sk_buff *sna_tc_segment(struct sna_hs_cb *hs, struct sk_buff *skb, int *err)
{
	struct sk_buff *r_skb = NULL;
	
	sna_debug(5, "init\n");
	*err = 0;
	if (skb->len > hs->max_rx_ru_size) {
		sna_debug(5, "rx'd skb ru too large\n");
		kfree_skb(skb);
		return NULL;
	}
	if (skb->nh.fh->mpf == SNA_TH_MPF_WHOLE_BIU)
		return skb;
	spin_lock(&hs->segment_lock);
	sna_tc_segment_queue(hs, skb);
	if (hs->segment_in == (SNA_HS_SEGMENT_FIRST_IN|SNA_HS_SEGMENT_LAST_IN)) {
		r_skb = sna_tc_segment_reasm(hs);
		if (hs->adjust_ipm_ack_outstanding)
			hs->adjust_ipm_ack_outstanding = 0;
	}
	spin_unlock(&hs->segment_lock);
	return r_skb;
}

/** 
 * this procedure updates the receive pacing counts in the local->common_cb
 * and determines the type of buffer reserve action to request of the BM.
 * This procedure is never called when the reidual pacing count and the
 * next-window size are both 0.
 *
 * @hs: local.common_cb.
 * @skb: mu.
 *
 * the receive pacing counts and the reserve_flag in the local.common_cb
 * may be changed.
 */
static int sna_tc_rx_pacing(struct sna_hs_cb *hs, struct sk_buff *skb)
{
	sna_debug(5, "init\n");

	if (skb->h.rh->pi != SNA_RH_PI_PAC) {	/* current window okay. */
		hs->rx_pacing.rpc--;
		hs->reserve_flag 	 = 0;
        } else {				/* first req in new win. */
		hs->rx_pacing.rpc 	 = hs->rx_pacing.nws - 1;
		hs->rx_pacing.nws 	 = 0;
		if (skb->h.rh->rlwi)
			hs->reserve_flag = SNA_HS_RESERVE_MORE;
		else
			hs->reserve_flag = SNA_HS_RESERVE_ALL; 
        }
        return 0;
}

/**
 * this procedure performs the format checks for pacing responses and
 * checks that the receive pacing counts are acceptable for begin biu
 * normal-flow requests.
 *
 * @hs: local.common_cb.
 * @skb: mu.
 *
 * local.sense_code is set if an error is found, otherwise, it 
 * remains unchanged.
 */
static int sna_tc_mu_pacing_chk(struct sna_hs_cb *hs, struct sk_buff *skb)
{
	int err = 0;

	sna_debug(5, "init\n");

#ifdef NOT
        struct sna_ipm_extension *ipm_extension = NULL;
        if (th->efi == SNA_TH_EFI_NORM) {
                if (local->tcb.ccb.receive_pacing.rpc == 0) {
                        if (local->tcb.ccb.receive_pacing.nws == 0)
                                local->sense = 0x20110000;
                        else {
                                if (mu->biu->rh.rh.rsp_h.pi != SNA_RH_PI_PAC)
                                        local->sense = 0x20110000;
                        }
                } else {
                        if (mu->biu->rh.rh.rsp_h.pi == SNA_RH_PI_PAC)
                                local->sense = 0x20110002;
                }
                return 0;
        }

        if (local->tcb.ccb.send_pacing.type == SNA_PACING_TYPE_ADAPTIVE
                && mu->biu->rh.rh.rsp_h.pi == SNA_RH_PI_PAC) {
                if (SNA_IPM_FORMAT_0) {
                        if (mu->dcf < sizeof(struct sna_rh)
                                + sizeof(struct sna_ipm_extension)) {
                                local->sense = 0x10020000;
                        } else {
                                if (ipm_extension->format_indicator
                                        != SNA_IPM_FORMAT_0) {
                                        local->sense = 0x10010003;
                                }
                        }
                } else {
                        switch (ipm_extension->type) {
                                case SNA_IPM_TYPE_SOLICITED:
                                        if (ipm_extension->nws == 0
                                                || mu->biu->ru.ru.ipm.rwi == SNA_IPM_RWI_RESET_WINDOW) {
                                                local->sense = 0x10010003;
                                        } else {
                                                if (local->tcb.ccb.send_pacing.nws > 0) {
                                                        local->sense = 0x20110001;
                                                }
                                        }
                                        break;

                                case SNA_IPM_TYPE_UNSOLICITED:
                                        if (ipm_extension->rwi != SNA_IPM_RWI_RESET_WINDOW)
                                                local->sense = 0x10010003;
                                        break;

                                case SNA_IPM_TYPE_RESET_ACK:
                                        if (local->tcb.ccb.receive_pacing.unsolicited_ipm_outstanding)
                                                local->sense = 0x20110001;
                                        break;

                                default:
                                        local->sense = 0x10010003;
                                        break;
                        }
                }
        }

        local->sense = 0x20110003;
#endif

	return err;
}

/** 
 * perform receive checks on all segments received from PC. 
 *
 * @hs: local information.
 * @skb: mu, containing a segment (perhaped the only segment in a biu).
 *
 * loca.sense_code is set to reflect the receive checks.
 */
static int sna_tc_rx_segment_chk(struct sna_hs_cb *hs, struct sk_buff *skb)
{
	int err = -EINVAL;

	sna_debug(5, "init\n");
	if (!hs->segmenting && skb->nh.fh->mpf != SNA_TH_MPF_WHOLE_BIU)
		goto out;	/* 0x80070001 */
	if (skb->nh.fh->mpf == SNA_TH_MPF_BBIU 
		&& (skb->h.rh->rri != SNA_RH_RRI_REQ
		|| skb->nh.fh->efi == SNA_TH_EFI_EXP))
		goto out;	/* 0x80070000 */

/*	finish this up.
 *
 *      if (SNA_DFC_RQD(req_h)
 *              || (th->mpf == SNA_TH_MPF_BBIU && local->segmenting_supported)) {
 *              local->sense = 0x80070000;
 *      }
 *      if (th->mpf == SNA_TH_MPF_EBIU && !local->segmenting_supported) {
 *              local->sense = 0x80070000;
 *      }
 *      if ((th->mpf == SNA_TH_MPF_EBIU && th->mpf == SNA_TH_MPF_BBIU)
 *              != th->mpf == SNA_TH_MPF_EBIU) {
 *              local->sense = 0x80070000;
 *      }
 */

	if (skb->nh.fh->mpf == SNA_TH_MPF_BBIU && skb->len < 10)
		goto out;	/* 0x80070000 */
	err = 0;
	if (skb->nh.fh->mpf == SNA_TH_MPF_BBIU)
                err = sna_tc_mu_pacing_chk(hs, skb);
out:	return err;
}

/** 
 * this procedure receives MUs sent from path control (through hsr). 
 *
 * @lf: local form session identifier to locate half-session.
 * @skb: mu containing a request or response is received from the hs router.
 *
 * if a pacing response is received, it is processed in tc and will not be
 * passed to dfc. tc updates the send pacing counts in the loca.common_cb and
 * the pacing response is discarded; else, dfc is called to receive and process
 * the mu.
 */
int sna_tc_rx(struct sna_lfsid *lf, struct sk_buff *skb)
{
	struct sna_hs_cb *hs;
	int err = -ENOENT;

	sna_debug(5, "init\n");
	hs = sna_hs_get_by_lfsid(lf);
	if (!hs)
		goto out;
	err = sna_tc_rx_segment_chk(hs, skb);
	if (err < 0)
		goto out;

	/* send expedited data right to dfc. */
	if (skb->nh.fh->efi != SNA_TH_EFI_NORM) {
		sna_debug(5, "efi=EXP data\n");
		err = sna_dfc_rx(hs, skb);
		goto out;
	}

	/* this is a new biu update pacing information. */
        if (skb->nh.fh->mpf == SNA_TH_MPF_BBIU)
                sna_tc_rx_pacing(hs, skb);

	/* reassemble segments to get a whole biu. */
	skb = sna_tc_segment(hs, skb, &err);
	if (!skb)
		goto out;

	/* validate entire biu */
	err = sna_tc_rx_biu_chk(hs, skb);
	if (err < 0)
		goto out;

	/* process pacing responses. */
	if (skb->h.rh->rri == SNA_RH_RRI_RSP && skb->h.rh->pi == SNA_RH_PI_PAC) {
		err = sna_tc_rx_pacing_rsp(hs, skb);
		goto out;
	}

	/* decrypt data. */
	if (skb->h.rh->edi == SNA_RH_EDI_ED && skb->h.rh->ru == SNA_RH_RU_FMD
		&& skb->h.rh->sdi != SNA_RH_SDI_SD && skb->len) {
		sna_debug(5, "rx encrypted packet\n");
		/* sna_tc_decipher_ru(skb); */
		err = -EINVAL;
		goto out;
	}

	/* pass the reassembled biu to dfc. */
	hs->sqn_rcv_cnt++;
	err = sna_dfc_rx(hs, skb);
out:	return err;
}

/** 
 * update the send pacing counts in the common control block and set the
 * pacing bits (rh->pi and rh->rlwi of the MU being sent) to the
 * appropriate values.
 *
 * @hs: local.common_cb, containing appropriate pacing bits settings.
 * @skb: mu, containing a normal-flow request (beginning biu).
 *
 * the pacing bits in the request mu may be changed; the set_rlwi bit 
 * and pacing counts in the local.common_cb may be changed.
 */
static int sna_tc_send_pacing(struct sna_hs_cb *hs, struct sk_buff *skb)
{
	sna_debug(5, "init\n");

        if (hs->tx_pacing.rpc > 0)
		hs->tx_pacing.rpc--;
        else {	/* start next window. */
		hs->tx_pacing.rpc 		= hs->tx_pacing.nws - 1;
		hs->tx_pacing.nws		= 0;
		skb->h.rh->pi 			= SNA_RH_PI_PAC;
		skb->h.rh->rlwi			= hs->rlwi;
		hs->rlwi			= 0;
        }
        return 0;
}

/** 
 * this procedure send an MU to path control. 
 *
 * @hs: local.common_cb.
 * @skb: mu.
 *
 * the input mu is sent to path control with the hs_to_pc header filled
 * in and, if necessary, the send pacing counts in local.common_cb
 * may also be updated.
 */
static int sna_tc_send_to_pc(struct sna_hs_cb *hs, struct sk_buff *skb)
{
	struct sna_pc_cb *pc;
	int err = -EINVAL;

	sna_debug(5, "init\n");
	pc = sna_pc_get_by_index(hs->pc_index);
	if (!pc)
		goto out;

	if (skb->nh.fh->mpf == SNA_TH_MPF_BBIU) {
		/* handle ipm messages and pacing. */
		if (hs->tx_pacing.type == SNA_HS_PACING_TYPE_ADAPTIVE
			&& skb->h.rh->rri == SNA_RH_RRI_RSP
			&& skb->h.rh->pi == SNA_RH_PI_PAC
			&& skb->h.rh->dr1i == SNA_RH_DR1I_NO_DR1
			&& skb->h.rh->dr2i == SNA_RH_DR2I_NO_DR2) {

			/* we need to make priority worth something. */
			sna_debug(5, "mu needs network priority!\n");
                } else {
			if (hs->tx_pacing.type != SNA_HS_PACING_TYPE_NONE
				&& skb->nh.fh->efi != SNA_TH_EFI_NORM) {
                                sna_tc_send_pacing(hs, skb);
			}
                }
	}
	err = sna_pc_tx_mu(pc, skb, &hs->lfsid);
out:	return err;
}

/** 
 * send the input MU to path control. 
 *
 * @hs: local.common_cb, containing appropriate pacing bits setting.
 * @skb: mu, containing normal-flow or expedited-flow request (or response).
 *
 * the mu is sent to pc or placed on q_pac, if no errors are found.
 * if send pacing is active, the send pacing counts in the cb may be
 * updated.
 */
int sna_tc_send_mu(struct sna_hs_cb *hs, struct sk_buff *skb)
{
	int err = 0;

	sna_debug(5, "init\n");

	/* send expedited data immediately. */
	if (skb->nh.fh->efi) {
		err = sna_tc_send_to_pc(hs, skb);
		goto out;
	}

	/* send normal flow request. */
	if (skb->h.rh->rri == SNA_RH_RRI_REQ) {
		/* perform any crypto on the packet. */
		if (hs->crypto && skb->h.rh->ru == SNA_RH_RU_FMD) {
			sna_debug(1, "FIXME: we don't support crypto\n");
			err = -EINVAL;
			goto out;
		}
		/* check if we are is a congested/pacing condition. */
		if (hs->tx_pacing.type != SNA_HS_PACING_TYPE_NONE) {
			if (!skb_queue_empty(&hs->write_queue)
				|| (skb->nh.fh->mpf == SNA_TH_MPF_BBIU
				&& !(hs->tx_pacing.rpc + hs->tx_pacing.nws))) {
				sna_debug(5, "req placing on write_queue\n");
				skb_queue_tail(&hs->write_queue, skb);
				goto out;
			}
		}
		/* finally send the packet. */
		err = sna_tc_send_to_pc(hs, skb);
		goto out;
	}

	/* send response. */
        if (skb->h.rh->rri == SNA_RH_RRI_RSP) {

		if (hs->tx_pacing.type  == SNA_HS_PACING_TYPE_NONE
			|| !skb_queue_empty(&hs->write_queue)
			|| skb->h.rh->qri == SNA_RH_QRI_NO_QR) {
			err = sna_tc_send_to_pc(hs, skb);
		} else {
			sna_debug(5, "rsp placing on write_queue\n");
                        skb_queue_tail(&hs->write_queue, skb);
		}
		goto out;
        }
out:	return err;
}

/**
 * this procedure sets up session parameters needed by tc.
 *
 * @hs: hs fields to be initialized by tc.
 *
 * the local fields used only by tc are initialized.
 */
int sna_tc_init(struct sna_hs_cb *hs)
{
	sna_ru_bind *bind;
	
	sna_debug(5, "init\n");
	if (!hs->rx_bind)
		return -EINVAL;
	bind = (sna_ru_bind *)hs->rx_bind->data;

	/* initialize defaults. */
	hs->sqn_rcv_cnt			= 0;
	hs->tx_pacing.rpc		= 0;
	hs->rx_pacing.rpc		= 0;
	hs->rlwi			= 0;
	skb_queue_head_init(&hs->write_queue);
	
	/* setup fsp/bidder dependent values. */
	if (hs->type == SNA_HS_TYPE_FSP) {
		hs->max_rx_ru_size	= bind->sec_max_ru_size;
		hs->tx_pacing.nws	= bind->sec_tx_win_size;
		hs->rx_pacing.nws	= bind->sec_rx_win_size;
	} else {
		hs->max_rx_ru_size	= bind->pri_max_ru_size;
		hs->tx_pacing.nws	= bind->pri_tx_win_size;
		hs->rx_pacing.nws	= bind->pri_rx_win_size;
	}

	/* setup pacing information. */
	if (bind->adaptive_pacing) {
		hs->tx_pacing.type	= SNA_HS_PACING_TYPE_ADAPTIVE;
		hs->rx_pacing.type	= SNA_HS_PACING_TYPE_ADAPTIVE;
	} else {
		hs->rx_pacing.type	= SNA_HS_PACING_TYPE_FIXED;
		if (hs->tx_pacing.nws)
			hs->tx_pacing.type = SNA_HS_PACING_TYPE_FIXED;
		else
			hs->tx_pacing.type = SNA_HS_PACING_TYPE_NONE;
	}

	/* setup segment handling. */
	hs->segment_num			= 0;
	hs->segment_len			= 0;
	hs->segment_in			= SNA_HS_SEGMENT_NONE;
	hs->segment_lock		= SPIN_LOCK_UNLOCKED;
	skb_queue_head_init(&hs->segments);
	
	/* finish reset of settings. */
	hs->first_ws			= hs->tx_pacing.nws;
	hs->unsolicited_ipm_outstanding = 0;
	hs->adjust_ipm_ack_outstanding	= 0;
	hs->unsolicited_nws		= 0;
	hs->reserve_flag		= SNA_HS_RESERVE_NO;
	if (bind->whole_biu)
		hs->segmenting		= 0;
	else
		hs->segmenting		= 1;
	if (bind->crypto_supp) {
		hs->crypto		= 1;
		/* need to build and send crv information. */
		sna_debug(1, "FIXME: build and send crv information!\n");
	} else
		hs->crypto		= 0;
	
	return 0;
}

#if defined(CONFIG_SNA_HS) || defined(CONFIG_SNA_HS_MODULE)
/* Handle the exchange of the Cryptograhpy Verification (CRV) request. */
static int sna_tc_exchange_crv(struct sna_init_hs *init_hs)
{
	struct sna_hs_local *local = sna_hs_find_local(init_hs->pc_id);
	struct sna_tc_cb *tcb = &local->tcb;
	struct sna_mu *mu = NULL;
	int err;

	if (init_hs->type == SNA_HS_PRI) {
		mu = sna_tc_build_crv(init_hs);
		local->tcb.ccb.caller = SNA_HS;
		sna_send_mu(mu, &tcb->ccb);

		mu = sna_catch_mu(local);	/* Spin till rcv the CRV. */

		err = sna_tc_crv_format_chk(mu);
		if (err == 0)
			local->sense = *(__u32 *)mu->biu->ru.ru.raw;
		sna_free_buffer(mu);
	} else {
		mu = sna_catch_mu(local);	/* Spin till rcv the CRV. */
		err = sna_tc_crv_format_chk(mu);
		if (err == 0) {
			/* Check the crypto test values. */
			if (err) {
				local->sense = 0x08350001;
				sna_free_buffer(mu);
			} else {
				local->sense = 0;
				mu->biu->rh.rh.rsp_h.rri = SNA_RH_RRI_RSP;
				mu->biu->rh.rh.rsp_h.rti = SNA_RH_RTI_POS;
				local->tcb.ccb.caller = SNA_HS;
				sna_send_mu(mu, &tcb->ccb);
			}
		}
	}
	return 0;
}

/* Build and MU (containing the CRV request) by appropriately initializing
 * the TH, RH, and RU fields.
 */
static struct sna_mu *sna_tc_build_crv(struct sna_init_hs *init_hs)
{
	struct sna_mu *mu = NULL;
	struct sna_req_h *req_h = &mu->biu->rh.rh.req_h;

	mu = sna_get_buffer(SNA_BM_TYPE_DEMAND, init_hs->dynamic_buf_pool_id, 
		0, SNA_BM_NO_WAIT);

	mu->biu->th.fid.fid0.efi 		= SNA_TH_EFI_EXP;
	mu->biu->th.fid.fid0.snf.number 	= *(__u16 *)jiffies;
	mu->biu->th.fid.fid0.mpf		= SNA_TH_MPF_WHOLE_BIU;

	req_h->rri	= SNA_RH_RRI_REQ;
	req_h->ru	= SNA_RH_RU_SC;
	req_h->fi	= SNA_RH_FI_FMH;
	req_h->sdi	= SNA_RH_SDI_NO_SD;
	req_h->bci	= SNA_RH_BCI_BC;
	req_h->eci	= SNA_RH_ECI_EC;
	SNA_DFC_SET_RQD1(req_h);
	req_h->rlwi	= SNA_RH_RLWI_NO_RLW;
	req_h->qri	= SNA_RH_QRI_NO_QR;
	req_h->pi	= SNA_RH_PI_NO_PAC;
	req_h->bbi	= SNA_RH_BBI_NO_BB;
	req_h->cdi	= SNA_RH_CDI_NO_CD;
	req_h->csi	= SNA_RH_CSI_CODE0;
	req_h->edi	= SNA_RH_EDI_NO_ED;
	req_h->pdi	= SNA_RH_PDI_NO_PD;
	req_h->cebi	= SNA_RH_CEBI_NO_CEB;

	/* More crypto stuff.. */

	return mu;
}

/* Check the RH bits of the CRV request or RSP(CRV) received from path_control
 * (from the partner half_session).
 */
static int sna_tc_crv_format_chk(struct sna_mu *mu)
{
	struct sna_hs_local *local = sna_hs_find_local(mu->hs_id);
	struct sna_req_h *req_h = &mu->biu->rh.rh.req_h;
	struct sna_rsp_h *rsp_h = &mu->biu->rh.rh.rsp_h;
	struct sna_fid0 *th = &mu->biu->th.fid.fid0;
	int length;

	/* Calculate the length of the RU data. */
	length = 0;

	if (req_h->rri == SNA_RH_RRI_REQ) {
		if (local->half_session == SNA_HS_PRI)
			local->sense = 0x20090000;
		if (req_h->ru != SNA_RH_RU_SC)
			local->sense = 0x20090000;
		if ((req_h->sdi == SNA_RH_SDI_NO_SD && length < 1)
			|| (req_h->sdi == SNA_RH_SDI_SD && length < 5))
			local->sense = 0x10020000;
		if ((req_h->sdi == SNA_RH_SDI_NO_SD 
			&& mu->biu->ru.ru.crv.rq_code != 0xC0)
			||(req_h->sdi == SNA_RH_SDI_SD 
			&& mu->biu->ru.ru.crv.rq_code != 0xC0))
			local->sense = 0x20090000;
		if (req_h->fi == SNA_RH_FI_NO_FMH)
			local->sense = 0x400F0000;

		if (req_h->sdi == SNA_RH_SDI_SD)
			local->sense = *(__u32 *)mu->biu->ru.ru.raw;
		if (req_h->bci == SNA_RH_BCI_NO_BC)
			local->sense = 0x400B0000;
		if (req_h->eci == SNA_RH_ECI_NO_EC)
	 		local->sense = 0x400B0000;
		if (SNA_DFC_RQD1(req_h))
			local->sense = 0x40140000;
		if (th->efi != SNA_TH_EFI_EXP)
			local->sense = 0x40110000;
		if (req_h->qri == SNA_RH_QRI_QR)
			local->sense = 0x40150000;
		if (req_h->pi == SNA_RH_PI_PAC)
			local->sense = 0x40080000;
		if (req_h->bbi == SNA_RH_BBI_BB)
			local->sense = 0x400C0000;
		if (req_h->ebi == SNA_RH_EBI_EB)
			local->sense = 0x400C0000;
		if (req_h->cdi == SNA_RH_CDI_CD)
			local->sense = 0x400D0000;
		if (req_h->csi == SNA_RH_CSI_CODE1)
			local->sense = 0x40100000;
		if (req_h->edi == SNA_RH_EDI_ED)
			local->sense = 0x40160000;
		if (req_h->pdi == SNA_RH_PDI_PD)
			local->sense = 0x40170000;
		if (req_h->cebi == SNA_RH_CEBI_CEB)
			local->sense = 0x400C0000;
	} else {	/* CRV Response */
		if (local->half_session == SNA_HS_SEC)
			local->sense = 0x20090000;
		if (req_h->ru != SNA_RH_RU_SC)
			local->sense = 0x20090000;
		if ((rsp_h->rti && length < 1) 
			|| (rsp_h->rti == SNA_RH_RTI_NEG && length < 5))
			local->sense = 0x10020000;
		if ((req_h->sdi != SNA_RH_SDI_SD 
			&& mu->biu->ru.ru.crv.rq_code != 0xC0)
			||(req_h->sdi == SNA_RH_SDI_SD 
			&& mu->biu->ru.ru.crv.rq_code != 0xC0))
			local->sense = 0x20090000;
		if (req_h->fi == SNA_RH_FI_NO_FMH)
			local->sense = 0x400F0000;
		if (req_h->bci == SNA_RH_BCI_NO_BC)
			local->sense = 0x400B0000;
		if (req_h->eci == SNA_RH_ECI_NO_EC)
			local->sense = 0x400B0000;
		if (th->efi != SNA_TH_EFI_EXP)
			local->sense = 0x40110000;
		if (req_h->dr1i != SNA_RH_DR1I_DR1 
			|| req_h->dr2i == SNA_RH_DR2I_DR2)
			local->sense = 0x40140000;
		if ((rsp_h->rti == SNA_RH_RTI_POS && req_h->sdi == SNA_RH_SDI_SD)
			|| (rsp_h->rti == SNA_RH_RTI_NEG && req_h->sdi== SNA_RH_SDI_NO_SD))
			local->sense = 0x40130000;
		if (req_h->qri == SNA_RH_QRI_QR)
			local->sense = 0x40150000;
		if (req_h->pi == SNA_RH_PI_PAC)
			local->sense = 0x40080000;
	}
	return 0;
}

/* This procedure receives buffers_reserved signals from the
 * BM, updates the appropriate pacing counts in the local->common_cb
 * and builds and sends the appropriate pacing response.
 */
int sna_buffers_reserved(struct sna_mu *mu)
{
	struct sna_hs_local *local = sna_hs_find_local(mu->hs_id);
	struct sna_common_cb *cb = &local->tcb.ccb;
	struct sna_fid0 *th = &mu->biu->th.fid.fid0;
	struct sna_ipm_extension *ipm_ex = NULL;

	if (1) {	/* reserved_buf_reduce. */
		cb->receive_pacing.unsolicited_ipm_outstanding = 1;
		cb->receive_pacing.unsolicited_nws = 0;
		mu->dcf = sizeof(struct sna_req_h) + sizeof(struct sna_ru_ipm);
		ipm_ex->type = SNA_IPM_TYPE_UNSOLICITED;
		ipm_ex->rwi = SNA_IPM_RWI_RESET_WINDOW;
		ipm_ex->format_indicator = SNA_IPM_FORMAT_0;
		ipm_ex->nws = cb->receive_pacing.nws;
	} else
		mu->dcf = th->dcf;

	th->mpf		= SNA_TH_MPF_WHOLE_BIU;
	th->efi 	= SNA_TH_EFI_EXP;
	th->snf.number 	= 0;

	sna_send_to_pc(mu, cb);
	return 0;
}

/* Decipher an enciphered message. */
static int sna_tc_decipher_ru(struct sna_mu *mu)
{
	struct sna_req_h *req_h = &mu->biu->rh.rh.req_h;
	struct sna_hs_local *local = sna_hs_find_local(mu->hs_id);
	int err;
	__u8 len, pad_cnt = 0;

	err = sna_des_decipher(mu);
	if (err)
		local->sense = 0x08480000;
	else {
		if (req_h->pdi == SNA_RH_PDI_PD) {
			len = mu->dcf - sizeof(mu->biu->ru);
			pad_cnt = mu->biu->ru.ru.raw[len];
			if (pad_cnt < 1 || pad_cnt > 7)
				local->sense = 0x10010000;
			else {
				mu->dcf -= pad_cnt;
				req_h->pdi = SNA_RH_PDI_NO_PD;
			}
		}
	}
	return 0;
}
#endif /* CONFIG_SNA_LU62_HS || CONFIG_SNA_LU62_HS_MODULE */
