/* tnX_log.c: generic tn log functions.
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
#include <stdarg.h>
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

/* our stuff. */
#include "libtnX.h"

FILE *tnX_logfile = NULL;

/* tn5250_log_open
 * SYNOPSIS
 *    tn5250_log_open (fname);
 * INPUTS
 *    const char *         fname      - Filename of tracefile.
 * DESCRIPTION
 *    Opens the debug tracefile for this session.
 */
void tnX_log_open(const char *fname)
{
   	if (tnX_logfile != NULL)
      		fclose(tnX_logfile);
   	tnX_logfile = fopen(fname, "w");
   	if (tnX_logfile == NULL) {
      		perror(fname);
      		exit(1);
   	}
   	/* FIXME: Write $TERM, version, and uname -a to the file. */
#ifndef WIN32
   	/* Set file mode to 0600 since it may contain passwords. */
   	fchmod(fileno(tnX_logfile), 0600);
#endif
   	setbuf(tnX_logfile, NULL);
	return;
}

/* tn5250_log_close
 * SYNOPSIS
 *    tn5250_log_close ();
 * INPUTS
 *    None
 * DESCRIPTION
 *    Close the current tracefile if one is open.
 */
void tnX_log_close()
{
   	if (tnX_logfile != NULL) {
      		fclose(tnX_logfile);
      		tnX_logfile = NULL;
   	}
	return;
}

/* tn5250_log_printf
 * SYNOPSIS
 *    tn5250_log_printf (fmt, );
 * INPUTS
 *    const char *         fmt        - 
 * DESCRIPTION
 *    This is an internal function called by the TN5250_LOG() macro.  Use
 *    the macro instead, since it can be conditionally compiled.
 */
void tnX_log_printf(const char *fmt,...)
{
   	va_list vl;
   	if (tnX_logfile != NULL) {
      		va_start(vl, fmt);
      		vfprintf(tnX_logfile, fmt, vl);
      		va_end(vl);
   	}
	return;
}

/* tn5250_log_assert
 * SYNOPSIS
 *    tn5250_log_assert (val, expr, file, line);
 * INPUTS
 *    int                  val        - 
 *    char const *         expr       - 
 *    char const *         file       - 
 *    int                  line       - 
 * DESCRIPTION
 *    This is an internal function called by the TN5250_ASSERT() macro.  Use
 *    the macro instead, since it can be conditionally compiled.
 */
void tnX_log_assert(int val, char const *expr, char const *file, int line)
{
   	if (!val) {
      		tnX_log_printf("\nAssertion %s failed at %s, line %d.\n", 
			expr, file, line);
      		fprintf (stderr,"\nAssertion %s failed at %s, line %d.\n", 
			expr, file, line);
		syslog(LOG_ERR, "\nAssertion %s failed at %s, line %d.\n",
                        expr, file, line);
      		abort ();
   	}
	return;
}
