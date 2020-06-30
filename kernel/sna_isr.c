/* sna_isr.c: Linux Systems Network Architecture implementation
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

int sna_isr_create(struct sna_cp_create_parms *newisr)
{
	return 0;
}

int sna_isr_assign_lfsid_rsp(void)
{
	return 0;
}

int sna_isr_lfsid_in_use(void)
{
	return 0;
}

int sna_isr_session_route_inop(void)
{
	return 0;
}

int sna_isr_update_slu_mode(void)
{
	return 0;
}

int sna_isr_delete_slu_mode(void)
{
	return 0;
}

int sna_isr_query_slu_mode(void)
{
	return 0;
}

int sna_isr_abort_sc(void)
{
	return 0;
}

int sna_isr_assign_pcid_rsp(void)
{
	return 0;
}

int sna_isr_cinit(void)
{
	return 0;
}

int sna_isr_init_neg_rsp(void)
{
	return 0;
}
