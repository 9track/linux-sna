/* snaconfig.c: Provide an easy interface into the dark devilish world of SNA.
 *
 * Author:
 * Jay Schulist         <jschlst@turbolinux.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <linux/if_ether.h>
#include </usr/src/linux/include/net/sna/sna.h>
#include </usr/src/linux/include/net/sna/sna_nof.h>
#include </usr/src/linux/include/net/sna/sna_lar.h>

#include "snaconfig.h"

#include <version.h>
#include <nof.h>
#include <lar.h>

static struct sna_all_info *sna_list = NULL;

#define next_arg(X)     (*X = *X + 1)
#define new(p) 		((p) = malloc(sizeof(*(p))))

#ifndef SOL_SNA_NOF
#define SOL_SNA_NOF	278
#endif

#define SNA_PORT_ROLE_PRI       0x1
#define SNA_PORT_ROLE_SEC       0x2
#define SNA_PORT_ROLE_NEG       0x4

char *Version = "snaconfig v";
char *Author  = "Jay Schulist <jschlst@turbolinux.com>";

int sna_sk;
int sna_debug = 1;

/*
 * - Node Information
 *   - Static Data
 *   - Statistics/Status
 * - DLC/Port Information
 *   - Static Data 
 *   - Statistics/Status
 * - Link Station Information
 *   - Static Data
 *   - Statistics/Status
 *
 * LNXSNA.EHEAD		nodetype:Appn  blockid:05D  puid:00000 
 *			maxlus:1000  curlus:1   LUSEG BINDSEG UP RUNNING
 *			datalink:eth0  port:0x04  type:llc2  role:Neg
 *			MIA:1200  MOA:1200  BTU:1400   UP RUNNING
 *			datalink:tr0  port:0x04  type:llc2  role:Neg
 *			MIA:1200  MOA:1200  BTU:1400   UP HALTED
 *			linkstation:001122334455  port:0x04  dev:eth0
 *			byteswap:off  aact:off  adeact:off   UP RUNNING RESET
 *			linkstation:554433221100  port:0x04  dev:tr0
 *			byteswap:on  aact:on  adeact:on  UP HALTED RESET
 *			local_lu:IBM sync_point:off lu_limit:1000 UP
 *			remote_lu:
 *			mode:#INTER partner:LNXSNA.IBM 
 *			tx_pacing:7 rx_pacing:7 max_tx_ru:1200 max_rx_ru:1200
 *			crypto:off  UP
 *
 *
 * Information about the above: (should be moved into the readme)
 * All Linux-SNA node information is correlated by the NetID.Node name.
 * The first line above is the datalink device, eth0. Port 0x04 is the
 * port that the destination link is estabilished over.
 * Mulitple datalink entries can exist per node.
 */

/* Print one entry from data in 'info'. The data has already been parsed and
 * verified. All data will be there. Just make it look nice.
 */
int se_print(struct sna_all_info *sna)
{
	struct dl_info *dl;
	struct port_info *port;
	struct ls_info *ls;
	struct mode_info *mode;
	struct lu_info *lu;
	struct plu_info *plu;

	if(sna_debug > 5)
		printf("se_print\n");

	printf("%-s.%-s\n", sna->net, sna->name);

	/* Node Information - Static */
	printf("	  nodetype:");
	if(sna->type & SNA_LEN_END_NODE) printf("Len  ");
        if(sna->type & SNA_APPN_END_NODE) printf("Appn_En  ");
        if(sna->type & SNA_APPN_NET_NODE) printf("Appn_Nn  ");
	printf("blockid:%02X  ", 69);
	printf("puid:%04X  ", 0);
	printf("maxlus:%ld  ", sna->max_lus);
	printf("curlus:%ld", 0);
	printf("\n");

	printf("	  luseg:");
	if(sna->lu_seg) printf("on  ");
	if(!sna->lu_seg) printf("off  ");
	printf("bindseg:");
	if(sna->bind_seg) printf("on  ");
	if(!sna->bind_seg) printf("off  ");
	if(sna->node_status & SNA_UP) printf("UP ");
        if(sna->node_status & SNA_STOPPED) printf("STOPPED ");
        if(sna->node_status & SNA_RUNNING) printf("RUNNING ");
	printf("\n");

	for(dl = sna->dl; dl != NULL; dl = dl->next)
	{
		printf("	  ");
		printf("datalink:%s  ", dl->dlc_devname);
		printf("type:");
		if(!strncmp(dl->dlc_devname, "eth", 3)
                	|| !strncmp(dl->dlc_devname, "tr", 2))
                	printf("llc2  ");
        	else
                	printf("unknown  ");
		printf("numports:%d  ", dl->dlc_port_qlen);
		if(dl->dlc_flags & SNA_UP) printf("UP ");
		printf("\n");

		for(port = dl->port; port != NULL; port = port->next)
		{
			printf("	  ");
			printf("port:0x%02X  ", port->port_saddr[0]);
			printf("role:");
			if(port->port_role & SNA_PORT_ROLE_PRI)
		                printf("Pri  ");
        		if(port->port_role & SNA_PORT_ROLE_SEC)
                		printf("Sec  ");
        		if(port->port_role & SNA_PORT_ROLE_NEG)
                		printf("Neg  ");
			printf("numls:%d  ", port->port_ls_qlen);
			printf("MIA:%d  ", port->port_mia);
        		printf("MOA:%d  ", port->port_moa);
        		printf("BTU:%d  ", port->port_btu);
			if(port->port_flags & SNA_UP) printf("UP ");
                	if(port->port_flags & SNA_STOPPED) printf("STOPPED ");
                	if(port->port_flags & SNA_RUNNING) printf("RUNNING ");
			printf("\n");

			for(ls = port->ls; ls != NULL; ls = ls->next)
			{
				printf("	  ");
				printf("linkstation:%s  ", ls->ls_dname);
				printf("port:0x%02X  ", ls->ls_daddr[0]);
				printf("dev:%s  ", ls->ls_devname);
				printf("byteswap:");
				if(ls->ls_byteswap) printf("on  ");
				if(!ls->ls_byteswap) printf("off ");
				printf("\n	  ");
				printf("aact:");
				if(ls->ls_auto_act) printf("on  ");
				if(!ls->ls_auto_act) printf("off  ");
				printf("adeact:");
				if(ls->ls_auto_deact) printf("on  ");
				if(!ls->ls_auto_deact) printf("off  ");
				if(ls->ls_flags & SNA_UP) printf("UP ");
                        	if(ls->ls_flags&SNA_STOPPED) printf("STOPPED ");
                        	if(ls->ls_flags&SNA_RUNNING) printf("RUNNING ");
				printf("\n");
			}
		}
	}

	for(lu = sna->lu; lu != NULL; lu = lu->next)
	{
		printf("          ");
		printf("local_lu:%s  ", lu->lu_luname);
		printf("sync_point:");
		if(lu->lu_sync_point)
			printf("on  ");
		else
			printf("off  ");
		printf("lu_limit:%ld  ", lu->lu_lu_sess_limit);
		if(lu->lu_flags & SNA_UP) printf("UP ");
                printf("\n");
	}

	for(plu = sna->plu; plu != NULL; plu = plu->next)
	{
		printf("          ");
		printf("remote_lu:%s.%s  ", plu->plu_plunet, plu->plu_pluname);
		printf("parallel_sessions:");
		if(plu->parallel_ss)
			printf("on  ");
		else
			printf("off  ");
		printf("cnv_security:");
		if(plu->cnv_security)
			printf("on  ");
		else
			printf("off  ");
		if(plu->plu_flags & SNA_UP) printf("UP ");
		printf("\n");
	}

	for(mode = sna->mode; mode != NULL; mode = mode->next)
	{
		printf("          ");
                printf("mode:%s  ", mode->mode_modename);
		printf("tx_pacing:%ld  ", mode->tx_pacing);
		printf("rx_pacing:%ld  ", mode->rx_pacing);
		printf("max_tx_ru:%ld  ", mode->max_tx_ru);
		printf("max_rx_ru:%ld  ", mode->max_rx_ru);
		printf("\n");

		printf("          ");
		printf("crypto:");
		if(mode->crypto)
			printf("on  ");
		else
			printf("off  ");

                if(mode->mode_flags & SNA_UP) printf("UP ");
                printf("\n");
	}

	return (0);
}

/* Gather all the node information and stuff in *sna */
int sna_fetch(struct sna_all_info *sna)
{
	struct sna_qsna *n;
	struct sna_qdlc *d, *dlc;
	struct dl_info *dl;
	struct sna_qport *p, *port;
	struct port_info *pt;
	struct sna_qls *l, *ls;
	struct ls_info *lk;
	struct sna_qmode *m, *mode;
	struct mode_info *md;
	struct sna_qlu *ll, *lu;
	struct lu_info *llu;
	struct sna_qplu *pl, *plu;
	struct plu_info *pplu;

	if(sna_debug > 5)
		printf("sna_fetch\n");

	n = nof_query_node(sna_sk, sna->net, sna->name);
	if(!n)
		return (-ENOENT);

	sna->type	= n->data.type;
	sna->lu_seg	= n->data.lu_seg;
	sna->bind_seg	= n->data.bind_seg;
	sna->max_lus	= n->data.max_lus;
	sna->node_status= n->data.node_status;

	d = nof_query_dlc(sna_sk, sna->net, sna->name, "eth0");
	if(!d)
		return (0);

	for(dlc = d; dlc != NULL; dlc = dlc->next)
	{
		new(dl);
		strncpy(dl->dlc_devname, dlc->data.dlc_devname, 8);
        	dl->dlc_port_qlen	= dlc->data.dlc_port_qlen;
        	dl->dlc_proc_id		= dlc->data.dlc_proc_id;
        	dl->dlc_flags		= dlc->data.dlc_flags;
		dl->port		= NULL;

		dl->next	= sna->dl;
		sna->dl		= dl;
	}

	/* Link each port with the correct DLC */
	p = nof_query_port(sna_sk, sna->net, sna->name, "*", "*");
	for(dl = sna->dl; dl != NULL; dl = dl->next)
	{
		for(port = p; port != NULL; port = port->next)
		{
			if(!strcmp(dl->dlc_devname, port->data.port_devname))
			{
				new(pt);
				strncpy(pt->port_portname, 
					port->data.port_portname, 8);
				memcpy(pt->port_saddr, 
					port->data.port_saddr, 12);
				pt->port_ls_qlen= port->data.port_ls_qlen;
				pt->port_proc_id= port->data.port_proc_id;
				pt->port_role	= port->data.port_role;
				pt->port_btu	= port->data.port_btu;
				pt->port_mia	= port->data.port_mia;
				pt->port_moa	= port->data.port_moa;
				pt->port_flags	= port->data.port_flags;
				pt->ls 		= NULL;
				pt->next	= dl->port;
				dl->port	= pt;
			}
		}
	}

	l = nof_query_ls(sna_sk, sna->net, sna->name, "*", "*", "*");
	for(dl = sna->dl; dl != NULL; dl = dl->next)
        {
                for(pt = dl->port; pt != NULL; pt = pt->next)
                {
			for(ls = l; ls != NULL; ls = ls->next)
			{
				if(strcmp(ls->data.ls_portname,pt->port_portname)
					&& strcmp(ls->data.ls_devname,
					dl->dlc_devname))
					break;
				new(lk);
				strncpy(lk->ls_lsname, ls->data.ls_lsname, 8);
				strncpy(lk->ls_devname, ls->data.ls_devname, 8);
				memcpy(lk->ls_dname, ls->data.ls_dname, 17);
				memcpy(lk->ls_daddr, ls->data.ls_daddr, 12);
				lk->ls_proc_id	= ls->data.ls_proc_id;
				lk->ls_flags	= ls->data.ls_flags;
				lk->ls_auto_act	= ls->data.ls_auto_act;
				lk->ls_auto_deact=ls->data.ls_auto_deact;
				lk->ls_byteswap	= ls->data.ls_byteswap;
				lk->next	= pt->ls;
				pt->ls		= lk;
			}
		}
	}

	/* Get Local LUs */
	ll = nof_query_lu(sna_sk, "*", "*", "*");
	for(lu = ll; lu != NULL; lu = lu->next)
	{
		new(llu);
		strncpy(llu->lu_net, lu->data.lu_net, 8);
		strncpy(llu->lu_name, lu->data.lu_name, 8);
		strncpy(llu->lu_luname, lu->data.lu_luname, 8);
		llu->lu_sync_point	= lu->data.lu_sync_point;
		llu->lu_lu_sess_limit	= lu->data.lu_lu_sess_limit;
		llu->lu_proc_id		= lu->data.lu_proc_id;
		llu->lu_flags		= lu->data.lu_flags;
		llu->next	= sna->lu;
		sna->lu		= llu;
	}

	/* Get Remote LUs */
	pl = nof_query_plu(sna_sk);
	for(plu = pl; plu != NULL; plu = plu->next)
	{
		new(pplu);
		strncpy(pplu->plu_net, plu->data.plu_net, 8);
		strncpy(pplu->plu_name, plu->data.plu_name, 8);
		strncpy(pplu->plu_plunet, plu->data.plu_plunet, 8);
		strncpy(pplu->plu_pluname, plu->data.plu_pluname, 8);
		strncpy(pplu->plu_fqcpnet, plu->data.plu_fqcpnet, 8);
		strncpy(pplu->plu_fqcpname, plu->data.plu_fqcpname, 8);
		pplu->parallel_ss	= plu->data.parallel_ss;
		pplu->cnv_security	= plu->data.cnv_security;
		pplu->plu_proc_id	= plu->data.plu_proc_id;
		pplu->plu_flags		= plu->data.plu_flags;
		pplu->next	= sna->plu;
		sna->plu	= pplu;
	}

	/* Get Modes */
	m = nof_query_mode(sna_sk, "*", "*", "*");
	for(mode = m; mode != NULL; mode = mode->next)
	{
		new(md);
		strncpy(md->mode_net, mode->data.mode_net, 8);
		strncpy(md->mode_name, mode->data.mode_name, 8);
		strncpy(md->mode_plunet, mode->data.mode_plunet, 8);
		strncpy(md->mode_pluname, mode->data.mode_pluname, 8);
		strncpy(md->mode_modename, mode->data.mode_modename, 8);
        	md->tx_pacing	= mode->data.tx_pacing;
        	md->rx_pacing	= mode->data.rx_pacing;
        	md->max_tx_ru	= mode->data.max_tx_ru;
        	md->max_rx_ru	= mode->data.max_rx_ru;
        	md->crypto	= mode->data.crypto; 
        	md->mode_flags	= mode->data.mode_flags;
        	md->mode_proc_id= mode->data.mode_proc_id;
		md->next	= sna->mode;
		sna->mode	= md;
	}

	return (0);
}

int do_sna_fetch(struct sna_all_info *sna)
{
	if(sna_debug > 5)
		printf("do_sna_fetch\n");

	if(sna_fetch(sna) < 0)
	{
        	char *errmsg;

		/* FIXME: make libnof use errno properly!!
 		 */

        	if (errno == ENOENT) 
           	 	errmsg = "Node not found";
		else
			errmsg = "Node not found";
            	//	errmsg = strerror(errno);

        	fprintf(stderr, "snaconfig: Error fetching node information - %s %s.%s\n",
                	errmsg, sna->net, sna->name);
        	return (-1);
    	}
    	return (0);
}

/* Future caching mechinism.. */
struct sna_all_info *lookup_sna(struct sna_netid *n)
{
	struct sna_all_info *sna = NULL;

    	if(!sna) 
	{
        	new(sna);
		strncpy(sna->net, n->net, 8);
        	strncpy(sna->name, n->name, 8);
    	}

    	return (sna);
}

int do_sna_print(struct sna_all_info *sna)
{
	int res;

	res = do_sna_fetch(sna);
	if(res >= 0)
		se_print(sna);

	return (res);
}

void add_node(struct sna_all_info *n)
{
	struct sna_all_info *sna, **pp;

	pp = &sna_list;
    	for(sna = sna_list; sna; pp = &sna->next, sna = sna->next) 
	{
        	if(strcmp(n->name, sna->name) > 0)
            		break;
    	}
    	n->next = (*pp);
    	(*pp) = n;
}

int get_netid(char *net, char *name, char *p)
{
	char buf[17];

	sscanf(p, "%s", buf);
	strcpy(name, strpbrk(buf, ".")+1);
	strcpy(net, strtok(buf , "."));

	return (0);
}

int sna_readlist(void)
{
    	FILE *fh;
    	char buf[512];
    	struct sna_all_info *sna;
    	int err;

	fh = fopen("/proc/net/sna/virtual_nodes", "r");
	if(!fh)
	{
		printf("Cannot open /proc/net/sna/virtual_nodes\n");
		return (-1);
	}

	fgets(buf, sizeof(buf), fh);	/* eat one line */
	fgets(buf, sizeof(buf), fh);	/* eat another line */

	err = 0;
	while(fgets(buf, sizeof(buf), fh))
	{
		new(sna);
		get_netid(sna->net, sna->name, buf);
		add_node(sna);
	}

	return (err);
}

int for_all_nodes(int (*doit)(struct sna_all_info *))
{
	struct sna_all_info *sna;

	if(!sna_list && (sna_readlist() < 0))
		return (-1);
	for(sna = sna_list; sna; sna = sna->next)
	{
		int err = doit(sna);
		if(err)
			return (err);
	}

	return (0);
}

/* Gather all the data available, parse it, format it, verifiy it, then
 * send it to be printed.
 */
int sna_print(struct sna_netid *n)
{
	int res;

	if(!n)
		res = for_all_nodes(do_sna_print);
	else
	{
		struct sna_all_info *sna;

		sna = lookup_sna(n);
		res = do_sna_fetch(sna);
		if(res >= 0)
			se_print(sna);
	}

	return (res);
}

int help(void)
{
	printf("Information available at: http://www.linux-sna.org\n");
	printf("Usage: snaconfig\n"); 
	printf("	[find <netid> <group_name> [appn|subarea|nameserver]]\n"); 
	printf("	[-f config_file] [-a] [-h] [-V] <NetID.Node>\n");
	printf("	[[[-]<len|appn|nn>] [[-]lu_seg] [[-]bind_seg] [max_lus <NN>]]\n");
	printf("	[dlc <dev> <port> [pri|sec|neg] [btu <NN>] [mia <NN>] [moa <NN>]]\n");
	printf("	[link <dev> <port> <dstaddr> <dstport> [[-]byte_swap]\n");
	printf("	  [[-]auto_act] [[-]auto_deact]]\n");
	printf("	[lu local <name> [[-]dlu] [[-]sync_point] [lu_sess_limit <NN>]]\n");
	printf("	[lu remote <NetID.Node> <FqCP.Name> [[-]parallel_ss] [[-]cnv_security]]\n");
	printf("	[mode <name> <plu_name> [[-]high_pri] [tx_pacing <NN>] [rx_pacing <NN>]\n");
	printf("	  [max_tx_ru <NN>] [max_rx_ru <NN>] [auto_act_limit <NN>]\n");
	printf("	  [min_con_win_limit <NN>] [ptr_min_con_win_limit <NN>]\n"); 
	printf("	  [parallel_sess_limit <NN>] [[-]encryption]\n");
	printf("	  [[-]tx_comp [[-]rle|lz9|lz10] [[-]rx_comp [[-]rle|lz9|lz10]]\n");
	printf("	[cpic <sym_dest_name> <mode> <plu_name> <tp_name> [[-]srvc_tp]\n");
	printf("	  [[-]secure [[-]<same|program|strong>] [<userid> <password>]]]\n");
	printf("	[start|stop|delete] ... \n");

	exit (1);
}

int version(void)
{
	printf("%s%s\n", Version, ToolsVersion);
	printf("%s\n", Author);

	exit (1);
}

int main(int argc, char **argv)
{
	int opt_a = 0, i;
	struct sna_netid *netid;
	int err;
	char fnet[8], name[8];
        unsigned long rtcap = 0, max_lus = 0;
	unsigned short lu_seg = 0, bind_seg = 0;
	unsigned char node_type = 0;

	sna_sk = socket(AF_SNA, SOCK_DGRAM, SOL_SNA_NOF);
	if(sna_sk < 0)
	{
		perror("socket");
		printf("snaconfig: did you load the sna modules?\n");
		exit (1);
	}

	/* Find any options. */
  	argc--;
	argv++;
  	while(argc && *argv[0] == '-')
	{
		/* Display status on All nodes. */
    		if(!strcmp(*argv, "-a"))
			opt_a = 1;

		/* Display auther and version */
    		if(!strcmp(*argv, "-V") || !strcmp(*argv, "-version")
        		|| !strcmp(*argv, "--version")) 
			version();

		/* Display useless help information. */
    		if(!strcmp(*argv, "-?") || !strcmp(*argv, "-h")
        		|| !strcmp(*argv, "-help") || !strcmp(*argv, "--help"))
			help();

		/* Parse a configuration file and not read from stdin. */
		if(!strcmp(*argv, "-f"))
		{
			printf("Linux-SNA Configuration file support is not finished.\n");
			printf("Please bug %s to finish it.\n", Author);
			exit (0);
		}

    		argv++;
    		argc--;
  	}

	/* Show current SNA configuration for all nodes. */
	if(argc == 0)
	{
		int err = sna_print(NULL);
		close(sna_sk);
		exit(err < 0);
	}

	if(!strcmp(*argv, "find"))
	{
		if(*++argv == NULL) help();
		netid = (struct sna_netid *)malloc(sizeof(struct sna_netid));
        	strcpy(netid->name, *argv);
		for(i = 0; i < 8; i++)
			netid->name[i] = toupper(netid->name[i]);
		for(i = strlen(netid->name); i < 8; i++)
			netid->name[i] = 0x20;

		if(*++argv == NULL) help();
		strcpy(name, *argv);
		for(i = 0; i < 8; i++)
			name[i] = toupper(name[i]);

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

		lar_search(sna_sk, netid->name, name, rtcap);
		exit (0);
	}

	/* Get the NetID.Node name */
	netid = (struct sna_netid *)malloc(sizeof(struct sna_netid));
	strcpy(netid->name, strpbrk(*argv, ".")+1);
	for(i = 0; i < 8; i++)
		netid->name[i] = toupper(netid->name[i]);
	for(i = strlen(netid->name); i < 8; i++)
		netid->name[i] = 0x20;
	strcpy(netid->net, strsep(argv++, "."));
	for(i = 0; i < 8; i++)
                netid->net[i] = toupper(netid->net[i]);

	if(*argv == (char *)NULL)
        {
		sna_print(netid);
		close(sna_sk);
                exit (0);
        }

	/* get any node options. */
	while(*argv != (char *)NULL)
	{
		if(!strcmp(*argv, "len"))
		{
			if(sna_debug > 5)
				printf("len node\n");
			node_type = SNA_LEN_END_NODE;
			*argv++;
			continue;
		}

		if(!strcmp(*argv, "-len"))
		{
			if(sna_debug > 5)
				printf("-len node\n");
	
			*argv++;
			continue;
		}

		if(!strcmp(*argv, "appn"))
		{
			if(sna_debug > 5)
				printf("appn node\n");
			node_type = SNA_APPN_END_NODE;
			*argv++;
			continue;
		}

		if(!strcmp(*argv, "-appn"))
		{
			if(sna_debug > 5)
				printf("-appn node\n");

			*argv++;
			continue;
		}

		if(!strcmp(*argv, "nn"))
		{
			if(sna_debug > 5)
				printf("nn node\n");
			node_type = SNA_APPN_NET_NODE;
			*argv++;
			continue;
		}

		if(!strcmp(*argv, "-nn"))
		{
			if(sna_debug > 5)
				printf("-nn node\n");

			*argv++;
			continue;
		}
	
		if(!strcmp(*argv, "lu_seg"))
		{
			lu_seg = 1;
			*argv++;
			continue;
		}

		if(!strcmp(*argv, "-lu_seg"))
		{
			lu_seg = 0;
			*argv++;
			continue;
		}

		if(!strcmp(*argv, "bind_seg"))
		{
			bind_seg = 1;
			*argv++;
			continue;
		}

		
		if(!strcmp(*argv, "-bind_seg"))
		{
			bind_seg = 0;
			*argv++;
			continue;
		}

		if(!strcmp(*argv, "max_lus"))
		{
			if(*++argv == NULL) help();
			max_lus = atoi(*argv);
			*argv++;
			continue;
		}

		/* Datalink options. */
		if(!strcmp(*argv, "dlc"))
		{
			struct sna_define_port *dport;
			unsigned long port;

			if(*++argv == NULL) help();

	                dport = (struct sna_define_port *)malloc
        	                (sizeof(struct sna_define_port));
        	        memset(dport, 0, sizeof(struct sna_define_port));
                	memcpy(&dport->netid, netid, sizeof(struct sna_netid));
			if(sna_debug > 5)
                                printf("dlc\n");

			/* get device/interface name */
			strcpy(dport->name, *argv);
			if(*++argv == NULL) help();
			
			/* Get port. */
			port = strtoul(*argv, NULL, 0);
			memcpy(dport->saddr, &port, sizeof(dport->saddr));
			if(*++argv == NULL) help();

			/* Get port type. */
			if(!strcmp(*argv, "pri"))
                        	dport->role = SNA_PORT_ROLE_PRI;
                	else
                	{
                        	if(!strcmp(*argv, "sec"))
                        	        dport->role = SNA_PORT_ROLE_SEC;
                        	else
                        	{
                               		if(!strcmp(*argv, "neg"))
                                	        dport->role = SNA_PORT_ROLE_NEG;
                        	}
                	}

			if(!strcmp(*argv, "btu"))
			{
				if(*++argv == NULL) help();
				if(sna_debug > 5)
					printf("btu <%d>\n", atoi(*argv));
				dport->btu = atoi(*argv);
				if(*++argv == NULL) goto define_dlc;
			}

			if(!strcmp(*argv, "mia"))
			{
				if(*++argv == NULL) help();
				if(sna_debug > 5)
					printf("mia <%d>\n", atoi(*argv));
				dport->mia = atoi(*argv);
				if(*++argv == NULL) goto define_dlc;
			}

			if(!strcmp(*argv, "moa"))
			{
				if(*++argv == NULL) help();
				if(sna_debug > 5)
					printf("moa <%d>\n", atoi(*argv));
				dport->moa = atoi(*argv);
				if(*++argv == NULL) goto define_dlc;
			}

			/* Start port if needed */
                	if(!strcmp(*argv, "start"))
                	{
                        	struct sna_start_port *sport;

                        	sport = (struct sna_start_port *)malloc
                        	        (sizeof(struct sna_start_port));
                        	memset(sport, 0, sizeof(struct sna_start_port));
                        	memcpy(&sport->netid, netid, 
					sizeof(struct sna_netid));
                        	strcpy(sport->name, dport->name);
				strcpy(sport->saddr, dport->saddr);
                        	err = nof_start_port(sna_sk, sport);
                        	if(err < 0)
                        	{
                        	        perror("nof_start_port");
                        	        exit (1);
                        	}

				*argv++;
				continue;
                	}

			/* Stop DLC */
			if(!strcmp(*argv, "stop"))
			{
				struct sna_stop_port *sport;
                        	sport = (struct sna_stop_port *)malloc
                        	        (sizeof(struct sna_stop_port));
                        	memset(sport, 0, sizeof(struct sna_stop_port));
                        	memcpy(&sport->netid, netid, 
					sizeof(struct sna_netid));
                        	strcpy(sport->name, dport->name);
                        	strcpy(sport->saddr, dport->saddr);
                        	err = nof_stop_port(sna_sk, sport);
                        	if(err < 0)
                        	{
                        	        perror("nof_stop_port");
                        	        exit (1);
                        	}

				*argv++;
				continue;
			}

			if(!strcmp(*argv, "delete"))
        		{
                		struct sna_delete_port *dlport;
                		dlport = (struct sna_delete_port *)malloc
                        		(sizeof(struct sna_delete_port));
                		memset(dlport, 0, sizeof(struct sna_delete_port));
                		memcpy(&dlport->netid, netid, 
					sizeof(struct sna_netid));
                		strcpy(dlport->name, dport->name);
                		strcpy(dlport->saddr, dport->saddr);
                		err = nof_delete_port(sna_sk, dlport);
                		if(err < 0)
                		{
                        		perror("nof_delete_port");
                        		exit (1);
                		}

				*argv++;
				continue;
        		}

define_dlc:
                        err = nof_define_port(sna_sk, dport);
                        if(err < 0)
                        {
				printf("snaconfig: Do you have any devices loaded?\n");
                                exit (1);
                        }

			continue;
		}

		/* Link station options. */
		if(!strcmp(*argv, "link"))
		{
			struct sna_define_link_station *dls;
			unsigned long port;

			if(*++argv == NULL) help();

                        dls = (struct sna_define_link_station *)malloc
                                (sizeof(struct sna_define_link_station));
                        memset(dls, 0, sizeof(struct sna_define_link_station));
                        memcpy(&dls->netid, netid, sizeof(struct sna_netid));

			if(sna_debug > 5)
				printf("link\n");

			/* Get source device */
			strcpy(dls->name, *argv);

			/* Get source port */
			if(*++argv == NULL) help();
                        port = strtoul(*argv, NULL, 0);
                        memcpy(dls->saddr, &port, sizeof(dls->saddr));

			/* Get link dest mac addr */
			if(*++argv == NULL) help();
			strcpy(dls->dname, *argv);

			/* Get dest port. */
			if(*++argv == NULL) help();
                        port = strtoul(*argv, NULL, 0);
                        memcpy(dls->daddr, &port, sizeof(dls->daddr));
			if(*++argv == NULL) goto define_ls;

			if(!strcmp(*argv, "auto_act"))
			{
				if(sna_debug > 5)
					printf("auto_act\n");
				dls->auto_act = 1;
				if(*++argv == NULL) continue;
			}

			if(!strcmp(*argv, "-auto_act"))
			{
				if(sna_debug > 5)
					printf("-auto_act\n");
				dls->auto_act = 0;
				if(*++argv == NULL) continue;
			}

			if(!strcmp(*argv, "auto_deact"))
			{
				if(sna_debug > 5)
					printf("auto_deact\n");
				dls->auto_deact = 1;
				if(*++argv == NULL) continue;
			}

			if(!strcmp(*argv, "-auto_deact"))
			{
				if(sna_debug > 5)
					printf("-auto_deact\n");
				dls->auto_deact = 0;
				if(*++argv == NULL) continue;
			}

			if(!strcmp(*argv, "start"))
			{
				if(sna_debug > 5)
					printf("start ls\n");
				*argv++;
				continue;
			}

			if(!strcmp(*argv, "stop"))
			{
				struct sna_stop_link_station *sls;

				if(sna_debug > 5)
					printf("stop ls\n");

		                sls = (struct sna_stop_link_station *)malloc
                		        (sizeof(struct sna_stop_link_station));
		                memset(sls, 0, sizeof(struct sna_stop_link_station));
        		        memcpy(&sls->netid, netid, sizeof(struct sna_netid));
		                strcpy(sls->name, dls->name);
		                strcpy(sls->saddr, dls->saddr);
		                strcpy(sls->dname, dls->dname);
		                strcpy(sls->daddr, dls->daddr);
		                err = nof_stop_link_station(sna_sk, sls);
		                if(err < 0)
		                {
		                        perror("nof_stop_link_station");
		                        exit (1);
		                }

				*argv++;
				continue;
			}

			if(!strcmp(*argv, "delete"))
			{
				struct sna_delete_link_station *dlls;

				if(sna_debug > 5)
					printf("delete ls\n");

		                dlls = (struct sna_delete_link_station *)malloc
                		        (sizeof(struct sna_delete_link_station));
		                memset(dlls, 0, sizeof(struct sna_delete_link_station));
		                memcpy(&dlls->netid, netid, sizeof(struct sna_netid));
		                strcpy(dlls->name, dls->name);
		                strcpy(dlls->saddr, dls->saddr);
		                strcpy(dlls->dname, dls->dname);
		                strcpy(dlls->daddr, dls->daddr);
		                err = nof_delete_link_station(sna_sk, dlls);
		                if(err < 0)
		                {
		                        perror("nof_delete_link_station");
		                        exit (1);
		                }

				*argv++;
				continue;
			}

define_ls:
			printf("define link station\n");
                	err = nof_define_link_station(sna_sk, dls);
                	if(err < 0)
                	{
                	        perror("nof_define_link_station");
                	        exit (1);
                	}

			continue;
		}

		/* LU Options */
                if(!strcmp(*argv, "lu"))
                {
			printf("lu\n");
                        if(*++argv == NULL) continue;

                        if(!strcmp(*argv, "local"))
                        {
				struct sna_define_local_lu *dlu;
 	                       	printf("local\n");
				if(*++argv == NULL) help();
				dlu = (struct sna_define_local_lu *)malloc(sizeof(struct sna_define_local_lu));
				strcpy(dlu->lu_name, *argv);
				for(i = 0; i < 8; i++)
                                	dlu->lu_name[i]=toupper(dlu->lu_name[i]);
				memcpy(&dlu->netid, netid, sizeof(struct sna_netid));
				if(*++argv == NULL) goto define_local_lu;

				if(!strcmp(*argv, "-dlu"))
				{
					printf("-dlu\n");
					if(*++argv == NULL) continue;
				}

				if(!strcmp(*argv, "dlu"))
				{
					printf("dlu\n");
					if(*++argv == NULL) continue;
				}

				if(!strcmp(*argv, "-sync_point"))
				{
					printf("-sync_point\n");
					dlu->sync_point = 0;
					if(*++argv == NULL) continue;
				}

				if(!strcmp(*argv, "sync_point"))
				{
					printf("sync_point\n");
					dlu->sync_point = 1;
					if(*++argv == NULL) continue;
				}

				if(!strcmp(*argv, "lu_sess_limit"))
				{
					if(*++argv == NULL) help();
					printf("lu_sess_limit <%d>\n", atoi(*argv));
					dlu->lu_sess_limit = atoi(*argv);
					if(*++argv == NULL) continue;
				}

				if(!strcmp(*argv, "delete"))
				{
					struct sna_delete_local_lu *del_lu;
					printf("delete\n");
					del_lu = (struct sna_delete_local_lu *)malloc(sizeof(struct sna_delete_local_lu));
					memcpy(&del_lu->netid, &dlu->netid, sizeof(struct sna_netid));
					memcpy(&del_lu->lu_name, &dlu->lu_name, 9);
					err = nof_delete_local_lu(sna_sk, del_lu);
					if(err < 0)
					{
						perror("nof_delete_local_lu");
						exit (1);
					}
					if(*++argv == NULL) continue;
				}

define_local_lu:
				printf("define_local_lu\n");
				err = nof_define_local_lu(sna_sk, dlu);
				if(err < 0)
                        	{
                                	perror("nof_define_local_lu");
                                	exit (1);
                        	}
                               	continue;
                         }

                         if(!strcmp(*argv, "remote"))
                         {
				struct sna_define_partner_lu *plu;
				int i;

                                printf("remote\n");
				if(*++argv == NULL) help();
				plu = (struct sna_define_partner_lu *)malloc(sizeof(struct sna_define_partner_lu));

				strcpy(plu->netid_plu.name, strpbrk(*argv, ".")+1);
                        	for(i = 0; i < 8; i++)
                                	plu->netid_plu.name[i]
                                        	=toupper(plu->netid_plu.name[i]);
                        	strcpy(plu->netid_plu.net, strsep(argv++, "."));
                        	for(i = 0; i < 8; i++)
                                	plu->netid_plu.net[i]
                                        	=toupper(plu->netid_plu.net[i]);

				if(*argv == NULL) help();
				strcpy(plu->netid_fqcp.name, strpbrk(*argv, ".")+1);
                                for(i = 0; i < 8; i++)
                                        plu->netid_fqcp.name[i]
                                                =toupper(plu->netid_fqcp.name[i]);
                                strcpy(plu->netid_fqcp.net, strsep(argv++,"."));
                                for(i = 0; i < 8; i++)
                                        plu->netid_fqcp.net[i]
                                                =toupper(plu->netid_fqcp.net[i]);
				memcpy(&plu->netid, netid, sizeof(struct sna_netid));
				if(*argv == NULL) goto define_partner_lu;

				if(!strcmp(*argv, "-parallel_ss"))
				{
					printf("-parallel_ss\n");
					plu->parallel_ss = 0;
					if(*++argv == NULL) continue;
				}

				if(!strcmp(*argv, "parallel_ss"))
				{
					printf("parallel_ss\n");
					plu->parallel_ss = 1;
					if(*++argv == NULL) continue;
				}

				if(!strcmp(*argv, "-cnv_security"))
				{
					printf("-cnv_security\n");
					plu->cnv_security = 0;
					if(*++argv == NULL) continue;
				}

				if(!strcmp(*argv, "cnv_security"))
				{
					printf("cnv_security\n");
					plu->cnv_security = 1;
					if(*++argv == NULL) continue;
				}

				if(!strcmp(*argv, "delete"))
				{
					struct sna_delete_partner_lu *del_plu;
					printf("delete\n");
					del_plu = (struct sna_delete_partner_lu *)malloc(sizeof(struct sna_delete_partner_lu));
					memcpy(&del_plu->netid, &plu->netid, sizeof(struct sna_netid));
					memcpy(&del_plu->netid_plu, &plu->netid_plu, sizeof(struct sna_netid));
					err = nof_delete_partner_lu(sna_sk, del_plu);
					if(err < 0)
					{
						perror("nof_delete_partner");
						exit (1);
					}
					if(*++argv == NULL) continue;
				}

define_partner_lu:
				printf("define_partner_lu\n");
				err = nof_define_partner_lu(sna_sk, plu);
				if(err < 0)
				{
					perror("nof_define_partner_lu");
					exit (1);
				}
                                continue;
                         }

                          *argv++;
                          continue;
		}

		/* Mode Options */
                if(!strcmp(*argv, "mode"))
                {
			struct sna_define_mode *mode;
			int i;

			if(*++argv == NULL) help();
			mode = (struct sna_define_mode *)malloc(sizeof(struct sna_define_mode));
			strcpy(mode->mode_name, *argv);
                	for(i = 0; i < 8; i++)
                        	mode->mode_name[i]=toupper(mode->mode_name[i]);

			if(*++argv == NULL) help();
        		strcpy(mode->netid_plu.name, strpbrk(*argv, ".")+1);
        		for(i = 0; i < 8; i++)
                		mode->netid_plu.name[i] 
					= toupper(mode->netid_plu.name[i]);
        		for(i = strlen(mode->netid_plu.name); i < 8; i++)
                		mode->netid_plu.name[i] = 0x20;
        		strcpy(mode->netid_plu.net, strsep(argv++, "."));
        		for(i = 0; i < 8; i++)
                		mode->netid_plu.net[i] 
					= toupper(mode->netid_plu.net[i]);
			memcpy(&mode->netid, netid, sizeof(struct sna_netid));
			if(*argv == NULL) goto define_mode;

			if(!strcmp(*argv, "-high_pri"))
			{
				printf("-high_pri\n");

				if(*++argv == NULL) continue;
			}

			if(!strcmp(*argv, "high_pri"))
			{
				printf("high_pri\n");

				if(*++argv == NULL) continue;
			}

			if(!strcmp(*argv, "tx_pacing"))
			{
				if(*++argv == NULL) help();
				printf("tx_pacing <%d>\n", atoi(*argv));

				if(*++argv == NULL) continue;
			}

			if(!strcmp(*argv, "rx_pacing"))
			{
				if(*++argv == NULL) help();
				printf("rx_pacing <%d>\n", atoi(*argv));

				if(*++argv == NULL) continue;
			}

			if(!strcmp(*argv, "max_tx_ru"))
			{
				if(*++argv == NULL) help();
				printf("max_tx_ru <%d>\n", atoi(*argv));

				if(*++argv == NULL) continue;
			}

			if(!strcmp(*argv, "max_rx_ru"))
			{
				if(*++argv == NULL) help();
				printf("max_rx_ru <%d>\n", atoi(*argv));

				if(*++argv == NULL) continue;
			}

			if(!strcmp(*argv, "auto_act_limit"))
			{
				if(*++argv == NULL) help();
				printf("auto_act_limit <%d>\n", atoi(*argv));

				if(*++argv == NULL) continue;
			}

			if(!strcmp(*argv, "min_con_win_limit"))
			{
				if(*++argv == NULL) help();
				printf("min_con_win_limit <%d>\n", atoi(*argv));

				if(*++argv == NULL) continue;
			}

			if(!strcmp(*argv, "ptr_min_con_win_limit"))
			{
				if(*++argv == NULL) help();
				printf("ptr_min_con_win_limit <%d>\n", atoi(*argv));

				if(*++argv == NULL) continue;
			}

			if(!strcmp(*argv, "parallel_sess_limit"))
			{
				if(*++argv == NULL) help();
				printf("parallel_sess_limit <%d>\n", atoi(*argv));

				if(*++argv == NULL) continue;
			}

			if(!strcmp(*argv, "-encryption"))
			{
				printf("-encryption\n");

				if(*++argv == NULL) continue;
			}

			if(!strcmp(*argv, "encryption"))
			{
				printf("encryption\n");

				if(*++argv == NULL) continue;
			}

			if(!strcmp(*argv, "-tx_comp"))
			{
				printf("-tx_comp\n");

				if(*++argv == NULL) continue;
			}

			if(!strcmp(*argv, "tx_comp"))
			{
				printf("tx_comp\n");
				if(*++argv == NULL) help();

				if(!strcmp(*argv, "-rle"))
				{
					printf("-rle\n");
					if(*++argv == NULL) continue;
				}

				if(!strcmp(*argv, "rle"))
				{
					printf("rle\n");
					if(*++argv == NULL) continue;
				}

				if(!strcmp(*argv, "-lz9"))
				{
					printf("-lz9\n");
					if(*++argv == NULL) continue;
				}

				if(!strcmp(*argv, "lz9"))
				{
					printf("lz9\n");
					if(*++argv == NULL) continue;
				}

				if(!strcmp(*argv, "-lz10"))
				{
					printf("-lz10\n");
					if(*++argv == NULL) continue;
				}

				if(!strcmp(*argv, "lz10"))
				{
					printf("lz10\n");
					if(*++argv == NULL) continue;
				}
			}

			if(!strcmp(*argv, "-rx_comp"))
			{
				printf("-rx_comp\n");
				if(*++argv == NULL) continue;
			}

			if(!strcmp(*argv, "rx_comp"))
                        {
                                printf("rx_comp\n");
                                if(*++argv == NULL) help();

                                if(!strcmp(*argv, "-rle"))
                                {
                                        printf("-rle\n");
                                        if(*++argv == NULL) continue;
                                }

                                if(!strcmp(*argv, "rle"))
                                {
                                        printf("rle\n");
                                        if(*++argv == NULL) continue;
                                }

                                if(!strcmp(*argv, "-lz9"))
                                {
                                        printf("-lz9\n");
                                        if(*++argv == NULL) continue;
                                }

                                if(!strcmp(*argv, "lz9"))
				{
                                        printf("rle\n");
                                        if(*++argv == NULL) continue;
                                }

                                if(!strcmp(*argv, "-lz9"))
                                {
                                        printf("-lz9\n");
                                        if(*++argv == NULL) continue;
                                }

                                if(!strcmp(*argv, "lz9"))
                                {
                                        printf("lz9\n");
                                        if(*++argv == NULL) continue;
                                }

                                if(!strcmp(*argv, "-lz10"))
                                {
                                        printf("-lz10\n");
                                        if(*++argv == NULL) continue;
                                }

                                if(!strcmp(*argv, "lz10"))
				{
					printf("lz10\n");
                                        if(*++argv == NULL) continue;
                                }
			}

			if(!strcmp(*argv, "delete"))
			{
				struct sna_delete_mode *dm;
				printf("delete\n");

				dm = (struct sna_delete_mode *)malloc(sizeof(struct sna_delete_mode));
				memcpy(&dm->netid, &mode->netid, sizeof(struct sna_netid));
				memcpy(&dm->netid_plu, &mode->netid_plu, sizeof(struct sna_netid));
				memcpy(&dm->mode_name, &mode->mode_name, 9);
				err = nof_delete_mode(sna_sk, dm);
				if(err < 0)
				{
					perror("nof_delete_mode");
					exit (1);
				}

				if(*++argv == NULL) continue;
			}

define_mode:
			printf("define_mode\n");
			err = nof_define_mode(sna_sk, mode);
                        if(err < 0)
                        {
                                perror("nof_define_mode");
                                exit (1);
                        }
                        continue;
                }

                /* CPI-C Side Information Options */
                if(!strcmp(*argv, "cpic"))
                {
                        printf("cpic\n");
			if(*++argv == NULL) help();

			printf("sym_dest_name\n");
			if(*++argv == NULL) help();

			printf("mode\n");
			if(*++argv == NULL) help();

			printf("plu_name\n");
			if(*++argv == NULL) help();

			printf("tp_name\n");
			if(*++argv == NULL) continue;

			if(!strcmp(*argv, "-srvc_tp"))
			{
				printf("-srvc_tp\n");
				if(*++argv == NULL) continue;
			}

			if(!strcmp(*argv, "srvc_tp"))
			{
				printf("srvc_tp\n");
				if(*++argv == NULL) continue;
			}

			if(!strcmp(*argv, "-secure"))
			{
				printf("-secure\n");
				if(*++argv == NULL) continue;
			}

			if(!strcmp(*argv, "secure"))
			{
				printf("secure\n");
				if(*++argv == NULL) help();

				if(!strcmp(*argv, "-same"))
				{
					printf("-same\n");
					if(*++argv == NULL) continue;
				}

				if(!strcmp(*argv, "-program"))
				{
					printf("-program\n");
					if(*++argv == NULL) continue;
				}

				if(!strcmp(*argv, "-strong"))
				{
					printf("-strong");
					if(*++argv == NULL) continue;
				}

				if(!strcmp(*argv, "same"))
				{
					printf("same\n");
					if(*++argv == NULL) help();
				}

				if(!strcmp(*argv, "program"))
				{
					printf("program\n");
					if(*++argv == NULL) help();
				}

				if(!strcmp(*argv, "strong"))
				{
					printf("strong\n");
					if(*++argv == NULL) help();
				}

				printf("userid\n");
				if(*++argv == NULL) help();

				printf("password\n");
				if(*++argv == NULL) continue;
			}

                        *argv++;
                	continue;
                }

		if(!strcmp(*argv, "start"))
		{
			struct sna_start_node *snode;

			if(sna_debug > 5)
				printf("start\n");

			/* Start Node */
                	snode = (struct sna_start_node *)malloc
                        	(sizeof(struct sna_start_node));

	                memset(snode, 0, sizeof(struct sna_start_node));
	                memcpy(&snode->netid, netid, sizeof(struct sna_netid));
			snode->type 	= node_type;
			snode->max_lus 	= max_lus;
			snode->lu_seg	= lu_seg;
			snode->bind_seg	= bind_seg;
	                err = nof_start_node(sna_sk, snode);
	                if(err < 0)
	                {
	                        perror("nof_start_node");
	                        exit (1);
	                }

			*argv++;
			continue;
		}

		if(!strcmp(*argv, "stop"))
		{
			struct sna_stop_node *snode;

			if(sna_debug > 5)
				printf("stop\n");

			/* Stop Node */
	                snode = (struct sna_stop_node *)malloc
        	                (sizeof(struct sna_stop_node));

        	        memset(snode, 0, sizeof(struct sna_stop_node));
        	        memcpy(&snode->netid, netid, sizeof(struct sna_netid));
        	        err = nof_stop_node(sna_sk, snode);
        	        if(err < 0)
        	        {
        	                perror("nof_stop_node");
        	                exit (1);
        	        }

			*argv++;
			continue;
		}

		if(!strcmp(*argv, "delete"))
		{
			struct sna_delete_node *dnode;

			if(sna_debug > 5)
				printf("delete\n");

                	dnode = (struct sna_delete_node *)malloc
                        	(sizeof(struct sna_delete_node));

                	memset(dnode, 0, sizeof(struct sna_delete_node));
                	memcpy(&dnode->netid, netid, sizeof(struct sna_netid));
                	err = nof_delete_node(sna_sk, dnode);
                	if(err < 0)
                	{
                	        perror("nof_delete_node");
                	        exit (1);
                	}

			*argv++;
			continue;
        	}

		/* Catch all for any commands that we don't do. */
		*argv++;
	}

	close(sna_sk);
	exit (0);
}
