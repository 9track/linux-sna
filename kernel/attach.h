#ifndef _LINUX_ATTACH_H
#define _LINUX_ATTACH_H

#ifdef __KERNEL__
typedef enum {
	AT_RESET = 1,
	AT_INIT,
	AT_WAITING,
	AT_ATTACHED
} attach_state;

struct attach_tp_info {
	struct list_head 	list;

	u_int32_t		index;
	u_int8_t		name[64];

	attach_state	state;
	unsigned short	flags;
	pid_t		pid;

	unsigned char   cnv_type;
	unsigned char   sync_level;
	unsigned long   limit;
};

struct attach {
	unsigned short          flags;

	struct attach_ops       *ops;		/* Calls backs for this ID */
	struct inode            *inode;
	struct fasync_struct    *fasync_list;
	struct file             *file;

	wait_queue_head_t       wait;
};

struct attach_ops {
	int family;
	void (*call)	(void *uaddr);
	int  (*release)	(struct attach *at);
};

extern int attach_register(struct attach_ops *ops);
extern int attach_unregister(int family);
extern struct attach *attach_alloc(void);
extern struct attach *attach_lookup_fd(int fd, int *err);
extern void attach_release(struct attach *at);
extern int attach_map_fd(struct attach *at);
extern void attach_put_fd(struct attach *at);

#endif /* __KERNEL__ */

struct tp_info {
	char            tp_name[64];
	unsigned char   cnv_type;
	unsigned char   sync_level;
	unsigned long   limit;
	unsigned long   flags;
};

struct tp_attach {
	char            tp_name[64];
	int             tp_len;
	unsigned long   tcb_id;
	char            netid[17];
	char            luow_id[6];
	char            raw_attach_hdr[100];
};

typedef enum {
	ATTACH_TP_REGISTER = 1,
	ATTACH_TP_UNREGISTER,
	ATTACH_TP_CORRELATE,
	ATTACH_OPEN,
	ATTACH_LISTEN,
	ATTACH_CLOSE
} attach_opcodes;

/* Input/Output attach call structure. */
typedef struct {
	unsigned long   *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8, *a9;
} attach_args;

#define aargo1(a, v1)                   \
	a = (attach_args *)malloc(sizeof(attach_args)); \
	a->a1 = (unsigned long *)v1;

#define aargo2(a, v1, v2)               \
	a = (attach_args *)malloc(sizeof(attach_args)); \
	a->a1 = (unsigned long *)v1; \
	a->a2 = (unsigned long *)v2;

#define aargo3(a, v1, v2, v3)           \
	a = (attach_args *)malloc(sizeof(attach_args)); \
	a->a1 = (unsigned long *)v1; \
	a->a2 = (unsigned long *)v2; \
	a->a3 = (unsigned long *)v3;

#define aargo4(a, v1, v2, v3, v4)       \
	a = (attach_args *)malloc(sizeof(attach_args)); \
	a->a1 = (unsigned long *)v1; \
	a->a2 = (unsigned long *)v2; \
	a->a3 = (unsigned long *)v3; \
	a->a4 = (unsigned long *)v4;

#define aargo5(a, v1, v2, v3, v4, v5)   \
	a = (attach_args *)malloc(sizeof(attach_args)); \
	a->a1 = (unsigned long *)v1; \
	a->a2 = (unsigned long *)v2; \
	a->a3 = (unsigned long *)v3; \
	a->a4 = (unsigned long *)v4; \
	a->a5 = (unsigned long *)v5;

#define aargo6(a, v1, v2, v3, v4, v5, v6) \
	a = (attach_args *)malloc(sizeof(attach_args)); \
	a->a1 = (unsigned long *)v1; \
	a->a2 = (unsigned long *)v2; \
	a->a3 = (unsigned long *)v3; \
	a->a4 = (unsigned long *)v4; \
	a->a5 = (unsigned long *)v5; \
	a->a6 = (unsigned long *)v6;
#endif /* __LINUX_ATTACH_H */
