/* snatchd.h: headers.
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

#ifndef _SNATCHD_H
#define _SNATCHD_H

#define _PATH_SNATCHD_XML_HREF  	"http://www.linux-sna.org/sna"
#define _PATH_SNATCHD_XML_TOP   	"snatchd"
#define _PATH_SNATCHD_CONF      	"/etc/sna.xml"
#define _PATH_SNATCHD_CONF_MAX  	300
#define _PATH_SNATCHD_PID		"/var/run/snatchd.pid"
#define _PATH_SNATCHD_USER_TABLE 	"/var/run"

#define SNATCHD_DIR_TIMEOUT     	50000
#define SNATCHD_PATH_MAX		300
#define SNATCHD_USER_MAX		100
#define SNATCHD_MAX_ARGS		50

#define new(p)          ((p) = calloc(1, sizeof(*(p))))
#define new_s(p, s)     ((p) = calloc(1, s))

#define snatch_debug(level, format, arg...) do {                           \
        if (snatch_debug_level >= level)                                   \
               printf("%s:%s: " format, __FILE__, __FUNCTION__, ## arg);  \
} while (0)

#define TOOMANY         40              /* don't start more than TOOMANY. */
#define CNT_INTVL       60              /* servers in CNT_INTVL seconds. */
#define RETRYTIME       (60*10)         /* retry time after bind/server fail. */

typedef enum {
	TP_STATUS_RESET = 0,
	TP_STATUS_ACTIVE
} tp_status;

typedef struct {
	struct list_head	list;

	char			name[64];
	int			type;		/* conversation type. */
	int			sync_level;
	int			queued;
	int			limit;
	char			path[SNATCHD_PATH_MAX];
	char			user[SNATCHD_USER_MAX];
#ifdef ARGS
	char			*args[SNATCHD_MAX_ARGS + 1];
#endif
	/* run-time information. */
	int			fd;
	int			status;
	pid_t			pid;
	int			count;
	struct timeval		time;
} local_tp_info;

extern int snatch_read_config_file(char *cfile);
extern int snatch_print_config(struct list_head *list);
#endif	/* _SNATCHD_H */
