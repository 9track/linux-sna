#ifndef _LIBTNX_H
#define _LIBTNX_H

#define tnX_new(type,count) (type *)malloc (sizeof (type) * (count))


struct _clientaddr {
	unsigned long int address;
	unsigned long int mask;
};
typedef struct _clientaddr clientaddr;

typedef unsigned char tnXchar;

typedef struct {
   	const char 		*name;
   	const unsigned char 	*to_remote_map;
   	const unsigned char 	*to_local_map;
} tnXchar_map;

typedef struct {
   	unsigned char        	*data;
   	int                  	len;
  	int                  	allocated;
} tnXbuff;

#define SYSCONFDIR	"/etc"
#define CONFIG_STRING 1
#define CONFIG_LIST   2

struct _tnXconfig_str {
	char          *name;
   	 int           type;
      	gpointer      value;
};
typedef struct _tnXconfig_str tnXconfig_str;

struct _tnXconfig {
	int      		ref;
	GSList 			*vars;
};
typedef struct _tnXconfig tnXconfig;

extern int tnX_config_load(tnXconfig *This, const char *filename);
extern int tnX_config_get_int (tnXconfig *This, const char *name);
extern const char *tnX_config_get (tnXconfig *This, const char *name);
extern tnXconfig *tnX_config_new();
extern void tnX_config_unref(tnXconfig *This);
extern void tnX_config_set (tnXconfig *This, const char *name,
	const int type, const  gpointer value);
extern void tnX_config_promote (tnXconfig *This, const char *prefix);

/* These don't work like new/destroy */
extern void tnX_buffer_init(tnXbuff *buff);
extern void tnX_buffer_free(tnXbuff *buff);

#define tnX_buffer_data(This) \
	((This)->data ? (This)->data : (unsigned char *)"")
#define tnX_buffer_length(This) ((This)->len)

extern void tnX_buffer_append_byte(tnXbuff *buff, unsigned char b);
extern void tnX_buffer_append_data(tnXbuff *buff, unsigned char *data, int len);
extern void tnX_buffer_log(tnXbuff *buff, const char *prefix);

#define tnX_socket               socket
#define tnX_connect              connect
#define tnX_select               select
#define tnX_gethostbyname        gethostbyname
#define tnX_getservbyname        getservbyname
#define tnX_send                 send
#define tnX_recv                 recv
#define tnX_close                close
#define tnX_ioctl                ioctl

#define tnX_makestring(expr) #expr
void tnX_log_open(const char *fname);
void tnX_log_printf(const char *fmt,...);
void tnX_log_close(void);
void tnX_log_assert(int val, char const *expr, char const *file, int line);
#define tnX_log(args) tnX_log_printf args
#define tnX_assert(expr) \
	tnX_log_assert((expr), tnX_makestring(expr), __FILE__, __LINE__)

extern tnXchar tnX_char_map_to_local(tnXchar_map *map, tnXchar ebcdic);
extern tnXchar_map *tnX_char_map_new(const char *map);
extern void tnX_char_map_destroy(tnXchar_map *map);
extern tnXchar tnX_char_map_to_remote(tnXchar_map *map, tnXchar ascii);

struct _tnXrec {
   	struct _tnXrec *prev;
   	struct _tnXrec *next;

   	tnXbuff data;
   	int cur_pos;
};
typedef struct _tnXrec tnXrec;

#define TN5250_RECORD_FLOW_DISPLAY 0x00
#define TN5250_RECORD_FLOW_STARTUP 0x90
#define TN5250_RECORD_FLOW_SERVERO 0x11
#define TN5250_RECORD_FLOW_CLIENTO 0x12

#define TN5250_RECORD_H_NONE            0
#define TN5250_RECORD_H_ERR             0x80
#define TN5250_RECORD_H_ATN             0x40
#define TN5250_RECORD_H_PRINTER_READY   0x20
#define TN5250_RECORD_H_FIRST_OF_CHAIN  0x10
#define TN5250_RECORD_H_LAST_OF_CHAIN   0x08
#define TN5250_RECORD_H_SRQ             0x04
#define TN5250_RECORD_H_TRQ             0x02
#define TN5250_RECORD_H_HLP             0x01

#define TN5250_RECORD_OPCODE_NO_OP              0
#define TN5250_RECORD_OPCODE_INVITE             1
#define TN5250_RECORD_OPCODE_OUTPUT_ONLY        2
#define TN5250_RECORD_OPCODE_PUT_GET            3
#define TN5250_RECORD_OPCODE_SAVE_SCR           4
#define TN5250_RECORD_OPCODE_RESTORE_SCR        5
#define TN5250_RECORD_OPCODE_READ_IMMED         6
#define TN5250_RECORD_OPCODE_READ_SCR           8
#define TN5250_RECORD_OPCODE_CANCEL_INVITE      10
#define TN5250_RECORD_OPCODE_MESSAGE_ON         11
#define TN5250_RECORD_OPCODE_MESSAGE_OFF        12
#define TN5250_RECORD_OPCODE_PRINT_COMPLETE     1
#define TN5250_RECORD_OPCODE_CLEAR              2

extern tnXrec *tnX_record_new(void);
extern void tnX_record_destroy(tnXrec *This);

extern unsigned char tnX_record_get_byte(tnXrec *This);
extern void tnX_record_unget_byte(tnXrec *This);
extern int tnX_record_is_chain_end(tnXrec *This);
#define tnX_record_length(This) \
	tnX_buffer_length(&((This)->data))
#define tnX_record_append_byte(This,c) \
	tnX_buffer_append_byte(&((This)->data),(c))
#define tnX_record_data(This) \
	tnX_buffer_data(&((This)->data))
#define tnX_record_set_cur_pos(This,newpos) \
	(void)((This)->cur_pos = (newpos))
#define tnX_record_opcode(This) \
	(tnX_record_data(This)[9])
#define tnX_record_flow_type(This) \
	((tnX_record_data(This)[4] << 8) | (tnX_record_data(This)[5]))
#define tnX_record_flags(This) \
	(tnX_record_data(This)[7])
#define tnX_record_sys_request(This) \
	((tnX_record_flags((This)) & TN5250_RECORD_H_SRQ) != 0)
#define tnX_record_attention(This) \
	((tnX_record_flags((This)) & TN5250_RECORD_H_ATN) != 0)

extern tnXrec *tnX_record_list_add(tnXrec *list, tnXrec *record);
extern tnXrec *tnX_record_list_remove(tnXrec *list, tnXrec *record);
extern tnXrec *tnX_record_list_destroy(tnXrec *list);
extern void tnX_record_dump(tnXrec *This);

#define tnX_stream_connect(This,to) \
	(*(This->connect))((This),(to))
#define tnX_stream_disconnect(This) \
	(*(This->disconnect))((This))
#define tnX_stream_handle_receive(This) \
	(*(This->handle_receive))((This))
#define tnX_stream_send_packet(This,len,flow,flags,opcode,data) \
	(*(This->send_packet))((This),(len),(flow),(flags),(opcode),(data))

#define TN3270_STREAM 	0
#define TN3270E_STREAM	1
#define TN5250_STREAM 	2
	
struct _tnXstream {
   	int (*connect) 		(struct _tnXstream *This, const char *to);
  	int (*accept) 		(struct _tnXstream *This, int masterSock);
   	void (*disconnect) 	(struct _tnXstream *This);
   	int (*handle_receive) 	(struct _tnXstream *This);
   	void (*send_packet) 	(struct _tnXstream *This, int length,
		int flowtype, unsigned char flags,
         	unsigned char opcode, unsigned char *data);
   	void (*destroy) 	(struct _tnXstream *This);

	tnXconfig 		*config;

   	tnXrec 			*records;
   	tnXrec 			*current_record;
   	int 			record_count;

   	tnXbuff 		sb_buf;

   	int 			sockfd;
   	int 			status;
   	int 			state;
  	int 			streamtype;
  	long 			msec_wait;

   	FILE 			*debugfile;

	/* not sure how this is used. */
	int			options;
};
typedef struct _tnXstream tnXstream;

extern tnXrec *tnX_stream_get_record(tnXstream *This);
extern void tnX_stream_destroy(tnXstream *This);
extern tnXstream *tnX_stream_host(int masterfd, long timeout, int streamtype);

struct _tnXhost {
  	tnXstream    	*stream;
  	tnXchar_map   	*map;
	tnXrec    	*record;
  	tnXrec    	*screenRec;
  	tnXbuff    	buffer;
  	unsigned short 	curattr;
  	unsigned short 	lastattr;
  	unsigned short 	cursorPos;
  	unsigned       	wtd_set         : 1;
  	unsigned       	clearState      : 1;
  	unsigned       	zapState        : 1;
  	unsigned       	inputInhibited  : 1;
  	unsigned       	disconnected    : 1;
  	unsigned       	inSysInterrupt  : 1;
  	int            	ra_count;
  	unsigned char  	ra_char;
  	int 		maxcol;
};
typedef struct _tnXhost tnXhost;

struct _tnXstream_type {
        char *prefix;
        int (*init)(tnXstream *This);
};
typedef struct _tnXstream_type tnXstream_type;

extern GSList *tnX_build_addr_list(GSList * addrlist);
extern int tnX_valid_client(GSList * addrlist, unsigned long int client);
extern int tnX_daemon(int nochdir, int noclose, int ignsigcld);
extern int tnX_make_socket(unsigned short int port);
#endif	/* _LIBTNX_H */
