/* libtnX.c: generic tn server functions.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <glib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* our stuff. */
#include "libtnX.h"
#include "transmaps.h"

/* tnX_closeall
 * SYNOPSIS
 *    tnX_closeall (fd);
 * INPUTS
 *    int fd    - The starting file descriptor.
 * DESCRIPTION
 *    Closes all file descriptors >= a specified value.
 */
void tnX_closeall(int fd)
{
    	int fdlimit = sysconf(_SC_OPEN_MAX);

    	while (fd < fdlimit)
      		close(fd++);
	return;
}

/* Signal handler for SIGCHLD.  We use waitpid instead of wait, since there
 * is no way to tell wait not to block if there are still non-terminated child
 * processes.
 */
void sig_child(int signum)
{
  	int pid, status;

  	while ((pid = waitpid(-1, &status, WNOHANG)) > 0);
  	return;
}

/* tnX_daemon
 * SYNOPSIS
 *    ret = tnX_daemon (nochdir, noclose);
 * INPUTS
 *    int nochdir       - 0 to perform chdir.
 *    int noclose       - 0 to close all file handles.
 * DESCRIPTION
 *    Detach process from user and disappear into the background
 *    returns -1 on failure, but you can't do much except exit in that 
 *    case since we may already have forked. Believed to work on all 
 *    Posix systems.
 */
int tnX_daemon(int nochdir, int noclose, int ignsigcld)
{
  	struct sigaction sa;

    	switch (fork()) {
        	case 0:  
			break;
        	case -1: 
			return -1;
        	default: 
			_exit(0);          /* exit the original process */
    	}

    	if (setsid() < 0)               /* shoudn't fail */
      		return -1;

    	/* dyke out this switch if you want to acquire a control tty in */
    	/* the future -- not normally advisable for daemons */
    	switch (fork()) {
        	case 0:  
			break;
        	case -1: 
			return -1;
        	default: 
			_exit(0);
    	}

    	if (!nochdir)
      		chdir("/");

    	if (!noclose) {
        	tnX_closeall(0);
        	open("/dev/null",O_RDWR);
        	dup(0); 
		dup(0);
    	}

    	umask(0);

    	if (ignsigcld) {
      		sa.sa_handler = sig_child;
      		sigemptyset(&sa.sa_mask);
      		sa.sa_flags = SA_RESTART;

#ifdef SIGCHLD
      		sigaction(SIGCHLD, &sa, NULL);
#else
#ifdef SIGCLD
      		sigaction(SIGCLD, &sa, NULL);
#endif
#endif
    	}
    	return 0;
}

int tnX_make_socket(unsigned short int port)
{
  	int sock;
  	int on = 1;
  	struct sockaddr_in name;
  	u_long ioctlarg = 0;

  	/* Create the socket. */
  	sock = socket(PF_INET, SOCK_STREAM, 0);
  	if (sock < 0) {
      		syslog(LOG_INFO, "socket: %s\n", strerror(errno));
      		exit(EXIT_FAILURE);
    	}

  	/* Give the socket a name. */
  	name.sin_family 	= AF_INET;
  	name.sin_port 		= htons(port);
  	name.sin_addr.s_addr 	= htonl(INADDR_ANY);
  	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  	tnX_ioctl(sock, FIONBIO, &ioctlarg);
  	if (bind(sock, (struct sockaddr *) &name, sizeof (name)) < 0) {
      		syslog(LOG_INFO, "bind: %s\n", strerror(errno));
      		exit(EXIT_FAILURE);
    	}
  	return sock;
}

/* tnX_char_map_to_remote
 * SYNOPSIS
 *    ret = tnX_char_map_to_remote (map,ascii);
 * INPUTS
 *    TnXChar           ascii      - The local character to translate.
 * DESCRIPTION
 *    Translate the specified character from local to remote.
 */
tnXchar tnX_char_map_to_remote(tnXchar_map *map, tnXchar ascii)
{
   	return map->to_remote_map[ascii];
}

tnXchar tnX_char_map_to_local(tnXchar_map *map, tnXchar ebcdic)
{
   	switch (ebcdic) {
   		case 0x1C:
      			return '*'; /* This should be an overstriken asterisk (DUP) */
   		case 0:
      			return ' ';
   		default:
      			return map->to_local_map[ebcdic];
   	}
}

/* tnX_char_map_new
 * SYNOPSIS
 *    cmap = tnX_char_map_new ("37");
 * INPUTS
 *    const char *         map        - Name of the character translation map.
 * DESCRIPTION
 *    Create a new translation map.
 * NOTES
 *    Translation maps are currently statically allocated, although you should
 *    call tnX_char_map_destroy (a no-op) for future compatibility.
 */
tnXchar_map *tnX_char_map_new(const char *map)
{
   	tnXchar_map *t;
   	for (t = tnX_transmaps; t->name; t++) {
      		if (strcmp(t->name, map) == 0)
         		return t;
   	}
   	return NULL;
}

/* tnX_char_map_destroy
 * SYNOPSIS
 *    tnX_char_map_destroy (map);
 * INPUTS
 *    TnXCharMap *         map     - The character map to destroy.
 * DESCRIPTION
 *    Frees the character map's resources.
 */
void tnX_char_map_destroy(tnXchar_map *map)
{
   	/* NOOP */
}

GSList *tnX_build_addr_list(GSList * addrlist)
{
  GSList * iter;
  GSList * result;
  char * slash;
  clientaddr * addr;
  struct in_addr tempaddr;
  unsigned long int mask;
  int rc;
  char *tail;

  iter = addrlist;
  result = NULL;

  printf("like\n");
  if(iter == NULL) {
    addr = g_new(clientaddr, 1);

    rc = inet_aton("127.0.0.1", &tempaddr);

    addr->address = tempaddr.s_addr;
    addr->mask = 0xFFFFFFFF;

    result = g_slist_append(result, addr);
  }

  printf("now\n");
  while(iter != NULL)
    {
	    printf("l\n");
	printf("ll %s\n", iter->data);
      slash = strchr(iter->data, '/');
      if(slash == NULL)
        goto error_out;

      *slash = '\0';

      printf("to\n");
      printf("%s\n", (char *)iter->data);
      rc = inet_aton(iter->data, &tempaddr);
      if(rc == 0)
        goto error_out;

      slash++;

      mask = strtol(slash, &tail, 0);

      if(*tail != '\0')
        goto error_out;

      mask = 0xFFFFFFFF << 32-mask;

      mask = htonl(mask);

      addr = g_new(clientaddr, 1);

      addr->address = tempaddr.s_addr;
      addr->mask = mask;

      result = g_slist_append(result, addr);

      iter = g_slist_next(iter);
    }

  return(result);

 error_out:
  return(NULL);
}

int tnX_valid_client(GSList * addrlist, unsigned long int client)
{
  GSList * iter;
  unsigned long int config;
  unsigned long int mask;
  clientaddr * data;

  iter = addrlist;

  while(iter != NULL)
    {
      data = (clientaddr *)iter->data;
      config = data->address;
      mask   = data->mask;

      if( (config & mask) == (client & mask) )
        return(1);

      iter = g_slist_next(iter);
    }

  return(0);
}
