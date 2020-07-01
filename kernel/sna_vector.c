/* sna_vector.c: Linux Systems Network Architecture implementation
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
#include <linux/sna.h>

#include "sna_common.h"

int sna_vector_cos_tpf(u_int8_t *name, int net_pri, int tx_pri,
	struct sk_buff *skb)
{
	sna_cv_cos_tpf_s *cv_cos_tpf_s = NULL;
	sna_vector_t *cv_cos_tpf;
	u_int8_t e_name[8];
	int len;

	sna_debug(5, "init: -%s- strlen=%zu\n", name, strlen(name));
	atoe_strncpy(e_name, name, strlen(name));
	cv_cos_tpf = sna_vector_put_w_offset(SNA_CV_KEY_COS_TPF,
		strlen(name), e_name, sizeof(sna_cv_cos_tpf_s), cv_cos_tpf_s);
	if (!cv_cos_tpf)
		return -ENOMEM;
	if (!cv_cos_tpf_s)
		return -ENOMEM;
	cv_cos_tpf_s->net_pri = net_pri;
	if (!net_pri)
		cv_cos_tpf_s->tx_pri = tx_pri;
	cv_cos_tpf_s->cos_len = strlen(name);
	memcpy(skb_put(skb, SNA_CV_TLEN(cv_cos_tpf)), cv_cos_tpf,
		SNA_CV_TLEN(cv_cos_tpf));
	len = SNA_CV_TLEN(cv_cos_tpf);
	kfree(cv_cos_tpf);
	return len;
}

int sna_vector_fqpcid(sna_netid *name, sna_fqpcid *fqpcid_u, struct sk_buff *skb)
{
	sna_vector_t *cv_fqpcid;
	sna_cv_fqpcid fqpcid;
	u_int8_t name_flat[17];
	int len;

	sna_debug(5, "init\n");
	memset(&fqpcid, 0, sizeof(fqpcid));
	sna_netid_to_char(name, name_flat);
	fqpcid.name_len = strlen(name_flat);
	fatoe_strncpy(fqpcid.name, name_flat, strlen(name_flat));
	memcpy(fqpcid.pcid, fqpcid_u, sizeof(sna_fqpcid));
	cv_fqpcid = sna_vector_put(SNA_CV_KEY_FQPCID,
		strlen(name_flat) + 9, &fqpcid);
	if (!cv_fqpcid)
		return -ENOMEM;
	memcpy(skb_put(skb, SNA_CV_TLEN(cv_fqpcid)), cv_fqpcid,
		SNA_CV_TLEN(cv_fqpcid));
	len = SNA_CV_TLEN(cv_fqpcid);
	kfree(cv_fqpcid);
	return len;
}

int sna_vector_network_name(sna_netid *netid, struct sk_buff *skb)
{
	sna_vector_t *cv_netname;
	sna_cv_netname netname;
	u_int8_t netid_flat[17];
	int len;

	sna_debug(5, "init\n");
	memset(&netname, 0, sizeof(netname));
	netname.type = SNA_NETNAME_TYPE_CP;
	sna_netid_to_char(netid, netid_flat);
	fatoe_strncpy(netname.name, netid_flat, strlen(netid_flat));
	cv_netname = sna_vector_put(SNA_CV_KEY_NETNAME,
		strlen(netid_flat) + 1, &netname);
	if (!cv_netname)
		return -ENOMEM;
	memcpy(skb_put(skb, SNA_CV_TLEN(cv_netname)), cv_netname,
		SNA_CV_TLEN(cv_netname));
	len = SNA_CV_TLEN(cv_netname);
	kfree(cv_netname);
	return len;
}

int sna_vector_product_id(struct sk_buff *skb)
{
	sna_vector_t *cv_product_id;
	sna_cv_product_id_s *product_id_s = NULL;
	sna_ms_sub_vector_t *sv_ms_product_id_sw;
	sna_ms_sub_vector_t *sv_ms_product_id_hw;
	sna_ms_product_id_c product_id_c;
	void *product_id_vectors;
	int len;

	sna_debug(5, "init\n");
	memset(&product_id_c, 0, sizeof(product_id_c));
	product_id_c.class      = SNA_MS_PRODUCT_CLASS_NON_IBM_SW;
	sv_ms_product_id_sw     = sna_ms_build_product_id_sv(&product_id_c);
	product_id_c.class      = SNA_MS_PRODUCT_CLASS_MAYBE_IBM_HW;
	sv_ms_product_id_hw     = sna_ms_build_product_id_sv(&product_id_c);
	product_id_vectors      = kmalloc(sv_ms_product_id_sw->len
		+ sv_ms_product_id_hw->len, GFP_ATOMIC);
	if (!product_id_vectors)
		return -ENOMEM;
	memcpy(product_id_vectors, sv_ms_product_id_sw, sv_ms_product_id_sw->len);
	memcpy(product_id_vectors + sv_ms_product_id_sw->len,
		sv_ms_product_id_hw, sv_ms_product_id_hw->len);
	cv_product_id = sna_vector_put_w_offset(SNA_CV_KEY_PRODUCT_ID,
		sv_ms_product_id_sw->len + sv_ms_product_id_hw->len,
		product_id_vectors, sizeof(*product_id_s), product_id_s);
	if (!cv_product_id)
		return -ENOMEM;
	if (!product_id_s)
		return -ENOMEM;
	product_id_s->rsv = 0;
	memcpy(skb_put(skb, SNA_CV_TLEN(cv_product_id)), cv_product_id,
		SNA_CV_TLEN(cv_product_id));
	len = SNA_CV_TLEN(cv_product_id);
	kfree(sv_ms_product_id_sw);
	kfree(sv_ms_product_id_hw);
	kfree(product_id_vectors);
	kfree(cv_product_id);
	return len;
}
