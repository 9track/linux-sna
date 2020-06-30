/* sna_nof.c: Linux Systems Network Architecture implementation
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
 
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/bitops.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/socket.h>
#include <linux/sockios.h>
#include <linux/in.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/notifier.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/route.h>
#include <linux/inet.h>
#include <linux/skbuff.h>
#include <net/datalink.h>
#include <net/sock.h>
#include <linux/proc_fs.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/sna.h>
#include <linux/cpic.h>
#include <net/cpic.h>

#ifdef CONFIG_SNA_LLC
#include <net/llc_if.h>
#include <net/llc_sap.h>
#include <net/llc_pdu.h>
#include <net/llc_conn.h>
#include <linux/llc.h>
#endif	/* CONFIG_SNA_LLC */

extern int sna_cpic_create(struct sna_nof_node *node);
extern int sna_cpic_destroy(struct sna_nof_node *delete);

static LIST_HEAD(nof_list);

sna_nodeid sna_nof_find_nodeid(sna_netid *n)
{
	struct sna_nof_cb *nof;
	struct list_head *le;

	sna_debug(5, "init\n");
	list_for_each(le, &nof_list) {
		nof = list_entry(le, struct sna_nof_cb, list);
		if (!strcmp(n->net, nof->netid.net) 
			&& !strcmp(n->name, nof->netid.name))
				return nof->nodeid;
	}
	return 0;
}

struct sna_nof_cb *sna_nof_get_by_netid(sna_netid *netid)
{
        struct sna_nof_cb *nof;
	struct list_head *le;

	list_for_each(le, &nof_list) {
		nof = list_entry(le, struct sna_nof_cb, list);
		if (!strncmp(nof->netid.net, netid->net, SNA_NETWORK_NAME_LEN)
			&& !strncmp(nof->netid.name, netid->name, 
			SNA_RESOURCE_NAME_LEN))
			return nof;
        }
        return NULL;
}

static int sna_nof_node_stop(struct sna_nof_node *node)
{
	struct sna_nof_cb *nof;
	int err = -ENOENT;
	
	sna_debug(5, "init: %s\n", node->netid.name);
	nof = sna_nof_get_by_netid(&node->netid);
	if (!nof)
		goto out;
	err = 0;
	if (nof->flags & SNA_STOPPED)
		goto out;
	nof->flags &= ~SNA_RUNNING;
        nof->flags |= SNA_STOPPED;	
        sna_cs_stop(node);
out:	return err;
}

static int sna_nof_node_delete(struct sna_nof_node *node)
{
        struct sna_nof_cb *nof;
	int err = -ENOENT;

        sna_debug(5, "init: %s\n", node->netid.name);
        nof = sna_nof_get_by_netid(&node->netid);
        if (!nof)
		goto out;

	/* stop node if active. */
	if (nof->flags & SNA_RUNNING) {
		err = sna_nof_node_stop(node);
		if (err < 0)
			goto out;
	}

	err = 0;
        sna_sm_destroy(node);
        sna_trs_destroy(node);
        sna_cs_destroy(node);
        sna_ds_destroy(node);
        sna_ss_destroy(node);
        sna_asm_destroy(node);

        /* finally delete this node. */
        list_del(&nof->list);
        kfree(nof);

        sna_mod_dec_use_count();
out:    return err;
}

static int sna_nof_node_start(struct sna_nof_node *node)
{
	struct sna_nof_cb *nof;
	int err = -ENOENT;

	sna_debug(5, "init: %s\n", node->netid.name);
	nof = sna_nof_get_by_netid(&node->netid);
        if (!nof)
                goto out;
	err = -EUSERS;
	if (nof->flags & SNA_RUNNING)
		goto out;
	err = 0;
	nof->flags &= ~SNA_STOPPED;
        nof->flags |= SNA_RUNNING;
	sna_cs_start(node);
out:	return err;
}

static int sna_nof_node_define(struct sna_nof_node *start)
{
        struct sna_nof_cb *nof;
	int err = -EEXIST;

        sna_debug(5, "init: %s\n", start->netid.name);
        if (sna_nof_get_by_netid(&start->netid))
		goto out;
	err = sna_tdm_define_node_chars(start);
        if (err < 0)
		goto out;
        new(nof, GFP_ATOMIC);
        if (!nof) {
		err = -ENOMEM;
		goto out;
	}
	err = 0;
	memcpy(&nof->netid, &start->netid, sizeof(sna_netid));
        nof->nodeid     = start->nodeid;
        nof->type       = start->type;
        nof->lu_seg     = start->lu_seg;
        nof->bind_seg   = start->bind_seg;
        nof->max_lus    = start->max_lus;
	nof->flags	= SNA_UP | SNA_STOPPED;
        list_add_tail(&nof->list, &nof_list);

	sna_asm_create(start);
        sna_ss_create(start);
        sna_ds_create(start);
        sna_cs_create(start);
        sna_trs_create(start);
        sna_sm_create(start);

#ifdef NOT
        nof->netid_registered                   = start->netid_registered;
        nof->ls_supp_type                       = start->ls_supp_type;
        nof->resource_registration              = start->resource_registration;
        nof->segment_generation_lvl             = start->segment_generation_lvl;
        nof->mode_to_cos_mapping                = start->mode_to_cos_mapping;
        nof->ms_node_type                       = start->ms_node_type;
        nof->mj_vector_file                     = start->mj_vector_file;
        nof->ms_log_file                        = start->ms_log_file;
        nof->peer_resource_registration
                = start->peer_resource_registration;
        nof->network_node_type                  = start->network_node_type;
        nof->directory_type_supp                = start->directory_type_supp;
        nof->rs_tree_update_type                = start->rs_tree_update_type;
        nof->tdm_node_name                      = start->tdm_node_name;
        nof->cosdm_node_name                    = start->cosdm_node_name;
        nof->max_rs_cache_trees                 = start->max_rs_cache_trees;
        nof->max_oos_tdm_updates                = start->max_oos_tdm_updates;
        nof->resource_service_search
                = start->resource_service_search;
        nof->general_odai_usage_supp
                = start->general_odai_usage_supp;
#endif
	sna_mod_inc_use_count();
out:	return err;
}

static int sna_nof_node(struct sna_nof_node *node)
{
	int err = -EINVAL;

	sna_debug(5, "init\n");
	switch (node->action) {
		case SNA_NOF_DEFINE:
			err = sna_nof_node_define(node);
			break;
		case SNA_NOF_START:
			err = sna_nof_node_start(node);
			break;
		case SNA_NOF_STOP:
			err = sna_nof_node_stop(node);
			break;
		case SNA_NOF_DELETE:
			err = sna_nof_node_delete(node);
			break;
		default:
                        sna_debug(5, "unknown action `%d'\n", node->action);
                        break;
	}
	return err;
}

static int sna_nof_mode(struct sna_nof_mode *mode)
{
	int err = -EINVAL;

	sna_debug(5, "init\n");
	switch (mode->action) {
		case SNA_NOF_DEFINE:
			err = sna_rm_define_mode(mode);
			break;
		case SNA_NOF_DELETE:
//			err = sna_rm_delete_mode(mode);
			break;
		default:
			sna_debug(5, "unknown action `%d'\n", mode->action);
                        break;
	}
	return err;
}

static int sna_nof_adjacent_node(struct sna_nof_adjacent_node *node)
{
	return -EINVAL;
}

static int sna_nof_cos(struct sna_nof_cos *cos)
{
	int err = -EINVAL;

	sna_debug(5, "init\n");
	switch (cos->action) {
		case SNA_NOF_DEFINE:
			err = sna_cosm_define_cos(cos);
			break;
		case SNA_NOF_DELETE:
			err = sna_cosm_delete_cos(cos);
			break;
		default:
                        sna_debug(5, "unknown action `%d'\n", cos->action);
                        break;
	}
	return err;
}

static int sna_nof_connection_network(struct sna_nof_connection_network *cn)
{
	return -EINVAL;
}

static int sna_nof_directory_entry(struct sna_nof_directory_entry *dir)
{
	return -EINVAL;
}

static int sna_nof_isr_tuning(struct sna_nof_isr_tuning *isr)
{
	return -EINVAL;
}

static int sna_nof_local_lu(struct sna_nof_local_lu *lu)
{
	int err = -EINVAL;

	sna_debug(5, "init\n");
	switch (lu->action) {
		case SNA_NOF_DEFINE:
			err = sna_rm_define_local_lu(lu);
			break;
		case SNA_NOF_DELETE:
			err = sna_rm_delete_local_lu(lu);
			break;
		default:
                        sna_debug(5, "unknown action `%d'\n", lu->action);
                        break;
	}
	return err;
}

static int sna_nof_remote_lu(struct sna_nof_remote_lu *lu)
{
	int err = -EINVAL;

	sna_debug(5, "init\n");
	switch (lu->action) {
                case SNA_NOF_DEFINE:
                        err = sna_rm_define_remote_lu(lu);
                        break;
                case SNA_NOF_DELETE:
                        err = sna_rm_delete_remote_lu(lu);
                        break;
		case SNA_NOF_START:	/* method to test bind flow. */
                        err = sna_rm_start_remote_lu(lu);
                        break;
		case SNA_NOF_STOP:
			err = sna_rm_stop_remote_lu(lu);
			break;
                default:
                        sna_debug(5, "unknown action `%d'\n", lu->action);
                        break;
        }
	return err;
}

static int sna_nof_tp(struct sna_nof_tp *tp)
{
	return -EINVAL;
}

static int sna_nof_ls(struct sna_nof_ls *ls)
{
	int err = -EINVAL;
	
	sna_debug(5, "init\n");
	switch (ls->action) {
                case SNA_NOF_DEFINE:
                        err = sna_cs_define_ls(ls);
                        break;
                case SNA_NOF_START:
                        err = sna_cs_start_ls(ls);
                        break;
                case SNA_NOF_STOP:
                        err = sna_cs_stop_ls(ls);
                        break;
                case SNA_NOF_DELETE:
                        err = sna_cs_delete_ls(ls);
                        break;
                default:
                        sna_debug(5, "unknown action `%d'\n", ls->action);
                        break;
        }
        return err;
}

static int sna_nof_port(struct sna_nof_port *port)
{
	int err = -EINVAL;
	
	sna_debug(5, "init\n");
	switch (port->action) {
		case SNA_NOF_DEFINE:
			err = sna_cs_define_port(port);
			break;
		case SNA_NOF_START:
			err = sna_cs_start_port(port);
			break;
		case SNA_NOF_STOP:
			err = sna_cs_stop_port(port);
			break;
		case SNA_NOF_DELETE:
			err = sna_cs_delete_port(port);
			break;
		default:
			sna_debug(5, "unknown action `%d'\n", port->action);
			break;
	}
	return err;
}

#ifdef CONFIG_PROC_FS
int sna_nof_get_info(char *buffer, char **start, off_t offset, int length)
{
        off_t pos = 0, begin = 0;
	struct sna_nof_cb *nof;
	struct list_head *le;
        int len = 0;

        /* Output the NOF data for the /proc filesystem. */
        len += sprintf(buffer, "%-17s%-11s%-5s%-7s%-7s%-9s%-8s\n", 
		"name", "blk.id_num", "type", "status",
		"lu_seg", "bind_seg", "max_lus");

	list_for_each(le, &nof_list) {
		nof = list_entry(le, struct sna_nof_cb, list);
		len += sprintf(buffer + len, "%-17s%-11s",
			sna_pr_netid(&nof->netid), sna_pr_nodeid(nof->nodeid));
		len += sprintf(buffer + len, "%02X   %02X     %-7d%-9d%-8ld\n",
			nof->type, nof->flags, nof->lu_seg,
			nof->bind_seg, nof->max_lus);

                /* Are we still dumping unwanted data then discard the record */
		pos = begin + len;
                if (pos < offset) {
                        len = 0;        /* Keep dumping into the buffer start */
			begin = pos;
                }
                if (pos > offset + length)       /* We have dumped enough */
                        break;
        }

        /* The data in question runs from begin to begin+len */
        *start = buffer + (offset - begin);     /* Start of wanted data */
        len -= (offset - begin);   /* Remove unwanted header data from length */
	if (len > length)
                len = length;      /* Remove unwanted tail data from length */
        if (len < 0)
                len = 0;
        return len;
}
#endif

int sna_nof_setsockopt(struct socket *sock, int level, int optname,
        char *optval, int optlen)
{
        int err;

	sna_debug(5, "init\n");
        switch (optname) {
                case SNA_NOF_ADJACENT_NODE: {
                        struct sna_nof_adjacent_node node;
                        if (optlen < sizeof(struct sna_nof_adjacent_node))
                                return -EINVAL;
                        if (copy_from_user(&node, optval, sizeof(node)))
                                return -EFAULT;
                        err = sna_nof_adjacent_node(&node);
                        break;
                }

                case SNA_NOF_COS: {
                        struct sna_nof_cos cos;
                        if (optlen < sizeof(struct sna_nof_cos))
                                return -EINVAL;
                        if (copy_from_user(&cos, optval, sizeof(cos)))
                                return -EFAULT;
                        err = sna_nof_cos(&cos);
                        break;
                }

                case SNA_NOF_ISR_TUNING: {
                        struct sna_nof_isr_tuning isr;
                        if (optlen < sizeof(struct sna_nof_isr_tuning))
                                return -EINVAL;
                        if (copy_from_user(&isr, optval, sizeof(isr)))
                                return -EFAULT;
                        err = sna_nof_isr_tuning(&isr);
                        break;
                }

                case SNA_NOF_LOCAL_LU: {
                        struct sna_nof_local_lu lu;
                        if (optlen < sizeof(struct sna_nof_local_lu))
                                return -EINVAL;
                        if (copy_from_user(&lu, optval, sizeof(lu)))
                                return -EFAULT;
                        err = sna_nof_local_lu(&lu);
                        break;
                }

                case SNA_NOF_MODE: {
                        struct sna_nof_mode mode;
                        if (optlen < sizeof(struct sna_nof_mode))
                                return -EINVAL;
                        if (copy_from_user(&mode, optval, sizeof(mode)))
                                return -EFAULT;
                       	err = sna_nof_mode(&mode);
                        break;
                }

                case SNA_NOF_REMOTE_LU: {
                        struct sna_nof_remote_lu lu;
                        if (optlen < sizeof(struct sna_nof_remote_lu))
                                return -EINVAL;
                        if (copy_from_user(&lu, optval, sizeof(lu)))
                                return -EFAULT;
                        err = sna_nof_remote_lu(&lu);
                        break;
                }

                case SNA_NOF_TP: {
                        struct sna_nof_tp tp;
                        if (optlen < sizeof(struct sna_nof_tp))
                                return -EINVAL;
                        if (copy_from_user(&tp, optval, sizeof(tp)))
                                return -EFAULT;
                        err = sna_nof_tp(&tp);
                        break;
                }

                case SNA_NOF_CONNECTION_NETWORK: {
                        struct sna_nof_connection_network cn;
                        if (optlen < sizeof(struct sna_nof_connection_network))
                                return -EINVAL;
                        if (copy_from_user(&cn, optval, sizeof(cn)))
                                return -EFAULT;
                        err = sna_nof_connection_network(&cn);
                        break;
                }

                case SNA_NOF_DIRECTORY_ENTRY: {
                        struct sna_nof_directory_entry dir;
                        if (optlen < sizeof(struct sna_nof_directory_entry))
                                return -EINVAL;
                        if (copy_from_user(&dir, optval, sizeof(dir)))
                                return -EFAULT;
                        err = sna_nof_directory_entry(&dir);
                        break;
                }

                case SNA_NOF_LS: {
                        struct sna_nof_ls ls;
                        if (optlen < sizeof(struct sna_nof_ls))
                                return -EINVAL;
                        if (copy_from_user(&ls, optval, sizeof(ls)))
                                return -EFAULT;
                        err = sna_nof_ls(&ls);
                        break;
                }

                case SNA_NOF_NODE: {
                        struct sna_nof_node node;
                        if (optlen < sizeof(struct sna_nof_node))
                                return -EINVAL;
                        if (copy_from_user(&node, optval, sizeof(node)))
                                return -EFAULT;
                        err = sna_nof_node(&node);
                        break;
                }

                case SNA_NOF_PORT: {
                        struct sna_nof_port port;
                        if (optlen < sizeof(struct sna_nof_port))
                                return -EINVAL;
                        if (copy_from_user(&port, optval, sizeof(port)))
                                return -EFAULT;
                        err = sna_nof_port(&port);
                        break;
                }

                default:
			sna_debug(5, "unknown component `%d'.\n", optname);
                        return -EINVAL;
        }
	return err;
}

int sna_nof_getsockopt(struct socket *sock, int level, int optname,
        char *optval, int *optlen)
{
	return -EINVAL;
}

int sna_nof_ginfo(struct sna_nof_cb *nof, char *buf, int len)
{
	struct snareq sr;
	int done = 0;

	sna_debug(10, "init\n");
        if (!buf) {
                done += sizeof(sr);
                return done;
        }
        if (len < (int)sizeof(sr))
                return done;
        memset(&sr, 0, sizeof(struct snareq));

        /* Move the data here */
	strncpy(sr.net, nof->netid.net, 8);
	strncpy(sr.name, nof->netid.name, 8);
	sr.type 	= nof->type;
	sr.lu_seg	= nof->lu_seg;
	sr.bind_seg	= nof->bind_seg;
	sr.max_lus	= nof->max_lus;
	sr.node_status	= nof->flags;
	sr.nodeid	= nof->nodeid;

	if (copy_to_user(buf, &sr, sizeof(struct snareq)))
                return -EFAULT;
        buf  += sizeof(struct snareq);
        len  -= sizeof(struct snareq);
        done += sizeof(struct snareq);
        return done;
}
	
int sna_nof_query_node(char *arg)
{
	struct sna_nof_cb *nof;
	int len, total, done;
	struct list_head *le;
	struct snaconf sc;
	char *pos;

	sna_debug(10, "init\n");
	if (copy_from_user(&sc, arg, sizeof(sc)))
                return -EFAULT;
	pos = sc.snac_buf;
        len = sc.snac_len;

        /*
         * Get the data and put it into the structure
         */
        total = 0;
	list_for_each(le, &nof_list) {
		nof = list_entry(le, struct sna_nof_cb, list);
                if (pos == NULL)
                        done = sna_nof_ginfo(nof, NULL, 0);
                else
                        done = sna_nof_ginfo(nof, pos + total, len - total);
                if (done < 0)
                        return -EFAULT;
                total += done;
	}

	sc.snac_len = total;
	if (copy_to_user(arg, &sc, sizeof(sc)))
                return -EFAULT;
	return 0;
}

int sna_nof_ioctl(int cmd, void *arg)
{
	sna_debug(5, "init\n");
        switch (cmd) {
		case SIOCGLS:
			sna_cs_query_ls(arg);
			break;

		case SIOCGPORT:
			sna_cs_query_port(arg);
			break;

		case SIOCGDLC:
			sna_cs_query_dlc(arg);
			break;

                case SIOCGNODE:
			sna_nof_query_node(arg);
			break;

		case SIOCGMODE:
			sna_rm_query_mode(arg);
			break;

		case SIOCGLU:
			sna_rm_query_lu(arg);
			break;

		case SIOCGPLU:
			sna_rm_query_plu(arg);
			break;

		case SIOCGCOS:
			sna_cosm_query_cos(arg);
			break;

                default:
                        return -EINVAL;
        }
        return 0;
} 
