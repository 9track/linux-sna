/* util.c: Utility functions for snatchd.
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

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#include <paths.h>
#include <unistd.h>

int daemon(int nochdir, int noclose)
{
        int fd;

        switch(fork())
	{
          	case -1:
                	return -1;
          	case 0:
                	break;
          	default:
                	_exit(0);
        }

        if(setsid() == -1) 
		return -1;
        if(!nochdir) 
		chdir("/");
        if(noclose) 
		return 0;

        fd = open(_PATH_DEVNULL, O_RDWR, 0);
        if(fd != -1)
	{
                dup2(fd, STDIN_FILENO);
                dup2(fd, STDOUT_FILENO);
                dup2(fd, STDERR_FILENO);
                if(fd > 2) 
			close(fd);
        }

	return (0);
}

/* setproctitle() - set process title for ps.
 * Parameters: 	fmt - a printf style format string.
 * 		a, b, c - possible parameters to fmt.
 * Side effects: Clobbers argv of our main procedure so ps(1) will
 *		 display the title.
 */

/* Pointers for setproctitle(), thie allows "ps" listings to give more
 * useful information.
 */
static char **Argv 	= NULL;              	/* pointer to argument vector */
static char *LastArgv 	= NULL;           	/* end of argv */
static char Argv0[128];                 	/* program name */

void initsetproctitle(int argc, char **argv, char **envp)
{
        register int i;
        char *tmp;

        for(i = 0; envp[i] != NULL; i++)
                continue;
        __environ = (char **) domalloc(sizeof (char *) * (i + 1));
        for(i = 0; envp[i] != NULL; i++)
                __environ[i] = (char *)dostrdup(envp[i]);
        __environ[i] = NULL;

        /* Save start and extent of argv for setproctitle. */
        Argv = argv;
        if(i > 0)
                LastArgv = envp[i - 1] + strlen(envp[i - 1]);
        else
                LastArgv = argv[argc - 1] + strlen(argv[argc - 1]);

        tmp = strrchr(argv[0], '/');
        if(!tmp) 
		tmp = argv[0];
        else 
		tmp++;
        strncpy(Argv0, tmp, sizeof(Argv0));
        /* remember to take away one or we go outside the array space */
        Argv0[sizeof(Argv0)-1] = 0;
}

void setproctitle(const char *fmt, ...)
{
        register char *p;
        register int i=0;
        static char buf[2048];
        va_list ap;

        p = buf;

        /* print progname: heading for grep */
        /* This can't overflow buf due to the relative size of Argv0. */
        (void) strcpy(p, Argv0);
        (void) strcat(p, ": ");
        p += strlen(p);

        /* print the argument string */
        va_start(ap, fmt);
        (void) vsnprintf(p, sizeof(buf) - (p - buf), fmt, ap);
        va_end(ap);

        i = strlen(buf);

        if(i > LastArgv - Argv[0] - 2)
        {
                i = LastArgv - Argv[0] - 2;
                buf[i] = '\0';
        }
        (void) strcpy(Argv[0], buf);
        p = &Argv[0][i];
        while(p < LastArgv)
                *p++ = ' ';
        Argv[1] = NULL;
}
