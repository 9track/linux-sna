/* snatchd.c: Linux-SNA attach daemon/manger. 
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

#include <linux/attach.h>
#include "attach.h"

#include "snatch_list.h"
#include "snatchd.h"

char    version_s[]             = VERSION;
char    name_s[]                = "snatchd";
char    desc_s[]                = "Linux-SNA attach daemon";
char    maintainer_s[]          = "Jay Schulist <jschlst@linux-sna.org>";
char    company_s[]             = "linux-SNA.ORG";
char    web_s[]                 = "http://www.linux-sna.org";
int     snatch_debug_level    	= 1;
list_head(tp_list);

static sigset_t blockmask, emptymask;
static int blocked = 0;
static int afd = 0;

extern void sig_block(void);
extern void sig_unblock(void);
extern void sig_preexec(void);

static local_tp_info *snatch_tp_get_by_name(char *name)
{
	struct list_head *le;
	local_tp_info *tp;

	list_for_each(le, &tp_list) {
		tp = list_entry(le, local_tp_info, list);
		if (!strcmp(tp->name, name))
			return tp;
	}
	return NULL;
}

static local_tp_info *snatch_tp_get_by_pid(pid_t pid)
{
        struct list_head *le;
        local_tp_info *tp;

        list_for_each(le, &tp_list) {
                tp = list_entry(le, local_tp_info, list);
		if (tp->pid == pid)
                        return tp;
        }
        return NULL;
}

static void snatch_exec_tp(local_tp_info *l_tp)
{
        struct passwd *pwd;
        struct group *grp = NULL;
        uid_t uid;
        gid_t gid;

	/* prepare to exec. */
        pwd = getpwnam(l_tp->user);
        if (pwd == NULL) {
		snatch_debug(5, "tp `%s': no such user `%s'.\n", 
			l_tp->name, l_tp->user);
                return;
        }

        /* use the uid and gid of the user. */
        uid = pwd->pw_uid;
        gid = pwd->pw_gid;

        /* if a nonroot user, do initgroups to run with that user's group
         * list. but if root, do not use root's group list - just use the one
         * gid.
         *
         * if no group was specified, keep the group inetd was run under.
         * This is the traditional behavior, but seems dumb - shouldn't
         * we use the group from the password file?
         */
        if (uid) {
                setgid(gid);
                initgroups(pwd->pw_name, gid);
                setuid(uid);
        } else {
        	if (grp) {
                	setgid(gid);
                	setgroups(1, &gid);
        	}
	}
        sig_preexec();
#ifdef ARGS
        execv(l_tp->path, l_tp->args);
#else
	execl(l_tp->path, NULL);
#endif
	snatch_debug(5, "exec `%s'.\n", l_tp->name);
	return;
}

static pid_t snatch_fork_tp(local_tp_info *l_tp)
{
        pid_t pid;

        if (l_tp->count++ == 0)
                gettimeofday(&l_tp->time, NULL);
        else { 
		if (l_tp->count >= l_tp->limit) {
                	struct timeval now;

                	gettimeofday(&now, (struct timezone *)0);
                	if (now.tv_sec - l_tp->time.tv_sec > CNT_INTVL) {
                	        l_tp->time  = now;
                	        l_tp->count = 1;
                	} else {
                        	snatch_debug(5, "%s server failing (looping or "
                        	       "being flooded), service terminated for "
                        	       "%d min.\n", l_tp->name, RETRYTIME/60);

                        	l_tp->count = 0;
                        	return -1;
                	}
        	}
	}
        pid = fork();
        if (pid < 0) {
		snatch_debug(5, "foke: %m\n");
        }
        return pid;
}

static void snatch_launch(local_tp_info *l_tp, struct tp_attach *tp)
{
	pid_t pid;
	int err;
	
	snatch_debug(5, "launching: %s\n", l_tp->name);
        pid = snatch_fork_tp(l_tp);
        if (pid < 0) {
                sleep(1);
                return;
	}
        if (pid == 0) {
                snatch_exec_tp(l_tp);
                _exit(1);
	}
        err = tp_correlate(afd, pid, tp->tcb_id, tp->tp_name);
	if (err < 0)
		snatch_debug(5, "correlation failed for %s:%d:%ld\n",
			l_tp->name, pid, tp->tcb_id);
	else
		snatch_debug(5, "successful correlation for %s:%d:%ld\n",
	                        l_tp->name, pid, tp->tcb_id);
	return;
}

static int snatch_setup_tp(local_tp_info *l_tp)
{
	struct tp_info *tp;
	int err;

	new(tp);
	if (!tp)
		return -ENOMEM;
	strcpy(tp->tp_name, l_tp->name);
	err = tp_register(afd, tp);
	if (err < 0) {
		snatch_debug(5, "tp %s register failed `%d'.\n",
			l_tp->name, err);
		return err;
	}
	l_tp->fd = err;
	return 0;
}

static int snatch_load_tp_list(struct list_head *list)
{
	local_tp_info *l_tp;
	struct list_head *le;

	list_for_each(le, list) {
		l_tp = list_entry(le, local_tp_info, list);
		snatch_setup_tp(l_tp);
	}
	return 0;
}

static int snatch_director(void)
{
	struct tp_attach r_tp;
	local_tp_info *l_tp;
	gid_t gid;
	int err;
	
        gid = getgid();
        setgroups(1, &gid);

	/* open the communications path to the sna stack. */
        afd = attach_open();
	if (afd < 0) {
		snatch_debug(5, "open failed `%d'.\n", afd);
		return err;
	}
	err = snatch_load_tp_list(&tp_list);
	if (err < 0)
		snatch_debug(5, "one or more tp failed to load `%d'.\n", err);
        for (;;) {
		memset(&r_tp, 0, sizeof(struct tp_attach));
		err = attach_listen(afd, &r_tp, sizeof(struct tp_attach), 0);
		if (err < 0) {
			snatch_debug(5, "listen failed `%d'.\n", err);
			sleep(1);
			continue;
		}
		l_tp = snatch_tp_get_by_name(r_tp.tp_name);
		if (!l_tp) {
			snatch_debug(5, "unable to locate transaction program `%s'.\n",
				r_tp.tp_name);
			continue;
		}
                snatch_launch(l_tp, &r_tp);
        }
        return 0;
}

void snatch_goaway(int signum)
{
	local_tp_info *l_tp;
	struct list_head *le, *se;

        (void)signum;
	list_for_each_safe(le, se, &tp_list) {
		l_tp = list_entry(le, local_tp_info, list);
		tp_unregister(afd, l_tp->fd);
		list_del(&l_tp->list);
		free(l_tp);
	}

        (void)unlink(_PATH_SNATCHD_PID);
        attach_close(afd);
        exit(0);
}

void snatch_reap_tp(int signum)
{
	local_tp_info *l_tp;
        int status;
        pid_t pid;
        char *name, tmp[64];

        (void)signum;
        while ((pid = wait3(&status, WNOHANG, NULL)) > 0) {
		snatch_debug(5, "pid %d, exit status %d\n", pid, status);
		l_tp = snatch_tp_get_by_pid(pid);
		if (!l_tp)
                        snprintf(tmp, sizeof(tmp), "pid %d", (int)pid);
                else
                        snprintf(tmp, sizeof(tmp), "%s (pid %d)",
                                 l_tp->name, (int)pid);
		name = tmp;

                if (WIFEXITED(status) && WEXITSTATUS(status)) {
                        snatch_debug(5, "%s: exit status %d", name,
                               WEXITSTATUS(status));
                } else {
                	if (WIFSIGNALED(status)) {
                        	snatch_debug(5, "%s: exit signal %d", name,
                               		WTERMSIG(status));
                	}
		}
        }
	return;
}

void snatch_config(int signum)
{
	(void)signum;
	return;
}

void snatch_retry(int signum)
{
        (void)signum;
        return;
}

void sig_block(void)
{
        sigprocmask(SIG_BLOCK, &blockmask, NULL);
        if (blocked) {
            syslog(LOG_ERR, "internal error - signals already blocked\n");
            syslog(LOG_ERR, "please report to jschlst@linux-sna.org\n");
        }
        blocked = 1;
	return;
}

void sig_unblock(void)
{
        sigprocmask(SIG_SETMASK, &emptymask, NULL);
        blocked = 0;
	return;
}

void sig_wait(void)
{
        sigsuspend(&emptymask);
	return;
}

void sig_preexec(void)
{
        struct sigaction sa;

        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = SIG_DFL;
        sigaction(SIGPIPE, &sa, NULL);
        sig_unblock();
	return;
}

void sig_init(void)
{
        struct sigaction sa;

        sigemptyset(&emptymask);
        sigemptyset(&blockmask);
        sigaddset(&blockmask, SIGCHLD);
        sigaddset(&blockmask, SIGHUP);
        sigaddset(&blockmask, SIGALRM);

        memset(&sa, 0, sizeof(sa));
        sa.sa_mask = blockmask;
        sa.sa_handler = snatch_retry;
        sigaction(SIGALRM, &sa, NULL);
        sa.sa_handler = snatch_config;
        sigaction(SIGHUP, &sa, NULL);
        sa.sa_handler = snatch_reap_tp;
        sigaction(SIGCHLD, &sa, NULL);
        sa.sa_handler = snatch_goaway;
        sigaction(SIGTERM, &sa, NULL);
        sa.sa_handler = snatch_goaway;
        sigaction(SIGINT, &sa,  NULL);
        sa.sa_handler = SIG_IGN;
        sigaction(SIGPIPE, &sa, NULL);
	return;
}

static void logpid(char *path)
{
        FILE *fp;

        if ((fp = fopen(path, "w")) != NULL) {
                fprintf(fp, "%u\n", getpid());
                (void)fclose(fp);
        }
	return;
}

static int help(void)
{
	printf("Usage: %s [-h] [-v] [-d level] [-f config_file]\n", name_s);
	exit(1);
}

static int version(void)
{
        printf("%s: %s %s\n%s %s\n%s\n", name_s, desc_s, version_s,
                company_s, maintainer_s, web_s);
        exit(1);
}

int main(int argc, char *argv[], char *envp[])
{
	char config_file[_PATH_SNATCHD_CONF_MAX];
        int nodaemon = 0, err, c;

	strcpy(config_file, _PATH_SNATCHD_CONF);
	while ((c = getopt(argc, argv, "hvVf:d:")) != EOF) {
                switch (c) {
                	case 'd':
                        	snatch_debug_level = nodaemon = atoi(optarg);
                        	break;
			case 'f':       /* Configuration file. */
                                strcpy(config_file, optarg);
                                break;
                        case 'V':       /* Display author and version. */
                        case 'v':       /* Display author and version. */
                                version();
                                break;
                	case 'h':
                	default:
				help();
				break;
                }
	}

	/* read configuration information. */
	err = snatch_read_config_file(config_file);
        if (err < 0) {
                snatch_debug(5, "configuration file (%s) invalid `%d'.\n",
                        config_file, err);
                return err;
        }
        if (snatch_debug_level >= 10)
                snatch_print_config(&tp_list);
        if (list_empty(&tp_list)) {
                snatch_debug(5, "no transaction programs configured, exiting\n");
                return 0;
        }

	/* fork to daemon. */
        if (nodaemon == 0)
                daemon(0, 0);

	/* setup logging. */
        openlog(name_s, LOG_PID | LOG_NOWAIT, LOG_DAEMON);
	syslog(LOG_INFO, "%s %s", desc_s, version_s);
        logpid(_PATH_SNATCHD_PID);

	/* setup signal handling */
        sig_init();
	
	/* do the real work now, looping and directing. */
	err = snatch_director();
	if (err < 0)
		snatch_debug(5, "fatal director error `%d'.\n", err);
	return err;
}
