/********************************************************************\

  Name:         MIDAS.H
  Created by:   Stefan Ritt

  Contents:     Type definitions and function declarations needed
                for MIDAS applications


  $Log$
  Revision 1.15  1999/02/18 11:20:39  midas
  Added "level" parameter to db_scan_tree and db_scan_tree_link

  Revision 1.14  1999/02/11 13:20:29  midas
  Changed cm_msg from void to INT

  Revision 1.13  1999/02/09 14:38:54  midas
  Added debug logging facility

  Revision 1.12  1999/01/20 09:03:38  midas
  Added LAM_SOURCE_CRATE and LAM_SOURCE_STATION macros

  Revision 1.11  1999/01/19 10:26:39  midas
  Changed CRATE and STATION macros to LAM_SOURCE and LAM_STATION

  Revision 1.10  1999/01/13 09:40:48  midas
  Added db_set_data_index2 function

  Revision 1.9  1998/12/11 11:23:17  midas
  Release 1.6.2

  Revision 1.8  1998/12/10 10:18:38  midas
  Added SS_END_OF_FILE

  Revision 1.7  1998/10/29 15:58:05  midas
  Release 1.6.1

  Revision 1.6  1998/10/23 14:35:16  midas
  Added version comment

  Revision 1.5  1998/10/23 14:21:35  midas
  - Modified version scheme from 1.06 to 1.6.0
  - cm_get_version() now returns versino as string

  Revision 1.4  1998/10/23 13:11:13  midas
  Modified release number to 1.6

  Revision 1.3  1998/10/22 12:40:20  midas
  Added "oflag" to ss_tape_open()

  Revision 1.2  1998/10/12 12:18:59  midas
  Added Log tag in header


  Previous Revision history
  ------------------------------------------------------------------
  date         by    modification
  ---------    ---   ------------------------------------------------
  11-NOV-95    SR    created
  03-JUN-97    SR    increased number of clients to 32

\********************************************************************/

#ifndef _MIDAS_H_
#define _MIDAS_H_

/*------------------------------------------------------------------*/

/* MIDAS library version number. This value is returned by 
   cm_get_version and will be incremented whenever changes are done
   to the MIDAS library. First digit is major release, second
   digit is minor release, third digit is patch level */

#define MIDAS_VERSION "1.6.2"

/*------------------------------------------------------------------*/

/* find out on which operating system we are running */

#if defined( VAX ) || defined( __VMS )
#define OS_VMS
#endif

#if defined( _MSC_VER )
#define OS_WINNT
#endif

#if defined( __MSDOS__ )
#define OS_MSDOS
#endif

#if defined ( vxw )
#define OS_VXWORKS
#undef OS_UNIX
#endif

#if !defined(OS_VMS) && !defined(OS_MSDOS) && !defined(OS_UNIX) && !defined(OS_VXWORKS) && !defined(OS_WINNT)
#error MIDAS cannot be used on this operating system
#endif

/*------------------------------------------------------------------*/

/* Define basic data types */

#ifndef MIDAS_TYPE_DEFINED
#define MIDAS_TYPE_DEFINED

typedef unsigned char      BYTE;
typedef unsigned short int WORD;

#ifdef __alpha
typedef unsigned int       DWORD;
#else
typedef unsigned long int  DWORD;
#endif

#ifndef OS_WINNT
#ifndef OS_VXWORKS
typedef          DWORD     BOOL;
#endif
#endif

#endif /* MIDAS_TYPE_DEFINED */

/*
  Definitions depending on integer types:

  Note that the alpha chip uses 32 bit integers by default.
  Therefore always use 'INT' instead 'int'.
*/
#if defined(OS_MSDOS)
typedef long int           INT;
#elif defined( OS_WINNT )

/* INT predefined in windows.h */
#ifndef _INC_WINDOWS
#include <windows.h>
#endif

#else
typedef int                INT;
#endif

typedef          INT       HNDLE;

/* Include vxWorks for BOOL definition */
#ifdef OS_VXWORKS
#ifndef __INCvxWorksh
#include <vxWorks.h>
#endif
#endif

/*
  Definitions depending on pointer types:

  Note that the alpha chip uses 64 bit pointers by default.
  Therefore, when converting pointer to integers, always
  use the (PTYPE) cast.
*/
#ifdef __alpha
#define PTYPE              long int
#else
#define PTYPE              int
#endif

#define TRUE  1
#define FALSE 0

/* directory separator */
#if defined(OS_MSDOS) || defined(OS_WINNT)
#define DIR_SEPARATOR     '\\'
#define DIR_SEPARATOR_STR "\\"
#elif defined(OS_VMS)
#define DIR_SEPARATOR     ']'
#define DIR_SEPARATOR_STR "]"
#else
#define DIR_SEPARATOR     '/'
#define DIR_SEPARATOR_STR "/"
#endif

/* inline functions */
#if defined( _MSC_VER )
#define INLINE __inline
#elif defined(__GNUC__)
#define INLINE __inline__
#else 
#define INLINE
#endif

/*------------------------------------------------------------------*/

/* Definition of implementation specific constants */

/* all buffer sizes must be multiples of 4 ! */
#define TAPE_BUFFER_SIZE       0x8000/* buffer size for taping data */
#define NET_TCP_SIZE           0xFFFF       /* maximum TCP transfer */
#define OPT_TCP_SIZE           8192      /* optimal TCP buffer size */
#define NET_UDP_SIZE           8192         /* maximum UDP transfer */

#define EVENT_BUFFER_SIZE      0x100000 /* buffer used for events   */
#define EVENT_BUFFER_NAME      "SYSTEM" /* buffer name for commands */
#define MAX_EVENT_SIZE         0x80000   /* maximum event size 512k */
#define DEFAULT_EVENT_BUFFER_SIZE 0x200000; /* 2M                   */
#define DEFAULT_ODB_SIZE       100000   /* online database          */

#define NAME_LENGTH            32    /* length of names, mult.of 8! */
#define HOST_NAME_LENGTH       256   /* length of TCP/IP names      */
#define MAX_CLIENTS            32    /* client processes per buf/db */
#define MAX_EVENT_REQUESTS     10    /* event requests per client   */
#define MAX_OPEN_RECORDS       100   /* number of open DB records   */
#define MAX_ODB_PATH           256   /* length of path in ODB       */
#define MAX_EXPERIMENT         32    /* number of different exp.    */

#define MIDAS_TCP_PORT 1175 /* port under which server is listening */

/* Timeouts [ms] */
#define DEFAULT_RPC_TIMEOUT    10000
#define WATCHDOG_INTERVAL      1000 

#define DEFAULT_WATCHDOG_TIMEOUT 10000

/*------------------------------------------------------------------*/

/* Enumeration definitions */

/* System states */
#define STATE_STOPPED 1
#define STATE_PAUSED  2
#define STATE_RUNNING 3

/* Data format */
#define FORMAT_MIDAS  1       /* MIDAS banks                        */
#define FORMAT_YBOS   2       /* YBOS  banks                        */
#define FORMAT_ASCII  3       /* ASCII format                       */
#define FORMAT_FIXED  4       /* Fixed length binary records        */
#define FORMAT_DUMP   5       /* Dump (detailed ASCII) format       */
#define FORMAT_HBOOK  6       /* CERN hbook (rz) format             */

/* Sampling type */
#define GET_ALL       1       /* get all events (consume)           */
#define GET_SOME      2       /* get as much as possible (sampling) */
#define GET_FARM      3       /* distribute events over several
                                 clients (farming)                  */

/* Synchronous / Asynchronous flags */
#define SYNC          0
#define ASYNC         1

/* Data types */        /*                      min      max    */
#define TID_BYTE      1 /* unsigned byte         0       255    */
#define TID_SBYTE     2 /* signed byte         -128      127    */
#define TID_CHAR      3 /* single character      0       255    */
#define TID_WORD      4 /* two bytes             0      65535   */
#define TID_SHORT     5 /* signed word        -32768    32767   */
#define TID_DWORD     6 /* four bytes            0      2^32-1  */
#define TID_INT       7 /* signed dword        -2^31    2^31-1  */
#define TID_BOOL      8 /* four bytes bool       0        1     */
#define TID_FLOAT     9 /* 4 Byte float format                  */
#define TID_DOUBLE   10 /* 8 Byte float format                  */
#define TID_BITFIELD 11 /* 32 Bits Bitfield      0  111... (32) */
#define TID_STRING   12 /* zero terminated string               */
#define TID_ARRAY    13 /* array with unknown contents          */
#define TID_STRUCT   14 /* structure with fixed length          */
#define TID_KEY      15 /* key in online database               */
#define TID_LINK     16 /* link in online database              */
#define TID_LAST     17 /* end of TID list indicator            */

/* Access modes */
#define MODE_READ      (1<<0)
#define MODE_WRITE     (1<<1)
#define MODE_DELETE    (1<<2)
#define MODE_EXCLUSIVE (1<<3)
#define MODE_ALLOC     (1<<7)

/* RPC options */
#define RPC_OTIMEOUT       1
#define RPC_OTRANSPORT     2
#define RPC_OCONVERT_FLAG  3
#define RPC_OHW_TYPE       4
#define RPC_OSERVER_TYPE   5
#define RPC_OSERVER_NAME   6
#define RPC_CONVERT_FLAGS  7
#define RPC_ODB_HANDLE     8
#define RPC_CLIENT_HANDLE  9
#define RPC_SEND_SOCK      10
#define RPC_WATCHDOG_TIMEOUT 11

#define RPC_TCP            0
#define RPC_FTCP           1

/* Watchdog flags */
#define WF_WATCH_ME   (1<<0)         /* see cm_set_watchdog_flags   */
#define WF_CALL_WD    (1<<1)

/* Transitions */
#define TR_START      (1<<0) 
#define TR_STOP       (1<<1) 
#define TR_PAUSE      (1<<2) 
#define TR_RESUME     (1<<3) 
#define TR_PRESTART   (1<<4) 
#define TR_POSTSTART  (1<<5) 
#define TR_PRESTOP    (1<<6) 
#define TR_POSTSTOP   (1<<7) 
#define TR_PREPAUSE   (1<<8) 
#define TR_POSTPAUSE  (1<<9) 
#define TR_PRERESUME  (1<<10) 
#define TR_POSTRESUME (1<<11) 

/* Equipment types */
#define EQ_PERIODIC   1
#define EQ_POLLED     2
#define EQ_INTERRUPT  3
#define EQ_SLOW       4

/* Read - On flags */
#define RO_RUNNING    (1<<0)
#define RO_STOPPED    (1<<1)
#define RO_PAUSED     (1<<2)
#define RO_BOR        (1<<3)
#define RO_EOR        (1<<4)
#define RO_PAUSE      (1<<5)
#define RO_RESUME     (1<<6)

#define RO_TRANSITIONS (RO_BOR|RO_EOR|RO_PAUSE|RO_RESUME)
#define RO_ALWAYS      (0xFF)

#define RO_ODB        (1<<8)

/* special characters */

#define CH_BS             8
#define CH_TAB            9
#define CH_CR            13

#define CH_EXT 0x100

#define CH_HOME   (CH_EXT+0)
#define CH_INSERT (CH_EXT+1)
#define CH_DELETE (CH_EXT+2)
#define CH_END    (CH_EXT+3)
#define CH_PUP    (CH_EXT+4)
#define CH_PDOWN  (CH_EXT+5)
#define CH_UP     (CH_EXT+6)
#define CH_DOWN   (CH_EXT+7)
#define CH_RIGHT  (CH_EXT+8)
#define CH_LEFT   (CH_EXT+9)

/* event sources in equipment */
#define LAM_SOURCE(c, s)         (c<<24 | ((s) & 0xFFFFFF))
#define LAM_STATION(s)           (1<<(s-1))
#define LAM_SOURCE_CRATE(c)      (c>>24)
#define LAM_SOURCE_STATION(s)    ((s) & 0xFFFFFF)

/* CNAF commands */

#define CNAF        0x1         /* normal read/write                */
#define CNAF_nQ     0x2         /* Repeat read until noQ            */

#define CNAF_INHIBIT_SET         0x100
#define CNAF_INHIBIT_CLEAR       0x101
#define CNAF_CRATE_CLEAR         0x102
#define CNAF_CRATE_ZINIT         0x103
#define CNAF_TEST                0x110

/* min, max */

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

/*------------------------------------------------------------------*/

/* Align macro for data alignment on 8-byte boundary */
#define ALIGN(x)  (((x)+7) & ~7)

/* Align macro for variable data alignment */
#define VALIGN(adr,align)  (((PTYPE) (adr)+align-1) & ~(align-1))

/*------------------------------------------------------------------*/

/* Bit flags */

#define EVENTID_ALL        -1     /* any event id                   */
#define TRIGGER_ALL        -1     /* any type of trigger            */

/* System message types */
#define MT_ERROR           (1<<0)
#define MT_INFO            (1<<1)
#define MT_DEBUG           (1<<2)
#define MT_USER            (1<<3)
#define MT_LOG             (1<<4)
#define MT_TALK            (1<<5)
#define MT_CALL            (1<<6)
#define MT_ALL              0xFF

#define MERROR             MT_ERROR, __FILE__, __LINE__
#define MINFO              MT_INFO,  __FILE__, __LINE__
#define MDEBUG             MT_DEBUG, __FILE__, __LINE__
#define MUSER              MT_USER,  __FILE__, __LINE__ /* produced by interactive user */
#define MLOG               MT_LOG,   __FILE__, __LINE__ /* info message which is only logged */
#define MTALK              MT_TALK,  __FILE__, __LINE__ /* info message for speech system */
#define MCALL              MT_CALL,  __FILE__, __LINE__ /* info message for telephone call */

/*------------------------------------------------------------------*/

/* Status and error codes */
#define SUCCESS                       1
#define CM_SUCCESS                    1
#define CM_SET_ERROR                102
#define CM_NO_CLIENT                103
#define CM_DB_ERROR                 104
#define CM_UNDEF_EXP                105
#define CM_VERSION_MISMATCH         106
#define CM_SHUTDOWN                 107
#define CM_WRONG_PASSWORD           108
#define CM_UNDEF_ENVIRON            109
                                       
/* Buffer Manager */                   
#define BM_SUCCESS                    1
#define BM_CREATED                  202
#define BM_NO_MEMORY                203
#define BM_INVALID_NAME             204
#define BM_INVALID_HANDLE           205
#define BM_NO_SLOT                  206
#define BM_NO_MUTEX                 207
#define BM_NOT_FOUND                208
#define BM_ASYNC_RETURN             209
#define BM_TRUNCATED                210
#define BM_MULTIPLE_HOSTS           211
#define BM_MEMSIZE_MISMATCH         212
#define BM_CONFLICT                 213
#define BM_EXIT                     214
#define BM_INVALID_PARAM            215
#define BM_MORE_EVENTS              216
#define BM_INVALID_MIXING           217
#define BM_NO_SHM                   218
                                       
/* Online Database */                  
#define DB_SUCCESS                    1
#define DB_CREATED                  302
#define DB_NO_MEMORY                303
#define DB_INVALID_NAME             304
#define DB_INVALID_HANDLE           305
#define DB_NO_SLOT                  306
#define DB_NO_MUTEX                 307
#define DB_MEMSIZE_MISMATCH         308
#define DB_INVALID_PARAM            309
#define DB_FULL                     310
#define DB_KEY_EXIST                311
#define DB_NO_KEY                   312
#define DB_KEY_CREATED              313
#define DB_TRUNCATED                314
#define DB_TYPE_MISMATCH            315
#define DB_NO_MORE_SUBKEYS          316
#define DB_FILE_ERROR               317
#define DB_NO_ACCESS                318
#define DB_STRUCT_SIZE_MISMATCH     319
#define DB_OPEN_RECORD              320
#define DB_OUT_OF_RANGE             321

/* System Services */
#define SS_SUCCESS                    1
#define SS_CREATED                  402
#define SS_NO_MEMORY                403
#define SS_INVALID_NAME             404
#define SS_INVALID_HANDLE           405
#define SS_INVALID_ADDRESS          406
#define SS_FILE_ERROR               407
#define SS_NO_MUTEX                 408
#define SS_NO_PROCESS               409
#define SS_NO_THREAD                410
#define SS_SOCKET_ERROR             411
#define SS_TIMEOUT                  412
#define SS_SERVER_RECV              413
#define SS_CLIENT_RECV              414
#define SS_ABORT                    415
#define SS_EXIT                     416
#define SS_NO_TAPE                  417
#define SS_DEV_BUSY                 418
#define SS_IO_ERROR                 419
#define SS_TAPE_ERROR               420
#define SS_NO_DRIVER                421
#define SS_END_OF_TAPE              422
#define SS_END_OF_FILE              423

/* Remote Procedure Calls */
#define RPC_SUCCESS                   1
#define RPC_ABORT              SS_ABORT
#define RPC_NO_CONNECTION           502
#define RPC_NET_ERROR               503
#define RPC_TIMEOUT                 504
#define RPC_EXCEED_BUFFER           505
#define RPC_NOT_REGISTERED          506
#define RPC_CONNCLOSED              507
#define RPC_INVALID_ID              508
#define RPC_SHUTDOWN                509
#define RPC_NO_MEMORY               510
#define RPC_DOUBLE_DEFINED          511

/* Frontend */
#define FE_SUCCESS                    1
#define FE_ERR_ODB                  602
#define FE_ERR_HW                   603
#define FE_ERR_DISABLED             604
#define FE_ERR_DRIVER               605

/* History */
#define HS_SUCCESS                    1
#define HS_FILE_ERROR               702
#define HS_NO_MEMORY                703
#define HS_TRUNCATED                704
#define HS_WRONG_INDEX              705
#define HS_UNDEFINED_EVENT          706
#define HS_UNDEFINED_VAR            707

/* FTP */
#define FTP_SUCCESS                   1
#define FTP_NET_ERROR               802
#define FTP_FILE_ERROR              803
#define FTP_RESPONSE_ERROR          804
#define FTP_INVALID_ARG             805

/* Slow control commands */
#define CMD_INIT                      1
#define CMD_EXIT                      2
#define CMD_IDLE                      3
#define CMD_SET                       4
#define CMD_SET_ALL                   5
#define CMD_GET                       6
#define CMD_GET_ALL                   7
#define CMD_GET_CURRENT               8
#define CMD_GET_CURRENT_ALL           9
#define CMD_SET_CURRENT_LIMIT        10

/* Commands for interrupt events */
#define CMD_INTERRUPT_ENABLE        100
#define CMD_INTERRUPT_DISABLE       101
#define CMD_INTERRUPT_ATTACH        102
#define CMD_INTERRUPT_DETACH        103

/*---- Buffer manager structures -----------------------------------*/

/* Event header */

typedef struct {
  short int     event_id;        /* event ID starting from one      */
  short int     trigger_mask;    /* hardware trigger mask           */
  DWORD         serial_number;   /* serial number starting from one */
  DWORD         time_stamp;      /* time of production of event     */
  DWORD         data_size;       /* size of event in bytes w/o header */
} EVENT_HEADER;

/* macros to access event header in mfeuser.c */
#define TRIGGER_MASK(e)  ((((EVENT_HEADER *) e)-1)->trigger_mask)
#define EVENT_ID(e)      ((((EVENT_HEADER *) e)-1)->event_id)
#define SERIAL_NUMBER(e) ((((EVENT_HEADER *) e)-1)->serial_number)
#define TIME_STAMP(e)    ((((EVENT_HEADER *) e)-1)->time_stamp)


/* system event IDs */
#define EVENTID_BOR      ((short int) 0x8000)  /* Begin-of-run      */
#define EVENTID_EOR      ((short int) 0x8001)  /* End-of-run        */
#define EVENTID_MESSAGE  ((short int) 0x8002)  /* Message events    */

/* magic number used in trigger_mask for BOR event */
#define MIDAS_MAGIC      0x494d            /* 'MI' */

/* Buffer structure */

typedef struct {
  INT           id;              /* request id                      */
  BOOL          valid;           /* indicating a valid entry        */
  short int     event_id;        /* event ID                        */
  short int     trigger_mask;    /* trigger mask                    */
  INT           sampling_type;   /* GET_ALL, GET_SOME, GET_FARM     */
                                 /* dispatch function */
  void          (*dispatch)(HNDLE,HNDLE,EVENT_HEADER*,void*); 
} EVENT_REQUEST;

typedef struct {
  char          name[NAME_LENGTH];    /* name of client             */
  INT           pid;                  /* process ID                 */
  INT           tid;                  /* thread ID                  */
  INT           thandle;              /* thread handle              */
  INT           port;                 /* UDP port for wake up       */
  INT           read_pointer;         /* read pointer to buffer     */
  INT           max_request_index;    /* index of last request      */
  INT           num_received_events;  /* no of received events      */
  INT           num_sent_events;      /* no of sent events          */
  INT           num_waiting_events;   /* no of waiting events       */
  float         data_rate;            /* data rate in kB/sec        */
  BOOL          read_wait;            /* wait for read - flag       */
  INT           write_wait;           /* wait for write # bytes     */
  BOOL          wake_up;              /* client got a wake-up msg   */
  BOOL          all_flag;             /* at least one GET_ALL request */
  INT           last_activity;        /* time of last activity      */
  INT           watchdog_timeout;     /* timeout in ms              */

  EVENT_REQUEST event_request[MAX_EVENT_REQUESTS];

} BUFFER_CLIENT;

typedef struct {
  char          name[NAME_LENGTH];    /* name of buffer             */
  INT           num_clients;          /* no of active clients       */
  INT           max_client_index;     /* index of last client       */
  INT           size;                 /* size of data area in bytes */
  INT           read_pointer;         /* read pointer               */
  INT           write_pointer;        /* read pointer               */
  INT           num_in_events;        /* no of received events      */
  INT           num_out_events;       /* no of distributed events   */

  BUFFER_CLIENT client[MAX_CLIENTS];  /* entries for clients        */

} BUFFER_HEADER;

/* Per-process buffer access structure (descriptor) */

typedef struct {
  BOOL          attached;           /* TRUE if buffer is attached   */
  INT           client_index;       /* index to CLIENT str. in buf. */
  BUFFER_HEADER *buffer_header;     /* pointer to buffer header     */
  void          *buffer_data;       /* pointer to buffer data       */
  char          *read_cache;        /* cache for burst read         */
  INT           read_cache_size;    /* cache size in bytes          */
  INT           read_cache_rp;      /* cache read pointer           */
  INT           read_cache_wp;      /* cache write pointer          */
  char          *write_cache;       /* cache for burst read         */
  INT           write_cache_size;   /* cache size in bytes          */
  INT           write_cache_rp;     /* cache read pointer           */
  INT           write_cache_wp;     /* cache write pointer          */
  HNDLE         mutex;              /* mutex/semaphore handle       */
  INT           shm_handle;         /* handle to shared memory      */
  INT           index;              /* connection index / tid       */
  BOOL          callback;           /* callback defined for this buffer */

} BUFFER;

/* Key list */

typedef struct {
  DWORD         type;                 /* TID_xxx type                      */
  INT           num_values;           /* number of values                  */
  char          name[NAME_LENGTH];    /* name of variable                  */
  INT           data;                 /* Address of variable (offset)      */
  INT           total_size;           /* Total size of data block          */
  INT           item_size;            /* Size of single data item          */
  WORD          access_mode;          /* Access mode                       */
  WORD          lock_mode;            /* Lock mode                         */
  WORD          exclusive_client;     /* Index of client in excl. mode     */
  WORD          notify_count;         /* Notify counter                    */
  INT           next_key;             /* Address of next key               */
  INT           parent_keylist;       /* keylist to which this key belongs */
  INT           last_written;         /* Time of last write action  */
} KEY;

typedef struct {
  INT           parent;               /* Address of parent key      */
  INT           num_keys;             /* number of keys             */
  INT           first_key;            /* Address of first key       */
} KEYLIST;

/*---- Equipment ---------------------------------------------------*/

#define CH_INPUT    1
#define CH_OUTPUT   2

typedef struct {
  char   name[NAME_LENGTH];           /* Driver name                       */
  INT    (*dd)(INT cmd, ...);         /* Device driver entry point         */
  INT    channels;                    /* Number of channels                */
  DWORD  type;                        /* channel type, combination of CH_xxx*/
  void   *dd_info;                    /* Private info for device driver    */
} DEVICE_DRIVER;

typedef struct {
  WORD   event_id;                    /* Event ID associated with equipm.  */
  WORD   trigger_mask;                /* Trigger mask                      */
  char   buffer[NAME_LENGTH];         /* Event buffer to send events into  */
  INT    eq_type;                     /* One of EQ_xxx                     */
  INT    source;                      /* Event soource (LAM/IRQ)           */
  char   format[8];                   /* Data format to produce            */
  BOOL   enabled;                     /* Enable flag                       */
  INT    read_on;                     /* Combination of Read-On flags RO_xxx */
  INT    period;                      /* Readout interval/Polling time in ms */
  DWORD  event_limit;                 /* Stop run when limit is reached    */
  INT    history;                     /* Log history                       */
  char   frontend_host[NAME_LENGTH];  /* Host on which FE is running       */
  char   frontend_name[NAME_LENGTH];  /* Frontend name                     */
  char   frontend_file_name[256];     /* Source file used for user FE      */
} EQUIPMENT_INFO;

typedef struct {
  DWORD  events_sent;
  DWORD  events_per_sec;
  double kbytes_per_sec;
} EQUIPMENT_STATS;

typedef struct eqpmnt *PEQUIPMENT;

typedef struct eqpmnt {
  char   name[NAME_LENGTH];             /* Equipment name                  */
  EQUIPMENT_INFO info;                  /* From above                      */
  INT    (*readout)(char *);            /* Pointer to user readout routine */
  INT    (*cd)(INT cmd, PEQUIPMENT);    /* Class driver routine            */
  DEVICE_DRIVER *driver;                /* Device driver list              */
  char   *init_string;                  /* Init string for fixed events    */
  void   *cd_info;                      /* private data for class driver   */
  INT    status;                        /* One of FE_xxx                   */
  DWORD  last_called;                   /* Last time event was read        */
  DWORD  poll_count;                    /* Needed to poll 'period'         */
  INT    format;                        /* FORMAT_xxx                      */
  HNDLE  buffer_handle;                 /* MIDAS buffer handle             */
  HNDLE  hkey_variables;                /* Key to variables subtree in ODB */
  DWORD  serial_number;                 /* event serial number             */
  DWORD  odb_out;                       /* # updates FE -> ODB             */
  DWORD  odb_in;                        /* # updated ODB -> FE             */
  DWORD  bytes_sent;                    /* number of bytes sent            */
  DWORD  events_sent;                   /* number of events sent           */
  EQUIPMENT_STATS stats;
} EQUIPMENT;

/*---- Banks -------------------------------------------------------*/

#define BANK_FORMAT_VERSION 1

typedef struct {
  DWORD  data_size;
  DWORD  flags;
} BANK_HEADER;

typedef struct {
  char   name[4];
  WORD   type;
  WORD   data_size;
} BANK;

typedef struct {
  char  name[NAME_LENGTH];
  DWORD type;
  DWORD n_data;
} TAG;

typedef struct {
  char   name[9];
  WORD   type;
  DWORD  size;
  char   **init_str;
  BOOL   output_flag;
  void   *addr;
  DWORD  n_data;
  HNDLE  def_key;
} BANK_LIST;

/*---- Analyzer request --------------------------------------------*/

typedef struct {
  char   name[NAME_LENGTH];           /* Module name                       */
  char   author[NAME_LENGTH];         /* Author                            */
  INT    (*analyzer)(EVENT_HEADER*,void*); /* Pointer to user analyzer routine  */
  INT    (*bor)(INT run_number);      /* Pointer to begin-of-run routine   */
  INT    (*eor)(INT run_number);      /* Pointer to end-of-run routine     */
  INT    (*init)();                   /* Pointer to init routine           */
  INT    (*exit)();                   /* Pointer to exit routine           */
  void   *parameters;                 /* Pointer to parameter structure    */
  INT    param_size;                  /* Size of parameter structure       */
  char   **init_str;                  /* Parameter init string             */
  BOOL   enabled;                     /* Enabled flag                      */
} ANA_MODULE;

typedef struct {
  INT    event_id;                    /* Event ID associated with equipm.  */
  INT    trigger_mask;                /* Trigger mask                      */
  INT    sampling_type;               /* GET_ALL/GET_SOME                  */
  char   buffer[NAME_LENGTH];         /* Event buffer to send events into  */
  BOOL   enabled;                     /* Enable flag                       */
  char   client_name[NAME_LENGTH];    /* Analyzer name                     */
  char   host[NAME_LENGTH];           /* Host on which analyzer is running */
} AR_INFO;

typedef struct {
  DWORD  events_received;
  DWORD  events_per_sec;
  DWORD  events_written;
} AR_STATS;

typedef struct {
  char   event_name[NAME_LENGTH];     /* Event name                        */
  AR_INFO ar_info;                    /* From above                        */
  INT    (*analyzer)(EVENT_HEADER*,void*); /* Pointer to user analyzer routine  */
  ANA_MODULE **ana_module;            /* List of analyzer modules          */
  BANK_LIST *bank_list;               /* List of banks for event           */
  INT    rwnt_buffer_size;            /* Size in events of RW N-tuple buf  */
  char   **init_string;
  INT    status;                      /* One of FE_xxx                     */
  HNDLE  buffer_handle;               /* MIDAS buffer handle               */
  HNDLE  request_id;                  /* Event request handle              */
  HNDLE  hkey_variables;              /* Key to variables subtree in ODB   */
  HNDLE  hkey_common;                 /* Key to common subtree             */
  void   *addr;                       /* Buffer for CWNT filling           */
  struct {                            /* Buffer for event number for CWNT  */
    DWORD run;
    DWORD serial;
    DWORD time;
    } number;
  DWORD  events_received;             /* number of events sent             */
  DWORD  events_written;              /* number of events written          */
  AR_STATS ar_stats;

} ANALYZE_REQUEST;

/*---- History structures ------------------------------------------*/

#define RT_DATA (*((DWORD *) "HSDA"))
#define RT_DEF  (*((DWORD *) "HSDF"))

typedef struct {
  DWORD record_type;  /* RT_DATA or RT_DEF */
  DWORD event_id;
  DWORD time;
  DWORD def_offset;   /* place of definition */
  DWORD data_size;    /* data following this header in bytes */
} HIST_RECORD;

typedef struct {
  DWORD event_id;
  char  event_name[NAME_LENGTH];
  DWORD def_offset;
} DEF_RECORD;

typedef struct {
  DWORD event_id;
  DWORD time;
  DWORD offset;
} INDEX_RECORD;

typedef struct {
  DWORD    event_id;
  char     event_name[NAME_LENGTH];
  DWORD    n_tag;
  TAG      *tag;
  DWORD    hist_fh;
  DWORD    index_fh;
  DWORD    def_fh;
  DWORD    base_time;
  DWORD    def_offset;
} HISTORY;

/*---- ODB runinfo -------------------------------------------------*/

typedef struct {
  INT       state;
  INT       online_mode;
  INT       run_number;
  INT       transition_in_progress;
  char      start_time[32];
  DWORD     start_time_binary;
  char      stop_time[32];
  DWORD     stop_time_binary;
} RUNINFO;

#define RUNINFO_STR(_name) char *_name[] = {\
"[.]",\
"State = INT : 1",\
"Online Mode = INT : 1",\
"Run number = INT : 0",\
"Transition in progress = INT : 0",\
"Start time = STRING : [32] Tue Sep 09 15:04:42 1997",\
"Start time binary = DWORD : 0",\
"Stop time = STRING : [32] Tue Sep 09 15:04:42 1997",\
"Stop time binary = DWORD : 0",\
"",\
NULL }

/*---- CERN libray -------------------------------------------------*/

#ifdef extname
#define PAWC_NAME pawc_
#else
#define PAWC_NAME PAWC
#endif

#define PAWC_DEFINE(size) \
INT PAWC_NAME[size/4];    \
INT pawc_size = size

/*---- RPC ---------------------------------------------------------*/

/* flags */
#define RPC_IN       (1 << 0)
#define RPC_OUT      (1 << 1)
#define RPC_POINTER  (1 << 2)
#define RPC_FIXARRAY (1 << 3)
#define RPC_VARARRAY (1 << 4)
#define RPC_OUTGOING (1 << 5)

/* Server types */
#define ST_NONE            0
#define ST_SINGLE          1
#define ST_MTHREAD         2
#define ST_MPROCESS        3
#define ST_SUBPROCESS      4
#define ST_REMOTE          5

/* function list */
typedef struct {
  WORD          tid;
  WORD          flags;
  INT           n;
} RPC_PARAM;

typedef struct {
  INT           id;
  char          *name;
  RPC_PARAM     param[15];
  INT           (*dispatch)(INT,void**);
} RPC_LIST;

/* IDs allow for users */
#define RPC_MIN_ID    1
#define RPC_MAX_ID 9999

/* Data conversion flags */
#define CF_ENDIAN          (1<<0)
#define CF_IEEE2VAX        (1<<1)
#define CF_VAX2IEEE        (1<<2)

#define CBYTE(_i)        (* ((BYTE *)       prpc_param[_i]))
#define CPBYTE(_i)       (  ((BYTE *)       prpc_param[_i]))

#define CSHORT(_i)       (* ((short *)      prpc_param[_i]))
#define CPSHORT(_i)      (  ((short *)      prpc_param[_i]))

#define CINT(_i)         (* ((INT *)        prpc_param[_i]))
#define CPINT(_i)        (  ((INT *)        prpc_param[_i]))

#define CWORD(_i)        (* ((WORD *)       prpc_param[_i]))
#define CPWORD(_i)       (  ((WORD *)       prpc_param[_i]))

#define CLONG(_i)        (* ((long *)       prpc_param[_i]))
#define CPLONG(_i)       (  ((long *)       prpc_param[_i]))

#define CDWORD(_i)       (* ((DWORD *)      prpc_param[_i]))
#define CPDWORD(_i)      (  ((DWORD *)      prpc_param[_i]))

#define CHNDLE(_i)       (* ((HNDLE *)      prpc_param[_i]))
#define CPHNDLE(_i)      (  ((HNDLE *)      prpc_param[_i]))

#define CBOOL(_i)        (* ((BOOL *)       prpc_param[_i]))
#define CPBOOL(_i)       (  ((BOOL *)       prpc_param[_i]))

#define CFLOAT(_i)       (* ((float *)      prpc_param[_i]))
#define CPFLOAT(_i)      (  ((float *)      prpc_param[_i]))

#define CDOUBLE(_i)      (* ((double *)     prpc_param[_i]))
#define CPDOUBLE(_i)     (  ((double *)     prpc_param[_i]))

#define CSTRING(_i)      (  ((char *)       prpc_param[_i]))
#define CARRAY(_i)       (  ((void *)       prpc_param[_i]))

#define CBYTE(_i)        (* ((BYTE *)       prpc_param[_i]))
#define CPBYTE(_i)       (  ((BYTE *)       prpc_param[_i]))

#define CSHORT(_i)       (* ((short *)      prpc_param[_i]))
#define CPSHORT(_i)      (  ((short *)      prpc_param[_i]))

#define CINT(_i)         (* ((INT *)        prpc_param[_i]))
#define CPINT(_i)        (  ((INT *)        prpc_param[_i]))

#define CWORD(_i)        (* ((WORD *)       prpc_param[_i]))
#define CPWORD(_i)       (  ((WORD *)       prpc_param[_i]))

#define CLONG(_i)        (* ((long *)       prpc_param[_i]))
#define CPLONG(_i)       (  ((long *)       prpc_param[_i]))

#define CDWORD(_i)       (* ((DWORD *)      prpc_param[_i]))
#define CPDWORD(_i)      (  ((DWORD *)      prpc_param[_i]))

#define CHNDLE(_i)       (* ((HNDLE *)      prpc_param[_i]))
#define CPHNDLE(_i)      (  ((HNDLE *)      prpc_param[_i]))

#define CBOOL(_i)        (* ((BOOL *)       prpc_param[_i]))
#define CPBOOL(_i)       (  ((BOOL *)       prpc_param[_i]))

#define CFLOAT(_i)       (* ((float *)      prpc_param[_i]))
#define CPFLOAT(_i)      (  ((float *)      prpc_param[_i]))

#define CDOUBLE(_i)      (* ((double *)     prpc_param[_i]))
#define CPDOUBLE(_i)     (  ((double *)     prpc_param[_i]))

#define CSTRING(_i)      (  ((char *)       prpc_param[_i]))
#define CARRAY(_i)       (  ((void *)       prpc_param[_i]))



#define cBYTE            (* ((BYTE *)       prpc_param[--n_param]))
#define cPBYTE           (  ((BYTE *)       prpc_param[--n_param]))

#define cSHORT           (* ((short *)      prpc_param[--n_param]))
#define cPSHORT          (  ((short *)      prpc_param[--n_param]))

#define cINT             (* ((INT *)        prpc_param[--n_param]))
#define cPINT            (  ((INT *)        prpc_param[--n_param]))

#define cWORD            (* ((WORD *)       prpc_param[--n_param]))
#define cPWORD           (  ((WORD *)       prpc_param[--n_param]))

#define cLONG            (* ((long *)       prpc_param[--n_param]))
#define cPLONG           (  ((long *)       prpc_param[--n_param]))

#define cDWORD           (* ((DWORD *)      prpc_param[--n_param]))
#define cPDWORD          (  ((DWORD *)      prpc_param[--n_param]))

#define cHNDLE           (* ((HNDLE *)      prpc_param[--n_param]))
#define cPHNDLE          (  ((HNDLE *)      prpc_param[--n_param]))

#define cBOOL            (* ((BOOL *)       prpc_param[--n_param]))
#define cPBOOL           (  ((BOOL *)       prpc_param[--n_param]))

#define cFLOAT           (* ((float *)      prpc_param[--n_param]))
#define cPFLOAT          (  ((float *)      prpc_param[--n_param]))

#define cDOUBLE          (* ((double *)     prpc_param[--n_param]))
#define cPDOUBLE         (  ((double *)     prpc_param[--n_param]))

#define cSTRING          (  ((char *)       prpc_param[--n_param]))
#define cARRAY           (  ((void *)       prpc_param[--n_param]))

/*---- Function declarations ---------------------------------------*/

/* make functions callable from a C++ program */
#ifdef __cplusplus
extern "C" {
#endif

/* make functions under WinNT dll exportable */
#if defined(OS_WINNT) && defined(MIDAS_DLL)
#define EXPRT __declspec(dllexport)
#else
#define EXPRT
#endif

/*---- common routines ----*/
INT EXPRT cm_get_error(INT code, char *string);
char EXPRT *cm_get_version(void);
INT EXPRT cm_get_environment(char *host_name, char *exp_name);
INT EXPRT cm_list_experiments(char *host_name, char exp_name[MAX_EXPERIMENT][NAME_LENGTH]);
INT EXPRT cm_select_experiment(char *host_name, char *exp_name);
INT EXPRT cm_connect_experiment(char *host_name, char *exp_name, char *client_name, void (*func)(char*));
INT EXPRT cm_connect_experiment1(char *host_name, char *exp_name, char *client_name, void (*func)(char*), INT odb_size);
INT EXPRT cm_disconnect_experiment(void);
INT EXPRT cm_register_transition(INT transition, INT (*func)(INT,char*));
INT EXPRT cm_transition(INT transition, INT run_number, char *error, INT strsize, INT async_flag);
INT EXPRT cm_register_server(void);
INT EXPRT cm_register_function(INT id, INT (*func)(INT,void**));
INT EXPRT cm_connect_client(char *client_name, HNDLE *hConn);
INT EXPRT cm_disconnect_client(HNDLE hConn, BOOL bShutdown);
INT EXPRT cm_set_experiment_database(HNDLE hDB, HNDLE hKeyClient);
INT EXPRT cm_get_experiment_database(HNDLE *hDB, HNDLE *hKeyClient);
INT EXPRT cm_set_client_info(HNDLE hDB, HNDLE *hKeyClient, char *host_name, char *client_name, INT computer_id, char *password);
INT EXPRT cm_get_client_info(char *client_name);
INT EXPRT cm_set_watchdog_params(BOOL call_watchdog, INT timeout);
INT EXPRT cm_get_watchdog_params(BOOL *call_watchdog, INT *timeout);
INT EXPRT cm_get_watchdog_info(HNDLE hDB, char *client_name, INT *timeout, INT *last);
INT EXPRT cm_enable_watchdog(BOOL flag);
void EXPRT cm_watchdog(int);
INT EXPRT cm_shutdown(char *name);
INT EXPRT cm_exist(char *name, BOOL bUnique);
INT EXPRT cm_cleanup(void);
INT EXPRT cm_yield(INT millisec);
INT EXPRT cm_execute(char *command, char *result, INT buf_size);
INT EXPRT cm_synchronize(DWORD *sec);
INT EXPRT cm_asctime(char *str, INT buf_size);
INT EXPRT cm_time(DWORD *time);

INT EXPRT cm_set_msg_print(INT system_mask, INT user_mask, int (*func)(const char*));
INT EXPRT cm_msg(INT message_type, char* filename, INT line, const char *routine, const char *format, ...);
INT EXPRT cm_msg_register(void (*func)(HNDLE,HNDLE,EVENT_HEADER*,void*));

BOOL EXPRT equal_ustring(char *str1, char *str2);

/*---- buffer manager ----*/
INT EXPRT bm_open_buffer(char *buffer_name, INT buffer_size, INT *buffer_handle);
INT EXPRT bm_close_buffer(INT buffer_handle);
INT EXPRT bm_close_all_buffers(void);
INT EXPRT bm_init_buffer_counters(INT buffer_handle);
INT EXPRT bm_get_buffer_info(INT buffer_handle, BUFFER_HEADER *buffer_header);
INT EXPRT bm_get_buffer_level(INT buffer_handle, INT *n_bytes);
INT EXPRT bm_set_cache_size(INT buffer_handle, INT read_size, INT write_size);
INT EXPRT bm_compose_event(EVENT_HEADER *event_header,
              short int event_id, short int trigger_mask,
              DWORD size, DWORD serial);
INT EXPRT bm_request_event(INT buffer_handle, short int event_id,
              short int trigger_mask,
              INT sampling_type, INT *request_id, 
              void (*func)(HNDLE,HNDLE,EVENT_HEADER*,void*));
INT EXPRT bm_add_event_request(INT buffer_handle, short int event_id,
              short int trigger_mask,
              INT sampling_type, void (*func)(HNDLE,HNDLE,EVENT_HEADER*,void*),
              INT request_id);
INT EXPRT bm_delete_request(INT request_id);
INT EXPRT bm_send_event(INT buffer_handle, void *event, INT buf_size,
                  INT async_flag);
INT EXPRT bm_receive_event(INT buffer_handle, void *destination,
                          INT *buf_size, INT async_flag);
INT EXPRT bm_flush_cache(INT buffer_handle, INT async_flag);
INT EXPRT bm_poll_event(INT flag);
INT EXPRT bm_empty_buffers(void);

/*---- online database functions -----*/
INT EXPRT db_open_database(char *database_name, INT database_size, HNDLE *hdb, char *client_name);
INT EXPRT db_close_database(HNDLE database_handle);
INT EXPRT db_close_all_databases(void);

INT EXPRT db_create_key(HNDLE hdb, HNDLE key_handle, char *key_name, DWORD type);
INT EXPRT db_create_link(HNDLE hdb, HNDLE key_handle, char *link_name, char *destination);
INT EXPRT db_set_value(HNDLE hdb, HNDLE hKeyRoot, char *key_name, void *data, INT size, INT num_values, DWORD type);
INT EXPRT db_get_value(HNDLE hdb, HNDLE hKeyRoot, char *key_name, void *data, INT *size, DWORD type);
INT EXPRT db_find_key(HNDLE hdb, HNDLE hkey, char *name, HNDLE *hsubkey);
INT EXPRT db_find_link(HNDLE hDB, HNDLE hKey, char *key_name, HNDLE *subhKey);
INT EXPRT db_scan_tree(HNDLE hDB, HNDLE hKey, int level, void (*callback)(HNDLE,HNDLE,KEY*,INT,void *), void *info);
INT EXPRT db_scan_tree_link(HNDLE hDB, HNDLE hKey, int level, void (*callback)(HNDLE,HNDLE,KEY*,INT,void *), void *info);
INT EXPRT db_get_path(HNDLE hDB, HNDLE hKey, char *path, INT buf_size);
INT EXPRT db_delete_key(HNDLE database_handle, HNDLE key_handle, BOOL follow_links);
INT EXPRT db_enum_key(HNDLE hdb, HNDLE key_handle, INT index, HNDLE *subkey_handle);
INT EXPRT db_enum_link(HNDLE hdb, HNDLE key_handle, INT index, HNDLE *subkey_handle);
INT EXPRT db_get_key(HNDLE hdb, HNDLE key_handle, KEY *key);
INT EXPRT db_get_key_time(HNDLE hdb, HNDLE key_handle, DWORD *delta);
INT EXPRT db_rename_key(HNDLE hDB, HNDLE hKey, char *name);
INT EXPRT db_reorder_key(HNDLE hDB, HNDLE hKey, INT index);
INT EXPRT db_get_data(HNDLE hdb, HNDLE key_handle, void *data, INT *buf_size, DWORD type);
INT EXPRT db_get_data_index(HNDLE hDB, HNDLE hKey, void *data, INT *buf_size, INT index, DWORD type);
INT EXPRT db_set_data(HNDLE hdb, HNDLE hKey, void *data, INT buf_size, INT num_values, DWORD type);
INT EXPRT db_set_data_index(HNDLE hDB, HNDLE hKey, void *data, INT size, INT index, DWORD type);
INT EXPRT db_set_data_index2(HNDLE hDB, HNDLE hKey, void *data, INT size, INT index, DWORD type, BOOL bNotify);
INT EXPRT db_merge_data(HNDLE hDB, HNDLE hKeyRoot, char *name, void *data, INT data_size, INT num_values, INT type);
INT EXPRT db_set_mode(HNDLE hdb, HNDLE key_handle, WORD mode, BOOL recurse);
INT EXPRT db_create_record(HNDLE hdb, HNDLE hkey, char *name, char *init_str);
INT EXPRT db_open_record(HNDLE hdb, HNDLE hkey, void *ptr, INT rec_size, WORD access, void (*dispatcher)(INT,INT,void*), void *info);
INT EXPRT db_close_record(HNDLE hdb, HNDLE hkey);
INT EXPRT db_get_record(HNDLE hdb, HNDLE hKey, void *data, INT *buf_size, INT align);
INT EXPRT db_get_record_size(HNDLE hdb, HNDLE hKey, INT align, INT *buf_size);
INT EXPRT db_set_record(HNDLE hdb, HNDLE hKey, void *data, INT buf_size, INT align);
INT EXPRT db_send_changed_records(void);
INT EXPRT db_get_open_records(HNDLE hDB, HNDLE hKey, char *str, INT buf_size);

INT EXPRT db_add_open_record(HNDLE hDB, HNDLE hKey, WORD access_mode);
INT EXPRT db_remove_open_record(HNDLE hDB, HNDLE hKey);

INT EXPRT db_load(HNDLE hdb, HNDLE key_handle, char *filename, BOOL bRemote);
INT EXPRT db_save(HNDLE hdb, HNDLE key_handle, char *filename, BOOL bRemote);
INT EXPRT db_copy(HNDLE hDB, HNDLE hKey, char *buffer, INT *buffer_size, char *path);
INT EXPRT db_paste(HNDLE hDB, HNDLE hKeyRoot, char *buffer);
INT EXPRT db_save_struct(HNDLE hDB, HNDLE hKey, char *file_name, char *struct_name, BOOL append);
INT EXPRT db_save_string(HNDLE hDB, HNDLE hKey, char *file_name, char *string_name, BOOL append);

INT EXPRT db_sprintf(char* string, void *data, INT data_size, INT index, DWORD type);
INT EXPRT db_sprintfh(char* string, void *data, INT data_size, INT index, DWORD type);
INT EXPRT db_sscanf(char* string, void *data, INT *data_size, INT index, DWORD type);
char EXPRT *strcomb(char **list);

/*---- Bank routines ----*/
void EXPRT bk_init(void *pbh);
INT EXPRT bk_size(void *pbh);
void EXPRT bk_create(void *pbh, char *name, WORD type, void *pdata);
void EXPRT bk_close(void *pbh, void *pdata);
INT EXPRT bk_locate(void *pbh, char *name, void *pdata);
INT EXPRT bk_iterate(void *pbh, BANK **pbk, void *pdata);
INT EXPRT bk_swap(void *event, BOOL force);

/*---- RPC routines ----*/
INT EXPRT rpc_register_functions(RPC_LIST *new_list, INT (*func)(INT,void**));
INT EXPRT rpc_register_function(INT id, INT (*func)(INT,void**));
INT EXPRT rpc_get_option(HNDLE hConn, INT item);
INT EXPRT rpc_set_option(HNDLE hConn, INT item, INT value);
INT EXPRT rpc_set_name(char *name);
INT EXPRT rpc_get_name(char *name);
INT EXPRT rpc_is_remote(void);
INT EXPRT rpc_set_debug(void (*func)(char*));

INT EXPRT rpc_register_server(INT server_type, char *name, INT *port, INT (*func)(INT,void**));
INT EXPRT rpc_server_thread(void *pointer);
INT EXPRT rpc_server_shutdown(void);
INT EXPRT rpc_client_call(HNDLE hConn, const INT routine_id, ...);
INT EXPRT rpc_call(const INT routine_id, ...);
INT EXPRT rpc_tid_size(INT id);
char EXPRT *rpc_tid_name(INT id);
INT EXPRT rpc_server_connect(char *host_name, char *exp_name);
INT EXPRT rpc_client_connect(char *host_name, INT midas_port, HNDLE *hConnection);
INT EXPRT rpc_client_disconnect(HNDLE hConn, BOOL bShutdown);

INT EXPRT rpc_send_event(INT buffer_handle, void *source, INT buf_size, INT async_flag);
INT EXPRT rpc_flush_event(void);

void EXPRT rpc_get_convert_flags(INT *convert_flags);
void EXPRT rpc_convert_single(void *data, INT tid, INT flags, INT convert_flags);
void EXPRT rpc_convert_data(void *data, INT tid, INT flags, INT size, INT convert_flags);

/*---- system services ----*/
DWORD EXPRT ss_millitime(void);
DWORD EXPRT ss_time(void);
DWORD EXPRT ss_settime(DWORD seconds);
char EXPRT *ss_asctime(void);
INT EXPRT ss_sleep(INT millisec);
BOOL EXPRT ss_kbhit(void);

void EXPRT ss_clear_screen(void);
void EXPRT ss_printf(INT x, INT y, const char *format, ...);
void ss_set_screen_size(int x, int y);

char EXPRT *ss_getpass(char *prompt);
INT EXPRT ss_getchar(BOOL reset);
char EXPRT *ss_crypt(char *key, char *salt);
char EXPRT *ss_gets(char *string, int size);

/*---- direct io routines ----*/
INT EXPRT ss_directio_init(void);
INT EXPRT ss_directio_exit(void);
INT EXPRT ss_directio_give_port(INT start, INT end);
INT EXPRT ss_directio_lock_port(INT start, INT end);

/*---- tape routines ----*/
INT EXPRT ss_tape_open(char *path, INT oflag, INT *channel);
INT EXPRT ss_tape_close(INT channel);
INT EXPRT ss_tape_status(char *path);
INT EXPRT ss_tape_read(INT channel, void *pdata, INT *count);
INT EXPRT ss_tape_write(INT channel, void *pdata, INT count);
INT EXPRT ss_tape_write_eof(INT channel);
INT EXPRT ss_tape_fskip(INT channel, INT count);
INT EXPRT ss_tape_rskip(INT channel, INT count);
INT EXPRT ss_tape_rewind(INT channel);
INT EXPRT ss_tape_spool(INT channel);
INT EXPRT ss_tape_mount(INT channel);
INT EXPRT ss_tape_unmount(INT channel);

/*---- disk routines ----*/
double EXPRT ss_disk_free(char *path);
/*-PAA-*/
double EXPRT ss_file_size(char * path);
INT    EXPRT ss_file_remove(char * path);
INT    EXPRT ss_file_find(char * path, char * pattern, char **plist);
double EXPRT ss_disk_size(char *path);



/*---- history routines ----*/
INT EXPRT hs_set_path(char *path);
INT EXPRT hs_define_event(DWORD event_id, char *name, TAG *tag, DWORD size);
INT EXPRT hs_write_event(DWORD event_id, void *data, DWORD size);
INT EXPRT hs_count_events(DWORD ltime, DWORD *count);
INT EXPRT hs_enum_events(DWORD ltime, DEF_RECORD *index, DWORD *size);
INT EXPRT hs_count_tags(DWORD ltime, DWORD event_id, DWORD *count);
INT EXPRT hs_enum_tags(DWORD ltime, DWORD event_id, TAG *tag, DWORD *size);
INT EXPRT hs_read(DWORD event_id, DWORD start_time, DWORD end_time, DWORD interval, 
            char *tag_name, DWORD var_index, DWORD *time_buffer, DWORD *tbsize,
            void *data_buffer, DWORD *dbsize, DWORD *type, DWORD *n);
INT EXPRT hs_dump(DWORD event_id, DWORD start_time, DWORD end_time,
            DWORD interval);

#ifdef __cplusplus
}
#endif

#endif /* _MIDAS_H */
