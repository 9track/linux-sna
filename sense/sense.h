/*
 * Linux-SNA Sense Decode header file.
 *
 * Written by Jay Schulist <jschlst@turbolinux.com>
 *
 * This software may be used and distributed according to the terms
 * of the GNU Public License, incorporated herein by reference.
 */

struct sentry
{
	char full_code[9];	/* Full sense code, ie. 8003005 */

	char top_code[3];		/* 1st sense section code, ie. 80 */
	char top_sdesc[200];	/* 1st sense section short description */
	char top_ldesc[1000];	/* 1st sense section long description */

	char cat_code[3];		/* 2nd sense section code, ie. 03 */
        char cat_sdesc[200];	/* 2nd sense section short description */
        char cat_ldesc[1000];	/* 2nd sense section long description */

	char sub1_code[3];
        char sub1_sdesc[200];
        char sub1_ldesc[1000];

	char sub2_code[3];
        char sub2_sdesc[200];
        char sub2_ldesc[1000];
};
