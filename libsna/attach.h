extern int tp_register(int a, struct tp_info *tp);
extern int tp_unregister(int a, int b);
extern int tp_correlate(int s, pid_t pid, unsigned long tcb_id, char *tp_name);

extern int attach_open(void);
extern int attach_listen(int s, void *buf, int len, unsigned int flags);
extern int attach_close(int s);
