/*
 * Transaction Program Table Definitions
 */

#define MAXARGV 20

struct servtab;

struct  servtab {
        char    *se_service;            /* name of service */
	int	se_cnvtype;		/* type of conversation */
	int	se_synclevel;		/* Sync level supported */
	short   se_wait;                /* single threaded server */
	char    *se_user;               /* user name to run as */
        char    *se_group;              /* group name to run as */
        char    *se_server;             /* server program */
        char    *se_argv[MAXARGV+1];    /* program arguments */
        int     se_fd;                  /* open descriptor */

        char    *se_address;            /* address to bind to (for printing) */
        int     se_socktype;            /* type of socket to use */
        int     se_family;              /* address family */
        char    *se_proto;              /* protocol used */
        int     se_rpcprog;             /* rpc program number */
        int     se_rpcversl;            /* rpc program lowest version */
        int     se_rpcversh;            /* rpc program highest version */
        short   se_checked;             /* looked at during merge */
        union {
                struct  sockaddr se_un_ctrladdr;
                struct  sockaddr_in se_un_ctrladdr_in;
                struct  sockaddr_un se_un_ctrladdr_un;
        } se_un;                        /* bound address */


        int     se_ctrladdr_size;
        int     se_max;                 /* max # of instances of this service */        int     se_count;               /* number started since se_time */
        struct  timeval se_time;        /* start of se_count */
#ifdef MULOG
        int     se_log;
#define MULOG_RFC931    0x40000000
#endif
        struct  servtab *se_next;
};

#define se_ctrladdr     se_un.se_un_ctrladdr
#define se_ctrladdr_in  se_un.se_un_ctrladdr_in
#define se_ctrladdr_un  se_un.se_un_ctrladdr_un

extern struct servtab *servtab;

#define isrpcservice(sep)	((sep)->se_rpcversl != 0)

struct servtab *find_service_by_fd(int fd);
struct servtab *find_service_by_pid(pid_t pid);
struct servtab *find_service_by_name(char *name, int len);
void restart_services(void);
const char *service_name(struct servtab *sep);

struct servtab *enter(struct servtab *);
void register_rpc(struct servtab *sep);
void unregister_rpc(struct servtab *sep);
void setup(struct servtab *sep);
void closeit(struct servtab *sep);

