/* libnof.c: Linux-SNA NOF wraper library. 
 *
 * Notes:
 * - Hopefully this will someday go away, but until Linus lets
 *   me add new system calls into the kernel we will be stuck
 *   wrapping socket calls within the SNA calls for ever.
 * - The getsockopt() call will block if necessary and we don't care what
 *   the user things about us blocking.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <syscall_pic.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>

#include <linux/if_ether.h>
#include <linux/sna.h>
#include <linux/cpic.h>

#ifndef SOL_SNA_NOF
#define SOL_SNA_NOF	278
#endif
#ifndef SOL_SNA_CPIC
#define SOL_SNA_CPIC	279
#endif

struct sna_qcpics *nof_query_cpic_side_info(int sk, char *net, 
	char *name, char *cpic_name)
{
	int numreqs = 30;
        struct cpicsconf cc;
        struct cpicsreq *cr;
        int err, n;
        struct sna_qcpics *list = NULL, *l = NULL;

        cc.cpicsc_buf = NULL;
/*
        memcpy(&lc.ls_net, net, 8);
        memcpy(&lc.ls_name, name, 8);
        memcpy(&lc.ls_name, devname, 8);
        memcpy(&lc.ls_portname, portname, 8);
        memcpy(&lc.ls_lsname, lsname, 8);
*/
        for (;;) {
                /* Total length for all structures not just snareq */
                cc.cpics_len  = sizeof(struct cpicsreq) * numreqs;
                cc.cpicsc_buf = (char *)realloc(cc.cpicsc_buf, cc.cpics_len);

                err = ioctl(sk, SIOCGCPICS, &cc);
                if (err < 0) {
                        perror("nof_query_cpic_side_info");
                        goto out;
                }

                if (cc.cpics_len == sizeof(struct cpicsreq) * numreqs) {
                        numreqs += 20;
                        continue;
                }
                break;
        }

        cr = cc.cpicsc_req;
        for (n = 0; n < cc.cpics_len; n += sizeof(struct cpicsreq)) {
                l = (struct sna_qcpics *)malloc(sizeof(struct sna_qcpics));
                memcpy(&l->data, cr, sizeof(struct cpicsreq));
                l->next = list;
                list = l;
                cr++;
        }
        err = 0;
out:    free(cc.cpicsc_buf);
	return (list);
}

struct sna_qmode *nof_query_mode(int sk, char *net, char *name, char *modename)
{
        int numreqs = 30;
        struct modeconf mc;
        struct modereq *mr;
        int err, n;
        struct sna_qmode *list = NULL, *l = NULL;

        mc.modec_buf = NULL;
/*
        memcpy(&lc.ls_net, net, 8);
        memcpy(&lc.ls_name, name, 8);
        memcpy(&lc.ls_name, devname, 8);
        memcpy(&lc.ls_portname, portname, 8);
        memcpy(&lc.ls_lsname, lsname, 8);
*/
        for (;;) {
                /* Total length for all structures not just snareq */
                mc.mode_len  = sizeof(struct modereq) * numreqs;
                mc.modec_buf = (char *)realloc(mc.modec_buf, mc.mode_len);

                err = ioctl(sk, SIOCGMODE, &mc);
                if (err < 0) {
                        perror("nof_query_mode");
                        goto out;
                }

                if (mc.mode_len == sizeof(struct modereq) * numreqs) {
                        numreqs += 20;
                        continue;
                }
                break;
        }

        mr = mc.modec_req;
        for (n = 0; n < mc.mode_len; n += sizeof(struct modereq)) {
                l = (struct sna_qmode *)malloc(sizeof(struct sna_qmode));
                memcpy(&l->data, mr, sizeof(struct modereq));
                l->next = list;
                list = l;
                mr++;
        }
        err = 0;
out:    free(mc.modec_buf);
        return (list);
}

struct sna_qplu *nof_query_plu(int sk)
{
        int numreqs = 30;
        struct pluconf pc;
        struct plureq *pr;
        int err, n;
        struct sna_qplu *list = NULL, *l = NULL;

        pc.pluc_buf = NULL;
/*
        memcpy(&lc.ls_net, net, 8);
        memcpy(&lc.ls_name, name, 8);
        memcpy(&lc.ls_name, devname, 8);
        memcpy(&lc.ls_portname, portname, 8);
        memcpy(&lc.ls_lsname, lsname, 8);
*/
        for (;;) {
                /* Total length for all structures not just snareq */
                pc.plu_len  = sizeof(struct plureq) * numreqs;
                pc.pluc_buf = (char *)realloc(pc.pluc_buf, pc.plu_len);

                err = ioctl(sk, SIOCGPLU, &pc);
                if (err < 0) {
                        perror("nof_query_plu");
                        goto out;
                }

                if (pc.plu_len == sizeof(struct plureq) * numreqs) {
                        numreqs += 20;
                        continue;
                }
                break;
        }

        pr = pc.pluc_req;
        for (n = 0; n < pc.plu_len; n += sizeof(struct plureq)) {
                l = (struct sna_qplu *)malloc(sizeof(struct sna_qplu));
                memcpy(&l->data, pr, sizeof(struct plureq));
                l->next = list;
                list = l;
                pr++;
        }
        err = 0;
out:    free(pc.pluc_buf);
        return (list);
}

struct sna_qlu *nof_query_lu(int sk, char *net, char *name, char *luname)
{
        int numreqs = 30;
        struct luconf lc;
        struct lureq *lr;
        int err, n;
        struct sna_qlu *list = NULL, *l = NULL;

        lc.luc_buf = NULL;
        memcpy(&lc.lu_net, net, 8);
        memcpy(&lc.lu_name, name, 8);
        memcpy(&lc.lu_luname, luname, 8);
        for (;;) {
                /* Total length for all structures not just snareq */
                lc.lu_len  = sizeof(struct lureq) * numreqs;
                lc.luc_buf = (char *)realloc(lc.luc_buf, lc.lu_len);

                err = ioctl(sk, SIOCGLU, &lc);
                if (err < 0) {
                        perror("nof_query_lu");
                        goto out;
                }

                if (lc.lu_len == sizeof(struct lureq) * numreqs) {
                        numreqs += 20;
                        continue;
                }
                break;
        }

        lr = lc.luc_req;
        for (n = 0; n < lc.lu_len; n += sizeof(struct lureq)) {
                l = (struct sna_qlu *)malloc(sizeof(struct sna_qlu));
                memcpy(&l->data, lr, sizeof(struct lureq));
                l->next = list;
                list = l;
                lr++;
        }
        err = 0;
out:	free(lc.luc_buf);
        return (list);
}

struct sna_qls *nof_query_ls(int sk, char *net, char *name, char *devname, char *portname,
	char *lsname)
{
        int numreqs = 30;
        struct lsconf lc;
        struct lsreq *lr;
        int err, n;
        struct sna_qls *list = NULL, *l = NULL;

        lc.lsc_buf = NULL;
        memcpy(&lc.ls_net, net, 8);
        memcpy(&lc.ls_name, name, 8);
        memcpy(&lc.ls_name, devname, 8);
        memcpy(&lc.ls_portname, portname, 8);
	memcpy(&lc.ls_lsname, lsname, 8);
        for (;;) {
                /* Total length for all structures not just snareq */
                lc.ls_len  = sizeof(struct lsreq) * numreqs;
                lc.lsc_buf = (char *)realloc(lc.lsc_buf, lc.ls_len);

                err = ioctl(sk, SIOCGLS, &lc);
                if (err < 0) {
                        perror("nof_query_ls");
                        goto out;
                }

                if (lc.ls_len == sizeof(struct lsreq) * numreqs) {
                        numreqs += 20;
                        continue;
                }
                break;
        }

        lr = lc.lsc_req;
        for (n = 0; n < lc.ls_len; n += sizeof(struct lsreq)) {
		l = (struct sna_qls *)malloc(sizeof(struct sna_qls));
                memcpy(&l->data, lr, sizeof(struct lsreq));
                l->next = list;
                list = l;
                lr++;
        }
        err = 0;
out:	free(lc.lsc_buf);
        return (list);
}

struct sna_qport *nof_query_port(int sk, char *net, char *name, char *devname, char *portname)
{
        int numreqs = 30;
        struct portconf pc;
        struct portreq *pr;
        int err, n;
	struct sna_qport *list = NULL, *p = NULL;

        pc.portc_buf = NULL;
        memcpy(&pc.port_net, net, 8);
        memcpy(&pc.port_name, name, 8);
        memcpy(&pc.port_name, devname, 8);
	memcpy(&pc.port_portname, portname, 8);
        for (;;) {
                /* Total length for all structures not just snareq */
                pc.port_len  = sizeof(struct portreq) * numreqs;
                pc.portc_buf = (char *)realloc(pc.portc_buf, pc.port_len);

                err = ioctl(sk, SIOCGPORT, &pc);
                if (err < 0) {
                        perror("nof_query_port");
                        goto out;
                }

                if (pc.port_len == sizeof(struct portreq) * numreqs) {
                        numreqs += 20;
                        continue;
                }
                break;
        }

        pr = pc.portc_req;
        for (n = 0; n < pc.port_len; n += sizeof(struct portreq)) {
		p = (struct sna_qport *)malloc(sizeof(struct sna_qport));
                memcpy(&p->data, pr, sizeof(struct portreq));
                p->next = list;
                list = p;
                pr++;
        }
        err = 0;
out:    free(pc.portc_buf);
	return (list);
}

struct sna_qcos *nof_query_cos(int sk, char *name)
{
        int numreqs = 30;
        struct cosconf cc;
        struct cosreq *cr;
        int err, n;
        struct sna_qcos *list = NULL, *c = NULL;

        cc.cosc_buf = NULL;
        memcpy(&cc.cos_name, name, SNA_RESOURCE_NAME_LEN);
        for (;;) {
                /* Total length for all structures not just snareq */
                cc.cos_len  = sizeof(struct cosreq) * numreqs;
                cc.cosc_buf = (char *)realloc(cc.cosc_buf, cc.cos_len);

                err = ioctl(sk, SIOCGCOS, &cc);
                if (err < 0) {
                        perror("nof_query_cos");
                        goto out;
                }

                if (cc.cos_len == sizeof(struct cosreq) * numreqs) {
                        numreqs += 20;
                        continue;
                }
                break;
        }

        cr = cc.cosc_req;
        for (n = 0; n < cc.cos_len; n += sizeof(struct cosreq)) {
                c = (struct sna_qcos *)malloc(sizeof(struct sna_qcos));
                memcpy(&c->data, cr, sizeof(struct cosreq));
                c->next = list;
                list = c;
                cr++;
        }
        err = 0;
out:    free(cc.cosc_buf);
        return (list);
}

struct sna_qdlc *nof_query_dlc(int sk, char *net, char *name, char *devname)
{
        int numreqs = 30;
        struct dlconf dc;
        struct dlcreq *dr;
        int err, n;
	struct sna_qdlc *list = NULL, *d = NULL;

        dc.dlc_buf = NULL;
        memcpy(&dc.dlc_net, net, 8);
        memcpy(&dc.dlc_name, name, 8);
	memcpy(&dc.dlc_name, devname, 8);
        for (;;) {
                /* Total length for all structures not just snareq */
                dc.dlc_len = sizeof(struct dlcreq) * numreqs;
                dc.dlc_buf = (char *)realloc(dc.dlc_buf, dc.dlc_len);

                err = ioctl(sk, SIOCGDLC, &dc);
                if (err < 0) {
                        perror("nof_query_dlc");
                        goto out;
                }

                if (dc.dlc_len == sizeof(struct dlcreq) * numreqs) {
                        numreqs += 20;
                        continue;
                }
                break;
        }

        dr = dc.dlc_req;
        for (n = 0; n < dc.dlc_len; n += sizeof(struct dlcreq)) {
		d = (struct sna_qdlc *)malloc(sizeof(struct sna_qdlc));
		memcpy(&d->data, dr, sizeof(struct dlcreq));
		d->next = list;
		list = d;
                dr++;
        }
        err = 0;
out:	free(dc.dlc_buf);
        return (list);
}

/* Gather all the information of a Linux-SNA node (May be wildcard for all) */
struct sna_qsna *nof_query_node(int sk, char *net, char *name)
{
        int numreqs = 10;
	struct snaconf 	sc;
	struct snareq 	*sr;
        int err, n;
	struct sna_qsna *list = NULL, *s = NULL;

        sc.snac_buf = NULL;
        memcpy(&sc.snac_net, net, 8);
        memcpy(&sc.snac_name, name, 8);
        for (;;) {
		/* Total length for all structures not just snareq */
                sc.snac_len = sizeof(struct snareq) * numreqs;
                sc.snac_buf = (char *)realloc(sc.snac_buf, sc.snac_len);

                err = ioctl(sk, SIOCGNODE, &sc);
                if (err < 0) {
                        perror("nof_query_node");
                        goto out;
                }

                if (sc.snac_len == sizeof(struct snareq) * numreqs) {
                        numreqs += 20;
                        continue;
                }
                break;
        }

	sr = sc.snac_req;
        for (n = 0; n < sc.snac_len; n += sizeof(struct snareq)) {
		s = (struct sna_qsna *)malloc(sizeof(struct sna_qsna));
                memcpy(&s->data, sr, sizeof(struct snareq));
                s->next = list;
                list = s;
                sr++;
        }
        err = 0;
out:	free(sc.snac_buf);
	return (list);
}

int nof_activate_control_sessions(int sk, 
	struct sna_activate_control_sessions *acs)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_ACTIVATE_CONTROL_SESSIONS,
                acs, sizeof(struct sna_activate_control_sessions));
        if (err < 0) {
                perror("nof_activate_control_sessions setsockopt");
                return (err);
        }
        return (0);
}

int nof_change_session_limit(int sk, 
	struct sna_change_session_limit *csl)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_CHANGE_SESSION_LIMIT,
                csl, sizeof(struct sna_change_session_limit));
        if (err < 0) {
                perror("nof_change_session_limit setsockopt");
                return (err);
        }
        return (0);
}

int nof_deactivate_control_sessions(int sk, 
	struct sna_deactivate_control_sessions *dcs)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_DEACTIVATE_CONTROL_SESSIONS,
                dcs, sizeof(struct sna_deactivate_control_sessions));
        if (err < 0) {
                perror("nof_deactivate_control_sessions setsockopt");
                return (err);
        }
        return (0);
}

int nof_define_adjacent_node(int sk, 
	struct sna_define_adjacent_node *dan)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_DEFINE_ADJACENT_NODE,
                dan, sizeof(struct sna_define_adjacent_node));
        if (err < 0) {
                perror("nof_define_adjacent_node setsockopt");
                return (err);
        }
        return (0);
}

int nof_define_class_of_service(int sk, 
	struct sna_define_cos *cos)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_DEFINE_CLASS_OF_SERVICE,
                cos, sizeof(struct sna_define_cos));
        if (err < 0) {
                perror("nof_define_class_of_service setsockopt");
                return (err);
        }
        return (0);
}

int nof_define_connection_network(int sk, 
	struct sna_define_connection_network *cn)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_DEFINE_CONNECTION_NETWORK,
                cn, sizeof(struct sna_define_connection_network));
        if (err < 0) {
                perror("nof_define_connection_network setsockopt");
                return (err);
        }
        return (0);
}

int not_define_directory_entry(int sk, 
	struct sna_define_directory_entry *de)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_DEFINE_DIRECTORY_ENTRY,
                de, sizeof(struct sna_define_directory_entry));
        if (err < 0) {
                perror("not_define_directory_entry setsockopt");
                return (err);
        }
        return (0);
}

int nof_define_isr_tuning(int sk, 
	struct sna_define_isr_tuning *it)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_DEFINE_ISR_TUNING,
                it, sizeof(struct sna_define_isr_tuning));
        if (err < 0) {
                perror("nof_define_isr_tuning setsockopt");
                return (err);
        }
        return (0);
}

/* Takes any define_link_station args, checks for errors,
 * submits to the kernel, returns the result.
 */
int nof_define_link_station(int sk,
	struct sna_define_link_station *ls)
{
        int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_DEFINE_LINK_STATION,
                ls, sizeof(struct sna_define_link_station));
        if (err < 0) {
                perror("nof_define_link_station setsockopt");
                return (err);
        }
	return (0);
}

int nof_define_local_lu(int sk, struct sna_define_local_lu *ll)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_DEFINE_LOCAL_LU,
                ll, sizeof(struct sna_define_local_lu));
        if (err < 0) {
                perror("nof_define_local_lu setsockopt");
                return (err);
        }
        return (0);
}

int nof_define_cpic_side_info(int sk, struct cpic_define_side_info *cs)
{
	int err;

	err = setsockopt(sk, SOL_SNA_CPIC, CPIC_DEFINE_SIDE,
		cs, sizeof(struct cpic_define_side_info));
	if (err < 0) {
		perror("nof_define_cpic_side_info");
		return (err);
	}
	return (0);
}

int nof_define_mode(int sk, struct sna_define_mode *dm)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_DEFINE_MODE,
                dm, sizeof(struct sna_define_mode));
        if (err < 0) {
                perror("nof_define_mode setsockopt");
                return (err);
        }
        return (0);
}

int nof_define_node_chars(int sk, 
	struct sna_define_node_chars *nc)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_DEFINE_NODE_CHARS,
                nc, sizeof(struct sna_define_node_chars));
        if (err < 0) {
                perror("nof_define_node_chars setsockopt");
                return (err);
        }
        return (0);
}

int nof_define_partner_lu(int sk, struct sna_define_partner_lu *pl)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_DEFINE_PARTNER_LU,
                pl, sizeof(struct sna_define_partner_lu));
        if (err < 0) {
                perror("nof_define_partner_lu setsockopt");
                return (err);
        }
        return (0);
}

int nof_define_port(int sk, struct sna_define_port *port)
{
        int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_DEFINE_PORT,
                port, sizeof(struct sna_define_port));
        if (err < 0) {
                perror("nof_define_port setsockopt");
                return (err);
        }
        return (0);
}

int nof_define_tp(int sk, struct sna_define_tp *tp)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_DEFINE_TP,
                tp, sizeof(struct sna_define_tp));
        if (err < 0) {
                perror("nof_define_tp setsockopt");
                return (err);
        }
        return (0);
}

int nof_delete_adjacent_node(int sk, 
	struct sna_delete_adjacent_node *dan)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_DELETE_ADJACENT_NODE,
                dan, sizeof(struct sna_delete_adjacent_node));
        if (err < 0) {
                perror("nof_delete_adjacent_node setsockopt");
                return (err);
        }
        return (0);
}

int nof_delete_class_of_service(int sk, 
	struct sna_delete_cos *dcos)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_DELETE_CLASS_OF_SERVICE,
                dcos, sizeof(struct sna_delete_cos));
        if (err < 0) {
                perror("nof_delete_class_of_service setsockopt");
                return (err);
        }
        return (0);
}

int nof_delete_connection_network(int sk, 
	struct sna_delete_connection_network *dcn)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_DELETE_CONNECTION_NETWORK,
                dcn, sizeof(struct sna_delete_connection_network));
        if (err < 0) {
                perror("nof_delete_connection_network setsockopt");
                return (err);
        }
        return (0);
}

int nof_delete_directory_entry(int sk, 
	struct sna_delete_directory_entry *dde)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_DELETE_DIRECTORY_ENTRY,
                dde, sizeof(struct sna_delete_directory_entry));
        if (err < 0) {
                perror("nof_delete_directory_entry setsockopt");
                return (err);
        }
        return (0);
}

int nof_delete_isr_tuning(int sk, 
	struct sna_delete_isr_tuning *dit)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_DELETE_ISR_TUNING,
                dit, sizeof(struct sna_delete_isr_tuning));
        if (err < 0) {
                perror("nof_delete_isr_tuning setsockopt");
                return (err);
        }
        return (0);
}

int nof_delete_link_station(int sk, struct sna_delete_link_station *dls)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_DELETE_LINK_STATION,
                dls, sizeof(struct sna_delete_link_station));
        if (err < 0) {
                perror("nof_delete_link_station setsockopt");
                return (err);
        }
        return (0);
}

int nof_delete_local_lu(int sk, struct sna_delete_local_lu *dll)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_DELETE_LOCAL_LU,
                dll, sizeof(struct sna_delete_local_lu));
        if (err < 0) {
                perror("nof_delete_local_lu setsockopt");
                return (err);
        }
        return (0);
}

int nof_delete_mode(int sk, struct sna_delete_mode *dm)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_DELETE_MODE,
                dm, sizeof(struct sna_delete_mode));
        if (err < 0) {
                perror("nof_delete_mode setsockopt");
                return (err);
        }
        return (0);
}

int nof_delete_partner_lu(int sk, struct sna_delete_partner_lu *dpl)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_DELETE_PARTNER_LU,
                dpl, sizeof(struct sna_delete_partner_lu));
        if (err < 0) {
                perror("nof_delete_partner_lu setsockopt");
                return (err);
        }
        return (0);
}

int nof_delete_port(int sk, struct sna_delete_port *dp)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_DELETE_PORT,
                dp, sizeof(struct sna_delete_port));
        if (err < 0) {
                perror("nof_delete_port setsockopt");
                return (err);
        }
        return (0);
}

int nof_delete_cpic_side_info(int sk, struct cpic_delete_side_info *dc)
{
	int err;

	err = setsockopt(sk, SOL_SNA_CPIC, CPIC_DELETE_SIDE,
		dc, sizeof(struct cpic_delete_side_info));
	if (err < 0) {
		perror("nof_delete_cpic_side_info");
		return (err);
	}
	return (0);
}

int nof_delete_tp(int sk, struct sna_delete_tp *dtp)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_DELETE_TP,
                dtp, sizeof(struct sna_delete_tp));
        if (err < 0) {
                perror("nof_delete_tp setsockopt");
                return (err);
        }
        return (0);
}

int nof_initialize_session_limit(int sk, 
	struct sna_initialize_session_limit *isl)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_INITIALIZE_SESSION_LIMIT,
                isl, sizeof(struct sna_initialize_session_limit));
        if (err < 0) {
                perror("nof_initialize_session_limit setsockopt");
                return (err);
        }
        return (0);
}

int nof_query_class_of_service(int sk, 
	struct sna_query_class_of_service *qcos)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_QUERY_CLASS_OF_SERVICE,
                qcos, sizeof(struct sna_query_class_of_service));
        if (err < 0) {
                perror("nof_query_class_of_service setsockopt");
                return (err);
        }
        return (0);
}

int nof_query_connection_network(int sk, 
	struct sna_query_connection_network *qcn)
{
	int err, size;

        err = getsockopt(sk, SOL_SNA_NOF, SNA_QUERY_CONNECTION_NETWORK,
                qcn, (size_t *)&size);
        if (err < 0) {
                perror("nof_query_connection_network getsockopt");
                return (err);
        }
        return (0);
}

int nof_query_isr_tuning(int sk, struct sna_query_isr_tuning *qit)
{
	int err, size;

        err = getsockopt(sk, SOL_SNA_NOF, SNA_QUERY_ISR_TUNING,
                qit, (size_t *)&size);
        if (err < 0) {
                perror("nof_query_isr_tuning getsockopt");
                return (err);
        }
        return (0);
}

int nof_query_stats(int sk, struct sna_query_stats *qs)
{
	int err, size;

        err = getsockopt(sk, SOL_SNA_NOF, SNA_QUERY_STATISTICS,
                qs, (size_t *)&size);
        if (err < 0) {
                perror("nof_query_stats getsockopt");
                return (err);
        }
        return (0);
}

int nof_reset_session_limit(int sk, 
	struct sna_reset_session_limit *rsl)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_RESET_SESSION_LIMIT,
                rsl, sizeof(struct sna_reset_session_limit));
        if (err < 0) {
                perror("nof_reset_session_limit setsockopt");
                return (err);
        }
        return (0);
}

int nof_start_link_station(int sk, 
	struct sna_start_link_station *sls)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_START_LINK_STATION,
                sls, sizeof(struct sna_start_link_station));
        if (err < 0) {
                perror("nof_start_link_station setsockopt");
                return (err);
        }
        return (0);
}

int nof_start_node(int sk, struct sna_start_node *start_node)
{
	int err;

	err = setsockopt(sk, SOL_SNA_NOF, SNA_START_NODE,
                start_node, sizeof(struct sna_start_node));
        if (err < 0) {
                perror("nof_start_node setsockopt");
                return (err);
        }
        return (0);
}

int nof_stop_node(int sk, struct sna_stop_node *stop_node)
{
        int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_STOP_NODE,
                stop_node, sizeof(struct sna_stop_node));
        if (err < 0) {
                perror("nof_stop_node setsockopt");
                return (err);
        }
        return (0);
}

int nof_delete_node(int sk, struct sna_delete_node *delete_node)
{
        int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_DELETE_NODE,
                delete_node, sizeof(struct sna_delete_node));
        if (err < 0) {
                perror("nof_delete_node setsockopt");
                return (err);
        }
        return (0);
}

int nof_start_port(int sk, struct sna_start_port *sp)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_START_PORT,
                sp, sizeof(struct sna_start_port));
	errno = err;
        return (err);
}

int nof_start_tp(int sk, struct sna_start_tp *stp)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_START_TP,
                stp, sizeof(struct sna_start_tp));
        if (err < 0) {
                perror("nof_start_tp setsockopt");
                return (err);
        }
        return (0);
}

int nof_stop_link_station(int sk, struct sna_stop_link_station *sls)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_STOP_LINK_STATION,
                sls, sizeof(struct sna_stop_link_station));
        if (err < 0) {
                perror("nof_stop_link_station setsockopt");
                return (err);
        }
        return (0);
}

int nof_stop_port(int sk, struct sna_stop_port *sp)
{
	int err;

        err = setsockopt(sk, SOL_SNA_NOF, SNA_STOP_PORT,
                sp, sizeof(struct sna_stop_port));
        if (err < 0) {
                perror("nof_stop_port setsockopt");
                return (err);
        }
        return (0);
}

/* 
 * Seems we have some Linux specific cruft below. 
 */
int nof_shutdown_node(int sk)
{
	setsockopt(sk, SOL_SNA_NOF, SNA_NOF_NODE_SHUTDOWN_RQ,
		NULL, 0);
	return (0);
}

int nof_query_connection_name(int sk, struct sna_query_connection_name *cn)
{
	int err, size;

	size = sizeof(struct sna_query_connection_name);
	err = getsockopt(sk, SOL_SNA_NOF, SNA_QUERY_CONNECTION_NAME,
		cn, &size);
	if (err < 0) {
		perror("nof_query_connection_name getsockopt");
		return (err);
	}
	return (0);
}
