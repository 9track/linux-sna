/* sna_externs.h: Linux-SNA external function declarations.
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

#ifndef __NET_SNA_EXTERNS_H
#define __NET_SNA_EXTERNS_H

#ifdef __KERNEL__

extern int sna_netid_to_char(sna_netid *n, unsigned char *c);
extern sna_netid *sna_char_to_netid(unsigned char *c);
extern char *sna_pr_ether(unsigned char *ptr);
extern char *sna_pr_netid(sna_netid *n);
extern char *sna_pr_nodeid(sna_nodeid n);

extern void sna_ctrl_info_destroy(struct sna_ctrl_info *ctrl);
extern struct sna_ctrl_info *sna_ctrl_info_create(int gfp_mask);
extern struct sk_buff *sna_alloc_skb(unsigned int size, int gfp_mask);

extern struct proc_dir_entry *proc_sna_create(const char *name,
        mode_t mode, get_info_t *get_info);
extern void proc_sna_remove(const char *name);

/* ASM. */
extern struct sna_lfsid *sna_asm_assign_lfsid(u_int32_t pc_index, u_int32_t sm_index);
extern int sna_asm_create(struct sna_nof_node *start);
extern int sna_asm_destroy(struct sna_nof_node *delete);
extern int sna_asm_get_info(char *buffer, char **start,
        off_t offset, int length);
extern int sna_asm_get_active_lfsids(char *buffer, char **start,
        off_t offset, int length);
extern int sna_asm_activate_as(struct sna_asm_cb *as);
extern int sna_asm_deactivate_as(u_int32_t index);
extern struct sna_asm_cb *sna_asm_get_by_lfsid(struct sna_lfsid *lfsid);
extern int sna_asm_rx(struct sna_asm_cb *as, struct sna_lfsid *lf, struct sk_buff *skb);
extern int sna_asm_tx_bind(struct sna_lfsid *lf, u_int32_t pc_index, struct sk_buff *skb);
extern struct sna_lfsid *sna_asm_lfsid_get_by_sidhl(u_int8_t odai, u_int8_t sidh,
        u_int8_t sidl);

/* Attach. */
extern int sna_attach_execute_tp(__u32 tcb_id, struct sk_buff *skb);
extern int sna_attach_init(void);
extern int sna_attach_fini(void);

/* COSM. */
extern int sna_cosm_query_cos(char *arg);
extern int sna_cosm_create(struct sna_nof_node *start);
extern int sna_cosm_destroy(struct sna_nof_node *delete);
extern void sna_cosm_init(void);
extern void sna_cosm_cleanup(void);
extern int sna_cosm_get_info(char *buffer, char **start,
        off_t offset, int length);
extern int sna_cosm_define_cos(struct sna_nof_cos *cos);
extern int sna_cosm_delete_cos(struct sna_nof_cos *cos);
extern int sna_cosm_cos_tpf_vector(struct sna_cos_tpf_vector *cos);
extern int sna_cosm_get_info_tg(char *buffer, char **start,
        off_t offset, int length);
extern int sna_cos_get_info_node(char *buffer, char **start,
        off_t offset, int length);

/* CPIC. */
extern struct sna_tp_cb *sna_cpic_find_tcb_by_id(unsigned long tcb_id);
extern unsigned long sna_cpic_create_tcb(int *err);
extern int sna_cpic_ioctl(int cmd, void *arg);
extern int sna_cpic_setsockopt(struct socket *sock, int level, int optname,
        char *optval, int optlen);
extern int sna_cpic_init(void);
extern int sna_cpic_fini(void);
extern struct sna_tcb *sna_cpic_find_tcb_by_daf(__u8 daf);

/* CS. */
extern struct sna_port_cb *sna_cs_port_get_by_addr(char *saddr);
extern struct sna_ls_cb *sna_cs_ls_get_by_index(struct sna_port_cb *port, u_int32_t index);
extern int sna_cs_dlc_create(struct net_device *dev);
extern int sna_cs_dlc_delete(struct net_device *dev);
extern struct sna_ls_cb *sna_cs_ls_get_by_addr(char *mac, char *lsap);
extern int sna_cs_xid_pkt_input(struct sk_buff *skb);
extern struct sna_dlc_cb *sna_cs_dlc_get_by_index(u_int32_t index);
extern struct sna_port_cb *sna_cs_port_get_by_index(u_int32_t index);

extern struct sna_port_cb *sna_cs_find_port(struct sna_dlc_cb *dlc, 
        char *saddr);
extern struct sna_dlc_cb *sna_cs_find_dlc_name(char *name);
extern int sna_cs_connect_in(struct sna_ls_cb *ls);
extern int sna_cs_rcv_xid(struct sk_buff *skb, struct net_device *dev);
extern int sna_cs_query_dlc(char *arg);
extern int sna_cs_query_port(char *arg);
extern int sna_cs_query_ls(char *arg);
extern int sna_cs_start(struct sna_nof_node *node);
extern int sna_cs_stop(struct sna_nof_node *node);
extern int sna_cs_create(struct sna_nof_node *start);
extern int sna_cs_destroy(struct sna_nof_node *delete);
extern int sna_cs_define_port(struct sna_nof_port *port);
extern int sna_cs_delete_port(struct sna_nof_port *port);
extern int sna_cs_start_port(struct sna_nof_port *port);
extern int sna_cs_stop_port(struct sna_nof_port *port);
extern int sna_cs_define_ls(struct sna_nof_ls *dls);
extern int sna_cs_delete_ls(struct sna_nof_ls *dls);
extern int sna_cs_start_ls(struct sna_nof_ls *sls);
extern int sna_cs_stop_ls(struct sna_nof_ls *sls);
extern u_int32_t sna_cs_activate_route(struct sna_tg_cb *tg, 
	sna_netid *remote_name, int *err);
extern void sna_cs_connect_out(unsigned long data);
extern int sna_cs_xid_xchg_state(struct sk_buff *skb);

#ifdef CONFIG_PROC_FS
extern int sna_cs_get_info_dlc(char *buffer, char **start,
        off_t offset, int length);
extern int sna_cs_get_info_port(char *buffer, char **start,
        off_t offset, int length);
extern int sna_cs_get_info_ls(char *buffer, char **start,
        off_t offset, int length);
#endif

/* DLC. */
#ifdef CONFIG_SNA_LLC
extern int sna_dlc_llc_indicate(struct llc_prim_if_block *prim);
extern int sna_dlc_llc_confirm(struct llc_prim_if_block *prim);
#endif
extern int sna_dlc_test_req(struct sna_ls_cb *ls);
extern int sna_dlc_xid_min_len(struct sna_ls_cb *ls);
extern int sna_dlc_xid_reserve(struct sna_ls_cb *ls, struct sk_buff *skb);
extern int sna_dlc_xid_req(struct sna_ls_cb *ls, struct sk_buff *skb);
extern int sna_dlc_xid_rsp(struct sna_ls_cb *ls, struct sk_buff *skb);
extern int sna_dlc_conn_req(struct sna_ls_cb *ls);
extern int sna_dlc_wait_for_conn(struct sna_ls_cb *ls, int seconds);
extern int sna_dlc_disc_req(struct sna_ls_cb *ls);
extern int sna_dlc_data_req(struct sna_ls_cb *ls, struct sk_buff *skb);
extern int sna_dlc_data_min_len(struct sna_ls_cb *ls);
extern int sna_dlc_data_reserve(struct sna_ls_cb *ls, struct sk_buff *skb);

/* DFC. */
extern int sna_dfc_init(struct sna_hs_cb *hs);
extern int sna_dfc_init_th_rh(struct sk_buff *skb);
extern int sna_dfc_rx(struct sna_hs_cb *hs, struct sk_buff *skb);
extern int sna_dfc_send_from_ps_data(struct sk_buff *skb, struct sna_rcb *rcb);
extern int sna_dfc_send_from_ps_req(struct sk_buff *skb, struct sna_rcb *rcb);

/* DS. */
extern int sna_ds_create(struct sna_nof_node *start);
extern int sna_ds_destroy(struct sna_nof_node *delete);

/* EBCDIC. */
extern unsigned char etor(unsigned char a);
extern unsigned char etoa(unsigned char a);
extern unsigned char atoe(unsigned char a);
extern char *atoe_strcpy(char *dest, char *src);
extern char *etoa_strcpy(char *dest, char *src);
extern char *etor_strncpy(char *dest, char *src, size_t count);
extern char *atoe_strncpy(char *dest, char *src, size_t count);
extern char *etoa_strncpy(char *dest, char *src, size_t count);
extern int atoe_strcmp(const char *acs, const char *ect);
extern int etoa_strcmp(const char *ecs, const char *act);
extern int atoe_strncmp(const char *acs, const char *ect, size_t count);
extern int etoa_strncmp(const char *ecs, const char *act, size_t count);
extern char *fatoe_strncpy(char *dest, char *src, size_t count);

/* HS. */
extern u_int32_t sna_hs_init(struct sna_lulu_cb *lulu, int *err);
extern int sna_hs_init_finish(u_int32_t index, u_int32_t pc_index, 
	struct sna_lfsid *lf, struct sk_buff *skb);
extern struct sna_hs_cb *sna_hs_get_by_index(u_int32_t index);
extern int sna_hs_ps_connected(struct sna_hs_cb *hs, u_int32_t bracket_index, 
	u_int32_t tp_index);
extern int sna_hs_process_lu_lu_session(int who, struct sk_buff *skb,
        struct sna_rcb *rcb);
extern int sna_hs_tx_ps_mu_data(struct sna_rcb *rcb, struct sk_buff *skb);
extern int sna_hs_tx_ps_mu_req(struct sna_rcb *rcb, struct sk_buff *skb);
extern int sna_hs_rx(struct sna_lfsid *lf, struct sk_buff *skb);
extern struct sna_hs_cb *sna_hs_get_by_lfsid(struct sna_lfsid *lf);

/* ISR. */
extern void sna_isr_init(void);
extern void sna_isr_cleanup(void);

/* NOF. */
extern int sna_nof_ioctl(int cmd, void *arg);
extern int sna_nof_setsockopt(struct socket *sock, int level, int optname,
        char *optval, int optlen);
extern int sna_nof_getsockopt(struct socket *sock, int level, int optname,
        char *optval, int *len);
extern int sna_nof_get_info(char *buffer, char **start,
        off_t offset, int length);
extern sna_nodeid sna_nof_find_nodeid(sna_netid *n);

/* PC. */
extern u_int32_t sna_pc_init(struct sna_pc_cb *pc, int *err);
extern int sna_pc_destroy(u_int32_t index);
extern struct sna_pc_cb *sna_pc_find(unsigned char *pc_id);
extern struct sna_pc_cb *sna_pc_find_by_netid(sna_netid *n);
extern int sna_pc_get_info(char *buffer, char **start,
        off_t offset, int length);
extern int sna_pc_rx_mu(struct sk_buff *skb);
extern int sna_pc_tx_mu(struct sna_pc_cb *pc, struct sk_buff *skb, struct sna_lfsid *lf);
extern struct sna_pc_cb *sna_pc_get_by_index(u_int32_t index);

/* PS. */
extern int sna_ps_init(struct sna_tp_cb *tp);
extern int sna_ps_conv(int verb, struct sna_tp_cb *tp);
extern int sna_ps_mc(int verb, struct sna_tcb *tcb);
extern int sna_ps_copr(int verb, struct sna_tcb *tcb);
extern int sna_ps_sync(int verb, struct sna_tcb *tcb);
extern int sna_ps_verb_router(int verb, struct sna_tp_cb *tp);
extern int sna_ps_process_fmh5(__u32 tcb_id, __u32 rcb_id, struct sk_buff *skb);

/* RM. */
extern int sna_rm_create(struct sna_nof_node *node);
extern int sna_rm_destroy(struct sna_nof_node *d);
extern struct sna_rcb *sna_rm_find_rcb_by_id(__u8 rcb_id);
extern __u8 sna_rm_allocate_rcb(struct sna_tcb *tcb);
extern int sna_rm_process_hs_to_rm(struct sk_buff *skb);
extern int sna_rm_destroy(struct sna_nof_node *d);
extern int sna_rm_define_mode(struct sna_nof_mode *dm);
extern int sna_rm_delete_mode(struct sna_nof_mode *dm);
extern int sna_rm_delete_local_lu(struct sna_nof_local_lu *dlu);
extern int sna_rm_delete_remote_lu(struct sna_nof_remote_lu *dplu);
extern int sna_rm_define_local_lu(struct sna_nof_local_lu *dlu);
extern int sna_rm_define_remote_lu(struct sna_nof_remote_lu *lu);
extern int sna_rm_start_remote_lu(struct sna_nof_remote_lu *lu);
extern int sna_rm_stop_remote_lu(struct sna_nof_remote_lu *lu_n);
extern int sna_rm_query_mode(char *arg);
extern int sna_rm_query_lu(char *arg);
extern int sna_rm_query_plu(char *arg);
extern int sna_rm_get_info_mode(char *buffer, char **start,
        off_t offset, int length);
extern int sna_rm_get_info_local_lu(char *buffer, char **start,
        off_t offset, int length);
extern int sna_rm_get_info_remote_lu(char *buffer, char **start,
        off_t offset, int length);
extern int sna_rm_session_activated_proc(struct sna_lulu_cb *lulu);
extern struct sna_remote_lu_cb *sna_rm_remote_lu_get_by_name(sna_netid *id);
extern struct sna_mode_cb *sna_rm_mode_get_by_name(char *mode_name);
extern struct sna_remote_lu_cb *sna_rm_remote_lu_get_by_index(u_int32_t index);
extern struct sna_mode_cb *sna_rm_mode_get_by_index(u_int32_t index);
extern struct sna_tp_cb *sna_rm_start_tp(u_int8_t *tp_name,       
        u_int8_t *mode_name, sna_netid *remote_name, int *err);
extern struct sna_tp_cb *sna_rm_tp_get_by_pid(pid_t pid);
extern struct sna_tp_cb *sna_rm_tp_get_by_index(u_int32_t index);
extern int sna_rm_get_session(struct sna_tp_cb *tp);
extern struct sna_rcb *sna_rm_alloc_rcb(struct sna_tp_cb *tp);
extern int sna_rm_proc_attach(struct sk_buff *skb);
extern int sna_rm_proc_security(struct sk_buff *skb);
extern struct sna_rcb *sna_rm_rcb_get_by_index(u_int32_t index);

/* RSS. */
extern int sna_rss_create(struct sna_nof_node *start);
extern int sna_rss_destroy(struct sna_nof_node *delete);
extern struct sna_tg_cb *sna_rss_req_single_hop_route(sna_netid *remote_name);

/* SM. */
extern int sna_sm_create(struct sna_nof_node *node);
extern int sna_sm_destroy(struct sna_nof_node *node);
extern int sna_sm_proc_bind_req(struct sna_lfsid *lf, struct sk_buff *skb);
extern int sna_sm_proc_bind_rsp(struct sna_lfsid *lf, struct sk_buff *skb);
extern int sna_sm_proc_unbind_req(struct sna_lfsid *lf, struct sk_buff *skb);
extern int sna_sm_proc_cinit_sig_rsp(u_int32_t lulu_index, u_int32_t pc_index);
extern int sna_sm_proc_init_sig_neg_rsp(u_int32_t lulu_index);
extern int sna_sm_lu_mode_session_limit(struct sna_remote_lu_cb *remote_lu,
        struct sna_mode_cb *mode, int type, int state);
extern int sna_sm_fsm_status(struct sna_lulu_cb *lulu);
extern int sna_sm_proc_activate_session(struct sna_mode_cb *mode,
        struct sna_remote_lu_cb *remote_lu, u_int32_t tp_index, int type);
extern struct sna_lulu_cb *sna_sm_lulu_get_by_index(u_int32_t index);

/* SS. */
extern int sna_ss_create(struct sna_nof_node *start);
extern int sna_ss_destroy(struct sna_nof_node *delete);
extern int sna_ss_proc_init_sig(struct sna_init_signal *init);
extern int sna_ss_assign_pcid(struct sna_lulu_cb *lulu);

/* TC. */
extern int sna_tc_init(struct sna_hs_cb *hs);
extern int sna_tc_send_mu(struct sna_hs_cb *hs, struct sk_buff *skb);
extern int sna_tc_rx(struct sna_lfsid *lfsid, struct sk_buff *skb);

/* TDM. */
extern int sna_tdm_create(struct sna_nof_node *start);
extern int sna_tdm_destroy(struct sna_nof_node *delete);
extern void sna_tdm_init(void);
extern void sna_tdm_cleanup(void);
extern int sna_tdm_get_info(char *buffer, char **start,
        off_t offset, int length);
extern int sna_tdm_get_info_tg(char *buffer, char **start,
        off_t offset, int length);
extern int sna_tdm_define_node_chars(struct sna_nof_node *n);
extern int sna_tdm_tg_update(sna_netid *name, struct sna_tg_update *update);
struct sna_tg_cb *sna_tdm_request_tg_vectors(sna_netid *remote_name);
extern struct sna_tdm_node_cb *sna_tdm_find_node_entry(sna_netid *netid);
extern struct sna_tg_cb *sna_tdm_find_tg(struct sna_tdm_node_cb *n,
        unsigned char tg_num);
extern struct sna_tg_cb *sna_tdm_find_tg_by_id(unsigned char tg_num);
extern int sna_tdm_init_tg_update(struct sna_tg_update *tg);
extern struct sna_tg_cb *sna_tdm_find_tg_by_mac(char *mac);

/* TRS. */
extern int sna_trs_create(struct sna_nof_node *start);
extern int sna_trs_destroy(struct sna_nof_node *delete);

#endif	/* __KERNEL__ */
#endif	/* __NET_SNA_EXTERNS_H */
