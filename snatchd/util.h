int daemon(int nochdir, int noclose);
void initsetproctitle(int argc, char **argv, char **envp);
void setproctitle(const char *fmt, ...);
