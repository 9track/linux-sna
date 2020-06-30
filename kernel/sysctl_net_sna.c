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

#include <linux/config.h>
#include <linux/mm.h>
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
extern __u8    	sna_debug_level;

/* xid. */
extern u_int32_t sysctl_xid_idle_limit;
extern u_int32_t sysctl_xid_retry_limit;
extern u_int32_t sysctl_test_retry_limit;


#ifdef CONFIG_SYSCTL
ctl_table sna_table[] = {
	{NET_SNA_MAX_LINK_STATIONS, "max_link_station_count",
        &sysctl_max_link_stations_cnt, sizeof(__u32), 0644, NULL,
        &proc_dointvec_jiffies},
        {NET_SNA_MAX_LU, "max_lu_count",
        &sysctl_max_lu_cnt, sizeof(__u32), 0644, NULL,
        &proc_dointvec_jiffies},
        {NET_SNA_MAX_MODE, "max_mode_count", &sysctl_max_mode_cnt,
        sizeof(__u32), 0644, NULL, &proc_dointvec_jiffies},

	/* xid. */	
        {NET_SNA_XID_RETRY_LIMIT, "xid_retry_limit", &sysctl_xid_retry_limit,
        sizeof(u_int32_t), 0644, NULL, &proc_dointvec_jiffies},
	{NET_SNA_XID_IDLE_LIMIT, "xid_idle_limit", &sysctl_xid_idle_limit,
	sizeof(u_int32_t), 0644, NULL, &proc_dointvec_jiffies},
	{NET_SNA_TEST_RETRY_LIMIT, "test_retry_limit", &sysctl_test_retry_limit,
	sizeof(u_int32_t), 0644, NULL, &proc_dointvec_jiffies},

        {NET_SNA_MIA, "max_inbound_activations",
        &sysctl_max_inbound_activations, sizeof(__u32), 0644, NULL,
        &proc_dointvec_jiffies},
        {NET_SNA_MOA, "max_outbound_activations",
        &sysctl_max_outbound_activations, sizeof(__u32), 0644, NULL,
        &proc_dointvec_jiffies},
        {NET_SNA_MAX_BTU, "max_btu_size", &sysctl_max_btu_size,
        sizeof(__u32), 0644, NULL, &proc_dointvec_jiffies},
        {NET_SNA_MAX_TX_RU, "max_tx_ru_size", &sysctl_max_tx_ru_size,
        sizeof(__u32), 0644, NULL, &proc_dointvec_jiffies},
        {NET_SNA_MAX_RX_RU, "max_rx_ru_size", &sysctl_max_rx_ru_size,
        sizeof(__u32), 0644, NULL, &proc_dointvec_jiffies},
	{NET_SNA_MAX_AUTO_ACT, "max_auto_activation_limit",
        &sysctl_max_auto_activation_limit,
        sizeof(__u32), 0644, NULL, &proc_dointvec_jiffies},
        {NET_SNA_DEBUG, "debug_level", &sna_debug_level,
        sizeof(__u32), 0644, NULL, &proc_dointvec_jiffies},
	{0}
};

static ctl_table sna_dir_table[] = {
        {NET_SNA, "sna", NULL, 0, 0555, sna_table},
        {0}
};

static ctl_table sna_root_table[] = {
        {CTL_NET, "net", NULL, 0, 0555, sna_dir_table},
        {0}
};

static struct ctl_table_header *sna_table_header;

void sna_register_sysctl(void)
{
	sna_table_header = register_sysctl_table(sna_root_table, 1);
	return;
}

void sna_unregister_sysctl(void)
{
	unregister_sysctl_table(sna_table_header);
	return;
}
#endif
