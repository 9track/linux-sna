/* parsecfg.c: Parse the snatchd config file.
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

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <netdb.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "pathnames.h"
#include "snatchd.h"
#include "parsecfg.h"
#include "util.h"

static void loadconfigent(struct servtab *cp);

static FILE *fconfig = NULL;

struct wordmap {
        const char *word;
        int val;
};

static struct wordmap socket_types[] = {
        { "stream",    SOCK_STREAM    },
        { "dgram",     SOCK_DGRAM     },
        { "rdm",       SOCK_RDM       },
        { "seqpacket", SOCK_SEQPACKET },
        { "raw",       SOCK_RAW       },
        { NULL,        -1 }
};

static struct wordmap conversation_types[] = {
	{ "either",	0	},
	{ "basic", 	1	},
	{ "mapped",	2	},
	{ NULL,		-1	}
};

static struct wordmap sync_levels[] = {
	{ "any",	0	},
	{ "none",	1	},
	{ "confirm",	2	},
	{ NULL,		-1	}
};

struct servtab *find_service_by_name(char *name, int len)
{
	struct servtab *sep;
	for(sep = servtab; sep; sep = sep->se_next)
	{
		printf("%s - %s\n", sep->se_service, name);
		if(!strncmp(sep->se_service, name, len))
			return (sep);
	}

	return (NULL);
}

struct servtab *find_service_by_fd(int fd)
{
        struct servtab *sep;
        for(sep = servtab; sep; sep = sep->se_next)
	{
                if(sep->se_fd == fd) 
			return (sep);
        }

        return (NULL);
}

struct servtab *find_service_by_pid(pid_t pid)
{
        struct servtab *sep;
        for(sep = servtab; sep; sep = sep->se_next)
	{
                if(sep->se_wait == pid)
			return (sep);
        }

        return (NULL);
}

static struct servtab *find_same_service(struct servtab *cp)
{
        struct servtab *sep;
        for(sep = servtab; sep; sep = sep->se_next)
	{
                if(strcmp(sep->se_service, cp->se_service))
			continue;
                if(strcmp(sep->se_proto, cp->se_proto)) 
			continue;

                /*
                 * At this point they're the same up to bind address.
                 * Catch 1: se_address can be null.
                 * Catch 2: two different se_addresses can give the same IP.
                 * Catch 3: contents of se_ctrladdr_in.sin_addr are undefined
                 *          if they're unix sockets. But we do know that this
                 *          isn't the case if se_address isn't null.
                 */

                /* Easy case: both null (cannot point to the same string) */
                if(sep->se_address == cp->se_address) 
			return (sep);

                /* Now, if one is null, the other isn't */
                if(sep->se_address == NULL || cp->se_address == NULL) 
			continue;

                /* Don't bother to compare the hostnames, just the IPs */
                if(sep->se_ctrladdr_in.sin_addr.s_addr
                    	== cp->se_ctrladdr_in.sin_addr.s_addr) 
			return (sep);
        }

        return (NULL);
}

const char *service_name(struct servtab *sep)
{
        static char rv[256];
        if(sep->se_address)
                snprintf(rv, sizeof(rv), "%s/%s@%s", sep->se_service,
                         sep->se_proto, sep->se_address);
        else
                snprintf(rv, sizeof(rv), "%s/%s", sep->se_service,
                         sep->se_proto);

        return (rv);
}

void *domalloc(size_t len)
{
        static int retries[] = { 2, 10, 60, 600, -1 };
        void *p;
        int try = 0;

        while(retries[try] > 0) 
	{
                p = malloc(len);
                if(p != NULL)
                        return p;

                syslog(LOG_ERR, "Out of memory - retrying in %d seconds.",
                       retries[try]);
                sleep(retries[try]);
                try++;
        }

        /* Should this be LOG_EMERG? */
        syslog(LOG_ALERT, "Out of memory - GIVING UP!");
        exit(100);
        return (NULL);  /* unreachable */
}

char *dostrdup(const char *cp)
{
        char *x = domalloc(strlen(cp)+1);
        strcpy(x, cp);
        return x;
}

static int map_word(struct wordmap *wm, const char *word)
{
        int i;
        for(i = 0; wm[i].word != NULL; i++)
                if(!strcmp(wm[i].word, word)) 
			return (wm[i].val);

        return (-1);
}

static const char *assemble_entry(struct servtab *sep, int nfields, char **fields)
{
        struct in_addr service_home;
        struct hostent *hp;
        char *s, *t;
        int i;

        if(nfields < 6)
                return ("Incomplete config entry");

        memset(sep, 0, sizeof(*sep));
        sep->se_service = fields[0];
        sep->se_cnvtype = map_word(conversation_types, fields[1]);
        free(fields[1]);
	sep->se_synclevel = map_word(sync_levels, fields[2]);
	free(fields[2]);

        s = strchr(fields[3], '.');
        if(s)
	{
                *s++ = 0;
                sep->se_max = atoi(s);
        }
        else
                sep->se_max = TOOMANY;

        sep->se_wait = !strcmp(fields[3], "wait");
        free(fields[3]);

        s = strchr(fields[4], '.');
        if(s)
	{
                *s++ = 0;
                sep->se_group = s;
        }
        else
                sep->se_group = NULL;

        sep->se_user = fields[4];
        sep->se_server = fields[5];

        /* The rest are argv[]. */
        for(i = 6; i < nfields; i++)
                sep->se_argv[i-6] = fields[i];

        /* Most programs core if argv[0] is null. */
        if(!sep->se_argv[0])
                sep->se_argv[0] = dostrdup(sep->se_server);

        return (NULL);
}

static const char *nexttoken(void)
{
        static char linebuf[256];
        char *s;

        /*
         * Hack: linebuf[0-1] is not read into; it is always "Q ". This permits
         * us to initialize strtok so that it will return the first word on
         * the line on the _next_ call if we so choose.
         */
        linebuf[0] = 'Q';
        linebuf[1] = ' ';

        s = strtok(NULL, " \t\n");
        if(s != NULL) 
		return (s);

        do {
                if(fgets(linebuf+2, sizeof(linebuf)-2, fconfig) == NULL)
                        return (NULL);

        } while (linebuf[2]=='#');

        if(!strchr(" \t", linebuf[2]))
	{
                /* Not a continuation line - send back EOL */
                strtok(linebuf, " \t\n");  /* returns Q */
                return "";
        }

        s = strtok(linebuf+2, " \t\n");
        if(!s) 
		return (""); /* empty line -> send EOL */
        return (s);
}

static void loadconfigfile(void (*sendents)(struct servtab *))
{
        /* 6 fields plus argv, but the last entry of argv[] must remain null */
        char *fields[MAXARGV+6-1];
        int nfields=0;
        int warned=0;
        const char *s;
        int eof=0;
        struct servtab assm; /* assemble entries into here */
        char junk[4];

        /*
         * Insure strtok() will start at EOL.
         */
        junk[0] = 'Q' ;
        junk[1] = 0;
        strtok(junk, " \t\n");

        while (!eof)
	{
                s = nexttoken();
                if(!s && nfields < 1)
                        eof = 1;
                else if (!s || (!*s && nfields>0)) {
                        const char *errmsg;
                        errmsg = assemble_entry(&assm, nfields, fields);
                        if(errmsg)
			{
                                syslog(LOG_WARNING, "Bad config for %s: %s"
                                       " (skipped)", fields[0], errmsg);
                        }
                        else
                                sendents(&assm);

                        if(!s)
                                eof = 1;
                        else
			{
                                nfields = 0;
                                warned = 0;
                        }
                }
                else if (!*s) {
                        /* blank line */
                }
                else if (nfields < MAXARGV+6) {
                        fields[nfields++] = dostrdup(s);
                }
                else if (!warned) {
                        syslog(LOG_WARNING, "%s: too many arguments, max %d",
                               fields[0], MAXARGV);
                        warned = 1;
                }
        }
}

static int setconfig(void)
{
        if(fconfig != NULL)
	{
                fseek(fconfig, 0L, SEEK_SET);
                return (1);
        }
        fconfig = fopen(configfile, "r");
        return (fconfig != NULL);
}

static void endconfig(void)
{
        if(fconfig)
	{
                (void) fclose(fconfig);
                fconfig = NULL;
        }
}

static void freeconfig(struct servtab *cp)
{
        int i;

        if(cp->se_service)
                free(cp->se_service);
        cp->se_service = NULL;

        cp->se_address = NULL;  /* points into se_service */

        if(cp->se_proto)
                free(cp->se_proto);
        cp->se_proto = NULL;

        if(cp->se_user)
                free(cp->se_user);
        cp->se_user = NULL;

        cp->se_group = NULL;  /* points into se_user */

        if(cp->se_server)
                free(cp->se_server);
        cp->se_server = NULL;

        for(i = 0; i < MAXARGV; i++)
	{
                if(cp->se_argv[i])
                        free(cp->se_argv[i]);
                cp->se_argv[i] = NULL;
        }
}

/* print_service(), Dump relevant information to stderr */
static void print_service(const char *action, struct servtab *sep)
{
	fprintf(stderr,
        	"%s: %s cnv_type=%d, sync_level=%d, wait.max=%d.%d, user.group=%s.%s server=%s\n",
                    action, sep->se_service, sep->se_cnvtype, sep->se_synclevel,
                    sep->se_wait, sep->se_max, sep->se_user, sep->se_group,
                    sep->se_server);
}

static void loadconfigent(struct servtab *cp)
{
        struct servtab *sep;
        unsigned n;

        /*
         * dholland 7/14/1997: always use the canonical service
         * name to protect against silly configs that list a
         * service twice under different aliases. This has been
         * observed in the wild thanks to Slackware...
         * Note: this is a patch, not a fix. The real fix is
         * to key the table by port and protocol, not service name
         * and protocol.
         */
        if (cp->se_family==AF_INET && !isrpcservice(cp)) {
                u_short port = htons(atoi(cp->se_service));

                if (!port) {
                        struct servent *sp;
                        sp = getservbyname(cp->se_service,
                                           cp->se_proto);
                        if (sp != NULL) { /* bogus services are caught later */
                                if (strcmp(cp->se_service, sp->s_name)) {
                                        /*
                                         * Ugh. Since se_address points into
                                         * se_service, we need to copy both
                                         * together. Ew.
                                         */
                                        char *tmp, *tmp2 = NULL;
                                        const char *addr = cp->se_address;
                                        size_t len = strlen(sp->s_name)+1;
                                        if (addr==NULL) addr = "";
                                        len += strlen(addr)+1;
                                        tmp = domalloc(len);
                                        strcpy(tmp, cp->se_service);
                                        if (cp->se_address) {
                                                tmp2 = tmp+strlen(tmp)+1;
                                                strcpy(tmp2, cp->se_address);
                                        }
                                        free(cp->se_service);
                                        cp->se_service = tmp;
                                        cp->se_address = tmp2;
                                }
                        }
                }
        }
        /* End silly patch */

        sep = find_same_service(cp);

        if (sep != NULL) {
                int i;

                if (sep->se_checked) {
                        syslog(LOG_WARNING,
                               "extra conf for service %s (skipped)\n",
                               service_name(sep));
                        return;
                }

#define SWAP(type, a, b) {type c=(type)a; (type)a=(type)b; (type)b=(type)c;}

                /*
                 * sep->se_wait may be holding the pid of a daemon
                 * that we're waiting for.  If so, don't overwrite
                 * it unless the config file explicitly says don't
                 * wait.
                 */
                if ((sep->se_wait == 1 || cp->se_wait == 0))
                        sep->se_wait = cp->se_wait;
                if (cp->se_max != sep->se_max)
                        SWAP(int, cp->se_max, sep->se_max);
                if (cp->se_user)
                        SWAP(char *, sep->se_user, cp->se_user);
                if (cp->se_group)
                        SWAP(char *, sep->se_group, cp->se_group);
                if (cp->se_server)
                        SWAP(char *, sep->se_server, cp->se_server);
                if (cp->se_address) {
                        /* must swap se_service; se_address points into it */
                        SWAP(char *, sep->se_service, cp->se_service);
                        SWAP(char *, sep->se_address, cp->se_address);
                }
                for (i = 0; i < MAXARGV; i++)
                        SWAP(char *, sep->se_argv[i], cp->se_argv[i]);
#undef SWAP
                sep->se_rpcversl = cp->se_rpcversl;
                sep->se_rpcversh = cp->se_rpcversh;
                freeconfig(cp);
                if (debug) {
                        print_service("REDO", sep);
                }
        }
        else {
                sep = enter(cp);
                if (debug)
                        print_service("ADD ", sep);
        }
        sep->se_checked = 1;

        if (sep->se_fd != -1)
		return;
	(void)unlink(sep->se_service);
                n = strlen(sep->se_service);
                if (n > sizeof(sep->se_ctrladdr_un.sun_path) - 1)
                        n = sizeof(sep->se_ctrladdr_un.sun_path) - 1;
                strncpy(sep->se_ctrladdr_un.sun_path,
                        sep->se_service, n);
                sep->se_ctrladdr_un.sun_path[n] = 0;
                sep->se_ctrladdr_un.sun_family = AF_UNIX;
                sep->se_ctrladdr_size = n +
                        sizeof sep->se_ctrladdr_un.sun_family;
	setup(sep);
}

void config(int signum)
{
        register struct servtab *sep, **sepp;

        (void)signum;

        if(!setconfig())
	{
                syslog(LOG_ERR, "%s: %m", configfile);
                return;
        }
        for(sep = servtab; sep; sep = sep->se_next)
                sep->se_checked = 0;

        loadconfigfile(loadconfigent);
        endconfig();

        /* Purge anything not looked at above. */
        sepp = &servtab;
        while((sep = *sepp) != NULL)
	{
                if(sep->se_checked)
		{
                        sepp = &sep->se_next;
                        continue;
                }
                *sepp = sep->se_next;
                if(sep->se_fd != -1)
                        closeit(sep);
                if(sep->se_family == AF_UNIX)
                        (void)unlink(sep->se_service);
                if(debug)
                        print_service("FREE", sep);
                freeconfig(sep);
                free((char *)sep);
        }
}

/* SIGALRM handler */
void restart_services(void)
{
        struct servtab *sep;
        for(sep = servtab; sep; sep = sep->se_next)
	{
                if(sep->se_fd == -1) 
		{
                        switch(sep->se_family)
			{
                        	case AF_UNIX:
                        	case AF_INET:
                                	setup(sep);
                                	break;
                        }
                }
        }
}
