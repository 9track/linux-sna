struct sna_qcpics *nof_query_cpic_side_info(int sk, char *net, char *name, char *cpic_name);
int nof_delete_cpic_side_info(int sk, struct cpic_delete_side_info *dc);
int nof_define_cpic_side_info(int sk, struct cpic_define_side_info *cs);
struct sna_qmode *nof_query_mode(int sk, char *net, char *name, char *modename);
struct sna_qplu *nof_query_plu(int sk);
struct sna_qlu *nof_query_lu(int sk, char *net, char *name, char *luname);
struct sna_qsna *nof_query_node(int sk, char *net, char *name);
struct sna_qdlc *nof_query_dlc(int sk, char *net, char *name, char *devname);
struct sna_qport *nof_query_port(int sk, char *net, char *name, char *devname,char *portname);
struct sna_qls *nof_query_ls(int sk, char *net, char *name, char *devname, char *portname,
        char *lsname);
struct sna_qcos *nof_query_cos(int sk, char *name);
int nof_activate_control_sessions(int sk,
        struct sna_activate_control_sessions *acs);
int nof_change_session_limit(int sk,
        struct sna_change_session_limit *csl);
int nof_deactivate_control_sessions(int sk,
        struct sna_deactivate_control_sessions *dcs);
int nof_define_adjacent_node(int sk,
        struct sna_define_adjacent_node *dan);
int nof_define_class_of_service(int sk,
        struct sna_define_cos *cos);
int nof_define_connection_network(int sk,
        struct sna_define_connection_network *cn);
int not_define_directory_entry(int sk,
        struct sna_define_directory_entry *de);
int nof_define_isr_tuning(int sk,
        struct sna_define_isr_tuning *it);
int nof_define_link_station(int sk,
        struct sna_define_link_station *ls);
int nof_define_local_lu(int sk,
        struct sna_define_local_lu *ll);
int nof_define_mode(int sk,
        struct sna_define_mode *dm);
int nof_define_node_chars(int sk,
        struct sna_define_node_chars *nc);
int nof_define_partner_lu(int sk,
        struct sna_define_partner_lu *pl);
int nof_define_port(int sk, struct sna_define_port *port);
int nof_define_tp(int sk, struct sna_define_tp *tp);
int nof_delete_adjacent_node(int sk,
        struct sna_delete_adjacent_node *dan);
int nof_delete_class_of_service(int sk,
        struct sna_delete_cos *dcos);
int nof_delete_connection_network(int sk,
        struct sna_delete_connection_network *dcn);
int nof_delete_directory_entry(int sk,
        struct sna_delete_directory_entry *dde);
int nof_delete_isr_tuning(int sk,
        struct sna_delete_isr_tuning *dit);
int nof_delete_link_station(int sk,
        struct sna_delete_link_station *dls);
int nof_delete_local_lu(int sk,
        struct sna_delete_local_lu *dll);
int nof_delete_mode(int sk,
        struct sna_delete_mode *dm);
int nof_delete_partner_lu(int sk,
        struct sna_delete_partner_lu *dpl);
int nof_delete_port(int sk,
        struct sna_delete_port *dp);
int nof_delete_tp(int sk, struct sna_delete_tp *dtp);
int nof_initialize_session_limit(int sk,
        struct sna_initialize_session_limit *isl);
int nof_query_class_of_service(int sk,
        struct sna_query_class_of_service *qcos);
int nof_query_connection_network(int sk,
        struct sna_query_connection_network *qcn);
int nof_query_isr_tuning(int sk, struct sna_query_isr_tuning *qit);
int nof_query_statistics(int sk,
        struct sna_query_stats *qs);
int nof_reset_session_limit(int sk,
        struct sna_reset_session_limit *rsl);
int nof_start_link_station(int sk,
        struct sna_start_link_station *sls);
int nof_start_node(int sk, struct sna_start_node *start_node);
int nof_stop_node(int sk, struct sna_stop_node *stop_node);
int nof_delete_node(int sk, struct sna_delete_node *delete_node);
int nof_start_port(int sk, struct sna_start_port *sp);
int nof_start_tp(int sk, struct sna_start_tp *stp);
int nof_stop_link_station(int sk, struct sna_stop_link_station *sls);
int nof_stop_port(int sk, struct sna_stop_port *sp);
