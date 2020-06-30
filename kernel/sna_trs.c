/* sna_trs.c: Linux Systems Network Architecture implementation
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

#include <linux/sna.h>

int sna_trs_create(struct sna_nof_node *start)
{
	sna_debug(5, "init\n");
	sna_cosm_create(start);
	sna_rss_create(start);
	sna_tdm_create(start);
	return 0;
}

int sna_trs_destroy(struct sna_nof_node *delete)
{
	sna_debug(5, "init\n");
	sna_tdm_destroy(delete);
	sna_rss_destroy(delete);
	sna_cosm_destroy(delete);
	return 0;
}
