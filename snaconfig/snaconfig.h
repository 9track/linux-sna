/* snaconfig.h: headers.
 *
 * Author:
 * Jay Schulist         <jschlst@turbolinux.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * None of the authors or maintainers or their employers admit
 * liability nor provide warranty for any of this software.
 * This material is provided "as is" and at no charge.
 */

#define next_arg(X)     (*X = *X + 1)
#define new(p)          ((p) = calloc(1, sizeof(*(p))))

#define GLOBAL		1
#define DLC		2
#define LINK		3
#define LU_LOCAL	4
#define LU_REMOTE	5
#define MODE		6
#define CPIC		7
#define COS		8

/* This structure describes DLC parameters.
 */
struct dlc {
	struct dlc *next;
	struct dlc *prev;
	
        char interface[SNA_RESOURCE_NAME_LEN];
        char port[SNA_RESOURCE_NAME_LEN];
        char role[SNA_RESOURCE_NAME_LEN];
	int btu;
	int mia;
	int moa;
	int mode;
};

/* This structure describes Link parameters.
 */
struct link {
	struct link *next;
	struct link *prev;
 
        char interface[SNA_RESOURCE_NAME_LEN];
        char port[SNA_RESOURCE_NAME_LEN];
	char plu_name[SNA_FQCP_NAME_LEN];
        char dstaddr[MAX_ADDR_LEN * 2];
        char dstport[8];
	int byteswap;
	int retry_on_fail;
	int retry_times;
        int autoact;
        int autodeact;
        int tg_number;
        int cost_per_byte;
        int cost_per_connect_time;
        int effective_capacity;
        int propagation_delay;
        int security;
        int user1;
        int user2;
        int user3;
        int mode;
};

/* This struct describes Local LU parameters.
 */
struct llu {
	struct llu *next;
	struct llu *prev;

        char name[SNA_RESOURCE_NAME_LEN];
        int syncpoint;
	int lu_sess_limit;
	int mode;
};

/* This struct describes Remote LU parameters.
 */
struct rlu {
	struct rlu *next;
	struct rlu *prev;

        char plu_name[SNA_FQCP_NAME_LEN];
        char fqcp_name[SNA_FQCP_NAME_LEN];
	int mode;
};

/* This struct describes Mode parameters.
 */
struct mode {
 	struct mode *next;
	struct mode *prev;

        char name[SNA_RESOURCE_NAME_LEN];
        char plu_name[SNA_FQCP_NAME_LEN];
	char cos_name[SNA_RESOURCE_NAME_LEN];
	int encryption;
        int tx_pacing;
        int rx_pacing;
        int max_tx_ru;
        int max_rx_ru;
	int max_sessions;
	int min_conwinners;
	int min_conlosers;
	int auto_activation;
	int mode;
};

/* This struct describes CPI-C Side parameters.
 */
struct cpic_side {
	struct cpic_side *next;
	struct cpic_side *prev; 

        char sym_dest_name[SNA_RESOURCE_NAME_LEN];
        char mode_name[SNA_RESOURCE_NAME_LEN];
        char plu_name[SNA_FQCP_NAME_LEN];
        char tp_name[65];
	int mode;
};

struct cos {
	struct cos *next;
	struct cos *prev;

	char cos_name[SNA_RESOURCE_NAME_LEN];
	int weight;
	int tx_priority;
	int default_cos_invalid;
	int default_cos_null;
	int min_cost_per_connect; 
        int max_cost_per_connect; 
        int min_cost_per_byte; 
        int max_cost_per_byte; 
        int min_security; 
        int max_security; 
        int min_propagation_delay; 
        int max_propagation_delay; 
        int min_effective_capacity; 
        int max_effective_capacity; 
        int min_user1; 
        int max_user1; 
        int min_user2; 
        int max_user2; 
        int min_user3; 
        int max_user3; 
        int min_route_resistance; 
        int max_route_resistance; 
        int min_node_congested; 
        int max_node_congested;
	int mode;
};

/* This structure describes global (ie., server-wide) parameters.
 */
typedef struct
{
        char fq_node_name[SNA_FQCP_NAME_LEN];
        char node_type[SNA_RESOURCE_NAME_LEN];
	char node_id[SNA_RESOURCE_NAME_LEN+2];
        int debug_level;

        struct dlc 		*dlcs;
	struct link		*links;
	struct llu		*llus;
	struct rlu		*rlus;
	struct mode		*modes;
	struct cpic_side	*cpics;
	struct cos		*coss;
	int mode;
} global;

struct wordmap {
        const char *word;
        int val;
};

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

#define SNA_LS_ROLE_PRI			0
#define SNA_LS_ROLE_SEC			1
#define SNA_LS_ROLE_NEG			2

#define SNA_TX_RX_CAP_TWA		0
#define SNA_TX_RX_CAP_TWS		1

#define SNA_COMPRESSION_RLE		0
#define SNA_COMPRESSION_LZ9		1

extern int help(void);

/* command parsers */
extern int sna_load_config(global *ginfo);
extern int sna_load_cfg_file(char *cfile);
extern int sna_load_cfg_stdin(char **argv);

/* display functions */
extern int sna_print(struct sna_netid *n);
extern int sna_print_global(global *g);
extern char *sna_pr_word(struct wordmap *wm, const int v);

/* utilities */
extern int sna_netid_to_char(struct sna_netid *n, unsigned char *c);
extern struct sna_netid *sna_char_to_netid(unsigned char *b);
extern char *sna_pr_netid(struct sna_netid *n);
extern char *sna_pr_ether(unsigned char *ptr);
extern unsigned char flip_byte(unsigned char v);
extern int in_ether(char *bufp, char *bmac);
extern int map_word(struct wordmap *wm, const char *word);
extern struct sna_nodeid *sna_char_to_nodeid(char *c);
extern char *sna_pr_nodeid(struct sna_nodeid *n);
