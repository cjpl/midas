/********************************************************************\

  Name:         MSYSTEM.H
  Created by:   Stefan Ritt

  Contents:     Function declarations and constants for internal
                routines

  $Log$
  Revision 1.7  1999/02/09 14:38:54  midas
  Added debug logging facility

  Revision 1.6  1999/01/21 23:01:13  pierre
  - Change ss_create_thread to ss_thread_create.
  - Include VX_TASK_SPAWN struct for ss_thread_create.
  - Adjust dm_async_area_read arg for ss_thread_create.

  Revision 1.5  1999/01/20 08:55:43  midas
  - Renames ss_xxx_mutex to ss_mutex_xxx
  - Added timout flag to ss_mutex_wait_for

  Revision 1.4  1999/01/18 17:28:47  pierre
  - Added prototypes for dm_()

  Revision 1.3  1998/10/27 10:54:03  midas
  Added ss_shell()

  Revision 1.2  1998/10/12 12:19:01  midas
  Added Log tag in header


\********************************************************************/

#include "midasinc.h"

/*------------------------------------------------------------------*/

/* data representations */
#define DRI_16              (1<<0)
#define DRI_32              (1<<1)
#define DRI_64              (1<<2)
#define DRI_LITTLE_ENDIAN   (1<<3)
#define DRI_BIG_ENDIAN      (1<<4)
#define DRF_IEEE            (1<<5)
#define DRF_G_FLOAT         (1<<6)

/*------------------------------------------------------------------*/

/* Byte and Word swapping big endian <-> little endian */
#define WORD_SWAP(x) { BYTE _tmp;                               \
                       _tmp= *((BYTE *)(x));                    \
                       *((BYTE *)(x)) = *(((BYTE *)(x))+1);     \
                       *(((BYTE *)(x))+1) = _tmp; }

#define DWORD_SWAP(x) { BYTE _tmp;                              \
                       _tmp= *((BYTE *)(x));                    \
                       *((BYTE *)(x)) = *(((BYTE *)(x))+3);     \
                       *(((BYTE *)(x))+3) = _tmp;               \
                       _tmp= *(((BYTE *)(x))+1);                \
                       *(((BYTE *)(x))+1) = *(((BYTE *)(x))+2); \
                       *(((BYTE *)(x))+2) = _tmp; }

#define QWORD_SWAP(x) { BYTE _tmp;                              \
                       _tmp= *((BYTE *)(x));                    \
                       *((BYTE *)(x)) = *(((BYTE *)(x))+7);     \
                       *(((BYTE *)(x))+7) = _tmp;               \
                       _tmp= *(((BYTE *)(x))+1);                \
                       *(((BYTE *)(x))+1) = *(((BYTE *)(x))+6); \
                       *(((BYTE *)(x))+6) = _tmp;               \
                       _tmp= *(((BYTE *)(x))+2);                \
                       *(((BYTE *)(x))+2) = *(((BYTE *)(x))+5); \
                       *(((BYTE *)(x))+5) = _tmp;               \
                       _tmp= *(((BYTE *)(x))+3);                \
                       *(((BYTE *)(x))+3) = *(((BYTE *)(x))+4); \
                       *(((BYTE *)(x))+4) = _tmp; }

/*------------------------------------------------------------------*/

/* Definition of implementation specific constants */

#define MESSAGE_BUFFER_SIZE    100000   /* buffer used for messages */
#define MESSAGE_BUFFER_NAME    "SYSMSG" /* buffer name for messages */
#define MAX_RPC_CONNECTION     10    /* server/client connections   */
#define MAX_STRING_LENGTH      256     /* max string length for odb */
#define NET_BUFFER_SIZE        (ALIGN(MAX_EVENT_SIZE)+sizeof(EVENT_HEADER)+\
4*8 + sizeof(NET_COMMAND_HEADER))

/*------------------------------------------------------------------*/

/* flag for conditional compilation of debug messages */
#undef  DEBUG_MSG

/* flag for local routines (not for pure network clients) */
#if !defined ( OS_MSDOS ) && !defined ( OS_VXWORKS )
#define LOCAL_ROUTINES
#endif

/* YBOS support not in MSDOS */
#if !defined ( OS_MSDOS )
#define YBOS_SUPPORT
#endif

/*------------------------------------------------------------------*/

/* Mapping of function names for socket operations */

#ifdef OS_MSDOS

#define closesocket(s) close(s)
#define ioctlsocket(s,c,d) ioctl(s,c,d)
#define malloc(i) farmalloc(i)

#undef NET_TCP_SIZE
#define NET_TCP_SIZE 0x7FFF

#undef MAX_EVENT_SIZE
#define MAX_EVENT_SIZE 4096

#endif /* OS_MSDOS */

#ifdef OS_VMS

#define closesocket(s) close(s)
#define ioctlsocket(s,c,d)

#ifndef FD_SET
typedef struct {
    INT fds_bits;
} fd_set;

#define FD_SET(n, p)    ((p)->fds_bits |= (1 << (n)))
#define FD_CLR(n, p)    ((p)->fds_bits &= ~(1 << (n)))
#define FD_ISSET(n, p)  ((p)->fds_bits & (1 << (n)))
#define FD_ZERO(p)      ((p)->fds_bits = 0)
#endif /* FD_SET */

#endif /* OS_VMS */

/* Missing #defines in VMS */

#ifdef OS_VMS

#define P_WAIT   0
#define P_NOWAIT 1
#define P_DETACH 4

#endif

/* and for UNIX */

#ifdef OS_UNIX

#define closesocket(s) close(s)
#define ioctlsocket(s,c,d) ioctl(s,c,d)

#define P_WAIT   0
#define P_NOWAIT 1
#define P_DETACH 4

extern char **environ;

#endif

#ifndef FD_SETSIZE
#define FD_SETSIZE 32
#endif

/* and VXWORKS */

#ifdef OS_VXWORKS

#define P_NOWAIT 1
#define closesocket(s) close(s)
#define ioctlsocket(s,c,d) ioctl(s,c,d)

#undef MAX_EVENT_SIZE
#define MAX_EVENT_SIZE 4096

#endif

/* missing O_BINARY for non-PC */
#ifndef O_BINARY
#define O_BINARY 0
#define O_TEXT   0
#endif

/* min/max/abs macros */
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#ifndef abs
#define abs(a)              (((a) < 0)   ? -(a) : (a))
#endif

/* missing tell() for some operating systems */
#define TELL(fh) lseek(fh, 0, SEEK_CUR)

/* reduced shared memory size */
#ifdef OS_SOLARIS
#define MAX_SHM_SIZE      0x20000 /* 128k */
#endif

/*------------------------------------------------------------------*/

/* Network structures */

typedef struct {
  DWORD              routine_id;    /* routine ID like ID_BM_xxx    */
  DWORD              param_size;    /* size in Bytes of parameter   */
} NET_COMMAND_HEADER;

typedef struct {
  NET_COMMAND_HEADER header;
  char               param[32];     /* parameter array              */
} NET_COMMAND;


typedef struct {
  DWORD              serial_number;
  DWORD              sequence_number;
} UDP_HEADER;

#define UDP_FIRST 0x80000000l
#define TCP_FAST  0x80000000l

#define MSG_BM       1
#define MSG_ODB      2
#define MSG_CLIENT   3
#define MSG_SERVER   4
#define MSG_LISTEN   5
#define MSG_WATCHDOG 6

/* RPC structures */

struct callback_addr {
  char  host_name[HOST_NAME_LENGTH];
  short host_port1;
  short host_port2;
  short host_port3;
  BOOL  debug;
  char  experiment[NAME_LENGTH];
  char  directory[MAX_STRING_LENGTH];
  char  user[NAME_LENGTH];
  INT   index;
};

typedef struct {                
  char host_name[HOST_NAME_LENGTH];  /*  server name        */
  INT  port;                    /*  ip port                 */
  char exp_name[NAME_LENGTH];   /*  experiment to connect   */
  int  send_sock;               /*  tcp send socket         */
  INT  remote_hw_type;          /*  remote hardware type    */
  INT  transport;               /*  RPC_TCP/RPC_FTCP        */
  INT  rpc_timeout;             /*  in milliseconds         */

} RPC_CLIENT_CONNECTION;

typedef struct {                
  char host_name[HOST_NAME_LENGTH];  /*  server name        */
  INT  port;                    /*  ip port                 */
  char exp_name[NAME_LENGTH];   /*  experiment to connect   */
  int  send_sock;               /*  tcp send socket         */
  int  recv_sock;               /*  tcp receive socket      */
  int  event_sock;              /*  event socket            */
  INT  remote_hw_type;          /*  remote hardware type    */
  INT  transport;               /*  RPC_TCP/RPC_FTCP        */
  INT  rpc_timeout;             /*  in milliseconds         */

} RPC_SERVER_CONNECTION;

typedef struct {
  INT  tid;                     /*  thread id               */
  char prog_name[NAME_LENGTH];  /*  client program name     */
  char host_name[HOST_NAME_LENGTH];  /*  client name        */
  int  send_sock;               /*  tcp send socket         */
  int  recv_sock;               /*  tcp receive socket      */
  int  event_sock;              /*  tcp event socket        */
  INT  remote_hw_type;          /*  hardware type           */
  INT  transport;               /*  RPC_TCP/RPC_FTCP        */
  INT  watchdog_timeout;        /*  in milliseconds         */
  DWORD last_activity;          /*  time of last recv       */
  INT  convert_flags;           /*  convertion flags        */
  char *net_buffer;             /*  TCP cache buffer        */
  char *ev_net_buffer;
  INT  net_buffer_size;         /*  size of TCP cache       */
  INT  write_ptr, read_ptr, misalign; /* pointers for cache */
  INT  ev_write_ptr, ev_read_ptr, ev_misalign;
  HNDLE odb_handle;             /*  handle to online datab. */
  HNDLE client_handle;          /*  client key handle .     */

} RPC_SERVER_ACCEPTION;

/*---- Online Database structures ----*/

typedef struct {
  INT           size;                 /* size in bytes              */
  INT           next_free;            /* Address of next free block */
} FREE_DESCRIP;

typedef struct {
  INT           handle;               /* Handle of record base key  */
  WORD          access_mode;          /* R/W flags                  */
  WORD          flags;                /* Data format, ...           */

} OPEN_RECORD;

typedef struct {
  char          name[NAME_LENGTH];    /* name of client             */
  INT           pid;                  /* process ID                 */
  INT           tid;                  /* thread ID                  */
  INT           thandle;              /* thread handle              */
  INT           port;                 /* UDP port for wake up       */
  INT           num_open_records;     /* number of open records     */
  INT           last_activity;        /* time of last activity      */
  INT           watchdog_timeout;     /* timeout in ms              */
  INT           max_index;            /* index of last opren record */

  OPEN_RECORD   open_record[MAX_OPEN_RECORDS];

} DATABASE_CLIENT;

typedef struct {
  char          name[NAME_LENGTH];    /* name of database           */
  INT           num_clients;          /* no of active clients       */
  INT           max_client_index;     /* index of last client       */
  INT           key_size;             /* size of key area in bytes  */
  INT           data_size;            /* size of data area in bytes */
  INT           root_key;             /* root key offset            */
  INT           first_free_key;       /* first free key memory      */
  INT           first_free_data;      /* first free data memory     */

  DATABASE_CLIENT client[MAX_CLIENTS];/* entries for clients        */

} DATABASE_HEADER;

/* Per-process buffer access structure (descriptor) */

typedef struct {
  char          name[NAME_LENGTH];  /* Name of database             */
  BOOL          attached;           /* TRUE if database is attached */
  INT           client_index;       /* index to CLIENT str. in buf. */
  DATABASE_HEADER *database_header; /* pointer to database header   */
  void          *database_data;     /* pointer to database data     */
  HNDLE         mutex;              /* mutex/semaphore handle       */
  INT           lock_cnt;           /* flag to avoid multiple locks */
  HNDLE         shm_handle;         /* handle (id) to shared memory */
  INT           index;              /* connection index / tid       */

} DATABASE;

/* Open record descriptor */

typedef struct {
  HNDLE         handle;               /* Handle of record base key  */
  HNDLE         hDB;                  /* Handle of record's database*/
  WORD          access_mode;          /* R/W flags                  */
  void          *data;                /* Pointer to local data      */
  void          *copy;                /* Pointer of copy to data    */
  INT           buf_size;             /* Record size in bytes       */
  void          (*dispatcher)(INT,INT,void*);  /* Pointer to dispatcher func.*/
  void          *info;                /* addtl. info for dispatcher */

} RECORD_LIST;

/* Event request descriptor */

typedef struct {
  INT           buffer_handle;                      /* Buffer handle */
  short int     event_id;                 /* same as in EVENT_HEADER */
  short int     trigger_mask;
  void          (*dispatcher)(HNDLE,HNDLE,EVENT_HEADER*,void*); /* Dispatcher func.*/

} REQUEST_LIST;

/*---- Logging channel information ---------------------------------*/

typedef struct {
  BOOL      active;
  char      type[8];
  char      filename[256];
  char      format[8];
  BOOL      odb_dump;
  DWORD     log_messages;
  char      buffer[32];
  INT       event_id;
  INT       trigger_mask;
  DWORD     event_limit;
  double    byte_limit;
  double    tape_capacity;
} CHN_SETTINGS;

typedef struct {
  double    events_written;
  double    bytes_written;
  double    bytes_written_total;
  INT       files_written;
} CHN_STATISTICS;

typedef struct {
  INT       handle;           
  char      path[256];        
  INT       type;             
  INT       format;           
  INT       buffer_handle;    
  INT       msg_buffer_handle;
  INT       request_id;
  INT       msg_request_id;
  HNDLE     stats_hkey;
  HNDLE     settings_hkey;
  CHN_SETTINGS settings;
  CHN_STATISTICS statistics;
  void*     *format_info;
  void*     ftp_con;
} LOG_CHN;

#define LOG_TYPE_DISK 1
#define LOG_TYPE_TAPE 2
#define LOG_TYPE_FTP  3


/*---- VxWorks specific taskSpawn arguments ----------------------*/
typedef struct {
char    name[32];
int     priority;
int     options;
int     stackSize;
int     arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10;
} VX_TASK_SPAWN;
 


/*---- Channels for ss_suspend_set_dispatch() ----------------------*/
#define CH_IPC        1
#define CH_CLIENT     2
#define CH_SERVER     3
#define CH_LISTEN     4

/*---- Function declarations ---------------------------------------*/

/*---- common function ----*/
INT EXPRT cm_set_path(char *path);
INT EXPRT cm_get_path(char *path);
INT cm_dispatch_ipc(INT msg, INT p1, INT p2, int socket);
INT EXPRT cm_msg_log(INT message_type, char *message);
void EXPRT name2c(char *str);

/*---- buffer manager ----*/
INT bm_lock_buffer(INT buffer_handle);
INT bm_unlock_buffer(INT buffer_handle);
INT bm_notify_client(INT buffer_handle, INT request_id, int socket);
INT EXPRT bm_mark_read_waiting(BOOL flag);
INT bm_push_event(INT buffer_handle);
INT bm_check_buffers(void);
INT EXPRT bm_remove_event_request(INT buffer_handle, INT request_id);

/*---- online database ----*/
INT db_lock_database(HNDLE database_handle);
INT db_unlock_database(HNDLE database_handle);
INT db_update_record(INT hDB, INT hKey, int socket);
INT db_close_all_records(void);
INT EXPRT db_flush_database(HNDLE hDB);
INT db_notify_clients(HNDLE hDB, HNDLE hKey, BOOL bWalk);
INT EXPRT db_set_client_name(HNDLE hDB, char *client_name);
INT db_delete_key1(HNDLE hDB, HNDLE hKey, INT level, BOOL follow_links);
INT EXPRT db_show_mem(HNDLE hDB, char *result, INT buf_size);

/*---- rpc functions -----*/
RPC_LIST EXPRT *rpc_get_internal_list(INT flag);
INT rpc_server_receive(INT index, int sock, BOOL check);
INT rpc_server_callback(struct callback_addr *callback);
INT EXPRT rpc_server_accept(int sock);
INT rpc_client_accept(int sock);
INT rpc_get_server_acception(void);
INT rpc_set_server_acception(INT index);
INT EXPRT rpc_set_server_option(INT item, PTYPE value);
PTYPE EXPRT rpc_get_server_option(INT item);
INT recv_tcp_check(int sock);
INT recv_event_check(int sock);
INT rpc_deregister_functions(void);
INT rpc_check_channels(void);
INT rpc_server_disconnect(void);
int EXPRT rpc_get_send_sock(void);
int EXPRT rpc_get_event_sock(void);
INT EXPRT rpc_set_opt_tcp_size(INT tcp_size);
INT EXPRT rpc_get_opt_tcp_size(void);

/*---- system services ----*/
INT ss_open_shm(char *name, INT size, void **adr, HNDLE *handle);
INT ss_close_shm(char *name, void *adr, HNDLE handle, INT destroy_flag);
INT ss_flush_shm(char *name, void *adr, INT size);
INT ss_spawnv(INT mode, char *cmdname, char *argv[]);
INT ss_shell(int sock);
INT ss_getpid(void);
INT ss_gettid(void);
INT ss_getthandle(void);
INT ss_set_async_flag(INT flag);
INT ss_mutex_create(char *mutex_name, HNDLE *mutex_handle);
INT ss_mutex_wait_for(HNDLE mutex_handle, INT timeout);
INT ss_mutex_release(HNDLE mutex_handle);
INT ss_mutex_delete(HNDLE mutex_handle, INT destroy_flag);
INT ss_wake(INT pid, INT tid, INT thandle);
INT ss_alarm(INT millitime, void (*func)(int));
INT ss_suspend_get_port(INT* port);
INT ss_suspend_set_dispatch(INT channel, void *connection, INT (*dispatch)());
INT ss_resume(INT port, INT msg, INT param1, INT param2);
INT ss_suspend_exit(void);
INT ss_exception_handler(void (*func)());
INT EXPRT ss_suspend(INT millisec, INT msg);
INT EXPRT ss_thread_create(INT (*func)(void *), void *param);
INT EXPRT ss_get_struct_align(void);

/*---- socket routines ----*/
INT EXPRT send_tcp(int sock, char *buffer, DWORD buffer_size, INT flags);
INT recv_tcp(int sock, char *buffer, DWORD buffer_size, INT flags);
INT send_udp(int sock, char *buffer, DWORD buffer_size, INT flags);
INT recv_udp(int sock, char *buffer, DWORD buffer_size, INT flags);
INT recv_string(int sock, char *buffer, DWORD buffer_size, INT flags);

/*---- system logging ----*/
INT EXPRT ss_syslog(const char *message);

/*---- event buffer routines ----*/
INT  EXPRT eb_create_buffer(INT size);
INT  EXPRT eb_free_buffer(void);
BOOL EXPRT eb_buffer_full(void);
BOOL EXPRT eb_buffer_empty(void);
EVENT_HEADER EXPRT *eb_get_pointer(void);
INT  EXPRT eb_increment_pointer(INT buffer_handle, INT event_size);
INT  EXPRT eb_send_events(BOOL send_all);

/*---- dual memory event buffer routines ----*/
INT  EXPRT dm_buffer_create(INT size);
INT  EXPRT dm_buffer_release(void);
BOOL EXPRT dm_area_full(void);
EVENT_HEADER EXPRT *dm_pointer_get(void);
INT  EXPRT dm_pointer_increment(INT buffer_handle, INT event_size);
INT  EXPRT dm_area_send(void);
INT  EXPRT dm_area_flush(void);
INT  EXPRT dm_async_area_send(void *pointer);
/*---- Include RPC identifiers -------------------------------------*/

#include "mrpc.h"
