/* sna_ctrl.c:
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>

/* required for linux-SNA. */
#include <asm/byteorder.h>
#include <sys/socket.h>
#include <linux/netdevice.h>
#include <linux/sna.h>
#include <linux/cpic.h>
#include <nof.h>

/* our stuff. */
#include "sna_list.h"
#include "snaconfig.h"

extern char     version_s[];
extern char     name_s[];
extern char     desc_s[];
extern char     maintainer_s[];
extern char     company_s[];
extern char     web_s[];
extern int      sna_debug_level;
extern global_info *sna_config_info;

static int sna_start_help(void)
{
	printf("Usage: %s start <service> <name>\n", name_s);
        exit(1);
}

static int sna_start_dlc(char *name)
{
	struct sna_nof_port *port;
	int err, sk;

	sk = sna_nof_connect();
        if (sk < 0)
                return sk;
	if (!new(port))
		return -ENOMEM;
	strcpy(port->use_name, name);
	err = nof_start_port(sk, port);
	if (err < 0)
		sna_debug(1, "start dlc failed `%d: %s'.\n", err, strerror(errno));
	free(port);
	sna_nof_disconnect(sk);
	return err;
}

static int sna_start_link(char *name)
{
	struct sna_nof_ls *ls;
	int err, sk;

	sk = sna_nof_connect();
        if (sk < 0)
                return sk;
        if (!new(ls))
                return -ENOMEM;
        strcpy(ls->use_name, name);
        err = nof_start_link_station(sk, ls);
	if (err < 0)
		sna_debug(1, "start link station failed `%d: %s'.\n", err, strerror(errno));
        free(ls);
	sna_nof_disconnect(sk);
        return err;
}

static int sna_start_lu(char *name)
{
	struct sna_nof_remote_lu *lu;
	int err, sk;

	sk = sna_nof_connect();
        if (sk < 0)
                return sk;
        if (!new(lu))
                return -ENOMEM;
	strcpy(lu->use_name, name);
        err = nof_start_remote_lu(sk, lu);
        if (err < 0)
                sna_debug(1, "start lu failed `%d: %s'.\n", err, strerror(errno));
        free(lu);
        sna_nof_disconnect(sk);
        return err;
}

static int sna_stop_lu(char *name)
{
        struct sna_nof_remote_lu *lu;
        int err, sk;

        sk = sna_nof_connect();
        if (sk < 0)
                return sk;
        if (!new(lu))
                return -ENOMEM;
	strcpy(lu->use_name, name);
        err = nof_stop_remote_lu(sk, lu);
        if (err < 0)
                sna_debug(1, "stop lu failed `%d: %s'.\n", err, strerror(errno));
        free(lu);
        sna_nof_disconnect(sk);
        return err;
}

static int sna_stop_dlc(char *name)
{
        struct sna_nof_port *port;
        int err, sk;

        sk = sna_nof_connect();
        if (sk < 0)
                return sk;
        if (!new(port))
                return -ENOMEM;
        strcpy(port->use_name, name);
        err = nof_stop_port(sk, port);
        if (err < 0)
                sna_debug(1, "stop port failed `%d: %s'.\n", err, strerror(errno));
        free(port);
        sna_nof_disconnect(sk);
        return err;
}

static int sna_stop_link(char *name)
{
        struct sna_nof_ls *ls;
        int err, sk;

        sk = sna_nof_connect();
        if (sk < 0)
                return sk;
        if (!new(ls))
                return -ENOMEM;
        strcpy(ls->use_name, name);
        err = nof_stop_link_station(sk, ls);
        if (err < 0)
                sna_debug(1, "stop link station failed `%d: %s'.\n", err, strerror(errno));
        free(ls);
        sna_nof_disconnect(sk);
        return err;
}

static int sna_delete_dlc(char *name)
{
        struct sna_nof_port *port;
        int err, sk;

        sk = sna_nof_connect();
        if (sk < 0)
                return sk;
        if (!new(port))
                return -ENOMEM;
        strcpy(port->use_name, name);
        err = nof_delete_port(sk, port);
        if (err < 0)
                sna_debug(1, "delete port failed `%d: %s'.\n", err, strerror(errno));
        free(port);
        sna_nof_disconnect(sk);
        return err;
}

static int sna_delete_link(char *name)
{
        struct sna_nof_ls *ls;
        int err, sk;

        sk = sna_nof_connect();
        if (sk < 0)
                return sk;
        if (!new(ls))
                return -ENOMEM;
        strcpy(ls->use_name, name);
        err = nof_delete_link_station(sk, ls);
        if (err < 0)
                sna_debug(1, "delete link station failed `%d: %s'.\n", err, strerror(errno));
        free(ls);
        sna_nof_disconnect(sk);
        return err;
}

static int sna_start_node(char *name)
{
        struct sna_nof_node *node;
        sna_netid *netid;
        int err, sk;

        sk = sna_nof_connect();
        if (sk < 0)
                return sk;
        if (!new(node))
                return -ENOMEM;
        netid = sna_char_to_netid(name);
        memcpy(&node->netid, netid, sizeof(sna_netid));
        err = nof_start_node(sk, node);
        if (err < 0)
                sna_debug(1, "start node failed `%d: %s'.\n", err, strerror(errno));
        free(node);
        sna_nof_disconnect(sk);
        return err;
}

static int sna_stop_node(char *name)
{
	struct sna_nof_node *node;
	sna_netid *netid;
	int err, sk;

	sk = sna_nof_connect();
        if (sk < 0)
                return sk;
        if (!new(node))
                return -ENOMEM;
	netid = sna_char_to_netid(name);
	memcpy(&node->netid, netid, sizeof(sna_netid));
        err = nof_stop_node(sk, node);
        if (err < 0)
                sna_debug(1, "stop node failed `%d: %s'.\n", err, strerror(errno));
        free(node);
        sna_nof_disconnect(sk);
        return err;
}

static int sna_delete_node(char *name)
{
        struct sna_nof_node *node;
        sna_netid *netid;
        int err, sk;

        sk = sna_nof_connect();
        if (sk < 0)
                return sk;
        if (!new(node))
                return -ENOMEM;
        netid = sna_char_to_netid(name);
        memcpy(&node->netid, netid, sizeof(sna_netid));
        err = nof_delete_node(sk, node);
        if (err < 0)
                sna_debug(1, "delete node failed `%d: %s'.\n", err, strerror(errno));
        free(node);
        sna_nof_disconnect(sk);
        return err;
}

int sna_start(int argc, char **argv)
{
	if (argc < 2)
		sna_start_help();
	if (!matches(*argv, "node")) {
		next_arg_fail(argv, argc, sna_start_help);
		return sna_start_node(*argv);
	}
	if (!matches(*argv, "dlc")) {
		next_arg_fail(argv, argc, sna_start_help);
		return sna_start_dlc(*argv);
	}
	if (!matches(*argv, "link")) {
		next_arg_fail(argv, argc, sna_start_help);
                return sna_start_link(*argv);
	}
	if (!matches(*argv, "lu")) {
		next_arg_fail(argv, argc, sna_start_help);
		return sna_start_lu(*argv);
	}
	if (!matches(*argv, "all")) {
		sna_debug(1, "feature currently not supported.\n");
		return 0;
	}
	sna_start_help();
	return 0;
}

static int sna_stop_help(void)
{
        printf("Usage: %s stop <service> <name>\n", name_s);
        exit(1);
}

int sna_stop(int argc, char **argv)
{
	
        if (argc < 2)
                sna_stop_help();
        if (!matches(*argv, "node")) {
		next_arg_fail(argv, argc, sna_stop_help);
		return sna_stop_node(*argv);
        }
        if (!matches(*argv, "dlc")) {
                next_arg_fail(argv, argc, sna_stop_help);
                return sna_stop_dlc(*argv);
        }
        if (!matches(*argv, "link")) {
                next_arg_fail(argv, argc, sna_stop_help);
                return sna_stop_link(*argv);
        }
	if (!matches(*argv, "lu")) {
		sna_debug(1, "feature currently not supported.\n");
                return 0;
		/*
		 * next_arg_fail(argv, argc, sna_stop_help);
		 * return sna_stop_lu(*argv);
		 */
	}
        if (!matches(*argv, "all")) {
                sna_debug(1, "feature currently not supported.\n");
                return 0;
        }
        sna_stop_help();
	return 0;
}

static int sna_delete_help(void)
{
        printf("Usage: %s delete <service> <name>\n", name_s);
        exit(1);
}

int sna_delete(int argc, char **argv)
{

        if (argc < 2)
                sna_delete_help();
        if (!matches(*argv, "node")) {
		next_arg_fail(argv, argc, sna_delete_help);
		return sna_delete_node(*argv);
        }
        if (!matches(*argv, "dlc")) {
                next_arg_fail(argv, argc, sna_delete_help);
                return sna_delete_dlc(*argv);
        }
        if (!matches(*argv, "link")) {
                next_arg_fail(argv, argc, sna_delete_help);
                return sna_delete_link(*argv);
        }
        if (!matches(*argv, "lu")) {
		sna_debug(1, "feature currently not supported.\n");
                return 0;
        }
        if (!matches(*argv, "all")) {
                sna_debug(1, "feature currently not supported.\n");
                return 0;
        }
        sna_delete_help();
        return 0;
}
