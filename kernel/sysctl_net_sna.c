/* sysctl_net_sna.c: Linux Systems Network Architecture implementation
 * - SysCtrl user interface to Linux-SNA
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

#include <linux/netdevice.h>
#include <linux/sysctl.h>

extern __u32   	sysctl_max_link_stations_cnt;
extern __u32   	sysctl_max_lu_cnt;
extern __u32   	sysctl_max_mode_cnt;
extern __u32   	sysctl_max_inbound_activations;
extern __u32   	sysctl_max_outbound_activations;
extern __u32   	sysctl_max_retry_limit;
extern __u32   	sysctl_max_btu_size;
extern __u32   	sysctl_max_tx_ru_size;
extern __u32   	sysctl_max_rx_ru_size;
extern __u32   	sysctl_max_auto_activation_limit;
extern __u32   	sysctl_bind_pacing_cnt;
extern __u32   	sna_debug_level;

/* xid. */
extern u_int32_t sysctl_xid_idle_limit;
extern u_int32_t sysctl_xid_retry_limit;
extern u_int32_t sysctl_test_retry_limit;


#ifdef CONFIG_SYSCTL
static struct ctl_table sna_table[] = {
	{
		.procname = "max_link_station_count",
		.data = &sysctl_max_link_stations_cnt,
		.maxlen = sizeof(__u32),
		.mode = 0644,
		.proc_handler = proc_dointvec_jiffies,
	},
	{
		.procname = "max_lu_count",
		.data = &sysctl_max_lu_cnt,
		.maxlen = sizeof(__u32),
		.mode = 0644,
		.proc_handler = proc_dointvec_jiffies,
	},
	{
		.procname = "max_mode_count",
		.data = &sysctl_max_mode_cnt,
		.maxlen = sizeof(__u32),
		.mode = 0644,
		.proc_handler = proc_dointvec_jiffies,
	},

	/* xid. */
	{
		.procname = "xid_retry_limit",
		.data = &sysctl_xid_retry_limit,
		.maxlen = sizeof(__u32),
		.mode = 0644,
		.proc_handler = proc_dointvec_jiffies,
	},
	{
		.procname = "xid_idle_limit",
		.data = &sysctl_xid_idle_limit,
		.maxlen = sizeof(__u32),
		.mode = 0644,
		.proc_handler = proc_dointvec_jiffies,
	},
	{
		.procname = "test_retry_limit",
		.data = &sysctl_test_retry_limit,
		.maxlen = sizeof(__u32),
		.mode = 0644,
		.proc_handler = proc_dointvec_jiffies,
	},

	{
		.procname = "max_inbound_activations",
		.data = &sysctl_max_inbound_activations,
		.maxlen = sizeof(__u32),
		.mode = 0644,
		.proc_handler = proc_dointvec_jiffies,
	},
	{
		.procname = "max_outbound_activations",
		.data = &sysctl_max_outbound_activations,
		.maxlen = sizeof(__u32),
		.mode = 0644,
		.proc_handler = proc_dointvec_jiffies,
	},
	{
		.procname = "max_btu_size",
		.data = &sysctl_max_btu_size,
		.maxlen = sizeof(__u32),
		.mode = 0644,
		.proc_handler = proc_dointvec_jiffies,
	},
	{
		.procname = "max_tx_ru_size",
		.data = &sysctl_max_tx_ru_size,
		.maxlen = sizeof(__u32),
		.mode = 0644,
		.proc_handler = proc_dointvec_jiffies,
	},
	{
		.procname = "max_rx_ru_size",
		.data = &sysctl_max_rx_ru_size,
		.maxlen = sizeof(__u32),
		.mode = 0644,
		.proc_handler = proc_dointvec_jiffies,
	},
	{
		.procname = "max_auto_activation_limit",
		.data = &sysctl_max_auto_activation_limit,
		.maxlen = sizeof(__u32),
		.mode = 0644,
		.proc_handler = proc_dointvec_jiffies,
	},
	{
		.procname = "debug_level",
		.data = &sna_debug_level,
		.maxlen = sizeof(__u32),
		.mode = 0644,
		.proc_handler = proc_dointvec_jiffies,
	},
	{ }
};

static struct ctl_table_header *sna_table_header;

void sna_register_sysctl(void)
{
	sna_table_header = register_net_sysctl(&init_net, "net/sna", sna_table);
}

void sna_unregister_sysctl(void)
{
	unregister_net_sysctl_table(sna_table_header);
}
#endif
