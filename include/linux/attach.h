#ifndef _LINUX_ATTACH_H
#define _LINUX_ATTACH_H

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
