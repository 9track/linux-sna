/* tnX_record.c: generic tn record functions.
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

/* our stuff. */
#include "libtnX.h"

tnXrec *tnX_record_new()
{
   tnXrec *This = tnX_new(tnXrec, 1);
   if (This == NULL)
      return NULL;

   tnX_buffer_init (&(This->data));

   This->cur_pos = 0;
   This->prev = NULL;
   This->next = NULL;
   return This;
}

void tnX_record_destroy(tnXrec * This)
{
   if (This != NULL) {
      tnX_buffer_free (&(This->data));
      free(This);
   }
}

unsigned char tnX_record_get_byte(tnXrec * This)
{
   This->cur_pos++;
   tnX_assert(This->cur_pos <= tnX_record_length(This));
   return (tnX_buffer_data (&(This->data)))[This->cur_pos - 1];
}

void tnX_record_dump(tnXrec *This)
{
   	tnX_buffer_log(&(This->data),"@record");
   	tnX_log(("@eor\n"));
	return;
}

tnXrec *tnX_record_list_add(tnXrec *list, tnXrec *record)
{
   	if (list == NULL) {
      		list = record->next = record->prev = record;
      		return list;
   	}
   	record->next = list;
   	record->prev = list->prev;
   	record->prev->next = record;
   	record->next->prev = record;
   	return list;
}

tnXrec *tnX_record_list_remove(tnXrec *list, tnXrec *record)
{
   	if (list == NULL)
      		return NULL;
   	if (list->next == list) {
      		record->prev = record->next = NULL;
      		return NULL;
   	}

   	if (list == record)
      		list = list->next;

   	record->next->prev = record->prev;
   	record->prev->next = record->next;
   	record->next = record->prev = NULL;
   	return list;
}

tnXrec *tnX_record_list_destroy(tnXrec *list)
{
   	tnXrec *iter, *next;

   	if ((iter = list) != NULL) {
      		do {
         		next = iter->next;
         		tnX_record_destroy(iter);
         		iter = next;
      		} while (iter != list);
   	}
   	return NULL;
}
