/* sense.c: SNA Sense Decoder for Linux.
 *
 * Written by Jay Schulist <jschlst@samba.org>
 *
 * This software may be used and distributed according to the terms
 * of the GNU Public License, incorporated herein by reference.
 */

#include <config.h>
#include <unistd.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <gdbm.h>

#include "sense.h"

#define NOTFOUND 0

int msg(int type)
{
	switch (type)
	{
		case (NOTFOUND):
			                printf("This sense code could not be located.\n");
			                printf("Please verify you have entered the correct sense code\n");
			                printf("or check IBM's Systems Network Architecture Formats (GA27-3136)\n");
			                printf("for more information on this sense code.\n");
			                printf("\nUpdates of this sense code database are available online at:\n");
			                printf("http://www.icenetworking.com\n");
					break;
	}

	return (0);	
}

int display(struct sentry *s)
{
	printf("%s:\n", s->full_code);
	printf("Category: %s\n", s->top_sdesc);
	printf("Description: %s\n", s->top_ldesc);
	printf("\n");
	printf("Problem: %s\n", s->cat_sdesc);
	printf("Description: %s\n", s->cat_ldesc);
	if(strncmp(s->sub1_code, "00", 2))
	{
		printf("\n");
		printf("Problem: %s\n", s->sub1_sdesc);
		printf("Description: %s\n", s->sub1_ldesc);
	}
	if(strncmp(s->sub2_code, "00", 2))
        {
                printf("\n");
                printf("Problem: %s\n", s->sub2_sdesc);
                printf("Description: %s\n", s->sub2_ldesc);
        }
	return (0);
}

int main(int argc, char *argv[])
{
	GDBM_FILE db;
	datum *key, content;
	struct sentry *srec;
	int c, pall = 0, len = 0;
	char sense[9], dbase[100];

	while((c=getopt(argc, argv, "das")) != EOF)
        {
                switch(c)
                {
			case 'a':
				pall = 1;
				break;

                        case 's':
                                strncpy(sense, argv[optind++], 9);
				if(pall)
				{
					printf("Usage: sense -d database [-s code|-a]\n");
					printf("  Must choose [-s code] or [-a], but not both.\n");
					exit(1);
				}
                                break;
			case 'd':
				strcpy(dbase, argv[optind++]);
				break;
                        default:
                                printf("Usage: sense -d database [-s code|-a]\n");
				exit(1);
                }
        }

	if (argc < 2)
	{
		printf("Usage: sense -d database [-s code|-a]\n");
		exit (1);
	}

	db = gdbm_open(dbase, 0, GDBM_READER, 
		(S_IRUSR | S_IWUSR | S_IROTH), NULL);
	if(db == NULL)
	{
		printf("Error: SenseError database is empty or not found\n");
		exit (1);
	}

	if(pall)
	{
		datum akey;
		akey = gdbm_firstkey(db);
		while(akey.dptr)
		{
			content = gdbm_fetch(db, akey);
        		if(content.dptr == 0)
        		{
                		msg(NOTFOUND);
                		exit (1);
        		}
			srec = (struct sentry *)malloc(sizeof(struct sentry));
		        memset(srec, '\0', sizeof(srec));
		        srec = (struct sentry *)content.dptr;
        		display(srec);
			printf("\n");

			free(srec);
			akey = gdbm_nextkey(db, akey);
		}

		gdbm_close(db);
                exit (1);
	}

	len = strlen(sense);
	if(len > 8 || len <= 1)
        {
                msg(NOTFOUND);
                exit (1);
        }
	if(len == 8)	/* Single Record lookup */
	{
		key = (datum *)malloc(sizeof(datum));
	        memset(key, 0, sizeof(key));
		key->dptr = sense;
		key->dsize = sizeof(sense);
		content = gdbm_fetch(db, *key);
		if(content.dptr == 0)
		{
			msg(NOTFOUND);
			exit (1);
		}
		srec = (struct sentry *)malloc(sizeof(struct sentry));
		memset(srec, '0', sizeof(srec));
		srec = (struct sentry *)content.dptr;
		if(srec == NULL)
		{
			printf("Error: Located sense code (%s) data is empty.\n", 
				key->dptr);
			exit(1);
		}

		display(srec);
		free(key);
		free(srec);
	}
	if(len == 2 || len == 4 || len == 6)
	{
		datum akey;
                akey = gdbm_firstkey(db);
                while(akey.dptr)
                {
                        content = gdbm_fetch(db, akey);
                        if(content.dptr == 0)
                        {
                                msg(NOTFOUND);
                                exit (1);
                        }
                        srec = (struct sentry *)malloc(sizeof(struct sentry));
                        memset(srec, '\0', sizeof(srec));
                        srec = (struct sentry *)content.dptr;
			if(len == 2)
			{
				if(!strncmp(srec->full_code, sense, 2))
				{
                        		display(srec);
					printf("\n");
				}
			}
			if(len == 4)
			{
				if(!strncmp(srec->full_code, sense, 4))
				{
					display(srec);
					printf("\n");
				}
			}
			if(len == 6)
			{
				if(!strncmp(srec->full_code, sense, 6))
				{
					display(srec);
					printf("\n");
				}
			}

                        free(srec);
                        akey = gdbm_nextkey(db, akey);
                }
	}

	gdbm_close(db);
	return (0);
}
