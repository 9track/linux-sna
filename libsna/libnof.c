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

#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>

#include <asm/byteorder.h>
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

int nof_define_adjacent_node(int sk, struct sna_nof_adjacent_node *node)
{
	int err;

	node->action = SNA_NOF_DEFINE;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_ADJACENT_NODE, 
		node, sizeof(*node));
        if (err < 0) {
                perror("nof_define_adjacent_node setsockopt");
                return (err);
        }
        return (0);
}

int nof_define_class_of_service(int sk, struct sna_nof_cos *cos)
{
	int err;

	cos->action = SNA_NOF_DEFINE;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_COS, cos, sizeof(*cos));
        if (err < 0) {
                perror("nof_define_class_of_service setsockopt");
                return (err);
        }
        return (0);
}

int nof_define_connection_network(int sk, struct sna_nof_connection_network *cn)
{
	int err;

	cn->action = SNA_NOF_DEFINE;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_CONNECTION_NETWORK,
                cn, sizeof(*cn));
        if (err < 0) {
                perror("nof_define_connection_network setsockopt");
                return (err);
        }
        return (0);
}

int not_define_directory_entry(int sk, struct sna_nof_directory_entry *dir)
{
	int err;

	dir->action = SNA_NOF_DEFINE;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_DIRECTORY_ENTRY, dir, sizeof(*dir));
        if (err < 0) {
                perror("not_define_directory_entry setsockopt");
                return (err);
        }
        return (0);
}

int nof_define_isr_tuning(int sk, struct sna_nof_isr_tuning *isr)
{
	int err;

	isr->action = SNA_NOF_DEFINE;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_ISR_TUNING, isr, sizeof(*isr));
        if (err < 0) {
                perror("nof_define_isr_tuning setsockopt");
                return (err);
        }
        return (0);
}

int nof_define_link_station(int sk, struct sna_nof_ls *ls)
{
        int err;

	ls->action = SNA_NOF_DEFINE;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_LS, ls, sizeof(*ls));
        if (err < 0) {
                perror("nof_define_link_station setsockopt");
                return (err);
        }
	return (0);
}

int nof_define_local_lu(int sk, struct sna_nof_local_lu *lu)
{
	int err;

	lu->action = SNA_NOF_DEFINE;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_LOCAL_LU, lu, sizeof(*lu));
        if (err < 0) {
                perror("nof_define_local_lu setsockopt");
                return (err);
        }
        return (0);
}

int nof_define_cpic_side_info(int sk, struct sna_nof_cpic *cs)
{
	int err;

	cs->action = SNA_NOF_DEFINE;
	err = setsockopt(sk, SOL_SNA_CPIC, SNA_NOF_CPIC,
		cs, sizeof(struct sna_nof_cpic));
	if (err < 0) {
		perror("nof_define_cpic_side_info");
		return (err);
	}
	return (0);
}

int nof_define_mode(int sk, struct sna_nof_mode *mode)
{
	int err;

	mode->action = SNA_NOF_DEFINE;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_MODE, mode, sizeof(*mode));
        if (err < 0) {
                perror("nof_define_mode setsockopt");
                return (err);
        }
        return (0);
}

int nof_define_node(int sk, struct sna_nof_node *node)
{
	int err;

	node->action = SNA_NOF_DEFINE;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_NODE, node, sizeof(*node));
        if (err < 0) {
                perror("nof_define_node_chars setsockopt");
                return (err);
        }
        return (0);
}

int nof_stop_remote_lu(int sk, struct sna_nof_remote_lu *lu)
{
        int err;

        lu->action = SNA_NOF_STOP;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_REMOTE_LU, lu, sizeof(*lu));
        if (err < 0) {
                perror("nof_sop_remote_lu setsockopt");
                return (err);
        }
        return (0);
}

int nof_start_remote_lu(int sk, struct sna_nof_remote_lu *lu)
{
	int err;

	lu->action = SNA_NOF_START;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_REMOTE_LU, lu, sizeof(*lu));
        if (err < 0) {
                perror("nof_define_partner_lu setsockopt");
                return (err);
        }
        return (0);
}

int nof_define_partner_lu(int sk, struct sna_nof_remote_lu *lu)
{
	int err;

	lu->action = SNA_NOF_DEFINE;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_REMOTE_LU, lu, sizeof(*lu));
        if (err < 0) {
                perror("nof_define_partner_lu setsockopt");
                return (err);
        }
        return (0);
}

int nof_define_port(int sk, struct sna_nof_port *port)
{
        int err;

	port->action = SNA_NOF_DEFINE;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_PORT, port, sizeof(*port));
        if (err < 0) {
                perror("nof_define_port setsockopt");
                return err;
        }
        return 0;
}

int nof_define_tp(int sk, struct sna_nof_tp *tp)
{
	int err;

	tp->action = SNA_NOF_DEFINE;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_TP, tp, sizeof(*tp));
        if (err < 0) {
                perror("nof_define_tp setsockopt");
                return (err);
        }
        return (0);
}

int nof_delete_adjacent_node(int sk, struct sna_nof_adjacent_node *node)
{
	int err;

	node->action = SNA_NOF_DELETE;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_ADJACENT_NODE,
		node, sizeof(*node));
        if (err < 0) {
                perror("nof_delete_adjacent_node setsockopt");
                return (err);
        }
        return (0);
}

int nof_delete_class_of_service(int sk, struct sna_nof_cos *cos)
{
	int err;

	cos->action = SNA_NOF_DELETE;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_COS, cos, sizeof(*cos));
        if (err < 0) {
                perror("nof_delete_class_of_service setsockopt");
                return (err);
        }
        return (0);
}

int nof_delete_connection_network(int sk, struct sna_nof_connection_network *cn)
{
	int err;

	cn->action = SNA_NOF_DELETE;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_CONNECTION_NETWORK,
                cn, sizeof(*cn));
        if (err < 0) {
                perror("nof_delete_connection_network setsockopt");
                return (err);
        }
        return (0);
}

int nof_delete_directory_entry(int sk, struct sna_nof_directory_entry *dir)
{
	int err;

	dir->action = SNA_NOF_DELETE;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_DIRECTORY_ENTRY, dir, sizeof(*dir));
        if (err < 0) {
                perror("nof_delete_directory_entry setsockopt");
                return (err);
        }
        return (0);
}

int nof_delete_isr_tuning(int sk, struct sna_nof_isr_tuning *isr)
{
	int err;

	isr->action = SNA_NOF_DELETE;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_ISR_TUNING, isr, sizeof(*isr));
        if (err < 0) {
                perror("nof_delete_isr_tuning setsockopt");
                return (err);
        }
        return (0);
}

int nof_delete_link_station(int sk, struct sna_nof_ls *ls)
{
	int err;

	ls->action = SNA_NOF_DELETE;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_LS, ls, sizeof(*ls));
        if (err < 0) {
                perror("nof_delete_link_station setsockopt");
                return (err);
        }
        return (0);
}

int nof_delete_local_lu(int sk, struct sna_nof_local_lu *lu)
{
	int err;

	lu->action = SNA_NOF_DELETE;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_LOCAL_LU, lu, sizeof(*lu));
        if (err < 0) {
                perror("nof_delete_local_lu setsockopt");
                return (err);
        }
        return (0);
}

int nof_delete_mode(int sk, struct sna_nof_mode *mode)
{
	int err;

	mode->action = SNA_NOF_DELETE;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_MODE, mode, sizeof(*mode));
        if (err < 0) {
                perror("nof_delete_mode setsockopt");
                return (err);
        }
        return (0);
}

int nof_delete_partner_lu(int sk, struct sna_nof_remote_lu *lu)
{
	int err;

	lu->action = SNA_NOF_DELETE;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_REMOTE_LU, lu, sizeof(*lu));
        if (err < 0) {
                perror("nof_delete_partner_lu setsockopt");
                return (err);
        }
        return (0);
}

int nof_delete_port(int sk, struct sna_nof_port *port)
{
	int err;

	port->action = SNA_NOF_DELETE;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_PORT, port, sizeof(*port));
        if (err < 0) {
                perror("nof_delete_port setsockopt");
                return (err);
        }
        return (0);
}

int nof_delete_cpic_side_info(int sk, struct sna_nof_cpic *dc)
{
	int err;

	dc->action = SNA_NOF_DELETE;
	err = setsockopt(sk, SOL_SNA_CPIC, SNA_NOF_CPIC,
		dc, sizeof(struct sna_nof_cpic));
	if (err < 0) {
		perror("nof_delete_cpic_side_info");
		return (err);
	}
	return (0);
}

int nof_delete_tp(int sk, struct sna_nof_tp *tp)
{
	int err;

	tp->action = SNA_NOF_DELETE;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_TP, tp, sizeof(*tp));
        if (err < 0) {
                perror("nof_delete_tp setsockopt");
                return (err);
        }
        return (0);
}

int nof_start_link_station(int sk, struct sna_nof_ls *ls)
{
	int err;

	ls->action = SNA_NOF_START;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_LS, ls, sizeof(*ls));
        if (err < 0) {
                perror("nof_start_link_station setsockopt");
                return (err);
        }
        return (0);
}

int nof_start_node(int sk, struct sna_nof_node *node)
{
	int err;

	node->action = SNA_NOF_START;
	err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_NODE, node, sizeof(*node));
        if (err < 0) {
                perror("nof_start_node setsockopt");
                return (err);
        }
        return (0);
}

int nof_stop_node(int sk, struct sna_nof_node *node)
{
        int err;

	node->action = SNA_NOF_STOP;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_NODE, node, sizeof(*node));
        if (err < 0) {
                perror("nof_stop_node setsockopt");
                return (err);
        }
        return (0);
}

int nof_delete_node(int sk, struct sna_nof_node *node)
{
        int err;

	node->action = SNA_NOF_DELETE;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_NODE, node, sizeof(*node));
        if (err < 0) {
                perror("nof_delete_node setsockopt");
                return (err);
        }
        return (0);
}

int nof_start_port(int sk, struct sna_nof_port *port)
{
	int err;
	port->action = SNA_NOF_START;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_PORT, port, sizeof(*port));
	errno = err;
        return (err);
}

int nof_start_tp(int sk, struct sna_nof_tp *tp)
{
	int err;

	tp->action = SNA_NOF_START;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_TP, tp, sizeof(*tp));
        if (err < 0) {
                perror("nof_start_tp setsockopt");
                return (err);
        }
        return (0);
}

int nof_stop_link_station(int sk, struct sna_nof_ls *ls)
{
	int err;

	ls->action = SNA_NOF_STOP;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_LS, ls, sizeof(*ls));
        if (err < 0) {
                perror("nof_stop_link_station setsockopt");
                return (err);
        }
        return (0);
}

int nof_stop_port(int sk, struct sna_nof_port *port)
{
	int err;

	port->action = SNA_NOF_STOP;
        err = setsockopt(sk, SOL_SNA_NOF, SNA_NOF_PORT, port, sizeof(*port));
        if (err < 0) {
                perror("nof_stop_port setsockopt");
                return (err);
        }
        return (0);
}
