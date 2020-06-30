/* sna_dlc.c: Linux Systems Network Architecture implementation
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

#ifdef CONFIG_SNA_LLC
#include <linux/if_arp.h>
#include <net/llc_if.h>
#include <net/llc_sap.h>
#include <net/llc_pdu.h>
#include <net/llc_conn.h>
#include <linux/llc.h>
#endif

extern char sna_product_name[];

#ifdef CONFIG_SNA_LOOPBACK
int sna_loopback_xmit(struct sk_buff *skb)
{
	sna_debug(5, "sna_loopback_xmit\n");
	netif_rx(skb);
	return 0;
}
#endif

#ifdef CONFIG_SNA_LLC
static inline u16 sna_dlc_llc_protocol_type(u16 arphrd)
{
	u16 rc = htons(ETH_P_802_2);

	if (arphrd == ARPHRD_IEEE802)
		rc = htons(ETH_P_TR_802_2);
	return rc;
}

static inline u_int8_t sna_dlc_llc_header_len(u_int8_t type)
{
	u_int8_t len = LLC_PDU_LEN_U;

	if (type == LLC_TEST_PRIM || type == LLC_XID_PRIM) {
		len = LLC_PDU_LEN_U;
	} else if (type == LLC_DATA_PRIM)
		len = LLC_PDU_LEN_I;
	return len;
}

static int sna_dlc_llc_send_llc1(struct llc_sap *sap, struct sk_buff *skb,
	struct sna_dlc_llc_addr *addr, int primitive, int type)
{
	union llc_u_prim_data prim_data;
	struct llc_prim_if_block prim;
	int err;

	sna_debug(5, "init %p\n", sna_dlc_llc_send_llc1);
	if (!sap) {
		sna_debug(5, "!sap\n");
		return -EINVAL;
	}
	if (!skb) {
		sna_debug(5, "!skb\n");
		return -EINVAL;
	}
	if (!skb->dev) {
		sna_debug(5, "!skb->dev\n");
		return -EINVAL;
	}
	if (!addr) {
		sna_debug(5, "!addr\n");
		return -EINVAL;
	}
	prim.data                 = &prim_data;
	prim.sap                  = sap;
	prim.prim                 = primitive;
	prim.type		  = type;
	prim_data.test.skb        = skb;
	prim_data.test.saddr.lsap = sap->laddr.lsap;
	prim_data.test.daddr.lsap = addr->dsap;
	skb->protocol = sna_dlc_llc_protocol_type(addr->arphrd);
	memcpy(prim_data.test.saddr.mac, skb->dev->dev_addr, IFHWADDRLEN);
	memcpy(prim_data.test.daddr.mac, addr->dmac, IFHWADDRLEN);
	err = sap->req(&prim);
	return err;
}

static int sna_dlc_llc_send_conn(struct llc_sap *sap,
	struct sna_dlc_llc_addr *addr, struct net_device *dev, u_int16_t link)
{
	union llc_u_prim_data prim_data;
	struct llc_prim_if_block prim;

	sna_debug(5, "init\n");
	prim.data           = &prim_data;
	prim.sap            = sap;
	prim.prim           = LLC_CONN_PRIM;
	prim_data.conn.dev  = dev;
	prim_data.conn.link = link;
	prim_data.conn.sk   = NULL;
	prim_data.conn.pri  = 0;
	prim_data.conn.saddr.lsap = sap->laddr.lsap;
	prim_data.conn.daddr.lsap = addr->dsap;
	memcpy(prim_data.conn.saddr.mac, dev->dev_addr, IFHWADDRLEN);
	memcpy(prim_data.conn.daddr.mac, addr->dmac, IFHWADDRLEN);
	return sap->req(&prim);
}

static int sna_dlc_llc_send_disc(struct llc_sap *sap, struct sock *sk, u_int16_t link)
{
	union llc_u_prim_data prim_data;
	struct llc_prim_if_block prim;
	int rc;

	sna_debug(5, "init\n");
	prim.data           = &prim_data;
	prim.sap            = sap;
	prim.prim           = LLC_DISC_PRIM;
	prim_data.disc.sk   = sk;
	prim_data.disc.link = link;
	rc = sap->req(&prim);
	return rc;
}

static int sna_dlc_llc_send_data(struct llc_sap *sap, struct sock *sk,
	struct sk_buff *skb)
{
	union llc_u_prim_data prim_data;
	struct llc_prim_if_block prim;
	int rc;

	sna_debug(5, "init\n");
	prim.data          = &prim_data;
	prim.sap           = sap;
	prim.prim          = LLC_DATA_PRIM;
	prim_data.data.skb = skb;
	prim_data.data.pri = 0;
	prim_data.data.sk  = sk;
	skb->protocol      = sna_dlc_llc_protocol_type(0);
	rc = sap->req(&prim);
	return rc;
}

/**
 * sna_dlc_llc_ind_test_cmd - echo test cmd back to sender.
 *
 * ideally we could keep some counters for diagnostics here.
 */
static void sna_dlc_llc_ind_test_cmd(struct llc_sap *sap,
	struct llc_prim_test *prim_data)
{
	struct sk_buff *skb2, *skb = prim_data->skb;
	struct sna_dlc_llc_addr addr;

	sna_debug(5, "init\n");
	skb2 = skb_copy(skb, GFP_ATOMIC);
	if (!skb2)
		goto out;
	memset(&addr, 0, sizeof(addr));
	addr.ssap = prim_data->daddr.lsap;
	memcpy(addr.smac, prim_data->daddr.mac, IFHWADDRLEN);
	addr.dsap = prim_data->saddr.lsap;
	memcpy(addr.dmac, prim_data->saddr.mac, IFHWADDRLEN);
	sna_dlc_llc_send_llc1(sap, skb2, &addr, LLC_TEST_PRIM,
		LLC_PRIM_TYPE_RESP);
out:	return;
}

static void sna_dlc_llc_ind_test_rsp(struct llc_sap *sap,
	struct llc_prim_test *prim_data)
{
	struct sna_ls_cb *ls;

	sna_debug(5, "init\n");
	ls = sna_cs_ls_get_by_addr(prim_data->saddr.mac,
		&prim_data->saddr.lsap);
	if (!ls) {
		sna_debug(5, "rouge test rsp\n");
		return;
	}
	ls->co_status = CO_R_TEST_R;
	sna_cs_connect_out(&ls->retry);
	return;
}

static void sna_dlc_llc_ind_test(struct llc_prim_if_block *prim)
{
	struct llc_prim_test *prim_data = &prim->data->test;
	struct llc_sap *sap = prim->sap;

	if (prim_data->pri == LLC_PRIM_TYPE_REQ)
		sna_dlc_llc_ind_test_cmd(sap, prim_data);
	else
		sna_dlc_llc_ind_test_rsp(sap, prim_data);
	return;
}

static void sna_dlc_llc_ind_xid_cmd(struct llc_prim_xid *prim_data)
{
	struct sk_buff *skb2, *skb = prim_data->skb;
	struct sna_ls_cb *ls;

	sna_debug(5, "init\n");
	ls = sna_cs_ls_get_by_addr(prim_data->saddr.mac,
		&prim_data->saddr.lsap);
	if (!ls) {
		sna_debug(5, "rouge xid cmd\n");
		return;
	}
	skb2 = skb_copy(skb, GFP_ATOMIC);
	if (!skb2)
		return;
	ls->xid_count++;
	ls->xid_input           = sna_cs_xid_pkt_input(skb);
	ls->xid_direction       = XID_DIR_INBOUND;
	if (ls->xid_last_rx) {
		kfree_skb(ls->xid_last_rx);
		ls->xid_last_rx = NULL;
	}
	ls->xid_last_rx                 = skb2;
	ls->xid_last_rx_direction       = ls->xid_direction;
	sna_cs_connect_in(ls);
	return;
}

static void sna_dlc_llc_ind_xid_rsp(struct llc_prim_xid *prim_data)
{
	struct sk_buff *skb2, *skb = prim_data->skb;
	struct sna_ls_cb *ls;

	sna_debug(5, "init: %p\n", sna_dlc_llc_ind_xid_rsp);
	ls = sna_cs_ls_get_by_addr(prim_data->saddr.mac,
		&prim_data->saddr.lsap);
	if (!ls) {
		sna_debug(5, "rouge xid rsp\n");
		return;
	}
	skb2 = skb_copy(skb, GFP_ATOMIC);
	if (!skb2)
		return;
	ls->xid_count++;
	ls->xid_input 		= sna_cs_xid_pkt_input(skb);
	ls->xid_direction	= XID_DIR_INBOUND;
	if (ls->xid_last_rx) {
		kfree_skb(ls->xid_last_rx);
		ls->xid_last_rx = NULL;
	}
	ls->xid_last_rx         	= skb2;
	ls->xid_last_rx_direction	= ls->xid_direction;
	sna_cs_connect_out(&ls->retry);
	return;
}

static void sna_dlc_llc_ind_xid(struct llc_prim_if_block *prim)
{
	struct llc_prim_xid *prim_data = &prim->data->xid;

	sna_debug(5, "init\n");
	if (prim_data->pri == LLC_PRIM_TYPE_REQ)
		sna_dlc_llc_ind_xid_cmd(prim_data);
	else
		sna_dlc_llc_ind_xid_rsp(prim_data);
	return;
}

static void sna_dlc_llc_ind_dataunit(struct llc_prim_if_block *prim)
{
	sna_debug(5, "init\n");
	return;
}

static void sna_dlc_llc_ind_conn(struct llc_prim_if_block *prim)
{
	struct llc_prim_conn *prim_data = &prim->data->conn;
	struct sna_ls_cb *ls;

	sna_debug(5, "init\n");
	ls = sna_cs_ls_get_by_addr(prim_data->saddr.mac,
		&prim_data->saddr.lsap);
	if (!ls) {
		sna_debug(5, "rouge ind conn (SETMODE)\n");
		return;
	}
	LLC_SK(prim_data->sk)->link = ls->index;
	ls->llc_sk		= prim_data->sk;
	ls->xid_input 		= XID_IN_SETMODE;
	ls->xid_direction	= XID_DIR_INBOUND;
	sna_cs_connect_out(&ls->retry);
	return;
}

static void sna_dlc_llc_ind_data(struct llc_prim_if_block *prim)
{
	struct llc_prim_data *prim_data = &prim->data->data;
	struct sk_buff *skb2, *skb = prim_data->skb;

	sna_debug(5, "init\n");
	skb2 = skb_copy(skb, GFP_ATOMIC);
	if (!skb2)
		return;
	sna_pc_rx_mu(skb2);
	return;
}

static void sna_dlc_llc_ind_disc(struct llc_prim_if_block *prim)
{
	struct llc_prim_disc *prim_data = &prim->data->disc;
	struct sna_port_cb *port;
	struct sna_ls_cb *ls;

	sna_debug(5, "init\n");
	port = sna_cs_port_get_by_addr(&prim->sap->laddr.lsap);
	if (!port) {
		sna_debug(5, "port not found for disc\n");
		return;
	}
	ls = sna_cs_ls_get_by_index(port, prim_data->link);
	if (!ls) {
		sna_debug(5, "ls not found for disc\n");
		return;
	}
	ls->xid_input 		= XID_IN_RESET;
	ls->xid_direction 	= XID_DIR_INBOUND;
	sna_cs_connect_in(ls);
	return;
}

int sna_dlc_llc_indicate(struct llc_prim_if_block *prim)
{
	switch (prim->prim) {
		case LLC_TEST_PRIM:
			sna_dlc_llc_ind_test(prim);
			break;
		case LLC_XID_PRIM:
			sna_dlc_llc_ind_xid(prim);
			break;
		case LLC_DATAUNIT_PRIM:
			sna_dlc_llc_ind_dataunit(prim);
			break;
		case LLC_CONN_PRIM:
			sna_dlc_llc_ind_conn(prim);
			break;
		case LLC_DATA_PRIM:
			sna_dlc_llc_ind_data(prim);
			break;
		case LLC_DISC_PRIM:
			sna_dlc_llc_ind_disc(prim);
			break;
		case LLC_RESET_PRIM:
		case LLC_FLOWCONTROL_PRIM:
		default:
			break;
	}
	return 0;
}

static void sna_dlc_llc_conf_conn(struct llc_prim_if_block *prim)
{
	struct llc_prim_conn *prim_data = &prim->data->conn;
	struct sna_ls_cb *ls;

	sna_debug(5, "init\n");
	ls = sna_cs_ls_get_by_addr(LLC_SK(prim_data->sk)->daddr.mac,
		&LLC_SK(prim_data->sk)->daddr.lsap);
	if (!ls) {
		sna_debug(5, "rouge connection rsp\n");
		return;
	}
	if (!prim->data->conn.status) {
		ls->llc_sk	= prim_data->sk;
		ls->xid_input 	= XID_IN_ACTIVE;
	} else {
		ls->xid_input	= XID_IN_RESET;
		ls->llc_sk	= NULL;
	}
	wake_up_interruptible(&ls->sleep);
	return;
}

static void sna_dlc_llc_conf_data(struct llc_prim_if_block *prim)
{
	struct llc_prim_data *prim_data = &prim->data->data;
	sna_debug(5, "init\n");
	return;
}

static void sna_dlc_llc_conf_disc(struct llc_prim_if_block *prim)
{
	struct llc_prim_disc *prim_data = &prim->data->disc;
	struct sna_ls_cb *ls;

	sna_debug(5, "init\n");
	ls = sna_cs_ls_get_by_addr(LLC_SK(prim_data->sk)->daddr.mac,
		&LLC_SK(prim_data->sk)->daddr.lsap);
	if (!ls) {
		sna_debug(5, "rouge disconnect rsp\n");
		return;
	}
	ls->xid_input = XID_IN_RESET;
	sna_cs_connect_out(&ls->retry);
	return;
}

int sna_dlc_llc_confirm(struct llc_prim_if_block *prim)
{
	switch (prim->prim) {
		case LLC_CONN_PRIM:
			sna_dlc_llc_conf_conn(prim);
			break;
		case LLC_DATA_PRIM:
			sna_dlc_llc_conf_data(prim);
			break;
		case LLC_DISC_PRIM:
			sna_dlc_llc_conf_disc(prim);
			break;
		case LLC_RESET_PRIM:
			break;
		default:
			sna_debug(5, "unknown prim %d\n",
			       prim->prim);
			break;
	}
	return 0;
}

static int sna_dlc_llc_tx_test_c(struct sna_dlc_cb *dlc,
	struct sna_port_cb *port, struct sna_ls_cb *ls)
{
	struct net_device *dev = dlc->dev;
	struct sna_dlc_llc_addr addr;
	struct sk_buff *skb;
	int size;

	sna_debug(5, "init\n");
	addr.arphrd	= dlc->type;
	addr.dsap	= ls->plu_port;
	addr.ssap	= port->saddr[0];
	memcpy(&addr.dmac, ls->plu_mac_addr, IFHWADDRLEN);
	memcpy(&addr.smac, dev->dev_addr, IFHWADDRLEN);
	size = dev->hard_header_len + sna_dlc_llc_header_len(LLC_TEST_PRIM);
	skb = alloc_skb(size + strlen(sna_product_name), GFP_ATOMIC);
	if (!skb)
		return -ENOMEM;
	skb_reserve(skb, size);
	skb->dev = dev;
	strncpy(skb_put(skb, strlen(sna_product_name)), sna_product_name,
		strlen(sna_product_name));
	return sna_dlc_llc_send_llc1(port->llc_dl, skb, &addr,
		LLC_TEST_PRIM, LLC_PRIM_TYPE_REQ);
}

static int sna_dlc_llc_tx_xid_c(struct sna_dlc_cb *dlc,
	struct sna_port_cb *port, struct sna_ls_cb *ls, struct sk_buff *skb)
{
	struct net_device *dev = dlc->dev;
	struct sna_dlc_llc_addr addr;

	sna_debug(5, "init %p\n", sna_dlc_llc_tx_xid_c);
	addr.arphrd     = dlc->type;
	addr.dsap       = ls->plu_port;
	addr.ssap       = port->saddr[0];
	memcpy(&addr.dmac, ls->plu_mac_addr, IFHWADDRLEN);
	memcpy(&addr.smac, dev->dev_addr, IFHWADDRLEN);
	skb->dev = dev;
	return sna_dlc_llc_send_llc1(port->llc_dl, skb, &addr,
		LLC_XID_PRIM, LLC_PRIM_TYPE_REQ);
}

static int sna_dlc_llc_tx_xid_r(struct sna_dlc_cb *dlc,
	struct sna_port_cb *port, struct sna_ls_cb *ls, struct sk_buff *skb)
{
	struct net_device *dev = dlc->dev;
	struct sna_dlc_llc_addr addr;

	sna_debug(5, "init\n");
	addr.arphrd     = dlc->type;
	addr.dsap       = ls->plu_port;
	addr.ssap       = port->saddr[0];
	memcpy(&addr.dmac, ls->plu_mac_addr, IFHWADDRLEN);
	memcpy(&addr.smac, dev->dev_addr, IFHWADDRLEN);
	skb->dev = dev;
	return sna_dlc_llc_send_llc1(port->llc_dl, skb, &addr,
		LLC_XID_PRIM, LLC_PRIM_TYPE_RESP);
}

static int sna_dlc_llc_tx_conn_c(struct sna_dlc_cb *dlc,
	struct sna_port_cb *port, struct sna_ls_cb *ls)
{
	struct net_device *dev = dlc->dev;
	struct sna_dlc_llc_addr addr;

	sna_debug(5, "init\n");
	addr.arphrd     = dlc->type;
	addr.dsap       = ls->plu_port;
	addr.ssap       = port->saddr[0];
	memcpy(&addr.dmac, ls->plu_mac_addr, IFHWADDRLEN);
	memcpy(&addr.smac, dev->dev_addr, IFHWADDRLEN);
	return sna_dlc_llc_send_conn(port->llc_dl, &addr, dev, ls->index);
}

static int sna_dlc_llc_tx_disc_c(struct sna_port_cb *port, struct sna_ls_cb *ls)
{
	sna_debug(5, "init\n");
	if (!ls->llc_sk || ls->xid_state != XID_STATE_ACTIVE)
		return 0;
	return sna_dlc_llc_send_disc(port->llc_dl, ls->llc_sk, ls->index);
}

static int sna_dlc_llc_tx_data_c(struct sna_dlc_cb *dlc,
	struct sna_port_cb *port, struct sna_ls_cb *ls, struct sk_buff *skb)
{
	struct net_device *dev = dlc->dev;

	sna_debug(5, "init\n");
	if (!ls->llc_sk)
		return -EINVAL;
	skb->dev = dev;
	return sna_dlc_llc_send_data(port->llc_dl, ls->llc_sk, skb);
}

#endif	/* CONFIG_SNA_LLC */

int sna_dlc_test_req(struct sna_ls_cb *ls)
{
	struct sna_port_cb *port;
	struct sna_dlc_cb *dlc;
	int err = -EINVAL;

	sna_debug(5, "init\n");
	dlc = sna_cs_dlc_get_by_index(ls->dlc_index);
	if (!dlc)
		goto out;
	port = sna_cs_port_get_by_index(ls->port_index);
	if (!port)
		goto out;
	switch (port->type) {
#ifdef CONFIG_SNA_LLC
		case SNA_PORT_TYPE_LLC:
			err = sna_dlc_llc_tx_test_c(dlc, port, ls);
			break;
#endif	/* CONFIG_SNA_LLC */
	}
out:	return err;
}

int sna_dlc_xid_min_len(struct sna_ls_cb *ls)
{
	struct sna_port_cb *port;
	struct sna_dlc_cb *dlc;
	int size = -ENOENT;

	dlc = sna_cs_dlc_get_by_index(ls->dlc_index);
	if (!dlc)
		goto out;
	port = sna_cs_port_get_by_index(ls->port_index);
	if (!port)
		goto out;
	size = dlc->dev->hard_header_len;
	switch (port->type) {
#ifdef CONFIG_SNA_LLC
		case SNA_PORT_TYPE_LLC:
			size += sna_dlc_llc_header_len(LLC_XID_PRIM);
			break;
#endif
	}
out:	return size;
}

int sna_dlc_xid_reserve(struct sna_ls_cb *ls, struct sk_buff *skb)
{
	skb_reserve(skb, sna_dlc_xid_min_len(ls));
	return 0;
}

int sna_dlc_xid_req(struct sna_ls_cb *ls, struct sk_buff *skb)
{
	struct sna_port_cb *port;
	struct sna_dlc_cb *dlc;
	int err = -EINVAL;

	sna_debug(5, "init\n");
	dlc = sna_cs_dlc_get_by_index(ls->dlc_index);
	if (!dlc)
		goto out;
	port = sna_cs_port_get_by_index(ls->port_index);
	if (!port)
		goto out;
	switch (port->type) {
#ifdef CONFIG_SNA_LLC
		case SNA_PORT_TYPE_LLC:
			err = sna_dlc_llc_tx_xid_c(dlc, port, ls, skb);
			break;
#endif  /* CONFIG_SNA_LLC */
	}
out:	return err;
}

int sna_dlc_xid_rsp(struct sna_ls_cb *ls, struct sk_buff *skb)
{
	struct sna_port_cb *port;
	struct sna_dlc_cb *dlc;
	int err = -EINVAL;

	sna_debug(5, "init\n");
	dlc = sna_cs_dlc_get_by_index(ls->dlc_index);
	if (!dlc)
		goto out;
	port = sna_cs_port_get_by_index(ls->port_index);
	if (!port)
		goto out;
	switch (port->type) {
#ifdef CONFIG_SNA_LLC
		case SNA_PORT_TYPE_LLC:
			err = sna_dlc_llc_tx_xid_r(dlc, port, ls, skb);
			break;
#endif  /* CONFIG_SNA_LLC */
	}
out:	return err;
}

int sna_dlc_disc_req(struct sna_ls_cb *ls)
{
	struct sna_port_cb *port;
	int err = -EINVAL;

	sna_debug(5, "init\n");
	port = sna_cs_port_get_by_index(ls->port_index);
	if (!port)
		goto out;
	switch (port->type) {
#ifdef CONFIG_SNA_LLC
		case SNA_PORT_TYPE_LLC:
			err = sna_dlc_llc_tx_disc_c(port, ls);
			break;
#endif  /* CONFIG_SNA_LLC */
	}
out:    return err;
}

int sna_dlc_conn_req(struct sna_ls_cb *ls)
{
	struct sna_port_cb *port;
	struct sna_dlc_cb *dlc;
	int err = -EINVAL;

	sna_debug(5, "init\n");
	dlc = sna_cs_dlc_get_by_index(ls->dlc_index);
	if (!dlc)
		goto out;
	port = sna_cs_port_get_by_index(ls->port_index);
	if (!port)
		goto out;
	switch (port->type) {
#ifdef CONFIG_SNA_LLC
		case SNA_PORT_TYPE_LLC:
			err = sna_dlc_llc_tx_conn_c(dlc, port, ls);
			break;
#endif  /* CONFIG_SNA_LLC */
	}
out:    return err;
}

int sna_dlc_data_min_len(struct sna_ls_cb *ls)
{
	struct sna_port_cb *port;
	struct sna_dlc_cb *dlc;
	int size = -ENOENT;

	dlc = sna_cs_dlc_get_by_index(ls->dlc_index);
	if (!dlc)
		goto out;
	port = sna_cs_port_get_by_index(ls->port_index);
	if (!port)
		goto out;
	size = dlc->dev->hard_header_len;
	switch (port->type) {
#ifdef CONFIG_SNA_LLC
		case SNA_PORT_TYPE_LLC:
			size += sna_dlc_llc_header_len(LLC_DATA_PRIM);
			break;
#endif
	}
out:    return size;
}

int sna_dlc_data_reserve(struct sna_ls_cb *ls, struct sk_buff *skb)
{
	skb_reserve(skb, sna_dlc_data_min_len(ls));
	return 0;
}

int sna_dlc_data_req(struct sna_ls_cb *ls, struct sk_buff *skb)
{
	struct sna_port_cb *port;
	struct sna_dlc_cb *dlc;
	int err = -EINVAL;

	sna_debug(5, "init\n");
	dlc = sna_cs_dlc_get_by_index(ls->dlc_index);
	if (!dlc)
		goto out;
	port = sna_cs_port_get_by_index(ls->port_index);
	if (!port)
		goto out;
	switch (port->type) {
#ifdef CONFIG_SNA_LLC
		case SNA_PORT_TYPE_LLC:
			err = sna_dlc_llc_tx_data_c(dlc, port, ls, skb);
			break;
#endif  /* CONFIG_SNA_LLC */
	}
out:    return err;
}
