/* snatchd.c: Linux-SNA Attach Manger. (Inetd for SNA).
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
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/un.h>

#include <netinet/in.h>
#include <arpa/inet.h>  /* for inet_ntoa */

#include <errno.h>
#include <netdb.h>
#include <syslog.h>
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>

#include "pathnames.h"
#include "snatchd.h"
#include "parsecfg.h"
#include "util.h"

#include <linux/attach.h>
#include "attach.h"
static int afd = 0;		/* SNA Attach Session FD */

static void logpid(void);
static int bump_nofile(void);

struct servtab *servtab;                     /* service table */
const char *configfile = _PATH_SNATCHDCONF;    /* config file path */
int debug = 0;                               /* debug flag */

/* Reserve some descriptors, 3 stdio + at least: 1 log, 1 conf. file */
#define FD_MARGIN       (8)

/* Length of socket listen queue. Should be per-service probably. */
static int      	global_queuelen = 128;
static volatile int     nsock;
static int              maxsock;
static fd_set           allsock;
static int              options;
static int              timingout;

static long rlim_ofile_cur = FOPEN_MAX;
struct rlimit   rlim_ofile;

static void discard_stupid_environment(void)
{
        static const char *const junk[] = {
                /* these are prefixes */
                "CVS",
                "DISPLAY=",
                "EDITOR=",
                "GROUP=",
                "HOME=",
                "IFS=",
                "LD_",
                "LOGNAME=",
                "MAIL=",
                "PATH=",
                "PRINTER=",
                "PWD=",
                "SHELL=",
                "SHLVL=",
                "SSH",
                "TERM",
                "TMP",
                "USER=",
                "VISUAL=",
                NULL
        };

        int i,k=0;
        for (i=0; __environ[i]; i++) {
                int found=0, j;
                for (j=0; junk[j]; j++) {
                        if (!strncmp(__environ[i], junk[j], strlen(junk[j]))) {
                                found=1;
                        }
                }
                if (!found) {
                        __environ[k++] = __environ[i];
                }
        }
        __environ[k] = NULL;
}

static void exec_child(struct servtab *sep)
{
        struct passwd *pwd;
        struct group *grp = NULL;
        int tmpint;
        uid_t uid;
        gid_t gid;

        /*
         * If debugging, we're in someone else's session; make a new one.
         */
        if (debug) {
                setsid();
        }

        /*
         * Prepare to exec.
         */

        pwd = getpwnam(sep->se_user);
        if (pwd == NULL) {
                syslog(LOG_ERR, "getpwnam: %s: No such user", sep->se_user);
                return;
        }

        /*
         * Use the uid and gid of the user.
         */
        uid = pwd->pw_uid;
        gid = pwd->pw_gid;

        /*
         * If a group was specified, use its gid instead of the user's gid.
         */
        if (sep->se_group) {
                grp = getgrnam(sep->se_group);
                if (grp == NULL) {
                        syslog(LOG_ERR, "getgrnam: %s: No such group",
                               sep->se_group);
                        return;
                }
                gid = grp->gr_gid;
        }

        /*
         * If a nonroot user, do initgroups to run with that user's group
         * list.
         *
         * But if root, do not use root's group list - just use the one gid.
         *
         * If no group was specified, keep the group inetd was run under.
         * This is the traditional behavior, but seems dumb - shouldn't
         * we use the group from the password file? XXX.
         */

        if (uid) {
                setgid(gid);
                initgroups(pwd->pw_name, gid);
                setuid(uid);
        }
        else if (grp) {
                setgid(gid);
                setgroups(1, &gid);
        }

        if (debug) {
                gid_t tmp[NGROUPS_MAX];
                int n, i;
                fprintf(stderr, "pid %d: exec %s\n", getpid(), sep->se_server);
                fprintf(stderr, "uid: %d  gid: %d\n", getuid(), getgid());
                fprintf(stderr, "groups: ");
                n = getgroups(NGROUPS_MAX, tmp);
                for (i=0; i<n; i++) fprintf(stderr, "%d ", tmp[i]);
                fprintf(stderr, "\n");
        }

#ifdef MULOG
        if (sep->se_log) {
                dolog(sep, 0);
        }
#endif

        if (rlim_ofile.rlim_cur != rlim_ofile_cur) {
                if (setrlimit(RLIMIT_NOFILE, &rlim_ofile) < 0) {
                        syslog(LOG_ERR,"setrlimit: %m");
                }
        }

        /*
         * Transfer stdout to stderr. This is not with the other dup2's
         * so debug logging works.
         */
        dup2(1, 2);
        for (tmpint = rlim_ofile_cur-1; tmpint > 2; tmpint--) {
                close(tmpint);
        }

        sig_preexec();

        execv(sep->se_server, sep->se_argv);
        syslog(LOG_ERR, "execv %s: %m", sep->se_server);
}

static pid_t fork_child(struct servtab *sep)
{
        pid_t pid;

        if (sep->se_count++ == 0) {
                gettimeofday(&sep->se_time, NULL);
        }
        else if (sep->se_count >= sep->se_max) {
                struct timeval now;

                gettimeofday(&now, (struct timezone *)0);
                if (now.tv_sec - sep->se_time.tv_sec > CNT_INTVL) {
                        sep->se_time = now;
                        sep->se_count = 1;
                }
                else {
                        syslog(LOG_ERR, "%s server failing (looping or "
                               "being flooded), service terminated for "
                               "%d min\n",
                               service_name(sep),
                               RETRYTIME/60);

                        FD_CLR(sep->se_fd, &allsock);
                        close(sep->se_fd);
                        sep->se_fd = -1;

                        sep->se_count = 0;
                        nsock--;
                        if (!timingout) {
                                timingout = 1;
                                alarm(RETRYTIME);
                        }
                        return -1;
                }
        }
        pid = fork();
        if (pid<0) {
                syslog(LOG_ERR, "fork: %m");
        }
        return pid;
}

static void launch(struct servtab *sep, struct tp_attach *tp)
{
        char buf[50];
        int ctrl, dofork;
	pid_t pid;

        if (debug) {
                fprintf(stderr, "launching: %s\n", sep->se_service);
        }

        ctrl = sep->se_fd;
        pid = fork_child(sep);
        if (pid < 0) 
	{
		if (ctrl != sep->se_fd)
                	close(ctrl);
                sleep(1);
                return;
	}
        if (pid==0) 
	{
		/* child */
                dup2(ctrl, 0);
                close(ctrl);
                dup2(0, 1);
                /* don't do stderr yet */
                exec_child(sep);
                _exit(1);
	}
	syslog(LOG_ERR, "Correlating this thing %d %ld\n", pid, tp->tcb_id);
        tp_correlate(afd, pid, tp->tcb_id, tp->tp_name);
}

int main(int argc, char *argv[], char *envp[])
{
	int ch, n, i, nodaemon = 0;
	struct servtab *sep;
	fd_set readable;
        char *progname;
	gid_t gid;

        gid = getgid();
        setgroups(1, &gid);

	initsetproctitle(argc, argv, envp);
	discard_stupid_environment();

        progname = strrchr(argv[0], '/');
        if(progname == NULL)
                progname = argv[0];
        else
                progname++;

	while((ch = getopt(argc, argv, "hdiq:")) != EOF)
	{
                switch(ch) 
		{
                	case 'd':
                        	debug = nodaemon = 1;
                        	options |= SO_DEBUG;
                        	break;

                	case 'i':
                        	nodaemon = 1;
                        	break;

                	case 'q':
                        	global_queuelen = atoi(optarg);
                        	if (global_queuelen < 8) 
					global_queuelen = 8;
                        	break;

                	case '?':
                	default:
                        	fprintf(stderr, "usage: %s [-di] [-q len] [conf]\n",
                                	progname);
                        	exit(1);
                }
	}

        argc -= optind;
        argv += optind;

        if(argc > 0)
                configfile = argv[0];

        if(nodaemon == 0)
                daemon(0, 0);
        else
	{
		if(debug == 0)
                	setsid();
        }

        openlog(progname, LOG_PID | LOG_NOWAIT, LOG_DAEMON);
        logpid();

	if (getrlimit(RLIMIT_NOFILE, &rlim_ofile) < 0)
                syslog(LOG_ERR, "getrlimit: %m");
        else 
	{
                rlim_ofile_cur = rlim_ofile.rlim_cur;
                if (rlim_ofile_cur == RLIM_INFINITY)    /* ! */
                        rlim_ofile_cur = FOPEN_MAX;
        }

	/* Open the SNA Attach Channel */
	afd = attach_open();

	config(0);

        sig_init();
        sig_block();

        for(;;)
	{
		struct tp_attach tp;

                readable = allsock;
                sig_unblock();
		memset(&tp, 0, sizeof(struct tp_attach));
		n = attach_listen(afd, &tp, sizeof(struct tp_attach), 0);
                sig_block();

                if(n <= 0)
		{
                        if(n < 0 && errno != EINTR)
			{
                                syslog(LOG_WARNING, "attach_listen: %m");
                                sleep(1);
                        }
                        continue;
                }

		sep = find_service_by_name(tp.tp_name, tp.tp_len);
		if(sep == NULL)
		{
			syslog(LOG_ERR, "can't find service");
			continue;
		}
		launch(sep, &tp);
        }

	return (0);
}

void reapchild(int signum)
{
        int status;
        pid_t pid;
        register struct servtab *sep;
        const char *name;
        char tmp[64];

        (void)signum;

        while ((pid = wait3(&status, WNOHANG, NULL)) > 0) {
                if (debug) {
                        fprintf(stderr, "pid %d, exit status %x\n", pid,
                                status);
                }

                sep = find_service_by_pid(pid);
                if (sep==NULL) {
                        snprintf(tmp, sizeof(tmp), "pid %d", (int)pid);
                        name = tmp;
                }
                else {
                        snprintf(tmp, sizeof(tmp), "%s (pid %d)",
                                 sep->se_server, (int)pid);
                        name = tmp;
                }

                if (WIFEXITED(status) && WEXITSTATUS(status)) {
                        syslog(LOG_WARNING, "%s: exit status %d", name,
                               WEXITSTATUS(status));
                }
                else if (WIFSIGNALED(status)) {
                        syslog(LOG_WARNING, "%s: exit signal %d", name,
                               WTERMSIG(status));
                }

                if (sep!=NULL) {
                        sep->se_wait = 1;
                        FD_SET(sep->se_fd, &allsock);
                        nsock++;
                        if (debug) {
                                fprintf(stderr, "restored %s, fd %d\n",
                                        sep->se_service, sep->se_fd);
                        }
                }
        }
}

void retry(int signum)
{
        (void)signum;

        timingout = 0;

        restart_services();
}

void goaway(int signum)
{
        register struct servtab *sep;

        (void)signum;
        for(sep = servtab; sep; sep = sep->se_next) 
	{
                if(sep->se_fd == -1)
                        continue;

                switch (sep->se_family) {
                case AF_UNIX:
                        (void)unlink(sep->se_service);
                        break;
                }
		tp_unregister(afd, sep->se_fd);
        }
        (void)unlink(_PATH_SNATCHDPID);

	attach_close(afd);		/* Close Attach Session */
        exit(0);
}

void closeit(struct servtab *sep)
{
        FD_CLR(sep->se_fd, &allsock);
        nsock--;
        (void) close(sep->se_fd);
        sep->se_fd = -1;
}

void setup(struct servtab *sep)
{
        int on = 1;
	struct tp_info *tp = (struct tp_info *)malloc(sizeof(struct tp_info));

	strcpy(tp->tp_name, sep->se_service);
	if((sep->se_fd = tp_register(afd, tp)) < 0)
	{
                syslog(LOG_ERR, "%s: tp_register: %m", service_name(sep),
                    sep->se_service, sep->se_proto);
                return;
        }
        if (bind(sep->se_fd, &sep->se_ctrladdr, sep->se_ctrladdr_size) < 0) {
                syslog(LOG_ERR, "%s: bind: %m", service_name(sep),
                    sep->se_service, sep->se_proto);
                (void) close(sep->se_fd);
                sep->se_fd = -1;
                if (!timingout) {
                        timingout = 1;
                        alarm(RETRYTIME);
                }
                return;
        }
        if (sep->se_socktype == SOCK_STREAM)
                listen(sep->se_fd, global_queuelen);

        FD_SET(sep->se_fd, &allsock);
        nsock++;
        if (sep->se_fd > maxsock) {
                maxsock = sep->se_fd;
                if (maxsock > rlim_ofile_cur - FD_MARGIN)
                        bump_nofile();
        }
}

struct servtab *enter(struct servtab *cp)
{
        register struct servtab *sep;

        sep = domalloc(sizeof(*sep));
        *sep = *cp;
        sep->se_fd = -1;
        sep->se_rpcprog = -1;
        sep->se_next = servtab;
        servtab = sep;
        return (sep);
}

//static char *skip(char **);
//static char *nextline(FILE *);

static void logpid(void)
{
        FILE *fp;

        if ((fp = fopen(_PATH_SNATCHDPID, "w")) != NULL) {
                fprintf(fp, "%u\n", getpid());
                (void)fclose(fp);
        }
}

static int bump_nofile(void)
{
#define FD_CHUNK        32

        struct rlimit rl;

        if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
                syslog(LOG_ERR, "getrlimit: %m");
                return -1;
        }
        rl.rlim_cur = MIN(rl.rlim_max, rl.rlim_cur + FD_CHUNK);
        if (rl.rlim_cur <= rlim_ofile_cur) {
                syslog(LOG_ERR,
                        "bump_nofile: cannot extend file limit, max = %d",
                        rl.rlim_cur);
                return -1;
        }

        if (setrlimit(RLIMIT_NOFILE, &rl) < 0) {
                syslog(LOG_ERR, "setrlimit: %m");
                return -1;
        }

        rlim_ofile_cur = rl.rlim_cur;
        return 0;
}

#ifdef MULOG
dolog(sep, ctrl)
        struct servtab *sep;
        int             ctrl;
{
        struct sockaddr         sa;
        struct sockaddr_in      *sin = (struct sockaddr_in *)&sa;
        int                     len = sizeof(sa);
        struct hostent          *hp;
        char                    *host, *dp, buf[BUFSIZ], *rfc931_name();
        int                     connected = 1;

        if (sep->se_family != AF_INET)
                return;

        if (getpeername(ctrl, &sa, &len) < 0) {
                if (errno != ENOTCONN) {
                        syslog(LOG_ERR, "getpeername: %m");
                        return;
                }
                if (recvfrom(ctrl, buf, sizeof(buf), MSG_PEEK, &sa, &len) < 0) {                        syslog(LOG_ERR, "recvfrom: %m");
                        return;
                }
                connected = 0;
        }
        if (sa.sa_family != AF_INET) {
                syslog(LOG_ERR, "unexpected address family %u", sa.sa_family);
                return;
        }

        hp = gethostbyaddr((char *) &sin->sin_addr.s_addr,
                                sizeof (sin->sin_addr.s_addr), AF_INET);

        host = hp?hp->h_name:inet_ntoa(sin->sin_addr);

        switch (sep->se_log & ~MULOG_RFC931) {
        case 0:
                return;
        case 1:
                if (curdom == NULL || *curdom == '\0')
                        break;
                dp = host + strlen(host) - strlen(curdom);
                if (dp < host)
                        break;
                if (debug)
                        fprintf(stderr, "check \"%s\" against curdom \"%s\"\n",
                                        host, curdom);
                if (strcasecmp(dp, curdom) == 0)
                        return;
                break;
        case 2:
        default:
                break;
        }

        openlog("", LOG_NOWAIT, MULOG);

        if (connected && (sep->se_log & MULOG_RFC931))
                syslog(LOG_INFO, "%s@%s wants %s",
                                rfc931_name(sin, ctrl), host, sep->se_service);
        else
                syslog(LOG_INFO, "%s wants %s",
                                host, sep->se_service);
}

/*
 * From tcp_log by
 *  Wietse Venema, Eindhoven University of Technology, The Netherlands.
 */
#if 0
static char sccsid[] = "@(#) rfc931.c 1.3 92/08/31 22:54:46";
#endif

#include <setjmp.h>

#define RFC931_PORT     113             /* Semi-well-known port */
#define TIMEOUT         4
#define TIMEOUT2        10

static sigjmp_buf timebuf;

/* timeout - handle timeouts */

static void timeout(sig)
int     sig;
{
        siglongjmp(timebuf, sig);
}

/* rfc931_name - return remote user name */

char *
rfc931_name(struct sockaddr_in *there, int ctrl)
{
        /* "there" is remote link information */
        struct sockaddr_in here;        /* local link information */
        struct sockaddr_in sin;         /* for talking to RFC931 daemon */
        int             length;
        int             s;
        unsigned        remote;
        unsigned        local;
        static char     user[256];              /* XXX */
        char            buf[256];
        char            *cp;
        char            *result = "USER_UNKNOWN";
        int             len;

        /* Find out local port number of our stdin. */

        length = sizeof(here);
        if (getsockname(ctrl, (struct sockaddr *) &here, &length) == -1) {
                syslog(LOG_ERR, "getsockname: %m");
                return (result);
        }
        /* Set up timer so we won't get stuck. */

        if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                syslog(LOG_ERR, "socket: %m");
                return (result);
        }

        sin = here;
        sin.sin_port = htons(0);
        if (bind(s, (struct sockaddr *) &sin, sizeof(sin)) == -1) {
                syslog(LOG_ERR, "bind: %m");
                close(s);
                return (result);
        }

        signal(SIGALRM, timeout);
        if (sigsetjmp(timebuf)) {
                close(s);                       /* not: fclose(fp) */
                return (result);
        }
        alarm(TIMEOUT);

        /* Connect to the RFC931 daemon. */

        sin = *there;
        sin.sin_port = htons(RFC931_PORT);
        if (connect(s, (struct sockaddr *) &sin, sizeof(sin)) == -1) {
                close(s);
                alarm(0);
                return (result);
        }

        /* Query the RFC 931 server. Would 13-byte writes ever be broken up? */
        snprintf(buf, sizeof(buf), "%u,%u\r\n",
                 ntohs(there->sin_port), ntohs(here.sin_port));


        for (len = 0, cp = buf; len < strlen(buf); ) {
                int     n;
                if ((n = write(s, cp, strlen(buf) - len)) == -1) {
                        close(s);
                        alarm(0);
                        return (result);
                }
                cp += n;
                len += n;
        }

        /* Read response */
        for (cp = buf; cp < buf + sizeof(buf) - 1; ) {
                char    c;
                if (read(s, &c, 1) != 1) {
                        close(s);
                        alarm(0);
                        return (result);
                }
                if (c == '\n')
                        break;
                *cp++ = c;
        }
        *cp = '\0';

        if (sscanf(buf, "%u , %u : USERID :%*[^:]:%255s", &remote, &local, user) == 3
                && ntohs(there->sin_port) == remote
                && ntohs(here.sin_port) == local) {

                /* Strip trailing carriage return. */
                if (cp = strchr(user, '\r'))
                        *cp = 0;
                result = user;
        }

        alarm(0);
        close(s);
        return (result);
}
#endif
