/* sna_trs.c: Linux Systems Network Architecture implementation
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
#include <linux/sna.h>

int sna_trs_create(struct sna_nof_node *start)
{
	sna_debug(5, "init\n");
	sna_cosm_create(start);
	sna_rss_create(start);
	sna_tdm_create(start);
	return 0;
}

int sna_trs_destroy(struct sna_nof_node *delete)
{
	sna_debug(5, "init\n");
	sna_tdm_destroy(delete);
	sna_rss_destroy(delete);
	sna_cosm_destroy(delete);
	return 0;
}
