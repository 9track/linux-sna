/* sna_ms_vectors.h: Linux-SNA MS Control Vectors.
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

#ifndef __NET_SNA_MS_VECTORS_H
#define __NET_SNA_MS_VECTORS_H

/* common subvectors. */
#define SNA_MS_COMM_SV_KEY_PRODUCT_ID		0x11

typedef struct {
	u_int16_t	len;
	u_int16_t	key;
} sna_ms_major_vector_t;

typedef struct {
	u_int8_t        len;
	u_int8_t        key;
} sna_ms_sub_vector_t;

/* product id control vector details.
 */

/* sub field names. */
#define SNA_MS_PRODUCT_SF_HW_PRD_ID		0x00
#define SNA_MS_PRODUCT_SF_EM_PRD_ID		0x01
#define SNA_MS_PRODUCT_SF_SW_PRD_SRV_ID		0x02
#define SNA_MS_PRODUCT_SF_SW_PRD_LEVEL		0x04
#define SNA_MS_PRODUCT_SF_SW_PRD_NAME		0x06
#define SNA_MS_PRODUCT_SF_SW_PRD_CUST_ID	0x07
#define SNA_MS_PRODUCT_SF_SW_PRD_PRG_NUM	0x08
#define SNA_MS_PRODUCT_SF_SW_PRD_CUST_TIME	0x09
#define SNA_MS_PRODUCT_SF_MC_EC_LEVEL		0x0B
#define SNA_MS_PRODUCT_SF_HW_PRD_NAME		0x0E
#define SNA_MS_PRODUCT_SF_VENDOR_ID		0x0F

/* product classes. */
#define SNA_MS_PRODUCT_CLASS_IBM_HW		0x01
#define SNA_MS_PRODUCT_CLASS_IBM_SW		0x04
#define SNA_MS_PRODUCT_CLASS_NON_IBM_HW		0x09
#define SNA_MS_PRODUCT_CLASS_NON_IBM_SW		0x0C
#define SNA_MS_PRODUCT_CLASS_MAYBE_IBM_HW       0x03
#define SNA_MS_PRODUCT_CLASS_MAYBE_IBM_SW	0x0E

typedef struct {
	u_int8_t	format;
	u_int8_t	data[13];
} sna_ms_hw_prd_id;

typedef struct {
	u_int8_t	version[2];
	u_int8_t	release[2];
	u_int8_t	modification[2];
} sna_ms_sw_prd_level;

typedef struct {
	u_int8_t	name[30];
} sna_ms_sw_prd_name;

typedef struct {
	u_int8_t	number[7];
} sna_ms_sw_prd_prg_num;

typedef struct {
	u_int8_t	vendor[16];
} sna_ms_vendor_id;

#pragma pack(1)
typedef struct {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	u_int8_t	class:4,
			rsv:4;
#elif defined(__BIG_ENDIAN_BITFIELD)
	u_int8_t	rsv:4,
			class:4;
#else
#error  "Please fix <asm/byteorder.h>"
#endif
} sna_ms_product_id_c;
#pragma pack()

#pragma pack(1)
typedef struct {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	u_int8_t        class:4,
			rsv:4;
#elif defined(__BIG_ENDIAN_BITFIELD)
	u_int8_t        rsv:4,
			class:4;
#else
#error  "Please fix <asm/byteorder.h>"
#endif
} sna_ms_product_id_s;
#pragma pack()

/* sna ms major vector macros.
 */
#define SNA_MS_MV_LENGTH(len)      ((len) + sizeof(sna_ms_major_vector_t))
#define SNA_MS_MV_DATA(mv)         ((void*)(((char*)mv) + SNA_MS_MV_LENGTH(0)))
#define SNA_MS_MV_SPACE(len)       SNA_MS_MV_LENGTH(len)
#define SNA_MS_MV_PAYLOAD(mv,llen) ((mv)->len - SNA_MS_MV_SPACE((llen)))
#define SNA_MS_MV_NH_PAYLOAD(mv,llen) (ntohs((mv)->len) - SNA_MS_MV_SPACE((llen)))
#define SNA_MS_MV_NEXTDATA(mv)     ((void *)(SNA_MS_MV_DATA(mv) \
					+ SNA_MS_MV_PAYLOAD(mv, 0)))

#define sna_ms_major_vector_put(mvkey, mvlen)			\
({                                                              \
	sna_ms_major_vector_t *__mv = kmalloc(mvlen, GFP_KERNEL); \
	if (__mv) {						\
		memset(__mv, 0, mvlen);				\
		__mv->key       = mvkey;                        \
		__mv->len       = mvlen;                        \
	}							\
	__mv;                                                   \
})

/* sna ms sub vector macros.
 */
#define SNA_MS_SV_OK(sv, llen)    ((llen) > 0 && (sv)->len \
				  >= sizeof(sna_ms_sub_vector_t) \
				  && (sv)->len <= (llen))
#define SNA_MS_SV_NEXT(sv,attrlen) ((attrlen) -= (sv)->len, \
				  (sna_ms_sub_vector_t *)(((char *)(sv)) + (sv)->len))
#define SNA_MS_SV_LENGTH(len)     (sizeof(sna_ms_sub_vector_t) + (len))
#define SNA_MS_SV_SPACE(len)      SNA_MS_SV_LENGTH(len)
#define SNA_MS_SV_DATA(sv)        ((void*)(((char*)(sv)) + SNA_MS_SV_LENGTH(0)))
#define SNA_MS_SV_PAYLOAD(sv)     ((int)((sv)->len) - SNA_MS_SV_LENGTH(0))
#define SNA_MS_SV_NEXTDATA(sv)	  ((void *)(SNA_MS_SV_DATA(sv) \
					+ SNA_MS_SV_PAYLOAD(sv)))
#define SNA_MS_SV_ADD_LENGTH(sv,llen) ((sv)->len += (llen))

#define sna_ms_sub_vector_alloc(svkey, size, flag)		\
({                                                              \
	sna_ms_sub_vector_t *__sv = kmalloc(size, flag); 	\
	if (__sv) {                                             \
		memset(__sv, 0, size);                         	\
		__sv->key       = svkey;                        \
		__sv->len       = SNA_MS_SV_LENGTH(0); 		\
	}                                                       \
	__sv;                                                   \
})

#define sna_ms_sub_vector_doit(svkey, svlen, svdata, flag)    	\
({                                                              \
	sna_ms_sub_vector_t *__sv;                              \
	int __llen = SNA_MS_SV_LENGTH(svlen);                   \
								\
	__sv = kmalloc(__llen, flag);             		\
	if (__sv) {						\
		__sv->len = __llen;                             \
		__sv->key  = svkey;                             \
		memcpy(SNA_MS_SV_DATA(__sv), svdata, svlen);    \
	}							\
	__sv;                                                   \
})

#define sna_ms_subsub_vector_put(sv, svkey, svlen, svdata, flag)\
({                                                              \
	sna_ms_sub_vector_t *__sv1, *__sv2;                     \
	int __llen = SNA_MS_SV_LENGTH(svlen);                   \
								\
	__sv1 = sna_ms_sub_vector_doit(svkey, svlen, svdata, flag); \
	if (__sv1) {                                            \
		__sv2 = SNA_MS_SV_NEXTDATA(sv);                 \
		memcpy(__sv2, __sv1, __llen);                   \
		kfree(__sv1);                                   \
		sv->len += __llen;                              \
	}                                                       \
	sv;                                                     \
})

#define sna_ms_sub_vector_put(mv, svkey, svlen, svdata, flag) 	\
({                                                              \
	sna_ms_sub_vector_t *__sv1, *__sv2;			\
	int __llen = SNA_MS_SV_LENGTH(svlen);                   \
								\
	__sv1 = sna_ms_sub_vector_doit(svkey, svlen, svdata, flag); \
	if (__sv1) {						\
		__sv2 = SNA_MS_MV_NEXTDATA(mv);               	\
		memcpy(__sv2, __sv1, __llen);			\
		kfree(__sv1);					\
		mv->len += __llen;                              \
	}							\
	mv;                                                     \
})

#define sna_ms_sub_vector_parse(mv, pfn, args...)               \
({                                                              \
	sna_ms_sub_vector_t *__sv = SNA_MS_MV_DATA(mv);         \
	int __llen = SNA_MS_MV_NH_PAYLOAD(mv, 0);               \
	int __err = 0;                                          \
								\
	while (SNA_MS_SV_OK(__sv, __llen)) {                    \
		__err = pfn(__sv, SNA_MS_SV_DATA(__sv), ## args); \
		if (__err != 0)                                  \
		       break;                                   \
		__sv = SNA_MS_SV_NEXT(__sv, __llen);            \
	}                                                       \
	__err;                                                  \
})

extern sna_ms_sub_vector_t *sna_ms_build_product_id_sv(sna_ms_product_id_c *sv_c);
#endif	/* __NET_SNA_MS_VECTORS_H */
