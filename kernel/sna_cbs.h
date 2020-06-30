/* sna_cbs.h: Linux-SNA Control blocks and layer specific declarations.
 * - Kept in one place to ease maintance headaches
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

#ifndef __NET_SNA_CBS_H
#define __NET_SNA_CBS_H

#ifdef __KERNEL__

#include <linux/netdevice.h>

/* CREATE.
 */

struct sna_hs_create_parms {
	__u8 lu_id;
	__u8 hs_id;
};

struct sna_ps_create_parms {
	struct sna_lucb *lucb_list_ptr;
	__u8 lu_id;

	struct sna_tcb *tcb_list_ptr;
	__u8 tcb_id;

	struct sna_rcb *rcb_list_ptr;
};

/* cpic.
 */

typedef enum {
	CM_RESET = 1,
	CM_INIT,
	CM_SEND,
	CM_RCV,
	CM_SEND_PEND,
	CM_CONFIRM_SEND,
	CM_CONFIRM_DEALLOC,
	CM_INIT_INCOMING,
	CM_SEND_RCV,
	CM_SEND_ONLY,
	CM_RCV_ONLY,
	CM_DEFER_RCV,
	CM_DEFER_DEALLOC,
	CM_PREPARED,
	CM_SYNC_POINT_SEND,
	CM_SYNC_POINT_DEALLOC
} cpic_state;

struct sna_cpic_side_info {
	struct list_head 	list;

	u_int32_t		index;

	sna_netid              	netid;
	sna_netid              	netid_plu;

	unsigned char           sym_dest_name[SNA_RESOURCE_NAME_LEN];

	unsigned char           mode_name[SNA_RESOURCE_NAME_LEN];
	u_int8_t		mode_name_length;

	unsigned char           tp_name[65];
	u_int8_t		tp_name_length;

	unsigned char           service_tp;
	unsigned short          security_level;
	unsigned char           username[11];
	unsigned char           password[11];
};

/* nof.
 */

struct sna_nof_cb {
	struct list_head list;

	sna_netid       netid;
	sna_nodeid      nodeid;

	u_int32_t	flags;

	unsigned char   type;           /* Node type. */
	unsigned short  lu_seg;         /* Is LU segmenting supported. */
	unsigned short  bind_seg;       /* Is Bind segmenting supported. */
	unsigned long   max_lus;        /* Max number of LU sessions,
					 * 0 = Unlimited.
					 *                                          */

	/* Data below not currently used. */
	unsigned long   netid_registered;
	unsigned long   ls_supp_type;
	unsigned long   resource_registration;
	unsigned long   segment_generation_lvl;
	unsigned long   mode_to_cos_mapping;
	unsigned long   ms_node_type;
	unsigned long   mj_vector_file;
	unsigned long   ms_log_file;
	unsigned long   peer_resource_registration;
	unsigned long   network_node_type;
	unsigned long   directory_type_supp;
	unsigned long   rs_tree_update_type;
	unsigned long   tdm_node_name;
	unsigned long   cosdm_node_name;
	unsigned long   max_rs_cache_trees;
	unsigned long   max_oos_tdm_updates;
	unsigned long   resource_service_search;
	unsigned long   general_odai_usage_supp;
};

/* ASM.
 */

struct sna_lfsid {
	u_int8_t	active;
	u_int16_t	sm_index;	/* lulu_cb->index. */

	/* lfsid -> odai/daf/oaf mapping. */
	u_int8_t 	odai;		/* pri=0, sec=1. */
	u_int8_t	sidh;
	u_int8_t	sidl;
};

struct sna_lfsid_block {
	struct list_head list;

	struct sna_lfsid l[256];
};

/* one for each PC */
struct sna_asm_cb {
	struct list_head 	list;

	u_int32_t		index;	/* sna_pc_cb->index */

	u_int8_t		odai;
	u_int8_t		intranode;
	u_int16_t		rx_max_btu;
	u_int8_t		tx_adaptive_bind_pacing;
	u_int8_t		rx_adaptive_bind_pacing;
	u_int8_t		dlus_lu_reg;
	u_int8_t		adptv_bind_pacing;
	u_int8_t		gen_odai_usage_opt_set;

	/* list of 256 unique addresses */
	u_int8_t		blk_count;
	struct list_head 	blk_list;
};

/* CS.
 */

typedef enum {
	XID_STATE_RESET = 1,
	XID_STATE_S_PN,
	XID_STATE_S_NEG,
	XID_STATE_S_PRI,
	XID_STATE_S_SEC,
	XID_STATE_R_PRI_S_SEC,
	XID_STATE_S_PRI_R_SEC,
	XID_STATE_ACTIVE
} xid_state_enum;

typedef enum {
	XID_IN_BEGIN = 1,
	XID_IN_NULL,
	XID_IN_PN,
	XID_IN_NEG,
	XID_IN_PRI,
	XID_IN_SEC,
	XID_IN_SETMODE,
	XID_IN_ACTIVE,
	XID_IN_RESET
} xid_input_enum;

typedef enum {
	XID_OUT_NULL = 1,
	XID_OUT_PN,
	XID_OUT_NEG,
	XID_OUT_NEG_ID,
	XID_OUT_PRI,
	XID_OUT_SEC,
	XID_OUT_PRI_R,
	XID_OUT_SEC_R,
	XID_OUT_RANDOM,
	XID_OUT_ERR_OPT,
	XID_OUT_Z1,
	XID_OUT_Z2,
	XID_OUT_RESEND
} xid_output_enum;

typedef enum {
	XID_DIR_INVALID = 0,
	XID_DIR_OUTBOUND,
	XID_DIR_INBOUND
} xid_direction_enum;

struct sna_xid_info {
	struct sk_buff  *last_tx_xid;
	struct sk_buff  *last_rx_xid;

//        xid_state               xid_status;
	struct timer_list       xid_timer;
	__u32                   xid_idle_limit;
	__u32                   xid_retry_interval;
	__u32                   xid_retry_limit;
	__u32			xid_retries;

	__u8    xid_initiator;		/* 0x00 = Local / 0x01 = Remote */
	__u8	xid_direction;		/* 0x00 = cmd / 0x01 = rsp */
	__u32   xid_count;              /* number of xids exchanged */

	__u16   i_tx_window;
	__u16   i_rx_window;
	__u16   i_mtu;                  /* max btu size */

	__u8    tg_type;
	__u8    tg_id;

	/* detailed xid information */
	__u8	init_self;
	__u8	tx_bind_wo_init_self;
	__u8	tx_bind_piu;
	__u8	rx_bind_piu;

	__u8	adaptive_tx_bind;
	__u8	adaptive_rx_bind;

	/* Junk below but keeping around for a while */
	__u32   adj_node_id;
	char    real_adj_cp_name[8];
	__u32   local_odai_value;
	__u32   sscp_med_sessions;
	__u32   adj_node_bind_reassembly;
	__u32   local_cpcp_sessions_supp_status;
	__u32   adj_node_cpcp_sessions_supp_status;
	__u32   curr_cpcp_sessions_supp_status;
	__u32   ss_cpcp_sessions_supp_override;
	__u32   partner_node_type;
	__u32   tg_sec_init_nonact_xchg;
	__u32   restart_nonact_xid3_xchg;
};

typedef enum {
	BIND_RESET = 1,
	BIND_S_BIND,
	BIND_S_BIND_W_AF,
	BIND_S_PR_FMD,
	BIND_S_PR_FMD_W_AF,
	BIND_S_PR_BIND,
	BIND_S_PR_BIND_W_AF,
	BIND_ACTIVE
} bind_state;

struct sna_session_initiation_info {
	char    adj_cp_name[8];
	__u32   real_node_tgs;
	__u32   virtual_node_tgs;
	char    virtual_node_cp_name[8];
};

struct sna_xid_error_info {
	__u32   sense;
	__u32   *procedure_name;
	__u32   byte_offset;
	__u32   bit_offset;
	__u32   send_xid;
};

struct sna_adj_node_cb {
	struct sna_adj_node_cb *next;
	struct sna_adj_node_cb *prev;

	char    real_adj_cp_name[8];
	__u32   last_adj_cp_contacted;
	__u32   parallel_tgs;
	__u32   tg_numbers;

	struct sna_cs_process_data *cs_instance;
};

/*
 * Link Station data structures.
 */

#define SNA_LS_TYPE_DEFINED             0x01
#define SNA_LS_TYPE_DYNAMIC             0x02
#define SNA_LS_TYPE_TEMP                0x04

#define SNA_TEST_RETRY_LIMIT	3 * HZ
#define SNA_XID_RETRY_LIMIT    	3 * HZ
#define SNA_XID_IDLE_LIMIT	5 * HZ

typedef enum {
	SNA_LS_XID_INIT_NULL = 0,
	SNA_LS_XID_INIT_PN,
	SNA_LS_XID_INIT_NP
} sna_ls_xid_init_method;

typedef enum {
	SNA_XID_XCHG_STATE_NONE = 0,
	SNA_XID_XCHG_STATE_NEG,
	SNA_XID_XCHG_STATE_PN,
	SNA_XID_XCHG_STATE_NONACTIVE
} sna_xid_xchg_state;

typedef enum {
	SNA_TG_STATE_RESET = 0,
	SNA_TG_STATE_DONE_LOCAL,
	SNA_TG_STATE_DONE_PARTNER
} sna_tg_state_enum;

typedef enum {
	SNA_TG_INPUT_BEGIN = 0,
	SNA_TG_INPUT_NULL,
	SNA_TG_INPUT_ADJ_LOC,
	SNA_TG_INPUT_SETMODE,
	SNA_TG_INPUT_RESET
} sna_tg_input_enum;

typedef enum {
	SNA_TG_OUTPUT_ADJ = 0,
	SNA_TG_OUTPUT_PIK,
	SNA_TG_OUTPUT_LOC,
	SNA_TG_OUTPUT_OPT
} sna_tg_output_enum;

struct sna_als_cb {
	/* negotiated link station role and tg number. */
	u_int8_t                effective_role;
	u_int8_t                effective_tg;
	u_int8_t                tg_state;
	u_int8_t                tg_input;
};

/**
 * sna_ls_cb - link station control block.
 * this structure is used for both local and adjacent link stations.
 */
struct sna_ls_cb {
	struct list_head 	list;

	char            	use_name[SNA_RESOURCE_NAME_LEN];

	u_int32_t 		index;		/* used as internal link number. */
	u_int32_t		flags;
	u_int32_t		state;

	u_int32_t		cs_index;	/* cs instance for this link station. */
	u_int32_t		dlc_index;	/* dlc used by this link station. */
	u_int32_t		port_index;	/* port used by this link station. */
	u_int32_t		pc_index;

	/* run-time activation timer. */
	u_int32_t		retries;
	struct timer_list 	retry;

	/* connect out information. (link test). */
	u_int32_t       	co_status;

	/* xid information. */
	u_int32_t       	xid_state;
	u_int32_t       	xid_direction;
	u_int32_t		xid_count;
	u_int32_t		xid_input;
	struct sk_buff  	*xid_last_tx;
	u_int32_t		xid_last_tx_direction;
	struct sk_buff  	*xid_last_rx;
	u_int32_t		xid_last_rx_direction;

	/* connection establish related data. */
	struct work_struct  	connect;
	wait_queue_head_t 	sleep;
#ifdef	CONFIG_SNA_LLC
	struct sock		*llc_sk;
#endif

	/* negotiated link station role and tg number. */
	u_int8_t		effective_role;
	u_int8_t		effective_tg;
	u_int8_t		tg_state;
	u_int8_t		tg_input;

	/* other values set from xid negotation. */
	u_int8_t		odai;
	u_int16_t               tx_max_btu;
	u_int16_t               rx_max_btu;
	u_int16_t		tx_window;
	u_int16_t		rx_window;
	u_int16_t		bind_pacing;

	/* destination link station. */
	sna_netid 		plu_name;
	sna_nodeid		plu_node_id;
	u_int8_t		plu_mac_addr[IFHWADDRLEN];
	u_int8_t		plu_port;

	/* this is really temporary until we support more than
	 * one adjacent node, then we will have to use something
	 * else.
	 */
	struct sna_als_cb 	als;

	/* user defined capabilities of this link station,
	 * not all are currently used.
	 */
	int			role;
	int			direction;
	int			xid_init_method;
	int 			byteswap;
	int 			retry_on_fail;
	int 			retry_times;
	int 			autoact;
	int 			autodeact;
	int 			tg_number;
	int 			cost_per_byte;
	int 			cost_per_connect_time;
	int 			effective_capacity;
	int 			propagation_delay;
	int 			security;
	int 			user1;
	int 			user2;
	int 			user3;

	/* detail xid information we don't gather dynamicly on xid_init. */
	u_int8_t		xid_format;
	u_int8_t		xid_type;
	u_int8_t		xid_init_self;
	u_int8_t		xid_standalone_bind;
	u_int8_t		xid_whole_bind;
	u_int8_t		xid_whole_bind_piu_req;
	u_int8_t		xid_actpu_suppression;
	u_int8_t		xid_network_node;
	u_int8_t		xid_cp_services;
	u_int8_t		xid_sec_nonactive_xchg;
	u_int8_t		xid_cp_name_chg;
	u_int8_t		xid_tx_adaptive_bind_pacing;
	u_int8_t		xid_rx_adaptive_bind_pacing;
	u_int8_t		xid_quiesce_tg;
	u_int8_t		xid_pu_cap_sup;
	u_int8_t		xid_appn_pbn;
	u_int8_t		xid_parallel_tg_sup;
	u_int8_t		xid_ls_txrx_cap;
	u_int8_t		xid_gen_odai_usage_opt_set;


	/* we need to clean up everything below here soon. */
	sna_netid 		netid;

	__u32                   adj_node_type;
	char                    adj_node_id[8];
	__u32                   adj_node_session;

	__u32                   bind_pacing_cnt;

	struct sna_session_initiation_info *session_initiation_info;
	struct sna_activate_route *activate_route_list;
};

/*
 * Port data structures.
 */

typedef enum {
	SNA_PORT_TYPE_INVALID = 0,
	SNA_PORT_TYPE_LOOPBACK,
	SNA_PORT_TYPE_LLC
} sna_port_type;

struct sna_port_cb {
	struct list_head 	list;

	char            	use_name[SNA_RESOURCE_NAME_LEN];

	u_int16_t       	type;
	u_int32_t       	index;
	u_int32_t		cs_index;
	u_int32_t		dlc_index;
	u_int32_t       	flags;

	struct list_head        ls_list;
	u_int32_t               ls_qlen;
	u_int32_t 		ls_system_index;

	char 			saddr[12];

	u_int32_t   		btu;
	u_int32_t		mia;
	u_int32_t		moa;

#ifdef CONFIG_SNA_LLC
	struct llc_sap 		*llc_dl;
#endif
#ifdef CONFIG_SNA_ATM
#endif
#ifdef CONFIG_SNA_CHANNEL
#endif
#ifdef CONFIG_SNA_SDLC
#endif
#ifdef CONFIG_SNA_HDLC
#endif

	/* need to cleanup the fields below. */
	sna_netid               netid;

	unsigned long   max_adjacent_ls;
	unsigned long   real_adjacent_ls;

	unsigned long   real_inbound_activation;
	unsigned long   inbound_activation_cnt;

	unsigned long   real_outbound_activation;
	unsigned long   outbound_activation_cnt;

	unsigned long   link_station_txrx;
	unsigned long   max_nonack_xid;

	unsigned long   xid_exchange_type;
};

/*
 * DLC data structures.
 */

#ifdef CONFIG_SNA_LLC
struct sna_dlc_llc_addr {
	sa_family_t     arphrd;         /* ARPHRD_ETHER */
	u_int8_t        dsap;
	u_int8_t        ssap;
	u_int8_t        dmac[IFHWADDRLEN];
	u_int8_t        smac[IFHWADDRLEN];
};
#endif  /* CONFIG_SNA_LLC */

/**
 * sna_dlc_cb - abstraction from the linux device layer.
 */
struct sna_dlc_cb {
	struct list_head list;

	struct net_device 	*dev;
	u_int16_t		type;	/* ARPHRD */
	u_int32_t 		flags;
	u_int32_t		index;
};

struct sna_cs_cb {
	struct list_head 	list;

	u_int32_t		index;
	u_int32_t		flags;

	sna_netid       	netid;
	sna_nodeid		nodeid;

	unsigned char		node_type;
	unsigned char		intranode_pc_id[8];


/* Not used below */
	struct sna_adj_node_cb  *adj_node_cb;
	struct sna_fq_netid     *netid_cp;
	unsigned long   bind_reassembly;
	unsigned long   adaptive_bind_pacing;
};

/* DFC.
 */

typedef enum {
	SNA_SM_S_STATE_ACTIVE = 0,
	SNA_SM_S_STATE_INIT_SENT,
	SNA_SM_S_STATE_BIND_SENT
} sna_sm_session_state;

typedef enum {
	SNA_SM_S_TYPE_FSP = 0,
	SNA_SM_S_TYPE_BIDDER
} sna_sm_session_type;

typedef enum {
	SNA_SM_FSM_STATE_RES = 0,
	SNA_SM_FSM_STATE_PND_CIN,
	SNA_SM_FSM_STATE_PND_BIN_RSP,
	SNA_SM_FSM_STATE_PND_INI_HS_RSP_PLU,
	SNA_SM_FSM_STATE_PND_INI_HS_RSP_SLU,
	SNA_SM_FSM_STATE_ACT
} sna_sm_fsm_status_state;

typedef enum {
	SNA_SM_FSM_INPUT_ACTIVATE_SESSION = 0,
	SNA_SM_FSM_INPUT_INIT_SIGNAL_NEG_RSP,
	SNA_SM_FSM_INPUT_CINIT_SIGNAL_OK,
	SNA_SM_FSM_INPUT_CINIT_SIGNAL_NG,
	SNA_SM_FSM_INPUT_POS_BIND_RSP_OK,
	SNA_SM_FSM_INPUT_POS_BIND_RSP_NG,
	SNA_SM_FSM_INPUT_NEG_BIND_RSP,
	SNA_SM_FSM_INPUT_BIND,
	SNA_SM_FSM_INPUT_POS_INIT_HS_RSP,
	SNA_SM_FSM_INPUT_NEG_INIT_HS_RSP,
	SNA_SM_FSM_INPUT_DEACTIVATE_SESSION,
	SNA_SM_FSM_INPUT_UNBIND,
	SNA_SM_FSM_INPUT_SESSION_ROUTE_INOP,
	SNA_SM_FSM_INPUT_ABORT_HS,
	SNA_SM_FSM_INPUT_RM_ABEND,
	SNA_SM_FSM_INPUT_HS_ABEND
} sna_sm_fsm_status_input;

typedef enum {
	SNA_SM_FSM_OUTPUT_A = 0,
	SNA_SM_FSM_OUTPUT_B,
	SNA_SM_FSM_OUTPUT_C,
	SNA_SM_FSM_OUTPUT_D,
	SNA_SM_FSM_OUTPUT_E,
	SNA_SM_FSM_OUTPUT_F,
	SNA_SM_FSM_OUTPUT_G,
	SNA_SM_FSM_OUTPUT_H,
	SNA_SM_FSM_OUTPUT_I,
	SNA_SM_FSM_OUTPUT_J,
	SNA_SM_FSM_OUTPUT_K,
	SNA_SM_FSM_OUTPUT_L,
	SNA_SM_FSM_OUTPUT_M,
	SNA_SM_FSM_OUTPUT_N,
	SNA_SM_FSM_OUTPUT_P,
	SNA_SM_FSM_OUTPUT_Q,
	SNA_SM_FSM_OUTPUT_R,
	SNA_SM_FSM_OUTPUT_S,
	SNA_SM_FSM_OUTPUT_T,
	SNA_SM_FSM_OUTPUT_U,
	SNA_SM_FSM_OUTPUT_V,
	SNA_SM_FSM_OUTPUT_X
} sna_sm_fsm_status_output;

struct sna_lulu_cb {
	struct list_head 	list;

	sna_netid		local_name;
	sna_netid		remote_name;

	u_int32_t		index;
	u_int32_t		mode_index;
	u_int32_t		remote_lu_index;
	u_int32_t		pc_index;
	u_int32_t		hs_index;

	u_int32_t		tp_index;		/* this is our correlator. */

	int                     type;                   /* fsp or bidder. */
	struct sna_lfsid        lfsid;
	sna_fqpcid              fqpcid;

	/* information for fsm_status() */
	u_int32_t		input;
	u_int32_t		state;

	/* not used below.. cleanup soon. */
	sna_netid netid;
	sna_netid plu_netid;
	unsigned char mode_name[SNA_RESOURCE_NAME_LEN];


/*	Not used anymore.

	__u8    bracket_id;

	struct sna_ct   ct_rcv;
	struct sna_ct   ct_send;

	sna_snf sqn_send_cnt;
	sna_snf phs_bb_register;
	sna_snf shs_bb_register;
	sna_snf current_bracket_sqn;

	__u8    rqd_required_on_ceb;
	__u8    deallocate_abend;

	__u16   normal_flow_rq_cnt;

	__u8    sig_received;
	sna_snf sig_snf;

	__u8    betc;
	__u8    send_error_rsp_state;
	__u8    bb_rsp_state;
	__u32   bb_rsp_sense;

	__u8    rtr_rsp_state;
	__u32   rtr_rsp_sense;

	__u8    sig_rq_outstanding;
	__u8    alternate_code;
	__u8    session_just_started;
	struct sna_mu   *saved_mu_ptr;
*/
};

#ifdef NOT
#define SNA_DFC_POS_RSP(X)      (SNA_DFC_RQD(X))
#define SNA_DFC_NEG_RSP(X)      (SNA_DFC_RQE(X))

#define SNA_DFC_TRUE    0x1
#define SNA_DFC_FALSE   0x0

#define SNA_DFC_YES     0x1
#define SNA_DFC_NO      0x0

#define SNA_DFC_NEG     0x0
#define SNA_DFC_POS     0x1

#define SNA_DFC_SIG_CURRENT     0x0
#define SNA_DFC_SIG_STRAY       0x1
#define SNA_DFC_SIG_FUTURE      0x2

#define SNA_DFC_RESET           0x0
#define SNA_DFC_NEG_OWED        0x1
#define SNA_DFC_POS_OWED        0x2

/* Finite State Machine definitions and structures. */
#define SNA_DFC_FSM_CURRENT     0x0     /* Current FSM state. */

#define SNA_DFC_FSM_R           0x0     /* MU is being received. */
#define SNA_DFC_FSM_S           0x1     /* MU is being sent. */

#define SNA_DFC_FSM_NO_CHAIN    0x0
#define SNA_DFC_FSM_BEGIN_CHAIN 0x1
#define SNA_DFC_FSM_END_CHAIN   0x2

#define SNA_DFC_FSM_BETB        0x01
#define SNA_DFC_FSM_BETC        0x01
#define SNA_DFC_FSM_INB         0x02
#define SNA_DFC_FSM_INC         0x02
#define SNA_DFC_FSM_NEG         0x03
#define SNA_DFC_FSM_RSP         0x03
#define SNA_DFC_FSM_SENT        0x03
#define SNA_DFC_FSM_PEND        0x04
#define SNA_DFC_FSM_SEND        0x05

#define SNA_DFC_FSM_PEND_SEND_REPLY     0x06
#define SNA_DFC_FSM_PEND_RCV_REPLY      0x07
#define SNA_DFC_FSM_PEND_REPLY          0x08

#define SNA_DFC_FSM_BB                  0x0
#define SNA_DFC_FSM_RQD                 0x0
#define SNA_DFC_FSM_RQE                 0x1
#define SNA_DFC_FSM_REPLY               0x2
#define SNA_DFC_FSM_BIS                 0x3
#define SNA_DFC_FSM_RTR                 0x4
#define SNA_DFC_FSM_FMH5                0x5
#define SNA_DFC_FSM_FMH12               0x6
#define SNA_DFC_FSM_LUSTAT              0x7
#define SNA_DFC_FSM_NOT_BID_REPLY       0x8
#define SNA_DFC_FSM_CEB_UNCOND          0x9
#endif

/* DS.
 */

struct sna_ds_pinfo {
	struct list_head list;

	sna_netid netid;

	unsigned long   bind_reassembly;
	unsigned long   adaptive_bind_pacing;
};

/* HS.
 */

#define SNA_LOCAL_CRV           0x00
#define SNA_LOCAL_NO_CRV        0x01
#define SNA_LOCAL_BIS           0x10
#define SNA_LOCAL_NO_BIS        0x11
#define SNA_LOCAL_LUSTAT        0x20
#define SNA_LOCAL_NO_LUSTAT     0x21
#define SNA_LOCAL_RTR           0x30
#define SNA_LOCAL_NO_RTR        0x31
#define SNA_LOCAL_SIG           0x40
#define SNA_LOCAL_NO_SIG        0x41
#define SNA_LOCAL_OTHER         0x50
#define SNA_LOCAL_NO_OTHER      0x51

#define SNA_HS_SEND             0x0
#define SNA_HS_RECEIVE          0x1

typedef enum {
	SNA_DFC_FSM_BSM_FMP19_STATE_BETB = 1,
	SNA_DFC_FSM_BSM_FMP19_STATE_INB
} sna_dfc_fsm_bsm_fmp19_state;

/* BETC: between chains.
 * INC: in the middle of a chain.
 * NEG_RSP_SENT: in the middle of a chain and a negative response has been sent.
 * PEND_RSP: has received (EC,RQD) and is waiting for the response to be sent.
 * PEND_SEND_REPLY: has received (EC,RQE,CD) and is waiting for the reply
 *                  or negative response to be sent.
 */

typedef enum {
	SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_BETC = 1,
	SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_INC,
	SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_NEG_RSP_SENT,
	SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_PEND_RSP,
	SNA_DFC_FSM_CHAIN_RCV_FMP19_STATE_PEND_SEND_REPLY
} sna_dfc_fsm_chain_rcv_fmp19_state;

typedef enum {
	SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_BETC = 1,
	SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_INC,
	SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_NET_RSP_RCVD,
	SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_PEND_RSP,
	SNA_DFC_FSM_CHAIN_SEND_FMP19_STATE_PEND_RCV_REPLY
} sna_dfc_fsm_chain_send_fmp19_state;

/* see sna_formats for details on this. */
#define SNA_DFC_RQD1(X)         (X->dr1i == 1 && X->dr2i == 0 && X->rti == 0)

#define SNA_DFC_RQE1(X)         (X->dr1i == 1 && X->dr2i == 0 && X->rti == 1)
#define SNA_DFC_SET_RQE1(x)	do {		\
	x->dr1i = 1;				\
	x->dr2i = 0;				\
	x->rti  = 1;				\
} while (0)

#define SNA_DFC_RQD2(X)         (X->dr1i == 0 && X->dr2i == 1 && X->rti == 0)
#define SNA_DFC_RQE2(X)         (X->dr1i == 0 && X->dr2i == 1 && X->rti == 1)

#define SNA_DFC_RQD3(X)         (X->dr1i == 1 && X->dr2i == 1 && X->rti == 0)
#define SNA_DFC_SET_RQD3(x)	do {		\
	x->dr1i = 1;				\
	x->dr2i = 1;				\
	x->rti  = 0;				\
} while (0)

#define SNA_DFC_RQE3(X)         (X->dr1i == 1 && X->dr2i == 1 && X->rti == 1)
#define SNA_DFC_SET_RQE3(x)	do {		\
	x->dr1i = 1;				\
	x->dr2i = 1;				\
	x->rti  = 1;				\
} while (0)

#define SNA_DFC_FSM_RQD(x)      (SNA_DFC_RQD1(x) || SNA_DFC_RQD2(x) || SNA_DFC_RQD3(x))
#define SNA_DFC_FSM_RQE(x)	(SNA_DFC_RQE1(x) || SNA_DFC_RQE2(x) || SNA_DFC_RQE3(x))

typedef enum {
	SNA_DFC_FSM_QRI_CHAIN_RCV_FMP19_STATE_RESET = 1,
	SNA_DFC_FSM_QRI_CHAIN_RCV_FMP19_STATE_INC_QR,
	SNA_DFC_FSM_QRI_CHAIN_RCV_FMP19_STATE_INC_NOT_QR
} sna_dfc_fsm_qri_chain_rcv_fmp19_state;

typedef enum {
	SNA_DFC_FSM_RCV_PURGE_FMP19_STATE_RESET = 1,
	SNA_DFC_FSM_RCV_PURGE_FMP19_STATE_PURGE
} sna_dfc_fsm_rcv_purge_fmp19_state;

typedef enum {
	SNA_DFC_FSM_CHAIN_IND_NONE = 0,
	SNA_DFC_FSM_CHAIN_IND_BEGIN,
	SNA_DFC_FSM_CHAIN_IND_END
} sna_dfc_fsm_chain_ind;

typedef enum {
	SNA_DFC_FSM_BSM_IND_NONE = 0,
	SNA_DFC_FSM_BSM_IND_INB,
	SNA_DFC_FSM_BSM_IND_BETB
} sna_dfc_fsm_bsm_ind;

typedef enum {
	SNA_HS_TYPE_FSP = 0,
	SNA_HS_TYPE_BIDDER
} sna_hs_type;

typedef enum {
	SNA_HS_PACING_TYPE_NONE = 0,
	SNA_HS_PACING_TYPE_FIXED,
	SNA_HS_PACING_TYPE_ADAPTIVE
} sna_hs_pacing_type;

typedef enum {
	SNA_HS_RESERVE_NO = 0,
	SNA_HS_RESERVE_MORE,
	SNA_HS_RESERVE_ALL
} sna_hs_reserve_type;

typedef enum {
	SNA_DFC_DIR_OUTBOUND = 0,
	SNA_DFC_DIR_INBOUND
} sna_dfc_direction;

typedef enum {
	SNA_DFC_RSP_STATE_RESET = 0,
	SNA_DFC_RSP_STATE_POS_OWED,
	SNA_DFC_RSP_STATE_NEG_OWED
} sna_dfc_rsp_state;

typedef enum {
	SNA_DFC_PS_REC_MU = 0,
	SNA_DFC_PS_REC_CONFIRMED,
	SNA_DFC_PS_REC_REQ_TO_SEND,
	SNA_DFC_PS_REC_RSP_TO_REQ_TO_SEND,
	SNA_DFC_PS_REC_RECEIVE_ERROR
} sna_dfc_ps_record_type;

#define SNA_HS_SEGMENT_NONE		0
#define SNA_HS_SEGMENT_FIRST_IN		1
#define SNA_HS_SEGMENT_LAST_IN		2
#define SNA_HS_SEGMENT_COMPLETE		4

#define	SNA_CT_RQ_CODE_CRV	SNA_RU_RC_CRV
#define	SNA_CT_RQ_CODE_BIS	SNA_RU_RC_BIS
#define	SNA_CT_RQ_CODE_LUSTAT	SNA_RU_RC_LUSTAT
#define	SNA_CT_RQ_CODE_RTR	SNA_RU_RC_RTR
#define	SNA_CT_RQ_CODE_SIG	SNA_RU_RC_SIG
#define	SNA_CT_RQ_CODE_OTHER	0

struct sna_c_table {
	u_int8_t	entry_present;
	u_int16_t	snf;
	u_int8_t	snf_who;	/* fsp or bidder. */
	u_int32_t	neg_rsp_sense;
	u_int8_t	rq_code;
	sna_rh		rh;
};

struct sna_pacing {
	u_int8_t	type;
	u_int8_t	rpc;
	u_int8_t 	nws;
};

struct sna_bracket {
	u_int8_t	who;	/* fsp or bidder. */
	u_int16_t	num;
};

struct sna_hs_cb {
	struct list_head	list;

	u_int32_t		index;
	u_int32_t		pc_index;
	u_int32_t		lulu_index;
	struct sna_lfsid	lfsid;

	int			type;		/* fsp or bidder. */

	struct sk_buff		*rx_bind;

	u_int16_t		snf;

	/* fields defined/used by tc. */
	u_int16_t		max_rx_ru_size;
	u_int16_t		sqn_rcv_cnt;
	u_int8_t		rlwi;
	u_int8_t        	first_ws;
	u_int8_t        	unsolicited_ipm_outstanding;
	u_int8_t        	adjust_ipm_ack_outstanding;
	u_int8_t        	unsolicited_nws;
	u_int8_t		reserve_flag;
	u_int8_t		segmenting;
	u_int8_t		crypto;
	struct sna_pacing	rx_pacing;
	struct sna_pacing	tx_pacing;

	struct sk_buff_head	write_queue;	/* pacing queue. */

	struct sk_buff_head	segments;	/* list of rx'd segments. */
	u_int16_t		segment_num;	/* number of segments. */
	u_int16_t		segment_len;	/* total len of ru segments. */
	u_int8_t		segment_in;	/* state of segement list. */
	spinlock_t		segment_lock;

	/* fields defined/used by dfc. */
	u_int16_t		sqn_tx_cnt;
	struct sna_bracket	current_bracket;
	struct sna_bracket	phs_bb_register;
	struct sna_bracket	shs_bb_register;
	u_int8_t		rqd_required_on_ceb;	/* rq definate rsp. */
	u_int8_t		normal_flow_rq_cnt;
	u_int8_t		sig_received;
	u_int8_t		sig_snf_sqn;
	u_int8_t		betc;			/* between chains. */
	u_int8_t		ec;			/* end chain received. */
	u_int8_t		deallocate_abend;
	u_int8_t		send_error_rsp_state;
	u_int8_t		bb_rsp_state;
	u_int8_t		rtr_rsp_state;
	u_int8_t		sig_rq_outstanding;

	u_int32_t		fsm_bsm_fmp19_state;
	u_int32_t		fsm_chain_rcv_fmp19_state;
	u_int32_t		fsm_chain_send_fmp19_state;
	u_int32_t		fsm_qri_chain_rcv_fmp19_state;
	u_int32_t		fsm_rcv_purge_fmp19_state;

	u_int32_t		sense;
	u_int8_t		direction;	/* 0 = outbound, 1 = inbound */
	struct sna_c_table 	ct_rcv;
	struct sna_c_table 	ct_send;

	struct sk_buff		*send_mu;
	struct sk_buff		*last_mu;
};

/* sna control information which rides with each skb.
 */

typedef enum {
	SNA_CTRL_T_NOT_END_OF_DATA = 0,
	SNA_CTRL_T_CONFIRM,
	SNA_CTRL_T_PREPARE_TO_RCV_CONFIRM_SHORT,
	SNA_CTRL_T_PREPARE_TO_RCV_CONFIRM_LONG,
	SNA_CTRL_T_PREPARE_TO_RCV_CONFIRM,
	SNA_CTRL_T_PREPARE_TO_RCV_FLUSH,
	SNA_CTRL_T_DEALLOCATE_CONFIRM,
	SNA_CTRL_T_DEALLOCATE_FLUSH,
	SNA_CTRL_T_REC_MU,
	SNA_CTRL_T_REC_CONFIRMED,
	SNA_CTRL_T_REC_REQ_TO_SEND,
	SNA_CTRL_T_REC_RECEIVE_ERROR,
	SNA_CTRL_T_REC_ALLOCATE,
	SNA_CTRL_T_REC_DEALLOCATE_ABEND,
	SNA_CTRL_T_REC_FLUSH,
	SNA_CTRL_T_REC_SEND_DATA
} sna_ctrl_info_types;

typedef enum {
	SNA_CTRL_R_PROTOCOL_VIOLATION = 0
} sna_ctrl_info_reasons;

/* IPC.
 */

struct sna_hs_ps_connected {
	__u8    ps_id;
	__u8    bracket_id;
};

struct sna_abend_notification {
	unsigned char *abending_process;
	__u8 process_id;
	__u8 reason;
};

struct sna_abort_hs {
	__u8 hs_id;
	__u8 sense_code;
};

struct sna_activate_session_rsp {
	__u8 correlator;
	__u8 type;
	__u8 session_information;
	__u8 error_type;
};

struct sna_allocate_rcb {
	__u8 tcb_id;
	__u8 specific_conv_group;
	__u8 conversation_group_id;
	__u8 *lu_name;
	__u8 *mode_name;
	__u8 immediate_session;
	__u8 sync_level;
	__u8 security_select;
};

struct sna_bid {
	__u8 hs_id;
};

struct sna_bid_rsp {
	__u8 hs_id;
	__u8 rti;
	__u32 sense;
};

struct sna_bid_without_attach {
	/* Blame IBM */
};

struct sna_bis_reply {
	__u8 hs_id;
};

struct sna_bis_rq {
	__u8 hs_id;
};

struct sna_bracket_freed {
	__u8 bracket_id;
};

struct sna_cos {
	unsigned short	type;
	unsigned char	rsv:5,
			network_priority:1,
			tx_priority:2;
	unsigned char	len;
	unsigned char	cos_name[8];
};

struct sna_cos_tpf_vector {
	unsigned char	mode_name[SNA_RESOURCE_NAME_LEN];
	struct sna_cos	v;
};

/* Control vector 0x80 */
struct sna_tg_id {
	unsigned short	type;
	unsigned char	tg_number;
	unsigned char	pcp_len;
	unsigned char	*pcp_name;
	unsigned char	pcp_name_id_cn:1,
			more_cfg_info:1,
			hpr_support:1,
			tg_type:2,
			intersubnet_tg:1,
			rsv1:1,
			rtp_support:1;
	unsigned short	subarea_number;
};

/* Control vector 0x81 */
struct sna_tg_cn {
	unsigned short	type;
	unsigned char	tg_number;
	unsigned char	ptg_number;
	unsigned short	rsv1;
};

/* Control vector 0x82 */
struct sna_tg_dlc_signal {
	unsigned short	type;
	unsigned char	mac[MAX_ADDR_LEN];
	unsigned char	lsap;
};

/* Control vector 0x83 */
struct sna_tg_cp_name {
	unsigned char	type;
	unsigned char	len;
	unsigned char	*cp_name;
	unsigned char	intersubnet_tg;
};

/* Control vector 0x46 */
struct sna_tg_desc {
	unsigned short	type;
	struct sna_tg_id		id;
	struct sna_tg_dlc_signal	dlc;
};

/* Control vector 0x47 */
struct sna_tg_chars {
	unsigned short	type;
	unsigned long	rsn;
	unsigned char	status:1,
			garbage:1,
			quiescing:1,
			cpcp_session:2,
			rsv1:3;
	unsigned char	effective_capacity;
	unsigned char	rsv2[5];
	unsigned char	cost_per_connect;
	unsigned char	cost_per_byte;
	unsigned char	rsv3;
	unsigned char	security;
	unsigned char	propagation_delay;
	unsigned char	rsv4;
	unsigned char	user1;
	unsigned char	user2;
	unsigned char	user3;
};

struct sna_tg_ttl {
	unsigned short	type;
	unsigned char	days;
};

/* Control vector 0x48 */
struct sna_tg_trd {
	unsigned short		type;
	struct sna_tg_ttl	*ttl;
};

struct sna_tg_vector {
	struct sna_tg_vector 	*next;
	struct sna_tg_vector	*prev;

	struct sna_tg_desc      desc;		/* Cv 0x46 */
	struct sna_tg_chars     chars;		/* Cv 0x47 */
	struct sna_tg_trd	trd;
};

/* Control vector 0x44 */
struct sna_node_desc {
	unsigned short	type;
	unsigned char	cp_len;
	sna_netid cp_name;
	unsigned char	rsv_len;
	unsigned char	cn:1,
			rsv1:7;
};

/* Control vector 0x80 */
struct sna_node_type {
	unsigned short	type;
	unsigned long	rsn;
	unsigned char	route_resistance;
	unsigned char	congested:1,
			interoute_depelted:1,
			rsv1:1,
			garbage:1,
			rsv2:1,
			quiescing:1,
			rsv3:2;
	unsigned char	gateway:1,
			cds:1,
			interoute:1,
			rsv4:1,		/* Set to 1 */
			rsv5:2,
			rsv6:2;		/* Set to 11 */
	unsigned char	peripheral:1,
			interchange:1,
			extended:1,
			hpr:2,
			rsv7:3;
};

/* Control vector 0x45 */
struct sna_node_chars {
	unsigned short		type;
	struct sna_node_type	info;
};

struct sna_node_vector {
	struct sna_node_desc	desc;
	struct sna_node_chars	chars;
};

struct sna_tg_update {
	sna_netid		plu_name;

	u_int8_t		tg_number;
	u_int8_t		l_node_type;
	u_int8_t		r_node_type;
	u_int8_t		routing;

	u_int32_t		ls_index;
	u_int32_t		port_index;
	u_int32_t		dlc_index;
};

struct sna_tg_cb {
	struct list_head 	list;

	struct timeval          updated;
	u_int32_t               frsn;
	u_int8_t                l_node_type;	/* local */
	u_int8_t		r_node_type;	/* remote */
	u_int8_t                routing;

	sna_netid		plu_name;
	u_int8_t                tg_number;
	u_int32_t               ls_index;
	u_int32_t               port_index;
	u_int32_t               dlc_index;

	/* we don't use these anymore. */
	struct list_head	cos_list;
};

struct sna_tdm_node_cb {
	struct list_head 	list;

	struct timeval          updated;
	sna_netid		netid;
	u_int32_t		frsn;

	struct list_head	tg_list;

	/* node characteristics, we don't currently use many of these. */
	sna_netid		cp_name;
	u_int8_t		route_resistance;
	u_int8_t		cn;
	u_int8_t		inter_route_depleted;
	u_int8_t		garbage;
	u_int8_t		quiescing;
	u_int8_t		gateway;
	u_int8_t		cds;
	u_int8_t		inter_route;
	u_int8_t		peripheral;
	u_int8_t		interchange;
	u_int8_t		extended;
	u_int8_t		hpr;

	/* we don't use these yet. */
	struct list_head	cos_list;
};

struct sna_tdm_an_cb {
	struct sna_tdm_an_cb	*next;
	struct sna_tdm_an_cb	*prev;
};

struct sna_rss_route {
	unsigned char		cos_name[SNA_RESOURCE_NAME_LEN];
};

/* Control vector 0x0E */
struct sna_cv_nn {
	unsigned short		type;
	unsigned char		nn_type;
	sna_netid	nq_name;
};

/* Control vector 0x2B */
struct sna_cv_rs {
	unsigned short		type;
	unsigned char		max_hops;
	unsigned char		current_hops;
	struct sna_cv_nn	nn;
	struct sna_tg_desc	tg_desc;
};


struct sna_rq_single_hop_route {
	unsigned char		cos_name[SNA_RESOURCE_NAME_LEN];
	sna_netid	dst_cp_name;
	unsigned char		sscp;
	unsigned char		tg_alive;
	unsigned char		tg_types;
	struct sna_cv_rs 	rs;
};

struct sna_rq_tg_vectors {
	sna_netid	org_cp_name;
	sna_netid	dst_cp_name;
	unsigned char		type;
	unsigned char		sscp;
	unsigned char		tg_type;
	unsigned char		tg_alive;
	struct sna_tg_vector	*tg_vectors;
};

struct sna_confirmed {
	__u8 bracket_id;
};

struct sna_cosm_tg_cb {
	struct sna_cosm_tg_cb *next;
	struct sna_cosm_tg_cb *prev;

	unsigned short  rsn;

	unsigned short	min_cost_per_connect;
	unsigned short	max_cost_per_connect;
	unsigned short	min_cost_per_byte;
	unsigned short	max_cost_per_byte;
	unsigned short	min_security;
	unsigned short	max_security;
	unsigned short	min_propagation_delay;
	unsigned short	max_propagation_delay;
	unsigned short	min_effective_capacity;
	unsigned short	max_effective_capacity;
	unsigned short	min_user1;
	unsigned short	max_user1;
	unsigned short	min_user2;
	unsigned short	max_user2;
	unsigned short	min_user3;
	unsigned short	max_user3;

	unsigned char	operational;
	unsigned char	quiescing;
	unsigned char	garbage_collection;
	unsigned char	cpcp_session_status;
};

struct sna_cosm_node_cb {
	struct sna_cosm_node_cb *next;
	struct sna_cosm_node_cb *prev;

	unsigned short	rsn;

	unsigned short	route_resistance;
	unsigned short	node_congested;
	unsigned short	inter_routing_depleted;
	unsigned short	garbage_collection;
	unsigned short	quiescing;
	unsigned short	gateway_support;
	unsigned short	central_directory;
};

/* Transmission Priorities */
#define SNA_TP_LOW              0x0
#define SNA_TP_MEDIUM           0x1
#define SNA_TP_HIGH             0x2
#define SNA_TP_NETWORK          0x3

struct sna_cosm_cb {
	struct list_head list;

	unsigned char	name[SNA_RESOURCE_NAME_LEN];
	unsigned short	weight;
	unsigned short	tx_priority;

	struct sna_cosm_tg_cb	*tg;
	struct sna_cosm_node_cb	*node;

	unsigned char	default_cos_invalid;
	unsigned char	default_cos_null;
};

struct sna_conversation_failure {
	__u8 rcb_id;
	__u8 reason;
};

struct sna_deallocate_rcb {
	__u8 tcb_id;
	__u8 rcb_id;
};

struct sna_init_hs {
	__u8    hs_id;
	__u8    lu_id;
	__u8    pc_id;
	__u8    type;
	__u8    dynamic_buf_pool_id;
	__u8    limit_buf_pool_id;
	__u8    perm_buf_pool_id;
	struct sna_lfsid lfsid;
	__u8    tx_priority;
};

struct sna_init_hs_rsp {
	__u8 type;
	__u8 sense;
	__u8 hs_id;
};

struct sna_init_signal {
	u_int32_t	index;

	sna_netid	local_name;
	sna_netid	remote_name;
	u_int8_t	mode_name[SNA_RESOURCE_NAME_LEN];
};

struct sna_lfsid_in_use_rsp {
	__u8 path_control_id;
	struct sna_lfsid *lfsid;
	__u8 answer;
};

struct sna_lfsid_in_use {
	__u8 path_control_id;
	struct sna_lfsid *lfsid;
	__u8 answer;
};

struct sna_pc_hs_disconnect {
	__u8 path_control_id;
	struct sna_lfsid *lfsid;
};

struct sna_receive_error {
	__u8 bracket_id;
};

struct sna_request_to_send {
	__u8 bracket_id;
};

struct sna_rm_act_session_rq {
	unsigned long tcb_id;
	sna_netid plu_netid;
	unsigned char mode_name[SNA_RESOURCE_NAME_LEN];
};

#define SNA_ACT_SESS_RETRY	0

struct sna_rm_deactivate_conv_group {
	__u8 tcb_id;
	__u8 gid;
	__u8 sense;
};

struct sna_rm_deactivate_session {
	__u8 tcb_id;
	__u8 session_id;
	__u8 type;
	__u8 sense;
};

struct sna_rm_hs_connected {
	/* Blame IBM */
};

struct sna_rsp_to_request_to_send {
	__u8 bracket_id;
};

struct sna_rtr_rq {
	__u8 hs_id;
};

struct sna_rtr_rsp {
	__u8 hs_id;
	__u8 rti;
	__u8 sense;
};

struct sna_send_error {
	/* Blame IBM */
};

struct sna_send_rtr {
	/* Blame IBM */
};

struct sna_deactivate_session {
	__u8 status;
	__u8 correlator;
	__u8 hs_id;
	__u8 type;
	__u8 sense;
};

struct sna_sessend_signal {
	__u8 sense;
	__u8 fqpcid;
	__u8 path_control_id;
};

struct sna_sessst_signal {
	__u8 path_control_id;
};

struct sna_start_tp_reply {
	__u8 rcode;
	__u8 tcb_id;
};

struct sna_terminate_ps {
	__u8 tcb_id;
};

struct sna_unbind_protocol_error {
	__u8 tcb_id;
	__u8 hs_id;
	__u8 sense;
};

struct sna_yield_session {
	/* Blame IBM */
};

/* IPM.
 */

#define SNA_RH_IPM_TYPE         0x830100

struct sna_ipm {
	__u8    type:2,
		rscwrc:1,
		rsv1:5;
	__u16   format:1,
		nws:15;
};

/* IPM type indicators */
#define SNA_IPM_TYPE_SOL        0x0
#define SNA_IPM_TYPE_UNSOL      0x1
#define SNA_IPM_TYPE_RS_ACK     0x2
#define SNA_IPM_TYPE_RSV        0x3

/* Reset residual-count indicators */
#define SNA_IPM_RS_CNT_FALSE    0x0
#define SNA_IPM_RS_CNT_TRUE     0x1

/* Next-window size format indicators. */
#define SNA_IPM_NWS_FORMAT_0    0x0

/* Path control
 */
#define SNA_BIND_RQ		0x01
#define SNA_BIND_RSP		0x02
#define SNA_UNBIND_RQ		0x03
#define SNA_UNBIND_RSP		0x04

#define SNA_LS_FLUSHED          0x0302
#define SNA_FREE_LFSID          0x0305
#define SNA_PC_TO_DLC           0x0306
#define SNA_SEND_MU             0x0307
#define SNA_FLUSH_LS            0x0308
#define SNA_INIT_PC             0x0309
#define SNA_LOCAL_BIND_RQ_SEND  0x030A
#define SNA_LOCAL_BIND_RSP_SEND 0x030B
#define SNA_LOCAL_FLUSH_LS      0x030C
#define SNA_LOCAL_UNBIND_RQ_SEND        0x030D
#define SNA_LOCAL_UNBIND_RSP_SEND       0x030E
#define SNA_BIND_IPM_SEND               0x030F
#define SNA_PC_HS_DISCONNECT            0x0310

#define	SNA_PC_INTRANODE		0x0
#define SNA_PC_INTERNODE		0x1

struct sna_pc_cb {
	struct list_head 	list;

	u_int32_t		index;
	u_int32_t		ls_index;
	u_int32_t		port_index;
	u_int32_t		dlc_index;

	sna_netid		cp_name;
	u_int8_t		tg_number;
	u_int8_t                intranode;
	u_int8_t		whole_bind;
	u_int8_t		whole_bind_piu_req;
	u_int8_t		gen_odai_usage_opt_set;
	u_int8_t		limited_resource;
	u_int16_t		tx_max_btu;
	u_int16_t		rx_max_btu;

	u_int8_t		odai;
};

struct sna_pc_hs_table {
	struct sna_pc_hs_table *next;
	struct sna_pc_hs_table *prev;

	__u8    odai;   /* OAF/DAF Assignor indicator. */
	__u8    oaf;    /* Origin Address field. */
	__u8    daf;    /* Destination Address field. */
	__u8    sidh;   /* Session identifier high (OAF). */
	__u8    sidl;   /* Session identifier low (DAF). */

	/* These below may be garbage, we'll see what works best. */
	__u8    hs_id;
	__u8    sc_id;
	__u8    pc_id;
	__u8    lfsid;
};

/* PS.
 */

typedef enum {
	PROGRAM_STATE_CHECK = 0,
	PROGRAM_PARAMETER_CHECK,
	RESOURCE_FAILURE_RETRY,
	RESOURCE_FAILURE_NO_RETRY
} sna_ps_errors;

typedef enum {
	ALLOCATE = 1,
	CONFIRM,
	CONFIRMED,
	DEALLOCATE,
	FLUSH,
	GET_ATTRIBUTES,
	POST_ON_RECEIPT,
	PREPARE_TO_RECEIVE,
	RECEIVE_AND_WAIT,
	RECEIVE_IMMEDIATE,
	REQUEST_TO_SEND,
	SEND_DATA,
	SEND_ERROR,
	TEST,
	MC_ALLOCATE,
	MC_CONFIRM,
	MC_CONFIRMED,
	MC_DEALLOCATE,
	MC_FLUSH,
	MC_GET_ATTRIBUTES,
	MC_POST_ON_RECEIPT,
	MC_PREPARE_TO_RECEIVE,
	MC_RECEIVE_AND_WAIT,
	MC_REQUEST_TO_SEND,
	MC_SEND_DATA,
	MC_SEND_ERROR,
	MC_TEST,
	INITIALIZE_SESSION_LIMIT,
	CHANGE_SESSION_LIMIT,
	RESET_SESSION_LIMIT,
	SET_LUCB,
	SET_PARTNER_LU,
	SET_MODE,
	SET_MODE_OPTIONS,
	SET_TRANSACTION_PROGRAM,
	SET_PRIVILAEGED_FUNCTION,
	SET_RESOURCE_SUPPORTED,
	SET_SYNC_LEVEL_SUPPORTED,
	SET_MC_FUNCTION_SUPPORTED_TP,
	GET_LUCB,
	GET_PARTNER_LU,
	GET_MODE,
	GET_LU_OPTION,
	GET_MODE_OPTION,
	GET_TRANSACTION_PROGRAM,
	GET_PRIVILEGED_FUNCTION,
	GET_RESOURCE_SUPPORTED,
	GET_SYNC_LEVEL_SUPPORTED,
	GET_MC_FUNCTION_SUPPORTED_LU,
	GET_MC_FUNCTION_SUPPORTED_TP,
	LIST_PARTNER_LU,
	LIST_MODE,
	LIST_LU_OPTION,
	LIST_MODE_OPTION,
	LIST_TRANSACTION_PROGRAM,
	LIST_PRIVILEGED_FUNCTION,
	LIST_RESOURCE_SUPPORTED,
	LIST_SYNC_LEVEL_SUPPORTED,
	LIST_MC_FUNCTION_SUPPORTED_LU,
	LIST_MC_FUNCTION_SUPPORTED_TP,
	PROCESS_SESSION_LIMIT,
	ACTIVATE_SESSION,
	DEACTIVATE_CONVERSATION_GROUP,
	DEACTIVATE_SESSION,
	SYNCPT,
	BACKOUT,
	GET_TP_PROPERTIES,
	GET_TYPE,
	WAIT
} ps_verbs;

/* RM.
 */

#define SNA_RM_FALSE    0x0
#define SNA_RM_TRUE     0x1

struct sna_local_lu_cb {
	struct list_head 	list;
	char                    use_name[SNA_RESOURCE_NAME_LEN];

	u_int32_t		index;

	sna_netid 		netid;


	__u8    		lu_name[SNA_RESOURCE_NAME_LEN];
	__u8    		sync_point;
	__u32   		lu_sess_limit;

	__u16   flags;
};

struct sna_remote_lu_cb {
	struct list_head 	list;
	char                    use_name[SNA_RESOURCE_NAME_LEN];

	u_int32_t		index;

	u_int16_t		sessions;

	sna_netid		netid_plu;

	int			parallel;		/* support parallel sessions. */

	/* below needs to be cleaned up. */
	sna_netid 		netid;
	sna_netid 		netid_fqcp;

	__u8    		cnv_security;

	__u16   		flags;
};

#define SNA_MODE_MAX_SESSIONS		1200
#define SNA_MODE_MIN_CONWINNERS		10
#define SNA_MODE_MIN_CONLOSERS		10

struct sna_lu_count {
	u_int16_t       sessions;
	u_int16_t       conwinners;
	u_int16_t       conlosers;
};

struct sna_mode_cb {
	struct list_head 	list;

	u_int32_t		index;

	sna_netid 		netid;
	sna_netid 		netid_plu;

	u_int8_t		mode_name[SNA_RESOURCE_NAME_LEN];

	struct sna_lu_count    	active;
	struct sna_lu_count    	pending;
	struct sna_lu_count	user_max;

	/* user specified general parameters. */
	u_int16_t		tx_max_ru;
	u_int16_t		rx_max_ru;
	u_int8_t		tx_pacing;
	u_int8_t		rx_pacing;

	/* need to cleaup the below. */
	__u8	cos_name[SNA_RESOURCE_NAME_LEN];
	__u32   crypto;         /* 0 = off / 1 = on */
	__u16   flags;
	__u16	auto_activation;
};

typedef enum {
	SNA_TP_STATE_RESET = 0,
	SNA_TP_STATE_PENDING,
	SNA_TP_STATE_ACTIVE
} sna_tp_state;

/* generic verb structure.
 */
struct sna_tp_cb {
	struct list_head 	list;

	u_int32_t		index;
	u_int32_t               rcb_index;
	u_int32_t		mode_index;
	u_int32_t               local_lu_index;
	u_int32_t		remote_lu_index;
	pid_t			pid;

	u_int32_t		state;

	struct sk_buff_head     receive_queue;
	struct sk_buff_head     write_queue;
	wait_queue_head_t       sleep;

	struct timeval		luw;
	u_int16_t		luw_seq;

	u_int8_t		rq_to_send_rcvd;

	int			immediate;

	/* cmrcv. */
	u_int8_t		rx_mc_ll_pull_left;
	u_int32_t		rx_req_len;	/* max_len. */
	u_int32_t		rx_rcv_len;	/* total copied so far. */
	u_int32_t		*rx_buffer;	/* user buffer. */


	u_int8_t		data_received;	/* what_rcvd */
	u_int8_t		status_received;

	/* information below is set by the user of the tp_cb struct. */
	u_int8_t		ae_qualifier[1024];
	u_int16_t		ae_qualifier_length;
	u_int8_t		ae_qualifier_format;

	u_int8_t		allocate_confirm;

	u_int8_t		ap_title[1024];
	u_int16_t		ap_title_length;
	u_int8_t		ap_title_format;

	u_int8_t		application_context_name[256];
	u_int8_t		application_context_length;

	u_int8_t		begin_transaction;

	u_int8_t		confirm_urgency;

	u_int8_t		conversation_security_type;

	u_int8_t                conversation_type;

	u_int8_t		deallocate_type;

	u_int8_t		directory_encoding;

	u_int8_t		directory_syntax;

	u_int8_t		error_direction;

	u_int32_t		fill;

	u_int8_t		init_data[10000];
	u_int16_t		init_data_length;

	u_int8_t		log_data[512];
	u_int16_t		log_data_length;

	u_int8_t		join_transaction;

	u_int8_t		*map_name;
	u_int16_t		map_name_length;

	u_int8_t		mode_name[9];
	u_int8_t		mode_name_length;

	u_int8_t		*partner_id;
	u_int8_t		partner_id_length;

	u_int8_t		partner_id_type;

	u_int8_t		partner_id_scope;

	u_int8_t		partner_lu_name[18];
	u_int8_t		partner_lu_name_length;

	u_int8_t		prepare_data_permitted;

	u_int8_t		prepare_to_receive_type;

	u_int8_t		processing_mode;

	u_int8_t		receive_type;

	u_int8_t		return_control;

	u_int8_t		security_password[11];
	u_int8_t		security_password_length;

	u_int8_t		security_user_id[11];
	u_int8_t		security_user_id_length;

	u_int8_t		send_receive_mode;

	u_int8_t		send_type;

	u_int8_t		sync_level;

	u_int8_t		tp_name[65];
	u_int8_t		tp_name_length;

	u_int8_t		transaction_control;

	u_int8_t		conversation_queue;
};

/* SCM.
 */

#define sna_scm_create(x)       sna_isr_create(x);
#define sna_scm(x)              sna_isr(x);

/* SM.
 */

#ifdef NOT
struct sna_session_info {
	__u8    hs_id;
	__u8    hs_type;
	__u8    bracket_type;
	__u8    send_ru_size;
	__u8    limit_buf_pool_id;
	__u8    perm_buf_pool_id;
	__u8    session_id;
	__u8    *random;
	__u8    limit_resource;
};

struct sna_security {
	__u8    *profile;
	__u8    *passwd;
	__u8    *user_id;
};

struct sna_abend_notify {
	__u8    process;
	__u8    process_id;
	__u8    reason;
};

struct sna_free_lfsid {
	__u8    pc_id;
	__u8    lfsid;
};

struct sna_free_session {
	__u8    hs_id;
};

struct sna_get_session {
	__u8    tcb_id;
	__u8    rcb_id;
	__u8    type;
	__u8    conv_gid;
};

struct sna_session_activated {
	struct sna_session_info session_info;
	__u8    *lu_name;
	__u8    *mode_name;
};

struct sna_session_allocated {
	__u16   send_ru_size;
	__u8    limit_buf_pool_id;
	__u8    perm_buf_pool_id;
	__u8    in_conver;
	__u8    rcode;
};

struct sna_session_deactivated {
	__u8    hs_id;
	__u8    reason;
	__u32   sense;
};

struct sna_session_route_inop {
	__u8    pc_id;
};

struct sna_act_sess_rsp {
	__u8    correlator;
	__u8    type;
	__u8    err_type;
	struct  sna_session_info sess_info;
};

struct sna_change_sessions {
	__u8    tcb_id;
	__u8    rsp;
	__u8    *lu_name;
	__u8    *mode_name;
	__u8    delta;
};

struct sna_luw {
	__u8    *fq_lu_name;
	__u8    instance;
	__u8    sequence;
};

/* Transaction control block. */
struct sna_tcb {
	struct sna_tcb *next;
	struct sna_tcb *prev;

	__u8    tcb_id;
	__u8    tp_name;
	__u8    own_lu_id;

	struct sna_luw luw_id;
	struct sna_resource resource_list;

	__u8    ctrl_cmpnt;

	struct sna_security security;
};

struct sna_scb {
	__u8    hs_id;
	__u8    session_id;

	struct sna_conver conversation;

	__u8    *lu_name;
	__u8    *mode_name;

	__u8    rcb_id;
	__u8    first_speaker;
	__u8    send_ru_size;
	__u8    limit_buf_pool_id;
	__u8    perm_buf_pool_id;
	__u8    bracket_id;

	__u8    rtr_owed;
	__u8    *fq_name;
	__u8    *random;

	__u8    limit_resource;

	struct timer *timer_unique_id;
};

struct sna_received_info {
	__u8    type;
};

struct sna_conver {
	__u8    correlator;
	__u8    gid;
};
#endif

typedef enum {
	SNA_RM_FSM_SCB_STATE_SESSION_ACTIVATION = 0,
	SNA_RM_FSM_SCB_STATE_FREE,
	SNA_RM_FSM_SCB_STATE_PENDING_ATTACH,
	SNA_RM_FSM_SCB_STATE_IN_USE,
	SNA_RM_FSM_SCB_STATE_PENDING_FMH12
} sna_rm_fsm_scb_state;

typedef enum {
	SNA_RM_FSM_SCB_INPUT_R_POS_BID_RSP = 0,
	SNA_RM_FSM_SCB_INPUT_S_GET_SESSION,
	SNA_RM_FSM_SCB_INPUT_R_BID,
	SNA_RM_FSM_SCB_INPUT_R_ATTACH,
	SNA_RM_FSM_SCB_INPUT_R_FMH_12,
	SNA_RM_FSM_SCB_INPUT_R_FREE_SESSION,
	SNA_RM_FSM_SCB_INPUT_S_YIELD_SESSION,
	SNA_RM_FSM_SCB_INPUT_R_SESSION_ACTIVATED_PRI,
	SNA_RM_FSM_SCB_INPUT_R_SESSION_ACTIVATED_SEC,
	SNA_RM_FSM_SCB_INPUT_R_SESSION_ACTIVATED_SECURE
} sna_rm_fsm_scb_input;

typedef enum {
	SNA_RM_SCB_TYPE_FSP = 0,
	SNA_RM_SCB_TYPE_BIDDER
} sna_rm_scb_type;

struct sna_scb {
	struct list_head        list;

	u_int32_t               index;
	u_int32_t		hs_index;
	u_int32_t		rcb_index;
	u_int32_t		session_index;
	u_int32_t		bracket_index;
	int                     type;   /* fsp or bidder. */

	/* information for fsm_rcb_status() */
	u_int32_t               input;
	u_int32_t               state;
};

typedef enum {
	SNA_RM_FSM_RCB_STATE_FREE = 0,
	SNA_RM_FSM_RCB_STATE_IN_USE,
	SNA_RM_FSM_RCB_STATE_PENDING_SCB
} sna_rm_fsm_rcb_state;

typedef enum {
	SNA_RM_FSM_RCB_INPUT_S_GET_SESSION = 0,
	SNA_RM_FSM_RCB_INPUT_R_POS_BID_RSP,
	SNA_RM_FSM_RCB_INPUT_R_NEG_BID_RSP,
	SNA_RM_FSM_RCB_INPUT_R_ATTACH_HS,
	SNA_RM_FSM_RCB_INPUT_S_ALLOCATE_RCB,
	SNA_RM_FSM_RCB_INPUT_S_DEALLOCATE_RCB
} sna_rm_fsm_rcb_input;

typedef enum {
	SNA_RM_RCB_TYPE_FSP = 0,
	SNA_RM_RCB_TYPE_BIDDER,
	SNA_RM_RCB_TYPE_NONE
} sna_rm_rcb_type;

typedef enum {
	SNA_PS_FSM_CONV_STATE_RESET = 1,
	SNA_PS_FSM_CONV_STATE_SEND_STATE,
	SNA_PS_FSM_CONV_STATE_RCV_STATE,
	SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM,
	SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_SEND,
	SNA_PS_FSM_CONV_STATE_RCVD_CONFIRM_DEALL,
	SNA_PS_FSM_CONV_STATE_PREP_TO_RCV_DEFER,
	SNA_PS_FSM_CONV_STATE_DEALL_DEFER,
	SNA_PS_FSM_CONV_STATE_PEND_DEALL,
	SNA_PS_FSM_CONV_STATE_END_CONV
} sna_ps_fsm_conv_state;

typedef enum {
	SNA_PS_FSM_CONV_INPUT_NONE = 0,
	SNA_PS_FSM_CONV_INPUT_S,
	SNA_PS_FSM_CONV_INPUT_R,
	SNA_PS_FSM_CONV_INPUT_ALLOCATE,
	SNA_PS_FSM_CONV_INPUT_ATTACH,
	SNA_PS_FSM_CONV_INPUT_SEND_DATA,			/* 5 */
	SNA_PS_FSM_CONV_INPUT_PREP_TO_RCV_FLUSH,
	SNA_PS_FSM_CONV_INPUT_PREP_TO_RCV_CONFIRM,
	SNA_PS_FSM_CONV_INPUT_PREP_TO_RCV_DEFER,
	SNA_PS_FSM_CONV_INPUT_FLUSH,
	SNA_PS_FSM_CONV_INPUT_CONFIRM,				/* 10 */
	SNA_PS_FSM_CONV_INPUT_SEND_ERROR,
	SNA_PS_FSM_CONV_INPUT_RECEIVE_AND_WAIT,
	SNA_PS_FSM_CONV_INPUT_POST_ON_RECEIPT,
	SNA_PS_FSM_CONV_INPUT_WAIT,
	SNA_PS_FSM_CONV_INPUT_TEST_POSTED,			/* 15 */
	SNA_PS_FSM_CONV_INPUT_TEST_RQ_TO_SEND_RCVD,
	SNA_PS_FSM_CONV_INPUT_RECEIVE_IMMEDIATE,
	SNA_PS_FSM_CONV_INPUT_REQUEST_TO_SEND,
	SNA_PS_FSM_CONV_INPUT_SEND_INDICATOR,
	SNA_PS_FSM_CONV_INPUT_CONFIRM_INDICATOR,		/* 20 */
	SNA_PS_FSM_CONV_INPUT_CONFIRM_SEND_IND,
	SNA_PS_FSM_CONV_INPUT_CONFIRM_DEALLOC_IND,
	SNA_PS_FSM_CONV_INPUT_CONFIRMED,
	SNA_PS_FSM_CONV_INPUT_PROGRAM_ERROR_RC,
	SNA_PS_FSM_CONV_INPUT_SERVICE_ERROR_RC,			/* 25 */
	SNA_PS_FSM_CONV_INPUT_DEALLOC_NORMAL_RC,
	SNA_PS_FSM_CONV_INPUT_DEALLOC_ABEND_RC,
	SNA_PS_FSM_CONV_INPUT_RESOURCE_FAILURE_RC,
	SNA_PS_FSM_CONV_INPUT_ALLOCATION_ERROR_RC,
	SNA_PS_FSM_CONV_INPUT_DEALLOCATE_FLUSH,			/* 30 */
	SNA_PS_FSM_CONV_INPUT_DEALLOCATE_CONFIRM,
	SNA_PS_FSM_CONV_INPUT_DEALLOCATE_DEFER,
	SNA_PS_FSM_CONV_INPUT_DEALLOCATE_ABEND,
	SNA_PS_FSM_CONV_INPUT_DEALLOCATE_LOCAL,
	SNA_PS_FSM_CONV_INPUT_DEALLOCATED_IND,
	SNA_PS_FSM_CONV_INPUT_GET_ATTRIBUTES
} sna_ps_fsm_conv_input;

typedef enum {
	SNA_PS_FSM_ERR_OR_FAIL_STATE_NO_REQUESTS = 1,
	SNA_PS_FSM_ERR_OR_FAIL_STATE_RCVD_ERROR,
	SNA_PS_FSM_ERR_OR_FAIL_STATE_CONV_FAILURE_PROTOCOL_ERROR,
	SNA_PS_FSM_ERR_OR_FAIL_STATE_CONV_FAILURE_SON
} sna_ps_fsm_err_or_fail_state;

typedef enum {
	SNA_PS_FSM_ERR_OR_FAIL_INPUT_RESET = 0,
	SNA_PS_FSM_ERR_OR_FAIL_INPUT_CONV_FAIL_PROTOCOL,
	SNA_PS_FSM_ERR_OR_FAIL_INPUT_CONV_FAIL_SON,
	SNA_PS_FSM_ERR_OR_FAIL_INPUT_RECEIVE_ERROR
} sna_ps_fsm_err_or_fail_input;

typedef enum {
	SNA_PS_FSM_POST_STATE_RESET = 1,
	SNA_PS_FSM_POST_STATE_PEND_POSTED,
	SNA_PS_FSM_POST_STATE_POSTED
} sna_ps_fsm_post_state;

typedef enum {
	SNA_PS_FSM_POST_INPUT_POST_ON_RECEIPT = 0,
	SNA_PS_FSM_POST_INPUT_TEST,
	SNA_PS_FSM_POST_INPUT_WAIT,
	SNA_PS_FSM_POST_INPUT_RECEIVE_IMMEDIATE,
	SNA_PS_FSM_POST_INPUT_POST
} sna_ps_fsm_post_input;

typedef enum {
	SNA_RCB_LL_STATE_NONE = 0,
	SNA_RCB_LL_STATE_COMPLETE,
	SNA_RCB_LL_STATE_INCOMPLETE
} sna_rcb_ll_states;

struct sna_post_info {
	u_int8_t	fill;		/* LL or BUFFER. */
	u_int16_t	max_len;
};

/* Resource control block. */
struct sna_rcb {
	struct list_head 	list;

	/* just a few indexes. */
	u_int32_t		index;
	u_int32_t		hs_index;
	u_int32_t		tp_index;
	u_int32_t		mode_index;
	u_int32_t		local_lu_index;
	u_int32_t               bracket_index;
	u_int32_t		session_index;
	u_int32_t		conversation_index;

	int			type;	/* fsp or bidder. */

	/* information for fsm_rcb_status() */
	u_int32_t               fsm_rcb_status_input;
	u_int32_t               fsm_rcb_status_state;

	/* fields used by ps. */
	u_int32_t               fsm_conv_state;
	u_int32_t               fsm_err_or_fail_state;
	u_int32_t               fsm_post_state;

	/* all records from hs to ps use this. */
	struct sk_buff_head	rm_to_ps_queue;		/* incomming data. */
	struct sk_buff_head	hs_to_ps_queue;		/* incomming data. */
	struct sk_buff_head	hs_to_ps_buffer_queue;	/* user data to be processed. */
	struct sk_buff_head	ps_to_tp_buffer_queue;	/* user data to be delivered. */
	long			wait_timeo;
	wait_queue_head_t       sleep;

	u_int16_t		ps_to_tp_buffer_size;

	/* logical record processing. */
	u_int8_t		ll_tx_state;	/* logical record status. */
	u_int16_t		ll_tx_rec_size;	/* total size for current record. */
	u_int16_t		ll_tx_rec_bytes;/* current byte total for record. */
	u_int8_t		ll_rx_state;
	u_int8_t		ll_rx_rec_size;
	u_int8_t		ll_rx_rec_bytes;
	u_int8_t		end_of_chain_type;

	/* biu hold buffer and limits. */
	u_int16_t               tx_max_btu;
	u_int16_t               rx_max_btu;
	struct sk_buff		*send_mu;

	/* fields used in conversation with tps. */
	u_int8_t		rq_to_send_rcvd;
	u_int8_t		sync_level;
	struct sna_post_info	rx_post;

	/* everything below here needs to be verified. */
	__u8    lu_name_length;
	__u8    lu_name[SNA_FQCP_NAME_LEN];

	__u8    mode_name_length;
	__u8    mode_name[SNA_RESOURCE_NAME_LEN];

	__u8    tp_name_length;
	__u8    tp_name[64];

/* Everything below here is questionable */

	__u8    security_select;

//      struct sna_session_parm *sessions_parm_ptr;

//      struct sna_post_cond post_conditions;

	__u8    locks;
	__u16   send_ru_size;

	__u32   mapper_save_area;
	__u16   mc_max_send_size;
	__u8    mc_rq_to_send_rcvd;
};

#ifdef NOT
struct sna_tp {
	struct sna_tp *next;
	struct sna_tp *prev;

	__u8    *tp_name;

	struct sna_priv_funct priv_functs;
	struct sna_rcb rcb;

	__u8    verify_pip;
	__u8    sub_pips;

	struct sna_sync_level sync_level;

	__u8    max_tp;
	__u8    tp_cnt;
	__u8    status;

	struct sna_wait_init_rqs wait_init_rqs;

	struct sna_mc_funct mc_funct;
};

struct sna_start_tp {
	__u8    reply;
	__u8    *tp_name;
	__u8    security_select;
	struct sna_security security;
	__u8    tcb_id;
	__u8    *pip_data;
	__u8    fq_lu_name;
};

struct sna_plu {
	struct sna_plu *next;
	struct sna_plu *prev;

	__u8    *lu_name;
	__u8    *fq_name;
	__u8    parallel_lus;

	struct sna_mode mode;
};

struct sna_lucb {
	struct sna_lucb *next;
	struct sna_lucb *prev;

	__u8    lu_id;          /* LU ID. */
	__u8    *fq_lu_name;
	__u8    slimit;         /* Session limit. */

	struct sna_plu          plu;
	struct sna_tp           tp;
	struct sna_rand_data    random;
};
#endif

/* SS.
 */

struct sna_ss_pinfo {
	struct list_head list;

	sna_netid netid;

	unsigned long   bind_reassembly;
	unsigned long   adaptive_bind_pacing;
};
#endif	/* __KERNEL__ */
#endif	/* __NET_SNA_CBS_H */
