/* sna_rss.c: Linux Systems Network Architecture implementation
 * Route Selection Services.
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

/*
 * Bugs:
 * - We do no weight calculations or tree building.
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
#include <linux/sna.h>

int sna_rss_request_route(struct sna_rss_route *r)
{
	return 0;
}

/* this function is very simple right now until we do more routing.
 */
struct sna_tg_cb *sna_rss_req_single_hop_route(sna_netid *remote_name)
{
	struct sna_tg_cb *tg;

	sna_debug(5, "init\n");
	tg = sna_tdm_request_tg_vectors(remote_name);
	if (!tg)
		return NULL;
	return tg;
}

int sna_rss_obtain_tg_vectors(void)
{
	return 0;
}

int sna_rss_select_tg_vector(void)
{
	return 0;
}

int sna_rss_build_rscv(void)
{
	return 0;
}

int sna_rss_obtain_trees(void)
{
	return 0;
}

int sna_rss_update_trees(void)
{
	return 0;
}

int sna_rss_update_resource_weights(void)
{
	return 0;
}

int sna_rss_create(struct sna_nof_node *start)
{
	return 0;
}

int sna_rss_destroy(struct sna_nof_node *delete)
{
	return 0;
}
