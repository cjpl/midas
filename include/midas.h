/********************************************************************\

  Name:         MIDAS.H
  Created by:   Stefan Ritt

  Contents:     Type definitions and function declarations needed
                for MIDAS applications


  $Log$
  Revision 1.64  2000/04/17 17:07:05  pierre
  - new arg in hs_dump and hs_fdump (for mhist)
  - first round of doc++ comments

  Revision 1.63  2000/03/08 17:35:56  midas
  Version 1.8.0

  Revision 1.62  2000/03/02 21:57:31  midas
  Added new slow control commands

  Revision 1.61  2000/03/01 00:52:34  midas
  Added num_subevents into equipment, made events_sent a double

  Revision 1.60  2000/02/29 02:09:33  midas
  Added cm_is_ctrlc_pressed and cm_ack_ctrlc_pressed

  Revision 1.59  2000/02/25 22:22:45  midas
  Added abort feature in db_scan_tree

  Revision 1.58  2000/02/25 20:22:23  midas
  Added super-event scheme

  Revision 1.57  2000/02/24 22:33:16  midas
  Added deferred transitions

  Revision 1.56  2000/02/15 11:07:50  midas
  Changed GET_xxx to bit flags

  Revision 1.55  2000/02/09 08:03:27  midas
  Changed version to 1.7.1

  Revision 1.54  1999/12/08 00:29:34  pierre
  - Add CM_SET_LABEL for cd_gen/dd_epca (chn_acc)

  Revision 1.53  1999/11/19 09:49:36  midas
  Added watchdog_timeout to cm_set_client_info

  Revision 1.52  1999/11/09 13:17:00  midas
  Changed shared memory function names to ss_shm_xxx instead ss_xxx_shm

  Revision 1.51  1999/11/08 13:55:54  midas
  Added AT_xxx

  Revision 1.50  1999/11/04 15:53:54  midas
  Added some slow control commands

  Revision 1.49  1999/10/18 14:41:50  midas
  Use /programs/<name>/Watchdog timeout in all programs as timeout value. The
  default value can be submitted by calling cm_connect_experiment1(..., timeout)

  Revision 1.48  1999/10/08 15:07:04  midas
  Program check creates new internal alarm when triggered

  Revision 1.47  1999/10/07 13:17:34  midas
  Put a few EXPRT im msystem.h to make NT happy, updated NT makefile

  Revision 1.46  1999/10/06 08:56:33  midas
  Added /programs/xxx/required flag

  Revision 1.45  1999/09/27 13:49:03  midas
  Added bUnique parameter to cm_shutdown

  Revision 1.44  1999/09/27 12:54:55  midas
  Finished alarm system

  Revision 1.43  1999/09/23 12:45:48  midas
  Added 32 bit banks

  Revision 1.42  1999/09/22 15:39:36  midas
  Logger won't start run if disk file already exists

  Revision 1.41  1999/09/21 13:49:02  midas
  Added PROGRAM_INFO

  Revision 1.40  1999/09/17 15:06:46  midas
  Moved al_check into cm_yield() and rpc_server_thread

  Revision 1.39  1999/09/17 11:54:43  midas
  OS_LINUX & co automatically define OS_UNIX

  Revision 1.38  1999/09/17 11:48:04  midas
  Alarm system half finished

  Revision 1.37  1999/09/15 13:33:32  midas
  Added remote el_submit functionality

  Revision 1.36  1999/09/14 15:15:43  midas
  Moved el_xxx funtions into midas.c

  Revision 1.35  1999/09/14 11:46:04  midas
  Added EL_FIRST_MESSAGE

  Revision 1.34  1999/08/31 15:12:34  midas
  Added EL_LAST_MSG

  Revision 1.33  1999/08/26 15:18:22  midas
  Added EL_xxx codes

  Revision 1.32  1999/08/20 13:31:18  midas
  Analyzer saves and reloads online histos

  Revision 1.31  1999/08/03 11:15:07  midas
  Added bm_skip_event

  Revision 1.30  1999/07/15 07:35:12  midas
  Added ss_ctrlc_handler

  Revision 1.29  1999/07/13 08:24:27  midas
  Added ANA_TEST

  Revision 1.28  1999/06/28 12:01:33  midas
  Added hs_fdump

  Revision 1.27  1999/06/25 12:02:12  midas
  Added bk_delete function

  Revision 1.26  1999/06/23 09:58:28  midas
  Fixed typo

  Revision 1.25  1999/05/05 12:01:42  midas
  Added and modified hs_xxx functions

  Revision 1.24  1999/04/30 10:58:20  midas
  Added mode to rpc_set_debug

  Revision 1.23  1999/04/27 11:11:39  midas
  Added rpc_register_client

  Revision 1.22  1999/04/23 11:43:15  midas
  Increased version to 1.7.0

  Revision 1.21  1999/04/19 07:46:43  midas
  Added cm_msg_retrieve

  Revision 1.20  1999/04/15 09:59:09  midas
  Added key name to db_get_key_info

  Revision 1.19  1999/04/13 12:20:42  midas
  Added db_get_data1 (for Java)

  Revision 1.18  1999/04/08 15:23:46  midas
  Added CF_ASCII and db_get_key_info

  Revision 1.17  1999/03/02 10:00:07  midas
  Added ANA_CONTINUE and ANA_SKIP

  Revision 1.16  1999/02/18 13:06:27  midas
  Release 1.6.3

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

#define MIDAS_VERSION "1.8.0"

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

#if defined(OS_LINUX) || defined(OS_OSF1) || defined(OS_ULTRIX) || defined(OS_FREEBSD) || defined(OS_SOLARIS)
#define OS_UNIX
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
#define GET_ALL   (1<<0)      /* get all events (consume)           */
#define GET_SOME  (1<<1)      /* get as much as possible (sampling) */
#define GET_FARM  (1<<2)      /* distribute events over several
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
#define TR_DEFERRED   (1<<12)

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
/** LAM\_SOURCE
Code the LAM crate and LAM station into a bitwise register.
@memo MACRO Code LAM register with crate and station.
@param c Crate number
@param s Slot number
*/
#define LAM_SOURCE(c, s)         (c<<24 | ((s) & 0xFFFFFF))

/** LAM\_STATION
Code the Station number bitwise for the LAM source.
@memo MACRO Code LAM Station.
@param s Slot number
*/
#define LAM_STATION(s)           (1<<(s-1))

/** LAM\_SOURCE\_CRATE
Convert the coded LAM crate to Crate number.
@memo MACRO Convert coded Crate.
@param c coded crate
*/
#define LAM_SOURCE_CRATE(c)      (c>>24)

/** LAM\_SOURCE\_STATION
Convert the coded LAM station to Station number.
@memo MACRO Convert coded Sattion.
@param s Slot number
*/
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
#define CM_DEFERRED_TRANSITION      110
#define CM_TRANSITION_IN_PROGRESS   111

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
#define SS_FILE_EXISTS              424

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

/* ELog */
#define EL_SUCCESS                    1
#define EL_FILE_ERROR               902
#define EL_NO_MESSAGE               903
#define EL_TRUNCATED                904
#define EL_FIRST_MSG                905
#define EL_LAST_MSG                 906

/* Alarm */
#define AL_SUCCESS                    1
#define AL_INVALID_NAME            1002
#define AL_ERROR_ODB               1003
#define AL_RESET                   1004

/* Slow control commands */
#define CMD_INIT                    (1<<1)
#define CMD_EXIT                    (1<<2)
#define CMD_IDLE                    (1<<3)
#define CMD_SET                     (1<<4)
#define CMD_SET_ALL                 (1<<5)
#define CMD_GET                     (1<<6)
#define CMD_GET_ALL                 (1<<7)
#define CMD_GET_CURRENT             (1<<8)
#define CMD_GET_CURRENT_ALL         (1<<9)
#define CMD_SET_CURRENT_LIMIT      (1<<10)
#define CMD_GET_DEMAND             (1<<11)
#define CMD_GET_DEFAULT_NAME       (1<<12)
#define CMD_GET_DEFAULT_THRESHOLD  (1<<13)
#define CMD_SET_LABEL              (1<<14)
#define CMD_ENABLE_COMMAND         (1<<15)
#define CMD_DISABLE_COMMAND        (1<<16)

/* Commands for interrupt events */
#define CMD_INTERRUPT_ENABLE        100
#define CMD_INTERRUPT_DISABLE       101
#define CMD_INTERRUPT_ATTACH        102
#define CMD_INTERRUPT_DETACH        103

#define ANA_CONTINUE                  1
#define ANA_SKIP                      0

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
/** TRIGGER\_MASK
Extract or set the trigger mask field pointed by the argument.
@memo MACRO Trigger mask.
@param e pointer to the midas event (pevent)
*/
#define TRIGGER_MASK(e)    ((((EVENT_HEADER *) e)-1)->trigger_mask)

/** EVENT\_ID
Extract or set the event ID field pointed by the argument..
@memo MACRO event ID.
@param e pointer to the midas event (pevent)
*/
#define EVENT_ID(e)        ((((EVENT_HEADER *) e)-1)->event_id)

/** SERIAL\_NUMBER
Extract or set/reset the serial number field pointed by the argument.
@memo MACRO serial number.
@param e pointer to the midas event (pevent)
*/
#define SERIAL_NUMBER(e)   ((((EVENT_HEADER *) e)-1)->serial_number)

/** TIME\_STAMP
Extract or set/reset the time stamp field pointed by the argument.
@memo MACRO Time stamp.
@param e pointer to the midas event (pevent)
*/
#define TIME_STAMP(e)      ((((EVENT_HEADER *) e)-1)->time_stamp)
#define EVENT_SOURCE(e,o)  (* (INT*) (e+o))


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
  DWORD  cmd_disabled;                /* Mask of disabled commands         */
  void   *dd_info;                    /* Private info for device driver    */
} DEVICE_DRIVER;

typedef struct {
  WORD    event_id;                    /* Event ID associated with equipm.  */
  WORD    trigger_mask;                /* Trigger mask                      */
  char    buffer[NAME_LENGTH];         /* Event buffer to send events into  */
  INT     eq_type;                     /* One of EQ_xxx                     */
  INT     source;                      /* Event source (LAM/IRQ)            */
  char    format[8];                   /* Data format to produce            */
  BOOL    enabled;                     /* Enable flag                       */
  INT     read_on;                     /* Combination of Read-On flags RO_xxx */
  INT     period;                      /* Readout interval/Polling time in ms */
  double  event_limit;                 /* Stop run when limit is reached    */
  DWORD   num_subevents;               /* Number of events in super event */
  INT     history;                     /* Log history                       */
  char    frontend_host[NAME_LENGTH];  /* Host on which FE is running       */
  char    frontend_name[NAME_LENGTH];  /* Frontend name                     */
  char    frontend_file_name[256];     /* Source file used for user FE      */
} EQUIPMENT_INFO;

typedef struct {
  double events_sent;
  double events_per_sec;
  double kbytes_per_sec;
} EQUIPMENT_STATS;

typedef struct eqpmnt *PEQUIPMENT;

typedef struct eqpmnt {
  char   name[NAME_LENGTH];             /* Equipment name                  */
  EQUIPMENT_INFO info;                  /* From above                      */
  INT    (*readout)(char *, INT);       /* Pointer to user readout routine */
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
  DWORD  subevent_number;               /* subevent number                 */
  DWORD  odb_out;                       /* # updates FE -> ODB             */
  DWORD  odb_in;                        /* # updated ODB -> FE             */
  DWORD  bytes_sent;                    /* number of bytes sent            */
  DWORD  events_sent;                   /* number of events sent           */
  EQUIPMENT_STATS stats;
} EQUIPMENT;

/*---- Banks -------------------------------------------------------*/

#define BANK_FORMAT_VERSION     1
#define BANK_FORMAT_32BIT   (1<<4)

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
  char   name[4];
  DWORD  type;
  DWORD  data_size;
} BANK32;

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
  double  events_received;
  double  events_per_sec;
  double  events_written;
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

/*---- Tests -------------------------------------------------------*/

typedef struct {
  char  name[NAME_LENGTH];
  BOOL  registered;
  DWORD count;
  BOOL  value;
} ANA_TEST;

#define SET_TEST(t, v) { if (!t.registered) test_register(&t); t.value = (v); }
#define TEST(t) (t.value)

#ifdef DEFINE_TESTS
#define DEF_TEST(t) ANA_TEST t = { #t, 0, 0, FALSE };
#else
#define DEF_TEST(t) extern ANA_TEST t;
#endif

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
  INT       requested_transition;
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
"Requested transition = INT : 0",\
"Start time = STRING : [32] Tue Sep 09 15:04:42 1997",\
"Start time binary = DWORD : 0",\
"Stop time = STRING : [32] Tue Sep 09 15:04:42 1997",\
"Stop time binary = DWORD : 0",\
"",\
NULL }

/*---- Alarm system ------------------------------------------------*/

typedef struct {
  BOOL      auto_start;
  BOOL      auto_stop;
  BOOL      auto_restart;
  BOOL      required;
  char      start_command[256];
  char      alarm_class[32];
  DWORD     checked_last;
  DWORD     alarm_count;
  INT       watchdog_timeout;
} PROGRAM_INFO;

#define AT_INTERNAL   1
#define AT_PROGRAM    2
#define AT_EVALUATED  3

#define PROGRAM_INFO_STR(_name) char *_name[] = {\
"[.]",\
"Auto start = BOOL : n",\
"Auto stop = BOOL : n",\
"Auto restart = BOOL : n",\
"Required = BOOL : n",\
"Start command = STRING : [256] ",\
"Alarm Class = STRING : [32] ",\
"Checked last = DWORD : 0",\
"Alarm count = DWORD : 0",\
"Watchdog timeout = INT : 10000",\
"",\
NULL }

typedef struct {
  BOOL      write_system_message;
  BOOL      write_elog_message;
  INT       system_message_interval;
  DWORD     system_message_last;
  char      execute_command[256];
  INT       execute_interval;
  DWORD     execute_last;
  BOOL      stop_run;
} ALARM_CLASS;

#define ALARM_CLASS_STR(_name) char *_name[] = {\
"[.]",\
"Write system message = BOOL : y",\
"Write Elog message = BOOL : n",\
"System message interval = INT : 60",\
"System message last = DWORD : 0",\
"Execute command = STRING : [256] ",\
"Execute interval = INT : 0",\
"Execute last = DWORD : 0",\
"Stop run = BOOL : n",\
"",\
NULL }

typedef struct {
  BOOL      active;
  INT       triggered;
  INT       type;
  INT       check_interval;
  DWORD     checked_last;
  char      time_triggered_first[32];
  char      time_triggered_last[32];
  char      condition[256];
  char      alarm_class[32];
  char      alarm_message[80];
} ALARM;

#define ALARM_STR(_name) char *_name[] = {\
"[.]",\
"Active = BOOL : n",\
"Triggered = INT : 0",\
"Type = INT : 3",\
"Check interval = INT : 60",\
"Checked last = DWORD : 0",\
"Time triggered first = STRING : [32] ",\
"Time triggered last = STRING : [32] ",\
"Condition = STRING : [256] /Runinfo/Run number > 100",\
"Alarm Class = STRING : [32] Alarm",\
"Alarm Message = STRING : [80] Run number became too large",\
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
  RPC_PARAM     param[20];
  INT           (*dispatch)(INT,void**);
} RPC_LIST;

/* IDs allow for users */
#define RPC_MIN_ID    1
#define RPC_MAX_ID 9999

/* Data conversion flags */
#define CF_ENDIAN          (1<<0)
#define CF_IEEE2VAX        (1<<1)
#define CF_VAX2IEEE        (1<<2)
#define CF_ASCII           (1<<3)

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

/** cm\_get\_environment()
    \begin{description} 
    \item[Description:] Returns MIDAS environment variables.
    \item[Remarks:] This function can be used to evaluate the standard MIDAS
    environment variables before connecting to an experiment 
    (see \Ref{Environment variables}).
    The usual way is that the host name and experiment name are first derived
    from the environment variables MIDAS\_SERVER\_HOST and MIDAS\_EXPT\_NAME.
    They can then be superseded by command line parameters with -h and -e flags.
    \item[Example:]
    \begin{verbatim}
#include <stdio.h>
#include <midas.h>
main(int argc, char *argv[]) 
{
INT  status, i;
char host_name[256],exp_name[32]; 
 
  // get default values from environment 
  cm_get_environment(host_name, exp_name);
 
  // parse command line parameters
  for (i=1 ; i<argc ; i++) 
    {
    if (argv[i][0] == '-') 
      {
      if (i+1 >= argc || argv[i+1][0] == '-')
        goto usage;
      if (argv[i][1] == 'e')
        strcpy(exp_name, argv[++i]);
      else if (argv[i][1] == 'h')
        strcpy(host_name, argv[++i]);
      else 
        {
usage:
        printf("usage: test [-h Hostname] [-e Experiment]\n\n");
        return 1;
        }
      }
    }
  status = cm_connect_experiment(host_name, exp_name, "Test", NULL);
  if (status != CM_SUCCESS)
    return 1;
    ...do anyting...
  cm_disconnect_experiment();
}
\end{verbatim}
    \end{description}
    @memo Returns MIDAS environment variables.
    @param host\_name Contents of MIDAS\_SERVER\_HOST environment variable.
    @param exp\_name Contents of MIDAS\_EXPT\_NAME environment variable.
    @return CM\_SUCCESS
*/
INT EXPRT cm_get_environment(char *host_name, char *exp_name);
INT EXPRT cm_list_experiments(char *host_name, char exp_name[MAX_EXPERIMENT][NAME_LENGTH]);
INT EXPRT cm_select_experiment(char *host_name, char *exp_name);

/** cm\_connect\_experiment()
    \begin{description}
    \item[Description:] This function connects to an existing MIDAS experiment.
    This must be the first call in a MIDAS application.
    It opens three TCP connection to the remote host (one for RPC calls,
    one to send events and one for hot-link notifications from the remote host)
    and writes client information into the ODB under /System/Clients.
    \item[Remarks:] All MIDAS applications should evaluate the MIDAS\_SERVER\_HOST
    and MIDAS\_EXPT\_NAME environment variables as defaults to the host name and
    experiment name (see \Ref{Environment variables}).
    For that purpose, the function \Ref{cm_get_environment()}
    should be called prior to cm\_connect\_experiment(). If command line
    parameters -h and -e are used, the evaluation should be done between
    cm\_get\_environment() and cm\_connect\_experiment(). The function
    cm\_disconnect\_experiment() must be called before a MIDAS application exits.
    \item[Example:]
    \begin{verbatim}
#include <stdio.h>
#include <midas.h>
main(int argc, char *argv[]) 
{
INT  status, i;
char host_name[256],exp_name[32]; 
 
  // get default values from environment 
  cm_get_environment(host_name, exp_name);
 
  // parse command line parameters 
  for (i=1 ; i<argc ; i++) 
    {
    if (argv[i][0] == '-') 
      {
      if (i+1 >= argc || argv[i+1][0] == '-')
        goto usage;
      if (argv[i][1] == 'e')
        strcpy(exp_name, argv[++i]);
      else if (argv[i][1] == 'h')
        strcpy(host_name, argv[++i]);
      else 
        {
usage:
        printf("usage: test [-h Hostname] [-e Experiment]\n\n");
        return 1;
        }
      }
    }
  status = cm_connect_experiment(host_name, exp_name, "Test", NULL);
  if (status != CM_SUCCESS)
    return 1;
  ...do operations...
  cm_disconnect_experiment();
}
\end{verbatim}
    \end{description}
    @memo Connects to a MIDAS experiment.
    @param host\_name Specifies host to connect to. Must be a valid IP host name.
    The string can be empty ("") if to connect to the local computer. 
    @param exp\_name Specifies the experiment to connect to.
    If this string is empty, the number of defined experiments in exptab is checked.
    If only one experiment is defined, the function automatically connects to this
    one. If more than one experiment is defined, a list is presented and the user
    can interactively select one experiment.
    @param client\_name Client name of the calling program as it can be seen by
    others (like the scl command in ODBEdit). 
    @param func Callback function to read in a password if security has
    been enabled. In all command line applications this function is NULL which
    invokes an internal ss\_gets() function to read in a password.
    In windows environments (MS Windows, X Windows) a function can be supplied to
    open a dialog box and read in the password. The argument of this function must
    be the returned password.
    @return CM\_SUCCESS, CM\_UNDEF\_EXP, CM\_SET\_ERROR, RPC\_NET\_ERROR, \\
    CM\_VERSION\_MISMATCH MIDAS library version different on local and remote computer
*/
INT EXPRT cm_connect_experiment(char *host_name, char *exp_name, char *client_name, void (*func)(char*));

INT EXPRT cm_connect_experiment1(char *host_name, char *exp_name, char *client_name, void (*func)(char*), INT odb_size, INT watchdog_timeout);

/** cm\_disconnect\_experiment()
    \begin{description}
    \item[Description:] Disconnect from a MIDAS experiment.
    \item[Remarks:] Should be the last call to a MIDAS library function in an
    application before it exits. This function removes the client information
    from the ODB, disconnects all TCP connections and frees all internal
    allocated memory. See \Ref{cm_connect_experiment} for example.
    \end{description}
    @memo Disconnect from a MIDAS experiment.
    @return CM\_SUCCESS
*/
INT EXPRT cm_disconnect_experiment(void);

/** cm\_register\_transition
    \begin{description}
    \item[Description:] Registers a callback function for run transitions.
    \item[Remarks:] This function internally registers the transition callback
    function and publishes its request for transition notification by writing
    the transition bit to /System/Clients/<pid>/Transition Mask.
    Other clients making a transition scan the transition masks of all clients
    and call their transition callbacks via RPC.
    
    Clients can register for transitions (Start/Stop/Pause/Resume) or for
    notifications before or after a transition occurs
    (Pre-start/Post-start/Pre-stop/Post-stop). The logger for example opens
    the logging files on pre-start and closes them on post-stop.

    The callback function returns CM\_SUCCESS if it can perform the transition or
    a value larger than one in case of error. An error string can be copied
    into the error variable.

    \item[Example:] 
    The callback function will be called on transitions from inside the
    \Ref{cm_yield()} function which therefore must be contained in the
    main program loop.
    \begin{verbatim}
INT start(INT run_number, char *error) 
{
  if (<not ok>) 
    {
    strcpy(error, "Cannot start because ...");
    return 2;
    }
  printf("Starting run %d\n", run_number);
  return CM_SUCCESS;
}
main()
{
  ...
  cm_register_transition(TR_START, start);
  do 
    {
    status = cm_yield(1000);
    } while (status != RPC_SHUTDOWN &&
             status != SS_ABORT);
  ...
}
\end{verbatim}
    \end{description}
    @memo Registers a callback function for run transitions.
    @param transition Transition to register for. Can be TR\_PRESTART, TR\_START,
    TR\_POSTSTART, TR\_PRSTOP, TR\_STOP, TR\_POSTSTOP, TR\_PAUSE or TR\_RESUME. 
    @param func Callback function.
    @return CM\_SUCCESS
*/
INT EXPRT cm_register_transition(INT transition, INT (*func)(INT,char*));
INT EXPRT cm_register_deferred_transition(INT transition, BOOL (*func)(INT,BOOL));
INT EXPRT cm_check_deferred_transition(void);
INT EXPRT cm_transition(INT transition, INT run_number, char *error, INT strsize, INT async_flag);
INT EXPRT cm_register_server(void);
INT EXPRT cm_register_function(INT id, INT (*func)(INT,void**));
INT EXPRT cm_connect_client(char *client_name, HNDLE *hConn);
INT EXPRT cm_disconnect_client(HNDLE hConn, BOOL bShutdown);
INT EXPRT cm_set_experiment_database(HNDLE hDB, HNDLE hKeyClient);
INT EXPRT cm_get_experiment_database(HNDLE *hDB, HNDLE *hKeyClient);
INT EXPRT cm_set_experiment_mutex(INT mutex_alarm, INT mutex_elog);
INT EXPRT cm_get_experiment_mutex(INT *mutex_alarm, INT *mutex_elog);
INT EXPRT cm_set_client_info(HNDLE hDB, HNDLE *hKeyClient, char *host_name, char *client_name, INT computer_id, char *password, INT watchdog_timeout);
INT EXPRT cm_get_client_info(char *client_name);
INT EXPRT cm_set_watchdog_params(BOOL call_watchdog, INT timeout);
INT EXPRT cm_get_watchdog_params(BOOL *call_watchdog, INT *timeout);
INT EXPRT cm_get_watchdog_info(HNDLE hDB, char *client_name, INT *timeout, INT *last);
INT EXPRT cm_enable_watchdog(BOOL flag);
void EXPRT cm_watchdog(int);
INT EXPRT cm_shutdown(char *name, BOOL bUnique);
INT EXPRT cm_exist(char *name, BOOL bUnique);
INT EXPRT cm_cleanup(void);
INT EXPRT cm_yield(INT millisec);
INT EXPRT cm_execute(char *command, char *result, INT buf_size);
INT EXPRT cm_synchronize(DWORD *sec);
INT EXPRT cm_asctime(char *str, INT buf_size);
INT EXPRT cm_time(DWORD *time);
BOOL EXPRT cm_is_ctrlc_pressed(); 
void EXPRT cm_ack_ctrlc_pressed();

INT EXPRT cm_set_msg_print(INT system_mask, INT user_mask, int (*func)(const char*));
INT EXPRT cm_msg(INT message_type, char* filename, INT line, const char *routine, const char *format, ...);
INT EXPRT cm_msg_register(void (*func)(HNDLE,HNDLE,EVENT_HEADER*,void*));
INT EXPRT cm_msg_retrieve(char *message, INT *buf_size);

BOOL EXPRT equal_ustring(char *str1, char *str2);

/*---- buffer manager ----*/
/** bm\_open\_buffer()
    \begin{description}
    \item[Description:] Open an event buffer.
    \item[Remarks:] Two default buffers are created by the system.
    The "SYSTEM" buffer is used to
    exchange events and the "SYSMSG" buffer is used to exchange system messages.
    The name and size of the event buffers is defined in midas.h as
    EVENT\_BUFFER\_NAME and EVENT\_BUFFER\_SIZE.
    Following example opens the "SYSTEM" buffer, requests events with ID 1 and
    enters a main loop. Events are then received in process\_event()
    \item[Example:] \begin{verbatim}
    #include <stdio.h>
    #include "midas.h"
    void process_event(HNDLE hbuf, HNDLE request_id,
               EVENT_HEADER *pheader, void *pevent)
    {
      printf("Received event #%d\r",
      pheader->serial_number);
    }
    main() 
    {
      INT status, request_id;
      HNDLE hbuf;
      status = cm_connect_experiment("pc810", "Sample", "Simple Analyzer", NULL);
      if (status != CM_SUCCESS)
      return 1;
      bm_open_buffer(EVENT_BUFFER_NAME, EVENT_BUFFER_SIZE, &hbuf);
      bm_request_event(hbuf, 1, TRIGGER_ALL, GET_ALL, request_id, process_event);
    
      do 
      {
       status = cm_yield(1000);
      } while (status != RPC_SHUTDOWN && status != SS_ABORT);
      cm_disconnect_experiment();
      return 0;
    }
    \end{verbatim}
    \end{description}
    @memo open an event buffer.
    @param buffer_name Name of buffer
    @param buffer_size Size of buffer in bytes 
    @param buffer_handle Buffer handle returned by function
    @return BM\_SUCCESS, BM\_CREATED,\\
    BM\_NO\_SHM Shared memory cannot be created \\
    BM\_NO\_MUTEX Mutex cannot be created \\
    BM\_NO\_MEMORY Not enough memory to create buffer descriptor \\
    BM\_MEMSIZE\_MISMATCH Buffer size conflicts with an existing buffer of
    different size \\
    BM\_INVALID\_PARAM Invalid parameter
*/
INT EXPRT bm_open_buffer(char *buffer_name, INT buffer_size, INT *buffer_handle);

/** bm\_close\_buffer()
    \begin{description}
    \item[Description:] Closes an event buffer previously opened with \Ref{bm_open_buffer()}.
    \end{description}
    @memo close event buffer.
    @param buffer_handle buffer handle 
    @return BM\_SUCCESS, BM\_INVALID\_HANDLE
*/
INT EXPRT bm_close_buffer(INT buffer_handle);
INT EXPRT bm_close_all_buffers(void);
INT EXPRT bm_init_buffer_counters(INT buffer_handle);
INT EXPRT bm_get_buffer_info(INT buffer_handle, BUFFER_HEADER *buffer_header);
INT EXPRT bm_get_buffer_level(INT buffer_handle, INT *n_bytes);

/** bm\_set\_cache\_size()
    \begin{description}
    \item[Description:] Modifies buffer cache size.
    \item[Remarks:] Without a buffer cache, events are copied to/from the shared memory event
    by event. To protect processed from accessing the shared memory simultaneously,
    semaphores are used. Since semaphore operations are CPU consuming (typically
    50-100us) this can slow down the data transfer especially for small events.
    By using a cache the number of semaphore operations is reduced dramatically.
    Instead writing directly to the shared memory, the events are copied to a
    local cache buffer. When this buffer is full, it is copied to the shared
    memory in one operation. The same technique can be used when receiving events.
    
    The drawback of this method is that the events have to be copied twice, once to the
    cache and once from the cache to the shared memory. Therefore it can happen that the
    usage of a cache even slows down data throughput on a given environment (computer
    type, OS type, event size).
    The cache size has therefore be optimized manually to maximize data throughput.
    \end{description}
    @memo change cache size.
    @param buffer_handle buffer handle obtained via \Ref{bm_open_buffer()} 
    @param read_size cache size for reading events in bytes, zero for no cache 
    @param write_size cache size for writing events in bytes, zero for no cache
    @return BM\_SUCCESS, BM\_INVALID\_HANDLE, BM\_NO\_MEMORY, BM\_INVALID\_PARAM
*/
INT EXPRT bm_set_cache_size(INT buffer_handle, INT read_size, INT write_size);

/** bm\_compose\_event()
    \begin{description}
    \item[Description:] Compose a Midas event header.
    \item[Remarks:] An event header can usually be set-up manually or
    through this routine. If the data size of the event is not known when
    the header is composed, it can be set later with event\_header->data-size = <...>
    Following structure is created at the beginning of an event
    \begin{verbatim}
    typedef struct {
     short int     event_id; 
     short int     trigger_mask; 
     DWORD         serial_number; 
     DWORD         time_stamp; 
     DWORD         data_size; 
    } EVENT_HEADER;
    \end{verbatim}
    \item[Example:] \begin{verbatim}
     char event[1000];
     bm_compose_event((EVENT_HEADER *)event, 1, 0, 100, 1);
     *(event+sizeof(EVENT_HEADER)) = <...>
    \end{verbatim}
    \end{description}
    @memo compose the Midas event header.
    @param event_header pointer to the event header
    @param event_id event ID of the event
    @param trigger_mask trigger mask of the event
    @param size size if the data part of the event in bytes
    @param serial serial number 
    @return BM\_SUCCESS
*/
INT EXPRT bm_compose_event(EVENT_HEADER *event_header,
              short int event_id, short int trigger_mask,
              DWORD size, DWORD serial);
/** bm\_request\_event()
    \begin{description}
    \item[Description:] Place an event request based on certain characteristics.
    \item[Remarks:] Multiple event requests can be placed for each buffer, which are later
    identified by their request ID. They can contain different callback routines.
    Example see \Ref{bm_open_buffer} and \Ref{bm_receive_event}
    \item[Example:]
    \end{description}
    @memo event request.
    @param buffer_handle buffer handle obtained via bm\_open\_buffer() 
    @param event_id event ID for requested events. Use EVENTID\_ALL
    to receive events with any ID. 
    @param trigger_mask trigger mask for requested events.
    The requested events must have at least one bit in its
    trigger mask common with the requested trigger mask. Use TRIGGER\_ALL to
    receive events with any trigger mask. 
    @param sampling_type specifies how many events to receive.
    A value of GET\_ALL receives all events which
    match the specified event ID and trigger mask. If the events are consumed slower than
    produced, the producer is automatically slowed down. A value of GET\_SOME
    receives as much events as possible without slowing down the producer. GET\_ALL is
    typically used by the logger, while GET\_SOME is typically used by analyzers. 
    @param request_id request ID returned by the function.
    This ID is passed to the callback routine and must
    be used in the bm\_delete\_request() routine. 
    @param func allback routine which gets called when an event of the
    specified type is received.
    @return BM\_SUCCESS, BM\_INVALID\_HANDLE\\
    BM\_NO\_MEMORY  too many requests. The value
    MAX\_EVENT\_REQUESTS in midas.h should be
    increased. 
*/

INT EXPRT bm_request_event(INT buffer_handle, short int event_id,
              short int trigger_mask,
              INT sampling_type, INT *request_id, 
              void (*func)(HNDLE,HNDLE,EVENT_HEADER*,void*));
INT EXPRT bm_add_event_request(INT buffer_handle, short int event_id,
              short int trigger_mask,
              INT sampling_type, void (*func)(HNDLE,HNDLE,EVENT_HEADER*,void*),
              INT request_id);

/** bm\_delete\_request()
    \begin{description}
    \item[Description:] Deletes an event request previously done with \Ref{bm_request_event()}.
    \item[Remarks:] When an event request gets deleted, events of that requested type are not
    received any more. When a buffer is closed via bm\_close\_buffer(), all
    event requests from that buffer are deleted automatically
    \item[Example:]
    \end{description}
    @memo delete event request.
    @param request_id request identifier given by \Ref{bm_request_event()}
    @return BM\_SUCCESS, BM\_INVALID\_HANDLE
*/

INT EXPRT bm_delete_request(INT request_id);
/** bm\_send\_event()
    \begin{description}
    \item[Description:] Sends an event to a buffer.
    \item[Remarks:] This function check if the buffer has enough space for the
    event, then copies the event to the buffer in shared memory.
    If clients have requests for the event, they are notified via an UDP packet.
    \item[Example:]
    \begin{verbatim}
    char event[1000];
    // create event with ID 1, trigger mask 0, size 100 bytes and serial number 1 
    bm_compose_event((EVENT_HEADER *) event, 1, 0, 100, 1);

    // set first byte of event 
    *(event+sizeof(EVENT_HEADER)) = <...>
    #include <stdio.h>
    #include "midas.h"
    main() 
    {
     INT status, i;
     HNDLE hbuf;
     char event[1000];
     status = cm_connect_experiment("", "Sample", "Producer", NULL);
     if (status != CM_SUCCESS)
     return 1;
     bm_open_buffer(EVENT_BUFFER_NAME, EVENT_BUFFER_SIZE, &hbuf);
    
     // create event with ID 1, trigger mask 0, size 100 bytes and serial number 1 
     bm_compose_event((EVENT_HEADER *) event, 1, 0, 100, 1);
    
     // set event data 
     for (i=0 ; i<100 ; i++)
     *(event+sizeof(EVENT_HEADER)+i) = i;
     // send event 
     bm_send_event(hbuf, event, 100+sizeof(EVENT_HEADER), SYNC);
     cm_disconnect_experiment();
     return 0;
    }
    \end{verbatim}
    \end{description}
    @memo send event to buffer.
    @param buffer_handle Buffer handle obtained via bm\_open\_buffer() 
    @param event Address of event buffer 
    @param buf_size Size of event including event header in bytes 
    @param async_flag Synchronous/asynchronous flag. If FALSE, the function
    blocks if the buffer has not enough free space to receive the event.
    If TRUE, the function returns immediately with a
    value of BM\_ASYNC\_RETURN without writing the event to the buffer
    @return BM\_SUCCESS, BM\_INVALID\_HANDLE, BM\_INVALID\_PARAM\\
    BM\_ASYNC\_RETURN Routine called with async\_flag == TRUE and
    buffer has not enough space to receive event\\
    BM\_NO\_MEMORY   Event is too large for network buffer or event buffer.
    One has to increase MAX\_EVENT\_SIZE or EVENT\_BUFFER\_SIZE in midas.h and
    recompile. 
*/
INT EXPRT bm_send_event(INT buffer_handle, void *event, INT buf_size,
                  INT async_flag);

/** bm\_receive\_event()
    \begin{description}
    \item[Description:] Receives events directly.
    \item[Remarks:] This function is an alternative way to receive events without
    a main loop. It can be used in analysis systems which actively receive events, rather
    than using callbacks. A analysis package could for example contain its own
    command line interface. A command
    like "receive 1000 events" could make it necessary to call bm\_receive\_event()
    1000 times in a row to receive these events and then return back to the command line
    prompt.
    \item[Example:] The according bm\_request\_event() call contains NULL as the callback
    routine to indicate that bm\_receive\_event() is called to receive events.
    \begin{verbatim}
    #include <stdio.h>
    #include "midas.h"
    void process_event(EVENT_HEADER *pheader)
    {
     printf("Received event #%d\r",
     pheader->serial_number);
    }
    main() 
    {
    INT status, request_id;
    HNDLE hbuf;
    char event_buffer[1000];
    status = cm_connect_experiment("", "Sample",
    "Simple Analyzer", NULL);
    if (status != CM_SUCCESS)
     return 1;
    bm_open_buffer(EVENT_BUFFER_NAME, EVENT_BUFFER_SIZE, &hbuf);
    bm_request_event(hbuf, 1, TRIGGER_ALL, GET_ALL, request_id, NULL);
    
    do 
    {
     size = sizeof(event_buffer);
     status = bm_receive_event(hbuf, event_buffer, &size, ASYNC);
    if (status == CM_SUCCESS)
     process_event((EVENT_HEADER *) event_buffer);
     <...do something else...>
     status = cm_yield(0);
    } while (status != RPC_SHUTDOWN &&
    status != SS_ABORT);
    cm_disconnect_experiment();
    return 0;
    }
    \end{verbatim}
    \end{description}
    @memo receive event from buffer.
    @param buffer_handle buffer handle
    @param destination destination address where event is written to
    @param buf_size size of destination buffer on input, size of event plus
    header on return. 
    @param async_flag Synchronous/asynchronous flag. If FALSE, the function
    blocks if no event is available. If TRUE, the function returns immediately
    with a value of BM\_ASYNC\_RETURN without receiving any event. \\
    @return BM\_SUCCESS, BM\_INVALID\_HANDLE \\
    BM\_TRUNCATED   The event is larger than the destination buffer and was
                   therefore truncated \\
    BM\_ASYNC\_RETURN No event available
*/
INT EXPRT bm_receive_event(INT buffer_handle, void *destination,
                          INT *buf_size, INT async_flag);
INT EXPRT bm_skip_event(INT buffer_handle);

/** bm\_flush\_cache()
    \begin{description}
    \item[Description:] Empty write cache.
    \item[Remarks:] This function should be used if events in the write cache
    should be visible to the consumers immediately. It should be called at the
    end of each run, otherwise events could be kept in the write buffer and will
    flow to the data of the next run.
    \item[Example:]
    \end{description}
    @memo empty write cache.
    @param buffer_handle Buffer handle obtained via \Ref{bm_open_buffer()}
    @param async_flag Synchronous/asynchronous flag.
    If FALSE, the function blocks if the buffer has not
    enough free space to receive the full cache. If TRUE, the function returns
    immediately with a value of BM\_ASYNC\_RETURN without writing the cache.
    @return BM\_SUCCESS, BM\_INVALID\_HANDLE\\
    BM\_ASYNC\_RETURN Routine called with async\_flag == TRUE
    and buffer has not enough space to receive cache \\
    BM\_NO\_MEMORY Event is too large for network buffer or event buffer.
    One has to increase MAX\_EVENT\_SIZE or EVENT\_BUFFER\_SIZE in midas.h and recompile.
*/
INT EXPRT bm_flush_cache(INT buffer_handle, INT async_flag);
INT EXPRT bm_poll_event(INT flag);

/** bm\_empty\_buffers()
    \begin{description}
    \item[Description:] Clears event buffer and cache.
    \item[Remarks:]If an event buffer is large and a consumer is slow in analyzing
    events, events are usually received some time after they are produced.
    This effect is even more experienced if a read cache is used
    (via \Ref{bm_set_cache_size()}).
    When changes to the hardware are made in the experience, the consumer will then
    still analyze old events before any new event which reflects the hardware change.
    Users can be fooled by looking at histograms which reflect the hardware change
    many seconds after they have been made.
    
    To overcome this potential problem, the analyzer can call
    bm\_empty\_buffers() just after the hardware change has been made which
    skips all old events contained in event buffers and read caches.
    Technically this is done by forwarding the read pointer of the client.
    No events are really deleted, they are still visible to other clients like
    the logger.
    
    Note that the front-end also contains write buffers which can delay the
    delivery of events.
    The standard front-end framework mfe.c reduces this effect by flushing
    all buffers once every second. 
    \item[Example:]
    \end{description}
    @memo empty event buffer.
    @param void
    @return BM\_SUCCESS
*/
INT EXPRT bm_empty_buffers(void);

/*---- online database functions -----*/
INT EXPRT db_open_database(char *database_name, INT database_size, HNDLE *hdb, char *client_name);
INT EXPRT db_close_database(HNDLE database_handle);
INT EXPRT db_close_all_databases(void);
INT EXPRT db_protect_database(HNDLE database_handle);

INT EXPRT db_create_key(HNDLE hdb, HNDLE key_handle, char *key_name, DWORD type);
INT EXPRT db_create_link(HNDLE hdb, HNDLE key_handle, char *link_name, char *destination);
INT EXPRT db_set_value(HNDLE hdb, HNDLE hKeyRoot, char *key_name, void *data, INT size, INT num_values, DWORD type);
INT EXPRT db_get_value(HNDLE hdb, HNDLE hKeyRoot, char *key_name, void *data, INT *size, DWORD type);
INT EXPRT db_find_key(HNDLE hdb, HNDLE hkey, char *name, HNDLE *hsubkey);
INT EXPRT db_find_link(HNDLE hDB, HNDLE hKey, char *key_name, HNDLE *subhKey);
INT EXPRT db_find_key1(HNDLE hdb, HNDLE hkey, char *name, HNDLE *hsubkey);
INT EXPRT db_find_link1(HNDLE hDB, HNDLE hKey, char *key_name, HNDLE *subhKey);
INT EXPRT db_scan_tree(HNDLE hDB, HNDLE hKey, int level, INT (*callback)(HNDLE,HNDLE,KEY*,INT,void *), void *info);
INT EXPRT db_scan_tree_link(HNDLE hDB, HNDLE hKey, int level, void (*callback)(HNDLE,HNDLE,KEY*,INT,void *), void *info);
INT EXPRT db_get_path(HNDLE hDB, HNDLE hKey, char *path, INT buf_size);
INT EXPRT db_delete_key(HNDLE database_handle, HNDLE key_handle, BOOL follow_links);
INT EXPRT db_enum_key(HNDLE hdb, HNDLE key_handle, INT index, HNDLE *subkey_handle);
INT EXPRT db_enum_link(HNDLE hdb, HNDLE key_handle, INT index, HNDLE *subkey_handle);
INT EXPRT db_get_key(HNDLE hdb, HNDLE key_handle, KEY *key);
INT EXPRT db_get_key_info(HNDLE hDB, HNDLE hKey, char *name, INT name_size, INT *type, INT *num_values, INT *item_size);
INT EXPRT db_get_key_time(HNDLE hdb, HNDLE key_handle, DWORD *delta);
INT EXPRT db_rename_key(HNDLE hDB, HNDLE hKey, char *name);
INT EXPRT db_reorder_key(HNDLE hDB, HNDLE hKey, INT index);
INT EXPRT db_get_data(HNDLE hdb, HNDLE key_handle, void *data, INT *buf_size, DWORD type);
INT EXPRT db_get_data1(HNDLE hDB, HNDLE hKey, void *data, INT *buf_size, DWORD type, INT *num_values);
INT EXPRT db_get_data_index(HNDLE hDB, HNDLE hKey, void *data, INT *buf_size, INT index, DWORD type);
INT EXPRT db_set_data(HNDLE hdb, HNDLE hKey, void *data, INT buf_size, INT num_values, DWORD type);
INT EXPRT db_set_data_index(HNDLE hDB, HNDLE hKey, void *data, INT size, INT index, DWORD type);
INT EXPRT db_set_data_index2(HNDLE hDB, HNDLE hKey, void *data, INT size, INT index, DWORD type, BOOL bNotify);
INT EXPRT db_set_num_values(HNDLE hDB, HNDLE hKey, INT num_values);
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
/** bk\_init()
    \begin{description}
    \item[Description:] Initializes an event for Midas banks structure.
    \item[Remarks:] Before banks can be created in an event, bk\_init()
    has to be called first. 
    \end{description}
    @memo Initialize an event.
    @param pbh pointer to the area of event 
    @return void
*/
void EXPRT bk_init(void *pbh);

/** bk\_init32()
    \begin{description}
    \item[Description:] Initializes an event for Midas banks structure for large.
    bank size (> 32KBytes)
    \item[Remarks:] Before banks can be created in an event, bk\_init32()
    has to be called first. 
    \end{description}
    @memo Initialize an event (> 32KBytes).
    @param pbh pointer to the area of event 
    @return void
*/
void EXPRT bk_init32(void *event);

BOOL EXPRT bk_is32(void *event);

/** bk\_size()
    \begin{description}
    \item[Description:] Returns the size of an event containing banks.
    \item[Remarks:] The total size of an event is the value returned by
    bk\_size() plus the size of the event header (sizeof(EVENT\_HEADER)). 
    \end{description}
    @memo compute event size.
    @param pbh pointer to the area of event 
    @return number of bytes contained in data area of event 
*/
INT EXPRT bk_size(void *pbh);

/** bk\_create()
    \begin{description}
    \item[Description:] Create a Midas bank.
    \item[Remarks:] The data pointer pdata must be used as an address to fill a
    bank. It is incremented with every value written to the bank and finally points
    to a location just after the last byte of the bank. It is then passed to
    the function \Ref{bk_close()} to finish the bank creation.
    \item[Example:] \begin{verbatim}
    INT *pdata;
    bk_init(pevent);
    bk_create(pevent, "ADC0", TID_INT, &pdata);
    *pdata++ = 123
    *pdata++ = 456
    bk_close(pevent, pdata);
    \end{verbatim}
    \end{description}
    @memo Create a bank.
    @param pbh pointer to the data area
    @param name of the bank, must be exactly 4 charaters
    @param type type of bank, one of the \Ref{Midas Data Types} values defined in
    midas.h 
    @param pdata pointer to the data area of the newly created bank
    @return void
*/
void EXPRT bk_create(void *pbh, char *name, WORD type, void *pdata);
int EXPRT bk_delete(void *event, char *name);

/** bk\_close()
    \begin{description}
    \item[Description:] Close the Midas bank priviously created by \Ref{bk_create}.
    \item[Remarks:] The data pointer pdata must be obtained by \Ref{bk_create()} and
    used as an address to fill a bank. It is incremented with every value written
    to the bank and finally points to a location just after the last byte of the
    bank. It is then passed to bk\_close() to finish the bank creation
    \end{description}
    @memo Close bank.
    @param pbh pointer to current composed event
    @param pdata  pointer to the data
    @return void
 */
void EXPRT bk_close(void *pbh, void *pdata);

/** bk\_locate()
    \begin{description}
    \item[Description:] Locates a MIDAS bank of given name inside an event.
    \item[Remarks:]
    \item[Example:]
    \end{description}
    \begin{verbatim}
    \end{verbatim}
    @memo loate a bank in event.
    @param pbh pointer to current composed event
    @param name bank name to look for
    @param pdata pointer to data area of bank, NULL if bank not found
    @return number of values inside the bank
*/
INT EXPRT bk_locate(void *pbh, char *name, void *pdata);

/** bk\_iterate()
    \begin{description}
    \item[Description:] Iterates through banks inside an event.
    \item[Remarks:] The function can be used to enumerate all banks of an event.
    The returned pointer to the bank header has following structure: 
    \begin{verbatim}
    typedef struct {
    char   name[4];
    WORD   type;
    WORD   data_size;
    } BANK;
    \end{verbatim}
    where type is a TID\_xxx value and data\_size the size of the bank in bytes.
    \item[Example:] \begin{verbatim}
    BANK *pbk;
    INT  size;
    void *pdata;
    char name[5];
    pbk = NULL;
    do
    {
     size = bk_iterate(event, &pbk, &pdata);
     if (pbk == NULL)
      break;
     *((DWORD *)name) = *((DWORD *)(pbk)->name);
     name[4] = 0;
     printf("bank %s found\n", name);
    } while(TRUE);
    \end{verbatim}
    \end{description}
    @memo Retrieve banks pointer from current event.
    @param pbh Pointer to data area of event
    @param pbk pointer to the bank header, must be NULL for the first call to this function
    @param pdata Pointer to the bank header, must be NULL for the first
    call to this function 
    @return Size of bank in bytes
*/
INT EXPRT bk_iterate(void *pbh, BANK **pbk, void *pdata);
INT EXPRT bk_iterate32(void *pbh, BANK32 **pbk, void *pdata);

/** bk\_swap()
    \begin{description}
    \item[Description:] Swaps bytes from little endian to big endian or vice versa for a whole event.
    \item[Remarks:] An event contains a flag which is set by bk\_init() to identify
    the endian format of an event. If force is FALSE, this flag is evaluated and
    the event is only swapped if it is in the "wrong" format for this system.
    An event can be swapped to the "wrong" format on purpose for example by a
    front-end which wants to produce events in a "right" format for a back-end
    analyzer which has different byte ordering. 
    \end{description}
    @memo Swap the content of an event.
    @param event pointer to data area of event
    @param force If TRUE, the event is always swapped, if FALSE, the event
    is only swapped if it is in the wrong format.
    @return 1==event has been swap, 0==event has not been swapped.
*/
INT EXPRT bk_swap(void *event, BOOL force);

/*---- RPC routines ----*/
INT EXPRT rpc_register_functions(RPC_LIST *new_list, INT (*func)(INT,void**));
INT EXPRT rpc_register_function(INT id, INT (*func)(INT,void**));
INT EXPRT rpc_get_option(HNDLE hConn, INT item);
INT EXPRT rpc_set_option(HNDLE hConn, INT item, INT value);
INT EXPRT rpc_set_name(char *name);
INT EXPRT rpc_get_name(char *name);
INT EXPRT rpc_is_remote(void);
INT EXPRT rpc_set_debug(void (*func)(char*), INT mode);

INT EXPRT rpc_register_server(INT server_type, char *name, INT *port, INT (*func)(INT,void**));
INT EXPRT rpc_register_client(char *name, RPC_LIST *list);
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

void EXPRT *ss_ctrlc_handler(void (*func)(int));

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
INT EXPRT hs_enum_events(DWORD ltime, char *event_name, DWORD *name_size, INT  event_id[], DWORD *id_size);
INT EXPRT hs_count_vars(DWORD ltime, DWORD event_id, DWORD *count);
INT EXPRT hs_enum_vars(DWORD ltime, DWORD event_id, char *var_name, DWORD *size);
INT EXPRT hs_get_var(DWORD ltime, DWORD event_id, char *var_name, DWORD *type, INT *n_data);
INT EXPRT hs_get_event_id(DWORD ltime, char *name, DWORD *id);
INT EXPRT hs_read(DWORD event_id, DWORD start_time, DWORD end_time, DWORD interval, 
            char *tag_name, DWORD var_index, DWORD *time_buffer, DWORD *tbsize,
            void *data_buffer, DWORD *dbsize, DWORD *type, DWORD *n);
INT EXPRT hs_dump(DWORD event_id, DWORD start_time, DWORD end_time,
            DWORD interval, BOOL binary_time);
INT EXPRT hs_fdump(char *file_name, DWORD id, BOOL binary_time);

/*---- ELog functions ----*/
INT EXPRT el_retrieve(char *tag, char *date, int *run, char *author, char *type, 
                char *system, char *subject, char *text, int *textsize, 
                char *orig_tag, char *reply_tag, 
                char *attachment1, char *attachment2, char *attachment3, char *encoding);
INT EXPRT el_submit(int run, char *author, char *type, char *system, char *subject, 
              char *text, char *reply_to, char *encoding, 
              char *afilename1, char *buffer1, INT buffer_size1, 
              char *afilename2, char *buffer2, INT buffer_size2, 
              char *afilename3, char *buffer3, INT buffer_size3, 
              char *tag, INT tag_size);
INT EXPRT el_search_message(char *tag, int *fh, BOOL walk);
INT EXPRT el_search_run(int run, char *return_tag);

/*---- Alarm functions ----*/
INT EXPRT al_check();
INT EXPRT al_trigger_alarm(char *alarm_name, char *alarm_message, char *default_class, char *cond_str, INT type);
INT EXPRT al_trigger_class(char *alarm_class, char *alarm_message, BOOL first);
INT EXPRT al_reset_alarm(char *alarm_name);
BOOL EXPRT al_evaluate_condition(char *condition, char *value);

/*---- analyzer functions ----*/
void EXPRT test_register(ANA_TEST *t);
void EXPRT add_data_dir(char *result, char *file);
                                                  
#ifdef __cplusplus
}
#endif

#endif /* _MIDAS_H */
