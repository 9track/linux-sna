/* getsense.c: display sense data information.
 *
 * Copyright (c) 2002 by Jay Schulist <jschlst@linux-sna.org>
 *
 * Portions of this program taken from the IBM getsense program.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#define MAXDATA				3
#define _PATH_GETSENSE_SENSEDATA	"/usr/lib/getsense/sense.data"

char    version_s[]             = VERSION;
char    name_s[]                = "getsense";
char    desc_s[]                = "Linux-SNA sense data translator";
char    maintainer_s[]          = "Jay Schulist <jschlst@linux-sna.org>";
char    company_s[]             = "linux-SNA.ORG";
char    web_s[]                 = "http://www.linux-sna.org";

typedef enum state {
	STATE_NOTHING,
    	STATE_IN_CODE,
    	STATE_IN_WRONG_DATA,
    	STATE_IN_RIGHT_DATA
} STATE;

struct gen_info {
    	char data_code[5];
    	long file_offset;
};

int check_for_generic_msg(FILE *fd, struct gen_info *generic_data, char *buffer)
{
	int i, j, generic_code_found = 0;
	char generic_chars[4] = "nxy";

	for (i = 1; i < 5 && !generic_code_found; i++) {
      		for (j = 0; j < 3 && !generic_code_found; j++) {
         		if (buffer[i] == generic_chars[j]) {
            			generic_code_found = 1;
            			generic_data -> file_offset = ftell(fd);
            			strncpy(generic_data->data_code, &buffer[1], 4);
            			generic_data->data_code[4] = '\0';
         		} 
      		} 
   	}
   	return generic_code_found;
}

int compare_generic(char *sense_data, char *data_code)
{
	int q, match = 1;

    	/* will be false if any one character doesn't fit pattern */
    	for (q = 0; q < 4 && match; q++) {
        	if ((isxdigit(data_code[q]) == 0) || (data_code[q] == sense_data[q]))
            		match = 1;
        	else
            		match = 0;
    	}
    	return match;
}

int show_sense(FILE *fd, char *sense_code, char *sense_data)
{
	int sense_data_found = 0, generic_code_found = 0, generic_match = 0;
	struct gen_info generic_data[MAXDATA];
	STATE state = STATE_NOTHING;
	int count, pointer, n = 0;
	char buffer[255];

	while (NULL != fgets(buffer, sizeof(buffer), fd)) {
		switch (state) {
			case STATE_NOTHING:
                		switch (buffer[0]) {
                    			case 'C':
                        			if (!strncmp(&buffer[1], sense_code, 4))
                            				state = STATE_IN_CODE;
                        			break;
                    			default:
                        			;
                		}
                		break;
			case STATE_IN_CODE:
                		switch (buffer[0]) {
                    			case '!':
                        			printf("%s", &buffer[1]);
                        			break;
                    			case 'C':
						exit(0);
                        			break;
                    			case 'D':
                        			if (sense_data == NULL
                            				|| !strncmp(&buffer[1], sense_data, 4)) {
                            				state = STATE_IN_RIGHT_DATA;
                            				sense_data_found = 1;
                        			} else {
                            				state = STATE_IN_WRONG_DATA;
                            				generic_code_found = check_for_generic_msg(fd,
                                                 		&generic_data[n], buffer);
                            				if (generic_code_found)
                               					++n;
                        			}
                        			break;
                    			default:
                        			;
                		}
                		break;
			case STATE_IN_WRONG_DATA:
                		switch (buffer[0]) {
                    			case '!':
                        			break;
                    			case 'C':
                        			for (count = 0; count < n && !generic_match; count++) {
                            				generic_match = compare_generic(sense_data,
                                       			generic_data[count].data_code);
                            				pointer = count;
                        			}
                        			if (generic_match) {
                            				fseek(fd, generic_data[pointer].file_offset, SEEK_SET);
                            				state = STATE_IN_RIGHT_DATA;
                        			} else {
                        				if (strcmp(sense_code, "0835") != 0) {
								printf("%s: details for this sense data "
								"were not found. please contact %s\n",
								name_s, maintainer_s);
								exit(0);
                        				}
						}
                        			break;
                    			case 'D':
                        			if (sense_data == NULL
                            				|| !strncmp(&buffer[1], sense_data, 4)) {
                            				state = STATE_IN_RIGHT_DATA;
                            				sense_data_found = 1;
                        			} else {
                            				state = STATE_IN_WRONG_DATA;
                            				generic_code_found = check_for_generic_msg(fd,
                                                 		&generic_data[n], buffer);
                            				if (generic_code_found)
                               					++n;
                        			}
                       				 break;
                    			default:
                        			;
                		}
                		break;
			case STATE_IN_RIGHT_DATA:
                		switch (buffer[0]) {
                    			case '!':
                        			printf("%s", &buffer[1]);
                        			break;
                    			case 'C':
						exit(0);
                        			break;
                    			case 'D':
                         			/* we're been printing the sense data we were looking
						 * for. if we aren't printing out all the sense
						 * data for this code, then exit.
						 */
                        			if (sense_data != NULL)
							exit(0);
                        			break;
                    			default:
                        			;
                		}
                		break;
            		default:
                		printf("%s: invalid state, %d\n", name_s, state);
				exit(1);
                		break;
		}
	}
	printf("%s: the send code was not found.\n", name_s);
	return 0;
}

int strupr(char *string)
{
	for (; *string; ++string)
		*string = toupper(*string);
	return 0;
}

int help(void)
{
	printf("Usage: %s [-h] [-v] [-f filename] sense_code\n", name_s);
	exit(1);
}

int version(void)
{
        printf("%s: %s %s\n%s %s\n%s\n", name_s, desc_s, version_s,
                company_s, maintainer_s, web_s);
        exit(1);
}

int main(int argc, char **argv)
{
	int sense_code_present = 0, sense_data_present = 0;
	char sense_code[5], sense_data[5], config_file[200];
	int c, len;
	FILE *fd;

	sense_code[0] = '\0';
    	sense_data[0] = '\0';
	strcpy(config_file, _PATH_GETSENSE_SENSEDATA);
        while((c = getopt(argc, argv, "hvVf:")) != EOF) {
                switch(c) {
                        case 'V':       /* Display author and version. */
                        case 'v':       /* Display author and version. */
                                version();
                                break;
                        case 'h':       /* Display useless help information. */
                                help();
                                break;
			case 'f':	/* path to sense data. */
				strcpy(config_file, optarg);
				break;
			default:
				help();
				break;
		}
	}

	argc -= optind;
	argv += optind;
	if (!argc) {
		printf("%s: you must specify a sense code.\n", name_s);
		help();
	}

	/* only option left is sense code from user. */
	len = strlen(*argv);
	if (len > 3) {
		memcpy(sense_code, *argv, 4);
                sense_code[4] = '\0';
                strupr(sense_code);
                sense_code_present = 1;
                if (len == 8) {
                    	memcpy(sense_data, argv[4], 4);
                    	sense_data[4] = '\0';
                    	strupr(sense_data);
                    	sense_data_present = 1;
                }
	}
	if (!sense_code_present) {
		printf("%s: you must specify a sense code.\n", name_s);
		help();
    	}

	/* open sense data file. */
	fd = fopen(config_file, "r");
	if (fd == NULL) {
		printf("%s: could not open sense data file: %s\n", name_s, 
			config_file);
		exit(1);
	}

	/* display the sense information. */
	show_sense(fd, sense_code, sense_data_present ? sense_data : NULL);
	return 0;
}
