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
#include <linux/list.h>

#include <linux/attach.h>
#include <linux/sna.h>

static LIST_HEAD(attach_tp_list);
static u_int32_t attach_tp_system_index = 0;

struct sk_buff_head attach_queue;
wait_queue_head_t attach_wait;

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
 * open/create a new attach manger instance.
 */
static int sna_attach_open(void)
{
	struct attach *at;
	int err = -ENOMEM;

	sna_debug(5, "init\n");
	if (!(at = attach_alloc()))
		goto out;
        err = attach_map_fd(at);
        if (err < 0)
                goto out_r;
        goto out;
out_r:  attach_release(at);
out:    return err;
}

/**
 * close an attach manager instance.
 */
static int sna_attach_close(u_int32_t *afd)
{
	struct attach *at;
	int fd, err;

	sna_debug(5, "init\n");
	sna_utok(afd, sizeof(fd), &fd);
	at = attach_lookup_fd(fd, &err);
	if (at)
		attach_release(at);
	return err;
}

/**
 * place an attach manager into the listen state.
 */
static int sna_attach_listen(u_int32_t *afd, u_int32_t *buf, 
	u_int32_t *buf_len, u_int32_t *flags)
{
	struct attach *at;
	int fd, err = -EBADF;
	struct tp_attach *tp;
	struct sk_buff *skb;

	sna_debug(5, "init\n");
	sna_utok(afd, sizeof(fd), &fd);
        at = attach_lookup_fd(fd, &err);
        if (!at)
		goto out;
	err = 0;
	do {
		skb = skb_dequeue(&attach_queue);
		if (!skb) {
			interruptible_sleep_on(&attach_wait);
			err = -ERESTARTSYS;
			if (signal_pending(current))
				goto error;
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

	err = sna_ktou(tp, sizeof(tp), buf);
	if (err == 0)
		err = sizeof(tp);
	kfree_skb(skb);
	kfree(tp);
error:	attach_put_fd(at);
out:	return err;
}

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

int sna_attach_execute_tp(u_int32_t tcb_id, struct sk_buff *skb)
{
        sna_debug(5, "init\n");
        memcpy(&skb->cb, &tcb_id, sizeof(unsigned long));
        skb_queue_tail(&attach_queue, skb);
        wake_up_interruptible(&attach_wait);
        return 0;
}

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

int sna_attach_release(struct attach *at)
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

void sna_attach_call(void *uaddr)
{
	attach_args *args = NULL;
	u_int32_t opcode;
	int err = -EINVAL;

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
		case ATTACH_OPEN:
                        err = sna_attach_open();
                        break;
		case ATTACH_CLOSE:
                        err = sna_attach_close((u_int32_t *)args->a3);
                        break;
		case ATTACH_LISTEN:
                        err = sna_attach_listen((u_int32_t *)args->a3, 
				(u_int32_t *)args->a4, (u_int32_t *)args->a5, 
				(u_int32_t *)args->a6);
                        break;
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
	return;
}

int sna_attach_get_info(char *buffer, char **start, off_t offset, int length)
{
	struct attach_tp_info *tp;
	off_t pos = 0, begin = 0;
        struct list_head *le;
        int len = 0;

	len += sprintf(buffer, "%-33s%-5s\n", "name", "index");
	list_for_each(le, &attach_tp_list) {
		tp = list_entry(le, struct attach_tp_info, list);
		len += sprintf(buffer + len, "%-33s%-5d\n", tp->name, tp->index);

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
	
static struct attach_ops aops = {
	PF_SNA,
	sna_attach_call,
	sna_attach_release
};

int sna_attach_init(void)
{
	sna_debug(5, "init\n");
        skb_queue_head_init(&attach_queue);
	init_waitqueue_head(&attach_wait);
	attach_register(&aops);
#ifdef CONFIG_PROC_FS
	proc_sna_create("transaction_programs", 0, sna_attach_get_info);
#endif
	return 0;
}

int sna_attach_fini(void)
{
	sna_debug(5, "init\n");
#ifdef CONFIG_PROC_FS
	proc_sna_remove("transaction_programs");
#endif
	attach_unregister(PF_SNA);
	return 0;
};
