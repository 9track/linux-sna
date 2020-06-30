/* sna_attach.c: Linux Systems Network Architecture implementation
 * - SNA Attach Manager Kernel Backend.
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

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <linux/uio.h>
#include <net/sock.h>

#include <linux/attach.h>
#include <linux/sna.h>

static LIST_HEAD(attach_tp_list);
static u_int32_t attach_tp_system_index = 0;

struct sk_buff_head attach_queue;
wait_queue_head_t attach_wait;

struct attach_sock {
	struct sock sk;
};

static struct attach_tp_info *sna_attach_tp_get_by_name(char *name)
{
	struct attach_tp_info *tp;
	struct list_head *le;

	list_for_each(le, &attach_tp_list) {
		tp = list_entry(le, struct attach_tp_info, list);
		if (!strcmp(tp->name, name))
			return tp;
	}
	return NULL;
}

static struct attach_tp_info *sna_attach_tp_get_by_index(u_int32_t index)
{
	struct attach_tp_info *tp;
	struct list_head *le;

	sna_debug(5, "init\n");
	list_for_each(le, &attach_tp_list) {
		tp = list_entry(le, struct attach_tp_info, list);
		if (tp->index == index)
			return tp;
	}
	return NULL;
}

static u_int32_t sna_attach_tp_new_index(void)
{
	for (;;) {
		if (++attach_tp_system_index <= 0)
			attach_tp_system_index = 1;
		if (sna_attach_tp_get_by_index(attach_tp_system_index) == NULL)
			return attach_tp_system_index;
	}
	return 0;
}

/**
 * wait for an attach message
 */
static int sna_attach_recvmsg(struct socket *sock, struct msghdr *msg,
			      size_t len, int flags)
{
	struct tp_attach *tp;
	struct sk_buff *skb;
    int err;

	sna_debug(5, "init\n");
	err = 0;
	do {
		skb = skb_dequeue(&attach_queue);
		if (!skb) {
			if (wait_event_interruptible(attach_wait, 0)) {
				err = -ERESTARTSYS;
				goto error;
			}
		}
	} while (!skb);

	new(tp, GFP_ATOMIC);
	if (!tp) {
		err = -ENOMEM;
		goto error;
	}

#ifdef NOT
	struct fmhdr *fm = skb->data;
	tp.tp_len = tp_len;
	strncpy(tp.tp_name, ptr + 1, tp_len);
	memcpy(&tp.tcb_id, &skb->cb, sizeof(unsigned long));
#endif

	err = copy_to_iter((void *)tp, sizeof(tp), &msg->msg_iter);
	kfree_skb(skb);
	kfree(tp);
error:	return err;
}

/* Register a transaction program
 * TODO: Is this required?
 */

static int sna_attach_tp_register(u_int32_t *afd, u_int32_t *tp_info_ptr)
{
	struct tp_info tp_info;
	struct attach_tp_info *tp;
	struct attach *at;
	int fd, err = -EBADF;

	sna_debug(5, "init\n");
	sna_utok(afd, sizeof(fd), &fd);
	at = attach_lookup_fd(fd, &err);
	if (!at)
		goto out;
	sna_utok(tp_info_ptr, sizeof(tp_info), &tp_info);
	tp = sna_attach_tp_get_by_name(tp_info.tp_name);
	if (tp) {
		err = -EEXIST;
		goto error;
	}
	new(tp, GFP_ATOMIC);
	if (!tp) {
		err = -ENOMEM;
		goto error;
	}
	err 		= 0;
	tp->index       = sna_attach_tp_new_index();
	tp->pid		= -1;
	tp->state	= AT_INIT;
	strcpy(tp->name, tp_info.tp_name);
	list_add_tail(&tp->list, &attach_tp_list);
error:	attach_put_fd(at);
out:    return err;
}

/* Unregister a transaction program
 * TODO: Is this required?
 */

static int sna_attach_tp_unregister(u_int32_t *afd, u_int32_t *tp_index_ptr)
{
	struct attach_tp_info *tp;
	struct attach *at;
	int fd, err = -EBADF;
	u_int32_t tp_index;

	sna_debug(5, "init\n");
	sna_utok(afd, sizeof(fd), &fd);
	at = attach_lookup_fd(fd, &err);
	if (!at)
		goto out;
	sna_utok(tp_index_ptr, sizeof(tp_index), &tp_index);
	tp = sna_attach_tp_get_by_index(tp_index);
	if (!tp) {
		err = -ENOENT;
		goto error;
	}
	err = 0;
	list_del(&tp->list);
	kfree(tp);
error:	attach_put_fd(at);
out:    return err;
}

/* Notify processes waiting on recvmsg that a new attach
 * request has arrived
 */

int sna_attach_execute_tp(u_int32_t tcb_id, struct sk_buff *skb)
{
	sna_debug(5, "init\n");
	memcpy(&skb->cb, &tcb_id, sizeof(u_int32_t));
	skb_queue_tail(&attach_queue, skb);
	wake_up_interruptible(&attach_wait);
	return 0;
}

/* Associate a PID with a transaction program */

static int sna_tp_correlate(u_int32_t *afd, u_int32_t *pid_ptr,
	u_int32_t *tp_index_ptr, u_int32_t *tp_name_ptr)
{
	struct attach *at;
	struct sna_tp_cb *tp;
	int fd, err = -EBADF;
	u_int32_t tp_index;
	pid_t pid;

	sna_debug(5, "init\n");
	sna_utok(afd, sizeof(fd), &fd);
	at = attach_lookup_fd(fd, &err);
	if (!at)
		goto out;
	sna_utok(pid_ptr, sizeof(pid), &pid);
	sna_utok(tp_index_ptr, sizeof(tp_index), &tp_index);
	tp = sna_rm_tp_get_by_index(tp_index);
	if (!tp) {
		err = -EINVAL;
		goto error;
	}
	err 	= 0;
	tp->pid = pid;
error:	attach_put_fd(at);
out:    return err;
}

static int sna_attach_error(void *uaddr, u_int32_t return_code)
{
	return sna_ktou(&return_code, sizeof(return_code), uaddr);
}

static int sna_attach_release(struct socket *sock)
{
	struct list_head *le, *se;
	struct attach_tp_info *tp;

	sna_debug(5, "init\n");
	list_for_each_safe(le, se, &attach_tp_list) {
		tp = list_entry(le, struct attach_tp_info, list);
		list_del(&tp->list);
		kfree(tp);
	}
	return 0;
}

static int sna_attach_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg)
{
	attach_args *args = NULL;
	u_int32_t opcode;
	int err = -EINVAL;
	void __user *uaddr = (void __user *)arg;

	sna_debug(5, "init\n");
	if (!uaddr)
		goto out;

	/* copy call arguments from user space, if needed. */
	new(args, GFP_ATOMIC);
	if (!args)
		goto out;
	if (sna_utok(uaddr, sizeof(attach_args), args) < 0)
		goto out;
	sna_utok(args->a1, sizeof(u_int32_t), &opcode);
	switch (opcode) {
		case ATTACH_TP_REGISTER:
			err = sna_attach_tp_register((u_int32_t *)args->a3,
				(u_int32_t *)args->a4);
			break;
		case ATTACH_TP_UNREGISTER:
			err = sna_attach_tp_unregister((u_int32_t *)args->a3,
				(u_int32_t *)args->a4);
			break;
		case ATTACH_TP_CORRELATE:
			err = sna_tp_correlate((u_int32_t *)args->a3,
				(u_int32_t *)args->a4, (u_int32_t *)args->a5,
				(u_int32_t *)args->a6);
			break;
		default:
			sna_debug(5, "unknown opcode=%02X\n", opcode);
	}
out:	sna_attach_error(args->a2, err);
	return 0;
}

static unsigned int sna_attach_poll(struct file * file, struct socket *sock,
	poll_table *pt)
{
	sna_debug(5, "init\n");
	return -EOPNOTSUPP;
}

static struct proto attach_proto = {
	.name		= "ATTACH",
	.owner		= THIS_MODULE,
	.obj_size	= sizeof(struct attach_sock)
};

static const struct proto_ops attach_sock_ops = {
	.family		= PF_SNA,
	.owner		= THIS_MODULE,
	.release	= sna_attach_release,
	.bind		= sock_no_bind,
	.connect	= sock_no_connect,
	.listen		= sock_no_listen,
	.accept		= sock_no_accept,
	.getname	= sock_no_getname,
	.sendmsg	= sock_no_sendmsg,
	.recvmsg	= sna_attach_recvmsg,
	.poll		= sna_attach_poll,
	.ioctl		= sna_attach_ioctl,
	.mmap		= sock_no_mmap,
	.socketpair	= sock_no_socketpair,
	.shutdown	= sock_no_shutdown,
	.setsockopt	= sock_no_setsockopt,
	.getsockopt	= sock_no_getsockopt
};

static int sna_attach_create(struct net *net, struct socket *sock, int protocol,
			     int kern)
{
	struct sock *sk;

	sock->state = SS_UNCONNECTED;
	sock->ops = &attach_sock_ops;

	sk = sk_alloc(net, PF_SNA, GFP_KERNEL, &attach_proto, kern);
	if (!sk)
		return -ENOMEM;

	sock_init_data(sock, sk);
	return 0;
}

static const struct net_proto_family attach_sock_family_ops = {
	.family	= PF_SNA,
	.owner	= THIS_MODULE,
	.create	= sna_attach_create,
};

static int sna_attach_get_info(struct seq_file *m, void *v)
{
	struct attach_tp_info *tp;
	struct list_head *le;

	seq_printf(m, "%-33s%-5s\n", "name", "index");
	list_for_each(le, &attach_tp_list) {
		tp = list_entry(le, struct attach_tp_info, list);
		seq_printf(m, "%-33s%-5d\n", tp->name, tp->index);
	}

	return 0;
}

int __init sna_attach_init(void)
{
	int err;

	sna_debug(5, "init\n");

	err = proto_register(&attach_proto, 0);
	if (err < 0)
		return err;

	err = sna_sock_register(SNAPROTO_ATTACH, &attach_sock_family_ops);
	if (err < 0)
		goto error;

	skb_queue_head_init(&attach_queue);
	init_waitqueue_head(&attach_wait);

#ifdef CONFIG_PROC_FS
	proc_sna_create("transaction_programs", 0, sna_attach_get_info);
#endif
	return 0;

error:
	proto_unregister(&attach_proto);
	return err;
}

int sna_attach_fini(void)
{
	sna_debug(5, "init\n");
#ifdef CONFIG_PROC_FS
	proc_sna_remove("transaction_programs");
#endif
	sna_sock_unregister(SNAPROTO_ATTACH);
	proto_unregister(&attach_proto);
	return 0;
};
