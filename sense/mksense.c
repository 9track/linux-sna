/* mksense.c: SNA Sense Decoder Database Builder for Linux.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <gdbm.h>

#include "sense.h"

int sense_get_data(FILE *fp, struct sentry *srec)
{
	char buf[4000];
	int size;

	/* Top code */
	memset(buf, '\0', sizeof(buf));
	fgets(buf, 4000, fp);
	size = strlen(buf);
	if(size == 0)
		return (1);
	strncpy(srec->top_code, buf, size-1);

	/* Top short description */
	memset(buf, '\0', sizeof(buf));
        fgets(buf, 4000, fp);
	size = strlen(buf);
        if(size == 0)
                return (1);
	strncpy(srec->top_sdesc, buf, size-1);

	/* Top long description */
	memset(buf, '\0', sizeof(buf));
        fgets(buf, 4000, fp);
	size = strlen(buf);
        if(size == 0)
                return (1);
	strncpy(srec->top_ldesc, buf, size-1);

	/* Full Code */
	memset(buf, '\0', sizeof(buf));
        fgets(buf, 4000, fp);
	size = strlen(buf);
        if(size == 0)
                return (1);
        strncpy(srec->full_code, buf, size-1);

	/* Catagory code */
	memset(buf, '\0', sizeof(buf));
        fgets(buf, 4000, fp);
	size = strlen(buf);
        if(size == 0)
                return (1);
        strncpy(srec->cat_code, buf, size-1);

	/* Catagory Short description */
	memset(buf, '\0', sizeof(buf));
        fgets(buf, 4000, fp);
	size = strlen(buf);
        if(size == 0)
                return (1);
        strncpy(srec->cat_sdesc, buf, size-1);

	/* Catagory Long description */
	memset(buf, '\0', sizeof(buf));
        fgets(buf, 4000, fp);
	size = strlen(buf);
        if(size == 0)
                return (1);
        strncpy(srec->cat_ldesc, buf, size-1);

	/* Sub-1 Code */
	memset(buf, '\0', sizeof(buf));
        fgets(buf, 4000, fp);
	size = strlen(buf);
        if(size == 0)
                return (1);
        strncpy(srec->sub1_code, buf, size-1);

	/* Sub-1 Short description */
	memset(buf, '\0', sizeof(buf));
	fgets(buf, 4000, fp);
	size = strlen(buf);
        if(size == 0)
                return (1);
        strncpy(srec->sub1_sdesc, buf, size-1);

	/* Sub-1 long description */
	memset(buf, '\0', sizeof(buf));
        fgets(buf, 4000, fp);
	size = strlen(buf);
        if(size == 0)
                return (1);
        strncpy(srec->sub1_ldesc, buf, size-1);

	/* Sub-2 Code */
	memset(buf, '\0', sizeof(buf));
	fgets(buf, 4000, fp);
	size = strlen(buf);
        if(size == 0)
                return (1);
        strncpy(srec->sub2_code, buf, size-1);

	/* Sub-2 Short description */
	memset(buf, '\0', sizeof(buf));
        fgets(buf, 4000, fp);
	size = strlen(buf);
        if(size == 0)
                return (1);
        strncpy(srec->sub2_sdesc, buf, size-1);

        /* Sub-2 long description */
	memset(buf, '\0', sizeof(buf));
        fgets(buf, 4000, fp);
	size = strlen(buf);
        if(size == 0)
                return (1);
        strncpy(srec->sub2_ldesc, buf, size-1);

	return 0;
}

int main(int argc, char *argv[])
{
	GDBM_FILE db;
	FILE *fp;
	datum *key, content;
	struct sentry *srec;
	int err, c, debug=1, i=0;
	char tfile[100], dfile[100];

	while((c=getopt(argc, argv, "td")) != EOF)
        {
                switch(c)
                {
			case 't':
				strcpy(tfile, argv[optind++]);
				break;
			case 'd':
				strcpy(dfile, argv[optind++]);
				break;
			default:
				printf("Usage: mksense -t textfile -d dbfile\n");
				exit(1);
		}
        }

        if(argc < 3)
        {
		printf("Usage: mksense t=textfile d=dbfile\n");
                exit(1);
	}

	fp = fopen(tfile, "r");
	if(fp == NULL)
	{
		printf("Error: Master text file cannot be opened.\n");
		exit (1);
	}

	db = gdbm_open(dfile, 0, GDBM_WRCREAT,
			(S_IRUSR | S_IWUSR | S_IROTH), NULL);

	do
	{
		i++;
		srec = (struct sentry *)malloc(sizeof(struct sentry));
		memset(srec, '0', sizeof(srec));
		err = sense_get_data(fp, srec);
		if (err)
			break;

		if(debug >= 1)
		{
			printf("%s\n", srec->full_code);
		}

		key = (datum *)malloc(sizeof(datum));
		memset(key, 0, sizeof(key));
		key->dptr = srec->full_code;
		key->dsize = sizeof(srec->full_code);

		content.dptr = (char *)srec;
		content.dsize = sizeof(*srec);

		err = gdbm_store(db, *key, content, GDBM_REPLACE);
		if(err != 0)
		{
			printf("Error: gdbm_store insert error %d.\n", err);
			fclose(fp);
        		gdbm_close(db);
			exit(1);
		}
		free(key);
		free(srec);
	} while(!feof(fp));

	printf("-----------------------------------------\n");
	printf(" %d records added to %s\n", i, dfile);
	printf("-----------------------------------------\n");

	fclose(fp);
	gdbm_close(db);
	return (0);
}
