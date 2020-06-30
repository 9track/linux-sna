/* sna_ps_main.c: Linux Systems Network Architecture implementation
 * - SNA LU 6.2 Presentation Services (PS)
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
#include <linux/cpic.h>
#include <net/cpic.h>

/* sna_ps_init - initialize anything required for a new transation program.
 * @tp: transaction program control block.
 */
int sna_ps_init(struct sna_tp_cb *tp)
{
	sna_debug(5, "init\n");
	return 0;
}

int sna_ps_process_fmh5(__u32 tcb_id, __u32 rcb_id, struct sk_buff *skb)
{
	sna_debug(5, "sna_ps_process_fmh5\n");
	sna_attach_execute_tp(tcb_id, skb);

#ifdef NOT
	struct sna_tcb *tcb;
	struct sna_rcb *rcb;
	__u8 code;

	code = sense;

	rcb = search_rcb(mu->rcb_id);
	sna_init_attached_rcb(rcb, mu_with_attach);

	move_to_head(rcb->hs_to_ps_buffer_list, mu_with_attach);

	if(code == OK)
	{
		sna_ps_attach_chk(rcb, code);
		if(pip != NULL)
			sna_receive_pip_field_from_hs(rcb, pip, code);
		sna_ps_pip_chk(pip, code);
	}

	if(code == OK)
		sna_upm_execute(tcb->tp_name, rcb->rcb_id, pip);
	else
		sna_attach_error_proc(rcb, code);
#endif

	return (0);
}

#ifdef NOT
static int sna_receive_pip_field_from_hs(struct sna_rcb *rcb, struct sna_pip *pip, __u8 code)
{
	struct sna_receive_and_wait *receive_and_wait;

	new(receive_and_wait, GFP_ATOMIC);
	receive_and_wait->post_conditions.fill		= LL;
	receive_and_wait->post_conditions.max_length	= 0x7FFF;

	sna_receive_and_test_posting(rcb, receive_and_wait);
	err = sna_chk_pip(receive_and_wait->pip);
	if(!err)
		return(receive_and_wait->pip);
	else
		code = 0x1008201D;

	return (0);
}

static int sna_ps_attach_chk(struct sna_attach *attach, __u8 code, struct sna_pip *pip)
{
	struct sna_rcb *rcb;
	struct sna_tcb *tcb;
	struct sna_mu *mu;

	mu = sna_get_buf_from_list(rcb->hs_to_ps_buffer_list);
	mu->tp_name = tcb->tp_name;

	/* Check attach fields for the following */
	if(lu_work_id fields are bad)
		code = 0x10086011;

	if(tp->conversation_type != BASIC || tp->conversation_type != MAPPED)
		code = 0x10086034;

	if(mu has been processed)
	{
		type_field = mu->end_of_chain_type;
		bm(FREE, mu);
	}

	return (0);
}

static int sna_ps_pip_chk(struct sna_pip *pip, __u8 code)
{
	struct sna_tcb *tcb;
	struct sna_tp *tp;

	tp->tp_name = attach->tp_name;	/* Screwed Big time */

	if(code != OK)
		return (-1);

	if(tp->pip_numbers != NULL)
	{
		if(tp->number_of_pip_subfields == 0
			&& attach->pip != NULL)
		{
			code = 0x10086031;
		}
		else
		{
			if(tp->number_of_pip_subfields
				!= attach->number_of_pip_subfields)
			{
				code = 0x10086032;
			}
			else
			{
				err = check_pip_format(pip);
				if(err)
					code = 0x1008201D;
			}
		}
	}

	if(tp->pip == NULL)
	{
		if(pip_data == BAD ;)
			code = 0x1008201D;
	}

	return (0);
}

static int sna_attach_error_proc(struct sna_rcb *rcb, __u8 code/sense)
{
	struct sna_mu *mu;

	switch(code)
	{
		case (0x1008200E):
		case (0x10086000):
		case (0x10086005):
		case (0x10086009):
		case (0x10086011):
		case (0x10086040):
		case (0x1008201D):
			sna_ps_protocol_error(rcb->hs_id, code);
			sna_end_conversation_proc(rcb);
			break;

		default:
			sna_send_error_to_hs_proc(rcb);
			sna_end_chain_from_hs(rcb);
			if(fsm_error_or_failure() == CONV_FAILURE_SON
				|| fsm_error_or_failure == CONV_FAILURE_PROTOCOL_ERROR)
			{
				Log_error();
			}
			else
			{
				switch(end-of-chain type)
				{
					case (DEALLOCATE_FLUSH):
						Log_error();
						break;

					case (DEALLOCATE_CONFIRM):
					case (CONFIRM):
					case (PREPARE_TO_RCV_CONFIRM):
					case (PREPARE_TO_RCV_FLUSH):
						sna_upm_attach_log(code, log_data);
						if(log_data != NULL)
						{
							Log_err();
							attach_fmh-7();
							sna_send_data_bm(log_data, rcb);
						}
					else
						put_fmh7_into_send_mu();

					mu->ps_to_hs.type = DEALLOCATE_FLUSH;
					break;
				}

				sna_end_conversation_proc(rcb);
			}
			break;
	}

	return (0);
}

#endif

/**
 * this procedure receives all verbs issued by the tp and routes them to the
 * appropriate ps component (e.g., basic conversation verbs to ps.conv,
 * and control-operator verbs to ps.copr) for processing.
 *
 * @verb: the current transaction program verb.
 * @tp: transaction program information.
 *
 * the resource parameter of the verb is checked to see if it is valid
 * before proceeding with additional processing. the return code of the
 * transaction_program_verb may be updated to ok or to indicate a detected
 * error. also, the controlling_component is updated to indicate tp or
 * service_component. refer to the ps components that are called from this
 * process for the specific outputs.
 */
int sna_ps_verb_router(int verb, struct sna_tp_cb *tp)
{
	int err = -EINVAL;

	sna_debug(5, "init: %d\n", verb);
	switch (verb) {
		case ALLOCATE:
		case CONFIRM:
		case CONFIRMED:
		case DEALLOCATE:
		case FLUSH:
		case GET_ATTRIBUTES:
		case POST_ON_RECEIPT:
		case PREPARE_TO_RECEIVE:
		case RECEIVE_AND_WAIT:
		case RECEIVE_IMMEDIATE:
		case REQUEST_TO_SEND:
		case SEND_DATA:
		case SEND_ERROR:
		case TEST:
			err = sna_ps_conv(verb, tp);
			break;

#ifdef NOT
		case MC_ALLOCATE:
			sna_ps_mc(verb);
			break;
		case MC_CONFIRM:
		case MC_CONFIRMED:
		case MC_DEALLOCATE:
		case MC_FLUSH:
		case MC_GET_ATTRIBUTES:
		case MC_POST_ON_RECEIPT:
		case MC_PREPARE_TO_RECEIVE:
		case MC_RECEIVE_AND_WAIT:
		case MC_REQUEST_TO_SEND:
		case MC_SEND_DATA:
		case MC_SEND_ERROR:
		case MC_TEST:
			if (verb->resource == tcb->resource_list) {
				rcb = search_rcb(verb->resource.rcb_id);
				if (rcb->conversation_type == MAPPED) {
					tcb->cntrl_component = SERVICE_COMPONENT;
					sna_ps_mc(verb);
					tcb->cntrl_component = TP;
				} else
					verb->rcode = PROGRAM_PARAM_CHECK;
			} else
				verb->rcode = PROGRAM_PARAM_CHECK;
			break;
		case INITIALIZE_SESSION_LIMIT:
		case CHANGE_SESSION_LIMIT:
		case RESET_SESSION_LIMIT:
		case SET_LUCB:
		case SET_PARTNER_LU:
		case SET_MODE:
		case SET_MODE_OPTIONS:
		case SET_TRANSACTION_PROGRAM:
		case SET_PRIVILAEGED_FUNCTION:
		case SET_RESOURCE_SUPPORTED:
		case SET_SYNC_LEVEL_SUPPORTED:
		case SET_MC_FUNCTION_SUPPORTED_TP:
		case GET_LUCB:
		case GET_PARTNER_LU:
		case GET_MODE:
		case GET_LU_OPTION:
		case GET_MODE_OPTION:
		case GET_TRANSACTION_PROGRAM:
		case GET_PRIVILEGED_FUNCTION:
		case GET_RESOURCE_SUPPORTED:
		case GET_SYNC_LEVEL_SUPPORTED:
		case GET_MC_FUNCTION_SUPPORTED_LU:
		case GET_MC_FUNCTION_SUPPORTED_TP:
		case LIST_PARTNER_LU:
		case LIST_MODE:
		case LIST_LU_OPTION:
		case LIST_MODE_OPTION:
		case LIST_TRANSACTION_PROGRAM:
		case LIST_PRIVILEGED_FUNCTION:
		case LIST_RESOURCE_SUPPORTED:
		case LIST_SYNC_LEVEL_SUPPORTED:
		case LIST_MC_FUNCTION_SUPPORTED_LU:
		case LIST_MC_FUNCTION_SUPPORTED_TP:
		case PROCESS_SESSION_LIMIT:
		case ACTIVATE_SESSION:
		case DEACTIVATE_CONVERSATION_GROUP:
		case DEACTIVATE_SESSION:
			tcb->cntrl_component = SERVICE_COMPONENT;
			sna_ps_copr(verb);
			tcb->cntrl_component = TP;
			break;
		case SYNCPT:
		case BACKOUT:
			tcb->cntrl_component = SERVICE_COMPONENT;
			sna_ps_sync(verb);
			tcb->cntrl_component = TP;
			break;
		case GET_TP_PROPERTIES:
			sna_get_tp_properties_proc(verb);
			break;
		case GET_TYPE:
			verb->rcode = OK;
			if(verb->resource == conversation_this_tp)
			{
				rcb = search_rcb(verb->resource.rcb_id);
				verb->type = rcb->conversation_type;
			}
			else
				verb->rcode = PROGRAM_PARAM_CHECK;
			break;
		case WAIT:
			tcb->cntrl_component = SERVICE_COMPONENT;
			sna_wait_proc(verb);
			tcb->cntrl_component = TP;
			break;
#endif
	}
	return err;
}

#ifdef NOT
static int sna_deallocation_cleanup_proc(struct sna_rcb *rcb)
{
	struct sna_terminate_ps *terminate_ps;
	struct sna_tcb *tcb;

	for(tcb = tcb_list; tcb != NULL; tcb = tcb->next)
	{
		if(tcb->rcb_id == rcb->rb_id)
			sna_upm_return_processing(rcb);
	}

	send_to_rm(terminate_ps);

	return (0);
}

static int sna_get_tp_properties_proc(struct sna_get_tp_properties *get_tp_properties)
{
	struct sna_lucb *lucb;
	struct sna_tcb *tcb;

	get_tp_properties->own_tp_name		= tcb->tp_name;
	get_tp_properties->own_tp_instance	= tcb->tcb_id;
	get_tp_properties->own_fq_lu_name	= lucb->fq_lu_name;
	get_tp_properties->security_profile	= tcb->init_security.profile;
	get_tp_properties->security_user_id	= tcb->init_security.user_id;
	get_tp_properties->rcode		= OK;

	return (0);
}

static int sna_wait_proc(struct sna_wait *wait, unsigned char *data)
{
	struct sna_tcb *tcb;
	struct sna_rcb *rcb;

	err = sna_check_resource_list();
	if(err)
	{
		wait->rcode = PROGRAM_PARAM_CHECK;
		return (-1);
	}

	if(no_activated_resource)
	{
		wait->rcode = POSTING_NOT_ACTIVE;
		return (-1);
	}

	for(each_resource_posting_active)
	{
		sna_test_for_resource_posted(rcb, rc);
		if(rc != UNSUCCESSFULL)
		{
			wait->rcode = rc;
			return (-1);
		}

	/* Spin until resource becomes posted */
	rc = UNSUCCESSFULL;
	while(rc == UNSUCCESSFULL)
	{
		sna_receive_rm_or_hs_to_ps_records(temp_resource_list);
		rcb = rcb_data_active();
		sna_test_for_resource_posted(rcb, rc);
	}

	resource_posted = rcb->rcb_id;
	wait->rcode = rc;

	return (0);
}

static int sna_ps_protocol_err(__u8 hs_id, __u8 tcb_id, __u8 sense)
{
	struct sna_unbind_protocol_error *unbind_proto_err;

	unbind_proto_err->hs_id 	= hs_id;
	unbind_proto_err->tcb_id	= tcb_id;
	unbind_proto_err->sense		= sense;

	send_to_rm(unbind_proto_err);

	return (0);
}

static int sna_init_attached_rcb(struct sna_rcb *rcb, attach)
{
	rcb->conversation_type 	= attach->conversation_type;
	rcb->limit_buf_pool_id 	= attach->limit_buf_pool_id;
	rcb->perm_buf_pool_id	= attach->perm_buf_pool_id;
	rcb->send_ru_size	= attach->send_ru_size;
	rcb->post_conditions.fill	= LL;
	rcb->post_conditions.max_length	= 0;
	rcb->locks		= SHORT;
	rcb->rq_to_send_rcvd	= NO;

	sna_empty_list(rcb->hs_to_ps_buffer_list);

	fsm_conversation	= RCV;
	fsm_error_or_failure	= NO_REQUESTS;
	fsm_post		= RESET;

	if(rcb->conversation_type == MAPPED_CONVERSATION)
	{
		sna_empty_list(rcb->mc_receive_buffer);
		rcb->mc_rq_to_send_rcvd	= NO;
		rcb->mapper_save_area	= /* I decide, Hah! */
		rcb->mc_max_send_size	= /* I decide this too!! Wee */
	}

	return (0);
}

static int sna_test_for_resource_posted(struct sna_rcb *rcb)
{
	struct sna_test *test;
	struct sna_mc_test *mc_test;

	switch(rcb->conversation_type)
	{
		case (BASIC):
			new(test, GFP_ATOMIC);
			test->resource 	= rcb->rcb_id;
			test->test	= POSTED;
			err = sna_test_proc(test);
			break;

		case (MAPPED):
			new(mc_test, GFP_ATOMIC);
			mc_test->resource	= rcb->rcb_id;
			mc_test->test		= POSTED;
			err = sna_mc_test_proc(mc_test);
			break;

		default:
			/* Error */
	}

	return (err);
}
#endif
