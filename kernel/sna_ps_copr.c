/* sna_ps_copr.c: Linux Systems Network Architecture implementation
 * - SNA LU 6.2 Presentation Service Control Operator (PS.CORP)
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

#include "sna_common.h"

int sna_ps_copr(int verb, struct sna_tcb *tcb)
{
#ifdef NOT
	switch(cnos_verb->type)
	{
		case (INITIALIZE_SESSION_LIMIT):
			sna_init_session_limit_proc(cnos_verb);
			break;

		case (CHANGE_SESSION_LIMIT):
			sna_change_sesion_limit_proc(cnos_verb);
			break;

		case (RESET_SESSION_LIMIT):
			sna_reset_session_limit_proc(cnos_verb);
			break;

		case (PROCESS_SESSION_LIMIT):
			sna_process_session_limit_proc(cnos_verb);
			break;

		case (DEACTIVATE_CONVERSATION_GROUP):
			sna_deact_conversation_group_proc(cnos_verb);
			break;

		case (DEACTIVATE_SESSION):
			sna_deactivate_session_proc(cnos_verb);
			break;

		case (ACTIVATE_SESSION):
			sna_activate_session_proc(cnos_verb);
			break;

		case (DEFINE_LOCAL_LU):
		case (DEFINE_REMOTE_LU):
		case (DEFINE_MODE):
		case (DEFINE_TP):
			sna_define_proc(cnos_verb);
			break;

		case (DELETE):
			sna_delete_proc(cnos_verb);
			break;

		default:
			/* Error */
	}
#endif

	return (0);
}

#ifdef NOT

static int sna_init_session_limit_proc(struct sna_init_session_limit_verb *verb, __u8 rcode)
{
	struct sna_lucb *lucb;

	lucb = search_lucb();

	if(lucb->tp == AUTH_OK)
	{
		type = lucb->parallel;

		if(lucb->lu != plu->lu)
			rcode = PARAMETER_ERROR;
		else
		{
			if(type == PARALLEL
				&& (lucb->mode_name != SNASVCMG
				|| lucb->mode_name != CPSVCMG))
			{
				sna_source_session_limit_proc(verb);
			}
			else
				sna_local_session_limit_proc(verb);
		}
	}
	else
		rcode = PROGRAM_PARAMETER_CHECK;

	return (0);
}

static int sna_reset_session_limit_proc(verb, __u8 rcode)
{
	struct sna_lucb *lucb;

	lucb = search_lucb();

	if(lucb->tp == AUTH_OK)
	{
		type = lucb->parallel;

		if(lucb->lu != plu)
			rcode = PARAMETER_CHECK;
		else
		{
			if(type == PARALLEL
				&& (lucb->mode_name != SNASVCMG
				|| lucb->mode_name != CPSVCMG)
			{
				sna_source_session_limit_proc(verb);
				if(verb->force == YES)
				{
					if(rcode == ALLOCATION_ERROR
						|| rcode == LU_MODE_SESSION_LIMIT
						|| rcode == RESOURCE_FAILURE_NO_RETRY
						|| rcode == UNRECOGNIZED_MODE_NAME)
					{
						sna_change_action(verb);
						rcode = OK_FORCED;
					}
				}
			}
			else
				sna_local_session_limit_proc(verb);
		}
	}
	else
		rcode = PROGRAM_PARAMETER_CHECK;

	return (0);
}

static int sna_change_session_limit_proc(verb, rcode)
{
	struct sna_lucb *lucb;

	lucb = search_lucb();

	if(lucb->tp != AUTH_OK)
		rcode = PROGRAM_PARAMETER_CHECK;
	else
	{
		type = lucb->parallel;

		if(lucb->lu != plu)
			rcode = PARAMETER_ERROR;
		else
		{
			if(type == INTERLU_PARALLEL
				&& lucb->mode_name != SNASVCMG)
			{
				sna_source_session_limit_proc(verb);
			}
			else
			{
				if(type == INTRALU_PARALLEL
					&& (lucb->mode_name != SNASVCMG
					|| lucb->mode_name != CPSVCMG)
				{
					sna_local_session_limit_proc(verb);
				}
				else
					rcode = PROGRAM_PARAM_CHECK;
			}
		}
	}

	return (0);
}

static int sna_activate_session_proc(verb, rcode)
{
	err = check_verb(activate_session);
	switch(err)
	{
		case (NO_AUTH):
			rcode = PROGRAM_PARAMETER_CHECK;
			break;

		case (ERROR_FOUND):
			rcode = err;
			break;

		case (OK):
			new(activate, GFP_ATOMIC);
			activate->tcb_id = ps_process_data->tcb_id;
			activate->lu_name = verb->lu_name;
			activate->mode_name = verb->mode_name;
			send_to_rm(activate);

			activated = recv_from_rm();
			rcode = activated->rcode;
			if(single sess && conwinner)
				sec_code = OK.AS_SPECIFIED;
			else
				sec_code = OK.AS_NEGOTIATED;
		}
	}

	destroy(activated);

	return (0);
}

static int deact_conversation_group_proc(verb)
{
	err = check_verb(verb);
	if(tp == AUTH_OK)
	{
		rcode = OK;
		new(deact_group, GFP_ATOMIC);
		deact_group->conv_group_id = verb->conv_group_id;
		deact_group->type = verb->type;
		if(deact_group->type == CLEANUP)
		{
			if(verb->sense != NULL)
				deact_group->sense = verb->sense
			else
				deact_group->sense = 0x08A00002;
		}
		else
			deact_group->sense = 0x00000000;
		send_to_rm(deact_group);
	}
	else
		rcode = PROGRAM_PARAM_CHECK;

	return (0);
}

static int deactivate_session_proc(verb)
{
	err = check_verb(verb);
	if(tp == AUTH_OK)
	{
		rcode = OK;
		new(deactivate, GFP_ATOMIC);
		deactivate->tcb_id = ps_process_data->tcb_id;
		deactivate->session_id = verb->session_id;
		deactivate->type = verb->type;
		if(deactivate->type == CLEANUP)
		{
			if(verb->sense != NULL)
				deactivate->sense = verb->sense;
			else
				deactivate->sense = 0x08A00002;
		}
		else
			deactivate->sense = 0x00000000;
		send_to_rm(deactivate);
	}
	else
		rcode = PROGRAM_PARAM_CHECK;

	return (0);
}

static int sna_define_proc(verb)
{
	struct sna_lucb *lucb;
	struct sna_partner_lu *plu;
	struct sna_mode *mode;
	struct sna_tp *tp;

	err = check_verb(verb);
	if(err == ABEND)
		rcode = PROGRAM_PARAM_CHECK;
	else
		assign values to data_structs..?

	return (0);
}

static int sna_display_proc(verb)
{
	struct sna_lucb *lucb;
	struct sna_partner_lu *plu;
	struct sna_mode *mode;
	struct sna_tp *tp;

	err = check_verb(verb);
	if(err == ABEND)
		rcode = PROGRAM_PARAM_CHECK;
	else
		/* Display request data, just copy to user space for us */

	return (0);
}

static int sna_delete_proc(verb)
{
	struct sna_lucb *lucb;
	struct sna_partner_lu *plu;
	struct sna_mode *mode;
	struct sna_tp *tp;

	err = check_verb(verb);
	if(err == ABEND)
		rcode = PROGRAM_PARAM_CHECK;
	else
		/* Delete attribs in delete verb */

	return (0);
}

static int sna_local_session_limit_proc(verb)
{

	switch(session_type)
	{
		case (SINGLE):
			err = sna_local_verb_parameter_check(verb);
			break;

		case (PARALLEL):
			if(mode_name == SNASVCMG || mode_name == CPSVCMG)
				err = sna_svcmg_verb_param_check(verb);
			break;

		case (INTRA_PARALLEL):
			if(mode_name != SNASVCMG || mode_name != CPSVCMG)
				err = sna_intra_lu_local_verb_param_check(verb);
			break;

	}

	if(err = OK)
		sna_change_action(verb);

	return (0);
}

static int sna_local_verb_param_check(verb, struct sna_parnter_lu *plu_list, struct sna_mode *mode_list)
{
	struct sna_lucb *lucb;
	struct sna_mode *mode;

	err = check_verb(verb);

	/* Swich is bad here */
	switch(err)
	{
		case (OK):
			rcode = OK.AS_SPECIFIED;
			break;

		case (ABEND):
			rcode = PROGRAM_PARAM_CHECK;
			break;

		case (ERROR):
			rcode = PARAM_ERROR;
			break;

		case (MODE):
			rcode = LU_MODE_SESSION_LIMIT_NOT_ZERO;
			break;

		case (LIMIT_EXCEEDED):
			rcode = LU_SESSION_LIMIT_EXCEEDED;
			break;

		if(mode->local_max_session_limit != NULL)
		{
			case (MAX_ALLOWED):
				rcode = REQUEST_EXCEEDS_MAX_ALLOWED;
				break;
		}
	}

	return (0);
}

static int sna_intra_lu_local_verb_parm_check(verb, struct sna_partner_lu *plu_list, struct sna_mode *mode_list)
{
	struct sna_lucb *lucb;
	struct sna_mode *mode;
	struct sna_partner_lu *plu;

	err = check_verb(verb);

	rcode = OK.AS_NEGOTIATED;
	switch(err)
	{
		case (ABEND):
			rcode = PROGRAM_PARAM_CHECK;
			break;

		case (ERROR):
			rcode = PARAM_ERROR;
			break;

		case (LIMIT_NOT_ZERO):
			rcode = LU_MODE_SESSION_LIMIT_NOT_ZERO;
			break;

		case (LIMIT_ZERO):
			rcode = LU_MODE_SESSION_LIMIT_ZERO;
			break;

		case LIMIT_EXCEEDED):
			rcode = LU_SESSION_LIMIT_EXCEEDED;
			break;

		case (MAX_ALLOWED):
			rcode = REQUEST_EXCEEDS_MAX_ALLOWED;
			break;
	}

	return (0);
}

static int sna_svcmg_verb_param_check(verb, struct sna_partner_lu *plu_list, struct sna_mode *mode_list)
{
	struct sna_lucb *lucb;
	struct sna_mode *mode;

	err = check_verb(verb);
	switch(err)
	{
		case (ABEND):
			rcode = PROGRAM_PARAM_CHECK;
			break;

		case (ERROR):
			rcode = PARAM_ERROR;
			break;

		case (LIMIT_NOT_ZERO):
			rcode = LU_MODE_SESSION_LIMIT_NOT_ZERO;
			break;

		case (LIMIT_EXCEEDED):
			rcode = LU_SESSION_LIMIT_EXCEEDED;
			break;
	}

	return (0);
}

struct sna_change_action(verb, struct sna_partner_lu *plu_list, struct sna_mode *mode_list)
{

	return (0);
}

#endif
