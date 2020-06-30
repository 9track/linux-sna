/* af_sna.c: Linux Systems Network Architecture implementation
 *  - currently sockets only used by nof, use cpic, appc, or lua for data tx/rx.
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
#if defined(CONFIG_SNA) || defined(CONFIG_SNA_MODULE)
#include <linux/module.h>
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
#include <linux/if_ether.h>
#include <linux/notifier.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/route.h>
#include <linux/inet.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <net/datalink.h>
#include <net/sock.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/rtnetlink.h>
#include <linux/ctype.h>

#include <linux/sna.h>
#include <linux/cpic.h>
#include <linux/attach.h>

#ifdef CONFIG_SNA_LLC
#include <net/llc_if.h>
#include <net/llc_sap.h>
#include <net/llc_pdu.h>
#include <net/llc_conn.h>
#include <linux/llc.h>
#endif

#ifdef CONFIG_SYSCTL
extern inline void sna_register_sysctl(void);
extern inline void sna_unregister_sysctl(void);
#endif /* CONFIG_SYSCTL */

#ifdef CONFIG_PROC_FS
static struct proc_dir_entry *proc_net_sna = NULL;
#endif

static struct proto_ops sna_ops;

/* following information is used in xid negotiations and other places. 
 * please follow the char maxes or things may not work like expected.
 */
char sna_maintainer[]           = "Jay Schulist <jschlst@linux-sna.org>";
char sna_sw_prd_version[] 	= "0";			/* 2 chars max. */
char sna_sw_prd_release[]	= "1";			/* 2 chars max. */
char sna_sw_prd_modification[]	= "28";			/* 2 chars max. */
char sna_product_id[] 		= "GoBIG";		/* 7 chars max */
char sna_product_name[] 	= "Linux-SNA";		/* 30 chars max */
char sna_vendor[]		= "linux-SNA.ORG";	/* 16 chars max */

__u32	sysctl_max_link_stations_cnt;
__u32	sysctl_max_lu_cnt;
__u32	sysctl_max_mode_cnt;
__u32	sysctl_max_inbound_activations;
__u32	sysctl_max_outbound_activations;
__u32	sysctl_max_btu_size;
__u32	sysctl_max_tx_ru_size;
__u32	sysctl_max_rx_ru_size;
__u32	sysctl_max_auto_activation_limit;
__u32	sysctl_bind_pacing_cnt;
__u32	sna_debug_level = 0;

#define MAX_SNA_ADDR		1024

/* Display an Ethernet address in readable format. */
char *sna_pr_ether(unsigned char *ptr)
{
  	static char buff[64];

  	sprintf(buff, "%02X%02X%02X%02X%02X%02X",
        	(ptr[0] & 0377), (ptr[1] & 0377), (ptr[2] & 0377),
        	(ptr[3] & 0377), (ptr[4] & 0377), (ptr[5] & 0377));
  	return buff;
}

char *sna_pr_nodeid(sna_nodeid n)
{
        static char buff[20];

        sprintf(buff, "%03X.%05X", xid_get_block_num(n), xid_get_pu_num(n));
        return buff;
}

/* display a padded netid correctly */
char *sna_pr_netid(sna_netid *n)
{
        sna_netid p;
	char *buff;
        int i;

	new_s(buff, 20, GFP_ATOMIC);
	if (!buff)
		return "";
        memcpy(&p, n, sizeof(sna_netid));
        for (i = 0; p.net[i] != 0x20; i++); p.net[i] = 0;
        for (i = 0; p.name[i] != 0x20; i++); p.name[i] = 0;
        sprintf(buff, "%s.%s", p.net, p.name);
        return buff;
}

int sna_utok(void *uaddr, int ulen, void *kaddr)
{
        if (ulen < 0|| ulen > MAX_SNA_ADDR)
                return -EINVAL;
        if (ulen == 0)
                return 0;
        if (copy_from_user(kaddr,uaddr,ulen))
                return -EFAULT;
        return 0;
}

int sna_ktou(void *kaddr, int klen, void *uaddr)
{
        if (klen < 0 || klen > MAX_SNA_ADDR)
                return -EINVAL;
        if (klen) {
                if (copy_to_user(uaddr,kaddr,klen))
                        return -EFAULT;
        }
        return 0;
}

void sna_ctrl_info_destroy(struct sna_ctrl_info *ctrl)
{
	sna_debug(5, "init\n");
	if (atomic_read(&ctrl->use) != 0)
		sna_debug(5, "free'ing ctrl in use\n");
	kfree(ctrl);
	return;
}

struct sna_ctrl_info *sna_ctrl_info_create(int gfp_mask)
{
	struct sna_ctrl_info *ctrl;

	sna_debug(5, "init\n");
	new(ctrl, gfp_mask);
        if (!ctrl)
                return NULL;
        atomic_set(&ctrl->use, 1);
        ctrl->destroy = sna_ctrl_info_destroy;
	return ctrl;
}

struct sk_buff *sna_alloc_skb(unsigned int size, int gfp_mask)
{
	struct sk_buff *skb;
	
	sna_debug(5, "init\n");
	skb = alloc_skb(size, gfp_mask);
	if (!skb)
		return NULL;
	skb->sna_ctrl = sna_ctrl_info_create(gfp_mask);
	if (!skb->sna_ctrl) {
		kfree_skb(skb);
		return NULL;
	}
	return skb;
}

sna_netid *sna_char_to_netid(unsigned char *b)
{
        sna_netid *n;
	unsigned char c[40];
        int i;

	sna_debug(5, "init: %s\n", b);
	strcpy(c, b);	/* always use protection */
	new(n, GFP_KERNEL);
	if (!n)
		return NULL;
        strcpy(n->name, strpbrk(c, ".")+1);
        for (i = 0; i < 8; i++)
                n->name[i] = toupper(n->name[i]);
        for (i = strlen(n->name); i < 8; i++)
                n->name[i] = 0x20;
        strcpy(n->net, strtok(c, "."));
        for (i = 0; i < 8; i++)
                n->net[i] = toupper(n->net[i]);
	for (i = strlen(n->net); i < 8; i++)
                n->net[i] = 0x20;
        return n;
} 

int sna_netid_to_char(sna_netid *n, unsigned char *c)
{
	int len = 0, i;

	sna_debug(5, "init: %s.%s\n", n->net, n->name);
	for (i = 0; i < 8 && (n->net[i] != 0x20); i++)
		/* Nothing */ ;
	len = i;
	strncpy(c, n->net, i);
	len = i + 1;
	strncpy(c + i, ".", 1);
	for (i = 0; i < 8 && (n->name[i] != 0x20); i++)
		/* Nothing */ ;
	strncpy(c + len, n->name, i);
	len += i;
	strncpy(c + len, "\0", 1);
	sna_debug(5, "finished string is (%s) length is (%d) (%d)\n",
		c, strlen(c), len);
	return len;
}

/* Temporary HexDump function. */
int hexdump(unsigned char *pkt_data, int pkt_len)
{
        int i;

        while (pkt_len > 0) {
                printk("  ");   /* Leading spaces. */

                /* Print the HEX representation. */
                for (i = 0; i < 8; ++i) {
                        if (pkt_len - (long)i>0)
                                printk("%2.2X ", pkt_data[i] & 0xFF);
                        else
                                printk("  ");
                }

                printk(":");

                for (i = 8; i < 16; ++i) {
                        if (pkt_len - (long)i > 0)
                                printk("%2.2X ", pkt_data[i]&0xFF);
                        else
                                printk("  ");
                }

                /* Print the ASCII representation. */
                printk("  ");
                for (i = 0; i < 16; ++i) {
                        if (pkt_len - (long)i > 0) {
                                if (isprint(pkt_data[i]))
                                        printk("%c", pkt_data[i]);
                                else
                                        printk(".");
                        }
                }

                printk("\n");
                pkt_len  -= 16;
                pkt_data += 16;
        }
        printk("\n");
	return 0;
}

static int sna_create(struct socket *sock, int protocol)
{
	struct sock *sk = sock->sk;

	sna_debug(5, "init\n");
	sk = sk_alloc(PF_SNA, GFP_KERNEL, 1);
	if (!sk)
		return -ENOMEM;
	switch (sock->type) {
		case SOCK_STREAM:
		case SOCK_DGRAM:
		case SOCK_RAW:
		case SOCK_SEQPACKET:
			sock->ops = &sna_ops;
			break;

		default:
			sk_free((void *)sk);
			return -ESOCKTNOSUPPORT;
	}
	MOD_INC_USE_COUNT;
	sock_init_data(sock, sk);
	sk->destruct 	= NULL;
        sk->zapped 	= 1;
        return 0;
}

static int sna_destroy_socket(struct sock *sk)
{
	sna_debug(5, "init\n");
	MOD_DEC_USE_COUNT;
	return 0;
}

static int sna_release(struct socket *sock)
{
	struct sock *sk = sock->sk;

	sna_debug(5, "init\n");
        if (!sk)
                return 0;
        if (!sk->dead)
                sk->state_change(sk);
        sk->dead = 1;
        sock->sk = NULL;
	sna_destroy_socket(sk);
	return 0;
}

static int sna_bind(struct socket *sock, struct sockaddr *uaddr, int addr_len)
{
	sna_debug(5, "init\n");
	return -EOPNOTSUPP;
}

static int sna_connect(struct socket *sock, struct sockaddr *uaddr,
	int addr_len, int flags)
{
	sna_debug(5, "init\n");
	return -EOPNOTSUPP;
}

static int sna_accept(struct socket *sock, struct socket *newsock, int flags)
{
	sna_debug(5, "init\n");
	return -EOPNOTSUPP;
}

static unsigned int sna_poll(struct file * file, struct socket *sock,
	poll_table *pt)
{
	sna_debug(5, "init\n");
	return -EOPNOTSUPP;
}

static int sna_getname(struct socket *sock, struct sockaddr *uaddr,
	int *uaddr_len, int peer)
{
	sna_debug(5, "init\n");
	return -EOPNOTSUPP;
}

static int sna_sendmsg(struct socket *sock, struct msghdr *msg, int size,
	struct scm_cookie *scm)
{
	sna_debug(5, "init\n");
	return -EOPNOTSUPP;
}

static int sna_recvmsg(struct socket *sock, struct msghdr *msg, int size,
	int flags, struct scm_cookie *scm)
{
	sna_debug(5, "init\n");
	return -EOPNOTSUPP;
}

static int sna_shutdown(struct socket *sk, int how)
{
	sna_debug(5, "init\n");
	return -EOPNOTSUPP;
}

/* SNA ioctl calls. */
static int sna_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg)
{
	sna_debug(5, "init\n");
	switch (cmd) {
		case SIOCGNODE:
		case SIOCGDLC:
		case SIOCGPORT:
		case SIOCGLS:
		case SIOCGMODE:
		case SIOCGLU:
		case SIOCGPLU:
		case SIOCGCOS:
			return sna_nof_ioctl(cmd, (void *)arg);

		case SIOCGCPICS:
			return sna_cpic_ioctl(cmd, (void *)arg);

#ifdef NOT
		case SIOPS:
			return sna_ps_ioctl(cmd, (void *)arg);
#endif

		default:
			return -EOPNOTSUPP;
	}
	return 0;
}

static int sna_setsockopt(struct socket *sock, int level, int optname,
        char *optval, int optlen)
{
        int err;

	sna_debug(5, "init\n");
        switch (level) {
		case SOL_SNA_NOF:
			err = sna_nof_setsockopt(sock, level, optname,
				optval, optlen);
			break;

#ifdef NOT
		case SOL_SNA_PS:
			err = sna_ps_setsockopt(sock, level, optname,
				optval, optlen);
			break;
#endif

		case SOL_SNA_CPIC:
			err = sna_cpic_setsockopt(sock, level, optname,
				optval, optlen);
			break;

		default:
			return -ENOPROTOOPT;
	}
	return err;
}

static int sna_getsockopt(struct socket *sock, int level, int optname,
        char *optval, int *optlen)
{
	int err;

	sna_debug(5, "init\n");
        switch (level) {
		case SOL_SNA_NOF:
			err = sna_nof_getsockopt(sock, level, optname,
				optval, optlen);
			break;

#ifdef NOT
		case SOL_SNA_PS:
			err = sna_ps_getsockopt(sock, level, optname,
				optval, optlen);
			break;
#endif

		default:
			return -ENOPROTOOPT;
        }
	return err;
}

void sna_mod_inc_use_count(void)
{
	MOD_INC_USE_COUNT;
}

void sna_mod_dec_use_count(void)
{
	MOD_DEC_USE_COUNT;
}

static struct net_proto_family sna_family_ops =
{
	PF_SNA,
	sna_create
};

static struct proto_ops sna_ops =
{
	PF_SNA,
	sna_release,
	sna_bind,
	sna_connect,
	sock_no_socketpair,
	sna_accept,
	sna_getname,
	sna_poll,
	sna_ioctl,
	sock_no_listen,
	sna_shutdown,
	sna_setsockopt,		/* Okay. */
	sna_getsockopt,		/* Okay. */
	sna_sendmsg,
	sna_recvmsg,
	sock_no_mmap
};

#ifdef CONFIG_PROC_FS
static void sna_proc_init(void)
{
        if (!proc_net_sna) {
                struct proc_dir_entry *ent;
                ent = proc_mkdir("net/sna", 0);
                if (ent) {
                        ent->owner = THIS_MODULE;
                        proc_net_sna = ent;
                }
        }
	return;
}

static void sna_proc_cleanup(void)
{
        if (proc_net_sna) {
                proc_net_sna = NULL;
                remove_proc_entry("net/sna", 0);
        }
	return;
}

struct proc_dir_entry *proc_sna_create(const char *name,
        mode_t mode, get_info_t *get_info)
{
        return create_proc_info_entry(name,mode,proc_net_sna,get_info);
}

void proc_sna_remove(const char *name)
{
        remove_proc_entry(name, proc_net_sna);
	return;
}
#endif

/* Watch the network devices to see if we need to add/delete/start/stop
 * a data link control. (Device)
 */
int sna_netdev_event(struct notifier_block *self, unsigned long event,
        void *data)
{
        struct net_device *dev = (struct net_device *)data;

        if (event == NETDEV_UP)
                sna_cs_dlc_create(dev);
        if (event == NETDEV_DOWN)
                sna_cs_dlc_delete(dev);
        return NOTIFY_DONE;
}

struct notifier_block nb_sna = {
        sna_netdev_event,
        NULL,
        0
};

int __init sna_init(void)
{
	struct net_device *dev;

	/* Scan all the existing interfaces for SNA
         * compatible devices, add the device to our list if it
         * is available for sna transfers.
         */
        rtnl_lock();
        for (dev = dev_base; dev != NULL; dev = dev->next) {
                if (dev->flags & IFF_UP)
                        sna_cs_dlc_create(dev);
        }
        rtnl_unlock();

        /* Attach a device notifier so we can watch for devices
         * going up and down.
         */
        register_netdevice_notifier(&nb_sna);

#ifdef CONFIG_PROC_FS
        sna_proc_init();
	proc_sna_create("local_nodes", 0, sna_nof_get_info);
	proc_sna_create("devices", 0, sna_cs_get_info_dlc);
	proc_sna_create("ports", 0, sna_cs_get_info_port);
	proc_sna_create("link_stations", 0, sna_cs_get_info_ls);
	proc_sna_create("path_controls", 0, sna_pc_get_info);
	
	proc_sna_create("modes", 0, sna_rm_get_info_mode);
	proc_sna_create("local_lus", 0, sna_rm_get_info_local_lu);
	proc_sna_create("remote_lus", 0, sna_rm_get_info_remote_lu);
	proc_sna_create("cos_levels", 0, sna_cosm_get_info);
	proc_sna_create("cos_tg_characteristics", 0, sna_cosm_get_info_tg);
	proc_sna_create("cos_node_characteristics", 0, sna_cos_get_info_node);
	proc_sna_create("node_map", 0, sna_tdm_get_info);
	proc_sna_create("transmission_groups", 0, sna_tdm_get_info_tg);
	proc_sna_create("asm_active_address_space", 0, sna_asm_get_info);
	proc_sna_create("asm_active_lfsids", 0, sna_asm_get_active_lfsids);
#endif

        printk(KERN_INFO "linux-SNA (System Network Architecture) "
		"v%s.%s.%s (%s) for Linux NET4.0\n", 
		sna_sw_prd_version, sna_sw_prd_release, sna_sw_prd_modification,
		sna_product_id);

	(void) sock_register(&sna_family_ops);

#ifdef CONFIG_SYSCTL
        sna_register_sysctl();
#endif /* CONFIG_SYSCTL */

	sna_attach_init();
#if defined(CONFIG_SNA_CPIC)
        sna_cpic_init();
#endif /* CONFIG_SNA_CPIC */
	
        return 0;
}

void __exit sna_exit(void)
{
#if defined(CONFIG_SNA_CPIC)
	sna_cpic_fini();
#endif /* CONFIG_SNA_CPIC */
	sna_attach_fini();
	
	/* Remove out notifier from the netdevice layer. */
        unregister_netdevice_notifier(&nb_sna);

#ifdef CONFIG_SYSCTL
        sna_unregister_sysctl();
#endif /* CONFIG_SYSCTL */

#ifdef CONFIG_PROC_FS
	proc_sna_remove("cos_levels");
	proc_sna_remove("cos_tg_characteristics");
	proc_sna_remove("cos_node_characteristitcs");
	proc_sna_remove("node_map");
	proc_sna_remove("transmission_groups");
	proc_sna_remove("asm_active_address_space");
	proc_sna_remove("asm_active_lfsids");
	proc_sna_remove("remote_lus");
	proc_sna_remove("local_lus");
	proc_sna_remove("modes");

	proc_sna_remove("path_controls");
	proc_sna_remove("link_stations");
	proc_sna_remove("ports");
	proc_sna_remove("devices");
	proc_sna_remove("local_nodes");
        sna_proc_cleanup();
#endif

	sock_unregister(PF_SNA);
        return;
}

EXPORT_SYMBOL(hexdump);
EXPORT_SYMBOL(sna_utok);
EXPORT_SYMBOL(sna_ktou);

module_init(sna_init);
module_exit(sna_exit);
MODULE_LICENSE("GPL");
#endif /* CONFIG_SNA || CONFIG_SNA_MODULE */
