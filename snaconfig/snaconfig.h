/* snaconfig.h: headers.
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

#ifndef _SNACONFIG_H
#define _SNACONFIG_H

#define _PATH_SNACFG_XML_HREF	"http://www.linux-sna.org/sna"
#define _PATH_SNACFG_XML_TOP	"node"
#define _PATH_SNACFG_CONF	"/etc/sna.xml"
#define _PATH_SNA_DEBUG_LEVEL	"/proc/sys/net/sna/debug_level"
#define _PATH_SNA_VIRTUAL_NODE	"/proc/net/sna/local_nodes"

#define _PATH_SNACFG_CONF_MAX	300

#define new(p)          ((p) = calloc(1, sizeof(*(p))))
#define new_s(p, s)	((p) = calloc(1, s))

#define next_arg_fail(xv, xc, use)	\
({					\
	if (!*++(xv)) {			\
		printf("fail\n");	\
		use();			\
	} else				\
		(xc)--;			\
	*(xv);				\
})

#define next_arg(xv, xc)		\
({					\
	(xv)++, (xc)--;			\
})

#define sna_debug(level, format, arg...) do {                           \
        if (sna_debug_level >= level)                                   \
                printf(__FILE__ ":" __FUNCTION__ ": " format, ## arg);  \
} while (0)

typedef struct {
	struct list_head	list;

        char 			cos_name[SNA_RESOURCE_NAME_LEN];
        int 			weight;
        int 			tx_priority;
        int 			default_cos_invalid;
        int 			default_cos_null;
        int 			min_cost_per_connect;
	int			max_cost_per_connect;
        int 			min_cost_per_byte;
	int			max_cost_per_byte;
        int 			min_security;
        int 			max_security;
        int 			min_propagation_delay;
        int 			max_propagation_delay;
        int 			min_effective_capacity;
        int 			max_effective_capacity;
        int 			min_user1;
        int 			max_user1;
        int 			min_user2;
        int 			max_user2;
        int 			min_user3;
        int 			max_user3;
        int 			min_route_resistance;
        int 			max_route_resistance;
        int 			min_node_congested;
        int 			max_node_congested;
} cos_info;

typedef struct {
	struct list_head	list;

        char 			sym_dest_name[SNA_RESOURCE_NAME_LEN];
        char 			mode_name[SNA_RESOURCE_NAME_LEN];
        char 			tp_name[65];
	sna_netid               plu_name;
} cpic_info;

typedef struct {
	struct list_head	list;

        char 			name[SNA_RESOURCE_NAME_LEN];
	char                    cos_name[SNA_RESOURCE_NAME_LEN];
        sna_netid		plu_name;
        int 			encryption;
        int 			tx_pacing;
        int 			rx_pacing;
        int 			max_tx_ru;
        int 			max_rx_ru;
        int 			max_sessions;
        int 			min_conwinners;
        int 			min_conlosers;
        int 			auto_activation;
} mode_info;

typedef struct {
	struct list_head	list;

	char			use_name[SNA_USE_NAME_LEN];
	int			type;

	/* remote */
	sna_netid		plu_name;
	sna_netid		fqcp_name;

	/* local */
        char 			name[SNA_RESOURCE_NAME_LEN];
        int 			syncpoint;
        int 			lu_sess_limit;
} lu_info;

typedef struct {
	struct list_head	list;

	char			use_name[SNA_USE_NAME_LEN];
       	char 			interface[SNA_RESOURCE_NAME_LEN];
        char 			port[SNA_RESOURCE_NAME_LEN];
	sna_netid		plu_name;
	sna_nodeid		plu_node_id;
	
        char 			dstaddr[MAX_ADDR_LEN];
        char 			dstport[8];
	int			role;
	int			direction;
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
} link_info;

typedef struct {
	struct list_head	list;

	char			use_name[SNA_USE_NAME_LEN];
	char 			interface[SNA_RESOURCE_NAME_LEN];
        char 			port[SNA_RESOURCE_NAME_LEN];
        int 			btu;
        int 			mia;
        int 			moa;
} dlc_info;

typedef struct {
	sna_netid		node_name;
	sna_nodeid		node_id;
	int			node_type;
        char			debug_level[8];
	int			max_lus;
	int			lu_seg;
	int			bind_seg;

	struct list_head	dlc_list;
	struct list_head	link_list;
	struct list_head	lu_list;
	struct list_head	mode_list;
	struct list_head	cpic_list;
	struct list_head	cos_list;
} global_info;

#ifdef NOT
#define SNA_LINK_NAME_SIZE              8
#define SNA_LINK_DESC_SIZE              24
#define SNA_MODE_NAME_SIZE              8
#define SNA_MODE_DESC_SIZE              24
#define SNA_LU_NAME_SIZE                8
#define SNA_LU_DESC_SIZE                24

#define SNA_NETWORK_NAME_SIZE           8
#define SNA_CP_NAME_SIZE                8

#define SNA_NODE_TYPE_LEN		0
#define SNA_NODE_TYPE_APPN		1
#define SNA_NODE_TYPE_NN		2

#define SNA_TX_RX_CAP_TWA		0
#define SNA_TX_RX_CAP_TWS		1

#define SNA_COMPRESSION_RLE		0
#define SNA_COMPRESSION_LZ9		1

/* utilities */
extern int sna_netid_to_char(struct sna_netid *n, unsigned char *c);
extern unsigned char flip_byte(unsigned char v);
#endif

/* generic functions. */
extern int sna_nof_connect(void);
extern int sna_nof_disconnect(int sk);
extern int matches(const char *cmd, char *pattern);

/* utilities. */
extern sna_netid *sna_char_to_netid(unsigned char *b);
extern unsigned long sna_char_to_nodeid(char *c);
extern int sna_char_to_ether(char *bufp, char *bmac);

extern char *sna_pr_nodeid(unsigned long nodeid);
extern char *sna_pr_netid(sna_netid *n);
extern char *sna_pr_ether(unsigned char *ptr);

/* actions. */
extern int sna_find(int argc, char **argv);
extern int sna_load(int argc, char **argv);
extern int sna_reload(int argc, char **argv);
extern int sna_unload(int argc, char **argv);
extern int sna_start(int argc, char **argv);
extern int sna_stop(int argc, char **argv);
extern int sna_delete(int argc, char **argv);
extern int sna_show(int argc, char **argv);
#endif	/* _SNACONFIG_H */
