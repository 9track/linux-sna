/* tn3270d.h: headers.
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

#ifndef _TN3270D_H
#define _TN3270D_H

#define _PATH_TN3270D_XML_HREF  "http://www.linux-sna.org/sna"
#define _PATH_TN3270D_XML_TOP   "tn3270"
#define _PATH_TN3270D_CONF      "/etc/sna.xml"
#define _PATH_TN3270D_CONF_MAX  300
#define _PATH_TN3270D_PID	"/var/run/tn3270d.pid"
#define _PATH_TN3270D_USER_TABLE "/var/run"

#define TN3270D_DIR_TIMEOUT	50000

#define new(p)          ((p) = calloc(1, sizeof(*(p))))
#define new_s(p, s)     ((p) = calloc(1, s))

#define tn3270_debug(level, format, arg...) do {                           \
	if (tn3270_debug_level >= level)                                   \
		printf("%s:%s: " format, __FILE__, __FUNCTION__, ## arg);  \
} while (0)

typedef struct {
	struct list_head	list;

	int			fd;
	struct sockaddr_in	from;

	tnXstream		*hoststream;
	tnXhost			*host;

	tnXbuff			tbuf;

	time_t			start_time;
	
	int			terminated;
} client_t;

typedef struct {
	struct list_head	list;
	
	unsigned long		ipaddr;
	unsigned long		netmask;
} allow_info;

typedef struct {
	struct list_head	list;

	char			use_name[40];
	int			debug_level;
	int			client_port;
	int			manage_port;
	int			test;
	int			limit;
	int			sysreq;
	char			pool[40];
	
	struct list_head	allow_list;
	
	/* run-time information. */
	int			client_sock;
	int			manage_sock;

	struct list_head	client_list;
	int			client_num;
	
	fd_set			all_fds;
	unsigned long 		open_fds;
        unsigned long 		wmark_fd;
        unsigned long 		highest_fd;
} server_info;

#define DEFAULT_CLIENT_PORT 2023
#define DEFAULT_MANAGE_PORT 5023

#define tn3270e_type(data) 		data[0]
#define tn3270e_sequence(data) 		data[3] >> 8 | data[4]
#define tn3270e_response_flag(data) 	data[2]
#define tn3270e_response(data) 		data[5]


extern int matches(const char *cmd, char *pattern);
extern int tn3270_read_config_file(char *cfile);
extern int tn3270_print_config(struct list_head *list);
#endif	/* _TN3270D_H */
