/********************************************************************\

  Name:         MIDAS.C
  Created by:   Stefan Ritt

  Contents:     MIDAS main library funcitons

  $Log$
  Revision 1.108  2000/03/17 10:55:15  midas
  Don't trigger internal alarms if alarm system is off

  Revision 1.107  2000/03/04 00:42:29  midas
  Delete elog & alarm mutexes correctly

  Revision 1.106  2000/03/03 22:46:07  midas
  Remove elog and alarm mutex on exit

  Revision 1.105  2000/03/03 01:45:13  midas
  Added web password for mhttpd, added webpasswd command in odbedit

  Revision 1.104  2000/03/01 23:06:19  midas
  bk_xxx functions now don't use global variable _pbk

  Revision 1.103  2000/02/29 21:59:05  midas
  Fixec bug with order of actions in cm_transition

  Revision 1.102  2000/02/29 02:10:26  midas
  Added cm_is_ctrlc_pressed and cm_ack_ctrlc_pressed

  Revision 1.101  2000/02/25 22:49:29  midas
  Increased timeouts

  Revision 1.100  2000/02/25 22:19:09  midas
  Improved Ctrl-C handling

  Revision 1.99  2000/02/24 23:58:29  midas
  Fixed problem with _requested_transition being update by hotlink too late

  Revision 1.98  2000/02/24 22:29:25  midas
  Added deferred transitions

  Revision 1.97  2000/02/23 21:07:44  midas
  Changed spaces and tabulators

  Revision 1.96  2000/02/15 11:07:51  midas
  Changed GET_xxx to bit flags

  Revision 1.95  2000/02/09 08:03:52  midas
  Fixed bracket indention

  Revision 1.94  1999/12/08 16:10:43  midas
  Fixed another watchdog bug causing remote clients to crash

  Revision 1.93  1999/12/08 11:44:25  midas
  Fixed bug with watchdog timeout

  Revision 1.92  1999/12/08 10:00:41  midas
  Changed error string to single line

  Revision 1.91  1999/12/08 00:25:20  pierre
  - add cm_get_path in cm_msg_retrieve for midas.log location.
  - mod dm_buffer_create for arg "max user event size".
  - fix dm_area_flush.
  - fix other compilation warnings for OSF/1

  Revision 1.90  1999/11/26 08:31:58  midas
  midas.log is now places in the same directory as the .SHM files in case
  there is no data dir in the ODB

  Revision 1.89  1999/11/25 13:29:55  midas
  Fixed bug in cm_msg_retrieve

  Revision 1.88  1999/11/23 15:52:40  midas
  If an event is larger than the buffer read or write cache, it bypasses the
  cache.

  Revision 1.87  1999/11/19 09:49:58  midas
  Fixed bug with wrong default watchdog timeout in cm_connect_experiment1

  Revision 1.86  1999/11/12 10:04:59  midas
  Fixed bug with WATCHDOG_INTERVAL

  Revision 1.85  1999/11/10 15:05:16  midas
  Did some additional database locking

  Revision 1.84  1999/11/10 13:56:12  midas
  Alarm record only gets created when old one mismatches

  Revision 1.83  1999/11/10 10:39:11  midas
  Changed initialization of alarms

  Revision 1.82  1999/11/10 08:30:44  midas
  Fixed bug when editing the last elog message

  Revision 1.81  1999/11/09 14:44:08  midas
  Changed ODB locking in cm_cleanup

  Revision 1.80  1999/11/09 13:17:25  midas
  Added secure ODB feature

  Revision 1.79  1999/11/08 13:56:09  midas
  Added different alarm types

  Revision 1.78  1999/10/27 15:13:56  midas
  Added "access(<key>)" in alarm system

  Revision 1.77  1999/10/27 13:37:57  midas
  Added event size check in bm_send_event

  Revision 1.76  1999/10/18 15:52:12  midas
  Use "alarm count" to declare programs dead if inactive for 5 minutes

  Revision 1.75  1999/10/18 14:41:51  midas
  Use /programs/<name>/Watchdog timeout in all programs as timeout value. The
  default value can be submitted by calling cm_connect_experiment1(..., timeout)

  Revision 1.74  1999/10/13 08:03:28  midas
  Fixed bug displaying executed message as %d

  Revision 1.73  1999/10/11 14:14:03  midas
  Use ss_system in certain places

  Revision 1.72  1999/10/11 13:01:22  midas
  Produce system message when executing an alarm script

  Revision 1.71  1999/10/08 22:15:03  midas
  Added ftruncate for LINUX

  Revision 1.70  1999/10/08 22:00:30  midas
  Finished editing of elog messages

  Revision 1.69  1999/10/08 15:07:06  midas
  Program check creates new internal alarm when triggered

  Revision 1.68  1999/10/08 13:21:20  midas
  Alarm system disabled when running offline

  Revision 1.67  1999/10/07 13:50:49  midas
  Fixed bug with date in el_submit

  Revision 1.66  1999/10/07 13:31:18  midas
  Fixed truncated date in el_submit, cut off @host in author search

  Revision 1.65  1999/10/06 06:56:02  midas
  Include weekday in elog

  Revision 1.64  1999/10/05 13:16:10  midas
  Added global alarm flag "/alarms/alarm system active"

  Revision 1.63  1999/10/04 11:54:14  midas
  Submit full alarm string to execute command

  Revision 1.62  1999/09/30 22:59:06  pierre
  - fix bk_close for BK32

  Revision 1.61  1999/09/29 19:23:33  pierre
  - Fix bk_iterate,swap,locate for bank32

  Revision 1.60  1999/09/27 13:49:04  midas
  Added bUnique parameter to cm_shutdown

  Revision 1.59  1999/09/27 12:54:08  midas
  Finished alarm system

  Revision 1.58  1999/09/27 08:56:53  midas
  Fixed bug with missing run number in elog

  Revision 1.57  1999/09/23 14:00:48  midas
  Used capital names for mutexes

  Revision 1.56  1999/09/23 12:45:49  midas
  Added 32 bit banks

  Revision 1.55  1999/09/22 08:57:08  midas
  Implemented auto start and auto stop in /programs

  Revision 1.54  1999/09/21 14:57:39  midas
  Added "execute on start/stop" under /programs

  Revision 1.53  1999/09/21 14:15:04  midas
  Replaces cm_execute by system()

  Revision 1.52  1999/09/21 13:48:04  midas
  Added programs check in al_check

  Revision 1.51  1999/09/17 15:59:03  midas
  Added internal alarms

  Revision 1.50  1999/09/17 15:06:48  midas
  Moved al_check into cm_yield() and rpc_server_thread

  Revision 1.49  1999/09/17 11:50:53  midas
  Added al_xxx functions

  Revision 1.48  1999/09/17 11:48:06  midas
  Alarm system half finished

  Revision 1.47  1999/09/15 13:33:34  midas
  Added remote el_submit functionality

  Revision 1.46  1999/09/14 15:15:45  midas
  Moved el_xxx funtions into midas.c

  Revision 1.45  1999/09/13 11:08:24  midas
  Check NULL as experiment in cm_connect_experiment

  Revision 1.44  1999/09/10 06:11:15  midas
  Used %100 for year in tms structure

  Revision 1.43  1999/08/03 14:41:09  midas
  Lock buffer in bm_skip_event

  Revision 1.42  1999/08/03 11:15:07  midas
  Added bm_skip_event

  Revision 1.41  1999/07/21 09:22:01  midas
  Added Ctrl-C handler to cm_connect_experiment and cm_yield

  Revision 1.40  1999/06/28 12:01:21  midas
  Added hs_fdump

  Revision 1.39  1999/06/25 12:01:54  midas
  Added bk_delete function

  Revision 1.38  1999/06/23 09:36:24  midas
  - Fixed "too many connections" bug
  - incorporated PAAs dm_xxx changes

  Revision 1.37  1999/05/05 12:02:33  midas
  Added and modified history functions, added db_set_num_values

  Revision 1.36  1999/04/30 14:22:01  midas
  Send buffer name via bm_notify_client to java application

  Revision 1.35  1999/04/30 13:19:54  midas
  Changed inter-process communication (ss_resume, bm_notify_clients, etc)
  to strings so that server process can receive it's own watchdog produced
  messages (pass buffer name insteas buffer handle)

  Revision 1.34  1999/04/30 10:58:58  midas
  Added -D debug to screen for mserver

  Revision 1.33  1999/04/29 10:48:02  midas
  Implemented "/System/Client Notify" key

  Revision 1.32  1999/04/28 15:27:28  midas
  Made hs_read working for Java

  Revision 1.31  1999/04/27 15:16:14  midas
  Increased ASCII_BUFFER_SIZE to 64500

  Revision 1.30  1999/04/27 11:11:26  midas
  Added rpc_register_client

  Revision 1.29  1999/04/23 11:42:52  midas
  Made db_get_data_index working for Java

  Revision 1.28  1999/04/19 07:47:00  midas
  Added cm_msg_retrieve

  Revision 1.27  1999/04/16 15:13:28  midas
  bm_notify_client notifies ASCII client (Java) always

  Revision 1.26  1999/04/15 15:43:06  midas
  Added functionality for bm_receive_event in ASCII mode

  Revision 1.25  1999/04/15 09:58:42  midas
  Switched if (rpc_list[i].id == 0) statements

  Revision 1.24  1999/04/13 12:20:43  midas
  Added db_get_data1 (for Java)

  Revision 1.23  1999/04/08 15:26:05  midas
  Worked on rpc_execute_ascii

  Revision 1.22  1999/03/23 10:37:39  midas
  Fixed bug in cm_set_watchdog_params which causes mtape report ODB errors

  Revision 1.21  1999/02/12 10:55:03  midas
  Accepted PAA's modification in cm_set_watchdog_params()

  Revision 1.20  1999/02/11 13:14:46  midas
  Basic ASCII protocol implemented in server

  Revision 1.19  1999/02/09 14:38:23  midas
  Added debug logging facility

  Revision 1.18  1999/02/06 00:17:12  pierre
  - Fix local watchdog timeout in cm_set_watchdog_params()
  - Touch dm_xxx functions for OS_WINNT

  Revision 1.17  1999/02/02 07:42:22  midas
  Only print warning about zero length bank in bk_close if bank has type TID_STRUCT

  Revision 1.16  1999/02/01 15:41:23  midas
  Added warning for zero length bank in bk_close

  Revision 1.15  1999/02/01 13:03:49  midas
  Added /system/clients/xxx/link timeout to show current TCP timeout value

  Revision 1.14  1999/01/22 09:31:16  midas
  Fixed again status return from ss_mutex_create in bm_open_buffer

  Revision 1.13  1999/01/21 23:09:17  pierre
  - Incorporate dm_semaphore_...() functionality into ss_mutex_...()
  - Remove dm_semaphore_...(), adjust dm_...() accordingly.
  - Incorporate taskSpawn into ss_thread_create (system.c).
  - Adjust status value returnd from ss_mutex_create().

  Revision 1.12  1999/01/20 08:55:44  midas
  - Renames ss_xxx_mutex to ss_mutex_xxx
  - Added timout flag to ss_mutex_wait_for

  Revision 1.11  1999/01/19 19:58:56  pierre
  - Fix compiler warning in dm_buffer_send

  Revision 1.10  1999/01/18 17:50:35  pierre
  - Added dm_...() functions for Dual Memory buffer handling.

  Revision 1.9  1998/12/11 17:00:02  midas
  Fixed a few typos

  Revision 1.8  1998/10/28 12:05:57  midas
  Fixed minor compiler warning

  Revision 1.7  1998/10/28 12:01:30  midas
  Added version number to run start notification

  Revision 1.6  1998/10/27 10:53:48  midas
  - Added run start notification
  - Added ss_shell() for NT

  Revision 1.5  1998/10/23 14:21:50  midas
  - Modified version scheme from 1.06 to 1.6.0
  - cm_get_version() now returns versino as string

  Revision 1.4  1998/10/13 07:34:42  midas
  Reopened database in case of wrong password

  Revision 1.3  1998/10/12 12:19:02  midas
  Added Log tag in header

  Revision 1.2  1998/10/12 11:59:10  midas
  Added Log tag in header

\********************************************************************/

#include "midas.h"
#include "msystem.h"

/*------------------------------------------------------------------*/
/* data type sizes */
INT tid_size[] = {
  0, /* tid == 0 not defined                               */
  1, /* TID_BYTE      unsigned byte         0       255    */
  1, /* TID_SBYTE     signed byte         -128      127    */
  1, /* TID_CHAR      single character      0       255    */
  2, /* TID_WORD      two bytes             0      65535   */
  2, /* TID_SHORT     signed word        -32768    32767   */
  4, /* TID_DWORD     four bytes            0      2^32-1  */
  4, /* TID_INT       signed dword        -2^31    2^31-1  */
  4, /* TID_BOOL      four bytes bool       0        1     */
  4, /* TID_FLOAT     4 Byte float format                  */
  8, /* TID_DOUBLE    8 Byte float format                  */
  1, /* TID_BITFIELD  8 Bits Bitfield    00000000 11111111 */
  0, /* TID_STRING    zero terminated string               */
  0, /* TID_ARRAY     variable length array of unkown type */
  0, /* TID_STRUCT    C structure                          */
  0, /* TID_KEY       key in online database               */
  0  /* TID_LINK      link in online database              */
};

/* data type names */
char* tid_name[] = {
  "NULL",
  "BYTE",
  "SBYTE",
  "CHAR",
  "WORD",
  "SHORT",
  "DWORD",
  "INT",
  "BOOL",
  "FLOAT",
  "DOUBLE",
  "BITFIELD",
  "STRING",
  "ARRAY",
  "STRUCT",
  "KEY",
  "LINK"
};

/* Globals */
#ifdef OS_MSDOS
extern unsigned _stklen = 60000U;
#endif

extern DATABASE       *_database;
extern INT            _database_entries;

static BUFFER         *_buffer;
static INT            _buffer_entries = 0;

static INT            _msg_buffer = 0;
static void           (*_msg_dispatch)(HNDLE,HNDLE,EVENT_HEADER*,void*);

static REQUEST_LIST   *_request_list;
static INT            _request_list_entries = 0;

static EVENT_HEADER   *_event_buffer;
static INT            _event_buffer_size = 0;

static char           *_net_recv_buffer;
static INT            _net_recv_buffer_size = 0;

static char           *_net_send_buffer;
static INT            _net_send_buffer_size = 0;

static char           *_tcp_buffer = NULL;
static INT            _tcp_wp = 0;
static INT            _tcp_rp = 0;

static INT            _send_sock;

static void           (*_debug_print)(char*) = NULL;
static INT            _debug_mode = 0;

static INT            _watchdog_last_called = 0;

/* table for transition functions */

typedef struct {
  INT transition;
  INT (*func)(INT, char*);
  } TRANS_TABLE;

TRANS_TABLE _trans_table[] = 
{
  { TR_START, NULL },
  { TR_STOP, NULL },
  { TR_PAUSE, NULL },
  { TR_RESUME, NULL },
  { TR_PRESTART, NULL },
  { TR_POSTSTART, NULL },
  { TR_PRESTOP, NULL },
  { TR_POSTSTOP, NULL },
  { TR_PREPAUSE, NULL },
  { TR_POSTPAUSE, NULL },
  { TR_PRERESUME, NULL },
  { TR_POSTRESUME, NULL },
  { 0, NULL }
};

TRANS_TABLE _deferred_trans_table[] =
{
  { TR_START, NULL },
  { TR_STOP, NULL },
  { TR_PAUSE, NULL },
  { TR_RESUME, NULL },
  { TR_PRESTART, NULL },
  { TR_POSTSTART, NULL },
  { TR_PRESTOP, NULL },
  { TR_POSTSTOP, NULL },
  { TR_PREPAUSE, NULL },
  { TR_POSTPAUSE, NULL },
  { TR_PRERESUME, NULL },
  { TR_POSTRESUME, NULL },
  { 0, NULL }
};

static BOOL _server_registered = FALSE;

static INT rpc_transition_dispatch(INT index, void *prpc_param[]);

void cm_ctrlc_handler(int sig);

typedef struct {
  INT code;
  char *string;
} ERROR_TABLE;

ERROR_TABLE _error_table[] =
{
  { CM_WRONG_PASSWORD, "Wrong password" },
  { CM_UNDEF_EXP, "Experiment not defined" },
  { CM_UNDEF_ENVIRON, "\"exptab\" file not found and MIDAS_DIR environment variable not defined" },
  { RPC_NET_ERROR, "Cannot connect to remote host" },
  { 0, NULL }
};

/*------------------------------------------------------------------*/

/* malloc/free routines for debugging */

#undef MEM_DBG

#ifdef MEM_DBG
#define MALLOC(x) dbg_malloc((x), __FILE__, __LINE__)
#define FREE(x)   dbg_free  ((x), __FILE__, __LINE__)
#else
#define MALLOC(x) malloc(x)
#define FREE(x) free(x)
#endif

void *dbg_malloc(int size, char *file, int line)
{
FILE *f;
void *adr;

  adr = malloc(size);
  
  f = fopen("mem.txt", "a");
  fprintf(f, "malloc size = %d, adr = %X, %s:%d\n", size, adr, file, line);
  fclose(f);

  return adr;
}

void dbg_free(void *adr, char *file, int line)
{
FILE *f;

  free(adr);

  f = fopen("mem.txt", "a");
  fprintf(f, "free adr = %X, %s:%d\n", adr, file, line);
  fclose(f);
  
}

/*------------------------------------------------------------------*/

/********************************************************************\
*                                                                    *
*              Common message functions                              *
*                                                                    *
\********************************************************************/

static int  (*_message_print)(const char*) = puts;
static INT  _message_mask_system = MT_ALL;
static INT  _message_mask_user   = MT_ALL;

/*------------------------------------------------------------------*/

INT cm_get_error(INT code, char *string)
/********************************************************************\

  Routine: cm_get_error

  Purpose: Convert error code to string. Used after 
           cm_connect_experiment to print error string in command
           line programs or windows programs.
  
  Input:
    INT  code               Error code as defined in MIDAS.H

  Output:
    char *string            Error string

  Function value:
    CM_SUCCESS              Successful completion

\********************************************************************/
{
INT i;

  for (i=0 ; _error_table[i].code ; i++)
    if (_error_table[i].code == code)
      {
      strcpy(string, _error_table[i].string);
      return CM_SUCCESS;
      }

  sprintf(string, "Unexpected error #%d", code);
  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_set_msg_print(INT system_mask, INT user_mask, int (*func)(const char*))
/********************************************************************\

  Routine: cm_set_msg_print

  Purpose: Set message masks. When a message is generated by calling
           cm_msg, it can got to two destinatinons. First a user
           defined callback routine and second to the "SYSMSG" buffer.

           A user defined callback receives all messages which satisfy
           the user_mask.
  
  Input:
    INT  system_mask        Bit masks for MERROR, MINFO etc. to send 
                            system messages
    INT  user_mask          Bit masks for MERROR, MINFO etc. to send 
                            messages to the user callback
    void (*func)()          Function which receives all printout.
                            By setting "puts", messages are just 
                            printed to the screen.

  Output:
    none

  Function value:
    CM_SUCCESS              Successful completion

\********************************************************************/
{
  _message_mask_system  = system_mask;
  _message_mask_user    = user_mask;
  _message_print        = func;

  return BM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_msg_log(INT message_type, char *message)
/********************************************************************\

  Routine: cm_msg_log

  Purpose: Write message to logging file. Called by cm_msg.
           Internal use only

  Input:
    INT    message_type      Message type 
    char   *message          Message string

  Output:
    none

  Function value:
    CM_SUCCESS

\********************************************************************/
{
char  dir[256];
char  filename[256];
char  path[256];
char  str[256];
FILE  *f;
INT   status, size;
HNDLE hDB, hKey;


  if (rpc_is_remote())
    return rpc_call(RPC_CM_MSG_LOG, message_type, message);

  if (message_type != MT_DEBUG)
    {
    cm_get_experiment_database(&hDB, NULL);

    if (hDB)
      {
      status = db_find_key(hDB, 0, "/Logger/Data dir", &hKey);
      if (status == DB_SUCCESS)
        {
        size = sizeof(dir);
        memset(dir, 0, size);
        db_get_value(hDB, 0, "/Logger/Data dir", dir, &size, TID_STRING);
        if (dir[0] != 0)
          if (dir[strlen(dir)-1] != DIR_SEPARATOR)
            strcat(dir, DIR_SEPARATOR_STR);
        
        strcpy(filename, "midas.log");
        db_get_value(hDB, 0, "/Logger/Message file", filename, &size, TID_STRING);

        strcpy(path, dir);
        strcat(path, filename);
        }
      else
        {
        cm_get_path(dir);
        if (dir[0] != 0)
          if (dir[strlen(dir)-1] != DIR_SEPARATOR)
            strcat(dir, DIR_SEPARATOR_STR);

        strcpy(path, dir);
        strcat(path, "midas.log");
        }
      }
    else
      strcpy(path, "midas.log");

    f = fopen(path, "a");
    if (f==NULL)
      {
      printf("Cannot open message log file %s\n", path);
      }
    else
      {
      strcpy(str, ss_asctime());
      fprintf(f, str);
      fprintf(f, " %s\n", message);
      fclose(f);
      }
    }

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_msg(INT message_type, char* filename, INT line, 
            const char *routine, const char *format, ...)
/********************************************************************\

  Routine: cm_msg (MIDAS-message)

  Purpose: This routine gets called whenever an internal error occurs
           or an informative message is produced. Different message
           types can be enabled or disabled by setting the type bits
           via cm_set_msg_print.
           

  Input:
    INT   message_type      (1<<0): Error
                            (1<<1): Info
                            (1<<2): Debug
                            (1<<3): User
                            (1<<4): Only logged
                            (1<<5): Talked by speech system

    char  *filename         Name of source file where error occured
    INT   line              Line number where error occured

    char* routine           Routine name
    char* error format      Error message to print out
    ...                     Parameters like for printf()
  Output:
    none                        

  Function value:
    CM_SUCCESS              Sucessful completion
    <return value form bm_open_buffer>

\********************************************************************/
{
va_list      argptr;
char         event[1000], str[256], local_message[256], send_message[256], *pc;
EVENT_HEADER *pevent;
INT          status;
static BOOL  in_routine = FALSE;

  /* avoid recursive calles */
  if (in_routine)
    return 0;

  in_routine = TRUE;

  /* strip path */
  pc = filename + strlen(filename);
  while (*pc != '\\' && *pc != '/' && pc != filename)
    pc--;
  if (pc != filename)
    pc++;  

  /* print client name into string */
  if (message_type == MT_USER)
    sprintf(send_message, "[%s] ", routine);
  else
    {
    rpc_get_name(str);
    if (str[0])
      sprintf(send_message, "[%s] ", str);
    else
      send_message[0] = 0;
    }

  local_message[0] = 0;

  /* preceed error messages with file and line info */
  if (message_type == MT_ERROR)
    {
    sprintf(str, "[%s:%ld:%s] ", pc, line, routine);
    strcat(send_message, str);
    strcat(local_message, str);
    }

  /* print argument list into message */
  va_start(argptr, format);
  vsprintf(str, (char *) format, argptr);
  va_end(argptr);
  strcat(send_message, str);
  strcat(local_message, str);

  /* call user function if set via cm_set_msg_print */
  if (_message_print != NULL &&
      (message_type & _message_mask_user) != 0)
    _message_print(local_message);

  /* return if system mask is not set */
  if ((message_type & _message_mask_system) == 0)
    {
    in_routine = FALSE;
    return CM_SUCCESS;
    }

  /* copy message to event */
  pevent = (EVENT_HEADER *) event;
  strcpy(event + sizeof(EVENT_HEADER), send_message);

  /* send event if not of type MLOG */
  if (message_type != MT_LOG)
    {
    /* if no message buffer already opened, do so now */
    if (_msg_buffer == 0)
      {
      status = bm_open_buffer(MESSAGE_BUFFER_NAME, MESSAGE_BUFFER_SIZE, &_msg_buffer);
      if (status != BM_SUCCESS && status != BM_CREATED)
        {
        in_routine = FALSE;
        return status;
        }
      }

    /* setup the event header and send the message */
    bm_compose_event(pevent, EVENTID_MESSAGE, (WORD) message_type,
                     strlen(event + sizeof(EVENT_HEADER))+1, 0);
    bm_send_event(_msg_buffer, event, pevent->data_size+sizeof(EVENT_HEADER), SYNC);
    }

  /* log message */
  cm_msg_log(message_type, send_message);

  in_routine = FALSE;

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_msg_register(void (*func)(HNDLE,HNDLE,EVENT_HEADER*,void*))
/********************************************************************\

  Routine: cm_msg_register

  Purpose: Register a dispatch function for receiving system
           messages.

  Input:
    void   *func()          Dispatch function

  Output:
    none

  Function value:
    <see bm_open_buffer and bm_request_event>

\********************************************************************/
{
INT status, id;
  
  /* if no message buffer already opened, do so now */
  if (_msg_buffer == 0)
    {
    status = bm_open_buffer(MESSAGE_BUFFER_NAME, MESSAGE_BUFFER_SIZE, &_msg_buffer);
    if (status != BM_SUCCESS && status != BM_CREATED)
      return status;
    }

  _msg_dispatch = func;
  
  status = bm_request_event(_msg_buffer, EVENTID_ALL, TRIGGER_ALL, 
                            GET_SOME, &id, func);

  return status;
}

/*------------------------------------------------------------------*/

INT cm_msg_retrieve(char *message, INT *buf_size)
/********************************************************************\

  Routine: cm_msg_retrieve

  Purpose: Retrive old messages from log file

  Input:
    INT    *buf_size         Size of message buffer to fill

  Output:
    char   *messages         buf_size bytes of messages, separated
                             by \n characters. The returned number
                             of bytes is normally smaller than the
                             initial buf_size, since only full
                             lines are returned.

  Function value:
    CM_SUCCESS

\********************************************************************/
{
char  dir[256];
char  filename[256];
char  path[256];
FILE  *f;
INT   status, size, offset;
HNDLE hDB, hKey;


  if (rpc_is_remote())
    return rpc_call(RPC_CM_MSG_RETRIEVE, message, buf_size);

  cm_get_experiment_database(&hDB, NULL);

  if (hDB)
  {
    status = db_find_key(hDB, 0, "/Logger/Data dir", &hKey);
    if (status == DB_SUCCESS)
      {
      size = sizeof(dir);
      memset(dir, 0, size);
      db_get_value(hDB, 0, "/Logger/Data dir", dir, &size, TID_STRING);
      if (dir[0] != 0)
        if (dir[strlen(dir)-1] != DIR_SEPARATOR)
          strcat(dir, DIR_SEPARATOR_STR);
      
      strcpy(filename, "midas.log");
      db_get_value(hDB, 0, "/Logger/Message file", filename, &size, TID_STRING);

      strcpy(path, dir);
      strcat(path, filename);
      }
    else
    {
      cm_get_path(dir);
      if (dir[0] != 0)
        if (dir[strlen(dir)-1] != DIR_SEPARATOR)
          strcat(dir, DIR_SEPARATOR_STR);

      strcpy(path, dir);
      strcat(path, "midas.log");
    }
  }
  else
    strcpy(path, "midas.log");

  f = fopen(path, "rb");
  if (f==NULL)
    {
    printf("Cannot open message log file %s\n", path);
    }
  else
    {
    /* position buf_size bytes before the EOF */
    fseek(f, -(*buf_size-1), SEEK_END);
    offset = ftell(f);
    if (offset != 0)
      {
      fgets(message, *buf_size-1, f);
      offset = ftell(f)-offset;
      *buf_size -= offset;
      }

    fread(message, 1, *buf_size-1, f);
    message[*buf_size-1] = 0;
    fclose(f);
    }

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_synchronize(DWORD *seconds)
/********************************************************************\

  Routine: cm_synchronize

  Purpose: Get time from MIDAS server and set local time.

  Input:
    none

  Output:
    none

  Function value:
    INT    Time in seconds

\********************************************************************/
{
INT sec, status;

  /* if connected to server, get time from there */
  if (rpc_is_remote())
    {
    status = rpc_call(RPC_CM_SYNCHRONIZE, &sec);

    /* set local time */
    if (status == CM_SUCCESS)
      ss_settime(sec);
    }

  /* return time to caller */
  if (seconds != NULL)
    {
    *seconds = ss_time();
    }

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_asctime(char *str, INT buf_size)
/********************************************************************\

  Routine: cm_asctime

  Purpose: Get time in ASCII format from server

  Input:
    INT    buf_size       Maximum size of str

  Output:
    char*  time string

  Function value:
    CM_SUCCESS

\********************************************************************/
{
  /* if connected to server, get time from there */
  if (rpc_is_remote())
    return rpc_call(RPC_CM_ASCTIME, str, buf_size);

  /* return local time */
  strcpy(str, ss_asctime());

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_time(DWORD *time)
/********************************************************************\

  Routine: cm_time

  Purpose: Get time from ss_time on server

  Input:
    none

  Output:
    char*  time string

  Function value:
    CM_SUCCESS

\********************************************************************/
{
  /* if connected to server, get time from there */
  if (rpc_is_remote())
    return rpc_call(RPC_CM_TIME, time);

  /* return local time */
  *time = ss_time();

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

/********************************************************************\
*                                                                    *
*           cm_xxx  -  Common Functions to buffer & database         *
*                                                                    *
\********************************************************************/

/* Globals */

static HNDLE _hKeyClient=0;  /* key handle for client in ODB */
static HNDLE _hDB=0;         /* Database handle */
static INT   _hardware_type;                                  
static char  _client_name[NAME_LENGTH];
static char  _path_name[MAX_STRING_LENGTH];
static INT   _call_watchdog     = TRUE;
static INT   _watchdog_timeout  = DEFAULT_WATCHDOG_TIMEOUT;
       INT   _mutex_alarm, _mutex_elog;

/*------------------------------------------------------------------*/

char *cm_get_version()
/********************************************************************\

  Routine: cm_get_version

  Purpose: Return version number of current MIDAS library as a string

  Input:
    none 

  Output:
    none

  Function value:
    INT    version number * 100

\********************************************************************/
{
  return MIDAS_VERSION;
}

/*------------------------------------------------------------------*/

INT cm_set_path(char *path)
/********************************************************************\

  Routine: cm_set_path

  Purpose: Set path to actual experiment. This function gets called
           by cm_connect_experiment if the connection is established
           to a local experiment (not through the TCP/IP server).
           The path is then used for all shared memory routines.

  Input:
    char  *path             Pathname

  Output:
    none

  Function value:
    CM_SUCCESS              Successful completion

\********************************************************************/
{
  strcpy(_path_name, path);

  /* check for trailing directory seperator */
  if (strlen(_path_name) > 0 &&
      _path_name[strlen(_path_name)-1] != DIR_SEPARATOR)
    strcat(_path_name, DIR_SEPARATOR_STR);
    
  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_get_path(char *path)
/********************************************************************\

  Routine: cm_get_path

  Purpose: Return the path name previously set with cm_set_path.

  Input:
    none

  Output:
    char  *path             Pathname

  Function value:
    CM_SUCCESS              Successful completion

\********************************************************************/
{
  strcpy(path, _path_name);

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

typedef struct {
  char name[NAME_LENGTH];
  char directory[MAX_STRING_LENGTH];
  char user[NAME_LENGTH];
  } experiment_table;

static experiment_table  exptab[MAX_EXPERIMENT];

INT cm_scan_experiments(void)
/********************************************************************\

  Routine: cm_scan_experiments

  Purpose: Scan the "exptab" file for MIDAS experiments and save them
           in exptab which will be used by rpc_server_accept. The
           file is first searched under $MIDAS/exptab, then the
           directory from argv[0] is probed.

  Input:
    none

    The input file has the folowing format:
    ---------------------------------------

    "Experiment name" "Directory" "User"

  Output:
    <implicit> exptab        Filled with experiment definition

  Function value:
    CM_SUCCESS
    CM_UNDEF_EXP             exptab not found and MIDAS_DIR not set

\********************************************************************/
{
INT  i;
FILE *f;
char str[MAX_STRING_LENGTH], alt_str[MAX_STRING_LENGTH], *pdir;

  for (i=0 ; i<MAX_EXPERIMENT ; i++)
    exptab[i].name[0] = 0;

  /* MIDAS_DIR overrides exptab */
  if (getenv("MIDAS_DIR"))
    {
    strcpy(str, getenv("MIDAS_DIR"));

    strcpy(exptab[0].name, "Default");
    strcpy(exptab[0].directory, getenv("MIDAS_DIR"));
    exptab[0].user[0] = 0;

    return CM_SUCCESS;
    }

  /* default directory for different OSes */
#if defined (OS_WINNT)
  if (getenv("SystemRoot"))
    strcpy(str, getenv("SystemRoot"));
  else if (getenv("windir"))
    strcpy(str, getenv("windir"));
  else
    strcpy(str, "");

  strcpy(alt_str, str);
  strcat(str, "\\system32\\exptab");
  strcat(alt_str, "\\system\\exptab");
#elif defined (OS_UNIX)
  strcpy(str, "/etc/exptab");
  strcpy(alt_str, "/exptab");
#else
  strcpy(str, "exptab");
  strcpy(alt_str, "exptab");
#endif

  /* read list of available experiments */
  f = fopen(str, "r");
  if (f == NULL)
    {
    f = fopen(alt_str, "r");
    if (f == NULL)
      return CM_UNDEF_ENVIRON;
    }
  
  i = 0;
  if (f != NULL)
    {
    do
      {
      str[0] = 0;
      fgets(str, 100, f);
      if (str[0])
        {
        sscanf(str, "%s %s %s", exptab[i].name, exptab[i].directory,
                    exptab[i].user);

        /* check for trailing directory separator */
        pdir = exptab[i].directory;
        if (pdir[strlen(pdir)-1] != DIR_SEPARATOR)
          strcat(pdir, DIR_SEPARATOR_STR);

        i++;
        }
      } while (!feof(f));
    fclose(f);
    }

  /*
  for (j=0 ; j<i ; j++)
    {
    sprintf(str, "Scanned experiment %s", exptab[j].name);
    cm_msg(MINFO, str);
    }
  */

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_delete_client_info(HNDLE hDB, INT pid)
/********************************************************************\

  Routine: cm_delete_client_info

  Purpose: Get info about the current client

  Input:
    HNDLE hDB               Database handle
    INT  pid                PID of entry to delete, zero for this
                            process.

  Output:
    char *client_name       Client name.

  Function value:
    CM_SUCCESS              Successful completion

\********************************************************************/
{
#ifdef LOCAL_ROUTINES

  /* only do it if local */
  if (!rpc_is_remote())
    {
    INT   status;
    HNDLE hKey;
    char  str[256];

    if (!pid)
      pid = ss_gettid();

    /* don't delete info from a closed database */
    if (_database_entries == 0)
      return CM_SUCCESS;

    /* make operation atomic by locking database */
    db_lock_database(hDB);

    sprintf(str, "System/Clients/%0d", pid);
    status = db_find_key1(hDB, 0, str, &hKey);
    if (status != DB_SUCCESS)
      {
      db_unlock_database(hDB);
      return status;
      }

    /* unlock client entry and delete it without locking DB */
    db_set_mode(hDB, hKey, MODE_READ | MODE_WRITE | MODE_DELETE, 2);
    db_delete_key1(hDB, hKey, 1, TRUE);

    db_unlock_database(hDB);

    /* touch notify key to inform others */
    status = 0;
    db_set_value(hDB, 0, "/System/Client Notify", &status, sizeof(status), 1, TID_INT);
    }

#endif /*LOCAL_ROUTINES*/

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_check_client(HNDLE hDB, HNDLE hKeyClient)
/********************************************************************\

  Routine: cm_check_client

  Purpose: Check if a client with a /system/client/xxx entry has
           a valid entry in the ODB client table. If not, remove 
           that client from the /system/client tree.

  Input:
    HNDLE hDB               Handle to online database
    HNDLE hKeyClient        Handle to client key

  Output:
    <none>

  Function value:
    CM_NO_CLIENT            Client not registed, entry deleted
    CM_SUCCESS              Successful completion

\********************************************************************/
{
#ifdef LOCAL_ROUTINES

KEY             key;
DATABASE_HEADER *pheader;
DATABASE_CLIENT *pclient;
INT             i, client_pid, status;
char            name[NAME_LENGTH];

  db_get_key(hDB, hKeyClient, &key);
  client_pid = atoi(key.name);
  
  i = sizeof(name);
  db_get_value(hDB, hKeyClient, "Name", name, &i, TID_STRING);

  db_lock_database(hDB);
  if (_database[hDB-1].attached)
    {
    pheader = _database[hDB-1].database_header;
    pclient = pheader->client;

    /* loop through clients */
    for (i=0 ; i<pheader->max_client_index ; i++,pclient++)
      if (pclient->tid == client_pid)
        break;

    if (i==pheader->max_client_index)
      {
      /* client not found : delete ODB stucture */
      db_unlock_database(hDB);

      status = cm_delete_client_info(hDB, client_pid);
      if (status != CM_SUCCESS)
        cm_msg(MERROR, "cm_check_client", "cannot delete client info");
      else
        cm_msg(MINFO, "cm_check_clinet", 
          "Deleted /System/Clients/%d entry for client %s\n", client_pid, name);

      return CM_NO_CLIENT;
      }
    }

  db_unlock_database(hDB);

#endif /*LOCAL_ROUTINES*/

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_set_client_info(HNDLE hDB, HNDLE *hKeyClient, char *host_name,
                       char *client_name, INT hw_type, char *password,
                       INT watchdog_timeout)
/********************************************************************\

  Routine: cm_set_client_info

  Purpose: Set client information in online database and return handle

  Input:
    HNDLE *hDB              Handle to online database
    char  *host_name        Host name of client
    char  *client_name      Name of this program as it will be seen 
                            by other clients.
    INT   hw_type           Hardware type returned by
                            rpc_get_option(RPC_OHW_TYPE)
    char  *passoword        MIDAS password
    INT   watchdog_timeout  Default watchdog timeout, can be overwritten
                            by ODB setting /programs/<name>/Watchdog timeout

  Output:
    HNDLE *hKeyClient       Handle to client key

  Function value:
    CM_SUCCESS              Successful completion

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_CM_SET_CLIENT_INFO, hDB, hKeyClient, 
                    host_name, client_name, hw_type, password);

#ifdef LOCAL_ROUTINES
{
INT   status, pid, data, i, index, size;
HNDLE hKey, hSubkey;
char  str[256], name[NAME_LENGTH], orig_name[NAME_LENGTH], pwd[NAME_LENGTH];
BOOL  call_watchdog, allow;
PROGRAM_INFO_STR(program_info_str);

  /* check security if password is presend */ 
  status = db_find_key(hDB, 0, "/Experiment/Security/Password", &hKey);
  if (hKey)
    {
    /* get password */
    size = sizeof(pwd);
    db_get_data(hDB, hKey, pwd, &size, TID_STRING);

    /* first check allowed hosts list */
    allow = FALSE;
    db_find_key(hDB, 0, "/Experiment/Security/Allowed hosts", &hKey);
    if (hKey && db_find_key(hDB, hKey, host_name, &hKey) == DB_SUCCESS)
      allow = TRUE;

    /* check allowed programs list */
    db_find_key(hDB, 0, "/Experiment/Security/Allowed programs", &hKey);
    if (hKey && db_find_key(hDB, hKey, client_name, &hKey) == DB_SUCCESS)
      allow = TRUE;
    
    /* now check password */
    if (!allow && 
        strcmp(password, pwd) != 0 &&
        strcmp(password, "mid7qBxsNMHux") != 0)
      {
      if (password[0])
        cm_msg(MINFO, "cm_set_client_info", "Wrong password for host %s", host_name);
      db_close_all_databases();
      bm_close_all_buffers();
      _msg_buffer = 0;
      return CM_WRONG_PASSWORD;
      }
    }

  /* check if entry with this pid exists already */
  pid = ss_gettid();

  sprintf(str, "System/Clients/%0d", pid);
  status = db_find_key(hDB, 0, str, &hKey);
  if (status == DB_SUCCESS)
    {
    db_set_mode(hDB, hKey, MODE_READ | MODE_WRITE | MODE_DELETE, TRUE);
    db_delete_key(hDB, hKey, TRUE);
    }

  if (strlen(client_name) >= NAME_LENGTH)
    client_name[NAME_LENGTH] = 0;

  strcpy(name, client_name);
  strcpy(orig_name, client_name);

  /* check if client name already exists */
  status = db_find_key(hDB, 0, "System/Clients", &hKey);

  for (index=1; status != DB_NO_MORE_SUBKEYS ; index++)
    {
    for (i=0; ; i++)
      {
      status = db_enum_key(hDB, hKey, i, &hSubkey);
      if (status == DB_NO_MORE_SUBKEYS)
        break;

      if (status == DB_SUCCESS)
        {
        size = sizeof(str);
        status = db_get_value(hDB, hSubkey, "Name", str, &size, TID_STRING);
        }

      /* check if client is living */
      if (cm_check_client(hDB, hSubkey) == CM_NO_CLIENT)
        continue;

      if (equal_ustring(str, name))
        {
        sprintf(name, "%s%d", client_name, index);
        break;
        }
      }
    }

  /* set name */
  sprintf(str, "System/Clients/%0d/Name", pid);
  status = db_set_value(hDB, 0, str, name, NAME_LENGTH, 1, TID_STRING);
  if (status != DB_SUCCESS)
    {
    cm_msg(MERROR, "cm_set_client_info", "cannot set client name");
    return status;
    }

  /* copy new client name */
  strcpy(client_name, name);
  db_set_client_name(hDB, client_name);

  /* set also as rpc name */
  rpc_set_name(client_name);

  /* use /system/clients/PID as root */
  sprintf(str, "System/Clients/%0d", pid);
  db_find_key(hDB, 0, str, &hKey);

  /* set host name */
  status = db_set_value(hDB, hKey, "Host", host_name, HOST_NAME_LENGTH, 1, TID_STRING);
  if (status != DB_SUCCESS)
    return status;

  /* set computer id */
  status = db_set_value(hDB, hKey, "Hardware type", &hw_type, 
                        sizeof(hw_type), 1, TID_INT);
  if (status != DB_SUCCESS)
    return status;

  /* set server port */
  data = 0;
  status = db_set_value(hDB, hKey, "Server Port", &data, 
                        sizeof(INT), 1, TID_INT);
  if (status != DB_SUCCESS)
    return status;

  /* set transition mask */
  data = 0;
  status = db_set_value(hDB, hKey, "Transition Mask", &data, 
                        sizeof(DWORD), 1, TID_DWORD);
  if (status != DB_SUCCESS)
    return status;

  /* set deferred transition mask */
  data = 0;
  status = db_set_value(hDB, hKey, "Deferred Transition Mask", &data, 
                        sizeof(DWORD), 1, TID_DWORD);
  if (status != DB_SUCCESS)
    return status;

  /* lock client entry */
  db_set_mode(hDB, hKey, MODE_READ, TRUE);

  /* get (set) default watchdog timeout */
  size = sizeof(watchdog_timeout);
  sprintf(str, "/Programs/%s/Watchdog Timeout", orig_name);
  db_get_value(hDB, 0, str, &watchdog_timeout, &size, TID_INT);
  
  /* define /programs entry */
  sprintf(str, "/Programs/%s", orig_name);
  db_create_record(hDB, 0, str, strcomb(program_info_str));

  /* save handle for ODB and client */
  rpc_set_server_option(RPC_ODB_HANDLE, hDB);
  rpc_set_server_option(RPC_CLIENT_HANDLE, hKey);

  /* save watchdog timeout */
  cm_get_watchdog_params(&call_watchdog, NULL);
  cm_set_watchdog_params(call_watchdog, watchdog_timeout);
  if (call_watchdog)
    ss_alarm(WATCHDOG_INTERVAL, cm_watchdog);

  /* touch notify key to inform others */
  data = 0;
  db_set_value(hDB, 0, "/System/Client Notify", &data, sizeof(data), 1, TID_INT);

  *hKeyClient = hKey;
}
#endif /* LOCAL_ROUTINES */

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_get_client_info(char *client_name)
/********************************************************************\

  Routine: cm_get_client_info

  Purpose: Get info about the current client

  Input:
    none

  Output:
    char *client_name       Client name.

  Function value:
    CM_SUCCESS              Successful completion
    CM_UNDEF_EXP            Not connected to experiment

\********************************************************************/
{
INT   status, length;
HNDLE hDB, hKey;

  /* get root key of client */
  cm_get_experiment_database(&hDB, &hKey);
  if (!hDB)
    {
    client_name[0] = 0;
    return CM_UNDEF_EXP;
    }

  status = db_find_key(hDB, hKey, "Name", &hKey);
  if (status != DB_SUCCESS)
    return status;

  length = NAME_LENGTH;
  status = db_get_data(hDB, hKey, client_name, &length, TID_STRING);
  if (status != DB_SUCCESS)
    return status;

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_get_environment(char *host_name, char *exp_name)
/********************************************************************\

  Routine: cm_get_environment

  Purpose: Get environment variables for host and experiment. If not
           defined, names are empty.

  Input:
    none

  Output:
    char *host_name         host anme from MIDAS_SERVER_HOST
    char *exp_name          experiment name from MIDAS_EXPT_NAME

  Function value:
    CM_SUCCESS              Successful completion

\********************************************************************/
{
  host_name[0] = exp_name[0] = 0;

  if (getenv("MIDAS_SERVER_HOST"))
    strcpy(host_name, getenv("MIDAS_SERVER_HOST"));

  if (getenv("MIDAS_EXPT_NAME"))
    strcpy(exp_name, getenv("MIDAS_EXPT_NAME"));

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

void cm_check_connect(void)
{
  if (_hKeyClient)
    cm_msg(MERROR, "", "cm_disconnect_experiment not called at end of program");
}

/*------------------------------------------------------------------*/

INT cm_connect_experiment(char *host_name, char *exp_name, 
                          char *client_name, void (*func)(char*))
/********************************************************************\

  Routine: cm_connect_experiment

  Purpose: Call cm_connect_experiment1 and print error message to
           stdout.

  Input:
    char *host_name         Internet host name. Null string for local
                            machine.
    char *exp_name          Experiment name, which is defined in "exptab". 
                            If exp_name==NULL or "", the shared memory
                            objects are created in the local directory.
    
    char *client_name       Name of this program as it will be seen 
                            by other clients.
    
    void *func()            Callback function to query password

  Output:
    none

  Function value:
    CM_SUCCESS              Successful completion
    CM_SET_ERROR            Error setting client info
    CM_UNDEF_EXP            Experiment not defined
    CM_VERSION_MISMATCH     MIDAS library version mismatch
    RPC_NET_ERROR           Network error
    RPC_NO_MEMORY           Not enough memory

\********************************************************************/
{
INT status;
char str[256];

  status = cm_connect_experiment1(host_name, exp_name, client_name, 
                                  func, DEFAULT_ODB_SIZE, DEFAULT_WATCHDOG_TIMEOUT);
  if (status != CM_SUCCESS)
    {
    cm_get_error(status, str);
    puts(str);
    }
  
  return status;
}

/*------------------------------------------------------------------*/

INT cm_connect_experiment1(char *host_name, char *exp_name, 
                           char *client_name, void (*func)(char*), 
                           INT odb_size, INT watchdog_timeout)
/********************************************************************\

  Routine: cm_connect_experiment1

  Purpose: Connect to a MIDAS experiment (to the online database) on
           a specific host. 

  Input:
    char *host_name         Internet host name. Null string for local
                            machine.
    char *exp_name          Experiment name, which is defined in "exptab". 
                            If exp_name==NULL or "", the shared memory
                            objects are created in the local directory.
    
    char *client_name       Name of this program as it will be seen 
                            by other clients.
    
    void *func()            Callback function to query password

    INT odb_size            Size in bytes of ODB. Only used when creating
                            a fresh ODB.

    INT watchdog_timeout    Default watchdog timeout, can be overwritten
                            by ODB setting /programs/<name>/Watchdog timeout

  Output:
    none

  Function value:
    CM_SUCCESS              Successful completion
    CM_SET_ERROR            Error setting client info
    CM_UNDEF_EXP            Experiment not defined
    CM_VERSION_MISMATCH     MIDAS library version mismatch
    RPC_NET_ERROR           Network error
    RPC_NO_MEMORY           Not enough memory

\********************************************************************/
{
INT   status, i, mutex_elog, mutex_alarm;
char  local_host_name[HOST_NAME_LENGTH];
char  client_name1[NAME_LENGTH];
char  password[NAME_LENGTH], str[NAME_LENGTH], exp_name1[NAME_LENGTH];
HNDLE hDB, hKeyClient;
BOOL  call_watchdog;

  if (_hKeyClient)
    cm_disconnect_experiment();

  rpc_set_name(client_name);

  /* check for local host */
  if (equal_ustring(host_name, "local"))
    host_name[0] = 0;

#ifdef OS_WINNT
  {
  WSADATA WSAData;

  /* Start windows sockets */
  if ( WSAStartup(MAKEWORD(1,1), &WSAData) != 0)
    return RPC_NET_ERROR;
  }
#endif

  /* search for experiment name in exptab */
  if (exp_name == NULL)
    exp_name = "";

  strcpy(exp_name1, exp_name);
  if (exp_name1[0] == 0)
    {
    status = cm_select_experiment(host_name, exp_name1);
    if (status != CM_SUCCESS)
      return status;
    }

  /* connect to MIDAS server */
  if (host_name[0])
    {
    status = rpc_server_connect(host_name, exp_name1);
    if (status != RPC_SUCCESS)
      return status;

    /* register MIDAS library functions */
    status = rpc_register_functions(rpc_get_internal_list(1), NULL);
    if (status != RPC_SUCCESS)
      return status;
    }
  else
    {
    /* lookup path for *SHM files and save it */
    status = cm_scan_experiments();
    if (status != CM_SUCCESS)
      return status;

    for (i=0 ; i<MAX_EXPERIMENT && exptab[i].name[0] ; i++)
      if (equal_ustring(exp_name1, exptab[i].name))
        break;

    /* return if experiment not defined */
    if (i == MAX_EXPERIMENT || exptab[i].name[0] == 0)
      {
      /* message should be displayed by application 
      sprintf(str, "Experiment %s not defined in exptab\r", exp_name1);
      cm_msg(MERROR, str);
      */
      return CM_UNDEF_EXP;
      }

    cm_set_path(exptab[i].directory);

    /* create alarm and elog mutexes */
    status = ss_mutex_create("ALARM", &mutex_alarm);
    if (status != SS_CREATED && status != SS_SUCCESS)
      {
      cm_msg(MERROR,"cm_connect_experiment", "Cannot create alarm mutex");
      return status;
      }
    status = ss_mutex_create("ELOG", &mutex_elog);
    if (status != SS_CREATED && status != SS_SUCCESS)
      {
      cm_msg(MERROR,"cm_connect_experiment", "Cannot create elog mutex");
      return status;
      }
    cm_set_experiment_mutex(mutex_alarm, mutex_elog);
    }

  /* open ODB */
  if (odb_size == 0)
    odb_size = DEFAULT_ODB_SIZE;

  status = db_open_database("ODB", odb_size, &hDB, client_name);
  if (status != DB_SUCCESS && status != DB_CREATED)
    {
    cm_msg(MERROR, "cm_connect_experiment1", "cannot open database");
    return status;
    }

  /* now setup client info */
  gethostname(local_host_name, sizeof(local_host_name));

  /* check watchdog timeout */
  if (watchdog_timeout == 0)
    watchdog_timeout = DEFAULT_WATCHDOG_TIMEOUT;

  strcpy(client_name1, client_name);
  password[0] = 0;
  status = cm_set_client_info(hDB, &hKeyClient, local_host_name, 
                              client_name1, rpc_get_option(0, RPC_OHW_TYPE), 
                              password, watchdog_timeout);

  if (status == CM_WRONG_PASSWORD)
    {
    if (func == NULL)
      strcpy(str, ss_getpass("Password: "));
    else
      func(str);

    /* re-open database */
    status = db_open_database("ODB", odb_size, &hDB, client_name);
    if (status != DB_SUCCESS && status != DB_CREATED)
      {
      cm_msg(MERROR, "cm_connect_experiment1", "cannot open database");
      return status;
      }

    strcpy(password, ss_crypt(str, "mi"));
    status = cm_set_client_info(hDB, &hKeyClient, local_host_name, 
                                client_name1, rpc_get_option(0, RPC_OHW_TYPE), 
                                password, watchdog_timeout);
    if (status != CM_SUCCESS)
      {
      /* disconnect */
      if (rpc_is_remote())
        rpc_server_disconnect();

      return status;
      }
    }

  cm_set_experiment_database(hDB, hKeyClient);

  /* set experiment name in ODB */
  db_set_value(hDB, 0, "/Experiment/Name", exp_name1, NAME_LENGTH, 1, TID_STRING);

  /* register server to be able to be called by other clients*/
  status = cm_register_server();
  if (status != CM_SUCCESS)
    return status;

  /* set watchdog timeout */
  cm_get_watchdog_params(&call_watchdog, &watchdog_timeout);
  cm_set_watchdog_params(call_watchdog, watchdog_timeout);

  /* send startup notification */
  if (strchr(local_host_name, '.'))
    *strchr(local_host_name, '.') = 0;

  /* startup message is not displayed */
  _message_print = NULL;
  
  cm_msg(MINFO, "cm_connect_experiment", "Program %s on host %s started", 
         client_name, local_host_name);

  /* enable system and user messages to stdout as default */
  cm_set_msg_print(MT_ALL, MT_ALL, puts);

  /* call cm_check_connect when exiting */
  atexit((void (*)(void))cm_check_connect);

  /* register ctrl-c handler */
  ss_ctrlc_handler(cm_ctrlc_handler);

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_list_experiments(char *host_name, char exp_name[MAX_EXPERIMENT][NAME_LENGTH])
/********************************************************************\

  Routine: cm_list_experiment

  Purpose: Connect to a MIDAS server and return all defined 
           experiments in *exp_name[MAX_EXPERIMENTS]

  Input:
    char *host_name         Internet host name.

  Output:
    char **exp_name         List of experiment names

  Function value:
    CM_SUCCESS              Successful completion
    RPC_NET_ERROR           Network error

\********************************************************************/
{
INT                  i, status;
struct sockaddr_in   bind_addr;
INT                  sock;
char                 str[MAX_EXPERIMENT * NAME_LENGTH];
struct hostent       *phe;

  if (host_name[0] == 0 || equal_ustring(host_name, "local"))
    {
    status = cm_scan_experiments();
    if (status != CM_SUCCESS)
      return status;

    for (i=0 ; i<MAX_EXPERIMENT ; i++)
      strcpy(exp_name[i], exptab[i].name);

    return CM_SUCCESS;
    }

#ifdef OS_WINNT
  {
  WSADATA WSAData;

  /* Start windows sockets */
  if ( WSAStartup(MAKEWORD(1,1), &WSAData) != 0)
    return RPC_NET_ERROR;
  }
#endif

  /* create a new socket for connecting to remote server */
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1)
    {
    cm_msg(MERROR, "cm_list_experiments", "cannot create socket");
    return RPC_NET_ERROR;
    }

  /* let OS choose any port number */
  memset(&bind_addr, 0, sizeof(bind_addr));
  bind_addr.sin_family      = AF_INET;
  bind_addr.sin_addr.s_addr = 0;
  bind_addr.sin_port        = 0;

  status = bind(sock, (void *)&bind_addr, sizeof(bind_addr));
  if (status < 0)
    {
    cm_msg(MERROR, "cm_list_experiments", "cannot bind");
    return RPC_NET_ERROR;
    }

  /* connect to remote node */
  memset(&bind_addr, 0, sizeof(bind_addr));
  bind_addr.sin_family      = AF_INET;
  bind_addr.sin_addr.s_addr = 0;
  bind_addr.sin_port        = htons((short) MIDAS_TCP_PORT);

#ifdef OS_VXWORKS
  {
  INT host_addr;

  host_addr = hostGetByName(host_name);
  memcpy((char *)&(bind_addr.sin_addr), &host_addr, 4);
  }
#else
  phe = gethostbyname(host_name);
  if (phe == NULL)
    {
    cm_msg(MERROR, "cm_list_experiments", "cannot get host name");
    return RPC_NET_ERROR;
    }
  memcpy((char *)&(bind_addr.sin_addr), phe->h_addr, phe->h_length);
#endif

#ifdef OS_UNIX
  do
    {
    status = connect(sock, (void *) &bind_addr, sizeof(bind_addr));

    /* don't return if an alarm signal was cought */
    } while (status == -1 && errno == EINTR); 
#else
  status = connect(sock, (void *) &bind_addr, sizeof(bind_addr));
#endif  

  if (status != 0)
    {
/*    cm_msg(MERROR, "cannot connect"); message should be displayed by application */
    return RPC_NET_ERROR;
    }

  /* request experiment list */
  send(sock, "I", 2, 0);

  for (i=0 ; i<MAX_EXPERIMENT ; i++)
    {
    exp_name[i][0] = 0;
    status = recv_string(sock, str, sizeof(str), 1000);

    if (status < 0)
      return RPC_NET_ERROR;

    if (status == 0)
      break;

    strcpy(exp_name[i], str);
    }

  exp_name[i][0] = 0;
  closesocket(sock);

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_select_experiment(char *host_name, char *exp_name)
/********************************************************************\

  Routine: cm_select_experimen

  Purpose: Connect to a MIDAS server and select an experiment
           from the experiments available on this server

  Input:
    char *host_name         Internet host name.

  Output:
    char *exp_name          Selected experiment

  Function value:
    CM_SUCCESS              Successful completion
    RPC_NET_ERROR           Network error

\********************************************************************/
{
INT    status, i;
char   expts[MAX_EXPERIMENT][NAME_LENGTH];
char   str[32];

  /* retrieve list of experiments and make selection */
  status = cm_list_experiments(host_name, expts);
  if (status != CM_SUCCESS)
    return status;

  if (expts[1][0])
    {
    if (host_name[0])
      printf("Available experiments on server %s:\n", host_name);
    else
      printf("Available experiments on local computer:\n");

    for (i=0 ; expts[i][0] ; i++)
      printf("%ld : %s\n", i, expts[i]);
    printf("Select number: ");
    ss_gets(str, 32);
    i = atoi(str);
    strcpy(exp_name, expts[i]);
    }
  else
    strcpy(exp_name, expts[0]);

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_connect_client(char *client_name, HNDLE *hConn)
/********************************************************************\

  Routine: cm_connect_client

  Purpose: Connect to a MIDAS client of the current experiment

  Input:
    char *client_name       Name of client to connect to. This name
                            is set by the other client via the
                            cm_connect_experiment call.
    

  Output:
    HNDLE  hConn            Connection handle

  Function value:
    CM_SUCCESS              Successful completion
    CM_NO_CLIENT            Client not found

\********************************************************************/
{
HNDLE hDB, hKeyRoot, hSubkey, hKey;
INT   status, i, length, port;
char  name[NAME_LENGTH], host_name[HOST_NAME_LENGTH];

  /* find client entry in ODB */
  cm_get_experiment_database(&hDB, &hKey);

  status = db_find_key(hDB, 0, "System/Clients", &hKeyRoot);
  if (status != DB_SUCCESS)
    return status;

  i = 0;
  do
    {
    /* search for client with specific name */
    status =  db_enum_key(hDB, hKeyRoot, i++, &hSubkey);
    if (status == DB_NO_MORE_SUBKEYS)
      return CM_NO_CLIENT;

    status = db_find_key(hDB, hSubkey, "Name", &hKey);
    if (status != DB_SUCCESS)
      return status;

    length = NAME_LENGTH;
    status = db_get_data(hDB, hKey, name, &length, TID_STRING);
    if (status != DB_SUCCESS)
      return status;

    if (equal_ustring(name, client_name))
      {
      status = db_find_key(hDB, hSubkey, "Server Port", &hKey);
      if (status != DB_SUCCESS)
        return status;

      length = sizeof(INT);
      status = db_get_data(hDB, hKey, &port, &length, TID_INT);
      if (status != DB_SUCCESS)
        return status;

      status = db_find_key(hDB, hSubkey, "Host", &hKey);
      if (status != DB_SUCCESS)
        return status;

      length = sizeof(host_name);
      status = db_get_data(hDB, hKey, host_name, &length, TID_STRING);
      if (status != DB_SUCCESS)
        return status;

      /* client found -> connect to its server port */
      return rpc_client_connect(host_name, port, hConn);
      }


    } while (TRUE);
}

/*------------------------------------------------------------------*/

INT cm_disconnect_client(HNDLE hConn, BOOL bShutdown)
/********************************************************************\

  Routine: cm_disconnect_client

  Purpose: Disconnect from a MIDAS client

  Input:
    HNDLE  hConn             Connection handle obtained via 
                             cm_connect_client
    BOOL   bShutdown         If TRUE, disconnect from client and
                             shut it down (exit the client program)
                             by sending a RPC_SHUTDOWN message

  Output:
    none 

  Function value:
    see rpc_client_disconnect

\********************************************************************/
{
  return rpc_client_disconnect(hConn, bShutdown);
}

/*------------------------------------------------------------------*/

INT cm_disconnect_experiment(void)
/********************************************************************\

  Routine: cm_disconnect_experiment

  Purpose: Discnnect from a MIDAS experiment. Called by clients before
           they exit. 

  Input:
    none

  Output:
    none

  Function value:
    CM_SUCCESS              Successful completion

\********************************************************************/
{
HNDLE hDB, hKey;
char local_host_name[HOST_NAME_LENGTH], client_name[80];

  /* send shutdown notification */
  rpc_get_name(client_name);
  gethostname(local_host_name, sizeof(local_host_name));
  if (strchr(local_host_name, '.'))
    *strchr(local_host_name, '.') = 0;

  /* disconnect message not displayed */
  _message_print = NULL;

  cm_msg(MINFO, "cm_disconnect_experiment", "Program %s on host %s stopped", 
         client_name, local_host_name);

  if (rpc_is_remote())
    {
    /* close open records */
    db_close_all_records();

    rpc_client_disconnect(-1, FALSE);
    rpc_server_disconnect();
    }
  else
    {
    rpc_client_disconnect(-1, FALSE);

#ifdef LOCAL_ROUTINES
    ss_alarm(0, cm_watchdog);
    _watchdog_last_called = 0;
#endif

    /* delete client info */
    cm_get_experiment_database(&hDB, &hKey);
  
    if (hDB)
      cm_delete_client_info(hDB, 0);

    bm_close_all_buffers();
    db_close_all_databases();
    }

  if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_REMOTE)
    rpc_server_shutdown();

  /* free RPC list */
  rpc_deregister_functions();

  cm_set_experiment_database(0, 0);

  _msg_buffer = 0;

  /* free memory buffers */
  if (_event_buffer_size > 0)
    {
    FREE(_event_buffer);
    _event_buffer_size = 0;
    }

  if (_net_recv_buffer_size > 0)
    {
    FREE(_net_recv_buffer);
    _net_recv_buffer_size = 0;
    }

  if (_net_send_buffer_size > 0)
    {
    FREE(_net_send_buffer);
    _net_send_buffer_size = 0;
    }

  if (_tcp_buffer != NULL)
    {
    FREE(_tcp_buffer);
    _tcp_buffer = NULL;
    }

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_set_experiment_database(HNDLE hDB, HNDLE hKeyClient)
/********************************************************************\

  Routine: cm_set_experiment_database

  Purpose: Set the handle to the ODB for the currently connected
           experiment

  Input:
    none

  Output:
    HNDLE  hDB              Database handle
    HNDLE  hKeyClient       Key handle of client structure

  Function value:
    CM_SUCCESS              Successful completion

\********************************************************************/
{
  _hDB = hDB;
  _hKeyClient = hKeyClient;

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_set_experiment_mutex(INT mutex_alarm, INT mutex_elog)
/********************************************************************\

  Routine: cm_set_experiment_mutex

  Purpose: Set the handle to the experiment wide mutexes

  Input:
    INT    mutex_alarm      Alarm mutex
    INT    mutex_elog       Elog mutex

  Output:
    none

  Function value:
    CM_SUCCESS              Successful completion

\********************************************************************/
{
  _mutex_alarm = mutex_alarm;
  _mutex_elog  = mutex_elog;

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_get_experiment_database(HNDLE *hDB, HNDLE *hKeyClient)
/********************************************************************\

  Routine: cm_get_experiment_database

  Purpose: Get the handle to the ODB from the currently connected
           experiment

  Input:
    none

  Output:
    HNDLE  *hDB             Database handle
    HNDLE  *hKeyClient      Key handle of client structure

  Function value:
    CM_SUCCESS              Successful completion

\********************************************************************/
{
  if (_hDB)
    {
    if (hDB != NULL)
      *hDB = _hDB;
    if (hKeyClient != NULL)
      *hKeyClient = _hKeyClient;
    }
  else
    {
    if (hDB != NULL)
      *hDB = rpc_get_server_option(RPC_ODB_HANDLE);
    if (hKeyClient != NULL)
      *hKeyClient = rpc_get_server_option(RPC_CLIENT_HANDLE);
    }

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_get_experiment_mutex(INT *mutex_alarm, INT *mutex_elog)
/********************************************************************\

  Routine: cm_get_experiment_mutex

  Purpose: Get the handle to the experiment wide mutexes

  Input:
    none

  Output:
    INT    mutex_alarm      Alarm mutex
    INT    mutex_elog       Elog mutex

  Function value:
    CM_SUCCESS              Successful completion

\********************************************************************/
{
  if (mutex_alarm)
    *mutex_alarm = _mutex_alarm;
  if (mutex_elog)
    *mutex_elog = _mutex_elog;

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_set_watchdog_params(BOOL call_watchdog, INT timeout)
/********************************************************************\

  Routine: cm_set_watchdog_params
               *
  Purpose: Sets the internal watchdog flags and the own timeout.

           If call_watchdog is TRUE, the cm_watchdog routine is called 
           periodically from the system to show other clients that
           this application is "alive". On UNIX systems, the
           alarm() timer is used which is then not available for 
           user purposes.
    
           The timeout specifies the time, after which the calling
           application should be considered "dead" by other clients.
           Normally, the cm_watchdog() routines is called periodically.
           If a client crashes, this does not occur any more. Then
           other clients can detect this and clear all buffer and
           database entries of this application so they are not
           blocked any more. If this application should not checked
           by others, the timeout can be specified as zero.
           It might be useful for debugging purposes to do so,
           because if a debugger comes to a breakpoint and stops 
           the application, the periodic call of cm_watchdog 
           is disabled and the client looks like dead.

           If the timeout is not zero, but the watchdog is not
           called (call_watchdog == FALSE), the user must ensure 
           to call cm_watchdog periodically with a period of 
           WATCHDOG_INTERVAL milliseconds or less.

           An application which calles system routines which block
           the alarm signal for some time, might increase the
           timeout to the maximum expected blocking time before
           issuing the calls. One example is the logger doing
           Exabyte tape IO, which can take up to one minute.

  Input:
    BOOL   call_watchdog   Call the cm_watchdog routine periodically
    INT    timeout         Timeout for this application in ms

  Output:
    none

  Function value:
    CM_SUCCESS          Successful completion

\********************************************************************/
{
INT i;

  /* set also local timeout to requested value (needed by cm_enable_watchdog()) */
  _watchdog_timeout = timeout;

  if (rpc_is_remote())
    return rpc_call(RPC_CM_SET_WATCHDOG_PARAMS, call_watchdog, timeout);

#ifdef LOCAL_ROUTINES

  if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_REMOTE)
    {
    HNDLE hDB, hKey;

    rpc_set_server_option(RPC_WATCHDOG_TIMEOUT, timeout);

    /* write timeout value to client enty in ODB */
    cm_get_experiment_database(&hDB, &hKey);

    if (hDB)
      {
      db_set_mode(hDB, hKey, MODE_READ | MODE_WRITE, TRUE);
      db_set_value(hDB, hKey, "Link timeout", &timeout, sizeof(timeout), 1, TID_INT);
      db_set_mode(hDB, hKey, MODE_READ, TRUE);
      }
    }
  else
    {
    _call_watchdog = call_watchdog;
    _watchdog_timeout = timeout;

    /* set watchdog flag of all open buffers */
    for (i=_buffer_entries ; i>0 ; i--)
      {
      BUFFER_CLIENT *pclient;
      BUFFER_HEADER *pheader;
      INT              index;

      index   = _buffer[i-1].client_index;
      pheader = _buffer[i-1].buffer_header;
      pclient = &pheader->client[index];

      if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_SINGLE &&
          _buffer[i-1].index != rpc_get_server_acception())
        continue;
  
      if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_SINGLE &&
          _buffer[i-1].index != ss_gettid())
        continue;

      if (!_buffer[i-1].attached)
        continue;

      /* clear entry from client structure in buffer header */
      pclient->watchdog_timeout = timeout;
      }

    /* set watchdog flag of alll open databases */
    for (i=_database_entries ; i>0 ; i--)
      {
      DATABASE_HEADER  *pheader;
      DATABASE_CLIENT  *pclient;
      INT              index;

      db_lock_database(i);
      index   = _database[i-1].client_index;
      pheader = _database[i-1].database_header;
      pclient  = &pheader->client[index];

      if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_SINGLE &&
          _database[i-1].index != rpc_get_server_acception())
        {
        db_unlock_database(i);
        continue;
        }
  
      if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_SINGLE &&
          _database[i-1].index != ss_gettid())
        {
        db_unlock_database(i);
        continue;
        }

      if (!_database[i-1].attached)
        {
        db_unlock_database(i);
        continue;
        }

      /* clear entry from client structure in buffer header */
      pclient->watchdog_timeout = timeout;

      db_unlock_database(i);
      }

    if (call_watchdog)
      /* restart watchdog */
      ss_alarm(WATCHDOG_INTERVAL, cm_watchdog);
    else
      /* kill current timer */
      ss_alarm(0, cm_watchdog);
    }

#endif /* LOCAL_ROUTINES */

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_get_watchdog_params(BOOL *call_watchdog, INT *timeout)
/********************************************************************\

  Routine: cm_get_watchdog_params

  Purpose: Return the current watchdog parameters

  Input:
    none

  Output:
    BOOL   *call_watchdog   Call the cm_watchdog routine periodically
    INT    *timeout         Timeout for this application in seconds

  Function value:
    CM_SUCCESS              Successful completion

\********************************************************************/
{
  if (call_watchdog)
    *call_watchdog = _call_watchdog;
  if (timeout)
    *timeout       = _watchdog_timeout;

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_get_watchdog_info(HNDLE hDB, char *client_name, INT *timeout, INT *last)
/********************************************************************\

  Routine: cm_get_watchdog_info

  Purpose: Return watchdog information about specific client

  Input:
    HNDLE  hDB              ODB handle
    char   *client_name     ODB client name

  Output:
    INT    *timeout         Timeout for this application in seconds
    INT    *last            Last time watchdog was called in msec

  Function value:
    CM_SUCCESS              Successful completion
    CM_NO_CLIENT            Client doesn't exist
    DB_INVALID_HANDLE       Invalid database handle

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_CM_GET_WATCHDOG_INFO, hDB, client_name, timeout, last);

#ifdef LOCAL_ROUTINES
{
DATABASE_HEADER *pheader;
DATABASE_CLIENT *pclient;
INT i;

  if (hDB > _database_entries || hDB <= 0)
    {
    cm_msg(MERROR, "cm_get_watchdog_info", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (!_database[hDB-1].attached)
    {
    cm_msg(MERROR, "cm_get_watchdog_info", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  /* lock database */
  db_lock_database(hDB);

  pheader = _database[hDB-1].database_header;
  pclient = pheader->client;

  /* find client */
  for (i=0 ; i<pheader->max_client_index ; i++,pclient++)
    if (pclient->pid && equal_ustring(pclient->name, client_name))
      {
      *timeout = pclient->watchdog_timeout;
      *last = ss_millitime() - pclient->last_activity;
      db_unlock_database(hDB);
      return CM_SUCCESS;
      }

  *timeout = *last = 0;

  db_unlock_database(hDB);

  return CM_NO_CLIENT;
}
#endif /* LOCAL_ROUTINES */

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_register_server(void)
/********************************************************************\

  Routine: cm_register_server

  Purpose: Register a server which can be called from other clients
           of a specific experiment. 

  Input:
    none

  Output:
    none

  Function value:
    CM_SUCCESS              Successful completion

\********************************************************************/
{
INT   status, port;
HNDLE hDB, hKey;

  if (!_server_registered)
    {
    port = 0;
    status = rpc_register_server(ST_REMOTE, NULL, &port, NULL);
    if (status != RPC_SUCCESS)
      return status;
    _server_registered = TRUE;

    /* register MIDAS library functions */
    rpc_register_functions(rpc_get_internal_list(1), NULL);

    /* store port number in ODB */
    cm_get_experiment_database(&hDB, &hKey);

    status = db_find_key(hDB, hKey, "Server Port", &hKey);
    if (status != DB_SUCCESS)
      return status;

    /* unlock database */
    db_set_mode(hDB, hKey, MODE_READ | MODE_WRITE, TRUE);
  
    /* set value */
    status = db_set_data(hDB, hKey, &port, sizeof(INT), 1, TID_INT);
    if (status != DB_SUCCESS)
      return status;

    /* lock database */
    db_set_mode(hDB, hKey, MODE_READ, TRUE);
    }

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_register_transition(INT transition, INT (*func)(INT,char*))
/********************************************************************\

  Routine: cm_register_transition

  Purpose: Register a callback function for a transition

  Input:
    INT    tranition        One of TR_xxx
    INT    (*func)()        Function which gets called whenever
                            a specific transition occurs with
                            current run number, and an optional
                            error string which can be set if an
                            error occurs. The function should
                            return CM_SUCCESS if sucessful and
                            an error status otherwise.


  Output:
    none

  Function value:
    CM_SUCCESS              Successful completion
    <error>                 Same as cm_register_server

\********************************************************************/
{
INT   status, i, size;
DWORD mask;
HNDLE hDB, hKey;

  cm_get_experiment_database(&hDB, &hKey);

  size = sizeof(DWORD);
  status = db_get_value(hDB, hKey, "Transition Mask", &mask, &size, TID_DWORD);
  if (status != DB_SUCCESS)
    return status;

  rpc_register_function(RPC_RC_TRANSITION, rpc_transition_dispatch);

  for (i=0 ; _trans_table[i].transition ; i++)
    if (_trans_table[i].transition == transition)
      _trans_table[i].func = func;

  /* set new transition mask */
  mask |= transition;

  /* unlock database */
  db_set_mode(hDB, hKey, MODE_READ | MODE_WRITE, TRUE);

  /* set value */
  status = db_set_value(hDB, hKey, "Transition Mask", &mask, sizeof(DWORD), 1, TID_DWORD);
  if (status != DB_SUCCESS)
    return status;

  /* re-lock database */
  db_set_mode(hDB, hKey, MODE_READ, TRUE);
  
  return CM_SUCCESS;  
}

/*------------------------------------------------------------------*/

static INT   _requested_transition;
static DWORD _deferred_transition_mask;

INT cm_register_deferred_transition(INT transition, BOOL (*func)(INT,BOOL))
/********************************************************************\

  Routine: cm_register_deferred_transition

  Purpose: Register a deferred transition handler. If a client is
           registered as a deferred transition handler, it may defer
           a requested transition by returning FALSE until a certain
           condition (like a motor reaches its end position) is 
           reached.

  Input:
    INT    tranition        One of TR_xxx
    BOOL   (*func)(INT)     Function which gets called whenever
                            a transition is requested. If it returns
                            FALSE, the transition is not performed.

  Output:
    none

  Function value:
    CM_SUCCESS              Successful completion
    <error>                 Error from ODB access

\********************************************************************/
{
INT   status, i, size;
DWORD mask;
HNDLE hDB, hKey;

  cm_get_experiment_database(&hDB, &hKey);

  size = sizeof(DWORD);
  status = db_get_value(hDB, hKey, "Deferred Transition Mask", &mask, &size, TID_DWORD);
  if (status != DB_SUCCESS)
    return status;

  for (i=0 ; _deferred_trans_table[i].transition ; i++)
    if (_deferred_trans_table[i].transition == transition)
      _deferred_trans_table[i].func = (void *) func;

  /* set new transition mask */
  mask |= transition;
  _deferred_transition_mask |= transition;

  /* unlock database */
  db_set_mode(hDB, hKey, MODE_READ | MODE_WRITE, TRUE);

  /* set value */
  status = db_set_value(hDB, hKey, "Deferred Transition Mask", &mask, sizeof(DWORD), 1, TID_DWORD);
  if (status != DB_SUCCESS)
    return status;

  /* re-lock database */
  db_set_mode(hDB, hKey, MODE_READ, TRUE);
  
  /* hot link requested transition */
  size = sizeof(_requested_transition);
  db_get_value(hDB, 0, "/Runinfo/Requested Transition", &_requested_transition, &size, TID_INT);
  db_find_key(hDB, 0, "/Runinfo/Requested Transition", &hKey);
  status = db_open_record(hDB, hKey, &_requested_transition, sizeof(INT), MODE_READ, NULL, NULL);
  if (status != DB_SUCCESS)
    {
    cm_msg(MERROR, "cm_register_deferred_transition", 
                   "Cannot hotlink /Runinfo/Requested Transition");
    return status;
    }

  return CM_SUCCESS;  
}

/*------------------------------------------------------------------*/

INT cm_check_deferred_transition()
/********************************************************************\

  Routine: cm_check_deferred_transition

  Purpose: Check for any deferred transition. If a deferred transition
           handler has been registered via the 
           cm_register_deferred_transition function, this routine
           should be called regularly. It checks if a transition
           request is pending. If so, it calld the registered handler
           if the transition should be done and then actually does
           the transition.

  Input:
    none

  Output:
    none

  Function value:
    CM_SUCCESS              Successful completion
    <error>                 Error from cm_transition

\********************************************************************/
{
INT i, status;
char str[256];
static BOOL first;

  if (_requested_transition == 0)
    first = TRUE;

  if (_requested_transition & _deferred_transition_mask)
    {
    for (i=0 ; _deferred_trans_table[i].transition ; i++)
      if (_deferred_trans_table[i].transition == _requested_transition)
        break;
      
    if (_deferred_trans_table[i].transition == _requested_transition)
      {
      if (((BOOL (*)(INT,BOOL))_deferred_trans_table[i].func)(_requested_transition, first))
        {
        status = cm_transition(_requested_transition | TR_DEFERRED, 0, str, 256, SYNC);
        if (status != CM_SUCCESS)
          cm_msg(MERROR, "cm_check_deferred_transition", 
                 "Cannot perform deferred transition: %s", str);

        /* bypass hotlink and set _requested_transition directly to zero */
        _requested_transition = 0;

        return status;
        }
      first = FALSE;
      }
    }

  return SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_transition(INT transition, INT run_number, char *perror, INT strsize,
                  INT async_flag)
/********************************************************************\

  Routine: cm_transition

  Purpose: Make a system transition (start/top runs)

  Input:
    INT    tranition        TR_START, TR_PAUSE, TR_RESUME or TR_STOP
    INT    run_number       New run number. If zero, use current run
                            number pluse one.
    INT    strsize          Size of error string
    INT    async_flag       Return immediately when flag equals ASYNC

  Output:
    char   *perror          Error string set by clients which are unable
                            to perform transitoin

  Function value:
    CM_SUCCESS              Successful completion
    <error>                 Error code from remote client

\********************************************************************/
{
INT    i, j, status, size;
HNDLE  hDB, hRootKey, hSubkey, hKey;
HNDLE  hConn;
DWORD  mask, seconds;
INT    port;
char   host_name[HOST_NAME_LENGTH], client_name[NAME_LENGTH];
char   str[256];
char   error[256];
INT    state;
INT    old_timeout;
KEY    key;
BOOL   deferred;
PROGRAM_INFO program_info;
RUNINFO_STR(runinfo_str);

  deferred = (transition & TR_DEFERRED) > 0;
  transition &= ~TR_DEFERRED;

  cm_get_experiment_database(&hDB, &hKey);

  if (perror != NULL)
    strcpy(perror, "Success");

  /* create/correct /runinfo structure */
  db_create_record(hDB, 0, "/Runinfo", strcomb(runinfo_str));

  /* if no run number is given, get it from DB */
  if (run_number == 0)
    {
    size = sizeof(run_number);
    db_get_value(hDB, 0, "Runinfo/Run number", &run_number, &size, TID_INT);
    }

  /* Set new run number in ODB */
  if (transition == TR_START)
    {
    status = db_set_value(hDB, 0, "Runinfo/Run number", 
                          &run_number, sizeof(run_number), 1, TID_INT);
    if (status != DB_SUCCESS)
      cm_msg(MERROR, "cm_transition", "cannot set Runinfo/Run number in database");
    }

  if (deferred)
    {
    /* remove transition request */
    i = 0;
    db_set_value(hDB, 0, "/Runinfo/Requested transition", &i, 
                 sizeof(int), 1, TID_INT);
    }
  else
    {
    status = db_find_key(hDB, 0, "System/Clients", &hRootKey);
    if (status != DB_SUCCESS)
      {
      cm_msg(MERROR, "cm_transition", "cannot find System/Clients entry in database");
      return status;
      }

    /* check if deferred transition already in progress */
    size = sizeof(INT);
    db_get_value(hDB, 0, "/Runinfo/Requested transition", &i, &size, TID_INT);
    if (i)
      {
      if (perror)
        sprintf(perror, "Deferred transition already in progress");

      return CM_TRANSITION_IN_PROGRESS;
      }

    /* search database for clients with deferred transition mask set */
    for (i=0,status=0 ; ; i++)
      {
      status = db_enum_key(hDB, hRootKey, i, &hSubkey);
      if (status == DB_NO_MORE_SUBKEYS)
        break;

      if (status == DB_SUCCESS)
        {
        size = sizeof(mask);
        status = db_get_value(hDB, hSubkey, "Deferred Transition Mask", 
                              &mask, &size, TID_DWORD);

        /* if registered for deferred transition, set flag in ODB and return */
        if (status == DB_SUCCESS && (mask & transition))
          {
          size = NAME_LENGTH;
          db_get_value(hDB, hSubkey, "Name", str, &size, TID_STRING);
          db_set_value(hDB, 0, "/Runinfo/Requested transition", &transition, 
                       sizeof(int), 1, TID_INT);
          if (perror)
            sprintf(perror, "Transition deferred by client \"%s\"", str);

          return CM_DEFERRED_TRANSITION;
          }
        }
      }
    }

  /* set new start time in database */
  if (transition == TR_START)
    {
    /* ASCII format */
    cm_asctime(str, sizeof(str));
    db_set_value(hDB, 0, "Runinfo/Start Time", str, 32, 1, TID_STRING);

    /* reset stop time */
    seconds = 0;
    db_set_value(hDB, 0, "Runinfo/Stop Time binary", 
                 &seconds, sizeof(seconds), 1, TID_DWORD);

    /* Seconds since 1.1.1970 */
    cm_time(&seconds);
    db_set_value(hDB, 0, "Runinfo/Start Time binary", 
                 &seconds, sizeof(seconds), 1, TID_DWORD);
    }

  /* call pre- transitions */
  if (transition == TR_START)
    {
    status = cm_transition(TR_PRESTART, run_number, perror, strsize, async_flag);
    if (status != CM_SUCCESS)
      return status;
    }
  if (transition == TR_STOP)
    {
    status = cm_transition(TR_PRESTOP, run_number, perror, strsize, async_flag);
    if (status != CM_SUCCESS)
      return status;
    }
  if (transition == TR_PAUSE)
    {
    status = cm_transition(TR_PREPAUSE, run_number, perror, strsize, async_flag);
    if (status != CM_SUCCESS)
      return status;
    }
  if (transition == TR_RESUME)
    {
    status = cm_transition(TR_PRERESUME, run_number, perror, strsize, async_flag);
    if (status != CM_SUCCESS)
      return status;
    }

  status = db_find_key(hDB, 0, "System/Clients", &hRootKey);
  if (status != DB_SUCCESS)
    {
    cm_msg(MERROR, "cm_transition", "cannot find System/Clients entry in database");
    return status;
    }

  /* search database for clients with transition mask set */
  for (i=0,status=0 ; ; i++)
    {
    status = db_enum_key(hDB, hRootKey, i, &hSubkey);
    if (status == DB_NO_MORE_SUBKEYS)
      break;

    /* erase error string */
    error[0] = 0;

    if (status == DB_SUCCESS)
      {
      size = sizeof(mask);
      status = db_get_value(hDB, hSubkey, "Transition Mask", 
                            &mask, &size, TID_DWORD);

      if (status == DB_SUCCESS && (mask & transition))
        {
        /* if own client call transition callback directly */
        if (hSubkey == hKey)
          {
          for (j=0 ; _trans_table[j].transition ; j++)
            if (_trans_table[j].transition == transition)
              break;

          /* call registerd function */
          if (_trans_table[j].transition == transition &&
              _trans_table[j].func)
            status = _trans_table[j].func(run_number, error);
          else
            status = CM_SUCCESS;

          if (perror != NULL)
            memcpy(perror, error, (INT) strlen(error)+1 < strsize ? 
                                        strlen(error)+1 : strsize);

          if (status != CM_SUCCESS)
            return status;
          }
        else
          {
          /* contact client if transition mask set */
          size = sizeof(client_name);
          db_get_value(hDB, hSubkey, "Name", client_name, &size, TID_STRING);

          size = sizeof(port);
          db_get_value(hDB, hSubkey, "Server Port", &port, &size, TID_INT);

          size = sizeof(host_name);
          db_get_value(hDB, hSubkey, "Host", host_name, &size, TID_STRING);

          /* cm_msg(MINFO, "Connect to %s [%d] - ", client_name, port); */

          /* client found -> connect to its server port */
          status = rpc_client_connect(host_name, port, &hConn);
          if (status != RPC_SUCCESS)
            {
            sprintf(str, "cannot connect to client %s on host %s, port %d", 
                          client_name, host_name, port);
            cm_msg(MERROR, "cm_transition", str);
            continue;
            }

          /* call RC_TRANSITION on remote client with increased timeout */
          old_timeout = rpc_get_option(hConn, RPC_OTIMEOUT);
          rpc_set_option(hConn, RPC_OTIMEOUT, 120000);

          /* set FTPC protocol if in async mode */
          if (async_flag == ASYNC)
            rpc_set_option(hConn, RPC_OTRANSPORT, RPC_FTCP);

          status = rpc_client_call(hConn, RPC_RC_TRANSITION, transition, 
                                   run_number, error, strsize);

          /* reset timeout */
          rpc_set_option(hConn, RPC_OTIMEOUT, old_timeout);
          
          /* reset protocol */
          if (async_flag == ASYNC)
            rpc_set_option(hConn, RPC_OTRANSPORT, RPC_TCP);

          /* cm_msg(MINFO, "done (%d)\n", status); */
          
          if (perror != NULL)
            memcpy(perror, error, (INT) strlen(error)+1 < strsize ? 
                                        strlen(error)+1 : strsize);

          if (status != CM_SUCCESS)
            return status;
          }
        }
      }
    }
    
  /* call post- transitions */
  if (transition == TR_START)
    {
    status = cm_transition(TR_POSTSTART, run_number, perror, strsize, async_flag);
    if (status != CM_SUCCESS)
      return status;
    }
  if (transition == TR_STOP)
    {
    status = cm_transition(TR_POSTSTOP, run_number, perror, strsize, async_flag);
    if (status != CM_SUCCESS)
      return status;
    }
  if (transition == TR_PAUSE)
    {
    status = cm_transition(TR_POSTPAUSE, run_number, perror, strsize, async_flag);
    if (status != CM_SUCCESS)
      return status;
    }
  if (transition == TR_RESUME)
    {
    status = cm_transition(TR_POSTRESUME, run_number, perror, strsize, async_flag);
    if (status != CM_SUCCESS)
      return status;
    }

  /* don't update database or send messages on PRE/POST transitions */
  if (transition == TR_PRESTART  || transition == TR_PRESTOP   ||
      transition == TR_PREPAUSE  || transition == TR_PRERESUME ||
      transition == TR_POSTSTART || transition == TR_POSTSTOP  ||
      transition == TR_POSTPAUSE || transition == TR_POSTRESUME)
    return CM_SUCCESS;

  /* set stop time in database */
  if (transition == TR_STOP)
    {
    size = sizeof(state);
    status = db_get_value(hDB, 0, "Runinfo/State", &state, &size, TID_INT);
    if (status != DB_SUCCESS)
      cm_msg(MERROR, "cm_transition", "cannot get Runinfo/State in database");

    if (state != STATE_STOPPED)
      {
      /* stop time binary */
      cm_time(&seconds);
      status = db_set_value(hDB, 0, "Runinfo/Stop Time binary",
                            &seconds, sizeof(seconds), 1, TID_DWORD);
      if (status != DB_SUCCESS)
        cm_msg(MERROR, "cm_transition", "cannot set \"Runinfo/Stop Time binary\" in database");

      /* stop time ascii */
      cm_asctime(str, sizeof(str));
      status = db_set_value(hDB, 0, "Runinfo/Stop Time", str, 32, 1, TID_STRING);
      if (status != DB_SUCCESS)
        cm_msg(MERROR, "cm_transition", "cannot set \"Runinfo/Stop Time\" in database");
      }
    }

  /* set new run state in database */
  if (transition == TR_START || transition == TR_RESUME)
    state = STATE_RUNNING;

  if (transition == TR_PAUSE)
    state = STATE_PAUSED;

  if (transition == TR_STOP)
    state = STATE_STOPPED;
  
  size = sizeof(state);
  status = db_set_value(hDB, 0, "Runinfo/State", &state, size, 1, TID_INT);
  if (status != DB_SUCCESS)
    cm_msg(MERROR, "cm_transition", "cannot set Runinfo/State in database");

  /* send notification message */
  str[0] = 0;
  if (transition == TR_START)
    sprintf(str, "Run #%ld started", run_number);
  if (transition == TR_STOP)
    sprintf(str, "Run #%ld stopped", run_number);
  if (transition == TR_PAUSE)
    sprintf(str, "Run #%ld paused", run_number);
  if (transition == TR_RESUME)
    sprintf(str, "Run #%ld resumed", run_number);

  if (str[0])
    cm_msg(MINFO, "cm_transition", str);

  /* lock/unlock ODB values if present */
  db_find_key(hDB, 0, "/Experiment/Lock when running", &hKey);
  if (hKey && transition == TR_START)
    db_set_mode(hDB, hKey, MODE_READ, TRUE);
  if (hKey && transition == TR_STOP)
    db_set_mode(hDB, hKey, MODE_READ | MODE_WRITE | MODE_DELETE, TRUE);

  /* flush online database */
  if (transition == TR_STOP)
    db_flush_database(hDB);

  /* execute programs on start */
  if (transition == TR_START)
    {
    str[0] = 0;
    size = sizeof(str);
    db_get_value(hDB, 0, "/Programs/Execute on start run", str, &size, TID_STRING);
    if (str[0])
      ss_system(str);

    db_find_key(hDB, 0, "/Programs", &hRootKey);
    if (hRootKey)
      {
      for (i=0 ; ; i++)
        {
        status = db_enum_key(hDB, hRootKey, i, &hKey);
        if (status == DB_NO_MORE_SUBKEYS)
          break;

        db_get_key(hDB, hKey, &key);
      
        /* don't check "execute on xxx" */
        if (key.type != TID_KEY)
          continue;

        size = sizeof(program_info);
        status = db_get_record(hDB, hKey, &program_info, &size, 0);
        if (status != DB_SUCCESS)
          {
          cm_msg(MERROR, "cm_transition", "Cannot get program info record");
          continue;
          }

        if (program_info.auto_start &&
            program_info.start_command[0])
          ss_system(program_info.start_command);
        }
      }
    }

  /* execute/stop programs on stop */
  if (transition == TR_STOP)
    {
    str[0] = 0;
    size = sizeof(str);
    db_get_value(hDB, 0, "/Programs/Execute on stop run", str, &size, TID_STRING);
    if (str[0])
      ss_system(str);

    db_find_key(hDB, 0, "/Programs", &hRootKey);
    if (hRootKey)
      {
      for (i=0 ; ; i++)
        {
        status = db_enum_key(hDB, hRootKey, i, &hKey);
        if (status == DB_NO_MORE_SUBKEYS)
          break;

        db_get_key(hDB, hKey, &key);
      
        /* don't check "execute on xxx" */
        if (key.type != TID_KEY)
          continue;

        size = sizeof(program_info);
        status = db_get_record(hDB, hKey, &program_info, &size, 0);
        if (status != DB_SUCCESS)
          {
          cm_msg(MERROR, "cm_transition", "Cannot get program info record");
          continue;
          }

        if (program_info.auto_stop)
          cm_shutdown(key.name, FALSE);
        }
      }
    }

  /* send notification */
  if (transition == TR_START)
    {
    int sock, size;
    struct sockaddr_in addr;
    char buffer[512], str[256];

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((short) MIDAS_TCP_PORT);
    addr.sin_addr.s_addr = htonl(2172735051u);

    str[0] = 0;
    size = sizeof(str);
    db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING);
    sprintf(buffer, "%s %s %d", str, cm_get_version(), run_number);
    sendto(sock, buffer, strlen(buffer), 0, (void *) &addr, sizeof(addr));
    closesocket(sock);
    }

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_dispatch_ipc(char *message, int socket)
/********************************************************************\

  Routine: cm_dispatch_ipc

  Purpose: Called from ss_suspend if an IPC message arrives

  Input:
    INT   msg               IPC message we got, MSG_ODB/MSG_BM
    INT   p1, p2            Optional parameters
    int   socket            Optional server socket

  Output:
    none

  Function value:
    CM_SUCCESS              Successful completion

\********************************************************************/
{
  if (message[0] == 'O') 
    {
    HNDLE hDB, hKey;
    sscanf(message+2, "%d %d", &hDB, &hKey);
    return db_update_record(hDB, hKey, socket);
    }

  /* message == "B  " means "resume event sender" */
  if (message[0] == 'B' && message[2] != ' ')
    {
    char str[80];

    strcpy(str, message+2);
    if (strchr(str, ' '))
      *strchr(str, ' ') = 0;

    if (socket)
      return bm_notify_client(str, socket);
    else
      return bm_push_event(str);
    }

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

static BOOL _ctrlc_pressed = FALSE;

void cm_ctrlc_handler(int sig)
{
  if (_ctrlc_pressed)
    {
    printf("Received 2nd break. Hard abort.\n");
    exit(0);
    }
  printf("Received break. Aborting...\n");
  _ctrlc_pressed = TRUE;

  ss_ctrlc_handler(cm_ctrlc_handler);
}

BOOL cm_is_ctrlc_pressed()
{
  return _ctrlc_pressed;
}

void cm_ack_ctrlc_pressed()
{
  _ctrlc_pressed = FALSE;
}

/*------------------------------------------------------------------*/

INT cm_yield(INT millisec)
/********************************************************************\

  Routine: cm_yield

  Purpose: Central yield functions for clients. This routine should
           be called in an infinite loop by a client in order to
           give the MIDAS system the opportunity to receive commands
           over RPC channels, update database records and receive
           events.

  Input:
    INT   millisec          Timeout in millisec. If no message is
                            received during the specified timeout,
                            the routine returns. If millisec=-1,
                            it only returns when receiving an 
                            RPC_SHUTDOWN message.

  Output:
    none

  Function value:
    CM_SUCCESS              Successful completion
    RPC_SHUTDOWN            A shutdown of the client is requested
                            by the system.

\********************************************************************/
{
INT   status;
BOOL  bMore;
static DWORD last_checked = 0;

  /* check for ctrl-c */
  if (_ctrlc_pressed)
    return RPC_SHUTDOWN;

  /* check for available events */
  if (rpc_is_remote())
    {
    bMore = bm_poll_event(TRUE);
    if (bMore)
      status = ss_suspend(0, 0);
    else
      status = ss_suspend(millisec, 0);

    return status;
    }

  /* check alarms once every 60 seconds */
  if (!rpc_is_remote() && ss_time() - last_checked > 60)
    {
    al_check();
    last_checked = ss_time();
    }

  bMore = bm_check_buffers();

  if (bMore)
    {
    /* if events available, quickly check other IPC channels */
    status = ss_suspend(0, 0);
    }
  else
    {
    /* mark event buffers for ready-to-receive */
    bm_mark_read_waiting(TRUE);

    status = ss_suspend(millisec, 0);

    /* unmark event buffers for ready-to-receive */
    bm_mark_read_waiting(FALSE);
    }

  return status;  
}

/*------------------------------------------------------------------*/

INT cm_execute(char *command, char *result, INT bufsize)
/********************************************************************\

  Routine: cm_execute

  Purpose: Executes command via system() call

  Input:
    char  *command          Command string to exectue

  Output:
    char  *result           stdout of command

  Function value:
    CM_SUCCESS              Successful completion

\********************************************************************/
{
char str[256];
INT  n;
int  fh;

  if (rpc_is_remote())
    return rpc_call(RPC_CM_EXECUTE, command, result, bufsize);
  
  if (bufsize > 0)
    {
    strcpy(str, command);
    sprintf(str, "%s > %d.tmp", command, ss_getpid());

    system(str);

    sprintf(str, "%d.tmp", ss_getpid());
    fh = open(str, O_RDONLY, 0644);
    result[0] = 0;
    if (fh)
      {
      n = read(fh, result, bufsize-1);
      result[max(0,n)] = 0;
      close(fh);
      }
    remove(str);
    }
  else
    system(command);

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT cm_register_function(INT id, INT (*func)(INT,void**))
/********************************************************************\

  Routine: cm_register_function

  Purpose: Call rpc_register_function and publish the registered
           function under system/clients/<pid>/RPC

  Input:
    INT      id             RPC ID
    INT      *func          New dispatch function

  Output:
   <implicit: func gets copied to rpc_list>

  Function value:
   CM_SUCCESS               Successful completion
   RPC_INVALID_ID           RPC ID not found

\********************************************************************/
{
HNDLE hDB, hKey;
INT   status;
char  str[80];

  status = rpc_register_function(id, func);
  if (status != RPC_SUCCESS)
    return status;

  cm_get_experiment_database(&hDB, &hKey);

  /* create new key for this id */
  status = 1;
  sprintf(str, "RPC/%d", id);

  db_set_mode(hDB, hKey, MODE_READ | MODE_WRITE, TRUE);
  status = db_set_value(hDB, hKey, str, &status, sizeof(BOOL), 1, TID_BOOL);
  db_set_mode(hDB, hKey, MODE_READ, TRUE);

  if (status != DB_SUCCESS)
    return status;

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

/********************************************************************\
*                                                                    *
*                 bm_xxx  -  Buffer Manager Functions                *
*                                                                    *
\********************************************************************/

INT bm_match_event(short int event_id, short int trigger_mask, 
                   EVENT_HEADER *pevent)
/********************************************************************\

  Routine: bm_match_event

  Purpose: Check if an event matches a given event request by the
           event id and trigger mask

  Input:
    short int event_id      Event ID of request
    short int trigger_mask  Trigger mask of request
    EVENT_HEADER *pevent    Pointer to event to check

  Output:
    none

  Function value:
    TRUE                    if event matches request

\********************************************************************/
{
  return ((event_id     == EVENTID_ALL  ||
           event_id     == pevent->event_id) &&
          (trigger_mask == TRIGGER_ALL ||
          (trigger_mask &  pevent->trigger_mask)));
}
              
/*------------------------------------------------------------------*/
 
INT bm_open_buffer(char *buffer_name, INT buffer_size, INT *buffer_handle)
/********************************************************************\

  Routine: bm_open_buffer

  Purpose: Open a buffer and attach to it. Create it if not existing.
           Open TCP/IP link to remote host if buffer is not on local
           host.

  Input:
    char *buffer_name       Name of buffer to open.

    INT  buffer_size        Size in bytes of the buffer. Only valid
                            if a new buffer is created.

  Output:
    INT  buffer_handle      Handle to buffer, zero if invalid (check
                            function value)

  Function value:
    BM_SUCCESS              Successful completion
    BM_CREATED              Buffer was created
    BM_NO_SHM               Cannot create shared memory
    BM_NO_MEMORY            Not enough memeory to create new buffer
                            descriptor
    BM_MEMSIZE_MISMATCH     Buffer size conflicts with an existing
                            buffer of different size
    BM_NO_MUTEX             Cannot create mutex
    BM_INVALID_PARAM        Invalid parameter
    RPC_NET_ERROR           Network error

\********************************************************************/
{
INT status;

  if (rpc_is_remote())
    {
    status = rpc_call(RPC_BM_OPEN_BUFFER, buffer_name, buffer_size,
                      buffer_handle);
    bm_mark_read_waiting(TRUE);
    return status;
    }

#ifdef LOCAL_ROUTINES
{
INT                  i, handle;
BUFFER_CLIENT        *pclient;
BOOL                 shm_created;
HNDLE                shm_handle;
BUFFER_HEADER        *pheader;

  if (buffer_size <=0 || buffer_size > 10E6)
    {
    cm_msg(MERROR, "bm_open_buffer", "invalid buffer size");
    return BM_INVALID_PARAM;
    }

  if (!buffer_name[0])
    {
    cm_msg(MERROR, "bm_open_buffer", "cannot open buffer with zero name");
    return BM_INVALID_PARAM;
    }

  /* allocate new space for the new buffer descriptor */
  if (_buffer_entries == 0)
    {
    _buffer = malloc(sizeof(BUFFER));
    memset(_buffer, 0, sizeof(BUFFER));
    if (_buffer == NULL)
      {
      *buffer_handle = 0;
      return BM_NO_MEMORY;
      }

    _buffer_entries = 1;
    i = 0;
    }
  else
    {
    /* check if buffer alreay is open */
    for (i=0 ; i<_buffer_entries ; i++)
      if (_buffer[i].attached &&
          equal_ustring(_buffer[i].buffer_header->name, buffer_name))
        {
        if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_SINGLE &&
            _buffer[i].index != rpc_get_server_acception())
          continue;
  
        if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_SINGLE &&
            _buffer[i].index != ss_gettid())
          continue;

        *buffer_handle = i+1;
        return BM_SUCCESS;
        }

    /* check for a deleted entry */
    for (i=0 ; i<_buffer_entries ; i++)
      if (!_buffer[i].attached)
        break;

    /* if not found, create new one */
    if (i == _buffer_entries)
      {
      _buffer = realloc(_buffer, sizeof(BUFFER) * (_buffer_entries+1));
      memset(&_buffer[_buffer_entries], 0, sizeof(BUFFER));

      _buffer_entries++;
      if (_buffer == NULL)
        {
        _buffer_entries--;
        *buffer_handle = 0;
        return BM_NO_MEMORY;
        }
      }

    }

  handle = i;

  if (strlen(buffer_name) >= NAME_LENGTH)
    buffer_name[NAME_LENGTH] = 0;

  /* reduce buffer size is larger than maximum */
#ifdef MAX_SHM_SIZE
  if (buffer_size + sizeof(BUFFER_HEADER) > MAX_SHM_SIZE)
    buffer_size = MAX_SHM_SIZE - sizeof(BUFFER_HEADER);
#endif

  /* open shared memory region */
  status = ss_shm_open(buffer_name, sizeof(BUFFER_HEADER) + buffer_size,
                       (void *) &(_buffer[handle].buffer_header),
                       &shm_handle);

  if (status == SS_NO_MEMORY || status == SS_FILE_ERROR)
    {
    *buffer_handle = 0;
    return BM_NO_SHM;
    }

  pheader = _buffer[handle].buffer_header;

  shm_created = (status == SS_CREATED);

  if (shm_created)
    {
    /* setup header info if buffer was created */
    memset(pheader, 0, sizeof(BUFFER_HEADER));

    strcpy(pheader->name, buffer_name);
    pheader->size = buffer_size;
    }
  else
    {
    /* check if buffer size is identical */
    if (pheader->size != buffer_size)
      {
      buffer_size = pheader->size;

      /* re-open shared memory with proper size */

      status = ss_shm_close(buffer_name, _buffer[handle].buffer_header,
                            shm_handle, FALSE);
      if (status != BM_SUCCESS)
        return BM_MEMSIZE_MISMATCH;

      status = ss_shm_open(buffer_name, sizeof(BUFFER_HEADER) + buffer_size,
                           (void *) &(_buffer[handle].buffer_header),
                           &shm_handle);

      if (status == SS_NO_MEMORY || status == SS_FILE_ERROR)
        {
        *buffer_handle = 0;
        return BM_INVALID_NAME;
        }

      pheader = _buffer[handle].buffer_header;
      }
    }

  /* create mutex for the buffer */
  status = ss_mutex_create(buffer_name, &(_buffer[handle].mutex));
  if (status != SS_CREATED && status != SS_SUCCESS)
    {
    *buffer_handle = 0;
    return BM_NO_MUTEX;
    }

  /* first lock buffer */
  bm_lock_buffer(handle + 1);

  /*
  Now we have a BUFFER_HEADER, so let's setup a CLIENT
  structure in that buffer. The information there can also
  be seen by other processes.
  */

  for (i=0 ; i<MAX_CLIENTS ; i++)
    if (pheader->client[i].pid == 0)
      break;

  if (i == MAX_CLIENTS)
    {
    bm_unlock_buffer(handle + 1);
    *buffer_handle = 0;
    cm_msg(MERROR, "bm_open_buffer", "maximum number of clients exceeded");
    return BM_NO_SLOT;
    }

  /* store slot index in _buffer structure */
  _buffer[handle].client_index = i;

  /*
  Save the index of the last client of that buffer so that later only
  the clients 0..max_client_index-1 have to be searched through.
  */
  pheader->num_clients++;
  if (i+1 > pheader->max_client_index)
    pheader->max_client_index = i+1;

  /* setup buffer header and client structure */
  pclient = &pheader->client[i];

  memset(pclient, 0, sizeof(BUFFER_CLIENT));
  /* use client name previously set by bm_set_name */
  cm_get_client_info(pclient->name);
  if (pclient->name[0] == 0)
    strcpy(pclient->name, "unknown");
  pclient->pid = ss_getpid();
  pclient->tid = ss_gettid();
  pclient->thandle = ss_getthandle();
  
  ss_suspend_get_port(&pclient->port);
  
  pclient->read_pointer = pheader->write_pointer;
  pclient->last_activity = ss_millitime();

  cm_get_watchdog_params(NULL, &pclient->watchdog_timeout);

  bm_unlock_buffer(handle + 1);

  /* setup _buffer entry */
  _buffer[handle].buffer_data    = _buffer[handle].buffer_header + 1;
  _buffer[handle].attached       = TRUE;
  _buffer[handle].shm_handle     = shm_handle;
  _buffer[handle].callback       = FALSE;

  /* remember to which connection acutal buffer belongs */
  if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_SINGLE)
    _buffer[handle].index = rpc_get_server_acception();
  else
    _buffer[handle].index = ss_gettid();

  *buffer_handle = (handle+1);

  /* initialize buffer counters */
  bm_init_buffer_counters(handle + 1);

  /* setup dispatcher for receive events */
  ss_suspend_set_dispatch(CH_IPC, 0, cm_dispatch_ipc);

  if (shm_created) return BM_CREATED;
}
#endif /* LOCAL_ROUTINES */

  return BM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT bm_close_buffer(INT buffer_handle)
/********************************************************************\

  Routine: bm_close_buffer

  Purpose: Close a buffer

  Input:
    INT  buffer_handle      Handle to the buffer, which is used as
                            an index to the _buffer array.

  Output:
    none

  Function value:
    BM_SUCCESS              Successful completion
    BM_INVALID_HANDLE       Buffer handle is invalid
    RPC_NET_ERROR           Network error

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_BM_CLOSE_BUFFER, buffer_handle);

#ifdef LOCAL_ROUTINES
{
BUFFER_CLIENT *pclient;
BUFFER_HEADER *pheader;
INT           i, j, index, destroy_flag;

  if (buffer_handle > _buffer_entries || buffer_handle <= 0)
    {
    cm_msg(MERROR, "bm_close_buffer", "invalid buffer handle %d", buffer_handle);
    return BM_INVALID_HANDLE;
    }

  /*
  Check if buffer was opened by current thread. This is necessary
  in the server process where one thread may not close the buffer
  of other threads.
  */

  index   = _buffer[buffer_handle-1].client_index;
  pheader = _buffer[buffer_handle-1].buffer_header;

  if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_SINGLE &&
      _buffer[buffer_handle-1].index != rpc_get_server_acception())
    return BM_INVALID_HANDLE;
  
  if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_SINGLE &&
      _buffer[buffer_handle-1].index != ss_gettid())
    return BM_INVALID_HANDLE;

  if (!_buffer[buffer_handle-1].attached)
    {
    cm_msg(MERROR, "bm_close_buffer", "invalid buffer handle %d", buffer_handle);
    return BM_INVALID_HANDLE;
    }

  /* delete all requests for this buffer */
  for (i=0 ; i<_request_list_entries ; i++)
    if (_request_list[i].buffer_handle == buffer_handle)
      bm_delete_request(i);

  /* first lock buffer */
  bm_lock_buffer(buffer_handle);

  /* mark entry in _buffer as empty */
  _buffer[buffer_handle-1].attached = FALSE;

  /* clear entry from client structure in buffer header */
  memset(&(pheader->client[index]), 0, sizeof(BUFFER_CLIENT));

  /* calculate new max_client_index entry */
  for (i=MAX_CLIENTS-1 ; i>=0 ; i--)
    if (pheader->client[i].pid != 0)
      break;
  pheader->max_client_index = i+1;

  /* count new number of clients */
  for (i=MAX_CLIENTS-1,j=0 ; i>=0 ; i--)
    if (pheader->client[i].pid != 0)
      j++;
  pheader->num_clients = j;

  destroy_flag = (pheader->num_clients == 0);

  /* free cache */
  if (_buffer[buffer_handle-1].read_cache_size > 0)
    FREE(_buffer[buffer_handle-1].read_cache);
  if (_buffer[buffer_handle-1].write_cache_size > 0)
    FREE(_buffer[buffer_handle-1].write_cache);

  /* check if anyone is waiting and wake him up */
  pclient = pheader->client;

  for (i=0 ; i<pheader->max_client_index ; i++,pclient++)
    if (pclient->pid && (pclient->write_wait || pclient->read_wait))
      ss_resume(pclient->port, "B  ");

  /* unmap shared memory, delete it if we are the last */
  ss_shm_close(pheader->name, _buffer[buffer_handle-1].buffer_header,
               _buffer[buffer_handle-1].shm_handle, destroy_flag);

  /* unlock buffer */
  bm_unlock_buffer(buffer_handle);

  /* delete mutex */
  ss_mutex_delete(_buffer[buffer_handle-1].mutex, destroy_flag);

  /* update _buffer_entries */
  if (buffer_handle == _buffer_entries)
    _buffer_entries--;

  if (_buffer_entries > 0)
    _buffer = realloc(_buffer, sizeof(BUFFER) * (_buffer_entries));
  else
    {
    free(_buffer);
    _buffer = NULL;
    }
}
#endif /* LOCAL_ROUTINES */

  return BM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT bm_close_all_buffers(void)
/********************************************************************\

  Routine: bm_close_all_buffers

  Purpose: Close all open buffers

  Input:
    none

  Output:
    none

  Function value:
    BM_SUCCESS              Successful completion

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_BM_CLOSE_ALL_BUFFERS);

#ifdef LOCAL_ROUTINES
{
INT i;

  for (i=_buffer_entries ; i>0 ; i--)
    bm_close_buffer(i);
}
#endif /* LOCAL_ROUTINES */

  return BM_SUCCESS;
}

/*-- Watchdog routines ---------------------------------------------*/

#ifdef LOCAL_ROUTINES

/*------------------------------------------------------------------*/

void cm_watchdog(int dummy)
/********************************************************************\

  Routine: cm_watchdog

  Purpose: Called at periodic intervals, checks if all clients are
           alive. If one process died, its client entries are cleaned up.

  Input:                                      
    none

  Output:
    none

  Function value:
    none

\********************************************************************/
{
BUFFER_HEADER   *pheader;
BUFFER_CLIENT   *pbclient, *pbctmp;
DATABASE_HEADER *pdbheader;
DATABASE_CLIENT *pdbclient;
KEY             *pkey;
INT             actual_time, interval;
INT             client_pid;
INT             i, j, k, nc, status;
BOOL            bDeleted, time_changed, wrong_interval;
char            str[256];

  /* return immediately if watchdog has been disabled in meantime */
  if (!_call_watchdog)
    return;

  /* tell system services that we are in async mode ... */
  ss_set_async_flag(TRUE);

  /* Calculate the time since last watchdog call. Kill clients if they
     are inactive for more than the timeout they specified */
  actual_time = ss_millitime();
  if (_watchdog_last_called == 0)
    _watchdog_last_called = actual_time - WATCHDOG_INTERVAL;
  interval = actual_time - _watchdog_last_called;

  /* check if system time has been changed more than 10 min */
  time_changed = interval < 0 || interval > 600000;
  wrong_interval = interval < 0.8*WATCHDOG_INTERVAL || interval > 1.2*WATCHDOG_INTERVAL;
    
  if(time_changed)
    cm_msg(MINFO, "cm_watchdog", "System time has been changed! last:%dms  now:%dms  delta:%dms", 
                                  _watchdog_last_called, actual_time, interval);

  /* check buffers */
  for (i=0 ; i<_buffer_entries ; i++)
    if (_buffer[i].attached)
      {
      /* update the last_activity entry to show that we are alive */
      pheader = _buffer[i].buffer_header;
      pbclient = pheader->client;
      pbclient[ _buffer[i].client_index ].last_activity = actual_time;

      /* don't check other clients if interval is stange */
      if (wrong_interval)
        continue;

      /* now check other clients */
      for (j=0 ; j<pheader->max_client_index ; j++,pbclient++)
        /* If client process has no activity, clear its buffer entry. */
        if (pbclient->pid && pbclient->watchdog_timeout > 0 &&
            actual_time - pbclient->last_activity > pbclient->watchdog_timeout)
          {
          bm_lock_buffer(i+1);
          str[0] = 0;

          /* now make again the check with the buffer locked */
          if (pbclient->pid && pbclient->watchdog_timeout > 0 &&
              actual_time - pbclient->last_activity > pbclient->watchdog_timeout)
            {
            sprintf(str, "Client %s on %s removed (idle %1.1lfs,TO %1.0lfs)",
                    pbclient->name, pheader->name,
                    (actual_time - pbclient->last_activity)/1000.0,
                    pbclient->watchdog_timeout/1000.0);

            /* clear entry from client structure in buffer header */
            memset(&(pheader->client[j]), 0, sizeof(BUFFER_CLIENT));

            /* calculate new max_client_index entry */
            for (k=MAX_CLIENTS-1 ; k>=0 ; k--)
              if (pheader->client[k].pid != 0)
                break;
            pheader->max_client_index = k+1;

            /* count new number of clients */
            for (k=MAX_CLIENTS-1,nc=0 ; k>=0 ; k--)
              if (pheader->client[k].pid != 0)
                nc++;
            pheader->num_clients = nc;

            /* check if anyone is wating and wake him up */
            pbctmp = pheader->client;

            for (k=0 ; k<pheader->max_client_index ; k++,pbctmp++)
              if (pbctmp->pid && (pbctmp->write_wait || pbctmp->read_wait))
                ss_resume(pbctmp->port, "B  ");

            }

          bm_unlock_buffer(i+1);
          
          /* display info message after unlocking buffer */
          if (str[0])
            cm_msg(MINFO, "cm_watchdog", str);
          }
      }

  /* check online databases */
  for (i=0 ; i<_database_entries ; i++)
    if (_database[i].attached)
      {
      /* update the last_activity entry to show that we are alive */
      pdbheader = _database[i].database_header;
      pdbclient = pdbheader->client;
      pdbclient[ _database[i].client_index ].last_activity = actual_time;

      /* don't check other clients if interval is stange */
      if (wrong_interval)
        continue;

      /* now check other clients */
      for (j=0 ; j<pdbheader->max_client_index ; j++,pdbclient++)
        /* If client process has no activity, clear its buffer entry. */
        if (pdbclient->pid && pdbclient->watchdog_timeout > 0 &&
            actual_time - pdbclient->last_activity > pdbclient->watchdog_timeout)
          {
          client_pid = pdbclient->tid;
          bDeleted = FALSE;
          db_lock_database(i+1);
          str[0] = 0;

          /* now make again the check with the buffer locked */
          if (pdbclient->pid && pdbclient->watchdog_timeout &&
              actual_time - pdbclient->last_activity > pdbclient->watchdog_timeout)
            {
            sprintf(str, "Client %s (PID %d) on %s removed (idle %1.1lfs,TO %1.0lfs)",
                    pdbclient->name, client_pid, pdbheader->name,
                    (actual_time - pdbclient->last_activity)/1000.0,
                    pdbclient->watchdog_timeout/1000.0);

            /* decrement notify_count for open records and clear exclusive mode */
            for (k=0 ; k<pdbclient->max_index ; k++)
              if (pdbclient->open_record[k].handle)
                {
                pkey = (KEY *) ((char *) pdbheader + 
                                pdbclient->open_record[k].handle);
                if (pkey->notify_count > 0)
                  pkey->notify_count--;

                if (pdbclient->open_record[k].access_mode & MODE_WRITE)
                  db_set_mode(i+1, pdbclient->open_record[k].handle, 
                             (WORD) (pkey->access_mode & ~MODE_EXCLUSIVE), 2);
                }

            /* clear entry from client structure in buffer header */
            memset(&(pdbheader->client[j]), 0, sizeof(DATABASE_CLIENT));

            /* calculate new max_client_index entry */
            for (k=MAX_CLIENTS-1 ; k>=0 ; k--)
              if (pdbheader->client[k].pid != 0)
                break;
            pdbheader->max_client_index = k+1;

            /* count new number of clients */
            for (k=MAX_CLIENTS-1,nc=0 ; k>=0 ; k--)
              if (pdbheader->client[k].pid != 0)
                nc++;
            pdbheader->num_clients = nc;
            bDeleted = TRUE;
            }                          

          db_unlock_database(i+1);

          /* display info message after unlocking db */
          if (str[0])
            cm_msg(MINFO, "cm_watchdog", str);

          /* delete client entry after unlocking db */
          if (bDeleted)
            {
            status = cm_delete_client_info(i+1, client_pid);
            if (status != CM_SUCCESS)
              cm_msg(MERROR, "cm_watchdog", "cannot delete client info");
            }
          }
    }

  _watchdog_last_called = actual_time;

  ss_set_async_flag(FALSE);

  /* Schedule next watchdog call */
  if (_call_watchdog)
    ss_alarm(WATCHDOG_INTERVAL, cm_watchdog);
}

/*------------------------------------------------------------------*/

INT cm_enable_watchdog(BOOL flag)
/********************************************************************\

  Routine: cm_enable_watchdog

  Purpose: Temporarily disable watchdog calling. Used for tape IO
           not to interrupt lengthy operations like mount.

  Input:
    BOOL   flag    FALSE for disable, TRUE for re-enable

  Output:
    none

  Function value:
    CM_SUCCESS              Successful completion

\********************************************************************/
{
static INT timeout = DEFAULT_WATCHDOG_TIMEOUT;
static BOOL call_flag = FALSE;

  if (flag)
    {
    if (call_flag)
      cm_set_watchdog_params(TRUE, timeout);
    }
  else
    {
    call_flag = _call_watchdog;
    timeout = _watchdog_timeout;
    if (call_flag)
      cm_set_watchdog_params(FALSE, timeout);
    }

  return CM_SUCCESS;
}

#endif /* local routines */

/*------------------------------------------------------------------*/

INT cm_shutdown(char *name, BOOL bUnique)
/********************************************************************\

  Routine: cm_shutdown

  Purpose: Shutdown (exit) other MIDAS client

  Input:
    char   name             Client name or "all" for all clients 
    BOOL   bUnique          If true, look for the exact client name.
                            If false, look for namexxx where xxx is
                            a any number

  Output:
    none

  Function value:
    CM_SUCCESS              Client shutdown sucessfully
    CM_NO_CLIENT            Client not found
    DB_NO_KEY               No /System/Clients entry in ODB

\********************************************************************/
{
INT    status, return_status, i, size;
HNDLE  hDB, hKeyClient, hKey, hSubkey, hKeyTmp, hConn;
KEY    key;
char   client_name[NAME_LENGTH], remote_host[HOST_NAME_LENGTH], str[256];
INT    port;
DWORD  start_time;

  cm_get_experiment_database(&hDB, &hKeyClient);

  status = db_find_key(hDB, 0, "System/Clients", &hKey);
  if (status != DB_SUCCESS)
    return DB_NO_KEY;

  return_status = CM_NO_CLIENT;

  /* loop over all clients */
  for (i=0 ; ; )
    {
    status = db_enum_key(hDB, hKey, i, &hSubkey);
    if (status == DB_NO_MORE_SUBKEYS)
      break;

    /* don't shutdown ourselves */
    if (hSubkey == hKeyClient)
      {
      i++;
      continue;
      }

    if (status == DB_SUCCESS)
      {
      db_get_key(hDB, hSubkey, &key);

      /* contact client */
      size = sizeof(client_name);
      db_get_value(hDB, hSubkey, "Name", client_name, &size, TID_STRING);

      if (!bUnique)
	client_name[strlen(name)] = 0; /* strip number */

      /* check if individual client */
      if (!equal_ustring("all", name) && 
          !equal_ustring(client_name, name))
        {
        i++;
        continue;
        }

      size = sizeof(port);
      db_get_value(hDB, hSubkey, "Server Port", &port, &size, TID_INT);

      size = sizeof(remote_host);
      db_get_value(hDB, hSubkey, "Host", remote_host, &size, TID_STRING);

      /* client found -> connect to its server port */
      status = rpc_client_connect(remote_host, port, &hConn);
      if (status != RPC_SUCCESS)
        {
        return_status = CM_NO_CLIENT;
        sprintf(str, "cannot connect to client %s on host %s, port %d",
                client_name, remote_host, port);
        cm_msg(MERROR, "cm_shutdown", str);
        i++;
        }
      else
        {
        /* call disconnect with shutdown=TRUE */
        rpc_client_disconnect(hConn, TRUE);

        /* wait until client has shut down */
        start_time = ss_millitime();
        do
          {
          status = db_find_key(hDB, hKey, key.name, &hKeyTmp);
          } while (status == DB_SUCCESS && (ss_millitime() - start_time < 5000));

        return_status = CM_SUCCESS;
        }
      }
    }

  return return_status;
}

/*------------------------------------------------------------------*/

INT cm_exist(char *name, BOOL bUnique)
/********************************************************************\

  Routine: cm_exist

  Purpose: Check if aMIDAS client exists in current experiment

  Input:
    char   name             Client name
    BOOL   bUnique          If true, look for the exact client name.
                            If false, look for namexxx where xxx is
                            a any number

  Output:
    none

  Function value:
    CM_SUCCESS              Client found
    CM_NO_CLINET            No client with that name

\********************************************************************/
{
INT    status, i, size;
HNDLE  hDB, hKeyClient, hKey, hSubkey;
char   client_name[NAME_LENGTH];

  if (rpc_is_remote())
    return rpc_call(RPC_CM_EXIST, name, bUnique);

  cm_get_experiment_database(&hDB, &hKeyClient);

  status = db_find_key(hDB, 0, "System/Clients", &hKey);
  if (status != DB_SUCCESS)
    return DB_NO_KEY;

  /* loop over all clients */
  for (i=0 ; ; i++)
    {
    status = db_enum_key(hDB, hKey, i, &hSubkey);
    if (status == DB_NO_MORE_SUBKEYS)
      break;

    if (hSubkey == hKeyClient)
      continue;

    if (status == DB_SUCCESS)
      {
      /* get client name */
      size = sizeof(client_name);
      db_get_value(hDB, hSubkey, "Name", client_name, &size, TID_STRING);

      if (equal_ustring(client_name, name))
        return CM_SUCCESS;

      if (!bUnique)
        {
        client_name[strlen(name)] = 0; /* strip number */
        if (equal_ustring(client_name, name))
          return CM_SUCCESS;
        }
      }
    }

  return CM_NO_CLIENT;
}

/*------------------------------------------------------------------*/

INT cm_cleanup(void)
/********************************************************************\

  Routine: cm_cleanup

  Purpose: Remove all hanging clients, even if their watchdog flags
           are off.

  Input:
    none

  Output:
    none

  Function value:
    none

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_CM_CLEANUP);

#ifdef LOCAL_ROUTINES
{
BUFFER_HEADER   *pheader;
BUFFER_CLIENT   *pbclient, *pbctmp;
DATABASE_HEADER *pdbheader;
DATABASE_CLIENT *pdbclient;
KEY             *pkey;
INT             actual_time, client_pid;
INT             i, j, k, status, nc;
BOOL            bDeleted;
char            str[256];

  actual_time = ss_millitime();

  /* check buffers */
  for (i=0 ; i<_buffer_entries ; i++)
    if (_buffer[i].attached)
      {
      /* update the last_activity entry to show that we are alive */
      pheader = _buffer[i].buffer_header;
      pbclient = pheader->client;
      pbclient[ _buffer[i].client_index ].last_activity = actual_time;

      /* now check other clients */
      for (j=0 ; j<pheader->max_client_index ; j++,pbclient++)
        if (pbclient->pid)
          {
          /* If client process has no activity, clear its buffer entry. */
          if (abs(actual_time - pbclient->last_activity) > 2*WATCHDOG_INTERVAL)
            {
            bm_lock_buffer(i+1);
            str[0] = 0;

            /* now make again the check with the buffer locked */
            if (abs(actual_time - pbclient->last_activity) > 2*WATCHDOG_INTERVAL)
              {
              sprintf(str, "Client %s on %s removed (idle %1.1lfs,TO %1.0lfs)",
                      pbclient->name, pheader->name,
                      (actual_time - pbclient->last_activity)/1000.0,
                      pbclient->watchdog_timeout/1000.0);

              /* clear entry from client structure in buffer header */
              memset(&(pheader->client[j]), 0, sizeof(BUFFER_CLIENT));

              /* calculate new max_client_index entry */
              for (k=MAX_CLIENTS-1 ; k>=0 ; k--)
                if (pheader->client[k].pid != 0)
                  break;
              pheader->max_client_index = k+1;

              /* count new number of clients */
              for (k=MAX_CLIENTS-1,nc=0 ; k>=0 ; k--)
                if (pheader->client[k].pid != 0)
                  nc++;
              pheader->num_clients = nc;

              /* check if anyone is wating and wake him up */
              pbctmp = pheader->client;

              for (k=0 ; k<pheader->max_client_index ; k++,pbctmp++)
                if (pbctmp->pid && (pbctmp->write_wait || pbctmp->read_wait))
                  ss_resume(pbctmp->port, "B  ");

              }

            bm_unlock_buffer(i+1);
            
            /* display info message after unlocking buffer */
            if (str[0])
              cm_msg(MINFO, "cm_cleanup", str);
            }
          }
      }

  /* check online databases */
  for (i=0 ; i<_database_entries ; i++)
    if (_database[i].attached)
      {
      /* update the last_activity entry to show that we are alive */
      db_lock_database(i+1);
      pdbheader = _database[i].database_header;
      pdbclient = pdbheader->client;
      pdbclient[ _database[i].client_index ].last_activity = actual_time;

      /* now check other clients */
      for (j=0 ; j<pdbheader->max_client_index ; j++,pdbclient++)
        if (pdbclient->pid)
          {
          client_pid = pdbclient->tid;

          /* If client process has no activity, clear its buffer entry. */

          if (abs(actual_time - pdbclient->last_activity) > 2*WATCHDOG_INTERVAL)
            {
            bDeleted = FALSE;
            str[0] = 0;

            /* now make again the check with the buffer locked */
            if (abs(actual_time - pdbclient->last_activity) > 2*WATCHDOG_INTERVAL)
              {
              sprintf(str, "Client %s on %s removed (idle %1.1lfs,TO %1.0lfs)",
                           pdbclient->name, pdbheader->name,
                           (actual_time - pdbclient->last_activity)/1000.0,
                           pdbclient->watchdog_timeout/1000.0);

              /* decrement notify_count for open records and clear exclusive mode */
              for (k=0 ; k<pdbclient->max_index ; k++)
                if (pdbclient->open_record[k].handle)
                  {
                  pkey = (KEY *) ((char *) pdbheader + 
                                  pdbclient->open_record[k].handle);
                  if (pkey->notify_count > 0)
                    pkey->notify_count--;

                  if (pdbclient->open_record[k].access_mode & MODE_WRITE)
                    db_set_mode(i+1, pdbclient->open_record[k].handle, 
                               (WORD) (pkey->access_mode & ~MODE_EXCLUSIVE), 2);
                  }

              /* clear entry from client structure in buffer header */
              memset(&(pdbheader->client[j]), 0, sizeof(DATABASE_CLIENT));

              /* calculate new max_client_index entry */
              for (k=MAX_CLIENTS-1 ; k>=0 ; k--)
                if (pdbheader->client[k].pid != 0)
                  break;
              pdbheader->max_client_index = k+1;

              /* count new number of clients */
              for (k=MAX_CLIENTS-1,nc=0 ; k>=0 ; k--)
                if (pheader->client[k].pid != 0)
                  nc++;
              pdbheader->num_clients = nc;

              bDeleted = TRUE;
              }


            /* delete client entry after unlocking db */
            if (bDeleted)
              {
              db_unlock_database(i+1);

              /* display info message after unlocking buffer */
              cm_msg(MINFO, "cm_cleanup", str);
              
              status = cm_delete_client_info(i+1, client_pid);
              if (status != CM_SUCCESS)
                cm_msg(MERROR, "cm_cleanup", "cannot delete client info");

              /* re-lock database */
              db_lock_database(i+1);
              pdbheader = _database[i].database_header;
              pdbclient = pdbheader->client;
              }
            }
          }

      db_unlock_database(i+1);
    }
}
#endif /* LOCAL_ROUTINES */

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT bm_get_buffer_info(INT buffer_handle, BUFFER_HEADER *buffer_header)
/********************************************************************\

  Routine: bm_buffer_info

  Purpose: Copies the current buffer header referenced by buffer_handle
           into the *buffer_header structure which must be supplied
           by the calling routine.

  Input:
    INT buffer_handle       Handle of the buffer to get the header from

  Output:
    BUFFER_HEADER *buffer_header   Destination address which gets a copy
                                   of the buffer header structure.

  Function value:
    BM_SUCCESS              Successful completion
    BM_INVALID_HANDLE       Buffer handle is invalid
    RPC_NET_ERROR           Network error

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_BM_GET_BUFFER_INFO, buffer_handle,
                    buffer_header);

#ifdef LOCAL_ROUTINES

  if (buffer_handle > _buffer_entries || buffer_handle <= 0)
    {
    cm_msg(MERROR, "bm_get_buffer_info", "invalid buffer handle %d", buffer_handle);
    return BM_INVALID_HANDLE;
    }

  if (!_buffer[buffer_handle-1].attached)
    {
    cm_msg(MERROR, "bm_get_buffer_info", "invalid buffer handle %d", buffer_handle);
    return BM_INVALID_HANDLE;
    }

  bm_lock_buffer(buffer_handle);

  memcpy(buffer_header, _buffer[buffer_handle-1].buffer_header,
         sizeof(BUFFER_HEADER));

  bm_unlock_buffer(buffer_handle);

#endif /* LOCAL_ROUTINES */

  return BM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT bm_get_buffer_level(INT buffer_handle, INT *n_bytes)
/********************************************************************\

  Routine: bm_get_buffer_level

  Purpose: Return number of bytes in buffer or in cache

  Input:
    INT buffer_handle       Handle of the buffer to get the info

  Output:
    INT *n_bytes              Number of bytes in buffer

  Function value:
    BM_SUCCESS              Successful completion
    BM_INVALID_HANDLE       Buffer handle is invalid
    RPC_NET_ERROR           Network error

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_BM_GET_BUFFER_LEVEL, buffer_handle,
                    n_bytes);

#ifdef LOCAL_ROUTINES
{
BUFFER        *pbuf;
BUFFER_HEADER *pheader;
BUFFER_CLIENT *pclient;

  if (buffer_handle > _buffer_entries || buffer_handle <= 0)
    {
    cm_msg(MERROR, "bm_get_buffer_level", "invalid buffer handle %d", buffer_handle);
    return BM_INVALID_HANDLE;
    }

  pbuf = &_buffer[buffer_handle-1];
  pheader = pbuf->buffer_header;

  if (!pbuf->attached)
    {
    cm_msg(MERROR, "bm_get_buffer_level", "invalid buffer handle %d", buffer_handle);
    return BM_INVALID_HANDLE;
    }

  bm_lock_buffer(buffer_handle);

  pclient = &(pheader->client[ _buffer[buffer_handle-1].client_index]);

  *n_bytes = pheader->write_pointer - pclient->read_pointer;
  if (*n_bytes < 0)
    *n_bytes += pheader->size;

  bm_unlock_buffer(buffer_handle);

  /* add bytes in cache */
  if (pbuf->read_cache_wp > pbuf->read_cache_rp)
    *n_bytes += pbuf->read_cache_wp - pbuf->read_cache_rp;
}
#endif /* LOCAL_ROUTINES */

  return BM_SUCCESS;
}

/*------------------------------------------------------------------*/

#ifdef LOCAL_ROUTINES

INT bm_lock_buffer(INT buffer_handle)
/********************************************************************\

  Routine: bm_lock_buffer

  Purpose: Lock a buffer for exclusive access via system mutex calls.

  Input:
    INT    bufer_handle     Handle to the buffer to lock
  Output:
    none

  Function value:
    BM_SUCCESS              Successful completion
    BM_INVALID_HANDLE       Buffer handle is invalid

\********************************************************************/
{
  if (buffer_handle > _buffer_entries || buffer_handle <= 0)
    {
    cm_msg(MERROR, "bm_lock_buffer", "invalid buffer handle %d", buffer_handle);
    return BM_INVALID_HANDLE;
    }

  ss_mutex_wait_for(_buffer[buffer_handle-1].mutex, 0);
  return BM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT bm_unlock_buffer(INT buffer_handle)
/********************************************************************\

  Routine: bm_unlock_buffer

  Purpose: Unlock a buffer via system mutex calls.

  Input:
    INT    bufer_handle     Handle to the buffer to lock
  Output:
    none

  Function value:
    BM_SUCCESS              Successful completion
    BM_INVALID_HANDLE       Buffer handle is invalid

\********************************************************************/
{
  if (buffer_handle > _buffer_entries || buffer_handle <= 0)
    {
    cm_msg(MERROR, "bm_unlock_buffer", "invalid buffer handle %d", buffer_handle);
    return BM_INVALID_HANDLE;
    }

  ss_mutex_release(_buffer[buffer_handle-1].mutex);
  return BM_SUCCESS;
}

#endif /* LOCAL_ROUTINES */

/*------------------------------------------------------------------*/

INT bm_init_buffer_counters(INT buffer_handle)
/********************************************************************\

  Routine: bm_init_event_counters

  Purpose: Initialize counters for a specific buffer. This routine 
           should be called at the beginning of a run.

  Input:
    INT    buffer_handle    Handle to the buffer to be
                            initialized.
  Output:
    none

  Function value:
    BM_SUCCESS              Successful completion
    BM_INVALID_HANDLE       Buffer handle is invalid

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_BM_INIT_BUFFER_COUNTERS, buffer_handle);

#ifdef LOCAL_ROUTINES

  if (buffer_handle > _buffer_entries || buffer_handle <= 0)
    {
    cm_msg(MERROR, "bm_init_buffer_counters", "invalid buffer handle %d", buffer_handle);
    return BM_INVALID_HANDLE;
    }

  if (!_buffer[buffer_handle-1].attached)
    {
    cm_msg(MERROR, "bm_init_buffer_counters", "invalid buffer handle %d", buffer_handle);
    return BM_INVALID_HANDLE;
    }

  _buffer[buffer_handle-1].buffer_header->num_in_events = 0;
  _buffer[buffer_handle-1].buffer_header->num_out_events = 0;

#endif /* LOCAL_ROUTINES */

  return BM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT bm_set_cache_size(INT buffer_handle, INT read_size, INT write_size)
/********************************************************************\

  Routine: bm_set_cache_size

  Purpose: Turns on/off caching for reading and writing to a buffer

  Input:
    INT    buffer_handle    Handle to the buffer.
    INT    read_size        Size of read cache buffer.
    INT    write_size       Size of write cache buffer.

  Output:
    none

  Function value:
    BM_SUCCESS              Successful completion
    BM_INVALID_HANDLE       Buffer handle is invalid
    BM_NO_MEMORY            Not enough memory for read cache
    BM_INVALID_PARAM        Invalid parameter

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_BM_SET_CACHE_SIZE, buffer_handle, read_size,
                    write_size);

#ifdef LOCAL_ROUTINES
{
BUFFER        *pbuf;

  if (buffer_handle > _buffer_entries || buffer_handle <= 0)
    {
    cm_msg(MERROR, "bm_set_cache_size", "invalid buffer handle %d", buffer_handle);
    return BM_INVALID_HANDLE;
    }

  if (!_buffer[buffer_handle-1].attached)
    {
    cm_msg(MERROR, "bm_set_cache_size", "invalid buffer handle %d", buffer_handle);
    return BM_INVALID_HANDLE;
    }

  if (read_size <0 || read_size > 1E6)
    {
    cm_msg(MERROR, "bm_set_cache_size", "invalid read chache size");
    return BM_INVALID_PARAM;
    }

  if (write_size <0 || write_size > 1E6)
    {
    cm_msg(MERROR, "bm_set_cache_size", "invalid write chache size");
    return BM_INVALID_PARAM;
    }

  /* manage read cache */
  pbuf = &_buffer[buffer_handle-1];

  if (pbuf->read_cache_size > 0)
    FREE(pbuf->read_cache);

  if (read_size > 0)
    {
    pbuf->read_cache = MALLOC(read_size);
    if (pbuf->read_cache == NULL)
      {
      cm_msg(MERROR, "bm_set_cache_size", "not enough memory to allocate cache buffer");
      return BM_NO_MEMORY;
      }
    }

  pbuf->read_cache_size = read_size;
  pbuf->read_cache_rp = pbuf->read_cache_wp = 0;

  /* manage write cache */
  if (pbuf->write_cache_size > 0)
    FREE(pbuf->write_cache);

  if (write_size > 0)
    {
    pbuf->write_cache = MALLOC(write_size);
    if (pbuf->write_cache == NULL)
      {
      cm_msg(MERROR, "bm_set_cache_size", "not enough memory to allocate cache buffer");
      return BM_NO_MEMORY;
      }
    }

  pbuf->write_cache_size = write_size;
  pbuf->write_cache_rp = pbuf->write_cache_wp = 0;

}
#endif /* LOCAL_ROUTINES */

  return BM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT bm_compose_event(EVENT_HEADER *event_header,
                     short int event_id, short int trigger_mask,
                     DWORD size, DWORD serial)
/********************************************************************\

  Routine: bm_compose_event

  Purpose: Compose a valid event header from a list of arguments

  Input:
    EVENT_HEADER *event_header      Address of a buffer where to store
                                    the composed event header
    short int    event_id           Event ID
    short int    trigger_mask       Trigger mask
    DWORD        size               Size of the event data in Bytes
    DWORD        serial             Serial number of the event

  Output:
    event_header                    This structure gets filled with
                                    the above arguments plus a time
                                    stamp.

  Function value:
    BM_SUCCESS                      Successful completion

\********************************************************************/
{
  event_header->event_id      = event_id;
  event_header->trigger_mask  = trigger_mask;
  event_header->data_size     = size;
  event_header->time_stamp    = ss_time();
  event_header->serial_number = serial;

  return BM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT bm_add_event_request(INT buffer_handle, short int event_id,
                         short int trigger_mask,
                         INT sampling_type,
                         void (*func)(HNDLE,HNDLE,EVENT_HEADER*,void*),
                         INT request_id)
/********************************************************************\

  Routine:  bm_add_event_request

  Purpose:  Place a request for a specific event type in the client
            structure of the buffer refereced by buffer_handle.

  Input:
    INT          buffer_handle  Handle to the buffer where the re-
                                quest should be placed in

    short int    event_id       Event ID      \
    short int    trigger_mask   Trigger mask  / Event specification

    INT          sampling_type  One of GET_ALL, GET_SOME or GET_FARM


                 Note: to request all types of events, use
                   event_id = 0 (all others should be !=0 !)
                   trigger_mask = TRIGGER_ALL
                   sampling_typ = GET_ALL


    void         *func          Callback function
    INT          request_id     Request id (unique number assigned
                                by bm_request_event)

  Output:
    none

  Function value:
    BM_SUCCESS              Successful completion
    BM_NO_MEMORY            Too much request. MAX_EVENT_REQUESTS in
                            MIDAS.H should be increased.
    BM_INVALID_HANDLE       Buffer handle is invalid
    RPC_NET_ERROR           Network error

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_BM_ADD_EVENT_REQUEST, buffer_handle, event_id,
                    trigger_mask, sampling_type, (INT) func, request_id);

#ifdef LOCAL_ROUTINES
{
INT           i;
BUFFER_CLIENT *pclient;

  if (buffer_handle > _buffer_entries || buffer_handle <= 0)
    {
    cm_msg(MERROR, "bm_add_event_request", "invalid buffer handle %d", buffer_handle);
    return BM_INVALID_HANDLE;
    }

  if (!_buffer[buffer_handle-1].attached)
    {
    cm_msg(MERROR, "bm_add_event_request", "invalid buffer handle %d", buffer_handle);
    return BM_INVALID_HANDLE;
    }

  /* avoid callback/non callback requests */
  if (func == NULL && _buffer[buffer_handle-1].callback)
    {
    cm_msg(MERROR, "bm_add_event_request", "mixing callback/non callback requests not possible");
    return BM_INVALID_MIXING;
    }

  /* get a pointer to the proper client structure */
  pclient = &(_buffer[buffer_handle-1].buffer_header->
              client[ _buffer[buffer_handle-1].client_index]);

  /* lock buffer */
  bm_lock_buffer(buffer_handle);

  /* look for a empty request entry */
  for (i=0 ; i<MAX_EVENT_REQUESTS ; i++)
    if (!pclient->event_request[i].valid)
      break;

  if (i == MAX_EVENT_REQUESTS)
    {
    bm_unlock_buffer(buffer_handle);
    return BM_NO_MEMORY;
    }

  /* setup event_request structure */
  pclient->event_request[i].id            = request_id;
  pclient->event_request[i].valid         = TRUE;
  pclient->event_request[i].event_id      = event_id;
  pclient->event_request[i].trigger_mask  = trigger_mask;
  pclient->event_request[i].sampling_type = sampling_type;
  pclient->event_request[i].dispatch      = func;

  pclient->all_flag = pclient->all_flag || (sampling_type & GET_ALL);

  /* set callback flag in buffer structure */
  if (func != NULL)
    _buffer[buffer_handle-1].callback = TRUE;

  /*
  Save the index of the last request in the list so that later only the
  requests 0..max_request_index-1 have to be searched through.
  */

  if (i+1 > pclient->max_request_index)
    pclient->max_request_index = i+1;

  bm_unlock_buffer(buffer_handle);
}
#endif /* LOCAL_ROUTINES */

  return BM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT bm_request_event(HNDLE buffer_handle, short int event_id,
                     short int trigger_mask,
                     INT sampling_type, HNDLE *request_id,
                     void (*func)(HNDLE,HNDLE,EVENT_HEADER*,void*))
/********************************************************************\

  Routine:  bm_request_event

  Purpose:  Place a request for a specific event type in the client
            structure of the buffer refereced by buffer_handle.

  Input:
    HNDLE        buffer_handle  Handle to the buffer where the re-
                                quest should be placed in

    short int    event_id       Event ID      \
    short int    trigger_mask   Trigger mask   } Event specification

    INT          sampling_type  One of GET_ALL, GET_SOME or GET_FARM


                 Note: to request all types of events, use
                   event_id = 0 (all others should be !=0 !)
                   trigger_mask = TRIGGER_ALL
                   sampling_typ = GET_ALL

    INT          (*func)()      Callback function

  Output:
    HNDLE        request_id     Unique ID for this request

  Function value:
    BM_SUCCESS              Successful completion
    BM_NO_MEMORY            Too much request. MAX_EVENT_REQUESTS in
                            MIDAS.H should be increased.
    BM_INVALID_HANDLE       Buffer handle is invalid
    RPC_NET_ERROR           Network error

\********************************************************************/
{
INT  index, status;

  /* allocate new space for the local request list */
  if (_request_list_entries == 0)
    {
    _request_list = malloc(sizeof(REQUEST_LIST));
    memset(_request_list, 0, sizeof(REQUEST_LIST));
    if (_request_list == NULL)
      {
      cm_msg(MERROR, "bm_request_event", "not enough memory to allocate request list buffer");
      return BM_NO_MEMORY;
      }

    _request_list_entries = 1;
    index = 0;
    }
  else
    {
    /* check for a deleted entry */
    for (index=0 ; index<_request_list_entries ; index++)
      if (!_request_list[index].buffer_handle)
        break;

    /* if not found, create new one */
    if (index == _request_list_entries)
      {
      _request_list = realloc(_request_list, 
                              sizeof(REQUEST_LIST)*(_request_list_entries+1));
      if (_request_list == NULL)
        {
        cm_msg(MERROR, "bm_request_event", "not enough memory to allocate request list buffer");
        return BM_NO_MEMORY;
        }

      memset(&_request_list[_request_list_entries], 0, sizeof(REQUEST_LIST));

      _request_list_entries++;
      }
    }

  /* initialize request list */
  _request_list[index].buffer_handle = buffer_handle;
  _request_list[index].event_id      = event_id;
  _request_list[index].trigger_mask  = trigger_mask;
  _request_list[index].dispatcher    = func;

  *request_id = index;

  /* add request in buffer structure */
  status = bm_add_event_request(buffer_handle, event_id, trigger_mask, 
                                sampling_type, func, index);
  if (status != BM_SUCCESS)
    return status;

  return BM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT bm_remove_event_request(INT buffer_handle, INT request_id)
/********************************************************************\

  Routine:  bm_remove_event_request

  Purpose:  Delete a previously placed request for a specific event
            type in the client structure of the buffer refereced by
            buffer_handle.

  Input:
    INT          buffer_handle  Handle to the buffer where the re-
                                quest should be placed in

    INT          request_id     Request id returned by bm_request_event

  Output:
    none

  Function value:
    BM_SUCCESS              Successful completion
    BM_INVALID_HANDLE       Buffer handle is invalid
    BM_NOT_FOUND            Specified request was not found
    RPC_NET_ERROR           Network error

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_BM_REMOVE_EVENT_REQUEST, buffer_handle, request_id);

#ifdef LOCAL_ROUTINES
{
INT           i, deleted;
BUFFER_CLIENT *pclient;

  if (buffer_handle > _buffer_entries || buffer_handle <= 0)
    {
    cm_msg(MERROR, "bm_remove_event_request", "invalid buffer handle %d", buffer_handle);
    return BM_INVALID_HANDLE;
    }

  if (!_buffer[buffer_handle-1].attached)
    {
    cm_msg(MERROR, "bm_remove_event_request", "invalid buffer handle %d", buffer_handle);
    return BM_INVALID_HANDLE;
    }

  /* get a pointer to the proper client structure */
  pclient = &(_buffer[buffer_handle-1].buffer_header->
              client[ _buffer[buffer_handle-1].client_index]);

  /* lock buffer */
  bm_lock_buffer(buffer_handle);

  /* check all requests and set to zero if matching */
  for (i=0,deleted=0 ; i<pclient->max_request_index ; i++)
    if (pclient->event_request[i].valid &&
        pclient->event_request[i].id == request_id)
      {
      memset(&pclient->event_request[i], 0, sizeof(EVENT_REQUEST));
      deleted++;
      }

  /* calculate new max_request_index entry */
  for (i=MAX_EVENT_REQUESTS-1 ; i>=0 ; i--)
    if (pclient->event_request[i].valid)
      break;

  pclient->max_request_index = i+1;

  /* caluclate new all_flag */
  pclient->all_flag = FALSE;

  for (i=0 ; i<pclient->max_request_index ; i++)
    if (pclient->event_request[i].valid &&
        (pclient->event_request[i].sampling_type & GET_ALL))
      {
      pclient->all_flag = TRUE;
      break;
      }

  bm_unlock_buffer(buffer_handle);

  if (!deleted)
    return BM_NOT_FOUND;
}
#endif /* LOCAL_ROUTINES */

  return BM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT bm_delete_request(INT request_id)
/********************************************************************\

  Routine:  bm_delete_request

  Purpose:  Delete a previously placed request via bm_request_event

  Input:
    INT          request_id     Request id receive by bm_request_event

  Output:
    none

  Function value:
    BM_SUCCESS              Successful completion
    BM_INVALID_HANDLE       Buffer handle is invalid
    RPC_NET_ERROR           Network error

\********************************************************************/
{
  if (request_id < 0 || request_id >=_request_list_entries)
    return BM_INVALID_HANDLE;

  /* remove request entry from buffer */
  bm_remove_event_request(_request_list[request_id].buffer_handle,
                          request_id);

  memset(&_request_list[request_id], 0, sizeof(REQUEST_LIST));

  return BM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT bm_send_event(INT buffer_handle, void *source, INT buf_size,
                  INT async_flag)
/********************************************************************\

  Routine: bm_send_event

  Purpose: Send an event into a specific buffer

  Input:
    INT  buffer_handle      Handle of the buffer to send the event to.
                            Must be obtained via bm_open_buffer.
    void *source            Address of the event to send. It must have
                            a proper event header.
    INT  buf_size           Size of event in bytes with header.
    INT  async_flag         SYNC / ASYNC flag. In ASYNC mode, the
                            function returns immediately if there is
                            not enough space in the buffer to place
                            the event there. In SYNC mode, it waits until
                            there is space available (blocking).

  Output:
    none

  Function value:
    BM_SUCCESS              Successful completion
    BM_INVALID_HANDLE       Buffer handle is invalid
    BM_INVALID_PARAM        Event size parameter mismatch
    BM_ASYNC_RETURN         Function called in ASYNC mode and not enough
                            space in buffer.
    BM_NO_MEMORY            Event is too large for network buffer or event 
                            buffer. One has to increase MAX_EVENT_SIZE or
                            EVENT_BUFFER_SIZE and recompile.
    RPC_NET_ERROR           Network error
    

\********************************************************************/
{
EVENT_HEADER    *pevent;

  /* check if event size defined in header matches buf_size */
  if (ALIGN(buf_size) != (INT) ALIGN(((EVENT_HEADER *) source)->data_size + 
                           sizeof(EVENT_HEADER)))
    {
    cm_msg(MERROR, "bm_send_event", "event size (%d) mismatch in header (%d)",
             ALIGN(buf_size), (INT) ALIGN(((EVENT_HEADER *) source)->data_size + 
             sizeof(EVENT_HEADER)));
    return BM_INVALID_PARAM;
    }

  /* check for maximal event size */
  if (((EVENT_HEADER *) source)->data_size > MAX_EVENT_SIZE)
    {
    cm_msg(MERROR, "bm_send_event", "event size (%d) larger than maximum event size (%d)", 
      ((EVENT_HEADER *) source)->data_size, MAX_EVENT_SIZE);
    return BM_NO_MEMORY;
    }

  if (rpc_is_remote())
    return rpc_call(RPC_BM_SEND_EVENT, buffer_handle,
                    source, buf_size, async_flag);

#ifdef LOCAL_ROUTINES
{
BUFFER          *pbuf;
BUFFER_HEADER   *pheader;
BUFFER_CLIENT   *pclient,*pc;
EVENT_REQUEST   *prequest;
EVENT_HEADER    *pevent_test;
INT             i, j, min_wp, size, total_size, status;
INT             increment;
INT             my_client_index;
INT             old_write_pointer;
INT             old_read_pointer, new_read_pointer;
INT             num_requests_client;
char            *pdata;
BOOL            blocking;
INT             n_blocking;
INT             request_id;
char            str[80];

  pbuf = &_buffer[buffer_handle-1];

  if (buffer_handle > _buffer_entries || buffer_handle <= 0)
    {
    cm_msg(MERROR, "bm_send_event", "invalid buffer handle %d", buffer_handle);
    return BM_INVALID_HANDLE;
    }

  if (!pbuf->attached)
    {
    cm_msg(MERROR, "bm_send_event", "invalid buffer handle %d", buffer_handle);
    return BM_INVALID_HANDLE;
    }

  pevent      = (EVENT_HEADER *) source;
  total_size  = buf_size;

  /* round up total_size to next DWORD boundary */
  total_size = ALIGN(total_size);

  /* look if there is space in the cache */
  if (pbuf->write_cache_size)
    {
    status = BM_SUCCESS;

    if (pbuf->write_cache_size - pbuf->write_cache_wp < total_size)
      status = bm_flush_cache(buffer_handle, async_flag);

    if (status != BM_SUCCESS)
      return status;

    if (total_size < pbuf->write_cache_size)
      {
      memcpy(pbuf->write_cache + pbuf->write_cache_wp, source, total_size);

      pbuf->write_cache_wp += total_size;
      return BM_SUCCESS;
      }
    }

  /* calculate some shorthands */
  pheader           = _buffer[buffer_handle-1].buffer_header;
  pdata             = (char *) (pheader + 1);
  my_client_index   = _buffer[buffer_handle-1].client_index;
  pclient           = pheader->client;

  /* check if buffer is large enough */
  if (total_size >= pheader->size)
    {
    cm_msg(MERROR, "bm_send_event", "total event size (%d) larger than buffer size (%d)", 
      total_size, pheader->size);
    return BM_NO_MEMORY;
    }

  /* lock the buffer */
  bm_lock_buffer(buffer_handle);

  /* check if enough space in buffer left */
  do
    {
    size = pheader->read_pointer - pheader->write_pointer;
    if (size <= 0)
      size += pheader->size;

    if (size <= total_size) /* note the '<=' to avoid 100% filling */
      {
      /* if not enough space, find out who's blocking */
      n_blocking = 0;

      for (i=0,pc=pclient ; i<pheader->max_client_index ; i++,pc++)
        if (pc->pid)
          {
          if (pc->read_pointer == pheader->read_pointer)
            {
            /*
            First assume that the client with the "minimum" read pointer
            is not really blocking due to a GET_ALL request.
            */
            blocking = FALSE;
            request_id = -1;

            /* check if this request blocks */

            prequest = pc->event_request;
            pevent_test = (EVENT_HEADER *) (pdata + pc->read_pointer);

            for (j=0 ; j<pc->max_request_index ; j++,prequest++)
              if (prequest->valid &&
                  bm_match_event(prequest->event_id, prequest->trigger_mask, pevent_test))
                {
                request_id = prequest->id;
                if (prequest->sampling_type & GET_ALL)
                  {
                  blocking = TRUE;
                  break;
                  }
                }

            if (!blocking)
              {
              /*
              The blocking guy has no GET_ALL request for this event
              -> shift its read pointer.
              */

              old_read_pointer = pc->read_pointer;

              increment = sizeof(EVENT_HEADER) +
                ((EVENT_HEADER *) (pdata + pc->read_pointer))->data_size;

              /* correct increment for DWORD boundary */
              increment = ALIGN(increment);

              new_read_pointer = (pc->read_pointer + increment) %
                                    pheader->size;

              if (new_read_pointer > pheader->size - (int) sizeof(EVENT_HEADER))
                new_read_pointer = 0;

              pc->read_pointer = new_read_pointer;
              }
            else
              {
              n_blocking++;
              }

            /* wake that client if it has a request */
            if (pc->read_wait && request_id != -1)
              {
#ifdef DEBUG_MSG
	            cm_msg(MDEBUG, "Send wake: rp=%d, wp=%d",
                      pheader->read_pointer, pheader->write_pointer);
#endif
              sprintf(str, "B %s %d", pheader->name, request_id);
              ss_resume(pc->port, str);
              }


            } /* read_pointer blocks */
          } /* client loop */

      if (n_blocking > 0)
        {
        /* at least one client is blocking */

        bm_unlock_buffer(buffer_handle);

        /* return now in ASYNC mode */
        if (async_flag)
          return BM_ASYNC_RETURN;

#ifdef DEBUG_MSG
	cm_msg(MDEBUG, "Send sleep: rp=%d, wp=%d, level=%1.1lf",
                pheader->read_pointer,
                pheader->write_pointer,
                100-100.0*size/pheader->size);
#endif

        /* has the read pointer moved in between ? */
        size = pheader->read_pointer - pheader->write_pointer;
        if (size <= 0)
          size += pheader->size;

        /* suspend process */
        if (size <= total_size)
          {
          /* signal other clients wait mode */
          pclient[my_client_index].write_wait = total_size;

          status = ss_suspend(1000, MSG_BM);

          pclient[my_client_index].write_wait = 0;

          /* return if TCP connection broken */
          if (status == SS_ABORT)
            return SS_ABORT;
          }

#ifdef DEBUG_MSG
	      cm_msg(MDEBUG, "Send woke up: rp=%d, wp=%d, level=%1.1lf",
                pheader->read_pointer,
                pheader->write_pointer,
                100-100.0*size/pheader->size);
#endif

        bm_lock_buffer(buffer_handle);

        /* has the write pointer moved in between ? */
        size = pheader->read_pointer - pheader->write_pointer;
        if (size <= 0)
          size += pheader->size;
        }
      else
        {
        /*
        calculate new global read pointer as "minimum" of
        client read pointers
        */
        min_wp = pheader->write_pointer;

        for (i=0,pc=pclient ; i<pheader->max_client_index ; i++,pc++)
          if (pc->pid)
            {
            if (pc->read_pointer < min_wp)
              min_wp = pc->read_pointer;

            if (pc->read_pointer > pheader->write_pointer &&
                pc->read_pointer - pheader->size < min_wp)
              min_wp = pc->read_pointer - pheader->size;
            }

        if (min_wp<0)
          min_wp += pheader->size;

        pheader->read_pointer = min_wp;
        }

      } /* if (size <= total_size) */

    } while (size <= total_size);

  /* we have space, so let's copy the event */
  old_write_pointer = pheader->write_pointer;

  if (pheader->write_pointer + total_size <= pheader->size)
    {
    memcpy(pdata + pheader->write_pointer, pevent, total_size);
    pheader->write_pointer = (pheader->write_pointer + total_size) %
                              pheader->size;
    if (pheader->write_pointer > pheader->size - (int) sizeof(EVENT_HEADER))
      pheader->write_pointer = 0;
    }
  else
    {
    /* split event */
    size = pheader->size - pheader->write_pointer;

    memcpy(pdata + pheader->write_pointer, pevent, size);
    memcpy(pdata, (char *) pevent + size, total_size - size);

    pheader->write_pointer = total_size - size;
    }

  /* check which clients have a request for this event */
  for (i=0 ; i<pheader->max_client_index ; i++)
    if (pclient[i].pid)
      {
      prequest = pclient[i].event_request;
      num_requests_client = 0;
      request_id = -1;

      for (j=0 ; j<pclient[i].max_request_index ; j++,prequest++)
        if (prequest->valid &&
            bm_match_event(prequest->event_id, prequest->trigger_mask, pevent))
          {
          if (prequest->sampling_type & GET_ALL)
            pclient[i].num_waiting_events++;

          num_requests_client++;
          request_id = prequest->id;
          }

      /* if that client has a request and is suspended, wake it up */
      if (num_requests_client && pclient[i].read_wait)
        {
#ifdef DEBUG_MSG
	cm_msg(MDEBUG, "Send wake: rp=%d, wp=%d", pheader->read_pointer,
		pheader->write_pointer);
#endif
        sprintf(str, "B %s %d", pheader->name, request_id);
        ss_resume(pclient[i].port, str);
        }

      /* if that client has no request, shift its read pointer */
      if (num_requests_client == 0 &&
          pclient[i].read_pointer == old_write_pointer)
        pclient[i].read_pointer = pheader->write_pointer;
      }

  /* shift read pointer of own client */
/* 16.4.99 SR, outcommented to receive own messages

  if (pclient[my_client_index].read_pointer == old_write_pointer)
    pclient[my_client_index].read_pointer = pheader->write_pointer;
*/

  /* calculate global read pointer as "minimum" of client read pointers */
  min_wp = pheader->write_pointer;

  for (i=0,pc=pclient ; i<pheader->max_client_index ; i++,pc++)
    if (pc->pid)
      {
      if (pc->read_pointer < min_wp)
        min_wp = pc->read_pointer;

      if (pc->read_pointer > pheader->write_pointer &&
          pc->read_pointer - pheader->size < min_wp)
        min_wp = pc->read_pointer - pheader->size;
      }

  if (min_wp<0)
    min_wp += pheader->size;

#ifdef DEBUG_MSG
  if (min_wp == pheader->read_pointer)
    cm_msg(MDEBUG, "bm_send_event -> wp=%d", pheader->write_pointer);
  else
    cm_msg(MDEBUG, "bm_send_event -> wp=%d, rp %d -> %d, size=%d",
            pheader->write_pointer, pheader->read_pointer, min_wp, size);
#endif

  pheader->read_pointer = min_wp;

  /* update statistics */
  pheader->num_in_events++;

  /* unlock the buffer */
  bm_unlock_buffer(buffer_handle);
}
#endif /* LOCAL_ROUTINES */

  return BM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT bm_flush_cache(INT buffer_handle, INT async_flag)
/********************************************************************\

  Routine: bm_flush_cache

  Purpose: Empty write cache for a specific buffer.

  Input:
    INT  buffer_handle      Handle of the buffer to send events to.
                            Must be obtained via bm_open_buffer.
    INT  async_flag         SYNC / ASYNC flag. In ASYNC mode, the
                            function returns immediately if there is
                            not enough space in the buffer to place
                            the event there. In SYNC mode, it waits until
                            there is space available (blocking).


  Output:
    none

  Function value:
    BM_SUCCESS              Successful completion
    BM_INVALID_HANDLE       Buffer handle is invalid
    BM_NO_MEMORY            Event is too large for network buffer or event 
                            buffer. One has to increase MAX_EVENT_SIZE or
                            EVENT_BUFFER_SIZE and recompile.
    RPC_NET_ERROR           Network error.

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_BM_FLUSH_CACHE, buffer_handle, async_flag);

#ifdef LOCAL_ROUTINES
{
BUFFER          *pbuf;
BUFFER_HEADER   *pheader;
BUFFER_CLIENT   *pclient,*pc;
EVENT_REQUEST   *prequest;
EVENT_HEADER    *pevent, *pevent_test;
INT             i, j, min_wp, size, total_size, status;
INT             increment;
INT             my_client_index;
INT             old_write_pointer;
INT             old_read_pointer, new_read_pointer;
char            *pdata;
BOOL            blocking;
INT             n_blocking;
INT             request_id;
char            str[80];

  pbuf = &_buffer[buffer_handle-1];

  if (buffer_handle > _buffer_entries || buffer_handle <= 0)
    {
    cm_msg(MERROR, "bm_flush_cache", "invalid buffer handle %d", buffer_handle);
    return BM_INVALID_HANDLE;
    }

  if (!pbuf->attached)
    {
    cm_msg(MERROR, "bm_flush_cache", "invalid buffer handle %d", buffer_handle);
    return BM_INVALID_HANDLE;
    }

  if (pbuf->write_cache_size == 0)
    return BM_SUCCESS;

  /* check if anything needs to be flushed */
  if (pbuf->write_cache_rp == pbuf->write_cache_wp)
    return BM_SUCCESS;

  /* calculate some shorthands */
  pheader           = _buffer[buffer_handle-1].buffer_header;
  pdata             = (char *) (pheader + 1);
  my_client_index   = _buffer[buffer_handle-1].client_index;
  pclient           = pheader->client;
  pevent            = (EVENT_HEADER *) (pbuf->write_cache + pbuf->write_cache_rp);

  /* lock the buffer */
  bm_lock_buffer(buffer_handle);

#ifdef DEBUG_MSG
  cm_msg(MDEBUG, "bm_flush_cache initial: rp=%d, wp=%d", pheader->read_pointer,
          pheader->write_pointer);
#endif

  /* check if enough space in buffer left */
  do
    {
    size = pheader->read_pointer - pheader->write_pointer;
    if (size <= 0)
      size += pheader->size;

    if (size <= pbuf->write_cache_wp)
      {
      /* if not enough space, find out who's blocking */
      n_blocking = 0;

      for (i=0,pc=pclient ; i<pheader->max_client_index ; i++,pc++)
        if (pc->pid)
          {
          if (pc->read_pointer == pheader->read_pointer)
            {
            /*
            First assume that the client with the "minimum" read pointer
            is not really blocking due to a GET_ALL request.
            */
            blocking = FALSE;
            request_id = -1;

            /* check if this request blocks. */
            prequest = pc->event_request;
            pevent_test = (EVENT_HEADER *) (pdata + pc->read_pointer);

            for (j=0 ; j<pc->max_request_index ; j++,prequest++)
              if (prequest->valid &&
                  bm_match_event(prequest->event_id, prequest->trigger_mask, pevent_test))
                {
                request_id = prequest->id;
                if (prequest->sampling_type & GET_ALL)
                  {
                  blocking = TRUE;
                  break;
                  }
                }

            if (!blocking)
              {
              /*
              The blocking guy has no GET_ALL request for this event
              -> shift its read pointer.
              */

              old_read_pointer = pc->read_pointer;

              increment = sizeof(EVENT_HEADER) +
                ((EVENT_HEADER *) (pdata + pc->read_pointer))->data_size;

              /* correct increment for DWORD boundary */
              increment = ALIGN(increment);

              new_read_pointer = (pc->read_pointer + increment) %
                                    pheader->size;

              if (new_read_pointer > pheader->size - (int) sizeof(EVENT_HEADER))
                new_read_pointer = 0;

#ifdef DEBUG_MSG
              cm_msg(MDEBUG, "bm_flush_cache: shift client %d rp=%d -> =%d", i, pc->read_pointer,
                      new_read_pointer);
#endif

              pc->read_pointer = new_read_pointer;
              }
            else
              {
              n_blocking++;
              }

            /* wake that client if it has a request */
            if (pc->read_wait && request_id != -1)
              {
#ifdef DEBUG_MSG
              cm_msg(MDEBUG, "Send wake: rp=%d, wp=%d",
                      pheader->read_pointer, pheader->write_pointer);
#endif
              sprintf(str, "B %s %d", pheader->name, request_id);
              ss_resume(pc->port, str);
              }


            } /* read_pointer blocks */
          } /* client loop */

      if (n_blocking > 0)
        {
        /* at least one client is blocking */

        bm_unlock_buffer(buffer_handle);

        /* return now in ASYNC mode */
        if (async_flag)
          return BM_ASYNC_RETURN;

#ifdef DEBUG_MSG
        cm_msg(MDEBUG, "Send sleep: rp=%d, wp=%d, level=%1.1lf",
                pheader->read_pointer,
                pheader->write_pointer,
                100-100.0*size/pheader->size);
#endif

        /* has the read pointer moved in between ? */
        size = pheader->read_pointer - pheader->write_pointer;
        if (size <= 0)
          size += pheader->size;

        /* suspend process */
        if (size <= pbuf->write_cache_wp)
          {
          /* signal other clients wait mode */
          pclient[my_client_index].write_wait = pbuf->write_cache_wp;

          status = ss_suspend(1000, MSG_BM);

          pclient[my_client_index].write_wait = 0;

          /* return if TCP connection broken */
          if (status == SS_ABORT)
            return SS_ABORT;
          }

#ifdef DEBUG_MSG
        cm_msg(MDEBUG, "Send woke up: rp=%d, wp=%d, level=%1.1lf",
                pheader->read_pointer,
                pheader->write_pointer,
                100-100.0*size/pheader->size);
#endif

        bm_lock_buffer(buffer_handle);

        /* has the write pointer moved in between ? */
        size = pheader->read_pointer - pheader->write_pointer;
        if (size <= 0)
          size += pheader->size;
        }
      else
        {
        /*
        calculate new global read pointer as "minimum" of
        client read pointers
        */
        min_wp = pheader->write_pointer;

        for (i=0,pc=pclient ; i<pheader->max_client_index ; i++,pc++)
          if (pc->pid)
            {
            if (pc->read_pointer < min_wp)
              min_wp = pc->read_pointer;

            if (pc->read_pointer > pheader->write_pointer &&
                pc->read_pointer - pheader->size < min_wp)
              min_wp = pc->read_pointer - pheader->size;
            }

        if (min_wp<0)
          min_wp += pheader->size;

        pheader->read_pointer = min_wp;
        }

      } /* if (size <= total_size) */

    } while (size <= pbuf->write_cache_wp);


  /* we have space, so let's copy the event */
  old_write_pointer = pheader->write_pointer;

#ifdef DEBUG_MSG
  cm_msg(MDEBUG, "bm_flush_cache: found space rp=%d, wp=%d", pheader->read_pointer,
          pheader->write_pointer);
#endif

  while (pbuf->write_cache_rp < pbuf->write_cache_wp)
    {
    /* loop over all events in cache */

    pevent     = (EVENT_HEADER *) (pbuf->write_cache + pbuf->write_cache_rp);
    total_size = pevent->data_size + sizeof(EVENT_HEADER);

    /* correct size for DWORD boundary */
    total_size = ALIGN(total_size);

    if (pheader->write_pointer + total_size <= pheader->size)
      {
      memcpy(pdata + pheader->write_pointer, pevent, total_size);
      pheader->write_pointer = (pheader->write_pointer + total_size) %
                                pheader->size;
      if (pheader->write_pointer > pheader->size - (int) sizeof(EVENT_HEADER))
        pheader->write_pointer = 0;
      }
    else
      {
      /* split event */
      size = pheader->size - pheader->write_pointer;

      memcpy(pdata + pheader->write_pointer, pevent, size);
      memcpy(pdata, (char *) pevent + size, total_size - size);

      pheader->write_pointer = total_size - size;
      }

    pbuf->write_cache_rp += total_size;
    } 

  pbuf->write_cache_rp = pbuf->write_cache_wp = 0;

  /* check which clients are waiting */
  for (i=0 ; i<pheader->max_client_index ; i++)
    if (pclient[i].pid && pclient[i].read_wait)
      {
#ifdef DEBUG_MSG
      cm_msg(MDEBUG, "Send wake: rp=%d, wp=%d", pheader->read_pointer,
              pheader->write_pointer);
#endif
      sprintf(str, "B %s %d", pheader->name, -1);
      ss_resume(pclient[i].port, str);
      }

  /* shift read pointer of own client */
/* 16.4.99 SR, outcommented to receive own messages

  if (pclient[my_client_index].read_pointer == old_write_pointer)
    pclient[my_client_index].read_pointer = pheader->write_pointer;
*/

  /* calculate global read pointer as "minimum" of client read pointers */
  min_wp = pheader->write_pointer;

  for (i=0,pc=pclient ; i<pheader->max_client_index ; i++,pc++)
    if (pc->pid)
      {
#ifdef DEBUG_MSG
      cm_msg(MDEBUG, "bm_flush_cache: client %d rp=%d", i, pc->read_pointer);
#endif
      if (pc->read_pointer < min_wp)
        min_wp = pc->read_pointer;

      if (pc->read_pointer > pheader->write_pointer &&
          pc->read_pointer - pheader->size < min_wp)
        min_wp = pc->read_pointer - pheader->size;
      }

  if (min_wp<0)
    min_wp += pheader->size;

#ifdef DEBUG_MSG
  if (min_wp == pheader->read_pointer)
    cm_msg(MDEBUG, "bm_flush_cache -> wp=%d", pheader->write_pointer);
  else
    cm_msg(MDEBUG, "bm_flush_cache -> wp=%d, rp %d -> %d, size=%d",
            pheader->write_pointer, pheader->read_pointer, min_wp, size);
#endif

  pheader->read_pointer = min_wp;

  /* update statistics */
  pheader->num_in_events++;

  /* unlock the buffer */
  bm_unlock_buffer(buffer_handle);
}
#endif /* LOCAL_ROUTINES */

  return BM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT bm_receive_event(INT buffer_handle, void *destination,
                     INT *buf_size, INT async_flag)
/********************************************************************\

  Routine: bm_receive_event

  Purpose: Wait for any previously requested event and copy it to
           the destination event_header.


  Input:
    INT  buffer_handle      Handle of the buffer. Must be obtained
                            via bm_open_buffer.
    void *destination       Address where the received event is
                            copied to. This space must supply enough
                            space to hold the received event.
    INT  *buf_size          Size in bytes of the destination buffer. If
                            a received event is longer, it is trun-
                            cated and the error code BM_TRUNCATED
                            is returned.
    INT  async_flag         SYNC / ASYNC flag. In ASYNC mode, the
                            function returns immediately if the is no
                            new event. In SYNC mode, it waits until
                            there is an event available (blocking).

  Output:
    INT  *buf_size          Size in bytes returned in the destination buffer.

  Function value:
    BM_SUCCESS              Successful completion
    BM_INVALID_HANDLE       Buffer handle is invalid
    BM_TRUNCATED            The event is larger than the destination
                            buffer and was therefore truncated.
    BM_ASYNC_RETURN         Function called in ASYNC mode and no event
                            available.
    RPC_NET_ERROR           Network error.

\********************************************************************/
{
  if (rpc_is_remote())
    {
    if (*buf_size > NET_BUFFER_SIZE)
      {
      cm_msg(MERROR, "bm_receive_event", "max. event size larger than NET_BUFFER_SIZE");
      return RPC_NET_ERROR;
      }

    return rpc_call(RPC_BM_RECEIVE_EVENT, buffer_handle,
                    destination, buf_size, async_flag);
    }

#ifdef LOCAL_ROUTINES
{
BUFFER          *pbuf;
BUFFER_HEADER   *pheader;
BUFFER_CLIENT   *pclient,*pc, *pctmp;
EVENT_REQUEST   *prequest;
EVENT_HEADER    *pevent;
char            *pdata;
INT             convert_flags;
INT             i, min_wp, size, max_size, total_size, status;
INT             my_client_index;
BOOL            found;
INT             old_read_pointer, new_read_pointer;

  pbuf = &_buffer[buffer_handle-1];

  if (buffer_handle > _buffer_entries || buffer_handle <= 0)
    {
    cm_msg(MERROR, "bm_receive_event", "invalid buffer handle %d", buffer_handle);
    return BM_INVALID_HANDLE;
    }

  if (!pbuf->attached)
    {
    cm_msg(MERROR, "bm_receive_event", "invalid buffer handle %d", buffer_handle);
    return BM_INVALID_HANDLE;
    }

  max_size = *buf_size;
  *buf_size = 0;

  if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_REMOTE)
    convert_flags = rpc_get_server_option(RPC_CONVERT_FLAGS);
  else
    convert_flags = 0;

CACHE_READ:

  /* look if there is anything in the cache */
  if (pbuf->read_cache_wp > pbuf->read_cache_rp)
    {
    pevent = (EVENT_HEADER *) (pbuf->read_cache + pbuf->read_cache_rp);
    size   = pevent->data_size + sizeof(EVENT_HEADER);

    if (size > max_size)
      {
      memcpy(destination, pbuf->read_cache + pbuf->read_cache_rp, max_size);
      cm_msg(MERROR, "bm_receive_event", "event size larger than buffer size");
      *buf_size = max_size;
      status = BM_TRUNCATED;
      }
    else
      {
      memcpy(destination, pbuf->read_cache + pbuf->read_cache_rp, size);
      *buf_size = size;
      status = BM_SUCCESS;
      }

    /* now convert event header */
    if (convert_flags)
      {
      pevent = (EVENT_HEADER *) destination;
      rpc_convert_single(&pevent->event_id, TID_SHORT, RPC_OUTGOING, convert_flags);
      rpc_convert_single(&pevent->trigger_mask, TID_SHORT, RPC_OUTGOING, convert_flags);
      rpc_convert_single(&pevent->serial_number, TID_DWORD, RPC_OUTGOING, convert_flags);
      rpc_convert_single(&pevent->time_stamp, TID_DWORD, RPC_OUTGOING, convert_flags);
      rpc_convert_single(&pevent->data_size, TID_DWORD, RPC_OUTGOING, convert_flags);
      }

    /* correct size for DWORD boundary */
    size = ALIGN(size);

    pbuf->read_cache_rp += size;

    if (pbuf->read_cache_rp == pbuf->read_cache_wp)
      pbuf->read_cache_rp = pbuf->read_cache_wp = 0;

    return status;
    }

  /* calculate some shorthands */
  pheader           = pbuf->buffer_header;
  pdata             = (char *) (pheader + 1);
  my_client_index   = pbuf->client_index;
  pclient           = pheader->client;
  pc                = pheader->client + my_client_index;

  /* first do a quick check without locking the buffer */
  if (async_flag == ASYNC && pheader->write_pointer == pc->read_pointer)
    return BM_ASYNC_RETURN;

  /* lock the buffer */
  bm_lock_buffer(buffer_handle);

LOOP:

  while (pheader->write_pointer == pc->read_pointer)
    {
    bm_unlock_buffer(buffer_handle);

    /* return now in ASYNC mode */
    if (async_flag == ASYNC)
      return BM_ASYNC_RETURN;

    pc->read_wait = TRUE;

    /* check again pointers (may have moved in between) */
    if (pheader->write_pointer == pc->read_pointer)
      {
#ifdef DEBUG_MSG
      cm_msg(MDEBUG, "Receive sleep: grp=%d, rp=%d wp=%d",
              pheader->read_pointer,
              pc->read_pointer,
              pheader->write_pointer);
#endif

      status = ss_suspend(1000, MSG_BM);

#ifdef DEBUG_MSG
      cm_msg(MDEBUG, "Receive woke up: rp=%d, wp=%d",
              pheader->read_pointer,
              pheader->write_pointer);
#endif

      /* return if TCP connection broken */
      if (status == SS_ABORT)
        return SS_ABORT;
      }

    pc->read_wait = FALSE;

    bm_lock_buffer(buffer_handle);
    }

  /* check if event at current read pointer matches a request */
  found = FALSE;

  do
    {
    pevent = (EVENT_HEADER *) (pdata + pc->read_pointer);

    total_size = pevent->data_size + sizeof(EVENT_HEADER);
    total_size = ALIGN(total_size);

    prequest = pc->event_request;

    for (i=0 ; i<pc->max_request_index ; i++,prequest++)
      if (prequest->valid &&
          bm_match_event(prequest->event_id, prequest->trigger_mask, pevent))
        {
        /* we found one, so copy it */

        if (pbuf->read_cache_size > 0 && 
            !(total_size > pbuf->read_cache_size && pbuf->read_cache_wp == 0))
          {
          if (pbuf->read_cache_size - pbuf->read_cache_wp < total_size)
            goto CACHE_FULL;

          if (pc->read_pointer + total_size <= pheader->size)
            {
            /* copy event to cache */
            memcpy(pbuf->read_cache+pbuf->read_cache_wp, pevent, total_size);
            }
          else
            {
            /* event is splitted */
            size = pheader->size - pc->read_pointer;
            memcpy(pbuf->read_cache+pbuf->read_cache_wp, pevent, size);
            memcpy((char *) pbuf->read_cache + pbuf->read_cache_wp + size,
                   pdata, total_size - size);
            }
          }
        else
          {
          if (pc->read_pointer + total_size <= pheader->size)
            {
            /* event is not splitted */
            if (total_size > max_size)
              memcpy(destination, pevent, max_size);
            else
              memcpy(destination, pevent, total_size);
            }
          else
            {
            /* event is splitted */
            size = pheader->size - pc->read_pointer;

            if (size > max_size)
              memcpy(destination, pevent, max_size);
            else
              memcpy(destination, pevent, size);

            if (total_size > max_size)
              {
              if (size <= max_size)
                memcpy((char *) destination + size, pdata, max_size - size);
              }
            else
              memcpy((char *) destination + size, pdata, total_size - size);
            }

          if (total_size < max_size)
            *buf_size = total_size;
          else
            *buf_size = max_size;

          /* now convert event header */
          if (convert_flags)
            {
            pevent = (EVENT_HEADER *) destination;
            rpc_convert_single(&pevent->event_id, TID_SHORT, RPC_OUTGOING, convert_flags);
            rpc_convert_single(&pevent->trigger_mask, TID_SHORT, RPC_OUTGOING, convert_flags);
            rpc_convert_single(&pevent->serial_number, TID_DWORD, RPC_OUTGOING, convert_flags);
            rpc_convert_single(&pevent->time_stamp, TID_DWORD, RPC_OUTGOING, convert_flags);
            rpc_convert_single(&pevent->data_size, TID_DWORD, RPC_OUTGOING, convert_flags);
            }
          }

        if (pbuf->read_cache_size > 0 &&
            !(total_size > pbuf->read_cache_size && pbuf->read_cache_wp == 0))
          {
          pbuf->read_cache_wp += total_size;
          }
        else
          {
          if (total_size > max_size)
            {
            cm_msg(MERROR, "bm_receive_event", "event size larger than buffer size");
            status = BM_TRUNCATED;
            }
          else
            status = BM_SUCCESS;
          }

        /* update statistics */
        found = TRUE;
        pheader->num_out_events++;
        break;
        }

    old_read_pointer = pc->read_pointer;

    /* shift read pointer */
    new_read_pointer = (pc->read_pointer + total_size) % pheader->size;

    if (new_read_pointer > pheader->size - (int) sizeof(EVENT_HEADER))
      new_read_pointer = 0;

#ifdef DEBUG_MSG
    cm_msg(MDEBUG, "bm_receive_event -> wp=%d, rp %d -> %d (found=%d,size=%d)",
            pheader->write_pointer, pc->read_pointer, new_read_pointer, found, total_size);
#endif

    pc->read_pointer = new_read_pointer;

    /*
    Repeat until a requested event is found or no more events
    are available.
    */

    if (pbuf->read_cache_size == 0 && found)
      break;

    /* break if event has bypassed read cache */
    if (pbuf->read_cache_size > 0 &&
        total_size > pbuf->read_cache_size && pbuf->read_cache_wp == 0)
      break;

    } while (pheader->write_pointer != pc->read_pointer);

CACHE_FULL:

  /* calculate global read pointer as "minimum" of client read pointers */
  min_wp = pheader->write_pointer;

  for (i=0,pctmp=pclient ; i<pheader->max_client_index ; i++,pctmp++)
    if (pctmp->pid)
      {
      if (pctmp->read_pointer < min_wp)
        min_wp = pctmp->read_pointer;

      if (pctmp->read_pointer > pheader->write_pointer &&
          pctmp->read_pointer - pheader->size < min_wp)
        min_wp = pctmp->read_pointer - pheader->size;
      }

  if (min_wp<0)
    min_wp += pheader->size;

  pheader->read_pointer = min_wp;

  /*
  If read pointer has been changed, it may have freed up some space
  for waiting producers. So check if free space is now more than 50%
  of the buffer size and wake waiting producers.
  */
  size = pc->read_pointer - pheader->write_pointer;
  if (size <= 0)
    size += pheader->size;

  if (size >= pheader->size * 0.5)
    for (i=0,pctmp=pclient ; i<pheader->max_client_index ; i++,pctmp++)
      if (pctmp->pid && (pctmp->write_wait < size) &&
          (pctmp->pid != ss_getpid() ||  /* check if not own thread */
           (pctmp->pid == ss_getpid() && pctmp->tid != ss_gettid()) ))
        {
#ifdef DEBUG_MSG
        cm_msg(MDEBUG, "Receive wake: rp=%d, wp=%d, level=%1.1lf",
                pheader->read_pointer,
                pheader->write_pointer,
                100-100.0*size/pheader->size);
#endif
        ss_resume(pctmp->port, "B  ");
        }

  /* if no matching event found, start again */
  if (!found)
    goto LOOP;

  bm_unlock_buffer(buffer_handle);

  if (pbuf->read_cache_size > 0 &&
      !(total_size > pbuf->read_cache_size && pbuf->read_cache_wp == 0))
    goto CACHE_READ;

  return status;
}
#endif /* LOCAL_ROUTINES */

  return SS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT bm_skip_event(INT buffer_handle)
/********************************************************************\

  Routine: bm_skip_event

  Purpose: Skip all events in current buffer. Useful for single
           event displays to see the newest events


  Input:
    INT  buffer_handle      Handle of the buffer. Must be obtained
                            via bm_open_buffer.

  Output:
    none

  Function value:
    BM_SUCCESS              Successful completion
    BM_INVALID_HANDLE       Buffer handle is invalid
    RPC_NET_ERROR           Network error.

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_BM_SKIP_EVENT, buffer_handle);

#ifdef LOCAL_ROUTINES
{
BUFFER          *pbuf;
BUFFER_HEADER   *pheader;
BUFFER_CLIENT   *pclient;

  if (buffer_handle > _buffer_entries || buffer_handle <= 0)
    {
    cm_msg(MERROR, "bm_skip_event", "invalid buffer handle %d", buffer_handle);
    return BM_INVALID_HANDLE;
    }

  pbuf = &_buffer[buffer_handle-1];
  pheader = pbuf->buffer_header;

  if (!pbuf->attached)
    {
    cm_msg(MERROR, "bm_skip_event", "invalid buffer handle %d", buffer_handle);
    return BM_INVALID_HANDLE;
    }

  /* clear cache */
  if (pbuf->read_cache_wp > pbuf->read_cache_rp)
    pbuf->read_cache_rp = pbuf->read_cache_wp = 0;

  bm_lock_buffer(buffer_handle);

  /* forward read pointer to global write pointer */
  pclient = pheader->client + pbuf->client_index;
  pclient->read_pointer = pheader->write_pointer;

  bm_unlock_buffer(buffer_handle);
}
#endif

  return BM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT bm_push_event(char *buffer_name)
/********************************************************************\

  Routine: bm_push_event

  Purpose: Check a buffer if an event is available and call the
           dispatch function if found.


  Input:
    char *buffer_name       Name of buffer
    INT  request_id         Request ID obtained form bm_request_event

  Output:
    <call to dispatch function with pointer to event>

  Function value:
    BM_SUCCESS              Successful completion
    BM_INVALID_HANDLE       Buffer handle is invalid
    BM_TRUNCATED            The event is larger than the destination
                            buffer and was therefore truncated.
    BM_ASYNC_RETURN         Function called in ASYNC mode and no event
                            available.
    RPC_NET_ERROR           Network error.

\********************************************************************/
{
#ifdef LOCAL_ROUTINES
{
BUFFER          *pbuf;
BUFFER_HEADER   *pheader;
BUFFER_CLIENT   *pclient,*pc, *pctmp;
EVENT_REQUEST   *prequest;
EVENT_HEADER    *pevent;
char            *pdata;
INT             i, min_wp, size, total_size, buffer_handle;
INT             my_client_index;
BOOL            found;
INT             old_read_pointer, new_read_pointer;

  for (i=0 ; i<_buffer_entries ; i++)
    if (strcmp(buffer_name, _buffer[i].buffer_header->name) == 0)
      break;
  if (i == _buffer_entries)
    return BM_INVALID_HANDLE;

  buffer_handle = i+1;
  pbuf = &_buffer[buffer_handle-1];

  if (!pbuf->attached)
    return BM_INVALID_HANDLE;

  /* return immediately if no callback routine is defined */
  if (!pbuf->callback)
    return BM_SUCCESS;

  if (_event_buffer_size == 0)
    {
    _event_buffer = MALLOC(1000);
    if (_event_buffer == NULL)
      {
      cm_msg(MERROR, "bm_push_event", "not enough memory to allocate cache buffer");
      return BM_NO_MEMORY;
      }
    _event_buffer_size = 1000;
    }

CACHE_READ:

  /* look if there is anything in the cache */
  if (pbuf->read_cache_wp > pbuf->read_cache_rp)
    {
    pevent = (EVENT_HEADER *) (pbuf->read_cache + pbuf->read_cache_rp);
    size   = pevent->data_size + sizeof(EVENT_HEADER);

    /* correct size for DWORD boundary */
    size = ALIGN(size);

    /* increment read pointer */
    pbuf->read_cache_rp += size;
    if (pbuf->read_cache_rp == pbuf->read_cache_wp)
      pbuf->read_cache_rp = pbuf->read_cache_wp = 0;

    /* call dispatcher */
    for (i=0 ; i<_request_list_entries ; i++)
      if (_request_list[i].buffer_handle == buffer_handle &&
          bm_match_event(_request_list[i].event_id,
                         _request_list[i].trigger_mask, pevent))
      {
      _request_list[i].dispatcher(buffer_handle, i, pevent, (void *)(pevent+1));
      }

    return BM_MORE_EVENTS;
    }

  /* calculate some shorthands */
  pheader           = pbuf->buffer_header;
  pdata             = (char *) (pheader + 1);
  my_client_index   = pbuf->client_index;
  pclient           = pheader->client;
  pc                = pheader->client + my_client_index;

  /* first do a quick check without locking the buffer */
  if (pheader->write_pointer == pc->read_pointer)
    return BM_SUCCESS;

  /* lock the buffer */
  bm_lock_buffer(buffer_handle);

LOOP:

  if (pheader->write_pointer == pc->read_pointer)
    {
    bm_unlock_buffer(buffer_handle);

    /* return if no event available */
    return BM_SUCCESS;
    }

  /* check if event at current read pointer matches a request */
  found = FALSE;

  do
    {
    pevent = (EVENT_HEADER *) (pdata + pc->read_pointer);

    total_size = pevent->data_size + sizeof(EVENT_HEADER);
    total_size = ALIGN(total_size);

    prequest = pc->event_request;

    for (i=0 ; i<pc->max_request_index ; i++,prequest++)
      if (prequest->valid &&
          bm_match_event(prequest->event_id, prequest->trigger_mask, pevent))
        {
        /* we found one, so copy it */

        if (pbuf->read_cache_size > 0 && 
            !(total_size > pbuf->read_cache_size && pbuf->read_cache_wp == 0))
          {
          /* copy dispatch function and event to cache */

          if (pbuf->read_cache_size - pbuf->read_cache_wp < 
              total_size + (INT)sizeof(void*) + (INT)sizeof(INT))
            goto CACHE_FULL;

          if (pc->read_pointer + total_size <= pheader->size)
            {
            /* copy event to cache */
            memcpy(pbuf->read_cache+pbuf->read_cache_wp, pevent, total_size);
            }
          else
            {
            /* event is splitted */

            size = pheader->size - pc->read_pointer;
            memcpy(pbuf->read_cache+pbuf->read_cache_wp, pevent, size);
            memcpy((char *) pbuf->read_cache + pbuf->read_cache_wp + size,
                   pdata, total_size - size);
            }

          pbuf->read_cache_wp += total_size;
          }
        else
          {
          /* copy event to copy buffer, save dispatcher */

          if (total_size > _event_buffer_size)
            {
            _event_buffer = realloc(_event_buffer, total_size);
            _event_buffer_size = total_size;
            }

          if (pc->read_pointer + total_size <= pheader->size)
            {
            memcpy(_event_buffer, pevent, total_size);
            }
          else
            {
            /* event is splitted */
            size = pheader->size - pc->read_pointer;

            memcpy(_event_buffer, pevent, size);
            memcpy((char *) _event_buffer + size, pdata, total_size - size);
            }
          }

        /* update statistics */
        found = TRUE;
        pheader->num_out_events++;
        break;
        }

    old_read_pointer = pc->read_pointer;

    /* shift read pointer */
    new_read_pointer = (pc->read_pointer + total_size) % pheader->size;

    if (new_read_pointer > pheader->size - (int) sizeof(EVENT_HEADER))
      new_read_pointer = 0;

#ifdef DEBUG_MSG
    cm_msg(MDEBUG, "bm_receive_event -> wp=%d, rp %d -> %d (found=%d,size=%d)",
            pheader->write_pointer, pc->read_pointer, new_read_pointer, found, total_size);
#endif

    pc->read_pointer = new_read_pointer;

    /*
    Repeat until a requested event is found or no more events
    are available or large event received.
    */

    if (pbuf->read_cache_size == 0 && found)
      break;

    /* break if event has bypassed read cache */
    if (pbuf->read_cache_size > 0 &&
        total_size > pbuf->read_cache_size && pbuf->read_cache_wp == 0)
      break;

    } while (pheader->write_pointer != pc->read_pointer);

CACHE_FULL:

  /* calculate global read pointer as "minimum" of client read pointers */
  min_wp = pheader->write_pointer;

  for (i=0,pctmp=pclient ; i<pheader->max_client_index ; i++,pctmp++)
    if (pctmp->pid)
      {
      if (pctmp->read_pointer < min_wp)
        min_wp = pctmp->read_pointer;

      if (pctmp->read_pointer > pheader->write_pointer &&
          pctmp->read_pointer - pheader->size < min_wp)
        min_wp = pctmp->read_pointer - pheader->size;
      }

  if (min_wp<0)
    min_wp += pheader->size;

  pheader->read_pointer = min_wp;

  /*
  If read pointer has been changed, it may have freed up some space
  for waiting producers. So check if free space is now more than 50%
  of the buffer size and wake waiting producers.
  */
  size = pc->read_pointer - pheader->write_pointer;
  if (size <= 0)
    size += pheader->size;

  if (size >= pheader->size * 0.5)
    for (i=0,pctmp=pclient ; i<pheader->max_client_index ; i++,pctmp++)
      if (pctmp->pid && (pctmp->write_wait < size) &&
          (pctmp->pid != ss_getpid() ||  /* check if not own thread */
           (pctmp->pid == ss_getpid() && pctmp->tid != ss_gettid()) ))
        {
#ifdef DEBUG_MSG
        cm_msg(MDEBUG, "Receive wake: rp=%d, wp=%d, level=%1.1lf",
                pheader->read_pointer,
                pheader->write_pointer,
                100-100.0*size/pheader->size);
#endif
        ss_resume(pctmp->port, "B  ");
        }

  /* if no matching event found, start again */
  if (!found)
    goto LOOP;

  bm_unlock_buffer(buffer_handle);

  if (pbuf->read_cache_size > 0 && 
      !(total_size > pbuf->read_cache_size && pbuf->read_cache_wp == 0)) 
    goto CACHE_READ;

  /* call dispatcher */
  for (i=0 ; i<_request_list_entries ; i++)
    if (_request_list[i].buffer_handle == buffer_handle &&
          bm_match_event(_request_list[i].event_id,
                         _request_list[i].trigger_mask, _event_buffer))
    {
    _request_list[i].dispatcher(buffer_handle, i, _event_buffer, 
                                (void *)(((EVENT_HEADER *) _event_buffer)+1));
    }

  return BM_MORE_EVENTS;
}
#endif /* LOCAL_ROUTINES */

  return BM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT bm_check_buffers()
/********************************************************************\

  Routine: bm_check_buffers

  Purpose: Check if any requested event is waiting in a buffer


  Input:
    none 

  Output:
    none

  Function value:
    TRUE             More events are waiting
    FALSE            No more events are waiting

\********************************************************************/
{
#ifdef LOCAL_ROUTINES
{
INT             index, status;
INT             server_type, server_conn, tid;
BOOL            bMore;
DWORD           start_time;

  server_type = rpc_get_server_option(RPC_OSERVER_TYPE);
  server_conn = rpc_get_server_acception();
  tid = ss_gettid();

  /* if running as a server, buffer checking is done by client
     via ASYNC bm_receive_event */
  if (server_type == ST_SUBPROCESS ||
      server_type == ST_MTHREAD)
    return FALSE;

  bMore = FALSE;
  start_time = ss_millitime();

  /* go through all buffers */
  for (index=0 ; index<_buffer_entries ; index++)
    {
    if (server_type == ST_SINGLE &&
        _buffer[index].index != server_conn)
      continue;

    if (server_type != ST_SINGLE &&
        _buffer[index].index != tid)
      continue;

    if (!_buffer[index].attached)
      continue;

    do
      {
      status = bm_push_event(_buffer[index].buffer_header->name);

      if (status != BM_MORE_EVENTS)
        break;
      
      /* stop after one second */
      if (ss_millitime() - start_time > 1000)
        {
        bMore = TRUE;
        break;
        }

      } while (TRUE);
    }

  return bMore;

}
#endif /* LOCAL_ROUTINES */

  return FALSE;
}

/*------------------------------------------------------------------*/

INT bm_mark_read_waiting(BOOL flag)
/********************************************************************\

  Routine: bm_mark_read_waiting

  Purpose: Mark all open buffers ready for receiving events.
           Called internally by ss_suspend


  Input:
    BOOL flag               TRUE for waiting, FALSE for not waiting

  Output:
    none

  Function value:
    BM_SUCCESS              Successful completion

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_BM_MARK_READ_WAITING, flag);

#ifdef LOCAL_ROUTINES
{
INT           i;
BUFFER_HEADER *pheader;
BUFFER_CLIENT *pclient;

  /* Mark all buffer for read waiting */
  for (i=0 ; i<_buffer_entries ; i++)
    {
    if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_SINGLE &&
        _buffer[i].index != rpc_get_server_acception())
      continue;
  
    if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_SINGLE &&
        _buffer[i].index != ss_gettid())
      continue;

    if (!_buffer[i].attached)
      continue;
    
    pheader            = _buffer[i].buffer_header;
    pclient            = pheader->client + _buffer[i].client_index;
    pclient->read_wait = flag;
    }
}
#endif /* LOCAL_ROUTINES */

  return BM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT bm_notify_client(char *buffer_name, int socket)
/********************************************************************\

  Routine: bm_notify_client

  Purpose: Called by cm_dispatch_ipc. Send an event notification to
           the connected client

  Input:
    char  *buffer_name      Name of buffer
    int   socket            Network socket to client

  Output:
    none

  Function value:
    BM_SUCCESS              Successful completion

\********************************************************************/
{
char         buffer[32];
NET_COMMAND  *nc;
INT          i, convert_flags;
static DWORD last_time = 0;

  for (i=0 ; i<_buffer_entries ; i++)
    if (strcmp(buffer_name, _buffer[i].buffer_header->name) == 0)
      break;
  if (i == _buffer_entries)
    return BM_INVALID_HANDLE;

  /* don't send notification if client has no callback defined
     to receive events -> client calls bm_receive_event manually */
  if(!_buffer[i].callback)
    return DB_SUCCESS;

  convert_flags = rpc_get_server_option(RPC_CONVERT_FLAGS);

  /* only send notification once each 500ms */
  if (ss_millitime()-last_time < 500 && !(convert_flags & CF_ASCII))
    return DB_SUCCESS;

  last_time = ss_millitime();

  if (convert_flags & CF_ASCII)
    {
    sprintf(buffer, "MSG_BM&%s", buffer_name);
    send_tcp(socket, buffer, strlen(buffer)+1, 0);
    }
  else
    {
    nc = (NET_COMMAND *) buffer;

    nc->header.routine_id  = MSG_BM;
    nc->header.param_size  = 0;

    if (convert_flags)
      {
      rpc_convert_single(&nc->header.routine_id, TID_DWORD, 
                         RPC_OUTGOING, convert_flags);
      rpc_convert_single(&nc->header.param_size, TID_DWORD, 
                         RPC_OUTGOING, convert_flags);
      }

    /* send the update notification to the client */
    send_tcp(socket, (char *) buffer, sizeof(NET_COMMAND_HEADER), 0);
    }

  return BM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT bm_poll_event(INT flag)
/********************************************************************\

  Routine: bm_poll_event

  Purpose: Poll an event from a remote server. Gets called by 
           rpc_client_dispatch

  Input:
    INT flag         TRUE if called from cm_yield

  Output:
    none

  Function value:
    TRUE             More events are waiting
    FALSE            No more events are waiting
    SS_ABORT         Network connection broken

\********************************************************************/
{
INT                  status, size, i, request_id;
DWORD                start_time;
BOOL                 bMore;
static BOOL          bMoreLast = FALSE;

  if (_event_buffer_size == 0)
    {
    _event_buffer = MALLOC(MAX_EVENT_SIZE + sizeof(EVENT_HEADER));
    if (!_event_buffer)
      {
      cm_msg(MERROR, "bm_poll_event", "not enough memory to allocate event buffer");
      return SS_ABORT;
      }
    _event_buffer_size = MAX_EVENT_SIZE + sizeof(EVENT_HEADER);
    }

  start_time = ss_millitime();

  /* if we got event notification, turn off read_wait */
  if (!flag)
    bm_mark_read_waiting(FALSE);

  /* if called from yield, return if no more events */
  if (flag)
    {
    if (!bMoreLast)
      return FALSE;
    }

  bMore = FALSE;

  /* loop over all requests */
  for (request_id=0 ; request_id<_request_list_entries ; request_id++)
    {
    /* continue if no dispatcher set (manual bm_receive_event) */
    if (_request_list[request_id].dispatcher == NULL)
      continue;

    do
      {
      /* receive event */
      size = _event_buffer_size;
      status = bm_receive_event(_request_list[request_id].buffer_handle,
                                _event_buffer, &size, ASYNC);

      /* call user function if successful */
      if (status == BM_SUCCESS)
        /* search proper request for this event */
        for (i=0 ; i<_request_list_entries ; i++)
          if ((_request_list[i].buffer_handle == 
                 _request_list[request_id].buffer_handle) &&
               bm_match_event(_request_list[i].event_id,
                              _request_list[i].trigger_mask, _event_buffer))
          {
          _request_list[i].dispatcher(_request_list[i].buffer_handle, i, _event_buffer,
                                      (void *)(((EVENT_HEADER *) _event_buffer)+1));
          }

      /* break if no more events */
      if (status == BM_ASYNC_RETURN)
        break;

      /* break if server died */
      if (status == RPC_NET_ERROR)
        return SS_ABORT;

      /* stop after one second */
      if (ss_millitime() - start_time > 1000)
        {
        bMore = TRUE;
        break;
        }

      } while (TRUE);
    }

  if (!bMore)
    bm_mark_read_waiting(TRUE);

  bMoreLast = bMore;

  return bMore;
}

/*------------------------------------------------------------------*/

INT bm_empty_buffers()
/********************************************************************\

  Routine: bm_empty_buffers

  Purpose: Clear any event cache and forward read pointers of all
           buffers to the current write pointers in order to skip
           old events. This routine can be called to make changes
           in the hardware immediately visible to the analyzer.

  Input:
    none 

  Output:
    none

  Function value:
    BM_SUCCESS              Successful completion

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_BM_EMPTY_BUFFERS);

#ifdef LOCAL_ROUTINES
{
INT             index, server_type, server_conn, tid;
BUFFER          *pbuf;
BUFFER_CLIENT   *pclient;

  server_type = rpc_get_server_option(RPC_OSERVER_TYPE);
  server_conn = rpc_get_server_acception();
  tid = ss_gettid();

  /* go through all buffers */
  for (index=0 ; index<_buffer_entries ; index++)
    {
    if (server_type == ST_SINGLE &&
        _buffer[index].index != server_conn)
      continue;

    if (server_type != ST_SINGLE &&
        _buffer[index].index != tid)
      continue;

    if (!_buffer[index].attached)
      continue;

    pbuf = &_buffer[index];

    /* empty cache */
    pbuf->read_cache_rp = pbuf->read_cache_wp = 0;

    /* set read pointer to write pointer */
    pclient = (pbuf->buffer_header)->client + pbuf->client_index;
    bm_lock_buffer(index+1);
    pclient->read_pointer = (pbuf->buffer_header)->write_pointer;
    bm_unlock_buffer(index+1);
    }

}
#endif /* LOCAL_ROUTINES */

  return BM_SUCCESS;
}

/*------------------------------------------------------------------*/

/********************************************************************\
*                                                                    *
*                         RPC functions                              *
*                                                                    *
\********************************************************************/

/*------------------------------------------------------------------*/

/* globals */

RPC_CLIENT_CONNECTION  _client_connection[MAX_RPC_CONNECTION];                         
RPC_SERVER_CONNECTION  _server_connection;
                                                                                    
static int             _lsock;
RPC_SERVER_ACCEPTION   _server_acception[MAX_RPC_CONNECTION];
static INT             _server_acception_index = 0;
static INT             _server_type;
static char            _server_name[256];

static RPC_LIST        *rpc_list=NULL;

int _opt_tcp_size = OPT_TCP_SIZE;

/*------------------------------------------------------------------*/

/********************************************************************\
*                       conversion functions                         *
\********************************************************************/

void rpc_calc_convert_flags(INT hw_type, INT remote_hw_type, 
                            INT *convert_flags)
{
  *convert_flags = 0;

  /* big/little endian conversion */
  if ( ((remote_hw_type & DRI_BIG_ENDIAN) &&
        (       hw_type & DRI_LITTLE_ENDIAN) ) ||
       ((remote_hw_type & DRI_LITTLE_ENDIAN) &&
        (       hw_type & DRI_BIG_ENDIAN) ))
    *convert_flags |= CF_ENDIAN;

  /* float conversion between IEEE and VAX G */
  if ( (remote_hw_type & DRF_G_FLOAT) &&
       (       hw_type & DRF_IEEE) )
    *convert_flags |= CF_VAX2IEEE;

  /* float conversion between VAX G and IEEE */
  if ( (remote_hw_type & DRF_IEEE) &&
       (       hw_type & DRF_G_FLOAT) )
    *convert_flags |= CF_IEEE2VAX;

  /* ASCII format */
  if (remote_hw_type & DR_ASCII)
    *convert_flags |= CF_ASCII;
}

/*------------------------------------------------------------------*/

void rpc_get_convert_flags(INT *convert_flags)
{
  rpc_calc_convert_flags( rpc_get_option(0, RPC_OHW_TYPE),
                          _server_connection.remote_hw_type,
                          convert_flags);
}

/*------------------------------------------------------------------*/

void rpc_ieee2vax_float(float *var)
{
unsigned short int lo, hi;

  /* swap hi and lo word */
  lo = *((short int *) (var) + 1);
  hi = *((short int *) (var));

  /* correct exponent */
  if (lo != 0)
    lo += 0x100;

  *((short int *) (var) + 1) = hi;
  *((short int *) (var))     = lo;
}

void rpc_vax2ieee_float(float *var)
{
unsigned short int lo, hi;

  /* swap hi and lo word */
  lo = *((short int *) (var) + 1);
  hi = *((short int *) (var));

  /* correct exponent */
  if (hi != 0)
    hi -= 0x100;

  *((short int *) (var) + 1) = hi;
  *((short int *) (var))     = lo;

}

void rpc_vax2ieee_double(double *var)
{
unsigned short int i1, i2, i3, i4;

  /* swap words */
  i1 = *((short int *) (var) + 3);
  i2 = *((short int *) (var) + 2);
  i3 = *((short int *) (var) + 1);
  i4 = *((short int *) (var));

  /* correct exponent */
  if (i4 != 0)
    i4 -= 0x20;

  *((short int *) (var) + 3) = i4;
  *((short int *) (var) + 2) = i3;
  *((short int *) (var) + 1) = i2;
  *((short int *) (var))     = i1;
}

void rpc_ieee2vax_double(double *var)
{
unsigned short int i1, i2, i3, i4;

  /* swap words */
  i1 = *((short int *) (var) + 3);
  i2 = *((short int *) (var) + 2);
  i3 = *((short int *) (var) + 1);
  i4 = *((short int *) (var));

  /* correct exponent */
  if (i1 != 0)
    i1 += 0x20;

  *((short int *) (var) + 3) = i4;
  *((short int *) (var) + 2) = i3;
  *((short int *) (var) + 1) = i2;
  *((short int *) (var))     = i1;
}

/*------------------------------------------------------------------*/

void rpc_convert_single(void *data, INT tid, INT flags, INT convert_flags)
{

  if (convert_flags & CF_ENDIAN)
    {
    if (tid == TID_WORD  || tid == TID_SHORT)
      WORD_SWAP(data);
    if (tid == TID_DWORD || tid == TID_INT   || tid == TID_BOOL ||
        tid == TID_FLOAT)
      DWORD_SWAP(data);
    if (tid == TID_DOUBLE)
      QWORD_SWAP(data);
    }

  if (((convert_flags & CF_IEEE2VAX) && !(flags & RPC_OUTGOING))  ||
      ((convert_flags & CF_VAX2IEEE) && (flags & RPC_OUTGOING)) )
    {
    if (tid == TID_FLOAT)
      rpc_ieee2vax_float(data);
    if (tid == TID_DOUBLE)
      rpc_ieee2vax_double(data);
    }

  if (((convert_flags & CF_IEEE2VAX) && (flags & RPC_OUTGOING))  ||
      ((convert_flags & CF_VAX2IEEE) && !(flags & RPC_OUTGOING)) )
    {
    if (tid == TID_FLOAT)
      rpc_vax2ieee_float(data);
    if (tid == TID_DOUBLE)
      rpc_vax2ieee_double(data);
    }
}

void rpc_convert_data(void *data, INT tid, INT flags, 
                      INT total_size, INT convert_flags)
/********************************************************************\

  Routine: rpc_convert_data

  Purpose: Convert data format between differenct computers

  Input:
    void   *data            Pointer to data
    INT    tid              Type ID of data, one of TID_xxx
    INT    flags            Combination of following flags:
                              RPC_IN: data is input parameter
                              RPC_OUT: data is output variable
                              RPC_FIXARRAY, RPC_VARARRAY: data is array
                                of "size" bytes (see next param.)
                              RPC_OUTGOING: data is outgoing
    INT    total_size       Size of bytes of data. Used for variable
                            length arrays.
    INT    convert_flags    Flags for data conversion

  Output:
    void   *data            Is converted according to _convert_flag
                            value

  Function value:
    RPC_SUCCESS             Successful completion

\********************************************************************/
{
INT  i, n, single_size;
char *p;

  /* convert array */
  if (flags & (RPC_FIXARRAY | RPC_VARARRAY))
    {
    single_size = tid_size[ tid ];
    /* don't convert TID_ARRAY & TID_STRUCT */
    if (single_size == 0)
      return;

    n = total_size / single_size;

    for (i=0 ; i<n ; i++)
      {
      p = (char *) data + (i * single_size);
      rpc_convert_single(p, tid, flags, convert_flags);
      }
    }
  else
    {
    rpc_convert_single(data, tid, flags, convert_flags);
    }
}

/********************************************************************\
*                       type ID functions                            *
\********************************************************************/

INT rpc_tid_size(INT id)
{
  if (id < TID_LAST)
    return tid_size[id];
  
  return 0;
}

char *rpc_tid_name(INT id)
{
  if (id < TID_LAST)
    return tid_name[id];
  else
    return "<unknown>";
}

/*------------------------------------------------------------------*/

/********************************************************************\
*                        client functions                            *
\********************************************************************/

/*------------------------------------------------------------------*/

INT rpc_register_client(char *name, RPC_LIST *list)
/********************************************************************\

  Routine: rpc_register_client

  Purpose: Register RPC client for standalone mode (without standard
           midas server)

  Input:
    RPC_LIST list           Array of RPC_LIST structures containing
                            function IDs and parameter definitions.
                            The end of the list must be indicated by
                            a function ID of zero.
    char     *name          Name of this client

  Output:
   <implicit: new_list gets appended to rpc_list>

  Function value:
   RPC_SUCCESS              Successful completion

\********************************************************************/
{
  rpc_set_name(name);
  rpc_register_functions(rpc_get_internal_list(0), NULL);
  rpc_register_functions(list, NULL);

  return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

INT rpc_register_functions(RPC_LIST *new_list, INT (*func)(INT,void**))
/********************************************************************\

  Routine: rpc_register_functions

  Purpose: Register a set of RPC functions (both as clients or servers)

  Input:
    RPC_LIST new_list       Array of RPC_LIST structures containing
                            function IDs and parameter definitions.
                            The end of the list must be indicated by
                            a function ID of zero.
    INT      *func          Default dispatch function

  Output:
   <implicit: new_list gets appended to rpc_list>

  Function value:
   RPC_SUCCESS              Successful completion
   RPC_NO_MEMORY            Out of memory
   RPC_DOUBLE_DEFINED       

\********************************************************************/
{
INT i, j, iold, inew;

  /* count number of new functions */
  for (i=0 ; new_list[i].id != 0 ; i++)
    {
    /* check double defined functions */
    for (j=0 ; rpc_list != NULL && rpc_list[j].id != 0 ; j++)
      if (rpc_list[j].id == new_list[i].id)
        return RPC_DOUBLE_DEFINED;
    }
  inew = i;

  /* count number of existing functions */
  for (i=0 ; rpc_list != NULL && rpc_list[i].id != 0 ; i++);
  iold = i;

  /* allocate new memory for rpc_list */
  if (rpc_list == NULL)
    rpc_list = malloc(sizeof(RPC_LIST) * (inew+1));
  else
    rpc_list = realloc(rpc_list, sizeof(RPC_LIST) * (iold+inew+1));

  if (rpc_list == NULL)
    {
    cm_msg(MERROR, "rpc_register_functions", "out of memory");
    return RPC_NO_MEMORY;
    }

  /* append new functions */
  for (i=iold ; i<iold+inew ; i++)
    {
    memcpy(rpc_list+i, new_list+i-iold, sizeof(RPC_LIST));
    
    /* set default dispatcher */
    if (rpc_list[i].dispatch == NULL)
      rpc_list[i].dispatch = func;

    /* check valid ID for user functions */
    if (new_list != rpc_get_internal_list(0) &&
        new_list != rpc_get_internal_list(1) &&        
        (rpc_list[i].id < RPC_MIN_ID || rpc_list[i].id > RPC_MAX_ID))
      cm_msg(MERROR, "rpc_register_functions", "registered RPC function with invalid ID");
    }

  /* mark end of list */
  rpc_list[i].id = 0;

  return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

INT rpc_deregister_functions()
/********************************************************************\

  Routine: rpc_deregister_functions

  Purpose: Free memory of previously registered functions

  Input:
    none

  Output:
    none

  Function value:
    RPC_SUCCESS              Successful completion

\********************************************************************/
{
  if (rpc_list)
    free(rpc_list);
  rpc_list = NULL;

  return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

INT rpc_register_function(INT id, INT (*func)(INT,void**))
/********************************************************************\

  Routine: rpc_register_function

  Purpose: Replace a dispatch function for a specific rpc routine

  Input:
    INT      id             RPC ID
    INT      *func          New dispatch function

  Output:
   <implicit: func gets copied to rpc_list>

  Function value:
   RPC_SUCCESS              Successful completion
   RPC_INVALID_ID           RPC ID not found

\********************************************************************/
{
INT i;

  for (i=0 ; rpc_list != NULL && rpc_list[i].id != 0 ; i++)
    if (rpc_list[i].id == id)
      break;

  if (rpc_list[i].id == id)
    rpc_list[i].dispatch = func;
  else
    return RPC_INVALID_ID;

  return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

INT rpc_client_dispatch(int sock)
/********************************************************************\

  Routine: rpc_client_dispatch

  Purpose: Gets called whenever a client receives data from the
           server. Get set via rpc_connect. Internal use only.

\********************************************************************/
{
INT         hDB, hKey, n;
NET_COMMAND *nc;
INT         status;
char        net_buffer[256];

  nc = (NET_COMMAND *) net_buffer;

  n = recv_tcp(sock, net_buffer, sizeof(net_buffer), 0);
  if (n<=0)
    return SS_ABORT;

  if (nc->header.routine_id == MSG_ODB)
    {
    /* update a changed record */
    hDB  = *((INT *)nc->param);
    hKey = *((INT *)nc->param+1);
    status = db_update_record(hDB, hKey, 0);
    }

  else if (nc->header.routine_id == MSG_WATCHDOG)
    {
    nc->header.routine_id = 1;
    nc->header.param_size = 0;
    send_tcp(sock, net_buffer, sizeof(NET_COMMAND_HEADER), 0);
    status = RPC_SUCCESS;
    }

  else if (nc->header.routine_id == MSG_BM)
    {
    fd_set         readfds;
    struct timeval timeout;

    /* receive further messages to empty TCP queue */
    do
      {
      FD_ZERO(&readfds);
      FD_SET(sock, &readfds);

      timeout.tv_sec  = 0;
      timeout.tv_usec = 0;

      select(FD_SETSIZE, (void *) &readfds, NULL, NULL, (void *) &timeout);

      if (FD_ISSET(sock, &readfds))
	      {
        n = recv_tcp(sock, net_buffer, sizeof(net_buffer), 0);
        if (n<=0)
          return SS_ABORT;

        if (nc->header.routine_id == MSG_ODB)
          {
          /* update a changed record */
          hDB  = *((INT *)nc->param);
          hKey = *((INT *)nc->param+1);
          status = db_update_record(hDB, hKey, 0);
          }

        else if (nc->header.routine_id == MSG_WATCHDOG)
          {
          nc->header.routine_id = 1;
          nc->header.param_size = 0;
          send_tcp(sock, net_buffer, sizeof(NET_COMMAND_HEADER), 0);
          status = RPC_SUCCESS;
          }
        }

      } while (FD_ISSET(sock, &readfds));

    /* poll event from server */
    status = bm_poll_event(FALSE);
    }

  return status;
}

/*------------------------------------------------------------------*/

INT rpc_client_connect(char *host_name, INT port, HNDLE *hConnection)
/********************************************************************\

  Routine: rpc_client_connect

  Purpose: Extablish a network connection to a remote client

  Input:
    char *host_name          IP address of host to connect to.

    INT  port                TPC port to connect to.

  Output:
    HNDLE *hConnection       Handle for new connection which can be used
                             in future rpc_call(hConnection....) calls

  Function value:
    RPC_SUCCESS              Successful completion
    RPC_NET_ERROR            Error in socket call
    RPC_NO_CONNECTION        Maximum number of connections reached
    RPC_NOT_REGISTERED       cm_connect_experiment was not called properly

\********************************************************************/
{
INT                  i, status, index;
struct sockaddr_in   bind_addr;
INT                  sock;
INT                  remote_hw_type, hw_type;
char                 str[200];
char                 version[32], v1[32];
char                 local_prog_name[NAME_LENGTH];
char                 local_host_name[HOST_NAME_LENGTH];
struct hostent       *phe;

#ifdef OS_WINNT
  {
  WSADATA WSAData;

  /* Start windows sockets */
  if ( WSAStartup(MAKEWORD(1,1), &WSAData) != 0)
    return RPC_NET_ERROR;
  }
#endif

  /* check if cm_connect_experiment was called */
  if (_client_name[0] == 0)
    {
    cm_msg(MERROR, "rpc_client_connect", "cm_connect_experiment/rpc_set_name not called");
    return RPC_NOT_REGISTERED;
    }

  /* check for broken connections */
  for (i=0 ; i<MAX_RPC_CONNECTION ; i++)
    if (_client_connection[i].send_sock != 0)
      {
      int             sock;
      fd_set          readfds;
      struct timeval  timeout;
      char            buffer[64];

      sock = _client_connection[i].send_sock;
      FD_ZERO(&readfds);
      FD_SET(sock, &readfds);

      timeout.tv_sec  = 0;
      timeout.tv_usec = 0;

      do
        {
        status = select(FD_SETSIZE, (void *) &readfds, NULL, NULL, (void *) &timeout);
        } while (status == -1); /* dont return if an alarm signal was cought */

      if (FD_ISSET(sock, &readfds))
        {
        status = recv(sock, (char *) buffer, sizeof(buffer), 0);
        if (status<=0)
          {
          /* connection broken -> reset */
          closesocket(sock);
          memset(&_client_connection[i], 0, sizeof(RPC_CLIENT_CONNECTION));
          }
        }
      }

  /* check if connection already exists */
  for (i=0 ; i<MAX_RPC_CONNECTION ; i++)
    if (_client_connection[i].send_sock != 0 &&
        strcmp(_client_connection[i].host_name, host_name) == 0 &&
        _client_connection[i].port == port)
      {
      *hConnection = i+1;
      return RPC_SUCCESS;
      }

  /* search for free entry */
  for (i=0 ; i<MAX_RPC_CONNECTION ; i++)
    if (_client_connection[i].send_sock == 0)
      break;

  /* open new network connection */
  if (i == MAX_RPC_CONNECTION)
    {
    cm_msg(MERROR, "rpc_client_connect", "maximum number of connections exceeded");
    return RPC_NO_CONNECTION;
    }

  /* create a new socket for connecting to remote server */
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1)
    {
    cm_msg(MERROR, "rpc_client_connect", "cannot create socket");
    return RPC_NET_ERROR;
    }

  index = i;
  strcpy(_client_connection[index].host_name, host_name);
  _client_connection[index].port = port;
  _client_connection[index].exp_name[0] = 0;
  _client_connection[index].transport = RPC_TCP;
  _client_connection[index].rpc_timeout = DEFAULT_RPC_TIMEOUT;
  _client_connection[index].rpc_timeout = DEFAULT_RPC_TIMEOUT;

  /* connect to remote node */
  memset(&bind_addr, 0, sizeof(bind_addr));
  bind_addr.sin_family      = AF_INET;
  bind_addr.sin_addr.s_addr = 0;
  bind_addr.sin_port        = htons((short) port);

#ifdef OS_VXWORKS
  {
  INT host_addr;

  host_addr = hostGetByName(host_name);
  memcpy((char *)&(bind_addr.sin_addr), &host_addr, 4);
  }
#else
  phe = gethostbyname(host_name);
  if (phe == NULL)
    {
    cm_msg(MERROR, "rpc_client_connect", "cannot get host name");
    return RPC_NET_ERROR;
    }
  memcpy((char *)&(bind_addr.sin_addr), phe->h_addr, phe->h_length);
#endif

#ifdef OS_UNIX
  do
    {
    status = connect(sock, (void *) &bind_addr, sizeof(bind_addr));

    /* don't return if an alarm signal was cought */
    } while (status == -1 && errno == EINTR); 
#else
  status = connect(sock, (void *) &bind_addr, sizeof(bind_addr));
#endif  

  if (status != 0)
    {
    /* cm_msg(MERROR, "rpc_client_connect", "cannot connect"); 
    message should be displayed by application */
    return RPC_NET_ERROR;
    }

  /* send local computer info */
  rpc_get_name(local_prog_name);
  gethostname(local_host_name, sizeof(local_host_name));

  hw_type = rpc_get_option(0, RPC_OHW_TYPE);
  sprintf(str, "%ld %s %s %s", hw_type, cm_get_version(), local_prog_name, local_host_name);

  send(sock, str, strlen(str)+1, 0);

  /* receive remote computer info */
  i = recv_string(sock, str, sizeof(str), 10000);
  if (i<=0)
    {
      cm_msg(MERROR, "rpc_client_connect", "timeout on receive remote computer info: %s", str);
      return RPC_NET_ERROR;
    }

  remote_hw_type = version[0] = 0;
  sscanf(str, "%ld %s", &remote_hw_type, version);
  _client_connection[index].remote_hw_type = remote_hw_type;
  _client_connection[index].send_sock = sock;

  /* print warning if version patch level doesn't agree */
  strcpy(v1, version);
  if (strchr(v1, '.'))
    if (strchr(strchr(v1, '.')+1, '.'))
      *strchr(strchr(v1, '.')+1, '.') = 0;

  strcpy(str, cm_get_version());
  if (strchr(str, '.'))
    if (strchr(strchr(str, '.')+1, '.'))
      *strchr(strchr(str, '.')+1, '.') = 0;

  if (strcmp(v1, str) != 0)
    {
    sprintf(str, "remote MIDAS version %s differs from local version %s",
                  version, cm_get_version());
    cm_msg(MERROR, "rpc_client_connect", str);
    }

  *hConnection = index+1;

  return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

INT rpc_server_connect(char *host_name, char *exp_name)
/********************************************************************\

  Routine: rpc_connect

  Purpose: Extablish a network connection to a remote MIDAS
           server using a callback scheme.

  Input:
    char *host_name         IP address of host to connect to.

    INT  port               TPC port to connect to.

    char *exp_name          Name of experiment to connect to. By using
                            this name, several experiments (e.g. online
                            DAQ and offline analysis) can run simultan-
                            eously on the same host.

  Output:
    none

  Function value:
    RPC_SUCCESS              Successful completion
    RPC_NET_ERROR            Error in socket call
    RPC_NOT_REGISTERED       cm_connect_experiment was not called properly
    CM_UNDEF_EXP             Undefined experiment on server

\********************************************************************/
{
INT                  i, status, flag;
struct sockaddr_in   bind_addr;
INT                  sock, lsock1, lsock2, lsock3;
INT                  listen_port1, listen_port2, listen_port3;
INT                  remote_hw_type, hw_type;
int                  size;
char                 str[200], version[32], v1[32];
char                 local_host_name[HOST_NAME_LENGTH];
char                 local_prog_name[NAME_LENGTH];
struct hostent       *phe;

#ifdef OS_WINNT
  {
  WSADATA WSAData;

  /* Start windows sockets */
  if ( WSAStartup(MAKEWORD(1,1), &WSAData) != 0)
    return RPC_NET_ERROR;
  }
#endif

  /* check if local connection */
  if (host_name[0] == 0)
    return RPC_SUCCESS;

  /* register system functions */
  rpc_register_functions(rpc_get_internal_list(0), NULL);

  /* check if cm_connect_experiment was called */
  if (_client_name[0] == 0)
    {
    cm_msg(MERROR, "rpc_server_connect", "cm_connect_experiment/rpc_set_name not called");
    return RPC_NOT_REGISTERED;
    }

  /* check if connection already exists */
  if (_server_connection.send_sock != 0)
    return RPC_SUCCESS;

  strcpy(_server_connection.host_name, host_name);
  strcpy(_server_connection.exp_name, exp_name);
  _server_connection.transport = RPC_TCP;
  _server_connection.rpc_timeout = DEFAULT_RPC_TIMEOUT;

  /* create new TCP sockets for listening */
  lsock1 = socket(AF_INET, SOCK_STREAM, 0);
  lsock2 = socket(AF_INET, SOCK_STREAM, 0);
  lsock3 = socket(AF_INET, SOCK_STREAM, 0);
  if (lsock3 == -1)
    {
    cm_msg(MERROR, "rpc_server_connect", "cannot create socket");
    return RPC_NET_ERROR;
    }

  flag = 1;
  setsockopt(lsock1, SOL_SOCKET, SO_REUSEADDR,
             (char *) &flag, sizeof(INT));
  setsockopt(lsock2, SOL_SOCKET, SO_REUSEADDR,
             (char *) &flag, sizeof(INT));
  setsockopt(lsock3, SOL_SOCKET, SO_REUSEADDR,
             (char *) &flag, sizeof(INT));

  /* let OS choose any port number */
  memset(&bind_addr, 0, sizeof(bind_addr));
  bind_addr.sin_family      = AF_INET;
  bind_addr.sin_addr.s_addr = 0;
  bind_addr.sin_port        = 0;

  gethostname(local_host_name, sizeof(local_host_name));

#ifdef OS_VXWORKS
  {
  INT host_addr;

  host_addr = hostGetByName(local_host_name);
  memcpy((char *)&(bind_addr.sin_addr), &host_addr, 4);
  }
#else
  phe = gethostbyname(local_host_name);
  if (phe == NULL)
    {
    cm_msg(MERROR, "rpc_server_connect", "cannot get host name");
    return RPC_NET_ERROR;
    }
  memcpy((char *)&(bind_addr.sin_addr), phe->h_addr, phe->h_length);
#endif

  status = bind(lsock1, (void *) &bind_addr, sizeof(bind_addr));
  bind_addr.sin_port = 0;
  status = bind(lsock2, (void *) &bind_addr, sizeof(bind_addr));
  bind_addr.sin_port = 0;
  status = bind(lsock3, (void *) &bind_addr, sizeof(bind_addr));
  if (status < 0)
    {
    cm_msg(MERROR, "rpc_server_connect", "cannot bind");
    return RPC_NET_ERROR;
    }

  /* listen for connection */
  status = listen(lsock1, 1);
  status = listen(lsock2, 1);
  status = listen(lsock3, 1);
  if (status < 0)
    {
    cm_msg(MERROR, "rpc_server_connect", "cannot listen");
    return RPC_NET_ERROR;
    }

  /* find out which port OS has chosen */
  size = sizeof(bind_addr);
  getsockname(lsock1, (void *)&bind_addr, (void *)&size);
  listen_port1 = ntohs(bind_addr.sin_port);
  getsockname(lsock2, (void *)&bind_addr, (void *)&size);
  listen_port2 = ntohs(bind_addr.sin_port);
  getsockname(lsock3, (void *)&bind_addr, (void *)&size);
  listen_port3 = ntohs(bind_addr.sin_port);

  /* create a new socket for connecting to remote server */
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1)
    {
    cm_msg(MERROR, "rpc_server_connect", "cannot create socket");
    return RPC_NET_ERROR;
    }

  /* let OS choose any port number */
  bind_addr.sin_port = 0;

  status = bind(sock, (void *)&bind_addr, sizeof(bind_addr));
  if (status < 0)
    {
    cm_msg(MERROR, "rpc_server_connect", "cannot bind");
    return RPC_NET_ERROR;
    }

  /* connect to remote node */
  memset(&bind_addr, 0, sizeof(bind_addr));
  bind_addr.sin_family      = AF_INET;
  bind_addr.sin_addr.s_addr = 0;
  bind_addr.sin_port        = htons((short) MIDAS_TCP_PORT);

#ifdef OS_VXWORKS
  {
  INT host_addr;

  host_addr = hostGetByName(host_name);
  memcpy((char *)&(bind_addr.sin_addr), &host_addr, 4);
  }
#else
  phe = gethostbyname(host_name);
  if (phe == NULL)
    {
    cm_msg(MERROR, "rpc_server_connect", "cannot get host name");
    return RPC_NET_ERROR;
    }
  memcpy((char *)&(bind_addr.sin_addr), phe->h_addr, phe->h_length);
#endif

#ifdef OS_UNIX
  do
    {
    status = connect(sock, (void *) &bind_addr, sizeof(bind_addr));

    /* don't return if an alarm signal was cought */
    } while (status == -1 && errno == EINTR); 
#else
  status = connect(sock, (void *) &bind_addr, sizeof(bind_addr));
#endif  

  if (status != 0)
    {
/*    cm_msg(MERROR, "rpc_server_connect", "cannot connect"); message should be displayed by application */
    return RPC_NET_ERROR;
    }

  /* connect to experiment */
  if (exp_name[0] == 0)
    sprintf(str, "C %ld %ld %ld %s Default", 
            listen_port1, listen_port2, listen_port3, cm_get_version());
  else
    sprintf(str, "C %ld %ld %ld %s %s", 
            listen_port1, listen_port2, listen_port3, cm_get_version(), exp_name);

  send(sock, str, strlen(str)+1, 0);
  i = recv_string(sock, str, sizeof(str), 10000);
  closesocket(sock);
  if (i<=0)
    {
    cm_msg(MERROR, "rpc_server_connect", "timeout on receive status from server");
    return RPC_NET_ERROR;
    }

  status = version[0] = 0;
  sscanf(str, "%ld %s", &status, version);

  if (status == 2)
    {
/*  message "undefined experiment" should be displayed by application */
    return CM_UNDEF_EXP;
    }

  /* print warning if version patch level doesn't agree */
  strcpy(v1, version);
  if (strchr(v1, '.'))
    if (strchr(strchr(v1, '.')+1, '.'))
      *strchr(strchr(v1, '.')+1, '.') = 0;

  strcpy(str, cm_get_version());
  if (strchr(str, '.'))
    if (strchr(strchr(str, '.')+1, '.'))
      *strchr(strchr(str, '.')+1, '.') = 0;

  if (strcmp(v1, str) != 0)
    {
    sprintf(str, "remote MIDAS version %s differs from local version %s",
                  version, cm_get_version());
    cm_msg(MERROR, "rpc_server_connect", str);
    }

  /* wait for callback on send and recv socket */
  size = sizeof(bind_addr);
  _server_connection.send_sock =
    accept(lsock1, (void *)&bind_addr, (void *)&size);

  _server_connection.recv_sock =
    accept(lsock2, (void *)&bind_addr, (void *)&size);

  _server_connection.event_sock =
    accept(lsock3, (void *)&bind_addr, (void *)&size);

  if (_server_connection.send_sock == -1 ||
      _server_connection.recv_sock == -1 ||
      _server_connection.event_sock == -1)
    {
    cm_msg(MERROR, "rpc_server_connect", "accept() failed");
    return RPC_NET_ERROR;
    }

  closesocket(lsock1);
  closesocket(lsock2);
  closesocket(lsock3);

  /* set TCP_NODELAY option for better performance */
#ifdef OS_VXWORKS
  flag = 1;
  setsockopt(_server_connection.send_sock,
             IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(flag));
  setsockopt(_server_connection.event_sock,
             IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(flag));
#endif

  /* increase send buffer size to 64kB */
  flag = 0x10000;
  setsockopt(_server_connection.event_sock, SOL_SOCKET, SO_SNDBUF,
            (char *) &flag, sizeof(flag));

  /* send local computer info */
  rpc_get_name(local_prog_name);
  hw_type = rpc_get_option(0, RPC_OHW_TYPE);
  sprintf(str, "%ld %s", hw_type, local_prog_name);

  send(_server_connection.send_sock, str, strlen(str)+1, 0);

  /* receive remote computer info */
  i = recv_string(_server_connection.send_sock,
                  str, sizeof(str), 10000);
  if (i<=0)
    {
    cm_msg(MERROR, "rpc_server_connect", "timeout on receive remote computer info");
    return RPC_NET_ERROR;
    }

  sscanf(str, "%ld", &remote_hw_type);
  _server_connection.remote_hw_type = remote_hw_type;

  /* set dispatcher which receives database updates */
  ss_suspend_set_dispatch(CH_CLIENT, &_server_connection, rpc_client_dispatch);

  return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

INT rpc_client_disconnect(HNDLE hConn, BOOL bShutdown)
/********************************************************************\

  Routine: rpc_client_disconnect

  Purpose: Close a rpc connection to a MIDAS client

  Input:
    HNDLE  hConn           Handle of connection
    BOOL   bShutdown       Shut down remote server if TRUE

  Output:
    none

  Function value:
   RPC_SUCCESS             Successful completion

\********************************************************************/
{
INT i;

  if (hConn == -1)
    {
    /* close all open connections */
    for (i=MAX_RPC_CONNECTION-1 ; i >= 0; i--)
      if (_client_connection[i].send_sock != 0)
        rpc_client_disconnect(i+1, FALSE);
    }
  else
    {
    /* notify server about exit */
    rpc_client_call(hConn, bShutdown ? RPC_ID_SHUTDOWN : RPC_ID_EXIT);

    /* close socket */
    if (_client_connection[hConn-1].send_sock)
      closesocket(_client_connection[hConn-1].send_sock);

    memset(&_client_connection[hConn-1], 0, sizeof(RPC_CLIENT_CONNECTION));
    }

  return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

INT rpc_server_disconnect()
/********************************************************************\

  Routine: rpc_server_disconnect

  Purpose: Close a rpc connection to a MIDAS server

  Input:
    none

  Output:
    none

  Function value:
   RPC_SUCCESS             Successful completion
   RPC_NET_ERROR           Error in socket call
   RPC_NO_CONNECTION       Maximum number of connections reached

\********************************************************************/
{
  /* flush remaining events */
  rpc_flush_event();

  /* notify server about exit */
  rpc_call(RPC_ID_EXIT);

  /* close sockets */
  closesocket(_server_connection.send_sock);
  closesocket(_server_connection.recv_sock);
  closesocket(_server_connection.event_sock);

  memset(&_server_connection, 0, sizeof(RPC_SERVER_CONNECTION));

  return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

INT rpc_is_remote(void)
/********************************************************************\

  Routine: rpc_is_remote

  Purpose: Return true if program is connected to a remote server

  Input:
   none

  Output:
    none

  Function value:
    INT    RPC connection index

\********************************************************************/
{
  return _server_connection.send_sock != 0;
}

/*------------------------------------------------------------------*/

INT rpc_get_server_acception(void)
/********************************************************************\

  Routine: rpc_get_server_acception

  Purpose: Return actual RPC server connection index

  Input:
   none

  Output:
    none

  Function value:
    INT    RPC server connection index

\********************************************************************/
{
  return _server_acception_index;
}

/*------------------------------------------------------------------*/

INT rpc_set_server_acception(INT index)
/********************************************************************\

  Routine: rpc_set_server_acception

  Purpose: Set actual RPC server connection index

  Input:
    INT  index              Server index

  Output:
    none

  Function value:
    RPC_SUCCESS             Successful completion

\********************************************************************/
{
  _server_acception_index = index;
  return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

INT rpc_get_option(HNDLE hConn, INT item)
/********************************************************************\

  Routine: rpc_get_option

  Purpose: Get actual RPC option

  Input:
    HNDLE hConn             RPC connection handle  
    INT   item              One of RPC_Oxxx

  Output:
    none

  Function value:
    INT                     Actual option

\********************************************************************/
{
  switch(item)
    {
    case RPC_OTIMEOUT:
      if (hConn == -1)
        return _server_connection.rpc_timeout;
      return _client_connection[hConn-1].rpc_timeout;

    case RPC_OTRANSPORT:
      if (hConn == -1)
        return _server_connection.transport;
      return _client_connection[hConn-1].transport;

    case RPC_OHW_TYPE:
      {
      INT    tmp_type, size;
      DWORD  dummy;
      unsigned char *p;
      float  f;
      double d;

      tmp_type = 0;

      /* test pointer size */
      size = sizeof(p);
      if (size == 2)
        tmp_type |= DRI_16;
      if (size == 4)
        tmp_type |= DRI_32;
      if (size == 8)
        tmp_type |= DRI_64;

      /* test if little or big endian machine */
      dummy = 0x12345678;
      p = (unsigned char *) &dummy;
      if ( *p == 0x78)
        tmp_type |= DRI_LITTLE_ENDIAN;
      else if ( *p == 0x12)
        tmp_type |= DRI_BIG_ENDIAN;
      else
        cm_msg(MERROR, "rpc_get_option", "unknown byte order format");

      /* floating point format */
      f = (float) 1.2345;
      dummy = 0;
      memcpy(&dummy, &f, sizeof(f));
      if (( dummy & 0xFF )       == 0x19 &&
          ((dummy >> 8) & 0xFF)  == 0x04 &&
          ((dummy >> 16) & 0xFF) == 0x9E &&
          ((dummy >> 24) & 0xFF) == 0x3F)
        tmp_type |= DRF_IEEE;
      else
      if (( dummy & 0xFF )       == 0x9E &&
          ((dummy >> 8) & 0xFF)  == 0x40 &&
          ((dummy >> 16) & 0xFF) == 0x19 &&
          ((dummy >> 24) & 0xFF) == 0x04)
        tmp_type |= DRF_G_FLOAT;
      else
        cm_msg(MERROR, "rpc_get_option", "unknown floating point format");

      d = (double) 1.2345;
      dummy = 0;
      memcpy(&dummy, &d, sizeof(f));
      if (( dummy & 0xFF )       == 0x8D &&  /* little endian */
          ((dummy >> 8) & 0xFF)  == 0x97 &&
          ((dummy >> 16) & 0xFF) == 0x6E &&
          ((dummy >> 24) & 0xFF) == 0x12)
        tmp_type |= DRF_IEEE;
      else
      if (( dummy & 0xFF )       == 0x83 &&  /* big endian */
          ((dummy >> 8) & 0xFF)  == 0xC0 &&
          ((dummy >> 16) & 0xFF) == 0xF3 &&
          ((dummy >> 24) & 0xFF) == 0x3F)
        tmp_type |= DRF_IEEE;
      else
      if (( dummy & 0xFF )       == 0x13 &&
          ((dummy >> 8) & 0xFF)  == 0x40 &&
          ((dummy >> 16) & 0xFF) == 0x83 &&
          ((dummy >> 24) & 0xFF) == 0xC0)
        tmp_type |= DRF_G_FLOAT;
      else if (( dummy & 0xFF )  == 0x9E &&
          ((dummy >> 8) & 0xFF)  == 0x40 &&
          ((dummy >> 16) & 0xFF) == 0x18 &&
          ((dummy >> 24) & 0xFF) == 0x04)
        cm_msg(MERROR, "rpc_get_option", "MIDAS cannot handle VAX D FLOAT format. Please compile with the /g_float flag");
      else
        cm_msg(MERROR, "rpc_get_option", "unknown floating point format");

      return tmp_type;
      }

    default: 
      cm_msg(MERROR, "rpc_get_option", "invalid argument");
      break;
    }

  return 0;
}

/*------------------------------------------------------------------*/

INT rpc_set_option(HNDLE hConn, INT item, INT value)
/********************************************************************\

  Routine: rpc_set_option

  Purpose: Set RPC option

  Input:
   HNDLE hConn              RPC connection handle
   INT   item               One of RPC_Oxxx
   INT   value              Value to set

  Output:
    none

  Function value:
    RPC_SUCCESS             Successful completion

\********************************************************************/
{
  switch(item)
    {
    case RPC_OTIMEOUT:
      if (hConn == -1)
        _server_connection.rpc_timeout = value;
      else
        _client_connection[hConn-1].rpc_timeout = value;
      break;

    case RPC_OTRANSPORT:
      if (hConn == -1)
        _server_connection.transport = value;
      else
        _client_connection[hConn-1].transport = value;
      break;

    default: 
      cm_msg(MERROR, "rpc_set_option", "invalid argument");
      break;
    }

  return 0;
}

/*------------------------------------------------------------------*/

PTYPE rpc_get_server_option(INT item)
/********************************************************************\

  Routine: rpc_get_server_option

  Purpose: Get actual RPC option for server connection

  Input:
    INT  item               One of RPC_Oxxx

  Output:
    none

  Function value:
    INT                     Actual option

\********************************************************************/
{
INT i;

  if (item == RPC_OSERVER_TYPE)
    return _server_type;

  if (item == RPC_OSERVER_NAME)
    return (PTYPE) _server_name;

  /* return 0 for local calls */
  if (_server_type == ST_NONE)
    return 0;

  /* check which connections belongs to caller */
  if (_server_type == ST_MTHREAD)
    {
    for (i=0 ; i<MAX_RPC_CONNECTION ; i++)
      if (_server_acception[i].tid == ss_gettid())
        break;
    }
  else if (_server_type == ST_SINGLE || _server_type == ST_REMOTE)
    i = max(0,_server_acception_index - 1);
  else
    i = 0;
    
  switch(item)
    {
    case RPC_CONVERT_FLAGS:
      return _server_acception[i].convert_flags;
    case RPC_ODB_HANDLE:
      return _server_acception[i].odb_handle;
    case RPC_CLIENT_HANDLE:
      return _server_acception[i].client_handle;
    case RPC_SEND_SOCK:
      return _server_acception[i].send_sock;
    case RPC_WATCHDOG_TIMEOUT:
      return _server_acception[i].watchdog_timeout;
    }

  return 0;
}

/*------------------------------------------------------------------*/

INT rpc_set_server_option(INT item, PTYPE value)
/********************************************************************\

  Routine: rpc_set_server_option

  Purpose: Set RPC option for server connection

  Input:
   INT  item               One of RPC_Oxxx
   INT  value              Value to set

  Output:
    none

  Function value:
    RPC_SUCCESS             Successful completion

\********************************************************************/
{
INT i;

  if (item == RPC_OSERVER_TYPE)
    {
    _server_type = value;
    return RPC_SUCCESS;
    }
  if (item == RPC_OSERVER_NAME)
    {
    strcpy(_server_name, (char *) value);
    return RPC_SUCCESS;
    }

  /* check which connections belongs to caller */
  if (_server_type == ST_MTHREAD)
    {
    for (i=0 ; i<MAX_RPC_CONNECTION ; i++)
      if (_server_acception[i].tid == ss_gettid())
        break;
    }
  else if (_server_type == ST_SINGLE || _server_type == ST_REMOTE)
    i = max(0,_server_acception_index - 1);
  else
    i = 0;

  switch(item)
    {
    case RPC_CONVERT_FLAGS:
      _server_acception[i].convert_flags = value;
      break;
    case RPC_ODB_HANDLE:
      _server_acception[i].odb_handle = value;
      break;
    case RPC_CLIENT_HANDLE:
      _server_acception[i].client_handle = value;
      break;
    case RPC_WATCHDOG_TIMEOUT:
      _server_acception[i].watchdog_timeout = value;
      break;
    }

  return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

INT rpc_get_name(char *name)
/********************************************************************\

  Routine: rpc_get_name

  Purpose: Get name set by rpc_set_name

  Input:
    none

  Output:
    char*  name             The location pointed by *name receives a
                            copy of the _prog_name

  Function value:
    RPC_SUCCESS             Successful completion

\********************************************************************/
{
  strcpy(name, _client_name);

  return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

INT rpc_set_name(char *name)
/********************************************************************\

  Routine: rpc_set_name

  Purpose: Set name of actual program for further rpc connections

  Input:
   char *name               Program name, up to NAME_LENGTH chars, 
                            no blanks

  Output:
    none

  Function value:
    RPC_SUCCESS             Successful completion

\********************************************************************/
{
  strcpy(_client_name, name);

  return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

INT rpc_set_debug(void (*func)(char*), INT mode)
/********************************************************************\

  Routine: rpc_set_debug

  Purpose: Set a function which is called on every RPC call to 
           display the function name and parameters of the RPC
           call.

  Input:
   void *func(char*)        Pointer to function.
   INT  mode                Debug mode

  Output:
    none

  Function value:
    RPC_SUCCESS             Successful completion

\********************************************************************/
{
  _debug_print = func;
  _debug_mode  = mode;
  return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

void rpc_va_arg(va_list* arg_ptr, INT arg_type, void *arg)
{
  switch(arg_type)
    {
    /* On the stack, the minimum parameter size is sizeof(int).
       To avoid problems on little endian systems, treat all
       smaller parameters as int's */
    case TID_BYTE:
    case TID_SBYTE:
    case TID_CHAR:
    case TID_WORD:
    case TID_SHORT:    *((int *) arg) = va_arg(*arg_ptr, int); break;

    case TID_INT:
    case TID_BOOL:     *((INT *) arg) = va_arg(*arg_ptr, INT); break;

    case TID_DWORD:    *((DWORD *) arg) = va_arg(*arg_ptr, DWORD); break;

    case TID_FLOAT:    *((float *) arg) = va_arg(*arg_ptr, float); break;

    case TID_DOUBLE:   *((double *) arg) = va_arg(*arg_ptr, double); break;

    case TID_ARRAY:    *((char**) arg) = va_arg(*arg_ptr, char *); break;
    }
}

/*------------------------------------------------------------------*/

INT rpc_client_call(HNDLE hConn, const INT routine_id, ...)
/********************************************************************\

  Routine: rpc_client_call

  Purpose: Call a function on a MIDAS client

  Input:
    INT  hConn              Client connection
    INT  routine_id         routine ID as defined in RPC.H (RPC_xxx)

    ...                     variable argument list

  Output:
    (depends on argument list)

  Function value:
    RPC_SUCCESS             Successful completion
    RPC_NET_ERROR           Error in socket call
    RPC_NO_CONNECTION       No active connection
    RPC_TIMEOUT             Timeout in RPC call
    RPC_INVALID_ID          Invalid routine_id (not in rpc_list)
    RPC_EXCEED_BUFFER       Paramters don't fit in network buffer

\********************************************************************/
{
va_list         ap, aptmp;                         
char            arg[8], arg_tmp[8];
INT             arg_type, transport, rpc_timeout;
INT             i, index, status, rpc_index;
INT             param_size, arg_size, send_size;
INT             tid, flags;
fd_set          readfds;
struct timeval  timeout;
char            *param_ptr, str[80];
BOOL            bpointer, bbig;
NET_COMMAND     *nc;
int             send_sock;
  
  index = hConn-1;

  if (_client_connection[index].send_sock == 0)
    {
    cm_msg(MERROR, "rpc_client_call", "no rpc connection");
    return RPC_NO_CONNECTION;
    }

  send_sock   = _client_connection[index].send_sock;
  rpc_timeout = _client_connection[index].rpc_timeout;
  transport   = _client_connection[index].transport;

  /* init network buffer */
  if (_net_send_buffer_size == 0)
    {
    _net_send_buffer = MALLOC(NET_BUFFER_SIZE);
    if (_net_send_buffer == NULL)
      {
      cm_msg(MERROR, "rpc_call", "not enough memory to allocate network buffer");
      return RPC_EXCEED_BUFFER;
      }
    _net_send_buffer_size = NET_BUFFER_SIZE;
    }

  nc = (NET_COMMAND *) _net_send_buffer;
  nc->header.routine_id = routine_id;

  if (transport == RPC_FTCP)
    nc->header.routine_id |= TCP_FAST;

  for (i=0 ;; i++)
    if (rpc_list[i].id == routine_id ||
        rpc_list[i].id == 0)
      break;
  rpc_index = i;
  if (rpc_list[i].id == 0)
    {
    sprintf(str, "invalid rpc ID (%ld)", routine_id);
    cm_msg(MERROR, "rpc_call", str);
    return RPC_INVALID_ID;
    }

  /* examine variable argument list and convert it to parameter array */
  va_start(ap, routine_id);

  /* find out if we are on a big endian system */
  bbig = ((rpc_get_option(0, RPC_OHW_TYPE) & DRI_BIG_ENDIAN) > 0);

  for (i=0, param_ptr=nc->param ; rpc_list[rpc_index].param[i].tid != 0; i++)
    {
    tid   = rpc_list[rpc_index].param[i].tid;
    flags = rpc_list[rpc_index].param[i].flags;

    bpointer = (flags & RPC_POINTER)  || (flags & RPC_OUT)      ||
	             (flags & RPC_FIXARRAY) || (flags & RPC_VARARRAY) ||
		            tid == TID_STRING     || tid == TID_ARRAY       ||
		            tid == TID_STRUCT     || tid == TID_LINK;

    if (bpointer)
      arg_type = TID_ARRAY;
    else
      arg_type = tid;

    /* floats are passed as doubles, at least under NT */
    if (tid == TID_FLOAT && !bpointer)
      arg_type = TID_DOUBLE;

    /* get pointer to argument */
    rpc_va_arg(&ap, arg_type, arg);

    /* shift 1- and 2-byte parameters to the LSB on big endian systems */
    if (bbig)
      {
      if (tid == TID_BYTE || tid == TID_CHAR || tid == TID_SBYTE)
        {
        arg[0] = arg[3];
        }
      if (tid == TID_WORD || tid == TID_SHORT)
        {
        arg[0] = arg[2];
        arg[1] = arg[3];
        }
      }
    
    if (flags & RPC_IN)
      {
      if (bpointer)
        arg_size = tid_size[tid];
      else
        arg_size = tid_size[arg_type];

      /* for strings, the argument size depends on the string length */
      if (tid == TID_STRING || tid == TID_LINK)
        arg_size = 1+strlen((char *) *((char **) arg));

      /* for varibale length arrays, the size is given by 
         the next parameter on the stack */
      if (flags & RPC_VARARRAY)
      	{
      	memcpy(&aptmp, &ap, sizeof(ap));
      	rpc_va_arg(&aptmp, TID_ARRAY, arg_tmp);
        
        if (flags & RPC_OUT)
          arg_size = *((INT *) *((void **) arg_tmp));
        else
          arg_size = *((INT *) arg_tmp);
      	
        *((INT *) param_ptr) = ALIGN(arg_size);
      	param_ptr += ALIGN( sizeof(INT) );
      	}

      if (tid == TID_STRUCT || (flags & RPC_FIXARRAY))
        arg_size = rpc_list[rpc_index].param[i].n;

      /* always align parameter size */
      param_size = ALIGN(arg_size);

      if ((PTYPE) param_ptr - (PTYPE) nc + param_size >
          NET_BUFFER_SIZE)
        {
        cm_msg(MERROR, "rpc_call", "parameters (%d) too large for network buffer (%d)",
               (PTYPE) param_ptr - (PTYPE) nc + param_size, NET_BUFFER_SIZE);
        return RPC_EXCEED_BUFFER;
        }

      if (bpointer)
        memcpy(param_ptr, (void *) *((void **) arg), arg_size);
      else
        {
        /* floats are passed as doubles on most systems */
        if (tid != TID_FLOAT)
          memcpy(param_ptr, arg, arg_size);
        else
          *((float *) param_ptr) = (float) *((double *) arg);
        }

      param_ptr += param_size;

      }
    }

  va_end(ap);

  nc->header.param_size = (PTYPE) param_ptr - (PTYPE) nc->param;

  send_size = nc->header.param_size + sizeof(NET_COMMAND_HEADER);

  /* in FAST TCP mode, only send call and return immediately */
  if (transport == RPC_FTCP)
    {
    i = send_tcp(send_sock, (char *) nc, send_size, 0);

    if (i != send_size)
      {
      cm_msg(MERROR, "rpc_call", "send_tcp() failed");
      return RPC_NET_ERROR;
      }

    return RPC_SUCCESS;
    }

  /* in TCP mode, send and wait for reply on send socket */
  i = send_tcp(send_sock, (char *) nc, send_size, 0);
  if (i != send_size)
    {
    cm_msg(MERROR, "rpc_call", "send_tcp() failed");
    return RPC_NET_ERROR;
    }

  /* make some timeout checking */
  if (rpc_timeout > 0)
    {
    FD_ZERO(&readfds);
    FD_SET(send_sock, &readfds);

    timeout.tv_sec  = rpc_timeout / 1000;
    timeout.tv_usec = (rpc_timeout % 1000) * 1000;

    do
      {
      status = select(FD_SETSIZE, (void *) &readfds, NULL, NULL, (void *) &timeout);

      /* if an alarm signal was cought, restart select with reduced timeout */
      if (status == -1 && timeout.tv_sec >= WATCHDOG_INTERVAL / 1000)
        timeout.tv_sec -= WATCHDOG_INTERVAL / 1000;

      } while (status == -1); /* dont return if an alarm signal was cought */

    if (!FD_ISSET(send_sock, &readfds))
      {
      cm_msg(MERROR, "rpc_call", "rpc timeout, routine = %d", routine_id);

      /* disconnect to avoid that the reply to this rpc_call comes at
         the next rpc_call */
      rpc_client_disconnect(hConn, FALSE);

      return RPC_TIMEOUT;
      }
    }

  /* receive result on send socket */
  i = recv_tcp(send_sock, _net_send_buffer, NET_BUFFER_SIZE, 0);

  if (i<=0)
    {
    cm_msg(MERROR, "rpc_call", "recv_tcp() failed, routine = %d", routine_id);
    return RPC_NET_ERROR;
    }

  /* extract result variables and place it to argument list */
  status = nc->header.routine_id;

  va_start(ap, routine_id);

  for (i=0, param_ptr=nc->param ; rpc_list[rpc_index].param[i].tid != 0; i++)
    {
    tid   = rpc_list[rpc_index].param[i].tid;
    flags = rpc_list[rpc_index].param[i].flags;

    bpointer = (flags & RPC_POINTER)  || (flags & RPC_OUT)      ||
               (flags & RPC_FIXARRAY) || (flags & RPC_VARARRAY) ||
                tid == TID_STRING     || tid == TID_ARRAY       ||
                tid == TID_STRUCT     || tid == TID_LINK;

    if (bpointer)
      arg_type = TID_ARRAY;
    else
      arg_type = rpc_list[rpc_index].param[i].tid;

    if (tid == TID_FLOAT && !bpointer)
      arg_type = TID_DOUBLE;

    rpc_va_arg(&ap, arg_type, arg);

    if (rpc_list[rpc_index].param[i].flags & RPC_OUT)
      {
      tid   = rpc_list[rpc_index].param[i].tid;
      flags = rpc_list[rpc_index].param[i].flags;

      arg_size = tid_size[tid];

      if (tid == TID_STRING || tid == TID_LINK)
        arg_size = strlen((char *) (param_ptr)) + 1;

      if (flags & RPC_VARARRAY)
        {
      	arg_size = *((INT *) param_ptr);
      	param_ptr += ALIGN( sizeof(INT) );
      	}

      if (tid == TID_STRUCT || (flags & RPC_FIXARRAY))
        arg_size = rpc_list[rpc_index].param[i].n;

      /* return parameters are always pointers */
      if (*((char **) arg))
        memcpy((void *) *((char **) arg), param_ptr, arg_size);

      /* parameter size is always aligned */
      param_size = ALIGN(arg_size);

      param_ptr += param_size;
      }
    }

  va_end(ap);

  return status;
}

/*------------------------------------------------------------------*/

INT rpc_call(const INT routine_id, ...)
/********************************************************************\

  Routine: rpc_call

  Purpose: Call a function on a MIDAS server

  Input:
    INT  routine_id         routine ID as defined in RPC.H (RPC_xxx)

    ...                     variable argument list

  Output:
    (depends on argument list)

  Function value:
    RPC_SUCCESS             Successful completion
    RPC_NET_ERROR           Error in socket call
    RPC_NO_CONNECTION       No active connection
    RPC_TIMEOUT             Timeout in RPC call
    RPC_INVALID_ID          Invalid routine_id (not in rpc_list)
    RPC_EXCEED_BUFFER       Paramters don't fit in network buffer

\********************************************************************/
{
va_list         ap, aptmp;                         
char            arg[8], arg_tmp[8];
INT             arg_type, transport, rpc_timeout;
INT             i, index, status;
INT             param_size, arg_size, send_size;
INT             tid, flags;
fd_set          readfds;
struct timeval  timeout;
char            *param_ptr, str[80];
BOOL            bpointer, bbig;
NET_COMMAND     *nc;
int             send_sock;

  send_sock   = _server_connection.send_sock;
  transport   = _server_connection.transport;
  rpc_timeout = _server_connection.rpc_timeout;

  /* init network buffer */
  if (_net_send_buffer_size == 0)
    {
    _net_send_buffer = MALLOC(NET_BUFFER_SIZE);
    if (_net_send_buffer == NULL)
      {
      cm_msg(MERROR, "rpc_call", "not enough memory to allocate network buffer");
      return RPC_EXCEED_BUFFER;
      }
    _net_send_buffer_size = NET_BUFFER_SIZE;
    }

  nc = (NET_COMMAND *) _net_send_buffer;
  nc->header.routine_id = routine_id;

  if (transport == RPC_FTCP)
    nc->header.routine_id |= TCP_FAST;

  for (i=0 ;; i++)
    if (rpc_list[i].id == routine_id ||
        rpc_list[i].id == 0)
      break;
  index = i;
  if (rpc_list[i].id == 0)
    {
    sprintf(str, "invalid rpc ID (%ld)", routine_id);
    cm_msg(MERROR, "rpc_call", str);
    return RPC_INVALID_ID;
    }

  /* examine variable argument list and convert it to parameter array */
  va_start(ap, routine_id);

  /* find out if we are on a big endian system */
  bbig = ((rpc_get_option(0, RPC_OHW_TYPE) & DRI_BIG_ENDIAN) > 0);

  for (i=0, param_ptr=nc->param ; rpc_list[index].param[i].tid != 0; i++)
    {
    tid   = rpc_list[index].param[i].tid;
    flags = rpc_list[index].param[i].flags;

    bpointer = (flags & RPC_POINTER)  || (flags & RPC_OUT)      ||
	             (flags & RPC_FIXARRAY) || (flags & RPC_VARARRAY) ||
		            tid == TID_STRING     || tid == TID_ARRAY       ||
		            tid == TID_STRUCT     || tid == TID_LINK;

    if (bpointer)
      arg_type = TID_ARRAY;
    else
      arg_type = tid;

    /* floats are passed as doubles, at least under NT */
    if (tid == TID_FLOAT && !bpointer)
      arg_type = TID_DOUBLE;

    /* get pointer to argument */
    rpc_va_arg(&ap, arg_type, arg);

    /* shift 1- and 2-byte parameters to the LSB on big endian systems */
    if (bbig)
      {
      if (tid == TID_BYTE || tid == TID_CHAR || tid == TID_SBYTE)
        {
        arg[0] = arg[3];
        }
      if (tid == TID_WORD || tid == TID_SHORT)
        {
        arg[0] = arg[2];
        arg[1] = arg[3];
        }
      }
    
    if (flags & RPC_IN)
      {
      if (bpointer)
        arg_size = tid_size[tid];
      else
        arg_size = tid_size[arg_type];

      /* for strings, the argument size depends on the string length */
      if (tid == TID_STRING || tid == TID_LINK)
        arg_size = 1+strlen((char *) *((char **) arg));

      /* for varibale length arrays, the size is given by 
         the next parameter on the stack */
      if (flags & RPC_VARARRAY)
      	{
      	memcpy(&aptmp, &ap, sizeof(ap));
      	rpc_va_arg(&aptmp, TID_ARRAY, arg_tmp);
        
        if (flags & RPC_OUT)
          arg_size = *((INT *) *((void **) arg_tmp));
        else
          arg_size = *((INT *) arg_tmp);
      	
        *((INT *) param_ptr) = ALIGN(arg_size);
      	param_ptr += ALIGN( sizeof(INT) );
      	}

      if (tid == TID_STRUCT || (flags & RPC_FIXARRAY))
        arg_size = rpc_list[index].param[i].n;

      /* always align parameter size */
      param_size = ALIGN(arg_size);

      if ((PTYPE) param_ptr - (PTYPE) nc + param_size >
          NET_BUFFER_SIZE)
        {
        cm_msg(MERROR, "rpc_call", "parameters (%d) too large for network buffer (%d)",
               (PTYPE) param_ptr - (PTYPE) nc + param_size, NET_BUFFER_SIZE);
        return RPC_EXCEED_BUFFER;
        }

      if (bpointer)
        memcpy(param_ptr, (void *) *((void **) arg), arg_size);
      else
        {
        /* floats are passed as doubles on most systems */
        if (tid != TID_FLOAT)
          memcpy(param_ptr, arg, arg_size);
        else
          *((float *) param_ptr) = (float) *((double *) arg);
        }

      param_ptr += param_size;

      }
    }

  va_end(ap);

  nc->header.param_size = (PTYPE) param_ptr - (PTYPE) nc->param;

  send_size = nc->header.param_size + sizeof(NET_COMMAND_HEADER);

  /* in FAST TCP mode, only send call and return immediately */
  if (transport == RPC_FTCP)
    {
    i = send_tcp(send_sock, (char *) nc, send_size, 0);

    if (i != send_size)
      {
      cm_msg(MERROR, "rpc_call", "send_tcp() failed");
      return RPC_NET_ERROR;
      }

    return RPC_SUCCESS;
    }

  /* in TCP mode, send and wait for reply on send socket */
  i = send_tcp(send_sock, (char *) nc, send_size, 0);
  if (i != send_size)
    {
    cm_msg(MERROR, "rpc_call", "send_tcp() failed");
    return RPC_NET_ERROR;
    }

  /* make some timeout checking */
  if (rpc_timeout > 0)
    {
    FD_ZERO(&readfds);
    FD_SET(send_sock, &readfds);

    timeout.tv_sec  = rpc_timeout / 1000;
    timeout.tv_usec = (rpc_timeout % 1000) * 1000;

    do
      {
      status = select(FD_SETSIZE, (void *) &readfds, NULL, NULL, (void *) &timeout);

      /* if an alarm signal was cought, restart select with reduced timeout */
      if (status == -1 && timeout.tv_sec >= WATCHDOG_INTERVAL / 1000)
        timeout.tv_sec -= WATCHDOG_INTERVAL / 1000;

      } while (status == -1); /* dont return if an alarm signal was cought */

    if (!FD_ISSET(send_sock, &readfds))
      {
      cm_msg(MERROR, "rpc_call", "rpc timeout, routine = %d", routine_id);

      /* disconnect to avoid that the reply to this rpc_call comes at
         the next rpc_call */
      rpc_server_disconnect();

      return RPC_TIMEOUT;
      }
    }

  /* receive result on send socket */
  i = recv_tcp(send_sock, _net_send_buffer, NET_BUFFER_SIZE, 0);

  if (i<=0)
    {
    cm_msg(MERROR, "rpc_call", "recv_tcp() failed, routine = %d", routine_id);
    return RPC_NET_ERROR;
    }

  /* extract result variables and place it to argument list */
  status = nc->header.routine_id;

  va_start(ap, routine_id);

  for (i=0, param_ptr=nc->param ; rpc_list[index].param[i].tid != 0; i++)
    {
    tid   = rpc_list[index].param[i].tid;
    flags = rpc_list[index].param[i].flags;

    bpointer = (flags & RPC_POINTER)  || (flags & RPC_OUT)      ||
               (flags & RPC_FIXARRAY) || (flags & RPC_VARARRAY) ||
                tid == TID_STRING     || tid == TID_ARRAY       ||
                tid == TID_STRUCT     || tid == TID_LINK;

    if (bpointer)
      arg_type = TID_ARRAY;
    else
      arg_type = rpc_list[index].param[i].tid;

    if (tid == TID_FLOAT && !bpointer)
      arg_type = TID_DOUBLE;

    rpc_va_arg(&ap, arg_type, arg);

    if (rpc_list[index].param[i].flags & RPC_OUT)
      {
      tid = rpc_list[index].param[i].tid;
      arg_size = tid_size[tid];

      if (tid == TID_STRING || tid == TID_LINK)
        arg_size = strlen((char *) (param_ptr)) + 1;

      if (flags & RPC_VARARRAY)
        {
      	arg_size = *((INT *) param_ptr);
      	param_ptr += ALIGN( sizeof(INT) );
      	}

      if (tid == TID_STRUCT || (flags & RPC_FIXARRAY))
        arg_size = rpc_list[index].param[i].n;

      /* return parameters are always pointers */
      if (*((char **) arg))
        memcpy((void *) *((char **) arg), param_ptr, arg_size);

      /* parameter size is always aligned */
      param_size = ALIGN(arg_size);

      param_ptr += param_size;
      }
    }

  va_end(ap);

  return status;
}

/*------------------------------------------------------------------*/

INT rpc_set_opt_tcp_size(INT tcp_size)
{
INT old;

  old = _opt_tcp_size;
  _opt_tcp_size = tcp_size;
  return old;
}

INT rpc_get_opt_tcp_size()
{
  return _opt_tcp_size;
}

INT rpc_send_event(INT buffer_handle, void *source, INT buf_size,
                   INT async_flag)
/********************************************************************\

  Routine: rpc_send_event

  Purpose: Fast send_event routine which bypasses the RPC layer and
           sends the event directly at the TCP level.

  Input:
    INT  buffer_handle      Handle of the buffer to send the event to.
                            Must be obtained via bm_open_buffer.
    void *source            Address of the event to send. It must have
                            a proper event header.
    INT  buf_size           Size of event in bytes with header.
    INT  async_flag         SYNC / ASYNC flag. In ASYNC mode, the
                            function returns immediately if it cannot
                            send the event over the network. In SYNC
                            mode, it waits until the packet is sent
                            (blocking).

  Output:
    none

  Function value:
    BM_INVALID_PARAM        Invalid event size parameter
    BM_ASYNC_RETURN         Function cannot send data
    RPC_SUCCESS             Successful completion
    RPC_NET_ERROR           Error in socket call
    RPC_NO_CONNECTION       No active connection
    RPC_EXCEED_BUFFER       Paramters don't fit in network buffer

\********************************************************************/
{
INT             i;
NET_COMMAND     *nc;
unsigned long   flag;
BOOL            would_block;
DWORD           aligned_buf_size;

  aligned_buf_size = ALIGN(buf_size);

  if (aligned_buf_size != (INT) ALIGN(((EVENT_HEADER *) source)->data_size + sizeof(EVENT_HEADER)))
    {
    cm_msg(MERROR, "rpc_send_event", "event size mismatch");
    return BM_INVALID_PARAM;
    }
  if (((EVENT_HEADER *) source)->data_size > MAX_EVENT_SIZE)
    {
    cm_msg(MERROR, "rpc_send_event", "event size (%d) larger than maximum event size (%d)", 
      ((EVENT_HEADER *) source)->data_size, MAX_EVENT_SIZE);
    return RPC_EXCEED_BUFFER;
    }

  if (!rpc_is_remote())
    return bm_send_event(buffer_handle, source, buf_size, async_flag);

  /* init network buffer */
  if (!_tcp_buffer)
    _tcp_buffer = MALLOC(NET_TCP_SIZE);
  if (!_tcp_buffer)
    {
    cm_msg(MERROR, "rpc_send_event", "not enough memory to allocate network buffer");
    return RPC_EXCEED_BUFFER;
    }

  /* check if not enough space in TCP buffer */
  if (aligned_buf_size + 4*8 + sizeof(NET_COMMAND_HEADER) >=
      (DWORD) (_opt_tcp_size - _tcp_wp) && _tcp_wp != _tcp_rp)
    {
    /* set socket to nonblocking IO */
    if (async_flag == ASYNC)
      {
      flag = 1;
#ifdef OS_VXWORKS
      ioctlsocket(_server_connection.send_sock, FIONBIO, (int) &flag);
#else
      ioctlsocket(_server_connection.send_sock, FIONBIO, &flag);
#endif
      }

    i = send_tcp(_server_connection.send_sock,
                 _tcp_buffer + _tcp_rp, _tcp_wp - _tcp_rp, 0);

    if (i < 0)
#ifdef OS_WINNT
      would_block = (WSAGetLastError() == WSAEWOULDBLOCK);
#else
      would_block = (errno == EWOULDBLOCK);
#endif

    /* set socket back to blocking IO */
    if (async_flag == ASYNC)
      {
      flag = 0;
#ifdef OS_VXWORKS
      ioctlsocket(_server_connection.send_sock, FIONBIO, (int) &flag);
#else
      ioctlsocket(_server_connection.send_sock, FIONBIO, &flag);
#endif
      }

    /* increment read pointer */
    if (i > 0)
      _tcp_rp += i;

    /* check if whole buffer is sent */
    if (_tcp_rp == _tcp_wp)
      _tcp_rp = _tcp_wp = 0;

    if (i < 0 && !would_block)
      {
      printf("send_tcp() returned %d\n", i);
      cm_msg(MERROR, "rpc_send_event", "send_tcp() failed");
      return RPC_NET_ERROR;
      }

    /* return if buffer is not emptied */
    if (_tcp_wp > 0)
      return BM_ASYNC_RETURN;
    }

  nc = (NET_COMMAND *) (_tcp_buffer + _tcp_wp);
  nc->header.routine_id = RPC_BM_SEND_EVENT | TCP_FAST;
  nc->header.param_size = 4*8 + aligned_buf_size;

  /* assemble parameters manually */
  *((INT *) (&nc->param[0])) = buffer_handle;
  *((INT *) (&nc->param[8])) = buf_size;

  /* send events larger than optimal buffer size directly */
  if (aligned_buf_size + 4*8 + sizeof(NET_COMMAND_HEADER) >= (DWORD) _opt_tcp_size)
    {
    /* send header */
    send_tcp(_server_connection.send_sock,
             _tcp_buffer + _tcp_wp, sizeof(NET_COMMAND_HEADER) + 16, 0);

    /* send data */
    send_tcp(_server_connection.send_sock, source, aligned_buf_size, 0);

    /* send last two parameters */
    *((INT *) (&nc->param[0])) = buf_size;
    *((INT *) (&nc->param[8])) = 0;
    send_tcp(_server_connection.send_sock, &nc->param[0], 16, 0);
    }
  else
    {
    /* copy event */
    memcpy(&nc->param[16], source, buf_size);

    /* last two parameters (buf_size and async_flag */
    *((INT *) (&nc->param[16+aligned_buf_size])) = buf_size;
    *((INT *) (&nc->param[24+aligned_buf_size])) = 0;

    _tcp_wp += nc->header.param_size + sizeof(NET_COMMAND_HEADER);
    }

  return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

int rpc_get_send_sock()
/********************************************************************\

  Routine: rpc_get_send_sock

  Purpose: Return send socket to MIDAS server. Used by MFE.C for
           optimized event sending.

  Input:
    none

  Output:
    none

  Function value:
    int    socket

\********************************************************************/
{
  return _server_connection.send_sock;
}

/*------------------------------------------------------------------*/

int rpc_get_event_sock()
/********************************************************************\

  Routine: rpc_get_event_sock

  Purpose: Return event send socket to MIDAS server. Used by MFE.C for
           optimized event sending.

  Input:
    none

  Output:
    none

  Function value:
    int    socket

\********************************************************************/
{
  return _server_connection.event_sock;
}

/*------------------------------------------------------------------*/

INT rpc_flush_event()
/********************************************************************\

  Routine: rpc_flush_event

  Purpose: Send event residing in the TCP cache buffer filled by
           rpc_send_event. This routine should be called when a
           run is stopped.

  Input:
    none

  Output:
    none

  Function value:
    RPC_SUCCESS             Successful completion
    RPC_NET_ERROR           Error in socket call

\********************************************************************/
{
INT i;

  if (!rpc_is_remote())
    return RPC_SUCCESS;

  /* return if rpc_send_event was not called */
  if (!_tcp_buffer || _tcp_wp == 0)
    return RPC_SUCCESS;

  /* empty TCP buffer */
  if (_tcp_wp > 0)
    {
    i = send_tcp(_server_connection.send_sock, 
                 _tcp_buffer + _tcp_rp, _tcp_wp - _tcp_rp, 0);

    if (i != _tcp_wp - _tcp_rp)
      {
      cm_msg(MERROR, "rpc_flush_event", "send_tcp() failed");
      return RPC_NET_ERROR;
      }
    }

  _tcp_rp = _tcp_wp = 0;

  return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

static INT rpc_transition_dispatch(INT index, void *prpc_param[])
/********************************************************************\

  Routine: rpc_transition_dispatch

  Purpose: Gets called when a transition function was registered and
           a transition occured. Internal use only.

  Input:
    INT    index            RPC function ID
    void   *prpc_param      RPC parameters

  Output:
    none

  Function value:
    INT    return value from called user routine

\********************************************************************/
{
INT status, i;

  if (index == RPC_RC_TRANSITION)
    {
    for (i=0 ; _trans_table[i].transition ; i++)
      if (_trans_table[i].transition == CINT(0))
        break;

    /* erase error string */
    *(CSTRING(2)) = 0;

    /* call registerd function */
    if (_trans_table[i].transition == CINT(0) &&
        _trans_table[i].func)
      status = _trans_table[i].func(CINT(1), CSTRING(2));
    else
      status = RPC_SUCCESS;
    }
  else
    {
    cm_msg(MERROR, "rpc_transition_dispatch", "received unrecognized command");
    status = RPC_INVALID_ID;
    }

  return status;
}

/*------------------------------------------------------------------*/

/********************************************************************\
*                        server functions                            *
\********************************************************************/


/*------------------------------------------------------------------*/

INT recv_tcp_server(INT index, char *buffer, DWORD buffer_size, INT flags, 
                    INT *remaining)
/********************************************************************\

  Routine: recv_tcp_server

  Purpose: TCP receive routine with local cache. To speed up network
           performance, a 64k buffer is read in at once and split into
           several RPC command on successive calls to recv_tcp_server.
           Therefore, the number of recv() calls is minimized.

           This routine is ment to be called by the server process.
           Clients should call recv_tcp instead.

  Input:
    INT   index              Index of server connection
    DWORD buffer_size        Size of the buffer in bytes.
    INT   flags              Flags passed to recv()
    INT   convert_flags      Convert flags needed for big/little
                             endian conversion

  Output:
    char  *buffer            Network receive buffer.
    INT   *remaining         Remaining data in cache

  Function value:
    INT                      Same as recv()

\********************************************************************/
{
INT         size, param_size;
NET_COMMAND *nc;
INT         write_ptr, read_ptr, misalign;
char        *net_buffer;
INT         copied, status;
INT         sock;

  sock = _server_acception[index].recv_sock;

  if (flags & MSG_PEEK)
    {
    status = recv(sock, buffer, buffer_size, flags);
    return status;
    }

  if (!_server_acception[index].net_buffer)
    {
    if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_REMOTE)
      _server_acception[index].net_buffer_size = NET_TCP_SIZE;
    else
      _server_acception[index].net_buffer_size = NET_BUFFER_SIZE;

    _server_acception[index].net_buffer = MALLOC(_server_acception[index].net_buffer_size);
    _server_acception[index].write_ptr = 0;
    _server_acception[index].read_ptr = 0;
    _server_acception[index].misalign = 0;
    }
  if (!_server_acception[index].net_buffer)
    {
    cm_msg(MERROR, "recv_tcp_server", "not enough memory to allocate network buffer");
    return -1;
    }

  if (buffer_size < sizeof(NET_COMMAND_HEADER))
    {
    cm_msg(MERROR, "recv_tcp_server", "parameters too large for network buffer");
    return -1;
    }

  copied = 0;
  param_size = -1;

  write_ptr  = _server_acception[index].write_ptr;
  read_ptr   = _server_acception[index].read_ptr;
  misalign   = _server_acception[index].misalign;
  net_buffer = _server_acception[index].net_buffer;
  
  do
    {
    if (write_ptr - read_ptr >= (INT) sizeof(NET_COMMAND_HEADER) - copied)
      {
      if (param_size == -1)
        {
        if (copied > 0)
          {
          /* assemble split header */
          memcpy(buffer + copied, net_buffer + read_ptr, 
                 (INT) sizeof(NET_COMMAND_HEADER) - copied);
          nc = (NET_COMMAND *) (buffer);
          }
        else
          nc = (NET_COMMAND *) (net_buffer + read_ptr);

        param_size = (INT) nc->header.param_size;

        if (_server_acception[index].convert_flags)
          rpc_convert_single(&param_size, TID_DWORD, 0, 
                             _server_acception[index].convert_flags);
        }

      /* check if parameters fit in buffer */
      if (buffer_size < param_size + sizeof(NET_COMMAND_HEADER))
        {
        cm_msg(MERROR, "recv_tcp_server", "parameters too large for network buffer");
        _server_acception[index].read_ptr = 
          _server_acception[index].write_ptr = 0;
        return -1;
        }

      /* check if we have all parameters in buffer */
      if (write_ptr - read_ptr >= 
            param_size + (INT) sizeof(NET_COMMAND_HEADER) - copied)
        break;
      }

    /* not enough data, so copy partially and get new */
    size = write_ptr - read_ptr;
    
    if (size > 0)
      {
      memcpy(buffer + copied, net_buffer + read_ptr, size);
      copied += size;
      read_ptr = write_ptr;
      }

#ifdef OS_UNIX
    do
      {
      write_ptr = recv(sock, net_buffer + misalign, _server_acception[index].net_buffer_size - 8, flags);

      /* don't return if an alarm signal was cought */
      } while (write_ptr == -1 && errno == EINTR); 
#else
    write_ptr = recv(sock, net_buffer + misalign, _server_acception[index].net_buffer_size - 8, flags);
#endif

    /* abort if connection broken */
    if (write_ptr <= 0)
      {
      if (remaining)
        *remaining = 0;

      return write_ptr;
      }

    read_ptr = misalign;
    write_ptr += misalign;

    misalign = write_ptr % 8;
    } while (TRUE);

  /* copy rest of parameters */
  size = param_size + sizeof(NET_COMMAND_HEADER) - copied;
  memcpy(buffer + copied, net_buffer + read_ptr, size);
  read_ptr += size;

  if (remaining)
    {
    /* don't keep rpc_server_receive in an infinite loop */
    if (write_ptr - read_ptr < param_size)
      *remaining = 0;
    else
      *remaining = write_ptr - read_ptr;
    }

  _server_acception[index].write_ptr = write_ptr;
  _server_acception[index].read_ptr  = read_ptr;
  _server_acception[index].misalign  = misalign;

  return size + copied;
}

/*------------------------------------------------------------------*/

INT recv_tcp_check(int sock)
/********************************************************************\

  Routine: recv_tcp_check

  Purpose: Check if in TCP receive buffer associated with sock is
           some data. Called by ss_suspend.

  Input:
    INT   sock               TCP receive socket

  Output:
    none

  Function value:
    INT   count              Number of bytes remaining in TCP buffer

\********************************************************************/
{
INT         index;

  /* figure out to which connection socket belongs */
  for (index=0 ; index<MAX_RPC_CONNECTION ; index++)
    if (_server_acception[index].recv_sock == sock)
      break;
  
  return _server_acception[index].write_ptr -
         _server_acception[index].read_ptr;
}

/*------------------------------------------------------------------*/

INT recv_event_server(INT index, char *buffer, DWORD buffer_size, INT flags, 
                      INT *remaining)
/********************************************************************\

  Routine: recv_event_server

  Purpose: TCP event receive routine with local cache. To speed up 
           network performance, a 64k buffer is read in at once and 
           split into several RPC command on successive calls to 
           recv_event_server. Therefore, the number of recv() calls 
           is minimized.

           This routine is ment to be called by the server process.
           Clients should call recv_tcp instead.

  Input:
    INT   index              Index of server connection
    DWORD buffer_size        Size of the buffer in bytes.
    INT   flags              Flags passed to recv()
    INT   convert_flags      Convert flags needed for big/little
                             endian conversion

  Output:
    char  *buffer            Network receive buffer.
    INT   *remaining         Remaining data in cache

  Function value:
    INT                      Same as recv()

\********************************************************************/
{
INT          size, event_size, aligned_event_size, *pbh, header_size;
EVENT_HEADER *pevent;
INT          write_ptr, read_ptr, misalign;
char         *net_buffer;
INT          copied, status;
INT          sock;
RPC_SERVER_ACCEPTION *psa;

  psa  = &_server_acception[index];
  sock = psa->event_sock;

  if (flags & MSG_PEEK)
    {
    status = recv(sock, buffer, buffer_size, flags);
    return status;
    }

  if (!psa->ev_net_buffer)
    {
    if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_REMOTE)
      psa->net_buffer_size = NET_TCP_SIZE;
    else
      psa->net_buffer_size = NET_BUFFER_SIZE;

    psa->ev_net_buffer = MALLOC(psa->net_buffer_size);
    psa->ev_write_ptr = 0;
    psa->ev_read_ptr = 0;
    psa->ev_misalign = 0;
    }
  if (!psa->ev_net_buffer)
    {
    cm_msg(MERROR, "recv_tcp_server", "not enough memory to allocate network buffer");
    return -1;
    }

  header_size = (INT) (sizeof(EVENT_HEADER) + sizeof(INT));

  if ((INT) buffer_size < header_size)
    {
    cm_msg(MERROR, "recv_tcp_server", "parameters too large for network buffer");
    return -1;
    }

  copied = 0;
  event_size = -1;

  write_ptr   = psa->ev_write_ptr;
  read_ptr    = psa->ev_read_ptr;
  misalign    = psa->ev_misalign;
  net_buffer  = psa->ev_net_buffer;
  
  do
    {
    if (write_ptr - read_ptr >= header_size - copied)
      {
      if (event_size == -1)
        {
        if (copied > 0)
          {
          /* assemble split header */
          memcpy(buffer + copied, net_buffer + read_ptr, header_size - copied);
          pbh = (INT *) buffer;
          }
        else
          pbh = (INT *) (net_buffer + read_ptr);

        pevent = (EVENT_HEADER *) (pbh+1);

        event_size = pevent->data_size;
        if (psa->convert_flags)
          rpc_convert_single(&event_size, TID_DWORD, 0, psa->convert_flags);

        aligned_event_size = ALIGN(event_size);
        }

      /* check if data part fits in buffer */
      if ((INT) buffer_size < aligned_event_size + header_size)
        {
        cm_msg(MERROR, "receive_event", "parameters too large for network buffer");
        psa->ev_read_ptr = psa->ev_write_ptr = 0;
        return -1;
        }

      /* check if we have whole event in buffer */
      if (write_ptr - read_ptr >= aligned_event_size + header_size - copied)
        break;
      }

    /* not enough data, so copy partially and get new */
    size = write_ptr - read_ptr;
    
    if (size > 0)
      {
      memcpy(buffer + copied, net_buffer + read_ptr, size);
      copied += size;
      read_ptr = write_ptr;
      }

#ifdef OS_UNIX
    do
      {
      write_ptr = recv(sock, net_buffer + misalign, psa->net_buffer_size - 8, flags);

      /* don't return if an alarm signal was cought */
      } while (write_ptr == -1 && errno == EINTR); 
#else
    write_ptr = recv(sock, net_buffer + misalign, psa->net_buffer_size - 8, flags);
#endif

    /* abort if connection broken */
    if (write_ptr <= 0)
      {
      if (remaining)
        *remaining = 0;

      return write_ptr;
      }

    read_ptr = misalign;
    write_ptr += misalign;

    misalign = write_ptr % 8;
    } while (TRUE);

  /* copy rest of event */
  size = aligned_event_size + header_size - copied;
  if (size > 0)
    {
    memcpy(buffer + copied, net_buffer + read_ptr, size);
    read_ptr += size;
    }

  if (remaining)
    *remaining = write_ptr - read_ptr;

  psa->ev_write_ptr = write_ptr;
  psa->ev_read_ptr  = read_ptr;
  psa->ev_misalign  = misalign;

  /* convert header little endian/big endian */
  if (psa->convert_flags)
    {
    pevent = (EVENT_HEADER *) (((INT *) buffer)+1);

    rpc_convert_single(buffer, TID_INT, 0, psa->convert_flags);
    rpc_convert_single(&pevent->event_id, TID_SHORT, 0, psa->convert_flags);
    rpc_convert_single(&pevent->trigger_mask, TID_SHORT, 0, psa->convert_flags);
    rpc_convert_single(&pevent->serial_number, TID_DWORD, 0, psa->convert_flags);
    rpc_convert_single(&pevent->time_stamp, TID_DWORD, 0, psa->convert_flags);
    rpc_convert_single(&pevent->data_size, TID_DWORD, 0, psa->convert_flags);
    }

  return header_size + event_size;
}

/*------------------------------------------------------------------*/

INT recv_event_check(int sock)
/********************************************************************\

  Routine: recv_event_check

  Purpose: Check if in TCP event receive buffer associated with sock 
           is some data. Called by ss_suspend.

  Input:
    INT   sock               TCP receive socket

  Output:
    none

  Function value:
    INT   count              Number of bytes remaining in TCP buffer

\********************************************************************/
{
INT         index;

  /* figure out to which connection socket belongs */
  for (index=0 ; index<MAX_RPC_CONNECTION ; index++)
    if (_server_acception[index].event_sock == sock)
      break;
  
  return _server_acception[index].ev_write_ptr -
         _server_acception[index].ev_read_ptr;
}

/*------------------------------------------------------------------*/

INT rpc_register_server(INT server_type, char *name, INT *port, 
                        INT (*func)(INT,void**))
/********************************************************************\

  Routine: rpc_register_server

  Purpose: Register the calling process as a MIDAS RPC server. Note
           that cm_connnect_experiment must be called prior to any call of
           rpc_register_server.

  Input:
    INT   server_type       One of the following constants:
                            ST_SINGLE: register a single process server
                            ST_MTHREAD: for each connection, start
                                        a new thread to serve it
                            ST_MPROCESS: for each connection, start
                                         a new process to server it
                            ST_SUBPROCESS: the routine was called from
                                           a multi process server
                            ST_REMOTE: register a client program server
                                       connected to the ODB
    char  *name             Name of .EXE file to start in MPROCESS mode
    INT   *port             TCP port for listen. NULL if listen as main
                            server (MIDAS_TCP_PORT is then used). If *port=0,
                            the OS chooses a free port and returns it. If
                            *port != 0, this port is used.
    INT   *func             Default dispatch function

  Output:
    INT   *port             Port under which server is listening.

  Function value:
    RPC_SUCCESS             Successful completion
    RPC_NET_ERROR           Error in socket call
    RPC_NOT_REGISTERED      cm_connect_experiment was not called

\********************************************************************/
{
struct sockaddr_in   bind_addr;
INT                  status, flag;
int                  size;
char                 host_name[HOST_NAME_LENGTH];
struct hostent       *phe;

#ifdef OS_WINNT
  {
  WSADATA WSAData;

  /* Start windows sockets */
  if ( WSAStartup(MAKEWORD(1,1), &WSAData) != 0)
    return RPC_NET_ERROR;
  }
#endif

  rpc_set_server_option(RPC_OSERVER_TYPE, server_type);

  /* register system functions */
  rpc_register_functions(rpc_get_internal_list(0), func);

  if (name != NULL)
    rpc_set_server_option(RPC_OSERVER_NAME, (PTYPE) name);

  /* in subprocess mode, don't start listener */
  if (server_type == ST_SUBPROCESS)
    return RPC_SUCCESS;

  /* create a socket for listening */
  _lsock = socket(AF_INET, SOCK_STREAM, 0);
  if (_lsock == -1)
    {
    cm_msg(MERROR, "rpc_register_server", "socket() failed");
    return RPC_NET_ERROR;
    }

  /* reuse address, needed if previous server stopped (30s timeout!) */
  flag = 1;
  status = setsockopt(_lsock, SOL_SOCKET, SO_REUSEADDR, 
                      (char *) &flag, sizeof(INT));
  if (status < 0)
    {
    cm_msg(MERROR, "rpc_register_server", "setsockopt() failed");
    return RPC_NET_ERROR;
    }

  /* bind local node name and port to socket */
  memset(&bind_addr, 0, sizeof(bind_addr));
  bind_addr.sin_family      = AF_INET;
  bind_addr.sin_addr.s_addr = 0;

  if (!port)
    bind_addr.sin_port        = htons(MIDAS_TCP_PORT);
  else
    bind_addr.sin_port        = htons((short) (*port));

  gethostname(host_name, sizeof(host_name));

#ifdef OS_VXWORKS
  {
  INT host_addr;

  host_addr = hostGetByName(host_name);
  memcpy((char *)&(bind_addr.sin_addr), &host_addr, 4);
  }
#else
  phe = gethostbyname(host_name);
  if (phe == NULL)
    {
    cm_msg(MERROR, "rpc_register_server", "cannot get host name");
    return RPC_NET_ERROR;
    }
  memcpy((char *)&(bind_addr.sin_addr), phe->h_addr, phe->h_length);
#endif

  status = bind(_lsock, (void *)&bind_addr, sizeof(bind_addr));
  if (status < 0)
    {
    cm_msg(MERROR, "rpc_register_server", "bind() failed");
    return RPC_NET_ERROR;
    }

  /* listen for connection */
#ifdef OS_MSDOS
  status = listen(_lsock, 1);
#else
  status = listen(_lsock, SOMAXCONN);
#endif
  if (status < 0)
    {
    cm_msg(MERROR, "rpc_register_server", "listen() failed");
    return RPC_NET_ERROR;
    }

  /* return port wich OS has choosen */
  if (port && *port == 0)
    {
    size = sizeof(bind_addr);
    getsockname(_lsock, (void *)&bind_addr, (void *)&size);
    *port = ntohs(bind_addr.sin_port);
    }

  /* define callbacks for ss_suspend */
  if (server_type == ST_REMOTE)
    ss_suspend_set_dispatch(CH_LISTEN, &_lsock, rpc_client_accept);
  else
    ss_suspend_set_dispatch(CH_LISTEN, &_lsock, rpc_server_accept);

  return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

INT rpc_execute(INT sock, char *buffer, INT convert_flags)
/********************************************************************\

  Routine: rpc_execute

  Purpose: Execute a RPC command received over the network

  Input:
    INT  sock               TCP socket to which the result should be
                            send back

    char *buffer            Command buffer
    INT  convert_flags      Flags for data conversion

  Output:
    none

  Function value:
    RPC_SUCCESS             Successful completion
    RPC_INVALID_ID          Invalid routine_id received
    RPC_NET_ERROR           Error in socket call
    RPC_EXCEED_BUFFER       Not enough memory for network buffer
    RPC_SHUTDOWN            Shutdown requested
    SS_ABORT                TCP connection broken
    SS_EXIT                 TCP connection closed

\********************************************************************/
{
INT          i, index, routine_id, status;
char         *in_param_ptr, *out_param_ptr, *last_param_ptr;
INT          tid, flags;
NET_COMMAND  *nc_in, *nc_out;
INT          param_size, max_size;
void         *prpc_param[20];
char         str[1024], debug_line[1024];

/* return buffer must be auto for multi-thread servers */
char         return_buffer[NET_BUFFER_SIZE];


  /* extract pointer array to parameters */
  nc_in  = (NET_COMMAND *) buffer;
  nc_out = (NET_COMMAND *) return_buffer;

  /* convert header format (byte swapping) */
  if (convert_flags)
    {
    rpc_convert_single(&nc_in->header.routine_id, TID_DWORD, 
                       0, convert_flags);
    rpc_convert_single(&nc_in->header.param_size, TID_DWORD, 
                       0, convert_flags);
    }

  /* no result return in FAST TCP mode */
  if (nc_in->header.routine_id & TCP_FAST)
    sock = 0;

  /* find entry in rpc_list */
  routine_id = nc_in->header.routine_id & ~TCP_FAST;

  for (i=0 ;; i++)
    if (rpc_list[i].id == 0 ||
        rpc_list[i].id == routine_id)
      break;
  index = i;
  if (rpc_list[i].id == 0)
    {
    cm_msg(MERROR, "rpc_execute", "Invalid rpc ID (%ld)", routine_id);
    return RPC_INVALID_ID;
    }

  in_param_ptr  = nc_in->param;
  out_param_ptr = nc_out->param;

  if (_debug_print)
    sprintf(debug_line, "%s(", rpc_list[index].name);

  for (i=0 ; rpc_list[index].param[i].tid != 0; i++)
    {
    tid   = rpc_list[index].param[i].tid;
    flags = rpc_list[index].param[i].flags;

    if (flags & RPC_IN)
      {
      param_size  = ALIGN(tid_size[ tid ]);

      if (tid == TID_STRING || tid == TID_LINK)
        param_size = ALIGN(1+strlen((char *) (in_param_ptr)));

      if (flags & RPC_VARARRAY)
        {
        /* for arrays, the size is stored as a INT in front of the array */
        param_size = *((INT *) in_param_ptr);
        if (convert_flags)
          rpc_convert_single(&param_size, TID_INT, 0, convert_flags);
        param_size = ALIGN(param_size);

        in_param_ptr += ALIGN( sizeof(INT) );
        }

      if (tid == TID_STRUCT)
        param_size = ALIGN( rpc_list[index].param[i].n );

      prpc_param[i] = in_param_ptr;

      /* convert data format */
      if (convert_flags)
        {
        if (flags & RPC_VARARRAY)
          rpc_convert_data(in_param_ptr, tid, flags,
                           param_size, convert_flags);
        else
          rpc_convert_data(in_param_ptr, tid, flags, 
                           rpc_list[index].param[i].n * tid_size[tid],
                           convert_flags);
        }

      if (_debug_print)
        {
        db_sprintf(str, in_param_ptr, param_size, 0, rpc_list[index].param[i].tid);
        if (rpc_list[index].param[i].tid == TID_STRING)
          {
          /* check for long strings (db_create_record...) */
          if (strlen(debug_line)+strlen(str)+2 < sizeof(debug_line))
            {
            strcat(debug_line, "\"");
            strcat(debug_line, str);
            strcat(debug_line, "\"");
            }
          else
            strcat(debug_line, "...");
          }
        else
          strcat(debug_line, str);
        }

      in_param_ptr += param_size;
      }

    if (flags & RPC_OUT)
      {
      param_size  = ALIGN(tid_size[tid]);

      if (flags & RPC_VARARRAY || tid == TID_STRING)
        {
        /* save maximum array length */
        max_size = *((INT *) in_param_ptr);
        if (convert_flags)
          rpc_convert_single(&max_size, TID_INT, 0, convert_flags);
        max_size = ALIGN(max_size);
        
        *((INT *) out_param_ptr) = max_size;

        /* save space for return array length */
        out_param_ptr += ALIGN( sizeof(INT) );

        /* use maximum array length from input */
        param_size += max_size;
        }

      if (rpc_list[index].param[i].tid == TID_STRUCT)
        param_size = ALIGN( rpc_list[index].param[i].n );

      if ((PTYPE) out_param_ptr - (PTYPE) nc_out + param_size >
          NET_BUFFER_SIZE)
        {
        cm_msg(MERROR, "rpc_execute", "return parameters (%d) too large for network buffer (%d)",
               (PTYPE) out_param_ptr - (PTYPE) nc_out + param_size, NET_BUFFER_SIZE);
        return RPC_EXCEED_BUFFER;
        }

      /* if parameter goes both directions, copy input to output */
      if (rpc_list[index].param[i].flags & RPC_IN)
        memcpy(out_param_ptr, prpc_param[i], param_size);

      if (_debug_print && !(flags & RPC_IN))
        strcat(debug_line, "-");

      prpc_param[i] = out_param_ptr;
      out_param_ptr += param_size;
      }
    
    if (_debug_print)
      if (rpc_list[index].param[i+1].tid)
        strcat(debug_line, ", ");
    }

  if (_debug_print)
    {
    strcat(debug_line, ")");
    _debug_print(debug_line);
    }

  last_param_ptr = out_param_ptr;

  /*********************************\
  *   call dispatch function        *
  \*********************************/
  if (rpc_list[index].dispatch)
    status = rpc_list[index].dispatch(routine_id, prpc_param);
  else
    status = RPC_INVALID_ID;

  if (routine_id == RPC_ID_EXIT || routine_id == RPC_ID_SHUTDOWN ||
      routine_id == RPC_ID_WATCHDOG)
    status = RPC_SUCCESS;

  /* Return if TCP connection broken */  
  if (status == SS_ABORT)
    return SS_ABORT;

  /* if sock == 0, we are in FTCP mode and may not sent results */
  if (!sock)
    return RPC_SUCCESS;

  /* compress variable length arrays */
  out_param_ptr = nc_out->param;
  for (i=0 ; rpc_list[index].param[i].tid != 0; i++)
    if (rpc_list[index].param[i].flags & RPC_OUT)
      {
      tid   = rpc_list[index].param[i].tid;
      flags = rpc_list[index].param[i].flags;
      param_size  = ALIGN(tid_size[ tid ]);

      if (tid == TID_STRING)
        {
        max_size = *((INT *) out_param_ptr);
        param_size = strlen((char *) prpc_param[i]) + 1;
        param_size = ALIGN(param_size);

        /* move string ALIGN(sizeof(INT)) left */
        memcpy(out_param_ptr, out_param_ptr + ALIGN(sizeof(INT)), param_size);

        /* move remaining parameters to end of string */
        memcpy(out_param_ptr + param_size,
               out_param_ptr + max_size + ALIGN(sizeof(INT)),
               (PTYPE) last_param_ptr -
                 ((PTYPE) out_param_ptr + max_size + ALIGN(sizeof(INT))));
        }

      if (flags & RPC_VARARRAY)
        {
        /* store array length at current out_param_ptr */
        max_size = *((INT *) out_param_ptr);
        param_size = *((INT *) prpc_param[i+1]);
        *((INT *) out_param_ptr) = param_size;
        if (convert_flags)
          rpc_convert_single(out_param_ptr, TID_INT, 
                             RPC_OUTGOING, convert_flags);

        out_param_ptr += ALIGN( sizeof(INT) );

        param_size = ALIGN(param_size);

        /* move remaining parameters to end of array */
        memcpy(out_param_ptr + param_size,
               out_param_ptr + max_size + ALIGN(sizeof(INT)),
               (PTYPE) last_param_ptr -
                 ((PTYPE) out_param_ptr + max_size + ALIGN(sizeof(INT))));
        }

      if (tid == TID_STRUCT)
        param_size = ALIGN( rpc_list[index].param[i].n );

      /* convert data format */
      if (convert_flags)
        {
        if (flags & RPC_VARARRAY)
          rpc_convert_data(out_param_ptr, tid, 
                           rpc_list[index].param[i].flags | RPC_OUTGOING,
                           param_size, convert_flags);
        else
          rpc_convert_data(out_param_ptr, tid, 
                           rpc_list[index].param[i].flags | RPC_OUTGOING,
                           rpc_list[index].param[i].n * tid_size[tid], 
                           convert_flags);
        }

      out_param_ptr += param_size;
      }

  /* send return parameters */
  param_size = (PTYPE) out_param_ptr - (PTYPE) nc_out->param;
  nc_out->header.routine_id = status;
  nc_out->header.param_size = param_size;

  /* convert header format (byte swapping) if necessary */
  if (convert_flags)
    {
    rpc_convert_single(&nc_out->header.routine_id, TID_DWORD, 
                       RPC_OUTGOING, convert_flags);
    rpc_convert_single(&nc_out->header.param_size, TID_DWORD, 
                       RPC_OUTGOING, convert_flags);
    }

  status = send_tcp(sock, return_buffer,
                    sizeof(NET_COMMAND_HEADER) + param_size, 0);

  if (status < 0)
    {
    cm_msg(MERROR, "rpc_execute", "send_tcp() failed");
    return RPC_NET_ERROR;
    }

  /* print return buffer */
/*
  printf("Return buffer, ID %d:\n", routine_id);
  for (i=0; i<param_size ; i++)
    {
    status = (char) nc_out->param[i];
    printf("%02X ", status);
    if (i%8 == 7)
      printf("\n");
    }
*/
  /* return SS_EXIT if RPC_EXIT is called */
  if (routine_id == RPC_ID_EXIT)
    return SS_EXIT;

  /* return SS_SHUTDOWN if RPC_SHUTDOWN is called */
  if (routine_id == RPC_ID_SHUTDOWN)
    return RPC_SHUTDOWN;

  return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

INT rpc_execute_ascii(INT sock, char *buffer)
/********************************************************************\

  Routine: rpc_execute_ascii

  Purpose: Execute a RPC command received over the network in ASCII
           mode

  Input:
    INT  sock               TCP socket to which the result should be
                            send back

    char *buffer            Command buffer

  Output:
    none

  Function value:
    RPC_SUCCESS             Successful completion
    RPC_INVALID_ID          Invalid routine_id received
    RPC_NET_ERROR           Error in socket call
    RPC_EXCEED_BUFFER       Not enough memory for network buffer
    RPC_SHUTDOWN            Shutdown requested
    SS_ABORT                TCP connection broken
    SS_EXIT                 TCP connection closed

\********************************************************************/
{
#define ASCII_BUFFER_SIZE 64500
#define N_APARAM           1024

INT          i, j, index, status, index_in;
char         *in_param_ptr, *out_param_ptr, *last_param_ptr;
INT          routine_id, tid, flags, array_tid, n_param;
INT          param_size, item_size, num_values;
void         *prpc_param[20];
char         *arpc_param[N_APARAM], *pc;
char         str[1024], debug_line[1024];
char         buffer1[ASCII_BUFFER_SIZE];       /* binary in */
char         buffer2[ASCII_BUFFER_SIZE];       /* binary out */
char         return_buffer[ASCII_BUFFER_SIZE]; /* ASCII out */

  /* parse arguments */
  arpc_param[0] = buffer;
  for (i=1 ; i<N_APARAM ; i++)
    {
    arpc_param[i] = strchr(arpc_param[i-1], '&');
    if (arpc_param[i] == NULL)
      break;
    *arpc_param[i] = 0;
    arpc_param[i]++;
    }

  /* decode '%' */
  for (i=0 ; i<N_APARAM && arpc_param[i] ; i++)
    while ((pc = strchr(arpc_param[i], '%')) != NULL)
      {
      if (isxdigit(pc[1]) && isxdigit(pc[2])) 
        {
        str[0] = pc[1];
        str[1] = pc[2];
        str[2] = 0;
        sscanf(str, "%02X", &i);

        *pc++ = i;
        while (pc[2])
          {
          pc[0] = pc[2];
          pc++;
          }
        }
      }

  /* find entry in rpc_list */
  for (i=0 ;; i++)
    if (rpc_list[i].id == 0 ||
        strcmp(arpc_param[0], rpc_list[i].name) == 0)
      break;
  index = i;
  routine_id = rpc_list[i].id;
  if (rpc_list[i].id == 0)
    {
    cm_msg(MERROR, "rpc_execute", "Invalid rpc name (%s)", arpc_param[0]);
    return RPC_INVALID_ID;
    }

  in_param_ptr  = buffer1;
  out_param_ptr = buffer2;
  index_in = 1;

  if (_debug_print)
    sprintf(debug_line, "%s(", rpc_list[index].name);

  for (i=0 ; rpc_list[index].param[i].tid != 0; i++)
    {
    tid   = rpc_list[index].param[i].tid;
    flags = rpc_list[index].param[i].flags;

    if (flags & RPC_IN)
      {
      if (flags & RPC_VARARRAY)
        {
        sscanf(arpc_param[index_in++], "%d %d", &n_param, &array_tid);

        prpc_param[i] = in_param_ptr;
        for (j=0 ; j<n_param ; j++)
          {
          db_sscanf(arpc_param[index_in++], in_param_ptr, &param_size, 0, array_tid);
          in_param_ptr += param_size;
          }
        in_param_ptr = (char *) ALIGN(((PTYPE)in_param_ptr));

        if (_debug_print)
          strcat(debug_line, "<array>");
        }
      else 
        {
        db_sscanf(arpc_param[index_in++], in_param_ptr, &param_size, 0, tid);
        param_size  = ALIGN(param_size);

        if (tid == TID_STRING || tid == TID_LINK)
          param_size = ALIGN(1+strlen((char *) (in_param_ptr)));

  /*
        if (tid == TID_STRUCT)
          param_size = ALIGN( rpc_list[index].param[i].n );
  */
        prpc_param[i] = in_param_ptr;

        if (_debug_print)
          {
          db_sprintf(str, in_param_ptr, param_size, 0, rpc_list[index].param[i].tid);
          if (rpc_list[index].param[i].tid == TID_STRING)
            {
            /* check for long strings (db_create_record...) */
            if (strlen(debug_line)+strlen(str)+2 < sizeof(debug_line))
              {
              strcat(debug_line, "\"");
              strcat(debug_line, str);
              strcat(debug_line, "\"");
              }
            else
              strcat(debug_line, "...");
            }
          else
            strcat(debug_line, str);
          }

        in_param_ptr += param_size;
        }

      if ((PTYPE) in_param_ptr - (PTYPE) buffer1 >
          ASCII_BUFFER_SIZE)
        {
        cm_msg(MERROR, "rpc_ascii_execute", "parameters (%d) too large for network buffer (%d)",
               param_size, ASCII_BUFFER_SIZE);
        return RPC_EXCEED_BUFFER;
        }

      }

    if (flags & RPC_OUT)
      {
      param_size  = ALIGN(tid_size[tid]);

      if (flags & RPC_VARARRAY || tid == TID_STRING)
        {
        /* reserve maximum array length */
        param_size = atoi(arpc_param[index_in]);
        param_size = ALIGN(param_size);
        }

/*
      if (rpc_list[index].param[i].tid == TID_STRUCT)
        param_size = ALIGN( rpc_list[index].param[i].n );
*/
      if ((PTYPE) out_param_ptr - (PTYPE) buffer2 + param_size >
          ASCII_BUFFER_SIZE)
        {
        cm_msg(MERROR, "rpc_execute", "return parameters (%d) too large for network buffer (%d)",
               (PTYPE) out_param_ptr - (PTYPE) buffer2 + param_size, ASCII_BUFFER_SIZE);
        return RPC_EXCEED_BUFFER;
        }

      /* if parameter goes both directions, copy input to output */
      if (rpc_list[index].param[i].flags & RPC_IN)
        memcpy(out_param_ptr, prpc_param[i], param_size);

      if (_debug_print && !(flags & RPC_IN))
        strcat(debug_line, "-");

      prpc_param[i] = out_param_ptr;
      out_param_ptr += param_size;
      }
    
    if (_debug_print)
      if (rpc_list[index].param[i+1].tid)
        strcat(debug_line, ", ");
    }

  if (_debug_print)
    {
    strcat(debug_line, ")");
    _debug_print(debug_line);
    }

  last_param_ptr = out_param_ptr;

  /*********************************\
  *   call dispatch function        *
  \*********************************/
  if (rpc_list[index].dispatch)
    status = rpc_list[index].dispatch(routine_id, prpc_param);
  else
    status = RPC_INVALID_ID;

  if (routine_id == RPC_ID_EXIT || routine_id == RPC_ID_SHUTDOWN ||
      routine_id == RPC_ID_WATCHDOG)
    status = RPC_SUCCESS;

  /* Return if TCP connection broken */  
  if (status == SS_ABORT)
    return SS_ABORT;

  /* if sock == 0, we are in FTCP mode and may not sent results */
  if (!sock)
    return RPC_SUCCESS;

  /* send return status */
  out_param_ptr = return_buffer;
  sprintf(out_param_ptr, "%d", status);
  out_param_ptr += strlen(out_param_ptr);

  /* convert return parameters */
  for (i=0 ; rpc_list[index].param[i].tid != 0; i++)
    if (rpc_list[index].param[i].flags & RPC_OUT)
      {
      *out_param_ptr++ = '&';
      
      tid   = rpc_list[index].param[i].tid;
      flags = rpc_list[index].param[i].flags;
      param_size  = ALIGN(tid_size[ tid ]);

      if (tid == TID_STRING && !(flags & RPC_VARARRAY))
        {
        strcpy(out_param_ptr, prpc_param[i]);
        param_size = strlen(prpc_param[i]);
        }

      else if (flags & RPC_VARARRAY)
        {
        if (rpc_list[index].id == RPC_BM_RECEIVE_EVENT)
          {
          param_size = *((INT *) prpc_param[i+1]);
          /* write number of bytes to output */
          sprintf(out_param_ptr, "%d", param_size);
          out_param_ptr += strlen(out_param_ptr)+1;  /* '0' finishes param */
          memcpy(out_param_ptr, prpc_param[i], param_size);
          out_param_ptr += param_size;
          *out_param_ptr = 0;
          }
        else
          {
          if (rpc_list[index].id == RPC_DB_GET_DATA1)
            {
            param_size = *((INT *) prpc_param[i+1]);
            array_tid  = *((INT *) prpc_param[i+2]);
            num_values = *((INT *) prpc_param[i+3]);
            }
          else if (rpc_list[index].id == RPC_DB_GET_DATA_INDEX)
            {
            param_size = *((INT *) prpc_param[i+1]);
            array_tid  = *((INT *) prpc_param[i+3]);
            num_values = 1;
            }
          else if (rpc_list[index].id == RPC_HS_READ)
            {
            param_size = *((INT *) prpc_param[i+1]);
            if (i == 6)
              {
              array_tid  = TID_DWORD;
              num_values = param_size / sizeof(DWORD);
              }
            else
              {
              array_tid  = *((INT *) prpc_param[10]);
              num_values = *((INT *) prpc_param[11]);
              }
            }
          else /* variable arrays of fixed type like hs_enum_events, hs_enum_vars */
            {
            param_size =  *((INT *) prpc_param[i+1]);
            array_tid = tid;
            if (tid == TID_STRING)
              num_values = param_size / NAME_LENGTH;
            else
              num_values = param_size / tid_size[tid];
            }

          /* derive size of individual item */
          if (array_tid == TID_STRING)
            item_size = param_size / num_values;
          else
            item_size = tid_size[array_tid];
        
          /* write number of elements to output */
          sprintf(out_param_ptr, "%d", num_values);
          out_param_ptr += strlen(out_param_ptr);

          /* write array of values to output */
          for (j=0 ; j<num_values ; j++)
            {
            *out_param_ptr++ = '&';
            db_sprintf(out_param_ptr, prpc_param[i], item_size, j, array_tid);
            out_param_ptr += strlen(out_param_ptr);
            }
          }
        }

/*
      else if (tid == TID_STRUCT)
        param_size = ALIGN( rpc_list[index].param[i].n );
*/
      else
        db_sprintf(out_param_ptr, prpc_param[i], param_size, 0, tid);

      out_param_ptr += strlen(out_param_ptr);

      if ((PTYPE) out_param_ptr - (PTYPE) return_buffer >
          ASCII_BUFFER_SIZE)
        {
        cm_msg(MERROR, "rpc_execute", "return parameter (%d) too large for network buffer (%d)",
               param_size, ASCII_BUFFER_SIZE);
        return RPC_EXCEED_BUFFER;
        }
      }

  /* send return parameters */
  param_size = (PTYPE) out_param_ptr - (PTYPE) return_buffer + 1;

  status = send_tcp(sock, return_buffer, param_size, 0);

  if (status < 0)
    {
    cm_msg(MERROR, "rpc_execute", "send_tcp() failed");
    return RPC_NET_ERROR;
    }

  /* print return buffer */
  if (_debug_print)
    {
    if (strlen(return_buffer) > sizeof(debug_line))
      {
      memcpy(debug_line, return_buffer, sizeof(debug_line)-10);
      strcat(debug_line, "...");
      }
    else
      sprintf(debug_line, "-> %s", return_buffer);
    _debug_print(debug_line);
    }

  /* return SS_EXIT if RPC_EXIT is called */
  if (routine_id == RPC_ID_EXIT)
    return SS_EXIT;

  /* return SS_SHUTDOWN if RPC_SHUTDOWN is called */
  if (routine_id == RPC_ID_SHUTDOWN)
    return RPC_SHUTDOWN;

  return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

INT rpc_server_accept(int lsock)
/********************************************************************\

  Routine: rpc_server_accept

  Purpose: Accept new incoming connections

  Input:
    INT    lscok            Listen socket

  Output:
    none

  Function value:
    RPC_SUCCESS             Successful completion
    RPC_NET_ERROR           Error in socket call
    RPC_CONNCLOSED          Connection was closed
    RPC_SHUTDOWN            Listener shutdown
    RPC_EXCEED_BUFFER       Not enough memory for network buffer

\********************************************************************/
{
INT                  index, i;
int                  size, status;
char                 command, version[32], v1[32];
INT                  sock, port1, port2, port3;
struct sockaddr_in   acc_addr;
struct hostent       *phe;
char                 str[100];
char                 host_port1_str[30], host_port2_str[30], host_port3_str[30];
char                 debug_str[30];
char                 *argv[10];
char                 net_buffer[256];
struct linger        ling;

static struct callback_addr callback;

  if (lsock > 0)
    {
    size = sizeof(acc_addr);
    sock = accept(lsock, (void *)&acc_addr, (void *)&size);

    if (sock == -1)
      return RPC_NET_ERROR;
    }
  else
    {
    /* lsock is stdin -> already connected from inetd */

    size = sizeof(acc_addr);
    sock = lsock;
    getpeername(sock, (void *) &acc_addr, (void *)&size);
    }

  /* receive string with timeout */
  i = recv_string(sock, net_buffer, 256, 10000);
  if (i > 0)
    {
    command = (char) toupper(net_buffer[0]);

    switch (command)
      {
      case 'S': /*----------- shutdown listener ----------------------*/
        closesocket(sock);
        return RPC_SHUTDOWN;

      case 7:
        ss_shell(sock);
        closesocket(sock);
        break;

      case 'I': /*----------- return available experiments -----------*/
        cm_scan_experiments();
        for (i=0 ; i<MAX_EXPERIMENT && exptab[i].name[0] ; i++)
          {
          sprintf(str, "%s", exptab[i].name);
          send(sock, str, strlen(str)+1, 0);
          }
        send(sock, "", 1, 0);
        closesocket(sock);
        break;

      case 'C': /*----------- connect to experiment -----------*/

        /* get callback information */
        callback.experiment[0] = 0;
        port1 = port2 = version[0] = 0;
        sscanf(net_buffer+2, "%ld %ld %ld %s", &port1, &port2, &port3, version);
        strcpy(callback.experiment, strchr(strchr(strchr(strchr(net_buffer+2, ' ')+1, ' ')+1, ' ')+1, ' ')+1);

        /* print warning if version patch level doesn't agree */
        strcpy(v1, version);
        if (strchr(v1, '.'))
          if (strchr(strchr(v1, '.')+1, '.'))
            *strchr(strchr(v1, '.')+1, '.') = 0;

        strcpy(str, cm_get_version());
        if (strchr(str, '.'))
          if (strchr(strchr(str, '.')+1, '.'))
            *strchr(strchr(str, '.')+1, '.') = 0;

        if (strcmp(v1, str) != 0)
          {
          sprintf(str, "client MIDAS version %s differs from local version %s", 
                        version, cm_get_version());
          cm_msg(MERROR, "rpc_server_accept", str);

          sprintf(str, "received string: %s", net_buffer+2);
          cm_msg(MERROR, "rpc_server_accept", str);
          }

        callback.host_port1 = (short) port1;
        callback.host_port2 = (short) port2;
        callback.host_port3 = (short) port3;
        callback.debug = _debug_mode;

        /* get the name of the remote host */
#ifdef OS_VXWORKS
        {
        INT status;
        status = hostGetByAddr(acc_addr.sin_addr.s_addr, callback.host_name);
        if (status != 0)
          {
          cm_msg(MERROR, "rpc_server_accept", "cannot get host name");
          break;
          }
        }
#else
        phe = gethostbyaddr((char *) &acc_addr.sin_addr, 4, PF_INET);
        if (phe == NULL)
          {
          /* use IP number instead */
          strcpy(callback.host_name, (char *)inet_ntoa(acc_addr.sin_addr));
          }
        else
          strcpy(callback.host_name, phe->h_name);
#endif

        if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_MPROCESS)
          {
          /* update experiment definition */
          cm_scan_experiments();

          /* lookup experiment */
          if (equal_ustring(callback.experiment, "Default"))
            index = 0;
          else
            for (index=0 ; index<MAX_EXPERIMENT && exptab[index].name[0] ; index++)
              if (equal_ustring(callback.experiment, exptab[index].name))
                break;

          if (index == MAX_EXPERIMENT || exptab[index].name[0] == 0)
            {
            sprintf(str, "experiment %s not defined in exptab\r", 
                     callback.experiment);
            cm_msg(MERROR, "rpc_server_accept", str);
        
            send(sock, "2", 2, 0); /* 2 means exp. not found */
            closesocket(sock);
            break;
            }

          sprintf(str, "1 %s", cm_get_version()); /* 1 means ok */
          send(sock, str, strlen(str)+1, 0);
          closesocket(sock);

          strcpy(callback.directory, exptab[index].directory);
          strcpy(callback.user,      exptab[index].user);

          /* create a new process */
          sprintf(host_port1_str, "%d", callback.host_port1);
          sprintf(host_port2_str, "%d", callback.host_port2);
          sprintf(host_port3_str, "%d", callback.host_port3);
          sprintf(debug_str, "%d", callback.debug);

          argv[0] = (char *) rpc_get_server_option(RPC_OSERVER_NAME);
          argv[1] = callback.host_name;
          argv[2] = host_port1_str;
          argv[3] = host_port2_str;
          argv[4] = host_port3_str;
          argv[5] = debug_str;
          argv[6] = callback.experiment;
          argv[7] = callback.directory;
          argv[8] = callback.user;
          argv[9] = NULL;

/*
          cm_msg(MINFO, "", "%s %s %s %s %s %s %s %s %s %s", 
            argv[0], argv[1], argv[2], argv[3], argv[4], 
            argv[5], argv[6], argv[7], argv[8], argv[9]);
*/
          status = ss_spawnv(P_NOWAIT, (char *) rpc_get_server_option(RPC_OSERVER_NAME), argv);
          if (status != SS_SUCCESS)
            cm_msg(MERROR, "rpc_server_accept", "cannot spawn subprocess");

          break;
          }
        else
          {
          sprintf(str, "1 %s", cm_get_version()); /* 1 means ok */
          send(sock, str, strlen(str)+1, 0);
          closesocket(sock);
          }

        /* look for next free entry */
        for (index=0 ; index<MAX_RPC_CONNECTION ; index++)
          if (_server_acception[index].recv_sock == 0)
            break;
        if (index == MAX_RPC_CONNECTION)
          return RPC_NET_ERROR;
        callback.index = index;

        /*----- multi thread server ------------------------*/
        if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_MTHREAD)
          ss_thread_create(rpc_server_thread, (void *) (&callback));

        /*----- single thread server -----------------------*/
        if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_SINGLE ||
            rpc_get_server_option(RPC_OSERVER_TYPE) == ST_REMOTE )
          rpc_server_callback(&callback);

        break;

      default:
        cm_msg(MERROR, "rpc_server_accept", "received unknown command '%c'", command);
        closesocket(sock);
        break;

      }
    }
  else /* if i>0 */
    {
    /* lingering needed for PCTCP */
    ling.l_onoff = 1;
    ling.l_linger = 0;
    setsockopt(sock, SOL_SOCKET, SO_LINGER, (char *) &ling, sizeof(ling));
    closesocket(sock);
    }

  return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

INT rpc_client_accept(int lsock)
/********************************************************************\

  Routine: rpc_client_accept

  Purpose: Accept new incoming connections as a client

  Input:
    INT    lsock            Listen socket

  Output:
    none

  Function value:
    RPC_SUCCESS             Successful completion
    RPC_NET_ERROR           Error in socket call
    RPC_CONNCLOSED          Connection was closed
    RPC_SHUTDOWN            Listener shutdown
    RPC_EXCEED_BUFFER       Not enough memory for network buffer

\********************************************************************/
{
INT                  index, i, version, status;
int                  size, sock;
struct sockaddr_in   acc_addr;
INT                  client_hw_type, hw_type;
char                 str[100], client_program[NAME_LENGTH];
char                 host_name[HOST_NAME_LENGTH];
INT                  convert_flags;
char                 net_buffer[256], *p;

  size = sizeof(acc_addr);
  sock = accept(lsock, (void *)&acc_addr, (void *)&size);

  if (sock == -1)
    return RPC_NET_ERROR;

  /* get the name of the calling host */
/* outcommented for speed reasons SR 7.10.98
#ifdef OS_VXWORKS
  {
  status = hostGetByAddr(acc_addr.sin_addr.s_addr, host_name);
  if (status != 0)
    strcpy(host_name, "unknown");
  }
#else
  phe = gethostbyaddr((char *) &acc_addr.sin_addr, 4, PF_INET);
  if (phe == NULL)
    strcpy(host_name, "unknown");
  strcpy(host_name, phe->h_name);
#endif
*/
  strcpy(host_name, "");

  /* look for next free entry */
  for (index=0 ; index<MAX_RPC_CONNECTION ; index++)
    if (_server_acception[index].recv_sock == 0)
      break;
  if (index == MAX_RPC_CONNECTION)
    {
    closesocket(sock);
    return RPC_NET_ERROR;
    }

  /* receive string with timeout */
  i = recv_string(sock, net_buffer, sizeof(net_buffer), 10000);
  if (i <= 0)
    {
    closesocket(sock);
    return RPC_NET_ERROR;
    }

  /* get remote computer info */
  p = strtok(net_buffer, " ");
  if (p != NULL)
    {
    client_hw_type = atoi(p);
    p = strtok(NULL, " ");
    }
  if (p != NULL)
    {
    version = atoi(p);
    p = strtok(NULL, " ");
    }
  if (p != NULL)
    {
    strcpy(client_program, p);
    p = strtok(NULL, " ");
    }
  if (p != NULL)
    {
    strcpy(host_name, p);
    p = strtok(NULL, " ");
    }

  /* save information in _server_acception structure */
  _server_acception[index].recv_sock = sock;
  _server_acception[index].send_sock = 0;
  _server_acception[index].event_sock = 0;
  _server_acception[index].remote_hw_type = client_hw_type;
  strcpy(_server_acception[index].host_name, host_name);
  strcpy(_server_acception[index].prog_name, client_program);
  _server_acception[index].tid = ss_gettid();
  _server_acception[index].last_activity = ss_millitime();
  _server_acception[index].watchdog_timeout = 0;

  /* send my own computer id */
  hw_type = rpc_get_option(0, RPC_OHW_TYPE);
  sprintf(str, "%ld %s", hw_type, cm_get_version());
  status = send(sock, str, strlen(str)+1, 0);
  if (status != (INT)strlen(str)+1)
    return RPC_NET_ERROR;

  rpc_set_server_acception(index+1);
  rpc_calc_convert_flags(hw_type, client_hw_type, &convert_flags);
  rpc_set_server_option(RPC_CONVERT_FLAGS, convert_flags);

  /* set callback function for ss_suspend */
  ss_suspend_set_dispatch(CH_SERVER, _server_acception,
                          rpc_server_receive);

  return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

INT rpc_server_callback(struct callback_addr *pcallback)
/********************************************************************\

  Routine: rpc_server_callback

  Purpose: Callback a remote client. Setup _server_acception entry
           with optional conversion flags and establish two-way
           TCP connection.

  Input:
    callback_addr pcallback Pointer to a callback structure

  Output:
    none

  Function value:
    RPC_SUCCESS             Successful completion

\********************************************************************/
{
INT                  index, status;
int                  recv_sock, send_sock, event_sock;
struct sockaddr_in   bind_addr;
struct hostent       *phe;
char                 str[100], client_program[NAME_LENGTH];
char                 host_name[HOST_NAME_LENGTH];
INT                  client_hw_type, hw_type;
INT                  convert_flags;
char                 net_buffer[256];
struct callback_addr callback;
int                  flag;

  /* copy callback information */
  memcpy(&callback, pcallback, sizeof(callback));
  index = callback.index;

  /* create new sockets for TCP */
  recv_sock = socket(AF_INET, SOCK_STREAM, 0);
  send_sock = socket(AF_INET, SOCK_STREAM, 0);
  event_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (event_sock == -1)
    return RPC_NET_ERROR;

  /* bind local host and port to sockets */
  memset(&bind_addr, 0, sizeof(bind_addr));
  bind_addr.sin_family      = AF_INET;
  bind_addr.sin_addr.s_addr = 0;
  bind_addr.sin_port        = 0; /* OS chooses port */

  /* reuse address, needed if previous server stopped (30s timeout!) */
  flag = 1;
  status = setsockopt(recv_sock, SOL_SOCKET, SO_REUSEADDR,
                      (char *) &flag, sizeof(int));
  status = setsockopt(send_sock, SOL_SOCKET, SO_REUSEADDR,
                      (char *) &flag, sizeof(int));
  status = setsockopt(event_sock, SOL_SOCKET, SO_REUSEADDR,
                      (char *) &flag, sizeof(int));

  status = bind(recv_sock, (void *)&bind_addr, sizeof(bind_addr));
  if (status < 0)
    {
    cm_msg(MERROR, "rpc_server_callback", "cannot bind receive socket");
    goto error;
    }
  status = bind(send_sock, (void *)&bind_addr, sizeof(bind_addr));
  if (status < 0)
    {
    cm_msg(MERROR, "rpc_server_callback", "cannot bind send socket");
    goto error;
    }
  status = bind(event_sock, (void *)&bind_addr, sizeof(bind_addr));
  if (status < 0)
    {
    cm_msg(MERROR, "rpc_server_callback", "cannot bind event socket");
    goto error;
    }

  /* callback to remote node */
  memset(&bind_addr, 0, sizeof(bind_addr));
  bind_addr.sin_family      = AF_INET;
  bind_addr.sin_port        = htons(callback.host_port1);

#ifdef OS_VXWORKS
  {
  INT host_addr;

  host_addr = hostGetByName(callback.host_name);
  memcpy((char *)&(bind_addr.sin_addr), &host_addr, 4);
  }
#else
  phe = gethostbyname(callback.host_name);
  if (phe == NULL)
    {
    cm_msg(MERROR, "rpc_server_callback", "cannot get host name");
    return RPC_NET_ERROR;
    }
  memcpy((char *)&(bind_addr.sin_addr), phe->h_addr, phe->h_length);
#endif

  /* connect receive socket */
#ifdef OS_UNIX
  do
    {
    status = connect(recv_sock, (void *) &bind_addr, sizeof(bind_addr));

    /* don't return if an alarm signal was cought */
    } while (status == -1 && errno == EINTR); 
#else
  status = connect(recv_sock, (void *) &bind_addr, sizeof(bind_addr));
#endif  

  if (status != 0)
    {
    cm_msg(MERROR, "rpc_server_callback", "cannot connect receive socket");
    goto error;
    }

  bind_addr.sin_port = htons(callback.host_port2);

  /* connect send socket */
#ifdef OS_UNIX
  do
    {
    status = connect(send_sock, (void *) &bind_addr, sizeof(bind_addr));

    /* don't return if an alarm signal was cought */
    } while (status == -1 && errno == EINTR); 
#else
  status = connect(send_sock, (void *) &bind_addr, sizeof(bind_addr));
#endif  

  if (status != 0)
    {
    cm_msg(MERROR, "rpc_server_callback", "cannot connect send socket");
    goto error;
    }

  bind_addr.sin_port = htons(callback.host_port3);

  /* connect event socket */
#ifdef OS_UNIX
  do
    {
    status = connect(event_sock, (void *) &bind_addr, sizeof(bind_addr));

    /* don't return if an alarm signal was cought */
    } while (status == -1 && errno == EINTR); 
#else
  status = connect(event_sock, (void *) &bind_addr, sizeof(bind_addr));
#endif  

  if (status != 0)
    {
    cm_msg(MERROR, "rpc_server_callback", "cannot connect event socket");
    goto error;
    }

  /* increase receive buffer size to 64k */
#ifndef OS_ULTRIX /* crashes ULTRIX... */
  flag = 0x10000;
  setsockopt(event_sock, SOL_SOCKET, SO_RCVBUF, (char *) &flag, sizeof(INT));
#endif

  if (recv_string(recv_sock, net_buffer, 256, 10000) <= 0)
    {
    cm_msg(MERROR, "rpc_server_callback", "timeout on receive remote computer info");
    goto error;
    }

  /* get remote computer info */
  sscanf(net_buffer, "%ld", &client_hw_type);

  strcpy(client_program, strchr(net_buffer, ' ')+1);

  /* get the name of the remote host */
#ifdef OS_VXWORKS
  status = hostGetByAddr(bind_addr.sin_addr.s_addr, host_name);
  if (status != 0)
    strcpy(host_name, "unknown");
#else
  phe = gethostbyaddr((char *) &bind_addr.sin_addr, 4, PF_INET);
  if (phe == NULL)
    strcpy(host_name, "unknown");
  else
    strcpy(host_name, phe->h_name);
#endif

  /* save information in _server_acception structure */
  _server_acception[index].recv_sock = recv_sock;
  _server_acception[index].send_sock = send_sock;
  _server_acception[index].event_sock = event_sock;
  _server_acception[index].remote_hw_type = client_hw_type;
  strcpy(_server_acception[index].host_name, host_name);
  strcpy(_server_acception[index].prog_name, client_program);
  _server_acception[index].tid = ss_gettid();
  _server_acception[index].last_activity = ss_millitime();
  _server_acception[index].watchdog_timeout = 0;

  /* send my own computer id */
  hw_type = rpc_get_option(0, RPC_OHW_TYPE);
  sprintf(str, "%ld", hw_type);
  send(recv_sock, str, strlen(str)+1, 0);

  rpc_set_server_acception(index+1);
  rpc_calc_convert_flags(hw_type, client_hw_type, &convert_flags);
  rpc_set_server_option(RPC_CONVERT_FLAGS, convert_flags);

  /* set callback function for ss_suspend */
  ss_suspend_set_dispatch(CH_SERVER, _server_acception,
                          rpc_server_receive);

  if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_REMOTE)
    printf("Connection to %s:%s established\n",
           _server_acception[index].host_name,
           _server_acception[index].prog_name);

  return RPC_SUCCESS;

error:

  closesocket(recv_sock);
  closesocket(send_sock);
  closesocket(event_sock);

  return RPC_NET_ERROR;
}

/*------------------------------------------------------------------*/

INT rpc_server_thread(void *pointer)
/********************************************************************\

  Routine: rpc_server_thread

  Purpose: New thread for a multi-threaded server. Callback to the
           client and process RPC requests.

  Input:
    vcoid  pointer          pointer to callback_addr structure.

  Output:
    none

  Function value:
    RPC_SUCCESS             Successful completion

\********************************************************************/
{
struct callback_addr callback;
int                  status, mutex_alarm, mutex_elog;
static DWORD         last_checked = 0;

  memcpy(&callback, pointer, sizeof(callback));

  status = rpc_server_callback(&callback);

  if (status != RPC_SUCCESS)
    return status;

  /* create alarm and elog mutexes */
  ss_mutex_create("ALARM", &mutex_alarm);
  ss_mutex_create("ELOG", &mutex_elog);
  cm_set_experiment_mutex(mutex_alarm, mutex_elog);

  do
    {
    status = ss_suspend(5000, 0);

    if (rpc_check_channels() == RPC_NET_ERROR)
      break;

    /* check alarms every 10 seconds */
    if (!rpc_is_remote() && ss_time() - last_checked > 10)
      {
      al_check();
      last_checked = ss_time();
      }

    } while (status != SS_ABORT && status != SS_EXIT);

  /* delete entry in suspend table for this thread */
  ss_suspend_exit();

  return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

INT rpc_server_receive(INT index, int sock, BOOL check)
/********************************************************************\

  Routine: rpc_server_receive

  Purpose: Receive rpc commands and execute them. Close the connection
           if client has broken TCP pipe.

  Input:
    INT    index            Index to _server_acception structure in-
                            dicating which connection got data.
    int    sock             Socket which got data
    BOOL   check            If TRUE, only check if connection is
                            broken. This may be called via
                            bm_receive_event/ss_suspend(..,MSG_BM)

  Output:
    none

  Function value:
    RPC_SUCCESS             Successful completion
    RPC_EXCEED_BUFFER       Not enough memeory to allocate buffer
    SS_EXIT                 Server connection was closed
    SS_ABORT                Server connection was broken

\********************************************************************/
{
INT   status, n_received;
INT   remaining, *pbh, start_time;
char  test_buffer[256], str[80];
EVENT_HEADER *pevent;

  /* init network buffer */
  if (_net_recv_buffer_size == 0)
    {
    _net_recv_buffer = MALLOC(NET_BUFFER_SIZE);
    if (_net_recv_buffer == NULL)
      {
      cm_msg(MERROR, "rpc_server_receive", "not enough memory to allocate network buffer");
      return RPC_EXCEED_BUFFER;
      }
    _net_recv_buffer_size = NET_BUFFER_SIZE;
    }

  /* only check if TCP connection is broken */
  if (check)
    {
    n_received = recv(sock, test_buffer, sizeof(test_buffer), MSG_PEEK);

    if (n_received <= 0)
      return SS_ABORT;

    return SS_SUCCESS;
    }

  remaining = 0;

  /* receive command */
  if (sock == _server_acception[index].recv_sock)
    {
    do
      {
      if (_server_acception[index].remote_hw_type == DR_ASCII)
        n_received = recv_string(_server_acception[index].recv_sock, _net_recv_buffer,
                                 NET_BUFFER_SIZE, 10000); 
      else
        n_received = recv_tcp_server(index, _net_recv_buffer, NET_BUFFER_SIZE, 0, &remaining);

      if (n_received <= 0)
        {
        status = SS_ABORT;
        goto error;
        }

      rpc_set_server_acception(index+1);

      if (_server_acception[index].remote_hw_type == DR_ASCII)
        status = rpc_execute_ascii(_server_acception[index].recv_sock,
                                   _net_recv_buffer);
      else
        status = rpc_execute(_server_acception[index].recv_sock,
                           _net_recv_buffer,
                           _server_acception[index].convert_flags);

      if (status == SS_ABORT)
        goto error;

      if (status == SS_EXIT || status == RPC_SHUTDOWN)
        {
        if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_REMOTE)
          printf("Connection to %s:%s closed\n",
                 _server_acception[index].host_name,
                 _server_acception[index].prog_name);
        goto exit;
        }

      } while (remaining);
    }
  else
    {
    /* receive event */
    if (sock == _server_acception[index].event_sock)
      {
      start_time = ss_millitime();

      do
        {
        n_received = recv_event_server(index, _net_recv_buffer, NET_BUFFER_SIZE, 0, &remaining);

        if (n_received <= 0)
          {
          status = SS_ABORT;
          goto error;
          }

        /* send event to buffer */
        pbh = (INT *) _net_recv_buffer;
        pevent = (EVENT_HEADER *) (pbh+1);

        bm_send_event(*pbh, pevent, pevent->data_size + sizeof(EVENT_HEADER), SYNC);

        /* repeat for maximum 0.5 sec */
        } while (ss_millitime() - start_time < 500 && remaining);
      }
    }

  return RPC_SUCCESS;

error:

  strcpy(str, _server_acception[index].host_name);
  if (strchr(str, '.'))
    *strchr(str, '.') = 0;
  cm_msg(MTALK, "rpc_server_receive", "Program %s on host %s aborted",
                    _server_acception[index].prog_name, str);

exit:

  /* disconnect from experiment as MIDAS server */
  if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_REMOTE)
    {
    HNDLE hDB, hKey;

    cm_get_experiment_database(&hDB, &hKey);

    /* only disconnect from experiment if previously connected. 
       Necessary for pure RPC servers (RPC_SRVR) */
    if (hDB)
      {
#ifdef LOCAL_ROUTINES
      ss_alarm(0, cm_watchdog);
#endif

      cm_delete_client_info(hDB, 0);

      bm_close_all_buffers();
      db_close_all_databases();

      rpc_deregister_functions();

      cm_set_experiment_database(0, 0);

      _msg_buffer = 0;
      }
    }

  /* close server connection */
  if (_server_acception[index].recv_sock)
    closesocket(_server_acception[index].recv_sock);
  if (_server_acception[index].send_sock)
    closesocket(_server_acception[index].send_sock);
  if (_server_acception[index].event_sock)
    closesocket(_server_acception[index].event_sock);

  /* free TCP cache */
  FREE(_server_acception[index].net_buffer);
  _server_acception[index].net_buffer = NULL;

  /* mark this entry as invalid */
  memset(&_server_acception[index], 0, sizeof(RPC_SERVER_ACCEPTION));

  /* signal caller a shutdonw */
  if (status == RPC_SHUTDOWN)
    return status;

  /* don't abort if other than main connection is broken */
  if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_REMOTE)
    return SS_SUCCESS;

  return status;
}

/*------------------------------------------------------------------*/

INT rpc_server_shutdown(void)
/********************************************************************\

  Routine: rpc_server_shutdown

  Purpose: Shutdown RPC server, abort all connections

  Input:
    none

  Output:
    none

  Function value:
    RPC_SUCCESS             Successful completion

\********************************************************************/
{
INT i;
struct linger ling;

  /* close all open connections */
  for (i=0 ; i<MAX_RPC_CONNECTION ; i++)
    if (_server_acception[i].recv_sock != 0)
      {
      /* lingering needed for PCTCP */
      ling.l_onoff = 1;
      ling.l_linger = 0;
      setsockopt(_server_acception[i].recv_sock,
                 SOL_SOCKET, SO_LINGER, (char *) &ling, sizeof(ling));
      closesocket(_server_acception[i].recv_sock);

      if (_server_acception[i].send_sock)
        {
        setsockopt(_server_acception[i].send_sock,
                   SOL_SOCKET, SO_LINGER, (char *) &ling, sizeof(ling));
        closesocket(_server_acception[i].send_sock);
        }

      if (_server_acception[i].event_sock)
        {
        setsockopt(_server_acception[i].event_sock,
                   SOL_SOCKET, SO_LINGER, (char *) &ling, sizeof(ling));
        closesocket(_server_acception[i].event_sock);
        }

      _server_acception[i].recv_sock = 0;
      _server_acception[i].send_sock = 0;
      _server_acception[i].event_sock = 0;
      }

  if (_lsock)
    {
    closesocket(_lsock);
    _lsock = 0;
    _server_registered = FALSE;
    }

  /* free suspend structures */
  ss_suspend_exit();

  return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

INT rpc_check_channels(void)
/********************************************************************\

  Routine: rpc_check_channels

  Purpose: Check open rpc channels by sending watchdog messages

  Input:
    none

  Output:
    none

  Function value:
    RPC_SUCCESS             Channel is still alive
    RPC_NET_ERROR           Connection is broken

\********************************************************************/
{
INT             status, index, i, convert_flags;
NET_COMMAND     nc;
fd_set          readfds;
struct timeval  timeout;

  for (index=0 ; index < MAX_RPC_CONNECTION ; index++)
    {
    if (_server_acception[index].recv_sock && 
        _server_acception[index].tid == ss_gettid() &&
        _server_acception[index].watchdog_timeout &&
        (ss_millitime() - _server_acception[index].last_activity > 
         (DWORD) _server_acception[index].watchdog_timeout))
      {
/* printf("Send watchdog message to %s on %s\n", 
                _server_acception[index].prog_name,
                _server_acception[index].host_name); */

      /* send a watchdog message */
      nc.header.routine_id  = MSG_WATCHDOG;
      nc.header.param_size  = 0;

      convert_flags = rpc_get_server_option(RPC_CONVERT_FLAGS);
      if (convert_flags)
        {
        rpc_convert_single(&nc.header.routine_id, TID_DWORD, RPC_OUTGOING, convert_flags);
        rpc_convert_single(&nc.header.param_size, TID_DWORD, RPC_OUTGOING, convert_flags);
        }

      /* send the header to the client */
      i = send_tcp(_server_acception[index].send_sock, 
                   (char *) &nc, sizeof(NET_COMMAND_HEADER), 0);

      if (i<0)
        goto exit;

      /* make some timeout checking */
      FD_ZERO(&readfds);
      FD_SET(_server_acception[index].send_sock, &readfds);
      FD_SET(_server_acception[index].recv_sock, &readfds);

      timeout.tv_sec  = _server_acception[index].watchdog_timeout / 1000;
      timeout.tv_usec = (_server_acception[index].watchdog_timeout % 1000) * 1000;

      do
        {
	      status = select(FD_SETSIZE, (void *) &readfds, NULL, NULL, (void *) &timeout);

        /* if an alarm signal was cought, restart select with reduced timeout */
        if (status == -1 && timeout.tv_sec >= WATCHDOG_INTERVAL / 1000)
          timeout.tv_sec -= WATCHDOG_INTERVAL / 1000;

        } while (status == -1); /* dont return if an alarm signal was cought */

      if (!FD_ISSET(_server_acception[index].send_sock, &readfds) &&
          !FD_ISSET(_server_acception[index].recv_sock, &readfds))
        goto exit;

      /* receive result on send socket */
      if (FD_ISSET(_server_acception[index].send_sock, &readfds))
        {
        i = recv_tcp(_server_acception[index].send_sock, (char *) &nc, sizeof(nc), 0);
        if (i<=0)
          goto exit;
        }
      }
    }

  return RPC_SUCCESS;

exit:

  cm_msg(MINFO, "rpc_check_channels", "client [%s]%s failed watchdog test after %d sec",
         _server_acception[index].host_name,
         _server_acception[index].prog_name,
         _server_acception[index].watchdog_timeout/1000);

  /* disconnect from experiment */
  if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_REMOTE)
    cm_disconnect_experiment();

  /* close server connection */
  if (_server_acception[index].recv_sock)
    closesocket(_server_acception[index].recv_sock);
  if (_server_acception[index].send_sock)
    closesocket(_server_acception[index].send_sock);
  if (_server_acception[index].event_sock)
    closesocket(_server_acception[index].event_sock);

  /* free TCP cache */
  FREE(_server_acception[index].net_buffer);
  _server_acception[index].net_buffer = NULL;

  /* mark this entry as invalid */
  memset(&_server_acception[index], 0, sizeof(RPC_SERVER_ACCEPTION));

  return RPC_NET_ERROR;
}

/*------------------------------------------------------------------*/

/********************************************************************\
*                                                                    *
*                 Bank functions                                     *
*                                                                    *
\********************************************************************/

/*------------------------------------------------------------------*/

void bk_init(void *event)
/********************************************************************\

  Routine: bk_init

  Purpose: Create a MIDAS bank structure inside an event

  Input:
    void   *event           pointer to the event

  Output:
    none

  Function value:
    none

\********************************************************************/
{
  ((BANK_HEADER *) event)->data_size = 0;
  ((BANK_HEADER *) event)->flags = BANK_FORMAT_VERSION;
}

/*------------------------------------------------------------------*/

BOOL bk_is32(void *event)
/********************************************************************\

  Routine: bk_is32

  Purpose: Return true if banks inside event are 32-bit banks

  Input:
    void   *event           pointer to the event

  Output:
    none

  Function value:
    none

\********************************************************************/
{
  return ((((BANK_HEADER *) event)->flags & BANK_FORMAT_32BIT) > 0);
}

/*------------------------------------------------------------------*/

void bk_init32(void *event)
/********************************************************************\

  Routine: bk_init

  Purpose: Create a MIDAS 32-bit bank structure inside an event

  Input:
    void   *event           pointer to the event

  Output:
    none

  Function value:
    none

\********************************************************************/
{
  ((BANK_HEADER *) event)->data_size = 0;
  ((BANK_HEADER *) event)->flags = BANK_FORMAT_VERSION | BANK_FORMAT_32BIT;
}

/*------------------------------------------------------------------*/

INT bk_size(void *event)
/********************************************************************\

  Routine: bk_size

  Purpose: Return size of event containing MIDAS bnaks

  Input:
    void   *event           pointer to the event

  Output:
    none

  Function value:
    Size of events in bytes

\********************************************************************/
{
  return ((BANK_HEADER *) event)->data_size + sizeof(BANK_HEADER);
}                         

/*------------------------------------------------------------------*/

void bk_create(void *event, char *name, WORD type, void *pdata)
/********************************************************************\

  Routine: bk_create

  Purpose: Create a MIDAS bank inside an event of given type

  Input:
    void   *event           pointer to the event
    char   *name            Name of bank (exactly four letters)
    WORD   type             type of bank, one of the TID_xxx values

  Output:
    void   *pdata           Pointer to data area of bank, to be
                            filled by user code

  Function value:
    none

\********************************************************************/
{
  if (((BANK_HEADER *)event)->flags & BANK_FORMAT_32BIT)
    {
    BANK32 *pbk32;

    pbk32 = (BANK32 *) ((char *) (((BANK_HEADER *) event) + 1) + ((BANK_HEADER *) event)->data_size);
    strncpy(pbk32->name, name, 4);
    pbk32->type = type;
    pbk32->data_size = 0;
    *((void **)pdata) = pbk32+1;
    }
  else
    {
    BANK *pbk;

    pbk = (BANK *) ((char *) (((BANK_HEADER *) event) + 1) + ((BANK_HEADER *) event)->data_size);
    strncpy(pbk->name, name, 4);
    pbk->type = type;
    pbk->data_size = 0;
    *((void **)pdata) = pbk+1;
    }
}

/*------------------------------------------------------------------*/

int bk_delete(void *event, char *name)
/********************************************************************\

  Routine: bk_delete

  Purpose: Delete a MIDAS bank inside an event

  Input:
    void   *event           pointer to the event
    char   *name            Name of bank (exactly four letters)

  Function value:
    CM_SUCCESS              Bank has been deleted
    0                       Bank has not been found

\********************************************************************/
{
BANK   *pbk;
BANK32 *pbk32;
DWORD  dname;
int    remaining;

  if (((BANK_HEADER *)event)->flags & BANK_FORMAT_32BIT)
    {
    /* locate bank */
    pbk32 = (BANK32 *) (((BANK_HEADER *) event)+1);
    strncpy((char *) &dname, name, 4);
    do
      {
      if (*((DWORD *) pbk32->name) == dname)
        {
        /* bank found, delete it */
        remaining = (int) ((char *) event + ((BANK_HEADER *) event)->data_size + sizeof(BANK_HEADER)) -
                    (int) ((char *) (pbk32 + 1) + ALIGN(pbk32->data_size));

        /* reduce total event size */
        ((BANK_HEADER *) event)->data_size -= sizeof(BANK32) + ALIGN(pbk32->data_size);

        /* copy remaining bytes */
        if (remaining > 0)
          memcpy(pbk32, (char *) (pbk32 + 1) + ALIGN(pbk32->data_size), remaining);
        return CM_SUCCESS;
        }

      pbk32 = (BANK32 *) ((char *) (pbk32 + 1) + ALIGN(pbk32->data_size));
      } while ((DWORD) pbk32 - (DWORD) event < ((BANK_HEADER *) event)->data_size + sizeof(BANK_HEADER));
    }
  else
    {
    /* locate bank */
    pbk = (BANK *) (((BANK_HEADER *) event)+1);
    strncpy((char *) &dname, name, 4);
    do
      {
      if (*((DWORD *) pbk->name) == dname)
        {
        /* bank found, delete it */
        remaining = (int) ((char *) event + ((BANK_HEADER *) event)->data_size + sizeof(BANK_HEADER)) -
                    (int) ((char *) (pbk + 1) + ALIGN(pbk->data_size));

        /* reduce total event size */
        ((BANK_HEADER *) event)->data_size -= sizeof(BANK) + ALIGN(pbk->data_size);

        /* copy remaining bytes */
        if (remaining > 0)
          memcpy(pbk, (char *) (pbk + 1) + ALIGN(pbk->data_size), remaining);
        return CM_SUCCESS;
        }

      pbk = (BANK *) ((char *) (pbk + 1) + ALIGN(pbk->data_size));
      } while ((DWORD) pbk - (DWORD) event < ((BANK_HEADER *) event)->data_size + sizeof(BANK_HEADER));
    }

  return 0;
}

/*------------------------------------------------------------------*/

void bk_close(void *event, void *pdata)
/********************************************************************\

  Routine: bk_close

  Purpose: Close a MIDAS bank previously opened with bk_open

  Input:
    void   *event           pointer to the event
    void   *pdata           pointer to data area _after_ user
                            data. To create a bank of two integers,
                            one would use

                            bk_create(event, "TEST", TID_INT, &pdata);
                            *pdata++ = 100;
                            *pdata++ = 200;
                            bk_close(event, pdata);

  Output:
    none

  Function value:
    none

\********************************************************************/
{
  if (((BANK_HEADER *)event)->flags & BANK_FORMAT_32BIT)
    {
    BANK32 *pbk32;

    pbk32 = (BANK32 *) ((char *) (((BANK_HEADER *) event) + 1) + ((BANK_HEADER *) event)->data_size);
    pbk32->data_size = (DWORD) (INT) pdata - (INT) (pbk32+1);
    if (pbk32->type == TID_STRUCT && pbk32->data_size == 0)
      printf("Warning: bank %c%c%c%c has zero size\n", 
        pbk32->name[0], pbk32->name[1], pbk32->name[2], pbk32->name[3]);
    ((BANK_HEADER *) event)->data_size += sizeof(BANK32) + ALIGN(pbk32->data_size);
    }
  else
    {
    BANK *pbk;

    pbk = (BANK *) ((char *) (((BANK_HEADER *) event) + 1) + ((BANK_HEADER *) event)->data_size);
    pbk->data_size = (WORD) (INT) pdata - (INT) (pbk+1);
    if (pbk->type == TID_STRUCT && pbk->data_size == 0)
      printf("Warning: bank %c%c%c%c has zero size\n", 
        pbk->name[0], pbk->name[1], pbk->name[2], pbk->name[3]);
    ((BANK_HEADER *) event)->data_size += sizeof(BANK) + ALIGN(pbk->data_size);
    }
}

/*------------------------------------------------------------------*/

INT bk_locate(void *event, char *name, void *pdata)
/********************************************************************\

  Routine: bk_locate

  Purpose: Locate a MIDAS bank of given name inside an event

  Input:
    void   *event           pointer to the event
    char   *name            Name of bank (exactly four letters)

  Output:
    void   *pdata           Pointer to data area of bank, NULL
                            if bank was not found.

  Function value:
    INT    number of items inside bank

\********************************************************************/
{
BANK   *pbk;
BANK32 *pbk32;
DWORD  dname;

  if (bk_is32(event))
    {
    pbk32 = (BANK32 *) (((BANK_HEADER *) event)+1);
    strncpy((char *) &dname, name, 4);
    do
      {
      if (*((DWORD *) pbk32->name) == dname)
        {
        *((void **)pdata) = pbk32+1;
        if (tid_size[pbk32->type & 0xFF] == 0)
          return pbk32->data_size;
        return pbk32->data_size / tid_size[pbk32->type & 0xFF];
        }
      pbk32 = (BANK32 *) ((char *) (pbk32 + 1) + ALIGN(pbk32->data_size));
      } while ((DWORD) pbk32 - (DWORD) event < ((BANK_HEADER *) event)->data_size + sizeof(BANK_HEADER));
    }
  else
    {
    pbk = (BANK *) (((BANK_HEADER *) event)+1);
    strncpy((char *) &dname, name, 4);
    do
      {
      if (*((DWORD *) pbk->name) == dname)
        {
        *((void **)pdata) = pbk+1;
        if (tid_size[pbk->type & 0xFF] == 0)
          return pbk->data_size;
        return pbk->data_size / tid_size[pbk->type & 0xFF];
        }
      pbk = (BANK *) ((char *) (pbk + 1) + ALIGN(pbk->data_size));
      } while ((DWORD) pbk - (DWORD) event < ((BANK_HEADER *) event)->data_size + sizeof(BANK_HEADER));
    }

  /* bank not found */
  *((void **)pdata) = NULL;
  return 0;
}

/*------------------------------------------------------------------*/

INT bk_iterate(void *event, BANK **pbk, void *pdata)
/********************************************************************\

  Routine: bk_iterate

  Purpose: Iterate through MIDAS banks inside an event

  Input:
    void   *event           pointer to the event
    BANK   **pbk            must be NULL for the first call to bk_iterate

  Output:
    BANK   **pbk            pointer to the bank header
    void   *pdata           pointer to data area of the bank

  Function value:
    INT    size of the bank in bytes

\********************************************************************/
{
  if (*pbk == NULL)
    *pbk = (BANK *) (((BANK_HEADER *) event)+1);
  else
    *pbk = (BANK *) ((char *) (*pbk + 1) + ALIGN((*pbk)->data_size));

  *((void **)pdata) = (*pbk)+1;

  if ((DWORD) *pbk - (DWORD) event >= ((BANK_HEADER *) event)->data_size + sizeof(BANK_HEADER))
    {
    *pbk = *((void **)pdata) = NULL;
    return 0;
    }

  return (*pbk)->data_size;
}

/*------------------------------------------------------------------*/

INT bk_iterate32(void *event, BANK32 **pbk, void *pdata)
/********************************************************************\

  Routine: bk_iterate

  Purpose: Iterate through 32 bit MIDAS banks inside an event

  Input:
    void   *event           pointer to the event
    BANK   **pbk32          must be NULL for the first call to bk_iterate

  Output:
    BANK   **pbk            pointer to the bank header
    void   *pdata           pointer to data area of the bank

  Function value:
    INT    size of the bank in bytes

\********************************************************************/
{
  if (*pbk == NULL)
    *pbk = (BANK32 *) (((BANK_HEADER *) event)+1);
  else
    *pbk = (BANK32 *) ((char *) (*pbk + 1) + ALIGN((*pbk)->data_size));

  *((void **)pdata) = (*pbk)+1;

  if ((DWORD) *pbk - (DWORD) event >= ((BANK_HEADER *) event)->data_size + sizeof(BANK_HEADER))
    {
    *pbk = *((void **)pdata) = NULL;
    return 0;
    }

  return (*pbk)->data_size;
}

/*------------------------------------------------------------------*/

INT bk_swap(void *event, BOOL force)
/********************************************************************\

  Routine: bk_swap

  Purpose: Swap (change big endian / little endian coding) of an
           whole event in MIDAS bank format.

  Input:
    void   *event           pointer to the event
    BOOL   force            Swap always

  Output:
    none

  Function value:
    CM_SUCCESS              Event has been swapped
    0                       Event has been not been swapped

\********************************************************************/
{
BANK_HEADER *pbh;
BANK        *pbk;
BANK32      *pbk32;
void        *pdata;
WORD        type;
BOOL        b32;

  pbh = (BANK_HEADER *) event;

  /* only swap if flags in high 16-bit */
  if (pbh->flags < 0x10000 && !force)
    return 0;

  /* swap bank header */
  DWORD_SWAP(&pbh->data_size);
  DWORD_SWAP(&pbh->flags);

  /* check for 32bit banks */
  b32 = ((pbh->flags & BANK_FORMAT_32BIT) > 0);
  
  pbk   = (BANK *) (pbh+1);
  pbk32 = (BANK32 *) pbk;

  /* scan event */
  while ((PTYPE) pbk - (PTYPE) pbh < (INT) pbh->data_size + (INT) sizeof(BANK_HEADER))
    {
    /* swap bank header */
    if (b32)
      {
      DWORD_SWAP(&pbk32->type);
      DWORD_SWAP(&pbk32->data_size);
      pdata = pbk32+1;
      type = (WORD) pbk32->type;
      }
    else
      {
      WORD_SWAP(&pbk->type);
      WORD_SWAP(&pbk->data_size);
      pdata = pbk+1;
      type = pbk->type;
      }

    /* pbk points to next bank */
    if (b32)
      {
      pbk32 = (BANK32 *) ((char *) (pbk32 + 1) + ALIGN(pbk32->data_size));
      pbk = (BANK *) pbk32;
      }
    else
      {
      pbk = (BANK *) ((char *) (pbk + 1) + ALIGN(pbk->data_size));
      pbk32 = (BANK32 *) pbk;
      }

    switch (type)
      {
      case TID_WORD :
      case TID_SHORT :
        while ((PTYPE) pdata < (PTYPE) pbk)
          {
	        WORD_SWAP(pdata);
          ((WORD *)pdata)++;
	        }
        break;

      case TID_DWORD :
      case TID_INT :
      case TID_BOOL :
      case TID_FLOAT :
        while ((PTYPE) pdata < (PTYPE) pbk)
          {
	        DWORD_SWAP(pdata);
          ((DWORD *)pdata)++;
	        }
        break;

      case TID_DOUBLE :
        while ((PTYPE) pdata < (PTYPE) pbk)
          {
	        QWORD_SWAP(pdata);
          ((double *)pdata)++;
	        }
        break;
      }
    }

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

/********************************************************************\
*                                                                    *
*                 History functions                                  *
*                                                                    *
\********************************************************************/


/*------------------------------------------------------------------*/

static HISTORY *_history;
static INT     _history_entries = 0;
static char    _hs_path_name[MAX_STRING_LENGTH];

/*------------------------------------------------------------------*/

INT hs_set_path(char *path)
/********************************************************************\

  Routine: hs_set_path

  Purpose: Sets the path for future history file accesses. Should
           be called before any other history function is called.

  Input:
    char   path             Directory where history files reside

  Output:
    none

  Function value:
    HS_SUCCESS              Successful completion

\********************************************************************/
{
  /* set path locally and remotely */
  if (rpc_is_remote())
    rpc_call(RPC_HS_SET_PATH, path);

  strcpy(_hs_path_name, path);

  /* check for trailing directory seperator */
  if (strlen(_hs_path_name) > 0 &&
      _hs_path_name[strlen(_hs_path_name)-1] != DIR_SEPARATOR)
    strcat(_hs_path_name, DIR_SEPARATOR_STR);
    
  return HS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT hs_open_file(DWORD ltime, char *suffix, INT mode, int *fh)
/********************************************************************\

  Routine: hs_open_file 

  Purpose: Open history file belonging to certain date. Internal use
           only.

  Input:
    DWORD  ltime            Date for which a history file should
                            be opened.

    char   suffix           File name suffix like "hst", "idx", "idf"

  Output:
    INT    *fh              File handle

  Function value:
    HS_SUCCESS              Successful completion

\********************************************************************/
{
struct      tm *tms;
char        file_name[256];

  /* generate new file name YYMMDD.xxx */
#if !defined(OS_VXWORKS) 
#if !defined(OS_VMS)
  tzset();
#endif
#endif
  tms = localtime((const time_t *)&ltime);

  sprintf(file_name, "%s%02d%02d%02d.%s", _hs_path_name, 
          tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday, suffix);

  /* open file, add O_BINARY flag for Windows NT */
  *fh = open(file_name, mode | O_BINARY, 0644);

  return HS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT hs_gen_index(DWORD ltime)
/********************************************************************\

  Routine: hs_gen_index

  Purpose: Regenerate index files ("idx" and "idf" files) for a given
           history file ("hst"). Interal use only.

  Input:
    DWORD  ltime            Date for which a history file should
                            be analyzed.

  Output:
    none

  Function value:
    HS_SUCCESS              Successful completion
    HS_FILE_ERROR           Index files cannot be created

\********************************************************************/
{
char         event_name[NAME_LENGTH];
int          fh, fhd, fhi;
INT          n;
HIST_RECORD  rec;
INDEX_RECORD irec;
DEF_RECORD   def_rec;

  printf("Recovering index files...\n");

  if (ltime == 0)
    ltime = time(NULL);

  /* open new index file */
  hs_open_file(ltime, "idx", O_RDWR | O_CREAT | O_TRUNC, &fhi);
  hs_open_file(ltime, "idf", O_RDWR | O_CREAT | O_TRUNC, &fhd);

  if (fhd < 0 || fhi < 0)
    {
    cm_msg(MERROR, "hs_gen_index", "cannot create index file");
    return HS_FILE_ERROR;
    }

  /* open history file */
  hs_open_file(ltime, "hst", O_RDONLY, &fh);
  if (fh < 0)
    return HS_FILE_ERROR;
  lseek(fh, 0, SEEK_SET);

  /* loop over file records in .hst file */
  do
    {
    n = read(fh, (char *)&rec, sizeof(rec));
    if (n < sizeof(rec))
      break;

    /* check if record type is definition */
    if (rec.record_type == RT_DEF)
      {
      /* read name */
      read(fh, event_name, sizeof(event_name));

      printf("Event definition %s, ID %d\n", event_name, rec.event_id);

      /* write definition index record */
      def_rec.event_id = rec.event_id;
      memcpy(def_rec.event_name, event_name, sizeof(event_name));
      def_rec.def_offset = TELL(fh)-sizeof(event_name)-sizeof(rec);
      write(fhd, (char *)&def_rec, sizeof(def_rec));

      /* skip tags */
      lseek(fh, rec.data_size, SEEK_CUR);
      }
    else
      {
      /* write index record */
      irec.event_id = rec.event_id;
      irec.time = rec.time;
      irec.offset = TELL(fh) - sizeof(rec);
      write(fhi, (char *)&irec, sizeof(irec));

      printf("ID %d, %s\n", rec.event_id, ctime((const time_t *)&irec.time)+4);

      /* skip data */
      lseek(fh, rec.data_size, SEEK_CUR);
      }

    } while (TRUE);

  close(fh);
  close(fhi);
  close(fhd);

  printf("...done.\n");

  return HS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT hs_search_file(DWORD *ltime, INT direction)
/********************************************************************\

  Routine: hs_search_file

  Purpose: Search an history file for a given date. If not found,
           look for files after date (direction==1) or before date
           (direction==-1) up to one year.

  Input:
    DWORD  *ltime           Date of history file 
    INT    direction        Search direction

  Output:
    DWORD  *ltime           Date of history file found

  Function value:
    HS_SUCCESS              Successful completion
    HS_FILE_ERROR           No file found

\********************************************************************/
{
DWORD     lt;
int       fh, fhd, fhi;
struct tm *tms;

  if (*ltime == 0)
    *ltime = time(NULL);

  lt = *ltime;
  do
    {
    /* try to open history file for date "lt" */
    hs_open_file(lt, "hst", O_RDONLY, &fh);

    /* if not found, look for next day */
    if (fh < 0)
      lt += direction*3600*24;

    /* stop if more than a year away from starting point */
    } while (fh < 0 && abs((INT)lt-(INT)*ltime) < 3600*24*365);

  if (fh < 0)
    return HS_FILE_ERROR;

  if (lt != *ltime)
    {
    /* if switched to new day, set start_time to 0:00 */
    tms = localtime((const time_t *)&lt);
    tms->tm_hour = tms->tm_min = tms->tm_sec = 0;
    *ltime = mktime(tms);
    }

  /* check if index files are there */
  hs_open_file(*ltime, "idf", O_RDONLY, &fhd);
  hs_open_file(*ltime, "idx", O_RDONLY, &fhi);

  close(fh);
  close(fhd);
  close(fhi);

  /* generate them if not */
  if (fhd < 0 || fhi < 0)
    hs_gen_index(*ltime);

  return HS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT hs_define_event(DWORD event_id, char *name, TAG *tag, 
                    DWORD size)
/********************************************************************\

  Routine: hs_define_evnet

  Purpose: Define a new event for which a history should be recorded.
           This routine must be called before any call to 
           hs_write_event. It also should be called if the definition
           of the event has changed.

           The event definition is written directly to the history
           file. If the definition is identical to a previous
           definition, it is not written to the file.


  Input:
    DWORD  event_id         ID for this event. Must be unique.
    char   name             Name of this event
    TAG    tag              Tag list containing names and types of
                            variables in this event.
    DWORD  size             Size of tag array

  Output:
    <none>

  Function value:
    HS_SUCCESS              Successful completion
    HS_NO_MEMEORY           Out of memory
    HS_FILE_ERROR           Cannot open history file

\********************************************************************/
{
/* History events are only written locally (?)

  if (rpc_is_remote())
    return rpc_call(RPC_HS_DEFINE_EVENT, event_id, name, tag, size);
*/
{
HIST_RECORD  rec, prev_rec;
DEF_RECORD   def_rec;
char         str[256], event_name[NAME_LENGTH], *buffer;
int          fh, fhi, fhd;
INT          i, n, len, index;
struct tm    *tmb;

  /* allocate new space for the new history descriptor */
  if (_history_entries == 0)
    {
    _history = malloc(sizeof(HISTORY));
    memset(_history, 0, sizeof(HISTORY));
    if (_history == NULL)
      return HS_NO_MEMORY;

    _history_entries = 1;
    index = 0;
    }
  else
    {
    /* check if history already open */
    for (i=0 ; i<_history_entries ; i++)
      if (_history[i].event_id == event_id)
        break;

    /* if not found, create new one */
    if (i == _history_entries)
      {
      _history = realloc(_history, sizeof(HISTORY) * (_history_entries+1));
      memset(&_history[_history_entries], 0, sizeof(HISTORY));

      _history_entries++;
      if (_history == NULL)
        {
        _history_entries--;
        return HS_NO_MEMORY;
        }
      }
    index = i;
    }

  /* assemble definition record header */
  rec.record_type = RT_DEF;
  rec.event_id = event_id;
  rec.time = time(NULL);
  rec.data_size = size;
  strncpy(event_name, name, NAME_LENGTH);

  /* pad tag names with zeos */
  for (i=0 ; (DWORD)i<size/sizeof(TAG) ; i++)
    {
    len = strlen(tag[i].name);
    memset(tag[i].name+len, 0, NAME_LENGTH-len);
    }

  /* if history structure not set up, do so now */
  if (!_history[index].hist_fh)
    {
    /* open history file */
    hs_open_file(rec.time, "hst", O_CREAT | O_RDWR, &fh);
    if (fh < 0)
      return HS_FILE_ERROR;

    /* open index files */
    hs_open_file(rec.time, "idf", O_CREAT | O_RDWR, &fhd);
    hs_open_file(rec.time, "idx", O_CREAT | O_RDWR, &fhi);
    lseek(fh,  0, SEEK_END);
    lseek(fhi, 0, SEEK_END);
    lseek(fhd, 0, SEEK_END);

    /* regenerate index if missing */
    if (TELL(fh) > 0 && TELL(fhd) == 0)
      {
      close(fh);
      close(fhi);
      close(fhd);
      hs_gen_index(rec.time);
      hs_open_file(rec.time, "hst", O_RDWR, &fh);
      hs_open_file(rec.time, "idx", O_RDWR, &fhi);
      hs_open_file(rec.time, "idf", O_RDWR, &fhd);
      lseek(fh, 0, SEEK_END);
      lseek(fhi, 0, SEEK_END);
      lseek(fhd, 0, SEEK_END);
      }

    tmb = localtime((const time_t *)&rec.time);
    tmb->tm_hour = tmb->tm_min = tmb->tm_sec = 0;
    
    /* setup history structure */
    _history[index].hist_fh = fh;
    _history[index].index_fh = fhi;
    _history[index].def_fh = fhd;
    _history[index].def_offset = TELL(fh);
    _history[index].event_id = event_id;
    strcpy(_history[index].event_name, event_name);
    _history[index].base_time = mktime(tmb);
    _history[index].n_tag = size/sizeof(TAG);
    _history[index].tag = malloc(size);
    memcpy(_history[index].tag, tag, size);
    
    /* search previous definition */
    n = TELL(fhd) / sizeof(def_rec);
    def_rec.event_id = 0;
    for (i=n-1 ; i>=0 ; i--)
      {
      lseek(fhd, i*sizeof(def_rec), SEEK_SET);
      read(fhd, (char *)&def_rec, sizeof(def_rec));
      if (def_rec.event_id == event_id)
        break;
      }
    lseek(fhd, 0, SEEK_END);

    /* if definition found, compare it with new one */
    if (def_rec.event_id == event_id)
      {
      buffer = malloc(size);
      memset(buffer, 0, size);

      lseek(fh, def_rec.def_offset, SEEK_SET);
      read(fh, (char *)&prev_rec, sizeof(prev_rec));
      read(fh, str, NAME_LENGTH);
      read(fh, buffer, size);
      lseek(fh, 0, SEEK_END);

      if (prev_rec.data_size != size ||
          strcmp(str, event_name) != 0 ||
          memcmp(buffer, tag, size) !=0)
        {
        /* write definition to history file */
        write(fh, (char *)&rec, sizeof(rec));
        write(fh, event_name, NAME_LENGTH);
        write(fh, (char *)tag, size);

        /* write index record */
        def_rec.event_id = event_id;
        memcpy(def_rec.event_name, event_name, sizeof(event_name));
        def_rec.def_offset = _history[index].def_offset;
        write(fhd, (char *)&def_rec, sizeof(def_rec));
        }
      else
        /* definition identical, just remember old offset */
        _history[index].def_offset = def_rec.def_offset;

      free(buffer);
      }
    else
      {
      /* write definition to history file */
      write(fh, (char *)&rec, sizeof(rec));
      write(fh, event_name, NAME_LENGTH);
      write(fh, (char *)tag, size);

      /* write definition index record */
      def_rec.event_id = event_id;
      memcpy(def_rec.event_name, event_name, sizeof(event_name));
      def_rec.def_offset = _history[index].def_offset;
      write(fhd, (char *)&def_rec, sizeof(def_rec));
      }
    }
  else
    {
    fh  = _history[index].hist_fh;
    fhd = _history[index].def_fh;

    /* compare definition with previous definition */
    buffer = malloc(size);
    memset(buffer, 0, size);

    lseek(fh, _history[index].def_offset, SEEK_SET);
    read(fh, (char *)&prev_rec, sizeof(prev_rec));
    read(fh, str, NAME_LENGTH);
    read(fh, buffer, size);
    
    lseek(fh, 0, SEEK_END);
    lseek(fhd, 0, SEEK_END);

    if (prev_rec.data_size != size ||
        strcmp(str, event_name) != 0 ||
        memcmp(buffer, tag, size) !=0)
      {
      /* save new definition offset */
      _history[index].def_offset = TELL(fh);
    
      /* write definition to history file */
      write(fh, (char *)&rec, sizeof(rec));
      write(fh, event_name, NAME_LENGTH);
      write(fh, (char *)tag, size);

      /* write index record */
      def_rec.event_id = event_id;
      memcpy(def_rec.event_name, event_name, sizeof(event_name));
      def_rec.def_offset = _history[index].def_offset;
      write(fhd, (char *)&def_rec, sizeof(def_rec));
      }

    free(buffer);
    }

  }

  return HS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT hs_write_event(DWORD event_id, void *data, DWORD size)
/********************************************************************\

  Routine: hs_write_event

  Purpose: Write an event to a history file.

  Input:
    DWORD  event_id         Event ID
    void   *data            Data buffer containing event
    DWORD  size             Data buffer size in bytes

  Output:
    none
                            future hs_write_event

  Function value:
    HS_SUCCESS              Successful completion
    HS_NO_MEMEORY           Out of memory
    HS_FILE_ERROR           Cannot write to history file
    HS_UNDEFINED_EVENT      Event was not defined via hs_define_event

\********************************************************************/
{
/* history events are only written locally (?)

  if (rpc_is_remote())
    return rpc_call(RPC_HS_WRITE_EVENT, event_id, data, size);
*/
HIST_RECORD  rec, drec;
DEF_RECORD   def_rec;
INDEX_RECORD irec;
int          fh, fhi, fhd;
INT          index;
struct tm    tmb, tmr;

  /* find index to history structure */
  for (index=0 ; index<_history_entries ; index++)
    if (_history[index].event_id == event_id)
      break;
  if (index == _history_entries)
    return HS_UNDEFINED_EVENT;

  /* assemble record header */
  rec.record_type = RT_DATA;
  rec.event_id = _history[index].event_id;
  rec.time = time(NULL);
  rec.def_offset = _history[index].def_offset;
  rec.data_size = size;
  
  irec.event_id = _history[index].event_id;
  irec.time = rec.time;
  
  /* check if new day */
  memcpy(&tmr, localtime((const time_t *)&rec.time), sizeof(tmr));
  memcpy(&tmb, localtime((const time_t *)&_history[index].base_time), sizeof(tmb));

  if (tmr.tm_yday != tmb.tm_yday)
    {
    /* close current history file */
    close(_history[index].hist_fh);
    close(_history[index].def_fh);
    close(_history[index].index_fh);

    /* open new history file */
    hs_open_file(rec.time, "hst", O_CREAT | O_RDWR, &fh);
    if (fh < 0)
      return HS_FILE_ERROR;

    /* open new index file */
    hs_open_file(rec.time, "idx", O_CREAT | O_RDWR, &fhi);
    if (fh < 0)
      return HS_FILE_ERROR;

    /* open new definition index file */
    hs_open_file(rec.time, "idf", O_CREAT | O_RDWR, &fhd);
    if (fhd < 0)
      return HS_FILE_ERROR;

    lseek(fh, 0, SEEK_END);
    lseek(fhi, 0, SEEK_END);
    lseek(fhd, 0, SEEK_END);

    /* remember new file handles */
    _history[index].hist_fh = fh;
    _history[index].index_fh = fhi;
    _history[index].def_fh = fhd;

    _history[index].def_offset = TELL(fh);
    rec.def_offset = _history[index].def_offset;

    tmr.tm_hour = tmr.tm_min = tmr.tm_sec = 0;
    _history[index].base_time = mktime(&tmr);

    /* write definition from _history structure */
    drec.record_type = RT_DEF;
    drec.event_id = _history[index].event_id;
    drec.time = rec.time;
    drec.data_size = _history[index].n_tag * sizeof(TAG);

    write(fh, (char *)&drec, sizeof(drec));
    write(fh, _history[index].event_name, NAME_LENGTH);
    write(fh, (char *)_history[index].tag, drec.data_size);

    /* write definition index record */
    def_rec.event_id = _history[index].event_id;
    memcpy(def_rec.event_name, _history[index].event_name, sizeof(def_rec.event_name));
    def_rec.def_offset = _history[index].def_offset;
    write(fhd, (char *)&def_rec, sizeof(def_rec));
    }
  
  /* got to end of file */
  lseek(_history[index].hist_fh, 0, SEEK_END);
  irec.offset = TELL(_history[index].hist_fh);

  /* write record header */
  write(_history[index].hist_fh, (char *)&rec, sizeof(rec));
  
  /* write data */
  write(_history[index].hist_fh, (char *)data, size);

  /* write index record */
  lseek(_history[index].index_fh, 0, SEEK_END);
  if (write(_history[index].index_fh, (char *)&irec, sizeof(irec)) < sizeof(irec))
    return HS_FILE_ERROR;
 
  return HS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT hs_enum_events(DWORD ltime, char *event_name, DWORD *name_size,
                                INT  event_id[], DWORD *id_size)
/********************************************************************\

  Routine: hs_enum_events

  Purpose: Enumerate events for a given date

  Input:
    DWORD  ltime            Date at which events should be enumerated

  Output:
    char   *event_name      Array containing event names
    DWORD  *name_size       Size of name array
    char   *event_id        Array containing event IDs
    DWORD  *id_size         Size of ID array

  Function value:
    HS_SUCCESS              Successful completion
    HS_NO_MEMEORY           Out of memory
    HS_FILE_ERROR           Cannot open history file

\********************************************************************/
{
int        fh, fhd;
INT        status, i, j, n;
DEF_RECORD def_rec;

  if (rpc_is_remote())
    return rpc_call(RPC_HS_ENUM_EVENTS, ltime, event_name, name_size, event_id, id_size);

  /* search latest history file */
  status = hs_search_file(&ltime, -1);
  if (status != HS_SUCCESS)
    {
    cm_msg(MERROR, "hs_enum_events", "cannot find recent history file");
    return HS_FILE_ERROR;
    }

  /* open history and definition files */
  hs_open_file(ltime, "hst", O_RDONLY, &fh);
  hs_open_file(ltime, "idf", O_RDONLY, &fhd);
  if (fh< 0 || fhd < 0)
    {
    cm_msg(MERROR, "hs_enum_events", "cannot open index files");
    return HS_FILE_ERROR;
    }
  lseek(fhd, 0, SEEK_SET);

  /* loop over definition index file */
  n = 0;
  do
    {
    /* read event definition */
    j = read(fhd, (char *)&def_rec, sizeof(def_rec));
    if (j < (int) sizeof(def_rec))
      break;

    /* look for existing entry for this event id */
    for (i=0 ; i<n ; i++)
      if (event_id[i] == (INT)def_rec.event_id)
        {
        strcpy(event_name+i*NAME_LENGTH, def_rec.event_name);
        break;
        }

    /* new entry found */
    if (i == n)
      {
      if (i*NAME_LENGTH > (INT) *name_size ||
          i*sizeof(INT) > (INT) *id_size)
        {
        cm_msg(MERROR, "hs_enum_events", "index buffer too small");
        close(fh);
        close(fhd);
        return HS_NO_MEMORY;
        }

      /* copy definition record */
      strcpy(event_name+i*NAME_LENGTH, def_rec.event_name);
      event_id[i] = def_rec.event_id;
      n++;
      }
    } while (TRUE);

  close(fh);
  close(fhd);
  *name_size = n*NAME_LENGTH;
  *id_size = n*sizeof(INT);
  
  return HS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT hs_count_events(DWORD ltime, DWORD *count)
/********************************************************************\

  Routine: hs_count_events

  Purpose: Count number of different events for a given date

  Input:
    DWORD  ltime            Date at which events should be counted

  Output:
    DWORD  *count           Number of different events found

  Function value:
    HS_SUCCESS              Successful completion
    HS_FILE_ERROR           Cannot open history file

\********************************************************************/
{
int        fh, fhd;
INT        status, i, j, n;
DWORD      *id;
DEF_RECORD def_rec;

  if (rpc_is_remote())
    return rpc_call(RPC_HS_COUNT_EVENTS, ltime, count);

  /* search latest history file */
  status = hs_search_file(&ltime, -1);
  if (status != HS_SUCCESS)
    {
    cm_msg(MERROR, "hs_count_events", "cannot find recent history file");
    return HS_FILE_ERROR;
    }

  /* open history and definition files */
  hs_open_file(ltime, "hst", O_RDONLY, &fh);
  hs_open_file(ltime, "idf", O_RDONLY, &fhd);
  if (fh< 0 || fhd < 0)
    {
    cm_msg(MERROR, "hs_count_events", "cannot open index files");
    return HS_FILE_ERROR;
    }

  /* allocate event id array */
  lseek(fhd, 0, SEEK_END);
  id = malloc(TELL(fhd)/sizeof(def_rec)*sizeof(DWORD));
  lseek(fhd, 0, SEEK_SET);

  /* loop over index file */
  n = 0;
  do
    {
    /* read definition index record */
    j = read(fhd, (char *)&def_rec, sizeof(def_rec));
    if (j < (int) sizeof(def_rec))
      break;

    /* look for existing entries */
    for (i=0 ; i<n ; i++)
      if (id[i] == def_rec.event_id)
        break;

    /* new entry found */
    if (i == n)
      {
      id[i] = def_rec.event_id;
      n++;
      }
    } while (TRUE);


  free(id);
  close(fh);
  close(fhd);
  *count = n;
  
  return HS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT hs_get_event_id(DWORD ltime, char *name, DWORD *id)
/********************************************************************\

  Routine: hs_get_event_id

  Purpose: Return event ID for a given name

  Input:
    DWORD  ltime            Date at which event ID should be looked for

  Output:
    DWORD  *id              Event ID

  Function value:
    HS_SUCCESS              Successful completion
    HS_FILE_ERROR           Cannot open history file

\********************************************************************/
{
int        fh, fhd;
INT        status, i;
DEF_RECORD def_rec;

  if (rpc_is_remote())
    return rpc_call(RPC_HS_GET_EVENT_ID, ltime, name, id);

  /* search latest history file */
  status = hs_search_file(&ltime, -1);
  if (status != HS_SUCCESS)
    {
    cm_msg(MERROR, "hs_count_events", "cannot find recent history file");
    return HS_FILE_ERROR;
    }

  /* open history and definition files */
  hs_open_file(ltime, "hst", O_RDONLY, &fh);
  hs_open_file(ltime, "idf", O_RDONLY, &fhd);
  if (fh< 0 || fhd < 0)
    {
    cm_msg(MERROR, "hs_count_events", "cannot open index files");
    return HS_FILE_ERROR;
    }

  /* allocate event id array */
  lseek(fhd, 0, SEEK_END);
  lseek(fhd, 0, SEEK_SET);

  /* loop over index file */
  *id = 0;
  do
    {
    /* read definition index record */
    i = read(fhd, (char *)&def_rec, sizeof(def_rec));
    if (i < (int) sizeof(def_rec))
      break;

    if (strcmp(name, def_rec.event_name) == 0)
      {
      *id = def_rec.event_id;
      break;
      }
    } while (TRUE);


  close(fh);
  close(fhd);
  
  return HS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT hs_count_vars(DWORD ltime, DWORD event_id, DWORD *count)
/********************************************************************\

  Routine: hs_count_vars

  Purpose: Count number of variables for a given date and event id

  Input:
    DWORD  ltime            Date at which tags should be counted

  Output:
    DWORD  *count           Number of tags

  Function value:
    HS_SUCCESS              Successful completion
    HS_FILE_ERROR           Cannot open history file

\********************************************************************/
{
int          fh, fhd;
INT          i, n, status;
DEF_RECORD   def_rec;
HIST_RECORD  rec;

  if (rpc_is_remote())
    return rpc_call(RPC_HS_COUNT_VARS, ltime, event_id, count);

  /* search latest history file */
  status = hs_search_file(&ltime, -1);
  if (status != HS_SUCCESS)
    {
    cm_msg(MERROR, "hs_count_tags", "cannot find recent history file");
    return HS_FILE_ERROR;
    }

  /* open history and definition files */
  hs_open_file(ltime, "hst", O_RDONLY, &fh);
  hs_open_file(ltime, "idf", O_RDONLY, &fhd);
  if (fh< 0 || fhd < 0)
    {
    cm_msg(MERROR, "hs_count_tags", "cannot open index files");
    return HS_FILE_ERROR;
    }

  /* search last definition */
  lseek(fhd, 0, SEEK_END);
  n = TELL(fhd) / sizeof(def_rec);
  def_rec.event_id = 0;
  for (i=n-1 ; i>=0 ; i--)
    {
    lseek(fhd, i*sizeof(def_rec), SEEK_SET);
    read(fhd, (char *)&def_rec, sizeof(def_rec));
    if (def_rec.event_id == event_id)
      break;
    }
  if (def_rec.event_id != event_id)
    {
    cm_msg(MERROR, "hs_count_tags", "event %d not found in index file", event_id);
    return HS_FILE_ERROR;
    }

  /* read definition */
  lseek(fh, def_rec.def_offset, SEEK_SET);
  read(fh, (char *)&rec, sizeof(rec));
  *count = rec.data_size / sizeof(TAG);

  close(fh);
  close(fhd);
  
  return HS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT hs_enum_vars(DWORD ltime, DWORD event_id, char *var_name, DWORD *size)
/********************************************************************\

  Routine: hs_enum_vars

  Purpose: Enumerate variable tags for a given date and event id

  Input:
    DWORD  ltime            Date at which tags should be enumerated
    DWORD  event_id         Event ID

  Output:
    char   *var_name        Array containing variable names
    DWORD  *size            Size of name array

  Function value:
    HS_SUCCESS              Successful completion
    HS_NO_MEMEORY           Out of memory
    HS_FILE_ERROR           Cannot open history file

\********************************************************************/
{
char         str[256];
int          fh, fhd;
INT          i, n, status;
DEF_RECORD   def_rec;
HIST_RECORD  rec;
TAG          *tag;

  if (rpc_is_remote())
    return rpc_call(RPC_HS_ENUM_VARS, ltime, event_id, var_name, size);

  /* search latest history file */
  status = hs_search_file(&ltime, -1);
  if (status != HS_SUCCESS)
    {
    cm_msg(MERROR, "hs_enum_tags", "cannot find recent history file");
    return HS_FILE_ERROR;
    }

  /* open history and definition files */
  hs_open_file(ltime, "hst", O_RDONLY, &fh);
  hs_open_file(ltime, "idf", O_RDONLY, &fhd);
  if (fh< 0 || fhd < 0)
    {
    cm_msg(MERROR, "hs_enum_tags", "cannot open index files");
    return HS_FILE_ERROR;
    }

  /* search last definition */
  lseek(fhd, 0, SEEK_END);
  n = TELL(fhd) / sizeof(def_rec);
  def_rec.event_id = 0;
  for (i=n-1 ; i>=0 ; i--)
    {
    lseek(fhd, i*sizeof(def_rec), SEEK_SET);
    read(fhd, (char *)&def_rec, sizeof(def_rec));
    if (def_rec.event_id == event_id)
      break;
    }
  if (def_rec.event_id != event_id)
    {
    cm_msg(MERROR, "hs_enum_tags", "event %d not found in index file", event_id);
    return HS_FILE_ERROR;
    }

  /* read definition header */
  lseek(fh, def_rec.def_offset, SEEK_SET);
  read(fh, (char *)&rec, sizeof(rec));
  read(fh, str, NAME_LENGTH);

  /* read event definition */
  n = rec.data_size/sizeof(TAG);
  tag = malloc(rec.data_size);
  read(fh, (char *)tag, rec.data_size);

  if (n*NAME_LENGTH > (INT)*size)
    {
    /* store partial definition */
    for (i=0 ; i<(INT)*size/NAME_LENGTH ; i++)
      strcpy(var_name+i*NAME_LENGTH, tag[i].name);

    cm_msg(MERROR, "hs_enum_tags", "tag buffer too small");
    free(tag);
    close(fh);
    close(fhd);
    return HS_NO_MEMORY;
    }

  /* store full definition */
  for (i=0 ; i<n ; i++)
    strcpy(var_name+i*NAME_LENGTH, tag[i].name);
  *size = n*NAME_LENGTH;

  free(tag);
  close(fh);
  close(fhd);
  
  return HS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT hs_get_var(DWORD ltime, DWORD event_id, char *var_name, DWORD *type, INT *n_data)
/********************************************************************\

  Routine: hs_get_var

  Purpose: Get definition for certain variable

  Input:
    DWORD  ltime            Date at which variable definition should 
                            be returned
    DWORD  event_id         Event ID
    char   *var_name        Name of variable

  Output:
    INT    *type            Type of variable
    INT    *n_data          Number of items in variable

  Function value:
    HS_SUCCESS              Successful completion
    HS_NO_MEMEORY           Out of memory
    HS_FILE_ERROR           Cannot open history file

\********************************************************************/
{
char         str[256];
int          fh, fhd;
INT          i, n, status;
DEF_RECORD   def_rec;
HIST_RECORD  rec;
TAG          *tag;

  if (rpc_is_remote())
    return rpc_call(RPC_HS_GET_VAR, ltime, event_id, var_name, type, n_data);

  /* search latest history file */
  status = hs_search_file(&ltime, -1);
  if (status != HS_SUCCESS)
    {
    cm_msg(MERROR, "hs_enum_tags", "cannot find recent history file");
    return HS_FILE_ERROR;
    }

  /* open history and definition files */
  hs_open_file(ltime, "hst", O_RDONLY, &fh);
  hs_open_file(ltime, "idf", O_RDONLY, &fhd);
  if (fh< 0 || fhd < 0)
    {
    cm_msg(MERROR, "hs_enum_tags", "cannot open index files");
    return HS_FILE_ERROR;
    }

  /* search last definition */
  lseek(fhd, 0, SEEK_END);
  n = TELL(fhd) / sizeof(def_rec);
  def_rec.event_id = 0;
  for (i=n-1 ; i>=0 ; i--)
    {
    lseek(fhd, i*sizeof(def_rec), SEEK_SET);
    read(fhd, (char *)&def_rec, sizeof(def_rec));
    if (def_rec.event_id == event_id)
      break;
    }
  if (def_rec.event_id != event_id)
    {
    cm_msg(MERROR, "hs_enum_tags", "event %d not found in index file", event_id);
    return HS_FILE_ERROR;
    }

  /* read definition header */
  lseek(fh, def_rec.def_offset, SEEK_SET);
  read(fh, (char *)&rec, sizeof(rec));
  read(fh, str, NAME_LENGTH);

  /* read event definition */
  n = rec.data_size/sizeof(TAG);
  tag = malloc(rec.data_size);
  read(fh, (char *)tag, rec.data_size);

  /* search variable */
  for (i=0 ; i<n ; i++)
    if (strcmp(tag[i].name, var_name) == 0)
      break;

  close(fh);
  close(fhd);

  if (i<n)
    {
    *type = tag[i].type;
    *n_data = tag[i].n_data;
    }
  else
    {
    *type = *n_data = 0;
    cm_msg(MERROR, "hs_get_var", "variable %s not found", var_name);
    free(tag);
    return HS_UNDEFINED_VAR;
    }

  free(tag);
  return HS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT hs_read(DWORD event_id, DWORD start_time, DWORD end_time, 
            DWORD interval, char *tag_name, DWORD var_index, 
            DWORD *time_buffer, DWORD *tbsize,
            void *data_buffer, DWORD *dbsize, 
            DWORD *type, DWORD *n)
/********************************************************************\

  Routine: hs_read

  Purpose: Read history for a variable at a certain time interval

  Input:
    DWORD  event_id         Event ID
    DWORD  start_time       Starting Date/Time
    DWORD  end_time         End Date/Time
    DWORD  interval         Minimum time in seconds between reported 
                            events. Can be used to skip events
    char   *tag_name        Variable name inside event
    DWORD  var_index        Index if variable is array

  Output:
    DWORD  *time_buffer     Buffer containing times for each value
    DWORD  *tbsize          Size of time buffer
    void   *data_buffer     Buffer containing variable values
    DWORD  *dbsize          Data buffer size
    DWORD  *type            Type of variable (one of TID_xxx)
    DWORD  *n               Number of time/value pairs found
                            in specified interval and placed into
                            time_buffer and data_buffer


  Function value:
    HS_SUCCESS              Successful completion
    HS_NO_MEMEORY           Out of memory
    HS_FILE_ERROR           Cannot open history file
    HS_WRONG_INDEX          var_index exceeds array size of variable
    HS_UNDEFINED_VAR        Variable "tag_name" not found in event 
    HS_TRUNCATED            Buffer too small, data has been truncated

\********************************************************************/
{
DWORD        prev_time, last_irec_time;
int          fh, fhd, fhi;
INT          i, delta, index, status;
INDEX_RECORD irec;
HIST_RECORD  rec, drec;
INT          old_def_offset, var_size, var_offset;
TAG          *tag;
char         str[NAME_LENGTH];
struct tm    *tms;

  if (rpc_is_remote())
    return rpc_call(RPC_HS_READ, event_id, start_time, end_time, interval, 
                    tag_name, var_index, time_buffer, tbsize, data_buffer, 
                    dbsize, type, n);

  /* if not time given, use present to one hour in past */
  if (start_time == 0)
    start_time = time(NULL)-3600;
  if (end_time == 0)
    end_time = time(NULL);

  /* search history file for start_time */
  status = hs_search_file(&start_time, 1);
  if (status != HS_SUCCESS)
    {
    cm_msg(MERROR, "hs_read", "cannot find recent history file");
    *tbsize = *dbsize = *n = 0;
    return HS_FILE_ERROR;
    }

  /* open history and definition files */
  hs_open_file(start_time, "hst", O_RDONLY, &fh);
  hs_open_file(start_time, "idf", O_RDONLY, &fhd);
  hs_open_file(start_time, "idx", O_RDONLY, &fhi);
  if (fh< 0 || fhd < 0 || fhi < 0)
    {
    cm_msg(MERROR, "hs_read", "cannot open index files");
    *tbsize = *dbsize = *n = 0;
    return HS_FILE_ERROR;
    }

  /* search record closest to start time */
  lseek(fhi, 0, SEEK_END);
  delta = (TELL(fhi) / sizeof(irec)) / 2;
  lseek(fhi, delta*sizeof(irec), SEEK_SET);
  do
    {
    delta = (int) (abs(delta)/2.0+0.5);
    read(fhi, (char *)&irec, sizeof(irec));
    if (irec.time > start_time)
      delta = -delta;

    lseek(fhi, (delta-1)*sizeof(irec), SEEK_CUR);
    } while (abs(delta) > 1 && irec.time != start_time);
  read(fhi, (char *)&irec, sizeof(irec));
  if (irec.time > start_time)
    delta = -abs(delta);
  lseek(fhi, (delta-1)*sizeof(irec), SEEK_CUR);
  read(fhi, (char *)&irec, sizeof(irec));

  /* read records, skip wrong IDs */
  old_def_offset = -1;
  *n = 0;
  prev_time = 0;
  do
    {
    last_irec_time = irec.time;
    if (irec.event_id == event_id &&
        irec.time <= end_time &&
        irec.time >= start_time)
      {
      /* check if record time more than "interval" seconds after previous time */
      if  (irec.time >= prev_time + interval)
        {
        prev_time = irec.time;
        lseek(fh, irec.offset, SEEK_SET);
        read(fh, (char *)&rec, sizeof(rec));

        /* if definition changed, read new definition */
        if ((INT) rec.def_offset != old_def_offset)
          {
          lseek(fh, rec.def_offset, SEEK_SET);
          read(fh, (char *)&drec, sizeof(drec));
          read(fh, str, NAME_LENGTH);

          tag = malloc(drec.data_size);
          if (tag == NULL)
            {
            *n = *tbsize = *dbsize = 0;
            return HS_NO_MEMORY;
            }
          read(fh, (char *)tag, drec.data_size);
        
          /* find index of tag_name in new definition */
          index = -1;
          for (i=0 ; (DWORD)i < drec.data_size/sizeof(TAG) ; i++)
            if (equal_ustring(tag[i].name, tag_name))
              {
              index = i;
              break;
              }
          
          if ((DWORD) i == drec.data_size/sizeof(TAG))
            {
            *n = *tbsize = *dbsize = 0;
            return HS_UNDEFINED_VAR;
            }
          
          if (var_index >= tag[i].n_data)
            {
            *n = *tbsize = *dbsize = 0;
            return HS_WRONG_INDEX;
            }

          /* calculate offset for variable */
          if (index >= 0)
            {
            *type = tag[i].type;

            /* loop over all previous variables */
            for (i=0,var_offset=0 ; i<index ; i++)
              var_offset += rpc_tid_size(tag[i].type)*tag[i].n_data;

            /* strings have size n_data */
            if (tag[index].type == TID_STRING)
              var_size = tag[i].n_data;
            else
              var_size = rpc_tid_size(tag[index].type);

            var_offset += var_size*var_index;
            }
        
          free(tag);
          old_def_offset = rec.def_offset;
          lseek(fh, irec.offset+sizeof(rec), SEEK_SET);
          }

        if (index >= 0)
          {
          /* check if data fits in buffers */
          if ((*n) * sizeof(DWORD) >= *tbsize ||
              (*n) * var_size >= *dbsize)
            {
            *dbsize = (*n) * var_size;
            *tbsize = (*n) * sizeof(DWORD);
            return HS_TRUNCATED;
            }

          /* copy time from header */
          time_buffer[*n] = irec.time;

          /* copy data from record */
          lseek(fh, var_offset, SEEK_CUR);
          read(fh, (char *) data_buffer + (*n) * var_size, var_size);

          /* increment counter */
          (*n)++;
          }
        }
      }

    /* read next index record */
    i = read(fhi, (char *)&irec, sizeof(irec));

    /* end of file: search next history file */
    if (i <= 0)
      {
      close(fh);
      close(fhd);
      close(fhi);

      /* advance one day */
      tms = localtime((const time_t *)&last_irec_time);
      tms->tm_hour = tms->tm_min = tms->tm_sec = 0;
      last_irec_time = mktime(tms);

      last_irec_time += 3600*24;
      if (last_irec_time > end_time)
        break;

      /* search next file */
      status = hs_search_file(&last_irec_time, 1);
      if (status != HS_SUCCESS)
        break;

      /* open history and definition files */
      hs_open_file(last_irec_time, "hst", O_RDONLY, &fh);
      hs_open_file(last_irec_time, "idf", O_RDONLY, &fhd);
      hs_open_file(last_irec_time, "idx", O_RDONLY, &fhi);
      if (fh< 0 || fhd < 0 || fhi < 0)
        {
        cm_msg(MERROR, "hs_read", "cannot open index files");
        break;
        }

      /* read first record */
      i = read(fhi, (char *)&irec, sizeof(irec));
      if (i <= 0)
        break;

      /* old definition becomes invalid */
      old_def_offset = -1;
      }
    } while (irec.time < end_time);

  *dbsize = *n * var_size;
  *tbsize = *n * sizeof(DWORD);

  return HS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT hs_dump(DWORD event_id, DWORD start_time, DWORD end_time,
            DWORD interval)
/********************************************************************\

  Routine: hs_dump

  Purpose: Display history for a given event at stdout. The output
           can be redirected to be read by Excel for example.

  Input:
    DWORD  event_id         Event ID
    DWORD  start_time       Starting Date/Time
    DWORD  end_time         End Date/Time
    DWORD  interval         Minimum time in seconds between reported 
                            events. Can be used to skip events

  Output:
    <screen output>

  Function value:
    HS_SUCCESS              Successful completion
    HS_FILE_ERROR           Cannot open history file

\********************************************************************/
{
DWORD        prev_time, last_irec_time;
int          fh, fhd, fhi;
INT          i, j, delta, status, n_tag, old_n_tag;
INDEX_RECORD irec;
HIST_RECORD  rec, drec;
INT          old_def_offset, offset;
TAG          *tag = NULL, *old_tag = NULL;
char         str[NAME_LENGTH], data_buffer[10000];
struct tm    *tms;

  /* if not time given, use present to one hour in past */
  if (start_time == 0)
    start_time = time(NULL)-3600;
  if (end_time == 0)
    end_time = time(NULL);

  /* search history file for start_time */
  status = hs_search_file(&start_time, 1);
  if (status != HS_SUCCESS)
    {
    cm_msg(MERROR, "hs_dump", "cannot find recent history file");
    return HS_FILE_ERROR;
    }

  /* open history and definition files */
  hs_open_file(start_time, "hst", O_RDONLY, &fh);
  hs_open_file(start_time, "idf", O_RDONLY, &fhd);
  hs_open_file(start_time, "idx", O_RDONLY, &fhi);
  if (fh< 0 || fhd < 0 || fhi < 0)
    {
    cm_msg(MERROR, "hs_dump", "cannot open index files");
    return HS_FILE_ERROR;
    }

  /* search record closest to start time */
  lseek(fhi, 0, SEEK_END);
  delta = (TELL(fhi) / sizeof(irec)) / 2;
  lseek(fhi, delta*sizeof(irec), SEEK_SET);
  do
    {
    delta = (int) (abs(delta)/2.0+0.5);
    read(fhi, (char *)&irec, sizeof(irec));
    if (irec.time > start_time)
      delta = -delta;

    lseek(fhi, (delta-1)*sizeof(irec), SEEK_CUR);
    } while (abs(delta) > 1 && irec.time != start_time);
  read(fhi, (char *)&irec, sizeof(irec));
  if (irec.time > start_time)
    delta = -abs(delta);
  lseek(fhi, (delta-1)*sizeof(irec), SEEK_CUR);
  read(fhi, (char *)&irec, sizeof(irec));

  /* read records, skip wrong IDs */
  old_def_offset = -1;
  prev_time = 0;
  do
    {
    last_irec_time = irec.time;
    if (irec.event_id == event_id &&
        irec.time <= end_time &&
        irec.time >= start_time)
      {
      if  (irec.time >= prev_time + interval)
        {
        prev_time = irec.time;
        lseek(fh, irec.offset, SEEK_SET);
        read(fh, (char *)&rec, sizeof(rec));

        /* if definition changed, read new definition */
        if ((INT) rec.def_offset != old_def_offset)
          {
          lseek(fh, rec.def_offset, SEEK_SET);
          read(fh, (char *)&drec, sizeof(drec));
          read(fh, str, NAME_LENGTH);

          if (tag == NULL)
            tag = malloc(drec.data_size);
          else
            tag = realloc(tag, drec.data_size);
          if (tag == NULL)
            return HS_NO_MEMORY;
          read(fh, (char *)tag, drec.data_size);
          n_tag = drec.data_size / sizeof(TAG);

          /* print tag names if definition has changed */
          if (old_tag == NULL || old_n_tag != n_tag ||
              memcmp(old_tag, tag, drec.data_size) != 0)
            {
            printf("Date\t");
            for (i=0 ; i<n_tag ; i++)
              {
              if (tag[i].n_data == 1 || tag[i].type == TID_STRING) 
                printf("%s\t", tag[i].name);
              else
                for (j=0 ; j<(INT)tag[i].n_data ; j++)
                  printf("%s%d\t", tag[i].name, j);
              }
            printf("\n");

            if (old_tag == NULL)
              old_tag = malloc(drec.data_size);
            else
              old_tag = realloc(old_tag, drec.data_size);
            memcpy(old_tag, tag, drec.data_size);
            old_n_tag = n_tag;
            }

          old_def_offset = rec.def_offset;
          lseek(fh, irec.offset+sizeof(rec), SEEK_SET);
          }

        /* print time from header */
        sprintf(str, "%s", ctime((const time_t *)&irec.time)+4);
        str[20] = '\t';
        printf(str);

        /* read data */
        read(fh, data_buffer, rec.data_size);

        /* interprete data from tag definition */
        offset = 0;
        for (i=0 ; i<n_tag ; i++)
          {
          /* strings have a length of n_data */
          if (tag[i].type == TID_STRING)
            {
            printf("%s\t", data_buffer+offset);
            offset += tag[i].n_data;
            }
          else if (tag[i].n_data == 1)
            {
            /* non-array data */
            db_sprintf(str, data_buffer + offset, rpc_tid_size(tag[i].type), 0, tag[i].type);
            printf("%s\t", str);
            offset += rpc_tid_size(tag[i].type);
            }
          else
            /* loop over array data */
            for (j=0 ; j<(INT)tag[i].n_data ; j++)
              {
              db_sprintf(str, data_buffer + offset, rpc_tid_size(tag[i].type), 0, tag[i].type);
              printf("%s\t", str);
              offset += rpc_tid_size(tag[i].type);
              }
          }
        printf("\n");
        }
      }

    /* read next index record */
    i = read(fhi, (char *)&irec, sizeof(irec));

    /* end of file: search next history file */
    if (i <= 0)
      {
      close(fh);
      close(fhd);
      close(fhi);

      /* advance one day */
      tms = localtime((const time_t *)&last_irec_time);
      tms->tm_hour = tms->tm_min = tms->tm_sec = 0;
      last_irec_time = mktime(tms);

      last_irec_time += 3600*24;
      if (last_irec_time > end_time)
        break;

      /* search next file */
      status = hs_search_file(&last_irec_time, 1);
      if (status != HS_SUCCESS)
        break;

      /* open history and definition files */
      hs_open_file(last_irec_time, "hst", O_RDONLY, &fh);
      hs_open_file(last_irec_time, "idf", O_RDONLY, &fhd);
      hs_open_file(last_irec_time, "idx", O_RDONLY, &fhi);
      if (fh< 0 || fhd < 0 || fhi < 0)
        {
        cm_msg(MERROR, "hs_dump", "cannot open index files");
        break;
        }

      /* read first record */
      i = read(fhi, (char *)&irec, sizeof(irec));
      if (i <= 0)
        break;

      /* old definition becomes invalid */
      old_def_offset = -1;
      }
    } while (irec.time < end_time);

  free(tag);
  free(old_tag);

  return HS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT hs_fdump(char *file_name, DWORD id)
/********************************************************************\

  Routine: hs_fdump

  Purpose: Display history for a given history file

  Input:
    char   *file_name       Name of file to dump

  Output:
    <screen output>

  Function value:
    HS_SUCCESS              Successful completion
    HS_FILE_ERROR           Cannot open history file

\********************************************************************/
{
int          fh;
INT          n;
HIST_RECORD  rec;
char         event_name[NAME_LENGTH];
char         str[80];

  /* open file, add O_BINARY flag for Windows NT */
  fh = open(file_name, O_RDONLY | O_BINARY, 0644);
  if (fh < 0)
    {
    cm_msg(MERROR, "hs_fdump", "cannot open file %s", file_name);
    return HS_FILE_ERROR;
    }

  /* loop over file records in .hst file */
  do
    {
    n = read(fh, (char *)&rec, sizeof(rec));
    if (n < sizeof(rec))
      break;

    /* check if record type is definition */
    if (rec.record_type == RT_DEF)
      {
      /* read name */
      read(fh, event_name, sizeof(event_name));

      if (rec.event_id == id || id == 0)
        printf("Event definition %s, ID %d\n", event_name, rec.event_id);

      /* skip tags */
      lseek(fh, rec.data_size, SEEK_CUR);
      }
    else
      {
      /* print data record */
      strcpy(str, ctime((const time_t *)&rec.time)+4);
      str[15] = 0;
      if (rec.event_id == id || id == 0)
        printf("ID %d, %s, size %d\n", rec.event_id, str, rec.data_size);

      /* skip data */
      lseek(fh, rec.data_size, SEEK_CUR);
      }

    } while (TRUE);

  close(fh);

  return HS_SUCCESS;
}

/*------------------------------------------------------------------*/

/********************************************************************\
*                                                                    *
*               Electronic logbook functions                         *
*                                                                    *
\********************************************************************/

void el_decode(char *message, char *key, char *result)
{
char *pc;

  if (result == NULL)
    return;

  *result = 0;

  if (strstr(message, key))
    {
    for (pc=strstr(message, key)+strlen(key) ; *pc != '\n' ; )
      *result++ = *pc++;
    *result = 0;
    }
}

INT el_submit(int run, char *author, char *type, char *system, char *subject, 
              char *text, char *reply_to, char *encoding, 
              char *afilename1, char *buffer1, INT buffer_size1, 
              char *afilename2, char *buffer2, INT buffer_size2, 
              char *afilename3, char *buffer3, INT buffer_size3, 
              char *tag, INT tag_size)
/********************************************************************\

  Routine: el_submit

  Purpose: Submit an ELog entry

  Input:
    int    run              Run number
    char   *author          Message author
    char   *type            Message type
    char   *system          Message system
    char   *subject         Subject
    char   *text            Message text
    char   *reply_to        In reply to this message
    char   *encoding        Text encoding, either HTML or plain

    char   *afilename1/2/3  File name of attachment
    char   *buffer1/2/3     File contents
    INT    *buffer_size1/2/3 Size of buffer in bytes
    char   *tag             If given, edit existing message
    INT    *tag_size        Maximum size of tag

  Output:
    char   *tag             Message tag in the form YYMMDD.offset
    INT    *tag_size        Size of returned tag

  Function value:
    EL_SUCCESS              Successful completion

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_EL_SUBMIT, run, author, type, system, subject,
                    text, reply_to, encoding, 
                    afilename1, buffer1, buffer_size1,
                    afilename2, buffer2, buffer_size2,
                    afilename3, buffer3, buffer_size3,
                    tag, tag_size);

#ifdef LOCAL_ROUTINES
{
INT     n, size, fh, status, run_number, mutex, buffer_size, index, offset, tail_size;
struct  tm *tms;
char    afilename[256], file_name[256], afile_name[3][256], dir[256], str[256], 
        start_str[80], end_str[80], last[80], date[80], thread[80], attachment[256];
HNDLE   hDB;
time_t  now;
char    message[10000], *p, *buffer;
BOOL    bedit;

  cm_get_experiment_database(&hDB, NULL);

  bedit = (tag[0] != 0);

  /* request semaphore */
  cm_get_experiment_mutex(NULL, &mutex);
  ss_mutex_wait_for(mutex, 0);

  /* get run number from ODB if not given */
  if (run > 0)
    run_number = run;
  else
    {
    /* get run number */
    size = sizeof(run_number);
    db_get_value(hDB, 0, "/Runinfo/Run number", &run_number, &size, TID_INT);
    }

  for (index = 0 ; index < 3 ; index++)
    {
    /* generate filename for attachment */
    afile_name[index][0] = file_name[0] = 0;

    if (index == 0)
      {
      strcpy(afilename, afilename1);
      buffer = buffer1;
      buffer_size = buffer_size1;
      }
    else if (index == 1)
      {
      strcpy(afilename, afilename2);
      buffer = buffer2;
      buffer_size = buffer_size2;
      }
    else if (index == 2)
      {
      strcpy(afilename, afilename3);
      buffer = buffer3;
      buffer_size = buffer_size3;
      }

    if (afilename[0])
      {
      strcpy(file_name, afilename);
      p = file_name;
      while (strchr(p, ':'))
        p = strchr(p, ':')+1;
      while (strchr(p, '\\'))
        p = strchr(p, '\\')+1; /* NT */
      while (strchr(p, '/'))
        p = strchr(p, '/')+1;  /* Unix */
      while (strchr(p, ']'))
        p = strchr(p, ']')+1;  /* VMS */

      /* assemble ELog filename */
      if (p[0])
        {
        dir[0] = 0;
        if (hDB > 0)
          {
          size = sizeof(dir);
          memset(dir, 0, size);
          db_get_value(hDB, 0, "/Logger/Data dir", dir, &size, TID_STRING);
          if (dir[0] != 0)
            if (dir[strlen(dir)-1] != DIR_SEPARATOR)
              strcat(dir, DIR_SEPARATOR_STR);
          }
	
#if !defined(OS_VXWORKS) 
#if !defined(OS_VMS)
        tzset();
#endif
#endif
	
        time(&now);
        tms = localtime(&now);

        strcpy(str, p);
        sprintf(afile_name[index], "%02d%02d%02d_%02d%02d%02d_%s",
                tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday,
                tms->tm_hour, tms->tm_min, tms->tm_sec, str);
        sprintf(file_name, "%s%02d%02d%02d_%02d%02d%02d_%s", dir, 
                tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday,
                tms->tm_hour, tms->tm_min, tms->tm_sec, str);
  
        /* save attachment */
        fh = open(file_name, O_CREAT | O_RDWR | O_BINARY, 0644);
        if (fh < 0)
          {
          cm_msg(MERROR, "el_submit", "Cannot write attachment file \"%s\"", file_name);
          }
        else
          {
          write(fh, buffer, buffer_size); 
          close(fh);
          }
        }
      }
    }

  /* generate new file name YYMMDD.log in data directory */
  cm_get_experiment_database(&hDB, NULL);
  size = sizeof(dir);
  memset(dir, 0, size);
  db_get_value(hDB, 0, "/Logger/Data dir", dir, &size, TID_STRING);
  if (dir[0] != 0)
    if (dir[strlen(dir)-1] != DIR_SEPARATOR)
      strcat(dir, DIR_SEPARATOR_STR);

#if !defined(OS_VXWORKS) 
#if !defined(OS_VMS)
  tzset();
#endif
#endif

  if (bedit)
    {
    /* edit existing message */
    strcpy(str, tag);
    if (strchr(str, '.'))
      {
      offset = atoi(strchr(str, '.')+1);
      *strchr(str, '.') = 0;
      }
    sprintf(file_name, "%s%s.log", dir, str);
    fh = open(file_name, O_CREAT | O_RDWR | O_BINARY, 0644);
    if (fh < 0)
      {
      ss_mutex_release(mutex);
      return EL_FILE_ERROR;
      }
    lseek(fh, offset, SEEK_SET);
    read(fh, str, 16);
    size = atoi(str+9);
    read(fh, message, size);

    el_decode(message, "Date: ", date);
    el_decode(message, "Thread: ", thread);
    el_decode(message, "Attachment: ", attachment);

    /* buffer tail of logfile */
    lseek(fh, 0, SEEK_END);
    tail_size = TELL(fh) - (offset+size);

    if (tail_size > 0)
      {
      buffer = malloc(tail_size);
      if (buffer == NULL)
        {
        close(fh);
        ss_mutex_release(mutex);
        return EL_FILE_ERROR;
        }

      lseek(fh, offset+size, SEEK_SET);
      n = read(fh, buffer, tail_size);
      }
    lseek(fh, offset, SEEK_SET);
    }
  else
    {
    /* create new message */
    time(&now);
    tms = localtime(&now);

    sprintf(file_name, "%s%02d%02d%02d.log", dir, 
            tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday);

    fh = open(file_name, O_CREAT | O_RDWR | O_BINARY, 0644);
    if (fh < 0)
      {
      ss_mutex_release(mutex);
      return EL_FILE_ERROR;
      }
    
    strcpy(date, ctime(&now));
    date[24] = 0;

    if (reply_to[0])
      sprintf(thread, "%16s %16s", reply_to, "0");
    else
      sprintf(thread, "%16s %16s", "0", "0");
  
    lseek(fh, 0, SEEK_END);
    }
  
  /* compose message */

  sprintf(message, "Date: %s\n", date);
  sprintf(message+strlen(message), "Thread: %s\n", thread);
  sprintf(message+strlen(message), "Run: %d\n", run_number);
  sprintf(message+strlen(message), "Author: %s\n", author);
  sprintf(message+strlen(message), "Type: %s\n", type);
  sprintf(message+strlen(message), "System: %s\n", system);
  sprintf(message+strlen(message), "Subject: %s\n", subject);

  /* keep original attachment if edit and no new attachment */
  if (bedit && afile_name[0][0] == 0 && afile_name[1][0] == 0 &&
                afile_name[2][0] == 0)
    sprintf(message+strlen(message), "Attachment: %s", attachment);
  else
    {
    sprintf(message+strlen(message), "Attachment: %s", afile_name[0]);
    if (afile_name[1][0])
      sprintf(message+strlen(message), ",%s", afile_name[1]);
    if (afile_name[2][0])
      sprintf(message+strlen(message), ",%s", afile_name[2]);
    }
  sprintf(message+strlen(message), "\n");
  
  sprintf(message+strlen(message), "Encoding: %s\n", encoding);
  sprintf(message+strlen(message), "========================================\n");
  strcat(message, text);

  size = 0;
  sprintf(start_str, "$Start$: %6d\n", size);
  sprintf(end_str,   "$End$:   %6d\n\f", size);

  size = strlen(message)+strlen(start_str)+strlen(end_str);

  if (tag != NULL && !bedit)
    sprintf(tag, "%02d%02d%02d.%d", tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday, TELL(fh));

  sprintf(start_str, "$Start$: %6d\n", size);
  sprintf(end_str,   "$End$:   %6d\n\f", size);

  write(fh, start_str, strlen(start_str));
  write(fh, message, strlen(message));
  write(fh, end_str, strlen(end_str));

  if (bedit)
    {
    if (tail_size > 0)
      {
      n = write(fh, buffer, tail_size);
      free(buffer);
      }
    
    /* truncate file here */
#ifdef OS_WINNT
    chsize(fh, TELL(fh));
#else
    ftruncate(fh, TELL(fh));
#endif
    }

  close(fh);

  /* if reply, mark original message */
  if (reply_to[0] && !bedit)
    {
    strcpy(last, reply_to);
    do
      {
      status = el_search_message(last, &fh, FALSE);
      if (status == EL_SUCCESS)
        {
        /* position to next thread location */
        lseek(fh, 63, SEEK_CUR);
        memset(str, 0, sizeof(str));
        read(fh, str, 16);
        lseek(fh, -16, SEEK_CUR);

        /* if no reply yet, set it */
        if (atoi(str) == 0)
          {
          sprintf(str, "%16s", tag);
          write(fh, str, 16);
          close(fh);
          break;
          }
        else
          {
          /* if reply set, find last one in chain */
          strcpy(last, strtok(str, " "));
          close(fh);
          }
        }
      else
        /* stop on error */
        break;

      } while (TRUE);
    }

  /* release elog mutex */
  ss_mutex_release(mutex);
}
#endif /* LOCAL_ROUTINES */

  return EL_SUCCESS;
}

/*------------------------------------------------------------------*/

INT el_search_message(char *tag, int *fh, BOOL walk)
{
int    i, size, offset, direction, last, status;
struct tm *tms, ltms;
DWORD  lt, ltime, lact;
char   str[256], file_name[256], dir[256];
HNDLE  hDB;

#if !defined(OS_VXWORKS) 
#if !defined(OS_VMS)
  tzset();
#endif
#endif

  /* open file */
  cm_get_experiment_database(&hDB, NULL);
  size = sizeof(dir);
  memset(dir, 0, size);
  db_get_value(hDB, 0, "/Logger/Data dir", dir, &size, TID_STRING);
  if (dir[0] != 0)
    if (dir[strlen(dir)-1] != DIR_SEPARATOR)
      strcat(dir, DIR_SEPARATOR_STR);

  /* check tag for direction */
  direction = 0;
  if (strpbrk(tag, "+-"))
    {
    direction = atoi(strpbrk(tag, "+-"));
    *strpbrk(tag, "+-") = 0;
    }

  /* if tag is given, open file directly */
  if (tag[0])
    {
    /* extract time structure from tag */
    tms = &ltms;
    memset(tms, 0, sizeof(struct tm));
    tms->tm_year = (tag[0]-'0')*10 + (tag[1]-'0');
    tms->tm_mon  = (tag[2]-'0')*10 + (tag[3]-'0') -1;
    tms->tm_mday = (tag[4]-'0')*10 + (tag[5]-'0');
    tms->tm_hour = 12;

    if (tms->tm_year < 90)
      tms->tm_year += 100;
    ltime = lt = mktime(tms);

    strcpy(str, tag);
    if (strchr(str, '.'))
      {
      offset = atoi(strchr(str, '.')+1);
      *strchr(str, '.') = 0;
      }
    else
      return EL_FILE_ERROR;

    do
      {
      tms = localtime(&ltime);

      sprintf(file_name, "%s%02d%02d%02d.log", dir, 
              tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday);
      *fh = open(file_name, O_RDWR | O_BINARY, 0644);

      if (*fh < 0)
        {
        if (!walk)
          return EL_FILE_ERROR;

        if (direction == -1)
          ltime -= 3600*24; /* one day back */
        else
          ltime += 3600*24; /* go forward one day */

        /* set new tag */
        tms = localtime(&ltime);
        sprintf(tag, "%02d%02d%02d.0", tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday);
        }

      } while (*fh < 0 && abs((INT)lt-(INT)ltime) < 3600*24*365);

    if (*fh < 0)
      return EL_FILE_ERROR;

    lseek(*fh, offset, SEEK_SET);

    /* check if start of message */
    i = read(*fh, str, 15);
    if (i <= 0)
      {
      close(*fh);
      return EL_FILE_ERROR;
      }

    if (strncmp(str, "$Start$: ", 9) != 0)
      {
      close(*fh);
      return EL_FILE_ERROR;
      }

    lseek(*fh, offset, SEEK_SET);
    }

  /* open most recent file if no tag given */
  if (tag[0] == 0)
    {
    time(&lt);
    ltime = lt;
    do
      {
      tms = localtime(&ltime);

      sprintf(file_name, "%s%02d%02d%02d.log", dir, 
              tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday);
      *fh = open(file_name, O_RDWR | O_BINARY, 0644);

      if (*fh < 0)
        ltime -= 3600*24; /* one day back */

      } while (*fh < 0 && (INT)lt-(INT)ltime < 3600*24*365);

    if (*fh < 0)
      return EL_FILE_ERROR;

    /* remember tag */
    sprintf(tag, "%02d%02d%02d", tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday);

    lseek(*fh, 0, SEEK_END);

    sprintf(tag+strlen(tag), ".%d", TELL(*fh));
    }


  if (direction == -1)
    {
    /* seek previous message */

    if (TELL(*fh) == 0)
      {
      /* go back one day */
      close(*fh);

      lt = ltime;
      do
        {
        lt -= 3600*24;
        tms = localtime(&lt);
        sprintf(str, "%02d%02d%02d.0",  
                tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday);

        status = el_search_message(str, fh, FALSE);

        } while (status != EL_SUCCESS && 
                 (INT)ltime-(INT)lt < 3600*24*365);

      if (status != EL_SUCCESS)
        return EL_FIRST_MSG;

      /* adjust tag */
      strcpy(tag, str);

      /* go to end of current file */
      lseek(*fh, 0, SEEK_END);
      }

    /* read previous message size */
    lseek(*fh, -17, SEEK_CUR);
    i = read(*fh, str, 17);
    if (i <= 0)
      {
      close(*fh);
      return EL_FILE_ERROR;
      }

    if (strncmp(str, "$End$: ", 7) == 0)
      {
      size = atoi(str+7);
      lseek(*fh, -size, SEEK_CUR);
      }
    else
      {
      close(*fh);
      return EL_FILE_ERROR;
      }

    /* adjust tag */
    sprintf(strchr(tag, '.')+1, "%d", TELL(*fh));
    }

  if (direction == 1)
    {
    /* seek next message */

    /* read current message size */
    last = TELL(*fh);

    i = read(*fh, str, 15);
    if (i <= 0)
      {
      close(*fh);
      return EL_FILE_ERROR;
      }
    lseek(*fh, -15, SEEK_CUR);

    if (strncmp(str, "$Start$: ", 9) == 0)
      {
      size = atoi(str+9);
      lseek(*fh, size, SEEK_CUR);
      }
    else
      {
      close(*fh);
      return EL_FILE_ERROR;
      }

    /* if EOF, goto next day */
    i = read(*fh, str, 15);
    if (i < 15)
      {
      close(*fh);
      time(&lact);

      lt = ltime;
      do
        {
        lt += 3600*24;
        tms = localtime(&lt);
        sprintf(str, "%02d%02d%02d.0",  
                tms->tm_year % 100, tms->tm_mon+1, tms->tm_mday);

        status = el_search_message(str, fh, FALSE);

        } while (status != EL_SUCCESS && 
                 (INT)lt-(INT)lact < 3600*24);

      if (status != EL_SUCCESS)
        return EL_LAST_MSG;

      /* adjust tag */
      strcpy(tag, str);

      /* go to beginning of current file */
      lseek(*fh, 0, SEEK_SET);
      }
    else
      lseek(*fh, -15, SEEK_CUR);

    /* adjust tag */
    sprintf(strchr(tag, '.')+1, "%d", TELL(*fh));
    }

  return EL_SUCCESS;
}

INT el_retrieve(char *tag, char *date, int *run, char *author, char *type, 
                char *system, char *subject, char *text, int *textsize, 
                char *orig_tag, char *reply_tag, 
                char *attachment1, char *attachment2, char *attachment3, 
                char *encoding)
/********************************************************************\

  Routine: el_retrieve

  Purpose: Retrieve an ELog entry by its message tab

  Input:
    char   *tag             tag in the form YYMMDD.offset
    int    *size            Size of text buffer

  Output:
    char   *tag             tag of retrieved message
    char   *date            Date/time of message recording
    int    *run             Run number
    char   *author          Message author
    char   *type            Message type
    char   *system          Message system
    char   *subject         Subject
    char   *text            Message text
    char   *orig_tag        Original message if this one is a reply
    char   *reply_tag       Reply for current message
    char   *attachment1/2/3 File attachment
    char   *encoding        Encoding of message
    int    *size            Actual message text size

  Function value:
    EL_SUCCESS              Successful completion
    EL_LAST_MSG             Last message in log

\********************************************************************/
{
int     size, fh, offset, search_status;
char    str[256], *p;
char    message[10000], thread[256], attachment_all[256];

  if (tag[0])
    {
    search_status = el_search_message(tag, &fh, TRUE);
    if (search_status != EL_SUCCESS)
      return search_status;
    }
  else
    {
    /* open most recent message */
    strcpy(tag, "-1");
    search_status = el_search_message(tag, &fh, TRUE);
    if (search_status != EL_SUCCESS)
      return search_status;
    }

  /* extract message size */
  offset = TELL(fh);
  read(fh, str, 16);
  size = atoi(str+9);
  read(fh, message, size);

  close(fh);

  /* decode message */
  if (strstr(message, "Run: ") && run)
    *run = atoi(strstr(message, "Run: ")+5);
    
  el_decode(message, "Date: ", date);
  el_decode(message, "Thread: ", thread);
  el_decode(message, "Author: ", author);
  el_decode(message, "Type: ", type);
  el_decode(message, "System: ", system);
  el_decode(message, "Subject: ", subject);
  el_decode(message, "Attachment: ", attachment_all);
  el_decode(message, "Encoding: ", encoding);

  /* break apart attachements */
  if (attachment1 && attachment2 && attachment3)
    {
    attachment1[0] = attachment2[0] = attachment3[0] = 0;
    p = strtok(attachment_all, ",");
    if (p != NULL)
      strcpy(attachment1, p);
    p = strtok(NULL, ",");
    if (p != NULL)
      strcpy(attachment2, p);
    p = strtok(NULL, ",");
    if (p != NULL)
      strcpy(attachment3, p);
    }

  /* conver thread in reply-to and reply-from */
  if (orig_tag != NULL && reply_tag != NULL)
    {
    p = strtok(thread, " \r");
    if (p != NULL)
      strcpy(orig_tag, p);
    else
      strcpy(orig_tag, "");
    p = strtok(NULL, " \r");
    if (p != NULL)
      strcpy(reply_tag, p);
    else
      strcpy(reply_tag, "");
    if (atoi(orig_tag) == 0)
      orig_tag[0] = 0;
    if (atoi(reply_tag) == 0)
      reply_tag[0] = 0;
    }

  p = strstr(message, "========================================\n");

  if (text != NULL)
    {
    if (p != NULL)
      {
      p += 41;
      if ((int) strlen(p) >= *textsize)
        {
        strncpy(text, p, *textsize-1);
        text[*textsize-1] = 0;
        return EL_TRUNCATED;
        }
      else
        {
        strcpy(text, p);
    
        /* strip end tag */
        if (strstr(text, "$End$"))
          *strstr(text, "$End$") = 0;
      
        *textsize = strlen(text);
        }
      }
    else
      {
      text[0] = 0;
      *textsize = 0;
      }
    }

  if (search_status == EL_LAST_MSG)
    return EL_LAST_MSG;

  return EL_SUCCESS;
}

/*------------------------------------------------------------------*/

INT el_search_run(int run, char *return_tag)
/********************************************************************\

  Routine: el_search_run

  Purpose: Find first message belonging to a specific run

  Input:
    int    run              Run number

  Output:
    char   *tag             tag of retrieved message

  Function value:
    EL_SUCCESS              Successful completion
    EL_LAST_MSG             Last message in log

\********************************************************************/
{
int     actual_run, fh, status;
char    tag[256];

  tag[0] = return_tag[0] = 0;

  do
    {
    /* open first message in file */
    strcat(tag, "-1");
    status = el_search_message(tag, &fh, TRUE);
    if (status == EL_FIRST_MSG)
      break;
    if (status != EL_SUCCESS)
      return status;
    close(fh);

    if (strchr(tag, '.') != NULL)
      strcpy(strchr(tag, '.'), ".0");

    el_retrieve(tag, NULL, &actual_run, NULL, NULL, 
                NULL, NULL, NULL, NULL, NULL, NULL, 
                NULL, NULL, NULL, NULL);
    } while (actual_run >= run);

  while (actual_run < run)
    {
    strcat(tag, "+1");
    status = el_search_message(tag, &fh, TRUE);
    if (status == EL_LAST_MSG)
      break;
    if (status != EL_SUCCESS)
      return status;
    close(fh);

    el_retrieve(tag, NULL, &actual_run, NULL, NULL, 
                NULL, NULL, NULL, NULL, NULL, NULL,
                NULL, NULL, NULL, NULL);
    }

  strcpy(return_tag, tag);

  if (status == EL_LAST_MSG || status == EL_FIRST_MSG)
    return status;
  
  return EL_SUCCESS;
}

/*------------------------------------------------------------------*/

/********************************************************************\
*                                                                    *
*                     Alarm functions                                *
*                                                                    *
\********************************************************************/

/*------------------------------------------------------------------*/

BOOL al_evaluate_condition(char *condition, char *value)
{
HNDLE  hDB, hkey;
int    i, j, index, size;
KEY    key;
double value1, value2;
char   str[256], op[3], function[80];
char   data[10000];
DWORD  time;

  strcpy(str, condition);
  op[1] = op[2] = 0;
  value1 = value2 = 0;
  index = 0;

  /* find value and operator */
  for (i=strlen(str)-1 ; i>0 ; i--)
    if (strchr("<>=!", str[i]) != NULL)
      break;
  op[0] = str[i];
  value2 = atof(str+i+1);
  str[i] = 0;

  if (i>0 && strchr("<>=!", str[i-1]))
    {
    op[1] = op[0];
    op[0] = str[--i];
    str[i] = 0;
    }

  i--;
  while (i>0 && str[i] == ' ')
    i--;
  str[i+1] = 0;

  /* check if function */
  function[0] = 0;
  if (str[i] == ')')
    {
    str[i--] = 0;
    if (strchr(str, '('))
      {
      *strchr(str, '(') = 0;
      strcpy(function, str);
      for (i=strlen(str)+1,j=0 ; str[i] ; i++,j++)
        str[j] = str[i];
      str[j] = 0;
      i = j-1;
      }
    }

  /* find key */
  if (str[i] == ']')
    {
    str[i--] = 0;
    while (i>0 && isdigit(str[i]))
      i--;
    index = atoi(str+i+1);
    str[i] = 0;
    }

  cm_get_experiment_database(&hDB, NULL);
  db_find_key(hDB, 0, str, &hkey);
  if (!hkey)
    {
    cm_msg(MERROR, "al_evaluate_condition", "Cannot find key %s to evaluate alarm condition", str);
    if (value)
      strcpy(value, "unknown");
    return FALSE;
    }

  if (equal_ustring(function, "access"))
    {
    /* check key access time */
    db_get_key_time(hDB, hkey, &time);
    sprintf(str, "%d", time);
    value1 = atof(str);
    }
  else
    {
    /* get key data and convert to double */
    db_get_key(hDB, hkey, &key);
    size = sizeof(data);
    db_get_data(hDB, hkey, data, &size, key.type);
    db_sprintf(str, data, size, index, key.type);
    value1 = atof(str);
    }
  
  /* return value */
  if (value)
    strcpy(value, str);

  /* now do logical operation */
  if (strcmp(op, "=") == 0)
    return value1 == value2;
  if (strcmp(op, "==") == 0)
    return value1 == value2;
  if (strcmp(op, "!=") == 0)
    return value1 != value2;
  if (strcmp(op, "<") == 0)
    return value1 < value2;
  if (strcmp(op, ">") == 0)
    return value1 > value2;
  if (strcmp(op, "<=") == 0)
    return value1 <= value2;
  if (strcmp(op, ">=") == 0)
    return value1 >= value2;

  return FALSE;
}

/*------------------------------------------------------------------*/

INT al_trigger_alarm(char *alarm_name, char *alarm_message, char *default_class,
                     char *cond_str, INT type)
/********************************************************************\

  Routine: al_trigger_alarm

  Purpose: Trigger a certain alarm

  Input:
    char   *alarm_name      Alarm name, defined in /alarms/alarms
    char   *alarm_message   Optional message which goes with alarm
    char   *default_class   If alarm is not yet defined under 
                            /alarms/alarms/<alarm_name>, a new one
                            is created and this default class is used.
    char   cond_str         String displayed in alarm condition
    INT    type             Alarm type, one of AT_xxx

  Output:

  Function value:
    AL_INVALID_NAME         Alarm name not defined
    AL_SUCCESS              Successful completion

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_AL_TRIGGER_ALARM, alarm_name, alarm_message,
                    default_class, cond_str, type);

#ifdef LOCAL_ROUTINES
{
int         status, size;
HNDLE       hDB, hkeyalarm;
char        str[256];
ALARM       alarm;
BOOL        flag;
ALARM_STR(alarm_str);

  cm_get_experiment_database(&hDB, NULL);

  /* find alarm */
  sprintf(str, "/Alarms/Alarms/%s", alarm_name);
  db_find_key(hDB, 0, str, &hkeyalarm);
  if (!hkeyalarm)
    {
    /* alarm must be an internal analyzer alarm, so create a default alarm */
    status = db_create_record(hDB, 0, str, strcomb(alarm_str));
    db_find_key(hDB, 0, str, &hkeyalarm);
    if (!hkeyalarm)
      {
      cm_msg(MERROR, "al_trigger_alarm", "Cannot create alarm record");
      return AL_ERROR_ODB;
      }

    if (default_class && default_class[0])
      db_set_value(hDB, hkeyalarm, "Alarm Class", default_class, 32, 1, TID_STRING);
    status = TRUE;
    db_set_value(hDB, hkeyalarm, "Active", &status, sizeof(status), 1, TID_BOOL); 
    }

  /* set parameters for internal alarms */
  if (type != AT_EVALUATED)
    {
    db_set_value(hDB, hkeyalarm, "Type", &type, sizeof(INT), 1, TID_INT);
    strcpy(str, cond_str);
    db_set_value(hDB, hkeyalarm, "Condition", str, 256, 1, TID_STRING); 
    }

  size = sizeof(alarm);
  status = db_get_record(hDB, hkeyalarm, &alarm, &size, 0);
  if (status != DB_SUCCESS || alarm.type < AT_INTERNAL || alarm.type > AT_EVALUATED)
    {
    /* make sure alarm record has right structure */
    db_create_record(hDB, hkeyalarm, "", strcomb(alarm_str));

    size = sizeof(alarm);
    status = db_get_record(hDB, hkeyalarm, &alarm, &size, 0);
    if (status != DB_SUCCESS)
      {
      cm_msg(MERROR, "al_trigger_alarm", "Cannot get alarm record");
      return AL_ERROR_ODB;
      }
    }

  /* if internal alarm, check if active and check interval */
  if (alarm.type != AT_EVALUATED)
    {
    /* check global alarm flag */
    flag = TRUE;
    size = sizeof(flag);
    db_get_value(hDB, 0, "/Alarms/Alarm system active", &flag, &size, TID_BOOL);
    if (!flag)
      return AL_SUCCESS;

    if (!alarm.active)
      return AL_SUCCESS;

    if ((INT)ss_time() - (INT)alarm.checked_last < alarm.check_interval)
      return AL_SUCCESS;

    /* now the alarm will be triggered, so save time */
    alarm.checked_last = ss_time();
    }

  /* write back alarm message for internal alarms */
  if (alarm.type != AT_EVALUATED)
    {
    strncpy(alarm.alarm_message, alarm_message, 79);
    alarm.alarm_message[79] = 0;
    }

  /* now trigger alarm class defined in this alarm */
  if (alarm.alarm_class[0])
    al_trigger_class(alarm.alarm_class, alarm_message, alarm.triggered > 0);

  /* signal alarm being triggered */
  cm_asctime(str, sizeof(str));

  if (!alarm.triggered)
    strcpy(alarm.time_triggered_first, str);

  alarm.triggered++;
  strcpy(alarm.time_triggered_last, str);

  alarm.checked_last = ss_time();

  status = db_set_record(hDB, hkeyalarm, &alarm, sizeof(alarm), 0);
  if (status != DB_SUCCESS)
    {
    cm_msg(MERROR, "al_trigger_alarm", "Cannot update alarm record");
    return AL_ERROR_ODB;
    }

}
#endif /* LOCAL_ROUTINES */

  return AL_SUCCESS;
}

/*------------------------------------------------------------------*/

INT al_trigger_class(char *alarm_class, char *alarm_message, BOOL first)
/********************************************************************\

  Routine: al_trigger_class

  Purpose: Trigger a certain alarm class

  Input:
    char   *alarm_class     Alarm class, must be defined in 
                            /alarms/classes
    char   *alarm_message   Optional message which goes with alarm
    BOOL   first            TRUE if alarm is triggered first time 
                            (used for elog)

  Output:

  Function value:
    AL_INVALID_NAME         Alarm class not defined
    AL_SUCCESS              Successful completion

\********************************************************************/
{
int         status, size, state;
HNDLE       hDB, hkeyclass;
char        str[256], command[256], tag[32];
ALARM_CLASS ac;

  cm_get_experiment_database(&hDB, NULL);

  /* get alarm class */
  sprintf(str, "/Alarms/Classes/%s", alarm_class);
  db_find_key(hDB, 0, str, &hkeyclass);
  if (!hkeyclass)
    {
    cm_msg(MERROR, "al_trigger_class", "Alarm class %s not found in ODB", 
                   alarm_class);
    return AL_INVALID_NAME;
    }

  size = sizeof(ac);
  status = db_get_record(hDB, hkeyclass, &ac, &size, 0);
  if (status != DB_SUCCESS)
    {
    cm_msg(MERROR, "al_trigger_class", "Cannot get alarm class record");
    return AL_ERROR_ODB;
    }

  /* write system message */
  if (ac.write_system_message && 
      (INT)ss_time() - (INT)ac.system_message_last > ac.system_message_interval)
    {
    sprintf(str, "%s: %s", alarm_class, alarm_message);
    cm_msg(MTALK, "al_trigger_class", str);
    ac.system_message_last = ss_time();
    }

  /* write elog message on first trigger */
  if (ac.write_elog_message && first)
    el_submit(0, "Alarm system", "Alarm", "General", alarm_class, str, 
              "", "plain", "", "", 0, "", "", 0, "", "", 0, tag, 32); 

  /* execute command */
  if (ac.execute_command[0] &&
      ac.execute_interval > 0 &&
      (INT)ss_time() - (INT)ac.execute_last > ac.execute_interval)
    {
    sprintf(str, "%s: %s", alarm_class, alarm_message);
    sprintf(command, ac.execute_command, str);
    cm_msg(MINFO, "al_trigger_class", "Execute: %s", command);
    ss_system(command);
    ac.execute_last = ss_time();
    }

  /* stop run */
  if (ac.stop_run)
    {
    state = STATE_STOPPED;
    size = sizeof(state);
    db_get_value(hDB, 0, "/Runinfo/State", &state, &size, TID_INT);
    if (state != STATE_STOPPED)
      cm_transition(TR_STOP, 0, NULL, 0, ASYNC);
    }

  status = db_set_record(hDB, hkeyclass, &ac, sizeof(ac), 0);
  if (status != DB_SUCCESS)
    {
    cm_msg(MERROR, "al_trigger_class", "Cannot update alarm class record");
    return AL_ERROR_ODB;
    }

  return AL_SUCCESS;
}

/*------------------------------------------------------------------*/

INT al_reset_alarm(char *alarm_name)
/********************************************************************\

  Routine: al_reset_alarm

  Purpose: Reset (acknowledge) alarm

  Input:
    char   *alarm_name      Alarm name, must be defined in /Alarms/Alarms
                            If NULL reset all alarms

  Output:
    <none>

  Function value:
    AL_INVALID_NAME         Alarm name not defined
    AL_RESET                Alarm was triggered and reset
    AL_SUCCESS              Successful completion

\********************************************************************/
{
int         status, size, i;
HNDLE       hDB, hkeyalarm, hkeyclass, hsubkey;
KEY         key;
char        str[256];
ALARM       alarm;
ALARM_CLASS ac;

  cm_get_experiment_database(&hDB, NULL);

  if (alarm_name == NULL)
    {
    /* reset all alarms */
    db_find_key(hDB, 0, "/Alarms/Alarms", &hkeyalarm);
    if (hkeyalarm)
      {
      for (i=0 ; ; i++)
        {
        db_enum_link(hDB, hkeyalarm, i, &hsubkey);

        if (!hsubkey)
          break;

        db_get_key(hDB, hsubkey, &key);
        al_reset_alarm(key.name);
        }
      }
    return AL_SUCCESS;
    }

  /* find alarm and alarm class */
  sprintf(str, "/Alarms/Alarms/%s", alarm_name);
  db_find_key(hDB, 0, str, &hkeyalarm);
  if (!hkeyalarm)
    {
    cm_msg(MERROR, "al_reset_alarm", "Alarm %s not found in ODB", alarm_name);
    return AL_INVALID_NAME;
    }

  size = sizeof(alarm);
  status = db_get_record(hDB, hkeyalarm, &alarm, &size, 0);
  if (status != DB_SUCCESS)
    {
    cm_msg(MERROR, "al_reset_alarm", "Cannot get alarm record");
    return AL_ERROR_ODB;
    }

  sprintf(str, "/Alarms/Classes/%s", alarm.alarm_class);
  db_find_key(hDB, 0, str, &hkeyclass);
  if (!hkeyclass)
    {
    cm_msg(MERROR, "al_reset_alarm", "Alarm class %s not found in ODB", alarm.alarm_class);
    return AL_INVALID_NAME;
    }

  size = sizeof(ac);
  status = db_get_record(hDB, hkeyclass, &ac, &size, 0);
  if (status != DB_SUCCESS)
    {
    cm_msg(MERROR, "al_reset_alarm", "Cannot get alarm class record");
    return AL_ERROR_ODB;
    }

  if (alarm.triggered)
    {
    alarm.triggered = 0;
    alarm.time_triggered_first[0] = 0;
    alarm.time_triggered_last[0] = 0;
    alarm.checked_last = 0;

    ac.system_message_last = 0;
    ac.execute_last = 0;

    status = db_set_record(hDB, hkeyalarm, &alarm, sizeof(alarm), 0);
    if (status != DB_SUCCESS)
      {
      cm_msg(MERROR, "al_reset_alarm", "Cannot update alarm record");
      return AL_ERROR_ODB;
      }
    status = db_set_record(hDB, hkeyclass, &ac, sizeof(ac), 0);
    if (status != DB_SUCCESS)
      {
      cm_msg(MERROR, "al_reset_alarm", "Cannot update alarm class record");
      return AL_ERROR_ODB;
      }
    return AL_RESET;
    }

  return AL_SUCCESS;
}

/*------------------------------------------------------------------*/

INT al_check()
/********************************************************************\

  Routine: al_scan

  Purpose: Scan ODB alarams and programs 

  Input:

  Output:

  Function value:
    AL_SUCCESS              Successful completion

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_AL_CHECK);

#ifdef LOCAL_ROUTINES
{
INT          i, status, size, mutex;
HNDLE        hDB, hkeyroot, hkey;
KEY          key;
ALARM        alarm;
char         str[256], value[256];
DWORD        now;
PROGRAM_INFO program_info;
BOOL         flag;

ALARM_CLASS_STR(alarm_class_str);
ALARM_STR(alarm_str);

  cm_get_experiment_database(&hDB, NULL);

  if (hDB == 0)
    return AL_SUCCESS; /* called from server not yet connected */

  /* check online mode */
  flag = TRUE;
  size = sizeof(flag);
  db_get_value(hDB, 0, "/Runinfo/Online Mode", &flag, &size, TID_INT);
  if (!flag)
    return AL_SUCCESS;

  /* check global alarm flag */
  flag = TRUE;
  size = sizeof(flag);
  db_get_value(hDB, 0, "/Alarms/Alarm system active", &flag, &size, TID_BOOL);
  if (!flag)
    return AL_SUCCESS;

  /* request semaphore */
  cm_get_experiment_mutex(&mutex, NULL);
  status = ss_mutex_wait_for(mutex, 100);
  if (status != SS_SUCCESS)
    return SUCCESS; /* someone else is doing alarm business */

  /* check ODB alarms */
  db_find_key(hDB, 0, "/Alarms/Alarms", &hkeyroot);
  if (!hkeyroot)
    {
    /* create default ODB alarm */
    status = db_create_record(hDB, 0, "/Alarms/Alarms/Test", strcomb(alarm_str));
    db_find_key(hDB, 0, "/Alarms/Alarms", &hkeyroot);
    if (!hkeyroot)
      {
      ss_mutex_release(mutex);
      return SUCCESS;
      }

    /* create default alarm classes */
    status = db_create_record(hDB, 0, "/Alarms/Classes/Alarm", strcomb(alarm_class_str));
    status = db_create_record(hDB, 0, "/Alarms/Classes/Warning", strcomb(alarm_class_str));
    if (status != DB_SUCCESS)
      {
      ss_mutex_release(mutex);
      return SUCCESS;
      }
    }

  for (i=0 ; ; i++)
    {
    status = db_enum_key(hDB, hkeyroot, i, &hkey);
    if (status == DB_NO_MORE_SUBKEYS)
      break;

    db_get_key(hDB, hkey, &key);

    size = sizeof(alarm);
    status = db_get_record(hDB, hkey, &alarm, &size, 0);
    if (status != DB_SUCCESS || alarm.type < AT_INTERNAL || alarm.type > AT_EVALUATED)
      {
      /* make sure alarm record has right structure */
      db_create_record(hDB, hkey, "", strcomb(alarm_str));
      size = sizeof(alarm);
      status = db_get_record(hDB, hkey, &alarm, &size, 0);
      if (status != DB_SUCCESS || alarm.type < AT_INTERNAL || alarm.type > AT_EVALUATED)
        {
        cm_msg(MERROR, "al_check", "Cannot get alarm record");
        continue;
        }
      }

    /* check alarm only when active and not internal */
    if (alarm.active &&
        alarm.type == AT_EVALUATED && 
        alarm.check_interval > 0 &&
        (INT)ss_time() - (INT)alarm.checked_last > alarm.check_interval)
      {
      /* if condition is true, trigger alarm */
      if (al_evaluate_condition(alarm.condition, value))
        {
        sprintf(str, alarm.alarm_message, value);
        al_trigger_alarm(key.name, str, alarm.alarm_class, "", AT_EVALUATED);
        }
      else
        {
        alarm.checked_last = ss_time();
        status = db_set_record(hDB, hkey, &alarm, sizeof(alarm), 0);
        if (status != DB_SUCCESS)
          {
          cm_msg(MERROR, "al_check", "Cannot write back alarm record");
          continue;
          }
        }
      }
    }

  /* check /programs alarms */
  db_find_key(hDB, 0, "/Programs", &hkeyroot);
  if (hkeyroot)
    {
    for (i=0 ; ; i++)
      {
      status = db_enum_key(hDB, hkeyroot, i, &hkey);
      if (status == DB_NO_MORE_SUBKEYS)
        break;

      db_get_key(hDB, hkey, &key);
      
      /* don't check "execute on xxx" */
      if (key.type != TID_KEY)
        continue;

      size = sizeof(program_info);
      status = db_get_record(hDB, hkey, &program_info, &size, 0);
      if (status != DB_SUCCESS)
        {
        cm_msg(MERROR, "al_check", "Cannot get program info record");
        continue;
        }

      now = ss_time();
      /* check once every minute */
      if (now - program_info.checked_last > 60)
        {
        program_info.checked_last = now;

        rpc_get_name(str);
        str[strlen(key.name)] = 0;
        if (!equal_ustring(str, key.name) &&
            cm_exist(key.name, FALSE) == CM_NO_CLIENT)
          {
          program_info.alarm_count++;

          /* fire alarm when not running for 5 minues */
          if (program_info.alarm_count >= 5)
            {
            /* if not running and alarm calss defined, trigger alarm */
            if (program_info.alarm_class[0])
              {
              sprintf(str, "Program %s is not running", key.name);
              al_trigger_alarm(key.name, str, program_info.alarm_class, 
                               "Program not running", AT_PROGRAM);
              }

            /* auto restart program */
            if (program_info.auto_restart &&
                program_info.start_command[0])
              {
              ss_system(program_info.start_command);
              cm_msg(MTALK, "al_check", "Program %s restarted", key.name);
              }
            }
          }
        else
          program_info.alarm_count = 0;

        db_set_record(hDB, hkey, &program_info, sizeof(program_info), 0);
        }
      }
    }

  ss_mutex_release(mutex);
}
#endif /* LOCAL_COUTINES */

  return SUCCESS;
}

/*------------------------------------------------------------------*/

/********************************************************************\
*                                                                    *
*                 Event buffer functions                             *
*                                                                    *
\********************************************************************/

/* PAA several modification in the eb_xxx()
   also new function eb_buffer_full()
*/
static char *_event_ring_buffer = NULL;
static INT  _eb_size;
static char *_eb_read_pointer, *_eb_write_pointer, *_eb_end_pointer;

/*------------------------------------------------------------------*/

INT eb_create_buffer(INT size)
/********************************************************************\

  Routine: eb_create_buffer

  Purpose: Create an event buffer. Has to be called initially before
           any other eb_xxx function

  Input:
    INT    size             Size in bytes

  Output:
    none
  
  Function value:
    CM_SUCCESS              Successful completion
    BM_NO_MEMEORY           Out of memory

\********************************************************************/
{
  _event_ring_buffer = malloc(size);
  if (_event_ring_buffer == NULL)
    return BM_NO_MEMORY;

  memset(_event_ring_buffer, 0, size);
  _eb_size = size;

  _eb_write_pointer = _eb_read_pointer = _eb_end_pointer = _event_ring_buffer;

  _send_sock = rpc_get_event_sock();

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT eb_free_buffer()
/********************************************************************\

  Routine: eb_free_buffer

  Purpose: Free memory allocated voa eb_create_buffer

  Input:
    none

  Output:
    none
  
  Function value:
    CM_SUCCESS              Successful completion

\********************************************************************/
{
  if (_event_ring_buffer)
    free(_event_ring_buffer);

  _eb_size = 0;
  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT eb_free_space  (void)
/********************************************************************\

  Routine: eb_free_space

  Purpose: Compute and return usable free space in the event buffer

  Input:
    none

  Output:
    none
  
  Function value:
    INT	   Number of usable free bytes in the event buffer

\********************************************************************/
{
  INT free;

  if (_event_ring_buffer == NULL)
    {
    cm_msg(MERROR, "eb_get_pointer", "please call eb_create_buffer first");
    return -1;
    }

  if (_eb_write_pointer >= _eb_read_pointer) {
    free = _eb_size - ((PTYPE)_eb_write_pointer - (PTYPE)_event_ring_buffer);
  } else if (_eb_write_pointer >= _event_ring_buffer) {
    free = (PTYPE)_eb_read_pointer - (PTYPE)_eb_write_pointer;
  } else if (_eb_end_pointer == _event_ring_buffer) {
    _eb_write_pointer = _event_ring_buffer;
    free = _eb_size;
  } else if (_eb_read_pointer == _event_ring_buffer) {
    free = 0;
  } else {
    _eb_write_pointer = _event_ring_buffer;
    free = (PTYPE)_eb_read_pointer - (PTYPE)_eb_write_pointer;
  }

  return free;
}

/*------------------------------------------------------------------*/

DWORD eb_get_level()
/********************************************************************\

  Routine: eb_get_level

  Purpose: Return filling level of event buffer in percent

  Input:
    none

  Output:
    none
  
  Function value:
    DWORD level              0..99

\********************************************************************/
{
INT size;

  size = _eb_size - eb_free_space();

  return (100 * size) / _eb_size;
}

/*------------------------------------------------------------------*/

BOOL eb_buffer_full  (void)
/********************************************************************\

  Routine: eb_buffer_full

  Purpose: Test if there is sufficient space in the event buffer 
		for another event

  Input:
    none

  Output:
    none
  
  Function value:
    BOOL	Is there enough space for another event in the event buffer

\********************************************************************/
{
  INT free;

  free = eb_free_space();

  /* if max. event won't fit, return zero */
  return (free < MAX_EVENT_SIZE+sizeof(EVENT_HEADER)+sizeof(INT));
}

/*------------------------------------------------------------------*/

EVENT_HEADER *eb_get_pointer()
/********************************************************************\

  Routine: eb_get_pointer

  Purpose: Get pointer to next free location in event buffer

  Input:
    none

  Output:
    none
  
  Function value:
    EVENT_HEADER *            Pointer to free location

\********************************************************************/
{
  /* if max. event won't fit, return zero */
  if (eb_buffer_full())
    {
#ifdef OS_VXWORKS
    logMsg( "eb_get_pointer(): Event won't fit: read=%d, write=%d, end=%d\n",
	    _eb_read_pointer - _event_ring_buffer,
	    _eb_write_pointer - _event_ring_buffer,
	    _eb_end_pointer - _event_ring_buffer,
	    0,0,0 );
#endif
    return NULL;
    }

  /* leave space for buffer handle */
  return (EVENT_HEADER *) (_eb_write_pointer+sizeof(INT));
}

/*------------------------------------------------------------------*/

INT eb_increment_pointer(INT buffer_handle, INT event_size)
/********************************************************************\

  Routine: eb_increment_pointer

  Purpose: Increment write pointer of event buffer after an event
           has been copied into the buffer (at an address previously
           obtained via eb_get_pointer)

  Input:
    INT buffer_handle         Buffer handle event should be sent to
    INT event_size            Event size in bytes including header

  Output:
    none
  
  Function value:
    CM_SUCCESS                Successful completion

\********************************************************************/
{
INT aligned_event_size;

  /* if not connected remotely, use bm_send_event */
  if (_send_sock == 0)
    return bm_send_event(buffer_handle,
                         _eb_write_pointer+sizeof(INT), 
                         event_size, SYNC);

  aligned_event_size = ALIGN(event_size);

  /* copy buffer handle */
  *((INT *) _eb_write_pointer) = buffer_handle;
  _eb_write_pointer += sizeof(INT) + aligned_event_size;
  
  if (_eb_write_pointer > _eb_end_pointer)
    _eb_end_pointer = _eb_write_pointer;

  if (_eb_write_pointer > _event_ring_buffer + _eb_size)
    cm_msg(MERROR, "eb_increment_pointer", "event size (%d) exeeds maximum event size (%d)",
                    event_size, MAX_EVENT_SIZE);

  if (_eb_size - ((PTYPE)_eb_write_pointer - (PTYPE)_event_ring_buffer) < 
      MAX_EVENT_SIZE+sizeof(EVENT_HEADER)+sizeof(INT))
    {
    _eb_write_pointer = _event_ring_buffer;
    
    /* avoid rp==wp */
    if (_eb_read_pointer == _event_ring_buffer)
      _eb_write_pointer--;
    }
    
  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

INT eb_send_events(BOOL send_all)
/********************************************************************\

  Routine: eb_send_events

  Purpose: Send events from the event buffer to the server

  Input:
    BOOL send_all             If FALSE, only send events if buffer
                              contains more than _opt_tcp_size bytes

  Output:
    none
  
  Function value:
    CM_SUCCESS                Successful completion

\********************************************************************/
{
char *eb_wp, *eb_ep;
INT  size, i;

  /* write pointers are volatile, so make copy */
  eb_ep = _eb_end_pointer;
  eb_wp = _eb_write_pointer;

  if (eb_wp == _eb_read_pointer)
    return CM_SUCCESS;
  if (eb_wp > _eb_read_pointer)
    {
    size = (PTYPE) eb_wp - (PTYPE) _eb_read_pointer;

    /* don't send if less than optimal TCP buffer size available */
    if (size < (INT) _opt_tcp_size && !send_all)
      return CM_SUCCESS;
    }
  else
    {
    /* send last piece of event buffer */
    size = (PTYPE) eb_ep - (PTYPE) _eb_read_pointer;
    }

  while (size > _opt_tcp_size)
    {
    /* send buffer */
    i = send_tcp(_send_sock, _eb_read_pointer, _opt_tcp_size, 0);
    if (i < 0)
      {
      printf("send_tcp() returned %d\n", i);
      cm_msg(MERROR, "eb_send_events", "send_tcp() failed");
      return RPC_NET_ERROR;
      }

    _eb_read_pointer += _opt_tcp_size;
    if (_eb_read_pointer == eb_ep && eb_wp < eb_ep)
      _eb_read_pointer = _eb_end_pointer = _event_ring_buffer;

    size -= _opt_tcp_size;
    }

  if (send_all || eb_wp < _eb_read_pointer)
    {
    /* send buffer */
    i = send_tcp(_send_sock, _eb_read_pointer, size, 0);
    if (i < 0)
      {
      printf("send_tcp() returned %d\n", i);
      cm_msg(MERROR, "eb_send_events", "send_tcp() failed");
      return RPC_NET_ERROR;
      }

    _eb_read_pointer += size;
    if (_eb_read_pointer == eb_ep && eb_wp < eb_ep)
      _eb_read_pointer = _eb_end_pointer = _event_ring_buffer;
    }

  /* Check for case where eb_wp = eb_ring_buffer - 1 */
  if (eb_wp < _event_ring_buffer && _eb_end_pointer == _event_ring_buffer) {
    return CM_SUCCESS;
  }

  if (eb_wp != _eb_read_pointer)
    return BM_MORE_EVENTS;

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

/********************************************************************\
*                                                                    *
*                 Dual memory buffer functions                       *
*                                                                    *
* Provide a dual memory buffer scheme for handling front-end         *
* event. This code as been requested for allowing contemporary       *
* task handling a)acquisition, b)network transfer if possible.       *
* The pre-compiler switch will determine the mode of operation.      *
* if DM_DUAL_THREAD is defined in mfe.c, it is expected to have      *
* a seperate task taking care of the dm_area_send                    *
*                                                                    *
* "*" : visible functions                                            *
* dm_buffer_create():     *Setup the dual memory buffer              *
*                          Setup semaphore                           *
*                          Spawn second thread                       *
* dm_buffer_release():    *Release memory allocation for dm          *
*                          Force a kill of 2nd thread                *
*                          Remove semaphore                          *
* dm_area_full():         *Check for both area being full            *
*                          None blocking, may be used for interrupt  *
*                          disable.                                  *
* dm_pointer_get()     :  *Check memory space and return pointer     *
*                          Blocking function with timeout if no more *
*                          space for next event. If error will abort.*
* dm_pointer_increment(): *Move pointer to next free location        *
*                          None blocking. performs bm_send_event if  *
*                          local connection.                         *
* dm_area_send():         *Transfer FULL buffer(s)                   *
*                          None blocking function.                   *
*                          if DUAL_THREAD: Give sem_send semaphore   *
*                          else transfer FULL buffer                 *
* dm_area_flush():        *Transfer all remaining events from dm     *
*                          Blocking function with timeout            *
*                          if DUAL_THREAD: Give sem_flush semaphore. * 
* dm_task():               Secondary thread handling DUAL_THREAD     *
*                          mechanism. Serves 2 requests:             *
*                          dm_send:  Transfer FULL buffer only.      *
*                          dm_flush: Transfer ALL buffers.           *
* dm_area_switch():        internal, used by dm_pointer_get()        *
* dm_active_full():        internal: check space in current buffer   *
* dm_buffer_send():        internal: send data for given area        *
* dm_buffer_time_get():    interal: return the time stamp of the     * 
*                          last switch                               *
\********************************************************************/

#define DM_FLUSH       10    /* flush request for DUAL_THREAD */
#define DM_SEND        11    /* FULL send request for DUAL_THREAD */
#define DM_KILL        12    /* Kill request for 2nd thread */  
#define DM_TIMEOUT     13    /* "timeout" return state in flush request for DUAL_THREAD */  
#define DM_ACTIVE_NULL 14    /* "both buffer were/are FULL with no valid area" return state */

typedef struct {
  char *        pt;       /* top pointer    memory buffer          */
  char *        pw;       /* write pointer  memory buffer          */
  char *        pe;       /* end   pointer  memory buffer          */
  char *        pb;       /* bottom pointer memory buffer          */
  BOOL          full;     /* TRUE if memory buffer is full         */
  DWORD       serial;     /* full buffer serial# for evt order     */
  } DMEM_AREA;

typedef struct {
  DMEM_AREA *  pa;     /* active memory buffer */
  DMEM_AREA area1;     /* mem buffer area 1 */
  DMEM_AREA area2;     /* mem buffer area 2 */
  DWORD    serial;     /* overall buffer serial# for evt order     */
  INT      action;     /* for multi thread configuration */
  DWORD last_active;    /* switch time stamp */
  HNDLE sem_send;      /* semaphore for dm_task */
  HNDLE sem_flush;     /* semaphore for dm_task */
  } DMEM_BUFFER;

DMEM_BUFFER  dm;
INT          dm_user_max_event_size;
extern       _opt_tcp_size;

/*------------------------------------------------------------------*/
INT dm_buffer_create(INT size, INT user_max_event_size)
/********************************************************************\
  Routine: dm_buffer_create

  Purpose: Setup a dual memory buffer. Has to be called initially before
           any other dm_xxx function
  Input:
    INT    size             Size in bytes
    INT    user_max_event_size  
  Output:
    none
  Function value:
    CM_SUCCESS              Successful completion
    BM_NO_MEMORY            Out of memory
    BM_MEMSIZE_MISMATCH     event size too large
\********************************************************************/
{

  dm.area1.pt = malloc(size);
  if (dm.area1.pt == NULL)
    return (BM_NO_MEMORY);
  dm.area2.pt = malloc(size);
  if (dm.area2.pt == NULL)
    return (BM_NO_MEMORY);

  /* check user event size against the system MAX_EVENT_SIZE */
  if (user_max_event_size > MAX_EVENT_SIZE)
  {
    cm_msg(MERROR,"dm_buffer_create","user max event size too large");
    return BM_MEMSIZE_MISMATCH;
  }
  dm_user_max_event_size = user_max_event_size;  
  
  memset(dm.area1.pt, 0, size);
  memset(dm.area2.pt, 0, size);
  
  /* initialize pointers */
  dm.area1.pb = dm.area1.pt + size - 1024;
  dm.area1.pw = dm.area1.pe = dm.area1.pt;
  dm.area2.pb = dm.area2.pt + size - 1024;
  dm.area2.pw = dm.area2.pe = dm.area2.pt;
  
  /*-PAA-*/
#ifdef DM_DEBUG
  printf(" in dm_buffer_create ---------------------------------\n");
  printf(" %i %p %p %p %p\n", size, dm.area1.pt, dm.area1.pw, dm.area1.pe, dm.area1.pb);
  printf(" %i %p %p %p %p\n", size, dm.area2.pt, dm.area2.pw, dm.area2.pe, dm.area2.pb);
#endif

  /* activate first area */
  dm.pa = &dm.area1;
  
  /* Default not full */
  dm.area1.full = dm.area2.full = FALSE;
  
  /* Reset serial buffer number with proper starting sequence */
  dm.area1.serial = dm.area2.serial = 0;
  /* ensure proper serial on next increment */
  dm.serial = 1;

  /* set active buffer time stamp */
  dm.last_active = ss_millitime();
  
  /* get socket for event sending */
  _send_sock = rpc_get_event_sock();
  
#ifdef DM_DUAL_THREAD
  {
    INT status;
    VX_TASK_SPAWN starg;

    /* create semaphore */
    status = ss_mutex_create("send",&dm.sem_send);
    if (status != SS_CREATED && status != SS_SUCCESS)
      {
        cm_msg(MERROR,"dm_buffer_create","error in ss_mutex_create send");
        return status;
      }
    status = ss_mutex_create("flush",&dm.sem_flush);
    if (status != SS_CREATED && status != SS_SUCCESS)
      {
        cm_msg(MERROR,"dm_buffer_create","error in ss_mutex_create flush");
        return status;
      }
    /* spawn dm_task */
    memset (&starg, 0, sizeof(VX_TASK_SPAWN));

#ifdef OS_VXWORKS
    /* Fill up the necessary arguments */
    strcpy(starg.name,"areaSend");
    starg.priority = 120;
    starg.stackSize = 20000;
#endif

    if ((status = ss_thread_create(dm_task, (void *) &starg))
                != SS_SUCCESS)
      {
        cm_msg(MERROR,"dm_buffer_create","error in ss_thread_create");
        return status;
      }

#ifdef OS_WINNT
    /* necessary for true MUTEX (NT) */
    ss_mutex_wait_for(dm.sem_send, 0);
#endif
  }
#endif /* DM_DUAL_THREAD */
  
  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/
INT dm_buffer_release(void)
/********************************************************************\
  Routine: dm_buffer_release

  Purpose: Release dual memory buffers
  Input:
    none
  Output:
    none
  Function value:
    CM_SUCCESS              Successful completion
\********************************************************************/
{
  if (dm.area1.pt)
    {
      free (dm.area1.pt);
      dm.area1.pt = NULL;
    }
  if (dm.area2.pt)
    {
      free (dm.area2.pt);
      dm.area2.pt = NULL;
    }
  dm.serial = 0;
  dm.area1.full = dm.area2.full = TRUE;
  dm.area1.serial = dm.area2.serial = 0;
  
#ifdef DM_DUAL_THREAD
 /* kill spawned dm_task */
  dm.action = DM_KILL;
  ss_mutex_release(dm.sem_send);
  ss_mutex_release(dm.sem_flush);

  /* release semaphore */
  ss_mutex_delete(dm.sem_send, 0);
  ss_mutex_delete(dm.sem_flush, 0);
#endif

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/
 INLINE DMEM_AREA * dm_area_switch(void)
/********************************************************************\
  Routine: dm_area_switch

  Purpose: set active area to the other empty area or NULL if both
           area are full. May have to check the serial consistancy...
  Input:
    none
  Output:
    none
  Function value:
    DMEM_AREA *            Pointer to active area or both full
\********************************************************************/
{
  volatile BOOL full1, full2;
  
  full1 = dm.area1.full;
  full2 = dm.area2.full;

  if (!full1 && !full2)
    {
      if (dm.area1.serial <= dm.area2.serial)
        return (&(dm.area1));
      else
        return (&(dm.area2));
    }

  if (!full1)
    {
        return (&(dm.area1));
    }
  else if (!full2)
    {
      return (&(dm.area2));
    }
  return (NULL);}

/*------------------------------------------------------------------*/
INLINE BOOL dm_area_full (void)
/********************************************************************\
  Routine: dm_area_full

  Purpose: Test if both area are full in order to block interrupt
  Input:
    none
  Output:
    none
  Function value:
    BOOL	       TRUE if not enough space for another event
\********************************************************************/
{
  if (dm.pa == NULL || (dm.area1.full && dm.area2.full))
    return TRUE;
  return FALSE;
}

/*------------------------------------------------------------------*/
INLINE BOOL dm_active_full (void)
/********************************************************************\
  Routine: dm_active_full

  Purpose: Test if there is sufficient space in either event buffer 
	         for another event.
  Input:
    none
  Output:
    none
  Function value:
    BOOL	       TRUE if not enough space for another event
\********************************************************************/
{
  /* catch both full areas, waiting for transfer */
  if (dm.pa == NULL)
    return TRUE;
  /* Check the space in the active buffer only
     as I don't switch buffer here */
  if (dm.pa->full)
    return TRUE;
  return ( ((PTYPE)dm.pa->pb - (PTYPE)dm.pa->pw) < (INT)
           (dm_user_max_event_size+sizeof(EVENT_HEADER)+sizeof(INT)));
}

/*------------------------------------------------------------------*/
DWORD dm_buffer_time_get (void)
/********************************************************************\
  Routine: dm_buffer_time_get

  Purpose: return the time from the last buffer switch.

  Input:
    none
  Output:
    none
  Function value:
    DWORD        time stamp

\********************************************************************/
{
  return (dm.last_active);
}

/*------------------------------------------------------------------*/

EVENT_HEADER *dm_pointer_get(void)
/********************************************************************\
  Routine: dm_pointer_get

  Purpose: Get pointer to next free location in event buffer.
           after 10sec tries, it times out return NULL indicating a 
           serious problem, i.e. abort.
  REMARK : Cannot be called twice in a raw due to +sizeof(INT)
  Input:
    none
  Output:
    DM_BUFFER * dm    local valid dm to work on
  Function value:
    EVENT_HEADER *    Pointer to free location
    NULL              cannot after several attempt get free space => abort
\********************************************************************/
{
  int timeout, status;
  
  /* Is there still space in the active area ? */
  if (!dm_active_full())
    return (EVENT_HEADER *) (dm.pa->pw + sizeof(INT));
  
  /* no more space => switch area */

  /* Tag current area with global dm.serial for order consistency */
  dm.pa->serial = dm.serial++;

  /* set active buffer time stamp */
  dm.last_active = ss_millitime();
  
  /* mark current area full */
  dm.pa->full = TRUE;
  
  /* Trigger/do data transfer (Now/don't wait) */
  if ((status = dm_area_send()) == RPC_NET_ERROR)
    {
      cm_msg(MERROR,"dm_pointer_get()","Net error or timeout %i",status);
      return NULL;
    }
  
  /* wait switch completion (max 10 sec) */
  timeout = ss_millitime();   /* with timeout */
  while ((ss_millitime() - timeout) < 10000)
    { 
      dm.pa = dm_area_switch();
      if (dm.pa != NULL)
        return (EVENT_HEADER *) (dm.pa->pw + sizeof(INT));
      ss_sleep(200);
#ifdef DM_DEBUG
          printf(" waiting for space ... %i  dm_buffer  %i %i %i %i %i \n",ss_millitime() - timeout,
                 dm.area1.full, dm.area2.full, dm.area1.serial, dm.area2.serial, dm.serial);
#endif
    }
  
  /* Time running out abort */
  cm_msg(MERROR,"dm_pointer_get","Timeout due to buffer full");
  return NULL;
}

/*------------------------------------------------------------------*/

dm_pointer_increment(INT buffer_handle, INT event_size)
/********************************************************************\
  Routine: dm_pointer_increment

  Purpose: Increment write pointer of event buffer after an event
           has been copied into the buffer (at an address previously
           obtained via dm_pointer_get)
  Input:
    INT buffer_handle         Buffer handle event should be sent to
    INT event_size            Event size in bytes including header
  Output:
    none
  Function value:
    CM_SUCCESS                Successful completion
    status                    from bm_send_event for local connection
\********************************************************************/
{
INT aligned_event_size;

  /* if not connected remotely, use bm_send_event */
  if (_send_sock == 0)
    {
    *((INT *) dm.pa->pw) = buffer_handle;
    return bm_send_event(buffer_handle, dm.pa->pw+sizeof(INT), event_size, SYNC);
    }
  aligned_event_size = ALIGN(event_size);
  
  *((INT *) dm.pa->pw) = buffer_handle;

  /* adjust write pointer */
  dm.pa->pw += sizeof(INT) + aligned_event_size;

  /* adjust end pointer */
  dm.pa->pe = dm.pa->pw;

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/
INLINE INT dm_buffer_send(DMEM_AREA * larea)
/********************************************************************\
  Routine: dm_buffer_send

  Purpose: Ship data to the cache in fact!
           Basically the same do loop is done in the send_tcp.
           but _opt_tcp_size preveal if <= NET_TCP_SIZE.
           Introduced for bringing tcp option to user code.
  Input:
    DMEM_AREA * larea   The area to work with.
  Output:
    none
  Function value:
    CM_SUCCESS       Successful completion
    DM_ACTIVE_NULL   Both area were/are full
    RPC_NET_ERROR    send error
\********************************************************************/
{
  INT  tot_size, nwrite;
  char * lpt;

  /* if not connected remotely, use bm_send_event */
 if (_send_sock == 0)
	return bm_flush_cache(*((INT *) dm.pa->pw), ASYNC);
	
	/* alias */
  lpt = larea->pt;

  /* Get overall buffer size */
  tot_size = (PTYPE) larea->pe - (PTYPE) lpt;

  /* shortcut for speed */
  if (tot_size == 0)
      return CM_SUCCESS;
  
#ifdef DM_DEBUG  
  printf("lpt:%p size:%i ",lpt, tot_size);
#endif
  nwrite = send_tcp(_send_sock, lpt, tot_size, 0);
#ifdef DM_DEBUG  
  printf("nwrite:%i  errno:%i\n",nwrite, errno);
#endif
  if (nwrite < 0)
    return RPC_NET_ERROR;

  /* reset area */
  larea->pw = larea->pe = larea->pt;
  larea->full = FALSE;
  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/
INT dm_area_send(void)
/********************************************************************\
  Routine: dm_area_send

  Purpose: Empty the FULL area only in proper event order
           Meant to be use either in mfe.c scheduler on every event
           
  Dual memory scheme: 
   DM_DUAL_THREAD : Trigger sem_send
   !DM_DUAL_THREAD: empty full buffer in order, return active to area1
                    if dm.pa is NULL (were both full) and now both are empty

  Input:
    none
  Output:
    none
  Function value:
    CM_SUCCESS                Successful completion
    RPC_NET_ERROR             send error
\********************************************************************/
{
#ifdef DM_DUAL_THREAD
  INT status; 
  
  /* force a DM_SEND if possible. Don't wait for completion */
  dm.action = DM_SEND;
  ss_mutex_release(dm.sem_send);
#ifdef OS_WINNT
  /* necessary for true MUTEX (NT) */
  status = ss_mutex_wait_for(dm.sem_send, 1);
  if (status == SS_NO_MUTEX)
    {
      printf(" timeout while waiting for sem_send\n");
      return RPC_NET_ERROR;
    }
#endif
  
  return CM_SUCCESS;
#else
  /* ---------- NOT IN DUAL THREAD -----------*/
  INT status;
  
  /* if no DUAL thread everything is local then */
  /* select the full area */
  if (dm.area1.full && dm.area2.full)
    if (dm.area1.serial <= dm.area2.serial)
      status = dm_buffer_send(&dm.area1);
    else
      status = dm_buffer_send(&dm.area2);
  else if (dm.area1.full)
    status = dm_buffer_send(&dm.area1);
  else if (dm.area2.full)
    status = dm_buffer_send(&dm.area2);
  if (status != CM_SUCCESS)
    return status;                  /* catch transfer error too */

  if (dm.pa == NULL)
    {
      printf(" sync send dm.pa:%p full 1%i 2%i\n", dm.pa, dm.area1.full, dm.area2.full);
      dm.pa = &dm.area1;
    }
  return CM_SUCCESS;
#endif
}

/*------------------------------------------------------------------*/
INT dm_task(void *pointer)
/********************************************************************\
  Routine: dm_task

  Purpose: async send events doing a double purpose:
  a) send full buffer if found (DM_SEND) set by dm_active_full
  b) flush full areas (DM_FLUSH) set by dm_area_flush
  Input:
  none
  Output:
  none
  Function value:
  none
  \********************************************************************/
{
#ifdef DM_DUAL_THREAD
  INT status, timeout;
  
  printf("Semaphores initialization ... in areaSend ");
  /* Check or Wait for semaphore to be setup */
  timeout = ss_millitime();
  while ((ss_millitime()-timeout < 3000) && (dm.sem_send == 0))
    ss_sleep(200);
  if (dm.sem_send == 0)
    goto kill;
  
#ifdef OS_WINNT
  /* necessary for true MUTEX (NT) get semaphore */
  ss_mutex_wait_for(dm.sem_flush, 0);
#endif
  
  /* Main FOREVER LOOP */
  printf("task areaSend ready...\n");
  while (1)
    {
      if (!dm_area_full())
        {
          /* wait semaphore here ........ 0 == forever*/ 
          ss_mutex_wait_for(dm.sem_send, 0);
#ifdef OS_WINNT
          /* necessary for true MUTEX (NT) give semaphore */
          ss_mutex_release(dm.sem_send);
#endif
        }
      if (dm.action == DM_SEND)                            
        {
#ifdef DM_DEBUG 
          printf("Send %i %i ", dm.area1.full, dm.area2.full);
#endif
          /* DM_SEND : Empty the oldest buffer only. */
          if (dm.area1.full && dm.area2.full)
            {
              if (dm.area1.serial <= dm.area2.serial)
                status = dm_buffer_send(&dm.area1);
              else
                status = dm_buffer_send(&dm.area2);
            }
          else if (dm.area1.full)
            status = dm_buffer_send(&dm.area1);
          else if (dm.area2.full)
            status = dm_buffer_send(&dm.area2);
          
          if (status != CM_SUCCESS)
            {
              cm_msg(MERROR,"dm_task","network error %i", status);
              goto kill;
            }
        } /* if DM_SEND */
      else if (dm.action == DM_FLUSH)
        {
	  /* DM_FLUSH: User is waiting for completion (i.e. No more incomming 
	     events) Empty both area in order independently of being full or not */
          if (dm.area1.serial <= dm.area2.serial)
            {
              status = dm_buffer_send(&dm.area1);
              if (status != CM_SUCCESS)
                goto error;
              status = dm_buffer_send(&dm.area2);
              if (status != CM_SUCCESS)
                goto error;
            }
          else
            {
              status = dm_buffer_send(&dm.area2);
              if (status != CM_SUCCESS)
                goto error;
              status = dm_buffer_send(&dm.area1);
              if (status != CM_SUCCESS)
                goto error;
            }
          /* reset counter */              
          dm.area1.serial = 0;
          dm.area2.serial = dm.serial = 1;
#ifdef DM_DEBUG              
          printf("dm.action: Flushing ...\n");
#endif
          /* reset area to #1 */
          dm.pa = &dm.area1;
          
          /* release user */
          ss_mutex_release(dm.sem_flush);
#ifdef OS_WINNT
          /* necessary for true MUTEX (NT) get semaphore back */
          ss_mutex_wait_for(dm.sem_flush, 0);
#endif
        } /* if FLUSH */
      
      if (dm.action == DM_KILL)
        goto kill;
      
    } /* FOREVER (go back wainting for semaphore) */
  
  /* kill spawn now */
 error:
  cm_msg(MERROR,"dm_area_flush","aSync Net error");
 kill:
  ss_mutex_release(dm.sem_flush);
#ifdef OS_WINNT
  ss_mutex_wait_for(dm.sem_flush, 1);
#endif
  cm_msg(MERROR,"areaSend","task areaSend exiting now");  
  exit;
#else
  printf("DM_DUAL_THREAD not defined\n");
  return 0;
#endif
  return 1;
}

/*------------------------------------------------------------------*/
INT dm_area_flush(void)
/********************************************************************\
  Routine: dm_area_flush

  Purpose: Flush all the events in the areas.
           Used in mfe for BOR events, periodic events and
           if rate to low in main loop once a second. The standard
           data transfer should be done/triggered by dm_area_send (sync/async)
           in dm_pointer_get(). 
  Input:
    none
  Output:
    none
  Function value:
    CM_SUCCESS       Successful completion
    RPC_NET_ERROR    send error
\********************************************************************/
{
  INT status;
#ifdef DM_DUAL_THREAD
  /* request FULL flush */
  dm.action = DM_FLUSH;
  ss_mutex_release(dm.sem_send);
#ifdef OS_WINNT
  /* necessary for true MUTEX (NT) get semaphore back */
  ss_mutex_wait_for(dm.sem_send, 0);
#endif
  
  /* important to wait for completion before continue with timeout
     timeout specified milliseconds */
  status = ss_mutex_wait_for(dm.sem_flush, 10000);
#ifdef DM_DEBUG
  printf("dm_area_flush after waiting %i\n",status);
#endif  
#ifdef OS_WINNT  
  ss_mutex_release(dm.sem_flush);   /* give it back now */
#endif
  
  return status;
#else
  /* full flush done here */
  /* select in order both area independently of being full or not*/
  if (dm.area1.serial <= dm.area2.serial)
    {
      status = dm_buffer_send(&dm.area1);
      if (status != CM_SUCCESS)
        return status;
      status = dm_buffer_send(&dm.area2);
      if (status != CM_SUCCESS)
        return status;
    }
  else
    {
      status = dm_buffer_send(&dm.area2);
      if (status != CM_SUCCESS)
        return status;
      status = dm_buffer_send(&dm.area1);
      if (status != CM_SUCCESS)
        return status;
    }
  /* reset serial counter */
  dm.area1.serial = dm.area2.serial = 0;
  dm.last_active = ss_millitime();
  return CM_SUCCESS;
#endif
}

/*------------------------------------------------------------------*/
