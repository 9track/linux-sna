/* sna_pc.c: Linux Systems Network Architecture implementation
 * - Path Control (Route message units between HS and DLC). 
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
#include <linux/ctype.h>
#include <linux/list.h>

#include <linux/sna.h>

static LIST_HEAD(pc_list);
static u_int32_t sna_pc_system_index = 0;

struct sna_pc_cb *sna_pc_get_by_index(u_int32_t index)
{
        struct sna_pc_cb *pc;
        struct list_head *le;

        sna_debug(5, "init\n");
        list_for_each(le, &pc_list) {
                pc = list_entry(le, struct sna_pc_cb, list);
                if (pc->index == index)
                        return pc;
        }
        return NULL;
}

static u_int32_t sna_pc_new_index(void)
{
        for (;;) {
                if (++sna_pc_system_index <= 0)
                        sna_pc_system_index = 1;
                if (sna_pc_get_by_index(sna_pc_system_index) == NULL)
                        return sna_pc_system_index;
        }
        return 0;
}

struct sna_pc_cb *sna_pc_find_by_netid(sna_netid *n)
{
	struct sna_pc_cb *pc;
	struct list_head *le;
	
	list_for_each(le, &pc_list) {
		pc = list_entry(le, struct sna_pc_cb, list);
		if (!strcmp(pc->cp_name.net, n->net)
			&& !strcmp(pc->cp_name.name, n->name))
			return pc;
	}
	return NULL;
}

/* Destroy a single PC instance. */
int sna_pc_destroy(u_int32_t index)
{
	struct list_head *le, *se;
	struct sna_pc_cb *pc;

	list_for_each_safe(le, se, &pc_list) {
		pc = list_entry(le, struct sna_pc_cb, list);
		if (pc->index == index) {
			list_del(&pc->list);
			kfree(pc);
			return 0;
		}
	}
	return -ENOENT;
}

/* create a pc instance, return the local pc_cb index. 
 **/
u_int32_t sna_pc_init(struct sna_pc_cb *pc, int *err)
{
        sna_debug(5, "init\n");
	if (!pc) {
		*err = -EINVAL;
		return 0;
	}
	*err = 0;
	pc->index = sna_pc_new_index();
	list_add_tail(&pc->list, &pc_list);
        return pc->index;
}

int sna_pc_rx_mu_chk_err(struct sk_buff *skb)
{
	sna_fid2 *fh = (sna_fid2 *)skb->data;
	int err = -EINVAL;

	sna_debug(5, "init: skb->len=%d\n", skb->len);

	/* validate the fid header, we only fid2 now. */
	if (skb->len < sizeof(sna_fid2)) {
		sna_debug(5, "mu smaller than fid2\n");
		/* 0x800b0000 */
		goto out;
	}
	if (fh->format != SNA_TH_FID2) {
		sna_debug(5, "not FID2 dropping\n");
		/* 0x80060000 */
		goto out;
	}
	skb->nh.fh = (sna_fid2 *)skb->data;
	skb_pull(skb, sizeof(sna_fid2));

	/* validate rh header. */
	if (skb->len < sizeof(sna_rh)) {
		sna_debug(5, "mu smaller than rh\n");
		/* 0x40050000 */
		goto out;
	}
	skb->h.rh = (sna_rh *)skb->data;
	skb_pull(skb, sizeof(sna_rh));	

	/* validate ru header. */
	/* not sure how to do this well yet. */	
	err = 0;
out:	return err;
}

/*
 * lfsid assignments - 2.6.2.2.5 (appnarch.boo)
 * lfsid mapping - 3.10.2.3.2 (appnarch.boo)
 */
struct sna_lfsid *sna_pc_rx_mu_map_lfsid(struct sk_buff *skb)
{
	sna_debug(5, "init\n");
	return sna_asm_lfsid_get_by_sidhl(skb->nh.fh->odai,
		skb->nh.fh->daf, skb->nh.fh->oaf);
}

int sna_pc_tx_mu_map_lfsid(struct sk_buff *skb, struct sna_lfsid *lf)
{
	sna_debug(5, "init\n");
	skb->nh.fh->odai	= lf->odai;
	skb->nh.fh->daf		= lf->sidl;
	skb->nh.fh->oaf		= lf->sidh;
	return 0;
}

/*
 * fsm_receive_router - 3.10.3.2.2 (appnarch.boo)
 */
int sna_pc_rx_mu(struct sk_buff *skb)
{
	struct sna_asm_cb *as;
	struct sna_lfsid *lf;
	int err;

	sna_debug(5, "init\n");
	err = sna_pc_rx_mu_chk_err(skb);
        if (err < 0) {
		sna_debug(5, "mu failed checks\n");
		goto toss;
	}
	err = -EINVAL;
	lf = sna_pc_rx_mu_map_lfsid(skb);
	if (!lf)
		goto toss;
	as = sna_asm_get_by_lfsid(lf);
	if (!as) {
		sna_debug(5, "unable to find asm this mu belongs to.\n");
		goto toss;
	}
	skb->sna_ctrl = sna_ctrl_info_create(GFP_ATOMIC);
	if (!skb->sna_ctrl) {
		sna_debug(5, "unable to allocate control information, tossing\n");
		goto toss;
	}
	
	/* based on the ru category route it. */	
	switch (skb->h.rh->ru) {
		/* session traffic. */
		case SNA_RH_RU_FMD:
		case SNA_RH_RU_DFC:
			sna_hs_rx(lf, skb);
			goto out;
		/* nonsession traffic. */
		case SNA_RH_RU_SC:
                case SNA_RH_RU_NC:
			sna_asm_rx(as, lf, skb);
			goto out;
		default:
			sna_debug(5, "unknown ru catagory %02X\n", skb->h.rh->ru);
			break;
	}
toss:	kfree_skb(skb);
out:	return err;
}

int sna_pc_tx_mu(struct sna_pc_cb *pc, struct sk_buff *skb, 
	struct sna_lfsid *lf)
{
	struct sna_port_cb *port;
	struct sna_ls_cb *ls;
	int err;

	sna_debug(5, "init\n");
	/* add the lfsid -> oaf/daf mapping if needed. */
	if (lf)
		sna_pc_tx_mu_map_lfsid(skb, lf);

	/* generate segments if required. */
	
	/* actually send the pkt. */
	port = sna_cs_port_get_by_index(pc->port_index);
	if (!port)
		return -ENOENT;
	ls = sna_cs_ls_get_by_index(port, pc->ls_index);
	if (!ls)
		return -ENOENT;
	err = sna_dlc_data_req(ls, skb);
	return err;
}

#ifdef CONFIG_PROC_FS
int sna_pc_get_info(char *buffer, char **start, off_t offset, int length)
{
	off_t pos = 0, begin = 0;
	struct sna_pc_cb *pc;
	struct list_head *le;
        int len = 0;

	len += sprintf(buffer, "%-18s%-6s%-10s%-4s%-17s\n",
		"name", "index", "intranode", "tg", "fqpc_id");
	list_for_each(le, &pc_list) {
		pc = list_entry(le, struct sna_pc_cb, list);
		len += sprintf(buffer + len, "%-18s%-6d%-10d%-4d\n",
			sna_pr_netid(&pc->cp_name), pc->index, pc->intranode,
			pc->tg_number);
		pos = begin + len;
                if (pos < offset) {
                        len   = 0;
			begin = pos;
                }
                if (pos > offset + length)
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
/* Internode. */
static int sna_pc_alert_handler(struct sna_mu *mu, __u32 sense)
{
	struct sna_msg_queue *msg = NULL;

	sna_debug(5, "received alert signal.\n");
	msg->mu = mu;
	msg->sense = sense;
//	sna_ms(msg);
	return 0;
}

/* Internode. */
static int sna_pc_flush_handler(void)
{
	sna_debug(5, "init\n");
	return 0;
}

static int sna_pc_hs_adder(struct sna_pc_hs_table *newhs)
{
	struct sna_pc_hs_table *next, *prev;

	sna_debug(5, "init\n");
	next = sna_hs_table;
	prev = next->prev;
	newhs->next = next;
	newhs->prev = prev;
	next->prev = newhs;
	prev->next = newhs;
	return 0;
}

static struct sna_pc_hs_table *sna_pc_hs_finder(__u8 lfsid)
{
	struct sna_pc_hs_table *i;

	for(i = sna_hs_table; i->next != NULL; i = i->next)
        {
                if(i->lfsid == lfsid)
                        return (i);
        }

	return (NULL);
}

static int sna_pc_hs_deleter(__u8 lfsid)
{
	struct sna_pc_hs_table *prev, *next, *result;

	sna_debug(5, "init\n");
	prev = sna_pc_hs_finder(lfsid);
	if(prev == NULL)
		return (-1);

        next = prev->next;
        result = NULL;
        if(next != prev)
	{
                result = next;
                next = next->next;
                next->prev = prev;
                prev->next = next;
                result->next = NULL;
                result->prev = NULL;
        }
	kfree(result);

	return (0);
}

static int sna_pc_local_bind_rq_send(struct sna_mu *mu)
{
	struct sna_msg_queue *msg = NULL;

	sna_debug(5, "init\n");
	sna_pc_hs_adder(NULL);

	msg->mu = mu;
	msg->cmd = SNA_BIND_RQ_RCV;
//	sna_asm(msg);

	return (0);
}

static int sna_pc_local_bind_rsp_send(struct sna_mu *mu)
{
	struct sna_msg_queue *msg = NULL;

	sna_debug(5, "init\n");
	sna_pc_hs_adder(NULL);

	msg->mu = mu;
        msg->cmd = SNA_BIND_RSP_RCV;
//        sna_asm(msg);

	return (0);
}

static int sna_pc_local_flush_ls(void)
{
	struct sna_msg_queue *msg = NULL;

	sna_debug(5, "init\n");
	msg->cmd = SNA_LS_FLUSHED;
//	sna_asm(msg);

	return (0);
}

static int sna_pc_local_unbind_rq_send(struct sna_mu *mu)
{
	struct sna_msg_queue *msg = NULL;

	sna_debug(5, "init\n");
	msg->mu = mu;
	msg->cmd = SNA_UNBIND_RQ_RCV;
//	sna_asm(msg);

	return (0);
}

static int sna_pc_local_unbind_rsp_send(struct sna_mu *mu)
{
	struct sna_msg_queue *msg = NULL;

	sna_debug(5, "init\n");
	msg->mu = mu;
        msg->cmd = SNA_UNBIND_RSP_RCV;
    //    sna_asm(msg);

	return (0);
}

static int sna_pc_send_free_lfsid(struct sna_mu *mu)
{
	struct sna_msg_queue *msg = NULL;

	sna_debug(5, "init\n");
	msg->mu = mu;
	msg->cmd = SNA_FREE_LFSID;
//	sna_asm(msg);

	return (0);
}

static __u16 sna_pc_get_mu_pcid(__u8 lfsid)
{
	struct sna_pc_hs_table *hs_table_entry;

	hs_table_entry = sna_pc_hs_finder(lfsid);

	return (hs_table_entry->pc_id);
}

static int sna_pc_remote_send_mu_processing(struct sna_mu *mu)
{
	if(mu->dcf > 600) // sna_pc_max_btu_size)
		sna_pc_segment_generator(mu);
	else
		sna_pc_enqueuer(mu);

	return (0);
}

static int sna_pc_segment_generator(struct sna_mu *mu)
{
	struct sna_mu *newmu = NULL, *cmu = NULL;
	int over_size, len = 0;
	int sna_pc_max_btu_size = 600;

//	sna_copy_mu(cmu, mu);

//	sna_cut_mu_tail(mu, over_size);
	sna_pc_enqueuer(mu);

	for(over_size = cmu->dcf - sna_pc_max_btu_size;
		over_size >= sna_pc_max_btu_size;
		len = over_size - sna_pc_max_btu_size, over_size -= len)
	{
//		newmu = sna_bm_get_buff(len);
		newmu->pc_id		= cmu->pc_id;
		newmu->hs_id		= cmu->hs_id;
		newmu->lfsid		= cmu->lfsid;
		newmu->tx_priority	= cmu->tx_priority;
		newmu->dcf		= len;
		newmu->biu->rh 		= cmu->biu->rh;
		newmu->biu->th.fid.fid2.fid = SNA_TH_FID2;
		newmu->biu->th.fid.fid2.mpf = SNA_TH_MPF_MID_MIU;
//		newmu->biu->th.fid.fid2.odai = cmu->biu->th.fid.fid0.odai;
		newmu->biu->th.fid.fid2.efi = cmu->biu->th.fid.fid0.efi;
		newmu->biu->th.fid.fid2.daf = cmu->biu->th.fid.fid0.daf;
		newmu->biu->th.fid.fid2.oaf = cmu->biu->th.fid.fid0.oaf;
		newmu->biu->th.fid.fid2.snf = cmu->biu->th.fid.fid0.snf;

		sna_pc_enqueuer(newmu);
	}

//	newmu = sna_bm_get_buff(len);
        newmu->pc_id    	= cmu->pc_id;
        newmu->hs_id    	= cmu->hs_id;
        newmu->lfsid    	= cmu->lfsid;
	newmu->tx_priority      = cmu->tx_priority;
        newmu->dcf      	= len;
        newmu->biu->rh  	= cmu->biu->rh;
        newmu->biu->th.fid.fid2.fid = SNA_TH_FID2;
	newmu->biu->th.fid.fid2.mpf = SNA_TH_MPF_EBIU;
//	newmu->biu->th.fid.fid2.odai = cmu->biu->th.fid.fid0.odai;
	newmu->biu->th.fid.fid2.efi = cmu->biu->th.fid.fid0.efi;
        newmu->biu->th.fid.fid2.daf = cmu->biu->th.fid.fid0.daf;
        newmu->biu->th.fid.fid2.oaf = cmu->biu->th.fid.fid0.oaf;
        newmu->biu->th.fid.fid2.snf = cmu->biu->th.fid.fid0.snf;

	sna_pc_enqueuer(newmu);

	return (0);
}
#endif
