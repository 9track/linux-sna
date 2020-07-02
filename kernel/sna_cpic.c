/* sna_cpic.c: Linux Systems Network Architecture implementation
 * - SNA CPI Communications (CPI-C) Pure processing backend.
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
#include <linux/list.h>
#include <net/sock.h>

#include <linux/sna.h>
#include <linux/appc.h>
#include <linux/cpic.h>

#include "sna_common.h"

#define MAX_CPIC_ADDR	1024

static LIST_HEAD(cpic_list);
static LIST_HEAD(cpic_side_list);
static u_int32_t sna_cpic_side_system_index = 0;

struct cpic {
	struct list_head		list;

	u_int32_t              		state;
	u_int32_t			flags;

	struct cpic_ops         	*ops;
	struct sna_cpic_side_info   	*side;
	struct inode            	*inode;
	struct fasync_struct    	*fasync_list;
	struct file             	*file;
	pid_t                   	pid;

	wait_queue_head_t       	wait;

	union {
		struct sna_tp_cb        *sna;
	} vi;
};

struct cpic_sock {
	struct sock sk;
	struct cpic cpic;
};

#define cpic_pi(sk) ((struct cpic_sock *) sk)

static u_int32_t sna_cpic_appc2cpic_rc(u_int32_t err)
{
	sna_debug(5, "init\n");
	if (err == AC_RC_OK)
		return CM_OK;
	if (err == AC_RC_UNSUCCESSFUL)
		return CM_UNSUCCESSFUL;
	if (err == AC_RC_INVALID_VERB)
		return CM_CALL_NOT_SUPPORTED;
	if (err == AC_RC_PROGRAM_PARAMETER_CHECK)
		return CM_PROGRAM_PARAMETER_CHECK;
	if (err == AC_RC_PROGRAM_STATE_CHECK)
		return CM_PROGRAM_STATE_CHECK;
	if (err == AC_RC_RESOURCE_FAILURE_RETRY)
		return CM_RESOURCE_FAILURE_RETRY;
	if (err == AC_RC_RESOURCE_FAILURE_NO_RETRY)
		return CM_RESOURCE_FAILURE_NO_RETRY;
	if (err == AC_RC_ACTIVATION_FAILURE_RETRY)
		return CM_ALLOCATE_FAILURE_RETRY;
	if (err == AC_RC_ACTIVATION_FAILIRE_NO_RETRY)
		return CM_ALLOCATE_FAILURE_NO_RETRY;
	if (err == AC_RC_PROG_ERROR_NO_TRUNC)
		return CM_PROGRAM_ERROR_NO_TRUNC;
	if (err == AC_RC_PROG_ERROR_PURGING)
		return CM_PROGRAM_ERROR_PURGING;
	if (err == AC_RC_SVC_ERROR_NO_TRUNC)
		return CM_SVC_ERROR_NO_TRUNC;
	if (err == AC_RC_SVC_ERROR_TRUNC)
		return CM_SVC_ERROR_TRUNC;
	if (err == AC_RC_SVC_ERROR_PURGING)
		return CM_SVC_ERROR_PURGING;
	if (err == AC_RC_DEALLOCATE_NORMAL)
		return CM_DEALLOCATED_NORMAL;
	if (err == AC_RC_DEALLOCATE_ABEND)
		return CM_DEALLOCATED_ABEND;
	if (err == AC_RC_DEALLOCATE_ABEND_PROG)
		return CM_DEALLOCATED_ABEND;
	if (err == AC_RC_DEALLOCATE_ABEND_SVC)
		return CM_DEALLOCATED_ABEND_SVC;
	if (err == AC_RC_DEALLOCATE_ABEND_TIMER)
		return CM_DEALLOCATED_ABEND_TIMER;
	if (err == AC_RC_PIP_NOT_ALLOWED)
		return CM_PIP_NOT_SPECIFIED_CORRECTLY;
	if (err == AC_RC_PIP_NOT_SPECIFIED_CORRECTLY)
		return CM_PIP_NOT_SPECIFIED_CORRECTLY;
	if (err == AC_RC_CONVERSATION_TYPE_MISMATCH)
		return CM_CONVERSATION_TYPE_MISMATCH;
	if (err == AC_RC_SYNC_LEVEL_NOT_SUPPORTED)
		return CM_SYNC_LVL_NOT_SUPPORTED_LU;
	if (err == AC_RC_SECURITY_NOT_VALID)
		return CM_SECURITY_NOT_VALID;
	if (err == AC_RC_TP_NAME_NOT_RECOGNIZED)
		return CM_TPN_NOT_RECOGNIZED;
	if (err == AC_RC_TP_NOT_AVAIL_RETRY)
		return CM_TP_NOT_AVAILABLE_RETRY;
	if (err == AC_RC_TP_NOT_AVAIL_NO_RETRY)
		return CM_TP_NOT_AVAILABLE_NO_RETRY;
	if (err == AC_RC_PRODUCT_SPECIFIC)
		return CM_PRODUCT_SPECIFIC_ERROR;
	if (err == AC_RC_ALLOCATION_FAILURE_NO_RETRY)
		return CM_ALLOCATE_FAILURE_NO_RETRY;
	if (err == AC_RC_ALLOCATION_FAILURE_RETRY)
		return CM_ALLOCATE_FAILURE_RETRY;

	sna_debug(5, "unable to translate appc error `%d'.\n", err);
	return err;
}

static struct sna_cpic_side_info *sna_cpic_side_info_get_by_index(u_int32_t index)
{
	struct sna_cpic_side_info *cpic;
	struct list_head *le;

	sna_debug(5, "init\n");
	list_for_each(le, &cpic_side_list) {
		cpic = list_entry(le, struct sna_cpic_side_info, list);
		if (cpic->index == index)
			return cpic;
	}
	return NULL;
}

static u_int32_t sna_cpic_side_info_new_index(void)
{
	for (;;) {
		if (++sna_cpic_side_system_index <= 0)
			sna_cpic_side_system_index = 1;
		if (sna_cpic_side_info_get_by_index(sna_cpic_side_system_index) == NULL)
			return sna_cpic_side_system_index;
	}
	return 0;
}

static struct sna_cpic_side_info *sna_cpic_side_info_get_by_name(unsigned char *name)
{
	struct sna_cpic_side_info *cpic;
	struct list_head *le;

	sna_debug(5, "init\n");
	list_for_each(le, &cpic_side_list) {
		cpic = list_entry(le, struct sna_cpic_side_info, list);
		sna_debug(5, "-%s- -%s-\n", name, cpic->sym_dest_name);
		if (!strncmp(cpic->sym_dest_name, name, 8))
			return cpic;
	}
	return NULL;
}

static int sna_cpic_unregister_side_info(struct sna_nof_cpic *side)
{
	struct sna_cpic_side_info *cpic;

	sna_debug(5, "init\n");
	cpic = sna_cpic_side_info_get_by_name(side->sym_dest_name);
	if (cpic) {
		list_del(&cpic->list);
		kfree(cpic);
		return 0;
	}
	return -ENOENT;
}

static int sna_cpic_register_side_info(struct sna_nof_cpic *side)
{
	struct sna_cpic_side_info *cpic;
	int i;

	sna_debug(5, "init\n");
	cpic = sna_cpic_side_info_get_by_name(side->sym_dest_name);
	if (cpic)
		return -EEXIST;
	new(cpic, GFP_ATOMIC);
	if (!cpic)
		return -ENOMEM;
	cpic->index = sna_cpic_side_info_new_index();

	memcpy(&cpic->netid, &side->netid, sizeof(sna_netid));
	memcpy(&cpic->netid_plu, &side->netid_plu, sizeof(sna_netid));

	strncpy(cpic->sym_dest_name, side->sym_dest_name, SNA_RESOURCE_NAME_LEN);
	for (i = strlen(cpic->sym_dest_name); i < 8; i++)
		cpic->sym_dest_name[i] = 0x20;

	strncpy(cpic->mode_name, side->mode_name, SNA_RESOURCE_NAME_LEN);
	cpic->mode_name_length		= strlen(cpic->mode_name);

	strncpy(cpic->tp_name, side->tp_name, 65);
	cpic->tp_name_length		= strlen(cpic->tp_name);

	cpic->service_tp          = side->service_tp;
	cpic->security_level      = side->security_level;
	strncpy(cpic->username, side->username, 11);
	strncpy(cpic->password, side->password, 11);

	list_add_tail(&cpic->list, &cpic_side_list);
	return 0;
}

static int sna_cpic_error(void *uaddr, u_int32_t return_code)
{
	return sna_ktou(&return_code, sizeof(return_code), uaddr);
}

static int sna_cpic_fsm_cc_a(struct cpic *cpic, int call)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	int err = 0;

	sna_debug(5, "init\n");
	if (tp->deallocate_type == AC_DEALLOCATE_ABEND)
		err = 1;
	return err;
}

static int sna_cpic_fsm_cc_b(struct cpic *cpic, int call)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	int err = 0;

	sna_debug(5, "init\n");
	if (tp->send_type == AC_BUFFER_DATA)
		err = 1;
	return err;
}

static int sna_cpic_fsm_cc_c(struct cpic *cpic, int call)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	int err = 0;

	sna_debug(5, "init\n");
	switch (call) {
		case CM_CMDEAL:
			if (tp->deallocate_type == AC_DEALLOCATE_CONFIRM) {
				err = 1;
				break;
			}
			if (tp->deallocate_type == AC_DEALLOCATE_SYNC_LEVEL
				&& tp->sync_level == AC_SYNC_LEVEL_CONFIRM) {
				err = 1;
				break;
			}
			if (tp->deallocate_type == AC_DEALLOCATE_SYNC_LEVEL
				&& tp->sync_level == AC_SYNC_LEVEL_SYNCPT) {
				sna_debug(5, "FIXME: conversation can't be in transaction.\n");
				err = 1;
				break;
			}
			break;
		case CM_CMPTR:
			if (tp->prepare_to_receive_type == AC_PREP_TO_RECEIVE_CONFIRM) {
				err = 1;
				break;
			}
			if (tp->prepare_to_receive_type == AC_PREP_TO_RECEIVE_SYNC_LEVEL
				&& tp->sync_level == AC_SYNC_LEVEL_CONFIRM) {
				err = 1;
				break;
			}
			if (tp->prepare_to_receive_type == AC_PREP_TO_RECEIVE_SYNC_LEVEL
				&& tp->sync_level == AC_SYNC_LEVEL_SYNCPT) {
				sna_debug(5, "FIXME: conversation can't be in transaction.\n");
				err = 1;
				break;
			}
			break;
		case CM_CMSEND:
			if (tp->send_type == AC_SEND_AND_CONFIRM) {
				err = 1;
				break;
			}
			break;
	}
	return err;
}

static int sna_cpic_fsm_cc_d(struct cpic *cpic, int call)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	int err = 0;

	sna_debug(5, "init\n");
	if (tp->send_type == AC_SEND_AND_DEALLOCATE)
		err = 1;
	return err;
}

static int sna_cpic_fsm_cc_f(struct cpic *cpic, int call)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	int err = 0;

	sna_debug(5, "init\n");
	switch (call) {
		case CM_CMDEAL:
			if (tp->deallocate_type == AC_DEALLOCATE_FLUSH) {
				err = 1;
				break;
			}
			if (tp->deallocate_type == AC_DEALLOCATE_SYNC_LEVEL
				&& (tp->sync_level == AC_SYNC_LEVEL_NONE
				|| cpic->state == CM_INITIALIZE_INCOMING_STATE)) {
				err = 1;
				break;
			}
			if (tp->deallocate_type == AC_DEALLOCATE_SYNC_LEVEL
				&& tp->sync_level == AC_SYNC_LEVEL_NO_CONFIRM) {
				sna_debug(5, "FIXME: conversation can't be in transaction.\n");
				err = 1;
				break;
			}
			break;
		case CM_CMPTR:
			if (tp->prepare_to_receive_type == AC_PREP_TO_RECEIVE_FLUSH) {
				err = 1;
				break;
			}
			if (tp->prepare_to_receive_type == AC_PREP_TO_RECEIVE_SYNC_LEVEL
				&& tp->sync_level == AC_SYNC_LEVEL_NONE) {
				err = 1;
				break;
			}
			if (tp->prepare_to_receive_type == AC_PREP_TO_RECEIVE_SYNC_LEVEL
				&& tp->sync_level == AC_SYNC_LEVEL_NO_CONFIRM) {
				sna_debug(5, "FIXME: conversation can't be in transaction.\n");
				err = 1;
				break;
			}
			break;
		case CM_CMSEND:
			if (tp->send_type == AC_SEND_AND_FLUSH) {
				err = 1;
				break;
			}
			break;
	}
	return err;
}

#ifdef SNA_UNUSED
static int sna_cpic_fsm_cc_i(struct cpic *cpic, int call)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	int err = 0;

	sna_debug(5, "init\n");
	if (tp->receive_type == AC_RECEIVE_IMMEDIATE)
		err = 1;
	return err;
}
#endif

static int sna_cpic_fsm_cc_p(struct cpic *cpic, int call)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	int err = 0;

	sna_debug(5, "init\n");
	if (tp->send_type == AC_SEND_AND_PREP_TO_RECEIVE)
		err = 1;
	return err;
}

static int sna_cpic_fsm_cc_s(struct cpic *cpic, int call)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	int err = 0;

	sna_debug(5, "init\n");
	switch (call) {
		case CM_CMDEAL:
			if (tp->deallocate_type == AC_DEALLOCATE_SYNC_LEVEL
				&& (tp->sync_level == AC_SYNC_LEVEL_SYNCPT
				|| tp->sync_level == AC_SYNC_LEVEL_NO_CONFIRM)) {
				sna_debug(5, "FIXME: conversation is in transaction\n");
				err = 1;
				break;
			}
			break;
		case CM_CMPTR:
			if (tp->prepare_to_receive_type == AC_PREP_TO_RECEIVE_SYNC_LEVEL
				&& (tp->sync_level == AC_SYNC_LEVEL_SYNCPT
				|| tp->sync_level == AC_SYNC_LEVEL_NO_CONFIRM)) {
				sna_debug(5, "FIXME: conversation is in transaction\n");
				err = 1;
				break;
			}
			break;
	}
	return err;
}

#ifdef SNA_UNUSED
static int sna_cpic_fsm_cc_w(struct cpic *cpic, int call)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	int err = 0;

	sna_debug(5, "init\n");
	if (tp->receive_type == AC_RECEIVE_AND_WAIT)
		err = 1;
	return err;
}
#endif

static int sna_cpic_fsm_rc_ae(int rc, int allocate)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (rc) {
		case CM_ALLOCATE_FAILURE_NO_RETRY:
		case CM_ALLOCATE_FAILURE_RETRY:
		case CM_RESOURCE_FAILURE_NO_RETRY:
		case CM_RESOURCE_FAILURE_RETRY:
		case CM_RETRY_LIMIT_EXCEEDED:
		case CM_SECURITY_MUTUAL_FAILED:
		case CM_SECURITY_NOT_SUPPORTED:
			if (allocate)
				err = 1;
			break;

		case CM_CONVERSATION_TYPE_MISMATCH:
		case CM_PIP_NOT_SPECIFIED_CORRECTLY:
		case CM_SECURITY_NOT_VALID:
		case CM_SEND_RCV_MODE_NOT_SUPPORTED:
		case CM_SYNC_LVL_NOT_SUPPORTED_PGM:
		case CM_SYNC_LVL_NOT_SUPPORTED_SYS:
		case CM_TP_NOT_AVAILABLE_NO_RETRY:
		case CM_TP_NOT_AVAILABLE_RETRY:
		case CM_TPN_NOT_RECOGNIZED:
			err = 1;
			break;
	}
	return err;
}

static int sna_cpic_fsm_rc_bo(int rc)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (rc == CM_TAKE_BACKOUT)
		err = 1;
	return err;
}

#ifdef SNA_UNUSED
static int sna_cpic_fsm_rc_bs(int rc)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (rc == CM_BUFFER_TOO_SMALL)
		err = 1;
	return err;
}
#endif

static int sna_cpic_fsm_rc_da(int rc)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (rc) {
		case CM_DEALLOCATED_ABEND:
		case CM_DEALLOCATED_ABEND_SVC:
		case CM_DEALLOCATED_ABEND_TIMER:
			err = 1;
			break;
	}
	return err;
}

static int sna_cpic_fsm_rc_db(int rc)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (rc) {
		case CM_DEALLOCATED_ABEND_BO:
		case CM_DEALLOCATED_ABEND_SVC_BO:
		case CM_DEALLOCATED_ABEND_TIMER_BO:
			err = 1;
			break;
	}
	return err;
}

static int sna_cpic_fsm_rc_dn(int rc)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (rc == CM_DEALLOCATED_NORMAL)
		err = 1;
	return err;
}

#ifdef SNA_UNUSED
static int sna_cpic_fsm_rc_dnb(int rc)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (rc == CM_DEALLOCATED_NORMAL_BO)
		err = 1;
	return err;
}
#endif

#ifdef SNA_UNUSED
static int sna_cpic_fsm_rc_ed(int rc)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (rc) {
		case CM_EXP_DATA_NOT_SUPPORTED:
		case CM_CONVERSATION_ENDING:
			err = 1;
			break;
	}
	return err;
}
#endif

static int sna_cpic_fsm_rc_en(int rc)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (rc) {
		case CM_PROGRAM_ERROR_NO_TRUNC:
		case CM_SVC_ERROR_NO_TRUNC:
			err = 1;
			break;
	}
	return err;
}

static int sna_cpic_fsm_rc_ep(int rc)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (rc) {
		case CM_PROGRAM_ERROR_PURGING:
		case CM_SVC_ERROR_PURGING:
			err = 1;
			break;
	}
	return err;
}

static int sna_cpic_fsm_rc_et(int rc)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (rc) {
		case CM_PROGRAM_ERROR_TRUNC:
		case CM_SVC_ERROR_TRUNC:
			err = 1;
			break;
	}
	return err;
}

#ifdef SNA_UNUSED
static int sna_cpic_fsm_rc_mp(int rc)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (rc) {
		case CM_UNKNOWN_MAP_NAME_REQUESTED:
		case CM_UNKNOWN_MAP_NAME_RECEIVED:
		case CM_UMAP_ROUTINE_ERROR:
			err = 1;
			break;
	}
	return err;
}
#endif

#ifdef SNA_UNUSED
static int sna_cpic_fsm_rc_ns(int rc)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (rc == CM_NO_SECONDARY_INFORMATION)
		err = 1;
	return err;
}
#endif

static int sna_cpic_fsm_rc_oi(int rc)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (rc == CM_OPERATION_INCOMPLETE)
		err = 1;
	return err;
}

static int sna_cpic_fsm_rc_ok(int rc)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (rc == CM_OK)
		err = 1;
	return err;
}

static int sna_cpic_fsm_rc_pb(int rc)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (rc == CM_INCLUDE_PARTNER_REJECT_BO)
		err = 1;
	return err;
}

static int sna_cpic_fsm_rc_pc(int rc)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (rc == CM_PROGRAM_PARAMETER_CHECK)
		err = 1;
	return err;
}

#ifdef SNA_UNUSED
static int sna_cpic_fsm_rc_pe(int rc)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (rc == CM_PARAMETER_ERROR)
		err = 1;
	return err;
}
#endif

static int sna_cpic_fsm_rc_pn(int rc)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (rc == CM_PARM_VALUE_NOT_SUPPORTED)
		err = 1;
	return err;
}

static int sna_cpic_fsm_rc_rb(int rc)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (rc) {
		case CM_RESOURCE_FAIL_NO_RETRY_BO:
		case CM_RESOURCE_FAILURE_RETRY_BO:
			err = 1;
			break;
	}
	return err;
}

static int sna_cpic_fsm_rc_rf(int rc)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (rc) {
		case CM_RESOURCE_FAILURE_NO_RETRY:
		case CM_RESOURCE_FAILURE_RETRY:
			err = 1;
			break;
	}
	return err;
}

static int sna_cpic_fsm_rc_sc(int rc)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (rc == CM_PROGRAM_STATE_CHECK)
		err = 1;
	return err;
}

#ifdef SNA_UNUSED
static int sna_cpic_fsm_rc_se(int rc)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (rc == CM_SYSTEM_EVENT)
		err = 1;
	return err;
}
#endif

static int sna_cpic_fsm_rc_un(int rc)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (rc == CM_UNSUCCESSFUL)
		err = 1;
	return err;
}

static int sna_cpic_fsm_dr_dr(int dr)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (dr) {
		case CM_DATA_RECEIVED:
		case CM_COMPLETE_DATA_RECEIVED:
		case CM_INCOMPLETE_DATA_RECEIVED:
			err = 1;
			break;
	}
	return err;
}

static int sna_cpic_fsm_dr_nd(int dr)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (dr == CM_NO_DATA_RECEIVED)
		err = 1;
	return err;
}

static int sna_cpic_fsm_dr_any(int dr)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (dr) {
		case CM_DATA_RECEIVED:
		case CM_COMPLETE_DATA_RECEIVED:
		case CM_NO_DATA_RECEIVED:
			err = 1;
			break;
	}
	return err;
}

static int sna_cpic_fsm_sr_cd(int sr)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sr == CM_CONFIRM_DEALLOC_RECEIVED)
		err = 1;
	return err;
}

static int sna_cpic_fsm_sr_co(int sr)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sr == CM_CONFIRM_RECEIVED)
		err = 1;
	return err;
}

static int sna_cpic_fsm_sr_cs(int sr)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sr == CM_CONFIRM_SEND_RECEIVED)
		err = 1;
	return err;
}

static int sna_cpic_fsm_sr_jt(int sr)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sr == CM_JOIN_TRANSACTION)
		err = 1;
	return err;
}

static int sna_cpic_fsm_sr_no(int sr)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sr == CM_NO_STATUS_RECEIVED)
		err = 1;
	return err;
}

static int sna_cpic_fsm_sr_po(int sr)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sr == CM_PREPARE_OK)
		err = 1;
	return err;
}

static int sna_cpic_fsm_sr_se(int sr)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sr == CM_SEND_RECEIVED)
		err = 1;
	return err;
}

static int sna_cpic_fsm_sr_tc(int sr)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (sr) {
		case CM_TAKE_COMMIT:
		case CM_TAKE_COMMIT_DATA_OK:
			err = 1;
			break;
	}
	return err;
}

static int sna_cpic_fsm_sr_td(int sr)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (sr) {
		case CM_TAKE_COMMIT_DEALLOCATE:
		case CM_TAKE_COMMIT_DEALLOC_DATA_OK:
			err = 1;
			break;
	}
	return err;
}

static int sna_cpic_fsm_sr_ts(int sr)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (sr) {
		case CM_TAKE_COMMIT_SEND:
		case CM_TAKE_COMMIT_SEND_DATA_OK:
			err = 1;
			break;
	}
	return err;
}

#ifdef TEMPLATE
static int sna_cpic_fsm_template_function(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_
			break;
		case CM_INITIALIZE_STATE:
			err = sna_cpic_fsm_
			break;
		case CM_SEND_STATE:
			err = sna_cpic_fsm_
			break;
		case CM_RECEIVE_STATE:
			err = sna_cpic_fsm_
			break;
		case CM_SEND_PENDING_STATE:
			err = sna_cpic_fsm_
			break;
		case CM_CONFIRM_STATE:
			err = sna_cpic_fsm_
			break;
		case CM_CONFIRM_SEND_STATE:
			err = sna_cpic_fsm_
			break;
		case CM_CONFIRM_DEALLOCATE_STATE:
			err = sna_cpic_fsm_
			break;
		case CM_DEFER_RECEIVE_STATE:
			err = sna_cpic_fsm_
			break;
		case CM_DEFER_DEALLOCATE_STATE:
			err = sna_cpic_fsm_
			break;
		case CM_SYNC_POINT_STATE:
			err = sna_cpic_fsm_
			break;
		case CM_SYNC_POINT_SEND_STATE:
			err = sna_cpic_fsm_
			break;
		case CM_SYNC_POINT_DEALLOCATE_STATE:
			err = sna_cpic_fsm_
			break;
		case CM_INITIALIZE_INCOMING_STATE:
			err = sna_cpic_fsm_
			break;
		case CM_SEND_ONLY_STATE:
			err = sna_cpic_fsm_
			break;
		case CM_RECEIVE_ONLY_STATE:
			err = sna_cpic_fsm_
			break;
		case CM_SEND_RECEIVE_STATE:
			err = sna_cpic_fsm_
			break;
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_
			break;
	}
	return err;
}
#endif

static int sna_cpic_fsm_generic_pc(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_generic_sc(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_STATE_CHECK;
}

static int sna_cpic_fsm_cmacci(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_STATE_CHECK;
}

static int sna_cpic_fsm_cmaccp(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_STATE_CHECK;
}

static int sna_cpic_fsm_cmallc_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");

	if (!readonly) {
		if (sna_cpic_fsm_rc_ok(rc))
			cpic->state = CM_SEND_STATE;
		if (sna_cpic_fsm_rc_ae(rc, 1))
			cpic->state = CM_RESET_STATE;
	}
	return CM_OK;
}

static int sna_cpic_fsm_cmallc(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
			err = sna_cpic_fsm_cmallc_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmcanc(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmcfm(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmcfmd(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmdeal(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmdfde(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmeacn(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmeaeq(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmeapt(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmecs(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmect(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmectx(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmeid(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmemn(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmepid(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmepln(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmesi(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmesl(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmesrm(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmesui(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmetc(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmetpn(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmflus(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmincl(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cminic(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cminit_reset(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;
	sna_debug(5, "init\n");
	if (!readonly && sna_cpic_fsm_rc_ok(rc))
		cpic->state = CM_INITIALIZE_STATE;
	return err;
}

static int sna_cpic_fsm_cminit(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_cminit_reset(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmprep(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmptr(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_receive_i_receive(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc)) {
		/* [ok] {dr,no} */
		if (sna_cpic_fsm_dr_dr(dr) && sna_cpic_fsm_sr_no(sr)) {
			goto out;
		}
		/* [ok] {dr,se} */
		if (sna_cpic_fsm_dr_dr(dr) && sna_cpic_fsm_sr_se(sr)) {
			if (!readonly)
				cpic->state = CM_SEND_PENDING_STATE;
			goto out;
		}
		/* [ok] {nd,se} */
		if (sna_cpic_fsm_dr_nd(dr) && sna_cpic_fsm_sr_se(sr)) {
			if (!readonly)
				cpic->state = CM_SEND_STATE;
			goto out;
		}
		/* [ok] {*,cd} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_cd(sr)) {
			if (!readonly)
				cpic->state = CM_CONFIRM_DEALLOCATE_STATE;
			goto out;
		}
		/* [ok] {*,co} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_co(sr)) {
			if (!readonly)
				cpic->state = CM_CONFIRM_STATE;
			goto out;
		}
		/* [ok] {*,cs} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_cs(sr)) {
			if (!readonly)
				cpic->state = CM_CONFIRM_SEND_STATE;
			goto out;
		}
		/* [ok] {*,jt} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_jt(sr)) {
			goto out;
		}
		/* [ok] {*,po} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_po(sr)) {
			goto out;
		}
		/* [ok] {*,tc} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_tc(sr)) {
			if (!readonly)
				cpic->state = CM_SYNC_POINT_STATE;
			goto out;
		}
		/* [ok] {*,td} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_td(sr)) {
			if (!readonly)
				cpic->state = CM_SYNC_POINT_DEALLOCATE_STATE;
			goto out;
		}
		/* [ok] {*,ts} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_ts(sr)) {
			if (!readonly)
				cpic->state = CM_SYNC_POINT_SEND_STATE;
			goto out;
		}
		goto out;
	}
	/* [ae] */
	if (sna_cpic_fsm_rc_ae(rc, 0)) {
		if (!readonly)
			cpic->state = CM_RESET_STATE;
		goto out;
	}
	/* [bo] */
	if (sna_cpic_fsm_rc_bo(rc)) {
		goto out;
	}
	/* [da,dn,rf] */
	if (sna_cpic_fsm_rc_da(rc) || sna_cpic_fsm_rc_dn(rc)
		|| sna_cpic_fsm_rc_rf(rc)) {
		if (!readonly)
			cpic->state = CM_RESET_STATE;
		goto out;
	}
	/* [db,pb,rb] */
	if (sna_cpic_fsm_rc_db(rc) || sna_cpic_fsm_rc_pb(rc)
		|| sna_cpic_fsm_rc_rb(rc)) {
		if (!readonly)
			cpic->state = CM_RESET_STATE;
		goto out;
	}
	/* [en,ep] */
	if (sna_cpic_fsm_rc_en(rc) || sna_cpic_fsm_rc_ep(rc)) {
		goto out;
	}
	/* [et] */
	if (sna_cpic_fsm_rc_et(rc)) {
		goto out;
	}
	/* [pc,rs,un] */
	if (sna_cpic_fsm_rc_pc(rc) || sna_cpic_fsm_rc_un(rc)) {
		goto out;
	}
out:	return err;
}

static int sna_cpic_fsm_receive_i_prepared(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc)) {
		/* [ok] {dr,no} */
		if (sna_cpic_fsm_dr_dr(dr) && sna_cpic_fsm_sr_no(sr)) {
			goto out;
		}
		/* [ok] {dr,se} */
		if (sna_cpic_fsm_dr_dr(dr) && sna_cpic_fsm_sr_se(sr)) {
			goto out;
		}
		/* [ok] {nd,se} */
		if (sna_cpic_fsm_dr_nd(dr) && sna_cpic_fsm_sr_se(sr)) {
			goto out;
		}
		/* [ok] {*,cd} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_cd(sr)) {
			goto out;
		}
		/* [ok] {*,co} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_co(sr)) {
			goto out;
		}
		/* [ok] {*,cs} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_cs(sr)) {
			goto out;
		}
		/* [ok] {*,jt} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_jt(sr)) {
			goto out;
		}
		/* [ok] {*,po} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_po(sr)) {
			goto out;
		}
		/* [ok] {*,tc} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_tc(sr)) {
			goto out;
		}
		/* [ok] {*,td} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_td(sr)) {
			goto out;
		}
		/* [ok] {*,ts} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_ts(sr)) {
			goto out;
		}
		goto out;
	}
	/* [ae] */
	if (sna_cpic_fsm_rc_ae(rc, 0)) {
		if (!readonly)
			cpic->state = CM_RESET_STATE;
		goto out;
	}
	/* [bo] */
	if (sna_cpic_fsm_rc_bo(rc)) {
		goto out;
	}
	/* [da,dn,rf] */
	if (sna_cpic_fsm_rc_da(rc) || sna_cpic_fsm_rc_dn(rc)
		|| sna_cpic_fsm_rc_rf(rc)) {
		goto out;
	}
	/* [db,pb,rb] */
	if (sna_cpic_fsm_rc_db(rc) || sna_cpic_fsm_rc_pb(rc)
		|| sna_cpic_fsm_rc_rb(rc)) {
		if (!readonly)
			cpic->state = CM_RESET_STATE;
		goto out;
	}
	/* [en,ep] */
	if (sna_cpic_fsm_rc_en(rc) || sna_cpic_fsm_rc_ep(rc)) {
		if (!readonly)
			cpic->state = CM_RECEIVE_STATE;
		goto out;
	}
	/* [et] */
	if (sna_cpic_fsm_rc_et(rc)) {
		goto out;
	}
	/* [pc,rs,un] */
	if (sna_cpic_fsm_rc_pc(rc) || sna_cpic_fsm_rc_un(rc)) {
		goto out;
	}
out:    return err;
}

static int sna_cpic_fsm_cmrcv_i(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
		case CM_SEND_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_RECEIVE_STATE:
			err = sna_cpic_fsm_receive_i_receive(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_receive_i_prepared(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_receive_w_send(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc)) {
		/* [ok] {dr,no} */
		if (sna_cpic_fsm_dr_dr(dr) && sna_cpic_fsm_sr_no(sr)) {
			if (!readonly)
				cpic->state = CM_RECEIVE_STATE;
			goto out;
		}
		/* [ok] {dr,se} */
		if (sna_cpic_fsm_dr_dr(dr) && sna_cpic_fsm_sr_se(sr)) {
			if (!readonly)
				cpic->state = CM_SEND_PENDING_STATE;
			goto out;
		}
		/* [ok] {nd,se} */
		if (sna_cpic_fsm_dr_nd(dr) && sna_cpic_fsm_sr_se(sr)) {
			goto out;
		}
		/* [ok] {*,cd} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_cd(sr)) {
			if (!readonly)
				cpic->state = CM_CONFIRM_DEALLOCATE_STATE;
			goto out;
		}
		/* [ok] {*,co} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_co(sr)) {
			if (!readonly)
				cpic->state = CM_CONFIRM_STATE;
			goto out;
		}
		/* [ok] {*,cs} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_cs(sr)) {
			if (!readonly)
				cpic->state =CM_CONFIRM_SEND_STATE;
			goto out;
		}
		/* [ok] {*,jt} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_jt(sr)) {
			goto out;
		}
		/* [ok] {*,po} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_po(sr)) {
			goto out;
		}
		/* [ok] {*,tc} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_tc(sr)) {
			if (!readonly)
				cpic->state = CM_SYNC_POINT_STATE;
			goto out;
		}
		/* [ok] {*,td} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_td(sr)) {
			if (!readonly)
				cpic->state = CM_SYNC_POINT_DEALLOCATE_STATE;
			goto out;
		}
		/* [ok] {*,ts} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_ts(sr)) {
			if (!readonly)
				cpic->state = CM_SYNC_POINT_SEND_STATE;
			goto out;
		}
		goto out;
	}
	/* [ae] */
	if (sna_cpic_fsm_rc_ae(rc, 0)) {
		if (!readonly)
			cpic->state = CM_RESET_STATE;
		goto out;
	}
	/* [bo] */
	if (sna_cpic_fsm_rc_bo(rc)) {
		if (!readonly)
			cpic->state = CM_RECEIVE_STATE;
		goto out;
	}
	/* [da,dn,rf] */
	if (sna_cpic_fsm_rc_da(rc) || sna_cpic_fsm_rc_dn(rc)
		|| sna_cpic_fsm_rc_rf(rc)) {
		if (!readonly)
			cpic->state = CM_RESET_STATE;
		goto out;
	}
	/* [db,pb,rb] */
	if (sna_cpic_fsm_rc_db(rc) || sna_cpic_fsm_rc_pb(rc)
		|| sna_cpic_fsm_rc_rb(rc)) {
		if (!readonly)
			cpic->state = CM_RESET_STATE;
		goto out;
	}
	/* [en,ep] */
	if (sna_cpic_fsm_rc_en(rc) || sna_cpic_fsm_rc_ep(rc)) {
		if (!readonly)
			cpic->state = CM_RECEIVE_STATE;
		goto out;
	}
	/* [et] */
	if (sna_cpic_fsm_rc_et(rc)) {
		goto out;
	}
	/* [oi,pc] */
	if (sna_cpic_fsm_rc_oi(rc) || sna_cpic_fsm_rc_pc(rc)) {
		goto out;
	}
	/* [sc] */
	if (sna_cpic_fsm_rc_sc(rc)) {
		goto out;
	}
out:	return err;
}

static int sna_cpic_fsm_receive_w_receive(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc)) {
		/* [ok] {dr,no} */
		if (sna_cpic_fsm_dr_dr(dr) && sna_cpic_fsm_sr_no(sr)) {
			goto out;
		}
		/* [ok] {dr,se} */
		if (sna_cpic_fsm_dr_dr(dr) && sna_cpic_fsm_sr_se(sr)) {
			if (!readonly)
				cpic->state = CM_SEND_PENDING_STATE;
			goto out;
		}
		/* [ok] {nd,se} */
		if (sna_cpic_fsm_dr_nd(dr) && sna_cpic_fsm_sr_se(sr)) {
			if (!readonly)
				cpic->state = CM_SEND_STATE;
			goto out;
		}
		/* [ok] {*,cd} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_cd(sr)) {
			if (!readonly)
				cpic->state = CM_CONFIRM_DEALLOCATE_STATE;
			goto out;
		}
		/* [ok] {*,co} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_co(sr)) {
			if (!readonly)
				cpic->state = CM_CONFIRM_STATE;
			goto out;
		}
		/* [ok] {*,cs} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_cs(sr)) {
			if (!readonly)
				cpic->state = CM_CONFIRM_SEND_STATE;
			goto out;
		}
		/* [ok] {*,jt} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_jt(sr)) {
			goto out;
		}
		/* [ok] {*,po} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_po(sr)) {
			goto out;
		}
		/* [ok] {*,tc} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_tc(sr)) {
			if (!readonly)
				cpic->state = CM_SYNC_POINT_STATE;
			goto out;
		}
		/* [ok] {*,td} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_td(sr)) {
			if (!readonly)
				cpic->state = CM_SYNC_POINT_DEALLOCATE_STATE;
			goto out;
		}
		/* [ok] {*,ts} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_ts(sr)) {
			if (!readonly)
				cpic->state = CM_SYNC_POINT_SEND_STATE;
			goto out;
		}
		goto out;
	}
	/* [ae] */
	if (sna_cpic_fsm_rc_ae(rc, 0)) {
		if (!readonly)
			cpic->state = CM_RESET_STATE;
		goto out;
	}
	/* [bo] */
	if (sna_cpic_fsm_rc_bo(rc)) {
		goto out;
	}
	/* [da,dn,rf] */
	if (sna_cpic_fsm_rc_da(rc) || sna_cpic_fsm_rc_dn(rc)
		|| sna_cpic_fsm_rc_rf(rc)) {
		if (!readonly)
			cpic->state = CM_RESET_STATE;
		goto out;
	}
	/* [db,pb,rb] */
	if (sna_cpic_fsm_rc_db(rc) || sna_cpic_fsm_rc_pb(rc)
		|| sna_cpic_fsm_rc_rb(rc)) {
		if (!readonly)
			cpic->state = CM_RESET_STATE;
		goto out;
	}
	/* [en,ep] */
	if (sna_cpic_fsm_rc_en(rc) || sna_cpic_fsm_rc_ep(rc)) {
		goto out;
	}
	/* [et] */
	if (sna_cpic_fsm_rc_et(rc)) {
		goto out;
	}
	/* [oi,pc] */
	if (sna_cpic_fsm_rc_oi(rc) || sna_cpic_fsm_rc_pc(rc)) {
		goto out;
	}
	/* [sc] */
	if (sna_cpic_fsm_rc_sc(rc)) {
		goto out;
	}
out:    return err;
}

static int sna_cpic_fsm_receive_w_confirm(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc)) {
		/* [ok] {dr,no} */
		if (sna_cpic_fsm_dr_dr(dr) && sna_cpic_fsm_sr_no(sr)) {
			if (!readonly)
				cpic->state = CM_RECEIVE_STATE;
			goto out;
		}
		/* [ok] {dr,se} */
		if (sna_cpic_fsm_dr_dr(dr) && sna_cpic_fsm_sr_se(sr)) {
			goto out;
		}
		/* [ok] {nd,se} */
		if (sna_cpic_fsm_dr_nd(dr) && sna_cpic_fsm_sr_se(sr)) {
			if (!readonly)
				cpic->state = CM_SEND_STATE;
			goto out;
		}
		/* [ok] {*,cd} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_cd(sr)) {
			if (!readonly)
				cpic->state = CM_CONFIRM_DEALLOCATE_STATE;
			goto out;
		}
		/* [ok] {*,co} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_co(sr)) {
			if (!readonly)
				cpic->state = CM_CONFIRM_STATE;
			goto out;
		}
		/* [ok] {*,cs} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_cs(sr)) {
			if (!readonly)
				cpic->state = CM_CONFIRM_SEND_STATE;
			goto out;
		}
		/* [ok] {*,jt} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_jt(sr)) {
			goto out;
		}
		/* [ok] {*,po} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_po(sr)) {
			goto out;
		}
		/* [ok] {*,tc} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_tc(sr)) {
			if (!readonly)
				cpic->state = CM_SYNC_POINT_STATE;
			goto out;
		}
		/* [ok] {*,td} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_td(sr)) {
			if (!readonly)
				cpic->state = CM_SYNC_POINT_DEALLOCATE_STATE;
			goto out;
		}
		/* [ok] {*,ts} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_ts(sr)) {
			if (!readonly)
				cpic->state = CM_SYNC_POINT_SEND_STATE;
			goto out;
		}
		goto out;
	}
	/* [ae] */
	if (sna_cpic_fsm_rc_ae(rc, 0)) {
		goto out;
	}
	/* [bo] */
	if (sna_cpic_fsm_rc_bo(rc)) {
		if (!readonly)
			cpic->state = CM_RECEIVE_STATE;
		goto out;
	}
	/* [da,dn,rf] */
	if (sna_cpic_fsm_rc_da(rc) || sna_cpic_fsm_rc_dn(rc)
		|| sna_cpic_fsm_rc_rf(rc)) {
		if (!readonly)
			cpic->state = CM_RESET_STATE;
		goto out;
	}
	/* [db,pb,rb] */
	if (sna_cpic_fsm_rc_db(rc) || sna_cpic_fsm_rc_pb(rc)
		|| sna_cpic_fsm_rc_rb(rc)) {
		if (!readonly)
			cpic->state = CM_RESET_STATE;
		goto out;
	}
	/* [en,ep] */
	if (sna_cpic_fsm_rc_en(rc) || sna_cpic_fsm_rc_ep(rc)) {
		if (!readonly)
			cpic->state = CM_RECEIVE_STATE;
		goto out;
	}
	/* [et] */
	if (sna_cpic_fsm_rc_et(rc)) {
		goto out;
	}
	/* [oi,pc] */
	if (sna_cpic_fsm_rc_oi(rc) || sna_cpic_fsm_rc_pc(rc)) {
		goto out;
	}
	/* [sc] */
	if (sna_cpic_fsm_rc_sc(rc)) {
		goto out;
	}
out:    return err;
}

static int sna_cpic_fsm_receive_w_prepared(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc)) {
		/* [ok] {dr,no} */
		if (sna_cpic_fsm_dr_dr(dr) && sna_cpic_fsm_sr_no(sr)) {
			goto out;
		}
		/* [ok] {dr,se} */
		if (sna_cpic_fsm_dr_dr(dr) && sna_cpic_fsm_sr_se(sr)) {
			goto out;
		}
		/* [ok] {nd,se} */
		if (sna_cpic_fsm_dr_nd(dr) && sna_cpic_fsm_sr_se(sr)) {
			goto out;
		}
		/* [ok] {*,cd} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_cd(sr)) {
			goto out;
		}
		/* [ok] {*,co} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_co(sr)) {
			goto out;
		}
		/* [ok] {*,cs} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_cs(sr)) {
			goto out;
		}
		/* [ok] {*,jt} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_jt(sr)) {
			goto out;
		}
		/* [ok] {*,po} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_po(sr)) {
			goto out;
		}
		/* [ok] {*,tc} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_tc(sr)) {
			goto out;
		}
		/* [ok] {*,td} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_td(sr)) {
			goto out;
		}
		/* [ok] {*,ts} */
		if (sna_cpic_fsm_dr_any(dr) && sna_cpic_fsm_sr_ts(sr)) {
			goto out;
		}
		goto out;
	}
	/* [ae] */
	if (sna_cpic_fsm_rc_ae(rc, 0)) {
		if (!readonly)
			cpic->state = CM_RESET_STATE;
		goto out;
	}
	/* [bo] */
	if (sna_cpic_fsm_rc_bo(rc)) {
		goto out;
	}
	/* [da,dn,rf] */
	if (sna_cpic_fsm_rc_da(rc) || sna_cpic_fsm_rc_dn(rc)
		|| sna_cpic_fsm_rc_rf(rc)) {
		goto out;
	}
	/* [db,pb,rb] */
	if (sna_cpic_fsm_rc_db(rc) || sna_cpic_fsm_rc_pb(rc)
		|| sna_cpic_fsm_rc_rb(rc)) {
		if (!readonly)
			cpic->state = CM_RESET_STATE;
		goto out;
	}
	/* [en,ep] */
	if (sna_cpic_fsm_rc_en(rc) || sna_cpic_fsm_rc_ep(rc)) {
		if (!readonly)
			cpic->state = CM_RECEIVE_STATE;
		goto out;
	}
	/* [et] */
	if (sna_cpic_fsm_rc_et(rc)) {
		goto out;
	}
	/* [oi,pc] */
	if (sna_cpic_fsm_rc_oi(rc) || sna_cpic_fsm_rc_pc(rc)) {
		goto out;
	}
	/* [sc] */
	if (sna_cpic_fsm_rc_sc(rc)) {
		goto out;
	}
out:    return err;
}

static int sna_cpic_fsm_cmrcv_w(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_SEND_STATE:
			err = sna_cpic_fsm_receive_w_send(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_RECEIVE_STATE:
			err = sna_cpic_fsm_receive_w_receive(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_CONFIRM_STATE:
			err = sna_cpic_fsm_receive_w_confirm(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_receive_w_prepared(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmrcvx(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmrts(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmsac_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmsac(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
			err = sna_cpic_fsm_cmsac_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmsacn_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmsacn(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
			err = sna_cpic_fsm_cmsacn_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmsaeq_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:	return err;
}

static int sna_cpic_fsm_cmsaeq(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
			err = sna_cpic_fsm_cmsaeq_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmsapt_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmsapt(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
			err = sna_cpic_fsm_cmsapt_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmsbt_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmsbt(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_cmsbt_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_INCOMING_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmscsp_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmscsp(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
			err = sna_cpic_fsm_cmscsp_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmscst_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmscst(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
			err = sna_cpic_fsm_cmscst_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmscsu_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmscsu(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
			err = sna_cpic_fsm_cmscsu_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmsct_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmsct(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
			err = sna_cpic_fsm_cmsct_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmscu_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmscu(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_cmscu_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmsdt_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmsdt(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_cmsdt_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_INCOMING_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmsed_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmsed(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_cmsed_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmsend_send(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc)) {
		/* (B) [ok] */
		if (sna_cpic_fsm_cc_b(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (C) [ok] */
		if (sna_cpic_fsm_cc_c(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (D(A)) [ok] */
		if (sna_cpic_fsm_cc_d(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_a(cpic, CM_CMSEND)) {
			if (!readonly)
				cpic->state = CM_RESET_STATE;
			goto out;
		}
		/* (D(C)) [ok] */
		if (sna_cpic_fsm_cc_d(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_c(cpic, CM_CMSEND)) {
			if (!readonly)
				cpic->state = CM_RESET_STATE;
			goto out;
		}
		/* (D(F)) [ok] */
		if (sna_cpic_fsm_cc_d(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_f(cpic, CM_CMSEND)) {
			if (!readonly)
				cpic->state = CM_RESET_STATE;
			goto out;
		}
		/* (D(S)) [ok] */
		if (sna_cpic_fsm_cc_d(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_s(cpic, CM_CMSEND)) {
			if (!readonly)
				cpic->state = CM_DEFER_DEALLOCATE_STATE;
			goto out;
		}
		/* (F) [ok] */
		if (sna_cpic_fsm_cc_f(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (P(C)) [ok] */
		if (sna_cpic_fsm_cc_p(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_c(cpic, CM_CMSEND)) {
			if (!readonly)
				cpic->state = CM_RECEIVE_STATE;
			goto out;
		}
		/* (P(F)) [ok] */
		if (sna_cpic_fsm_cc_p(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_f(cpic, CM_CMSEND)) {
			if (!readonly)
				cpic->state = CM_RECEIVE_STATE;
			goto out;
		}
		/* (P(S)) [ok] */
		if (sna_cpic_fsm_cc_p(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_s(cpic, CM_CMSEND)) {
			if (!readonly)
				cpic->state = CM_DEFER_RECEIVE_STATE;
			goto out;
		}
	}
	/* [ae] */
	if (sna_cpic_fsm_rc_ae(rc, 0)) {
		if (!readonly)
			cpic->state = CM_RESET_STATE;
		goto out;
	}
	/* [bo] */
	if (sna_cpic_fsm_rc_bo(rc)) {
		goto out;
	}
	/* [da,rf] */
	if (sna_cpic_fsm_rc_da(rc) || sna_cpic_fsm_rc_rf(rc)) {
		if (!readonly)
			cpic->state = CM_RESET_STATE;
		goto out;
	}
	/* [db,rb] */
	if (sna_cpic_fsm_rc_db(rc) || sna_cpic_fsm_rc_rb(rc)) {
		if (!readonly)
			cpic->state = CM_RESET_STATE;
		goto out;
	}
	/* [ep] */
	if (sna_cpic_fsm_rc_ep(rc)) {
		if (!readonly)
			cpic->state = CM_RECEIVE_STATE;
		goto out;
	}
	/* [oi,pc] */
	if (sna_cpic_fsm_rc_oi(rc) || sna_cpic_fsm_rc_pc(rc)) {
		goto out;
	}
	/* [pb] */
	if (sna_cpic_fsm_rc_pb(rc)) {
		if (!readonly)
			cpic->state = CM_RESET_STATE;
		goto out;
	}
	/* [sc] */
	if (sna_cpic_fsm_rc_sc(rc)) {
		goto out;
	}
out:	return err;
}

static int sna_cpic_fsm_cmsend_send_pending(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc)) {
		/* (B) [ok] */
		if (sna_cpic_fsm_cc_b(cpic, CM_CMSEND)) {
			if (!readonly)
				cpic->state = CM_SEND_STATE;
			goto out;
		}
		/* (C) [ok] */
		if (sna_cpic_fsm_cc_c(cpic, CM_CMSEND)) {
			if (!readonly)
				cpic->state = CM_SEND_STATE;
			goto out;
		}
		/* (D(A)) [ok] */
		if (sna_cpic_fsm_cc_d(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_a(cpic, CM_CMSEND)) {
			if (!readonly)
				cpic->state = CM_RESET_STATE;
			goto out;
		}
		/* (D(C)) [ok] */
		if (sna_cpic_fsm_cc_d(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_c(cpic, CM_CMSEND)) {
			if (!readonly)
				cpic->state = CM_RESET_STATE;
			goto out;
		}
		/* (D(F)) [ok] */
		if (sna_cpic_fsm_cc_d(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_f(cpic, CM_CMSEND)) {
			if (!readonly)
				cpic->state = CM_RESET_STATE;
			goto out;
		}
		/* (D(S)) [ok] */
		if (sna_cpic_fsm_cc_d(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_s(cpic, CM_CMSEND)) {
			if (!readonly)
				cpic->state = CM_DEFER_DEALLOCATE_STATE;
			goto out;
		}
		/* (F) [ok] */
		if (sna_cpic_fsm_cc_f(cpic, CM_CMSEND)) {
			if (!readonly)
				cpic->state = CM_SEND_STATE;
			goto out;
		}
		/* (P(C)) [ok] */
		if (sna_cpic_fsm_cc_p(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_c(cpic, CM_CMSEND)) {
			if (!readonly)
				cpic->state = CM_RECEIVE_STATE;
			goto out;
		}
		/* (P(F)) [ok] */
		if (sna_cpic_fsm_cc_p(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_f(cpic, CM_CMSEND)) {
			if (!readonly)
				cpic->state = CM_RECEIVE_STATE;
			goto out;
		}
		/* (P(S)) [ok] */
		if (sna_cpic_fsm_cc_p(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_s(cpic, CM_CMSEND)) {
			if (!readonly)
				cpic->state = CM_DEFER_RECEIVE_STATE;
			goto out;
		}
	}
	/* [ae] */
	if (sna_cpic_fsm_rc_ae(rc, 0)) {
		goto out;
	}
	/* [bo] */
	if (sna_cpic_fsm_rc_bo(rc)) {
		if (!readonly)
			cpic->state = CM_SEND_STATE;
		goto out;
	}
	/* [da,rf] */
	if (sna_cpic_fsm_rc_da(rc) || sna_cpic_fsm_rc_rf(rc)) {
		if (!readonly)
			cpic->state = CM_RESET_STATE;
		goto out;
	}
	/* [db,rb] */
	if (sna_cpic_fsm_rc_db(rc) || sna_cpic_fsm_rc_rb(rc)) {
		if (!readonly)
			cpic->state = CM_RESET_STATE;
		goto out;
	}
	/* [ep] */
	if (sna_cpic_fsm_rc_ep(rc)) {
		if (!readonly)
			cpic->state = CM_RECEIVE_STATE;
		goto out;
	}
	/* [oi,pc] */
	if (sna_cpic_fsm_rc_oi(rc) || sna_cpic_fsm_rc_pc(rc)) {
		goto out;
	}
	/* [pb] */
	if (sna_cpic_fsm_rc_pb(rc)) {
		goto out;
	}
	/* [sc] */
	if (sna_cpic_fsm_rc_sc(rc)) {
		goto out;
	}
out:    return err;
}

static int sna_cpic_fsm_cmsend_sync_point(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc)) {
		/* (B) [ok] */
		if (sna_cpic_fsm_cc_b(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (C) [ok] */
		if (sna_cpic_fsm_cc_c(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (D(A)) [ok] */
		if (sna_cpic_fsm_cc_d(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_a(cpic, CM_CMSEND)) {
			if (!readonly)
				cpic->state = CM_RESET_STATE;
			goto out;
		}
		/* (D(C)) [ok] */
		if (sna_cpic_fsm_cc_d(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_c(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (D(F)) [ok] */
		if (sna_cpic_fsm_cc_d(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_f(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (D(S)) [ok] */
		if (sna_cpic_fsm_cc_d(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_s(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (F) [ok] */
		if (sna_cpic_fsm_cc_f(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (P(C)) [ok] */
		if (sna_cpic_fsm_cc_p(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_c(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (P(F)) [ok] */
		if (sna_cpic_fsm_cc_p(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_f(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (P(S)) [ok] */
		if (sna_cpic_fsm_cc_p(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_s(cpic, CM_CMSEND)) {
			goto out;
		}
	}
	/* [ae] */
	if (sna_cpic_fsm_rc_ae(rc, 0)) {
		goto out;
	}
	/* [bo] */
	if (sna_cpic_fsm_rc_bo(rc)) {
		goto out;
	}
	/* [da,rf] */
	if (sna_cpic_fsm_rc_da(rc) || sna_cpic_fsm_rc_rf(rc)) {
		goto out;
	}
	/* [db,rb] */
	if (sna_cpic_fsm_rc_db(rc) || sna_cpic_fsm_rc_rb(rc)) {
		if (!readonly)
			cpic->state = CM_RESET_STATE;
		goto out;
	}
	/* [ep] */
	if (sna_cpic_fsm_rc_ep(rc)) {
		goto out;
	}
	/* [oi,pc] */
	if (sna_cpic_fsm_rc_oi(rc) || sna_cpic_fsm_rc_pc(rc)) {
		goto out;
	}
	/* [pb] */
	if (sna_cpic_fsm_rc_pb(rc)) {
		goto out;
	}
	/* [sc] */
	if (sna_cpic_fsm_rc_sc(rc)) {
		goto out;
	}
out:    return err;
}

static int sna_cpic_fsm_cmsend_sync_point_send(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc)) {
		/* (B) [ok] */
		if (sna_cpic_fsm_cc_b(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (C) [ok] */
		if (sna_cpic_fsm_cc_c(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (D(A)) [ok] */
		if (sna_cpic_fsm_cc_d(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_a(cpic, CM_CMSEND)) {
			if (!readonly)
				cpic->state = CM_RESET_STATE;
			goto out;
		}
		/* (D(C)) [ok] */
		if (sna_cpic_fsm_cc_d(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_c(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (D(F)) [ok] */
		if (sna_cpic_fsm_cc_d(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_f(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (D(S)) [ok] */
		if (sna_cpic_fsm_cc_d(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_s(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (F) [ok] */
		if (sna_cpic_fsm_cc_f(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (P(C)) [ok] */
		if (sna_cpic_fsm_cc_p(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_c(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (P(F)) [ok] */
		if (sna_cpic_fsm_cc_p(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_f(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (P(S)) [ok] */
		if (sna_cpic_fsm_cc_p(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_s(cpic, CM_CMSEND)) {
			goto out;
		}
	}
	/* [ae] */
	if (sna_cpic_fsm_rc_ae(rc, 0)) {
		goto out;
	}
	/* [bo] */
	if (sna_cpic_fsm_rc_bo(rc)) {
		goto out;
	}
	/* [da,rf] */
	if (sna_cpic_fsm_rc_da(rc) || sna_cpic_fsm_rc_rf(rc)) {
		goto out;
	}
	/* [db,rb] */
	if (sna_cpic_fsm_rc_db(rc) || sna_cpic_fsm_rc_rb(rc)) {
		if (!readonly)
			cpic->state = CM_RESET_STATE;
		goto out;
	}
	/* [ep] */
	if (sna_cpic_fsm_rc_ep(rc)) {
		goto out;
	}
	/* [oi,pc] */
	if (sna_cpic_fsm_rc_oi(rc) || sna_cpic_fsm_rc_pc(rc)) {
		goto out;
	}
	/* [pb] */
	if (sna_cpic_fsm_rc_pb(rc)) {
		goto out;
	}
	/* [sc] */
	if (sna_cpic_fsm_rc_sc(rc)) {
		goto out;
	}
out:    return err;
}

static int sna_cpic_fsm_cmsend_sync_point_deallocate(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc)) {
		/* (B) [ok] */
		if (sna_cpic_fsm_cc_b(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (C) [ok] */
		if (sna_cpic_fsm_cc_c(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (D(A)) [ok] */
		if (sna_cpic_fsm_cc_d(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_a(cpic, CM_CMSEND)) {
			if (!readonly)
				cpic->state = CM_RESET_STATE;
			goto out;
		}
		/* (D(C)) [ok] */
		if (sna_cpic_fsm_cc_d(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_c(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (D(F)) [ok] */
		if (sna_cpic_fsm_cc_d(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_f(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (D(S)) [ok] */
		if (sna_cpic_fsm_cc_d(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_s(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (F) [ok] */
		if (sna_cpic_fsm_cc_f(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (P(C)) [ok] */
		if (sna_cpic_fsm_cc_p(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_c(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (P(F)) [ok] */
		if (sna_cpic_fsm_cc_p(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_f(cpic, CM_CMSEND)) {
			goto out;
		}
		/* (P(S)) [ok] */
		if (sna_cpic_fsm_cc_p(cpic, CM_CMSEND)
			&& sna_cpic_fsm_cc_s(cpic, CM_CMSEND)) {
			goto out;
		}
	}
	/* [ae] */
	if (sna_cpic_fsm_rc_ae(rc, 0)) {
		goto out;
	}
	/* [bo] */
	if (sna_cpic_fsm_rc_bo(rc)) {
		goto out;
	}
	/* [da,rf] */
	if (sna_cpic_fsm_rc_da(rc) || sna_cpic_fsm_rc_rf(rc)) {
		goto out;
	}
	/* [db,rb] */
	if (sna_cpic_fsm_rc_db(rc) || sna_cpic_fsm_rc_rb(rc)) {
		if (!readonly)
			cpic->state = CM_RESET_STATE;
		goto out;
	}
	/* [ep] */
	if (sna_cpic_fsm_rc_ep(rc)) {
		goto out;
	}
	/* [oi,pc] */
	if (sna_cpic_fsm_rc_oi(rc) || sna_cpic_fsm_rc_pc(rc)) {
		goto out;
	}
	/* [pb] */
	if (sna_cpic_fsm_rc_pb(rc)) {
		goto out;
	}
	/* [sc] */
	if (sna_cpic_fsm_rc_sc(rc)) {
		goto out;
	}
out:    return err;
}

static int sna_cpic_fsm_cmsend(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_SEND_STATE:
			err = sna_cpic_fsm_cmsend_send(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_SEND_PENDING_STATE:
			err = sna_cpic_fsm_cmsend_send_pending(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_SYNC_POINT_STATE:
			err = sna_cpic_fsm_cmsend_sync_point(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_SYNC_POINT_SEND_STATE:
			err = sna_cpic_fsm_cmsend_sync_point_send(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_SYNC_POINT_DEALLOCATE_STATE:
			err = sna_cpic_fsm_cmsend_sync_point_deallocate(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
		case CM_RECEIVE_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmserr(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmsf_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmsf(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_cmsf_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmsid_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmsid(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
		case CM_RECEIVE_STATE:
			err = sna_cpic_fsm_cmsid_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_SEND_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmsld_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmsld(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_cmsld_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmsmn_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmsmn(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
			err = sna_cpic_fsm_cmsmn_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmsndx(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmspdp_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmspdp(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_cmspdp_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_INCOMING_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmspid_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmspid(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
			err = sna_cpic_fsm_cmspid_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmspln_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmspln(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
			err = sna_cpic_fsm_cmspln_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmspm_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmspm(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_cmspm_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmsptr_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmsptr(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_cmsptr_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_INCOMING_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmsqcf_n_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmsqcf_n(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
			err = sna_cpic_fsm_cmsqcf_n_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

#ifdef SNA_UNUSED
static int sna_cpic_fsm_cmsqcf_q_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}
#endif

static int sna_cpic_fsm_cmsqcf_q(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_cmsed_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_INCOMING_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmsqcf(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	int err = 0;

	sna_debug(5, "init\n");
	switch (tp->conversation_queue) {
		case AC_INITIALIZATION_QUEUE:
			err = sna_cpic_fsm_cmsqcf_n(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case AC_SEND_RECEIVE_QUEUE:
		case AC_EXPEDITED_SEND_QUEUE:
		case AC_EXPEDITED_RECEIVE_QUEUE:
			err = sna_cpic_fsm_cmsqcf_q(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmsqpm_n_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmsqpm_n(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
			err = sna_cpic_fsm_cmsqpm_n_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmsqpm_q_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmsqpm_q(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_cmsqpm_q_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_INCOMING_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmsqpm(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	int err = 0;

	sna_debug(5, "init\n");
	switch (tp->conversation_queue) {
		case AC_INITIALIZATION_QUEUE:
			err = sna_cpic_fsm_cmsqpm_n(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case AC_SEND_RECEIVE_QUEUE:
		case AC_EXPEDITED_SEND_QUEUE:
		case AC_EXPEDITED_RECEIVE_QUEUE:
			err = sna_cpic_fsm_cmsqpm_q(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmsrc_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmsrc(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
			err = sna_cpic_fsm_cmsrc_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmsrt_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmsrt(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_cmsrt_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmssl_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc)
		|| sna_cpic_fsm_rc_pn(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmssl(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
			err = sna_cpic_fsm_cmssl_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmssrm_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmssrm(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
			err = sna_cpic_fsm_cmssrm_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmsst_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmsst(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_cmsst_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_INCOMING_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmstc_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmstc(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
			err = sna_cpic_fsm_cmstc_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmstpn_initialize(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_rc_ok(rc) || sna_cpic_fsm_rc_pc(rc))
		goto out;
out:    return err;
}

static int sna_cpic_fsm_cmstpn(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	int err = 0;

	sna_debug(5, "init\n");
	switch (cpic->state) {
		case CM_RESET_STATE:
			err = sna_cpic_fsm_generic_pc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_INITIALIZE_STATE:
		case CM_INITIALIZE_INCOMING_STATE:
			err = sna_cpic_fsm_cmstpn_initialize(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
		case CM_SEND_STATE:
		case CM_RECEIVE_STATE:
		case CM_SEND_PENDING_STATE:
		case CM_CONFIRM_STATE:
		case CM_CONFIRM_SEND_STATE:
		case CM_CONFIRM_DEALLOCATE_STATE:
		case CM_DEFER_RECEIVE_STATE:
		case CM_DEFER_DEALLOCATE_STATE:
		case CM_SYNC_POINT_STATE:
		case CM_SYNC_POINT_SEND_STATE:
		case CM_SYNC_POINT_DEALLOCATE_STATE:
		case CM_SEND_ONLY_STATE:
		case CM_RECEIVE_ONLY_STATE:
		case CM_SEND_RECEIVE_STATE:
		case CM_PREPARED_STATE:
			err = sna_cpic_fsm_generic_sc(cpic, input, rc,
				dr, sr, fd, readonly);
			break;
	}
	return err;
}

static int sna_cpic_fsm_cmtrts(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

static int sna_cpic_fsm_cmwait(struct cpic *cpic, int input, int rc,
	int dr, int sr, int fd, int readonly)
{
	sna_debug(5, "init\n");
	return CM_PROGRAM_PARAMETER_CHECK;
}

/**
 * @input: cpic call name.
 * @rc: return code.
 * @dr: data recived.
 * @sr: status received.
 * @fd: full duplex.
 * @readonly: just test input against state machine, don't modify anything.
 */
static int sna_cpic_fsm_hd(struct cpic *cpic, CM_CALL_ID input, CM_RETURN_CODE rc,
	int dr, int sr, int fd, int readonly)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	int err = 0;

	sna_debug(5, "init: input:%ld rc:%ld dr:%d sr:%d fd:%d readonly:%d state:%d\n",
		input, rc, dr, sr, fd, readonly, cpic->state);
	switch (input) {
		case CM_CMACCI:
			err = sna_cpic_fsm_cmacci(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMACCP:
			err = sna_cpic_fsm_cmaccp(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMALLC:
			err = sna_cpic_fsm_cmallc(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMCANC:
			err = sna_cpic_fsm_cmcanc(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMCFM:
			err = sna_cpic_fsm_cmcfm(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMCFMD:
			err = sna_cpic_fsm_cmcfmd(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMDEAL:
			err = sna_cpic_fsm_cmdeal(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMDFDE:
			err = sna_cpic_fsm_cmdfde(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMEACN:
			err = sna_cpic_fsm_cmeacn(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMEAEQ:
			err = sna_cpic_fsm_cmeaeq(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMEAPT:
			err = sna_cpic_fsm_cmeapt(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMECS:
			err = sna_cpic_fsm_cmecs(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMECT:
			err = sna_cpic_fsm_cmect(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMECTX:
			err = sna_cpic_fsm_cmectx(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMEID:
			err = sna_cpic_fsm_cmeid(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMEMN:
			err = sna_cpic_fsm_cmemn(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMEPID:
			err = sna_cpic_fsm_cmepid(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMEPLN:
			err = sna_cpic_fsm_cmepln(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMESI:
			err = sna_cpic_fsm_cmesi(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMESL:
			err = sna_cpic_fsm_cmesl(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMESRM:
			err = sna_cpic_fsm_cmesrm(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMESUI:
			err = sna_cpic_fsm_cmesui(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMETC:
			err = sna_cpic_fsm_cmetc(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMETPN:
			err = sna_cpic_fsm_cmetpn(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMFLUS:
			err = sna_cpic_fsm_cmflus(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMINCL:
			err = sna_cpic_fsm_cmincl(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMINIC:
			err = sna_cpic_fsm_cminic(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMINIT:
			err = sna_cpic_fsm_cminit(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMPREP:
			err = sna_cpic_fsm_cmprep(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMPTR:
			err = sna_cpic_fsm_cmptr(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMRCV:
			switch (tp->receive_type) {
				case AC_RECEIVE_AND_WAIT:
					err = sna_cpic_fsm_cmrcv_w(cpic, input, rc,
						dr, sr, fd, readonly);
					break;
				case AC_RECEIVE_IMMEDIATE:
					err = sna_cpic_fsm_cmrcv_i(cpic, input, rc,
						dr, sr, fd, readonly);
					break;
			}
			break;
		case CM_CMRCVX:
			err = sna_cpic_fsm_cmrcvx(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMRTS:
			err = sna_cpic_fsm_cmrts(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSAC:
			err = sna_cpic_fsm_cmsac(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSACN:
			err = sna_cpic_fsm_cmsacn(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSAEQ:
			err = sna_cpic_fsm_cmsaeq(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSAPT:
			err = sna_cpic_fsm_cmsapt(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSBT:
			err = sna_cpic_fsm_cmsbt(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSCSP:
			err = sna_cpic_fsm_cmscsp(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSCST:
			err = sna_cpic_fsm_cmscst(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSCSU:
			err = sna_cpic_fsm_cmscsu(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSCT:
			err = sna_cpic_fsm_cmsct(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSCU:
			err = sna_cpic_fsm_cmscu(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSDT:
			err = sna_cpic_fsm_cmsdt(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSED:
			err = sna_cpic_fsm_cmsed(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSEND:
			err = sna_cpic_fsm_cmsend(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSERR:
			err = sna_cpic_fsm_cmserr(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSF:
			err = sna_cpic_fsm_cmsf(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSID:
			err = sna_cpic_fsm_cmsid(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSLD:
			err = sna_cpic_fsm_cmsld(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSMN:
			err = sna_cpic_fsm_cmsmn(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSNDX:
			err = sna_cpic_fsm_cmsndx(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSPDP:
			err = sna_cpic_fsm_cmspdp(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSPID:
			err = sna_cpic_fsm_cmspid(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSPLN:
			err = sna_cpic_fsm_cmspln(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSPM:
			err = sna_cpic_fsm_cmspm(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSPTR:
			err = sna_cpic_fsm_cmsptr(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSQCF:
			err = sna_cpic_fsm_cmsqcf(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSQPM:
			err = sna_cpic_fsm_cmsqpm(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSRC:
			err = sna_cpic_fsm_cmsrc(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSRT:
			err = sna_cpic_fsm_cmsrt(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSSL:
			err = sna_cpic_fsm_cmssl(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSSRM:
			err = sna_cpic_fsm_cmssrm(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSST:
			err = sna_cpic_fsm_cmsst(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSTC:
			err = sna_cpic_fsm_cmstc(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMSTPN:
			err = sna_cpic_fsm_cmstpn(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMTRTS:
			err = sna_cpic_fsm_cmtrts(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
		case CM_CMWAIT:
			err = sna_cpic_fsm_cmwait(cpic, input, rc, dr, sr, fd,
				readonly);
			break;
	}
	sna_debug(5, "fini: state:%d\n", cpic->state);
	return err;
}

static int sna_cpic_release_session(struct socket *sock)
{
	struct sock *sk = sock->sk;
	struct cpic *cpic = &cpic_pi(sk)->cpic;
	struct sna_tp_cb *tp = cpic->vi.sna;

	sna_debug(5, "init\n");
	list_del(&cpic->list);
	if (!tp)
		return 0;
	kfree(tp);
	cpic->vi.sna = NULL;
	return 0;
}

static CM_RETURN_CODE sna_cpic_create_session(struct sna_cpic_side_info *side,
	struct cpic *cpic)
{
	struct sna_tp_cb *tp;
	int err = 0;

	sna_debug(5, "init\n");
	cpic->side = side;
	if (!side)
		tp = sna_rm_start_tp(NULL, NULL, NULL, &err);
	else
		tp = sna_rm_start_tp(side->tp_name, side->mode_name,
			&side->netid_plu, &err);
	if (tp)
		cpic->vi.sna = tp;
	return err;
}

/**
 * Set_AE_Qualifier (cmsaeq) is used by a program to set the AE_qualifier,
 * AE_qualifier_length, and AE_qualifier_format characteristics for a
 * conversation. Set_AE_Qualifier overrides the current values that were
 * originally acquired from the side information using sym_dest_name.
 */
static CM_RETURN_CODE sna_cpic_cmsaeq(struct cpic *cpic, u_int32_t *ae_qualifier_ptr,
	u_int32_t *ae_qualifier_length_ptr, u_int32_t *ae_qualifier_format_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	u_int16_t ae_qualifier_length;
	u_int8_t ae_qualifier_format;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSAEQ, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(ae_qualifier_format_ptr, sizeof(u_int8_t),
		&ae_qualifier_format);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	err = sna_utok(ae_qualifier_length_ptr, sizeof(u_int16_t),
		&ae_qualifier_length);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (ae_qualifier_format != CM_DN && ae_qualifier_format != CM_INT_DIGITS) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	if (ae_qualifier_length < 1 || ae_qualifier_length > 1024) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = sna_utok(ae_qualifier_ptr, ae_qualifier_length, tp->ae_qualifier);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	err = CM_OK;
	tp->ae_qualifier_length = ae_qualifier_length;
	tp->ae_qualifier_format = ae_qualifier_format;
out:    sna_cpic_fsm_hd(cpic, CM_CMSAEQ, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

/* Set_Allocaet_Confirm (cmsac) is ued by a program to set the allocate_confirm
 * characteristic for a given conversation. Set_Allocate_Confirm overrides
 * the value that was assigned when the Initialize_Conversation call was
 * issued.
 */
static CM_RETURN_CODE sna_cpic_cmsac(struct cpic *cpic, u_int32_t *allocate_confirm_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	u_int8_t allocate_confirm;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSAC, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(allocate_confirm_ptr, sizeof(u_int8_t),
		&allocate_confirm);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (allocate_confirm != CM_ALLOCATE_NO_CONFIRM
		&& allocate_confirm != CM_ALLOCATE_CONFIRM) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = CM_OK;
	tp->allocate_confirm = allocate_confirm;
out:    sna_cpic_fsm_hd(cpic, CM_CMSAC, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

/**
 * Set_Application_Context_Name (cmsacn) is used by a program to set the
 * application_context_name and application_context_name_length characteristics
 * for a conversation. Set_Application_Context_Name overrides the current
 * values that were originally acquired from the side information using
 * sym_dest_name.
 */
static CM_RETURN_CODE sna_cpic_cmsacn(struct cpic *cpic,
	u_int32_t *app_context_name_ptr, u_int32_t *app_context_name_length_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	u_int16_t app_context_name_length;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSACN, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(app_context_name_length_ptr, sizeof(u_int16_t),
		&app_context_name_length);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (app_context_name_length < 1 || app_context_name_length > 256) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = sna_utok(app_context_name_ptr, app_context_name_length,
		tp->application_context_name);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	err = CM_OK;
	tp->application_context_length = app_context_name_length;
out:    sna_cpic_fsm_hd(cpic, CM_CMSACN, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

/**
 * Set_AP_Title (cmsapt) is used by a program to set the AP_title,
 * AP_title_length, and AP_title_format characteristics for a conversation.
 * Set_AP_Title overrides the current values that were originally acquired
 * from the side information using sym_dest_name.
 */
static CM_RETURN_CODE sna_cpic_cmsapt(struct cpic *cpic, u_int32_t *ap_title_ptr,
	u_int32_t *ap_title_length_ptr, u_int32_t *ap_title_format_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	u_int16_t ap_title_length;
	u_int8_t ap_title_format;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSAPT, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(ap_title_format_ptr, sizeof(u_int8_t),
		&ap_title_format);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	err = sna_utok(ap_title_length_ptr, sizeof(u_int16_t),
		&ap_title_length);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (ap_title_format != CM_DN && ap_title_format != CM_OID) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	if (ap_title_length < 1 || ap_title_length > 1024) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = sna_utok(ap_title_ptr, ap_title_length, tp->ap_title);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	err = CM_OK;
	tp->ap_title_length = ap_title_length;
	tp->ap_title_format = ap_title_format;
out:    sna_cpic_fsm_hd(cpic, CM_CMSAPT, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

/**
 * Set_Begin_Transaction (cmsbt) is used by a program to set the
 * begin_transaction characteristics for a given conversation.
 * Set_Begin_Transaction overrides the value that was assigned when the
 * Initialize_Conversation call was issued.
 */
static CM_RETURN_CODE sna_cpic_cmsbt(struct cpic *cpic,
	u_int32_t *begin_transaction_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	u_int8_t begin_transaction;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSBT, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(begin_transaction_ptr, sizeof(u_int8_t), &begin_transaction);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (begin_transaction != CM_BEGIN_IMPLICIT
		&& begin_transaction != CM_BEGIN_EXPLICIT) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = CM_OK;
	tp->begin_transaction = begin_transaction;
out:    sna_cpic_fsm_hd(cpic, CM_CMSBT, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

/**
 * Set_Conversation_Security_Password (cmscsp) is used by a program to set the
 * security_password and security_password_length characteristics for a
 * conversation. Set_Conversation_Security_Password overrides the current
 * values, which were originally acquired from the side information using
 * sym_dest_name.
 */
static CM_RETURN_CODE sna_cpic_cmscsp(struct cpic *cpic, u_int32_t *pw_ptr,
	u_int32_t *pw_length_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	u_int8_t pw_length;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSCSP, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(pw_length_ptr, sizeof(u_int8_t), &pw_length);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (pw_length > 10) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = sna_utok(pw_ptr, pw_length, tp->security_password);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	err = CM_OK;
	tp->security_password_length = pw_length;
out:    sna_cpic_fsm_hd(cpic, CM_CMSCSP, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

/**
 * Set_Conversation_Security_Type (cmscst) is used by a program to set the
 * conversation_security_type characteristic for a conversation.
 * Set_Conversation_Security_Type overrides the current value, which was
 * originally acquired from the side infromation using sym_dest_name.
 */
static CM_RETURN_CODE sna_cpic_cmscst(struct cpic *cpic, u_int32_t *conv_security_type_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	u_int8_t conv_security_type;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSCST, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(conv_security_type_ptr, sizeof(u_int8_t), &conv_security_type);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (conv_security_type != CM_SECURITY_NONE
		&& conv_security_type != CM_SECURITY_SAME
		&& conv_security_type != CM_SECURITY_PROGRAM
		&& conv_security_type != CM_SECURITY_PROGRAM_STRONG
		&& conv_security_type != CM_SECURITY_DISTRIBUTED
		&& conv_security_type != CM_SECURITY_MUTUAL) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = CM_OK;
	tp->conversation_security_type = conv_security_type;
out:    sna_cpic_fsm_hd(cpic, CM_CMSCST, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

/**
 * Set_Conversation_Security_User_ID (cmscsu) is used by a program to set the
 * security_user_ID and security_user_ID_length characteristics for a
 * conversation. Set_Conversation_Security_User_ID overrides the current values
 * which were originally acquired from the side information using sym_dest_name.
 */
static CM_RETURN_CODE sna_cpic_cmscsu(struct cpic *cpic, u_int32_t *user_id_ptr,
	u_int32_t *user_id_length_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	u_int8_t user_id_length;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSCSU, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(user_id_length_ptr, sizeof(u_int8_t), &user_id_length);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (user_id_length > 10) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = sna_utok(user_id_ptr, user_id_length, tp->security_user_id);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	err = CM_OK;
	tp->security_user_id_length = user_id_length;
out:    sna_cpic_fsm_hd(cpic, CM_CMSCSU, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

/**
 * Set_Conversation_Type (cmsct) is used by a program to set the
 * conversation_type characteristic for a given conversation. It overrides
 * the value that was assigned when the Initialize_Conversation or
 * Initialize_For_Incomming call was issued.
 */
static CM_RETURN_CODE sna_cpic_cmsct(struct cpic *cpic, __u32 *c_type_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	u_int8_t c_type;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSCT, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(c_type_ptr, sizeof(u_int8_t), &c_type);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (c_type != CM_BASIC_CONVERSATION
		&& c_type != CM_MAPPED_CONVERSATION) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = CM_OK;
	tp->conversation_type = c_type;
out:	sna_cpic_fsm_hd(cpic, CM_CMSCT, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

/**
 * Set_Confirmation_Urgency (cmscu) is used by a program to set the
 * confirm_urgency characteristic for a given conversation.
 * Set_Confirmation_Urgency overrides the value that was assigned when the
 * Initialize_Conversation, Accept_Conversation, or Initialize_For_Incoming
 * call was issued.
 */
static CM_RETURN_CODE sna_cpic_cmscu(struct cpic *cpic, u_int32_t *conf_urgency_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	u_int8_t conf_urgency;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSCU, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(conf_urgency_ptr, sizeof(u_int8_t), &conf_urgency);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (conf_urgency != CM_CONFIRMATION_NOT_URGENT
		&& conf_urgency != CM_CONFIRMATION_URGENT) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = CM_OK;
	tp->confirm_urgency = conf_urgency;
out:    sna_cpic_fsm_hd(cpic, CM_CMSCU, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

/**
 * Set_Deallocaet_Type (cmsdt) is used by a prgoram to set the deallocate_type
 * characteristic for a given conversation. Set_Deallocate_Type overrides
 * the value that was assigned when the Initialize_Converation,
 * Accept_Conversation, or Initialize_For_Incoming call was issues.
 */
static CM_RETURN_CODE sna_cpic_cmsdt(struct cpic *cpic,
	u_int32_t *deallocate_type_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	u_int8_t deallocate_type;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSDT, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(deallocate_type_ptr, sizeof(u_int8_t), &deallocate_type);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (deallocate_type != CM_DEALLOCATE_SYNC_LEVEL
		&& deallocate_type != CM_DEALLOCATE_FLUSH
		&& deallocate_type != CM_DEALLOCATE_CONFIRM
		&& deallocate_type != CM_DEALLOCATE_ABEND) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = CM_OK;
	tp->deallocate_type = deallocate_type;
out:    sna_cpic_fsm_hd(cpic, CM_CMSDT, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

/**
 * Set_Error_Direction (cmsed) is used by a program to set the error_direction
 * characteristic for a give conversation. Set_Error_Direction overrides the
 * value that was assigned when he Initialize_Converation,
 * Accept_Conversation, or Initialize_For_Incoming call was issued.
 */
static CM_RETURN_CODE sna_cpic_cmsed(struct cpic *cpic,
	u_int32_t *error_direction_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	u_int8_t error_direction;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSED, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(error_direction_ptr, sizeof(u_int8_t), &error_direction);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (error_direction != CM_RECEIVE_ERROR
		&& error_direction != CM_SEND_ERROR) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = CM_OK;
	tp->error_direction = error_direction;
out:    sna_cpic_fsm_hd(cpic, CM_CMSED, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

/**
 * A program uses the Send_Data (cmsend) call to send data to the remote
 * program. When issued durning a mapped conversation, this call sends
 * one data record to the remote program. The data record consists entirely
 * of data and is not examined by the system for possible logical records.
 *
 * When issued during a basic conversation, this call sends data to the
 * remote program. The data consists of logical records. The amount of data
 * specified is independently of the data format.
 */
static CM_RETURN_CODE sna_cpic_cmsend(struct cpic *cpic, u_int32_t *buffer,
	u_int32_t *send_length_ptr, u_int32_t *control_information_received)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	u_int16_t send_length, len;
	struct sk_buff *skb;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSEND, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(send_length_ptr, sizeof(u_int16_t), &send_length);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (send_length > AC_MAX_BUFFER) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	sna_debug(5, "send_length=%d\n", send_length);
	len = send_length;
	if (tp->conversation_type == AC_CONVERSATION_TYPE_MAPPED)
		len += 4;
	skb = sna_alloc_skb(len, GFP_ATOMIC);
	if (!skb) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (tp->conversation_type == AC_CONVERSATION_TYPE_MAPPED) {
		u_int16_t *ll_len, *ll_gds;
		ll_len = (u_int16_t *)skb_put(skb, sizeof(u_int16_t));
		ll_gds = (u_int16_t *)skb_put(skb, sizeof(u_int16_t));
		*ll_len = htons(len);
		if (len > 4)
			*ll_gds = htons(SNA_GDS_APP_DATA);
		else
			*ll_gds = htons(SNA_GDS_NULL_DATA);
	}
	if (send_length) {
		err = sna_utok(buffer, send_length, skb_put(skb, send_length));
		if (err < 0) {
			kfree_skb(skb);
			err = CM_PRODUCT_SPECIFIC_ERROR;
			goto out;
		}
	}
	skb_queue_tail(&tp->write_queue, skb);
	err = sna_ps_verb_router(SEND_DATA, tp);
	err = sna_cpic_appc2cpic_rc(err);
out:	sna_cpic_fsm_hd(cpic, CM_CMSEND, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

/**
 * A program uses the Receive (cmrcv) call to receive information from a given
 * conversation. The information received can be a data record (on a mapped
 * conversation), data (on a basic converation), conversation status, or a
 * request for confirmation or for resource recovery services.
 */
static CM_RETURN_CODE sna_cpic_cmrcv(struct cpic *cpic, u_int32_t *buffer,
	u_int32_t *requested_length_ptr, u_int32_t *data_received,
	u_int32_t *received_length, u_int32_t *status_received,
	u_int32_t *control_information_received)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	u_int16_t requested_length;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMRCV, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(requested_length_ptr, sizeof(u_int16_t), &requested_length);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (requested_length > AC_MAX_BUFFER) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	tp->rx_rcv_len	= 0;
	tp->rx_req_len 	= requested_length;
	tp->rx_buffer	= buffer;
	if (tp->receive_type == AC_RECEIVE_AND_WAIT)
		err = sna_ps_verb_router(RECEIVE_AND_WAIT, tp);
	else
		err = sna_ps_verb_router(RECEIVE_IMMEDIATE, tp);
	err = sna_cpic_appc2cpic_rc(err);
	if (err) {
		sna_debug(5, "verb failed `%d'.\n", err);
		goto out;
	}
	err = sna_ktou(&tp->data_received, sizeof(u_int8_t), data_received);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	err = sna_ktou(&tp->rx_rcv_len, sizeof(u_int32_t), received_length);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
out:    sna_cpic_fsm_hd(cpic, CM_CMRCV, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

static CM_RETURN_CODE sna_cpic_cmserr(struct cpic *c, __u32 *control_information_recevied)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;
}

/**
 * Set_Fill (cmsf) is used by a program to set the fill characteristic for
 * a given conversation. Set_Fill overrides the value that was assigned when
 * the Initialize_Converation, Accept_Conversation, or
 * Initialize_For_Incoming call was issued.
 */
static CM_RETURN_CODE sna_cpic_cmsf(struct cpic *cpic, u_int32_t *fill_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	CM_FILL fill;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSF, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(fill_ptr, sizeof(CM_FILL), &fill);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (fill != CM_FILL_LL && fill != CM_FILL_BUFFER) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = CM_OK;
	tp->fill = fill;
out:    sna_cpic_fsm_hd(cpic, CM_CMSF, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

/**
 * Set_Initialization_Data (cmsid) is used by a program to set the
 * initialization_data and initialization_data_length conversation
 * characterisitics to be sent to the remote program for a given conversation.
 * Set_Initialization_Data overrides the values that were assigned when the
 * Initialize_Converation, Accept_Conversation, or Initialize_For_Incoming
 * call was issued.
 */
static CM_RETURN_CODE sna_cpic_cmsid(struct cpic *cpic, u_int32_t *init_data_ptr,
	u_int32_t *init_data_length_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	u_int32_t init_data_length;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSID, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(init_data_length_ptr, sizeof(u_int32_t), &init_data_length);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (init_data_length > 10000) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = sna_utok(init_data_ptr, init_data_length, tp->init_data);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	err = CM_OK;
	tp->init_data_length = init_data_length;
out:    sna_cpic_fsm_hd(cpic, CM_CMSID, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

/**
 * Set_Log_Data (cmsld) is used by a program to set the log_data and
 * log_data_length charateristics for a given conversation. Set_Log_Data
 * overrides the values that were assigned when the
 * Initialize_Converation, Accept_Conversation, or Initialize_For_Incoming
 * call was issued.
 */
static CM_RETURN_CODE sna_cpic_cmsld(struct cpic *cpic, u_int32_t *log_data_ptr,
	u_int32_t *log_data_length_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	u_int32_t log_data_length;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSLD, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(log_data_length_ptr, sizeof(u_int32_t), &log_data_length);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (log_data_length > 512) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = sna_utok(log_data_ptr, log_data_length, tp->log_data);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	err = CM_OK;
	tp->log_data_length = log_data_length;
out:    sna_cpic_fsm_hd(cpic, CM_CMSLD, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

/**
 * Set_Mode_Name (cmsmn) is used by a program to set the mode_name and
 * mode_name-Length characteristics for a conversation. Set_Mode_Name
 * overrides the current values that were originally acquired from the side
 * information using the sym_dest_name.
 */
static CM_RETURN_CODE sna_cpic_cmsmn(struct cpic *cpic,
	u_int32_t *mode_name_ptr, u_int32_t *mode_name_length_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	u_int32_t mode_name_length;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSMN, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(mode_name_length_ptr, sizeof(u_int32_t),
		&mode_name_length);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (mode_name_length > 8) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = sna_utok(mode_name_ptr, mode_name_length, tp->mode_name);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	err = CM_OK;
	tp->mode_name_length = mode_name_length;
out:    sna_cpic_fsm_hd(cpic, CM_CMSMN, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

static CM_RETURN_CODE sna_cpic_cmsndx(struct cpic *c, __u32 *buffer, __u32 *send_length,
	__u32 *control_information_received)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;
}

/**
 * Set_Prepare_Data_Permitted (cmspdp) is used by a program to set the
 * prepare_data_permitted chartacteristic for a given conversation.
 * Set_Prepare_Data_Permitted overrides the value that was assigned when the
 * Initialize_Conversation call was issued. The subordinate program on the
 * converation cannot issue the Set_Prepare_Data_Permitted call.
 */
static CM_RETURN_CODE sna_cpic_cmspdp(struct cpic *cpic,
	u_int32_t *prepare_data_permitted_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	CM_PREPARE_DATA_PERMITTED_TYPE prepare_data_permitted;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSPDP, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(prepare_data_permitted_ptr,
		sizeof(CM_PREPARE_DATA_PERMITTED_TYPE),
		&prepare_data_permitted);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (prepare_data_permitted != CM_PREPARE_DATA_NOT_PERMITTED
		&& prepare_data_permitted != CM_PREPARE_DATA_PERMITTED) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = CM_OK;
	tp->prepare_data_permitted = prepare_data_permitted;
out:    sna_cpic_fsm_hd(cpic, CM_CMSPDP, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

/**
 * Programs use the Set_Parter_ID (cmspid) call to specify a partner_ID
 * characteristic that will be used to allocate the conversation. Set_Partner_ID
 * overrides the current values for the partner_ID, partner_ID_length,
 * partner_ID_tytpe, and partner_ID_scope characteristics that were originally
 * acquired from the side infromation using the sym_dest_name.
 *
 */
static CM_RETURN_CODE sna_cpic_cmspid(struct cpic *cpic,
	u_int32_t *partner_id_type_ptr, u_int32_t *partner_id_ptr,
	u_int32_t *partner_id_length_ptr, u_int32_t *partner_id_scope_ptr,
	u_int32_t *directory_syntax_ptr, u_int32_t *directory_encoding_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	u_int32_t partner_id_type, partner_id_length, partner_id_scope;
	u_int32_t directory_syntax, directory_encoding;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSPID, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(partner_id_type_ptr, sizeof(u_int32_t),
		&partner_id_type);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	err = sna_utok(partner_id_length_ptr, sizeof(u_int32_t),
		&partner_id_length);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	err = sna_utok(partner_id_scope_ptr, sizeof(u_int32_t),
		&partner_id_scope);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	err = sna_utok(directory_syntax_ptr, sizeof(u_int32_t),
		&directory_syntax);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	err = sna_utok(directory_encoding_ptr, sizeof(u_int32_t),
		&directory_encoding);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (partner_id_type != CM_DISTINGUISHED_NAME
		&& partner_id_type != CM_LOCAL_DISTINGUISHED_NAME
		&& partner_id_type != CM_PROGRAM_FUNCTION_ID
		&& partner_id_type != CM_OSI_TPSU_TITLE_OID
		&& partner_id_type != CM_PROGRAM_BINDING) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	if (partner_id_length > 32767) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	if (partner_id_scope != CM_EXPLICIT
		&& partner_id_scope != CM_REFERENCE) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	if (directory_syntax != CM_DEFAULT_SYNTAX
		&& directory_syntax != CM_DCE_SYNTAX
		&& directory_syntax != CM_XDS_SYNTAX
		&& directory_syntax != CM_NDS_SYNTAX) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	if (directory_encoding != CM_DEFAULT_ENCODING
		&& directory_encoding != CM_UNICODE_ENCODING) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	new_s(tp->partner_id, partner_id_length, GFP_ATOMIC);
	if (!tp->partner_id) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	err = sna_utok(partner_id_ptr, partner_id_length, tp->partner_id);
	if (err < 0) {
		kfree(tp->partner_id);
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	err = CM_OK;
	tp->partner_id_length	= partner_id_length;
	tp->partner_id_type	= partner_id_type;
	tp->partner_id_scope	= partner_id_scope;
	tp->directory_encoding	= directory_encoding;
	tp->directory_syntax	= directory_syntax;
out:    sna_cpic_fsm_hd(cpic, CM_CMSPID, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

/**
 * Set_Partner_LU_Name (cmspln) is used by a program to set the partner_lu_name
 * and parnter_LU_name_length characteristics for a conversation.
 * Set_Partner_LU_Name overrides the current values that were originally
 * acquired from the side infromation using the sym_dest_name.
 */
static CM_RETURN_CODE sna_cpic_cmspln(struct cpic *cpic,
	u_int32_t *partner_lu_name_ptr, u_int32_t *partner_lu_name_length_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	u_int32_t partner_lu_name_length;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSPLN, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(partner_lu_name_length_ptr, sizeof(u_int32_t),
		&partner_lu_name_length);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (partner_lu_name_length < 1 || partner_lu_name_length > 17) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = sna_utok(partner_lu_name_ptr, partner_lu_name_length,
		tp->partner_lu_name);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	err = CM_OK;
	tp->partner_lu_name_length = partner_lu_name_length;
out:    sna_cpic_fsm_hd(cpic, CM_CMSPLN, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

/**
 * A program used the Set_Processing_Mode (cmspm) call to set the
 * processing_mode characteristic of a conversation. The processing_mode
 * characteristic indicates whether CPI Communications calls on the specified
 * conversation are to be processed in blocking or conversation-level
 * non-blocking mode. Set_Processing_Mode overrides the default value of
 * CM_BLOCKING that was assigned when the INitialize_Conversation,
 * Initialize_For_Incoming, or Accept_Conversation call was issued. The
 * processing mode of a conversation connot be changed prior to the completion
 * of all previous call operations on that conversation.
 */
static CM_RETURN_CODE sna_cpic_cmspm(struct cpic *cpic,
	u_int32_t *processing_mode_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	CM_PROCESSING_MODE processing_mode;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSPM, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(processing_mode_ptr,
		sizeof(CM_PROCESSING_MODE), &processing_mode);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (processing_mode != CM_BLOCKING
		&& processing_mode != CM_NON_BLOCKING) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = CM_OK;
	tp->processing_mode = processing_mode;
out:    sna_cpic_fsm_hd(cpic, CM_CMSPM, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

/**
 * Set_Prepare_To_Receive (cmsptr) is used by a program to set the
 * prepare_to_receive_type characteristic for a conversation. This call
 * overrides the value that was assigned when the Initialize_Conversation,
 * Accept_Conversation, or Initialize_For_Incoming call was issued.
 */
static CM_RETURN_CODE sna_cpic_cmsptr(struct cpic *cpic,
	u_int32_t *prepare_to_receive_type_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	CM_PREPARE_TO_RECEIVE_TYPE prepare_to_receive_type;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSPTR, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(prepare_to_receive_type_ptr,
		sizeof(CM_PREPARE_TO_RECEIVE_TYPE), &prepare_to_receive_type);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (prepare_to_receive_type != CM_PREP_TO_RECEIVE_SYNC_LEVEL
		&& prepare_to_receive_type != CM_PREP_TO_RECEIVE_FLUSH
		&& prepare_to_receive_type != CM_PREP_TO_RECEIVE_CONFIRM) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = CM_OK;
	tp->prepare_to_receive_type = prepare_to_receive_type;
out:    sna_cpic_fsm_hd(cpic, CM_CMSPTR, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

static CM_RETURN_CODE sna_cpic_cmsqcf(struct cpic *cpic,
	u_int32_t *conversation_queue_ptr,
	u_int32_t *callback_function_ptr,
	u_int32_t *callback_info_ptr,
	u_int32_t *user_field_ptr)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;
}

static CM_RETURN_CODE sna_cpic_cmcanc(struct cpic *c)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;
}

static CM_RETURN_CODE sna_cpic_cmprep(struct cpic *c)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;
}

static CM_RETURN_CODE sna_cpic_cmsqpm(struct cpic *cpic,
	u_int32_t *conversation_queue_ptr,
	u_int32_t *queue_processing_mode_ptr,
	u_int32_t *user_field_ptr, u_int32_t *ooid_ptr)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;
}

/**
 * Set_Return_Control (cmsrc) is used to set the return_control characteristic
 * for a given conversation. Set_Return_Control overrides the value that was
 * assigned when the Initialize_Conversation call was issued.
 */
static CM_RETURN_CODE sna_cpic_cmsrc(struct cpic *cpic, u_int32_t *return_control_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	CM_RETURN_CONTROL return_control;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSRC, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(return_control_ptr, sizeof(CM_RETURN_CONTROL),
		&return_control);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (return_control != CM_WHEN_SESSION_ALLOCATED
		&& return_control != CM_IMMEDIATE
		&& return_control != CM_WHEN_CONWINNER_ALLOCATED
		&& return_control != CM_WHEN_SESSION_FREE) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = CM_OK;
	tp->return_control = return_control;
out:    sna_cpic_fsm_hd(cpic, CM_CMSRC, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

/**
 * Set_Receive_Type (cmsrt) is used by a program to set the receive_type
 * characteristic for a conversation. Set_Receive_Type overrides the value
 * that was assigned when the Initialize_Converation, Accept_Conversation, or
 * Initialize_For_Incoming call was issued.
 */
static CM_RETURN_CODE sna_cpic_cmsrt(struct cpic *cpic, u_int32_t *receive_type_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	CM_RECEIVE_TYPE receive_type;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSRT, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(receive_type_ptr, sizeof(CM_RECEIVE_TYPE), &receive_type);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (receive_type != CM_RECEIVE_AND_WAIT
		&& receive_type != CM_RECEIVE_IMMEDIATE) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = CM_OK;
	tp->receive_type = receive_type;
out:    sna_cpic_fsm_hd(cpic, CM_CMSRT, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

/**
 * Set_Sync_Level (cmssl) is used by a program to set the sync_level
 * characteristic for a given conversation. The sync_level characteristic
 * is used to specify the level of synchronization processing between the two
 * programs. It determines whether the programs support no synchronization,
 * confirmation-level synchronization (using the Confirm and Confirmed CPI
 * Communicatinons calls), or sync-point-level synchronization (using the calls
 * of a resource recovery interface). Set_Sync_Level overrides the value that
 * was assigned when the Initialize_Converation call was issued.
 */
static CM_RETURN_CODE sna_cpic_cmssl(struct cpic *cpic,
	u_int32_t *sync_level_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	CM_SYNC_LEVEL sync_level;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSSL, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(sync_level_ptr, sizeof(CM_SYNC_LEVEL), &sync_level);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (sync_level != CM_NONE && sync_level != CM_CONFIRM
		&& sync_level != CM_SYNC_POINT
		&& sync_level != CM_SYNC_POINT_NO_CONFIRM) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = CM_OK;
	tp->sync_level = sync_level;
out:    sna_cpic_fsm_hd(cpic, CM_CMSSL, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

/**
 * The Set_Send_Receive_Mode (cmssrm) call is used by a program to set the
 * send_receive_mode characterisitic for a conversation. Set_Send_receive_mode
 * overrides the value that was assgined when the Initialize_conversation call
 * was issued.
 */
static CM_RETURN_CODE sna_cpic_cmssrm(struct cpic *cpic,
	u_int32_t *send_receive_mode_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	CM_SEND_RECEIVE_MODE send_receive_mode;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSSRM, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(send_receive_mode_ptr, sizeof(CM_SEND_RECEIVE_MODE),
		&send_receive_mode);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (send_receive_mode != CM_HALF_DUPLEX
		&& send_receive_mode != CM_FULL_DUPLEX) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = CM_OK;
	tp->send_receive_mode = send_receive_mode;
out:    sna_cpic_fsm_hd(cpic, CM_CMSSRM, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

/**
 * Set_Send_Type (cmsst) is used by a program to set the send_type characteristic
 * for a conversation. Set_Send_Type overrides the value that was assigned when
 * the Initialize_Converation, Accept_Conversation, or Initialize_For_Incoming
 * call was issued.
 */
static CM_RETURN_CODE sna_cpic_cmsst(struct cpic *cpic, u_int32_t *send_type_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	CM_SEND_TYPE send_type;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSST, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(send_type_ptr, sizeof(CM_SEND_TYPE), &send_type);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (send_type != CM_BUFFER_DATA
		&& send_type != CM_SEND_AND_FLUSH
		&& send_type != CM_SEND_AND_CONFIRM
		&& send_type != CM_SEND_AND_PREP_TO_RECEIVE
		&& send_type != CM_SEND_AND_DEALLOCATE) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = CM_OK;
	tp->send_type = send_type;
out:    sna_cpic_fsm_hd(cpic, CM_CMSST, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

/**
 * Set_Transaction_Control (cmstc) is used by a program to set the
 * transaction_control characterisitic for a given converation.
 * Set_Transaction_Control overrides the value that was assigned when the
 * Initialize_Conversation call was issued.
 */
static CM_RETURN_CODE sna_cpic_cmstc(struct cpic *cpic,
	u_int32_t *transaction_control_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	CM_TRANSACTION_CONTROL transaction_control;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSTC, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(transaction_control_ptr, sizeof(CM_TRANSACTION_CONTROL),
		&transaction_control);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (transaction_control != CM_CHAINED_TRANSACTIONS
		&& transaction_control != CM_UNCHAINED_TRANSACTIONS) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = CM_OK;
	tp->transaction_control = transaction_control;
out:    sna_cpic_fsm_hd(cpic, CM_CMSTC, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

/**
 * Set_TP_Name (cmstpn) is used by a program that initiates a converation,
 * using the Initialize_Converation call, to set the TP_name and TP_name_length
 * characteristics for a given converation. Set_TP_Name overrides the curent
 * values that were originally acquired from the side information using the
 * sym_dest_name.
 */
static CM_RETURN_CODE sna_cpic_cmstpn(struct cpic *cpic, u_int32_t *tp_name_ptr,
	u_int32_t *tp_name_length_ptr)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	u_int32_t tp_name_length;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMSTPN, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}
	err = sna_utok(tp_name_length_ptr, sizeof(u_int32_t), &tp_name_length);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (tp_name_length < 1 || tp_name_length > 64) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	err = sna_utok(tp_name_ptr, tp_name_length, tp->tp_name);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	err = CM_OK;
	tp->tp_name_length = tp_name_length;
out:    sna_cpic_fsm_hd(cpic, CM_CMSTPN, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

static CM_RETURN_CODE sna_cpic_cmeaeq(struct cpic *cpic, __u32 *ae_qualifier,
	__u32 *ae_qualifier_length, __u32 *ap_title_format)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;

#ifdef NOT
	struct sna_tp_cb *tp = cpic->vi.sna;

	sna_debug(5, "init\n");
	if (cpic->state != CM_INIT_INCOMING)
		return CM_PROGRAM_STATE_CHECK;
	sna_ktou(&tp->ae_qualifier, tp->ae_qualifier_length, ae_qualifier);
	sna_ktou(&tp->ae_qualifier_length, sizeof(tp->ae_qualifier_length),
		ae_qualifier_length);
	sna_ktou(&tp->ae_qualifier_format, sizeof(tp->ae_qualifier_format),
		ap_title_format);
	return CM_OK;
#endif
}

static CM_RETURN_CODE sna_cpic_cmeapt(struct cpic *cpic, __u32 *ap_title, __u32 *ap_title_length,
	__u32 *ap_title_format)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;

#ifdef NOT
	struct sna_tp_cb *tp = cpic->vi.sna;

	sna_debug(5, "init\n");
	if (cpic->state != CM_INIT_INCOMING)
		return CM_PROGRAM_STATE_CHECK;
	sna_ktou(&tp->ap_title, tp->ap_title_length, ap_title);
	sna_ktou(&tp->ap_title_length, sizeof(tp->ap_title_length),
		ap_title_length);
	sna_ktou(&tp->ap_title_format, sizeof(tp->ap_title_format),
		ap_title_format);
	return CM_OK;
#endif
}

static CM_RETURN_CODE sna_cpic_cmeacn(struct cpic *cpic, __u32 *application_context_name,
	__u32 *appl_context_name_length)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;

#ifdef NOT
	struct sna_tp_cb *tp = cpic->vi.sna;

	sna_debug(5, "init\n");
	if (cpic->state != CM_INIT_INCOMING)
		return CM_PROGRAM_STATE_CHECK;
	sna_ktou(&tp->application_context_name,
		tp->application_context_length, application_context_name);
	sna_ktou(&tp->application_context_length,
		sizeof(tp->application_context_length),
		appl_context_name_length);
	return CM_OK;
#endif
}

static CM_RETURN_CODE sna_cpic_cmecs(struct cpic *cpic, __u32 *conversation_state)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;

#ifdef NOT
	struct sna_tp_cb *tp = cpic->vi.sna;

	sna_debug(5, "init\n");
	sna_ktou(&cpic->state, sizeof(cpic->state), conversation_state);
	return CM_OK;
#endif
}

static CM_RETURN_CODE sna_cpic_cmect(struct cpic *c, __u32 *conversation_type)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;
}

static CM_RETURN_CODE sna_cpic_cmectx(struct cpic *c, __u32 *context_id, __u32 *context_id_length)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;
}

static CM_RETURN_CODE sna_cpic_cmeid(struct cpic *cpic, __u32 *initialization_data,
	__u32 *requested_length, __u32 *initialization_data_length)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;

#ifdef NOT
	struct sna_tp_cb *tp = cpic->vi.sna;
	CM_INT32 len;

	sna_debug(5, "init\n");
	sna_utok(requested_length, sizeof(CM_INT32), &len);
	if (len < 0)
		return CM_PROGRAM_PARAMETER_CHECK;
	if (cpic->state != CM_INIT_INCOMING)
		return CM_PROGRAM_STATE_CHECK;
	sna_ktou(&tp->init_data, len, initialization_data);
	sna_ktou(&len, sizeof(len), initialization_data_length);
	return CM_OK;
#endif
}

static CM_RETURN_CODE sna_cpic_cmemn(struct cpic *cpic, __u32 *mode_name, __u32 *mode_name_length)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;

#ifdef NOT
	struct sna_tp_cb *tp = cpic->vi.sna;

	sna_debug(5, "init\n");
	if (cpic->state != CM_INIT_INCOMING)
		return CM_PROGRAM_STATE_CHECK;
	sna_ktou(&tp->mode_name, tp->mode_name_length, mode_name);
	sna_ktou(&tp->mode_name_length, sizeof(tp->mode_name_length),
		mode_name_length);
	return CM_OK;
#endif
}

static CM_RETURN_CODE sna_cpic_cmepid(struct cpic *c, __u32 *partner_id_type, __u32 *partner_id,
	__u32 *requested_length, __u32 *partner_id_length,
	__u32 *partner_id_scope, __u32 *directory_syntax,
	__u32 *directory_encoding)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;
}

static CM_RETURN_CODE sna_cpic_cmepln(struct cpic *c, __u32 *partner_lu_name,
	__u32 *partner_lu_name_length)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;

#ifdef NOT
	struct sna_tp_cb *tp = c->vi.sna;

	sna_debug(5, "init\n");

/*
	if(cpic->state != CM_INIT_INCOMING)
		return (CM_PROGRAM_STATE_CHECK);

	sna_ktou(&cpic->partner_lu_name, cpic->partner_lu_name_length,
		partner_lu_name);
	sna_ktou(&cpic->partner_lu_name_length,
		sizeof(cpic->partner_lu_name_length), partner_lu_name_length);
*/
	return CM_OK;
#endif
}

static CM_RETURN_CODE sna_cpic_cmesi(struct cpic *c, __u32 *call_id, __u32 *buffer,
	__u32 *requested_length, __u32 *data_received)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;
}

static CM_RETURN_CODE sna_cpic_cmesl(struct cpic *c, __u32 *sync_level)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;
}

static CM_RETURN_CODE sna_cpic_cmesrm(struct cpic *c, __u32 *send_receive_mode)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;
}

static CM_RETURN_CODE sna_cpic_cmesui(struct cpic *c, __u32 *user_id, __u32 *user_id_length)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;
}

static CM_RETURN_CODE sna_cpic_cmetc(struct cpic *c, __u32 *transaction_control)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;
}

static CM_RETURN_CODE sna_cpic_cmetpn(struct cpic *cpic, __u32 *tp_name, __u32 *tp_name_length)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;

#ifdef NOT
	struct sna_tp_cb *tp = cpic->vi.sna;

	sna_debug(5, "init\n");
	if (cpic->state != CM_INIT_INCOMING)
		return CM_PROGRAM_STATE_CHECK;
	sna_ktou(&tp->tp_name, tp->tp_name_length, tp_name);
	sna_ktou(&tp->tp_name_length, sizeof(tp->tp_name_length),
		tp_name_length);
	return CM_OK;
#endif
}

/**
 * A program uses the Allocate (cmallc) call to establish a basic or mapped
 * conversation (depending on the conversation_type characteristic) with its
 * partner program. The partner program is specified in the TP_name
 * characteristics.
 */
static CM_RETURN_CODE sna_cpic_cmallc(struct cpic *cpic)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	int err;

	sna_debug(5, "init\n");
	if (sna_cpic_fsm_hd(cpic, CM_CMALLC, CM_OK, tp->data_received,
		tp->status_received, tp->send_receive_mode, 1)) {
		err = CM_PROGRAM_STATE_CHECK;
		goto out;
	}

	/* ensure user has set required information. */
	if (!tp->mode_name_length) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	if (!tp->tp_name_length) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	if (!tp->partner_lu_name_length) {
		err = CM_PROGRAM_PARAMETER_CHECK;
		goto out;
	}
	if (tp->conversation_security_type == AC_SECURITY_PROGRAM) {
		if (!tp->security_password_length) {
			err = CM_PROGRAM_PARAMETER_CHECK;
			goto out;
		}
		if (!tp->security_user_id_length) {
			err = CM_PROGRAM_PARAMETER_CHECK;
			goto out;
		}
	}
	err = sna_ps_verb_router(ALLOCATE, tp);
	err = sna_cpic_appc2cpic_rc(err);
out:	sna_cpic_fsm_hd(cpic, CM_CMALLC, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
	return err;
}

static CM_RETURN_CODE sna_cpic_cmcfmd(struct cpic *c)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;

#ifdef NOT
	struct sna_tp_cb *tp = c->vi.sna;

	sna_debug(5, "init\n");
	return CM_OK;
#endif
}

static CM_RETURN_CODE sna_cpic_cmdfde(struct cpic *c)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;
}

static CM_RETURN_CODE sna_cpic_cmacci(struct cpic *c)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;
}

static CM_RETURN_CODE sna_cpic_cmaccp(struct cpic *cpic, pid_t *p)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;

#ifdef NOT
	struct sna_tp_cb *tp;
	__u32 rc;

	/* Create all of local structures for CPIC to exist. */
	rc = sna_cpic_create_session(NULL, cpic);
	if (rc != CM_OK)
		goto out;

	/* Now this is a local (server) tp being launched.
	 * So we need to located the already allocated tcb and link
	 * it to this cpic session. We do this by using the pid
	 * provided by the user.
	 */
	sna_utok(p, sizeof(pid_t), &cpic->pid);
	tp = sna_rm_tp_get_by_pid(cpic->pid);
	if (!tp) {
		sna_debug(5, "No Tp found by PID %d\n", cpic->pid);
		rc = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	cpic->vi.sna = tp;
	cpic->state = CM_INIT_INCOMING;
out:    return rc;
#endif
}

static CM_RETURN_CODE sna_cpic_cmwait(struct cpic *c, __u32 *a1)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;
}

static CM_RETURN_CODE sna_cpic_cmflus(struct cpic *c)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;
}

static CM_RETURN_CODE sna_cpic_cmincl(struct cpic *c)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;
}

static CM_RETURN_CODE sna_cpic_cmptr(struct cpic *c)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;
}

static CM_RETURN_CODE sna_cpic_cmrts(struct cpic *c)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;
}

static CM_RETURN_CODE sna_cpic_cmtrts(struct cpic *c, __u32 *a1)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;
}

static CM_RETURN_CODE sna_cpic_cminic(struct cpic *c)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;
}

/**
 * A program uses the Initialize_Conversation (cminit) call to initialize
 * values for various conversation characteristics before the conversation
 * is allocated (with a call to Allocate). The remote partner program uses
 * the Accept_Conversation call or the Initialize_Incoming and
 * Accept_Incoming calls to initialize values for the conversation
 * charateristics on its end of the conversation.
 */
static CM_RETURN_CODE sna_cpic_cminit(struct cpic *cpic,
	unsigned char CM_PTR sym_dest_name_ptr)
{
	unsigned char blank_sym_dest[CM_SDN_SIZE + 1] = "        ";
	unsigned char sym_dest_name[CM_SDN_SIZE + 1];
	struct sna_cpic_side_info *c_side = NULL;
	int err, blank = 1;
	struct sna_tp_cb *tp;

	sna_debug(5, "init\n");
	memset(&sym_dest_name, '\0', CM_SDN_SIZE + 1);
	err = sna_utok(sym_dest_name_ptr, CM_SDN_SIZE, &sym_dest_name);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}
	if (memcmp(&sym_dest_name, &blank_sym_dest, CM_SDN_SIZE)) {
		c_side = sna_cpic_side_info_get_by_name(sym_dest_name);
		if (!c_side) {
			err = CM_PROGRAM_PARAMETER_CHECK;
			goto out;
		}
		blank = 0;
	}
	err = sna_cpic_create_session(c_side, cpic);
	if (err < 0) {
		err = CM_PRODUCT_SPECIFIC_ERROR;
		goto out;
	}

	/* set conversation defaults. */
	err = CM_OK;
	tp  = cpic->vi.sna;

	memset(tp->ae_qualifier, '\0', 1024);
	tp->ae_qualifier_length 	= 0;
	tp->ae_qualifier_format 	= 0;

	tp->allocate_confirm 		= AC_ALLOCATE_NO_CONFIRM;

	memset(tp->ap_title, '\0', 1024);
	tp->ap_title_length 		= 0;
	tp->ap_title_format 		= 0;

	memset(tp->application_context_name, '\0', 256);
	tp->application_context_length 	= 0;

	tp->begin_transaction 		= AC_BEGIN_IMPLICIT;

	tp->confirm_urgency		= AC_CONFIRMATION_URGENT;

	tp->conversation_security_type	= AC_SECURITY_SAME;

	tp->conversation_type		= AC_CONVERSATION_TYPE_MAPPED;

	tp->deallocate_type		= AC_DEALLOCATE_SYNC_LEVEL;

	tp->directory_encoding		= AC_DEFAULT_ENCODING;

	tp->directory_syntax		= AC_DEFAULT_SYNTAX;

	tp->error_direction		= AC_RECEIVE_ERROR;

	tp->fill			= AC_FILL_LL;

	memset(tp->init_data, '\0', 10000);
	tp->init_data_length		= 0;

	memset(tp->log_data, '\0', 512);
	tp->log_data_length		= 0;

	tp->join_transaction		= 0;

	tp->map_name			= NULL;
	tp->map_name_length		= 0;

	memset(tp->mode_name, '\0', 9);
	tp->mode_name_length		= 0;

	tp->partner_id			= NULL;
	tp->partner_id_length		= 0;

	tp->partner_id_type		= AC_DISTINGUISHED_NAME;

	tp->partner_id_scope		= AC_EXPLICIT;

	memset(tp->partner_lu_name, '\0', 18);
	tp->partner_lu_name_length	= 0;

	tp->prepare_data_permitted	= AC_PREPARE_DATA_NOT_PERMITTED;

	tp->prepare_to_receive_type	= AC_PREP_TO_RECEIVE_SYNC_LEVEL;

	tp->processing_mode		= AC_BLOCKING;

	tp->receive_type		= AC_RECEIVE_AND_WAIT;

	tp->return_control		= AC_WHEN_SESSION_ALLOCATED;

	memset(tp->security_password, '\0', 11);
	tp->security_password_length	= 0;

	memset(tp->security_user_id, '\0', 11);
	tp->security_user_id_length	= 0;

	tp->send_receive_mode		= AC_HALF_DUPLEX;

	tp->send_type			= AC_BUFFER_DATA;

	tp->sync_level			= AC_SYNC_LEVEL_NONE;

	memset(tp->tp_name, '\0', 65);
	tp->tp_name_length		= 0;

	tp->transaction_control		= AC_CHAINED_TRANSACTIONS;

	if (!blank) {
		sna_debug(5, "initializing from side information\n");

		/* set ae_qualifier from side information. */
		memset(tp->ae_qualifier, '\0', 1024);
		tp->ae_qualifier_length = 0;
		tp->ae_qualifier_format = 0;

		/* set ap_title from side information. */
		memset(tp->ap_title, '\0', 1024);
		tp->ap_title_length = 0;
		tp->ap_title_format = 0;

		/* set application_context from side information. */
		memset(tp->application_context_name, '\0', 256);
		tp->application_context_length = 0;

		/* set security_type from side information. */
		tp->conversation_security_type  = AC_SECURITY_SAME;

		/* set directory_encoding from side information. */
		tp->directory_encoding          = AC_DEFAULT_ENCODING;

		/* set direcotry_syntax from side information. */
		tp->directory_syntax            = AC_DEFAULT_SYNTAX;

		strcpy(tp->mode_name, c_side->mode_name);
		tp->mode_name_length		= c_side->mode_name_length;

		/* set partner_id from side information. */
		tp->partner_id			= NULL;
		tp->partner_id_length           = 0;

		sna_netid_to_char(&c_side->netid_plu, tp->partner_lu_name);
		tp->partner_lu_name_length	= strlen(tp->partner_lu_name);

		/* set security_password from side information. */
		memset(tp->security_password, '\0', 11);
		tp->security_password_length    = 0;

		/* set security_user_id from side information. */
		memset(tp->security_user_id, '\0', 11);
		tp->security_user_id_length     = 0;

		strcpy(tp->tp_name, c_side->tp_name);
		tp->tp_name_length = c_side->tp_name_length;
	}
	sna_cpic_fsm_hd(cpic, CM_CMINIT, err, tp->data_received,
		tp->status_received, tp->send_receive_mode, 0);
out:    return err;
}

static CM_RETURN_CODE sna_cpic_cmcfm(struct cpic *cpic, __u32 *a1)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;
}

static CM_RETURN_CODE sna_cpic_cmrcvx(struct cpic *cpic, __u32 *a1, __u32 *a2,
		__u32 *a3, __u32 *a4, __u32 *a5)
{
	sna_debug(5, "init\n");
	return CM_CALL_NOT_SUPPORTED;
}

static CM_RETURN_CODE sna_cpic_cmdeal(struct cpic *cpic)
{
	struct sna_tp_cb *tp = cpic->vi.sna;
	int err;

	sna_debug(5, "init\n");
	err = sna_ps_verb_router(DEALLOCATE, tp);
	return sna_cpic_appc2cpic_rc(err);
}

int sna_cpic_setsockopt(struct socket *sock, int level, int optname,
			 char *optval, int optlen)
{
	struct sna_nof_cpic cpic;
	int err = -EINVAL;

	if (level != SOL_SNA_CPIC)
		goto out;
	if (optlen < sizeof(cpic))
		goto out;
	if (copy_from_user(&cpic, optval, sizeof(cpic))) {
		err = -EFAULT;
		goto out;
	}
	switch (cpic.action) {
		case SNA_NOF_DEFINE:
			err = sna_cpic_register_side_info(&cpic);
			break;
		case SNA_NOF_DELETE:
			err = sna_cpic_unregister_side_info(&cpic);
			break;
	}
out:	return err;
}

int cpic_ginfo(struct sna_cpic_side_info *side, char *buf, int len)
{
	struct cpicsreq cr;
	int done = 0;

	if (!buf) {
		done += sizeof(cr);
		return done;
	}
	if (len < (int)sizeof(cr))
		return done;
	memset(&cr, '\0', sizeof(struct cpicsreq));

	/* Move the data here */
	sprintf(cr.netid, "%s", sna_pr_netid(&side->netid));
	sprintf(cr.netid_plu, "%s", sna_pr_netid(&side->netid_plu));
	strncpy(cr.sym_dest_name, side->sym_dest_name, SNA_RESOURCE_NAME_LEN);
	strncpy(cr.mode_name, side->mode_name, SNA_RESOURCE_NAME_LEN);
	strncpy(cr.tp_name, side->tp_name, 65);
	strncpy(cr.username, side->username, 11);
	strncpy(cr.password, side->password, 11);
	cr.service_tp     = side->service_tp;
	cr.security_level = side->security_level;
	cr.proc_id        = side->index;

	if (copy_to_user(buf, &cr, sizeof(struct cpicsreq)))
		return -EFAULT;
	buf  += sizeof(struct cpicsreq);
	len  -= sizeof(struct cpicsreq);
	done += sizeof(struct cpicsreq);
	return done;
}

int cpic_query_side_info(char *arg)
{
	struct sna_cpic_side_info *cpic;
	struct list_head *le;
	struct cpicsconf cc;
	int len, total, err;
	char *pos;

	if (copy_from_user(&cc, arg, sizeof(cc)))
		return -EFAULT;
	pos = cc.cpicsc_buf;
	len = cc.cpics_len;

	total = 0;
	list_for_each(le, &cpic_side_list) {
		cpic = list_entry(le, struct sna_cpic_side_info, list);
		if (pos == NULL)
			err = cpic_ginfo(cpic, NULL, 0);
		else
			err = cpic_ginfo(cpic, pos + total, len - total);
		if (err < 0)
			return -EFAULT;
		total += err;
	}
	cc.cpics_len = total;
	if (copy_to_user(arg, &cc, sizeof(cc)))
		return -EFAULT;
	return 0;
}

static int sna_cpic_cpicall(struct socket *sock, void *uaddr)
{
	CM_RETURN_CODE rc = CM_PRODUCT_SPECIFIC_ERROR;
	struct sock *sk = sock->sk;
	struct cpic *cpic = &cpic_pi(sk)->cpic;
	cpic_args *args = NULL;

	sna_debug(5, "init\n");
	/* copy call arguments from user space, if needed. */
	if (uaddr) {
		new(args, GFP_ATOMIC);
		if (!args)
			return -ENOMEM;
		if (sna_utok(uaddr, sizeof(cpic_args), args) < 0)
			return -EFAULT;
	} else
		return -EINVAL;

	/* process the command. */
	switch (args->opcode) {
		case CM_CMACCI:
			rc = sna_cpic_cmacci(cpic);
			break;
		case CM_CMACCP: /* No Cvid */
			rc = sna_cpic_cmaccp(cpic, (pid_t *)args->a1);
			break;
		case CM_CMALLC:
			rc = sna_cpic_cmallc(cpic);
			break;
		case CM_CMCANC:
			rc = sna_cpic_cmcanc(cpic);
			break;
		case CM_CMCFM:
			rc = sna_cpic_cmcfm(cpic, (__u32 *)args->a1);
			break;
		case CM_CMCFMD:
			rc = sna_cpic_cmcfmd(cpic);
			break;
		case CM_CMDEAL:
			rc = sna_cpic_cmdeal(cpic);
			break;
		case CM_CMDFDE:
			rc = sna_cpic_cmdfde(cpic);
			break;
		case CM_CMEACN:
			rc = sna_cpic_cmeacn(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2);
			break;
		case CM_CMEAEQ:
			rc = sna_cpic_cmeaeq(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2, (__u32 *)args->a3);
			break;
		case CM_CMEAPT:
			rc = sna_cpic_cmeapt(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2, (__u32 *)args->a3);
			break;
		case CM_CMECS:
			rc = sna_cpic_cmecs(cpic, (__u32 *)args->a1);
			break;
		case CM_CMECT:
			rc = sna_cpic_cmect(cpic, (__u32 *)args->a1);
			break;
		case CM_CMECTX:
			rc = sna_cpic_cmectx(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2);
			break;
		case CM_CMEID:
			rc = sna_cpic_cmeid(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2, (__u32 *)args->a3);
			break;
		case CM_CMEMN:
			rc = sna_cpic_cmemn(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2);
			break;
		case CM_CMEPID:
			rc = sna_cpic_cmepid(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2, (__u32 *)args->a3,
				(__u32 *)args->a4, (__u32 *)args->a5,
				(__u32 *)args->a6, (__u32 *)args->a7);
			break;
		case CM_CMEPLN:
			rc = sna_cpic_cmepln(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2);
			break;
		case CM_CMESI:
			rc = sna_cpic_cmesi(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2, (__u32 *)args->a3,
				(__u32 *)args->a4);
			break;
		case CM_CMESL:
			rc = sna_cpic_cmesl(cpic, (__u32 *)args->a1);
			break;
		case CM_CMESRM:
			rc = sna_cpic_cmesrm(cpic, (__u32 *)args->a1);
			break;
		case CM_CMESUI:
			rc = sna_cpic_cmesui(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2);
			break;
		case CM_CMETC:
			rc = sna_cpic_cmetc(cpic, (__u32 *)args->a1);
			break;
		case CM_CMETPN:
			rc = sna_cpic_cmetpn(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2);
			break;
		case CM_CMFLUS:
			rc = sna_cpic_cmflus(cpic);
			break;
		case CM_CMINCL:
			rc = sna_cpic_cmincl(cpic);
			break;
		case CM_CMINIC:
			rc = sna_cpic_cminic(cpic);
			break;
		case CM_CMINIT: /* No Cvid */
            rc = sna_cpic_cminit(cpic, (__u8 *)args->a1);
			break;
		case CM_CMPREP:
			rc = sna_cpic_cmprep(cpic);
			break;
		case CM_CMPTR:
			rc = sna_cpic_cmptr(cpic);
			break;
		case CM_CMRCV:
			rc = sna_cpic_cmrcv(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2, (__u32 *)args->a3,
				(__u32 *)args->a4, (__u32 *)args->a5,
				(__u32 *)args->a6);
			break;
		case CM_CMRCVX:
			rc = sna_cpic_cmrcvx(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2, (__u32 *)args->a3,
				(__u32 *)args->a4, (__u32 *)args->a5);
			break;
		case CM_CMRTS:
			rc = sna_cpic_cmrts(cpic);
			break;
		case CM_CMSAC:
			rc = sna_cpic_cmsac(cpic, (__u32 *)args->a1);
			break;
		case CM_CMSACN:
			rc = sna_cpic_cmsacn(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2);
			break;
		case CM_CMSAEQ:
			rc = sna_cpic_cmsaeq(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2, (__u32 *)args->a3);
			break;
		case CM_CMSAPT:
			rc = sna_cpic_cmsapt(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2, (__u32 *)args->a3);
			break;
		case CM_CMSBT:
			rc = sna_cpic_cmsbt(cpic, (__u32 *)args->a1);
			break;
		case CM_CMSCSP:
			rc = sna_cpic_cmscsp(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2);
			break;
		case CM_CMSCST:
			rc = sna_cpic_cmscst(cpic, (__u32 *)args->a1);
			break;
		case CM_CMSCSU:
			rc = sna_cpic_cmscsu(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2);
			break;
		case CM_CMSCT:
			rc = sna_cpic_cmsct(cpic, (__u32 *)args->a1);
			break;
		case CM_CMSCU:
			rc = sna_cpic_cmscu(cpic, (__u32 *)args->a1);
			break;
		case CM_CMSDT:
			rc = sna_cpic_cmsdt(cpic, (__u32 *)args->a1);
			break;
		case CM_CMSED:
			rc = sna_cpic_cmsed(cpic, (__u32 *)args->a1);
			 break;
		case CM_CMSEND:
			rc = sna_cpic_cmsend(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2, (__u32 *)args->a3);
			break;
		case CM_CMSERR:
			rc = sna_cpic_cmserr(cpic, (__u32 *)args->a1);
			break;
		case CM_CMSF:
			rc = sna_cpic_cmsf(cpic, (__u32 *)args->a1);
			break;
		case CM_CMSID:
			rc = sna_cpic_cmsid(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2);
			break;
		case CM_CMSLD:
			rc = sna_cpic_cmsld(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2);
			break;
		case CM_CMSMN:
			rc = sna_cpic_cmsmn(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2);
			break;
		case CM_CMSNDX:
			rc = sna_cpic_cmsndx(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2, (__u32 *)args->a3);
			break;
		case CM_CMSPDP:
			rc = sna_cpic_cmspdp(cpic, (__u32 *)args->a1);
			break;
		case CM_CMSPID:
			rc = sna_cpic_cmspid(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2, (__u32 *)args->a3,
				(__u32 *)args->a4, (__u32 *)args->a5,
				(__u32 *)args->a6);
			break;
		case CM_CMSPLN:
			rc = sna_cpic_cmspln(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2);
			break;
		case CM_CMSPM:
			rc = sna_cpic_cmspm(cpic, (__u32 *)args->a1);
			break;
		case CM_CMSPTR:
			rc = sna_cpic_cmsptr(cpic, (__u32 *)args->a1);
			break;
		case CM_CMSQCF:
			rc = sna_cpic_cmsqcf(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2, (__u32 *)args->a3,
				(__u32 *)args->a4);
			break;
		case CM_CMSQPM:
			rc = sna_cpic_cmsqpm(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2, (__u32 *)args->a3,
				(__u32 *)args->a4);
			break;
		case CM_CMSRC:
			rc = sna_cpic_cmsrc(cpic, (__u32 *)args->a1);
			break;
		case CM_CMSRT:
			rc = sna_cpic_cmsrt(cpic, (__u32 *)args->a1);
			break;
		case CM_CMSSL:
			rc = sna_cpic_cmssl(cpic, (__u32 *)args->a1);
			break;
		case CM_CMSSRM:
			rc = sna_cpic_cmssrm(cpic, (__u32 *)args->a1);
			break;
		case CM_CMSST:
			rc = sna_cpic_cmsst(cpic, (__u32 *)args->a1);
			break;
		case CM_CMSTC:
			rc = sna_cpic_cmstc(cpic, (__u32 *)args->a1);
			break;
		case CM_CMSTPN:
			rc = sna_cpic_cmstpn(cpic, (__u32 *)args->a1,
				(__u32 *)args->a2);
			break;
		case CM_CMTRTS:
			rc = sna_cpic_cmtrts(cpic, (__u32 *)args->a1);
			break;
		case CM_CMWAIT:
			rc = sna_cpic_cmwait(cpic, (__u32 *)args->a1);
			break;
		default:
			sna_debug(5, "unknown opcode=%02X\n", args->opcode);
			break;
	}
	sna_cpic_error(args->return_code, rc);
	return 0;
}

int sna_cpic_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg)
{
	int err = -EINVAL;

	sna_debug(5, "init\n");
	switch (cmd) {
		case SIOCGCPICS:
			err = cpic_query_side_info((void *)arg);
			break;
		case SIOCCPICC:
			err = sna_cpic_cpicall(sock, (void *)arg);
			break;
	}
	return err;
}

static unsigned int sna_cpic_poll(struct file * file, struct socket *sock,
	poll_table *pt)
{
	sna_debug(5, "init\n");
	return -EOPNOTSUPP;
}

static struct proto cpic_proto = {
	.name		= "CPIC",
	.owner		= THIS_MODULE,
	.obj_size	= sizeof(struct cpic_sock)
};

static const struct proto_ops cpic_sock_ops = {
	.family		= PF_SNA,
	.owner		= THIS_MODULE,
	.release	= sna_cpic_release_session,
	.bind		= sock_no_bind,
	.connect	= sock_no_connect,
	.listen		= sock_no_listen,
	.accept		= sock_no_accept,
	.getname	= sock_no_getname,
	.sendmsg	= sock_no_sendmsg,
	.recvmsg	= sock_no_recvmsg,
	.poll		= sna_cpic_poll,
	.ioctl		= sna_cpic_ioctl,
	.mmap		= sock_no_mmap,
	.socketpair	= sock_no_socketpair,
	.shutdown	= sock_no_shutdown,
	.setsockopt	= sock_no_setsockopt,
	.getsockopt	= sock_no_getsockopt
};

static int sna_cpic_create(struct net *net, struct socket *sock, int protocol,
			int kern)
{
	struct sock *sk;
	struct cpic *cpic;

	sock->state = SS_UNCONNECTED;
	sock->ops = &cpic_sock_ops;

	sk = sk_alloc(net, PF_SNA, GFP_KERNEL, &cpic_proto, kern);
	if (!sk)
		return -ENOMEM;

	sock_init_data(sock, sk);
	cpic = &cpic_pi(sk)->cpic;
	memset(cpic, 0, sizeof(struct cpic));
	list_add_tail(&cpic->list, &cpic_list);
	return 0;
}

static const struct net_proto_family cpic_sock_family_ops = {
	.family	= PF_SNA,
	.owner	= THIS_MODULE,
	.create	= sna_cpic_create,
};

#ifdef CONFIG_PROC_FS
static int sna_cpic_get_info_side(struct seq_file *m, void *v)
{
	struct sna_cpic_side_info *cpic;
	struct list_head *le;

	seq_printf(m, "%-18s%-18s%-9s%-10s%-s\n",
		"netid.node", "netid.plu", "sym_name", "mode_name", "tp_name");

	list_for_each(le, &cpic_side_list) {
		cpic = list_entry(le, struct sna_cpic_side_info, list);
		seq_printf(m, "%-18s",
			sna_pr_netid(&cpic->netid));
		seq_printf(m, "%-18s",
			sna_pr_netid(&cpic->netid_plu));
		seq_printf(m, "%-9s", cpic->sym_dest_name);
		seq_printf(m, "%-10s", cpic->mode_name);
		seq_printf(m, "%-s\n", cpic->tp_name);
	}

	return 0;
}

static int sna_cpic_get_info(struct seq_file *m, void *v)
{
	struct list_head *le;
	struct sna_tp_cb *tp;
	struct cpic *cpic;

	seq_printf(m, "%-9s%-6s%-9s%-8s%-8s\n", "cpicPtr", "state",
		"tp_index", "rx_qlen", "tx_qlen");
	list_for_each(le, &cpic_list) {
		cpic = list_entry(le, struct cpic, list);
		tp   = cpic->vi.sna;
		if (!tp)
			continue;
		seq_printf(m, "%p %-6d%-9d%-8d%-8d\n", cpic,
			cpic->state, tp->index, tp->receive_queue.qlen,
			tp->write_queue.qlen);
	}

	return 0;
}
#endif

int __init sna_cpic_init(void)
{
	int err;

	sna_debug(5, "init\n");

	err = proto_register(&cpic_proto, 0);
	if (err < 0)
		return err;

	err = sna_sock_register(SNAPROTO_CPIC, &cpic_sock_family_ops);
	if (err < 0)
		goto error;

#ifdef CONFIG_PROC_FS
	proc_sna_create("cpic_side_information", 0, sna_cpic_get_info_side);
	proc_sna_create("cpic_conversations", 0, sna_cpic_get_info);
#endif
	return 0;

error:
	proto_unregister(&cpic_proto);
	return err;

}

int sna_cpic_fini(void)
{
	struct sna_cpic_side_info *cpic;
	struct list_head *le, *se;

	sna_debug(5, "init\n");

#ifdef CONFIG_PROC_FS
	proc_sna_remove("cpic_conversations");
	proc_sna_remove("cpic_side_information");
#endif

	sna_sock_unregister(SNAPROTO_CPIC);
	proto_unregister(&cpic_proto);

	list_for_each_safe(le, se, &cpic_side_list) {
		cpic = list_entry(le, struct sna_cpic_side_info, list);
		list_del(&cpic->list);
		kfree(cpic);
	}
	return 0;
}
