/* sna_ds.c: Linux Systems Network Architecture implementation
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

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/sna.h>

#include "sna_common.h"

static LIST_HEAD(ds_clients);

struct sna_ds_pinfo *sna_ds_find(__u8 *name)
{
	struct sna_ds_pinfo *ds;
	struct list_head *le;

	list_for_each(le, &ds_clients) {
		ds = list_entry(le, struct sna_ds_pinfo, list);
		if (!strncmp(ds->netid.name, name, SNA_NODE_NAME_LEN))
			return ds;
	}
	return NULL;
}

int sna_ds_shutdown(void)
{
	struct list_head *le, *se;
	struct sna_ds_pinfo *ds;

	sna_debug(5, "init\n");
	list_for_each_safe(le, se, &ds_clients) {
		ds = list_entry(le, struct sna_ds_pinfo, list);
		list_del(&ds->list);
		kfree(ds);
	}
	return 0;
}

int sna_ds_create(struct sna_nof_node *start)
{
	struct sna_ds_pinfo *ds;

	sna_debug(5, "init: %s\n", start->netid.name);
	ds = sna_ds_find(start->netid.name);
	if (ds)
		return -EEXIST;
	new(ds, GFP_ATOMIC);
	if (!ds)
		return -ENOMEM;
	memcpy(&ds->netid, &start->netid, sizeof(sna_netid));
	list_add_tail(&ds->list, &ds_clients);
	return 0;
}

int sna_ds_destroy(struct sna_nof_node *delete)
{
	struct list_head *le, *se;
	struct sna_ds_pinfo *ds;

	sna_debug(5, "init\n");
	list_for_each_safe(le, se, &ds_clients) {
		ds = list_entry(le, struct sna_ds_pinfo, list);
		if (!strncmp(ds->netid.name, delete->netid.name, 8)) {
			list_del(&ds->list);
			kfree(ds);
			return 0;
		}
	}
	return -ENOENT;
}

int sna_ds_update_directory(void)
{
	return 0;
}

int sna_ds_update_cp_status(void)
{
	return 0;
}

int sna_ds_locate_message(void)
{
	return 0;
}

int sna_ds_request_local_search(void)
{
	return 0;
}
