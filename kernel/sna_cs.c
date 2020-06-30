/* sna_cs.c: Linux Systems Network Architecture implementation
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
#include <linux/if_arp.h>
#include <linux/proc_fs.h>
#include <linux/sna.h>

#ifdef CONFIG_SNA_LLC
#include <net/llc_if.h>
#include <net/llc_sap.h>
#include <net/llc_pdu.h>
#include <net/llc_conn.h>
#include <linux/llc.h>
#endif  /* CONFIG_SNA_LLC */

u_int32_t sysctl_xid_idle_limit		= SNA_XID_IDLE_LIMIT;
u_int32_t sysctl_xid_retry_limit	= SNA_XID_RETRY_LIMIT;
u_int32_t sysctl_test_retry_limit	= SNA_TEST_RETRY_LIMIT;

static LIST_HEAD(cs_list);
static LIST_HEAD(dlc_list);		/* devices SNA can use. */
static LIST_HEAD(port_list);		/* ports link stations can use. */

static u_int32_t	sna_cs_system_index	= 0;
static u_int32_t	sna_dlc_system_index	= 0;
static u_int32_t	sna_port_system_index 	= 0;

static void xid_output_connect_finish(void *data);
static int xid_doit(struct sna_ls_cb *ls);
static int sna_cs_stop_port_gen(struct sna_port_cb *port);
static int sna_cs_delete_port_gen(struct sna_port_cb *port);

struct sna_cs_cb *sna_cs_get_by_name(char *name)
{
        struct sna_cs_cb *cs;
	struct list_head *le;

	sna_debug(5, "init: %s\n", name);
	list_for_each(le, &cs_list) {
		cs = list_entry(le, struct sna_cs_cb, list);
		sna_debug(5, "-%s- -%s-\n", cs->netid.name, name);
		if (!strncmp(cs->netid.name, name, SNA_NODE_NAME_LEN))
                       	return cs;
	}
        return NULL;
}

static struct sna_cs_cb *sna_cs_get_by_index(u_int32_t index)
{
        struct sna_cs_cb *cs;
        struct list_head *le;

        sna_debug(5, "init\n");
        list_for_each(le, &cs_list) {
                cs = list_entry(le, struct sna_cs_cb, list);
                if (cs->index == index)
                        return cs;
        }
        return NULL;
}

static u_int32_t sna_cs_new_index(void)
{
        for (;;) {
                if (++sna_cs_system_index <= 0)
                        sna_cs_system_index = 1;
                if (sna_cs_get_by_index(sna_cs_system_index) == NULL)
                        return sna_cs_system_index;
        }
        return 0;
}

static struct sna_dlc_cb *sna_cs_dlc_get_by_name(char *name)
{
        struct sna_dlc_cb *dlc;
	struct list_head *le;

	sna_debug(5, "init: %s\n", name);
	list_for_each(le, &dlc_list) {
		dlc = list_entry(le, struct sna_dlc_cb, list);
                if (!strncmp(dlc->dev->name, name, IFNAMSIZ))
			return dlc;
	}
        return NULL;
}

struct sna_dlc_cb *sna_cs_dlc_get_by_index(u_int32_t index)
{
	struct sna_dlc_cb *dlc;
        struct list_head *le;

        sna_debug(5, "init\n");
        list_for_each(le, &dlc_list) {
                dlc = list_entry(le, struct sna_dlc_cb, list);
		if (dlc->index == index)
                        return dlc;
        }
        return NULL;
}

static u_int32_t sna_cs_dlc_new_index(void)
{
        for (;;) {
                if (++sna_dlc_system_index <= 0)
                        sna_dlc_system_index = 1;
                if (sna_cs_dlc_get_by_index(sna_dlc_system_index) == NULL)
                        return sna_dlc_system_index;
        }
	return 0;
}

static struct sna_port_cb *sna_cs_port_get_by_name(char *name)
{
	struct sna_port_cb *port;
        struct list_head *le;

        sna_debug(5, "init\n");
        list_for_each(le, &port_list) {
                port = list_entry(le, struct sna_port_cb, list);
		sna_debug(5, "-%s- -%s-\n", port->use_name, name);
                if (!strcmp(port->use_name, name))
                        return port;
        }
        return NULL;
}

struct sna_port_cb *sna_cs_port_get_by_addr(char *saddr)
{
        struct sna_port_cb *port;
	struct list_head *le;
	
	sna_debug(5, "init\n");
	list_for_each(le, &port_list) {
		port = list_entry(le, struct sna_port_cb, list);
		sna_debug(5, "p->s(%02X) s(%02X)\n", port->saddr[0], saddr[0]);
		if (!strncmp(port->saddr, saddr, 1))
			return port;
	}
        return NULL; 
}

struct sna_port_cb *sna_cs_port_get_by_index(u_int32_t index)
{
        struct sna_port_cb *port;
        struct list_head *le;

        sna_debug(5, "init\n");
        list_for_each(le, &port_list) {
                port = list_entry(le, struct sna_port_cb, list);
                if (port->index == index)
                        return port;
        }
        return NULL;
}

static u_int32_t sna_cs_port_new_index(void)
{
        for (;;) {
                if (++sna_port_system_index <= 0)
                        sna_port_system_index = 1;
                if (sna_cs_port_get_by_index(sna_port_system_index) == NULL)
                        return sna_port_system_index;
        }
        return 0;
}

struct sna_ls_cb *sna_cs_ls_get_by_name(char *name)
{
	struct sna_port_cb *port;
        struct sna_ls_cb *ls;
        struct list_head *le, *se;

        sna_debug(5, "init\n");
        list_for_each(le, &port_list) {
                port = list_entry(le, struct sna_port_cb, list);
                list_for_each(se, &port->ls_list) {
                        ls = list_entry(se, struct sna_ls_cb, list);
			sna_debug(5, "-%s- -%s-\n", ls->use_name, name);
                        if (!strcmp(ls->use_name, name))
                                return ls;
                }
        }
        return NULL;
}

/**
 * sna_cs_ls_get_by_addr - search all ports for any clash of [mac + lsap].
 */
struct sna_ls_cb *sna_cs_ls_get_by_addr(char *mac, char *lsap)
{
	struct sna_port_cb *port;
        struct sna_ls_cb *ls;
	struct list_head *le, *se;
	
        sna_debug(5, "init\n");
	list_for_each(le, &port_list) {
		port = list_entry(le, struct sna_port_cb, list);
		list_for_each(se, &port->ls_list) {
			ls = list_entry(se, struct sna_ls_cb, list);
			sna_debug(5, "plu=%s\n", sna_pr_ether(ls->plu_mac_addr));
			sna_debug(5, "mac=%s\n", sna_pr_ether(mac));
			sna_debug(5, "plu->port=%d lsap=%d\n", ls->plu_port, lsap[0]);
			if (!memcmp(ls->plu_mac_addr, mac, IFHWADDRLEN)
				&& ls->plu_port == lsap[0])
				return ls;
		}
	}
        return NULL;
}

struct sna_ls_cb *sna_cs_ls_get_by_index(struct sna_port_cb *port, u_int32_t index)
{
        struct sna_ls_cb *ls;
        struct list_head *le;

        sna_debug(5, "init\n");
        list_for_each(le, &port->ls_list) {
                ls = list_entry(le, struct sna_ls_cb, list);
                if (ls->index == index)
                        return ls;
        }
        return NULL;
}

static u_int32_t sna_cs_ls_new_index(struct sna_port_cb *port)
{
        for (;;) {
                if (++port->ls_system_index <= 0)
                        port->ls_system_index = 1;
                if (sna_cs_ls_get_by_index(port, port->ls_system_index) == NULL)
                        return port->ls_system_index;
        }
        return 0;
}

int sna_cs_start(struct sna_nof_node *node)
{
	struct sna_cs_cb *cs;
	
	sna_debug(5, "init\n");
	cs = sna_cs_get_by_name(node->netid.name);
        if (!cs)
                return -ENOENT;
	if (cs->flags & SNA_RUNNING)
		return -EUSERS;
	cs->flags &= ~SNA_STOPPED;
        cs->flags |= SNA_RUNNING;
        return 0;
}

int sna_cs_stop(struct sna_nof_node *node)
{
	struct list_head *le, *se;
	struct sna_port_cb *port;
	struct sna_cs_cb *cs;
	
        sna_debug(5, "init\n");
	cs = sna_cs_get_by_name(node->netid.name);
        if (!cs)
                return -ENOENT;
	if (cs->flags & SNA_STOPPED)
		return 0;
	cs->flags &= ~SNA_RUNNING;
        cs->flags |= SNA_STOPPED;
	list_for_each_safe(le, se, &port_list) {
		port = list_entry(le, struct sna_port_cb, list);
		if (!memcmp(&node->netid, &port->netid, sizeof(sna_netid)))
			sna_cs_stop_port_gen(port);
	}
        return 0;
}

int sna_cs_create(struct sna_nof_node *start)
{
	struct sna_cs_cb *cs;

	sna_debug(5, "init: %s\n", start->netid.name);
	cs = sna_cs_get_by_name(start->netid.name);
	if (cs)
		return -EEXIST;
	new(cs, GFP_KERNEL);
	if (!cs)
		return -ENOMEM;
	cs->index 	= sna_cs_new_index();
	cs->flags     	= SNA_UP | SNA_STOPPED;
	cs->nodeid	= start->nodeid;
	memcpy(&cs->netid, &start->netid, sizeof(sna_netid));
	list_add_tail(&cs->list, &cs_list);
	sna_mod_inc_use_count();

#ifdef NOT
	struct sna_pc_cb *pc;
        struct sna_rm_act_session_rq *as;

	/* Create the intranode Path Control - One per Node */
	new(pc, GFP_KERNEL);
	if (!pc)
		return -ENOMEM;
	pc->type 	= SNA_PC_INTRANODE;
	pc->dlc		= NULL;
	pc->port	= NULL;
	pc->ls		= NULL;
	memcpy(&pc->fqcp, &cs->netid, sizeof(sna_netid));
	sna_pc_init(pc);
	memcpy(&cs->intranode_pc_id, &pc->pc_id, 8);
	kfree(pc);
	cs->node_type = start->type;

	/* Activate the loopback link - hidden */
	new(as, GFP_KERNEL);
	if (!as)
		return -ENOMEM;
	memcpy(&as->plu_netid, &cs->netid, sizeof(sna_netid));
	strcpy(as->mode_name, "CONNECT");
	sna_rm_activate_session(as);
#endif
	return 0;
}

int sna_cs_destroy(struct sna_nof_node *node)
{
	struct list_head *le, *se;
	struct sna_port_cb *port;
	struct sna_cs_cb *cs;

        sna_debug(5, "init\n");
        cs = sna_cs_get_by_name(node->netid.name);
        if (!cs)
                return -ENOENT;

	/* ensure cs is inactive. */
	sna_cs_stop(node);

	/* delete port and link stations. */
	list_for_each_safe(le, se, &port_list) {
                port = list_entry(le, struct sna_port_cb, list);
                if (!memcmp(&node->netid, &port->netid, sizeof(sna_netid)))
			sna_cs_delete_port_gen(port);
        }

	/* delete this cs. */
	list_del(&cs->list);
	kfree(cs);

	sna_mod_dec_use_count();
	return 0;
}

/**
 * sna_cs_dlc_port_type - given a dlc return a port type.
 */
static u_int16_t sna_cs_dlc_port_type(struct sna_dlc_cb *dlc)
{
	switch (dlc->type) {
                case ARPHRD_LOOPBACK:
			return SNA_PORT_TYPE_LOOPBACK;
                case ARPHRD_ETHER:
                case ARPHRD_IEEE802:
                case ARPHRD_FDDI:
			return SNA_PORT_TYPE_LLC;
	}
	return SNA_PORT_TYPE_INVALID;
}

static u_int8_t sna_cs_dlc_xid_type(struct sna_dlc_cb *dlc)
{
	switch (dlc->type) {
		case ARPHRD_ETHER:
                case ARPHRD_IEEE802:
                case ARPHRD_FDDI:
			return 0x01;
	}
	return 0;
}

static u_int8_t sna_cs_dlc_xid_len(struct sna_dlc_cb *dlc)
{
	switch (dlc->type) {
                case ARPHRD_ETHER:
                case ARPHRD_IEEE802:
                case ARPHRD_FDDI:
                        return 11;
        }
        return 0;
}

static u_int8_t sna_cs_dlc_xid_abm(struct sna_dlc_cb *dlc)
{
        switch (dlc->type) {
                case ARPHRD_ETHER:
                case ARPHRD_IEEE802:
                case ARPHRD_FDDI:
                        return 1;
        }
        return 0;
}

static u_int8_t sna_cs_dlc_xid_init_mode(struct sna_dlc_cb *dlc)
{
        switch (dlc->type) {
                case ARPHRD_ETHER:
                case ARPHRD_IEEE802:
                case ARPHRD_FDDI:
                        return 0;
        }
        return 0;
}

static u_int16_t sna_cs_dlc_xid_max_btu_len(struct sna_dlc_cb *dlc)
{
        switch (dlc->type) {
                case ARPHRD_ETHER:
                case ARPHRD_IEEE802:
                case ARPHRD_FDDI:
			return htons(dlc->dev->mtu - dlc->dev->hard_header_len);
        }
        return 0;
}

static u_int8_t sna_cs_dlc_xid_max_iframes(struct sna_dlc_cb *dlc)
{
        switch (dlc->type) {
                case ARPHRD_ETHER:
                case ARPHRD_IEEE802:
                case ARPHRD_FDDI:
                        return 2;
        }
        return 0;
}

int sna_cs_dlc_delete(struct net_device *dev)
{
        struct list_head *le, *se;
        struct sna_dlc_cb *dlc;

        sna_debug(5, "init: %s\n", dev->name);
        list_for_each_safe(le, se, &dlc_list) {
                dlc = list_entry(le, struct sna_dlc_cb, list);
                if (!strncmp(dlc->dev->name, dev->name, IFNAMSIZ)) {
                        list_del(&dlc->list);
                        if (dlc->dev)
                                dev_put(dlc->dev);
                        kfree(dlc);
                        return 0;
                }
        }
        return -ENOENT;
}

/**
 * sna_cs_dlc_create - define a physical device for use by SNA.
 */
int sna_cs_dlc_create(struct net_device *dev)
{
	struct sna_dlc_cb *dlc;

	if (!dev)
		return -EINVAL;
	sna_debug(5, "init: %s\n", dev->name);
	switch (dev->type) {
		case ARPHRD_LOOPBACK:
		case ARPHRD_ETHER:
		case ARPHRD_IEEE802:
		case ARPHRD_FDDI:
			new(dlc, GFP_KERNEL);
			if (!dlc)
				return -ENOMEM;
			dev_hold(dev);
			dlc->dev        = dev;
			dlc->type	= dev->type;
                        dlc->index    	= sna_cs_dlc_new_index();
                        dlc->flags      = SNA_UP | SNA_RUNNING;
			list_add_tail(&dlc->list, &dlc_list);
                        break;
#ifdef CONFIG_SNA_ATM
		case ARPHRD_ATM:
			break;
#endif
#ifdef CONFIG_SNA_CHANNEL
		case ARPHRD_CHANNEL:
			break;
#endif
#ifdef CONFIG_SNA_HDLC
		case ARPHRD_HDLC:
			break;
#endif
#ifdef CONFIG_SNA_SDLC
		case ARPHRD_SDLC:
			break;
#endif
		default:
			return -EINVAL;
	}
	return 0;
}

/**
 * sna_cs_define_ls - setup a link station so it is ready to be activated.
 */
int sna_cs_define_ls(struct sna_nof_ls *cfg)
{
	struct sna_port_cb *port;
	struct sna_cs_cb *cs;
	struct sna_dlc_cb *dlc;
	struct sna_ls_cb *ls;
        int err = -ENOENT;

	sna_debug(5, "init\n");
	cs = sna_cs_get_by_name(cfg->netid.name);
	if (!cs)
		goto out;
	dlc = sna_cs_dlc_get_by_name(cfg->name);
	if (!dlc)
		goto out;
	port = sna_cs_port_get_by_addr(cfg->saddr);
	if (!port)
		goto out;
	err = -EEXIST;
	ls = sna_cs_ls_get_by_addr(cfg->dname, cfg->daddr);
	if (ls)
		goto out;
	ls = sna_cs_ls_get_by_name(cfg->use_name);
	if (ls)
		goto out;
	err = -ENOMEM;
	new(ls, GFP_KERNEL);
	if (!ls)
		goto out;
	err = 0;
	init_waitqueue_head(&ls->sleep);
	memset(&ls->connect, 0, sizeof(struct tq_struct));
        ls->connect.routine 		= xid_output_connect_finish;
        ls->connect.data    		= ls;
	memcpy(&ls->use_name, &cfg->use_name, SNA_USE_NAME_LEN);
	ls->index       		= sna_cs_ls_new_index(port);
	ls->cs_index			= cs->index;
	ls->dlc_index   		= dlc->index;
        ls->port_index  		= port->index;
        ls->flags       		= SNA_UP | SNA_STOPPED;
	ls->state			= SNA_LS_STATE_DEFINED;
	ls->role                        = cfg->role;
	ls->direction			= cfg->direction;
	ls->effective_role		= cfg->role;
	if (ls->direction == SNA_LS_DIR_OUT)
	        ls->co_status           = CO_RESET;
        else
                ls->co_status           = CO_TEST_OK;
        ls->xid_state  			= XID_STATE_RESET;
	ls->xid_input			= XID_IN_RESET;
	ls->byteswap			= cfg->byteswap;
	ls->retry_on_fail		= cfg->retry_on_fail;
	ls->retry_times			= cfg->retry_times;
	ls->autoact			= cfg->autoact;
	ls->autodeact			= cfg->autodeact;
	ls->tg_number			= cfg->tg_number;
	ls->effective_tg		= cfg->tg_number;
	ls->cost_per_byte		= cfg->cost_per_byte;
	ls->cost_per_connect_time	= cfg->cost_per_connect_time;
	ls->effective_capacity		= cfg->effective_capacity;
	ls->propagation_delay		= cfg->propagation_delay;
	ls->user1			= cfg->user1;
	ls->user2			= cfg->user2;
	ls->user3			= cfg->user3;
	ls->plu_port 			= cfg->daddr[0];
	init_timer(&ls->retry);
	ls->retry.function		= sna_cs_connect_out;
	ls->retry.data			= (unsigned long)ls;
	ls->retries			= 0;
	ls->plu_node_id 		= cfg->plu_node_id;
	memcpy(ls->plu_mac_addr, cfg->dname, IFHWADDRLEN);
	memcpy(&ls->netid, &cfg->netid, sizeof(sna_netid));
	memcpy(&ls->plu_name, &cfg->plu_name, sizeof(sna_netid));

	/* default settings for now. */
	if (cs->node_type == SNA_APPN_NET_NODE)
		ls->xid_network_node = 1;
	else 
		ls->xid_network_node = 0;
        ls->xid_type                    = SNA_XID_NODE_T2;
        ls->xid_format                  = SNA_XID_TYPE_3;
	ls->xid_init_self		= 1;
        ls->xid_standalone_bind		= 0;
        ls->xid_whole_bind		= 1;
        ls->xid_whole_bind_piu_req	= 1;
        ls->xid_actpu_suppression	= 1;
        ls->xid_cp_services		= 0;
        ls->xid_sec_nonactive_xchg	= 0;
        ls->xid_cp_name_chg		= 0;
        ls->xid_tx_adaptive_bind_pacing	= 0;
        ls->xid_rx_adaptive_bind_pacing = 0;
        ls->xid_quiesce_tg		= 0;
        ls->xid_pu_cap_sup		= 1;
        ls->xid_appn_pbn		= 0;
        ls->xid_parallel_tg_sup		= 0;
        ls->xid_ls_txrx_cap		= 0;
	ls->xid_gen_odai_usage_opt_set	= 0;
#ifdef CONFIG_SNA_LLC
	ls->llc_sk			= NULL;
#endif
	list_add_tail(&ls->list, &port->ls_list);
	port->ls_qlen++;
	sna_mod_inc_use_count();
out:	return err;
}

/**
 * sna_cs_define_port - setup datalink details. ie setup llc sap.
 */
int sna_cs_define_port(struct sna_nof_port *dport)
{
	struct sna_port_cb *port;
	struct sna_cs_cb *cs;
	struct sna_dlc_cb *dlc;

	sna_debug(5, "init\n");
	cs = sna_cs_get_by_name(dport->netid.name);
	if (!cs)
		return -ENOENT;
	dlc = sna_cs_dlc_get_by_name(dport->name);
	if (!dlc)
		return -ENOENT;
	port = sna_cs_port_get_by_addr(dport->saddr);
	if (port)
		return -EEXIST;
	port = sna_cs_port_get_by_name(dport->use_name);
	if (port)
		return -EEXIST;
	new(port, GFP_KERNEL);
	if (!port)
		return -ENOMEM;
	memcpy(&port->use_name, &dport->use_name, SNA_USE_NAME_LEN);
	memcpy(&port->netid, &dport->netid, sizeof(sna_netid));
	memcpy(&port->saddr, &dport->saddr, SNA_PORT_ADDR_LEN);
	port->type	= sna_cs_dlc_port_type(dlc);
	port->index	= sna_cs_port_new_index();
	port->cs_index	= cs->index;
	port->dlc_index	= dlc->index;
	port->flags	= SNA_UP | SNA_STOPPED;
	port->ls_qlen	= 0;
	port->btu	= dport->btu;
	port->mia	= dport->mia;
	port->moa	= dport->moa;
#ifdef CONFIG_SNA_LLC
	port->llc_dl	= NULL;
#endif
	INIT_LIST_HEAD(&port->ls_list);
	list_add_tail(&port->list, &port_list);
	sna_mod_inc_use_count();
	sna_debug(5, "fini\n");
	return 0;
}

int sna_cs_start_ls(struct sna_nof_ls *cfg)
{
	struct sna_port_cb *port;
	struct sna_cs_cb *cs;
        struct sna_ls_cb *ls;
	int err = -ENOENT;

        sna_debug(5, "init\n");
	ls = sna_cs_ls_get_by_name(cfg->use_name);
	if (!ls)
		goto out;
	port = sna_cs_port_get_by_index(ls->port_index);
        if (!port)
		goto out;
	cs = sna_cs_get_by_index(port->cs_index);
        if (!cs)
		goto out;
        if (cs->flags & SNA_STOPPED) {
		sna_debug(5, "node is not ACTIVE.\n");
                err = -EHOSTDOWN;
		goto out;
	}
	if (port->flags & SNA_STOPPED) {
		sna_debug(5, "port/dlc is not ACTIVE.\n");
		err = -ENETDOWN;
		goto out;
	}
	if (ls->state != SNA_LS_STATE_DEFINED || ls->flags & SNA_RUNNING) {
		sna_debug(5, "link station already in use.\n");
		err = -EUSERS;
		goto out;
	}
	
	err			= 0;
	ls->state		= SNA_LS_STATE_ACTIVATED;
	ls->xid_input		= XID_IN_BEGIN;
	ls->xid_direction	= XID_DIR_OUTBOUND;
	ls->flags      		&= ~SNA_STOPPED;
        ls->flags      		|= SNA_RUNNING;
	sna_cs_connect_out((unsigned long)ls);
out:	return err;
}

/* Start the "port". Depending on the underlying device and
 * network type and what DLC support we have enabled we will
 * start the DLC specific actions to start the DLC to send up
 * packets. We mux all similar DLC types into the same rx
 * handlers and demux it there. Actual specific DLC handlers are
 * located in sna_dlc.c.
 */
int sna_cs_start_port(struct sna_nof_port *sport)
{
	struct sna_port_cb *port;
	struct sna_cs_cb *cs;
	
	sna_debug(5, "init\n");
	port = sna_cs_port_get_by_name(sport->use_name);
	if (!port)
		return -ENOENT;
	cs = sna_cs_get_by_index(port->cs_index);
	if (!cs)
		return -ENOENT;
	if (cs->flags & SNA_STOPPED)
		return -EHOSTDOWN;
	if (port->flags & SNA_RUNNING)
		return -EUSERS;
	switch (port->type) {
#ifdef CONFIG_SNA_LLC
		case SNA_PORT_TYPE_LLC:
			port->llc_dl = llc_sap_open(sna_dlc_llc_indicate,
				sna_dlc_llc_confirm, port->saddr[0]);
			if (!port->llc_dl) {
				sna_debug(5, "FIXME: LLC SAP in use.. should retry\n");
				return -EAGAIN;
			}
			break;
#endif
		default:
			return -EINVAL;
	}
	port->flags	&= ~SNA_STOPPED;
	port->flags	|= SNA_RUNNING;
	return 0;
}

static int sna_cs_stop_ls_gen(struct sna_ls_cb *ls)
{
	sna_debug(5, "init\n");
	if (ls->flags & SNA_STOPPED)
                return 0;
	ls->flags &= ~SNA_RUNNING;
        ls->flags |= SNA_STOPPED;
        if (ls->xid_state == XID_STATE_ACTIVE) {
#ifdef CONFIG_SNA_LLC
                sna_dlc_disc_req(ls);

                /* in case we don't get a disc rsp, force cleanup. */
                ls->xid_input 	  = XID_IN_RESET;
                ls->retry.expires = jiffies + sysctl_xid_idle_limit;
                add_timer(&ls->retry);
#endif
        } else {
                ls->xid_input = XID_IN_RESET;
                xid_doit(ls);
        }
	return 0;
}

int sna_cs_stop_ls(struct sna_nof_ls *sls)
{
        struct sna_ls_cb *ls;

        sna_debug(5, "init\n");
	ls = sna_cs_ls_get_by_name(sls->use_name);
	if (!ls)
		return -ENOENT;
	return sna_cs_stop_ls_gen(ls);
}

static int sna_cs_stop_ls_all(struct sna_port_cb *port)
{
	struct list_head *le;
	struct sna_ls_cb *ls;

	sna_debug(5, "init\n");
	list_for_each(le, &port->ls_list) {
		ls = list_entry(le, struct sna_ls_cb, list);
		sna_cs_stop_ls_gen(ls);
	}
	return 0;
}

static int sna_cs_delete_ls_gen(struct sna_ls_cb *ls)
{
	sna_debug(5, "init\n");

	/* ensure link is offline. */
        sna_cs_stop_ls_gen(ls);

        /* remove the link from the system. */
        list_del(&ls->list);
        kfree(ls);

        sna_mod_dec_use_count();
	return 0;
}

int sna_cs_delete_ls(struct sna_nof_ls *dls)
{
	struct sna_port_cb *port;
	struct sna_ls_cb *ls;

        sna_debug(5, "init\n");
        ls = sna_cs_ls_get_by_name(dls->use_name);
        if (!ls)
                return -ENOENT;
	port = sna_cs_port_get_by_index(ls->port_index);
	if (!port)
		return -ENOENT;
	port->ls_qlen--;
	return sna_cs_delete_ls_gen(ls);
}

static int sna_cs_delete_ls_all(struct sna_port_cb *port)
{
	struct list_head *le, *se;
        struct sna_ls_cb *ls;

        sna_debug(5, "init\n");
        list_for_each_safe(le, se, &port->ls_list) {
                ls = list_entry(le, struct sna_ls_cb, list);
		sna_cs_delete_ls_gen(ls);
		port->ls_qlen--;
        }
        return 0;
}

static int sna_cs_stop_port_gen(struct sna_port_cb *port)
{
	sna_debug(5, "init\n");
	if (port->flags & SNA_STOPPED)
                return 0;
	port->flags &= ~SNA_RUNNING;
        port->flags |= SNA_STOPPED;
	
	/* stop all link stations using this port. */
        sna_cs_stop_ls_all(port);

        /* stop the actual port. */
        switch (port->type) {
#ifdef CONFIG_SNA_LLC
                case SNA_PORT_TYPE_LLC:
                        if (port->llc_dl)
                                llc_sap_close(port->llc_dl);
                        break;
#endif
                default:
                        return -EINVAL;
        }
	return 0;
}

int sna_cs_stop_port(struct sna_nof_port *sport)
{
	struct sna_port_cb *port;

        sna_debug(5, "init\n");
	port = sna_cs_port_get_by_name(sport->use_name);
	if (!port)
		return -ENOENT;
	return sna_cs_stop_port_gen(port);
}

static int sna_cs_delete_port_gen(struct sna_port_cb *port)
{
	sna_debug(5, "init\n");

	/* ensure port is inactive. */
        sna_cs_stop_port_gen(port);

        /* delete all link stations using this port. */
        sna_cs_delete_ls_all(port);

        /* remove the port from the system. */
        list_del(&port->list);
        kfree(port);

        sna_mod_dec_use_count();
	return 0;
}

int sna_cs_delete_port(struct sna_nof_port *sport)
{
	struct sna_port_cb *port;

        sna_debug(5, "init\n");
        port = sna_cs_port_get_by_name(sport->use_name);
        if (!port)
                return -ENOENT;
	return sna_cs_delete_port_gen(port);
}

int sna_dlc_ginfo(struct sna_dlc_cb *dlc, char *buf, int len)
{
	struct dlcreq dr;
	int done = 0;

	sna_debug(5, "init\n");
	if (!buf) {
                done += sizeof(dr);
                return done;
        }
        if (len < (int)sizeof(dr))
                return done;
        memset(&dr, 0, sizeof(struct dlcreq));

        /* Move the data here */
	strcpy(dr.dev_name, dlc->dev->name);
	memcpy(dr.dev_addr, dlc->dev->dev_addr, MAX_ADDR_LEN);
	dr.index 		= dlc->index;
	dr.flags 		= dlc->flags;
	dr.type			= dlc->type;
	dr.mtu			= dlc->dev->mtu;
	
        if (copy_to_user(buf, &dr, sizeof(struct dlcreq)))
                return -EFAULT;
        buf  += sizeof(struct dlcreq);
        len  -= sizeof(struct dlcreq);
        done += sizeof(struct dlcreq);
	return done;
}

int sna_cs_query_dlc(char *arg)
{
        struct sna_dlc_cb *dlc;
	struct list_head *le;
	int len, total, done;
        struct dlconf dc;
        char *pos;

        sna_debug(5, "sna_cs_query_dlc\n");
        if (copy_from_user(&dc, arg, sizeof(dc)))
                return -EFAULT;

        pos = dc.dlc_buf;
        len = dc.dlc_len;

        /*
         * Get the data and put it into the structure
         */
        total = 0;
	list_for_each(le, &dlc_list) {
		dlc = list_entry(le, struct sna_dlc_cb, list);
                if (pos == NULL)
                        done = sna_dlc_ginfo(dlc, NULL, 0);
                else
                        done = sna_dlc_ginfo(dlc, pos + total, len - total);
                if (done < 0)
                        return -EFAULT;
                total += done;
        }

        dc.dlc_len = total;
        if (copy_to_user(arg, &dc, sizeof(dc)))
                return -EFAULT;
        return 0;
}

int sna_port_ginfo(struct sna_port_cb *port, char *buf, int len)
{
	struct sna_dlc_cb *dlc;
        struct portreq pr;
        int done = 0;

        sna_debug(10, "sna_port_ginfo\n");
	dlc = sna_cs_dlc_get_by_index(port->dlc_index);
	if (!dlc)
		return done;
        if (!buf) {
                done += sizeof(pr);
                return done;
        }
        if (len < (int)sizeof(pr))
                return done;
        memset(&pr, 0, sizeof(struct portreq));

        /* Move the data here */
	memcpy(&pr.netid, &port->netid, sizeof(sna_netid));
	memcpy(&pr.use_name, &port->use_name, SNA_RESOURCE_NAME_LEN);
	memcpy(&pr.saddr, port->saddr, 12);
	strcpy(pr.dev_name, dlc->dev->name);
	pr.ls_qlen	= port->ls_qlen;
	pr.index	= port->index;
	pr.type		= port->type;
	pr.flags	= port->flags;
	pr.btu		= port->btu;
	pr.mia		= port->mia;
	pr.moa		= port->moa;

        if (copy_to_user(buf, &pr, sizeof(struct portreq)))
                return -EFAULT;
        buf  += sizeof(struct portreq);
        len  -= sizeof(struct portreq);
        done += sizeof(struct portreq);
        return done;
}

int sna_cs_query_port(char *arg)
{
	struct list_head *le;
	struct sna_port_cb *port;
	int len, total, done;
	struct portconf pc;
        char *pos;

        sna_debug(10, "init\n");
        if (copy_from_user(&pc, arg, sizeof(pc)))
                return -EFAULT;
        pos = pc.portc_buf;
        len = pc.port_len;

	/*
         * Get the data and put it into the structure
         */
        total = 0;
	list_for_each(le, &port_list) {
		port = list_entry(le, struct sna_port_cb, list);
                if (pos == NULL)
                        done = sna_port_ginfo(port, NULL, 0);
                else
                        done = sna_port_ginfo(port,pos+total,len-total);
                if (done < 0)
                        return -EFAULT;
                total += done;
	}
        pc.port_len = total;
        if (copy_to_user(arg, &pc, sizeof(pc)))
                return -EFAULT;
	return 0;
}

int sna_ls_ginfo(struct sna_ls_cb *ls, char *buf, int len)
{
	struct sna_port_cb *port;
	struct sna_dlc_cb *dlc;
        struct lsreq lr;
        int done = 0;

        sna_debug(10, "sna_ls_ginfo\n");
	port = sna_cs_port_get_by_index(ls->port_index);
	if (!port)
		return done;
	dlc = sna_cs_dlc_get_by_index(ls->dlc_index);
	if (!dlc)
		return done;
        if (!buf) {
                done += sizeof(lr);
                return done;
        }
        if (len < (int)sizeof(lr))
                return done;
        memset(&lr, 0, sizeof(struct lsreq));

        /* Move the data here */
	memcpy(&lr.netid, &ls->netid, sizeof(sna_netid));
	strcpy(lr.use_name, ls->use_name);
	strcpy(lr.port_name, port->use_name);
        strcpy(lr.dev_name, dlc->dev->name);
	
	lr.index			= ls->index;
	lr.flags			= ls->flags;

	lr.retries			= ls->retries;
	lr.xid_count			= ls->xid_count;
	
	lr.state			= ls->state;
	lr.co_status			= ls->co_status;
	lr.xid_state			= ls->xid_state;
	
	lr.effective_role		= ls->effective_role;
	lr.effective_tg			= ls->effective_tg;

	lr.tx_max_btu			= ls->tx_max_btu;
	lr.rx_max_btu			= ls->rx_max_btu;
	lr.tx_window			= ls->tx_window;
	lr.rx_window			= ls->rx_window;
	
	memcpy(&lr.plu_name, &ls->plu_name, sizeof(sna_netid));
	memcpy(&lr.plu_node_id, &ls->plu_node_id, sizeof(sna_nodeid));
	memcpy(lr.plu_mac_addr, ls->plu_mac_addr, IFHWADDRLEN);
	lr.plu_port			= ls->plu_port;

	lr.role				= ls->role;
	lr.direction			= ls->direction;
	lr.xid_init_method		= ls->xid_init_method;
	lr.byteswap			= ls->byteswap;
	lr.retry_on_fail		= ls->retry_on_fail;
	lr.retry_times			= ls->retry_times;
	lr.autoact			= ls->autoact;
	lr.autodeact			= ls->autodeact;
	lr.tg_number			= ls->tg_number;
	lr.cost_per_byte		= ls->cost_per_byte;
	lr.cost_per_connect_time	= ls->cost_per_connect_time;
	lr.effective_capacity		= ls->effective_capacity;
	lr.propagation_delay		= ls->propagation_delay;
	lr.security			= ls->security;
	lr.user1			= ls->user1;
	lr.user2			= ls->user2;
	lr.user3			= ls->user3;

        if (copy_to_user(buf, &lr, sizeof(struct lsreq)))
                return -EFAULT;
        buf  += sizeof(struct lsreq);
        len  -= sizeof(struct lsreq);
        done += sizeof(struct lsreq);
        return done;
}

int sna_cs_query_ls(char *arg)
{
	struct sna_port_cb *port;
        int len, total, done;
	struct sna_ls_cb *ls;
	struct list_head *se, *ae;
	struct lsconf lc;
        char *pos;

       	sna_debug(10, "init\n");
        if (copy_from_user(&lc, arg, sizeof(lc)))
                return -EFAULT;
        pos = lc.lsc_buf;
        len = lc.ls_len;

        /*
         * Get the data and put it into the structure
         */
        total = 0;
	list_for_each(se, &port_list) {
		port = list_entry(se, struct sna_port_cb, list);
		list_for_each(ae, &port->ls_list) {
			ls = list_entry(ae, struct sna_ls_cb, list);
	                if (pos == NULL)
	                        done = sna_ls_ginfo(ls, NULL, 0);
	                else
		                done = sna_ls_ginfo(ls,pos+total,len-total);
	                if (done < 0)
	                        return -EFAULT;
	                total += done;
	        }
	}
        lc.ls_len = total;
        if (copy_to_user(arg, &lc, sizeof(lc)))
                return -EFAULT;
        return 0;
}

int sna_cs_pc_activate(struct sna_ls_cb *ls)
{
	struct sna_pc_cb *pc;
	int err = -ENOMEM;

	sna_debug(5, "init\n");
	new(pc, GFP_ATOMIC);
        if (!pc) {
		sna_debug(5, "out of memory, pc_init failed\n");
		goto out;
        }
	memcpy(&pc->cp_name, &ls->plu_name, sizeof(sna_netid));
	pc->odai                	= ls->odai;
	pc->tx_max_btu			= ls->tx_max_btu;
	pc->rx_max_btu			= ls->rx_max_btu;
	pc->intranode                   = 0;
	pc->limited_resource		= 0;
	pc->ls_index                    = ls->index;
        pc->port_index                  = ls->port_index;
        pc->dlc_index                   = ls->dlc_index;
	pc->tg_number			= ls->effective_tg;
	pc->whole_bind			= ls->xid_whole_bind;
	pc->whole_bind_piu_req		= ls->xid_whole_bind_piu_req;
      	pc->gen_odai_usage_opt_set	= ls->xid_gen_odai_usage_opt_set;
        ls->pc_index    		= sna_pc_init(pc, &err);
        if (err < 0) {
        	sna_debug(5, "pc_init failed `%d'.\n", err);
		kfree(pc);
		goto out;
        }
out:	return err;
}

int sna_cs_pc_deactivate(struct sna_ls_cb *ls)
{
	int err;

	sna_debug(5, "init\n");
	err = sna_pc_destroy(ls->pc_index);
	if (err < 0)
		sna_debug(5, "falied to destroy pc instance\n");
	ls->pc_index = 0;
	return err;
}

int sna_cs_as_activate(struct sna_ls_cb *ls)
{
	struct sna_asm_cb *as;
	int err = -ENOMEM;

	sna_debug(5, "init\n");
	new(as, GFP_ATOMIC);
	if (!as) {
		sna_debug(5, "out of memory.. failed\n");
		goto out;
	}
	as->index			= ls->pc_index;
	as->odai			= ls->odai;
	as->rx_max_btu			= ls->rx_max_btu;
	as->intranode			= 0;
	as->tx_adaptive_bind_pacing	= 0;
	as->rx_adaptive_bind_pacing	= 0;
	as->dlus_lu_reg			= 0;
	as->adptv_bind_pacing		= 0;
	as->gen_odai_usage_opt_set	= ls->xid_gen_odai_usage_opt_set;
	err = sna_asm_activate_as(as);
	if (err < 0) {
		sna_debug(5, "asm_activate failed `%d'.\n", err);
		kfree(as);
		goto out;
	}
out:	return err;
}

int sna_cs_as_deactivate(struct sna_ls_cb *ls)
{
	int err;

	sna_debug(5, "init\n");
	err = sna_asm_deactivate_as(ls->pc_index);
	if (err < 0)
		sna_debug(5, "failed to destory as instance.\n");
	return err;
}

int sna_cs_tg_activate(struct sna_ls_cb *ls)
{
	struct sna_tg_update *tg;
	int err = -ENOMEM;
	
	sna_debug(5, "init\n");
	new(tg, GFP_ATOMIC);
	if (!tg) {
		sna_debug(5, "out of memory.. failed\n");
		goto out;
	}
	memcpy(&tg->plu_name, &ls->plu_name, sizeof(sna_netid));
	tg->tg_number			= ls->effective_tg;
	tg->l_node_type			= 0;	/* endpoint. */
	tg->r_node_type			= 0;	/* endpoint. */
	tg->routing			= 0;	/* no routing on endpoints. */
	tg->ls_index			= ls->index;
	tg->port_index			= ls->port_index;
	tg->dlc_index			= ls->dlc_index;
	err = sna_tdm_tg_update(&ls->netid, tg);
	if (err < 0)
		sna_debug(5, "tg_update failed `%d'.\n", err);
	kfree(tg);
out:	return err;
}

int sna_cs_xid_results(struct sna_ls_cb *ls, int suprise)
{
	sna_xid3 *local = NULL, *remote = NULL;

	sna_debug(5, "init\n");

	if (!ls->xid_last_tx || !ls->xid_last_rx)
		return -EINVAL;
	local  = (sna_xid3 *)ls->xid_last_tx->data;
	remote = (sna_xid3 *)ls->xid_last_rx->data;

	switch (ls->effective_role) {
                case SNA_LS_ROLE_PRI:
                        ls->odai = 0;
                        break;
                case SNA_LS_ROLE_SEC:
                default:
                        ls->odai = 1;
                        break;
        }	
	if (suprise) {
		ls->tx_max_btu	= ntohs(local->max_btu_len);
		ls->rx_max_btu	= ntohs(local->max_btu_len);
		ls->tx_window	= 7;
		ls->rx_window	= 7;
		goto out;
	}

	ls->tx_max_btu	= ntohs(remote->max_btu_len);
	ls->rx_max_btu	= ntohs(local->max_btu_len);
	ls->tx_window	= remote->max_iframes;
	ls->rx_window	= local->max_iframes;

	/* there are other values we should set, but for now
	 * we don't need them and I don't want to waste the time
	 * coding them up. (see appnarch.boo 2.9.11.4)
	 */

out:	return 0;
}

int sna_cs_xid_pkt_input(struct sk_buff *skb)
{
	sna_xid3 *xid = (sna_xid3 *)skb->data;

	sna_debug(5, "init: %p %02X len = %d\n", sna_cs_xid_pkt_input, skb->data[0], skb->len);
	if (skb->len == 0)
		return XID_IN_NULL;
	sna_debug(5, "state = %d role = %d\n", xid->state, xid->ls_role);
	if (xid->state == SNA_XID_XCHG_STATE_PN)
		return XID_IN_PN;
	if (xid->state == SNA_XID_XCHG_STATE_NONE 
		|| xid->state == SNA_XID_XCHG_STATE_NEG) {
		if (xid->ls_role == SNA_LS_ROLE_SEC)
			return XID_IN_SEC;
		if (xid->ls_role == SNA_LS_ROLE_PRI)
			return XID_IN_PRI;
		if (xid->ls_role == SNA_LS_ROLE_NEG)
			return XID_IN_NEG;
	}
	return -1;
}

int xid_flip_direction(struct sna_ls_cb *ls)
{
	switch (ls->xid_direction) {
		case XID_DIR_INBOUND:
			ls->xid_direction = XID_DIR_OUTBOUND;
			break;
		case XID_DIR_OUTBOUND:
			ls->xid_direction = XID_DIR_INBOUND;
			break;
	}
	return ls->xid_direction;
}

int xid_node_id_high(struct sna_ls_cb *ls)
{
	struct sna_cs_cb *cs;
	sna_xid3 *xid;

	sna_debug(5, "init\n");
	if (!ls->xid_last_rx)
		return 1;
	cs = sna_cs_get_by_index(ls->cs_index);
        if (!cs)
		return 0;
	xid = (sna_xid3 *)ls->xid_last_rx->data;
	if (cs->nodeid	> xid->nodeid)
		return 1;
	return 0;
}

int xid_node_id_low(struct sna_ls_cb *ls)
{
	struct sna_cs_cb *cs;
        sna_xid3 *xid;

	sna_debug(5, "init\n");
	if (!ls->xid_last_rx)
                return 1;
        cs = sna_cs_get_by_index(ls->cs_index);
        if (!cs)
                return 0;
        xid = (sna_xid3 *)ls->xid_last_rx->data;
	if (cs->nodeid < xid->nodeid)
                return 1;
	return 0;
}

int xid_node_id_eq(struct sna_ls_cb *ls)
{
	struct sna_cs_cb *cs;
        sna_xid3 *xid;

	sna_debug(5, "init\n");
	if (!ls->xid_last_rx)
                return 1;
        cs = sna_cs_get_by_index(ls->cs_index);
        if (!cs)
                return 0;
        xid = (sna_xid3 *)ls->xid_last_rx->data;
	if (cs->nodeid == xid->nodeid)
                return 1;
	return 0;
}

static int sna_cs_tg_name_high(sna_netid *local_n, sna_netid *remote_n)
{
	char local_c[SNA_FQCP_NAME_LEN], remote_c[SNA_FQCP_NAME_LEN];

	memset(local_c, 0, SNA_FQCP_NAME_LEN);
	memset(remote_c, 0, SNA_FQCP_NAME_LEN);
	sna_netid_to_char(local_n, local_c);
	sna_netid_to_char(remote_n, remote_c);
	return strncmp(local_c, remote_c, SNA_FQCP_NAME_LEN);
}

static int sna_cs_tg_negotiate(struct sna_ls_cb *ls)
{
	int err = 0;
	
	sna_debug(5, "init\n");
	if (ls->tg_input == SNA_TG_INPUT_BEGIN
		|| ls->tg_input == SNA_TG_INPUT_NULL) {
		goto out;
	}
	if (ls->tg_input == SNA_TG_INPUT_ADJ_LOC) {
		if (ls->als.effective_tg == 0 && ls->effective_tg == 0
			&& !ls->xid_parallel_tg_sup) {
			ls->tg_state = SNA_TG_STATE_DONE_LOCAL;
			goto out;
		}
		if (ls->als.effective_tg == 0 && ls->effective_tg == 0
			&& ls->xid_parallel_tg_sup) {
			if (sna_cs_tg_name_high(&ls->netid, &ls->plu_name)) {
				ls->tg_state = SNA_TG_STATE_DONE_LOCAL;
				/* for now we simply have a hard coded tg
				 * number as we only support a single tg.
				 * needs to be in range 21 - 239.
				 */
				ls->effective_tg = 21;
				goto out;
			}
			goto out;
		}
		if (ls->als.effective_tg == 0 && ls->effective_tg > 0) {
			if (ls->tg_state == SNA_TG_STATE_RESET) {
				ls->tg_state = SNA_TG_STATE_DONE_LOCAL;
				goto out;
			}
			if (ls->tg_state == SNA_TG_STATE_DONE_PARTNER) {
				/* use the current local tg.
				 * ls->effective_tg = ls->effective_tg;
				 */
				goto out;
			}
			goto out;
		}
		if (ls->als.effective_tg > 0 && ls->effective_tg == 0) {
			if (ls->tg_state == SNA_TG_STATE_RESET) {
				ls->tg_state = SNA_TG_STATE_DONE_PARTNER;
				ls->effective_tg = ls->als.effective_tg;
				goto out;
			}
			goto out;
		}
		if (ls->als.effective_tg > 0 && ls->effective_tg > 0) {
			if (ls->tg_state == SNA_TG_STATE_RESET) {
				if (sna_cs_tg_name_high(&ls->netid, &ls->plu_name)) {
					ls->tg_state = SNA_TG_STATE_DONE_LOCAL;
					goto out;
				} else {
					ls->tg_state = SNA_TG_STATE_DONE_PARTNER;
					ls->effective_tg = ls->als.effective_tg;
					goto out;
				}
				goto out;
			}
			if (ls->tg_state == SNA_TG_STATE_DONE_PARTNER) {
                                /* use the current local tg.
                                 * ls->effective_tg = ls->effective_tg;
                                 */
                                goto out;
                        }
			goto out;
		}
		goto out;
	}
	if (ls->tg_input == SNA_TG_INPUT_SETMODE) {
		ls->tg_state = SNA_TG_STATE_DONE_LOCAL;
		/* we use the tgn currently in ls->effective_tg. */
		goto out;
	}
	if (ls->tg_input == SNA_TG_INPUT_RESET) {
		if (ls->tg_state != SNA_TG_STATE_RESET) {
			ls->tg_state		= SNA_TG_STATE_RESET;
			ls->tg_input		= SNA_TG_INPUT_BEGIN;
			ls->effective_tg	= ls->tg_number;
		}
		goto out;
	}
out:	return err;
}

int sna_cs_validate_adjacent_node_id(struct sk_buff *skb)
{
	sna_debug(5, "sna_cs_validate_adjacent_node_id\n");

	return 0;
}

int sna_cs_xid_error_chk(struct sk_buff *skb)
{
	sna_debug(5, "sna_cs_xid_error_chk\n");

	return 0;
}

sna_xid3 *xid_pkt_init(struct sna_ls_cb *ls, struct sk_buff *skb)
{
	struct sna_cs_cb *cs;
	struct sna_dlc_cb *dlc;
        sna_xid3 *x3;
	u_int8_t len;
	
        sna_debug(5, "init\n");
	cs = sna_cs_get_by_index(ls->cs_index);
	if (!cs)
		return NULL;
	dlc = sna_cs_dlc_get_by_index(ls->dlc_index);
	if (!dlc)
		return NULL;
	len = sizeof(*x3);
        x3  = (sna_xid3 *)skb_put(skb, sizeof(*x3));
	memset(x3, 0, sizeof(*x3));

	/* gathered xid values. */
	x3->type			= ls->xid_type;
	x3->format			= ls->xid_format;
	x3->ls_role			= ls->effective_role;
	x3->nodeid 			= htonl(cs->nodeid);
	
	/* settable xid values. */
	x3->init_self			= ls->xid_init_self;
	x3->standalone_bind		= ls->xid_standalone_bind;
	x3->whole_bind			= ls->xid_whole_bind;
	x3->whole_bind_piu_req		= ls->xid_whole_bind_piu_req;
	x3->actpu_suppression		= ls->xid_actpu_suppression;
	x3->network_node		= ls->xid_network_node;
	x3->cp_services			= ls->xid_cp_services;
	x3->tx_adaptive_bind_pacing	= ls->xid_tx_adaptive_bind_pacing;
	x3->rx_adaptive_bind_pacing     = ls->xid_rx_adaptive_bind_pacing;
	x3->quiesce_tg			= ls->xid_quiesce_tg;
	x3->pu_cap_sup			= ls->xid_pu_cap_sup;
	x3->appn_pbn			= ls->xid_appn_pbn;
	x3->parallel_tg_sup		= ls->xid_parallel_tg_sup;
	x3->ls_txrx_cap			= ls->xid_ls_txrx_cap;
	
	/* dlc related xid values. */
	x3->tg_num			= ls->effective_tg;
	x3->dlc_type			= sna_cs_dlc_xid_type(dlc);
        x3->dlc_len                    	= sna_cs_dlc_xid_len(dlc);
	x3->dlc_abm			= sna_cs_dlc_xid_abm(dlc);
	x3->dlc_init_mode		= sna_cs_dlc_xid_init_mode(dlc);
	x3->max_btu_len                 = sna_cs_dlc_xid_max_btu_len(dlc);
	x3->max_iframes			= sna_cs_dlc_xid_max_iframes(dlc);

	/* control vectors always present.
	 * - product id.
	 * - network name.
	 */

	/* product id control vector information. */
	len += sna_vector_product_id(skb);
	
	/* network name control vector information. */
	len += sna_vector_network_name(&cs->netid, skb);
	
	/* set xid length for data we added. */
	x3->len = len;
        return x3;
}

static int xid_suprise_srnm(struct sna_ls_cb *ls)
{
	sna_xid3 *xid;
	int err = -EINVAL;
	
	sna_debug(5, "init\n");
	if (!ls->xid_last_rx)
		goto out;
	xid = (sna_xid3 *)ls->xid_last_rx->data;
	if (xid->network_node) {
		sna_debug(10, "network_node\n");
		goto out;
	}
	if (xid->cp_cp_support) {
		sna_debug(10, "cp_cp_support\n");
		goto out;
	}
	if (xid->parallel_tg_sup) {
		sna_debug(10, "parallel_tg_sup\n");
		goto out;
	}
	if (xid->tg_num) {
		sna_debug(10, "tg_num\n");
		goto out;
	}
	if (!xid->whole_bind) {
		sna_debug(10, "whole_bind\n");
		goto out;
	}
	if (!xid->whole_bind_piu_req) {
		sna_debug(10, "whole_bind_piu_req\n");
		goto out;
	}
	if (xid->rx_adaptive_bind_pacing) {
		sna_debug(10, "rx_adaptive_bind_pacing\n");
		goto out;
	}
	if (xid->tx_adaptive_bind_pacing) {
		sna_debug(10, "tx_adaptive_bind_pacing\n");
		goto out;
	}
	if (xid->ls_role != SNA_LS_ROLE_SEC && ls->effective_role != SNA_LS_ROLE_SEC) {
		sna_debug(10, "ls_role\n");
		goto out;
	}
	if (xid->sec_nonactive_xchg) {
		sna_debug(10, "sec_nonactive_xchg\n");
		goto out;
	}
	if (xid->cp_name_chg) {
		sna_debug(10, "cp_name_chg\n");
		goto out;
	}
	if (xid->quiesce_tg) {
		sna_debug(10, "quiesce_tg\n");
		goto out;
	}

	ls->tg_input  = SNA_TG_INPUT_SETMODE;
        sna_cs_tg_negotiate(ls);
	ls->xid_input = XID_IN_ACTIVE;
	err = xid_doit(ls);
out:	return err;
}

static void xid_output_connect_finish(void *data)
{
	struct sna_ls_cb *ls = (struct sna_ls_cb *)data;

	if (!ls) {
		sna_debug(5, "!ls\n");
		return;
	}
	if (ls->xid_input == XID_IN_ACTIVE) {
		if (!ls->llc_sk)
			return;		/* retry timer will try again. */
		/* execute active state change. */
	        ls->xid_input           = XID_IN_ACTIVE;
	        ls->xid_direction       = XID_DIR_OUTBOUND;
	        xid_doit(ls);
		return;
	}
	if (ls->xid_state == XID_STATE_RESET)
		return;
	if (signal_pending(current))
		return;
        queue_task(&ls->connect, &tq_immediate);
        mark_bh(IMMEDIATE_BH);
	return;
}

static int xid_output_resend(struct sna_ls_cb *ls)
{
	struct sk_buff *skb;
	int err = -EINVAL;

	sna_debug(5, "init\n");
	if (!ls->xid_last_tx)
		goto out;
	err = -ENOMEM;
	skb = skb_copy(ls->xid_last_tx, GFP_ATOMIC);
	if (!skb)
		goto out;

	/* once in resend, means we have hit an error state,
	 * retry until failed.
	 */
	if (ls->state != SNA_LS_STATE_ACTIVATED) {
		err = 0;
		goto out;
	}
        if (!ls->retry_on_fail || (ls->retry_on_fail
        	&& ls->retry_times && ls->retries >= ls->retry_times)) {
		sna_debug(5, "XID failed\n");
		ls->xid_input = XID_IN_RESET;
                xid_doit(ls);
		err = 0;
		goto out;
	}
        ls->retries++;
        ls->retry.expires = jiffies + sysctl_xid_retry_limit;
        add_timer(&ls->retry);

	ls->xid_count++;
	if (ls->xid_last_tx_direction == XID_DIR_OUTBOUND)
                err = sna_dlc_xid_req(ls, skb);
        else
                err = sna_dlc_xid_rsp(ls, skb);
out:    return err;
}

static int xid_output_null(struct sna_ls_cb *ls)
{
	struct sk_buff *skb;
	int size, err = -ENOMEM;

	sna_debug(5, "init: %p\n", xid_output_null);
	size = sna_dlc_xid_min_len(ls) + 150;   /* static max for now. */
        skb = sna_alloc_skb(size, GFP_ATOMIC);
        if (!skb)
		goto out;
        sna_dlc_xid_reserve(ls, skb);
	if (ls->xid_last_tx) {
		sna_debug(5, "prev->skb=%p\n", ls->xid_last_tx);
		kfree_skb(ls->xid_last_tx);
		ls->xid_last_tx = NULL;
	}
	ls->xid_last_tx 		= skb_copy(skb, GFP_ATOMIC);
	ls->xid_last_tx_direction	= ls->xid_direction;
	ls->xid_count++;
	sna_debug(5, "skb=%p, copy=%p\n", skb, ls->xid_last_tx);
	if (ls->xid_direction == XID_DIR_OUTBOUND) {
		ls->retry.expires = jiffies + sysctl_xid_idle_limit;
	        add_timer(&ls->retry);
		err = sna_dlc_xid_req(ls, skb);
	} else
		err = sna_dlc_xid_rsp(ls, skb);
out:	return err;
}

static int xid_output_pn(struct sna_ls_cb *ls)
{
	struct sk_buff *skb;
        int size, err = -ENOMEM;
	sna_xid3 *x3;

        sna_debug(5, "init\n");
        size = sna_dlc_xid_min_len(ls) + sizeof(*x3) + 150;   /* static max for now. */
        skb = sna_alloc_skb(size, GFP_ATOMIC);
        if (!skb)
                goto out;
        sna_dlc_xid_reserve(ls, skb);
	x3 = xid_pkt_init(ls, skb);
	if (!x3) {
		kfree_skb(skb);
		goto out;
	}
	/* set prenegotiation xid values. */
	x3->state = SNA_XID_XCHG_STATE_PN;
        if (ls->xid_last_tx) {
                kfree_skb(ls->xid_last_tx);
		ls->xid_last_tx = NULL;
	}
	ls->xid_last_tx                 = skb_copy(skb, GFP_ATOMIC);
        ls->xid_last_tx_direction       = ls->xid_direction;
        ls->xid_count++;
	if (ls->xid_direction == XID_DIR_OUTBOUND) {
		ls->retry.expires = jiffies + sysctl_xid_idle_limit;
                add_timer(&ls->retry);
                err = sna_dlc_xid_req(ls, skb);
        } else
                err = sna_dlc_xid_rsp(ls, skb);
out:    return err;
}

static int xid_output_neg(struct sna_ls_cb *ls)
{
	struct sk_buff *skb;
        int size, err = -ENOMEM;
	sna_xid3 *x3;

        sna_debug(5, "init\n");
	ls->tg_input = SNA_TG_INPUT_ADJ_LOC;
	sna_cs_tg_negotiate(ls);
        size = sna_dlc_xid_min_len(ls) + sizeof(*x3) + 150;   /* static max for now. */
        skb = sna_alloc_skb(size, GFP_ATOMIC);
        if (!skb)
                goto out;
        sna_dlc_xid_reserve(ls, skb);
	x3 = xid_pkt_init(ls, skb);
        if (!x3) {
                kfree_skb(skb);
                goto out;
        }
	/* set negotiation-proceeding xid values. */
	x3->state = SNA_XID_XCHG_STATE_NEG;
        if (ls->xid_last_tx) {
                kfree_skb(ls->xid_last_tx);
		ls->xid_last_tx = NULL;
	}
	ls->xid_last_tx                 = skb_copy(skb, GFP_ATOMIC);
        ls->xid_last_tx_direction       = ls->xid_direction;
        ls->xid_count++;
	if (ls->xid_direction == XID_DIR_OUTBOUND) {
		ls->retry.expires = jiffies + sysctl_xid_idle_limit;
                add_timer(&ls->retry);
                err = sna_dlc_xid_req(ls, skb);
        } else
                err = sna_dlc_xid_rsp(ls, skb);
out:    return err;
}

static int xid_output_neg_id(struct sna_ls_cb *ls)
{
        struct sk_buff *skb;
        int size, err = -ENOMEM;
	sna_xid3 *x3;

        sna_debug(5, "init\n");
	ls->tg_input = SNA_TG_INPUT_ADJ_LOC;
        sna_cs_tg_negotiate(ls);
        size = sna_dlc_xid_min_len(ls) + sizeof(*x3) + 150;   /* static max for now. */
        skb = sna_alloc_skb(size, GFP_ATOMIC);
        if (!skb)
                goto out;
        sna_dlc_xid_reserve(ls, skb);
	x3 = xid_pkt_init(ls, skb);
        if (!x3) {
                kfree_skb(skb);
                goto out;
        }
        /* set negotiation-proceeding xid values. */
	x3->state = SNA_XID_XCHG_STATE_NEG;
        if (ls->xid_last_tx) {
                kfree_skb(ls->xid_last_tx);
		ls->xid_last_tx = NULL;
	}
	ls->xid_last_tx                 = skb_copy(skb, GFP_ATOMIC);
        ls->xid_last_tx_direction       = ls->xid_direction;
        ls->xid_count++;
	if (ls->xid_direction == XID_DIR_OUTBOUND) {
		ls->retry.expires = jiffies + sysctl_xid_idle_limit;
                add_timer(&ls->retry);
                err = sna_dlc_xid_req(ls, skb);
        } else
                err = sna_dlc_xid_rsp(ls, skb);
out:    return err;
}

static int xid_output_pri(struct sna_ls_cb *ls)
{
        struct sk_buff *skb;
        int size, err = -ENOMEM;
	sna_xid3 *x3;

        sna_debug(5, "init\n");
	ls->tg_input = SNA_TG_INPUT_ADJ_LOC;
        sna_cs_tg_negotiate(ls);
        size = sna_dlc_xid_min_len(ls) + sizeof(*x3) + 150;   /* static max for now. */
        skb = sna_alloc_skb(size, GFP_ATOMIC);
        if (!skb)
                goto out;
        sna_dlc_xid_reserve(ls, skb);
	x3 = xid_pkt_init(ls, skb);
        if (!x3) {
                kfree_skb(skb);
                goto out;
        }
        /* set negotiation-proceeding xid values. */
	x3->state = SNA_XID_XCHG_STATE_NEG;
        if (ls->xid_last_tx) {
                kfree_skb(ls->xid_last_tx);
		ls->xid_last_tx = NULL;
	}
	ls->xid_last_tx                 = skb_copy(skb, GFP_ATOMIC);
        ls->xid_last_tx_direction       = ls->xid_direction;
        ls->xid_count++;
	if (ls->xid_direction == XID_DIR_OUTBOUND) {
		ls->retry.expires = jiffies + sysctl_xid_idle_limit;
                add_timer(&ls->retry);
                err = sna_dlc_xid_req(ls, skb);
        } else
                err = sna_dlc_xid_rsp(ls, skb);
out:    return err;
}

static int xid_output_sec(struct sna_ls_cb *ls)
{
        struct sk_buff *skb;
        int size, err = -ENOMEM;
	sna_xid3 *x3;

        sna_debug(5, "init\n");
	ls->tg_input = SNA_TG_INPUT_ADJ_LOC;
        sna_cs_tg_negotiate(ls);
        size = sna_dlc_xid_min_len(ls) + sizeof(*x3) + 150;   /* static max for now. */
        skb = sna_alloc_skb(size, GFP_ATOMIC);
        if (!skb)
                goto out;
        sna_dlc_xid_reserve(ls, skb);
	x3 = xid_pkt_init(ls, skb);
        if (!x3) {
                kfree_skb(skb);
                goto out;
        }
        /* set negotiation-proceeding xid values. */
	x3->state = SNA_XID_XCHG_STATE_NEG;
        if (ls->xid_last_tx) {
                kfree_skb(ls->xid_last_tx);
		ls->xid_last_tx = NULL;
	}
	ls->xid_last_tx                 = skb_copy(skb, GFP_ATOMIC);
        ls->xid_last_tx_direction       = ls->xid_direction;
        ls->xid_count++;
	if (ls->xid_direction == XID_DIR_OUTBOUND) {
		ls->retry.expires = jiffies + sysctl_xid_idle_limit;
                add_timer(&ls->retry);
                err = sna_dlc_xid_req(ls, skb);
        } else
                err = sna_dlc_xid_rsp(ls, skb);
out:    return err;
}

int xid_output_pri_r(struct sna_ls_cb *ls)
{
	sna_debug(5, "init\n");
	ls->effective_role = SNA_LS_ROLE_PRI;
	return 0;
}

static int xid_output_sec_r(struct sna_ls_cb *ls)
{
	sna_debug(5, "init\n");
	ls->effective_role = SNA_LS_ROLE_SEC;
	return 0;
}

static int xid_output_random(struct sna_ls_cb *ls)
{
	sna_debug(5, "init\n");
	return 0;
}

static int xid_output_err_opt(struct sna_ls_cb *ls)
{
	int err;
	
	sna_debug(5, "init\n");
	err = xid_suprise_srnm(ls);
	if (err < 0) {
		ls->tg_input 		= SNA_TG_INPUT_RESET;
                sna_cs_tg_negotiate(ls);
		ls->xid_input 		= XID_IN_RESET;
		xid_doit(ls);
		ls->xid_input 		= XID_IN_PN;
		ls->xid_direction 	= XID_DIR_OUTBOUND;
		xid_doit(ls);
	}
        return 0;
}

static int xid_output_z1(struct sna_ls_cb *ls)
{
	sna_debug(5, "init\n");
	if (ls->xid_state == XID_STATE_RESET) {
		ls->tg_input 		= SNA_TG_INPUT_RESET;
	        sna_cs_tg_negotiate(ls);
		ls->xid_input 		= XID_IN_PN;
		ls->xid_direction	= XID_DIR_OUTBOUND;
		xid_doit(ls);
	}
        return 0;
}

static int xid_output_z2(struct sna_ls_cb *ls)
{
	sna_debug(5, "init\n");
        return 0;
}

static int xid_output(struct sna_ls_cb *ls, int action)
{
	int err = -EINVAL;

	sna_debug(5, "init\n");
	switch (action) {
		case XID_OUT_NULL:
			err = xid_output_null(ls);
			break;
        	case XID_OUT_PN:
			err = xid_output_pn(ls);
			break;
        	case XID_OUT_NEG:
			ls->effective_role = SNA_LS_ROLE_NEG;
			err = xid_output_neg(ls);
			break;
        	case XID_OUT_NEG_ID:
			ls->effective_role = SNA_LS_ROLE_NEG;
			err = xid_output_neg_id(ls);
			break;
        	case XID_OUT_PRI:
			ls->effective_role = SNA_LS_ROLE_PRI;
			err = xid_output_pri(ls);
			break;
        	case XID_OUT_SEC:
			ls->effective_role = SNA_LS_ROLE_SEC;
			err = xid_output_sec(ls);
			break;
        	case XID_OUT_PRI_R:
			err = xid_output_pri_r(ls);
			break;
        	case XID_OUT_SEC_R:
			err = xid_output_sec_r(ls);
			break;
        	case XID_OUT_RANDOM:
			err = xid_output_random(ls);
			break;
        	case XID_OUT_ERR_OPT:
			err = xid_output_err_opt(ls);
			break;
        	case XID_OUT_Z1:
			err = xid_output_z1(ls);
			break;
        	case XID_OUT_Z2:
			err = xid_output_z2(ls);
			break;
		case XID_OUT_RESEND:
			err = xid_output_resend(ls);
			break;
		default:
			sna_debug(5, "Uknown output command `0x%02X'.\n", 
				action);
			break;
	}
	sna_debug(5, "fini\n");
	return err;
}

static int xid_input_begin(struct sna_ls_cb *ls)
{
	sna_debug(5, "init\n");
	
	/* allow user defined link activation direction. */
	if (ls->xid_direction == XID_DIR_INBOUND
		&& ls->direction == SNA_LS_DIR_OUT)
		return -EINVAL;
	if (ls->xid_direction == XID_DIR_OUTBOUND
		&& ls->direction == SNA_LS_DIR_IN)
		return -EINVAL;
	switch (ls->xid_state) {
		case XID_STATE_RESET:
			/* DNULL: - (NULL) */
			if (ls->xid_init_method == SNA_LS_XID_INIT_NULL) {
				xid_output(ls, XID_OUT_NULL);
				break;
			}
			/* DPN: 2 (PN) */
			if (ls->xid_init_method == SNA_LS_XID_INIT_PN) {
				xid_output(ls, XID_OUT_PN);
				ls->xid_state = XID_STATE_S_PN;
				break;
			}
			/* DNP,DNEG: 3 (NEG) */
			if (ls->xid_init_method == SNA_LS_XID_INIT_NP
				&& ls->effective_role == SNA_LS_ROLE_NEG) {
				xid_output(ls, XID_OUT_NEG);
				ls->xid_state = XID_STATE_S_NEG;
				break;
			}
			/* DNP,DPRI: 4 (PRI) */
			if (ls->xid_init_method == SNA_LS_XID_INIT_NP
                                && ls->effective_role == SNA_LS_ROLE_PRI) {
				xid_output(ls, XID_OUT_PRI);
				ls->xid_state = XID_STATE_S_PRI;
				break;
			}
			break;
		case XID_STATE_S_PN:
		case XID_STATE_S_NEG:
		case XID_STATE_S_PRI:
		case XID_STATE_S_SEC:
		case XID_STATE_R_PRI_S_SEC:
		case XID_STATE_S_PRI_R_SEC:
		case XID_STATE_ACTIVE:
                        break;
	}
	return 0;
}

static int xid_input_null(struct sna_ls_cb *ls)
{
	sna_debug(5, "init\n");
        switch (ls->xid_state) {
                case XID_STATE_RESET:
                        /* DNULL|DPN: - 2 (PN) */
                        if (ls->xid_init_method == SNA_LS_XID_INIT_NULL
				|| ls->xid_init_method == SNA_LS_XID_INIT_PN) {
				xid_flip_direction(ls);
                                xid_output(ls, XID_OUT_PN);
				ls->xid_state = XID_STATE_S_PN;
				break;
                        }
                        /* DNP,DNEG: 3 (NEG) */
                        if (ls->xid_init_method == SNA_LS_XID_INIT_NP
				&& ls->effective_role == SNA_LS_ROLE_NEG) {
				xid_flip_direction(ls);
                                xid_output(ls, XID_OUT_NEG);
                                ls->xid_state = XID_STATE_S_NEG;
				break;
                        }
                        /* DNP,DPRI: 4 (PRI) */
                        if (ls->xid_init_method == SNA_LS_XID_INIT_NP
                                && ls->effective_role == SNA_LS_ROLE_PRI) {
				xid_flip_direction(ls);
                                xid_output(ls, XID_OUT_PRI);
                                ls->xid_state = XID_STATE_S_PRI;
				break;
                        }
                        /* DNP,DSEC: 5 (SEC) */
                        if (ls->xid_init_method == SNA_LS_XID_INIT_NP
                                && ls->effective_role == SNA_LS_ROLE_SEC) {
				xid_flip_direction(ls);
                                xid_output(ls, XID_OUT_SEC);
                                ls->xid_state = XID_STATE_S_SEC;
				break;
                        }
                        break;
                case XID_STATE_S_PN:
                case XID_STATE_S_NEG:
                case XID_STATE_S_PRI:
                case XID_STATE_S_SEC:
                case XID_STATE_R_PRI_S_SEC:
                case XID_STATE_S_PRI_R_SEC:
			/* resend last tx'd XID image. */
			xid_output(ls, XID_OUT_RESEND);
			break;
                case XID_STATE_ACTIVE:
			/* send to nonactive fsm. */
			sna_debug(5, "FIXME: nonactive fsm\n");
                        break;
        }
        return 0;
}

static int xid_input_pn(struct sna_ls_cb *ls)
{
	sna_debug(5, "init\n");
        switch (ls->xid_state) {
                case XID_STATE_RESET:
			/* DNULL|DPN: - 2 (PN) */
                        if (ls->xid_init_method == SNA_LS_XID_INIT_NULL
                                || ls->xid_init_method == SNA_LS_XID_INIT_PN) {
				xid_flip_direction(ls);
                                xid_output(ls, XID_OUT_PN);
                                ls->xid_state = XID_STATE_S_PN;
				break;
                        }
                        /* DNP,DNEG: 3 (NEG) */
                        if (ls->xid_init_method == SNA_LS_XID_INIT_NP
                                && ls->effective_role == SNA_LS_ROLE_NEG) {
				xid_flip_direction(ls);
                                xid_output(ls, XID_OUT_NEG);
                                ls->xid_state = XID_STATE_S_NEG;
				break;
                        }
                        /* DNP,DPRI: 4 (PRI) */
                        if (ls->xid_init_method == SNA_LS_XID_INIT_NP
                                && ls->effective_role == SNA_LS_ROLE_PRI) {
				xid_flip_direction(ls);
                                xid_output(ls, XID_OUT_PRI);
                                ls->xid_state = XID_STATE_S_PRI;
				break;
                        }
                        /* DNP,DSEC: 5 (SEC) */
                        if (ls->xid_init_method == SNA_LS_XID_INIT_NP
                                && ls->effective_role == SNA_LS_ROLE_SEC) {
				xid_flip_direction(ls);
                                xid_output(ls, XID_OUT_SEC);
                                ls->xid_state = XID_STATE_S_SEC;
				break;
                        }
                        break;
                case XID_STATE_S_PN:
			/* DNEG: 3 (NEG) */
			if (ls->effective_role == SNA_LS_ROLE_NEG) {
				xid_flip_direction(ls);
				xid_output(ls, XID_OUT_NEG);
				ls->xid_state = XID_STATE_S_NEG;
				break;
			}
			/* DPRI: 4 (PRI) */
			if (ls->effective_role == SNA_LS_ROLE_PRI) {
				xid_flip_direction(ls);
				xid_output(ls, XID_OUT_PRI);
				ls->xid_state = XID_STATE_S_PRI;
				break;
			}
			/* DSEC: 5 (SEC) */
			if (ls->effective_role == SNA_LS_ROLE_SEC) {
				xid_flip_direction(ls);
				xid_output(ls, XID_OUT_SEC);
				ls->xid_state = XID_STATE_S_SEC;
				break;
			}
			break;
                case XID_STATE_S_NEG:
                case XID_STATE_S_PRI:
                case XID_STATE_S_SEC:
                case XID_STATE_R_PRI_S_SEC:
                case XID_STATE_S_PRI_R_SEC:
			/* resend last tx'd XID image. */
			xid_output(ls, XID_OUT_RESEND);
                        break;
                case XID_STATE_ACTIVE:
			/* send to nonactive fsm. */
			sna_debug(5, "FIXME: nonactive fsm\n");
                        break;
        }
        return 0;
}

static int xid_input_neg(struct sna_ls_cb *ls)
{
	sna_debug(5, "init\n");
        switch (ls->xid_state) {
                case XID_STATE_RESET:
		case XID_STATE_S_PN:
                        /* DNEG,HI: 4 (PRI) */
			if (ls->effective_role == SNA_LS_ROLE_NEG
				&& xid_node_id_high(ls)) {
				xid_flip_direction(ls);
				xid_output(ls, XID_OUT_PRI);
				ls->xid_state = XID_STATE_S_PRI;
				break;
			}
			/* DNEG,LO: 5 (SEC) */
			if (ls->effective_role == SNA_LS_ROLE_NEG
				&& xid_node_id_low(ls)) {
				xid_flip_direction(ls);
				xid_output(ls, XID_OUT_SEC);
				ls->xid_state = XID_STATE_S_SEC;
				break;
			}
			/* DNEG,EQ: 3 (NEG-ID) */
			if (ls->effective_role == SNA_LS_ROLE_NEG
				&& xid_node_id_eq(ls)) {
				xid_flip_direction(ls);
				xid_output(ls, XID_OUT_NEG_ID);
				ls->xid_state = XID_STATE_S_NEG;
				break;
			}
			/* DPRI: 4 (PRI) */
			if (ls->effective_role == SNA_LS_ROLE_PRI) {
				xid_flip_direction(ls);
				xid_output(ls, XID_OUT_PRI);
				ls->xid_state = XID_STATE_S_PRI;
				break;
			}
			/* DSEC: 5 (SEC) */
			if (ls->effective_role == SNA_LS_ROLE_SEC) {
				xid_flip_direction(ls);
				xid_output(ls, XID_OUT_SEC);
				ls->xid_state = XID_STATE_S_SEC;
				break;
			}
                        break;
                case XID_STATE_S_NEG:
			/* HI: 4 (PRI-R) */
			if (xid_node_id_high(ls)) {
				xid_output(ls, XID_OUT_PRI_R);
				ls->xid_state = XID_STATE_S_PRI;
				break;
			}
			/* LO: 5 (SEC-R) */
			if (xid_node_id_low(ls)) {
				xid_output(ls, XID_OUT_SEC_R);
				ls->xid_state = XID_STATE_S_SEC;
				break;
			}
			/* EQ: - (RANDOM) */
			if (xid_node_id_eq(ls)) {
				xid_output(ls, XID_OUT_RANDOM);
				break;
			}
			break;
                case XID_STATE_S_PRI:
                case XID_STATE_S_SEC:
                case XID_STATE_R_PRI_S_SEC:
                case XID_STATE_S_PRI_R_SEC:
			/* resend last tx'd XID image. */
			xid_output(ls, XID_OUT_RESEND);
			break;
                case XID_STATE_ACTIVE:
			/* send to nonactive fsm. */
			sna_debug(5, "FIXME: nonactive fsm\n");
                        break;
        }
        return 0;
}

static int xid_input_pri(struct sna_ls_cb *ls)
{
	sna_debug(5, "init\n");
        switch (ls->xid_state) {
                case XID_STATE_RESET:
			/* DNEG: 6 (SEC) */
			if (ls->effective_role == SNA_LS_ROLE_NEG) {
				xid_flip_direction(ls);
				xid_output(ls, XID_OUT_SEC);
				ls->xid_state = XID_STATE_R_PRI_S_SEC;
				break;
			}
			/* DPRI: - (Z1) */
			if (ls->effective_role == SNA_LS_ROLE_PRI) {
				xid_output(ls, XID_OUT_Z1);
				break;
			}
			/* DSEC: 6 (SEC) */
			if (ls->effective_role == SNA_LS_ROLE_SEC) {
				xid_flip_direction(ls);
				xid_output(ls, XID_OUT_SEC);
				ls->xid_state = XID_STATE_R_PRI_S_SEC;
				break;
			}
                        break;
                case XID_STATE_S_PN:
			/* DNEG: 6 (SEC) */
			if (ls->effective_role == SNA_LS_ROLE_NEG) {
				xid_flip_direction(ls);
				xid_output(ls, XID_OUT_SEC);
				ls->xid_state = XID_STATE_R_PRI_S_SEC;
				break;
			}
			/* DPRI: 1 (Z1) */
			if (ls->effective_role == SNA_LS_ROLE_PRI) {
				xid_flip_direction(ls);
				xid_output(ls, XID_OUT_Z1);
				ls->xid_state = XID_STATE_RESET;
				break;
			}
			/* DSEC: 6 (SEC) */
			if (ls->effective_role == SNA_LS_ROLE_SEC) {
				xid_flip_direction(ls);
				xid_output(ls, XID_OUT_SEC);
				ls->xid_state = XID_STATE_R_PRI_S_SEC;
				break;
			}
			break;
                case XID_STATE_S_NEG:
			/* 6 (SEC-R) */
			xid_output(ls, XID_OUT_SEC_R);
			ls->xid_state = XID_STATE_R_PRI_S_SEC;
			break;
                case XID_STATE_S_PRI:
			/* 1 (Z1) */
			xid_output(ls, XID_OUT_Z1);
			ls->xid_state = XID_STATE_RESET;
			break;
                case XID_STATE_S_SEC:
			/* 6 */
			ls->xid_state = XID_STATE_R_PRI_S_SEC;
			break;
                case XID_STATE_R_PRI_S_SEC:
			break;
                case XID_STATE_S_PRI_R_SEC:
			/* 1 (Z1) */
			xid_output(ls, XID_OUT_Z1);
                        ls->xid_state = XID_STATE_RESET;
			break;
                case XID_STATE_ACTIVE:
			/* send to nonactive fsm. */
			sna_debug(5, "FIXME: nonactive fsm\n");
                        break;
        }
        return 0;
}

static int xid_input_sec(struct sna_ls_cb *ls)
{
	int err = 0;
	
	sna_debug(5, "init\n");
        switch (ls->xid_state) {
                case XID_STATE_RESET:
		case XID_STATE_S_PN:
			/* !DSEC: 4 (PRI) */
			if (ls->effective_role != SNA_LS_ROLE_SEC) {
				xid_flip_direction(ls);
				xid_output(ls, XID_OUT_PRI);
				ls->xid_state = XID_STATE_S_PRI;
			} else {
				/* DSEC: - (Z1) */
				xid_output(ls, XID_OUT_Z1);
			}
                        break;
                case XID_STATE_S_NEG:
			/* 4 (PRI-R) */
			xid_output(ls, XID_OUT_PRI_R);
			ls->xid_state = XID_STATE_S_PRI;
			break;
                case XID_STATE_S_PRI:
			/* 7 */
			ls->xid_state = XID_STATE_S_PRI_R_SEC;
			/* we are currently in interrupt context here, so
			 * we can't sleep for the connect to complete. hence
			 * we must queue a task and check to see if the task
			 * has finished from there.
			 */
			err = sna_dlc_conn_req(ls);
		        if (err)
		                goto out;
			queue_task(&ls->connect, &tq_immediate);
                        mark_bh(IMMEDIATE_BH);
			break;
                case XID_STATE_S_SEC:
		case XID_STATE_R_PRI_S_SEC:
			/* 1 (Z1) */
			xid_output(ls, XID_OUT_Z1);
			ls->xid_state = XID_STATE_RESET;
			break;
                case XID_STATE_S_PRI_R_SEC:
			/* ignore xid. */
			break;
                case XID_STATE_ACTIVE:
			/* send to nonactive fsm. */
			sna_debug(5, "FIXME: nonactive fsm\n");
                        break;
        }
out:	return err;
}

static int xid_input_setmode(struct sna_ls_cb *ls)
{
	sna_debug(5, "init\n");
        switch (ls->xid_state) {
                case XID_STATE_RESET:
                case XID_STATE_S_PN:
                case XID_STATE_S_NEG:
		case XID_STATE_S_SEC:
			/* 6 (ERR_OPT) */
			xid_output(ls, XID_OUT_ERR_OPT);
			ls->xid_state = XID_STATE_R_PRI_S_SEC;
			break;
                case XID_STATE_S_PRI:
		case XID_STATE_S_PRI_R_SEC:
			/* 1 (Z2) */
			xid_output(ls, XID_OUT_Z2);
			ls->xid_state = XID_STATE_RESET;
			break;
                case XID_STATE_R_PRI_S_SEC:
			/* this is simple for us, if we get here the llc connection is
			 * successful and we can move right into an ACTIVE state.
			 * execute active state change. 
			 */
                        ls->xid_input           = XID_IN_ACTIVE;
                        ls->xid_direction       = XID_DIR_INBOUND;
                        xid_doit(ls);
			break;
                case XID_STATE_ACTIVE:
			/* send to nonactive fsm. */
			sna_debug(5, "FIXME: nonactive fsm\n");
                        break;
        }
        return 0;
}

static int xid_input_active(struct sna_ls_cb *ls)
{
	int err;
	
	sna_debug(5, "init\n");
        switch (ls->xid_state) {
                case XID_STATE_RESET:
                case XID_STATE_S_PN:
                case XID_STATE_S_NEG:
                case XID_STATE_S_PRI:
                case XID_STATE_S_SEC:
			sna_debug(5, "active state from others.\n");
			ls->xid_state   = XID_STATE_ACTIVE;
                        ls->state       = SNA_LS_STATE_ACTIVE;

			/* only get here from suprise. */
			sna_cs_xid_results(ls, 1);
			err = sna_cs_pc_activate(ls);
                        if (err < 0) {
                                sna_debug(5, "pc_activate failed `%d'.\n", err);
                                return err;
                        }
			err = sna_cs_as_activate(ls);
			if (err < 0) {
				sna_debug(5, "as_activate failed `%d'.\n", err);
				return err;
			}
			err = sna_cs_tg_activate(ls);
			if (err < 0) {
				sna_debug(5, "tg_activate failed `%d'.\n", err);
				return err;
			}
			/* wake up anyone waiting for this link to become active. */
			wake_up_interruptible(&ls->sleep);
			break;
                case XID_STATE_R_PRI_S_SEC:
		case XID_STATE_S_PRI_R_SEC:
			sna_debug(5, "active state from normal.\n");
			ls->xid_state 	= XID_STATE_ACTIVE;
			ls->state	= SNA_LS_STATE_ACTIVE;
			
			/* perform necessary finish XID processing for CS. */
			sna_cs_xid_results(ls, 0);
			err = sna_cs_pc_activate(ls);
			if (err < 0) {
				sna_debug(5, "pc_activate failed `%d'.\n", err);
				return err;
			}
			err = sna_cs_as_activate(ls);
                        if (err < 0) {
                                sna_debug(5, "as_activate failed '%d'.\n", err);
                                return err;
                        }
			err = sna_cs_tg_activate(ls);
			if (err < 0) {
				sna_debug(5, "tg_activate_failed `%d'.\n", err);
				return err;
			}
			/* wake up anyone waiting for this link to become active. */
			wake_up_interruptible(&ls->sleep);
			break;
                case XID_STATE_ACTIVE:
                        break;
        }
        return 0;
}

static int xid_input_reset(struct sna_ls_cb *ls)
{
	sna_debug(5, "init\n");
        switch (ls->xid_state) {
                case XID_STATE_RESET:
                case XID_STATE_S_PN:
                case XID_STATE_S_NEG:
                case XID_STATE_S_PRI:
                case XID_STATE_S_SEC:
                case XID_STATE_R_PRI_S_SEC:
                case XID_STATE_S_PRI_R_SEC:
                case XID_STATE_ACTIVE:
			del_timer(&ls->retry);
			ls->retries			= 0;
			ls->tg_input  			= SNA_TG_INPUT_RESET;
		        sna_cs_tg_negotiate(ls);
			if (ls->direction == SNA_LS_DIR_OUT)
				ls->co_status 		= CO_RESET;
			else
		                ls->co_status           = CO_TEST_OK;
			ls->xid_state                   = XID_STATE_RESET;
                        ls->xid_input                   = XID_IN_RESET;		
			ls->xid_direction 		= 0;
			ls->xid_count 			= 0;
			ls->xid_last_tx_direction	= 0;
			ls->xid_last_rx_direction	= 0;
			ls->effective_role		= ls->role;
			if (ls->xid_last_tx) {
				kfree_skb(ls->xid_last_tx);
				ls->xid_last_tx = NULL;
			}
			if (ls->xid_last_rx) {
				kfree_skb(ls->xid_last_rx);
				ls->xid_last_rx = NULL;
			}
#ifdef CONFIG_SNA_LLC
			/* shouldn't be needed but current llc stack will assume
			 * connect and complete a connection before we are notified.
			 */
			if (ls->llc_sk)
				sna_dlc_disc_req(ls);
#endif
			sna_cs_as_deactivate(ls);
			sna_cs_pc_deactivate(ls);

			/* this was an unexpected disconnect, retry connection
			 * until retry limit exceeded.
			 */
			if (ls->flags & SNA_RUNNING) {
				ls->state               = SNA_LS_STATE_ACTIVATED;
        			ls->xid_input           = XID_IN_BEGIN;
        			ls->xid_direction       = XID_DIR_OUTBOUND;
        			sna_cs_connect_out((unsigned long)ls);
			} else {
				wake_up_interruptible(&ls->sleep);
				ls->state = SNA_LS_STATE_DEFINED;
			}
                        break;
        }
        return 0;
}

static int xid_doit(struct sna_ls_cb *ls)
{
	int err = -EINVAL;

	sna_debug(5, "init\n");
	switch (ls->xid_input) {
		case XID_IN_BEGIN:
			err = xid_input_begin(ls);
			break;
		case XID_IN_NULL:
			err = xid_input_null(ls);
			break;
		case XID_IN_PN:
			err = xid_input_pn(ls);
			break;
		case XID_IN_NEG:
			err = xid_input_neg(ls);
			break;
		case XID_IN_PRI:
			err = xid_input_pri(ls);
			break;
		case XID_IN_SEC:
			err = xid_input_sec(ls);
			break;
		case XID_IN_SETMODE:
			err = xid_input_setmode(ls);
			break;
		case XID_IN_ACTIVE:
			err = xid_input_active(ls);
			break;
		case XID_IN_RESET:
			err = xid_input_reset(ls);
			break;
	}
	return err;
}

/**
 * sna_cs_connect_in - inbound xid connection state machine.
 *                      perform all necessary functions to make a link up.
 */
int sna_cs_connect_in(struct sna_ls_cb *ls)
{
	int err = -EINVAL;

	sna_debug(5, "init\n");
	del_timer(&ls->retry);
	if (ls->state == SNA_LS_STATE_DEFINED)
		return err;
	err = xid_doit(ls);
        if (err < 0)
                sna_debug(5, "xid_doit error `%d'.\n", err);
	return err;
}

/**
 * sna_cs_connect_out - outbound xid connection state machine.
 * 			perform all necessary functions to make a link up.
 */
void sna_cs_connect_out(unsigned long data)
{
	struct sna_ls_cb *ls = (struct sna_ls_cb *)data;
	int err;
	
	sna_debug(5, "init: %p ls=%d role=%d retry_on_fail=%d retry_times=%d retires=%d\n",
		sna_cs_connect_out, ls->index, ls->role, ls->retry_on_fail, 
		ls->retry_times, ls->retries);
	del_timer(&ls->retry);
	switch (ls->co_status) {
		case CO_RESET:
		case CO_S_TEST_C:
			if (ls->state != SNA_LS_STATE_ACTIVATED)
				return;
			if (!ls->retry_on_fail || (ls->retry_on_fail 
				&& ls->retry_times && ls->retries >= ls->retry_times)) {
				sna_debug(5, "CO test failed\n");
				ls->co_status = CO_FAIL;
				return;
			}
			ls->retries++;
			ls->co_status 		= CO_S_TEST_C;
			ls->retry.expires 	= jiffies + sysctl_test_retry_limit;
			add_timer(&ls->retry);
			err = sna_dlc_test_req(ls);
                        if (err < 0)
                                sna_debug(5, "dlc_test_req err = %d\n", err);
			return;
		case CO_R_TEST_R:
			ls->co_status 	= CO_TEST_OK;
			ls->retries 	= 0;
			break;
		case CO_TEST_OK:
			break;
		case CO_FAIL:
		default:
			return;
	}
	err = xid_doit(ls);
	if (err < 0)
		sna_debug(5, "xid_doit error `%d'.\n", err);
	return;
}

int sna_cs_wait_for_link_station(struct sna_ls_cb *ls, int seconds)
{
        DECLARE_WAITQUEUE(wait, current);
        int rc, timeout = seconds * HZ;

        sna_debug(5, "init\n");
        add_wait_queue_exclusive(&ls->sleep, &wait);
        for (;;) {
                __set_current_state(TASK_INTERRUPTIBLE);
                rc = 0;
		if (ls->state != SNA_LS_STATE_ACTIVE)
                        timeout = schedule_timeout(timeout);
                if (ls->state == SNA_LS_STATE_ACTIVE) {
                        if (!ls->llc_sk)
                                rc = -EAGAIN;
                        break;
                }
                rc = -EAGAIN;
                if (ls->state == SNA_LS_STATE_DEFINED)
                        break;
                rc = -ERESTARTSYS;
                if (signal_pending(current))
                        break;
                rc = -EAGAIN;
                if (!timeout)
                        break;
        }
        __set_current_state(TASK_RUNNING);
        remove_wait_queue(&ls->sleep, &wait);
        return rc;
}

/**
 * sna_cs_activate_route - negotiate the actual connection. ie perform XID.
 *
 * currently blocks for debug purposes. This will be:
 * sna_cs_activate_route
 *   connect_out
 *   return;
 *
 * connect_out (timer expire or xid finished).
 *   sna_cs_activate_route_finish
 *   return;
 */
u_int32_t sna_cs_activate_route(struct sna_tg_cb *tg, sna_netid *remote_name, int *err)
{
	u_int32_t pc_index = 0;
	struct sna_port_cb *port;
	struct sna_ls_cb *ls;

	sna_debug(5, "init\n");
	*err = 0;
	port = sna_cs_port_get_by_index(tg->port_index);
	if (!port) {
		*err = -ENOENT;
		goto out;
	}
	ls = sna_cs_ls_get_by_index(port, tg->ls_index);
	if (!ls) {
		*err = -ENOENT;
		goto out;
	}
	/* link station is already active, just return the pc_index. */
	if (ls->state == SNA_LS_STATE_ACTIVE) {
		sna_debug(5, "active connection found\n");
		goto done;
	}
	sna_cs_connect_out((unsigned long)ls);
	*err = sna_cs_wait_for_link_station(ls, 255);
	if (*err < 0) {
		sna_debug(5, "connect out failed `%d'.\n", *err);
		goto out;
	}
done:	pc_index = ls->pc_index;
out:	return pc_index;
}

#ifdef CONFIG_PROC_FS
int sna_cs_get_info_dlc(char *buffer, char **start,
        off_t offset, int length)
{
        off_t pos = 0, begin = 0;
	struct sna_dlc_cb *dlc;
	struct list_head *le;
        int len = 0;

        /* output the dlc data for the /proc filesystem. */
	len += sprintf(buffer, "%-8s%-6s%-5s%-5s\n",
		"name", "index", "type", "flags");
	list_for_each(le, &dlc_list) {
		dlc = list_entry(le, struct sna_dlc_cb, list);
                len += sprintf(buffer + len, "%-8s%-6d%03X  %02X   \n",
			dlc->dev->name, dlc->index, dlc->type, dlc->flags);
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

int sna_cs_get_info_port(char *buffer, char **start,
        off_t offset, int length)
{
	off_t pos = 0, begin = 0;
        struct sna_port_cb *port;
	struct list_head *le;
        int len = 0;

        /* output the port data for the /proc filesystem. */
	len += sprintf(buffer, "%-20s%-6s%-5s%-6s%-5s%-8s%-5s%-5s%-5s\n",
		"name", "index", "type", "flags", "lsap", "ls_qlen",
		"btu", "mia", "moa");
	list_for_each(le, &port_list) {
		port = list_entry(le, struct sna_port_cb, list);
		len += sprintf(buffer + len, "%-20s%-6d%02X   %02X    0x%02X %-8d%-5d%-5d%-5d\n",
			port->use_name, port->index, port->type, port->flags,
			port->saddr[0], port->ls_qlen, port->btu, port->mia,
			port->moa);
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

int sna_cs_get_info_ls(char *buffer, char **start,
        off_t offset, int length)
{
	struct list_head *le, *se;
	off_t pos = 0, begin = 0;
	struct sna_port_cb *port;
	struct sna_ls_cb *ls;
        int len = 0;

        /* output the link station data for the /proc filesystem. */
	len += sprintf(buffer, "%-20s%-6s%-6s%-6s%-5s%-4s%-11s%-11s\n",
		"name", "index", "flags", "state", "role", "tg", "tx_max_btu", "rx_max_btu");
	len += sprintf(buffer + len, "  %-18s%-12s%-13s%-9s\n",
		"plu_name", "plu_node_id", "plu_mac_addr", "plu_lsap");
	list_for_each(le, &port_list) {
		port = list_entry(le, struct sna_port_cb, list);
		list_for_each(se, &port->ls_list) {
			ls = list_entry(se, struct sna_ls_cb, list);
			len += sprintf(buffer + len, "%-20s%-6d%02X    %02X    %-5d%-4d%-11d%-11d\n",
				ls->use_name, ls->index, ls->flags, ls->state, 
                                ls->effective_role, ls->effective_tg, 
				ls->tx_max_btu, ls->rx_max_btu);
			len += sprintf(buffer + len, "  %-18s%-12s%-13s%02X\n",
				sna_pr_netid(&ls->plu_name), 
				sna_pr_nodeid(ls->plu_node_id),
				sna_pr_ether(ls->plu_mac_addr), ls->plu_port);
               			if (pos < offset) {
                       			len = 0;
					begin = pos;
               			}
               			if (pos > offset + length) 
                       			break;
       		}
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
