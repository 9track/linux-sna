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

#ifndef _LINUX_SNA_H
#define _LINUX_SNA_H

#define SNA_USE_NAME_LEN	40
#define SNA_NETWORK_NAME_LEN    9
#define SNA_RESOURCE_NAME_LEN   9
#define SNA_FQCP_NAME_LEN	18
#define SNA_PORT_ADDR_LEN       12

typedef struct {
        unsigned char net[SNA_NETWORK_NAME_LEN];
        unsigned char name[SNA_RESOURCE_NAME_LEN];
} sna_netid;

typedef u_int32_t       sna_nodeid;
typedef u_int8_t	sna_mode_name[SNA_RESOURCE_NAME_LEN];
typedef u_int8_t	sna_fqpcid[8];

#define xid_get_block_num(nodeid)   ((((u_int32_t)nodeid) >> 20) & 0xFFF)
#define xid_get_pu_num(nodeid)      (((u_int32_t)nodeid) & 0xFFFFF)
#define xid_set_block_num(block)    (((u_int32_t)block) << 20)
#define xid_set_pu_num(pu)          (((u_int32_t)pu))


#define SNA_DOWN        0x1
#define SNA_UP          0x2             /* Registed w/ the system */
#define SNA_RUNNING     0x4             /* In a Running state */
#define SNA_STOPPED     0x8             /* In a Stopped state */

#define SNA_PORT_ROLE_PRI       0x1
#define SNA_PORT_ROLE_SEC       0x2
#define SNA_PORT_ROLE_NEG       0x4

typedef enum {
	SNA_LEN_END_NODE = 1,
	SNA_APPN_END_NODE,
	SNA_APPN_NET_NODE
} sna_node_types;

typedef enum {
        SNA_LS_ROLE_SEC = 0,
        SNA_LS_ROLE_PRI,
        SNA_LS_ROLE_RSV,
        SNA_LS_ROLE_NEG
} sna_ls_role;

typedef enum {
	SNA_LS_DIR_BOTH = 0,
	SNA_LS_DIR_IN,
	SNA_LS_DIR_OUT
} sna_ls_direction;

struct sna_nof_cpic {
        int             action;

        sna_netid       netid;
        sna_netid       netid_plu;

        unsigned char   sym_dest_name[9];
        unsigned char   mode_name[9];
        unsigned char   tp_name[64];
        unsigned char   service_tp;
        unsigned short  security_level;
        unsigned char   username[10];
        unsigned char   password[10];
};

struct cpicsreq {
        struct cpicsreq         *next;
        struct cpicsreq         *prev;

        char                    netid[18];
        char                    netid_plu[18];
        char                    sym_dest_name[9];
        char                    mode_name[9];
        char                    tp_name[65];
        unsigned char           service_tp;
        unsigned char           security_level;
        char                    username[11];
        char                    password[11];
        unsigned short          flags;
        unsigned long           proc_id;
};

struct cpicsconf {
        int     cpics_len;
        char    cpics_net[9];
        char    cpics_name[9];
        char    cpics_plunet[9];
        char    cpics_pluname[9];
        char    cpics_sym_dest_name[9];

        union {
                char            *cpicsc_buf;
                struct cpicsreq   *cpicsc_req;
        } cpicsc_cpicscu;
};

struct sna_qcpics {
        struct sna_qcpics *next;
        struct cpicsreq   data;
};

#define cpicsc_buf cpicsc_cpicscu.cpicsc_buf         /* buffer address       */
#define cpicsc_req cpicsc_cpicscu.cpicsc_req         /* array of structures  */

#ifdef __KERNEL__

extern char sna_sw_prd_version[];
extern char sna_sw_prd_release[];
extern char sna_sw_prd_modification[];
extern char sna_maintainer[];
extern char sna_product_id[];
extern char sna_product_name[];
extern char sna_vendor[];

extern u_int32_t sna_debug_level;

extern void sna_mod_inc_use_count(void);
extern void sna_mod_dec_use_count(void);

extern int hexdump(unsigned char *pkt_data, int pkt_len);
extern int sna_utok(void *uaddr, int ulen, void *kaddr);
extern int sna_ktou(void *kaddr, int klen, void *uaddr);

#define SNA_DEBUG 1		/* remove this define for performance builds. */
#ifndef SNA_DEBUG
#define sna_debug(level, format, arg...)
#else
#define sna_debug(level, format, arg...) \
        if(sna_debug_level >= level)  \
                printk(KERN_EMERG __FILE__ ":" __FUNCTION__ ": " format, ## arg)
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

#endif 	/* __KERNEL__ */

struct cosreq {
	struct cosreq		*next;
	struct cosreq		*prev;

	unsigned char   name[SNA_RESOURCE_NAME_LEN];
        unsigned short  weight;
        unsigned short  tx_priority;
	unsigned char   default_cos_invalid;
        unsigned char   default_cos_null;
};

struct cosconf {
        int     cos_len;
        char    cos_name[SNA_RESOURCE_NAME_LEN];

        union {
                char            *cosc_buf;
                struct cosreq   *cosc_req;
        } cosc_coscu;
};

struct sna_qcos {
        struct sna_qcos *next;
        struct cosreq   data;
};

#define cosc_buf cosc_coscu.cosc_buf              /* buffer address       */
#define cosc_req cosc_coscu.cosc_req              /* array of structures  */

struct plureq {
	struct plureq		*next;
	struct plureq		*prev;

	sna_netid	netid;
	sna_netid	plu_name;
	sna_netid	fqcp_name;
	unsigned char		parallel_ss;
        unsigned char   	cnv_security;
        unsigned long   	proc_id;
        unsigned short  	flags;
};

struct pluconf {
	int	plu_len;
	char	plu_net[SNA_NETWORK_NAME_LEN];
	char	plu_name[SNA_RESOURCE_NAME_LEN];
	char	plu_plunet[SNA_NETWORK_NAME_LEN];
	char	plu_pluname[SNA_RESOURCE_NAME_LEN];
	char	plu_fqcpnet[SNA_NETWORK_NAME_LEN];
	char	plu_fqcpname[SNA_RESOURCE_NAME_LEN];

        union {
                char            *pluc_buf;
                struct plureq	*pluc_req;
        } pluc_plucu;
};

struct sna_qplu {
        struct sna_qplu *next;
        struct plureq   data;
};

#define pluc_buf pluc_plucu.pluc_buf              /* buffer address       */
#define pluc_req pluc_plucu.pluc_req              /* array of structures  */

struct lureq {
	struct lureq		*next;
	struct lureq		*prev;

	sna_netid		netid;
	char			name[SNA_RESOURCE_NAME_LEN];
	unsigned char		sync_point;
	unsigned long		lu_sess_limit;
	unsigned long   	proc_id;
        unsigned short  	flags;
};

struct luconf {
	int             lu_len;
        char            lu_net[SNA_NETWORK_NAME_LEN];
        char            lu_name[SNA_RESOURCE_NAME_LEN];
	char		lu_luname[SNA_RESOURCE_NAME_LEN];

        union {
                char              *luc_buf;
                struct lureq      *luc_req;
        } luc_lucu;
};

struct sna_qlu {
        struct sna_qlu *next;
        struct lureq   data;
};

#define luc_buf luc_lucu.luc_buf              /* buffer address       */
#define luc_req luc_lucu.luc_req              /* array of structures  */

/* mode information request record. */
struct modereq {
	struct modereq		*next;
	struct modereq		*prev;

	char			mode_name[SNA_RESOURCE_NAME_LEN];
	char			cos_name[SNA_RESOURCE_NAME_LEN];

	sna_netid		netid;
	sna_netid		plu_name;
	unsigned long		tx_pacing;
        unsigned long   	rx_pacing;
        unsigned long   	max_tx_ru;
        unsigned long   	max_rx_ru;
        unsigned long   	crypto;
        unsigned long   	proc_id;
        unsigned short  	flags;

	unsigned short  	auto_activation;
        unsigned long   	max_sessions;
        unsigned long   	min_conlosers;
        unsigned long   	min_conwinners;

        unsigned long   	act_sessions;
        unsigned long   	act_conwinners;
        unsigned long   	act_conlosers;

        unsigned long   	pend_sessions;
        unsigned long   	pend_conwinners;
        unsigned long   	pend_conlosers;
};

struct modeconf {
	int             mode_len;
        char            mode_net[SNA_NETWORK_NAME_LEN];
        char            mode_name[SNA_RESOURCE_NAME_LEN];
	char		mode_modename[SNA_RESOURCE_NAME_LEN];

        union {
                char              *modec_buf;
                struct modereq      *modec_req;
        } modec_modecu;
};

struct sna_qmode {
        struct sna_qmode *next;
        struct modereq   data;
};

#define modec_buf modec_modecu.modec_buf              /* buffer address       */
#define modec_req modec_modecu.modec_req              /* array of structures  */

struct lsreq {
	struct lsreq		*next;
	struct lsreq		*prev;

	sna_netid		netid;

	u_int8_t		use_name[SNA_USE_NAME_LEN];
	u_int8_t		dev_name[SNA_RESOURCE_NAME_LEN];
	u_int8_t		port_name[SNA_RESOURCE_NAME_LEN];
	
	u_int32_t		index;
	u_int32_t		flags;

	u_int32_t		retries;
	u_int32_t		xid_count;
	
	u_int32_t		state;
	u_int32_t		co_status;
	u_int32_t		xid_state;

	u_int8_t		effective_role;
	u_int8_t		effective_tg;

	u_int16_t               tx_max_btu;
        u_int16_t               rx_max_btu;
        u_int16_t               tx_window;
        u_int16_t               rx_window;
	
	/* destination link station. */
        sna_netid               plu_name;
        sna_nodeid              plu_node_id;
        u_int8_t                plu_mac_addr[6];
        u_int8_t                plu_port;

	/* user defined link station options. */
	int                     role;
        int                     direction;
        int                     xid_init_method;
        int                     byteswap;
        int                     retry_on_fail;
        int                     retry_times;
        int                     autoact;
        int                     autodeact;
        int                     tg_number;
        int                     cost_per_byte;
        int                     cost_per_connect_time;
        int                     effective_capacity;
        int                     propagation_delay;
        int                     security;
        int                     user1;
        int                     user2;
        int                     user3;
};

struct lsconf {
        int             ls_len;
        char            ls_net[SNA_NETWORK_NAME_LEN];
        char            ls_name[SNA_RESOURCE_NAME_LEN];
        char            ls_devname[SNA_RESOURCE_NAME_LEN];
        char            ls_portname[SNA_RESOURCE_NAME_LEN];
	char		ls_lsname[SNA_RESOURCE_NAME_LEN];

        union {
                char              *lsc_buf;
                struct lsreq      *lsc_req;
        } lsc_lscu;
};

struct sna_qls {
        struct sna_qls *next;
        struct lsreq   data;
};

#define lsc_buf lsc_lscu.lsc_buf              /* buffer address       */
#define lsc_req lsc_lscu.lsc_req              /* array of structures  */

struct portreq {
	struct portreq		*next;
	struct portreq		*prev;

	sna_netid		netid;
	u_int8_t		use_name[SNA_USE_NAME_LEN];
        u_int8_t            	dev_name[SNA_RESOURCE_NAME_LEN];

	u_int16_t		type;
	u_int16_t		index;
	u_int32_t		flags;
	
	u_int32_t		ls_qlen;
	
	char			saddr[12];

	u_int32_t		btu;
	u_int32_t		mia;
	u_int32_t		moa;

	struct lsreq		*ls;
};

struct portconf {
        int             port_len;
        char            port_net[SNA_NETWORK_NAME_LEN];
        char            port_name[SNA_RESOURCE_NAME_LEN];
        char            port_devname[SNA_RESOURCE_NAME_LEN];
	char		port_portname[SNA_RESOURCE_NAME_LEN];

        union {
                char              *portc_buf;
                struct portreq    *portc_req;
        } portc_portcu;
};

struct sna_qport {
        struct sna_qport *next;
        struct portreq   data;
};

#define portc_buf portc_portcu.portc_buf              /* buffer address       */
#define portc_req portc_portcu.portc_req              /* array of structures  */

struct dlcreq {
	struct dlcreq		*next;
	struct dlcreq		*prev;

	u_int8_t		dev_name[SNA_RESOURCE_NAME_LEN];
	
	u_int32_t		index;
	u_int32_t		flags;
	u_int16_t		type;
	u_int16_t		mtu;
	u_int8_t		dev_addr[8];
	
	struct portreq		*port;
};

struct dlconf {
        int             dlc_len;
        char            dlc_net[SNA_NETWORK_NAME_LEN];
        char            dlc_name[SNA_RESOURCE_NAME_LEN];
	char		dlc_devname[SNA_RESOURCE_NAME_LEN];

        union {
                char             *dlc_buf;
                struct dlcreq    *dlc_req;
        } dlc_dlcu;
};

struct sna_qdlc {
        struct sna_qdlc *next;
        struct dlcreq   data;
};

#define dlc_buf dlc_dlcu.dlc_buf              /* buffer address       */
#define dlc_req dlc_dlcu.dlc_req              /* array of structures  */

struct snareq {
	char		net[SNA_NETWORK_NAME_LEN];
	char		name[SNA_RESOURCE_NAME_LEN];
	sna_nodeid	nodeid;
	unsigned short  type;           /* Node type. */
        unsigned short  lu_seg;         /* Is LU segmenting supported. */
        unsigned short   bind_seg;      /* Is Bind segmenting supported. */
        unsigned long    max_lus;	/* Max LU sessions */
	unsigned short	node_status;
};

struct snaconf {
        int             snac_len;
	char		snac_net[SNA_NETWORK_NAME_LEN];
	char		snac_name[SNA_RESOURCE_NAME_LEN];

        union {
                char             *snac_buf;
                struct snareq    *snac_req;
        } snac_snacu;
};

struct sna_qsna {
        struct sna_qsna *next;
        struct snareq   data;
};

#define snac_buf snac_snacu.snac_buf              /* buffer address       */
#define snac_req snac_snacu.snac_req              /* array of structures  */

struct sna_all_info {
	struct sna_all_info *next;		/* Next node */

	/* Std. Node info. */
	char		net[SNA_RESOURCE_NAME_LEN];
	char		name[SNA_RESOURCE_NAME_LEN];
	sna_nodeid	nodeid;
	unsigned short  type;           /* Node type. */
        unsigned short  lu_seg;         /* Is LU segmenting supported. */
        unsigned short  bind_seg;      	/* Is Bind segmenting supported. */
        unsigned long   max_lus;       	/* Max LU sessions */
	unsigned short	node_status;	

	struct modereq		*mode;
	struct plureq		*plu;
	struct lureq		*lu;
	struct dlcreq		*dl;
	struct cpicsreq		*cpics;
	struct cosreq		*cos;
};

#define SNA_NODE_NAME_LEN	8

typedef enum {
	SNA_QUERY_CONNECTION_NAME,
	SNA_QUERY_CLASS_OF_SERVICE,
	SNA_QUERY_CONNECTION_NETWORK,
	SNA_QUERY_DLC,
	SNA_QUERY_ISR_TUNING,
	SNA_QUERY_LINK_STATION,
	SNA_QUERY_PORT,
	SNA_QUERY_STATISTICS,
	SNA_RESET_SESSION_LIMIT,
} sna_nof_commands;

#define SIOCGNODE       0x1000
#define SIOCGDLC        0x1001
#define SIOCGPORT       0x1002
#define SIOCGLS         0x1003
#define SIOCGMODE       0x1004
#define SIOCGLU         0x1005
#define SIOCGPLU        0x1006
#define SIOCGCPICS      0x1007
#define SIOCGPS         0x1008
#define SIOCGCOS        0x1009

typedef enum {
	SNA_NOF_NODE = 0,
	SNA_NOF_DLC,
	SNA_NOF_PORT,
	SNA_NOF_LS,
	SNA_NOF_MODE,
	SNA_NOF_LOCAL_LU,
	SNA_NOF_REMOTE_LU,
	SNA_NOF_TP,
	SNA_NOF_COS,
	SNA_NOF_CPIC,
	SNA_NOF_ADJACENT_NODE,
	SNA_NOF_CONNECTION_NETWORK,
	SNA_NOF_DIRECTORY_ENTRY,
	SNA_NOF_ISR_TUNING,
	SNA_NOF_SESSION_LIMIT,
	SNA_NOF_CONTROL_SESSIONS
} sna_nof_component;

typedef enum {
	SNA_NOF_RESET = 0,
	SNA_NOF_DEFINE,
	SNA_NOF_START,
	SNA_NOF_STOP,
	SNA_NOF_DELETE,
	SNA_NOF_CHANGE,
} sna_nof_action;

struct sna_cp_create_parms {
	char connection_name[SNA_RESOURCE_NAME_LEN];
	struct sna_fq_netid	*netid_cp;
	unsigned long	*netid;
	unsigned long	bind_reassembly;
	unsigned long	adaptive_bind_pacing;
	unsigned long	sense;
};

struct sna_trs_create_parms {
	char connection_name[SNA_RESOURCE_NAME_LEN];
	unsigned long	node_type;
	unsigned long	*cp_name;
	unsigned long	netid;
	unsigned long	rs_tree_caching;
	unsigned long	incr_rs_tree_updates;
	unsigned long	max_rs_trees;
	unsigned long	max_tdm_nodes;
	unsigned long	max_oos_tdus;
	unsigned long	cosdm_file_name;
	unsigned long	tdm_file_name;
	unsigned long	gateway;
	unsigned long	central_directory_server;
	unsigned long   sense;
};

struct sna_nof_adjacent_node {
	int			action;

	char 			connection_name[SNA_RESOURCE_NAME_LEN];
	struct sna_fq_netid 	*netid_cp;
	unsigned long		cp_capabilities;
	unsigned long		node_type;
	unsigned long		auth_lvl_adj_node;
	unsigned long		*netid_lu_list;
	unsigned long   	sense;
};

struct sna_nof_cos {
	int		action;
	unsigned char   name[SNA_RESOURCE_NAME_LEN];
        unsigned short  weight;
        unsigned short  tx_priority;
	unsigned char   default_cos_invalid;
        unsigned char   default_cos_null;

	/* Tg Characteristics */
	unsigned short	tg_rsn;
	unsigned short  min_cost_per_connect;
        unsigned short  max_cost_per_connect;
        unsigned short  min_cost_per_byte;
        unsigned short  max_cost_per_byte;
        unsigned short  min_security;
        unsigned short  max_security;
        unsigned short  min_propagation_delay;
        unsigned short  max_propagation_delay;
        unsigned short  min_effective_capacity;
        unsigned short  max_effective_capacity;
        unsigned short  min_user1;
        unsigned short  max_user1;
        unsigned short  min_user2;
        unsigned short  max_user2;
        unsigned short  min_user3;
        unsigned short  max_user3;

	/* Node Characteristics */
	unsigned short  node_rsn;
        unsigned short  min_route_resistance;
	unsigned short	max_route_resistance;
        unsigned short  min_node_congested;
	unsigned short	max_node_congested;
};

struct sna_nof_connection_network {
	int		action;

	char 		connection_name[SNA_RESOURCE_NAME_LEN];
	unsigned long	*netid;
	unsigned long	tg_characteristics;
	unsigned long	port_list;
	unsigned long   sense;
};

struct sna_nof_directory_entry {
	int		action;

	char 		connection_name[SNA_RESOURCE_NAME_LEN];
	unsigned long	*resource_name;
	unsigned long	resource_type;
	unsigned long	*parent_resource;
	unsigned long	entry_class;
	unsigned long	directory_entry_scope;
	unsigned long	registration_requirements;
	unsigned long   sense;
};

#define SNA_LS_MODE_ABM				0x1
#define SNA_LS_MODE_NRM				0x2

struct sna_nof_isr_tuning {
	int		action;

	char 		connection_name[SNA_RESOURCE_NAME_LEN];
	unsigned long	*secondary_lu_name;
	unsigned long	*mode_name;
	unsigned long	slu_mode_entry;
	unsigned long	pri_rws;
	unsigned long	sec_rws;
	unsigned long	max_pri_rws;
	unsigned long	max_sec_rws;
	unsigned long	nonreserved_perm_buf_pool_size;
	unsigned long	max_pri_ru_size;
	unsigned long	max_sec_ru_size;
	unsigned long	adaptive_session_lvl_pacing;
	unsigned long   sense;
};

struct sna_nof_ls {
	int 		action;
	char		use_name[SNA_USE_NAME_LEN];
	
	sna_netid 	netid;
	char    	name[SNA_RESOURCE_NAME_LEN];	/* eth0 */
        char    	saddr[SNA_PORT_ADDR_LEN];       /* 0x04 */

	sna_netid 	plu_name;
	sna_nodeid	plu_node_id;
	char		dname[7];			/* Dest. MAC addr */
	char		daddr[SNA_PORT_ADDR_LEN];	/* 0x04 */

	int 		role;
	int		direction;
	int 		byteswap;
	int 		retry_on_fail;
        int 		retry_times;
        int 		autoact;
        int 		autodeact;
        int 		tg_number;
        int 		cost_per_byte;
        int 		cost_per_connect_time;
        int 		effective_capacity;
        int 		propagation_delay;
        int 		security;
        int 		user1;
        int 		user2;
        int 		user3;

	unsigned long  	deact_type;
};

struct sna_nof_local_lu {
	int 		action;
	char            use_name[SNA_USE_NAME_LEN];
	
	sna_netid 	netid;
	unsigned char 	lu_name[SNA_RESOURCE_NAME_LEN];
	unsigned char	sync_point;
	unsigned long	lu_sess_limit;

	/* Data below not used */
	unsigned long	reg_requirments;
	unsigned long	bind_range_process;
	unsigned long	queueing_capability;
	unsigned long	map_lu_names_list;
};

struct sna_nof_mode {
	int 		action;

	sna_netid	netid;
	sna_netid	netid_plu;
	unsigned char	mode_name[SNA_RESOURCE_NAME_LEN];
	unsigned char	cos_name[SNA_RESOURCE_NAME_LEN];

	unsigned long	tx_pacing;
	unsigned long	rx_pacing;
	unsigned long	tx_max_ru;
	unsigned long	rx_max_ru;
	unsigned char	crypto;
	unsigned long	min_conwinners;
	unsigned long	min_conlosers;
	unsigned long	max_sessions;
	unsigned char	auto_activation;
};

struct sna_nof_remote_lu {
	int 		action;
	char            use_name[SNA_USE_NAME_LEN];
	
	sna_netid 	netid;
	sna_netid 	netid_plu;
	sna_netid 	netid_fqcp;
	u_int8_t 	parallel_ss;
	unsigned char 	cnv_security;

	/* Data below not used. */
	unsigned long	init_lu_type;
	unsigned long	multi_sessions;
	unsigned long	cnos_lvl_exchg;
	unsigned long	password;
	unsigned long	lu_access_security;
};

struct sna_nof_port {
	int		action;
	char		use_name[SNA_USE_NAME_LEN];
	
	sna_netid 	netid;
	char		name[SNA_RESOURCE_NAME_LEN];	/* eth0 */
	char		saddr[SNA_PORT_ADDR_LEN];	/* 0x4 */
	unsigned long	btu;		/* Max Rx/Tx BTU size. */
	unsigned long	mia;		/* Max inbound activations */
	unsigned long	moa;		/* Max outbound activations */
	
	unsigned long	link_station_txrx;
	unsigned long	max_nonack_xid;

	/* stop port. */
	unsigned long   deact_type;
};

struct sna_nof_tp {
	int			action;

	char 			connection_name[SNA_RESOURCE_NAME_LEN];
	struct sna_fq_netid 	*netid_lu;
	unsigned long		*tp_name;
	unsigned long		init_tp_status;
	unsigned long		conv_types_allowed;
	unsigned long		sync_lvl_supp;
	unsigned long		pip_lvl;
	unsigned long		num_pip_parms;
	unsigned long		data_map_supp;
	unsigned long		fm_hdr_supp;
	unsigned long		max_concurrent_tp_instances;
	unsigned long		*issue_verb_list;
	unsigned long   	sense;
};

struct sna_nof_node {
	int 			action;
	
	sna_netid 		netid;	/* Local NetID.Node. */
	sna_nodeid		nodeid;		/* Local Block and PU ID. */
	unsigned char		type;		/* Node type. */
	unsigned short		lu_seg;		/* Is LU segmenting supported. */
	unsigned short		bind_seg;	/* Is Bind segmenting supported. */
	unsigned long		max_lus;	/* Max number of LU sessions,
					 * 0 = Unlimited.
					 */

	/* Data below not used at the moment */
	unsigned long		netid_registered;
	unsigned long		ls_supp_type;
	unsigned long		resource_registration;
	unsigned long		segment_generation_lvl;
	unsigned long		mode_to_cos_mapping;
	unsigned long		ms_node_type;
	unsigned long		*mj_vector_file;
	unsigned long		*ms_log_file;
	unsigned long		peer_resource_registration;
	unsigned long		network_node_type;
	unsigned long		directory_type_supp;
	unsigned long		rs_tree_update_type;
	unsigned long		*tdm_node_name;
	unsigned long		*cosdm_node_name;
	unsigned long		max_rs_cache_trees;
	unsigned long		max_oos_tdm_updates;
	unsigned long		*resource_service_search;
	unsigned long		general_odai_usage_supp;

        unsigned char           route_resistance;
        unsigned char           quiescing;
	
	unsigned long 		deact_type;
};	

/* SNA Network qualified name is registered or not. */
#define SNA_NOF_NO_NETID_REG		0x0
#define SNA_NOF_NETID_REG		0x1

/* SNA resource registration indicatiors. */
#define SNA_NOF_RESOURCE_REG_NONE	0x0
#define SNA_NOF_RESOURCE_REG_RESOURCES	0x1

/* MS node type. */
#define SNA_NOF_MS_ENTRY_POINT		0x0
#define SNA_NOF_MS_FOCAL_POINT		0x1

typedef enum {
        CO_FAIL = 0,
        CO_RESET,
        CO_ACTIVE,
        CO_S_TEST_C,
        CO_R_TEST_R,
        CO_TEST_OK,
} connect_out_status;

typedef enum {
        SNA_LS_STATE_DEFINED = 0,
        SNA_LS_STATE_ACTIVATED,
        SNA_LS_STATE_ACTIVE
} sna_ls_state_enum;

#ifdef __KERNEL__
#include <net/sna_formats.h>
#include <net/sna_vectors.h>
#include <net/sna_ms_vectors.h>
#include <net/sna_cbs.h>
#include <net/sna_externs.h>
#include <net/sna_errors.h>
#endif  /* __KERNEL__ */
#endif	/* _LINUX_SNA_H */
