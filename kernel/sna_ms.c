/* sna_ms.c: Linux Systems Network Architecture implementation
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
#include <linux/if_arp.h>
#include <linux/proc_fs.h>
#include <linux/sna.h>

sna_ms_sub_vector_t *sna_ms_build_product_id_sv(sna_ms_product_id_c *sv_c)
{
	sna_ms_sw_prd_prg_num sw_prd_prg_num;
	sna_ms_sw_prd_level sw_prd_level;
	sna_ms_sw_prd_name sw_prd_name;
	sna_ms_vendor_id vendor_id;
	sna_ms_hw_prd_id hw_prd_id;
	sna_ms_product_id_s *sd;
	sna_ms_sub_vector_t *sv;

	sna_debug(5, "init\n");		/* static max sv size for now. */
	sv = sna_ms_sub_vector_alloc(SNA_MS_COMM_SV_KEY_PRODUCT_ID, 300, GFP_ATOMIC);
	if (!sv)
		return NULL;
	/* set static payload information. */
	sd = SNA_MS_SV_DATA(sv);
	sd->class = sv_c->class;
	SNA_MS_SV_ADD_LENGTH(sv, sizeof(*sd));

	/* sub sub vectors. */
	switch (sv_c->class) {
		case SNA_MS_PRODUCT_CLASS_NON_IBM_SW:
			memset(&sw_prd_level, 0xF0, sizeof(sw_prd_level));
			atoe_strcpy(sw_prd_level.version, sna_sw_prd_version);
			atoe_strcpy(sw_prd_level.release, sna_sw_prd_release);
			atoe_strcpy(sw_prd_level.modification, sna_sw_prd_modification);
			sna_ms_subsub_vector_put(sv, SNA_MS_PRODUCT_SF_SW_PRD_LEVEL, 
				sizeof(sw_prd_level), &sw_prd_level, GFP_ATOMIC);
			memset(&sw_prd_name, 0xF0, sizeof(sw_prd_name));
			atoe_strncpy(sw_prd_name.name, sna_product_name, 
				strlen(sna_product_name));
			sna_ms_subsub_vector_put(sv, SNA_MS_PRODUCT_SF_SW_PRD_NAME, 
                                strlen(sna_product_name), &sw_prd_name, GFP_ATOMIC);
			memset(&sw_prd_prg_num, 0xF0, sizeof(sw_prd_prg_num));
			atoe_strncpy(sw_prd_prg_num.number, sna_product_id,
				strlen(sna_product_id));
			sna_ms_subsub_vector_put(sv, SNA_MS_PRODUCT_SF_SW_PRD_PRG_NUM, 
                                sizeof(sw_prd_prg_num), &sw_prd_prg_num, GFP_ATOMIC);
			memset(&vendor_id, 0, sizeof(vendor_id));
			atoe_strncpy(vendor_id.vendor, sna_vendor, strlen(sna_vendor));
			sna_ms_subsub_vector_put(sv, SNA_MS_PRODUCT_SF_VENDOR_ID, 
                                strlen(sna_vendor), &vendor_id, GFP_ATOMIC);
			break;
		case SNA_MS_PRODUCT_CLASS_MAYBE_IBM_HW:
			memset(&hw_prd_id, 0xF0, sizeof(hw_prd_id));
			hw_prd_id.format = 0x10;
			sna_ms_subsub_vector_put(sv, SNA_MS_PRODUCT_SF_HW_PRD_ID,
				sizeof(hw_prd_id), &hw_prd_id, GFP_ATOMIC);
			break;
		case SNA_MS_PRODUCT_CLASS_IBM_HW:
                case SNA_MS_PRODUCT_CLASS_IBM_SW:
                case SNA_MS_PRODUCT_CLASS_NON_IBM_HW:
                case SNA_MS_PRODUCT_CLASS_MAYBE_IBM_SW:
		default:
                        sna_debug(1, "FIXME: use of unsupported product class.\n");
                        break;
	}
	return sv;
}
