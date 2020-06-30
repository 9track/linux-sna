/* sna_ss.c: Linux Systems Network Architecture implementation
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
#include <linux/list.h>

#include <linux/sna.h>

static LIST_HEAD(ss_clients);

struct sna_ss_pinfo *sna_ss_find(char *name)
{
        struct sna_ss_pinfo *ss;
	struct list_head *le;
	
        sna_debug(5, "init\n");
	list_for_each(le, &ss_clients) {
		ss = list_entry(le, struct sna_ss_pinfo, list);
                if (!strncmp(ss->netid.name, name, SNA_NODE_NAME_LEN))
			return ss;
	}
	return NULL;
}

int sna_ss_shutdown(void)
{
	struct list_head *le, *se;
        struct sna_ss_pinfo *ss;

        sna_debug(5, "init\n");
	list_for_each_safe(le, se, &ss_clients) {
		ss = list_entry(le, struct sna_ss_pinfo, list);
		list_del(&ss->list);
                kfree(ss);
        }
        return 0;
}

int sna_ss_create(struct sna_nof_node *start)
{
	struct sna_ss_pinfo *ss;

	sna_debug(5, "init: %s\n", start->netid.name);
	ss = sna_ss_find(start->netid.name);
	if (ss)
		return -EEXIST;
	new(ss, GFP_ATOMIC);
	if (!ss)
		return -ENOMEM;
	memcpy(&ss->netid, &start->netid, sizeof(sna_netid));
	list_add_tail(&ss->list, &ss_clients);
	return 0;
}

int sna_ss_destroy(struct sna_nof_node *delete)
{
	struct list_head *le, *se;
        struct sna_ss_pinfo *ss;

        sna_debug(5, "init\n");
	list_for_each_safe(le, se, &ss_clients) {
		ss = list_entry(le, struct sna_ss_pinfo, list);
                if (!strncmp(ss->netid.name, delete->netid.name, 8)) {
			list_del(&ss->list);
                        kfree(ss);
                        return 0;
                }
        }
        return -ENOENT;
}

int sna_ss_act_cp_cp_session(void)
{
	return 0;
}

int sna_ss_deact_cp_cp_session(void)
{
	return 0;
}

int sna_ss_act_control_sessions(void)
{
	return 0;
}

int sna_ss_deact_control_sessions(void)
{
	return 0;
}

int sna_ss_update_node_authorization(void)
{
	return 0;
}

int sna_ss_update_search_control_vectors(void)
{
	return 0;
}

int sna_ss_process_sesst_signal(void)
{
	return 0;
}

int sna_ss_process_sessend_signal(void)
{
	return 0;
}

int sna_ss_isr_init(void)
{
	return 0;
}

int sna_ss_isr_sessend(void)
{
	return 0;
}

static u_int32_t sna_pc_system_id  = 1;

/*
  Returns a string of 0 and 1 characters, representing the provided 64bit
  number in binary.
*/
void rl_binary(unsigned char * number, unsigned char * result)
{
  	int curbyte;
  	int resultidx;
  	unsigned char curbit;

  	resultidx = 63;
  	result[64] = '\0';

  	for (curbyte=0; curbyte < 8; curbyte++) {
      		for (curbit = 0; curbit < 8; curbit++) {
          		result[resultidx] = (number[curbyte] & (1<<curbit)) ? '1' : '0';
          		resultidx--;
        	}
    	}
	return;
}

/*
  Subtracts two 64bit numbers.  The top value must be >= the bottom.
 */
void rl_sub(unsigned char *top, unsigned char *bottom)
{
  	unsigned int curbyte;
  	int borrowed;
  	int curtop;

  	if (top[7] < bottom[7])
      		return;
  	borrowed = 0;
  	for (curbyte = 0; curbyte < 8; curbyte++) {
      		curtop = top[curbyte];

      		if (borrowed) {
          		if (curtop == 0) {
              			curtop = 0xFF;
              			borrowed = 1;
            		} else {
              			curtop = curtop--;
              			borrowed = 0;
            		}
        	}

      		if (curtop < bottom[curbyte]) {
          		curtop = curtop + 0x100;
          		borrowed = 1;
        	}
      		curtop = curtop - bottom[curbyte];
      		top[curbyte] = curtop;
    	}
	return;
}

/*
  Shifts the 64bit number one byte to the left.  Zero is inserted in the lsb.
*/
int rl_left(unsigned char *number)
{
        int carry;
        unsigned char mask;
        int curbyte;

        if (number[7] & 0x80)
                carry = 1;
        else
                carry = 0;

        for (curbyte = 7; curbyte >= 0; curbyte--) {
                if (curbyte > 0) {
                        mask = (number[curbyte-1] & 0x80) ? 0x01 : 0x00;
                        number[curbyte] = (number[curbyte] << 1) | mask;
                } else
                        number[curbyte] <<= 1;
        }
        return carry;
}

/*
  Shifts the 64bit number one bit to the right.  Zero is inserted as the msb
 */
void rl_right(unsigned char * number)
{
  	unsigned char mask;
  	int curbyte;

  	for (curbyte = 0; curbyte < 8; curbyte++) {
      		if (curbyte < 7) {
          		mask = (number[curbyte+1] & 0x01) ? 0x80 : 0x00;
          		number[curbyte] = (number[curbyte] >> 1) | mask;
        	} else {
          		number[curbyte] >>= 1;
        	}
    	}
	return;
}

/*
  Shifts the divisor to the left as necessary to align the most signifcant
  bit with the msb of the dividend.
 */
void rl_align(unsigned char * dividend, unsigned char * divisor)
{
  	int maxbyte;
  	int maxbit;

  	for (maxbyte = 7; maxbyte >= 0; maxbyte--) {
      		if (dividend[maxbyte] > 0)
        		break;
    	}
  	for (maxbit = 8; maxbit >= 0; maxbit--) {
      		if (dividend[maxbyte] & (1 << maxbit)) 
          		break;
    	}
  	while (!(divisor[maxbyte] & (1 << maxbit)))
    		rl_left(divisor);
	return;
}

/*
  Returns true if the dividend is greater than or equal to the divisor.
 */
int rl_lt(unsigned char * dividend, unsigned char * divisor)
{
  	int maxbyte;
  	int curbyte;

  	for (maxbyte = 7; maxbyte > 0; maxbyte--) {
      		if ((dividend[maxbyte] != 0) || (divisor[maxbyte] != 0))
          		break;
    	}
  	for (curbyte = maxbyte; curbyte > 0; curbyte--) {
      		if (dividend[curbyte] == divisor[curbyte])
          		continue;
		else
          		return dividend[curbyte] >= divisor[curbyte];
        }
  	return 0;
}

/*
  Divides two 64 bit numbers.  The numbers need to be passed as 8 element
  arrays of unsigned char.  The quotient is returned in quotient, and the
  remainder is returned in dividend.  The divisor is shifted during the
  division, so don't count on it having the same value after invocation.
 */
void rl_div(unsigned char * dividend, unsigned char * divisor,
       unsigned char * quotient)
{
  	unsigned char tempdiv[8];
  	int curbyte;

  	memset(quotient, 0, 8);

  	for (curbyte=0; curbyte < 8; curbyte++)
      		tempdiv[curbyte] = divisor[curbyte];
  	rl_align(dividend, divisor);

  	do {
    		if (rl_lt(dividend, divisor)) {
        		rl_sub(dividend, divisor);
        		rl_left(quotient);
        		quotient[0] = quotient[0] | 0x01;
      		} else {
        		rl_left(quotient);
      		}
    		rl_right(divisor);
  	} while (rl_lt(dividend, tempdiv));
	return;
}

/* Generates a FQPCID in the and returns it in the correct order. */
int sna_ss_generate_pcid(u_int32_t *hash1, u_int32_t *hash2, 
	char *net, char *name)
{
        unsigned char prime1[8] = {0xC7,0xFF,0xFF,0x0F,0x00,0x00,0x00,0x00};
        unsigned char prime2[8] = {0x1F,0x88,0x53,0x7E,0x00,0x00,0x00,0x00};
        unsigned char pname[8], pnet[8], fname[8], fnet[8];
        unsigned char quotient[8], dividend[8], dividend2[8];
        unsigned long scratch1;
        struct timeval time;
        int i;

        /* Dividend */
        atoe_strncpy(pname, name, strlen(name));
        for (i = strlen(name); i < 8; i++)
                pname[i] = 0x40;
        for (i = 0; i < 8; i++)          /* reverse order */
                fname[i] = pname[7-i];
        atoe_strncpy(pnet, net, strlen(net));
        for (i = strlen(net); i < 8; i++)
                pnet[i] = 0x40;
        etor_strncpy(pnet, pnet, 8);
        for (i = 0; i < 8; i++)          /* reverse order */
                fnet[i] = pnet[7-i];
        for (i = 0; i < 8; i++)
                dividend[i] = fname[i] ^ fnet[i];
        memcpy(dividend2, dividend, 8);

        /* Hash 1 */
        rl_div(dividend2, prime1, quotient);
        memcpy(hash1, dividend2, sizeof(unsigned long));
        scratch1 = (*hash1 ^ (unsigned short)*hash1) << 2;
        set_bit(16, &scratch1);
        set_bit(17, &scratch1);
        set_bit(30, &scratch1);
        set_bit(31, &scratch1);
        *hash1 = (scratch1 | (unsigned short)*hash1);
        *hash1 += htonl(sna_pc_system_id++);

        sna_debug(5, "HASH1 - %04lX\n", *hash1);

        /* Hash 2 */
        memcpy(dividend2, dividend, 8);
        rl_div(dividend2, prime2, quotient);
        memcpy(hash2, dividend2, sizeof(unsigned long));

        do_gettimeofday(&time);
        *hash2 += time.tv_sec;

        sna_debug(5, "HASH2 - %04lX\n", *hash2);
        return 0;
}

int sna_ss_update_pcid(u_int32_t *hash1, u_int32_t *hash2, sna_fqpcid *r)
{
        unsigned char result[8], scratch2[8];
        unsigned long nhash1, nhash2;

        *hash2 = *hash2 + 1;

        /* Generate final pcid */
        memcpy(scratch2, &hash1, 4);
        memcpy(scratch2 + 4, &hash2, 4);
        nhash1 = ntohl(*hash1);
        nhash2 = ntohl(*hash2);
        memcpy(result, &nhash1, 4);
        memcpy(result + 4, &nhash2, 4);
        memcpy(r, result, 8);
        return 0;
}

/**
 * @lulu: lulu control block to build a fqpcid.
 *
 * function must ensure never to return a duplicate.
 */
int sna_ss_assign_pcid(struct sna_lulu_cb *lulu)
{
	struct sna_remote_lu_cb *remote_lu;
	u_int32_t hash1 = 0, hash2 = 0;
	int err;
	
	sna_debug(5, "init\n");
	remote_lu = sna_rm_remote_lu_get_by_index(lulu->remote_lu_index);
        if (!remote_lu)
        	return -ENOENT;
	err = sna_ss_generate_pcid(&hash1, &hash2, 
		remote_lu->netid.net, remote_lu->netid.name);
	if (err < 0)
		goto out;
	err = sna_ss_update_pcid(&hash1, &hash2, &lulu->fqpcid);
	if (err < 0)
		goto out;
out:	return err;
}

/* entrance from the lu into the control point, activate the link.
 * this function sleeps until the link is active or times out.
 */
int sna_ss_proc_init_sig(struct sna_init_signal *init)
{
	struct sna_tg_cb *tg;
	u_int32_t pc_index;
	int err = 0;

	sna_debug(5, "init\n");

	/* we need to do cos to complete this function.
	 * err = sna_cosm_cos_tpf_vector(cos);
	 * if (err < 0)
	 *	return err;
	 */

	/* returns the tg which we need to use. 
	 * it would make so much more sense to use remote_name... 
	 */
	tg = sna_rss_req_single_hop_route(&init->local_name);
	if (!tg)
		return -ENOENT;

	/* function blocks until route is active or we give up. */
	pc_index = sna_cs_activate_route(tg, &init->remote_name, &err);
	if (err < 0)
		err = sna_sm_proc_init_sig_neg_rsp(init->index);
	else
		err = sna_sm_proc_cinit_sig_rsp(init->index, pc_index);
	return err;
}
