/* sna.h: Linux-SNA Network Operator Facility Headers.
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

#ifndef _SNA_COMMON_H
#define _SNA_COMMON_H

#include <linux/netdevice.h>
#include <linux/skbuff.h>

#include "sna_formats.h"
#include "sna_vectors.h"
#include "sna_ms_vectors.h"
#include "sna_cbs.h"
#include "sna_externs.h"
#include "sna_errors.h"

static inline sna_rh *sna_transport_header(const struct sk_buff *skb)
{
       return (sna_rh *)skb_transport_header(skb);
}

static inline sna_fid2 *sna_network_header(const struct sk_buff *skb)
{
       return (sna_fid2 *)skb_network_header(skb);
}

#define SNA_SKB_CB(skb) ((struct sna_skb_cb *)(skb)->cb)
struct sna_skb_cb {
	u_int32_t       bracket_id;
	u_int8_t        fmh;
	u_int8_t        hs_ps_type;
	u_int8_t        type;
	u_int8_t        reason;
};

extern char sna_sw_prd_version[];
extern char sna_sw_prd_release[];
extern char sna_sw_prd_modification[];
extern char sna_maintainer[];
extern char sna_product_id[];
extern char sna_product_name[];
extern char sna_vendor[];

extern u_int32_t sna_debug_level;

extern int hexdump(unsigned char *pkt_data, int pkt_len);
extern int sna_utok(void *uaddr, int ulen, void *kaddr);
extern int sna_ktou(void *kaddr, int klen, void *uaddr);

#define SNA_DEBUG 1		/* remove this define for performance builds. */
#ifndef SNA_DEBUG
#define sna_debug(level, format, arg...)
#else
#define sna_debug(level, format, arg...) \
	if(sna_debug_level >= level)  \
		printk(KERN_EMERG "%s:%s: " format, __FILE__, __FUNCTION__, ## arg)
#endif  /* SNA_DEBUG */

#define new(ptr, flag)	do { 				\
	(ptr) = kmalloc(sizeof(*(ptr)), flag);		\
	if (ptr) 					\
		memset(ptr, 0, sizeof(*(ptr))); 	\
} while (0)

#define new_s(ptr, size, flag)	do {			\
	(ptr) = kmalloc(size, flag);			\
	if (ptr)					\
		memset(ptr, 0, size);			\
} while (0)

/* Linux-SNA MU header types */
#define SNA_MU_BIND_RQ_SEND             0x01
#define SNA_MU_BIND_RSP_SEND            0x02
#define SNA_MU_UNBIND_RQ_SEND           0x03
#define SNA_MU_UNBIND_RQ_RCV            0x04
#define SNA_MU_UNBIND_RSP_SEND          0x05
#define SNA_MU_BIND_RQ_RCV              0x06
#define SNA_MU_BIND_RSP_RCV             0x07
#define SNA_MU_HS_TO_RM                 0x08
#define SNA_MU_RM_TO_PS                 0x09
#define SNA_MU_HS_TO_PS                 0x0A
#define SNA_MU_PS_TO_HS                 0x0B
#define SNA_MU_HS_TO_PC                 0x0C

/* Linux-SNA Deactivate session identifiers */
#define SNA_DEACT_PENDING               0x01
#define SNA_DEACT_ACTIVE                0x02

/* Linux-SNA Sync level indicators */
#define SNA_SYNC_NONE                   0x01
#define SNA_SYNC_CONFIRM                0x02
#define SNA_SYNC_SYNCPT                 0x03
#define SNA_SYNC_BACKOUT                0x04

/* Linux-SNA HS type indicators */
#define SNA_HS_PRI                      0x01
#define SNA_HS_SEC                      0x02

/* Linux-SNA Security indicators */
#define SNA_SECURITY_NONE               0x01
#define SNA_SECURITY_SAME               0x02
#define SNA_SECURITY_PGM                0x03

/* Linux-SNA sna_send_deactivate_pending() indicators */
#define SNA_PENDING                     0x01
#define SNA_ACTIVE                      0x02
#define SNA_ABNORMAL                    0x03
#define SNA_ABNORMAL_RETRY              0x04
#define SNA_ABNORMAL_NO_RETRY           0x05
#define SNA_CLEANUP                     0x06
#define SNA_NORMAL                      0x07

/* Linux-SNA Structure Identifiers */
#define SNA_REC_BID                     0x001
#define SNA_REC_BID_RSP                 0x002
#define SNA_REC_MU                      0x003
#define SNA_REC_MU_FMH5                 0x004
#define SNA_REC_MU_FMH12                0x005
#define SNA_REC_FREE_SESSION            0x006
#define SNA_REC_RTR_RQ                  0x007
#define SNA_REC_RTR_RSP                 0x008
#define SNA_REC_BIS_REPLY               0x009
#define SNA_REC_ALLOCATE_RCB            0x00A
#define SNA_REC_GET_SESSION             0x00B
#define SNA_REC_DEALLOCATE_RCB          0x00C
#define SNA_REC_TERMINATE_PS            0x00D
#define SNA_REC_CHANGE_SESSIONS         0x00E
#define SNA_REC_RM_ACTIVATE_SESSION     0x00F
#define SNA_REC_RM_DEACTIVATE_SESSION   0x010
#define SNA_REC_DEACTIVATE_CONV_GROUP   0x011
#define SNA_REC_UNBIND_PROTOCOL_ERROR   0x012
#define SNA_REC_ABEND_NOTIFICATION      0x013
#define SNA_REC_ACTIVATE_SESSION_RSP    0x014
#define SNA_REC_SESSION_ACTIVATED       0x015
#define SNA_REC_SESSION_DEACTIVATED     0x016
#define SNA_REC_BIS_RQ                  0x017
#define SNA_REC_ACTIVATE_SESSION        0x018
#define SNA_REC_DEACTIVATE_SESSION      0x019
#define SNA_REC_INIT_HS_RSP             0x01B
#define SNA_REC_ABORT_HS                0x01C
#define SNA_REC_INIT_SIGNAL_NEG_RSP     0x01D
#define SNA_REC_CINIT_SIGNAL            0x01E
#define SNA_REC_SESSION_ROUTE_INOP      0x01F
#define SNA_REC_LFSID_IN_USE            0x020
#define SNA_REC_BRACKET_FREED           0x021
#define SNA_REC_BID_WITHOUT_ATTACH      0x022
#define SNA_REC_HS_PS_CONNECTED         0x023
#define SNA_REC_RM_HS_CONNECTED         0x024
#define SNA_REC_SECURITY_REPLY_2        0x025
#define SNA_REC_YIELD_SESSION           0x026
#define SNA_REC_INIT_HS                 0x027
#define SNA_REC_RCB_ALLOCATED           0x028
#define SNA_REC_SESSION_ALLOCATED       0x029
#define SNA_REC_RM_SESSION_ALLOCATED    0x02A
#define SNA_REC_CONV_FAIL               0x02B
#define SNA_REC_RM_SESSION_ACTIVATED    0x02C
#define SNA_REC_START_TP                0x02D
#define SNA_REC_FREE_LFSID              0x02E
#define SNA_REC_PC_HS_DISCONNECT        0x02F
#define SNA_REC_ASSIGN_LFSID            0x030
#define SNA_REC_LFSID_IN_USE_RSP        0x031
#define SNA_REC_INIT_SIGNAL             0x032
#define SNA_REC_SESSEND_SIGNAL          0x033
#define SNA_REC_SESSST_SIGNAL           0x034
#define SNA_REC_ASSIGN_PCID             0x035
#define SNA_REC_RCB_DEALLOCATED         0x036
#define SNA_REC_SEND_RTR                0x037
#define SNA_REC_RM_TIMER_POP            0x038

#endif	/* _SNA_COMMON_H */
