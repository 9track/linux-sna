/* llcping.c: Linux-SNA LLC Ping utility.
 *
 * Written by Jay Schulist <jschlst@turbolinux.com>
 *
 * This software may be used and distributed according to the terms
 * of the GNU Public License, incorporated herein by reference.
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/signal.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <ctype.h>

#include <linux/if_ether.h>
#include <linux/llc.h>

#ifndef LLC_PING_SAP
#define LLC_PING_SAP	0x88
#endif

#ifndef AF_LLC
#define AF_LLC          24
#define PF_LLC          AF_LLC
#endif

#ifndef SOL_LLC
#define SOL_LLC         277
#endif

struct llchdr {
        unsigned char dsap;
        unsigned char ssap;
        unsigned char ctrl;
};

#define LLC_CTRL_TEST_CMD	0xF3
#define LLC_CTRL_TEST_RSP	0xF3

/*
 * Note: on some systems dropping root makes the process dumpable or
 * traceable. In that case if you enable dropping root and someone
 * traces ping, they get control of a raw socket and can start
 * spoofing whatever packets they like. SO BE CAREFUL.
 */

static time_t last_time;

#define LLC_MINLEN	(3 + 8)

#define	DEFDATALEN	(64 - 8)	/* default data length */
#define	MAXPACKET	(65535 - 8 - 3)/* max packet size */
#define	MAXWAIT		10		/* max seconds to wait for response */

#define	A(bit)		rcvd_tbl[(bit)>>3]	/* identify byte in array */
#define	B(bit)		(1 << ((bit) & 0x07))	/* identify bit in byte */
#define	SET(bit)	(A(bit) |= B(bit))
#define	CLR(bit)	(A(bit) &= (~B(bit)))
#define	TST(bit)	(A(bit) & B(bit))

/* various options */
int options;
#define	F_FLOOD		0x001
#define	F_INTERVAL	0x002
#define	F_NUMERIC	0x004
#define	F_PINGFILLED	0x008
#define	F_QUIET		0x010
#define	F_RROUTE	0x020
#define	F_SO_DEBUG	0x040
#define	F_SO_DONTROUTE	0x080
#define	F_VERBOSE	0x100

/* multicast options */
int moptions;
#define MULTICAST_NOLOOP	0x001
#define MULTICAST_TTL		0x002
#define MULTICAST_IF		0x004

/*
 * MAX_DUP_CHK is the number of bits in received table, i.e. the maximum
 * number of received sequence numbers we can keep track of.  Change 128
 * to 8192 for complete accuracy...
 */
#define	MAX_DUP_CHK	(8 * 128)
int mx_dup_ck = MAX_DUP_CHK;
char rcvd_tbl[MAX_DUP_CHK / 8];

struct sockaddr_llc whereto;	/* who to ping */
int datalen = DEFDATALEN;
int s;				/* socket file descriptor */
u_char outpack[MAXPACKET];
char BSPACE = '\b';		/* characters written for flood */
char DOT = '.';
static char *hostname;
static int ident;		/* process id to identify our packets */

/* counters */
static long npackets;		/* max packets to transmit */
static long nreceived;		/* # of packets we got back */
static long nrepeats;		/* number of duplicates */
static long ntransmitted;	/* sequence # for outbound packets = #sent */
static int interval = 1;	/* interval between packets */

/* timing */
static int timing;		/* flag to do timing */
static long tmin = LONG_MAX;	/* minimum round trip time */
static long tmax = 0;		/* maximum round trip time */
static u_long tsum;		/* sum of all times, for doing average */

/* protos */
static void catcher(int);
static void finish(int ignore);
static void pinger(void);
static void fill(void *bp, char *patp);
static void usage(void);
static void tvsub(struct timeval *out, struct timeval *in);
int hexdump(unsigned char *pkt_data, int pkt_len);

void pr_pack(char *buf, int cc, struct sockaddr_in *from)
{
        struct timeval tv, *tp;
        long triptime = 0;

        (void)gettimeofday(&tv, (struct timezone *)NULL);
	++nreceived;

	tp = (struct timeval *)(buf + 8);
	tvsub(&tv, tp);
	triptime = tv.tv_sec * 10000 + (tv.tv_usec / 100);
	tsum += triptime;
	if (triptime < tmin)
		tmin = triptime;
	if (triptime > tmax)
		tmax = triptime;

	if (options & F_QUIET)
                        return;

        if (options & F_FLOOD)
	        (void)write(STDOUT_FILENO, &BSPACE, 1);
	else
	{
		(void)printf("%d bytes from %s:", cc, hostname);
        	(void)printf(" num=%ld time=%ld.%ld ms\n", nreceived, triptime/10, triptime%10);
	}
}

/* Input an Ethernet address and convert to binary. Save in sap->sa_dst; */
static int in_ether(char *bufp, struct sockaddr_llc *sap, int w)
{
    unsigned char *ptr;
    char c, *orig;
    int i;
    unsigned val;

    if(w == 1)	/* Dst */
    	ptr = sap->sllc_dmac;
    else	/* Src */
	ptr = sap->sllc_smac;

    i = 0;
    orig = bufp;
    while ((*bufp != '\0') && (i < ETH_ALEN)) {
        val = 0;
        c = *bufp++;
        if (isdigit(c))
            val = c - '0';
        else if (c >= 'a' && c <= 'f')
            val = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F')
            val = c - 'A' + 10;
        else {
            errno = EINVAL;
            return (-1);
        }
        val <<= 4;
        c = *bufp;
        if (isdigit(c))
            val |= c - '0';
        else if (c >= 'a' && c <= 'f')
            val |= c - 'a' + 10;
        else if (c >= 'A' && c <= 'F')
            val |= c - 'A' + 10;
        else if (c == ':' || c == 0)
            val >>= 4;
        else {
            errno = EINVAL;
            return (-1);
        }
        if (c != 0)
            bufp++;
        *ptr++ = (unsigned char) (val & 0377);
        i++;

        /* We might get a semicolon here - not required. */
        if (*bufp == ':') {
            if (i == ETH_ALEN) {
                    ;           /* nothing */
            }
            bufp++;
        }
    }

    return (0);
}

int hexdump(unsigned char *pkt_data, int pkt_len)
{
        int i;

        while(pkt_len>0)
        {
                printf("  ");   /* Leading spaces. */

                /* Print the HEX representation. */
                for(i=0; i<8; ++i)
                {
                        if(pkt_len - (long)i>0)
                                printf("%2.2X ", pkt_data[i] & 0xFF);
                        else
                                printf("  ");
                }

                printf(":");

                for(i=8; i<16; ++i)
                {
                        if(pkt_len - (long)i>0)
                                printf("%2.2X ", pkt_data[i]&0xFF);
                        else
                                printf("  ");
                }

                /* Print the ASCII representation. */
                printf("  ");

                for(i=0; i<16; ++i)
                {
                        if(pkt_len - (long)i>0)
                        {
                                if(isprint(pkt_data[i]))
                                        printf("%c", pkt_data[i]);
                                else
                                        printf(".");
                        }
                }

                printf("\n");
                pkt_len -= 16;
                pkt_data += 16;
        }

        printf("\n");

        return (0);
}

int main(int argc, char *argv[])
{
	struct timeval timeout;
	struct sockaddr_llc *to;
	int i;
	int ch, fdmask, hold, packlen, preload;
	u_char *datap, *packet;
	char *target, *from;
	int am_i_root;
	unsigned char sap;

	static char *null = NULL;
	__environ = &null;
	am_i_root = (getuid()==0);

	/* setup last_time to be current time -1 second */
	time(&last_time);
	last_time--;

	setuid(getuid());
	preload = 0;
	datap = &outpack[8 + sizeof(struct timeval)];
	while ((ch = getopt(argc, argv, "I:LRc:dfh:i:l:np:qrs:t:v")) != EOF)
	{
		switch(ch) {
		case 'c':
			npackets = atoi(optarg);
			if (npackets <= 0) {
				(void)fprintf(stderr,
				    "llcping: bad number of packets to transmit.\n");
				exit(2);
			}
			break;

		case 'f':
			if (!am_i_root) {
				(void)fprintf(stderr,
				    "llcping: %s\n", strerror(EPERM));
				exit(2);
			}
			options |= F_FLOOD;
			setbuf(stdout, NULL);
			break;

		case 'i':		/* wait between sending packets */
			interval = atoi(optarg);
			if (interval <= 0) {
				(void)fprintf(stderr,
				    "llcping: bad timing interval.\n");
				exit(2);
			}
			options |= F_INTERVAL;
			break;

		case 'p':		/* fill buffer with user pattern */
			options |= F_PINGFILLED;
			fill(datap, optarg);
			break;

		case 'q':
			options |= F_QUIET;
			break;

		case 's':		/* size of packet to send */
			datalen = atoi(optarg);
			if (datalen > MAXPACKET) {
				(void)fprintf(stderr,
				    "llcping: packet size too large.\n");
				exit(2);
			}
			if (datalen <= LLC_MINLEN) {
				(void)fprintf(stderr,
				    "llcping: illegal packet size.\n");
				exit(2);
			}
			break;

		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	memset(&whereto, 0, sizeof(struct sockaddr_llc));
        to = (struct sockaddr_llc *)&whereto;

	from = *argv;
	in_ether(from, to, 2);

	argc -= optind;
        argv += optind;
	
	if (argc != 1)
		usage();
	target = *argv;
	in_ether(target, to, 1);
	hostname = target;

	if((s = socket(PF_LLC, SOCK_DGRAM, 0)) < 0)
        {
                if(errno == EPERM)
                        fprintf(stderr, "llcping: ping must run as root\n");
                else
                        perror("llcping: socket");
                exit(2);
        }

        sap = (unsigned char)LLC_PING_SAP;
        if(setsockopt(s, SOL_LLC, LLC_REG_SAP_CLIENT, &sap,
                sizeof(LLC_PING_SAP)))
        {
                perror("llcping: sap setsockopt");
                exit(2);
        }

	to->sllc_family = AF_LLC;

	if(options & F_FLOOD && options & F_INTERVAL)
	{
		(void)fprintf(stderr,
		    "llcping: -f and -i incompatible options.\n");
		exit(2);
	}

	if(datalen >= (int)sizeof(struct timeval)) /* can we time transfer */
		timing = 1;
	packlen = datalen;
	packet = malloc((u_int)packlen);
	if(!packet)
	{
		(void)fprintf(stderr, "llcping: out of memory.\n");
		exit(2);
	}
	if(!(options & F_PINGFILLED))
	{
		for (i = 8; i < datalen; ++i)
			*datap++ = i;
	}

	ident = getpid() & 0xFFFF;
	hold = 1;

	printf("LLCPING v1.0, LLC PING for Linux-SNA.\n");

	printf("LLCPING - %s: %d data bytes\n", hostname, datalen);

	(void)signal(SIGINT, finish);
	(void)signal(SIGALRM, catcher);

	while(preload--)		/* fire off them quickies */
		pinger();

	if((options & F_FLOOD) == 0)
		catcher(0);		/* start things going */

	for(;;)
	{
		struct sockaddr_in from;
		register int cc;
		size_t fromlen;

		if (options & F_FLOOD)
		{
			pinger();
			timeout.tv_sec = 0;
			timeout.tv_usec = 10000;
			fdmask = 1 << s;
			if (select(s + 1, (fd_set *)&fdmask, (fd_set *)NULL,
			    (fd_set *)NULL, &timeout) < 1)
				continue;
		}
		fromlen = sizeof(from);
		if((cc = recvfrom(s, (char *)packet, packlen, 0,
		    (struct sockaddr *)&from, &fromlen)) < 0)
		{
			if(errno == EINTR)
				continue;
			perror("llcping: recvfrom");
			continue;
		}
		pr_pack((char *)packet, cc, &from);
		if(npackets && nreceived >= npackets)
			break;
	}
	finish(0);

	/* NOTREACHED */
	return 0;
}

 /* This routine causes another PING to be transmitted, and then
 * schedules another SIGALRM for 1 second from now.
 */
static void catcher(int ignore)
{
	int waittime;

	(void)ignore;
	pinger();
	(void)signal(SIGALRM, catcher);
	if(!npackets || ntransmitted < npackets)
		alarm((u_int)interval);
	else
	{
		if(nreceived)
		{
			waittime = 2 * tmax / 1000;
			if(!waittime)
				waittime = 1;
			if(waittime > MAXWAIT)
				waittime = MAXWAIT;
		}
		else
			waittime = MAXWAIT;
		(void)signal(SIGALRM, finish);
		(void)alarm((u_int)waittime);
	}
}

/* Compose and transmit an LLC TEST CMD packet. The first 8 bytes
 * of the data portion are used to hold a UNIX "timeval" struct in VAX
 * byte-order, to compute the round-trip time.
 */
static void pinger(void)
{
	register struct llchdr *llc;
	register int cc;
	int i;

	ntransmitted++;

	llc = (struct llchdr *)outpack;
	llc->dsap = 0;
	llc->ssap = LLC_PING_SAP;
	llc->ctrl = LLC_CTRL_TEST_CMD;

	(void)gettimeofday((struct timeval *)&outpack[8],
	    (struct timezone *)NULL);

	cc = datalen + 8;
	i = sendto(s, (char *)outpack, cc, 0, (struct sockaddr_in *)&whereto,
	    sizeof(struct sockaddr_llc));

	if(i < 0 || i != cc)
	{
		if(i < 0)
			perror("llcping: sendto");
		(void)printf("llcping: wrote %s %d chars, ret=%d\n",
		    hostname, cc, i);
	}
	if(!(options & F_QUIET) && options & F_FLOOD)
		(void)write(STDOUT_FILENO, &DOT, 1);
}

/* Subtract 2 timeval structs:  out = out - in.  Out is assumed to
 * be >= in.
 */
static void tvsub(register struct timeval *out, register struct timeval *in)
{
	if((out->tv_usec -= in->tv_usec) < 0)
	{
		--out->tv_sec;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}

/* Print out statistics, and give up. */
static void finish(int ignore)
{
	(void)ignore;

	(void)signal(SIGINT, SIG_IGN);
	(void)putchar('\n');
	(void)fflush(stdout);
	(void)printf("--- %s llcping statistics ---\n", hostname);
	(void)printf("%ld packets transmitted, ", ntransmitted);
	(void)printf("%ld packets received, ", nreceived);
	if(nrepeats)
		(void)printf("+%ld duplicates, ", nrepeats);
	if(ntransmitted)
	{
		if(nreceived > ntransmitted)
			(void)printf("-- somebody's printing up packets!");
		else
			(void)printf("%d%% packet loss",
			    (int) (((ntransmitted - nreceived) * 100) /
			    ntransmitted));
	}
	(void)putchar('\n');
	if(nreceived && timing)
	{
		(void)printf("round-trip min/avg/max = %ld.%ld/%lu.%ld/%ld.%ld ms\n",
			tmin/10, tmin%10,
			(tsum / (nreceived + nrepeats))/10,
			(tsum / (nreceived + nrepeats))%10,
			tmax/10, tmax%10);
	}

	if(nreceived==0)
		exit(1);

	exit(0);
}

static void fill(void *bp1, char *patp)
{
	register int ii, jj, kk;
	int pat[16];
	char *cp, *bp = (char *)bp1;

	for(cp = patp; *cp; cp++)
	{
		if(!isxdigit(*cp))
		{
			(void)fprintf(stderr,
			    "llcping: patterns must be specified as hex digits.\n");
			exit(2);
		}
	}

	ii = sscanf(patp,
	    "%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x",
	    &pat[0], &pat[1], &pat[2], &pat[3], &pat[4], &pat[5], &pat[6],
	    &pat[7], &pat[8], &pat[9], &pat[10], &pat[11], &pat[12],
	    &pat[13], &pat[14], &pat[15]);

	if(ii > 0)
	{
		for (kk = 0; kk <= MAXPACKET - (8 + ii); kk += ii)
			for (jj = 0; jj < ii; ++jj)
				bp[jj + kk] = pat[jj];
	}

	if(!(options & F_QUIET))
	{
		(void)printf("PATTERN: 0x");
		for(jj = 0; jj < ii; ++jj)
			(void)printf("%02x", bp[jj] & 0xFF);
		(void)printf("\n");
	}
}

static void usage(void)
{
	(void)fprintf(stderr, "LLCPING v1.0, LLC PING for Linux-SNA.\n\n");
	(void)fprintf(stderr,
	    "Usage: llcping [-fq] [-c count] [-i wait] [-p pattern]\n\t[-s packetsize] SRCMACADDRES DSTMACADDRES\n");

	exit(2);
}
