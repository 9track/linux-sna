/* snaconfig.c: Provide an easy interface into the dark devilish world of SNA.
 * - main() and high level entrance into parsers.
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

/* required for linux-SNA. */
#include <asm/byteorder.h>
#include <linux/sna.h>
#include <linux/cpic.h>
#include <nof.h>

/* our stuff. */
#include "sna_list.h"
#include "snaconfig.h"

char 	version_s[]            	= VERSION;
char 	name_s[]                = "snaconfig";
char 	desc_s[]                = "Linux-SNA configuration interface";
char 	maintainer_s[]          = "Jay Schulist <jschlst@linux-sna.org>";
char 	company_s[]             = "linux-SNA.ORG";
char 	web_s[]			= "http://www.linux-sna.org";
int 	sna_debug_level		= 1;
global_info *sna_config_info	= NULL;

#ifndef SOL_SNA_NOF
#define SOL_SNA_NOF     278
#endif

int help(void)
{
	printf("Usage: %s [-h] [-v] [-d level] ACTION <SERVICE> <NAME>\n", name_s);
	printf("   ACTION := [ find | load | reload | unload | start | stop | show ]\n");
	printf("   SERVICE:= [ file | node | dlc | link | lu | all ]\n");
	printf("   NAME   := [ name assigned to component in config_file ]\n");
	printf("\n");
	printf("Examples:\n");
	printf("	find   := [<netid> <group_name> [appn | subarea | nameserver]]\n");
	printf("	load   := [<config_file>]\n");
	printf("	unload := [<config_file>]\n"); 
	printf("	start  := [SERVICE NAME]\n");
	printf("	stop   := [SERVICE NAME]\n");
	printf("	show   := [<SERVICE NAME>]\n");
#ifdef NOT	
	printf("\n");	
	printf("	[find <netid> <group_name> [appn|subarea|nameserver]]\n"); 
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
#endif
	exit (1);
}

int version(void)
{
	printf("%s: %s %s\n%s %s\n%s\n", name_s, desc_s, version_s,
                company_s, maintainer_s, web_s);
	exit(1);
}

int matches(const char *cmd, char *pattern)
{
        int len = strlen(cmd);
        if (len > strlen(pattern))
                return -1;
        return memcmp(pattern, cmd, len);
}

int sna_nof_connect(void)
{
	int sk;

	sk = socket(AF_SNA, SOCK_DGRAM, SOL_SNA_NOF);
        if (sk < 0) {
		sna_debug(1, "unable to open connection to NOF.\n");
		sna_debug(1, "did you load the sna modules?\n");
	}
	return sk;
}

int sna_nof_disconnect(int sk)
{
	return close(sk);
}

int main(int argc, char **argv)
{
	/* parse flags. */
	next_arg(argv, argc);
	while (argc) {
		char *opt = *argv;
		if (opt[0] != '-')
			break;
		if (opt[1] == '-')
			opt++;
		if (!matches(opt, "h") || !matches(opt, "-help"))
			help();
		if (!matches(opt, "v") || !matches(opt, "-version"))
			version();
		if (!matches(opt, "d") || !matches(opt, "-debug")) {
			opt = next_arg_fail(argv, argc, help);
			sna_debug_level = atoi(opt);
			next_arg(argv, argc);
			continue;
		}
		next_arg(argv, argc);
	}

	/* parse actions. */
	if (argc < 1)
		help();
	if (!matches(*argv, "find")) {
		next_arg(argv, argc);
		return sna_find(argc, argv);
	}
	if (!matches(*argv, "load")) {
		next_arg(argv, argc);
		return sna_load(argc, argv);
	}
	if (!matches(*argv, "reload")) {
		next_arg(argv, argc);
		return sna_reload(argc, argv);
	}
	if (!matches(*argv, "unload")) {
		next_arg(argv, argc);
		return sna_unload(argc, argv);
	}
	if (!matches(*argv, "start")) {
		next_arg(argv, argc);
		return sna_start(argc, argv);
	}
	if (!matches(*argv, "stop")) {
		next_arg(argv, argc);
		return sna_stop(argc, argv);
	}
	if (!matches(*argv, "show")) {
		next_arg(argv, argc);
		return sna_show(argc, argv);
	}
	help();
	exit(0);
}
	
#ifdef NOT	
	/* Show current SNA configuration for all nodes. */
        if(!from_file && (argc == 0 || opt_a))
        {
                int err;
                printf("here\n");
                err = sna_print(NULL);
                exit(err < 0);
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
#endif
