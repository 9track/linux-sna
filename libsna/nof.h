/* nof.h:
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
 
#ifndef _NOF_H
#define _NOF_H

struct sna_qcpics *nof_query_cpic_side_info(int sk, char *net, char *name, char *cpic_name);
int nof_delete_cpic_side_info(int sk, struct sna_nof_cpic *dc);
int nof_define_cpic_side_info(int sk, struct sna_nof_cpic *cs);
struct sna_qmode *nof_query_mode(int sk, char *net, char *name, char *modename);
struct sna_qplu *nof_query_plu(int sk);
struct sna_qlu *nof_query_lu(int sk, char *net, char *name, char *luname);
struct sna_qsna *nof_query_node(int sk, char *net, char *name);
struct sna_qdlc *nof_query_dlc(int sk, char *net, char *name, char *devname);
struct sna_qport *nof_query_port(int sk, char *net, char *name, char *devname,char *portname);
struct sna_qls *nof_query_ls(int sk, char *net, char *name, char *devname, char *portname,
        char *lsname);
struct sna_qcos *nof_query_cos(int sk, char *name);

int nof_define_adjacent_node(int sk, struct sna_nof_adjacent_node *node);
int nof_define_class_of_service(int sk, struct sna_nof_cos *cos);
int nof_define_connection_network(int sk, struct sna_nof_connection_network *cn);
int not_define_directory_entry(int sk, struct sna_nof_directory_entry *dir);
int nof_define_isr_tuning(int sk, struct sna_nof_isr_tuning *isr);
int nof_define_link_station(int sk, struct sna_nof_ls *ls);
int nof_define_local_lu(int sk, struct sna_nof_local_lu *lu);
int nof_define_mode(int sk, struct sna_nof_mode *mode);
int nof_define_node(int sk, struct sna_nof_node *node);
int nof_define_partner_lu(int sk, struct sna_nof_remote_lu *lu);
int nof_define_port(int sk, struct sna_nof_port *port);
int nof_define_tp(int sk, struct sna_nof_tp *tp);

int nof_delete_adjacent_node(int sk, struct sna_nof_adjacent_node *node);
int nof_delete_class_of_service(int sk, struct sna_nof_cos *cos);
int nof_delete_connection_network(int sk, struct sna_nof_connection_network *cn);
int nof_delete_directory_entry(int sk, struct sna_nof_directory_entry *dir);
int nof_delete_isr_tuning(int sk, struct sna_nof_isr_tuning *isr);
int nof_delete_link_station(int sk, struct sna_nof_ls *ls);
int nof_delete_local_lu(int sk, struct sna_nof_local_lu *lu);
int nof_delete_mode(int sk, struct sna_nof_mode *mode);
int nof_delete_partner_lu(int sk, struct sna_nof_remote_lu *lu);
int nof_delete_port(int sk, struct sna_nof_port *port);
int nof_delete_tp(int sk, struct sna_nof_tp *dtp);
int nof_delete_node(int sk, struct sna_nof_node *node);

int nof_start_node(int sk, struct sna_nof_node *node);
int nof_start_port(int sk, struct sna_nof_port *port);
int nof_start_link_station(int sk, struct sna_nof_ls *ls);
int nof_start_tp(int sk, struct sna_nof_tp *tp);
int nof_start_remote_lu(int sk, struct sna_nof_remote_lu *lu);

int nof_stop_node(int sk, struct sna_nof_node *node);
int nof_stop_port(int sk, struct sna_nof_port *port);
int nof_stop_link_station(int sk, struct sna_nof_ls *ls);
int nof_stop_remote_lu(int sk, struct sna_nof_remote_lu *lu);
#endif	/* _NOF_H */
