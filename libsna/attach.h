/* attach.h:
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

#ifndef _ATTACH_H
#define _ATTACH_H
int tp_register(int a, struct tp_info *tp);
int tp_unregister(int a, int b);
int tp_correlate(int s, pid_t pid, unsigned long tcb_id, char *tp_name);

int attach_open(void);
int attach_listen(int s, void *buf, int len, unsigned int flags);
int attach_close(int s);
#endif	/* _ATTACH_H */
