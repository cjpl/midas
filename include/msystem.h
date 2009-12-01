/********************************************************************\

  Name:         MSYSTEM.H
  Created by:   Stefan Ritt

  Contents:     Function declarations and constants for internal
                routines

  $Id$

\********************************************************************/

/**dox***************************************************************/
/** @file msystem.h
The Midas System include file
*/

/** @defgroup msystemincludecode The msystem.h & system.c
 */
/** @defgroup msdefineh System Define
 */
/** @defgroup msmacroh System Macros
 */
/** @defgroup mssectionh System Structure Declaration
 */

/**dox***************************************************************/
/** @addtogroup msystemincludecode
 *  
 *  @{  */

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "midasinc.h"

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/**dox***************************************************************/
/** @addtogroup msdefineh
 *  
 *  @{  */

/**
data representations
*/
#define DRI_16              (1<<0)  /**< - */
#define DRI_32              (1<<1)  /**< - */
#define DRI_64              (1<<2)  /**< - */
#define DRI_LITTLE_ENDIAN   (1<<3)  /**< - */
#define DRI_BIG_ENDIAN      (1<<4)  /**< - */
#define DRF_IEEE            (1<<5)  /**< - */
#define DRF_G_FLOAT         (1<<6)  /**< - */
#define DR_ASCII            (1<<7)  /**< - */

/**dox***************************************************************/
          /** @} *//* end of msdefineh */

/**dox***************************************************************/
/** @addtogroup msmacroh
 *  
 *  @{  */

/* Byte and Word swapping big endian <-> little endian */
/**
SWAP WORD macro */
#ifndef WORD_SWAP
#define WORD_SWAP(x) { BYTE _tmp;                               \
                       _tmp= *((BYTE *)(x));                    \
                       *((BYTE *)(x)) = *(((BYTE *)(x))+1);     \
                       *(((BYTE *)(x))+1) = _tmp; }
#endif

/**
SWAP DWORD macro */
#ifndef DWORD_SWAP
#define DWORD_SWAP(x) { BYTE _tmp;                              \
                       _tmp= *((BYTE *)(x));                    \
                       *((BYTE *)(x)) = *(((BYTE *)(x))+3);     \
                       *(((BYTE *)(x))+3) = _tmp;               \
                       _tmp= *(((BYTE *)(x))+1);                \
                       *(((BYTE *)(x))+1) = *(((BYTE *)(x))+2); \
                       *(((BYTE *)(x))+2) = _tmp; }
#endif

/**
SWAP QWORD macro */
#ifndef QWORD_SWAP
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
#endif

/**dox***************************************************************/
          /** @} *//* end of msmacroh */

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/**
Definition of implementation specific constants */
#define MESSAGE_BUFFER_SIZE    100000   /**< buffer used for messages */
#define MESSAGE_BUFFER_NAME    "SYSMSG" /**< buffer name for messages */
#define MAX_RPC_CONNECTION     64       /**< server/client connections   */
#define MAX_STRING_LENGTH      256      /**< max string length for odb */
#define NET_BUFFER_SIZE        (ALIGN8(MAX_EVENT_SIZE)+sizeof(EVENT_HEADER)+\
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
#ifndef stricmp
#define stricmp(s1, s2) strcasecmp(s1, s2)
#endif

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

#endif

/* missing O_BINARY for non-PC */
#ifndef O_BINARY
#define O_BINARY 0
#define O_TEXT   0
#endif

/* min/max/abs macros */
#ifndef MAX
#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#ifndef ABS
#define ABS(a)              (((a) < 0)   ? -(a) : (a))
#endif

/* missing tell() for some operating systems */
#define TELL(fh) lseek(fh, 0, SEEK_CUR)

/* define file truncation */
#ifdef OS_WINNT
#define TRUNCATE(fh) chsize(fh, TELL(fh))
#else
#define TRUNCATE(fh) ftruncate(fh, TELL(fh))
#endif

/* reduced shared memory size */
#ifdef OS_SOLARIS
#define MAX_SHM_SIZE      0x20000       /* 128k */
#endif

/*------------------------------------------------------------------*/

/* Network structures */

typedef struct {
   DWORD routine_id;            /* routine ID like ID_BM_xxx    */
   DWORD param_size;            /* size in Bytes of parameter   */
} NET_COMMAND_HEADER;

typedef struct {
   NET_COMMAND_HEADER header;
   char param[32];              /* parameter array              */
} NET_COMMAND;


typedef struct {
   DWORD serial_number;
   DWORD sequence_number;
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
   char host_name[HOST_NAME_LENGTH];
   unsigned short host_port1;
   unsigned short host_port2;
   unsigned short host_port3;
   int debug;
   char experiment[NAME_LENGTH];
   char directory[MAX_STRING_LENGTH];
   char user[NAME_LENGTH];
   INT index;
};

typedef struct {
   char host_name[HOST_NAME_LENGTH];    /*  server name        */
   INT port;                    /*  ip port                 */
   char exp_name[NAME_LENGTH];  /*  experiment to connect   */
   int send_sock;               /*  tcp send socket         */
   INT remote_hw_type;          /*  remote hardware type    */
   char client_name[NAME_LENGTH];       /* name of remote client    */
   INT transport;               /*  RPC_TCP/RPC_FTCP        */
   INT rpc_timeout;             /*  in milliseconds         */

} RPC_CLIENT_CONNECTION;

typedef struct {
   char host_name[HOST_NAME_LENGTH];    /*  server name        */
   INT port;                    /*  ip port                 */
   char exp_name[NAME_LENGTH];  /*  experiment to connect   */
   int send_sock;               /*  tcp send socket         */
   int recv_sock;               /*  tcp receive socket      */
   int event_sock;              /*  event socket            */
   INT remote_hw_type;          /*  remote hardware type    */
   INT transport;               /*  RPC_TCP/RPC_FTCP        */
   INT rpc_timeout;             /*  in milliseconds         */

} RPC_SERVER_CONNECTION;

typedef struct {
   INT tid;                     /*  thread id               */
   char prog_name[NAME_LENGTH]; /*  client program name     */
   char host_name[HOST_NAME_LENGTH];    /*  client name        */
   int send_sock;               /*  tcp send socket         */
   int recv_sock;               /*  tcp receive socket      */
   int event_sock;              /*  tcp event socket        */
   INT remote_hw_type;          /*  hardware type           */
   INT transport;               /*  RPC_TCP/RPC_FTCP        */
   INT watchdog_timeout;        /*  in milliseconds         */
   DWORD last_activity;         /*  time of last recv       */
   INT convert_flags;           /*  convertion flags        */
   char *net_buffer;            /*  TCP cache buffer        */
   char *ev_net_buffer;
   INT net_buffer_size;         /*  size of TCP cache       */
   INT write_ptr, read_ptr, misalign;   /* pointers for cache */
   INT ev_write_ptr, ev_read_ptr, ev_misalign;
   HNDLE odb_handle;            /*  handle to online datab. */
   HNDLE client_handle;         /*  client key handle .     */

} RPC_SERVER_ACCEPTION;

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/**dox***************************************************************/
/** @addtogroup mssectionh
 *  
 *  @{  */

typedef struct {
   INT size;                          /**< size in bytes              */
   INT next_free;                     /**< Address of next free block */
} FREE_DESCRIP;

typedef struct {
   INT handle;                        /**< Handle of record base key  */
   WORD access_mode;                  /**< R/W flags                  */
   WORD flags;                        /**< Data format, ...           */

} OPEN_RECORD;

typedef struct {
   char name[NAME_LENGTH];      /* name of client             */
   INT pid;                     /* process ID                 */
   INT tid;                     /* thread ID                  */
   INT thandle;                 /* thread handle              */
   INT port;                    /* UDP port for wake up       */
   INT num_open_records;        /* number of open records     */
   DWORD last_activity;         /* time of last activity      */
   DWORD watchdog_timeout;      /* timeout in ms              */
   INT max_index;               /* index of last opren record */

   OPEN_RECORD open_record[MAX_OPEN_RECORDS];

} DATABASE_CLIENT;

typedef struct {
   char name[NAME_LENGTH];      /* name of database           */
   INT version;                 /* database version           */
   INT num_clients;             /* no of active clients       */
   INT max_client_index;        /* index of last client       */
   INT key_size;                /* size of key area in bytes  */
   INT data_size;               /* size of data area in bytes */
   INT root_key;                /* root key offset            */
   INT first_free_key;          /* first free key memory      */
   INT first_free_data;         /* first free data memory     */

   DATABASE_CLIENT client[MAX_CLIENTS]; /* entries for clients        */

} DATABASE_HEADER;

/* Per-process buffer access structure (descriptor) */

typedef struct {
   char name[NAME_LENGTH];      /* Name of database             */
   BOOL attached;               /* TRUE if database is attached */
   INT client_index;            /* index to CLIENT str. in buf. */
   DATABASE_HEADER *database_header;    /* pointer to database header   */
   void *database_data;         /* pointer to database data     */
   HNDLE semaphore;             /* semaphore handle             */
   INT lock_cnt;                /* flag to avoid multiple locks */
   HNDLE shm_handle;            /* handle (id) to shared memory */
   INT index;                   /* connection index / tid       */
   BOOL protect;                /* read/write protection        */

} DATABASE;

/* Open record descriptor */

typedef struct {
   HNDLE handle;                /* Handle of record base key  */
   HNDLE hDB;                   /* Handle of record's database */
   WORD access_mode;            /* R/W flags                  */
   void *data;                  /* Pointer to local data      */
   void *copy;                  /* Pointer of copy to data    */
   INT buf_size;                /* Record size in bytes       */
   void (*dispatcher) (INT, INT, void *);       /* Pointer to dispatcher func. */
   void *info;                  /* addtl. info for dispatcher */

} RECORD_LIST;

/* Event request descriptor */

typedef struct {
   INT buffer_handle;           /* Buffer handle */
   short int event_id;          /* same as in EVENT_HEADER */
   short int trigger_mask;
   void (*dispatcher) (HNDLE, HNDLE, EVENT_HEADER *, void *);   /* Dispatcher func. */

} REQUEST_LIST;

/**dox***************************************************************/
          /** @} *//* end of mssectionh */

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/*---- Logging channel information ---------------------------------*/

#define CHN_SETTINGS_STR(_name) char *_name[] = {\
"[Settings]",\
"Active = BOOL : 1",\
"Type = STRING : [8] Disk",\
"Filename = STRING : [256] run%05d.mid",\
"Format = STRING : [8] MIDAS",\
"Compression = INT : 0",\
"ODB dump = BOOL : 1",\
"Log messages = DWORD : 0",\
"Buffer = STRING : [32] SYSTEM",\
"Event ID = INT : -1",\
"Trigger mask = INT : -1",\
"Event limit = DOUBLE : 0",\
"Byte limit = DOUBLE : 0",\
"Subrun Byte limit = DOUBLE : 0",\
"Tape capacity = DOUBLE : 0",\
"Subdir format = STRING : [32]",\
"Current filename = STRING : [256]",\
"",\
"[Statistics]",\
"Events written = DOUBLE : 0",\
"Bytes written = DOUBLE : 0",\
"Bytes written uncompressed = DOUBLE : 0",\
"Bytes written total = DOUBLE : 0",\
"Bytes written subrun = DOUBLE : 0",\
"Files written = DOUBLE : 0",\
"",\
NULL}

typedef struct {
   BOOL active;
   char type[8];
   char filename[256];
   char format[8];
   INT compression;
   BOOL odb_dump;
   DWORD log_messages;
   char buffer[32];
   INT event_id;
   INT trigger_mask;
   double event_limit;
   double byte_limit;
   double subrun_byte_limit;
   double tape_capacity;
   char subdir_format[32];
   char current_filename[256];
} CHN_SETTINGS;

typedef struct {
   double events_written;
   double bytes_written;
   double bytes_written_uncompressed;
   double bytes_written_total;
   double bytes_written_subrun;
   double files_written;
} CHN_STATISTICS;

typedef struct {
   INT handle;
   char path[256];
   INT type;
   INT format;
   INT compression;
   INT subrun_number;
   INT buffer_handle;
   INT msg_buffer_handle;
   INT request_id;
   INT msg_request_id;
   HNDLE stats_hkey;
   HNDLE settings_hkey;
   CHN_SETTINGS settings;
   CHN_STATISTICS statistics;
   void **format_info;
   FTP_CON *ftp_con;
   void *gzfile;
} LOG_CHN;

#define LOG_TYPE_DISK 1
#define LOG_TYPE_TAPE 2
#define LOG_TYPE_FTP  3

/*---- VxWorks specific taskSpawn arguments ----------------------*/

typedef struct {
   char name[32];
   int priority;
   int options;
   int stackSize;
   int arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10;
} VX_TASK_SPAWN;

/*---- Channels for ss_suspend_set_dispatch() ----------------------*/

#define CH_IPC        1
#define CH_CLIENT     2
#define CH_SERVER     3
#define CH_LISTEN     4

/*---- Function declarations ---------------------------------------*/

/* make functions callable from a C++ program */
#ifdef __cplusplus
extern "C" {
#endif

   /*---- common function ----*/
   INT EXPRT cm_set_path(char *path);
   INT EXPRT cm_get_path(char *path);
   INT cm_dispatch_ipc(char *message, int s);
   INT EXPRT cm_msg_log(INT message_type, const char *message);
   INT EXPRT cm_msg_log1(INT message_type, const char *message, const char *facility);
   void EXPRT name2c(char *str);

   /*---- buffer manager ----*/
   INT bm_lock_buffer(INT buffer_handle);
   INT bm_unlock_buffer(INT buffer_handle);
   INT bm_notify_client(char *buffer_name, int s);
   INT EXPRT bm_mark_read_waiting(BOOL flag);
   INT bm_push_event(char *buffer_name);
   INT bm_check_buffers(void);
   INT EXPRT bm_remove_event_request(INT buffer_handle, INT request_id);
   void EXPRT bm_defragment_event(HNDLE buffer_handle, HNDLE request_id,
                                  EVENT_HEADER * pevent, void *pdata,
                                  void (*dispatcher) (HNDLE, HNDLE,
                                                      EVENT_HEADER *, void *));

   /*---- online database ----*/
   INT EXPRT db_lock_database(HNDLE database_handle);
   INT EXPRT db_unlock_database(HNDLE database_handle);
   INT EXPRT db_get_lock_cnt(HNDLE database_handle);
   INT db_update_record(INT hDB, INT hKey, int s);
   INT db_close_all_records(void);
   INT EXPRT db_flush_database(HNDLE hDB);
   INT EXPRT db_notify_clients(HNDLE hDB, HNDLE hKey, BOOL bWalk);
   INT EXPRT db_set_client_name(HNDLE hDB, const char *client_name);
   INT db_delete_key1(HNDLE hDB, HNDLE hKey, INT level, BOOL follow_links);
   INT EXPRT db_show_mem(HNDLE hDB, char *result, INT buf_size, BOOL verbose);

   /*---- rpc functions -----*/
   RPC_LIST EXPRT *rpc_get_internal_list(INT flag);
   INT rpc_server_receive(INT idx, int sock, BOOL check);
   INT rpc_server_callback(struct callback_addr *callback);
   INT EXPRT rpc_server_accept(int sock);
   INT rpc_client_accept(int sock);
   INT rpc_get_server_acception(void);
   INT rpc_set_server_acception(INT idx);
   INT EXPRT rpc_set_server_option(INT item, POINTER_T value);
   POINTER_T EXPRT rpc_get_server_option(INT item);
   INT recv_tcp_check(int sock);
   INT recv_event_check(int sock);
   INT rpc_deregister_functions(void);
   INT rpc_check_channels(void);
   void EXPRT rpc_client_check();
   INT rpc_server_disconnect(void);
   int EXPRT rpc_get_send_sock(void);
   int EXPRT rpc_get_event_sock(void);
   INT EXPRT rpc_set_opt_tcp_size(INT tcp_size);
   INT EXPRT rpc_get_opt_tcp_size(void);

   /*---- system services ----*/
   INT ss_shm_open(char *name, INT size, void **adr, HNDLE *handle, BOOL get_size);
   INT ss_shm_close(char *name, void *adr, HNDLE handle, INT destroy_flag);
   INT ss_shm_flush(char *name, void *adr, INT size);
   INT ss_shm_protect(HNDLE handle, void *adr);
   INT ss_shm_unprotect(HNDLE handle, void **adr);
   INT ss_spawnv(INT mode, char *cmdname, char *argv[]);
   INT ss_shell(int sock);
   INT EXPRT ss_daemon_init(BOOL keep_stdout);
   INT EXPRT ss_system(char *command);
   INT EXPRT ss_exec(char *cmd, INT * child_pid);
   BOOL EXPRT ss_existpid(INT pid);
   INT EXPRT ss_getpid(void);
   INT EXPRT ss_gettid(void);
   INT ss_getthandle(void);
   INT ss_set_async_flag(INT flag);
   INT EXPRT ss_semaphore_create(const char *semaphore_name, HNDLE * semaphore_handle);
   INT EXPRT ss_semaphore_wait_for(HNDLE semaphore_handle, INT timeout);
   INT EXPRT ss_semaphore_release(HNDLE semaphore_handle);
   INT EXPRT ss_semaphore_delete(HNDLE semaphore_handle, INT destroy_flag);
   INT EXPRT ss_mutex_create(HNDLE * mutex_handle);
   INT EXPRT ss_mutex_wait_for(HNDLE mutex_handle, INT timeout);
   INT EXPRT ss_mutex_release(HNDLE mutex_handle);
   INT EXPRT ss_mutex_delete(HNDLE mutex_handle);
   INT ss_wake(INT pid, INT tid, INT thandle);
   INT ss_alarm(INT millitime, void (*func) (int));
   INT ss_suspend_get_port(INT * port);
   INT ss_suspend_set_dispatch(INT channel, void *connection, INT(*dispatch) ());
   INT ss_resume(INT port, char *message);
   INT ss_suspend_exit(void);
   INT ss_exception_handler(void (*func) ());
   void EXPRT ss_force_single_thread();
   INT EXPRT ss_suspend(INT millisec, INT msg);
   midas_thread_t EXPRT ss_thread_create(INT(*func) (void *), void *param);
   INT EXPRT ss_thread_kill(midas_thread_t thread_id);
   INT EXPRT ss_get_struct_align(void);
   INT EXPRT ss_timezone(void);
   INT EXPRT ss_stack_get(char ***string);
   void EXPRT ss_stack_print();
   void EXPRT ss_stack_history_entry(char *tag);
   void EXPRT ss_stack_history_dump(char *filename);

   /*---- socket routines ----*/
   INT EXPRT send_tcp(int sock, char *buffer, DWORD buffer_size, INT flags);
   INT EXPRT recv_tcp(int sock, char *buffer, DWORD buffer_size, INT flags);
   INT send_udp(int sock, char *buffer, DWORD buffer_size, INT flags);
   INT recv_udp(int sock, char *buffer, DWORD buffer_size, INT flags);
   INT EXPRT recv_string(int sock, char *buffer, DWORD buffer_size, INT flags);

   /*---- system logging ----*/
   INT EXPRT ss_syslog(const char *message);

   /*---- event buffer routines ----*/
   INT EXPRT eb_create_buffer(INT size);
   INT EXPRT eb_free_buffer(void);
   BOOL EXPRT eb_buffer_full(void);
   BOOL EXPRT eb_buffer_empty(void);
   EVENT_HEADER EXPRT *eb_get_pointer(void);
   INT EXPRT eb_increment_pointer(INT buffer_handle, INT event_size);
   INT EXPRT eb_send_events(BOOL send_all);

   /*---- dual memory event buffer routines ----*/
   INT EXPRT dm_buffer_create(INT size, INT usize);
   INT EXPRT dm_buffer_release(void);
   BOOL EXPRT dm_area_full(void);
   EVENT_HEADER EXPRT *dm_pointer_get(void);
   INT EXPRT dm_pointer_increment(INT buffer_handle, INT event_size);
   INT EXPRT dm_area_send(void);
   INT EXPRT dm_area_flush(void);
   INT EXPRT dm_task(void *pointer);
   DWORD EXPRT dm_buffer_time_get(void);
   INT EXPRT dm_async_area_send(void *pointer);

   /*---- ring buffer routines ----*/
   int EXPRT rb_set_nonblocking();
   int EXPRT rb_create(int size, int max_event_size, int *ring_buffer_handle);
   int EXPRT rb_delete(int ring_buffer_handle);
   int EXPRT rb_get_wp(int handle, void **p, int millisec);
   int EXPRT rb_increment_wp(int handle, int size);
   int EXPRT rb_get_rp(int handle, void **p, int millisec);
   int EXPRT rb_increment_rp(int handle, int size);
   int EXPRT rb_get_buffer_level(int handle, int * n_bytes);

/*---- Include RPC identifiers -------------------------------------*/

#include "mrpc.h"

#ifdef __cplusplus
}
#endif
/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */
          /**dox***************************************************************//** @} *//* end of msystemincludecode */
