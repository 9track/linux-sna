/* snatchd.h: definitions for everything unholy :)
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


#define TOOMANY         40              /* don't start more than TOOMANY */
#define CNT_INTVL       60              /*   servers in CNT_INTVL seconds */

#define RETRYTIME       (60*10)         /* retry time after bind/server fail */

#define RPC                             /* Use SunRPC */


/* globals */
extern const char *configfile;          /* /etc/inetd.conf or alternate */
extern int debug;                       /* debug flag */

/* util functions */
void *domalloc(size_t len);
char *dostrdup(const char *str);

void sig_init(void);
void sig_block(void);
void sig_unblock(void);
void sig_wait(void);
void sig_preexec(void);

/* signal handlers */
void config(int);     /* servtab.c */
void reapchild(int);  /* inetd.c */
void retry(int);      /* inetd.c */
void goaway(int);     /* inetd.c */
