/* tnX_buffer.c: generic tn buffer functions.
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
#include <ctype.h>
#include <glib.h>
#include <netinet/in.h>

/* our stuff. */
#include "libtnX.h"

#define BUFFER_DELTA 128

/* tnX_buffer_init
 * SYNOPSIS
 *    tnX_buffer_init (&buf);
 * INPUTS
 *    Tn5250Buffer *	buf	  - Pointer to a Tn5250Buffer object.
 * DESCRIPTION
 *    Zeros internal members.
 */
void tnX_buffer_init(tnXbuff *buff)
{
	buff->allocated	= 0;
      	buff->len  	= 0;
      	buff->data 	= NULL;
      	return;
}

/* tnX_buffer_free
 * SYNOPSIS
 *    tnX_buffer_free (&buf);
 * INPUTS
 *    TnXBuffer *	buf	 - Pointer to a buffer object.
 * DESCRIPTION
 *    Frees variable-length data and zeros internal members.
 */
void tnX_buffer_free(tnXbuff *buff)
{
      	if (buff->data != NULL)
		g_free(buff->data);
      	buff->data 	= NULL;
      	buff->len 	= 0;
	buff->allocated = 0;
      	return;
}

/* tnX_buffer_append_byte
 * SYNOPSIS
 *    tnX_buffer_append_byte (&buf, byte);
 * INPUTS
 *    TnXBuffer *	buf	 - Pointer to a buffer object.
 *    unsigned char byte	 - The byte to append.
 * DESCRIPTION
 *    Appends a single byte to the end of the variable-length data
 *    and reallocates the buffer if necessary to accomodate the new
 *    byte.
 */
void tnX_buffer_append_byte(tnXbuff *buff, unsigned char b)
{
	if (buff->len + 1 >= buff->allocated) {
		if (buff->data == NULL) {
			buff->allocated = BUFFER_DELTA;
	 		buff->data = (unsigned char *)g_malloc(buff->allocated);
      		} else {
	 		buff->allocated += BUFFER_DELTA;
	 		buff->data = (unsigned char *)g_realloc(buff->data, 
				buff->allocated);
      		}
   	}
   	tnX_assert(buff->data != NULL);
   	buff->data[buff->len++] = b;
	return;
}

/* tnX_buffer_append_data
 * SYNOPSIS
 *    tnX_buffer_append_data (&buf, data, len);
 * INPUTS
 *    TnXBuffer *	buf	    - Pointer to a buffer object.
 *    unsigned char *	data	    - Data to append.
 *    int		len	    - Length of data to append.
 * DESCRIPTION
 *    Appends a variable number of bytes to the end of the variable-length
 *    data and reallocates the buffer if necessary to accomodate the new
 *    data.
 */
void tnX_buffer_append_data(tnXbuff *buff, unsigned char *data, int len)
{
   	while (len--)
      		tnX_buffer_append_byte(buff, *data++);
	return;
}

/* tnX_buffer_log
 * SYNOPSIS
 *    tnX_buffer_log (&buf,"> ");
 * INPUTS
 *    TnXBuffer *	buf	    - Pointer to a buffer object.
 *    const char *	prefix	    - Character string to prefix dump lines
 *                                    with in log.
 * DESCRIPTION
 *    Dumps the contents of the buffer to the 5250 logfile, if one is
 *    currently open.
 */
void tnX_buffer_log(tnXbuff *buff, const char *prefix)
{
   	int pos;
   	unsigned char t[17];
   	unsigned char c;
   	unsigned char a;
   	int n;
   	tnXchar_map *map = tnX_char_map_new("37");

   	tnX_log(("Dumping buffer (length=%d):\n", buff->len));
   	for (pos = 0; pos < buff->len;) {
      		memset(t, 0, sizeof(t));
      		tnX_log(("%s +%4.4X ", prefix, pos));
      		for (n = 0; n < 16; n++) {
	 		if (pos < buff->len) {
	    			c = buff->data[pos];
	    			a = tnX_char_map_to_local(map, c);
	    			tnX_log(("%02x", c));
	    			t[n] = (isprint(a)) ? a : '.';
	 		} else
	    			tnX_log(("  "));
	 		pos++;
	 		if ((pos & 3) == 0)
	    			tnX_log((" "));
      		}
      		tnX_log((" %s\n", t));
   	}
   	tnX_log (("\n"));
	return;
}
