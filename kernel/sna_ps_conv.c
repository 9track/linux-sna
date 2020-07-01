/* sna_ps_conv.c: Linux Systems Network Architecture implementation
 * - SNA LU 6.2 Presentation Service Conversations (PS.CONV)
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
#include <linux/appc.h>
#include <linux/cpic.h>

#include "sna_common.h"

static u_int32_t sna_ps_conv_attach_sqn = 1;

static struct sk_buff *sna_ps_conv_create_and_init_limited_mu(struct sna_rcb *rcb);

/**
 * this finite-state machine maintains the posting status of a conversation.
 */
static int sna_ps_conv_fsm_post(struct sna_rcb *rcb, int signal)
{
	sna_debug(5, "init\n");
	if (signal == SNA_PS_FSM_POST_INPUT_POST_ON_RECEIPT) {
		if (rcb->fsm_post_state == SNA_PS_FSM_POST_STATE_RESET
			|| rcb->fsm_post_state == SNA_PS_FSM_POST_STATE_POSTED) {
			rcb->fsm_post_state = SNA_PS_FSM_POST_STATE_PEND_POSTED;
			goto out;
		}
		goto out;
	}
	if (signal == SNA_PS_FSM_POST_INPUT_TEST
		|| signal == SNA_PS_FSM_POST_INPUT_WAIT) {
		if (rcb->fsm_post_state == SNA_PS_FSM_POST_STATE_POSTED) {
			rcb->fsm_post_state = SNA_PS_FSM_POST_STATE_RESET;
			goto out;
		}
		goto out;
	}
	if (signal == SNA_PS_FSM_POST_INPUT_RECEIVE_IMMEDIATE) {
		if (rcb->fsm_post_state == SNA_PS_FSM_POST_STATE_PEND_POSTED
			|| rcb->fsm_post_state == SNA_PS_FSM_POST_STATE_POSTED) {
			rcb->fsm_post_state = SNA_PS_FSM_POST_STATE_RESET;
			goto out;
		}
		goto out;
	}
	if (signal == SNA_PS_FSM_POST_INPUT_POST) {
		if (rcb->fsm_post_state == SNA_PS_FSM_POST_STATE_PEND_POSTED) {
			rcb->fsm_post_state = SNA_PS_FSM_POST_STATE_POSTED;
			goto out;
		}
		goto out;
	}
out:	return 0;
}

/**
 * this finite-state machine remembers if any error or failure mu records
 * (either from hs to ps or rm to ps) have been received by ps. this
 * knowledge is maintained until the information reflected by the records can
 * be passed to the transaction program. the meanings of the states are as
 * follows:
 *  - no_requests:
 *  - rcvd_error:
 *  - conv_failure_protocol_error:
 *  - conv_failure_son:
 */
static int sna_ps_conv_fsm_error_or_failure(struct sna_rcb *rcb, int signal)
{
	sna_debug(5, "init: signal:%d rcb->fsm_err_or_fail_state:%d\n",
		signal, rcb->fsm_err_or_fail_state);
	if (signal == SNA_PS_FSM_ERR_OR_FAIL_INPUT_CONV_FAIL_PROTOCOL) {
		if (rcb->fsm_err_or_fail_state == SNA_PS_FSM_ERR_OR_FAIL_STATE_NO_REQUESTS
			|| rcb->fsm_err_or_fail_state == SNA_PS_FSM_ERR_OR_FAIL_STATE_RCVD_ERROR) {
			rcb->fsm_err_or_fail_state = SNA_PS_FSM_ERR_OR_FAIL_STATE_CONV_FAILURE_PROTOCOL_ERROR;
			goto out;
		}
		goto out;
	}
	if (signal == SNA_PS_FSM_ERR_OR_FAIL_INPUT_CONV_FAIL_SON) {
		if (rcb->fsm_err_or_fail_state == SNA_PS_FSM_ERR_OR_FAIL_STATE_NO_REQUESTS
			|| rcb->fsm_err_or_fail_state == SNA_PS_FSM_ERR_OR_FAIL_STATE_RCVD_ERROR) {
			rcb->fsm_err_or_fail_state = SNA_PS_FSM_ERR_OR_FAIL_STATE_CONV_FAILURE_SON;
			goto out;
		}
		goto out;
	}
	if (signal == SNA_PS_FSM_ERR_OR_FAIL_INPUT_RECEIVE_ERROR) {
		if (rcb->fsm_err_or_fail_state == SNA_PS_FSM_ERR_OR_FAIL_STATE_NO_REQUESTS) {
			rcb->fsm_err_or_fail_state = SNA_PS_FSM_ERR_OR_FAIL_STATE_RCVD_ERROR;
			/* if send mu exists then reset it. */
			goto out;
		}
		goto out;
	}
	if (signal == SNA_PS_FSM_ERR_OR_FAIL_INPUT_RESET) {
		if (rcb->fsm_err_or_fail_state == SNA_PS_FSM_ERR_OR_FAIL_STATE_RCVD_ERROR) {
			rcb->fsm_err_or_fail_state = SNA_PS_FSM_ERR_OR_FAIL_STATE_NO_REQUESTS;
		}
		goto out;
	}
out:	sna_debug(5, "fini: rcb->fsm_err_or_fail_state:%d\n", rcb->fsm_err_or_fail_state);
	return 0;
}

static int sna_ps_conv_fsm_conv_output_a(struct sna_rcb *rcb)
{
	struct sk_buff *skb;

	sna_debug(5, "init\n");
	if (rcb->fsm_err_or_fail_state == SNA_PS_FSM_ERR_OR_FAIL_STATE_NO_REQUESTS
		|| rcb->fsm_err_or_fail_state == SNA_PS_FSM_ERR_OR_FAIL_STATE_RCVD_ERROR) {
		skb = sna_ps_conv_create_and_init_limited_mu(rcb);
		if (!skb)
			return -ENOMEM;
		SNA_SKB_CB(skb)->type = SNA_CTRL_T_PREPARE_TO_RCV_FLUSH;
		sna_hs_tx_ps_mu_data(rcb, skb);
	}
	return 0;
}

static int sna_ps_conv_fsm_conv_output_b(struct sna_rcb *rcb)
{
	sna_debug(5, "init\n");
	sna_ps_conv_fsm_error_or_failure(rcb, SNA_PS_FSM_ERR_OR_FAIL_INPUT_RESET);
	return 0;
}

/**
 * this finite-state machine maintains the status of a conversation resource.
 * the states have the following meanings:
 *  - reset: conversation initial state, the program can allocate it.
 *  - send_state: the program can send data, request confirmation, or
 *    request sync point.
 *  - rcv_state: receive, the program can receive information from the
 *    remote program.
 *  - rcvd_confirm: received confirm, ps received the confirm indicator
 *    from the hs.
 */
static int sna_ps_conv_fsm_conversation(int who, int action,
	struct sna_rcb *rcb, int readonly)
{
	int err = 0;

	sna_debug(5, "init: who:%d action:%d readonly:%d rcb->fsm_conv_state=%d\n",
		who, action, readonly, rcb->fsm_conv_state);
	if (who == SNA_PS_FSM_CONV_INPUT_NONE) {
		switch (action) {
			case SNA_PS_FSM_CONV_INPUT_ALLOCATE:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RESET) {
					if (!readonly)
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_SEND_STATE;
					goto out;
				}
				break;
		}
		goto out;
	}
	if (who == SNA_PS_FSM_CONV_INPUT_S) {
		switch (action) {
			case SNA_PS_FSM_CONV_INPUT_SEND_DATA:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCV_STATE
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_SEND
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_DEALL
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_PREP_TO_RCV_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_DEALL_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_END_CONV) {
					err = 1;
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_PREP_TO_RCV_FLUSH:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_SEND_STATE) {
					if (!readonly)
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_RCV_STATE;
					goto out;
				}
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCV_STATE
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_SEND
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_DEALL
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_PREP_TO_RCV_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_DEALL_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_END_CONV) {
					err = 1;
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_PREP_TO_RCV_CONFIRM:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_SEND_STATE) {
					if (!readonly)
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_RCV_STATE;
					goto out;
				}
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCV_STATE
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_SEND
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_DEALL
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_PREP_TO_RCV_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_DEALL_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_END_CONV) {
					err = 1;
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_PREP_TO_RCV_DEFER:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_SEND_STATE) {
					if (!readonly)
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_PREP_TO_RCV_DEFER;
					goto out;
				}
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCV_STATE
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_SEND
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_DEALL
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_PREP_TO_RCV_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_DEALL_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_END_CONV) {
					err = 1;
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_FLUSH:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_PREP_TO_RCV_DEFER) {
					if (!readonly)
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_RCV_STATE;
					goto out;
				}
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_DEALL_DEFER) {
					if (!readonly)
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_RESET;
					goto out;
				}
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCV_STATE
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_SEND
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_DEALL
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_END_CONV) {
					err = 1;
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_CONFIRM:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_PREP_TO_RCV_DEFER) {
					if (!readonly)
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_RCV_STATE;
					goto out;
				}
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_DEALL_DEFER) {
					if (!readonly)
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_PEND_DEALL;
					goto out;
				}
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCV_STATE
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_SEND
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_DEALL
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_END_CONV) {
					err = 1;
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_SEND_ERROR:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCV_STATE
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_SEND
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_DEALL) {
					if (!readonly)
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_SEND_STATE;
					goto out;
				}
				if (rcb->fsm_conv_state ==  SNA_PS_FSM_CONV_STATE_PREP_TO_RCV_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_DEALL_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_END_CONV) {
					err = 1;
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_RECEIVE_AND_WAIT:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_SEND_STATE) {
					if (!readonly) {
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_RCV_STATE;
						sna_ps_conv_fsm_conv_output_a(rcb);
					}
					goto out;
				}
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_SEND
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_DEALL
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_PREP_TO_RCV_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_DEALL_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_END_CONV) {
					err = 1;
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_POST_ON_RECEIPT:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_SEND_STATE
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_SEND
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_DEALL
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_PREP_TO_RCV_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_DEALL_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_END_CONV) {
					err = 1;
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_WAIT:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_SEND_STATE
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_SEND
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_DEALL
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_PREP_TO_RCV_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_DEALL_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_END_CONV) {
					err = 1;
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_TEST_POSTED:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_SEND_STATE
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_SEND
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_DEALL
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_PREP_TO_RCV_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_DEALL_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_END_CONV) {
					err = 1;
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_TEST_RQ_TO_SEND_RCVD:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_SEND
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_DEALL
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_END_CONV) {
					err = 1;
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_RECEIVE_IMMEDIATE:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_SEND_STATE
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_SEND
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_DEALL
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_PREP_TO_RCV_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_DEALL_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_END_CONV) {
					err = 1;
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_REQUEST_TO_SEND:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_PREP_TO_RCV_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_DEALL_DEFER) {
					err = 1;
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_CONFIRMED:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM) {
					if (!readonly)
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_RCV_STATE;
					goto out;
				}
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_SEND) {
					if (!readonly)
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_SEND_STATE;
					goto out;
				}
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_DEALL) {
					if (!readonly)
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_END_CONV;
					goto out;
				}
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_SEND_STATE
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCV_STATE
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_PREP_TO_RCV_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_DEALL_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_END_CONV) {
					err = 1;
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_DEALLOCATE_FLUSH:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_SEND_STATE) {
					if (!readonly)
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_RESET;
					goto out;
				}
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCV_STATE
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_SEND
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_DEALL
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_PREP_TO_RCV_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_DEALL_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_END_CONV) {
					err = 1;
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_DEALLOCATE_CONFIRM:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_SEND_STATE) {
					if (!readonly)
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_PEND_DEALL;
					goto out;
				}
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCV_STATE
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_SEND
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_DEALL
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_PREP_TO_RCV_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_DEALL_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_END_CONV) {
					err = 1;
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_DEALLOCATE_DEFER:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_SEND_STATE) {
					if (!readonly)
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_DEALL_DEFER;
					goto out;
				}
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCV_STATE
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_SEND
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_DEALL
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_PREP_TO_RCV_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_DEALL_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_END_CONV) {
					err = 1;
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_DEALLOCATE_ABEND:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_SEND_STATE
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCV_STATE
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_SEND
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_DEALL
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_PREP_TO_RCV_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_DEALL_DEFER) {
					if (!readonly)
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_RESET;
					goto out;
				}
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_END_CONV) {
					err = 1;
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_DEALLOCATE_LOCAL:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_END_CONV) {
					if (!readonly)
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_RESET;
					goto out;
				}
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_SEND_STATE
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCV_STATE
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_SEND
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_DEALL
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_PREP_TO_RCV_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_DEALL_DEFER) {
					err = 1;
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_GET_ATTRIBUTES:
				break;
		}
		goto out;
	}
	if (who == SNA_PS_FSM_CONV_INPUT_R) {
		switch (action) {
			case SNA_PS_FSM_CONV_INPUT_ATTACH:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RESET) {
					if (!readonly)
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_RCV_STATE;
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_SEND_INDICATOR:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCV_STATE) {
					if (!readonly)
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_SEND_STATE;
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_CONFIRM_INDICATOR:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCV_STATE) {
					if (!readonly)
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM;
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_CONFIRM_SEND_IND:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCV_STATE) {
					if (!readonly)
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_SEND;
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_CONFIRM_DEALLOC_IND:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCV_STATE) {
					if (!readonly)
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_DEALL;
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_PROGRAM_ERROR_RC:
			case SNA_PS_FSM_CONV_INPUT_SERVICE_ERROR_RC:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCV_STATE) {
					if (!readonly)
						sna_ps_conv_fsm_conv_output_b(rcb);
					goto out;
				}
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_SEND_STATE
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_PREP_TO_RCV_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_DEALL_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_PEND_DEALL) {
					if (!readonly) {
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_RCV_STATE;
						sna_ps_conv_fsm_conv_output_b(rcb);
					}
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_DEALLOC_NORMAL_RC:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_SEND_STATE
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCV_STATE) {
					if (!readonly)
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_END_CONV;
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_DEALLOC_ABEND_RC:
			case SNA_PS_FSM_CONV_INPUT_RESOURCE_FAILURE_RC:
			case SNA_PS_FSM_CONV_INPUT_ALLOCATION_ERROR_RC:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_SEND_STATE
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCV_STATE
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_PREP_TO_RCV_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_DEALL_DEFER
					|| rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_PEND_DEALL) {
					if (!readonly) {
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_END_CONV;
						sna_ps_conv_fsm_conv_output_b(rcb);
					}
					goto out;
				}
				break;
			case SNA_PS_FSM_CONV_INPUT_DEALLOCATED_IND:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_PEND_DEALL) {
					if (!readonly)
						rcb->fsm_conv_state = SNA_PS_FSM_CONV_STATE_RESET;
					goto out;
				}
				break;
		}
		goto out;
	}
out:	sna_debug(5, "fini: rcb->fsm_conv_state:%d\n", rcb->fsm_conv_state);
	return err;
}

/**
 * this procedure processes receive error conditions that require
 * the session to be deactivated.
 */
static int sna_ps_conv_protocol_error(struct sna_rcb *rcb, u_int32_t sense)
{
	sna_debug(5, "init\n");
	sna_debug(5, "FIXME: finish me!\n");
	return 0;
}

/**
 * this procedure removes from the rcv receive buffer a deallocate mu.
 * if the receive buffer is empty, ps waits until an mu whose mu.hs_to_ps.type
 * field is set to deallocate is received.
 */
static int sna_ps_conv_get_deallocate_from_hs(struct sna_rcb *rcb)
{
	sna_debug(5, "init\n");
	sna_debug(5, "FIXME: finish me!\n");
	return 0;
}

/**
 * this procedure sets the return_code parameter of the passed transaction
 * program verb based upon the passed sense data.
 *
 * @rcb: the rcb corresponding to the resource to which the fmh-7 applies.
 * @sense: the received fmh-7 sense data.
 *
 * the return_code parameter of the verb is set, based upon the sense
 * data passed from the received fmh-7.
 */
static int sna_ps_conv_set_fmh7_rc(struct sna_rcb *rcb, u_int32_t sense)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (sense) {
		case 0x10086021:	/* ALLOCATION_ERROR_TPN_NOT_RECOGNIZED. */
			if (!err)
				err = AC_RC_TP_NAME_NOT_RECOGNIZED;
		case 0x10086031:	/* ALLOCATION_ERROR_PIP_NOT_ALLOWED. */
			if (!err)
				err = AC_RC_PIP_NOT_ALLOWED;
		case 0x10086032:	/* ALLOCATION_ERROR_PIP_NOT_SPECIFIED_CORRECTLY. */
			if (!err)
				err = AC_RC_PIP_NOT_SPECIFIED_CORRECTLY;
		case 0x10086034:	/* ALLOCATION_ERROR_CONVERSATION_TYPE_MISMATCH. */
			if (!err)
				err = AC_RC_CONVERSATION_TYPE_MISMATCH;
		case 0x10086041:	/* ALLOCATION_ERROR_SYNC_LEVEL_NOT_SUPPORTED_BY_PGM. */
			if (!err)
				err = AC_RC_SYNC_LEVEL_NOT_SUPPORTED;
		case 0x080F0983:	/* ALLOCATION_ERROR_ACCESS_DENIED. */
		case 0x080F6051:	/* ALLOCATION_ERROR_SECURITY_NOT_VALID. */
		case 0x080F8000:	/* ALLOCATION_ERROR_SECURITY_NOT_VALID. */
		case 0x080F8001:        /* ALLOCATION_ERROR_SECURITY_NOT_VALID. */
		case 0x080F8002:        /* ALLOCATION_ERROR_SECURITY_NOT_VALID. */
		case 0x080F8003:        /* ALLOCATION_ERROR_SECURITY_NOT_VALID. */
		case 0x080F8004:        /* ALLOCATION_ERROR_SECURITY_NOT_VALID. */
		case 0x080F8005:        /* ALLOCATION_ERROR_SECURITY_NOT_VALID. */
		case 0x080F8006:        /* ALLOCATION_ERROR_SECURITY_NOT_VALID. */
		case 0x080F8007:        /* ALLOCATION_ERROR_SECURITY_NOT_VALID. */
		case 0x080F8008:        /* ALLOCATION_ERROR_SECURITY_NOT_VALID. */
		case 0x080F8009:        /* ALLOCATION_ERROR_SECURITY_NOT_VALID. */
		case 0x080F800A:        /* ALLOCATION_ERROR_SECURITY_NOT_VALID. */
		case 0x080F800B:        /* ALLOCATION_ERROR_SECURITY_NOT_VALID. */
		case 0x080F8101:	/* ALLOCATION_ERROR_SECURITY_NOT_VALID. */
		case 0x080F8102:        /* ALLOCATION_ERROR_SECURITY_NOT_VALID. */
		case 0x080F8103:        /* ALLOCATION_ERROR_SECURITY_NOT_VALID. */
		case 0x080F8104:        /* ALLOCATION_ERROR_SECURITY_NOT_VALID. */
			if (!err)
				err = AC_RC_SECURITY_NOT_VALID;
		case 0x084B6031:	/* ALLOCATION_ERROR_TP_NOT_AVAIL_RETRY. */
			if (!err)
				err = AC_RC_TP_NOT_AVAIL_RETRY;
		case 0x084C0000:	/* ALLOCATION_ERROR_TP_NOT_AVAIL_NO_RETRY. */
			if (!err)
				err = AC_RC_TP_NOT_AVAIL_NO_RETRY;
			sna_ps_conv_get_deallocate_from_hs(rcb);
			if (rcb->fsm_conv_state != SNA_PS_FSM_CONV_STATE_END_CONV) {
				sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_R,
					SNA_PS_FSM_CONV_INPUT_ALLOCATION_ERROR_RC, rcb, 0);
			}
			break;
		case 0x1008600B:	/* RESOURCE_FAILURE_NO_RETRY. */
			err = AC_RC_RESOURCE_FAILURE_NO_RETRY;
			sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_R,
				SNA_PS_FSM_CONV_INPUT_RESOURCE_FAILURE_RC, rcb, 0);
			break;
		case 0x08890000:	/* PROG_ERROR_NO_TRUNC or PROG_ERROR_PURGING. */
			if (rcb->fsm_err_or_fail_state == SNA_PS_FSM_ERR_OR_FAIL_STATE_RCVD_ERROR)
				err = AC_RC_PROG_ERROR_PURGING;
			else
				err = AC_RC_PROG_ERROR_NO_TRUNC;
			sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_R,
				SNA_PS_FSM_CONV_INPUT_PROGRAM_ERROR_RC, rcb, 0);
			break;
		case 0x08890001:	/* PROG_ERROR_TRUNC. */
			err = AC_RC_PROG_ERROR_PURGING;
			sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_R,
				SNA_PS_FSM_CONV_INPUT_PROGRAM_ERROR_RC, rcb, 0);
			break;
		case 0x08890100:	/* SVC_ERROR_NO_TRUNC or SVC_ERROR_PURGING. */
			if (rcb->fsm_err_or_fail_state == SNA_PS_FSM_ERR_OR_FAIL_STATE_RCVD_ERROR)
				err = AC_RC_SVC_ERROR_PURGING;
			else
				err = AC_RC_SVC_ERROR_NO_TRUNC;
			sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_R,
				SNA_PS_FSM_CONV_INPUT_PROGRAM_ERROR_RC, rcb, 0);
			break;
		case 0x08890101:	/* SVC_ERROR_TRUNC. */
			err = AC_RC_SVC_ERROR_TRUNC;
			sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_R,
				SNA_PS_FSM_CONV_INPUT_PROGRAM_ERROR_RC, rcb, 0);
			break;
		case 0x08640000:	/* DEALLOCATE_ABEND_PROG. */
			err = AC_RC_DEALLOCATE_ABEND_PROG;
			sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_R,
				SNA_PS_FSM_CONV_INPUT_DEALLOC_ABEND_RC, rcb, 0);
			break;
		case 0x08640001:	/* DEALLOCATE_ABEND_SVC. */
			err = AC_RC_DEALLOCATE_ABEND_SVC;
			sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_R,
				SNA_PS_FSM_CONV_INPUT_DEALLOC_ABEND_RC, rcb, 0);
			break;
		case 0x08640002:	/* DEALLOCATE_ABEND_TIMER. */
			err = AC_RC_DEALLOCATE_ABEND_TIMER;
			sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_R,
				SNA_PS_FSM_CONV_INPUT_DEALLOC_ABEND_RC, rcb, 0);
			break;
		default:
			sna_ps_conv_protocol_error(rcb, sense);
			err = AC_RC_RESOURCE_FAILURE_NO_RETRY;
			sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_R,
				SNA_PS_FSM_CONV_INPUT_RESOURCE_FAILURE_RC, rcb, 0);
			break;
	}
	return err;
}

/**
 * this procedure is invoked upon encountering an fmh-7 with log_data
 * following it in the hs_to_ps_buffer_list.
 */
static int sna_ps_conv_process_fmh7_log_data_proc(struct sna_rcb *rcb, u_int32_t sense)
{
	sna_debug(5, "init\n");
	sna_debug(5, "FIXME: finish me!\n");
	return 0;
}

/**
 * this procedure is invoked upon encountering an fmh-7 in the
 * hs_to_ps_buffer_queue. the return_code parameter of the passed
 * transaction program verb is set based upon the sense data carried in the
 * fmh-7. if the fmh-7 indicates that log data follows, this procedure
 * simulates a receive_and_wait verb and causes receive processing to take
 * place. the receive_and_wait processing waits for a logical record, which
 * consists of the log data, to arrive from hs. if the sense data carried
 * in the fmh-7 indicates a type of deallocate_abend_* this procedure
 * retrieves the deallocation notification from the receive buffer before
 * returning to the transaction program.
 *
 * @rcb: the rcb corresponding to the resource to which the fmh-7 applies.
 * @skb: the mu containing the received fmh-7
 */
static int sna_ps_conv_process_fmh7_proc(struct sna_rcb *rcb,
	struct sk_buff *skb)
{
	u_int32_t err = 0;
	sna_fmh7 *fm;
	int logi;

	sna_debug(5, "init\n");
	if (skb->len < sizeof(sna_fmh7)) {
		err = 0x10086000;
		goto error;
	}
	fm = (sna_fmh7 *)skb->data;
	if (fm->len != skb->len) {
		err = 0x10086000;
		goto error;
	}
	err  = ntohl(fm->sense);
	logi = fm->logi;
	kfree_skb(skb);
	if (logi) {
		sna_ps_conv_process_fmh7_log_data_proc(rcb, err);
		goto out;
	}
	if (err != 0x08640000 && err != 0x08640001 && err != 0x08640002)
		goto out;
	switch (rcb->fsm_err_or_fail_state) {
		case SNA_PS_FSM_ERR_OR_FAIL_STATE_NO_REQUESTS:
			sna_ps_conv_get_deallocate_from_hs(rcb);
			err = sna_ps_conv_set_fmh7_rc(rcb, err);
			break;
		case SNA_PS_FSM_ERR_OR_FAIL_STATE_CONV_FAILURE_PROTOCOL_ERROR:
			err = sna_ps_conv_set_fmh7_rc(rcb, err);
			sna_ps_conv_fsm_error_or_failure(rcb,
				SNA_PS_FSM_ERR_OR_FAIL_INPUT_CONV_FAIL_PROTOCOL);
			break;
		case SNA_PS_FSM_ERR_OR_FAIL_STATE_CONV_FAILURE_SON:
			err = sna_ps_conv_set_fmh7_rc(rcb, err);
			sna_ps_conv_fsm_error_or_failure(rcb,
				SNA_PS_FSM_ERR_OR_FAIL_INPUT_CONV_FAIL_SON);
			break;
		default:
			err = sna_ps_conv_set_fmh7_rc(rcb, err);
			break;
	}
	goto out;

error:	sna_ps_conv_protocol_error(rcb, err);
	sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_R,
		SNA_PS_FSM_CONV_INPUT_RESOURCE_FAILURE_RC, rcb, 0);
	rcb->rx_post.max_len = 0;
out:	return err;
}

/**
 * this procedure is invoked upon receipt of a receive_error from hs. the
 * next element expected in the hs_to_ps_buffer_list is an fmh-7. if the
 * next element in the buffer is an fmh-7, it is removed from the buffer
 * and processed (the return_code parameter of the passed verb parameters
 * is set based upon the sense data carried in the fmh-7). if the next
 * element is not an fmh-7, the partner lu has violated the protocl and the
 * session over which the protocol violation occurred is deactivated in an
 * implementation-dependent fashion.
 *
 * @rcb: the rcb corresponding to the resource specified in parameters
 *       of the verb.
 *
 * the state of fsm_post is changes to reset. if the record in the buffer is
 * an fmh-7, then is processed; otherwise, the return code and
 * fsm_conversation are set to indicate the protocol violation, and the
 * session is deactivated.
 */
static int sna_ps_conv_dequeue_fmh7_proc(struct sna_rcb *rcb)
{
	int err = AC_RC_RESOURCE_FAILURE_NO_RETRY;
	struct sk_buff *skb;

	sna_debug(5, "init\n");
	sna_ps_conv_fsm_post(rcb, SNA_PS_FSM_POST_INPUT_RECEIVE_IMMEDIATE);
	skb = skb_peek(&rcb->hs_to_ps_buffer_queue);
	if (!skb || skb->len < sizeof(sna_fmh))
		goto error;
	if (((sna_fmh *)skb->data)->type != SNA_FMH_TYPE_7)
		goto error;
	skb = skb_dequeue(&rcb->hs_to_ps_buffer_queue);
	if (!skb)
		goto error;
	err = sna_ps_conv_process_fmh7_proc(rcb, skb);
	goto out;

error:	sna_ps_conv_protocol_error(rcb, 0x1008201D);
	sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_R,
		SNA_PS_FSM_CONV_INPUT_RESOURCE_FAILURE_RC, rcb, 0);
out:    return err;
}

/**
 * this procedure creates and initializes an mu in a buffer from the limited
 * buffer pool associated with the limited_buffer_pool_id value stored in
 * the rcb.
 *
 * @rcb: the rcb corresponding to the conversation for which the mu
 *       is being requested.
 *
 * appropriate fields in the mu are initialized, the mu is returned
 * to the calling procedure.
 */
static struct sk_buff *sna_ps_conv_create_and_init_limited_mu(struct sna_rcb *rcb)
{
	struct sk_buff *skb;

	sna_debug(5, "init\n");
	skb = sna_alloc_skb(rcb->tx_max_btu, GFP_ATOMIC);
	if (!skb)
		return NULL;

	/* initialize the sna_ctrl fields in the buffer. */
	SNA_SKB_CB(skb)->type = SNA_CTRL_T_REC_SEND_DATA;
	SNA_SKB_CB(skb)->fmh  = 0;
	return skb;
}

/**
 * this procedure determines if there is enough data to be sent to hs.
 * ps continues to send data to hs until the amount of data remaining to be
 * sent is less than or equal to the maximum send ru size, in which case ps
 * stores the data in the mu in the rcb until more data is issues by the
 * transaction program or the buffer is forced to be sent (e.g. flush, cd)
 * to the partner. if the data in the buffer is exactly equal to the
 * maximum send ru size, ps stores the data to be sent later.
 *
 * @rcb: rcb corresponding to the resource specified in the current
 *       transaction_pgm_verb.
 * @skb: data to be sent to hs.
 */
static int sna_ps_conv_send_data_buffer_management(struct sna_rcb *rcb,
	struct sk_buff *skb, int force)
{
	struct sk_buff *r_skb;
	int f_len, copied;

	sna_debug(5, "init\n");
	r_skb = rcb->send_mu;
	do {
		if (!r_skb) {
			r_skb = sna_ps_conv_create_and_init_limited_mu(rcb);
			if (!r_skb)
				return -ENOMEM;
		}
		if (r_skb->len >= rcb->tx_max_btu) {
			sna_hs_tx_ps_mu_data(rcb, r_skb);
			r_skb = NULL;
			continue;
		}

		f_len = rcb->tx_max_btu - r_skb->len;
		if (skb->len > f_len)
			copied = f_len;
		else
			copied = skb->len;
		memcpy(skb_put(r_skb, copied), skb->data, copied);
		skb_pull(skb, copied);
	} while (skb->len);

	if (force) {
		sna_hs_tx_ps_mu_data(rcb, r_skb);
		r_skb = NULL;
	}
	rcb->send_mu = r_skb;
	return 0;
}

/**
 * perform basic enforcement of the logical record protocol. the first
 * two bytes of every record contains the length field of the record.
 */
static int sna_ps_conv_validate_logical_records(struct sna_rcb *rcb,
	struct sk_buff *skb)
{
	int err = AC_RC_PROGRAM_PARAMETER_CHECK;
	int size, offset;

	sna_debug(5, "init\n");
	offset = 0;
	do {
		if (rcb->ll_tx_state == SNA_RCB_LL_STATE_NONE) {
			if (skb->len < 2)
				goto out;
			size = ntohs(*(u_int16_t *)&skb->data[offset]);
			sna_debug(5, "size=%d\n", size);
			if (size < 2 || size > 32767)
				goto out;
			rcb->ll_tx_rec_size  = size;
			rcb->ll_tx_rec_bytes = 0;
			rcb->ll_tx_state     = SNA_RCB_LL_STATE_INCOMPLETE;
		}
		if (rcb->ll_tx_state == SNA_RCB_LL_STATE_INCOMPLETE) {
			if ((skb->len + rcb->ll_tx_rec_bytes) < rcb->ll_tx_rec_size) {
				rcb->ll_tx_rec_bytes += skb->len;
				offset = skb->len;
			} else {
				offset += rcb->ll_tx_rec_size - rcb->ll_tx_rec_bytes;
				rcb->ll_tx_state = SNA_RCB_LL_STATE_COMPLETE;
			}
		}
		if (rcb->ll_tx_state == SNA_RCB_LL_STATE_COMPLETE) {
			sna_debug(5, "ll size=%d complete\n", rcb->ll_tx_rec_size);
			rcb->ll_tx_rec_size  = 0;
			rcb->ll_tx_rec_bytes = 0;
			rcb->ll_tx_state     = SNA_RCB_LL_STATE_NONE;
		}
	} while (skb->len > offset);
	err = AC_RC_OK;
out:	return err;
}

/**
 * this procedure processes conversation_failure records.
 */
static int sna_ps_conv_conversation_failure_proc(struct sna_rcb *rcb, struct sk_buff *skb)
{
	sna_debug(5, "init\n");
	if (SNA_SKB_CB(skb)->reason == SNA_CTRL_R_PROTOCOL_VIOLATION)
		sna_ps_conv_fsm_error_or_failure(rcb, SNA_PS_FSM_ERR_OR_FAIL_INPUT_CONV_FAIL_PROTOCOL);
	else
		sna_ps_conv_fsm_error_or_failure(rcb, SNA_PS_FSM_ERR_OR_FAIL_INPUT_CONV_FAIL_SON);
	if (rcb->fsm_post_state == SNA_PS_FSM_POST_STATE_PEND_POSTED)
		sna_ps_conv_fsm_post(rcb, SNA_PS_FSM_POST_INPUT_POST);
	return 0;
}

static inline int sna_ps_conv_intr_errno(long timeo)
{
	return timeo == MAX_SCHEDULE_TIMEOUT ? -ERESTARTSYS : -EINTR;
}

static inline long sna_ps_conv_wait_timeo(struct sna_rcb *rcb, int suspend)
{
	return suspend ? rcb->wait_timeo : 0;
}

static int sna_ps_conv_wait_for_records(struct sna_rcb *rcb, int *err, long *timeo_p)
{
	DECLARE_WAITQUEUE(wait, current);
	int error;

	sna_debug(100, "init\n");
	__set_current_state(TASK_INTERRUPTIBLE);
	add_wait_queue_exclusive(&rcb->sleep, &wait);

	if (!skb_queue_empty(&rcb->hs_to_ps_queue))
		goto ready;
	if (!skb_queue_empty(&rcb->rm_to_ps_queue))
		goto ready;

	/* handle signals */
	if (signal_pending(current))
		goto interrupted;

	*timeo_p = schedule_timeout(*timeo_p);

ready:  current->state = TASK_RUNNING;
	remove_wait_queue(&rcb->sleep, &wait);
	return 0;

interrupted:
	error = sna_ps_conv_intr_errno(*timeo_p);
	*err = error;
out:    current->state = TASK_RUNNING;
	remove_wait_queue(&rcb->sleep, &wait);
	return error;
	*err  = 0;
	error = 1;
	goto out;
}

/**
 * this procedure receives records from rm and all hs processes and updates
 * the appropriate rcb. if suspend_list is not empty, this procedure waits
 * until at least one record associated with an rcb in suspend_list is
 * received.
 */
static int sna_ps_conv_receive_rm_or_hs_to_ps_records(struct sna_rcb *rcb, int suspend)
{
	struct sk_buff *skb;
	int from_hs_or_rm;	/* 0 = hs, 1 = rm. */
	long timeo;
	int err;

	sna_debug(5, "init\n");

	/* run until we have no more records or an error. suspend will cause
	 * this function to block.
	 */
	timeo = sna_ps_conv_wait_timeo(rcb, suspend);
	sna_debug(5, "timeo=%ld\n", timeo);
	do {
		skb = skb_dequeue(&rcb->rm_to_ps_queue);
		if (skb) {
			sna_debug(5, "got record from RM\n");
			from_hs_or_rm = 1;
			goto g_record;
		}
		skb = skb_dequeue(&rcb->hs_to_ps_queue);
		if (skb) {
			sna_debug(5, "got record from HS\n");
			from_hs_or_rm = 0;
			goto g_record;
		}

		/* either not suspended or suspend time ran out. */
		if (!timeo)
			goto out;

		sna_debug(5, "goto g_more\n");
		goto g_more;

g_record:	if (from_hs_or_rm) {
			sna_ps_conv_conversation_failure_proc(rcb, skb);
			goto g_fini;
		}
		switch (SNA_SKB_CB(skb)->type) {
			case SNA_CTRL_T_REC_REQ_TO_SEND:
				if (rcb->fsm_err_or_fail_state == SNA_PS_FSM_ERR_OR_FAIL_INPUT_RESET)
					rcb->rq_to_send_rcvd = 1;
				kfree_skb(skb);
				break;
			case SNA_CTRL_T_REC_RECEIVE_ERROR:
				sna_ps_conv_fsm_error_or_failure(rcb,
					SNA_PS_FSM_ERR_OR_FAIL_INPUT_RECEIVE_ERROR);
				kfree_skb(skb);
				break;
			case SNA_CTRL_T_REC_CONFIRMED:
				if (rcb->fsm_err_or_fail_state != SNA_PS_FSM_ERR_OR_FAIL_INPUT_RESET)
					kfree_skb(skb);
				break;
			case SNA_CTRL_T_REC_MU:
				if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_RCV_STATE
					|| rcb->fsm_err_or_fail_state == SNA_PS_FSM_ERR_OR_FAIL_STATE_RCVD_ERROR) {
					skb_queue_tail(&rcb->hs_to_ps_buffer_queue, skb);
				} else {
					sna_debug(5, "bad state tossing skb\n");
					kfree_skb(skb);
					if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_END_CONV)
						sna_ps_conv_protocol_error(rcb, 0x20040000);
				}
				break;
			default:
				sna_debug(5, "unknown ps type\n");
				kfree_skb(skb);
				break;
		}
g_fini:		if (suspend)
			timeo = sna_ps_conv_wait_timeo(rcb, !suspend);
g_more:
		while (0) {};
	} while (sna_ps_conv_wait_for_records(rcb, &err, &timeo) == 0);
out:	return 0;
}

/**
 * this procedure handles the receipt of data from the transaction program. if
 * the resource specified in the send_data is valid and the conversation is
 * in the send state, processing of the record continues. ps first retrieves
 * any records from rm and hs. appropriate action is taken depending upon
 * which, if any, record was received.
 *
 * @tp: send_data verb parameters and a possible rq_to_send may have
 *      been received on this conversation.
 *
 * the return_code of the send_data verb is set. the states of
 * fsm_conversation and fsm_error_or_failure may have changed. if
 * rq_to_send_rcvd has been received, an indication is stored in the
 * rcb.rq_to_send_rcvd field. this yes/no indication will be passed up to the
 * tp and then the rcb.rq_to_send_rcvd field is reset to indicate that no
 * rq_to_sends are outstanding.
 */
static int sna_ps_conv_send_data_proc(struct sna_tp_cb *tp)
{
	struct sk_buff *skb;
	struct sna_rcb *rcb;
	int err = AC_RC_PRODUCT_SPECIFIC;

	sna_debug(5, "init\n");
	rcb = sna_rm_rcb_get_by_index(tp->rcb_index);
	if (!rcb)
		goto out;
	err = sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_S,
		SNA_PS_FSM_CONV_INPUT_SEND_DATA, rcb, 1);
	if (err) {
		err = PROGRAM_STATE_CHECK;
		goto out;
	}
	sna_ps_conv_receive_rm_or_hs_to_ps_records(rcb, 0);
	switch (rcb->fsm_err_or_fail_state) {
		case SNA_PS_FSM_ERR_OR_FAIL_STATE_CONV_FAILURE_PROTOCOL_ERROR:
		case SNA_PS_FSM_ERR_OR_FAIL_STATE_CONV_FAILURE_SON:
			if (rcb->fsm_err_or_fail_state == SNA_PS_FSM_ERR_OR_FAIL_STATE_CONV_FAILURE_SON)
				err = AC_RC_RESOURCE_FAILURE_RETRY;
			else
				err = AC_RC_RESOURCE_FAILURE_NO_RETRY;
			sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_R,
				SNA_PS_FSM_CONV_INPUT_RESOURCE_FAILURE_RC, rcb, 0);
			break;
		case SNA_PS_FSM_ERR_OR_FAIL_STATE_RCVD_ERROR:
			skb = skb_dequeue(&tp->write_queue);
			if (!skb) {
				skb = sna_ps_conv_create_and_init_limited_mu(rcb);
				if (!skb) {
					err = AC_RC_PRODUCT_SPECIFIC;
					break;
				}
				SNA_SKB_CB(skb)->type = SNA_CTRL_T_PREPARE_TO_RCV_FLUSH;
				sna_hs_tx_ps_mu_req(rcb, skb);
				sna_ps_conv_receive_rm_or_hs_to_ps_records(rcb, 1);

				if (rcb->fsm_err_or_fail_state != SNA_PS_FSM_ERR_OR_FAIL_STATE_CONV_FAILURE_SON
					&& rcb->fsm_err_or_fail_state != SNA_PS_FSM_ERR_OR_FAIL_STATE_CONV_FAILURE_PROTOCOL_ERROR) {
					err = sna_ps_conv_dequeue_fmh7_proc(rcb);
					break;
				}
				if (rcb->fsm_err_or_fail_state == SNA_PS_FSM_ERR_OR_FAIL_STATE_CONV_FAILURE_SON)
					err = AC_RC_RESOURCE_FAILURE_RETRY;
				else
					err = AC_RC_RESOURCE_FAILURE_NO_RETRY;
				sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_R,
					SNA_PS_FSM_CONV_INPUT_RESOURCE_FAILURE_RC, rcb, 0);
			}
			break;
		case SNA_PS_FSM_ERR_OR_FAIL_STATE_NO_REQUESTS:
			skb = skb_dequeue(&tp->write_queue);
			if (skb) {
				err = sna_ps_conv_validate_logical_records(rcb, skb);
				if (err) {
					kfree_skb(skb);
					break;
				}
				sna_ps_conv_send_data_buffer_management(rcb, skb, 0);
			}
			break;
	}
	tp->rq_to_send_rcvd	= rcb->rq_to_send_rcvd;
	rcb->rq_to_send_rcvd 	= 0;
out:	return err;
}

/**
 * this procedure handles the send_error verb processing. if the resource
 * specified in the send_error is valid and the converstaion is in an
 * appropriate state, processing of the send_error continues. ps first
 * retrieves any records from rm and hs. appropriate action is taken
 * depending upon which, if any, record was received (as reflected by the
 * state of fsm_error_or_failure).
 *
 * @tp: send_error verb parameters.
 *
 * the return code of the send_error verb is updated. if the rcb indicates
 * that a rq_to_send_rcvd has been received, it will be passed up to the tp
 * at this time and the rcv.rq_to_send_rcvd field will be reset to no. the
 * state of fsm_conversation may be changed.
 */
static int sna_ps_conv_send_error_proc(struct sna_tp_cb *tp)
{
	struct sna_rcb *rcb;
	int err = AC_RC_PRODUCT_SPECIFIC;

	sna_debug(5, "init\n");
	rcb = sna_rm_rcb_get_by_index(tp->rcb_index);
	if (!rcb)
		goto out;
	err = AC_RC_OK;
#ifdef NOT
	if(fsm_conversation(s, send_error, rcb) > state_condition)
		send_error_verb->rcode = PROGRAM_STATE_CHECK;
	else
	{
		sna_receive_rm_or_hs_to_ps_records(suspend_list);
		switch(state = fsm_error_or_failure())
		{
			case (CONV_FAILURE_PROTOCOL_ERROR):
			case (CONV_FAILURE_SON):
				fsm_conversation(s, send_error, rcb);
				if(state == CONV_FAILURE_SON)
					send_error_verb->rcode = RESOURCE_FAILURE_RETRY;
				else
					send_error_verb->rcode = RESOURCE_FAILURE_NO_RETRY;
				fsm_conversation(r, resource_failure_rc, rcb);
				break;

			case (NO_REQUESTS):
			case (RCVD_ERROR):
				switch(fsm_conversation())
				{
					case (SEND_STATE):
						sna_send_error_in_send_state(send_error_verb, rcb);
						break;

					case (RCVD_CONFIRM):
					case (RCVD_CONFIRM_SEND):
					case (RCVD_CONFIRM_DEALL):
						sna_send_error_to_hs_proc(rcb);
						fsm_conversation(s, send_error, rcb);
						sna_send_error_done_proc(send_error_verb, rcb);
						break;

					case (RCV_STATE):
						sna_send_error_in_receive_state(send_error_verb, rcb);
						break;

					default:
						/* Error */
				}
			}
		}

		send_error_verb->rq_to_send_rcvd = rcb->rq_to_send_rcvd;
		rcb->rq_to_send_rcvd = NO;
	}
#endif
out:	return err;
}

/**
 * this procedure handles request_to_send verb processing. if the conversation
 * is in the receive state, ps completes the processing of the request_to_send
 * record, as described below.
 *
 * @tp: request_to_send verb parameters.
 *
 * the request_to_send return code is updated, and a request_to_send will be
 * sent.
 */
static int sna_ps_conv_request_to_send_proc(struct sna_tp_cb *tp)
{
	struct sna_rcb *rcb;
	int err = AC_RC_PRODUCT_SPECIFIC;

	sna_debug(5, "init\n");
	rcb = sna_rm_rcb_get_by_index(tp->rcb_index);
	if (!rcb)
		goto out;
	err = AC_RC_OK;
#ifdef NOT
	if(fsm_conversation(s, receive_immediate, rcb) > state_condition)
		request_to_send_verb->rcode = PROGRAM_STATE_CHECK;
	else
	{
		sna_receive_rm_or_hs_to_ps_records(suspend_list);
		state = fsm_error_or_failure();
		if(state == NO_REQUESTS || state == RCVD_ERROR)
		{
			if(rcb->ec_type != DEALLOCATE_FLUSH)
			{
				sna_send_request_to_send_proc(rcb);
				sna_wait_for_rsp_to_rq_to_send_proc(rcb);
			}
		}
		request_to_send_verb->rcode = OK;
	}
#endif
out:	return err;
}

/**
 * this procedure handles the processing of mus from the hs_to_ps_buffer_list.
 * this procedure first checks to see if that data in the mu being processed
 * is a ps header or a logical record having an invalid ll value, in order
 * to take appropraite action. if this data is not a ps header or an invalid
 * ll, then further processing of the data in the mu occurs as described
 * below.
 *
 * @rcb: the rcb corresponding to the resource specified in the
 *       passed receive verb parameters.
 */
static int sna_ps_conv_process_data_proc(struct sna_rcb *rcb,
	struct sk_buff *skb, int *reason, int *end_of_chain_type)
{
	int err, ll_end, size, c_size;
	struct sna_tp_cb *tp;

	sna_debug(5, "init\n");
	tp = sna_rm_tp_get_by_index(rcb->tp_index);
	if (!tp) {
		err = AC_RC_UNSUCCESSFUL;
		goto error;
	}

	/* display buffer control header. */
	sna_debug(5, "sna_ctrl: type:%d hs_ps_type:%d\n",
		SNA_SKB_CB(skb)->type, SNA_SKB_CB(skb)->hs_ps_type);

	/* process an entire skb of logical records. */
	*end_of_chain_type = SNA_SKB_CB(skb)->hs_ps_type;
	*reason 	   = 0;
	do {
		ll_end   = 0;
		if (rcb->ll_rx_state == SNA_RCB_LL_STATE_NONE) {
			if (skb->len < 2) {
				sna_debug(5, "FIXME: unsupported 1 byte ll hdr rx'd.\n");
				err = AC_RC_UNSUCCESSFUL;
				goto error;       /* not entire ll field received. */
			}

			/* check ll header to see if valid. */
			size = ntohs(*(u_int16_t *)&skb->data[0]);
			sna_debug(5, "len=%d size=%d\n", skb->len, size);
			if (size < 2 || size > 32767) {
				err = AC_RC_UNSUCCESSFUL;
				goto error;
			}
			/* strip ll header for mapped conversations. */
			if (tp->conversation_type == AC_CONVERSATION_TYPE_MAPPED) {
				tp->rx_mc_ll_pull_left = 4;
				size -= 4;
			}
			rcb->ll_rx_rec_size  = size;
			rcb->ll_rx_rec_bytes = 0;
			rcb->ll_rx_state     = SNA_RCB_LL_STATE_INCOMPLETE;
		}
		if (rcb->ll_rx_state == SNA_RCB_LL_STATE_INCOMPLETE) {
			/* pull of ll header for mapped conversations. */
			if (tp->conversation_type == AC_CONVERSATION_TYPE_MAPPED
				&& tp->rx_mc_ll_pull_left) {
				if (skb->len >= tp->rx_mc_ll_pull_left) {
					skb_pull(skb, tp->rx_mc_ll_pull_left);
					tp->rx_mc_ll_pull_left = 0;
				} else {
					tp->rx_mc_ll_pull_left = tp->rx_mc_ll_pull_left - skb->len;
					skb_pull(skb, skb->len);
				}
			}
			if (skb->len >= (rcb->ll_rx_rec_size - rcb->ll_rx_rec_bytes)) {
				/* (start and end) or (end) of ll record data. */
				if ((tp->rx_req_len - tp->rx_rcv_len)
					>= (rcb->ll_rx_rec_size - rcb->ll_rx_rec_bytes)) {
					/* user buffer has enough room to copy entire ll data. */
					c_size = rcb->ll_rx_rec_size - rcb->ll_rx_rec_bytes;
					ll_end = 1;
				} else {
					/* not enough room for entire ll so copy what we can. */
					c_size = tp->rx_req_len - tp->rx_rcv_len;
				}
			} else {
				/* (start) or (middle) of ll record data. */
				if ((tp->rx_req_len - tp->rx_rcv_len) >= skb->len) {
					/* user buffer has enough room for entire ll data. */
					c_size = skb->len;
				} else {
					/* not enough room for entire ll so copy what we can. */
					c_size = tp->rx_req_len - tp->rx_rcv_len;
				}
			}

			sna_debug(5, "copy %d bytes to user.\n", c_size);
			sna_ktou(skb->data, c_size, (((u_int8_t *)tp->rx_buffer) + tp->rx_rcv_len));
			skb_pull(skb, c_size);
			tp->rx_rcv_len          += c_size;
			rcb->ll_rx_rec_bytes    += c_size;
			if (ll_end)
				rcb->ll_rx_state = SNA_RCB_LL_STATE_COMPLETE;
		}
		if (rcb->ll_rx_state == SNA_RCB_LL_STATE_COMPLETE) {
			sna_debug(5, "complete\n");
			rcb->ll_rx_state        = SNA_RCB_LL_STATE_NONE;
			rcb->ll_rx_rec_size     = 0;
			rcb->ll_rx_rec_bytes    = 0;
		}
		/* if fill type is ll and end of record or user receive buffer
		 * is full return control to the user.
		 */
		if ((tp->fill == AC_FILL_LL && ll_end)
			|| (tp->rx_req_len == tp->rx_rcv_len)) {
			if (skb->len)
				skb_queue_head(&rcb->hs_to_ps_buffer_queue, skb);
			else
				kfree_skb(skb);
			if (tp->fill == AC_FILL_LL && ll_end)
				*reason = AC_REASON_FULL_LL;
			else
				*reason = AC_REASON_FULL_BUFFER;
			if (tp->fill == AC_FILL_LL) {
				if (*reason == AC_REASON_FULL_LL)
					tp->data_received = AC_WR_DATA_COMPLETE;
				else
					tp->data_received = AC_WR_DATA_INCOMPLETE;
			} else {
				tp->data_received = AC_WR_DATA;
			}

			err = AC_RC_OK;
			goto out;
		}
	} while (skb->len);

	/* not end of record or the user buffer is not full, but the
	 * skb contains no more data.
	 */
	err 			= AC_RC_OK;
	*reason 		= AC_REASON_NONE;
error:  kfree_skb(skb);
out:    return err;
}

/**
 * this procedure processes the end-of-chain type that has been received
 * and saved for this conversation. this procedure is called only if the
 * end-of-chain type received is a value other than not_end_of_data.
 *
 * @rcb: the rcb corresponding to the resource specified in the verb
 *       parameters, and receive verb parameters.
 *
 * the return code field of the receive verb is updated to the appropriate
 * value. the staet of fsm_conversation may be changed.
 */
static int sna_ps_conv_perform_receive_ec_processing(struct sna_rcb *rcb,
	struct sna_tp_cb *tp, int end_of_chain_type)
{
	int err = AC_RC_UNSUCCESSFUL;

	sna_debug(5, "init\n");
	if (rcb->ll_rx_state == SNA_RCB_LL_STATE_INCOMPLETE) {
		sna_ps_conv_protocol_error(rcb, 0x10010000);
		err = AC_RC_RESOURCE_FAILURE_NO_RETRY;
		sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_R,
			SNA_PS_FSM_CONV_INPUT_RESOURCE_FAILURE_RC, rcb, 0);
		goto out;
	}
	if (rcb->sync_level == AC_SYNC_LEVEL_NONE
		&& (end_of_chain_type == SNA_CTRL_T_CONFIRM
		|| end_of_chain_type == SNA_CTRL_T_PREPARE_TO_RCV_CONFIRM
		|| end_of_chain_type == SNA_CTRL_T_DEALLOCATE_CONFIRM)) {
		sna_ps_conv_protocol_error(rcb, 0x10010000);
		err = AC_RC_RESOURCE_FAILURE_NO_RETRY;
		sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_R,
			SNA_PS_FSM_CONV_INPUT_RESOURCE_FAILURE_RC, rcb, 0);
		goto out;
	}
	switch (end_of_chain_type) {
		case SNA_CTRL_T_CONFIRM:
			err = AC_RC_OK;
			tp->status_received = AC_SR_CONFIRM_RECEIVED;
			sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_R,
				SNA_PS_FSM_CONV_INPUT_CONFIRM_INDICATOR, rcb, 0);
			break;
		case SNA_CTRL_T_PREPARE_TO_RCV_CONFIRM:
			err = AC_RC_OK;
			tp->status_received = AC_SR_CONFIRM_SEND_RECEIVED;
			sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_R,
				SNA_PS_FSM_CONV_INPUT_CONFIRM_SEND_IND, rcb, 0);
			break;
		case SNA_CTRL_T_PREPARE_TO_RCV_FLUSH:
			err = AC_RC_OK;
			tp->status_received = AC_SR_SEND_RECEIVED;
			sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_R,
				SNA_PS_FSM_CONV_INPUT_SEND_INDICATOR, rcb, 0);
			break;
		case SNA_CTRL_T_DEALLOCATE_CONFIRM:
			err = AC_RC_OK;
			tp->data_received = AC_SR_CONFIRM_DEALLOCATE_RECEIVED;
			sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_R,
				SNA_PS_FSM_CONV_INPUT_CONFIRM_DEALLOC_IND, rcb, 0);
			break;
		case SNA_CTRL_T_DEALLOCATE_FLUSH:
			err = AC_RC_DEALLOCATE_NORMAL;
			sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_R,
				SNA_PS_FSM_CONV_INPUT_DEALLOC_NORMAL_RC, rcb, 0);
			break;
	}
out:	return err;
}

/**
 * this procedure checks the rcb.hs_to_ps_buffer_list receive buffer
 * to see if any information has arrived for the conversation specified
 * in the passed receive verb parameters and, if so, updates the verb
 * parameters to reflect that information. examples of the type of information
 * that can be received include a request for confirmation, notification
 * that the partner transaction program has deallocated the conversation, and
 * conversation data. if no information has been received from the specified
 * conversation, the return_code parameters is set to unsuccessful and
 * control is returned to the calling procedure.
 *
 * @rcb: the rcb corresponding to the resource specified in the verb
 *       parameters, and receive verb parameters.
 *
 * the information is copied from the mu into the receive verb data buffer
 * to be passed up to the tp. if the data in the mu is exhausted, the mu is
 * freed and the next mu in the receive buffer, if present, will begin to be
 * processed.
 */
static int sna_ps_conv_perform_receive_processing(struct sna_rcb *rcb,
	struct sna_tp_cb *tp)
{
	struct sk_buff *skb;
	int err = AC_RC_UNSUCCESSFUL, reason = 0, fmh7 = 0;
	int end_of_chain_type = SNA_CTRL_T_NOT_END_OF_DATA;

	sna_debug(5, "init\n");
	if (signal_pending(current)) {	/* handle signal.s */
		sna_ps_conv_fsm_post(rcb, SNA_PS_FSM_POST_INPUT_POST);
		goto out;
	}
	if (skb_queue_empty(&rcb->hs_to_ps_buffer_queue)) {
//		|| rcb->end_of_chain_type != SNA_CTRL_T_NOT_END_OF_DATA) {
		if (rcb->fsm_err_or_fail_state == SNA_PS_FSM_ERR_OR_FAIL_STATE_CONV_FAILURE_PROTOCOL_ERROR) {
			err = AC_RC_RESOURCE_FAILURE_NO_RETRY;
			sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_R,
				SNA_PS_FSM_CONV_INPUT_RESOURCE_FAILURE_RC, rcb, 0);
			goto out;
		}
		if (rcb->fsm_err_or_fail_state == SNA_PS_FSM_ERR_OR_FAIL_STATE_CONV_FAILURE_SON) {
			err = AC_RC_RESOURCE_FAILURE_RETRY;
			sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_R,
				SNA_PS_FSM_CONV_INPUT_RESOURCE_FAILURE_RC, rcb, 0);
			goto out;
		}
		goto out;
	}
	while (!skb_queue_empty(&rcb->hs_to_ps_buffer_queue)
		&& rcb->fsm_post_state == SNA_PS_FSM_POST_STATE_PEND_POSTED) {
		skb = skb_dequeue(&rcb->hs_to_ps_buffer_queue);
		if (!skb) /* someone stole our data. */
			continue;
		if (sna_transport_header(skb)->fi == SNA_RH_FI_FMH
			&& ((sna_fmh *)skb->data)->type == SNA_FMH_TYPE_7) {
			sna_ps_conv_fsm_post(rcb, SNA_PS_FSM_POST_INPUT_POST);
			err = sna_ps_conv_process_fmh7_proc(rcb, skb);
			fmh7 = 1;
			continue;
		}
		err = sna_ps_conv_process_data_proc(rcb, skb, &reason,
			&end_of_chain_type);
		if (err) {
			sna_debug(5, "process data proc error `%d'.\n", err);
			sna_ps_conv_fsm_post(rcb, SNA_PS_FSM_POST_INPUT_POST);
			goto out;
		}
		if (reason) {
			/* full buffer or end of logical record. */
			sna_ps_conv_fsm_post(rcb, SNA_PS_FSM_POST_INPUT_POST);
			break;
		}
	}
	if (!fmh7 && end_of_chain_type != SNA_CTRL_T_NOT_END_OF_DATA) {
		sna_debug(5, "End of CHAIN!\n");
		sna_ps_conv_fsm_post(rcb, SNA_PS_FSM_POST_INPUT_POST);
		err = sna_ps_conv_perform_receive_ec_processing(rcb, tp,
			end_of_chain_type);
	}
out:	return err;
}

/**
 * this procedure tests whether the post conditions specified in the rcb
 * have been satisified.
 *
 * @rcb: the rcb correpsonding to the resource to be tested.
 *
 * the state of fsm_post is set to posted if the post conditions
 * are satisfied.
 *
 * *note* this function has been merged with the recieve processing logic.
 */
#ifdef SNA_SANDBOX
static int sna_ps_conv_test_for_post_satisfied(struct sna_rcb *rcb)
{
	struct sk_buff *skb;
	int post = 0, size;

	sna_debug(5, "init\n");
	if (rcb->end_of_chain_type != SNA_CTRL_T_NOT_END_OF_DATA) {
		post = 1;
		goto out;
	}
	skb = skb_peek(&rcb->hs_to_ps_buffer_queue);
	if (!skb)
		goto out;

	if (sna_transport_header(skb)->fi == SNA_RH_FI_FMH
		&& ((sna_fmh *)skb->data)->type == SNA_FMH_TYPE_7) {
		post = 1;
		goto out;
	}

	if (rcb->ll_rx_state == SNA_RCB_LL_STATE_COMPLETE) {
		if (rcb->rx_post.fill == AC_FILL_LL)
			post = 1;
		goto out;
	}
	if (rcb->ll_rx_state == SNA_RCB_LL_STATE_NONE) {
		if (skb->len < 2)
			goto out;	/* not entire ll field received. */
		size = ntohs((u_int16_t)skb->data[0]);
		if (size < 2 || size > 32767) {
			post = 1;
			goto out;	/* invalid ll header. */
		}
		rcb->ll_rx_rec_size  = size;
		rcb->ll_rx_rec_bytes = 0;
		rcb->ll_rx_state     = SNA_RCB_LL_STATE_INCOMPLETE;
	}
	if (rcb->ll_rx_state == SNA_RCB_LL_STATE_INCOMPLETE) {
		if (skb->len >= (rcb->ll_rx_rec_size - rcb->ll_rx_rec_bytes)) {
			rcb->ll_rx_state = SNA_RCB_LL_STATE_COMPLETE;
			post = 1;
			goto out;
		} else {
			rcb->ll_rx_rec_bytes += skb->len;
		}
	}
out:	if (post)
		sna_ps_conv_fsm_post(rcb, SNA_PS_FSM_POST_INPUT_POST);
	return 0;
}
#endif

/**
 * this procedure transfers data from the received mus into the
 * recieve_and_wait data buffer while checking for posting to be satisfied.
 *
 * @rcb: rcb of the conversation.
 *
 * buffer area of the verb is filled with the requested amount of data. if
 * data is to be returned to the tp, receive_and_wait.max_length is set
 * to the amount of data being returned. receive_and_wait.return_code and
 * receive_and_wait.what_received are initialized. also, the fsm_post
 * undergoes state transitions, along with updates to the
 * rcb.post_conditions.max_length and rcb.post_conditions.fill fields.
 */
static int sna_ps_conv_receive_and_test_posting(struct sna_rcb *rcb)
{
	struct sna_tp_cb *tp;
	int err = AC_RC_PRODUCT_SPECIFIC;
	u_int16_t wait_cnt = 0;

	sna_debug(5, "init\n");
	tp = sna_rm_tp_get_by_index(rcb->tp_index);
	if (!tp)
		goto out;
	sna_ps_conv_fsm_post(rcb, SNA_PS_FSM_POST_INPUT_POST_ON_RECEIPT);
	rcb->rx_post.fill	= tp->fill;
	rcb->rx_post.max_len	= tp->rx_req_len;
	tp->data_received	= AC_WR_NO_DATA_RECEIVED;
	tp->status_received	= AC_SR_NO_STATUS_RECEIVED;
	do {
		/* process the hs_to_ps_buffer_queue one buffer at a time
		 * until our posting conditions have been met, if no buffer
		 * is available in queue we wait for at least one to arrive.
		 */
		if (wait_cnt && skb_queue_empty(&rcb->hs_to_ps_buffer_queue))
			sna_ps_conv_receive_rm_or_hs_to_ps_records(rcb, 1);
		err = sna_ps_conv_perform_receive_processing(rcb, tp);
		wait_cnt++;
	} while (rcb->fsm_post_state == SNA_PS_FSM_POST_STATE_PEND_POSTED);
	sna_ps_conv_fsm_post(rcb, SNA_PS_FSM_POST_INPUT_RECEIVE_IMMEDIATE);
out:	return err;
}

/**
 * this procedure handles the receive_and_wait verb. if the conversation
 * is in an appropriate state (ie receive_and_wait can be issued when the
 * conversation is in the send or receive state), processing of the record
 * continues. ps first receives any records from rm and hs. appropriate
 * action is taken depending upon which, if any, record was received (as
 * reflected by the state of fsm_error_or_failure).
 *
 * @tp receive_and_wait verb parameters.
 *
 * the data field is cleared before receiveding data from hs.
 * rcb.rq_to_send_rcvd is updated. if a rq_to_send has been received, an
 * indication will be passed up to the tp at this time, and the field in the
 * rcb is updated. the state of fsm_conversation may change.
 */
static int sna_ps_conv_rcv_and_wait_proc(struct sna_tp_cb *tp)
{
	struct sk_buff *skb;
	struct sna_rcb *rcb;
	int err = AC_RC_PRODUCT_SPECIFIC;

	sna_debug(5, "init\n");
	rcb = sna_rm_rcb_get_by_index(tp->rcb_index);
	if (!rcb)
		goto out;
	err = sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_S,
		SNA_PS_FSM_CONV_INPUT_RECEIVE_AND_WAIT, rcb, 1);
	if (err) {
		err = AC_RC_PROGRAM_STATE_CHECK;
		goto out;
	}
	sna_ps_conv_receive_rm_or_hs_to_ps_records(rcb, 0);
	if (rcb->fsm_err_or_fail_state == SNA_PS_FSM_ERR_OR_FAIL_STATE_RCVD_ERROR) {
		if (rcb->fsm_conv_state == SNA_PS_FSM_CONV_STATE_SEND_STATE) {
			skb = sna_ps_conv_create_and_init_limited_mu(rcb);
			if (!skb)
				goto out;
			SNA_SKB_CB(skb)->type = SNA_CTRL_T_PREPARE_TO_RCV_FLUSH;
			sna_hs_tx_ps_mu_req(rcb, skb);
		}
		if (skb_queue_empty(&rcb->hs_to_ps_buffer_queue))
			sna_ps_conv_receive_rm_or_hs_to_ps_records(rcb, 1);
		if (rcb->fsm_err_or_fail_state != SNA_PS_FSM_ERR_OR_FAIL_STATE_CONV_FAILURE_SON
			&& rcb->fsm_err_or_fail_state != SNA_PS_FSM_ERR_OR_FAIL_STATE_CONV_FAILURE_PROTOCOL_ERROR) {
			err = sna_ps_conv_dequeue_fmh7_proc(rcb);
			goto out;
		}
		if (rcb->fsm_err_or_fail_state == SNA_PS_FSM_ERR_OR_FAIL_STATE_CONV_FAILURE_SON)
			err = AC_RC_RESOURCE_FAILURE_RETRY;
		else
			err = AC_RC_RESOURCE_FAILURE_NO_RETRY;
		sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_R,
			SNA_PS_FSM_CONV_INPUT_RESOURCE_FAILURE_RC, rcb, 0);
	} else {
		sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_S,
			SNA_PS_FSM_CONV_INPUT_RECEIVE_AND_WAIT, rcb, 0);
		err = sna_ps_conv_receive_and_test_posting(rcb);
	}
	tp->rq_to_send_rcvd     = rcb->rq_to_send_rcvd;
	rcb->rq_to_send_rcvd    = 0;
out:	return err;
}

static int sna_ps_conv_rcv_immediate_proc(struct sna_tp_cb *tp)
{
	struct sna_rcb *rcb;
	int err = AC_RC_PRODUCT_SPECIFIC;

	sna_debug(5, "init\n");
	rcb = sna_rm_rcb_get_by_index(tp->rcb_index);
	if (!rcb)
		goto out;
	err = sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_S,
		SNA_PS_FSM_CONV_INPUT_RECEIVE_IMMEDIATE, rcb, 1);
	if (err) {
		err = AC_RC_PROGRAM_STATE_CHECK;
		goto out;
	}
	sna_ps_conv_receive_rm_or_hs_to_ps_records(rcb, 0);
	if (rcb->fsm_err_or_fail_state == SNA_PS_FSM_ERR_OR_FAIL_STATE_RCVD_ERROR) {
		if (skb_queue_empty(&rcb->hs_to_ps_buffer_queue))
			sna_ps_conv_receive_rm_or_hs_to_ps_records(rcb, 1);
		if (rcb->fsm_err_or_fail_state != SNA_PS_FSM_ERR_OR_FAIL_STATE_CONV_FAILURE_SON
			&& rcb->fsm_err_or_fail_state != SNA_PS_FSM_ERR_OR_FAIL_STATE_CONV_FAILURE_PROTOCOL_ERROR) {
			err = sna_ps_conv_dequeue_fmh7_proc(rcb);
			goto out;
		}
		if (rcb->fsm_err_or_fail_state == SNA_PS_FSM_ERR_OR_FAIL_STATE_CONV_FAILURE_SON)
			err = AC_RC_RESOURCE_FAILURE_RETRY;
		else
			err = AC_RC_RESOURCE_FAILURE_NO_RETRY;
		sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_R,
			SNA_PS_FSM_CONV_INPUT_RESOURCE_FAILURE_RC, rcb, 0);
	} else {
		sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_S,
			SNA_PS_FSM_CONV_INPUT_RECEIVE_IMMEDIATE, rcb, 0);
		rcb->rx_post.max_len 	= tp->rx_req_len;
		rcb->rx_post.fill	= tp->fill;
		err = sna_ps_conv_perform_receive_processing(rcb, tp);
		sna_ps_conv_fsm_post(rcb, SNA_PS_FSM_POST_INPUT_RECEIVE_IMMEDIATE);
	}
	tp->rq_to_send_rcvd     = rcb->rq_to_send_rcvd;
	rcb->rq_to_send_rcvd    = 0;
out:	return err;
}

/**
 * this procedure performs the processing of the post_on_receipt verb.
 * this procedure updates fsm_conversation and fsm_port, saves the post
 * conditions in the rcb, and retrieves any records originated in rm or hs.
 * the data just received from rm or hs may cause the resource to be posted.
 *
 * @tp: post_on_receipt verb parameters.
 *
 * the return code of the verb is updated. if the verb is issued in a valid
 * state, then the state of fsm_post is changed. fsm_conversation is called
 * but does not change states. also, post_conditions.fill and
 * post_conditions.max_length in the rcb are updated to the posting conditions.
 */
static int sna_ps_conv_post_on_receipt_proc(struct sna_tp_cb *tp)
{
	struct sna_rcb *rcb;
	int err = AC_RC_PRODUCT_SPECIFIC;

	sna_debug(5, "init\n");
	rcb = sna_rm_rcb_get_by_index(tp->rcb_index);
	if (!rcb)
		goto out;
	err = AC_RC_OK;
#ifdef NOT
	if(fsm_conversation(s, post_on_receipt, rcb) > state_condition)
		post_on_receipt_verb->rcode = PROGRAM_STATE_CHECK;
	else {
		fsm_conversation(s, post_on_receipt, rcb);
		fsm_post(post_on_receipt);
		rcb->post_conditions.fill = post_on_receipt_verb->fill;
		rcb->post_conditions.max_length = post_receipt_verb->max_length;
		sna_receive_rm_or_hs_to_ps_records(suspend_list);

		post_on_receipt_verb->rcode = OK;
	}
#endif
out:	return err;
}

/**
 * this procedure performs the processing of a test record. the procedure
 * first receives any records from rm and hs. it then tests whether the
 * conversation has been posted or whether request_to_send notification has
 * been received from the remote transaction. the return_code field of test
 * records the result of the test.
 *
 * @tp: test record.
 *
 * the return_code field of test records the result of the test. if the tp
 * is informed that a rq_to_send has been received, then the
 * rcb.rq_to_send_rcvd field is reset to no.
 */
static int sna_ps_conv_test_proc(struct sna_tp_cb *tp)
{
	struct sna_rcb *rcb;
	int err = AC_RC_PRODUCT_SPECIFIC;

	sna_debug(5, "init\n");
	rcb = sna_rm_rcb_get_by_index(tp->rcb_index);
	if (!rcb)
		goto out;
	err = AC_RC_OK;
#ifdef NOT
	test_verb->rcode = OK;
	sna_receive_rm_or_hs_to_ps_records(suspend_list);
	switch(test_verb->param) {
		case (POSTED):
			if(fsm_conversation(s, test_posted, rcb) > state_condition)
				test_verb->rcode = PROGRAM_STATE_CHECK;
			else {
				switch(fsm_error_or_failure()) {
					case (CONV_FAILURE_SON):
						test_verb->rcode = RESOURCE_FAILURE_RETRY;
						fsm_conversation(r, resource_failure_rc, rcb);
						break;
					case (CONV_FAILURE_PROTOCOL_ERROR):
						test_verb->rcode = RESOURCE_FAILURE_NO_RETRY;
						fsm_conversation(r, resource_failure_rc, rcb);
						break;
					case (RCVD_ERROR):
						if(fmh7_in_list(rcb->hs_to_ps_buffer_list) == NOT)
							sna_receive_rm_or_hs_to_ps_records(suspend_list);
						state = fsm_error_or_failure();
						if(state == CONV_FAILURE_SON
							|| state == CONV_FAILURE_PROTOCOL_ERROR) {
							if(state == CONV_FAILURE_SON)
								test_verb->rcode = RESOURCE_FAILURE_RETRY;
							else
								test_verb->rcode = RESOURCE_FAILURE_NO_RETRY;
							fsm_conversation(r, resource_failure_rc, rcb);
						} else
							sna_dequeue_fmh7_proc(test_verb, rcb);
						break;
					case (NO_REQUESTS):
						sna_test_for_post_statisfied(rcb);
						switch(fsm_port) {
							case (PEND_POST):
								test_verb->rcode = UNSUCCESSFUL;
								break;
							case (POSTED):
								if(fmh7 next to process)
									sna_dequeue_fmh7_proc(test_verb, rcb);
								else
									test_verb->subcode = NOT_DATA || DATA;
						}
						if(fsm_conversation() != END_CONV)
							fsm_conversation(s, test, rcb);
						fsm_post(test);
						break;
					case (REQUEST_TO_SEND_RECEIVED):
						if(fsm_conversation(s, test_rq_to_send_rcvd, rcb) > state_condition)
							test_verb->rcode = PROGRAM_STATE_CHECK;
						else {
							if(rcb->rq_to_send_rcvd == YES)
								rcb->rq_to_send_rcvd = NO;
							else
								test_verb->rcode = UNSUCCESSFUL;
							fsm_conversation(s, test_rq_to_send_rcvd, rcv);
						}
						break;
				}
			}
			break;
	}
#endif
out:	return err;
}

static int sna_ps_conv_fmh5_append_tp_name(struct sna_tp_cb *tp,
	struct sk_buff *skb)
{
	u_int8_t e_tp_name[64];
	u_int8_t *tpn_len;

	sna_debug(5, "init\n");
	tpn_len = (u_int8_t *)skb_put(skb, 1);
	atoe_strncpy(e_tp_name, tp->tp_name, tp->tp_name_length);
	memcpy(skb_put(skb, tp->tp_name_length), e_tp_name, tp->tp_name_length);
	*tpn_len = tp->tp_name_length;
	return tp->tp_name_length + 1;
}

static int sna_ps_conv_fmh5_append_security(struct sna_tp_cb *tp,
	struct sk_buff *skb)
{
	u_int8_t *sec_len;

	sna_debug(5, "init\n");
	sec_len = (u_int8_t *)skb_put(skb, 1);
	*sec_len = 0;
	return 1;
}

static int sna_ps_conv_fmh5_append_luw(struct sna_tp_cb *tp, struct sna_remote_lu_cb *remote_lu,
	struct sk_buff *skb)
{
	u_int8_t name_len, name_flat[17], luw_buf[6];
	u_int8_t *luw_len, *lu_len;
	u_int16_t *luw_seq;
	int len = 0;

	sna_debug(5, "init\n");

	/* length. */
	luw_len = (u_int8_t *)skb_put(skb, 1);
	len += 1;

	/* network qualified lu network name length and name. */
	name_len = sna_netid_to_char(&remote_lu->netid_plu, name_flat);
	fatoe_strncpy(name_flat, name_flat, name_len);
	lu_len = (u_int8_t *)skb_put(skb, 1);
	*lu_len = name_len;
	len += 1;
	memcpy(skb_put(skb, name_len), name_flat, name_len);
	len += name_len;

	/* logical-unit-of-work instance number. */
	sna_debug(5, "sizeof(timeval)=%zu\n", sizeof(struct timeval));
	memcpy(luw_buf, &tp->luw, 6);
	memcpy(skb_put(skb, 6), luw_buf, 6);
	len += 6;

	/* logical-unit-of-work sequence number. */
	luw_seq = (u_int16_t *)skb_put(skb, 2);
	*luw_seq = htons(tp->luw_seq);
	len += 2;

	*luw_len = len - 1;
	return len;
}

static int sna_ps_conv_fmh5_append_cnv_correlator(struct sna_tp_cb *tp,
	struct sk_buff *skb)
{
	u_int8_t *cnv_len;

	sna_debug(5, "init\n");
	cnv_len = (u_int8_t *)skb_put(skb, 1);
	*cnv_len = 0;
	return 1;
}

static int sna_ps_conv_fmh5_append_attach_snf(struct sna_tp_cb *tp,
	struct sk_buff *skb)
{
	u_int8_t snf[8];
	u_int8_t *snf_len;
	u_int32_t local_snf;

	sna_debug(5, "init\n");
	snf_len = (u_int8_t *)skb_put(skb, 1);
	memset(snf, 0, 8);
	local_snf = htonl(sna_ps_conv_attach_sqn++);
	memcpy(&snf[4], &local_snf, sizeof(u_int32_t));
	memcpy(skb_put(skb, 8), snf, 8);
	*snf_len = 8;
	return 9;
}

static int sna_ps_conv_build_and_send_fmh5(struct sna_tp_cb *tp,
	struct sna_rcb *rcb)
{
	struct sna_remote_lu_cb *remote_lu;
	struct sna_port_cb *port;
	struct sna_lulu_cb *lulu;
	struct sna_ls_cb *ls;
	struct sna_pc_cb *pc;
	struct sna_hs_cb *hs;
	struct sk_buff *skb;
	int err, size, len;
	sna_fmh5 *fmh5;

	sna_debug(5, "init\n");
	hs = sna_hs_get_by_index(rcb->hs_index);
	if (!hs)
		return -ENOENT;
	lulu = sna_sm_lulu_get_by_index(hs->lulu_index);
	if (!lulu)
		return -ENOENT;
	pc = sna_pc_get_by_index(hs->pc_index);
	if (!pc)
		return -ENOENT;
	port = sna_cs_port_get_by_index(pc->port_index);
	if (!port)
		return -ENOENT;
	ls = sna_cs_ls_get_by_index(port, pc->ls_index);
	if (!ls)
		return -ENOENT;
	remote_lu = sna_rm_remote_lu_get_by_index(tp->remote_lu_index);
	if (!remote_lu)
		return -ENOENT;
	size = sna_dlc_data_min_len(ls) + sizeof(sna_rh)
		+ sizeof(sna_fmh5) + 60; /* static max for now. */
	skb = sna_alloc_skb(size, GFP_ATOMIC);
	if (!skb)
		return -ENOMEM;
	sna_dlc_data_reserve(ls, skb);
	skb_reserve(skb, sizeof(sna_fid2));
	skb_reserve(skb, sizeof(sna_rh));

	/* set fmh5 header. */
	len = sizeof(sna_fmh5);
	fmh5 = (sna_fmh5 *)skb_put(skb, sizeof(sna_fmh5));
	memset(fmh5, 0, sizeof(sna_fmh5));
	fmh5->type		= SNA_FMH_TYPE_5;
	fmh5->cmd		= htons(SNA_FMH_CMD_ATTACH);
	fmh5->vid		= 0;
	fmh5->pvid		= 0;
	fmh5->spwdi		= 0;
	fmh5->pipi		= 0;
	fmh5->xaid		= 0;
	fmh5->fix_len		= 3;
	if (tp->conversation_type == AC_CONVERSATION_TYPE_BASIC)
		fmh5->rsrc_type	= SNA_FMH_RSRC_TYPE_HD_BASIC;
	else
		fmh5->rsrc_type	= SNA_FMH_RSRC_TYPE_HD_MAPPED;
	fmh5->sync_level	= 0;

	/* setup tp name. */
	len += sna_ps_conv_fmh5_append_tp_name(tp, skb);

	/* setup security. */
	len += sna_ps_conv_fmh5_append_security(tp, skb);

	/* setup logical unit of work. */
	len += sna_ps_conv_fmh5_append_luw(tp, remote_lu, skb);

	/* setup conversation correlator. */
	len += sna_ps_conv_fmh5_append_cnv_correlator(tp, skb);

	/* setup attach sequence number field. */
	len += sna_ps_conv_fmh5_append_attach_snf(tp, skb);

	fmh5->len = len;

	/* set buffer transmit information. */
	SNA_SKB_CB(skb)->type = SNA_CTRL_T_REC_ALLOCATE;
	SNA_SKB_CB(skb)->fmh  = 1;
	err = sna_hs_tx_ps_mu_data(rcb, skb);
	if (err < 0) {
		sna_debug(5, "hs_tx_mu failed `%d'.\n", err);
		kfree_skb(skb);
	}
	return err;
}

/* rm_get_session blocks which is how we handle waiting for the
 * session_allocated event.
 */
static int sna_ps_conv_obtain_session(struct sna_rcb *rcb, struct sna_tp_cb *tp)
{
	struct sna_hs_cb *hs;
	struct sna_pc_cb *pc;
	int err;

	sna_debug(5, "init\n");
	err = sna_rm_get_session(tp);
	if (err < 0) {
		sna_debug(5, "get session failed `%d'.\n", err);
		return err;
	}

	/* finish setting up rcb if needed. */
	hs = sna_hs_get_by_index(rcb->hs_index);
	if (!hs)
		return -ENOENT;
	pc = sna_pc_get_by_index(hs->pc_index);
	if (!pc)
		return -ENOENT;
	rcb->tx_max_btu = pc->tx_max_btu;
	rcb->rx_max_btu = pc->rx_max_btu;
	return err;
}

/**
 * this procedure performs further processing of an allocate request. it
 * is invoked when ps received an rcb_allocated record from the resource
 * manager.
 */
static int sna_ps_conv_rcb_allocated_proc(struct sna_rcb *rcb,
	struct sna_tp_cb *tp)
{
	int err = AC_RC_OK;

	sna_debug(5, "init\n");
	err = sna_ps_conv_obtain_session(rcb, tp);
	if (err < 0) {
		sna_debug(5, "obtain session failed `%d'.\n", err);
		err = AC_RC_ALLOCATION_FAILURE_NO_RETRY;
		sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_R,
			SNA_PS_FSM_CONV_INPUT_ALLOCATION_ERROR_RC, rcb, 0);
		goto out;
	}
	sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_NONE,
		SNA_PS_FSM_CONV_INPUT_ALLOCATE, rcb, 0);
	err = sna_ps_conv_build_and_send_fmh5(tp, rcb);
	if (err < 0) {
		err = AC_RC_UNSUCCESSFUL;
		sna_debug(5, "build and send fmh5 failed `%d'.\n", err);
	}
out:	return err;
}

/**
 * this procedure handles allocation of new resources to the transaction
 * program. if the allocate parameters are valid, this procedure requests
 * that rm create a new resource control block (rcb). if the supplied
 * return_control paramter specifies immediate, ps at this time also
 * requests rm to acquire a sessino for use by the conversation resource.
 * if the return_control is set to when_session_allocated,
 * when_conwinner_allocated, or when_conv_group_allocated, ps sends a
 * seperate get_session request to rm at a later time.
 *
 * @tp: allocate verb with parameters; rcb_allocated record received from rm.
 *
 * the allocate_rcb record is initialized and sent to rm and the rcb_allocated
 * record (from rm) is destroyed. if an error is found in the allocate, the
 * return code is updated.
 */
static int sna_ps_conv_alloc_proc(struct sna_tp_cb *tp)
{
	struct sna_rcb *rcb;
	int err = AC_RC_PRODUCT_SPECIFIC;

	sna_debug(5, "init\n");
	rcb = sna_rm_alloc_rcb(tp);
	if (!rcb) {
		err = AC_RC_UNSUCCESSFUL;
		goto out;
	}
	tp->rcb_index = rcb->index;
	err = sna_ps_conv_rcb_allocated_proc(rcb, tp);
out:	return err;
}

/**
 * this procedure completes the processing of a deallocate verb that
 * specifies type = abend.
 */
#ifdef NOT
static int sna_ps_conv_complete_deallocate_abend_proc(struct sna_rcb *rcb)
{
	sna_debug(5, "init\n");
	struct sna_mu *mu;
	switch(deallocate->type) {
		case (ABEND_PROG):
			sense = 0x08640000;
			break;
		case (ABEND_SVC):
			sense = 0x08640001;
			break;
		case (ABEND_TIMER):
			sense = 0x08640002;
			break;
	}
	mu = bm(GET_SEND_BUF);
	if(mu != NULL)
		send_to_hs(mu);
	sna_create_and_init_limited_mu(rcb, mu);
	if(log_data != NULL) {
		mu->log_data = log_data;
		mu->sense = sense;

		err_log_gds = create();
		sna_send_data_buffer_management(err_log_gds, rcb);
		Log_err(err_log_gds);
	} else
		store_mu(no_log_data);
	mu->ps_to_hs.type = DEALLOCATE_FLUSH;
	send_to_hs(mu);
	return 0;
}
#endif

/**
 * this procedure creates a deallocate_rcb and sends it to tm. rm's
 * processing of this record includes removing the resource from the
 * resource list, destroying the rcb and returning a rcb_deallocated
 * to inform ps that the processing is complete.
 */
static int sna_ps_conv_end_conversation_proc(struct sna_rcb *rcb)
{
	sna_debug(5, "init\n");
#ifdef NOT
	struct sna_mu *mu;
	struct sna_deallocate_rcb *deallocate_rcb;

	for(mu = rcb->hs_to_ps_buffer_list; mu != NULL; mu = mu->next)
		bm(FREE, mu);

	if(rcb->send_buffer != NULL)
		bm(FREE, rcb->send_buffer);

	new(deallocate_rcb, GFP_ATOMIC);
	rcb_deallocated = sna_wait_for_rm_reply(rcb);
	destroy(rcb_deallocated);
#endif
	return 0;
}

/**
 * this procedure is invoked when a deallocate is received that specifies
 * type = flush, or type = sync_level and the sync_level of the conversation
 * is none.
 */
static int sna_ps_conv_deallocate_flush_proc(struct sna_rcb *rcb)
{
	sna_debug(5, "init\n");
#ifdef NOT
	struct sna_mu *mu;
	if(tp->data != logical_record_boundary)
		deallocate->rcode = PROGRAM_STATE_CHECK;
	else {
		sna_receive_rm_or_hs_to_ps_records(suspends_list);
		state = fsm_error_or_failure();
		if(state == RCVD_ERROR || state == NO_REQUESTS) {
			if(mu == NULL)
				sna_create_and_init_limited_mu(rcb, mu);
			mu->ps_to_hs.type = DEALLOCATE_FLUSH;
		}
		deallocate->rcode = OK;
		fsm_conversation(s, deallocate_flush, rcb);
		sna_end_conversation_proc(rcb);
	}
#endif
	return 0;
}

/**
 * this procedure is invoked when deallocate type(confirm) or deallocate
 * type(sync_level) is issued for a conversation whole sync_level is confirm.
 */
static int sna_ps_conv_deallocate_confirm_proc(struct sna_rcb *rcb)
{
	sna_debug(5, "init\n");
#ifdef NOT
	struct sna_mu *mu;

	if(tp->data != at_logical_boundary)
		deallocate->rcode = PROGRAM_STATE_CHECK;
	else {
		fsm_conversation(s, deallocate_confim, rcb);
		sna_receive_rm_or_hs_to_ps_records(suspend_list);
		switch(fsm_error_or_failure()) {
			case (CONV_FAILURE_PROTOCOL_ERROR):
				deallocate->rcode = RESOURCE_FAILURE_NO_RETRY;
				fsm_conversation(r, RESOURCE_FAILURE_RC, rcb);
				break;
			case (CONV_FAILURE_SON)
				deallocate->rcode = RESOURCE_FAILURE_RETRY;
				fsm_conversation(r, RESOURCE_FAILURE_RC, rcb);
				break;
			case (RCVD_ERROR):
				if(mu == NULL)
					sna_create_and_init_limited_mu(rcb, mu);
				mu->ps_to_hs.type = PREPARE_TO_RCV_FLUSH;
				send_to_hs(mu);
				if(fmh7 !in rcb->hs_to_ps_buffer_list)
					sna_receive_rm_or_hs_to_ps_records(suspend_list);
				else
					sna_receive_rm_or_hs_to_ps_records(suspend_list); /* Empty */
				if(state == CONV_FAILURE_SON
					|| state == CONV_FAILURE_PROTOCOL_ERROR) {
					if(state == CONV_FAILURE_SON)
						deallocate->rcode = RESOURCE_FAILURE_RETRY;
					if(state == CONV_FAILURE_PROTOCOL_ERROR)
						deallocate->rcode = RESOURCE_FAILURE_NO_RETRY;
					fsm_conversation(r, resource_failure_rc, rcb);
				}
				else
					sna_deqeue_fmh7_proc(deallocate, rcb);
			case (NO_REQUESTS):
				if(mu == NULL)
					mu = sna_create_and_init_limited_mu(rcb, mu);
				mu->ps_to_hs.type = DEALLOCATE_CONFIRM;
				sna_wait_for_confirmed_proc(deallocate, rcb);
				break;
		}
	}
#endif
	return 0;
}

/**
 * this prodecure is invoked when the type parameter of deallocate verb
 * is abend_prog, abend_svc, or abend_timer.
 */
static int sna_ps_conv_deallocate_abend_proc(struct sna_rcb *rcb)
{
	sna_debug(5, "init\n");
#ifdef NOT
	sna_receive_rm_or_hs_to_ps_records(susped_list);
	state = fsm_error_or_failure();
	if(state == NO_REQUEST || state == RCVD_ERROR) {
		switch(fsm_conversation()) {
			case (RCV_STATE):
				if(DEALLOCATE_FLUSH != received) {
					sna_send_eror_to_hs_proc(rcb);
					sna_wait_for_send_error_done_proc(deallocate, rcb);
				}
				break;
			case (RCVD_CONFIRM):
			case (RCVD_CONFIRM_SEND):
			case (RCVD_CONFIRM_DEALL):
				sna_send_error_to_hs_proc(rcb);
				sna_wait_for_send_error_done_proc(deallocate, rcb);
				break;
			case (SEND_STATE):
			case (PREP_TO_RCV_DEFER):
			case (DEALL_DEFER):
				sna_complete_deallocate_abend_proc(deallocate,rcb);
				break;
		}
	}
	deallocate->rcode = OK;
	sna_fsm_conversation(s, dellocate, rcb);
	sna_end_conversation_proc(rcb);
#endif
	return 0;
}

/**
 * this procedure handles the deallocation of resources. if the resource
 * specified in the deallocate is valid resource and the conversation is in
 * a pertinent state, ps calls the appropraite deallocation procedure to
 * continue processing the deallocate.
 *
 * @tp: deallocate verb parameters.
 *
 * the return code of the deallocate is set here or in one of the called
 * procedures, and fsm_conversation may change states. also, the pertinent
 * deallocation procedure is called. when appropriate, ps sends
 * deallocate_rcb to rm.
 */
static int sna_ps_conv_deallocate_proc(struct sna_tp_cb *tp)
{
	struct sna_rcb *rcb;
	int err = AC_RC_PRODUCT_SPECIFIC;

	sna_debug(5, "init\n");
	rcb = sna_rm_rcb_get_by_index(tp->rcb_index);
	if (!rcb)
		goto out;
	err = AC_RC_OK;
	if ((tp->deallocate_type == AC_DEALLOCATE_FLUSH
		|| tp->deallocate_type == AC_DEALLOCATE_SYNC_LEVEL)
		&& rcb->sync_level == AC_SYNC_LEVEL_NONE) {
		if (sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_S,
			SNA_PS_FSM_CONV_INPUT_DEALLOCATE_FLUSH, rcb, 1))
			err = AC_RC_PROGRAM_STATE_CHECK;
		else
			sna_ps_conv_deallocate_flush_proc(rcb);
		goto out;
	}
	if (tp->deallocate_type == AC_DEALLOCATE_CONFIRM) {
		if (sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_S,
			SNA_PS_FSM_CONV_INPUT_DEALLOCATE_CONFIRM, rcb, 1))
			err = AC_RC_PROGRAM_STATE_CHECK;
		else {
			if (rcb->sync_level == AC_SYNC_LEVEL_CONFIRM
				|| rcb->sync_level == AC_SYNC_LEVEL_SYNCPT)
				sna_ps_conv_deallocate_confirm_proc(rcb);
			else
				err = AC_RC_PROGRAM_PARAMETER_CHECK;
		}
		goto out;
	}
	if (tp->deallocate_type == AC_DEALLOCATE_SYNC_LEVEL
		&& rcb->sync_level == AC_SYNC_LEVEL_CONFIRM) {
		if (sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_S,
			SNA_PS_FSM_CONV_INPUT_DEALLOCATE_CONFIRM, rcb, 1))
			err = AC_RC_PROGRAM_STATE_CHECK;
		else
			sna_ps_conv_deallocate_confirm_proc(rcb);
		goto out;
	}
	if (tp->deallocate_type == AC_DEALLOCATE_SYNC_LEVEL
		&& rcb->sync_level == AC_SYNC_LEVEL_SYNCPT) {
		if (sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_S,
			SNA_PS_FSM_CONV_INPUT_DEALLOCATE_DEFER, rcb, 1))
			err = AC_RC_PROGRAM_STATE_CHECK;
		else {
			if (rcb->ll_tx_state != SNA_RCB_LL_STATE_COMPLETE
				|| rcb->ll_rx_state != SNA_RCB_LL_STATE_COMPLETE) {
				sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_S,
					SNA_PS_FSM_CONV_INPUT_DEALLOCATE_DEFER, rcb, 0);
				err = AC_RC_OK;
			} else
				err = AC_RC_PROGRAM_STATE_CHECK;
		}
		goto out;
	}
	if (tp->deallocate_type == AC_DEALLOCATE_ABEND_PROG
		|| tp->deallocate_type == AC_DEALLOCATE_ABEND_SVC
		|| tp->deallocate_type == AC_DEALLOCATE_ABEND_TIMER) {
		if (sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_S,
			SNA_PS_FSM_CONV_INPUT_DEALLOCATE_LOCAL, rcb, 1))
			err = AC_RC_PROGRAM_STATE_CHECK;
		else
			sna_ps_conv_deallocate_abend_proc(rcb);
		goto out;
	}
	if (tp->deallocate_type == AC_DEALLOCATE_FLUSH) {
		if (sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_S,
			SNA_PS_FSM_CONV_INPUT_DEALLOCATE_LOCAL, rcb, 1))
			err = AC_RC_PROGRAM_STATE_CHECK;
		else {
			err = AC_RC_OK;
			sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_S,
				SNA_PS_FSM_CONV_INPUT_DEALLOCATE_LOCAL, rcb, 0);
			sna_ps_conv_end_conversation_proc(rcb);
		}
		goto out;
	}
out:	return err;
}

/**
 * this procedure handles the confirm verb processing. if it is appropriate
 * for the transaction program to issue a confirm for the specified
 * conversation (ie the sync_level of the conversation for which the confirm
 * was issued is confirm or syncpt and any data issued by the transaction
 * program is on a logical record boundary), this procedure retries any
 * records from hs and rm. appropriate action is taken depending upon which,
 * if any, record was received (as reflected by the state of
 * fsm_error_or_failure).
 *
 * @tp: confirm verb paramters.
 *
 * an rq_to_send_rcvd indication could be passed up to the tp at this time
 * if the rcb.rq_to_send_rcvd field has been set to show receipt of a
 * rq_to_send. the rcb.rq_to_send_rcvd field is reset to no. the return code
 * of the confirm verb is updated.
 */
static int sna_ps_conv_confirm_proc(struct sna_tp_cb *tp)
{
	struct sna_rcb *rcb;
	int err = AC_RC_PRODUCT_SPECIFIC;

	sna_debug(5, "init\n");
	rcb = sna_rm_rcb_get_by_index(tp->rcb_index);
	if (!rcb)
		goto out;
	err = AC_RC_OK;
#ifdef NOT
	struct sna_mu *mu;
	if(rcb->sync_level == NONE || send data != logical bounds) {
		if(rcb->sync_level == NONE)
			confirm_verb->rcode = PROGRAM_PARAM_CHECK;
		else
			confirm_verb->rcode = PROGRAM_STATE_CHECK;
	} else {
		if(fsm_conversation(s, confirm, rcb) cause state check)
			confirm_verb->rcode = PROGRAM_STATE_CHECK;
		else {
			sna_receive_rm_or_hs_to_ps_records(suspend_list);
			switch(fsm_error_or_failure())
			{
				case (CONV_FAILURE_PROTOCOL_ERROR):
					confirm_verb->rcode = RESOURCE_FAILURE_NO_RETRY;
					fsm_conversation(r, resource_failure_rc, rcb);
					break;

				case (CONV_FAILURE_SON):
					confirm_verb->rcode = RESOURCE_FAILURE_RETRY;
					fsm_conversation(r, resource_failure_rc, rcb);
					break;

				case (RCVD_ERROR):
					if(mu == NULL)
						sna_create_and_init_limited_mu(rcb, mu);
					mu->ps_to_hs.type = PREPARE_TO_RCV_FLUSH;
					send_to_hs(mu);

					sna_receive_rm_or_hs_to_ps_records(suspend_list);
					state = fsm_error_or_failure();
					if(state == CONV_FAILURE_SON
						|| state == CONV_FAILURE_PROTOCOL_ERROR)
					{
						if(state == CONV_FAILURE_SON)
							confirm_verb->rcode = RESOURCE_FAILURE_RETRY;
						else
							confirm_verb->rcode = RESOURCE_FAILURE_NO_RETRY;
						fsm_conversation(r, resource_failure_rc, rcb);
					}
					else
						sna_dequeue_fmh7_proc(confirm_verb, rcb);
					break;

				case (NO_REQUESTS):
					sna_complete_confirm_proc(confirm_verb, rcb);
			}

		confirm_verb->request_to_send_received = rcb->rq_to_send_rcvd;
		rcb->request_to_send_received = NO;
	}
#endif
out:	return err;
}

/**
 * this procedure handles confirmed verb processing. ps first retrieves any
 * records from hs and rm. appropriate action is taken depending upon which,
 * if any, record was received.
 *
 * @tp: confirmed verb parameters.
 *
 * the return code of the confirmed verb is set. the states of fsm_conversation
 * and fsm_error_or_failure may change.
 */
static int sna_ps_conv_confirmed_proc(struct sna_tp_cb *tp)
{
	struct sna_rcb *rcb;
	int err = AC_RC_PRODUCT_SPECIFIC;

	sna_debug(5, "init\n");
	rcb = sna_rm_rcb_get_by_index(tp->rcb_index);
	if (!rcb)
		goto out;
	err = AC_RC_OK;

#ifdef NOT
	if(fsm_conversation(s, confirmed, rcb) > cause state_condition)
		confirmed_verb->rcode = PROGRAM_STATE_CHECK;
	else {
		sna_receive_rm_or_hs_to_ps_records(suspend_list);
		switch(fsm_error_or_failure())
		{
			case (NO_REQUESTS):
				sna_send_confirmed_proc(rcb);
				break;

			case (RCVD_ERROR):
				sna_ps_protocol_error(rcb->hs_id, 0x10010000);
				break;

			case (CONV_FAILURE_PROTOCOL_ERROR):
			case (CONV_FAILURE_SON):
				/* Do nothing */
				break;
		}

		fsm_conversation(s, confirmed, rcb);
		confirmed_verb->rcode = OK;
	}
#endif
out:	return err;
}

/**
 * this procedure handles the flush verb processing. the procedure first
 * receives records from rm and hs. appropriate action is taken depending
 * upon the type of the received record as indicated by the
 * fsm_conversation and fsm_error_or_failure states.
 *
 * @tp: flush verb parameters.
 *
 * the send mu.ps_to_hs.type field is set according to the state of
 * fsm_conversation, and the return code on the flush verb is set. if
 * a sending mu is present and contains data, the mu will be sent to hs.
 */
static int sna_ps_conv_flush_proc(struct sna_tp_cb *tp)
{
	struct sna_rcb *rcb;
	int err = AC_RC_PRODUCT_SPECIFIC;

	sna_debug(5, "init\n");
	rcb = sna_rm_rcb_get_by_index(tp->rcb_index);
	if (!rcb)
		goto out;
	err = AC_RC_OK;
#ifdef NOT
	struct sna_mu *mu;
	if(fsm_conversation(s, flush, rcb) > state_condition)
		flush_verb->rcode = PROGRAM_STATE_CHECK;
	else {
		sna_receive_rm_or_hs_to_ps_records(suspend_list);
		state = fsm_error_or_failure();
		if(state == RCVD_ERROR || state == NO_REQUESTS) {
			switch(fsm_conversation()) {
				case (SEND_STATE):
					if(mu != NULL)
						send_to_hs(mu);
					break;
				case (PREP_TO_RCV_DEFER):
					if(mu == NULL)
						sna_create_and_init_limited_mu(rcb, mu);
					mu->ps_to_hs.type = PREPARE_TO_RCV_FLUSH;
					send_to_hs(mu);
					break;
				case (DEALL_DEFER):
					if(mu == NULL)
						sna_create_and_init_limited_mu(rcb, mu);
					mu->ps_to_hs.type = DEALLOCATE_FLUSH;
					send_to_hs(mu);
					break;

			}
			if(fsm_conversation() == DEALL_DEFER)
				sna_end_conversation_proc(rcb);
			fsm_conversation(s, flush, rcb);
		}
		flush_verb->rcode = OK;
	}
#endif
out:	return err;
}

/**
 * this procedure handles the prepare_to_receive verb. depending on the
 * type of the prepare_to_receive (flush, confirm or sync_level) and
 * the sync_level of the conversation (none, confirm, or syncpt), the
 * processing of the prepare_to_receive is continued by other procedures.
 *
 * @tp: prepare_to_receive verb parameters.
 *
 * if the prepare_to_receive specifies type = sync_level and the
 * sync_level of the conversation is syncpt, the return_code is set to
 * ok and fsm_conversation is updated to indicate that completion of the
 * prepare_to_receive processing is deferred until a flush, confirm, or
 * syncpt verb is issued. otherwise, processing is continued by other
 * procedures.
 */
static int sna_ps_conv_prepare_to_receive_proc(struct sna_tp_cb *tp)
{
	struct sna_rcb *rcb;
	int err = AC_RC_PRODUCT_SPECIFIC;

	sna_debug(5, "init\n");
	rcb = sna_rm_rcb_get_by_index(tp->rcb_index);
	if (!rcb)
		goto out;
	err = AC_RC_OK;
#ifdef NOT
	if(tp->data_sent != logical_bounday)
		prepare_to_receive_verb->rcode = PROGRAM_STATE_CHECK;
	else {
		switch(prepare_to_receive_verb->type) {
			case (FLUSH):
			case (SYNC_LEVEL):
				if(prepare_to_receive_verb->sync_level == NONE) {
					if(fsm_conversation(s, prepare_to_receive_flush, rcb) > state_condition)
						prepare_to_receive_verb->rcode = PROGRAM_STATE_CHECK;
					else
						sna_prepare_to_receive_flush_proc(prepare_to_receive_verb, rcb);
					break;
				}
			case (CONFIRM):
				if(fsm_conversation(s, prepare_to_receive_confirm, rcb) > state_condition)
					prepare_to_receive_verb->rcode = PROGRAM_STATE_CHECK;
				else {
					if(sync_level == CONFIRM
						|| sync_level == SYNCPT)
						sna_prepare_to_receive_confirm_proc(prepare_to_receive_verb, rcb);
					else
						prepare_to_receive_verb->rcode = PROGRAM_PARAM_CHECK;
				}
				break;
			case (SYNC_LEVEL):
				if(sync_level == CONFIRM) {
					if(fsm_conversation(s, prepare_to_receive_confirm, rcb) > state_condition)
						prepare_to_receive_verb->rcode = PROGRAM_STATE_CHECK;
					else
						sna_prepare_to_receive_confirm_proc(prepare_to_receive_verb, rcb);
					break;
				}
				if(sync_level == SYNCPT) {
					if(fsm_conversation(s, prepare_to_receive_defer, rcb) > state_condition)
						prepare_to_receive_verb->rcode = PROGRAM_STATE_CHECK;
					else {
						fsm_conversation(s, prepare_to_receve_defer, rcb);
						rcb->locks = prepare_to_receive_verb->locks;
						prepare_to_receive_verb->rcode = OK;
					}
					break;
				}
		}
	}
#endif
out:	return err;
}

/**
 * this procedure handles requests for information about a conversation.
 * information about the conversation resource is retrived from the pertinent
 * control blocks, and placed in the returned parameters of the get_attributes
 * verb.
 *
 * @tp: get_attributes verb parameters.
 *
 * get_attributes verb returned parameters containing information about
 * the conversation.
 */
static int sna_ps_conv_get_attributes_proc(struct sna_tp_cb *tp)
{
	struct sna_rcb *rcb;
	int err = AC_RC_PRODUCT_SPECIFIC;

	sna_debug(5, "init\n");
	rcb = sna_rm_rcb_get_by_index(tp->rcb_index);
	if (!rcb)
		goto out;
	err = AC_RC_OK;
	sna_ps_conv_fsm_conversation(SNA_PS_FSM_CONV_INPUT_S,
		SNA_PS_FSM_CONV_INPUT_GET_ATTRIBUTES, rcb, 0);

#ifdef NOT
	struct sna_partner_lu *plu;

	get_attributes_verb->conversation_group_id = rcb->conversation_group_id;
	get_attributes_verb->partner_fq_lu_name = plu->fq_lu_name;
	get_attributes_verb->partner_lu_name = rcb->lu_name;
	get_attributes_verb->mode_name = rcb->mode_name;
	get_attributes_verb->sync_level = rcb->sync_level;
#endif
out:	return err;
}

/**
 * this procedure receives conversation verbs issued by the tp or by other
 * ps components, and calls the appropriate procedures to process them.
 *
 * @verb: transaction program verb.
 * @tp: transaction program information.
 *
 * refer to the procedures that are called from this procedure for the
 * specific outputs.
 */
int sna_ps_conv(int verb, struct sna_tp_cb *tp)
{
	int err = AC_RC_INVALID_VERB;

	sna_debug(5, "init\n");
	switch (verb) {
		case ALLOCATE:
			err = sna_ps_conv_alloc_proc(tp);
			break;
		 case CONFIRM:
			err = sna_ps_conv_confirm_proc(tp);
			break;
		case CONFIRMED:
			err = sna_ps_conv_confirmed_proc(tp);
			break;
		case DEALLOCATE:
			err = sna_ps_conv_deallocate_proc(tp);
			break;
		case FLUSH:
			err = sna_ps_conv_flush_proc(tp);
			break;
		case GET_ATTRIBUTES:
			err = sna_ps_conv_get_attributes_proc(tp);
			break;
		case POST_ON_RECEIPT:
			err = sna_ps_conv_post_on_receipt_proc(tp);
			break;
		case PREPARE_TO_RECEIVE:
			err = sna_ps_conv_prepare_to_receive_proc(tp);
			break;
		case RECEIVE_AND_WAIT:
			err = sna_ps_conv_rcv_and_wait_proc(tp);
			break;
		case RECEIVE_IMMEDIATE:
			err = sna_ps_conv_rcv_immediate_proc(tp);
			break;
		case REQUEST_TO_SEND:
			err = sna_ps_conv_request_to_send_proc(tp);
			break;
		case SEND_DATA:
			err = sna_ps_conv_send_data_proc(tp);
			break;
		case SEND_ERROR:
			err = sna_ps_conv_send_error_proc(tp);
			break;
		case TEST:
			err = sna_ps_conv_test_proc(tp);
			break;
	}
	return err;
}

#ifdef NOT
static int sna_complete_confirm_proc(struct sna_confirm *confirm, struct sna_rcb *rcb)
{
	struct sna_mu *mu;

	mu = bm(GET_MU_BUFF);
	if(mu == NULL)
		sna_create_and_init_limited_mu(rcb, mu);

	switch(fsm_conversation())
	{
		case (SEND_STATE):
			mu->ps_to_hs.type = CONFIRM;
			send_to_hs(mu);
			break;

		case (PREP_TO_RCV_DEFER):
			if(rcb->locks == SHORT)
				mu->ps_to_hs.type = PREPARE_TO_RCV_CONFIRM_SHORT;
			else
				mu->ps_to_hs.type = PREPARE_TO_RCV_CONFIRM_LONG;

			send_to_hs(mu);
			break;

		case (DEALL_DEFER):
			mu->ps_to_hs.type = DEALLOCATE_CONFIRM;
			send_to_hs(mu);
			break;

	}

	fsm_conversation(s, confirm, rcb);
	sna_wait_for_confirmed_proc(confirm, rcb);

	return (0);
{

static int sna_get_dallocate_from_hs(struct sna_tp_verb *tp_verb, struct sna_rcb *rcb)
{
	chain_type = sna_get_end_chain_from_hs(rcb);
	if(chain_type == DEALLOCATE_FLUSH || chain_type == DEALLOCATE_CONFIRM)
		do_nothing();

	state = fsm_error_or_failure();
	if(state == CONV_FAILURE_PROTOCOL_ERROR || state == CONV_FAILURE_SON)
		do_nothing();

	/* Otherwise */
	sna_ps_protocol_error(rcb->hs_id, 0x1008201D);
	tp_verb->rcode = RESOURCE_FAILURE_NO_RETRY;
	rcb->fsm_conversation(r, resource_failure_rc, rcb);

	return (0);
}

static int sna_get_end_chain_from_hs(struct sna_rcb *rcb)
{
	struct sna_mu *mu;

	if(end_of_chain != recieved_for_this_conv)
	{
		for(mu = rcb->hs_to_ps_buffer_list; mu != NULL; mu = mu->next)
		{
			if(mu->hs_to_ps.type == END_OF_CHAIN)
				type = mu;

			bm(FREE, mu);
		}
	}

	while(end_of_chain != recieved)
	{
		record = grab_record(rcb);
		switch(record->type)
		{
			case (CONVERSATION_FAILURE):
				sna_conversation_failure_proc(record);
				break;

			case (REQUEST_TO_SEND):
				rcb->rq_to_send_rcvd = YES;
				destroy(record);
				break;

			case (RECEIVE_ERROR):
				destroy(record);
				break;

			case (MU):
				if(mu->hs_to_ps.type == END_OF_CHAIN)
					type = mu;
				bm(FREE, mu);
				break;

			default:
				sna_ps_protocol_error(rcb->hs_id, 0x10010000);
				fsm_conversation(s, confirmed, rcb);
				break;
		}
	}

	/* Update rcb to show receipt of eoc type */
	rcb->end_of_chain = eoc;

	return (0);
}

static int sna_obtain_session_proc(struct sna_rcb *rcb, sna_allocate_verb *allocate_verb, __u8 session_req, __u8 conv_group_id)
{
	struct sna_mu *mu;
	struct sna_get_session *get_session;
	struct sna_session_allocated *session_allocated;

	new(get_session, GFP_ATOMIC);
	if (!get_session)
		return -ENOMEM;
	get_session->tcb_id 	= rcb->tcb_id;
	get_session->rcb_id 	= rcb->rcb_id;
	get_session->type	= session_req;
	get_session->conv_group_id = conv_group_id;
	send_to_rm(get_session);

	session_allocated = sna_wait_for_rm_reply();
	switch(session_allocated->rcode)
	{
		case (OK):
			rcb->send_ru_size = session_allocated->send_ru_size;
			rcb->limit_buf_pool_id = session_allocated->limit_buf_pool_id;
			rcb->perm_buf_pool_id = session_allocated->perm_buf_pool_id;
			sna_create_and_init_limited_mu(rcb, mu);
			if(session_allocated->in_conversation == YES)
				mu->ps_to_hs.allocate = NO;
			else
				mu->ps_to_hs.allocate = YES;
			break;

		default:
			allocate_verb->rcode = ALLOCATION_ERROR;
			switch(sesion_allocated->rcode);
			{
				case (UNSUCCESSFUL_RETRY):
					allocate_verb->subcode = ALLOCATION_FAILURE_RETRY;
					break;

				case (UNSUCCESSFUL_NO_RETRY):
					allocate_verb->subcode = ALLOCATION_FAILURE_NO_RETRY;
					break;

				case (SYNC_LEVEL_NOT_SUPPORTED):
					allocate_verb->subcode = SYNC_LEVEL_NOT_SUPPORTED;
					break;
			}
			break;
	}

	destroy(session_allocated);

	return (0);
}

static int sna_perform_more_rcv_processing(struct sna_rcb *rcb, struct sna_receive_verb *receive_verb)
{
	switch(fsm_error_or_failure())
	{
		case (CONV_FAILURE_PROTOCOL_ERROR):
			receive_verb->rcode = RESOURCE_FAILURE_NO_RETRY;
			fsm_conversation(r, resource_failure_rc, rcb);
			break;

		case (CONV_FAILURE_SON):
			receive_verb->rcode = RESOURCE_FAILURE_RETRY;
			fsm_conversation(r, resource_failure_rc, rcb);
			break;

		default:
			if(receive_verb->type == RECEIVE_IMMEDIATE)
				receive_verb->rcode = UNSUCCESSFUL;
			else
				receive_verb->rcode = OK;
			break;
	}

	return (0);
}

static int sna_perpare_to_receive_confirm_proc(struct sna_prepare_to_receive_verb *prepare_to_receive_verb, struct sna_rcb *rcb)
{
	struct sna_mu *mu;

	fsm_conversation(s, prepare_to_receive_confirm, rcb);
	sna_receive_rm_or_hs_to_ps_records(suspend_list);

	switch(fsm_error_or_failure())
	{
		case (CONV_FAILURE_PROTOCOL_ERROR):
			prepare_to_receive_verb->rcode = RESOURCE_FAILURE_NO_RETRY;
			fsm_conversation(r, resource_failure_rc, rcb);
			break;

		case (CONV_FAILURE_SON):
			prepare_to_receive_verb->rcode = RESOURCE_FAILURE_RETRY;
			fsm_conversation(r, resource_failure_rc, rcb);
			break;

		case (RCVD_ERROR):
			if(mu == NULL)
				sna_create_and_init_limited_mu(rcb, mu);
			mu->ps_to_hs.type = PREPARE_TO_RCV_FLUSH;
			sna_receive_rm_or_hs_to_ps_records(suspend_list);

			state = fsm_error_or_failure();
			if(state == CONV_FAILURE_SON
				|| state == CONV_FALIURE_PROTOCOL_ERROR)
			{
				if(state == CONV_FAILURE_SON)
					prepare_to_receive_verb->rcode = RESOURCE_FAILURE_RETRY;
				else
					prepare_to_receive_verb->rcode = RESOURCE_FAILURE_NO_RETRY;
				fsm_conversation(r, resource_failure_rc, rcb);
			}
			else
				sna_dequeue_fmh7_proc(prepare_to_receive_verb, rcb);
			break;

		case (NO_REQUESTS):
			if(rcb->locks == SHORT)
				mu->ps_to_hs.type = PREPARE_TO_RCV_CONFIRM_SHORT
			else
				mu->ps_to_hs.type = PREPARE_TO_RCV_CONFIRM_LONG;
			sna_wait_for_confirmed_proc(prepare_to_receive_verb, rcb);
			break;

		default:
			/* Error */
	}

	return (0);
}

static int sna_perpare_to_receive_flush_proc(struct sna_prepare_to_receive_verb *prepare_to_receive_verb, struct sna_rcb *rcb)
{
	struct sna_mu *mu;

	sna_receive_rm_or_hs_to_ps_records(suspend_list);
	state = fsm_error_or_failure();
	if(state == RCVD_ERROR || state == NO_REQUESTS)
	{
		if(mu == NULL)
			sna_create_and_init_limited_mu(rcb, mu);
		mu->ps_to_hs.type = PREPARE_TO_RECEIVE_FLUSH;
		send_to_hs(mu);
	}

	prepare_to_receive_verb->rcode = OK;
	fsm_conversation(s, prepare_to_receive_flush, rcb);

	return (0);
}

static int sna_send_confirmed_proc(struct sna_rcb *rcb)
{
	struct sna_mu *mu;

	mu = bm(GET_BUFFER, rcb->perm_buf_pool_id, no_wait);
	if(mu == NULL)
		mu = bm(GET_BUFFER, demand, size, no_wait);

	mu->header_type = PS_TO_HS;
	mu->ps_to_hs.bracket_id = rcb->bracket_id;
	mu->ps_to_hs.ps_to_hs_variant = CONFIRMED;

	send_to_hs(mu);

	return (0);
}

static int sna_send_err_done_proc(struct sna_send_error_verb *send_error_verb, struct sna_rcb *rcb)
{
	struct sna_mu *mu;

	switch(send_error_verb->type)
	{
		case (PROG):
			state = fsm_conversation();
			if(state == SEND_STATE)
			{
				if(data sent by tp is at logical bounds)
					sense = 0x08890000;
				else
					sense = 0x08890001;
				break;
			}

			if(state == RCV_STATE
				|| state == RCVD_CONFIRM
				|| state == RCVD_CONFIRM_SEND
				|| state == RCVD_CONFIRM_DEALL)
			{
				sense = 0x08890100;
				break;
			}

		case (SVC):
			state = fsm_conversation();

			if(state == SEND_STATE)
			{
				if(data is at logical bounds)
					sense = 0x08890100;
				else
					sense = 0x08890101;
				break;
			}

			if(state == RCV_STATE
				|| state == RCVD_CONFIRM
				|| state == RCVD_CONFIRM_SEND
				|| state == RCVD_CONFIRM_DEALL)
			{
				sense = 0x08890100;
				break;
			}

		default:
			/* Error */
	}

	if(send_error_verb->log_data != NULL)
	{
		create_fmh7_with_log_data();
		create_log_gds(log_data);
		sna_send_data_buffer_management(log_gds, rcb);
		Log_err(sys);
	}
	else
		create_fmh7_with_log_data();

	if(FLUSH == NOT_IMPLEMENTED || fmh7->flush_immediately == TRUE)
		send_to_hs(mu);

	send_error_verb->rcode = OK;

	return (0);
}

static int sna_send_err_in_receive_state(struct sna_send_error_verb *send_error_verb, struct sna_rcb *rcb)
{
	struct sna_mu *mu, *mu_ptr;

	mu_ptr = rcb->hs_to_ps_buffer_list;

	if(rcb->ec_type == DEALLOCATE_FLUSH)
	{
		if(mu_ptr != NULL)
			bm(FREE, mu_ptr);
		send_error_verb->rcode = DEALLOCATE_NORMAL;
		fsm_conversation(r, deallocate_normal_rc, rcb);
	}
	else
	{
		sna_send_error_to_hs_proc(rcb);
		sna_wait_for_send_error_done_proc(send_error_verb, rcb);
	}

	return (0);
}

static int sna_send_err_in_send_state(struct sna_send_error_verb *send_error_verb, struct sna_rcb *rcb)
{
	struct sna_mu *mu;

	if(fsm_error_or_failure() == NO_REQUESTS)
	{
		if(send_mu_buffer_present)
			send_to_hs(mu);
		fsm_conversation(s, send_error, rcb);
		sna_send_error_done_proc(send_error_verb, rcb);
	}
	else
	{
		mu->ps_to_hs.type = PREPARE_TO_RCV_FLUSH;
		send_to_hs(mu);
		sna_receive_rm_or_hs_to_ps_records(suspend_list);

		state = fsm_error_or_failure();
		if(state == CONV_FAILURE_SON
			|| state == CONV_FAILURE_PROTOCOL_ERROR)
		{
			if(state == CONV_FAILURE_SON)
				send_error_verb->rcode = RESOURCE_FAILURE_RETRY;
			else
				send_error_verb->rcode = RESOURCE_FAILURE_NO_RETRY;
			fsm_conversation(r, resource_failure_rc, rcb);
		}
		else
			sna_dequeue_fmh7_proc(send_error_verb, rcb);
	}

	set_rcb_send_fields_to_init(rcb);

	return (0);
}

static int sna_send_err_to_hs_proc(struct sna_rcb *rcb)
{
	struct sna_mu *mu;

	mu = bm(GET_BUFFER, size);

	mu->header_type			= PS_TO_HS;
	mu->ps_to_hs.bracket_id		= rcb->bracket_id;
	mu->ps_to_hs.ps_to_hs_variant 	= SEND_ERROR;	/* verb?? */

	send_to_hs(mu);

	return (0);
}

static int sna_send_request_to_send_proc(struct sna_rcb *rcb)
{
	struct sna_mu *mu;

	mu = bm(GET_BUFFER, size);

	mu->header_type			= PS_TO_HS;
	mu->ps_to_hs.bracket_id		= rcb->bracket_id;
	mu->ps_to_hs.ps_to_hs_variant	= REQUEST_TO_SEND;

	send_to_hs(mu);

	return (0);
}

static int sna_wait_for_confirmed_proc(struct sna_tp_verb *tp_verb, struct sna_rcb *rcb)
{
	struct sna_confirmed *confirmed;

	while((confirmed = recv_confirmed()) == NULL)
	{
		record = get_record(1st);
		switch(record->type)
		{
			case (CONVERSATION_FAILURE):
				sna_conversation_failure_proc(record);
				if(fsm_error_or_failure() == CONV_FAILURE_SON)
					tp_verb->rcode = RESOURCE_FAILURE_RETRY;
				else
					tp_verb->rcode = RESOURCE_FAILURE_NO_RETRY;
				fsm_conversation(r, resource_failure_rc, rcb);
				break;

			case (REQUEST_TO_SEND):
				rcb->rq_to_send_rcvd = YES;
				destroy(record);
				break;

			case (RECEIVE_ERROR):
				fsm_error_or_failure(receive_error, rcb);
				while(rcb->hs_to_ps_buffer_list == NULL
					&& fsm_error_or_failure() == RCVD_ERROR)
				{
					sna_receive_rm_or_hs_to_ps_records(suspend_list);
				}

				state = fsm_error_or_failure();
				if(state == CONV_FAILURE_SON
					|| state == CONV_FAILURE_PROTOCOL_ERROR)
				{
					if(state == CONV_FAILURE_SON)
						tp_verb->rcode = RESOURCE_FAILURE_RETRY;
					else
						tp_verb->rcode = RESOURCE_FAILURE_NO_RETRY;
					fsm_conversation(r, resource_failure_rc, rcb);
				}
				else
					sna_dequeue_fmh7_proc(confirm_verb, rcb);
				destroy(record);
				break;

			case (CONFIRMED):
				tp_verb->rcode = OK;
				if(fsm_conversation() == PEND_DEALL)
				{
					fsm_conversation(r, DEALLOCATION_INDICATOR, rcb);
					sna_end_conversation_proc(rcb);
				}
				destroy(record);
				break;

			default:
				sna_ps_protocol_error(rcb->hs_id, 0x10010000);
				fsm_conversation(r, confirmed, rcb);
				break;
		}
	}

	return (0);
}

static int sna_wait_for_rm_reply(struct sna_mu *mu)
{
	mu = NULL

	while(mu == NULL)
	{
		sleep()
		mu = get_record();

		if(mu->type == CONVERSATION_FAILURE)
			sna_conversation_failure_proc(mu);
		else
			return (0);
	}

	return (0);
}

static int sna_wait_for_rsp_to_rq_to_send_proc(struct sna_rcb *rcb, struct sna_hs_to_ps_records *hs_to_ps_records)
{
	struct sna_mu *mu = NULL;

	while(mu == NULL)
	{
		mu = get_record(1st);
		switch(mu->type)
		{
			case (CONVERSATION_FAILURE):
				sna_conversation_failure_proc(mu);
				break;

			case (REQUEST_TO_SEND):
				rcb->rq_to_send_rcvd = YES;
				destroy(mu);
				break;

			case (RECEIVE_ERROR):
				fsm_error_or_failure(receive_error, rcb);
				destroy(mu);
				break;

			case (RSP_TO_REQUEST_TO_SEND):
				destroy(mu);
				break;

			case (MU):
				queue_mu(rcb->hs_to_ps_buffer_list, mu);
				queue_tail(hs_to_ps_buffer_list, mu);
				if(rcb->ec_type == DEALLOCATE_FLUSH)
					break;
				break;

			default:
				sna_ps_protocol_error(rcb->hs_id, fmh7_sense);
				fsm_conversation(s, confirmed, rcb);
				break;
		}
	}

	return (0);
}

static int sna_wait_for_send_err_done_proc(struct sna_tp_verb *tp_verb, struct sna_rcb *rcb)
{
	sna_get_end_chain_from_hs(rcb);
	switch(fsm_error_or_failure()) {
		case (CONV_FAILURE_SON):
			tp_verb->rcode = RESOURCE_FAILURE_RETRY;
			fsm_conversation(s, resource_failure_rc, rcb);
			break;
		case (CONV_FAILURE_PROTOCOL_ERROR):
			tp_verb->rcode = RESOURCE_FAILURE_NO_RETRY;
			fsm_conversation(r, resource_failure_rc, rcb);
		default:
			switch(rcb->ec_type) {
				case (DEALLOCATE_FLUSH):
					if(verb is SEND_ERROR) {
						tp_verb->rcode = DEALLOCATE_NORMAL;
						fsm_conversation(r, deallocate_normal_rc, rcb);
						break;
					}
					if(verb is DEALLOCATE) {
						tp_verb->rcode = OK;
						break;
					}

					break;
				case (DEALLOCATE_CONFIRM):
				case (CONFIRM):
				case (PREPARE_TO_RCV_CONFIRM):
				case (PREPARE_TO_RCV_FLUSH):
					if(verb is SEND_ERROR) {
						purge_ec_type();
						sna_send_error_done_proc(send_error, rcb);
						break;
					}
					if(verb is DEALLOCATE) {
						sna_complete_deallocate_abend_proc(deallocate, rcb);
						break;
					}
					break;
			}
	}
	fsm_error_or_failure(reset);
	return 0;
}
#endif
