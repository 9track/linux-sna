/* sna_vectors.h: Linux-SNA Control Vectors.
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

#ifndef __NET_SNA_VECTORS_H
#define __NET_SNA_VECTORS_H

#define SNA_CV_KEY_NODE_ID		0x00
#define SNA_CV_KEY_DATE_TIME		0x01
#define SNA_CV_KEY_SUBAREA_RT		0x02
#define SNA_CV_KEY_SDLC_2ND_ST		0x03
#define SNA_CV_KEY_NETWORK_ID		0x03
#define SNA_CV_KEY_LU			0x04
#define SNA_CV_KEY_CHANNEL		0x05
#define SNA_CV_KEY_NETWORK_ADDR		0x05
#define SNA_CV_KEY_CD_RSRC_MGR		0x06
#define SNA_CV_KEY_PRODUCT_ID		0x10
#define SNA_CV_KEY_NETNAME              0x0E
#define SNA_CV_KEY_XID_NEG_ERR		0x22
#define SNA_CV_KEY_COS_TPF		0x2C
#define SNA_CV_KEY_SHORT_HOLD_CTRL	0x32
#define SNA_CV_KEY_TG_DESC		0x46
#define SNA_CV_KEY_FQPCID		0x60
#define SNA_CV_KEY_HPR_CAP		0x61
#define SNA_CV_KEY_CP_NAME              0xF4

typedef struct {
	u_int8_t        key;
	u_int8_t        len;
} sna_vector_t;

#define SNA_NETNAME_TYPE_PU		0xF1
#define SNA_NETNAME_TYPE_LU		0xF3
#define SNA_NETNAME_TYPE_CP		0xF4
#define SNA_NETNAME_TYPE_SSCP		0xF5
#define SNA_NETNAME_TYPE_NNCP		0xF6
#define SNA_NETNAME_TYPE_LS		0xF7
#define SNA_NETNAME_TYPE_CP_PLU		0xF8
#define SNA_NETNAME_TYPE_CP_SLU		0xF9
#define SNA_NETNAME_TYPE_GENERIC	0xFA

typedef struct {
	u_int8_t	type;
	u_int8_t	name[17];
} sna_cv_netname;

typedef struct {
	u_int8_t	rsv;
} sna_cv_product_id_s;

#pragma pack(1)
typedef struct {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	u_int8_t	tx_pri:2,
			net_pri:1,
			rsv:5		__attribute__ ((packed));
	u_int8_t        cos_len;
#elif defined(__BIG_ENDIAN_BITFIELD)
	u_int8_t	rsv:5,
			net_pri:1,
			tx_pri:2	__attribute__ ((packed));
	u_int8_t        cos_len;
#else
#error  "Please fix <asm/byteorder.h>"
#endif
} sna_cv_cos_tpf_s;
#pragma pack()

typedef struct {
	u_int8_t	pcid[8];
	u_int8_t	name_len;
	u_int8_t	name[17];
} sna_cv_fqpcid;

typedef struct {
	u_int16_t	byte_offset;
	u_int8_t	bit_offset;
	u_int32_t	sense_data;
} sna_cv_xid_neg_err;

typedef struct {
	u_int8_t	rsv;
} sna_cv_short_hold_ctrl;

typedef struct {
	u_int8_t	cv;
} sna_cv_tg_desc;

typedef struct {
	u_int8_t	blah;
} sna_cv_hpr_cap;

/* sna vector macros.
 */
#define SNA_CV_OK(cv, llen)    	((llen) > 0 && (cv)->len >= sizeof(sna_vector_t) \
				&& (cv)->len <= (llen))
#define SNA_CV_NEXT(cv,attrlen) ((attrlen) -= (cv)->len, \
				(sna_vector_t *)(((char *)(cv)) + (cv)->len))
#define SNA_CV_TLEN(cv)		((cv)->len + sizeof(sna_vector_t))
#define SNA_CV_DLEN(len)	((len) - sizeof(sna_vector_t))
#define SNA_CV_LENGTH(len)      (sizeof(sna_vector_t) + (len))
#define SNA_CV_SPACE(len)       SNA_CV_LENGTH(len)
#define SNA_CV_DATA(cv)         ((void*)(((char*)(cv)) + SNA_CV_LENGTH(0)))
#define SNA_CV_DATA_W_OFFSET(cv,offset)  ((void*)(((char*)(cv)) + SNA_CV_LENGTH(offset)))
#define SNA_CV_PAYLOAD(cv)      ((int)((cv)->len) - SNA_CV_LENGTH(0))

#define sna_vector_put_w_offset(cvkey, cvlen, cvdata, olen, odata) \
({                                                              \
	sna_vector_t *__cv = NULL;                              \
	int __llen = SNA_CV_LENGTH(cvlen + olen);         	\
								\
	__cv = kmalloc(__llen, GFP_KERNEL);                     \
	if (__cv) {                                             \
		memset(__cv, 0, __llen);                        \
		__cv->len = SNA_CV_DLEN(__llen);                \
		__cv->key = cvkey;                              \
		memcpy(SNA_CV_DATA_W_OFFSET(__cv, olen), cvdata, cvlen); \
		odata = SNA_CV_DATA(__cv);			\
	}                                                       \
	__cv;                                                   \
})

#define sna_vector_put(cvkey, cvlen, cvdata)            	\
({                                                              \
	sna_vector_t *__cv = NULL;                              \
	int __llen = SNA_CV_LENGTH(cvlen);                     	\
								\
	__cv = kmalloc(__llen, GFP_KERNEL);			\
	if (__cv) {						\
		memset(__cv, 0, __llen);			\
		__cv->len = SNA_CV_DLEN(__llen);                \
		__cv->key = cvkey;                              \
		memcpy(SNA_CV_DATA(__cv), cvdata, cvlen);       \
	}							\
	__cv;                                                   \
})

#define sna_vector_parse(cvstart, cvtlen, pfn, args...)       	\
({                                                              \
	sna_vector_t *__cv = ((void *)(cvstart));              	\
	int __llen = cvtlen;                 			\
	int __err = 0;                                          \
								\
	while (SNA_CV_OK(__cv, __llen)) {                      	\
		__err = pfn(__cv, SNA_CV_DATA(__cv), ## args); 	\
		if(__err != 0)                                  \
		       break;                                   \
		__cv = SNA_CV_NEXT(__cv, __llen);              	\
	}                                                       \
	__err;                                                  \
})

extern int sna_vector_cos_tpf(u_int8_t *name, int net_pri, int tx_pri,
	struct sk_buff *skb);
extern int sna_vector_fqpcid(sna_netid *name, sna_fqpcid *fqpcid, struct sk_buff *skb);
extern int sna_vector_network_name(sna_netid *netid, struct sk_buff *skb);
extern int sna_vector_product_id(struct sk_buff *skb);
#endif	/* __NET_SNA_VECTORS_H */
