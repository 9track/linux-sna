/* cpic.h: CPI Communications Header.
 *
 * Author:
 * Jay Schulist         <jschlst@samba.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * None of the authors or maintainers or their employers admit
 * liability nor provide warranty for any of this software.
 * This material is provided "as is" and at no charge.
 */

#ifndef _NET_CPIC_H
#define _NET_CPIC_H

#ifdef __KERNEL__
struct cpic {
	struct list_head		list;
	
        u_int32_t              		state;
        u_int32_t			flags;

        struct cpic_ops         	*ops;
        struct sna_cpic_side_info   	*side;
        struct inode            	*inode;
        struct fasync_struct    	*fasync_list;
        struct file             	*file;
        pid_t                   	pid;

	wait_queue_head_t       	wait;
	
	union {
        	struct sna_tp_cb        *sna;
	} vi;
};

struct cpic_ops {
        int     family;

	void 	(*cpicall)	(unsigned char *conversation_id,
				unsigned short opcode, void *uaddr, 
				signed long int *return_code);
        int     (*release)      (struct cpic *cpic);
};

extern int cpic_register(struct cpic_ops *ops);
extern int cpic_unregister(int family);
extern void cpic_release(struct cpic *cpic);
extern struct cpic *cpic_alloc(void);
extern int cpic_map_fd(struct cpic *cpic);
extern struct cpic *cpic_lookup_fd(int fd, int *err);
extern void cpic_put_fd(struct cpic *cpic);
#endif  /* __KERNEL__ */
#endif /* _NET_CPIC_H */
