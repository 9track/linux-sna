/* sna_rm.c: Linux Systems Network Architecture implementation
 * - SNA LU 6.2 Resource Manager (RM)
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
 
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/bitops.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/socket.h>
#include <linux/sockios.h>
#include <linux/in.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/notifier.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/route.h>
#include <linux/inet.h>
#include <linux/skbuff.h>
#include <net/datalink.h>
#include <net/sock.h>
#include <linux/proc_fs.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/sna.h>

#include <linux/appc.h>
#include <linux/cpic.h>
#include <net/cpic.h>

static LIST_HEAD(tp_list);
static LIST_HEAD(mode_list);
static LIST_HEAD(rcb_list);
static LIST_HEAD(remote_lu_list);
static LIST_HEAD(local_lu_list);

static u_int32_t	sna_tp_active_count		= 0;
static u_int32_t	sna_tp_system_index 		= 0;
static u_int32_t 	sna_mode_system_index 		= 1;
static u_int32_t	sna_local_lu_system_index	= 1;
static u_int32_t	sna_remote_lu_system_index	= 1;
static u_int32_t	sna_rcb_system_index		= 1;
static u_int32_t	sna_bracket_index		= 1;
static u_int32_t	sna_conversation_correlator 	= 1;
static u_int32_t	sna_correlator_cnt 		= 1;

static u_int32_t sna_rm_conversation_new_index(void)
{
	return ++sna_conversation_correlator;
}

static u_int32_t sna_rm_bracket_new_index(void)
{
        return ++sna_bracket_index;
}

struct sna_tp_cb *sna_rm_tp_get_by_pid(pid_t pid)
{
        struct list_head *le;
        struct sna_tp_cb *tp;

        sna_debug(5, "init: %d\n", pid);
        list_for_each(le, &tp_list) {
                tp = list_entry(le, struct sna_tp_cb, list);
                sna_debug(5, "%d %d\n", tp->pid, pid);
                if (tp->pid == pid)
                        return tp;
        }
        return NULL;
}

struct sna_tp_cb *sna_rm_tp_get_by_name(u_int8_t *name)
{
	struct list_head *le;
        struct sna_tp_cb *tp;

        sna_debug(5, "init: %s\n", name);
        list_for_each(le, &tp_list) {
                tp = list_entry(le, struct sna_tp_cb, list);
		if (!strcmp(tp->tp_name, name))
                        return tp;
        }
        return NULL;
}

struct sna_tp_cb *sna_rm_tp_get_by_index(u_int32_t index)
{
        struct list_head *le;
        struct sna_tp_cb *tp;

        sna_debug(5, "init\n");
        list_for_each(le, &tp_list) {
                tp = list_entry(le, struct sna_tp_cb, list);
                sna_debug(5, "%d %d\n", tp->index, index);
                if (tp->index == index)
                        return tp;
        }
        return NULL;
}

static u_int32_t sna_rm_tp_new_index(void)
{
	sna_debug(5, "init\n");
        for (;;) {
                if (++sna_tp_system_index <= 0)
                        sna_tp_system_index = 1;
                if (sna_rm_tp_get_by_index(sna_tp_system_index) == NULL)
                        return sna_tp_system_index;
        }
        return 0;
}

struct sna_remote_lu_cb *sna_rm_remote_lu_get_by_index(u_int32_t index)
{
        struct sna_remote_lu_cb *lu;
        struct list_head *le;

        sna_debug(5, "init\n");
        list_for_each(le, &remote_lu_list) {
                lu = list_entry(le, struct sna_remote_lu_cb, list);
                if (lu->index == index)
                        return lu;
        }
        return NULL;
}

static u_int32_t sna_rm_remote_lu_new_index(void)
{
	sna_debug(5, "init\n");
        for (;;) {
                if (++sna_remote_lu_system_index <= 0)
                        sna_remote_lu_system_index = 1;
                if (sna_rm_remote_lu_get_by_index(sna_remote_lu_system_index) == NULL)
                        return sna_remote_lu_system_index;
        }
        return 0;
}

static struct sna_local_lu_cb *sna_rm_local_lu_get_by_index(u_int32_t index)
{
        struct sna_local_lu_cb *lu;
        struct list_head *le;

        sna_debug(5, "init\n");
        list_for_each(le, &local_lu_list) {
                lu = list_entry(le, struct sna_local_lu_cb, list);
                if (lu->index == index)
                        return lu;
        }
        return NULL;
}

static u_int32_t sna_rm_local_lu_new_index(void)
{
	sna_debug(5, "init\n");
        for (;;) {
                if (++sna_local_lu_system_index <= 0)
                        sna_local_lu_system_index = 1;
                if (sna_rm_local_lu_get_by_index(sna_local_lu_system_index) == NULL)
                        return sna_local_lu_system_index;
        }
        return 0;
}

struct sna_mode_cb *sna_rm_mode_get_by_index(u_int32_t index)
{
        struct sna_mode_cb *mode;
        struct list_head *le;

        sna_debug(5, "init\n");
        list_for_each(le, &mode_list) {
                mode = list_entry(le, struct sna_mode_cb, list);
                if (mode->index == index)
                        return mode;
        }
        return NULL;
}

static u_int32_t sna_rm_mode_new_index(void)
{
	sna_debug(5, "init\n");
        for (;;) {
                if (++sna_mode_system_index <= 0)
                        sna_mode_system_index = 1;
                if (sna_rm_mode_get_by_index(sna_mode_system_index) == NULL)
                        return sna_mode_system_index;
        }
        return 0;
}

struct sna_mode_cb *sna_rm_mode_get_by_name(char *mode_name)
{
        struct sna_mode_cb *mode;
        struct list_head *le;

        sna_debug(5, "init\n");
        list_for_each(le, &mode_list) {
                mode = list_entry(le, struct sna_mode_cb, list);
                sna_debug(5, "m->mode(%s), mode(%s)\n",
                        mode->mode_name, mode_name);
                if (!strcmp(mode->mode_name, mode_name))
                        return mode;
        }
        return NULL;
}

struct sna_local_lu_cb *sna_rm_local_lu_get_by_alias(char *alias)
{
	struct sna_local_lu_cb *lu;
        struct list_head *le;

        sna_debug(5, "init\n");
        list_for_each(le, &local_lu_list) {
                lu = list_entry(le, struct sna_local_lu_cb, list);
                if (!strcmp(lu->use_name, alias))
                        return lu;
        }
        return NULL;
}

struct sna_local_lu_cb *sna_rm_local_lu_get_by_name(char *lu_name)
{
        struct sna_local_lu_cb *lu;
        struct list_head *le;

        sna_debug(5, "init\n");
        list_for_each(le, &local_lu_list) {
                lu = list_entry(le, struct sna_local_lu_cb, list);
                if (!strcmp(lu->lu_name, lu_name))
                        return lu;
        }
        return NULL;
}

struct sna_remote_lu_cb *sna_rm_remote_lu_get_by_alias(char *alias)
{
        struct sna_remote_lu_cb *lu;
        struct list_head *le;

        sna_debug(5, "init: -%s-\n", alias);
        list_for_each(le, &remote_lu_list) {
                lu = list_entry(le, struct sna_remote_lu_cb, list);
		sna_debug(5, "lu->use_name=`%s`\n", lu->use_name);
                if (!strcmp(lu->use_name, alias))
                        return lu;
        }
        return NULL;
}

struct sna_remote_lu_cb *sna_rm_remote_lu_get_by_name(sna_netid *id)
{
        struct sna_remote_lu_cb *plu;
        struct list_head *le;

        sna_debug(5, "init\n");
        list_for_each(le, &remote_lu_list) {
                plu = list_entry(le, struct sna_remote_lu_cb, list);
                if (!strcmp(plu->netid_plu.net, id->net)
                        && !strcmp(plu->netid_plu.name, id->name))
                        return plu;
        }
        return NULL;
}

struct sna_rcb *sna_rm_rcb_get_by_index(u_int32_t index)
{
        struct sna_rcb *rcb;
        struct list_head *le;

        sna_debug(5, "init\n");
        list_for_each(le, &rcb_list) {
                rcb = list_entry(le, struct sna_rcb, list);
                if (rcb->index == index)
                        return rcb;
        }
        return NULL;
}

static u_int32_t sna_rm_rcb_new_index(void)
{
	sna_debug(5, "init\n");
        for (;;) {
                if (++sna_rcb_system_index <= 0)
                        sna_rcb_system_index = 1;
                if (sna_rm_rcb_get_by_index(sna_rcb_system_index) == NULL)
                        return sna_rcb_system_index;
        }
        return 0;
}

int sna_rm_send_attach_to_ps(__u32 tcb_id, __u32 rcb_id, struct sk_buff *skb)
{
	sna_debug(5, "init\n");
	sna_ps_process_fmh5(tcb_id, rcb_id, skb);
	return 0;
}

int sna_rm_create_ps(__u32 *tcb_id, __u32 *rcb_id, struct sk_buff *skb)
{
	struct sna_tcb *tcb;

	sna_debug(5, "init\n");
#ifdef NOT
	struct snathdr *th = (struct snathdr *)skb->data;
	int err;
	
	sna_debug(5, "sna_rm_create_ps DAF is %02X\n", th->fid.f2.daf);
	*tcb_id = sna_cpic_create_tcb(&err);
	if (err < 0)
		return err;
	tcb = sna_cpic_find_tcb_by_daf(th->fid.f2.daf);
	if (!tcb) {
		sna_debug(5, "No TCB found\n");
		return -1;
	}
	*rcb_id = sna_rm_allocate_rcb(tcb);
	*tcb_id = tcb->tcb_id;
#endif
	return 0;
}

int sna_rm_proc_security(struct sk_buff *skb)
{
	sna_debug(5, "init\n");
	return 0;
}

int sna_rm_proc_attach(struct sk_buff *skb)
{
	sna_debug(5, "init\n");
	sna_debug(5, "ATTACH INBOUND HAS HIT RM.. READY!\n");
	
#ifdef NOT
	__u32 tcb_id = 0, rcb_id = 0;

	sna_rm_create_ps(&tcb_id, &rcb_id, skb);
	sna_rm_send_attach_to_ps(tcb_id, rcb_id, skb);

	/* old. */
        struct sna_scb *scb;
        __u8 tcb_id, rcb_id, err;

        tcb_id = 0;
        rcb_id = 0;

        scb = sna_search_scb();

        if (FSM_SCB_STATUS != PENDING_ATTACH)
                sna_send_deactivate_session(ACTIVE, scb->hs_id, ABNORMAL, 0x20030000);
        else {
                err = sna_attach_chk(fmh5, mu->layer.hs_to_rm.hs_id);
                switch (err) {
                        case 0xFFFFFFFF:
                                sna_send_deactivate_session(ACTIVE, scb->hs_id,
                                        ABNORMAL, 0x080F6051);
                                sna_free_buffer(mu);
                                break;


                        case 0x10086040:
                                sna_send_deactivate_session(ACTIVE, scb->hs_id,
                                        ABNORMAL, 0x10086040);
                                sna_free_buffer(mu);
                                break;

                        case 0x10086011:
                                sna_send_deactivate_session(ACTIVE, scb->hs_id,
                                        ABNORMAL, 0x10086011);
                                sna_free_buffer(mu);
                                break;
                }

                if (fmh5->fmh5cmd == ATTACH) {
                        if (err == 0x00000000) {
                                tp = sna_search_tp();   /* ??? */

                                /* Spell insane for now */
                                if (err == 0x00000000
                                        || tp->instance_cnt
                                        < tp->instance_limit)

                                        err = sna_ps_creation_proc(mu, tcb_id,rcb_id, tp, create_rc);
                                        if(err == SUCCESS)
                                        {
                                                sna_fsm_scb_status(r, attach, undefined);
                                                scb->rcb_id = rcb_id;
                                                sna_connect_rcb_and_scb(rcb_id,
mu->layer.hs_to_rm.hs_id);
                                                sna_send_attach_to_ps(mu, tcb_id, rcb_id, sense_code);
                                        }
                                        else
                                        {
                                                if(tp->tp_cnt > 0
                                                        && err == 0)
                                                {
                                                        sna_queue_attach_proc(mu);
                                                }
                                                else
                                                {
                                                        sna_send_deactivate_session(active, mu->layer.hs_to_rm.hs_id, abnormal, 0x08640000);
                                                        bm_free(FREE, mu);
                                                }
                                                if(tp->tp_cnt == 0)
                                                        sna_purge_queued_requests(tp);
                                        }

                                        sna_queue_attach(mu);
                                }
                                else
                                {
                                        mu->tp = NULL;
                                        sna_ps_creation_proc(mu, tcb_id, rcb_id, tp, create_rc);
                                        if(create_rc == SUCCESS)
                                        {
                                                sna_fsm_scb_status(r, attach, undefined);
                                                scb->rcb_id = rcb_id;
                                                sna_connect_rcb_and_scb(rcb_id,
mu->layer.hs_to_rm.hs_id);
                                                sna_send_attach_to_ps(mu, tcb_id, rcb_id, sense_code);
                                        }
                                        else
                                        {
                                                sna_send_deactivate_session(active, mu->layer.hs_to_rm.hs_id, abnormal, 0x08640000);
                                                bm_free(FREE, mu);
                                        }
                                }
                        }
                }
        }
#endif
        return 0;
}

int sna_rm_process_hs_to_rm(struct sk_buff *skb)
{
	sna_debug(5, "sna_rm_process_hs_to_rm\n");

	/* if FMH-5 */
//	sna_rm_process_attach(skb);

	return 0;
}

int sna_rm_delete_local_lu(struct sna_nof_local_lu *dlu)
{
	struct list_head *le, *se;
        struct sna_local_lu_cb *lu;

	sna_debug(5, "init\n");
	list_for_each_safe(le, se, &local_lu_list) {
		lu = list_entry(le, struct sna_local_lu_cb, list);
                if (!strcmp(lu->lu_name, dlu->lu_name)) {
			list_del(&lu->list);
                        kfree(lu);
                        sna_mod_dec_use_count();
                        return 0;
                }
        }
        return -ENOENT;
}

int sna_rm_delete_remote_lu(struct sna_nof_remote_lu *dplu)
{
	struct list_head *le, *se;
        struct sna_remote_lu_cb *plu;

	sna_debug(5, "init\n");
	list_for_each_safe(le, se, &remote_lu_list) {
		plu = list_entry(le, struct sna_remote_lu_cb, list);
                if (!strcmp(plu->netid_plu.net, dplu->netid_plu.net)
			&& !strcmp(plu->netid_plu.name, dplu->netid_plu.name)) {
			list_del(&plu->list);
                        kfree(plu);
                        sna_mod_dec_use_count();
                        return 0;
                }
        }
        return -ENOENT;
}

int sna_rm_delete_mode(struct sna_nof_mode *dm)
{
	struct list_head *le, *se;
	struct sna_mode_cb *mode;

	sna_debug(5, "init\n");
	list_for_each_safe(le, se, &mode_list) {
		mode = list_entry(le, struct sna_mode_cb, list);
		if (!strcmp(mode->mode_name, dm->mode_name)) {
			list_del(&mode_list);
			kfree(mode);
			sna_mod_dec_use_count();
			return 0;
		}
	}
	return -ENOENT;
}

int sna_rm_define_local_lu(struct sna_nof_local_lu *dlu)
{
	struct sna_local_lu_cb *lu;

	sna_debug(5, "init\n");
        lu = sna_rm_local_lu_get_by_name(dlu->lu_name);
        if (lu)
                return -EEXIST;
	new(lu, GFP_ATOMIC);
	if (!lu)
		return -ENOMEM;
	memcpy(&lu->use_name, &dlu->use_name, SNA_USE_NAME_LEN);
	memcpy(&lu->netid, &dlu->netid, sizeof(sna_netid));
	strncpy(lu->lu_name, dlu->lu_name, SNA_RESOURCE_NAME_LEN);
	lu->index	= sna_rm_local_lu_new_index();
	lu->sync_point	= dlu->sync_point;
	lu->lu_sess_limit=dlu->lu_sess_limit;
        lu->flags      	= SNA_UP;
	list_add_tail(&lu->list, &local_lu_list);
        sna_mod_inc_use_count();
	return 0;
}

int sna_rm_start_remote_lu(struct sna_nof_remote_lu *lu_n)
{
        struct sna_remote_lu_cb *lu;
	struct sna_mode_cb *mode;
	int err;

        sna_debug(5, "init\n");
        lu = sna_rm_remote_lu_get_by_alias(lu_n->use_name);
        if (!lu)
                return -ENOENT;
	mode = sna_rm_mode_get_by_name("SNASVCMG");
	if (!mode)
		return -ENOENT;
	err = sna_sm_proc_activate_session(mode, lu, 0, SNA_SM_S_TYPE_FSP);
	if (err < 0)
		sna_debug(5, "activate_session failed `%d'.\n", err);
	return err;
}

int sna_rm_stop_remote_lu(struct sna_nof_remote_lu *lu_n)
{
	struct sna_remote_lu_cb *lu;
	int err = 0;

	sna_debug(5, "init\n");
	lu = sna_rm_remote_lu_get_by_alias(lu_n->use_name);
        if (!lu)
                return -ENOENT;
	return err;
}

int sna_rm_define_remote_lu(struct sna_nof_remote_lu *lu)
{
	struct sna_remote_lu_cb *plu;

	sna_debug(5, "init:  -%s-\n", sna_pr_netid(&lu->netid_plu));
	plu = sna_rm_remote_lu_get_by_name(&lu->netid_plu);
	if (plu)
		return -EEXIST;
	new(plu, GFP_ATOMIC);
	if (!plu)
		return -ENOMEM;
	memcpy(&plu->use_name, &lu->use_name, SNA_USE_NAME_LEN);
	memcpy(&plu->netid, &lu->netid, sizeof(sna_netid));
	memcpy(&plu->netid_plu, &lu->netid_plu, sizeof(sna_netid));
	memcpy(&plu->netid_fqcp, &lu->netid_fqcp, sizeof(sna_netid));
	plu->index	= sna_rm_remote_lu_new_index();
	plu->parallel	= lu->parallel_ss;
	plu->cnv_security=lu->cnv_security;
	plu->flags	= SNA_UP;
	list_add_tail(&plu->list, &remote_lu_list);
	sna_mod_inc_use_count();
	return 0;
}

int sna_rm_define_mode(struct sna_nof_mode *dm)
{
	struct sna_mode_cb *mode;

	sna_debug(5, "init\n");
	mode = sna_rm_mode_get_by_name(dm->mode_name);
	if (mode)
		return -EEXIST;
	new(mode, GFP_ATOMIC);
	if (!mode)
		return -ENOMEM;
	memcpy(&mode->netid, &dm->netid, sizeof(sna_netid));
	memcpy(&mode->netid_plu, &dm->netid_plu, sizeof(sna_netid));
	strncpy(mode->mode_name, dm->mode_name, SNA_RESOURCE_NAME_LEN);
	strncpy(mode->cos_name, dm->cos_name, SNA_RESOURCE_NAME_LEN);
	mode->tx_pacing = dm->tx_pacing;
	mode->rx_pacing = dm->rx_pacing;
	mode->tx_max_ru = dm->tx_max_ru;
	mode->rx_max_ru = dm->rx_max_ru;
	mode->crypto	= dm->crypto;
	mode->flags	= SNA_UP;
	mode->index	= sna_rm_mode_new_index();

	mode->user_max.sessions		= SNA_MODE_MAX_SESSIONS;
	mode->user_max.conwinners	= SNA_MODE_MIN_CONWINNERS;
	mode->user_max.conlosers	= SNA_MODE_MIN_CONLOSERS;
	mode->active.sessions		= 0;
	mode->active.conwinners		= 0;
	mode->active.conlosers		= 0;
	mode->pending.sessions		= 0;
	mode->pending.conwinners	= 0;
	mode->pending.conlosers		= 0;
	list_add_tail(&mode->list, &mode_list);
	sna_mod_inc_use_count();
	return 0;
}

int sna_mode_ginfo(struct sna_mode_cb *mode, char *buf, int len)
{
        struct modereq mr;
        int done = 0;

	sna_debug(10, "sna_mode_ginfo\n");
        if (!buf) {
                done += sizeof(mr);
                return done;
        }
        if (len < (int)sizeof(mr))
                return done;
        memset(&mr, 0, sizeof(struct modereq));

        /* Move the data here */
	memcpy(&mr.netid, &mode->netid, sizeof(sna_netid));
	memcpy(&mr.plu_name, &mode->netid_plu, sizeof(sna_netid));
	strncpy(mr.mode_name, mode->mode_name, SNA_RESOURCE_NAME_LEN);
	strncpy(mr.cos_name, mode->cos_name, SNA_RESOURCE_NAME_LEN);
	mr.tx_pacing		= mode->tx_pacing;
	mr.rx_pacing		= mode->rx_pacing;
	mr.max_tx_ru		= mode->tx_max_ru;
	mr.max_rx_ru		= mode->rx_max_ru;
	mr.crypto		= mode->crypto;
	mr.proc_id		= mode->index;
	mr.flags		= mode->flags;
	mr.auto_activation	= mode->auto_activation;
	mr.max_sessions		= mode->user_max.sessions;
	mr.min_conlosers	= mode->user_max.conlosers;
	mr.min_conwinners	= mode->user_max.conwinners;
	mr.act_sessions		= mode->active.sessions;
	mr.act_conwinners	= mode->active.conwinners;
	mr.act_conlosers	= mode->active.conlosers;
	mr.pend_sessions	= mode->pending.sessions;
	mr.pend_conwinners	= mode->pending.conwinners;
	mr.pend_conlosers	= mode->pending.conlosers;

        if (copy_to_user(buf, &mr, sizeof(struct modereq)))
                return -EFAULT;
        buf  += sizeof(struct modereq);
        len  -= sizeof(struct modereq);
        done += sizeof(struct modereq);
        return done;
}

int sna_lu_ginfo(struct sna_local_lu_cb *lu, char *buf, int len)
{
        struct lureq lr;
        int done = 0;

        sna_debug(10, "sna_lu_ginfo\n");
        if (!buf) {
                done += sizeof(lr);
                return done;
        }
        if (len < (int)sizeof(lr))
                return done;
        memset(&lr, 0, sizeof(struct lureq));

        /* Move the data here */
	memcpy(&lr.netid, &lu->netid, sizeof(sna_netid));
	strncpy(lr.name, lu->lu_name, SNA_RESOURCE_NAME_LEN);
	lr.sync_point		= lu->sync_point;
	lr.lu_sess_limit	= lu->lu_sess_limit;
	lr.proc_id		= lu->index;
	lr.flags		= lu->flags;

        if (copy_to_user(buf, &lr, sizeof(struct lureq)))
                return -EFAULT;
        buf += sizeof(struct lureq);
        len -= sizeof(struct lureq);
        done += sizeof(struct lureq);
        return done;
}

int sna_plu_ginfo(struct sna_remote_lu_cb *plu, char *buf, int len)
{
        struct plureq pr;
        int done = 0;

        sna_debug(10, "sna_plu_ginfo\n");
        if (!buf) {
                done += sizeof(pr);
                return done;
        }
        if (len < (int)sizeof(pr))
                return done;
        memset(&pr, 0, sizeof(struct plureq));

        /* Move the data here */
	memcpy(&pr.netid, &plu->netid, sizeof(sna_netid));
	memcpy(&pr.plu_name, &plu->netid_plu, sizeof(sna_netid));
	memcpy(&pr.fqcp_name, &plu->netid_fqcp, sizeof(sna_netid));
	pr.parallel_ss	= plu->parallel;
	pr.cnv_security	= plu->cnv_security;
	pr.proc_id	= plu->index;
	pr.flags	= plu->flags;

        if (copy_to_user(buf, &pr, sizeof(struct plureq)))
                return -EFAULT;
        buf  += sizeof(struct plureq);
        len  -= sizeof(struct plureq);
        done += sizeof(struct plureq);
        return done;
}

int sna_rm_query_mode(char *arg)
{
	struct sna_mode_cb *mode;
	int len, total, done;
	struct list_head *le;
	struct modeconf mc;
	char *pos;

	sna_debug(5, "init\n");
	if (copy_from_user(&mc, arg, sizeof(mc)))
                return -EFAULT;
        pos = mc.modec_buf;
        len = mc.mode_len;

        /*
         * Get the data and put it into the structure
         */
        total = 0;
	list_for_each(le, &mode_list) {
		mode = list_entry(le, struct sna_mode_cb, list);
                if (pos == NULL)
                	done = sna_mode_ginfo(mode, NULL, 0);
                else
                        done = sna_mode_ginfo(mode,pos+total,len-total);
                if (done < 0)
                	return -EFAULT;
                total += done;
        }
        mc.mode_len = total;
        if (copy_to_user(arg, &mc, sizeof(mc)))
                return -EFAULT;
        return 0;
}

int sna_rm_query_lu(char *arg)
{
	struct sna_local_lu_cb *lu;
	struct list_head *le;
        int len, total, done;
	struct luconf lc;
	char *pos;

	sna_debug(5, "init\n");
        if (copy_from_user(&lc, arg, sizeof(lc)))
                return -EFAULT;
        pos = lc.luc_buf;
        len = lc.lu_len;

        /*
         * Get the data and put it into the structure
         */
        total = 0;
	list_for_each(le, &local_lu_list) {
		lu = list_entry(le, struct sna_local_lu_cb, list);
                if (pos == NULL)
                	done = sna_lu_ginfo(lu, NULL, 0);
                else
                        done = sna_lu_ginfo(lu,pos+total,len-total);
                if (done < 0)
                        return -EFAULT;
                total += done;
        }
        lc.lu_len = total;
        if (copy_to_user(arg, &lc, sizeof(lc)))
                return -EFAULT;
        return 0;
}

int sna_rm_query_plu(char *arg)
{
	struct sna_remote_lu_cb *plu;
	struct list_head *le;
	int len, total, done;
	struct pluconf pc;
	char *pos;

	sna_debug(5, "init\n");
        if (copy_from_user(&pc, arg, sizeof(pc)))
                return -EFAULT;
        pos = pc.pluc_buf;
        len = pc.plu_len;

        /*
         * Get the data and put it into the structure
         */
        total = 0;
	list_for_each(le, &remote_lu_list) {
		plu = list_entry(le, struct sna_remote_lu_cb, list);
                if (pos == NULL)
	                done = sna_plu_ginfo(plu, NULL, 0);
                else
                        done = sna_plu_ginfo(plu, pos+total, len-total);
                if (done < 0)
                        return -EFAULT;
                total += done;
        }
        pc.plu_len = total;
        if (copy_to_user(arg, &pc, sizeof(pc)))
                return -EFAULT;
        return 0;
}

static int sna_rm_fsm_scb_input_r_pos_bid_rsp(struct sna_scb *scb)
{
	int err = -EINVAL;

	sna_debug(5, "init\n");
	if (scb->type != SNA_RM_SCB_TYPE_BIDDER)
		goto out;
	if (scb->state != SNA_RM_FSM_SCB_STATE_SESSION_ACTIVATION
		&& scb->state != SNA_RM_FSM_SCB_STATE_FREE)
		goto out;
	err = 0;
	scb->state = SNA_RM_FSM_SCB_STATE_IN_USE;
out:	return err;
}

static int sna_rm_fsm_scb_input_s_get_session(struct sna_scb *scb)
{
        int err = -EINVAL;

        sna_debug(5, "init\n");
	if (scb->type != SNA_RM_SCB_TYPE_FSP)
		goto out;
	if (scb->state != SNA_RM_FSM_SCB_STATE_SESSION_ACTIVATION
		&& scb->state != SNA_RM_FSM_SCB_STATE_FREE)
		goto out;
	err = 0;
	scb->state = SNA_RM_FSM_SCB_STATE_IN_USE;
out:    return err;
}

static int sna_rm_fsm_scb_input_r_bid(struct sna_scb *scb)
{
        int err = -EINVAL;

        sna_debug(5, "init\n");
	if (scb->state != SNA_RM_FSM_SCB_STATE_FREE)
		goto out;
	err = 0;
	scb->state = SNA_RM_FSM_SCB_STATE_PENDING_ATTACH;
out:    return err;
}

static int sna_rm_fsm_scb_input_r_attach(struct sna_scb *scb)
{
        int err = -EINVAL;

        sna_debug(5, "init\n");
	if (scb->state != SNA_RM_FSM_SCB_STATE_PENDING_ATTACH)
		goto out;
	err = 0;
	scb->state = SNA_RM_FSM_SCB_STATE_IN_USE;
out:    return err;
}

static int sna_rm_fsm_scb_input_r_fmh_12(struct sna_scb *scb)
{
        int err = -EINVAL;

        sna_debug(5, "init\n");
	if (scb->state != SNA_RM_FSM_SCB_STATE_PENDING_FMH12)
		goto out;
	err = 0;
	scb->state = SNA_RM_FSM_SCB_STATE_PENDING_ATTACH;
out:    return err;
}

static int sna_rm_fsm_scb_input_r_free_session(struct sna_scb *scb)
{
        int err = -EINVAL;

        sna_debug(5, "init\n");
	if (scb->state != SNA_RM_FSM_SCB_STATE_PENDING_ATTACH
		&& scb->state != SNA_RM_FSM_SCB_STATE_IN_USE)
		goto out;
	err = 0;
	scb->state = SNA_RM_FSM_SCB_STATE_FREE;
out:    return err;
}

static int sna_rm_fsm_scb_input_s_yield_session(struct sna_scb *scb)
{
        int err = -EINVAL;

        sna_debug(5, "init\n");
	if (scb->state != SNA_RM_FSM_SCB_STATE_SESSION_ACTIVATION)
		goto out;
	err = 0;
	scb->state = SNA_RM_FSM_SCB_STATE_IN_USE;
out:    return err;
}

static int sna_rm_fsm_scb_input_r_session_activated_pri(struct sna_scb *scb)
{
        int err = -EINVAL;

        sna_debug(5, "init\n");
	if (scb->state == SNA_RM_FSM_SCB_STATE_SESSION_ACTIVATION)
		err = 0;
	return err;
}

static int sna_rm_fsm_scb_input_r_session_activated_sec(struct sna_scb *scb)
{
        int err = -EINVAL;

        sna_debug(5, "init\n");
	if (scb->state != SNA_RM_FSM_SCB_STATE_SESSION_ACTIVATION)
		goto out;
	err = 0;
	scb->state = SNA_RM_FSM_SCB_STATE_PENDING_ATTACH;
out:    return err;
}

static int sna_rm_fsm_scb_input_r_session_activated_secure(struct sna_scb *scb)
{
        int err = -EINVAL;

        sna_debug(5, "init\n");
	if (scb->state != SNA_RM_FSM_SCB_STATE_SESSION_ACTIVATION)
                goto out;
        err = 0;
        scb->state = SNA_RM_FSM_SCB_STATE_PENDING_FMH12;
out:    return err;
}

static int sna_rm_fsm_scb_status(struct sna_scb *scb)
{
	int err = -EINVAL;

	sna_debug(5, "init\n");
	switch (scb->input) {
		case SNA_RM_FSM_SCB_INPUT_R_POS_BID_RSP:
			err = sna_rm_fsm_scb_input_r_pos_bid_rsp(scb);
			break;
        	case SNA_RM_FSM_SCB_INPUT_S_GET_SESSION:
			err = sna_rm_fsm_scb_input_s_get_session(scb);
                        break;
        	case SNA_RM_FSM_SCB_INPUT_R_BID:
			err = sna_rm_fsm_scb_input_r_bid(scb);
                        break;
        	case SNA_RM_FSM_SCB_INPUT_R_ATTACH:
			err = sna_rm_fsm_scb_input_r_attach(scb);
                        break;
        	case SNA_RM_FSM_SCB_INPUT_R_FMH_12:
			err = sna_rm_fsm_scb_input_r_fmh_12(scb);
                        break;
        	case SNA_RM_FSM_SCB_INPUT_R_FREE_SESSION:
			err = sna_rm_fsm_scb_input_r_free_session(scb);
                        break;
        	case SNA_RM_FSM_SCB_INPUT_S_YIELD_SESSION:
			err = sna_rm_fsm_scb_input_s_yield_session(scb);
                        break;
        	case SNA_RM_FSM_SCB_INPUT_R_SESSION_ACTIVATED_PRI:
			err = sna_rm_fsm_scb_input_r_session_activated_pri(scb);
                        break;
        	case SNA_RM_FSM_SCB_INPUT_R_SESSION_ACTIVATED_SEC:
			err = sna_rm_fsm_scb_input_r_session_activated_sec(scb);
                        break;
        	case SNA_RM_FSM_SCB_INPUT_R_SESSION_ACTIVATED_SECURE:
			err = sna_rm_fsm_scb_input_r_session_activated_secure(scb);
                        break;
	}
        return err;
}

static int sna_rm_fsm_rcb_input_s_get_session(struct sna_rcb *rcb)
{
	int err = -EINVAL;

	sna_debug(5, "init\n");
	if (rcb->fsm_rcb_status_state != SNA_RM_FSM_RCB_STATE_FREE)
		goto out;
	err = 0;
	if (rcb->type == SNA_RM_RCB_TYPE_FSP)
		rcb->fsm_rcb_status_state = SNA_RM_FSM_RCB_STATE_IN_USE;
	else
		rcb->fsm_rcb_status_state = SNA_RM_FSM_RCB_STATE_PENDING_SCB;
out:	return err;
}

static int sna_rm_fsm_rcb_input_r_pos_bid_rsp(struct sna_rcb *rcb)
{
        int err = -EINVAL;

        sna_debug(5, "init\n");
	if (rcb->type != SNA_RM_RCB_TYPE_BIDDER)
		goto out;
	if (rcb->fsm_rcb_status_state != SNA_RM_FSM_RCB_STATE_PENDING_SCB)
		goto out;
	err = 0;
	rcb->fsm_rcb_status_state = SNA_RM_FSM_RCB_STATE_IN_USE;
out:    return err;
}

static int sna_rm_fsm_rcb_input_r_neg_bid_rsp(struct sna_rcb *rcb)
{
        int err = -EINVAL;

        sna_debug(5, "init\n");
	if (rcb->type != SNA_RM_RCB_TYPE_BIDDER)
                goto out;
        if (rcb->fsm_rcb_status_state != SNA_RM_FSM_RCB_STATE_PENDING_SCB)
                goto out;
        err = 0;
        rcb->fsm_rcb_status_state = SNA_RM_FSM_RCB_STATE_FREE;
out:    return err;
}

static int sna_rm_fsm_rcb_input_r_attach_hs(struct sna_rcb *rcb)
{
        int err = -EINVAL;

        sna_debug(5, "init\n");
	if (rcb->fsm_rcb_status_state != SNA_RM_FSM_RCB_STATE_FREE)
		goto out;
	err = 0;
	rcb->fsm_rcb_status_state = SNA_RM_FSM_RCB_STATE_IN_USE;
out:    return err;
}

static int sna_rm_fsm_rcb_input_s_allocate_rcb(struct sna_rcb *rcb)
{
        int err = -EINVAL;

        sna_debug(5, "init\n");
	/* this can not be enforced, due to at allocate time we don't know type.
 	* though by default we set type to FSP, so we are okay.
 	*/
 	if (rcb->type != SNA_RM_RCB_TYPE_FSP)
 		goto out;
	if (rcb->fsm_rcb_status_state == SNA_RM_FSM_RCB_STATE_FREE)
		err = 0;
out:    return err;
}

static int sna_rm_fsm_rcb_input_s_deallocate_rcb(struct sna_rcb *rcb)
{
        int err = 0;

        sna_debug(5, "init\n");
	if (rcb->fsm_rcb_status_state == SNA_RM_FSM_RCB_STATE_FREE)
		goto out;
	if (rcb->fsm_rcb_status_state != SNA_RM_FSM_RCB_STATE_IN_USE) {
		err = -EINVAL;
		goto out;
	}
	rcb->fsm_rcb_status_state = SNA_RM_FSM_RCB_STATE_FREE;
out:    return err;
}

/* maintain the status of a conversation resource associated with a bidder
 * half-session.
 */
static int sna_rm_fsm_rcb_status(struct sna_rcb *rcb)
{
	int err = -EINVAL;

	sna_debug(5, "init\n");
	switch (rcb->fsm_rcb_status_input) {
		case SNA_RM_FSM_RCB_INPUT_S_GET_SESSION:
			err = sna_rm_fsm_rcb_input_s_get_session(rcb);
			break;
		case SNA_RM_FSM_RCB_INPUT_R_POS_BID_RSP:
			err = sna_rm_fsm_rcb_input_r_pos_bid_rsp(rcb);
			break;
		case SNA_RM_FSM_RCB_INPUT_R_NEG_BID_RSP:
			err = sna_rm_fsm_rcb_input_r_neg_bid_rsp(rcb);
			break;
		case SNA_RM_FSM_RCB_INPUT_R_ATTACH_HS:
			err = sna_rm_fsm_rcb_input_r_attach_hs(rcb);
			break;
		case SNA_RM_FSM_RCB_INPUT_S_ALLOCATE_RCB:
			err = sna_rm_fsm_rcb_input_s_allocate_rcb(rcb);
			break;
		case SNA_RM_FSM_RCB_INPUT_S_DEALLOCATE_RCB:
			err = sna_rm_fsm_rcb_input_s_deallocate_rcb(rcb);
			break;
	}
        return err;
}

/* maintain the status of a bidder half-session with respect to BIS_RQ and
 * BIS_REPLY.
 */
static int sna_rm_fsm_bis(int signal)
{
        sna_debug(5, "init\n");
        return 0;
}

static int sna_rm_connect_rcb_and_scb(struct sna_rcb *rcb, struct sna_scb *scb)
{
	struct sna_hs_cb *hs;

	sna_debug(5, "init\n");
	hs = sna_hs_get_by_index(scb->hs_index);
	if (!hs)
		return -ENOENT;
	rcb->session_index = scb->session_index;
	scb->bracket_index = rcb->bracket_index;
	return sna_hs_ps_connected(hs, rcb->bracket_index, rcb->tp_index);
}

/* the spec is rather a bit wrong on how to get hs_index at this moment in the
 * processing from an allocate_rcb call, we simply don't have an hs_index at that
 * point to we initialize it to zero and continue processing.
 *
 * this will have to be looked into more later, for now I suggest not forcing
 * an immediate session on allocate_rcb().
 */
static int sna_rm_set_rcb_and_scb_fields(struct sna_rcb *rcb, struct sna_scb *scb)
{
        struct sna_hs_cb *hs;
	int err = -ENOENT;

	sna_debug(5, "init\n");
	hs = sna_hs_get_by_index(scb->hs_index);
	if (!hs)
		goto out;
	scb->rcb_index 	= rcb->index;
	rcb->hs_index	= scb->hs_index;
	scb->type	= hs->type;
	rcb->type	= hs->type;
	if (hs->type == SNA_HS_TYPE_FSP) {
		scb->input = SNA_RM_FSM_SCB_INPUT_S_GET_SESSION;
		rcb->fsm_rcb_status_input = SNA_RM_FSM_RCB_INPUT_S_GET_SESSION;
	} else {
		scb->input = SNA_RM_FSM_SCB_INPUT_R_POS_BID_RSP;
		rcb->fsm_rcb_status_input = SNA_RM_FSM_RCB_INPUT_R_POS_BID_RSP;
	}
        err = sna_rm_fsm_scb_status(scb);
	if (err < 0)
		sna_debug(5, "fsm_scb_status failed `%d'.\n", err);
        err = sna_rm_fsm_rcb_status(rcb);
	if (err < 0)
		sna_debug(5, "fsm_rcb_status failed `%d'.\n", err);
out:	return err;
}

struct sna_rcb *sna_rm_create_rcb(struct sna_tp_cb *tp)
{
	struct sna_rcb *rcb;

	sna_debug(5, "init\n");
	new(rcb, GFP_ATOMIC);
        if (!rcb)
                return NULL;
	rcb->index			= sna_rm_rcb_new_index();
	rcb->bracket_index		= sna_rm_bracket_new_index();
	rcb->conversation_index 	= sna_rm_conversation_new_index();
	rcb->tp_index			= tp->index;
	rcb->mode_index			= tp->mode_index;
	rcb->local_lu_index		= tp->local_lu_index;
	rcb->ll_rx_state		= SNA_RCB_LL_STATE_NONE;
	rcb->ll_tx_state		= SNA_RCB_LL_STATE_NONE;
	rcb->fsm_rcb_status_state	= SNA_RM_FSM_RCB_STATE_FREE;
	rcb->fsm_conv_state		= SNA_PS_FSM_CONV_STATE_RESET;
	rcb->fsm_err_or_fail_state	= SNA_PS_FSM_ERR_OR_FAIL_STATE_NO_REQUESTS;
	rcb->fsm_post_state		= SNA_PS_FSM_POST_STATE_RESET;
	rcb->wait_timeo			= MAX_SCHEDULE_TIMEOUT;
	rcb->send_mu			= NULL;
	init_waitqueue_head(&rcb->sleep);
	skb_queue_head_init(&rcb->hs_to_ps_buffer_queue);
	skb_queue_head_init(&rcb->hs_to_ps_queue);
	skb_queue_head_init(&rcb->rm_to_ps_queue);
	list_add_tail(&rcb->list, &rcb_list);
	rcb->fsm_rcb_status_input = SNA_RM_FSM_RCB_INPUT_S_ALLOCATE_RCB;
	sna_rm_fsm_rcb_status(rcb);
        return rcb;
}

static struct sna_rcb *sna_rm_test_for_free_fsp_session(struct sna_tp_cb *tp)
{
	struct sna_mode_cb *mode;
        struct sna_rcb *rcb;
        struct sna_scb *scb;
	int err;

	sna_debug(5, "init\n");
	mode = sna_rm_mode_get_by_index(tp->mode_index);
	if (!mode)
		return NULL;
	if (mode->active.conwinners < mode->user_max.conwinners) {
		sna_debug(5, "no fsp sessions available.\n");
		return NULL;
	}
	rcb = sna_rm_create_rcb(tp);
	if (!rcb)
		return NULL;
	/* check partner lu security level and sync level.
	 * return error if unable to support either.
	 */
        err = sna_rm_set_rcb_and_scb_fields(rcb, scb);
	if (err < 0)
		sna_debug(5, "set_rcb_and_scb failed\n");
        err = sna_rm_connect_rcb_and_scb(rcb, scb);
	if (err < 0)
		sna_debug(5, "connect_rcb_and_scb failed\n");

	/* this would be better moved to a session count manager. */
	mode->active.sessions++;
	mode->active.conwinners++;
        return rcb;
}

struct sna_rcb *sna_rm_alloc_rcb(struct sna_tp_cb *tp)
{
        struct sna_rcb *rcb;

        sna_debug(5, "init\n");
	if (tp->immediate)
		rcb = sna_rm_test_for_free_fsp_session(tp);
	else
		rcb = sna_rm_create_rcb(tp);
	return rcb;
}

static int sna_rm_successful_session_activation(struct sna_mode_cb *mode,
	struct sna_remote_lu_cb *remote_lu, struct sna_lulu_cb *lulu)
{
	struct sna_tp_cb *tp;
	struct sna_rcb *rcb;
	
	sna_debug(5, "init\n");
	tp = sna_rm_tp_get_by_index(lulu->tp_index);
	if (!tp) {
		sna_debug(5, "unable to find tp for which session was act\n");
		goto out;
	}

	/* finish setting up any activation data. 
	 * -- this seems very similar to sna_rm_set_rcb_and_scb_fields()
	 *
	 * I think we have some confusion over the use of scb.
	 */
	rcb = sna_rm_rcb_get_by_index(tp->rcb_index);
	if (!rcb) {
		sna_debug(5, "unable to find rcb for this session, bailing\n");
		goto out;
	}
	rcb->hs_index = lulu->hs_index;
	
	tp->state = SNA_TP_STATE_ACTIVE;
	wake_up_interruptible(&tp->sleep);
out:	return 0;
}

int sna_rm_session_activated_proc(struct sna_lulu_cb *lulu)
{
	struct sna_remote_lu_cb *remote_lu;
	struct sna_mode_cb *mode;
	int err;

	sna_debug(5, "init\n");
	mode = sna_rm_mode_get_by_index(lulu->mode_index);
	if (!mode)
		return -ENOENT;
	remote_lu = sna_rm_remote_lu_get_by_index(lulu->remote_lu_index);
	if (!remote_lu)
		return -ENOENT;
        mode->active.sessions++;
        if (lulu->type == SNA_RM_RCB_TYPE_FSP)
                mode->active.conwinners++;
        else
                mode->active.conlosers++;
        err = sna_rm_successful_session_activation(mode, remote_lu, lulu);
        if (err < 0)
                sna_debug(5, "session activation failed `%d'.\n", err);
	return err;
}

int sna_rm_wait_for_session_activated(struct sna_tp_cb *tp, int seconds)
{
        DECLARE_WAITQUEUE(wait, current);
        int rc, timeout = seconds * HZ;

        sna_debug(5, "init\n");
        add_wait_queue_exclusive(&tp->sleep, &wait);
        for (;;) {
                __set_current_state(TASK_INTERRUPTIBLE);
                rc = 0;
                if (tp->state != SNA_TP_STATE_ACTIVE)
                        timeout = schedule_timeout(timeout);
                if (tp->state == SNA_TP_STATE_ACTIVE)
                        break;
                rc = -EAGAIN;
                if (tp->state == SNA_TP_STATE_RESET)
                        break;
                rc = -ERESTARTSYS;
                if (signal_pending(current))
                        break;
                rc = -EAGAIN;
                if (!timeout)
                        break;
        }
        __set_current_state(TASK_RUNNING);
        remove_wait_queue(&tp->sleep, &wait);
        return rc;
}

static int sna_rm_send_activate_session(struct sna_mode_cb *mode,
	struct sna_remote_lu_cb *remote_lu, u_int32_t tp_index, int type)
{
	int err;

        sna_debug(5, "init\n");
	mode->pending.sessions++;
	if (type == SNA_RM_RCB_TYPE_FSP)
		mode->pending.conwinners++;
	else
		mode->pending.conlosers++;
	err = sna_sm_proc_activate_session(mode, remote_lu, tp_index, type);
	if (err < 0) {
		sna_debug(5, "sm_proc_activate_session failed `%d'.\n", err);
		goto out;
	}
out:	return err;
}

static int sna_rm_session_activation_polarity(struct sna_mode_cb *mode,
	struct sna_remote_lu_cb *remote_lu)
{
	sna_debug(5, "init\n");
	if (mode->pending.sessions + mode->active.sessions 
		>= mode->user_max.sessions)
		return SNA_RM_RCB_TYPE_NONE;
	if (mode->pending.sessions + mode->active.sessions
		&& !remote_lu->parallel)
		return SNA_RM_RCB_TYPE_NONE;
	if (mode->user_max.sessions - mode->user_max.conlosers
		> mode->active.conwinners + mode->pending.conwinners)
		return SNA_RM_RCB_TYPE_FSP;
	else
		return SNA_RM_RCB_TYPE_BIDDER;
}

/* this function starts a session activation, then sleeps waiting for
 * the activation to complete. (ie successful_session_activation()).
 */
int sna_rm_get_session(struct sna_tp_cb *tp)
{
	struct sna_remote_lu_cb *remote_lu;
	struct sna_mode_cb *mode;
	struct sna_rcb *rcb;
	int err = -ENOENT;
	
	sna_debug(5, "init\n");
	rcb = sna_rm_rcb_get_by_index(tp->rcb_index);
	if (!rcb)
		goto out;
	remote_lu = sna_rm_remote_lu_get_by_index(tp->remote_lu_index);
	if (!remote_lu)
		goto out;
	mode = sna_rm_mode_get_by_index(tp->mode_index);
	if (!mode)
		goto out;
	/* mode is closed by the user, 0 sessions allowed. */
	if (!mode->user_max.sessions) {
		err = -EUSERS;
		goto out;
	}

	/* try and find an unused session over this lu/mode pair.
	 * we don't support this at the moment so we just allocate
	 * a new session every time.
	 */	

	/* we need to do some checking here on activation/pending limits.
	 * specs are confusing here, so now we just allow user to allocate
	 * a new session if we aren't over our max of active sessions.
	 */
	err = sna_rm_session_activation_polarity(mode, remote_lu);
	switch (err) {
		case SNA_RM_RCB_TYPE_NONE:
			/* again we need to try and find an used connection here
			 * for another mode if available. for now we just
			 * return too many users.
			 */
			err = -EUSERS;
			break;
		case SNA_RM_RCB_TYPE_BIDDER:
		case SNA_RM_RCB_TYPE_FSP:
			err = sna_rm_send_activate_session(mode, remote_lu, 
				tp->index, err);
			break;
		default:
			err = -EINVAL;
			break;
	}
	err = sna_rm_wait_for_session_activated(tp, 255);
	if (err < 0) {
		sna_debug(5, "session activation failed `%d'.\n", err);
	} else {
		sna_debug(5, "woke with an activated session.\n");
	}
out:	return err;
}

static struct sna_tp_cb *sna_rm_create_tcb_and_ps(u_int8_t *tp_name,
	struct sna_mode_cb *mode, struct sna_remote_lu_cb *remote_lu, int *err)
{
	struct sna_tp_cb *tp;

	sna_debug(5, "init\n");
	*err = -ENOMEM;
	new(tp, GFP_ATOMIC);
        if (!tp)
                return NULL;
        tp->index 		= sna_rm_tp_new_index();
	tp->mode_index		= 0;
	tp->local_lu_index	= 0;
	tp->remote_lu_index	= 0;
	tp->luw_seq             = 1;
	do_gettimeofday(&tp->luw);
        init_waitqueue_head(&tp->sleep);
        skb_queue_head_init(&tp->receive_queue);
        skb_queue_head_init(&tp->write_queue);
	
	/* set mode, plu, tp_name if passed by the caller. */
	if (mode)
		tp->mode_index          = mode->index;
	if (remote_lu)
		tp->remote_lu_index     = remote_lu->index;
	if (tp_name) {
		tp->tp_name_length 	= strlen(tp_name);
		strncpy(tp->tp_name, tp_name, tp->tp_name_length);
	}

	/* set appc conversation defaults. */
	tp->conversation_type   = AC_CONVERSATION_TYPE_MAPPED;
	tp->rx_mc_ll_pull_left	= 0;
	
	*err = sna_ps_init(tp);
	if (*err < 0) {
		kfree(tp);
		tp = NULL;
	} else {
        	list_add_tail(&tp->list, &tp_list);
		sna_tp_active_count++;
	}
	return tp;
}

/* sna_rm_start_tp - entrance point for all sna transaction programs.
 * this function is called by cpic/appc when a tp is initialized. 
 *
 * if we allowed the user to define tp parameters, we would enforce them here.
 */
struct sna_tp_cb *sna_rm_start_tp(u_int8_t *tp_name, 
	u_int8_t *mode_name, sna_netid *remote_name, int *err)
{
	struct sna_remote_lu_cb *remote_lu = NULL;
	struct sna_mode_cb *mode = NULL;
	struct sna_tp_cb *tp = NULL;

	sna_debug(5, "init\n");
	*err = -ENOENT;
	mode 	  = sna_rm_mode_get_by_name(mode_name);
	remote_lu = sna_rm_remote_lu_get_by_name(remote_name);
	tp = sna_rm_create_tcb_and_ps(tp_name, mode, remote_lu, err);
	if (tp)
		sna_mod_inc_use_count();
	return tp;
}

int sna_rm_create(struct sna_nof_node *node)
{
	sna_debug(5, "init\n");
	return 0;
}

int sna_rm_destroy(struct sna_nof_node *d)
{
        struct list_head *le, *se;
        struct sna_mode_cb *mode;
        struct sna_remote_lu_cb *plu;
        struct sna_local_lu_cb *lu;

	sna_debug(5, "init\n");
        list_for_each_safe(le, se, &mode_list) {
                mode = list_entry(le, struct sna_mode_cb, list);
                list_del(&mode->list);
                kfree(mode);
                sna_mod_dec_use_count();
        }
        list_for_each_safe(le, se, &local_lu_list) {
                lu = list_entry(le, struct sna_local_lu_cb, list);
                list_del(&lu->list);
                kfree(lu);
                sna_mod_dec_use_count();
        }
        list_for_each_safe(le, se, &remote_lu_list) {
                plu = list_entry(le, struct sna_remote_lu_cb, list);
                list_del(&plu->list);
                kfree(plu);
                sna_mod_dec_use_count();
        }
        return 0;
}

#ifdef CONFIG_PROC_FS
int sna_rm_get_info_local_lu(char *buffer, char **start,
        off_t offset, int length)
{
	off_t pos = 0, begin = 0;
	struct sna_local_lu_cb *lu;
	struct list_head *le;
        int len = 0;

        len += sprintf(buffer, "%-6s%-18s%-9s%-11s%-14s%-5s\n",
                "lu_id", "netid.node", "lu_name", "sync_point",
		"lu_sess_limit", "flags");
	list_for_each(le, &local_lu_list) {
		lu = list_entry(le, struct sna_local_lu_cb, list);
                len += sprintf(buffer + len, "%-6d%-18s%-9s%-11d%-14d%04X\n",
                        lu->index, sna_pr_netid(&lu->netid), 
			lu->lu_name, lu->sync_point,
			lu->lu_sess_limit, lu->flags);

                /* Are we still dumping unwanted data then discard the record */
		pos = begin + len;
                if (pos < offset) {
                        len = 0;        /* Keep dumping into the buffer start */
                        begin = pos;
                }
                if (pos > offset + length)       /* We have dumped enough */
                        break;
        }

        /* The data in question runs from begin to begin+len */
        *start = buffer + (offset - begin);     /* Start of wanted data */
        len -= (offset - begin);   /* Remove unwanted header data from length */
	if (len > length)
                len = length;      /* Remove unwanted tail data from length */
        if (len < 0)
                len = 0;
	return len;
}

int sna_rm_get_info_remote_lu(char *buffer, char **start,
        off_t offset, int length)
{
        off_t pos = 0, begin = 0;
	struct sna_remote_lu_cb *plu;
	struct list_head *le;
        int len = 0;

        len += sprintf(buffer, "%-7s%-18s%-18s%-18s%-12s%-12s%-5s\n",
                "plu_id", "netid.node", "netid.plu", "netid.fqcp",
		"parallel_ss", "cnv_security", "flags");
	list_for_each(le, &remote_lu_list) {
		plu = list_entry(le, struct sna_remote_lu_cb, list);
                len += sprintf(buffer + len, "%-7d%-17s%-17s%-17s%-12d%-12d%04X\n",
			plu->index, sna_pr_netid(&plu->netid), 
			sna_pr_netid(&plu->netid_plu), 
			sna_pr_netid(&plu->netid_fqcp),
			plu->parallel, plu->cnv_security, plu->flags);

                /* Are we still dumping unwanted data then discard the record */
		pos = begin + len;
                if (pos < offset) {
                        len = 0;        /* Keep dumping into the buffer start */
                        begin = pos;
                }
                if (pos > offset + length)       /* We have dumped enough */
                        break;
        }

        /* The data in question runs from begin to begin+len */
        *start = buffer + (offset - begin);     /* Start of wanted data */
        len -= (offset - begin);   /* Remove unwanted header data from length */
	if (len > length)
                len = length;      /* Remove unwanted tail data from length */
        if (len < 0)
                len = 0;
        return len;
}

int sna_rm_get_info_mode(char *buffer, char **start,
        off_t offset, int length)
{
	struct sna_mode_cb *mode;
        off_t pos = 0, begin = 0;
	struct list_head *le;
        int len = 0;

        len += sprintf(buffer, "%-8s%-18s%-18s%-10s%-10s%-10s%-10s%-10s%-7s%-5s\n", 
		"mode_id", "netid.node", 
		"netid.plu", "mode_name", "tx_pacing", "rx_pacing",
		"max_tx_ru", "max_rx_ru", "crypto", "flags");
	list_for_each(le, &mode_list) {
		mode = list_entry(le, struct sna_mode_cb, list);
                len += sprintf(buffer + len, "%-8d%-17s%-17s%-8s%-10d%-10d%-10d%-10d%-7d%04X\n",
			mode->index, sna_pr_netid(&mode->netid), 
			sna_pr_netid(&mode->netid_plu), mode->mode_name, 
			mode->tx_pacing,
			mode->rx_pacing, mode->tx_max_ru, mode->rx_max_ru,
			mode->crypto, mode->flags);

                /* Are we still dumping unwanted data then discard the record */
		pos = begin + len;
                if (pos < offset) {
                        len = 0;        /* Keep dumping into the buffer start */
			begin = pos;
                }
                if (pos > offset + length)       /* We have dumped enough */
                        break;
        }

        /* The data in question runs from begin to begin+len */
        *start = buffer + (offset - begin);     /* Start of wanted data */
        len -= (offset - begin);   /* Remove unwanted header data from length */
        if (len > length)
                len = length;      /* Remove unwanted tail data from length */
        if (len < 0)
                len = 0;
        return len;
}
#endif

#ifdef NOT
static int sna_process_ps_to_rm(struct sna_mu *mu)
{
	switch(mu->record_type)
	{
		case (SNA_REC_GET_SESSION):
			sna_get_session_proc(mu);
			break;

		case (SNA_REC_DEALLOCATE_RCB):
			rcb = sna_search_rcb(deallocate_rcb->rcb_id);
			if (scb == NULL) {
				new(bracket_freed, GFP_ATOMIC);
				bracket_freed->bracket_id = rcb->bracket_id;
				sna_hs(bracket_freed);
			}
			discard(rcb);
			new(rcb_deallocated, GFP_ATOMIC);
			sna_ps(rcb_deallocated);
			break;

		case (SNA_REC_TERMINATE_PS):
			sna_ps_termination_proc(mu);
			break;

		case (SNA_REC_CHANGE_SESSIONS):
			sna_change_sessions_proc(mu);
			break;

		case (SNA_REC_RM_ACTIVATE_SESSION):
			sna_rm_activate_session_proc(mu);
			break;

		case (SNA_REC_RM_DEACTIVATE_SESSION):
			sna_rm_deactivate_session_proc(mu);
			break;

		case (SNA_REC_RM_DEACTIVATE_CONV_GROUP):
			sna_rm_deactivate_conv_group_proc(mu);
			break;

		case (SNA_REC_UNBIND_PROTOCOL_ERROR):
			sna_send_deactivate_session(active, unbind_protocol_error->hs_id, abnormal, unbind_protocol_error->sense);
			break;

		case (SNA_REC_ABEND_NOTIFICATION):
			sna_ps_abend_proc(mu);
			break;

		default:
			kfree(mu);
	}

	return (0);
}

/* We will cast data to appropiate struct type below */
static int sna_process_initiator_to_rm(struct sna_mu *mu)
{
	switch(mu->record_type)
	{
		case (SNA_REC_START_TP):
			sna_start_tp_proc(mu);
			break;

		case (SNA_REC_SEND_RTR):
			sna_send_rtr_proc(mu);
			break;

		case (SNA_REC_RM_TIMER_POP):
			sna_rm_timer_deactivate_sess_proc(mu);
			break;

		default:
			kfree(mu);
	}

	return (0);
}

static int sna_activate_needed_sessions(__u8 *lu_name, __u8 *mode_name)
{
	struct sna_lucb *lucb;
	struct sna_mode *mode;
	__u8 polarity;

	lucb = sna_search_lucb(lu_name);		/* ??? */
	mode = sna_search_mode(lucb, mode_name);	/* ??? */

	while((polarity = sna_session_activation_polarity(lu_name, mode_name)) 
		!= NULL)
	{
		if(polarity == SNA_SESSION_FIRST_SPEAKER)
			sna_send_activate_session(lu_name, mode_name, 
				SNA_SESSION_FIRST_SPEAKER);
		else	/* Bidder */
			sna_send_activate_session(lu_name, mode_name, 
				SNA_SESSION_BIDDER);

	}

	while((mode->active.conwinners + mode->pending.conwinners)
		< min(mode->auto_act_limit, mode->min_conwinners))
	{
		polarity = sna_session_activation_polarity(lu_name, mode_name);
		if(polarity == SNA_SESSION_FIRST_SPEAKER)
			sna_send_activate_session(lu_name, mode_name, 
				SNA_SESSION_FIRST_SPEAKER);
	}

	return (0);
}

static int sna_activate_session_rsp_proc(struct sna_mu *mu)
{
	struct sna_act_sess_rsp *rsp =(struct sna_act_sess_rsp *)mu->record_ptr;

	if(pending_act->correlator == act_sess_rsp->sorrelator)
	{
		mode = pending_act->mode;
		if(mode->polarity == SNA_SESSION_FIRST_SPEAKER)
			mode->pending->conwinners--;
		else
			mode->pending->conlosers--;

		mode->pending->sessions--;
		if(act_sess_rsp->type == POS)
		{
			if(mode->polarity == SNA_SESSION_FIRST_SPEAKER)
				mode->active->conwinners++;
			else
				mode->active->conlosers++;
			mode->active->sessions++;

			sna_successful_session_activation(pending_act->lu_name, 
				pending_act->mode_name,act_sess_rsp->sess_info);
		}
		else
		{
			sna_unsuccessful_session_activation(pending_act->lu_name,
				pending_act->mode_name, act_sess_rsp->err_type);
			remove_and_toss_pending_activation;
		}
	}

	return (0);
}

static __u32 sna_attach_chk(__u8 hs_id, struct sna_fmh5 *fmh5)
{
	__u8 err;

	if(fmh5->fmh5cmd != ATTACH)
		return (0x1008600B);

	err = sna_attach_length_chk(fmh5);
	if(err)
		return (err);

	if(fmh5->luw_id)	/* ??? */
	{
		/* This is wrong/bad */
		if(fmh5->luw_net == NULL && fmh5->lu_net != NULL)
			return (0x10086011);
	}
	else
	{
		if(fmh5->sync_leve == SYNCPT)
			return (0x10086011);
	}

	if(TP_is_a_local_TP)
	{
		switch(fmh5->sync)
		{
			case (NONE):
			case (CONFIRM):
				break;
			case (CONFIRM | SYNCPT | BACKOUT):
				if(RemoteLU != supportabove)
					return (0x10086040);
			default:
				return (0x10086040);
		}

		if(fmh5->sync != supported_by_TP)
			return (0x10086041);
		if(TP_temp_disabled)
			return (0x084B6031);
		if(TP_perm_disabled)
			return (0x084C0000);

		err = sna_attach_security_chk(fmh5);
		if(err)
			return (err);
	}
	else
		return (0x1008600B);

	return (0x00000000);
}

/* Need better Docs to do this one */
static int sna_attach_length_chk()
{

	return 0;
}

static int sna_security_chk()
{

	return 0;
}

static int sna_bid_proc(struct sna_mu *mu)
{
	struct sna_bid *bid = (struct sna_bid *)mu->record_ptr;
	struct sna_scb *scb;
	struct sna_bid_rsp *bid_rsp;

	scb = sna_search_scb(bid->hs_id);	/* ??? */
	if(FSM_BIS == BIS_RCVD || FSM_BIS == CLOSED)
		send_deactivate_session(active, bid->hs_id, abnormal, 0x20080000);
	else
	{
		mode = sna_search_mode(bid);
		if(mode->parallel == SNA_RM_FALSE
			&& mode->session_limit == 0
			&& mode->drain_partner == SNA_RM_FALSE
			&& FSM_BIS == BIS_SENT)
		{
			bid_rsp->rti	= NEG;
			bid_rsp->sense	= 0x088B0000;
			sna_hs(bid_rsp);
		}
		else
		{
			if(FSM_SCB_STATUS == FREE)
			{
				FSM_SCB_STATUS(R, bid, UNDEFINED);
				if(session_is_in_free_sessino_pool)
					scb = pull_scb_pool();
				bid_rsp->rti	= POS;
				bid_rsp->sense	= 0;
				sna_hs(bid_rsp);
				if(scb->timer_unique_id != NULL)
				{
					stop_timer();
					scb->timer_unique_id = NULL;
				}
			}
			else
			{
				if(first_speaker)
				{
					bid_rsp->rti 	= NEG;
					bid_rsp->sense 	= 0x08130000;
					sna_hs(bid_rsp);
					if(sense_code == 0x08140000)
					{
						Remember_LU_stuff;
					}
				}
				else
					send_deactivate_session(active, 
						bid->hs_id,abnormal,0x20030000);
			}
		}
	}

	bm_free(FREE, bid);

	return (0);
}

static int sna_bid_rsp_proc(struct sna_mu *mu)
{
	struct sna_bid_rsp *rsp = (struct sna_bid_rsp *)mu->record_ptr;
	struct sna_session_allocated *sallocated;
	struct sna_get_session *get_session;
	struct sna_scb *scb;
	struct sna_rcb *rcb;

	if(rsp->rti == NEG && rsp->sense == 0x88B0000)
	{
		if(parallel != supported)
		{
			mode->conwinners 	= 0;
			mode->conlosers 	= 0;
			mode->sessions 		= 0;
			send_deactivate_session(active, rsp->hs_id, cleanup, 0);
		}
		else
			send_deactivate_session(active, rsp->hs_id, abnormal, 0x20100000);
	}
	else
	{
		rcb = sna_search_rcb(PENDING_SCB, rsp->hs_id);
		if(rsp->rti == POS)
		{
			sna_set_rcb_and_scb_fields(rcb->rcb_id, rsp->hs_id);
			sna_connect_rcb_and_scb(rcb->rcb_id, rsp->hs_id, reply);
			scb = sna_search_scb(hs);

			sallocated->rcode 		= OK;
			sallocated->send_ru_size 	= scb->send_ru_size;
			sallocated->limit_buf_pool_id 	=scb->limit_buf_pool_id;
			sallocated->perm_buf_pool_id 	= scb->perm_buf_pool_id;
			sallocated->in_conver		= SNA_RM_TRUE;

			sna_ps(sallocated);
		}
		else
		{
			rcb->hs_id = NULL;
			sna_fsm_rcb_status(R, NEG_BID_RSP, UNDEFINED);
			if(rsp->sense == 0x08140000)
				remember_LU_owes_rtr;
			
			get_session = rcb->get_session;
			sna_get_session_proc(get_session);
		}
	}

	return (0);
}

static int sna_bidder_proc(struct sna_get_session *get_session, __u8 hs_id)
{
	struct sna_rcb *rcb;
	struct sna_bid_without_attach *bid_wo_attach;

	rcb = sna_search_rcb(get_session->rcb_id);
	rcb->hs_id = hs_id;

	sna_fsm_rcb_status = FSM_RCB_STATUS_BIDDER;
	rcb->session_parms_ptr = get_session;

	sna_hs(bid_wo_attach);

	return (0);
}

static int sna_bis_race_loser(__u8 hs_id)
{
	struct sna_mode *mode;
	struct sna_bis_reply *bis_reply;

	mode = sna_search_mode(hs_id);

	if(mode->polarity == SNA_SESSION_FIRST_SPEAKER)
		mode->pending.conwinners--;
	else
		mode->pending.conlosers--;

	sna_hs(bis_reply);
	close = sna_session_deactivation_polarity(mode->lu_name,
		mode->mode_name);

	if(close == EITHER)
	{
		sna_send_bis_rq(hs_id);
		remove_from_free_sess_pool;
	}

	return (0);
}

static int sna_change_sessions_proc(struct sna_mu *mu)
{
	struct sna_change_sessions *chg_sess = (struct sna_change_sessions *)mu->record_ptr;
	struct sna_mode *mode;
	struct sna_get_session *get_session;
	struct sna_session_allocated *sess_allocd;

	if(chg_sess->rsp == SNA_RM_TRUE)
	{
		mode = sna_search_mode(chg_sess->lu_name, chg_sess->mode_name);
		conwinners = mode->active.conwinners + mode->pending.conwinners;
		conlosers = mode->active.conlosers + mode->pending.conlosers;
		old_slimit = mode->max_sessions - chg_sess->delta;
		plateau = min(mode->active.sessions + mode->pending.sessions, old_slimit);
		conwinner_incr = max(0, mode->min_conwinners - conwinners);
		session_decr = max(0, plateau - mode->max_sessions);
		conloser_incr = max(0, mode->min_conlosers - conlosers);
		need_to_activate = conwinner_incr + conloser_incr;
		room_for_activation = max(0, mode->max_sessions - plateau);
		decrement_for_polarity = max(0, need_to_activate - room_for_activation);
		mode->termination_cnt = mode->termination_cnt + session_decr + decrement_for_polarity;

		if(mode->termination_cnt > 0)
		{
			sna_deactivate_pending_sessions(chg_sess->lu_name, 
				chg_sess->mode_name);
		}
		if(mode->termination_cnt > 0)
		{
			sna_deactivate_free_sessions(chg_sess->lu_name, 
				chg_sess->mode_name);
		}
	}

	if((mode->sessions == 0 && mode->drain_self == SNA_RM_FALSE)
		|| (mode->active.sessions - (mode->pending.conwinners + mode->pending.conlosers) == 0))
	{
		struct sna_get_session *get_session;

		while((get_session = sna_get_waiting_sessions(chg_sess->lu_name, chg_sess->mode_name)) != NULL)
		{
			struct sna_session_allocated *sallocated;
			sallocated->rcode = UNSUCCESSFUL_NO_RETRY;
			sna_ps(sallocated);
			bm_free(FREE, get_session);
		}
	}

	sna_activate_needed_sessions(chg_sess->lu_name, chg_sess->mode_name);

	return (0);
}

static int sna_check_for_bis_reply(__u8 hs_id)
{
	struct sna_mode *mode;
	struct sna_get_session *get_session;

	mode = sna_search_mode(hs_id);
	if(mode->drain_self == SNA_RM_FALSE 
		|| (sna_get_waiting_gsess(mode->lu_name,mode->mode_name)==NULL))
	{
		if(sess_free_brackets(hs_id))
		{
			sna_send_bis_reply(hs_id);
			remove_from_free_sess_pool;
		}
	}

	return (0);
}

static int sna_create_scb(unsigned char *lu_name, unsigned char *mode_name,
				struct sna_session_info *session_info)
{
	struct sna_scb *scb;

	new(scb, GFP_ATOMIC);
	scb->hs_id 		= session_info->hs_id;
	scb->lu_name 		= lu_name;
	scb->mode_name		= mode_name;
	scb->rcb_id		= 0;
	scb->session_id		= session_info->session_id;
	scb->send_ru_size 	= session_info->send_ru_size;
	scb->limit_buf_pool_id	= session_info->limit_buf_pool_id;
	scb->perm_buf_pool_id 	= session_info->perm_buf_pool_id;
	scb->bracket_id		= 0;
	scb->random		= session_info->random;
	scb->limit_resource 	= session_info->limit_resource;
	scb->timer_unique_id 	= 0;
	scb->conversation.gid	= conv_gid++;

	if(session_info->bracket_type == SNA_SESSION_FIRST_SPEAKER)
	{
		sna_fsm_bis(FSM_BIS_FSP);
		sna_fsm_scb_status(FSM_SCB_STATUS_FSP);
		scb->first_speaker = SNA_RM_TRUE;
	}

	if(session_info->bracket_type == SNA_SESSION_BIDDER)
	{
		sna_fsm_bis(FSM_BIS_BIDDER);
		sna_fsm_scb_status(FSM_SCB_STATUS_BIDDER);
		scb->first_speaker = SNA_RM_FALSE;
	}

	return (0);
}

static int sna_deactivate_free_sessions(unsigned char *lu_name, 
	unsigned char *mode_name)
{
	struct sna_scb *scb;

	/* XXXX */
	while(free_session)
	{
		polarity = sna_session_deactivation_polarity(lu_name,mode_name);
		scb = sna_search_scb(lu_name, mode_name);
		remove_from_free_session_pool;
		sna_send_bis(scb->hs_id);
	}

	return (0);
}

/* Saving it for a little later, when fsp functions firm up */
static int sna_deactivate_pending_sessions(unsigned char *lu_name, 
	unsigned char *mode_name)
{
	struct sna_mode *mode;

	mode = sna_search_mode(mode_name);

	return (0);
}

static int sna_dequeue_waiting_request(__u8 hs_id)
{
	struct sna_mode *mode;
	struct sna_get_session *get_session;

	mode = sna_search_mode(hs_id);
	if((get_session = sna_get_waiting_gsess(hs_id)) != NULL)
	{
		/* Remove from waiting queue */

		sna_get_session_proc(get_session);
	}

	return (0);
}

static int sna_enqueue_free_scb(__u8 hs_id)
{
	struct sna_mode *mode;
	struct sna_scb *scb;

	scb = sna_search_scb(hs_id);
	if(nobody_is_using_this_scb)
	{
		if(timer_is_to_be_set)
			start_timer(scb->timer);
		scb->timer_unique_id = timer_id++;
	}
	put_scb_in_free_pool(scb);	/* ??? */

	return (0);
}

static int sna_first_speaker_proc(struct sna_get_session *get_session, 
	__u8 hs_id)
{
	struct sna_scb *scb;
	struct sna_session_allocated *session_allocd;

	sna_set_rcb_and_scb_fields(get_session->rcb_id, hs_id);
	sna_connect_rcb_and_scb(get_session->rcb_id, hs_id);

	new(session_allocd, GFP_ATOMIC);
	if (!session_allocd)
		return -ENOMEM;
	session_allocd->rcode = OK;

	scb = sna_search_scb(hs_id);
	session_allocd->send_ru_size		= scb->send_ru_size;
	session_allocd->limit_buf_pool_id	= scb->limit_buf_pool_id;
	session_allocd->perm_buf_pool_id	= scb->perm_buf_pool_id;
	session_allocd->in_conver 		= SNA_RM_FALSE;

	sna_ps(session_allocd);

	return (0);
}

static int sna_free_session_proc(struct sna_mu *mu)
{
	struct sna_free_session *free = (struct sna_free_session *)mu->record_ptr;
	struct sna_scb *scb;
	struct sna_rcb *rcb;
	struct sna_rtr_rq *rtr_rq;
	struct sna_bracket_freed *bracket_freed;
	struct sna_get_session *get_session;
	int send_now;

	scb = sna_search_scb(free->hs_id);
	rcb = sna_search_rcb(scb->rcb_id);

	if(rcb == NULL)
	{
		new(bracket_freed, GFP_ATOMIC);
		bracket_freed->bracket_id = scb->bracket_id;
		sna_hs(bracket_freed);
	}

	scb->rcb_id = NULL;
	if(sna_fsm_scb_status() == PENDING_FMH12)
	{
		sna_send_deactivate_session(active, scb->hs_id, abnormal, 0x080F6051);
		return (-1);
	}
	else
		sna_fsm_scb_status(R, FREE_SESSION, UNDEFINED);

	if(sna_fsm_rcb_status() == PENDING_SCB && rcb->hs_id == scb->hs_id)
		return (0);
	if(scb->rtr_owed == SNA_RM_TRUE)
	{
		if(scb->first_speaker == SNA_RM_TRUE)
		{
			if(no_get_session_rqs_waiting)
			{
				if(rtr_is_to_be_sent_now)
				{
					sna_hs(rtr_rq);
					scb->rtr_owed = SNA_RM_FALSE;
				}
				else
					sna_enqueue_free_scb(scb->hs_id);
				return (0);
			}
			else
				return (0);
		}
	}

	send_now = sna_should_send_bis(scb->hs_id);
	if(send_now)
		sna_send_bis(scb->hs_id);
	if(sna_fsm_bis() == BIS_SENT || sna_fsm_bis() == CLOSED)
		return (0);
	else
	{
		sna_enqueue_free_scb(scb->hs_id);
		if(get_sessions_waiting)
			sna_dequeue_waiting_request(scb->hs_id);
	}

	bm_free(FREE, free_session);

	return (0);
}

static int sna_ps_abend_proc(struct sna_mu *mu)
{
	struct sna_abend_notify *abend 
		= (struct sna_abend_notify *)mu->record_ptr;
	struct sna_tcb *tcb;
	struct sna_scb *scb;
	struct sna_rcb *rcb;
	struct sna_mu *mu;
	struct sna_start_tp *tp;
	struct sna_mode *mode;
	struct sna_deactivate_session *deactivate_session;

	tcb = sna_search_tcb(abend);
	sna_destroy_queued_get_sessions(abend);
	if(tcb != NULL)
	{
		for(each_rcb_associated_w_abended_PS;;)
		{
			if(sna_fsm_rcb_status() == FREE)
				mode = sna_search_mode(rcb->lu_name, rcb->mode_name);
			if(sna_fsm_rcb_status() == IN_USE 
				|| sna_fsm_rcb_status() == PENDING_SCB)
			{
				scb = sna_search_scb(rcb->hs_id);
				bm_free(FREE, rcb);
				if(scb != NULL)
				{
					new(deactivate_session, GFP_ATOMIC);
					deactivate_session->status = ACTIVE;
					deactivate_session->hs_id = scb->hs_id;
					deactivate_session->type = ABNORMAL;
					deactivate_session->sense = 0x08640000;
					sna_sm(deactivate_session);	/* ?? */

					new(session_deactivated, GFP_ATOMIC);
					session_deactivated->hs_id = scb->hs_id;
					session_deactivated->reason = ABNORMAL;
					session_deactivated->sense = 0x08640000;
					sna_session_deactivated_proc(session_deactivated);
				}
			}
		}

		tp = sna_search_tp(tcb->tp_name);
		if(tp != NULL)
		{
			tp->tp_cnt--;
			if(init_req_queued_for_tp && tp->tp_cnt < tp->max_tp)
			{
				remove_init_req_from_queue;
				if(init_req_is_an_mu(containing_attach)
				{
					rcb = sna_search_rcb(mu->layer.rm_to_ps.rcb_id);
					mu->layer.hs_to_rm.hs_id = rcb->hs_id;
					scb = sna_search_rcb(rcb->hs_id);
					sna_fsm_scb_status(r, free_session, undefined);
					sna_fsm_scb_status(r, bid, undefined);
					scb->bracket_id = NULL;
					scb->rcb_id = NULL;
					destroy_rcb(rcb);
					sna_attach_proc(mu);
				}

				if(queued_init_req == START_TP)
					sna_start_tp_proc(start_tp);
			}
		}
	}

	destroy_tcb(tcb);

	return (0);
}

static int sna_ps_creation_proc(struct sna_mu *mu, __u8 *tcb_id, __u8 *rcb_id, 
	struct sna_tp *target_tp)
{
	struct sna_scb *scb;
	struct sna_rcb *rcb;
	struct sna_tcb *tcb;
	struct sna_lucb *lucb;

	new(tcb, GFP_ATOMIC);
	if (!tcb)
		return -ENOMEM;
	tcb->tcb_id 	= sys_tcb_id_cnt++;
	tcb->tp_name 	= mu->attach.tp_name;
	tcb->ctrl_cmpnt = tp;
	tcb->own_lu_id	= lucb->lu_id;

	/* Colapse these later */
	if(attach->security.user_id != NULL)
		tcb->security.user_id = attach->security.user_id;
	else
		tcb->security.user_id = NULL;

	if(attach->security.profile != NULL) 
		tcb->security.profile = attach->security.profile;
	else
		tcb->security.profile = NULL;

	if(attach->luw_id != NULL)
		tcb->luw_id = attach->luw_id;
	else
	{
		tcb->luw_id.fq_lu_name = lucb->fq_lu_name;
		sna_complete_luw_id(tcb);
	}

	scb = sna_search_scb(mu->layer.hs_to_rm.hs_id);
	new(rcb, GFP_ATOMIC);
	rcb->rcb_id 	= sys_rcb_id_cnt++;
	rcb->tcb_id 	= tcb->tcb_id;
	rcb->lu_name 	= scb->lu_name;
	rcb->mode_name 	= scb->mode_name;
	rcb->tp_name 	= attach->tp_name;
	rcb->bracket_id = sys_bracket_id_cnt++;
	rcb->sync_level = attach->sync;
	rcb->hs_to_ps_buffer_list = NULL;

	if(attach->conversation != NULL)
		rcb->conversation = attach->conversation;
	else
		rcb->conversation = NULL;

	if(first_speaker == SNA_RM_TRUE)
		sna_fsm_rcb_status(FSM_RCB_STATUS_FSP);
	else
		sna_fsm_rcb_status(FSM_RCB_STATUS_BIDDER);

	sna_fsm_rcb_status(r, attach, hs);
	rcb->hs_id = mu->layer.hs_to_rm.hs_id;

	/* Initlize PS_CREATE_PARMS */

	if(ps != NULL)
	{
		create_rc = SUCCESS;
		if(tp != NULL)
			tp->tp_cnt++;
	}
	else
	{
		create_rc = FAILURE;
		bm_free(FREE, tcb);
		bm_free(FREE, rcb);
	}

	return (0);
}

static int sna_ps_termination_proc(struct sna_mu *mu)
{
	struct sna_terminate_ps *term_ps = (struct sna_terminate_ps *)mu->record_ptr;
	struct sna_lucb *lucb;
	struct sna_tp *tp;
	struct sna_mu *mu;
	struct sna_start_tp *start_tp;
	struct sna_start_tp_reply *start_tp_reply;
	struct sna_hs_ps_connected *hs_ps_connected;
	struct sna_tcb *tcb;
	struct sna_rcb *rcb;

	tcb = sna_search_tcb(term_ps);
	tp  = sna_search_tp(term_ps);
	if(tp != sna_NULL)
	{
		if(queued_init_req(tp) == SNA_RM_TRUE 
			&& tp->max_tp != tp->tp_cnt)
		{
			switch(first_queued_req_rec_type)
			{
				case (MU_ATTACH):
					mu->layer.rm_to_ps.tcb_id = tcb->tcb_id;
					tcb->ctrl_cmpnt = tp;
					if(attach->security != NULL)
						tcb->security = attach->security;
					if(attach->luw_id != NULL)
						tcb->luw_id = attach->luw_id;
					else
					{
						rcb->luw_id.fq_lu_name = lucb->fq_lu_name;
						sna_complete_luw_id(tcb);
					}

					rcb = sna_search_rcb(mu->layer.rm_to_ps.rcb_id);
					rcb->tcb_id = tcb->tcb_id;
					new(hs_ps_connected, GFP_ATOMIC);
					hs_ps_connected->bracket_id = rcb->bracket_id;
					hs_ps_connected->ps_id = rcb->tcb_id;

					sna_hs(hs_pd_connected);
					err = sna_ps(mu);
					if(err < 0)
						bm_free(FREE, mu);
					break;

				case (START_TP):
					start_tp->tcb_id = tcb->tcb_id;
					tcb->luw_id.fq_lu_name = start_tp->fq_lu_name;
					sna_complete_luw_id(tcb);
					tcb->ctrl_cmpnt = tp;
					if(start_tp->security_select 
						== SNA_SECURITY_PGM)
					{
						tcb->security = start_tp->security;
					}
					else
						tcb->security = NULL;

					if(start_tp->reply == SNA_RM_TRUE)
					{
						new(start_tp_reply, GFP_ATOMIC);
						start_tp_reply->rcode = OK;
						start_tp_reply->tcb_id = start_tp->tcb_id;
						send_to_caller(start_tp_reply);
					}
					break;
			}
		}
	}
	else
	{
		tp->tp_cnt--;
		destroy_tcb(tcb);
		destroy_ps(terminate_ps);
	}

	bm_free(terminate_ps);

	return (0);
}

static int sna_purge_queued_requests(struct sna_tp *tp)
{
	struct sna_start_tp_reply *start_tp_reply;
	struct sna_start_tp *start_tp;
	struct sna_mu *mu;
	struct sna_rcb *rcb;
	struct sna_scb *scb;
	struct sna_deactivate_session *deactivate_session;
	struct sna_session_deactivated *session_deactivated;

	if(tp != NULL)
	{
		while((request_type = get_wait_reqs(tp)) != NULL)
		{
			switch(request_type)
			{
				case (MU_ATTACH):
					rcb = sna_search_rcb(mu->layer.rm_to_ps.rcb_id);
					scb = sna_search_scb(rcb->hs_id);

					destroy_rcb(rcb);
					bm_free(FREE, mu);
					if(scb != NULL)
					{
						new(deactivate_session, GFP_ATOMIC);
						deactivate_session->status = ACTIVE;
						deactivate_session->hs_id = scb->hs_id;
						deactivate_session->type = ABNORMAL;
						deactivate_session->sense = 0x08640000;

						sna_sm(deactivate_session);
						new(session_deactivated, GFP_ATOMIC);
						session_deactivated->hs_id = scb->hs_id;
						session_deactivated->reason = ABNORMAL_RETRY;
						session_deactivated->sense = 0x08640000;
						sna_session_deactivated_proc(session_deactivated);
					}
					break;

				case (START_TP):
					if(start_tp->reply == SNA_RM_TRUE)
					{
						new(start_tp_reply, GFP_ATOMIC);
						start_tp_reply->rcode = PS_CREATION_FAILURE;
						start_tp_reply->tcb_id = NULL;
						send_tp_init_proc(start_tp_reply);
					}

					destroy_start_tp(start_tp);
					break;
			}
		}
	}


	return (0);
}

static int sna_queue_attach_proc(struct sna_mu *mu)
{
	struct sna_rcb *rcb;
	struct sna_scb *scb;

	new(rcb, GFP_ATOMIC);
	rcb->rcb_id 		= sys_rcb_id_cnt++;
	rcb->tcb_id 		= NULL;
	rcb->tp_name 		= mu->fmh5->tp_name;
	rcb->hs_id 		= mu->layer.hs_to_rm.hs_id;
	rcb->bracket_id 	= sys_bracket_id_cnt++;
	rcb->sync 		= mu_fmh5->sync;
	rcb->hs_buffer_list 	= NULL;

	if(mu->fsmh5.conversation != NULL)
		rcb->conversation = mu->fmh5.conversation;
	else
		rcb->conversation = NULL;

	scb = sna_search_scb(mu->layer.hs_to_rm.hs_id);
	rcb->lu_name 		= scb->lu_name;
	rcb->mode_name 		= scb->mode_name;

	if(scb->first_speaker == SNA_RM_TRUE)
		sna_fsm_rcb_status(FSM_RCB_STATUS_FSP);
	else
		sna_fsm_rcb_status(FSM_RCB_STATUS_BIDDER);
	sna_fsm_rcb_status(r, attach, hs);

	scb->bracket_id 	= rcb->bracket_id;
	scb->rcb_id		= rcb->rcb_id;
	rcb->conversation.gid	= scb->conversation.gid;
	rcb->session_id		= scb->session_id;
	sna_fsm_scb_status(r,

	mu->layer.rm_to_ps.rcb_id		=
	mu->layer.rm_to_ps.send_ru_size
	mu->layer.rm_to_ps.limit_buf_pool_id 	= scb->limit_buf_pool_id;
	mu->layer.rm_to_ps.perm_buf_pool_id 	= scb->perm_buf_pool_id;
	mu->layer.rm_to_ps.sense		= 0;

	queue_attach_mu(mu);	/* What about rcb ?? */

	return (0);
}

static int sna_rm_deactivate_conv_group_proc(struct sna_mu *mu)
{
	struct sna_rm_deactivate_conv_group *rm_dact_conv_group
		= (struct sna_rm_deactivate_conv_group *)mu->record_ptr;
	struct sna_rm_deactivate_session *rm_deactivate;
	struct sna_scb *scb;

	while((scb = sna_search_scb_gid(rm_dact_conv_group->gid)) != NULL)
	{
		new(rm_deactivate, GFP_ATOMIC);
		rm_deactivate->tcb_id 		= NULL;
		rm_deactivate->session_id 	= scb->hs_id;
		rm_deactivate->type 		= rm_dact_conv_group->type;
		rm_deactivate->sense		= rm_dact_conv_group->sense;

		sna_rm_deactivate_session_proc(rm_deactivate);
	}

	destroy_rm_dact_conv_group(rm_dact_conv_group);

	return (0);
}

static int sna_rm_deactivate_session_proc(struct sna_rm_deactivate_session *rm_deactivate)
{
	struct sna_scb *scb;

	scb = sna_search_scb(rm_deactivate->session_id);
	if(scb != NULL)
	{
		switch(rm_deactivate->type)
		{
			case (CLEANUP):
				sna_send_deactivate_session(ACTIVE, rm_deactivate->session_id, CLEANUP, rm_deactivate);
				kfree(rm_deactivate);
				break;

			case (NORMAL):
				if(session_in_free_session_pool)
				{
					if(sna_fsm_bis() != BIS_SENT)
						queue_deact(rm_deactivate);
					else
						kfree(rm_deactivate);
				}
				else
				{
					queue_deact(rm_deactivate);
					sna_send_bis_rq(hs_id);
					remove_from_free_pool(hs_id);
				}
				break;
		}
	}
	else
		kfree(rm_deactivate);

	return (0);
}

static int sna_rm_timer_deactivate_session_proc(struct sna_rm_timer_pop *rm_timer_pop)
{
	struct sna_scb *scb;
	struct sna_mode *mode;
	struct sna_rm_deactivate_session *rm_deactivate;

	mode = sna_search_mode(rm_timer_pop->lu_name, rm_timer_pop->mode_name);
	scb = sna_search_scb(rm_timer_pop->unique_id);
	if(scb != NULL)
	{
		if(scb_in_free_ses_pool && scb->first_speaker == SNA_RM_TRUE
			&& mode->active.conwinners + mode->pending.conwinners 
			> mode->auto_activate_limit)
		{
			new(rm_deactivate, GFP_ATOMIC);
			rm_deactivate_session_proc(rm_deactivate);
		}
	}

	kfree(rm_timer_pop);

	return (0);
}

static int sna_rtr_rq_proc(struct sna_mu *mu)
{
	struct sna_rtr_rq *rtr_rq = (struct sna_rtr_rq *)mu->record_ptr;
	struct sna_get_session *get_session;
	struct sna_rtr_rsp *rtr_rsp;
	struct sna_scb *scb;

	scb = sna_search_scb(rtr_rq);
	if(scb->rtr_owed == SNA_RM_TRUE)
	{
		if((get_session = get_waiting_get_session()) != NULL)
		{
			sna_enqueue_free_scb(scb->hs_id);
			new(rtr_rsp, GFP_ATOMIC);
			rtr_rsp->rti = POS;
			rtr_rsp->sense = 0x00000000;
			sna_hs(rtr_rsp);
			remove_get_session_wait(get_session);
			sna_get_session_proc(get_session);
		}
		else
		{
			new(rtr_rsp, GFP_ATOMIC);
			rtr_rsp->rti = NEG;
			rtr_rsp->sense = 0x8190000;
			send_to_hs(rtr_rsp);
			bis_send = sna_should_send_bis(rtr_rq->hs_id);
			if(bis_send != NULL)
				sna_send_bis(rtr_rq->hs_id);
			else
				sna_enqueue_free_scb(scb->hs_id);
		}
		scb->rtr_owed = SNA_RM_FALSE;
	}
	else
		sna_send_deactivate_session(ACTIVE, rtr_rq->hs_id, ABNORMAL, 0x2003000);

	return (0);
}

static int sna_rtr_rsp_proc(struct sna_mu *mu)
{
	struct sna_rtr_rsp *rtr_rsp = (struct sna_rtr_rsp *)mu->record_ptr;

	if(rtr_rsp->rti == NEG && fsm_bis == RESET)
	{
		bis_send = sna_should_send_bis(rtr_rsp->hs_id);
		if(bis_send != NULL)
			sna_send_bis(rtr_rsp->hs_id);
		else
		{
			sna_enqueue_free_scb(scb->hs_id);
			sna_dequeue_waiting_req(rtr_rsp->hs_id);
		}
	}

	kfree(rtr_rsp);

	return (0);
}

static int sna_security_proc(struct sna_mu *mu)
{
	struct sna_lucb *lucb;
	struct sna_scb *scb;

	scb = sna_search_scb(mu->layer.hs_to_rm.hs_id);
	remove_random_data();

	if(sna_fsm_scb_status() != PENDING_FMH12
		|| fmh12->length != 10
		|| fmh12->security_reply != expected)
	{
		sna_send_deactivate_session(ACTIVE, scb->hs_id, ABNORMAL, 
			0x080F6051);
	}
	else
		sna_fsm_scb_status(R, FMH_12, UNDEFINED);

	bm_free(FREE_BUFFER, mu);

	return (0);
}

static int sna_send_bis(__u8 hs_id)
{
	switch(sna_fsm_bis())
	{
		case (RESET):
			sna_send_bis_rq(hs_id);
			break;

		case (BIS_RCVD):
			sna_send_bis_reply(hs_id);
			break;

		default:
			break;
	}

	return (0);
}

static int sna_send_bis_reply(__u8 hs_id)
{
	struct sna_mode *mode;
	struct sna_bis_reply *bis_reply;

	sna_fsm_bis(s, BIS_REPLY, hs_id);

	new(bis_reply, GFP_ATOMIC);
	send_to_hs(bis_reply);

	mode = sna_search_mode(hs_id);

	if(mode->polarity == FREE_SPEAKER)
		mode->termination.conwinners++;
	else
		mode->termination.conlosers++;

	return (0);
}

static int sna_send_bis_rq(__u8 hs_id)
{
	struct sna_mode *mode;
	struct sna_bis_rq *bis_rq;

	new(bis_rq, GFP_ATOMIC);
	sna_send_to_hs(bis_rq);

	sna_fsm_bis(s, bis_rq, hs_id);

	mode = sna_search_mode(hs_id);

	if(mode->polarity == SNA_SESSION_FIRST_SPEAKER)
		mode->termination.conwinners++;
	else
		mode->termination.conlossers++;

	if(pending_cnos(hs_id))
		discard_all_queued(rm_deactivate_session);
	else
		mode->termnination.sessions--;

	return (0);
}

static int sna_send_deactivate_session(__u8 status, __u8 correlator, __u8 type, 
	__u32 sense)
{
	struct sna_pending_activation *pending;
	struct sna_deactivate_session *deactivate;
	struct sna_session_deactivated *deactivated;
	struct sna_mode *mode;
	struct sna_scb *scb;

	switch(status)
	{
		case (PENDING):
			pending = sna_search_pending(correlator);
			if(pending != NULL)
			{
				new(deactivate, GFP_ATOMIC);
				deactivate->status 	= PENDING;
				deactivate->correlator 	= correlator;
				deactivate->type	= type;
				deactivate->sense	= sense;
				send_to_sm(deactivate);

				mode = sna_search_mode();
				mode->pending.sessions--;

				kfree(pending_activation);

				if((mode->active.sessions + mode->pending.sessions) == 0)
				{
					while((get_session = get_waiting_gsessions()) != NULL)
					{
						new(session_allocated, GFP_ATOMIC);
						session_allocated->rcode = UNSUCCESSFUL_NO_RETRY;
						send_to_ps(session_allocated);
						kfree(get_session);
					}
				}

				if((mode->active.conwinners + mode->pending.conwinners) == 0)
				{
					while((get_session = get_waiting_gsessions()) != NULL)
					{
						if(get_session->type != CONWINNER)
						{
							continue;
						}

						new(session_allocated, GFP_ATOMIC);
						session_allocated->rcode = UNSUCCESSFUL_NO_RETRY;
						send_to_ps(session_allocated);
						kfree(get_session);
					}
				}
			}
			break;

		case (ACTIVE):
			scb = sna_search_scb(correlator);
			if(scb != NULL)
			{
				new(deactivate, GFP_ATOMIC);
				deactivate->hs_id	= correlator;
				deactivate->type	= type;
				deactivate->sense	= sense;

				send_to_sm(deactivate_session);

				new(deactivated, GFP_ATOMIC);
				if(type == NORMAL)
					deactivated->reason = NORMAL;
				else
				{
					deactivated->reason = ABNORMAL_NO_RETRY;
					deactivated->sense = sense;
				}
				sna_session_deactivated_proc(deactivated);
			}
			break;
	}

	return (0);
}

static int sna_send_rtr_proc(struct sna_mu *mu)
{
	struct sna_send_rtr *send_rtr = (struct sna_send_rtr *)mu->record_type;
	struct sna_scb *scb;
	struct sna_rtr_rq *rtr_rq;

	scb = sna_search_scb(send_rtr->hs_id);
	if(scb != NULL)
	{
		if(scb->first_speaker == SNA_RM_TRUE && scb_is_free)
		{
			new(rtr_rq, GFP_ATOMIC);
			sna_send_to_hs(rtr_rq);

			scb->rtr_owed = SNA_RM_FALSE;
			remove_from_free_pool(scb);
		}
	}

	kfree(send_rtr);

	return (0);
}

static int sna_session_activated_allocation(struct sna_get_session *get, __u8 hs_id)
{
	struct sna_scb *scb;
	struct sna_session_allocated *allocated;

	if(session == SNA_SESSION_BIDDER)
	{
		sna_fsm_rcb_status(s, get_session, UNDEFINED);
	}

	sna_set_rcb_and_scb_fields(get->rcb_id, hs_id);
	sna_connect_rcb_and_scb(get->rcb_id, hs_id, NORMAL);

	scb = sna_search_scb(hs_id);
	new(allocated, GFP_ATOMIC);
	allocated->rcode 		= OK;
	allocated->send_ru_size 	= scb->send_ru_size;
	allocated->limit_buf_pool_id 	= scb->limit_buf_pool_id;
	allocated->perm_buf_pool_id 	= scb->perm_buf_pool_id;
	allocated->in_conver 		= SNA_RM_TRUE;

	send_to_ps(allocated);

	return (0);
}

static int sna_session_deactivated_proc(struct sna_session_deactivated *deactivated)
{
	struct sna_mu *mu;
	struct sna_get_session *get_session;
	struct sna_scb *scb;
	struct sna_rcb *rcb;
	struct sna_mode *mode;
	struct sna_partner_lu *partner;
	struct sna_conv_failure *conv_fail;

	scb = sna_search_scb(deactivated->hs_id);
	if(scb != NULL)
	{
		mode = sna_search_mode(deactivated->lu_name, deactivated->mode_name);
		if(sna_fsm_scb_status() == IN_USE)
		{
			if((rcb = sna_search_rcb(scb->rcb_id)) != NULL)
			{
				if(rcb->tcb_id != NULL)
				{
					new(conv_fail, GFP_ATOMIC);
					conv_fail->rcb_id = scb->rcb_id;
					switch(deactivated->reason)
					{
						case (NORMAL):
						case (ABNORMAL_RETRY):
							conv_fail->reason = SON;
							break;

						case (ABNORMAL_NO_RETRY):
							conv_fail->reason = PROTO_VIOLATION;
							break;
					}
					send_to_ps(conv_fail);
				}
				else
				{
					mu = search_mu(rcb->tp_name, rcb->rcb_id);
					kfree(rcb);
					remove_queue(mu);
					bm_free(FREE, mu);
				}
			}
		}
		else
			remove_from_free_sess(session);

		rcb = sna_search_rcb(deactivated->hs_id);
		if(rcb != NULL)
		{
			if(sna_fsm_rcb_status() == PENDING_SCB)
			{
				rcb->hs_id = NULL;
				sna_fsm_rcb_status(r, NEG_BID_RSP, UNDEFINED);
				new(get_session, GFP_ATOMIC);
				sna_get_session_proc(get_session);
			}
		}

		if(mode->polarity == SNA_SESSION_FIRST_SPEAKER)
			mode->active.conwinners--;
		else
			mode->active.conlosers--;
		mode->active.sessions--;

		if(session->pending.deactivation == SNA_RM_TRUE)
		{
			if(mode->polarity == SNA_SESSION_FIRST_SPEAKER)
				mode->termination.conwinners--;
			else
				mode->termination.conlosers--;
		}

		if(deactivated->reason != ABNORMAL_NO_RETRY)
			sna_activate_needed_sessions(scb->lu_name, scb->mode_name);

		if((mode->active.sessions + mode->pending.sessions) == 0)
		{
			if(partner->parallel == SNA_RM_FALSE)
			{
				if(waiting_request)
					sna_activate_needed_sessions(partner->local_lu_name, mode->name);
			}

			while((get_session = get_sessions()) != NULL)
			{
				new(allocated, GFP_ATOMIC);
				allocated->rcode = UNSUCCESSFUL_NO_RETRY;
				send_to_ps(allocated);
				kfree(get_session);
			}

			while((rm_activate = get_rm_activate_pend()) != NULL)
			{
				new(rm_activated, GFP_ATOMIC);
				rm_activated->rcode = ACTIVATION_FAILURE_NO_RETRY;
				send_to_ps(rm_activated);
				kfree(rm_activate);
			}
		}

		kfree(scb);
	}

	kfree(deactivated);

	return (0);
}

static int sna_session_deactivation_polarity(unsigned char *lu_name, unsigned char *mode_name)
{
	struct sna_mode *mode;

	mode = sna_search_mode(lu_name, mode_name);
	if(mode->term_count == 0)
		return (DEACTIVATE);
	conwinner_cnt = mode->active.conwinners + mode->pending.conwinners 
		- mode->terminator.conwinners;
	conloser_cnt = mode->active.conlosers + mode->pending.conlosers 
		- mode->termination.conlosers;

	if(conwinner_cnt <= mode->min_conwinners
		&& conloser_cnt <= mode->min_conlosers)
	{
		mode->term_count = 0;
		return (SNA_SESSION_NONE);
	}

	if(conwinner_cnt <= mode->min_conwinners
		&& conloser_cnt > mode->min_conlosers)
	{
		return (SNA_SESSION_BIDDER);
	}

	if(conwinner_cnt > mode->min_conwinner
		&& conloser_cnt <= mode->min_conlosers)
	{
		return (SNA_SESSION_FIRST_SPEAKER);
	}

	if(conwinner_cnt > mode->min_conwinners
		&& conloser_cnt > mode->min_conlosers)
	{
		return (SNA_SESSION_EITHER);
	}
	
	return (0);
}

static int sna_should_send_bis(__u8 hs_id)
{
	struct sna_partner *plu;
	struct sna_mode *mode;
	int polarity;

	mode = sna_search_mode(hs_id);
	plu = mode->partner;

	if(!waiting_req(mode) && plu->parallel == NOT_SUPPORTED)
	{
		if(waiting_req(plu))
			return (SNA_RM_TRUE);
	}

	switch(sna_fsm_bis())
	{
		case (RESET):
			polarity = sna_session_deactivation_polarity(lu_name, mode_name);
			if(polarity == SNA_SESSION_EITHER 
				|| polarity_mode->polarity)
			{
				if(mode->drain_self == SNA_RM_FALSE 
					|| !waiting_req())
					return (SNA_RM_TRUE);
				if(pending_rm_deactivate_session)
					return (SNA_RM_FALSE);
				return (SNA_RM_FALSE);
			}
			break;

		case (BIS_RCVD):
			if(mode->drain_self == SNA_RM_FALSE || !waiting_req())
				return (SNA_RM_TRUE);
			else
				return (SNA_RM_FALSE);

		case (BIS_SENT):
			return (SNA_RM_FALSE);

		default:
			return (-ERROR);
	}

	return (0);
}

static int sna_start_tp_security_valid(struct sna_start_tp *start_tp)
{
	if(start_tp->security_select == NULL)
	{
		if(tp_requires_security)
			return (SNA_RM_FALSE);
		else
			return (SNA_RM_TRUE);
	}

	if(start_tp->security.profile != NULL
		&& start_tp->security.user_id == NULL)
	{
		return (SNA_RM_FALSE);
	}

	if(start_tp->security.passwd != NULL
		&& start_tp->security.user_id == NULL)
	{
		return (SNA_RM_FALSE);
	}

	if(start_tp->security_select ==SNA_SECURITY_PGM)
	{
		if(start_tp->security.user_id != NULL
			&& start_tp->security.passwd == NULL)
		{
			return (SNA_RM_FALSE);
		}

		if(start_tp->security.user_id == NULL
			|| (start_tp->security.passwd == NULL
			&& tp_requires_security))
		{
			return (SNA_RM_FALSE);
		}

		if(!tp requires security
			&& (start_tp->security.user_id == NULL
			|| start_tp->security.passwd == NULL))
		{
			return (SNA_RM_TRUE);
		}

		if(start_tp->security is bad combination)
		{
			return (SNA_RM_FALSE);
		}

	}
	else
	{
		if(start_tp->security.user_id == NULL
			|| start_tp->security.passwd == NULL)
		{
			return (SNA_RM_FALSE);
		}

	}

	if(limit_access_to_tp)
	{
		if(sna_access_ok(start_tp->security.user_id, 
			start_tp->security.profile))
		{
			return (SNA_RM_TRUE);
		}
		else
		{
			return (SNA_RM_FALSE);
		}
	}
		
	return (SNA_RM_TRUE);
}

static int sna_successful_session_activation(unsigned char *lu_name, 
	unsigned char *mode_name, struct sna_session_information *info)
{
	struct sna_scb *scb;
	struct sna_get_session *session;
	struct sna_session_allocated *allocd;
	struct sna_rm_hs_connected *connected;
	struct sna_rm_activate_sesion *activate;
	struct sna_rm_session_activated *activated;

	sna_create_scb(lu_name, mode_name, info);
	new(connected, GFP_ATOMIC);
	sna_send_to_hs(connected);

	if(info->first_speaker != SNA_RM_TRUE && plu->parallel != SNA_RM_TRUE)
	{
		while((session = get_wait_sessions(CONWINNER)) != NULL)
		{
			new(allocd, GFP_ATOMIC);
			allocd->rcode = UNSUCCESSFUL_NO_RETRY;
			send_to_ps(allocd);
			remove_from_list(allocd);
		}
	}

	if(info->primary_hs == SNA_RM_TRUE)
	{
		sna_fsm_scb_status(r, activated, PRI);
		while(activated)
		{
			if(session == waiting)
			{
				if(session->security.level != waiting_req->security.level)
				{
					session->security.level = NONE;
				}

				if(session->sync_level != waiting_req->sync_level)
				{
					new(allocd, GFP_ATOMIC);
					allocd->rcode = SYNC_LEVEL_NOT_SUPPORTED;
					send_to_ps(allocd);
					kfree(session);
				}
				else
				{
					if(scb->random != NULL)
					{
						new(sec2, GFP_ATOMIC);
						sec2->send_parm.allocate = NO;
						sec2->send_parm.fmh = YES;
						sec2->send_parm.type = FLUSH;
						sna_send_to_hs(sec2);
						sna_session_activated_alloc(session, scb->hs_id);
						kfree(session);
					}
				}
			}
			else
			{
				sna_fsm_scb_status(s, YEILD_SESSION, undefined);
				if(scb->random_data != NULL)
				{
					new(sec2, GFP_ATOMIC);
					sec2->send_parm.allocate = NO;
					sec2->send_parm.fmh = YES;
					sec2->send_parm.type = DEALLOCATE_FLUSH;
					sna_send_to_hs(sec2);
				}
				else
				{
					new(yield, GFP_ATOMIC);
					sna_send_to_hs(yield);
				}
			}
		}
	}
	else
	{
		if(scb->random_data != NULL)
			sna_fsm_scb_status(r, activated, secure);
		else
			sna_fsm_scb_status(r, activated, sec);
	}

	activate = sna_rm_activate_pending();
	if(activate != NULL)
	{
		new(activated, GFP_ATOMIC);
		activated->rcode = OK;
		kfree(activate);
	}

	return (0);
}

static int sna_unsuccessful_session_activation(unsigned char *lu_name, unsigned char *mode_name, int err)
{
	struct sna_mode *mode;
	struct sna_session_allocated *allocd;

	mode = sna_search_mode(lu_name, mode_name);
	if(mode->active.sessions == 0 && mode->pending.sessions == 0)
	{
		while((waiting = waiting_req(lu_name, mode_name)) != NULL)
		{
			new(allocd, GFP_ATOMIC);

			if(err == CAN_RETRY)
				allocd->rcode = UNSUCCESSFUL_RETRY;
			else
				allocd->rcode = UNSUCCESSFUL_NO_RETRY;

			send_to_ps(allocd);
			kfree(waiting);
		}

		sessions = get_wait_sessions();
		if(plu->parallel != SNA_RM_TRUE && sessions != NULL)
		{
			sna_activate_needed_sessions(lu_name, mode_name);
		}
	}

	while(pending_rm_activate_reqs > mode->pending.sessions)
	{
		new(activated, GFP_ATOMIC);

		if(err == CAN_RETRY)
			activated->rcode = ACTIVATION_FAILURE_RETRY;
		else
			acitvated->rcode = ACTIVATION_FAILURE_NO_RETRY;

		send_to_ps(activated);
		kfree(activate);
	}

	if((mode->active.conwinners + mode->pending.conwinners) == 0)
	{
		while((get_session = get_wait_sessions()) != NULL)
		{
			new(allocd, GFP_ATOMIC);
			if(err == CAN_RETRY)
				allocd->rcode = UNSUCCESSFUL_RETRY;
			else
				allocd->rcode = UNSUCCESSFUL_NO_RETRY;
			send_to_ps(allocd);
			kfree(get_session);
		}
	}

	return (0);
}
#endif
