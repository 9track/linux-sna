/* tn3270_list.c: structure list functions.
 *  Written by Jay Schulist <jschlst@samba.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>

/* our stuff. */
#include "tn3270_list.h"

void list_add(struct list_head *new, struct list_head *prev,
        struct list_head *next)
{                       
        next->prev = new;   
        new->next  = next;
        new->prev  = prev;      
        prev->next = new;       
}                                       
                                        
void list_add_tail(struct list_head *new, struct list_head *head)
{                               
        list_add(new, head->prev, head);
}                       

void __list_del(struct list_head *prev, struct list_head *next)
{                       
        next->prev = prev;
        prev->next = next;
}

void list_del(struct list_head *entry)
{
        __list_del(entry->prev, entry->next);
}
