/* snaconfig.c: Provide an easy interface into the dark devilish world of SNA.
 * - main() and high level entrance into parsers.
 *
 * Author:
 * Jay Schulist         <jschlst@samba.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * None of the authors or maintainers or their employers admit
 * liability nor provide warranty for any of this software.
 * This material is provided "as is" and at no charge.
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if_arp.h>
#include <net/ethernet.h>

#include <linux/sna.h>
#include <linux/cpic.h>

#include "snaconfig.h"
#include "paths.h"

#include <version.h>
#include <nof.h>

int sna_sk;
int sna_debug = 10;

int opt_a = 0;

#ifndef SOL_SNA_NOF
#define SOL_SNA_NOF	278
#endif

int help(void)
{
	printf("Information available at: http://www.linux-sna.org\n");
	printf("Usage: snaconfig\n"); 
	printf("	[find <netid> <group_name> [appn|subarea|nameserver]]\n"); 
	printf("	[-f config_file] [-a] [-d <level>] [-h] [-V]\n"); 
	printf("	<NetID.Node> [nodeid <X'blknpuid>] [<len|appn|nn>]\n");
	printf("	[dlc <dev> <port> [pri|sec|neg] [btu <NN>] [mia <NN>] [moa <NN>]]\n");
	printf("	[link <dev> <port> <plu_name> <dstaddr> <dstport> [[-]byte_swap]\n");
	printf("	  [[-]auto_act] [[-]auto_deact] [[-]auto_retry] [retry <NN>]\n");
	printf("	  [tg_number <NN> [cost_per_byte <NN>] [cost_per_connect <NN>]\n"); 
	printf("	  [effective_capacity <NN>] [propagation_delay <lan>] [security <none>]\n");
	printf("	  [user1 <NN>] [user2 <NN>] [user3 <NN>]]]\n");
	printf("	[lu local <name> [[-]sync_point] [lu_sess_limit <NN>]]\n");
	printf("	[lu remote <NetID.Node> <FqCP.Name> [[-]parallel_ss] [[-]cnv_security]]\n");
	printf("	[mode <name> <plu_name> <cos_name> [tx_pacing <NN>] [rx_pacing <NN>]\n");
	printf("	  [max_tx_ru <NN>] [max_rx_ru <NN>] [min_conwinners <NN>]\n");
	printf("	  [min_conlosers <NN>] [max_sessions <NN>] [[-]auto_activation]\n");
 	printf("	  [auto_act_limit <NN>] [[-]encryption] [[-]tx_comp [[-]rle|lz9|lz10]\n");
	printf("	  [[-]rx_comp [[-]rle|lz9|lz10]]\n");
	printf("	[cpic <sym_dest_name> <mode> <plu_name> <tp_name> [[-]srvc_tp]\n");
	printf("	  [[-]secure [[-]<same|program|strong>] [<userid> <password>]]]\n");
	printf("	[cos <name> [weight <NN>] [tx_priority <low|medium|high|network>]\n");
	printf("	  [[-]default_cos_invalid] [[-]default_cos_null]\n");
	printf("	  [min_cost_per_connect <NN>] [max_cost_per_connect <NN>]\n");
	printf("	  [min_cost_per_byte <NN>] [max_cost_per_byte <NN>]\n");
	printf("          [min_propagation_delay <NN>] [max_propagation_delay <NN>]\n");
        printf("          [min_effective_capacity <NN>] [max_effective_capacity <NN>]\n");
	printf("          [min_route_resistance <NN>] [max_route_resistance <NN>]\n");
        printf("          [min_node_congested <NN>] [max_node_congested <NN>\n");
	printf("	  [min_security <NN>] [min_security <NN>]\n");
	printf("	  [min_user1 <NN>] [max_user1 <NN>]\n");
	printf("	  [min_user2 <NN>] [max_user2 <NN>]\n");
	printf("	  [min_user3 <NN>] [max_user3 <NN>]]\n");
	printf("	[start|stop|delete] ... \n");

	exit (1);
}

int version(void)
{
	printf("snaconfig v%s\n", ToolsVersion);
	printf("%s\n", ToolsMaintain);

	exit (1);
}

int main(int argc, char **argv)
{
	int i, err;

	/* Find any options. */
  	(void)argc--;
	(void)argv++;
  	while(argc && *argv[0] == '-')
	{
		/* Display status on All nodes. */
    		if(!strcmp(*argv, "-a"))
		{
			opt_a = 1;
			goto wrap;
		}

		/* Set the debug level. */
                if(!strcmp(*argv, "-d"))
		{
			if(*++argv == NULL) help();
                        sna_debug = atoi(*argv);
			argc--;
			goto wrap;
		}

		/* Display author and version */
    		if(!strcmp(*argv, "-V") || !strcmp(*argv, "-version")
        		|| !strcmp(*argv, "--version") || !strcmp(*argv, "-v")) 
			version();

		/* Display useless help information. */
    		if(!strcmp(*argv, "-?") || !strcmp(*argv, "-h")
        		|| !strcmp(*argv, "-help") || !strcmp(*argv, "--help"))
			help();

		/* Parse a Linux-SNA configuration file. */
		if(!strcmp(*argv, "-f"))
		{
			if(*++argv == NULL) help();
			err = sna_load_cfg_file(*argv);
			exit (err);
		}

wrap:
    		(void)argv++;
    		(void)argc--;
  	}

	/* Show current SNA configuration for all nodes. */
	if(argc == 0 || opt_a)
	{
		int err;
		printf("here\n");
		err = sna_print(NULL);
		exit(err < 0);
	}

	sna_sk = socket(AF_SNA, SOCK_DGRAM, SOL_SNA_NOF); 
        if(sna_sk < 0)  
        {               
                perror("socket");
                printf("snaconfig: did you load the sna modules?\n");
                exit (1);
        }       

#ifdef OLD_LAR_CODE
	if(!strcmp(*argv, "find"))
	{
		char name[SNA_RESOURCE_NAME_LEN];
		char net[SNA_RESOURCE_NAME_LEN];
		unsigned long rtcap = 0;

		if(*++argv == NULL) help();
		strcpy(net, *argv);
		for(i = 0; i < 8; i++)
        	        net[i] = toupper(net[i]);
	        for(i = strlen(net); i < 8; i++)
        	        net[i] = 0x20;

		if(*++argv == NULL) help();
		strcpy(name, *argv);
		for(i = 0; i < 8; i++)
			name[i] = toupper(name[i]);
		for(i = strlen(name); i < 8; i++)
                        name[i] = 0x20;

		if(*++argv == NULL) help();
		if(!strcmp(*argv, "appn"))
			rtcap = SNA_RTCAP_APPN_NN;
		else
		{
			if(!strcmp(*argv, "subarea"))
				rtcap = SNA_RTCAP_SUBAREA;
			else
			{
				if(!strcmp(*argv, "nameserver"))
					rtcap = SNA_RTCAP_NAME;
				else
					help();
			}
		}

		lar_search(sna_sk, net, name, rtcap);
		exit (0);
	}
#endif

	sna_load_cfg_stdin(argv);

	close(sna_sk);
	exit (0);
}
