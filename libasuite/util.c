/* util - general utility functions for Linux-SNA asuite
 *
 * Author: Michael Madore <mmadore@turbolinux.com>
 *
 * Copyright (C) 2000 TurboLinux, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "util.h"
/* Allocate a unique area for a string. */

char *
savebuf (char const *s, size_t size)
{
  register char *rv;

  assert (s && size);
  rv = malloc (size);

  if (! rv)
    {
      memory_fatal ();
    }
  else
    memcpy (rv, s, size);

  return rv;
}

char *
savestr (char const *s)
{
  return savebuf (s, strlen (s) + 1);
}

void
memory_fatal(void)
{
  exit(EXIT_FAILURE);
}
