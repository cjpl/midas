/********************************************************************\

  Name:         MIDAS.C
  Created by:   Stefan Ritt

  Contents:     MIDAS main library funcitons

  $Id$

\********************************************************************/

#include "midas.h"
#include "msystem.h"
#include "strlcpy.h"
#include <assert.h>
#include <signal.h>

/* SVN revision number. This value will be changed by the Subversion
   system automatically on every revision of midas.c. It will be
   returned by cm_get_revision(), for example in the odbedit "ver"
   command */
char *svn_revision = "$Rev$";

/**dox***************************************************************/
/** @file midas.c
The main core C-code for Midas.
*/

/** @defgroup cmfunctionc Midas Common Functions (cm_xxx)
 */
/** @defgroup bmfunctionc Midas Buffer Manager Functions (bm_xxx)
 */
/** @defgroup msgfunctionc Midas Message Functions (msg_xxx)
 */
/** @defgroup bkfunctionc Midas Bank Functions (bk_xxx)
 */
/** @defgroup rpcfunctionc Midas RPC Functions (rpc_xxx)
 */
/** @defgroup dmfunctionc Midas Dual Buffer Memory Functions (dm_xxx)
 */
/** @defgroup rbfunctionc Midas Ring Buffer Functions (rb_xxx)
 */

/**dox***************************************************************/
/** @addtogroup midasincludecode
 *
 *  @{  */

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************/
/* data type sizes */
INT tid_size[] = {
   0,                           /* tid == 0 not defined                               */
   1,                           /* TID_BYTE      unsigned byte         0       255    */
   1,                           /* TID_SBYTE     signed byte         -128      127    */
   1,                           /* TID_CHAR      single character      0       255    */
   2,                           /* TID_WORD      two bytes             0      65535   */
   2,                           /* TID_SHORT     signed word        -32768    32767   */
   4,                           /* TID_DWORD     four bytes            0      2^32-1  */
   4,                           /* TID_INT       signed dword        -2^31    2^31-1  */
   4,                           /* TID_BOOL      four bytes bool       0        1     */
   4,                           /* TID_FLOAT     4 Byte float format                  */
   8,                           /* TID_DOUBLE    8 Byte float format                  */
   1,                           /* TID_BITFIELD  8 Bits Bitfield    00000000 11111111 */
   0,                           /* TID_STRING    zero terminated string               */
   0,                           /* TID_ARRAY     variable length array of unkown type */
   0,                           /* TID_STRUCT    C structure                          */
   0,                           /* TID_KEY       key in online database               */
   0                            /* TID_LINK      link in online database              */
};

/* data type names */
char *tid_name[] = {
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

struct {
   int transition;
   char name[32];
} trans_name[] = {
   {
   TR_START, "START",}, {
   TR_STOP, "STOP",}, {
   TR_PAUSE, "PAUSE",}, {
   TR_RESUME, "RESUME",}, {
   TR_STARTABORT, "STARTABORT",}, {
   TR_DEFERRED, "DEFERRED",}, {
0, "",},};

/* Globals */
#ifdef OS_MSDOS
extern unsigned _stklen = 60000U;
#endif

extern DATABASE *_database;
extern INT _database_entries;

static BUFFER *_buffer;
static INT _buffer_entries = 0;

static INT _msg_buffer = 0;
static INT _msg_rb = 0;
static MUTEX_T *_msg_mutex = NULL;
static void (*_msg_dispatch) (HNDLE, HNDLE, EVENT_HEADER *, void *);

static REQUEST_LIST *_request_list;
static INT _request_list_entries = 0;

static EVENT_HEADER *_event_buffer;
static INT _event_buffer_size = 0;

static char *_net_recv_buffer;
static INT _net_recv_buffer_size = 0;
static INT _net_recv_buffer_size_odb = 0;

static char *_net_send_buffer;
static INT _net_send_buffer_size = 0;

static char *_tcp_buffer = NULL;
static INT _tcp_wp = 0;
static INT _tcp_rp = 0;
static INT _rpc_sock = 0;
static MUTEX_T *_mutex_rpc = NULL;

static void (*_debug_print) (char *) = NULL;
static INT _debug_mode = 0;

static INT _watchdog_last_called = 0;

int _rpc_connect_timeout = 10000;

/* table for transition functions */

typedef struct {
   INT transition;
   INT sequence_number;
    INT(*func) (INT, char *);
} TRANS_TABLE;

#define MAX_TRANSITIONS 20

TRANS_TABLE _trans_table[MAX_TRANSITIONS];

TRANS_TABLE _deferred_trans_table[] = {
   {TR_START, 0, NULL},
   {TR_STOP, 0, NULL},
   {TR_PAUSE, 0, NULL},
   {TR_RESUME, 0, NULL},
   {0, 0, NULL}
};

static BOOL _server_registered = FALSE;

static INT rpc_transition_dispatch(INT idx, void *prpc_param[]);

void cm_ctrlc_handler(int sig);

typedef struct {
   INT code;
   char *string;
} ERROR_TABLE;

ERROR_TABLE _error_table[] = {
   {CM_WRONG_PASSWORD, "Wrong password"},
   {CM_UNDEF_EXP, "Experiment not defined"},
   {CM_UNDEF_ENVIRON,
    "\"exptab\" file not found and MIDAS_DIR environment variable not defined"},
   {RPC_NET_ERROR, "Cannot connect to remote host"},
   {0, NULL}
};


typedef struct {
   void *adr;
   int size;
   char file[80];
   int line;
} DBG_MEM_LOC;

DBG_MEM_LOC *_mem_loc = NULL;
INT _n_mem = 0;

void *dbg_malloc(unsigned int size, char *file, int line)
{
   FILE *f;
   void *adr;
   int i;

   adr = malloc(size);

   /* search for deleted entry */
   for (i = 0; i < _n_mem; i++)
      if (_mem_loc[i].adr == NULL)
         break;

   if (i == _n_mem) {
      _n_mem++;
      if (!_mem_loc)
         _mem_loc = (DBG_MEM_LOC *) malloc(sizeof(DBG_MEM_LOC));
      else
         _mem_loc = (DBG_MEM_LOC *) realloc(_mem_loc, sizeof(DBG_MEM_LOC) * _n_mem);
   }

   _mem_loc[i].adr = adr;
   _mem_loc[i].size = size;
   strcpy(_mem_loc[i].file, file);
   _mem_loc[i].line = line;

   f = fopen("mem.txt", "w");
   for (i = 0; i < _n_mem; i++)
      if (_mem_loc[i].adr)
         fprintf(f, "%s:%d size=%d adr=%p\n", _mem_loc[i].file, _mem_loc[i].line, _mem_loc[i].size,
                 _mem_loc[i].adr);
   fclose(f);

   return adr;
}

void *dbg_calloc(unsigned int size, unsigned int count, char *file, int line)
{
   void *adr;

   adr = dbg_malloc(size * count, file, line);
   if (adr)
      memset(adr, 0, size * count);

   return adr;
}

void dbg_free(void *adr, char *file, int line)
{
   FILE *f;
   int i;

   free(adr);

   for (i = 0; i < _n_mem; i++)
      if (_mem_loc[i].adr == adr)
         break;

   if (i < _n_mem)
      _mem_loc[i].adr = NULL;

   f = fopen("mem.txt", "w");
   for (i = 0; i < _n_mem; i++)
      if (_mem_loc[i].adr)
         fprintf(f, "%s:%d %s:%d size=%d adr=%p\n", _mem_loc[i].file, _mem_loc[i].line,
                 file, line, _mem_loc[i].size, _mem_loc[i].adr);
   fclose(f);
}

/********************************************************************/
static int resize_net_send_buffer(const char* from, int new_size)
/********************************************************************/
{
   if (new_size <= _net_send_buffer_size)
      return SUCCESS;

   _net_send_buffer = (char *) realloc(_net_send_buffer, new_size);
   if (_net_send_buffer == NULL) {
      cm_msg(MERROR, from, "Cannot allocate %d bytes for network buffer", new_size);
      return RPC_EXCEED_BUFFER;
   }

   //printf("reallocate_net_send_buffer %p size %d->%d from %s\n", _net_send_buffer, _net_send_buffer_size, new_size, from);
   _net_send_buffer_size = new_size;
   return SUCCESS;
}

/********************************************************************\
*                                                                    *
*              Common message functions                              *
*                                                                    *
\********************************************************************/

static int (*_message_print) (const char *) = puts;
static INT _message_mask_system = MT_ALL;
static INT _message_mask_user = MT_ALL;


/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/**dox***************************************************************/
/** @addtogroup msgfunctionc
 *
 *  @{  */

/********************************************************************/
/**
Convert error code to string. Used after cm_connect_experiment to print
error string in command line programs or windows programs.
@param code Error code as defined in midas.h
@param string Error string
@return CM_SUCCESS
*/
INT cm_get_error(INT code, char *string)
{
   INT i;

   for (i = 0; _error_table[i].code; i++)
      if (_error_table[i].code == code) {
         strcpy(string, _error_table[i].string);
         return CM_SUCCESS;
      }

   sprintf(string, "Unexpected error #%d", code);
   return CM_SUCCESS;
}

/********************************************************************/
/**
Set message masks. When a message is generated by calling cm_msg(),
it can got to two destinatinons. First a user defined callback routine
and second to the "SYSMSG" buffer.

A user defined callback receives all messages which satisfy the user_mask.

\code
int message_print(const char *msg)
{
  char str[160];

  memset(str, ' ', 159);
  str[159] = 0;
  if (msg[0] == '[')
    msg = strchr(msg, ']')+2;
  memcpy(str, msg, strlen(msg));
  ss_printf(0, 20, str);
  return 0;
}
...
  cm_set_msg_print(MT_ALL, MT_ALL, message_print);
...
\endcode
@param system_mask Bit masks for MERROR, MINFO etc. to send system messages.
@param user_mask Bit masks for MERROR, MINFO etc. to send messages to the user callback.
@param func Function which receives all printout. By setting "puts",
       messages are just printed to the screen.
@return CM_SUCCESS
*/
INT cm_set_msg_print(INT system_mask, INT user_mask, int (*func) (const char *))
{
   _message_mask_system = system_mask;
   _message_mask_user = user_mask;
   _message_print = func;

   return BM_SUCCESS;
}

/********************************************************************/
/**
Write message to logging file. Called by cm_msg.
@attention May burn your fingers
@param message_type      Message type
@param message          Message string
@return CM_SUCCESS
*/
INT cm_msg_log(INT message_type, const char *message)
{
   char dir[256];
   char filename[256];
   char path[256];
   char str[256];
   INT status, size, fh, semaphore;
   HNDLE hDB, hKey;

   if (rpc_is_remote())
      return rpc_call(RPC_CM_MSG_LOG, message_type, message);

   if (message_type != MT_DEBUG) {
      cm_get_experiment_database(&hDB, NULL);

      if (hDB) {

         strcpy(filename, "midas.log");
         size = sizeof(filename);
         db_get_value(hDB, 0, "/Logger/Message file", filename, &size, TID_STRING, TRUE);

         if (strchr(filename, '%')) {
            /* replace stings such as midas_%y%m%d.mid with current date */
            time_t now;
            struct tm *tms;

            tzset();
            time(&now);
            tms = localtime(&now);

            strftime(str, sizeof(str), filename, tms);
            strlcpy(filename, str, sizeof(filename));
         }

         if (strchr(filename, DIR_SEPARATOR) == NULL) {
            status = db_find_key(hDB, 0, "/Logger/Data dir", &hKey);
            if (status == DB_SUCCESS) {
               size = sizeof(dir);
               memset(dir, 0, size);
               db_get_value(hDB, 0, "/Logger/Data dir", dir, &size, TID_STRING, TRUE);
               if (dir[0] != 0)
                  if (dir[strlen(dir) - 1] != DIR_SEPARATOR)
                     strcat(dir, DIR_SEPARATOR_STR);

               strcpy(path, dir);
               strcat(path, filename);
            } else {
               cm_get_path(dir);
               if (dir[0] != 0)
                  if (dir[strlen(dir) - 1] != DIR_SEPARATOR)
                     strcat(dir, DIR_SEPARATOR_STR);

               strcpy(path, dir);
               strcat(path, "midas.log");
            }
         } else {
            strcpy(path, filename);
         }
      } else
         strcpy(path, "midas.log");

      fh = open(path, O_WRONLY | O_CREAT | O_APPEND | O_LARGEFILE, 0644);
      if (fh < 0) {
         printf("Cannot open message log file %s\n", path);
      } else {

         cm_get_experiment_semaphore(NULL, NULL, NULL, &semaphore);
         status = ss_semaphore_wait_for(semaphore, 5 * 1000);

         strcpy(str, ss_asctime());
         write(fh, str, strlen(str));
         write(fh, " ", 1);
         write(fh, message, strlen(message));
         write(fh, "\n", 1);
         close(fh);

         ss_semaphore_release(semaphore);
      }
   }

   return CM_SUCCESS;
}

/********************************************************************/
/**
Write message to logging file. Called by cm_msg().
@internal
@param message_type      Message type
@param message          Message string
@param facility         Message facility, filename in which messages will be written
@return CM_SUCCESS
*/
INT cm_msg_log1(INT message_type, const char *message, const char *facility)
/********************************************************************\

  Routine: cm_msg_log1

  Purpose: Write message to logging file. Called by cm_msg.
           Internal use only

  Input:
    INT    message_type      Message type
    char   *message          Message string
    char   *

  Output:
    none

  Function value:
    CM_SUCCESS

\********************************************************************/
{
   char dir[256];
   char filename[256];
   char path[256];
   char str[256];
   FILE *f;
   INT status, size;
   HNDLE hDB, hKey;


   if (rpc_is_remote())
      return rpc_call(RPC_CM_MSG_LOG1, message_type, message, facility);

   if (message_type != MT_DEBUG) {
      cm_get_experiment_database(&hDB, NULL);

      if (hDB) {
         strcpy(filename, "midas.log");
         size = sizeof(filename);
         db_get_value(hDB, 0, "/Logger/Message file", filename, &size, TID_STRING, TRUE);

         if (strchr(filename, '%')) {
            /* replace stings such as midas_%y%m%d.mid with current date */
            time_t now;
            struct tm *tms;

            tzset();
            time(&now);
            tms = localtime(&now);

            strftime(str, sizeof(str), filename, tms);
            strlcpy(filename, str, sizeof(filename));
         }

         if (strchr(filename, DIR_SEPARATOR) == NULL) {

            status = db_find_key(hDB, 0, "/Logger/Data dir", &hKey);
            if (status == DB_SUCCESS) {
               size = sizeof(dir);
               memset(dir, 0, size);
               db_get_value(hDB, 0, "/Logger/Data dir", dir, &size, TID_STRING, TRUE);
               if (dir[0] != 0)
                  if (dir[strlen(dir) - 1] != DIR_SEPARATOR)
                     strcat(dir, DIR_SEPARATOR_STR);

               if (facility[0]) {
                  strcpy(filename, facility);
                  strcat(filename, ".log");
               } else {
                  strcpy(filename, "midas.log");
                  size = sizeof(filename);
                  db_get_value(hDB, 0, "/Logger/Message file", filename, &size, TID_STRING, TRUE);

                  if (strchr(filename, '%')) {
                     /* replace stings such as midas_%y%m%d.mid with current date */
                     time_t now;
                     struct tm *tms;

                     tzset();
                     time(&now);
                     tms = localtime(&now);

                     strftime(str, sizeof(str), filename, tms);
                     strlcpy(filename, str, sizeof(filename));
                  }
               }

               strcpy(path, dir);
               strcat(path, filename);
            } else {
               cm_get_path(dir);
               if (dir[0] != 0)
                  if (dir[strlen(dir) - 1] != DIR_SEPARATOR)
                     strcat(dir, DIR_SEPARATOR_STR);

               strcpy(path, dir);
               if (facility[0]) {
                  strcat(path, facility);
                  strcat(path, ".log");
               } else
                  strcat(path, "midas.log");
            }
         } else {
            strcpy(path, filename);
            *(strrchr(path, DIR_SEPARATOR) + 1) = 0;
            if (facility[0]) {
               strcat(path, facility);
               strcat(path, ".log");
            } else
               strcat(path, "midas.log");
         }
      } else {
         if (facility[0]) {
            strcpy(path, facility);
            strcat(path, ".log");
         } else
            strcpy(path, "midas.log");
      }

      f = fopen(path, "a");
      if (f == NULL) {
         printf("Cannot open message log file %s\n", path);
      } else {
         strcpy(str, ss_asctime());
         fprintf(f, "%s", str);
         fprintf(f, " %s\n", message);
         fclose(f);
      }
   }

   return CM_SUCCESS;
}

static INT cm_msg_format(char* local_message, int sizeof_local_message, char* send_message, int sizeof_send_message, INT message_type, const char *filename, INT line, const char *routine, const char *format, va_list* argptr)
{
   char type_str[256], str[1000], format_cpy[900];
   const char *pc;

   /* strip path */
   pc = filename + strlen(filename);
   while (*pc != '\\' && *pc != '/' && pc != filename)
      pc--;
   if (pc != filename)
      pc++;

   /* convert type to string */
   type_str[0] = 0;
   if (message_type & MT_ERROR)
      strlcat(type_str, MT_ERROR_STR, sizeof(type_str));
   if (message_type & MT_INFO)
      strlcat(type_str, MT_INFO_STR, sizeof(type_str));
   if (message_type & MT_DEBUG)
      strlcat(type_str, MT_DEBUG_STR, sizeof(type_str));
   if (message_type & MT_USER)
      strlcat(type_str, MT_USER_STR, sizeof(type_str));
   if (message_type & MT_LOG)
      strlcat(type_str, MT_LOG_STR, sizeof(type_str));
   if (message_type & MT_TALK)
      strlcat(type_str, MT_TALK_STR, sizeof(type_str));

   /* print client name into string */
   if (message_type == MT_USER)
      sprintf(send_message, "[%s] ", routine);
   else {
      rpc_get_name(str);
      if (str[0])
         sprintf(send_message, "[%s,%s] ", str, type_str);
      else
         send_message[0] = 0;
   }

   local_message[0] = 0;

   /* preceed error messages with file and line info */
   if (message_type == MT_ERROR) {
      sprintf(str, "[%s:%d:%s,%s] ", pc, line, routine, type_str);
      strlcat(send_message, str, sizeof_send_message);
      strlcat(local_message, str, sizeof_send_message);
   } else if (message_type == MT_USER)
      sprintf(local_message, "[%s,%s] ", routine, type_str);

   /* limit length of format */
   strlcpy(format_cpy, format, sizeof(format_cpy));

   /* print argument list into message */
   vsprintf(str, (char *) format, *argptr);

   strlcat(send_message, str, sizeof_send_message);
   strlcat(local_message, str, sizeof_local_message);

   return CM_SUCCESS;
}

static INT cm_msg_send_event(INT ts, INT message_type, const char *send_message)
{
   int status;
   char event[1000];
   EVENT_HEADER *pevent;

   //printf("cm_msg_send: ts %d, type %d, message [%s]\n", ts, message_type, send_message);

   /* copy message to event */
   pevent = (EVENT_HEADER *) event;
   strlcpy(event + sizeof(EVENT_HEADER), send_message, sizeof(event) - sizeof(EVENT_HEADER));

   /* send event if not of type MLOG */
   if (message_type != MT_LOG) {
      /* if no message buffer already opened, do so now */
      if (_msg_buffer == 0) {
         status = bm_open_buffer(MESSAGE_BUFFER_NAME, MESSAGE_BUFFER_SIZE, &_msg_buffer);
         if (status != BM_SUCCESS && status != BM_CREATED) {
            return status;
         }
      }

      /* setup the event header and send the message */
      bm_compose_event(pevent, EVENTID_MESSAGE, (WORD) message_type, strlen(event + sizeof(EVENT_HEADER)) + 1, 0);
      pevent->time_stamp = ts;
      bm_send_event(_msg_buffer, event, pevent->data_size + sizeof(EVENT_HEADER), SYNC);
   }

   return CM_SUCCESS;
}

static INT cm_msg_buffer(int ts, int message_type, const char *message)
{
   int status;
   int len;
   char *wp;
   void *vp;

   //printf("cm_msg_buffer ts %d, type %d, message [%s]!\n", ts, message_type, message);

   if (!_msg_rb) {
      status = rb_create(100*1024, 1024, &_msg_rb);
      assert(status==SUCCESS);

      status = ss_mutex_create(&_msg_mutex);
      assert(status==SS_SUCCESS || status==SS_CREATED);
   }

   len = strlen(message) + 1;

   // lock
   status = ss_mutex_wait_for(_msg_mutex, 0);
   assert(status == SS_SUCCESS);

   status = rb_get_wp(_msg_rb, &vp, 1000);
   wp = (char *)vp;
   
   if (status != SUCCESS || wp == NULL) {
      // unlock
      ss_mutex_release(_msg_mutex);
      return SS_NO_MEMORY;
   }

   *wp++ = 'M';
   *wp++ = 'S';
   *wp++ = 'G';
   *wp++ = '_';
   *(int*)wp = ts;
   wp += sizeof(int);
   *(int*)wp = message_type;
   wp += sizeof(int);
   *(int*)wp = len;
   wp += sizeof(int);
   memcpy(wp, message, len);
   rb_increment_wp(_msg_rb, 4+3*sizeof(int)+len);

   // unlock
   ss_mutex_release(_msg_mutex);

   return CM_SUCCESS;
}

/********************************************************************/
/**
This routine can be called to process messages buffered by cm_msg(). Normally
it is called from cm_yield() and cm_disconnect_experiment() to make sure
all accumulated messages are processed.
*/
INT cm_msg_flush_buffer()
{
   int i;

   //printf("cm_msg_flush_buffer!\n");

   if (!_msg_rb)
     return CM_SUCCESS;

   for (i=0; i<100; i++) {
      int status;
      int ts;
      int message_type;
      char message[1024];
      int n_bytes;
      char *rp;
      void *vp;
      int len;

      status = rb_get_buffer_level(_msg_rb, &n_bytes);
      
      if (status != SUCCESS || n_bytes <= 0)
	break;

      // lock
      status = ss_mutex_wait_for(_msg_mutex, 0);
      assert(status == SS_SUCCESS);
      
      status = rb_get_rp(_msg_rb, &vp, 0);
      rp = (char *)vp;
      if (status != SUCCESS || rp == NULL) {
         // unlock
         ss_mutex_release(_msg_mutex);
         return SS_NO_MEMORY;
      }

      assert(rp);
      assert(rp[0]=='M');
      assert(rp[1]=='S');
      assert(rp[2]=='G');
      assert(rp[3]=='_');
      rp += 4;

      ts = *(int*)rp;
      rp += sizeof(int);

      message_type = *(int*)rp;
      rp += sizeof(int);

      len = *(int*)rp;
      rp += sizeof(int);

      strlcpy(message, rp, sizeof(message));
      
      rb_increment_rp(_msg_rb, 4+3*sizeof(int)+len);

      // unlock
      ss_mutex_release(_msg_mutex);
      
      /* log message */
      cm_msg_log(message_type, message);

      /* send message to SYSMSG */
      status = cm_msg_send_event(ts, message_type, message);
      if (status != CM_SUCCESS)
         return status;
   }

   return CM_SUCCESS;
}

/********************************************************************/
/**
This routine can be called whenever an internal error occurs
or an informative message is produced. Different message
types can be enabled or disabled by setting the type bits
via cm_set_msg_print().
@attention Do not add the "\n" escape carriage control at the end of the
formated line as it is already added by the client on the receiving side.
\code
   ...
   cm_msg(MINFO, "my program", "This is a information message only);
   cm_msg(MERROR, "my program", "This is an error message with status:%d", my_status);
   cm_msg(MTALK, "my_program", My program is Done!");
   ...
\endcode
@param message_type (See @ref midas_macro).
@param filename Name of source file where error occured
@param line Line number where error occured
@param routine Routine name.
@param format message to printout, ... Parameters like for printf()
@return CM_SUCCESS
*/
INT cm_msg(INT message_type, const char *filename, INT line, const char *routine, const char *format, ...)
{
   va_list argptr;
   char local_message[1000], send_message[1000];
   INT status;
   static BOOL in_routine = FALSE;
   int ts = ss_time();

   /* avoid recursive calls */
   if (in_routine)
      return CM_SUCCESS;

   in_routine = TRUE;

   /* print argument list into message */
   va_start(argptr, format);
   cm_msg_format(local_message, sizeof(local_message), send_message, sizeof(send_message), message_type, filename, line, routine, format, &argptr);
   va_end(argptr);

   /* call user function if set via cm_set_msg_print */
   if (_message_print != NULL && (message_type & _message_mask_user) != 0)
      _message_print(local_message);

   /* return if system mask is not set */
   if ((message_type & _message_mask_system) == 0) {
      in_routine = FALSE;
      return CM_SUCCESS;
   }

   status = cm_msg_buffer(ts, message_type, send_message);

   in_routine = FALSE;

   return status;
}

/********************************************************************/
/**
This routine is similar to @ref cm_msg().
It differs from cm_msg() only by the logging destination being a file
given through the argument list i.e:\b facility
@internal
@attention Do not add the "\n" escape carriage control at the end of the
formated line as it is already added by the client on the receiving side.
The first arg in the following example uses the predefined
macro MINFO which handles automatically the first 3 arguments of the function
(see @ref midas_macro).
\code   ...
   cm_msg1(MINFO, "my_log_file", "my_program"," My message status:%d", status);
   ...
//----- File my_log_file.log
Thu Nov  8 17:59:28 2001 [my_program] My message status:1
\endcode
@param message_type See @ref midas_macro.
@param filename Name of source file where error occured
@param line Line number where error occured
@param facility Logging file name
@param routine Routine name
@param format message to printout, ... Parameters like for printf()
@return CM_SUCCESS
*/
INT cm_msg1(INT message_type, const char *filename, INT line,
            const char *facility, const char *routine, const char *format, ...)
{
   va_list argptr;
   char event[1000], str[256], local_message[256], send_message[256];
   const char *pc;
   EVENT_HEADER *pevent;
   INT status;
   static BOOL in_routine = FALSE;

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
   else {
      rpc_get_name(str);
      if (str[0])
         sprintf(send_message, "[%s] ", str);
      else
         send_message[0] = 0;
   }

   local_message[0] = 0;

   /* preceed error messages with file and line info */
   if (message_type == MT_ERROR) {
      sprintf(str, "[%s:%d:%s] ", pc, line, routine);
      strcat(send_message, str);
      strcat(local_message, str);
   }

   /* print argument list into message */
   va_start(argptr, format);
   vsprintf(str, (char *) format, argptr);
   va_end(argptr);

   if (facility)
      sprintf(local_message + strlen(local_message), "{%s} ", facility);

   strcat(send_message, str);
   strcat(local_message, str);

   /* call user function if set via cm_set_msg_print */
   if (_message_print != NULL && (message_type & _message_mask_user) != 0)
      _message_print(local_message);

   /* return if system mask is not set */
   if ((message_type & _message_mask_system) == 0) {
      in_routine = FALSE;
      return CM_SUCCESS;
   }

   /* copy message to event */
   pevent = (EVENT_HEADER *) event;
   strcpy(event + sizeof(EVENT_HEADER), send_message);

   /* send event if not of type MLOG */
   if (message_type != MT_LOG) {
      /* if no message buffer already opened, do so now */
      if (_msg_buffer == 0) {
         status = bm_open_buffer(MESSAGE_BUFFER_NAME, MESSAGE_BUFFER_SIZE, &_msg_buffer);
         if (status != BM_SUCCESS && status != BM_CREATED) {
            in_routine = FALSE;
            return status;
         }
      }

      /* setup the event header and send the message */
      bm_compose_event(pevent, EVENTID_MESSAGE, (WORD) message_type, strlen(event + sizeof(EVENT_HEADER)) + 1, 0);
      bm_send_event(_msg_buffer, event, pevent->data_size + sizeof(EVENT_HEADER), SYNC);
   }

   /* log message */
   cm_msg_log1(message_type, send_message, facility);

   in_routine = FALSE;

   return CM_SUCCESS;
}

/********************************************************************/
/**
Register a dispatch function for receiving system messages.
- example code from mlxspeaker.c
\code
void receive_message(HNDLE hBuf, HNDLE id, EVENT_HEADER *header, void *message)
{
  char str[256], *pc, *sp;
  // print message
  printf("%s\n", (char *)(message));

  printf("evID:%x Mask:%x Serial:%i Size:%d\n"
                 ,header->event_id
                 ,header->trigger_mask
                 ,header->serial_number
                 ,header->data_size);
  pc = strchr((char *)(message),']')+2;
  ...
  // skip none talking message
  if (header->trigger_mask == MT_TALK ||
      header->trigger_mask == MT_USER)
   ...
}

int main(int argc, char *argv[])
{
  ...
  // now connect to server
  status = cm_connect_experiment(host_name, exp_name, "Speaker", NULL);
  if (status != CM_SUCCESS)
    return 1;
  // Register callback for messages
  cm_msg_register(receive_message);
  ...
}
\endcode
@param func Dispatch function.
@return CM_SUCCESS or bm_open_buffer and bm_request_event return status
*/
INT cm_msg_register(void (*func) (HNDLE, HNDLE, EVENT_HEADER *, void *))
{
   INT status, id;

   /* if no message buffer already opened, do so now */
   if (_msg_buffer == 0) {
      status = bm_open_buffer(MESSAGE_BUFFER_NAME, MESSAGE_BUFFER_SIZE, &_msg_buffer);
      if (status != BM_SUCCESS && status != BM_CREATED)
         return status;
   }

   _msg_dispatch = func;

   status = bm_request_event(_msg_buffer, EVENTID_ALL, TRIGGER_ALL, GET_NONBLOCKING, &id, func);

   return status;
}

/* Retrieve message from an individual file. Internal use only */
INT cm_msg_retrieve1(char *filename, INT n_message, char *message, INT buf_size)
{
   FILE *f;
   INT offset, i, j;
   char *p;

   f = fopen(filename, "rb");
   if (f == NULL) {
      sprintf(message, "Cannot open message log file %s\n", filename);
      return -1;
   }

   if (buf_size <= 2)
      return 0;

   /* position buf_size bytes before the EOF */
   fseek(f, -(buf_size - 1), SEEK_END);
   offset = ftell(f);
   if (offset != 0) {
      /* go to end of line */
      fgets(message, buf_size - 1, f);
      offset = ftell(f) - offset;
      buf_size -= offset;
   }

   memset(message, 0, buf_size);
   fread(message, 1, buf_size - 1, f);
   message[buf_size - 1] = 0;
   fclose(f);

   p = message + (buf_size - 2);

   /* goto end of buffer */
   while (p != message && *p == 0)
      p--;

   /* strip line break */
   j = 0;
   while (p != message && (*p == '\n' || *p == '\r')) {
      *(p--) = 0;
      j = 1;
   }

   /* trim buffer so that last n_messages remain */
   for (i = 0; i < n_message; i++) {
      while (p != message && *p != '\n')
         p--;

      while (p != message && (*p == '\n' || *p == '\r'))
         p--;

      if (p == message)
         break;

      j++;
   }
   if (p != message) {
      p++;
      while (*p == '\n' || *p == '\r')
         p++;
   }

   buf_size = (buf_size - 1) - (p - message);

   memmove(message, p, buf_size);
   message[buf_size] = 0;

   return j;
}

/********************************************************************/
/**
Retrieve old messages from log file
@param  n_message        Number of messages to retrieve
@param  message          buf_size bytes of messages, separated
                         by \n characters. The returned number
                         of bytes is normally smaller than the
                         initial buf_size, since only full
                         lines are returned.
@param *buf_size         Size of message buffer to fill
@return CM_SUCCESS
*/
INT cm_msg_retrieve(INT n_message, char *message, INT buf_size)
{
   char dir[256];
   char filename[256], filename2[256];
   char path[256], *message2;
   INT status, size, n, i;
   HNDLE hDB, hKey;
   time_t now;
   struct tm *tms;

   if (rpc_is_remote())
      return rpc_call(RPC_CM_MSG_RETRIEVE, n_message, message, buf_size);

   cm_get_experiment_database(&hDB, NULL);
   status = 0;

   if (hDB) {
      strcpy(filename, "midas.log");
      size = sizeof(filename);
      db_get_value(hDB, 0, "/Logger/Message file", filename, &size, TID_STRING, TRUE);

      strlcpy(filename2, filename, sizeof(filename2));

      if (strchr(filename, '%')) {
         /* replace strings such as midas_%y%m%d.mid with current date */
         tzset();
         time(&now);
         tms = localtime(&now);

         strftime(filename2, sizeof(filename2), filename, tms);
      }

      if (strchr(filename2, DIR_SEPARATOR) == NULL) {
         status = db_find_key(hDB, 0, "/Logger/Data dir", &hKey);
         if (status == DB_SUCCESS) {
            size = sizeof(dir);
            memset(dir, 0, size);
            db_get_value(hDB, 0, "/Logger/Data dir", dir, &size, TID_STRING, TRUE);
            if (dir[0] != 0)
               if (dir[strlen(dir) - 1] != DIR_SEPARATOR)
                  strcat(dir, DIR_SEPARATOR_STR);

            strcpy(path, dir);
            strcat(path, filename2);
         } else {
            cm_get_path(dir);
            if (dir[0] != 0)
               if (dir[strlen(dir) - 1] != DIR_SEPARATOR)
                  strcat(dir, DIR_SEPARATOR_STR);

            strcpy(path, dir);
            strcat(path, filename2);
         }
      } else {
         strcpy(path, filename2);
      }
   } else
      strcpy(path, "midas.log");

   n = cm_msg_retrieve1(path, n_message, message, buf_size);

   while (n < n_message && strchr(filename, '%')) {
      now -= 3600 * 24;         // go one day back 
      tms = localtime(&now);

      strftime(filename2, sizeof(filename2), filename, tms);

      if (strchr(filename2, DIR_SEPARATOR) == NULL) {
         status = db_find_key(hDB, 0, "/Logger/Data dir", &hKey);
         if (status == DB_SUCCESS) {
            size = sizeof(dir);
            memset(dir, 0, size);
            db_get_value(hDB, 0, "/Logger/Data dir", dir, &size, TID_STRING, TRUE);
            if (dir[0] != 0)
               if (dir[strlen(dir) - 1] != DIR_SEPARATOR)
                  strcat(dir, DIR_SEPARATOR_STR);

            strcpy(path, dir);
            strcat(path, filename2);
         } else {
            cm_get_path(dir);
            if (dir[0] != 0)
               if (dir[strlen(dir) - 1] != DIR_SEPARATOR)
                  strcat(dir, DIR_SEPARATOR_STR);

            strcpy(path, dir);
            strcat(path, filename2);
         }
      } else {
         strcpy(path, filename2);
      }

      message2 = (char *) malloc(buf_size);

      i = cm_msg_retrieve1(path, n_message - n, message2, buf_size - strlen(message) - 1);
      if (i < 0)
         break;
      strlcat(message2, "\r\n", buf_size);

      memmove(message + strlen(message2), message, strlen(message) + 1);
      memmove(message, message2, strlen(message2));
      free(message2);
      n += i;
   }

   return status;
}

/**dox***************************************************************/
                                                                                                                               /** @} *//* end of msgfunctionc */

/**dox***************************************************************/
/** @addtogroup cmfunctionc
 *
 *  @{  */

/********************************************************************/
/**
Get time from MIDAS server and set local time.
@param    seconds         Time in seconds
@return CM_SUCCESS
*/
INT cm_synchronize(DWORD * seconds)
{
   INT sec, status;

   /* if connected to server, get time from there */
   if (rpc_is_remote()) {
      status = rpc_call(RPC_CM_SYNCHRONIZE, &sec);

      /* set local time */
      if (status == CM_SUCCESS)
         ss_settime(sec);
   }

   /* return time to caller */
   if (seconds != NULL) {
      *seconds = ss_time();
   }

   return CM_SUCCESS;
}

/********************************************************************/
/**
Get time from MIDAS server and set local time.
@param    str            return time string
@param    buf_size       Maximum size of str
@return   CM_SUCCESS
*/
INT cm_asctime(char *str, INT buf_size)
{
   /* if connected to server, get time from there */
   if (rpc_is_remote())
      return rpc_call(RPC_CM_ASCTIME, str, buf_size);

   /* return local time */
   strcpy(str, ss_asctime());

   return CM_SUCCESS;
}

/********************************************************************/
/**
Get time from ss_time on server.
@param    t string
@return   CM_SUCCESS
*/
INT cm_time(DWORD * t)
{
   /* if connected to server, get time from there */
   if (rpc_is_remote())
      return rpc_call(RPC_CM_TIME, t);

   /* return local time */
   *t = ss_time();

   return CM_SUCCESS;
}

/**dox***************************************************************/
                                                                                                                               /** @} *//* end of cmfunctionc */

/********************************************************************\
*                                                                    *
*           cm_xxx  -  Common Functions to buffer & database         *
*                                                                    *
\********************************************************************/

/* Globals */

static HNDLE _hKeyClient = 0;   /* key handle for client in ODB */
static HNDLE _hDB = 0;          /* Database handle */
static char _experiment_name[NAME_LENGTH];
static char _client_name[NAME_LENGTH];
static char _path_name[MAX_STRING_LENGTH];
static INT _call_watchdog = TRUE;
static INT _watchdog_timeout = DEFAULT_WATCHDOG_TIMEOUT;
INT _semaphore_alarm, _semaphore_elog, _semaphore_history, _semaphore_msg;

/**dox***************************************************************/
/** @addtogroup cmfunctionc
 *
 *  @{  */

/**
Return version number of current MIDAS library as a string
@return version number
*/
char *cm_get_version()
{
   return MIDAS_VERSION;
}

/**
Return svn revision number of current MIDAS library as a string
@return revision number
*/
int cm_get_revision()
{
   return atoi(svn_revision + 6);
}

/********************************************************************/
/**
Set path to actual experiment. This function gets called
by cm_connect_experiment if the connection is established
to a local experiment (not through the TCP/IP server).
The path is then used for all shared memory routines.
@param  path             Pathname
@return CM_SUCCESS
*/
INT cm_set_path(const char *path)
{
   strlcpy(_path_name, path, sizeof(_path_name));

   /* check for trailing directory seperator */
   if (strlen(_path_name) > 0 && _path_name[strlen(_path_name) - 1] != DIR_SEPARATOR)
      strcat(_path_name, DIR_SEPARATOR_STR);

   return CM_SUCCESS;
}

/********************************************************************/
/**
Return the path name previously set with cm_set_path.
@param  path             Pathname
@return CM_SUCCESS
*/
INT cm_get_path(char *path)
{
   strcpy(path, _path_name);

   return CM_SUCCESS;
}

INT cm_get_path1(char *path, int path_size)
{
   strlcpy(path, _path_name, path_size);

   return CM_SUCCESS;
}

/********************************************************************/
/**
Set name of the experiment
@param  name             Experiment name
@return CM_SUCCESS
*/
INT cm_set_experiment_name(const char *name)
{
   strlcpy(_experiment_name, name, sizeof(_experiment_name));
   return CM_SUCCESS;
}

/********************************************************************/
/**
Return the experiment name
@param  name             Pointer to user string, size should be at least NAME_LENGTH
@param  name_size        Size of user string
@return CM_SUCCESS
*/
INT cm_get_experiment_name(char *name, int name_length)
{
   strlcpy(name, _experiment_name, name_length);

   return CM_SUCCESS;
}

/**dox***************************************************************/
                                                                                                                               /** @} *//* end of cmfunctionc */

/**dox***************************************************************/
/** @addtogroup cmfunctionc
 *
 *  @{  */

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

typedef struct {
   char name[NAME_LENGTH];
   char directory[MAX_STRING_LENGTH];
   char user[NAME_LENGTH];
} experiment_table;

static experiment_table exptab[MAX_EXPERIMENT];

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/**
Scan the "exptab" file for MIDAS experiment names and save them
for later use by rpc_server_accept(). The file is first searched
under $MIDAS/exptab if present, then the directory from argv[0] is probed.
@return CM_SUCCESS<br>
        CM_UNDEF_EXP exptab not found and MIDAS_DIR not set
*/
INT cm_scan_experiments(void)
{
   INT i;
   FILE *f;
   char str[MAX_STRING_LENGTH], alt_str[MAX_STRING_LENGTH], *pdir;

   for (i = 0; i < MAX_EXPERIMENT; i++)
      exptab[i].name[0] = 0;

   /* MIDAS_DIR overrides exptab */
   if (getenv("MIDAS_DIR")) {
      strlcpy(str, getenv("MIDAS_DIR"), sizeof(str));

      strcpy(exptab[0].name, "Default");
      strlcpy(exptab[0].directory, getenv("MIDAS_DIR"), sizeof(exptab[0].directory));
      exptab[0].user[0] = 0;

      return CM_SUCCESS;
   }

   /* default directory for different OSes */
#if defined (OS_WINNT)
   if (getenv("SystemRoot"))
      strlcpy(str, getenv("SystemRoot"), sizeof(str));
   else if (getenv("windir"))
      strlcpy(str, getenv("windir"), sizeof(str));
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

   /* MIDAS_EXPTAB overrides default directory */
   if (getenv("MIDAS_EXPTAB")) {
      strlcpy(str, getenv("MIDAS_EXPTAB"), sizeof(str));
      strlcpy(alt_str, getenv("MIDAS_EXPTAB"), sizeof(alt_str));
   }

   /* read list of available experiments */
   f = fopen(str, "r");
   if (f == NULL) {
      f = fopen(alt_str, "r");
      if (f == NULL)
         return CM_UNDEF_ENVIRON;
   }

   i = 0;
   if (f != NULL) {
      do {
         str[0] = 0;
         if (fgets(str, 100, f) == NULL)
            break;
         if (str[0] && str[0] != '#' && str[0] != ' ' && str[0] != '\t'
             && (strchr(str, ' ') || strchr(str, '\t'))) {
            sscanf(str, "%s %s %s", exptab[i].name, exptab[i].directory, exptab[i].user);

            /* check for trailing directory separator */
            pdir = exptab[i].directory;
            if (pdir[strlen(pdir) - 1] != DIR_SEPARATOR)
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

/********************************************************************/
/**
Delete client info from database
@param hDB               Database handle
@param pid               PID of entry to delete, zero for this process.
@return CM_SUCCESS
*/
INT cm_delete_client_info(HNDLE hDB, INT pid)
{
#ifdef LOCAL_ROUTINES

   /* only do it if local */
   if (!rpc_is_remote()) {
      INT status;
      HNDLE hKey;
      char str[256];

      if (!pid)
         pid = ss_getpid();

      /* don't delete info from a closed database */
      if (_database_entries == 0)
         return CM_SUCCESS;

      /* make operation atomic by locking database */
      db_lock_database(hDB);

      sprintf(str, "System/Clients/%0d", pid);
      status = db_find_key1(hDB, 0, str, &hKey);

      if (status == DB_NO_KEY) {
         db_unlock_database(hDB);
         return DB_SUCCESS;
      }

      if (status != DB_SUCCESS) {
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
#endif                          /*LOCAL_ROUTINES */

   return CM_SUCCESS;
}

/********************************************************************/
/**
Check if a client with a /system/client/xxx entry has
a valid entry in the ODB client table. If not, remove
that client from the /system/client tree.
@param   hDB               Handle to online database
@param   hKeyClient        Handle to client key
@return  CM_SUCCESS, CM_NO_CLIENT
*/
INT cm_check_client(HNDLE hDB, HNDLE hKeyClient)
{
   if (rpc_is_remote())
      return rpc_call(RPC_CM_CHECK_CLIENT, hDB, hKeyClient);

#ifdef LOCAL_ROUTINES
   {
      KEY key;
      DATABASE_HEADER *pheader;
      DATABASE_CLIENT *pclient;
      INT i, client_pid, status, dead = 0, found = 0;
      char name[NAME_LENGTH];

      db_lock_database(hDB);

      status = db_get_key(hDB, hKeyClient, &key);
      if (status != DB_SUCCESS)
         return CM_NO_CLIENT;

      client_pid = atoi(key.name);

      name[0] = 0;
      i = sizeof(name);
      status = db_get_value(hDB, hKeyClient, "Name", name, &i, TID_STRING, FALSE);

      //fprintf(stderr, "cm_check_client: hkey %d, status %d, pid %d, name \'%s\', my name %s\n", hKeyClient, status, client_pid, name, _client_name);

      if (status != DB_SUCCESS) {
         db_unlock_database(hDB);
         return CM_NO_CLIENT;
      }

      if (_database[hDB - 1].attached) {
         pheader = _database[hDB - 1].database_header;
         pclient = pheader->client;

         /* loop through clients */
         for (i = 0; i < pheader->max_client_index; i++, pclient++)
            if (pclient->pid == client_pid) {
               found = 1;
               break;
            }

#ifdef OS_UNIX
#ifdef ESRCH
         if (found) {           /* check that the client is still running: PID still exists */
            /* Only enable this for systems that define ESRCH and hope that they also support kill(pid,0) */
            errno = 0;
            kill(client_pid, 0);
            if (errno == ESRCH) {
               dead = 1;
            }
         }
#endif
#endif

         if (!found || dead) {
            /* client not found : delete ODB stucture */

            status = cm_delete_client_info(hDB, client_pid);

            if (status != CM_SUCCESS)
               cm_msg(MERROR, "cm_check_client",
                      "Cannot delete client info for client \'%s\', pid %d, cm_delete_client_info() status %d", name, client_pid, status);
            else if (!found)
               cm_msg(MINFO, "cm_check_client",
                      "Deleted entry \'/System/Clients/%d\' for client \'%s\' because it is not connected to ODB", client_pid, name);
            else if (dead)
               cm_msg(MINFO, "cm_check_client",
                      "Deleted entry \'/System/Clients/%d\' for client \'%s\' because process pid %d does not exists", client_pid, name, client_pid);

            db_unlock_database(hDB);

            return CM_NO_CLIENT;
         }
      }

      db_unlock_database(hDB);
   }
#endif                          /*LOCAL_ROUTINES */

   return CM_SUCCESS;
}

/********************************************************************/
/**
Set client information in online database and return handle
@param  hDB              Handle to online database
@param  hKeyClient       returned key
@param  host_name        server name
@param  client_name      Name of this program as it will be seen
                         by other clients.
@param  hw_type          Type of byte order
@param  password         MIDAS password
@param  watchdog_timeout Default watchdog timeout, can be overwritten
                         by ODB setting /programs/\<name\>/Watchdog timeout
@return   CM_SUCCESS
*/
INT cm_set_client_info(HNDLE hDB, HNDLE * hKeyClient, char *host_name,
                       char *client_name, INT hw_type, char *password, DWORD watchdog_timeout)
{
   if (rpc_is_remote())
      return rpc_call(RPC_CM_SET_CLIENT_INFO, hDB, hKeyClient,
                      host_name, client_name, hw_type, password, watchdog_timeout);

#ifdef LOCAL_ROUTINES
   {
      INT status, pid, data, i, idx, size;
      HNDLE hKey, hSubkey;
      char str[256], name[NAME_LENGTH], orig_name[NAME_LENGTH], pwd[NAME_LENGTH];
      BOOL call_watchdog, allow;
      PROGRAM_INFO_STR(program_info_str);

      /* check security if password is present */
      status = db_find_key(hDB, 0, "/Experiment/Security/Password", &hKey);
      if (hKey) {
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
         if (!allow && strcmp(password, pwd) != 0) {
            if (password[0])
               cm_msg(MINFO, "cm_set_client_info", "Wrong password for host %s", host_name);
            db_close_all_databases();
            bm_close_all_buffers();
            _msg_buffer = 0;
            return CM_WRONG_PASSWORD;
         }
      }

      /* make following operation atomic by locking database */
      db_lock_database(hDB);

      /* check if entry with this pid exists already */
      pid = ss_getpid();

      sprintf(str, "System/Clients/%0d", pid);
      status = db_find_key(hDB, 0, str, &hKey);
      if (status == DB_SUCCESS) {
         db_set_mode(hDB, hKey, MODE_READ | MODE_WRITE | MODE_DELETE, TRUE);
         db_delete_key(hDB, hKey, TRUE);
      }

      if (strlen(client_name) >= NAME_LENGTH)
         client_name[NAME_LENGTH] = 0;

      strcpy(name, client_name);
      strcpy(orig_name, client_name);

      /* check if client name already exists */
      status = db_find_key(hDB, 0, "System/Clients", &hKey);

      for (idx = 1; status != DB_NO_MORE_SUBKEYS; idx++) {
         for (i = 0;; i++) {
            status = db_enum_key(hDB, hKey, i, &hSubkey);
            if (status == DB_NO_MORE_SUBKEYS)
               break;

            if (status == DB_SUCCESS) {
               size = sizeof(str);
               status = db_get_value(hDB, hSubkey, "Name", str, &size, TID_STRING, FALSE);
               if (status != DB_SUCCESS)
                  continue;
            }

            /* check if client is living */
            if (cm_check_client(hDB, hSubkey) == CM_NO_CLIENT)
               continue;

            if (equal_ustring(str, name)) {
               sprintf(name, "%s%d", client_name, idx);
               break;
            }
         }
      }

      /* set name */
      sprintf(str, "System/Clients/%0d/Name", pid);
      status = db_set_value(hDB, 0, str, name, NAME_LENGTH, 1, TID_STRING);
      if (status != DB_SUCCESS) {
         db_unlock_database(hDB);
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
      if (status != DB_SUCCESS) {
         db_unlock_database(hDB);
         return status;
      }

      /* set computer id */
      status = db_set_value(hDB, hKey, "Hardware type", &hw_type, sizeof(hw_type), 1, TID_INT);
      if (status != DB_SUCCESS) {
         db_unlock_database(hDB);
         return status;
      }

      /* set server port */
      data = 0;
      status = db_set_value(hDB, hKey, "Server Port", &data, sizeof(INT), 1, TID_INT);
      if (status != DB_SUCCESS) {
         db_unlock_database(hDB);
         return status;
      }

      /* lock client entry */
      db_set_mode(hDB, hKey, MODE_READ, TRUE);

      /* get (set) default watchdog timeout */
      size = sizeof(watchdog_timeout);
      sprintf(str, "/Programs/%s/Watchdog Timeout", orig_name);
      db_get_value(hDB, 0, str, &watchdog_timeout, &size, TID_INT, TRUE);

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

      /* end of atomic operations */
      db_unlock_database(hDB);

      /* touch notify key to inform others */
      data = 0;
      db_set_value(hDB, 0, "/System/Client Notify", &data, sizeof(data), 1, TID_INT);

      *hKeyClient = hKey;
   }
#endif                          /* LOCAL_ROUTINES */

   return CM_SUCCESS;
}

/********************************************************************/
/**
Get info about the current client
@param  *client_name       Client name.
@return   CM_SUCCESS, CM_UNDEF_EXP
*/
INT cm_get_client_info(char *client_name)
{
   INT status, length;
   HNDLE hDB, hKey;

   /* get root key of client */
   cm_get_experiment_database(&hDB, &hKey);
   if (!hDB) {
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

/********************************************************************/
/**
Returns MIDAS environment variables.
@attention This function can be used to evaluate the standard MIDAS
           environment variables before connecting to an experiment
           (see @ref Environment_variables).
           The usual way is that the host name and experiment name are first derived
           from the environment variables MIDAS_SERVER_HOST and MIDAS_EXPT_NAME.
           They can then be superseded by command line parameters with -h and -e flags.
\code
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
\endcode
@param host_name           Contents of MIDAS_SERVER_HOST environment variable.
@param host_name_size     string length
@param exp_name           Contents of MIDAS_EXPT_NAME environment variable.
@param exp_name_size      string length
@return CM_SUCCESS
*/
INT cm_get_environment(char *host_name, int host_name_size, char *exp_name, int exp_name_size)
{
   host_name[0] = exp_name[0] = 0;

   if (getenv("MIDAS_SERVER_HOST"))
      strlcpy(host_name, getenv("MIDAS_SERVER_HOST"), host_name_size);

   if (getenv("MIDAS_EXPT_NAME"))
      strlcpy(exp_name, getenv("MIDAS_EXPT_NAME"), exp_name_size);

   return CM_SUCCESS;
}


/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************/
void cm_check_connect(void)
{
   if (_hKeyClient) {
      cm_msg(MERROR, "", "cm_disconnect_experiment not called at end of program");
      cm_msg_flush_buffer();
   }
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
This function connects to an existing MIDAS experiment.
This must be the first call in a MIDAS application.
It opens three TCP connection to the remote host (one for RPC calls,
one to send events and one for hot-link notifications from the remote host)
and writes client information into the ODB under /System/Clients.
@attention All MIDAS applications should evaluate the MIDAS_SERVER_HOST
and MIDAS_EXPT_NAME environment variables as defaults to the host name and
experiment name (see @ref Environment_variables).
For that purpose, the function cm_get_environment()
should be called prior to cm_connect_experiment(). If command line
parameters -h and -e are used, the evaluation should be done between
cm_get_environment() and cm_connect_experiment(). The function
cm_disconnect_experiment() must be called before a MIDAS application exits.
\code
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
\endcode
@param host_name Specifies host to connect to. Must be a valid IP host name.
  The string can be empty ("") if to connect to the local computer.
@param exp_name Specifies the experiment to connect to.
  If this string is empty, the number of defined experiments in exptab is checked.
  If only one experiment is defined, the function automatically connects to this
  one. If more than one experiment is defined, a list is presented and the user
  can interactively select one experiment.
@param client_name Client name of the calling program as it can be seen by
  others (like the scl command in ODBEdit).
@param func Callback function to read in a password if security has
  been enabled. In all command line applications this function is NULL which
  invokes an internal ss_gets() function to read in a password.
  In windows environments (MS Windows, X Windows) a function can be supplied to
  open a dialog box and read in the password. The argument of this function must
  be the returned password.
@return CM_SUCCESS, CM_UNDEF_EXP, CM_SET_ERROR, RPC_NET_ERROR <br>
CM_VERSION_MISMATCH MIDAS library version different on local and remote computer
*/
INT cm_connect_experiment(const char *host_name, const char *exp_name, const char *client_name, void (*func) (char *))
{
   INT status;
   char str[256];

   status =
       cm_connect_experiment1(host_name, exp_name, client_name, func, DEFAULT_ODB_SIZE,
                              DEFAULT_WATCHDOG_TIMEOUT);
   cm_msg_flush_buffer();
   if (status != CM_SUCCESS) {
      cm_get_error(status, str);
      puts(str);
   }

   return status;
}

/********************************************************************/
/**
Connect to a MIDAS experiment (to the online database) on
           a specific host.
@internal
*/
INT cm_connect_experiment1(const char *host_name, const char *exp_name,
                           const char *client_name, void (*func) (char *), INT odb_size, DWORD watchdog_timeout)
{
   INT status, i, semaphore_elog, semaphore_alarm, semaphore_history, semaphore_msg, size;
   char local_host_name[HOST_NAME_LENGTH];
   char client_name1[NAME_LENGTH];
   char password[NAME_LENGTH], str[256], exp_name1[NAME_LENGTH];
   HNDLE hDB, hKeyClient;
   BOOL call_watchdog;
   RUNINFO_STR(runinfo_str);

   if (_hKeyClient)
      cm_disconnect_experiment();

   rpc_set_name(client_name);

   /* check for local host */
   if (equal_ustring(host_name, "local"))
      host_name = NULL;

#ifdef OS_WINNT
   {
      WSADATA WSAData;

      /* Start windows sockets */
      if (WSAStartup(MAKEWORD(1, 1), &WSAData) != 0)
         return RPC_NET_ERROR;
   }
#endif

   /* search for experiment name in exptab */
   if (exp_name == NULL)
      exp_name = "";

   strcpy(exp_name1, exp_name);
   if (exp_name1[0] == 0) {
      status = cm_select_experiment(host_name, exp_name1);
      if (status != CM_SUCCESS)
         return status;
   }

   /* connect to MIDAS server */
   if (host_name && host_name[0]) {
      status = rpc_server_connect(host_name, exp_name1);
      if (status != RPC_SUCCESS)
         return status;

      /* register MIDAS library functions */
      status = rpc_register_functions(rpc_get_internal_list(1), NULL);
      if (status != RPC_SUCCESS)
         return status;
   } else {
      /* lookup path for *SHM files and save it */
      status = cm_scan_experiments();
      if (status != CM_SUCCESS)
         return status;

      for (i = 0; i < MAX_EXPERIMENT && exptab[i].name[0]; i++)
         if (equal_ustring(exp_name1, exptab[i].name))
            break;

      /* return if experiment not defined */
      if (i == MAX_EXPERIMENT || exptab[i].name[0] == 0) {
         /* message should be displayed by application
            sprintf(str, "Experiment %s not defined in exptab\r", exp_name1);
            cm_msg(MERROR, str);
          */
         return CM_UNDEF_EXP;
      }

      cm_set_experiment_name(exptab[i].name);
      cm_set_path(exptab[i].directory);

      /* create alarm and elog semaphores */
      status = ss_semaphore_create("ALARM", &semaphore_alarm);
      if (status != SS_CREATED && status != SS_SUCCESS) {
         cm_msg(MERROR, "cm_connect_experiment", "Cannot create alarm semaphore");
         return status;
      }
      status = ss_semaphore_create("ELOG", &semaphore_elog);
      if (status != SS_CREATED && status != SS_SUCCESS) {
         cm_msg(MERROR, "cm_connect_experiment", "Cannot create elog semaphore");
         return status;
      }
      status = ss_semaphore_create("HISTORY", &semaphore_history);
      if (status != SS_CREATED && status != SS_SUCCESS) {
         cm_msg(MERROR, "cm_connect_experiment", "Cannot create history semaphore");
         return status;
      }
      status = ss_semaphore_create("MSG", &semaphore_msg);
      if (status != SS_CREATED && status != SS_SUCCESS) {
         cm_msg(MERROR, "cm_connect_experiment", "Cannot create message semaphore");
         return status;
      }

      cm_set_experiment_semaphore(semaphore_alarm, semaphore_elog, semaphore_history, semaphore_msg);
   }

   /* open ODB */
   if (odb_size == 0)
      odb_size = DEFAULT_ODB_SIZE;

   status = db_open_database("ODB", odb_size, &hDB, client_name);
   if (status != DB_SUCCESS && status != DB_CREATED) {
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
                               client_name1, rpc_get_option(0, RPC_OHW_TYPE), password, watchdog_timeout);

   if (status == CM_WRONG_PASSWORD) {
      if (func == NULL)
         strcpy(str, ss_getpass("Password: "));
      else
         func(str);

      /* re-open database */
      status = db_open_database("ODB", odb_size, &hDB, client_name);
      if (status != DB_SUCCESS && status != DB_CREATED) {
         cm_msg(MERROR, "cm_connect_experiment1", "cannot open database");
         return status;
      }

      strcpy(password, ss_crypt(str, "mi"));
      status = cm_set_client_info(hDB, &hKeyClient, local_host_name,
                                  client_name1, rpc_get_option(0, RPC_OHW_TYPE), password, watchdog_timeout);
      if (status != CM_SUCCESS) {
         /* disconnect */
         if (rpc_is_remote())
            rpc_server_disconnect();

         return status;
      }
   }

   cm_set_experiment_database(hDB, hKeyClient);

   /* set experiment name in ODB */
   db_set_value(hDB, 0, "/Experiment/Name", exp_name1, NAME_LENGTH, 1, TID_STRING);

   /* set data dir in ODB */
   cm_get_path(str);
   size = sizeof(str);
   db_get_value(hDB, 0, "/Logger/Data dir", str, &size, TID_STRING, TRUE);

   /* check /runinfo structure */
   status = db_check_record(hDB, 0, "/Runinfo", strcomb(runinfo_str), TRUE);
   if (status == DB_STRUCT_MISMATCH) {
      cm_msg(MERROR, "cm_connect_experiment1", "Aborting on mismatching /Runinfo structure");
      cm_disconnect_experiment();
      abort();
   }

   /* register server to be able to be called by other clients */
   status = cm_register_server();
   if (status != CM_SUCCESS)
      return status;

   /* set watchdog timeout */
   cm_get_watchdog_params(&call_watchdog, &watchdog_timeout);
   size = sizeof(watchdog_timeout);
   sprintf(str, "/Programs/%s/Watchdog Timeout", client_name);
   db_get_value(hDB, 0, str, &watchdog_timeout, &size, TID_INT, TRUE);
   cm_set_watchdog_params(call_watchdog, watchdog_timeout);

   /* send startup notification */
   if (strchr(local_host_name, '.'))
      *strchr(local_host_name, '.') = 0;

   /* startup message is not displayed */
   _message_print = NULL;

   cm_msg(MINFO, "cm_connect_experiment", "Program %s on host %s started", client_name, local_host_name);

   /* enable system and user messages to stdout as default */
   cm_set_msg_print(MT_ALL, MT_ALL, puts);

   /* call cm_check_connect when exiting */
   atexit((void (*)(void)) cm_check_connect);

   /* register ctrl-c handler */
   ss_ctrlc_handler(cm_ctrlc_handler);

   return CM_SUCCESS;
}

/********************************************************************/
/**
Connect to a MIDAS server and return all defined
           experiments in *exp_name[MAX_EXPERIMENTS]
@param  host_name         Internet host name.
@param  exp_name          list of experiment names
@return CM_SUCCESS, RPC_NET_ERROR
*/
INT cm_list_experiments(const char *host_name, char exp_name[MAX_EXPERIMENT][NAME_LENGTH])
{
   INT i, status;
   struct sockaddr_in bind_addr;
   INT sock;
   char str[MAX_EXPERIMENT * NAME_LENGTH];
   struct hostent *phe;
   int port = MIDAS_TCP_PORT;
   char hname[256];
   char *s;

   if (host_name == NULL || host_name[0] == 0 || equal_ustring(host_name, "local")) {
      status = cm_scan_experiments();
      if (status != CM_SUCCESS)
         return status;

      for (i = 0; i < MAX_EXPERIMENT; i++)
         strcpy(exp_name[i], exptab[i].name);

      return CM_SUCCESS;
   }
#ifdef OS_WINNT
   {
      WSADATA WSAData;

      /* Start windows sockets */
      if (WSAStartup(MAKEWORD(1, 1), &WSAData) != 0)
         return RPC_NET_ERROR;
   }
#endif

   /* create a new socket for connecting to remote server */
   sock = socket(AF_INET, SOCK_STREAM, 0);
   if (sock == -1) {
      cm_msg(MERROR, "cm_list_experiments", "cannot create socket");
      return RPC_NET_ERROR;
   }

   /* extract port number from host_name */
   strlcpy(hname, host_name, sizeof(hname));
   s = strchr(hname, ':');
   if (s) {
      *s = 0;
      port = strtoul(s + 1, NULL, 0);
   }

   /* connect to remote node */
   memset(&bind_addr, 0, sizeof(bind_addr));
   bind_addr.sin_family = AF_INET;
   bind_addr.sin_addr.s_addr = 0;
   bind_addr.sin_port = htons(port);

#ifdef OS_VXWORKS
   {
      INT host_addr;

      host_addr = hostGetByName(hname);
      memcpy((char *) &(bind_addr.sin_addr), &host_addr, 4);
   }
#else
   phe = gethostbyname(hname);
   if (phe == NULL) {
      cm_msg(MERROR, "cm_list_experiments", "cannot resolve host name \'%s\'", hname);
      return RPC_NET_ERROR;
   }
   memcpy((char *) &(bind_addr.sin_addr), phe->h_addr, phe->h_length);
#endif

#ifdef OS_UNIX
   do {
      status = connect(sock, (void *) &bind_addr, sizeof(bind_addr));

      /* don't return if an alarm signal was cought */
   } while (status == -1 && errno == EINTR);
#else
   status = connect(sock, (struct sockaddr *) &bind_addr, sizeof(bind_addr));
#endif

   if (status != 0) {
/*    cm_msg(MERROR, "cannot connect"); message should be displayed by application */
      return RPC_NET_ERROR;
   }

   /* request experiment list */
   send(sock, "I", 2, 0);

   for (i = 0; i < MAX_EXPERIMENT; i++) {
      exp_name[i][0] = 0;
      status = recv_string(sock, str, sizeof(str), _rpc_connect_timeout);

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

/********************************************************************/
/**
Connect to a MIDAS server and select an experiment
           from the experiments available on this server
@internal
@param  host_name         Internet host name.
@param  exp_name          list of experiment names
@return CM_SUCCESS, RPC_NET_ERROR
*/
INT cm_select_experiment(const char *host_name, char *exp_name)
{
   INT status, i;
   char expts[MAX_EXPERIMENT][NAME_LENGTH];
   char str[32];

   /* retrieve list of experiments and make selection */
   status = cm_list_experiments(host_name, expts);
   if (status != CM_SUCCESS)
      return status;

   if (expts[1][0]) {
      if (host_name[0])
         printf("Available experiments on server %s:\n", host_name);
      else
         printf("Available experiments on local computer:\n");

      for (i = 0; expts[i][0]; i++)
         printf("%d : %s\n", i, expts[i]);
      printf("Select number: ");
      ss_gets(str, 32);
      i = atoi(str);
      strcpy(exp_name, expts[i]);
   } else
      strcpy(exp_name, expts[0]);

   return CM_SUCCESS;
}

/********************************************************************/
/**
Connect to a MIDAS client of the current experiment
@internal
@param  client_name       Name of client to connect to. This name
                            is set by the other client via the
                            cm_connect_experiment call.
@param  hConn            Connection handle
@return CM_SUCCESS, CM_NO_CLIENT
*/
INT cm_connect_client(char *client_name, HNDLE * hConn)
{
   HNDLE hDB, hKeyRoot, hSubkey, hKey;
   INT status, i, length, port;
   char name[NAME_LENGTH], host_name[HOST_NAME_LENGTH];

   /* find client entry in ODB */
   cm_get_experiment_database(&hDB, &hKey);

   status = db_find_key(hDB, 0, "System/Clients", &hKeyRoot);
   if (status != DB_SUCCESS)
      return status;

   i = 0;
   do {
      /* search for client with specific name */
      status = db_enum_key(hDB, hKeyRoot, i++, &hSubkey);
      if (status == DB_NO_MORE_SUBKEYS)
         return CM_NO_CLIENT;

      status = db_find_key(hDB, hSubkey, "Name", &hKey);
      if (status != DB_SUCCESS)
         return status;

      length = NAME_LENGTH;
      status = db_get_data(hDB, hKey, name, &length, TID_STRING);
      if (status != DB_SUCCESS)
         return status;

      if (equal_ustring(name, client_name)) {
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
         return rpc_client_connect(host_name, port, client_name, hConn);
      }


   } while (TRUE);
}

/********************************************************************/
/**
Disconnect from a MIDAS client
@param   hConn             Connection handle obtained via
                             cm_connect_client()
@param   bShutdown         If TRUE, disconnect from client and
                             shut it down (exit the client program)
                             by sending a RPC_SHUTDOWN message
@return   see rpc_client_disconnect()
*/
INT cm_disconnect_client(HNDLE hConn, BOOL bShutdown)
{
   return rpc_client_disconnect(hConn, bShutdown);
}

/********************************************************************/
/**
Disconnect from a MIDAS experiment.
@attention Should be the last call to a MIDAS library function in an
application before it exits. This function removes the client information
from the ODB, disconnects all TCP connections and frees all internal
allocated memory. See cm_connect_experiment() for example.
@return CM_SUCCESS
*/
INT cm_disconnect_experiment(void)
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

   cm_msg(MINFO, "cm_disconnect_experiment", "Program %s on host %s stopped", client_name, local_host_name);

   cm_msg_flush_buffer();

   if (rpc_is_remote()) {
      /* close open records */
      db_close_all_records();

      rpc_client_disconnect(-1, FALSE);
      rpc_server_disconnect();
   } else {
      rpc_client_disconnect(-1, FALSE);

#ifdef LOCAL_ROUTINES
      ss_alarm(0, cm_watchdog);
      _watchdog_last_called = 0;
#endif                          /* LOCAL_ROUTINES */

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

   if (_msg_mutex)
      ss_mutex_delete(_msg_mutex);
   _msg_mutex = 0;
   if (_msg_rb)
      rb_delete(_msg_rb);
   _msg_rb = 0;

   _msg_buffer = 0;

   /* free memory buffers */
   if (_event_buffer_size > 0) {
      M_FREE(_event_buffer);
      _event_buffer = NULL;
      _event_buffer_size = 0;
   }

   if (_net_recv_buffer_size > 0) {
      M_FREE(_net_recv_buffer);
      _net_recv_buffer = NULL;
      _net_recv_buffer_size = 0;
      _net_recv_buffer_size_odb = 0;
   }

   if (_net_send_buffer_size > 0) {
      M_FREE(_net_send_buffer);
      _net_send_buffer = NULL;
      _net_send_buffer_size = 0;
   }

   if (_tcp_buffer != NULL) {
      M_FREE(_tcp_buffer);
      _tcp_buffer = NULL;
   }

   return CM_SUCCESS;
}

/********************************************************************/
/**
Set the handle to the ODB for the currently connected experiment
@param hDB              Database handle
@param hKeyClient       Key handle of client structure
@return CM_SUCCESS
*/
INT cm_set_experiment_database(HNDLE hDB, HNDLE hKeyClient)
{
   _hDB = hDB;
   _hKeyClient = hKeyClient;

   return CM_SUCCESS;
}



/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************/
INT cm_set_experiment_semaphore(INT semaphore_alarm, INT semaphore_elog, INT semaphore_history, INT semaphore_msg)
/********************************************************************\

  Routine: cm_set_experiment_semaphore

  Purpose: Set the handle to the experiment wide semaphorees

  Input:
    INT    semaphore_alarm      Alarm semaphore
    INT    semaphore_elog       Elog semaphore
    INT    semaphore_history    History semaphore
    INT    semaphore_msg        Message semaphore

  Output:
    none

  Function value:
    CM_SUCCESS              Successful completion

\********************************************************************/
{
   _semaphore_alarm = semaphore_alarm;
   _semaphore_elog = semaphore_elog;
   _semaphore_history = semaphore_history;
   _semaphore_msg = semaphore_msg;

   return CM_SUCCESS;
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Get the handle to the ODB from the currently connected experiment.

@attention This function returns the handle of the online database (ODB) which
can be used in future db_xxx() calls. The hkeyclient key handle can be used
to access the client information in the ODB. If the client key handle is not needed,
the parameter can be NULL.
\code
HNDLE hDB, hkeyclient;
 char  name[32];
 int   size;
 db_get_experiment_database(&hdb, &hkeyclient);
 size = sizeof(name);
 db_get_value(hdb, hkeyclient, "Name", name, &size, TID_STRING, TRUE);
 printf("My name is %s\n", name);
\endcode
@param hDB Database handle.
@param hKeyClient Handle for key where search starts, zero for root.
@return CM_SUCCESS
*/
INT cm_get_experiment_database(HNDLE * hDB, HNDLE * hKeyClient)
{
   if (_hDB) {
      if (hDB != NULL)
         *hDB = _hDB;
      if (hKeyClient != NULL)
         *hKeyClient = _hKeyClient;
   } else {
      if (hDB != NULL)
         *hDB = rpc_get_server_option(RPC_ODB_HANDLE);
      if (hKeyClient != NULL)
         *hKeyClient = rpc_get_server_option(RPC_CLIENT_HANDLE);
   }

   return CM_SUCCESS;
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************/
INT cm_get_experiment_semaphore(INT * semaphore_alarm, INT * semaphore_elog, INT * semaphore_history, INT * semaphore_msg)
/********************************************************************\

  Routine: cm_get_experiment_semaphore

  Purpose: Get the handle to the experiment wide semaphores

  Input:
    none

  Output:
    INT    semaphore_alarm      Alarm semaphore
    INT    semaphore_elog       Elog semaphore
    INT    semaphore_history    History semaphore
    INT    semaphore_msg        Message semaphore

  Function value:
    CM_SUCCESS              Successful completion

\********************************************************************/
{
   if (semaphore_alarm)
      *semaphore_alarm = _semaphore_alarm;
   if (semaphore_elog)
      *semaphore_elog = _semaphore_elog;
   if (semaphore_history)
      *semaphore_history = _semaphore_history;
   if (semaphore_msg)
      *semaphore_msg = _semaphore_msg;

   return CM_SUCCESS;
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

static int bm_validate_client_index(const BUFFER * buf, BOOL abort_if_invalid)
{
   static int prevent_recursion = 1;
   int badindex = 0;
   BUFFER_CLIENT *bcl = buf->buffer_header->client;

   if (buf->client_index < 0)
      badindex = 1;
   else if (buf->client_index > buf->buffer_header->max_client_index)
      badindex = 1;
   else {
      bcl = &(buf->buffer_header->client[buf->client_index]);
      if (bcl->name[0] == 0)
         badindex = 1;
      else if (bcl->pid != ss_getpid())
         badindex = 1;
   }

#if 0
   printf
       ("bm_validate_client_index: badindex=%d, buf=%p, client_index=%d, max_client_index=%d, client_name=\'%s\', client_pid=%d, pid=%d\n",
        badindex, buf, buf->client_index, buf->buffer_header->max_client_index,
        buf->buffer_header->client[buf->client_index].name, buf->buffer_header->client[buf->client_index].pid,
        ss_getpid());
#endif

   if (badindex) {

      if (!abort_if_invalid)
         return -1;

      if (prevent_recursion) {
         prevent_recursion = 0;
         cm_msg(MERROR, "bm_validate_client_index",
                "My client index %d in buffer \'%s\' is invalid: client name \'%s\', pid %d should be my pid %d",
                buf->client_index, buf->buffer_header->name, bcl->name, bcl->pid, ss_getpid());
         cm_msg(MERROR, "bm_validate_client_index",
                "Maybe this client was removed by a timeout. Cannot continue, aborting...");
      }

      abort();
      exit(1);
   }

   return buf->client_index;
}

/********************************************************************/
/**
Sets the internal watchdog flags and the own timeout.
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
@param    call_watchdog   Call the cm_watchdog routine periodically
@param    timeout         Timeout for this application in ms
@return   CM_SUCCESS
*/
INT cm_set_watchdog_params(BOOL call_watchdog, DWORD timeout)
{
   INT i;

   /* set also local timeout to requested value (needed by cm_enable_watchdog()) */
   _watchdog_timeout = timeout;

   if (rpc_is_remote())
      return rpc_call(RPC_CM_SET_WATCHDOG_PARAMS, call_watchdog, timeout);

#ifdef LOCAL_ROUTINES

   if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_REMOTE) {
      HNDLE hDB, hKey;

      rpc_set_server_option(RPC_WATCHDOG_TIMEOUT, timeout);

      /* write timeout value to client enty in ODB */
      cm_get_experiment_database(&hDB, &hKey);

      if (hDB) {
         db_set_mode(hDB, hKey, MODE_READ | MODE_WRITE, TRUE);
         db_set_value(hDB, hKey, "Link timeout", &timeout, sizeof(timeout), 1, TID_INT);
         db_set_mode(hDB, hKey, MODE_READ, TRUE);
      }
   } else {
      _call_watchdog = call_watchdog;
      _watchdog_timeout = timeout;

      /* set watchdog flag of all open buffers */
      for (i = _buffer_entries; i > 0; i--) {
         BUFFER_CLIENT *pclient;
         BUFFER_HEADER *pheader;
         INT idx;
	 
         if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_SINGLE
             && _buffer[i - 1].index != rpc_get_server_acception())
            continue;

         if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_SINGLE && _buffer[i - 1].index != ss_gettid())
            continue;

         if (!_buffer[i - 1].attached)
            continue;

         idx = bm_validate_client_index(&_buffer[i - 1], TRUE);
         pheader = _buffer[i - 1].buffer_header;
         pclient = &pheader->client[idx];

         /* clear entry from client structure in buffer header */
         pclient->watchdog_timeout = timeout;

         /* show activity */
         pclient->last_activity = ss_millitime();
      }

      /* set watchdog flag of all open databases */
      for (i = _database_entries; i > 0; i--) {
         DATABASE_HEADER *pheader;
         DATABASE_CLIENT *pclient;
         INT idx;

         db_lock_database(i);

         idx = _database[i - 1].client_index;
         pheader = _database[i - 1].database_header;
         pclient = &pheader->client[idx];

         if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_SINGLE &&
             _database[i - 1].index != rpc_get_server_acception()) {
            db_unlock_database(i);
            continue;
         }

         if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_SINGLE && _database[i - 1].index != ss_gettid()) {
            db_unlock_database(i);
            continue;
         }

         if (!_database[i - 1].attached) {
            db_unlock_database(i);
            continue;
         }

         /* clear entry from client structure in buffer header */
         pclient->watchdog_timeout = timeout;

         /* show activity */
         pclient->last_activity = ss_millitime();

         db_unlock_database(i);
      }

      if (call_watchdog)
         /* restart watchdog */
         ss_alarm(WATCHDOG_INTERVAL, cm_watchdog);
      else
         /* kill current timer */
         ss_alarm(0, cm_watchdog);
   }

#endif                          /* LOCAL_ROUTINES */

   return CM_SUCCESS;
}

/********************************************************************/
/**
Return the current watchdog parameters
@param call_watchdog   Call the cm_watchdog routine periodically
@param timeout         Timeout for this application in seconds
@return   CM_SUCCESS
*/
INT cm_get_watchdog_params(BOOL * call_watchdog, DWORD * timeout)
{
   if (call_watchdog)
      *call_watchdog = _call_watchdog;
   if (timeout)
      *timeout = _watchdog_timeout;

   return CM_SUCCESS;
}

/********************************************************************/
/**
Return watchdog information about specific client
@param    hDB              ODB handle
@param    client_name     ODB client name
@param    timeout         Timeout for this application in seconds
@param    last            Last time watchdog was called in msec
@return   CM_SUCCESS, CM_NO_CLIENT, DB_INVALID_HANDLE
*/

INT cm_get_watchdog_info(HNDLE hDB, char *client_name, DWORD * timeout, DWORD * last)
{
   if (rpc_is_remote())
      return rpc_call(RPC_CM_GET_WATCHDOG_INFO, hDB, client_name, timeout, last);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      DATABASE_CLIENT *pclient;
      INT i;

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "cm_get_watchdog_info", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "cm_get_watchdog_info", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      /* lock database */
      db_lock_database(hDB);

      pheader = _database[hDB - 1].database_header;
      pclient = pheader->client;

      /* find client */
      for (i = 0; i < pheader->max_client_index; i++, pclient++)
         if (pclient->pid && equal_ustring(pclient->name, client_name)) {
            *timeout = pclient->watchdog_timeout;
            *last = ss_millitime() - pclient->last_activity;
            db_unlock_database(hDB);
            return CM_SUCCESS;
         }

      *timeout = *last = 0;

      db_unlock_database(hDB);

      return CM_NO_CLIENT;
   }
#else                           /* LOCAL_ROUTINES */
   return CM_SUCCESS;
#endif                          /* LOCAL_ROUTINES */
}


/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************/
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
   INT status, port;
   HNDLE hDB, hKey;

   if (!_server_registered) {
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

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Registers a callback function for run transitions.
This function internally registers the transition callback
function and publishes its request for transition notification by writing
a transition request to /System/Clients/\<pid\>/Transition XXX.
Other clients making a transition scan the transition requests of all clients
and call their transition callbacks via RPC.

Clients can register for transitions (Start/Stop/Pause/Resume) in a given
sequence. All sequence numbers given in the registration are sorted on
a transition and the clients are contacted in ascending order. By default,
all programs register with a sequence number of 500. The logger however
uses 200 for start, so that it can open files before the other clients
are contacted, and 800 for stop, so that the files get closed when all
other clients have gone already through the stop trantition.

The callback function returns CM_SUCCESS if it can perform the transition or
a value larger than one in case of error. An error string can be copied
into the error variable.
@attention The callback function will be called on transitions from inside the
    cm_yield() function which therefore must be contained in the main program loop.
\code
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
  cm_register_transition(TR_START, start, 500);
  do
    {
    status = cm_yield(1000);
    } while (status != RPC_SHUTDOWN &&
             status != SS_ABORT);
  ...
}
\endcode
@param transition Transition to register for (see @ref state_transition)
@param func Callback function.
@param sequence_number Sequence number for that transition (1..1000)
@return CM_SUCCESS
*/
INT cm_register_transition(INT transition, INT(*func) (INT, char *), INT sequence_number)
{
   INT status, i;
   HNDLE hDB, hKey, hKeyTrans;
   KEY key;
   char str[256];

   /* check for valid transition */
   if (transition != TR_START && transition != TR_STOP && transition != TR_PAUSE && transition != TR_RESUME
       && transition != TR_STARTABORT) {
      cm_msg(MERROR, "cm_register_transition", "Invalid transition request \"%d\"", transition);
      return CM_INVALID_TRANSITION;
   }

   cm_get_experiment_database(&hDB, &hKey);

   rpc_register_function(RPC_RC_TRANSITION, rpc_transition_dispatch);

   /* register new transition request */

   /* find empty slot */
   for (i = 0; i < MAX_TRANSITIONS; i++)
      if (!_trans_table[i].transition)
         break;

   if (i == MAX_TRANSITIONS) {
      cm_msg(MERROR, "cm_register_transition",
             "To many transition registrations. Please increase MAX_TRANSITIONS and recompile");
      return CM_TOO_MANY_REQUESTS;
   }

   _trans_table[i].transition = transition;
   _trans_table[i].func = func;
   _trans_table[i].sequence_number = sequence_number;

   for (i = 0;; i++)
      if (trans_name[i].name[0] == 0 || trans_name[i].transition == transition)
         break;

   sprintf(str, "Transition %s", trans_name[i].name);

   /* unlock database */
   db_set_mode(hDB, hKey, MODE_READ | MODE_WRITE | MODE_DELETE, TRUE);

   /* set value */
   status = db_find_key(hDB, hKey, str, &hKeyTrans);
   if (!hKeyTrans) {
      status = db_set_value(hDB, hKey, str, &sequence_number, sizeof(INT), 1, TID_INT);
      if (status != DB_SUCCESS)
         return status;
   } else {
      status = db_get_key(hDB, hKeyTrans, &key);
      if (status != DB_SUCCESS)
         return status;
      status = db_set_data_index(hDB, hKeyTrans, &sequence_number, sizeof(INT), key.num_values, TID_INT);
      if (status != DB_SUCCESS)
         return status;
   }

   /* re-lock database */
   db_set_mode(hDB, hKey, MODE_READ, TRUE);

   return CM_SUCCESS;
}

INT cm_deregister_transition(INT transition)
{
   INT status, i;
   HNDLE hDB, hKey, hKeyTrans;
   char str[256];

   /* check for valid transition */
   if (transition != TR_START && transition != TR_STOP && transition != TR_PAUSE && transition != TR_RESUME) {
      cm_msg(MERROR, "cm_deregister_transition", "Invalid transition request \"%d\"", transition);
      return CM_INVALID_TRANSITION;
   }

   cm_get_experiment_database(&hDB, &hKey);

   /* remove existing transition request */
   for (i = 0; i < MAX_TRANSITIONS; i++)
      if (_trans_table[i].transition == transition)
         break;

   if (i == MAX_TRANSITIONS) {
      cm_msg(MERROR, "cm_register_transition",
             "Cannot de-register transition registration, request not found");
      return CM_INVALID_TRANSITION;
   }

   _trans_table[i].transition = 0;
   _trans_table[i].func = NULL;
   _trans_table[i].sequence_number = 0;

   for (i = 0;; i++)
      if (trans_name[i].name[0] == 0 || trans_name[i].transition == transition)
         break;

   sprintf(str, "Transition %s", trans_name[i].name);

   /* unlock database */
   db_set_mode(hDB, hKey, MODE_READ | MODE_WRITE | MODE_DELETE, TRUE);

   /* set value */
   status = db_find_key(hDB, hKey, str, &hKeyTrans);
   if (hKeyTrans) {
      status = db_delete_key(hDB, hKeyTrans, FALSE);
      if (status != DB_SUCCESS)
         return status;
   }

   /* re-lock database */
   db_set_mode(hDB, hKey, MODE_READ, TRUE);

   return CM_SUCCESS;
}

/********************************************************************/
/**
Change the transition sequence for the calling program.
@param transition TR_START, TR_PAUSE, TR_RESUME or TR_STOP.
@param sequence_number New sequence number, should be between 1 and 1000
@return     CM_SUCCESS
*/
INT cm_set_transition_sequence(INT transition, INT sequence_number)
{
   INT status, i;
   HNDLE hDB, hKey;
   char str[256];

   /* check for valid transition */
   if (transition != TR_START && transition != TR_STOP && transition != TR_PAUSE && transition != TR_RESUME) {
      cm_msg(MERROR, "cm_set_transition_sequence", "Invalid transition request \"%d\"", transition);
      return CM_INVALID_TRANSITION;
   }

   cm_get_experiment_database(&hDB, &hKey);

   /* Find the transition type from the list */
   for (i = 0;; i++)
      if (trans_name[i].name[0] == 0 || trans_name[i].transition == transition)
         break;
   sprintf(str, "Transition %s", trans_name[i].name);

   /* Change local sequence number for this transition type */
   for (i = 0; i < MAX_TRANSITIONS; i++)
      if (_trans_table[i].transition == transition) {
         _trans_table[i].sequence_number = sequence_number;
         break;
      }

   /* unlock database */
   db_set_mode(hDB, hKey, MODE_READ | MODE_WRITE, TRUE);

   /* set value */
   status = db_set_value(hDB, hKey, str, &sequence_number, sizeof(INT), 1, TID_INT);
   if (status != DB_SUCCESS)
      return status;

   /* re-lock database */
   db_set_mode(hDB, hKey, MODE_READ, TRUE);

   return CM_SUCCESS;

}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

static INT _requested_transition;
static DWORD _deferred_transition_mask;

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Register a deferred transition handler. If a client is
registered as a deferred transition handler, it may defer
a requested transition by returning FALSE until a certain
condition (like a motor reaches its end position) is
reached.
@param transition      One of TR_xxx
@param (*func)         Function which gets called whenever
                       a transition is requested. If it returns
                       FALSE, the transition is not performed.
@return CM_SUCCESS,    \<error\> Error from ODB access
*/
INT cm_register_deferred_transition(INT transition, BOOL(*func) (INT, BOOL))
{
   INT status, i, size;
   char tr_key_name[256];
   HNDLE hDB, hKey;

   cm_get_experiment_database(&hDB, &hKey);

   for (i = 0; _deferred_trans_table[i].transition; i++)
      if (_deferred_trans_table[i].transition == transition)
         _deferred_trans_table[i].func = (int (*)(int, char *)) func;

   /* set new transition mask */
   _deferred_transition_mask |= transition;

   for (i = 0;; i++)
      if (trans_name[i].name[0] == 0 || trans_name[i].transition == transition)
         break;

   sprintf(tr_key_name, "Transition %s DEFERRED", trans_name[i].name);

   /* unlock database */
   db_set_mode(hDB, hKey, MODE_READ | MODE_WRITE, TRUE);

   /* set value */
   i = 0;
   status = db_set_value(hDB, hKey, tr_key_name, &i, sizeof(INT), 1, TID_INT);
   if (status != DB_SUCCESS)
      return status;

   /* re-lock database */
   db_set_mode(hDB, hKey, MODE_READ, TRUE);

   /* hot link requested transition */
   size = sizeof(_requested_transition);
   db_get_value(hDB, 0, "/Runinfo/Requested Transition", &_requested_transition, &size, TID_INT, TRUE);
   db_find_key(hDB, 0, "/Runinfo/Requested Transition", &hKey);
   status = db_open_record(hDB, hKey, &_requested_transition, sizeof(INT), MODE_READ, NULL, NULL);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "cm_register_deferred_transition", "Cannot hotlink /Runinfo/Requested Transition");
      return status;
   }

   return CM_SUCCESS;
}

/********************************************************************/
/**
Check for any deferred transition. If a deferred transition
handler has been registered via the
cm_register_deferred_transition function, this routine
should be called regularly. It checks if a transition
request is pending. If so, it calld the registered handler
if the transition should be done and then actually does
the transition.
@return     CM_SUCCESS, \<error\>  Error from cm_transition()
*/
INT cm_check_deferred_transition()
{
   INT i, status;
   char str[256];
   static BOOL first;

   if (_requested_transition == 0)
      first = TRUE;

   if (_requested_transition & _deferred_transition_mask) {
      for (i = 0; _deferred_trans_table[i].transition; i++)
         if (_deferred_trans_table[i].transition == _requested_transition)
            break;

      if (_deferred_trans_table[i].transition == _requested_transition) {
         if (((BOOL(*)(INT, BOOL)) _deferred_trans_table[i].func) (_requested_transition, first)) {
            status = cm_transition(_requested_transition | TR_DEFERRED, 0, str, sizeof(str), SYNC, FALSE);
            if (status != CM_SUCCESS)
               cm_msg(MERROR, "cm_check_deferred_transition", "Cannot perform deferred transition: %s", str);

            /* bypass hotlink and set _requested_transition directly to zero */
            _requested_transition = 0;

            return status;
         }
         first = FALSE;
      }
   }

   return SUCCESS;
}


/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************/

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

typedef struct {
   int sequence_number;
   char host_name[HOST_NAME_LENGTH];
   char client_name[NAME_LENGTH];
   int port;
} TR_CLIENT;

int tr_compare(const void *arg1, const void *arg2)
{
   return ((TR_CLIENT *) arg1)->sequence_number - ((TR_CLIENT *) arg2)->sequence_number;
}

/********************************************************************/
/**
Performs a run transition (Start/Stop/Pause/Resume).

Synchronous/Asynchronous flag.
If set to ASYNC, the transition is done
asynchronously, meaning that clients are connected and told to execute their
callback routine, but no result is awaited. The return value is
specified by the transition callback function on the remote clients. If all callbacks
can perform the transition, CM_SUCCESS is returned. If one callback cannot
perform the transition, the return value of this callback is returned from
cm_transition().
The async_flag is usually FALSE so that transition callbacks can block a
run transition in case of problems and return an error string. The only exception are
situations where a run transition is performed automatically by a program which
cannot block in a transition. For example the logger can cause a run stop when a
disk is nearly full but it cannot block in the cm_transition() function since it
has its own run stop callback which must flush buffers and close disk files and
tapes.
\code
...
    i = 1;
    db_set_value(hDB, 0, "/Runinfo/Transition in progress", &i, sizeof(INT), 1, TID_INT);

      status = cm_transition(TR_START, new_run_number, str, sizeof(str), SYNC, debug_flag);
      if (status != CM_SUCCESS)
      {
        // in case of error
        printf("Error: %s\n", str);
      }
    ...
\endcode
@param transition TR_START, TR_PAUSE, TR_RESUME or TR_STOP.
@param run_number New run number. If zero, use current run number plus one.
@param errstr returned error string.
@param errstr_size Size of error string.
@param async_flag SYNC: synchronization flag (SYNC:wait completion, ASYNC: retun immediately)
@param debug_flag If 1 output debugging information, if 2 output via cm_msg().
@return CM_SUCCESS, \<error\> error code from remote client
*/
INT cm_transition1(INT transition, INT run_number, char *errstr, INT errstr_size, INT async_flag,
                   INT debug_flag)
{
   INT i, j, status, idx, size, sequence_number, port, state, old_timeout, n_tr_clients;
   HNDLE hDB, hRootKey, hSubkey, hKey, hKeylocal, hConn, hKeyTrans;
   DWORD seconds;
   char host_name[HOST_NAME_LENGTH], client_name[NAME_LENGTH], str[256], error[256], tr_key_name[256];
   char *trname = "unknown";
   KEY key;
   BOOL deferred;
   PROGRAM_INFO program_info;
   TR_CLIENT *tr_client;
   int connect_timeout = 10000;
   int timeout = 120000;
   int t0, t1;

   deferred = (transition & TR_DEFERRED) > 0;
   transition &= ~TR_DEFERRED;

   /* check for valid transition */
   if (transition != TR_START && transition != TR_STOP && transition != TR_PAUSE && transition != TR_RESUME
       && transition != TR_STARTABORT) {
      cm_msg(MERROR, "cm_transition", "Invalid transition request \"%d\"", transition);
      if (errstr != NULL)
         strlcpy(errstr, "Invalid transition request", errstr_size);
      return CM_INVALID_TRANSITION;
   }

   /* get key of local client */
   cm_get_experiment_database(&hDB, &hKeylocal);

   if (errstr != NULL)
      strlcpy(errstr, "Unknown error", errstr_size);

   if (debug_flag == 0) {
      size = sizeof(i);
      db_get_value(hDB, 0, "/Experiment/Transition debug flag", &debug_flag, &size, TID_INT, TRUE);
   }

   /* do detached transition via mtransition tool */
   if (async_flag == DETACH) {
      char *args[100], path[256];
      int iarg = 0;
      char debug_arg[256];
      char start_arg[256];
      char expt_name[256];
      extern RPC_SERVER_CONNECTION _server_connection;

      path[0] = 0;
      if (getenv("MIDASSYS")) {
         strlcpy(path, getenv("MIDASSYS"), sizeof(path));
         strlcat(path, DIR_SEPARATOR_STR, sizeof(path));
#ifdef OS_LINUX
#ifdef OS_DARWIN
         strlcat(path, "darwin/bin/", sizeof(path));
#else
         strlcat(path, "linux/bin/", sizeof(path));
#endif
#else
#ifdef OS_WINNT
         strlcat(path, "nt/bin/", sizeof(path));
#endif
#endif         
      }
      strlcat(path, "mtransition", sizeof(path));
              
      args[iarg++] = path;

      if (_server_connection.send_sock) {
         /* if connected to mserver, pass connection info to mtransition */
         args[iarg++] = "-h";
         args[iarg++] = _server_connection.host_name;
         args[iarg++] = "-e";
         args[iarg++] = _server_connection.exp_name;
      } else {
         /* get experiment name from ODB */
         size = sizeof(expt_name);
         db_get_value(hDB, 0, "/Experiment/Name", expt_name, &size, TID_STRING, FALSE);

         args[iarg++] = "-e";
         args[iarg++] = expt_name;
      }

      if (debug_flag) {
         args[iarg++] = "-d";

         sprintf(debug_arg, "%d", debug_flag);
         args[iarg++] = debug_arg;
      }

      if (transition == TR_STOP)
         args[iarg++] = "STOP";
      else if (transition == TR_PAUSE)
         args[iarg++] = "PAUSE";
      else if (transition == TR_RESUME)
         args[iarg++] = "RESUME";
      else if (transition == TR_START) {
         args[iarg++] = "START";

         sprintf(start_arg, "%d", run_number);
         args[iarg++] = start_arg;
      }

      args[iarg++] = NULL;

      if (0)
         for (iarg = 0; args[iarg] != NULL; iarg++)
            printf("arg[%d] [%s]\n", iarg, args[iarg]);

      status = ss_spawnv(P_DETACH, args[0], args);

      if (status != SUCCESS) {
         if (errstr != NULL)
            sprintf(errstr, "Cannot execute mtransition, ss_spawnvp() returned %d", status);
         return CM_SET_ERROR;
      }

      return CM_SUCCESS;
   }

   /* if no run number is given, get it from DB */
   if (run_number == 0) {
      size = sizeof(run_number);
      status = db_get_value(hDB, 0, "Runinfo/Run number", &run_number, &size, TID_INT, TRUE);
      assert(status == SUCCESS);
   }

   if (run_number <= 0) {
      cm_msg(MERROR, "cm_transition", "aborting on attempt to use invalid run number %d", run_number);
      abort();
   }

   /* check if transition in progress */
   if (!deferred) {
      i = 0;
      size = sizeof(i);
      db_get_value(hDB, 0, "/Runinfo/Transition in progress", &i, &size, TID_INT, TRUE);
      if (i == 1) {
         sprintf(errstr, "Start/Stop transition %d already in progress, please try again later\n", i);
         strlcat(errstr, "or set \"/Runinfo/Transition in progress\" manually to zero.\n", errstr_size);
         return CM_TRANSITION_IN_PROGRESS;
      }
   }

   /* get transition timeout for rpc connect */
   size = sizeof(timeout);
   db_get_value(hDB, 0, "/Experiment/Transition connect timeout", &connect_timeout, &size, TID_INT, TRUE);

   if (connect_timeout < 1000)
      connect_timeout = 1000;

   /* get transition timeout */
   size = sizeof(timeout);
   db_get_value(hDB, 0, "/Experiment/Transition timeout", &timeout, &size, TID_INT, TRUE);

   if (timeout < 1000)
      timeout = 1000;

   /* indicate transition in progress */
   i = transition;
   db_set_value(hDB, 0, "/Runinfo/Transition in progress", &i, sizeof(INT), 1, TID_INT);

   /* clear run abort flag */
   i = 0;
   db_set_value(hDB, 0, "/Runinfo/Start abort", &i, sizeof(INT), 1, TID_INT);

   /* Set new run number in ODB */
   if (transition == TR_START) {
      if (debug_flag == 1)
         printf("Setting run number %d in ODB\n", run_number);
      if (debug_flag == 2)
         cm_msg(MINFO, "cm_transition", "cm_transition: Setting run number %d in ODB", run_number);

      status = db_set_value(hDB, 0, "Runinfo/Run number", &run_number, sizeof(run_number), 1, TID_INT);
      assert(status == SUCCESS);
      if (status != DB_SUCCESS)
         cm_msg(MERROR, "cm_transition", "cannot set Runinfo/Run number in database");
   }

   if (deferred) {
      /* remove transition request */
      i = 0;
      db_set_value(hDB, 0, "/Runinfo/Requested transition", &i, sizeof(int), 1, TID_INT);
   } else {
      status = db_find_key(hDB, 0, "System/Clients", &hRootKey);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "cm_transition", "cannot find System/Clients entry in database");
         if (errstr != NULL)
            strlcpy(errstr, "Cannot find /System/Clients in ODB", errstr_size);
         return status;
      }

      /* check if deferred transition already in progress */
      size = sizeof(i);
      db_get_value(hDB, 0, "/Runinfo/Requested transition", &i, &size, TID_INT, TRUE);
      if (i) {
         if (errstr != NULL)
            strlcpy(errstr, "Deferred transition already in progress", errstr_size);
         return CM_TRANSITION_IN_PROGRESS;
      }

      for (i = 0; trans_name[i].name[0] != 0; i++)
         if (trans_name[i].transition == transition) {
            trname = trans_name[i].name;
            break;
         }

      sprintf(tr_key_name, "Transition %s DEFERRED", trname);

      /* search database for clients with deferred transition request */
      for (i = 0, status = 0;; i++) {
         status = db_enum_key(hDB, hRootKey, i, &hSubkey);
         if (status == DB_NO_MORE_SUBKEYS)
            break;

         if (status == DB_SUCCESS) {
            size = sizeof(sequence_number);
            status = db_get_value(hDB, hSubkey, tr_key_name, &sequence_number, &size, TID_INT, FALSE);

            /* if registered for deferred transition, set flag in ODB and return */
            if (status == DB_SUCCESS) {
               size = NAME_LENGTH;
               db_get_value(hDB, hSubkey, "Name", str, &size, TID_STRING, TRUE);
               db_set_value(hDB, 0, "/Runinfo/Requested transition", &transition, sizeof(int), 1, TID_INT);

               if (debug_flag == 1)
                  printf("---- Transition %s deferred by client \"%s\" ----\n", trname, str);
               if (debug_flag == 2)
                  cm_msg(MINFO, "cm_transition",
                         "cm_transition: ---- Transition %s deferred by client \"%s\" ----", trname, str);

               if (errstr)
                  sprintf(errstr, "Transition %s deferred by client \"%s\"", trname, str);

               return CM_DEFERRED_TRANSITION;
            }
         }
      }
   }

   /* execute programs on start */
   if (transition == TR_START) {
      str[0] = 0;
      size = sizeof(str);
      db_get_value(hDB, 0, "/Programs/Execute on start run", str, &size, TID_STRING, TRUE);
      if (str[0])
         ss_system(str);

      db_find_key(hDB, 0, "/Programs", &hRootKey);
      if (hRootKey) {
         for (i = 0;; i++) {
            status = db_enum_key(hDB, hRootKey, i, &hKey);
            if (status == DB_NO_MORE_SUBKEYS)
               break;

            db_get_key(hDB, hKey, &key);

            /* don't check "execute on xxx" */
            if (key.type != TID_KEY)
               continue;

            size = sizeof(program_info);
            status = db_get_record(hDB, hKey, &program_info, &size, 0);
            if (status != DB_SUCCESS) {
               cm_msg(MERROR, "cm_transition", "Cannot get program info record");
               continue;
            }

            if (program_info.auto_start && program_info.start_command[0])
               ss_system(program_info.start_command);
         }
      }
   }

   /* set new start time in database */
   if (transition == TR_START) {
      /* ASCII format */
      cm_asctime(str, sizeof(str));
      db_set_value(hDB, 0, "Runinfo/Start Time", str, 32, 1, TID_STRING);

      /* reset stop time */
      seconds = 0;
      db_set_value(hDB, 0, "Runinfo/Stop Time binary", &seconds, sizeof(seconds), 1, TID_DWORD);

      /* Seconds since 1.1.1970 */
      cm_time(&seconds);
      db_set_value(hDB, 0, "Runinfo/Start Time binary", &seconds, sizeof(seconds), 1, TID_DWORD);
   }

   /* set stop time in database */
   if (transition == TR_STOP) {
      size = sizeof(state);
      status = db_get_value(hDB, 0, "Runinfo/State", &state, &size, TID_INT, TRUE);
      if (status != DB_SUCCESS)
         cm_msg(MERROR, "cm_transition", "cannot get Runinfo/State in database");

      if (state != STATE_STOPPED) {
         /* stop time binary */
         cm_time(&seconds);
         status = db_set_value(hDB, 0, "Runinfo/Stop Time binary", &seconds, sizeof(seconds), 1, TID_DWORD);
         if (status != DB_SUCCESS)
            cm_msg(MERROR, "cm_transition", "cannot set \"Runinfo/Stop Time binary\" in database");

         /* stop time ascii */
         cm_asctime(str, sizeof(str));
         status = db_set_value(hDB, 0, "Runinfo/Stop Time", str, 32, 1, TID_STRING);
         if (status != DB_SUCCESS)
            cm_msg(MERROR, "cm_transition", "cannot set \"Runinfo/Stop Time\" in database");
      }
   }

   status = db_find_key(hDB, 0, "System/Clients", &hRootKey);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "cm_transition", "cannot find System/Clients entry in database");
      if (errstr)
         strlcpy(errstr, "Cannot find /System/Clients in ODB", errstr_size);
      return status;
   }

   for (i = 0; trans_name[i].name[0] != 0; i++)
      if (trans_name[i].transition == transition) {
         trname = trans_name[i].name;
         break;
      }

   /* check that all transition clients are alive */
   for (i = 0;;) {
      status = db_enum_key(hDB, hRootKey, i, &hSubkey);
      if (status != DB_SUCCESS)
         break;

      status = cm_check_client(hDB, hSubkey);

      if (status == DB_SUCCESS) {
         /* this client is alive. Check next one! */
         i++;
         continue;
      }

      assert(status == CM_NO_CLIENT);

      /* start from scratch: removing odb entries as we iterate over them
       * does strange things to db_enum_key() */
      i = 0;
   }

   if (debug_flag == 1)
      printf("---- Transition %s started ----\n", trname);
   if (debug_flag == 2)
      cm_msg(MINFO, "cm_transition", "cm_transition: ---- Transition %s started ----", trname);

   sprintf(tr_key_name, "Transition %s", trname);

   /* search database for clients which registered for transition */
   n_tr_clients = 0;
   tr_client = NULL;

   for (i = 0, status = 0;; i++) {
      status = db_enum_key(hDB, hRootKey, i, &hSubkey);
      if (status == DB_NO_MORE_SUBKEYS)
         break;

      if (status == DB_SUCCESS) {
         status = db_find_key(hDB, hSubkey, tr_key_name, &hKeyTrans);

         if (status == DB_SUCCESS) {

            db_get_key(hDB, hKeyTrans, &key);

            for (j = 0; j < key.num_values; j++) {
               size = sizeof(sequence_number);
               status = db_get_data_index(hDB, hKeyTrans, &sequence_number, &size, j, TID_INT);
               assert(status == DB_SUCCESS);

               if (tr_client == NULL)
                  tr_client = (TR_CLIENT *) malloc(sizeof(TR_CLIENT));
               else
                  tr_client = (TR_CLIENT *) realloc(tr_client, sizeof(TR_CLIENT) * (n_tr_clients + 1));
               assert(tr_client);

               tr_client[n_tr_clients].sequence_number = sequence_number;

               if (hSubkey == hKeylocal) {
                  /* remember own client */
                  tr_client[n_tr_clients].port = 0;
               } else {
                  /* get client info */
                  size = sizeof(client_name);
                  db_get_value(hDB, hSubkey, "Name", client_name, &size, TID_STRING, TRUE);
                  strcpy(tr_client[n_tr_clients].client_name, client_name);

                  size = sizeof(port);
                  db_get_value(hDB, hSubkey, "Server Port", &port, &size, TID_INT, TRUE);
                  tr_client[n_tr_clients].port = port;

                  size = sizeof(host_name);
                  db_get_value(hDB, hSubkey, "Host", host_name, &size, TID_STRING, TRUE);
                  strcpy(tr_client[n_tr_clients].host_name, host_name);
               }

               n_tr_clients++;
            }
         }
      }
   }

   /* sort clients according to sequence number */
   if (n_tr_clients > 1)
      qsort(tr_client, n_tr_clients, sizeof(TR_CLIENT), tr_compare);

   /* contact ordered clients for transition */
   for (idx = 0; idx < n_tr_clients; idx++) {
      /* erase error string */
      error[0] = 0;

      if (debug_flag == 1)
         printf("\n==== Found client \"%s\" with sequence number %d\n",
                tr_client[idx].client_name, tr_client[idx].sequence_number);
      if (debug_flag == 2)
         cm_msg(MINFO, "cm_transition",
                "cm_transition: ==== Found client \"%s\" with sequence number %d",
                tr_client[idx].client_name, tr_client[idx].sequence_number);

      /* if own client call transition callback directly */
      if (tr_client[idx].port == 0) {
         for (i = 0; _trans_table[i].transition; i++)
            if (_trans_table[i].transition == transition)
               break;

         /* call registered function */
         if (_trans_table[i].transition == transition && _trans_table[i].func) {
            if (debug_flag == 1)
               printf("Calling local transition callback\n");
            if (debug_flag == 2)
               cm_msg(MINFO, "cm_transition", "cm_transition: Calling local transition callback");

            status = _trans_table[i].func(run_number, error);

            if (debug_flag == 1)
               printf("Local transition callback finished\n");
            if (debug_flag == 2)
               cm_msg(MINFO, "cm_transition", "cm_transition: Local transition callback finished");
         } else
            status = CM_SUCCESS;

         if (errstr != NULL)
            memcpy(errstr, error,
                   (INT) strlen(error) + 1 < errstr_size ? (INT) strlen(error) + 1 : errstr_size);

         if (status != CM_SUCCESS) {
            /* indicate abort */
            i = 1;
            db_set_value(hDB, 0, "/Runinfo/Start abort", &i, sizeof(INT), 1, TID_INT);
            i = 0;
            db_set_value(hDB, 0, "/Runinfo/Transition in progress", &i, sizeof(INT), 1, TID_INT);

            free(tr_client);
            return status;
         }

      } else {

         /* contact client if transition mask set */
         if (debug_flag == 1)
            printf("Connecting to client \"%s\" on host %s...\n", tr_client[idx].client_name,
                   tr_client[idx].host_name);
         if (debug_flag == 2)
            cm_msg(MINFO, "cm_transition",
                   "cm_transition: Connecting to client \"%s\" on host %s...",
                   tr_client[idx].client_name, tr_client[idx].host_name);

         /* set our timeout for rpc_client_connect() */
         old_timeout = rpc_get_option(-2, RPC_OTIMEOUT);
         rpc_set_option(-2, RPC_OTIMEOUT, connect_timeout);

         /* client found -> connect to its server port */
         status =
             rpc_client_connect(tr_client[idx].host_name, tr_client[idx].port, tr_client[idx].client_name,
                                &hConn);

         rpc_set_option(-2, RPC_OTIMEOUT, old_timeout);

         if (status != RPC_SUCCESS) {
            cm_msg(MERROR, "cm_transition",
                   "cannot connect to client \"%s\" on host %s, port %d, status %d",
                   tr_client[idx].client_name, tr_client[idx].host_name, tr_client[idx].port, status);
            if (errstr != NULL) {
               strlcpy(errstr, "Cannot connect to client \'", errstr_size);
               strlcat(errstr, tr_client[idx].client_name, errstr_size);
               strlcat(errstr, "\'", errstr_size);
            }

            /* clients that do not respond to transitions are dead or defective, get rid of them. K.O. */
            cm_shutdown(tr_client[idx].client_name, TRUE);
            cm_cleanup(tr_client[idx].client_name, TRUE);

            if (transition != TR_STOP) {
               /* indicate abort */
               i = 1;
               db_set_value(hDB, 0, "/Runinfo/Start abort", &i, sizeof(INT), 1, TID_INT);
               i = 0;
               db_set_value(hDB, 0, "/Runinfo/Transition in progress", &i, sizeof(INT), 1, TID_INT);

               free(tr_client);
               return status;
            }

            continue;
         }

         if (debug_flag == 1)
            printf("Connection established to client \"%s\" on host %s\n",
                   tr_client[idx].client_name, tr_client[idx].host_name);
         if (debug_flag == 2)
            cm_msg(MINFO, "cm_transition",
                   "cm_transition: Connection established to client \"%s\" on host %s",
                   tr_client[idx].client_name, tr_client[idx].host_name);

         /* call RC_TRANSITION on remote client with increased timeout */
         old_timeout = rpc_get_option(hConn, RPC_OTIMEOUT);
         rpc_set_option(hConn, RPC_OTIMEOUT, timeout);

         /* set FTPC protocol if in async mode */
         if (async_flag == ASYNC)
            rpc_set_option(hConn, RPC_OTRANSPORT, RPC_FTCP);

         if (debug_flag == 1)
            printf("Executing RPC transition client \"%s\" on host %s...\n",
                   tr_client[idx].client_name, tr_client[idx].host_name);
         if (debug_flag == 2)
            cm_msg(MINFO, "cm_transition",
                   "cm_transition: Executing RPC transition client \"%s\" on host %s...",
                   tr_client[idx].client_name, tr_client[idx].host_name);

         t0 = ss_millitime();

         status = rpc_client_call(hConn, RPC_RC_TRANSITION, transition,
                                  run_number, error, errstr_size, tr_client[idx].sequence_number);

         t1 = ss_millitime();

         /* reset timeout */
         rpc_set_option(hConn, RPC_OTIMEOUT, old_timeout);

         /* reset protocol */
         if (async_flag == ASYNC)
            rpc_set_option(hConn, RPC_OTRANSPORT, RPC_TCP);

         if (debug_flag == 1)
            printf("RPC transition finished client \"%s\" on host %s in %d ms with status %d\n",
                   tr_client[idx].client_name, tr_client[idx].host_name, t1 - t0, status);
         if (debug_flag == 2)
            cm_msg(MINFO, "cm_transition",
                   "cm_transition: RPC transition finished client \"%s\" on host %s in %d ms with status %d",
                   tr_client[idx].client_name, tr_client[idx].host_name, t1 - t0, status);

         if (errstr != NULL) {
            if (strlen(error) < 2)
               sprintf(errstr, "Unknown error %d from client \'%s\' on host %s", status,
                       tr_client[idx].client_name, tr_client[idx].host_name);
            else
               memcpy(errstr, error,
                      (INT) strlen(error) + 1 < (INT) errstr_size ? (INT) strlen(error) + 1 : errstr_size);
         }

         if (transition != TR_STOP && status != CM_SUCCESS) {
            /* indicate abort */
            i = 1;
            db_set_value(hDB, 0, "/Runinfo/Start abort", &i, sizeof(INT), 1, TID_INT);
            i = 0;
            db_set_value(hDB, 0, "/Runinfo/Transition in progress", &i, sizeof(INT), 1, TID_INT);

            free(tr_client);
            return status;
         }
      }
   }

   if (tr_client) {
      free(tr_client);
      tr_client = NULL;
   }

   if (debug_flag == 1)
      printf("\n---- Transition %s finished ----\n", trname);
   if (debug_flag == 2)
      cm_msg(MINFO, "cm_transition", "cm_transition: ---- Transition %s finished ----", trname);

   /* set new run state in database */
   if (transition == TR_START || transition == TR_RESUME)
      state = STATE_RUNNING;

   if (transition == TR_PAUSE)
      state = STATE_PAUSED;

   if (transition == TR_STOP)
      state = STATE_STOPPED;

   if (transition == TR_STARTABORT)
      state = STATE_STOPPED;

   size = sizeof(state);
   status = db_set_value(hDB, 0, "Runinfo/State", &state, size, 1, TID_INT);
   if (status != DB_SUCCESS)
      cm_msg(MERROR, "cm_transition", "cannot set Runinfo/State in database");

   /* send notification message */
   str[0] = 0;
   if (transition == TR_START)
      sprintf(str, "Run #%d started", run_number);
   if (transition == TR_STOP)
      sprintf(str, "Run #%d stopped", run_number);
   if (transition == TR_PAUSE)
      sprintf(str, "Run #%d paused", run_number);
   if (transition == TR_RESUME)
      sprintf(str, "Run #%d resumed", run_number);
   if (transition == TR_STARTABORT)
      sprintf(str, "Run #%d start aborted", run_number);

   if (str[0])
      cm_msg(MINFO, "cm_transition", str);

   /* lock/unlock ODB values if present */
   db_find_key(hDB, 0, "/Experiment/Lock when running", &hKey);
   if (hKey) {
      if (state == STATE_STOPPED)
         db_set_mode(hDB, hKey, MODE_READ | MODE_WRITE | MODE_DELETE, TRUE);
      else
         db_set_mode(hDB, hKey, MODE_READ, TRUE);
   }

   /* flush online database */
   if (transition == TR_STOP)
      db_flush_database(hDB);

   /* execute/stop programs on stop */
   if (transition == TR_STOP) {
      str[0] = 0;
      size = sizeof(str);
      db_get_value(hDB, 0, "/Programs/Execute on stop run", str, &size, TID_STRING, TRUE);
      if (str[0])
         ss_system(str);

      db_find_key(hDB, 0, "/Programs", &hRootKey);
      if (hRootKey) {
         for (i = 0;; i++) {
            status = db_enum_key(hDB, hRootKey, i, &hKey);
            if (status == DB_NO_MORE_SUBKEYS)
               break;

            db_get_key(hDB, hKey, &key);

            /* don't check "execute on xxx" */
            if (key.type != TID_KEY)
               continue;

            size = sizeof(program_info);
            status = db_get_record(hDB, hKey, &program_info, &size, 0);
            if (status != DB_SUCCESS) {
               cm_msg(MERROR, "cm_transition", "Cannot get program info record");
               continue;
            }

            if (program_info.auto_stop)
               cm_shutdown(key.name, FALSE);
         }
      }
   }


   /* indicate success */
   i = 0;
   db_set_value(hDB, 0, "/Runinfo/Transition in progress", &i, sizeof(INT), 1, TID_INT);

   if (errstr != NULL)
      strlcpy(errstr, "Success", errstr_size);

   return CM_SUCCESS;
}

INT cm_transition(INT transition, INT run_number, char *errstr, INT errstr_size, INT async_flag,
                  INT debug_flag)
{
   int status;

   status = cm_transition1(transition, run_number, errstr, errstr_size, async_flag, debug_flag);

   if (transition == TR_START && status != CM_SUCCESS) {
      cm_msg(MERROR, "cm_transition", "Could not start a run: cm_transition() status %d, message \'%s\'",
             status, errstr);
      cm_transition1(TR_STARTABORT, run_number, NULL, 0, async_flag, debug_flag);
   }

   return status;
}



/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************/
INT cm_dispatch_ipc(char *message, int s)
/********************************************************************\

  Routine: cm_dispatch_ipc

  Purpose: Called from ss_suspend if an IPC message arrives

  Input:
    INT   msg               IPC message we got, MSG_ODB/MSG_BM
    INT   p1, p2            Optional parameters
    int   s                 Optional server socket

  Output:
    none

  Function value:
    CM_SUCCESS              Successful completion

\********************************************************************/
{
   if (message[0] == 'O') {
      HNDLE hDB, hKey;
      sscanf(message + 2, "%d %d", &hDB, &hKey);
      return db_update_record(hDB, hKey, s);
   }

   /* message == "B  " means "resume event sender" */
   if (message[0] == 'B' && message[2] != ' ') {
      char str[80];

      strcpy(str, message + 2);
      if (strchr(str, ' '))
         *strchr(str, ' ') = 0;

      if (s)
         return bm_notify_client(str, s);
      else
         return bm_push_event(str);
   }

   return CM_SUCCESS;
}

/********************************************************************/
static BOOL _ctrlc_pressed = FALSE;

void cm_ctrlc_handler(int sig)
{
   int i;

   i = sig;                     /* avoid compiler warning */

   if (_ctrlc_pressed) {
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


/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Central yield functions for clients. This routine should
be called in an infinite loop by a client in order to
give the MIDAS system the opportunity to receive commands
over RPC channels, update database records and receive
events.
@param millisec         Timeout in millisec. If no message is
                        received during the specified timeout,
                        the routine returns. If millisec=-1,
                        it only returns when receiving an
                        RPC_SHUTDOWN message.
@return CM_SUCCESS, RPC_SHUTDOWN
*/
INT cm_yield(INT millisec)
{
   INT status;
   BOOL bMore;
   static DWORD last_checked = 0;

   /* check for ctrl-c */
   if (_ctrlc_pressed)
      return RPC_SHUTDOWN;

   /* flush the cm_msg buffer */
   cm_msg_flush_buffer();

   /* check for available events */
   if (rpc_is_remote()) {
      bMore = bm_poll_event(TRUE);
      if (bMore)
         status = ss_suspend(0, 0);
      else
         status = ss_suspend(millisec, 0);

      return status;
   }

   /* check alarms once every 10 seconds */
   if (!rpc_is_remote() && ss_time() - last_checked > 10) {
      al_check();
      last_checked = ss_time();
   }

   bMore = bm_check_buffers();

   if (bMore) {
      /* if events available, quickly check other IPC channels */
      status = ss_suspend(0, 0);
   } else {
      /* mark event buffers for ready-to-receive */
      bm_mark_read_waiting(TRUE);

      status = ss_suspend(millisec, 0);

      /* unmark event buffers for ready-to-receive */
      bm_mark_read_waiting(FALSE);
   }

   /* flush the cm_msg buffer */
   cm_msg_flush_buffer();

   return status;
}

/********************************************************************/
/**
Executes command via system() call
@param    command          Command string to execute
@param    result           stdout of command
@param    bufsize          string size in byte
@return   CM_SUCCESS
*/
INT cm_execute(const char *command, char *result, INT bufsize)
{
   char str[256];
   INT n;
   int fh;

   if (rpc_is_remote())
      return rpc_call(RPC_CM_EXECUTE, command, result, bufsize);

   if (bufsize > 0) {
      strcpy(str, command);
      sprintf(str, "%s > %d.tmp", command, ss_getpid());

      system(str);

      sprintf(str, "%d.tmp", ss_getpid());
      fh = open(str, O_RDONLY, 0644);
      result[0] = 0;
      if (fh) {
         n = read(fh, result, bufsize - 1);
         result[MAX(0, n)] = 0;
         close(fh);
      }
      remove(str);
   } else
      system(command);

   return CM_SUCCESS;
}



/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************/
INT cm_register_function(INT id, INT(*func) (INT, void **))
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
   INT status;
   char str[80];

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


/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/**dox***************************************************************/
                                                                                                                               /** @} *//* end of cmfunctionc */

/**dox***************************************************************/
/** @addtogroup bmfunctionc
 *
 *  @{  */

/********************************************************************\
*                                                                    *
*                 bm_xxx  -  Buffer Manager Functions                *
*                                                                    *
\********************************************************************/

/********************************************************************/
/**
Check if an event matches a given event request by the
event id and trigger mask
@param event_id      Event ID of request
@param trigger_mask  Trigger mask of request
@param pevent    Pointer to event to check
@return TRUE      if event matches request
*/
INT bm_match_event(short int event_id, short int trigger_mask, EVENT_HEADER * pevent)
{
   if ((pevent->event_id & 0xF000) == EVENTID_FRAG1 || (pevent->event_id & 0xF000) == EVENTID_FRAG)
      /* fragmented event */
      return ((event_id == EVENTID_ALL ||
               event_id == (pevent->event_id & 0x0FFF)) &&
              (trigger_mask == TRIGGER_ALL || (trigger_mask & pevent->trigger_mask)));

   return ((event_id == EVENTID_ALL ||
            event_id == pevent->event_id) && (trigger_mask == TRIGGER_ALL
                                              || (trigger_mask & pevent->trigger_mask)));
}

/********************************************************************/
/**
Called to forcibly disconnect given client from a data buffer
*/
void bm_remove_client_locked(BUFFER_HEADER * pheader, int j)
{
   int k, nc;
   BUFFER_CLIENT *pbctmp;

   /* clear entry from client structure in buffer header */
   memset(&(pheader->client[j]), 0, sizeof(BUFFER_CLIENT));

   /* calculate new max_client_index entry */
   for (k = MAX_CLIENTS - 1; k >= 0; k--)
      if (pheader->client[k].pid != 0)
         break;
   pheader->max_client_index = k + 1;

   /* count new number of clients */
   for (k = MAX_CLIENTS - 1, nc = 0; k >= 0; k--)
      if (pheader->client[k].pid != 0)
         nc++;
   pheader->num_clients = nc;

   /* check if anyone is waiting and wake him up */
   pbctmp = pheader->client;

   for (k = 0; k < pheader->max_client_index; k++, pbctmp++)
      if (pbctmp->pid && (pbctmp->write_wait || pbctmp->read_wait))
         ss_resume(pbctmp->port, "B  ");
}

/********************************************************************/
/**
Check all clients on buffer, remove invalid clients
*/
static void bm_cleanup_buffer_locked(int i, const char *who, DWORD actual_time)
{
   BUFFER_HEADER *pheader;
   BUFFER_CLIENT *pbclient;
   int j;

   pheader = _buffer[i].buffer_header;
   pbclient = pheader->client;

   /* now check other clients */
   for (j = 0; j < pheader->max_client_index; j++, pbclient++) {
      
#ifdef OS_UNIX
#ifdef ESRCH
      if (pbclient->pid) {
         errno = 0;
         kill(pbclient->pid, 0);
         if (errno == ESRCH) {
            cm_msg(MINFO, "bm_cleanup", "Client \'%s\' on buffer \'%s\' removed by %s because process pid %d does not exist", pbclient->name, pheader->name, who, pbclient->pid);
            
            bm_remove_client_locked(pheader, j);
            continue;
         }
      }
#endif
#endif
      
      /* If client process has no activity, clear its buffer entry. */
      if (pbclient->pid && pbclient->watchdog_timeout > 0 &&
          actual_time > pbclient->last_activity &&
          actual_time - pbclient->last_activity > pbclient->watchdog_timeout) {
         
         /* now make again the check with the buffer locked */
         actual_time = ss_millitime();
         if (pbclient->pid && pbclient->watchdog_timeout > 0 &&
             actual_time > pbclient->last_activity &&
             actual_time - pbclient->last_activity > pbclient->watchdog_timeout) {
            
            cm_msg(MINFO, "bm_cleanup", "Client \'%s\' on buffer \'%s\' removed by %s (idle %1.1lfs,TO %1.0lfs)",
                   pbclient->name, pheader->name, who,
                   (actual_time - pbclient->last_activity) / 1000.0,
                   pbclient->watchdog_timeout / 1000.0);
            
            bm_remove_client_locked(pheader, j);
         }
      }
   }
}

/**
Update last activity time
*/
static void cm_update_last_activity(DWORD actual_time)
{
   int i, idx;

   /* check buffers */
   for (i = 0; i < _buffer_entries; i++)
      if (_buffer[i].attached) {
         idx = bm_validate_client_index(&_buffer[i], FALSE);
         if (idx >= 0) {
            _buffer[i].buffer_header->client[idx].last_activity = actual_time;
         }
      }

   /* check online databases */
   for (i = 0; i < _database_entries; i++)
      if (_database[i].attached) {
         /* update the last_activity entry to show that we are alive */
         _database[i].database_header->client[_database[i].client_index].last_activity = actual_time;
      }
}

/**
Check all clients on all buffers, remove invalid clients
*/
static void bm_cleanup(const char *who, DWORD actual_time, BOOL wrong_interval)
{
   BUFFER_HEADER *pheader;
   BUFFER_CLIENT *pbclient;
   int i, idx;

   /* check buffers */
   for (i = 0; i < _buffer_entries; i++)
      if (_buffer[i].attached) {
         /* update the last_activity entry to show that we are alive */

         bm_lock_buffer(i+1);

         pheader = _buffer[i].buffer_header;
         pbclient = pheader->client;
         idx = bm_validate_client_index(&_buffer[i], TRUE);
         pbclient[idx].last_activity = actual_time;

         /* don't check other clients if interval is strange */
         if (!wrong_interval)
	   bm_cleanup_buffer_locked(i, who, actual_time);

         bm_unlock_buffer(i+1);
      }
}

/********************************************************************/
/**
Open an event buffer.
Two default buffers are created by the system.
The "SYSTEM" buffer is used to
exchange events and the "SYSMSG" buffer is used to exchange system messages.
The name and size of the event buffers is defined in midas.h as
EVENT_BUFFER_NAME and 2*MAX_EVENT_SIZE.
Following example opens the "SYSTEM" buffer, requests events with ID 1 and
enters a main loop. Events are then received in process_event()
\code
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
  bm_open_buffer(EVENT_BUFFER_NAME, 2*MAX_EVENT_SIZE, &hbuf);
  bm_request_event(hbuf, 1, TRIGGER_ALL, GET_ALL, request_id, process_event);

  do
  {
   status = cm_yield(1000);
  } while (status != RPC_SHUTDOWN && status != SS_ABORT);
  cm_disconnect_experiment();
  return 0;
}
\endcode
@param buffer_name Name of buffer
@param buffer_size Default size of buffer in bytes. Can by overwritten with ODB value
@param buffer_handle Buffer handle returned by function
@return BM_SUCCESS, BM_CREATED <br>
BM_NO_SHM Shared memory cannot be created <br>
BM_NO_SEMAPHORE Semaphore cannot be created <br>
BM_NO_MEMORY Not enough memory to create buffer descriptor <br>
BM_MEMSIZE_MISMATCH Buffer size conflicts with an existing buffer of
different size <br>
BM_INVALID_PARAM Invalid parameter
*/
INT bm_open_buffer(char *buffer_name, INT buffer_size, INT * buffer_handle)
{
   INT status;

   if (rpc_is_remote()) {
      status = rpc_call(RPC_BM_OPEN_BUFFER, buffer_name, buffer_size, buffer_handle);
      bm_mark_read_waiting(TRUE);
      return status;
   }
#ifdef LOCAL_ROUTINES
   {
      INT i, handle, size;
      BUFFER_CLIENT *pclient;
      BOOL shm_created;
      HNDLE shm_handle;
      BUFFER_HEADER *pheader;
      HNDLE hDB, odb_key;
      char odb_path[256];
      void *p;

      bm_cleanup("bm_open_buffer", ss_millitime(), FALSE);

      if (!buffer_name || !buffer_name[0]) {
         cm_msg(MERROR, "bm_open_buffer", "cannot open buffer with zero name");
         return BM_INVALID_PARAM;
      }

      status = cm_get_experiment_database(&hDB, &odb_key);

      if (status != SUCCESS || hDB == 0) {
         //cm_msg(MERROR, "bm_open_buffer", "cannot open buffer \'%s\' - not connected to ODB", buffer_name);
         return BM_NO_SHM;
      }

      /* get buffer size from ODB, user parameter as default if not present in ODB */
      strlcpy(odb_path, "/Experiment/Buffer sizes/", sizeof(odb_path));
      strlcat(odb_path, buffer_name, sizeof(odb_path));

      size = sizeof(INT);
      status = db_get_value(hDB, 0, odb_path, &buffer_size, &size, TID_DWORD, TRUE);

      if (buffer_size <= 0 || buffer_size > 1 * 1024 * 1024 * 1024) {
         cm_msg(MERROR, "bm_open_buffer", "cannot open buffer \'%s\' - invalid buffer size %d", buffer_name, buffer_size);
         return BM_INVALID_PARAM;
      }

      /* allocate new space for the new buffer descriptor */
      if (_buffer_entries == 0) {
         _buffer = (BUFFER *) M_MALLOC(sizeof(BUFFER));
         memset(_buffer, 0, sizeof(BUFFER));
         if (_buffer == NULL) {
            *buffer_handle = 0;
            return BM_NO_MEMORY;
         }

         _buffer_entries = 1;
         i = 0;
      } else {
         /* check if buffer alreay is open */
         for (i = 0; i < _buffer_entries; i++)
            if (_buffer[i].attached && equal_ustring(_buffer[i].buffer_header->name, buffer_name)) {
               if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_SINGLE &&
                   _buffer[i].index != rpc_get_server_acception())
                  continue;

               if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_SINGLE && _buffer[i].index != ss_gettid())
                  continue;

               *buffer_handle = i + 1;
               return BM_SUCCESS;
            }

         /* check for a deleted entry */
         for (i = 0; i < _buffer_entries; i++)
            if (!_buffer[i].attached)
               break;

         /* if not found, create new one */
         if (i == _buffer_entries) {
            _buffer = (BUFFER *) realloc(_buffer, sizeof(BUFFER) * (_buffer_entries + 1));
            memset(&_buffer[_buffer_entries], 0, sizeof(BUFFER));

            _buffer_entries++;
            if (_buffer == NULL) {
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
      //status = ss_shm_open(buffer_name, sizeof(BUFFER_HEADER) + buffer_size,
      //               (void **) &(_buffer[handle].buffer_header), &shm_handle, FALSE);
      status = ss_shm_open(buffer_name, sizeof(BUFFER_HEADER) + buffer_size, &p, &shm_handle, FALSE);
      _buffer[handle].buffer_header = (BUFFER_HEADER *) p;

      if (status != SS_SUCCESS && status != SS_CREATED) {
         *buffer_handle = 0;
         _buffer_entries--;
         return BM_NO_SHM;
      }

      pheader = _buffer[handle].buffer_header;

      shm_created = (status == SS_CREATED);

      if (shm_created) {
         /* setup header info if buffer was created */
         memset(pheader, 0, sizeof(BUFFER_HEADER) + buffer_size);

         strcpy(pheader->name, buffer_name);
         pheader->size = buffer_size;
      } else {
         /* check if buffer size is identical */
         if (pheader->size != buffer_size) {
            cm_msg(MERROR, "bm_open_buffer", "Requested buffer size (%d) differs from existing size (%d)",
                   buffer_size, pheader->size);
            *buffer_handle = 0;
            _buffer_entries--;
            return BM_MEMSIZE_MISMATCH;
         }
      }

      /* create semaphore for the buffer */
      status = ss_semaphore_create(buffer_name, &(_buffer[handle].semaphore));
      if (status != SS_CREATED && status != SS_SUCCESS) {
         *buffer_handle = 0;
         _buffer_entries--;
         return BM_NO_SEMAPHORE;
      }

      /* first lock buffer */
      bm_lock_buffer(handle + 1);

      bm_cleanup_buffer_locked(handle, "bm_open_buffer", ss_millitime());

      /*
         Now we have a BUFFER_HEADER, so let's setup a CLIENT
         structure in that buffer. The information there can also
         be seen by other processes.
       */

      for (i = 0; i < MAX_CLIENTS; i++)
         if (pheader->client[i].pid == 0)
            break;

      if (i == MAX_CLIENTS) {
         bm_unlock_buffer(handle + 1);
         *buffer_handle = 0;
         cm_msg(MERROR, "bm_open_buffer", "buffer \'%s\' maximum number of clients exceeded", buffer_name);
         return BM_NO_SLOT;
      }

      /* store slot index in _buffer structure */
      _buffer[handle].client_index = i;

      /*
         Save the index of the last client of that buffer so that later only
         the clients 0..max_client_index-1 have to be searched through.
       */
      pheader->num_clients++;
      if (i + 1 > pheader->max_client_index)
         pheader->max_client_index = i + 1;

      /* setup buffer header and client structure */
      pclient = &pheader->client[i];

      memset(pclient, 0, sizeof(BUFFER_CLIENT));
      /* use client name previously set by bm_set_name */
      cm_get_client_info(pclient->name);
      if (pclient->name[0] == 0)
         strcpy(pclient->name, "unknown");
      pclient->pid = ss_getpid();

      ss_suspend_get_port(&pclient->port);

      pclient->read_pointer = pheader->write_pointer;
      pclient->last_activity = ss_millitime();

      cm_get_watchdog_params(NULL, &pclient->watchdog_timeout);

      bm_unlock_buffer(handle + 1);

      /* setup _buffer entry */
      _buffer[handle].buffer_data = _buffer[handle].buffer_header + 1;
      _buffer[handle].attached = TRUE;
      _buffer[handle].shm_handle = shm_handle;
      _buffer[handle].callback = FALSE;

      /* remember to which connection acutal buffer belongs */
      if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_SINGLE)
         _buffer[handle].index = rpc_get_server_acception();
      else
         _buffer[handle].index = ss_gettid();

      *buffer_handle = (handle + 1);

      /* initialize buffer counters */
      bm_init_buffer_counters(handle + 1);

      /* setup dispatcher for receive events */
      ss_suspend_set_dispatch(CH_IPC, 0, (int (*)(void)) cm_dispatch_ipc);

      bm_cleanup("bm_open_buffer", ss_millitime(), FALSE);

      if (shm_created)
         return BM_CREATED;
   }
#endif                          /* LOCAL_ROUTINES */

   return BM_SUCCESS;
}

/********************************************************************/
/**
Closes an event buffer previously opened with bm_open_buffer().
@param buffer_handle buffer handle
@return BM_SUCCESS, BM_INVALID_HANDLE
*/
INT bm_close_buffer(INT buffer_handle)
{
   if (rpc_is_remote())
      return rpc_call(RPC_BM_CLOSE_BUFFER, buffer_handle);

#ifdef LOCAL_ROUTINES
   {
      BUFFER_CLIENT *pclient;
      BUFFER_HEADER *pheader;
      INT i, j, idx, destroy_flag;

      if (buffer_handle > _buffer_entries || buffer_handle <= 0) {
         cm_msg(MERROR, "bm_close_buffer", "invalid buffer handle %d", buffer_handle);
         return BM_INVALID_HANDLE;
      }

      /* check if buffer got already closed */
      if (!_buffer[buffer_handle - 1].attached) {
         return BM_SUCCESS;
      }

      /*
         Check if buffer was opened by current thread. This is necessary
         in the server process where one thread may not close the buffer
         of other threads.
       */

      idx = bm_validate_client_index(&_buffer[buffer_handle - 1], FALSE);
      pheader = _buffer[buffer_handle - 1].buffer_header;

      if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_SINGLE &&
          _buffer[buffer_handle - 1].index != rpc_get_server_acception()) {
         return BM_INVALID_HANDLE;
      }

      if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_SINGLE
          && _buffer[buffer_handle - 1].index != ss_gettid()) {
         return BM_INVALID_HANDLE;
      }

      if (!_buffer[buffer_handle - 1].attached) {
         /* don't produce error, since bm_close_all_buffers() might want to close an
            already closed buffer */
         return BM_SUCCESS;
      }

      /* delete all requests for this buffer */
      for (i = 0; i < _request_list_entries; i++)
         if (_request_list[i].buffer_handle == buffer_handle)
            bm_delete_request(i);

      /* first lock buffer */
      bm_lock_buffer(buffer_handle);

      /* mark entry in _buffer as empty */
      _buffer[buffer_handle - 1].attached = FALSE;

      if (idx >= 0) {
         /* clear entry from client structure in buffer header */
         memset(&(pheader->client[idx]), 0, sizeof(BUFFER_CLIENT));
      }

      /* calculate new max_client_index entry */
      for (i = MAX_CLIENTS - 1; i >= 0; i--)
         if (pheader->client[i].pid != 0)
            break;
      pheader->max_client_index = i + 1;

      /* count new number of clients */
      for (i = MAX_CLIENTS - 1, j = 0; i >= 0; i--)
         if (pheader->client[i].pid != 0)
            j++;
      pheader->num_clients = j;

      destroy_flag = (pheader->num_clients == 0);

      /* free cache */
      if (_buffer[buffer_handle - 1].read_cache_size > 0) {
         M_FREE(_buffer[buffer_handle - 1].read_cache);
         _buffer[buffer_handle - 1].read_cache = NULL;
      }
      if (_buffer[buffer_handle - 1].write_cache_size > 0) {
         M_FREE(_buffer[buffer_handle - 1].write_cache);
         _buffer[buffer_handle - 1].write_cache = NULL;
      }

      /* check if anyone is waiting and wake him up */
      pclient = pheader->client;

      for (i = 0; i < pheader->max_client_index; i++, pclient++)
         if (pclient->pid && (pclient->write_wait || pclient->read_wait))
            ss_resume(pclient->port, "B  ");

      /* unmap shared memory, delete it if we are the last */
      ss_shm_close(pheader->name, _buffer[buffer_handle - 1].buffer_header,
                   _buffer[buffer_handle - 1].shm_handle, destroy_flag);

      /* unlock buffer */
      bm_unlock_buffer(buffer_handle);

      /* delete semaphore */
      ss_semaphore_delete(_buffer[buffer_handle - 1].semaphore, destroy_flag);

      /* update _buffer_entries */
      if (buffer_handle == _buffer_entries)
         _buffer_entries--;

      if (_buffer_entries > 0)
         _buffer = (BUFFER *) realloc(_buffer, sizeof(BUFFER) * (_buffer_entries));
      else {
         M_FREE(_buffer);
         _buffer = NULL;
      }
   }
#endif                          /* LOCAL_ROUTINES */

   return BM_SUCCESS;
}

/********************************************************************/
/**
Close all open buffers
@return BM_SUCCESS
*/
INT bm_close_all_buffers(void)
{
   if (rpc_is_remote())
      return rpc_call(RPC_BM_CLOSE_ALL_BUFFERS);

#ifdef LOCAL_ROUTINES
   {
      INT i;

      for (i = _buffer_entries; i > 0; i--)
         bm_close_buffer(i);
   }
#endif                          /* LOCAL_ROUTINES */

   return BM_SUCCESS;
}

/**dox***************************************************************/
                                                                                                                               /** @} *//* end of bmfunctionc */

/**dox***************************************************************/
/** @addtogroup cmfunctionc
 *
 *  @{  */

/*-- Watchdog routines ---------------------------------------------*/
#ifdef LOCAL_ROUTINES

/********************************************************************/
/**
Called at periodic intervals, checks if all clients are
alive. If one process died, its client entries are cleaned up.
@param dummy unused!
*/
void cm_watchdog(int dummy)
{
   DATABASE_HEADER *pdbheader;
   DATABASE_CLIENT *pdbclient;
   KEY *pkey;
   DWORD actual_time, interval;
   INT client_pid;
   INT i, j, k, nc, status;
   BOOL time_changed, wrong_interval;

   i = dummy;                   /* avoid compiler warning */

   /* return immediately if watchdog has been disabled in meantime */
   if (!_call_watchdog)
      return;

#if 0
   actual_time = ss_millitime();

   cm_update_last_activity(actual_time);

   if ((actual_time > _watchdog_last_called) && (actual_time - _watchdog_last_called < 10000)) {

      /* Schedule next watchdog call */
      if (_call_watchdog)
         ss_alarm(WATCHDOG_INTERVAL, cm_watchdog);
      
      return;
   }
#endif

   /* prevent deadlock between ODB and SYSMSG by skipping watchdog tests if odb is locked by us */
   if (1) {
      int count = -1;
      HNDLE hDB;

      cm_get_experiment_database(&hDB, NULL);

      if (hDB)
         count = db_get_lock_cnt(hDB);

      if (count) {
         //cm_msg(MINFO, "cm_watchdog", "Called with ODB lock count %d!", count);

         cm_update_last_activity(ss_millitime());

         /* Schedule next watchdog call */
         if (_call_watchdog)
            ss_alarm(WATCHDOG_INTERVAL, cm_watchdog);
         
         return;
      }
   }

   /* extra check on watchdog interval */
   if (0) {
      static time_t last = 0;
      time_t now = time(NULL);
      fprintf(stderr, "cm_watchdog interval %d\n", (int)(now - last));
      last = now;
   }

   /* tell system services that we are in async mode ... */
   ss_set_async_flag(TRUE);

   /* Calculate the time since last watchdog call. Kill clients if they
      are inactive for more than the timeout they specified */
   actual_time = ss_millitime();
   if (_watchdog_last_called == 0)
      _watchdog_last_called = actual_time - WATCHDOG_INTERVAL;
   interval = actual_time - _watchdog_last_called;

   /* check if system time has been changed more than 10 min */
   time_changed = interval > 600000;
   wrong_interval = interval < 0.8 * WATCHDOG_INTERVAL || interval > 1.2 * WATCHDOG_INTERVAL;

   if (time_changed)
      cm_msg(MINFO, "cm_watchdog",
             "System time has been changed! last:%dms  now:%dms  delta:%dms",
             _watchdog_last_called, actual_time, interval);

   bm_cleanup("cm_watchdog", actual_time, wrong_interval);

   /* check online databases */
   for (i = 0; i < _database_entries; i++)
      if (_database[i].attached) {
         /* update the last_activity entry to show that we are alive */
         pdbheader = _database[i].database_header;
         pdbclient = pdbheader->client;
         pdbclient[_database[i].client_index].last_activity = actual_time;

         /* don't check other clients if interval is stange */
         if (wrong_interval)
            continue;

         /* now check other clients */
         for (j = 0; j < pdbheader->max_client_index; j++, pdbclient++)
            /* If client process has no activity, clear its buffer entry. */
            if (pdbclient->pid && pdbclient->watchdog_timeout > 0 &&
                actual_time - pdbclient->last_activity > pdbclient->watchdog_timeout) {
               client_pid = pdbclient->pid;

               db_lock_database(i + 1);

               /* now make again the check with the buffer locked */
               actual_time = ss_millitime();
               if (pdbclient->pid && pdbclient->watchdog_timeout &&
                   actual_time > pdbclient->last_activity &&
                   actual_time - pdbclient->last_activity > pdbclient->watchdog_timeout) {

                  cm_msg(MINFO, "cm_watchdog", "Client \'%s\' (PID %d) on database \'%s\' removed by cm_watchdog (idle %1.1lfs,TO %1.0lfs)",
                         pdbclient->name, client_pid, pdbheader->name,
                         (actual_time - pdbclient->last_activity) / 1000.0,
                         pdbclient->watchdog_timeout / 1000.0);

                  /* decrement notify_count for open records and clear exclusive mode */
                  for (k = 0; k < pdbclient->max_index; k++)
                     if (pdbclient->open_record[k].handle) {
                        pkey = (KEY *) ((char *) pdbheader + pdbclient->open_record[k].handle);
                        if (pkey->notify_count > 0)
                           pkey->notify_count--;

                        if (pdbclient->open_record[k].access_mode & MODE_WRITE)
                           db_set_mode(i + 1, pdbclient->open_record[k].handle, (WORD) (pkey->access_mode & ~MODE_EXCLUSIVE), 2);
                     }

                  status = cm_delete_client_info(i + 1, client_pid);
                  if (status != CM_SUCCESS)
                     cm_msg(MERROR, "cm_watchdog", "Cannot delete client info for client \'%s\', pid %d from database \'%s\', status %d", pdbclient->name, client_pid, pdbheader->name, status);

                  /* clear entry from client structure in buffer header */
                  memset(&(pdbheader->client[j]), 0, sizeof(DATABASE_CLIENT));

                  /* calculate new max_client_index entry */
                  for (k = MAX_CLIENTS - 1; k >= 0; k--)
                     if (pdbheader->client[k].pid != 0)
                        break;
                  pdbheader->max_client_index = k + 1;

                  /* count new number of clients */
                  for (k = MAX_CLIENTS - 1, nc = 0; k >= 0; k--)
                     if (pdbheader->client[k].pid != 0)
                        nc++;
                  pdbheader->num_clients = nc;
               }

               db_unlock_database(i + 1);
            }
      }

   _watchdog_last_called = actual_time;

   ss_set_async_flag(FALSE);

   /* Schedule next watchdog call */
   if (_call_watchdog)
      ss_alarm(WATCHDOG_INTERVAL, cm_watchdog);
}

/********************************************************************/
/**
Temporarily disable watchdog calling. Used for tape IO
not to interrupt lengthy operations like mount.
@param flag FALSE for disable, TRUE for re-enable
@return CM_SUCCESS
*/
INT cm_enable_watchdog(BOOL flag)
{
   static INT timeout = DEFAULT_WATCHDOG_TIMEOUT;
   static BOOL call_flag = FALSE;

   if (flag) {
      if (call_flag)
         cm_set_watchdog_params(TRUE, timeout);
   } else {
      call_flag = _call_watchdog;
      timeout = _watchdog_timeout;
      if (call_flag)
         cm_set_watchdog_params(FALSE, 0);
   }

   return CM_SUCCESS;
}

#endif                          /* local routines */

/********************************************************************/
/**
Shutdown (exit) other MIDAS client
@param name           Client name or "all" for all clients
@param bUnique        If true, look for the exact client name.
                      If false, look for namexxx where xxx is
                      a any number.

@return CM_SUCCESS, CM_NO_CLIENT, DB_NO_KEY
*/
INT cm_shutdown(const char *name, BOOL bUnique)
{
   INT status, return_status, i, size;
   HNDLE hDB, hKeyClient, hKey, hSubkey, hKeyTmp, hConn;
   KEY key;
   char client_name[NAME_LENGTH], remote_host[HOST_NAME_LENGTH];
   INT port;
   DWORD start_time;

   cm_get_experiment_database(&hDB, &hKeyClient);

   status = db_find_key(hDB, 0, "System/Clients", &hKey);
   if (status != DB_SUCCESS)
      return DB_NO_KEY;

   return_status = CM_NO_CLIENT;

   /* loop over all clients */
   for (i = 0;; i++) {
      status = db_enum_key(hDB, hKey, i, &hSubkey);
      if (status == DB_NO_MORE_SUBKEYS)
         break;

      /* don't shutdown ourselves */
      if (hSubkey == hKeyClient)
         continue;

      if (status == DB_SUCCESS) {
         db_get_key(hDB, hSubkey, &key);

         /* contact client */
         size = sizeof(client_name);
         status = db_get_value(hDB, hSubkey, "Name", client_name, &size, TID_STRING, FALSE);
         if (status != DB_SUCCESS)
            continue;

         if (!bUnique)
            client_name[strlen(name)] = 0;      /* strip number */

         /* check if individual client */
         if (!equal_ustring("all", name) && !equal_ustring(client_name, name))
            continue;

         size = sizeof(port);
         db_get_value(hDB, hSubkey, "Server Port", &port, &size, TID_INT, TRUE);

         size = sizeof(remote_host);
         db_get_value(hDB, hSubkey, "Host", remote_host, &size, TID_STRING, TRUE);

         /* client found -> connect to its server port */
         status = rpc_client_connect(remote_host, port, client_name, &hConn);
         if (status != RPC_SUCCESS) {
            int client_pid = atoi(key.name);
            return_status = CM_NO_CLIENT;
            cm_msg(MERROR, "cm_shutdown", "Cannot connect to client \'%s\' on host \'%s\', port %d",
                   client_name, remote_host, port);
#ifdef SIGKILL
            cm_msg(MERROR, "cm_shutdown", "Killing and Deleting client \'%s\' pid %d", client_name,
                   client_pid);
            kill(client_pid, SIGKILL);
            status = cm_delete_client_info(hDB, client_pid);
            if (status != CM_SUCCESS)
               cm_msg(MERROR, "cm_shutdown", "Cannot delete client info for client \'%s\', pid %d, status %d",
                      name, client_pid, status);
#endif
         } else {
            /* call disconnect with shutdown=TRUE */
            rpc_client_disconnect(hConn, TRUE);

            /* wait until client has shut down */
            start_time = ss_millitime();
            do {
               ss_sleep(100);
               status = db_find_key(hDB, hKey, key.name, &hKeyTmp);
            } while (status == DB_SUCCESS && (ss_millitime() - start_time < 5000));

            if (status == DB_SUCCESS) {
               int client_pid = atoi(key.name);
               return_status = CM_NO_CLIENT;
               cm_msg(MERROR, "cm_shutdown", "Client \'%s\' not responding to shutdown command", client_name);
#ifdef SIGKILL
               cm_msg(MERROR, "cm_shutdown", "Killing and Deleting client \'%s\' pid %d", client_name,
                      client_pid);
               kill(client_pid, SIGKILL);
               status = cm_delete_client_info(hDB, client_pid);
               if (status != CM_SUCCESS)
                  cm_msg(MERROR, "cm_shutdown",
                         "Cannot delete client info for client \'%s\', pid %d, status %d", name, client_pid,
                         status);
#endif
               return_status = CM_NO_CLIENT;
            } else {
               return_status = CM_SUCCESS;
               i--;
            }
         }
      }

      /* display any message created during each shutdown */
      cm_msg_flush_buffer();
   }

   return return_status;
}

/********************************************************************/
/**
Check if a MIDAS client exists in current experiment
@param    name            Client name
@param    bUnique         If true, look for the exact client name.
                          If false, look for namexxx where xxx is
                          a any number
@return   CM_SUCCESS, CM_NO_CLIENT
*/
INT cm_exist(const char *name, BOOL bUnique)
{
   INT status, i, size;
   HNDLE hDB, hKeyClient, hKey, hSubkey;
   char client_name[NAME_LENGTH];

   if (rpc_is_remote())
      return rpc_call(RPC_CM_EXIST, name, bUnique);

   cm_get_experiment_database(&hDB, &hKeyClient);

   status = db_find_key(hDB, 0, "System/Clients", &hKey);
   if (status != DB_SUCCESS)
      return DB_NO_KEY;

   db_lock_database(hDB);

   /* loop over all clients */
   for (i = 0;; i++) {
      status = db_enum_key(hDB, hKey, i, &hSubkey);
      if (status == DB_NO_MORE_SUBKEYS)
         break;

      if (hSubkey == hKeyClient)
         continue;

      if (status == DB_SUCCESS) {
         /* get client name */
         size = sizeof(client_name);
         status = db_get_value(hDB, hSubkey, "Name", client_name, &size, TID_STRING, FALSE);

         if (status != DB_SUCCESS) {
            //fprintf(stderr, "cm_exist: name %s, i=%d, hSubkey=%d, status %d, client_name %s, my name %s\n", name, i, hSubkey, status, client_name, _client_name);
            continue;
         }

         if (equal_ustring(client_name, name)) {
            db_unlock_database(hDB);
            return CM_SUCCESS;
         }

         if (!bUnique) {
            client_name[strlen(name)] = 0;      /* strip number */
            if (equal_ustring(client_name, name)) {
               db_unlock_database(hDB);
               return CM_SUCCESS;
            }
         }
      }
   }

   db_unlock_database(hDB);

   return CM_NO_CLIENT;
}

/********************************************************************/
/**
Remove hanging clients independent of their watchdog
           timeout.

Since this function does not obey the client watchdog
timeout, it should be only called to remove clients which
have their watchdog checking turned off or which are
known to be dead. The normal client removement is done
via cm_watchdog().

Currently (Sept. 02) there are two applications for that:
-# The ODBEdit command "cleanup", which can be used to
remove clients which have their watchdog checking off,
like the analyzer started with the "-d" flag for a
debugging session.
-# The frontend init code to remove previous frontends.
This can be helpful if a frontend dies. Normally,
one would have to wait 60 sec. for a crashed frontend
to be removed. Only then one can start again the
frontend. Since the frontend init code contains a
call to cm_cleanup(<frontend_name>), one can restart
a frontend immediately.

Added ignore_timeout on Nov.03. A logger might have an
increased tiemout of up to 60 sec. because of tape
operations. If ignore_timeout is FALSE, the logger is
then not killed if its inactivity is less than 60 sec.,
while in the previous implementation it was always
killed after 2*WATCHDOG_INTERVAL.
@param    client_name      Client name, if zero check all clients
@param    ignore_timeout   If TRUE, ignore a possible increased
                           timeout defined by each client.
@return   CM_SUCCESS
*/
INT cm_cleanup(const char *client_name, BOOL ignore_timeout)
{
   if (rpc_is_remote())
      return rpc_call(RPC_CM_CLEANUP, client_name);

#ifdef LOCAL_ROUTINES
   {
      BUFFER_HEADER *pheader = NULL;
      BUFFER_CLIENT *pbclient;
      DATABASE_HEADER *pdbheader;
      DATABASE_CLIENT *pdbclient;
      KEY *pkey;
      INT client_pid;
      INT i, j, k, status, nc;
      BOOL bDeleted;
      char str[256];
      DWORD interval;
      DWORD now = ss_millitime();

      /* check buffers */
      for (i = 0; i < _buffer_entries; i++)
         if (_buffer[i].attached) {
            int idx;
            /* update the last_activity entry to show that we are alive */
            pheader = _buffer[i].buffer_header;
            pbclient = pheader->client;
            idx = bm_validate_client_index(&_buffer[i], FALSE);
            if (idx >= 0)
               pbclient[idx].last_activity = ss_millitime();

            /* now check other clients */
            for (j = 0; j < pheader->max_client_index; j++, pbclient++)
               if (j != _buffer[i].client_index && pbclient->pid &&
                   (client_name == NULL || client_name[0] == 0
                    || strncmp(pbclient->name, client_name, strlen(client_name)) == 0)) {
                  if (ignore_timeout)
                     interval = 2 * WATCHDOG_INTERVAL;
                  else
                     interval = pbclient->watchdog_timeout;

                  /* If client process has no activity, clear its buffer entry. */
                  if (interval > 0
                      && now > pbclient->last_activity && now - pbclient->last_activity > interval) {

                     bm_lock_buffer(i + 1);

                     str[0] = 0;

                     /* now make again the check with the buffer locked */
                     if (interval > 0
                         && now > pbclient->last_activity && now - pbclient->last_activity > interval) {
                        sprintf(str,
                                "Client \'%s\' on \'%s\' removed by cm_cleanup (idle %1.1lfs,TO %1.0lfs)",
                                pbclient->name, pheader->name,
                                (ss_millitime() - pbclient->last_activity) / 1000.0, interval / 1000.0);

                        bm_remove_client_locked(pheader, j);
                     }

                     bm_unlock_buffer(i + 1);

                     /* display info message after unlocking buffer */
                     if (str[0])
                        cm_msg(MINFO, "cm_cleanup", str);

                     /* go again through whole list */
                     j = 0;
                  }
               }
         }

      /* check online databases */
      for (i = 0; i < _database_entries; i++)
         if (_database[i].attached) {
            /* update the last_activity entry to show that we are alive */

            db_lock_database(i + 1);

            pdbheader = _database[i].database_header;
            pdbclient = pdbheader->client;
            pdbclient[_database[i].client_index].last_activity = ss_millitime();

            /* now check other clients */
            for (j = 0; j < pdbheader->max_client_index; j++, pdbclient++)
               if (j != _database[i].client_index && pdbclient->pid &&
                   (client_name == NULL || client_name[0] == 0
                    || strncmp(pdbclient->name, client_name, strlen(client_name)) == 0)) {
                  client_pid = pdbclient->pid;
                  if (ignore_timeout)
                     interval = 2 * WATCHDOG_INTERVAL;
                  else
                     interval = pdbclient->watchdog_timeout;

                  /* If client process has no activity, clear its buffer entry. */

                  if (interval > 0 && ss_millitime() - pdbclient->last_activity > interval) {
                     bDeleted = FALSE;

                     /* now make again the check with the buffer locked */
                     if (interval > 0 && ss_millitime() - pdbclient->last_activity > interval) {
                        cm_msg(MINFO, "cm_cleanup", "Client \'%s\' on \'%s\' removed by cm_cleanup (idle %1.1lfs,TO %1.0lfs)",
                               pdbclient->name, pdbheader->name,
                               (ss_millitime() - pdbclient->last_activity) / 1000.0, interval / 1000.0);

                        /* decrement notify_count for open records and clear exclusive mode */
                        for (k = 0; k < pdbclient->max_index; k++)
                           if (pdbclient->open_record[k].handle) {
                              pkey = (KEY *) ((char *) pdbheader + pdbclient->open_record[k].handle);
                              if (pkey->notify_count > 0)
                                 pkey->notify_count--;

                              if (pdbclient->open_record[k].access_mode & MODE_WRITE)
                                 db_set_mode(i + 1, pdbclient->open_record[k].handle, (WORD) (pkey->access_mode & ~MODE_EXCLUSIVE), 2);
                           }

                        /* clear entry from client structure in buffer header */
                        memset(&(pdbheader->client[j]), 0, sizeof(DATABASE_CLIENT));

                        /* calculate new max_client_index entry */
                        for (k = MAX_CLIENTS - 1; k >= 0; k--)
                           if (pdbheader->client[k].pid != 0)
                              break;
                        pdbheader->max_client_index = k + 1;

                        /* count new number of clients */
                        for (k = MAX_CLIENTS - 1, nc = 0; k >= 0; k--)
                           if (pheader->client[k].pid != 0)
                              nc++;
                        pdbheader->num_clients = nc;

                        bDeleted = TRUE;
                     }


                     /* delete client entry after unlocking db */
                     if (bDeleted) {

                        status = cm_delete_client_info(i + 1, client_pid);
                        if (status != CM_SUCCESS)
                           cm_msg(MERROR, "cm_cleanup", "cannot delete client info, status %d", status);

                        pdbheader = _database[i].database_header;
                        pdbclient = pdbheader->client;

                        /* go again though whole list */
                        j = 0;
                     }
                  }
               }

            db_unlock_database(i + 1);
         }

   }
#endif                          /* LOCAL_ROUTINES */

   return CM_SUCCESS;
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************/
INT bm_get_buffer_info(INT buffer_handle, BUFFER_HEADER * buffer_header)
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
      return rpc_call(RPC_BM_GET_BUFFER_INFO, buffer_handle, buffer_header);

#ifdef LOCAL_ROUTINES

   if (buffer_handle > _buffer_entries || buffer_handle <= 0) {
      cm_msg(MERROR, "bm_get_buffer_info", "invalid buffer handle %d", buffer_handle);
      return BM_INVALID_HANDLE;
   }

   if (!_buffer[buffer_handle - 1].attached) {
      cm_msg(MERROR, "bm_get_buffer_info", "invalid buffer handle %d", buffer_handle);
      return BM_INVALID_HANDLE;
   }

   bm_lock_buffer(buffer_handle);

   memcpy(buffer_header, _buffer[buffer_handle - 1].buffer_header, sizeof(BUFFER_HEADER));

   bm_unlock_buffer(buffer_handle);

#endif                          /* LOCAL_ROUTINES */

   return BM_SUCCESS;
}

/********************************************************************/
INT bm_get_buffer_level(INT buffer_handle, INT * n_bytes)
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
      return rpc_call(RPC_BM_GET_BUFFER_LEVEL, buffer_handle, n_bytes);

#ifdef LOCAL_ROUTINES
   {
      BUFFER *pbuf;
      BUFFER_HEADER *pheader;
      BUFFER_CLIENT *pclient;

      if (buffer_handle > _buffer_entries || buffer_handle <= 0) {
         cm_msg(MERROR, "bm_get_buffer_level", "invalid buffer handle %d", buffer_handle);
         return BM_INVALID_HANDLE;
      }

      pbuf = &_buffer[buffer_handle - 1];
      pheader = pbuf->buffer_header;

      if (!pbuf->attached) {
         cm_msg(MERROR, "bm_get_buffer_level", "invalid buffer handle %d", buffer_handle);
         return BM_INVALID_HANDLE;
      }

      bm_lock_buffer(buffer_handle);

      pclient = &(pheader->client[bm_validate_client_index(pbuf, TRUE)]);

      *n_bytes = pheader->write_pointer - pclient->read_pointer;
      if (*n_bytes < 0)
         *n_bytes += pheader->size;

      bm_unlock_buffer(buffer_handle);

      /* add bytes in cache */
      if (pbuf->read_cache_wp > pbuf->read_cache_rp)
         *n_bytes += pbuf->read_cache_wp - pbuf->read_cache_rp;
   }
#endif                          /* LOCAL_ROUTINES */

   return BM_SUCCESS;
}



#ifdef LOCAL_ROUTINES

/********************************************************************/
INT bm_lock_buffer(INT buffer_handle)
/********************************************************************\

  Routine: bm_lock_buffer

  Purpose: Lock a buffer for exclusive access via system semaphore calls.

  Input:
    INT    bufer_handle     Handle to the buffer to lock
  Output:
    none

  Function value:
    BM_SUCCESS              Successful completion
    BM_INVALID_HANDLE       Buffer handle is invalid

\********************************************************************/
{
   int status;

   if (buffer_handle > _buffer_entries || buffer_handle <= 0) {
      cm_msg(MERROR, "bm_lock_buffer", "invalid buffer handle %d", buffer_handle);
      return BM_INVALID_HANDLE;
   }

   status = ss_semaphore_wait_for(_buffer[buffer_handle - 1].semaphore, 5 * 60 * 1000);

   if (status != SS_SUCCESS) {
      cm_msg(MERROR, "bm_lock_buffer",
             "Cannot lock buffer handle %d, ss_semaphore_wait_for() status %d, aborting...", buffer_handle, status);
      abort();
      return BM_INVALID_HANDLE;
   }

   return BM_SUCCESS;
}

/********************************************************************/
INT bm_unlock_buffer(INT buffer_handle)
/********************************************************************\

  Routine: bm_unlock_buffer

  Purpose: Unlock a buffer via system semaphore calls.

  Input:
    INT    bufer_handle     Handle to the buffer to lock
  Output:
    none

  Function value:
    BM_SUCCESS              Successful completion
    BM_INVALID_HANDLE       Buffer handle is invalid

\********************************************************************/
{
   if (buffer_handle > _buffer_entries || buffer_handle <= 0) {
      cm_msg(MERROR, "bm_unlock_buffer", "invalid buffer handle %d", buffer_handle);
      return BM_INVALID_HANDLE;
   }

   ss_semaphore_release(_buffer[buffer_handle - 1].semaphore);
   return BM_SUCCESS;
}

#endif                          /* LOCAL_ROUTINES */

/********************************************************************/
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

   if (buffer_handle > _buffer_entries || buffer_handle <= 0) {
      cm_msg(MERROR, "bm_init_buffer_counters", "invalid buffer handle %d", buffer_handle);
      return BM_INVALID_HANDLE;
   }

   if (!_buffer[buffer_handle - 1].attached) {
      cm_msg(MERROR, "bm_init_buffer_counters", "invalid buffer handle %d", buffer_handle);
      return BM_INVALID_HANDLE;
   }

   _buffer[buffer_handle - 1].buffer_header->num_in_events = 0;
   _buffer[buffer_handle - 1].buffer_header->num_out_events = 0;

#endif                          /* LOCAL_ROUTINES */

   return BM_SUCCESS;
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/**dox***************************************************************/
                                                                                                                               /** @} *//* end of cmfunctionc */

/**dox***************************************************************/
/** @addtogroup bmfunctionc
 *
 *  @{  */

/********************************************************************/
/**
Modifies buffer cache size.
Without a buffer cache, events are copied to/from the shared
memory event by event.

To protect processed from accessing the shared memory simultaneously,
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
@param buffer_handle buffer handle obtained via bm_open_buffer()
@param read_size cache size for reading events in bytes, zero for no cache
@param write_size cache size for writing events in bytes, zero for no cache
@return BM_SUCCESS, BM_INVALID_HANDLE, BM_NO_MEMORY, BM_INVALID_PARAM
*/
INT bm_set_cache_size(INT buffer_handle, INT read_size, INT write_size)
/*------------------------------------------------------------------*/
{
   if (rpc_is_remote())
      return rpc_call(RPC_BM_SET_CACHE_SIZE, buffer_handle, read_size, write_size);

#ifdef LOCAL_ROUTINES
   {
      BUFFER *pbuf;

      if (buffer_handle > _buffer_entries || buffer_handle <= 0) {
         cm_msg(MERROR, "bm_set_cache_size", "invalid buffer handle %d", buffer_handle);
         return BM_INVALID_HANDLE;
      }

      if (!_buffer[buffer_handle - 1].attached) {
         cm_msg(MERROR, "bm_set_cache_size", "invalid buffer handle %d", buffer_handle);
         return BM_INVALID_HANDLE;
      }

      if (read_size < 0 || read_size > 1E6) {
         cm_msg(MERROR, "bm_set_cache_size", "invalid read chache size");
         return BM_INVALID_PARAM;
      }

      if (write_size < 0 || write_size > 1E6) {
         cm_msg(MERROR, "bm_set_cache_size", "invalid write chache size");
         return BM_INVALID_PARAM;
      }

      /* manage read cache */
      pbuf = &_buffer[buffer_handle - 1];

      if (pbuf->read_cache_size > 0) {
         M_FREE(pbuf->read_cache);
         pbuf->read_cache = NULL;
      }

      if (read_size > 0) {
         pbuf->read_cache = (char *) M_MALLOC(read_size);
         if (pbuf->read_cache == NULL) {
            cm_msg(MERROR, "bm_set_cache_size", "not enough memory to allocate cache buffer");
            return BM_NO_MEMORY;
         }
      }

      pbuf->read_cache_size = read_size;
      pbuf->read_cache_rp = pbuf->read_cache_wp = 0;

      /* manage write cache */
      if (pbuf->write_cache_size > 0) {
         M_FREE(pbuf->write_cache);
         pbuf->write_cache = NULL;
      }

      if (write_size > 0) {
         pbuf->write_cache = (char *) M_MALLOC(write_size);
         if (pbuf->write_cache == NULL) {
            cm_msg(MERROR, "bm_set_cache_size", "not enough memory to allocate cache buffer");
            return BM_NO_MEMORY;
         }
      }

      pbuf->write_cache_size = write_size;
      pbuf->write_cache_rp = pbuf->write_cache_wp = 0;

   }
#endif                          /* LOCAL_ROUTINES */

   return BM_SUCCESS;
}

/********************************************************************/
/**
Compose a Midas event header.
An event header can usually be set-up manually or
through this routine. If the data size of the event is not known when
the header is composed, it can be set later with event_header->data-size = <...>
Following structure is created at the beginning of an event
\code
typedef struct {
 short int     event_id;
 short int     trigger_mask;
 DWORD         serial_number;
 DWORD         time_stamp;
 DWORD         data_size;
} EVENT_HEADER;

char event[1000];
 bm_compose_event((EVENT_HEADER *)event, 1, 0, 100, 1);
 *(event+sizeof(EVENT_HEADER)) = <...>
\endcode
@param event_header pointer to the event header
@param event_id event ID of the event
@param trigger_mask trigger mask of the event
@param size size if the data part of the event in bytes
@param serial serial number
@return BM_SUCCESS
*/
INT bm_compose_event(EVENT_HEADER * event_header, short int event_id, short int trigger_mask, DWORD size,
                     DWORD serial)
{
   event_header->event_id = event_id;
   event_header->trigger_mask = trigger_mask;
   event_header->data_size = size;
   event_header->time_stamp = ss_time();
   event_header->serial_number = serial;

   return BM_SUCCESS;
}


/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************/
INT bm_add_event_request(INT buffer_handle, short int event_id,
                         short int trigger_mask,
                         INT sampling_type, void (*func) (HNDLE, HNDLE, EVENT_HEADER *, void *),
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

    INT          sampling_type  One of GET_ALL, GET_NONBLOCKING or GET_RECENT


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
    BM_INVALID_PARAM        GET_RECENT is used with non-zero cache size
    RPC_NET_ERROR           Network error

\********************************************************************/
{
   if (rpc_is_remote())
      return rpc_call(RPC_BM_ADD_EVENT_REQUEST, buffer_handle, event_id,
                      trigger_mask, sampling_type, (INT) (POINTER_T) func, request_id);

#ifdef LOCAL_ROUTINES
   {
      INT i;
      BUFFER_CLIENT *pclient;

      if (buffer_handle > _buffer_entries || buffer_handle <= 0) {
         cm_msg(MERROR, "bm_add_event_request", "invalid buffer handle %d", buffer_handle);
         return BM_INVALID_HANDLE;
      }

      if (!_buffer[buffer_handle - 1].attached) {
         cm_msg(MERROR, "bm_add_event_request", "invalid buffer handle %d", buffer_handle);
         return BM_INVALID_HANDLE;
      }

      /* avoid callback/non callback requests */
      if (func == NULL && _buffer[buffer_handle - 1].callback) {
         cm_msg(MERROR, "bm_add_event_request", "mixing callback/non callback requests not possible");
         return BM_INVALID_MIXING;
      }

      /* do not allow GET_RECENT with nonzero cache size */
      if (sampling_type == GET_RECENT && _buffer[buffer_handle - 1].read_cache_size > 0)
         return BM_INVALID_PARAM;

      /* lock buffer */
      bm_lock_buffer(buffer_handle);

      /* get a pointer to the proper client structure */
      pclient = &(_buffer[buffer_handle - 1].buffer_header->
                  client[bm_validate_client_index(&_buffer[buffer_handle - 1], TRUE)]);

      /* look for a empty request entry */
      for (i = 0; i < MAX_EVENT_REQUESTS; i++)
         if (!pclient->event_request[i].valid)
            break;

      if (i == MAX_EVENT_REQUESTS) {
         bm_unlock_buffer(buffer_handle);
         return BM_NO_MEMORY;
      }

      /* setup event_request structure */
      pclient->event_request[i].id = request_id;
      pclient->event_request[i].valid = TRUE;
      pclient->event_request[i].event_id = event_id;
      pclient->event_request[i].trigger_mask = trigger_mask;
      pclient->event_request[i].sampling_type = sampling_type;

      pclient->all_flag = pclient->all_flag || (sampling_type & GET_ALL);

      /* set callback flag in buffer structure */
      if (func != NULL)
         _buffer[buffer_handle - 1].callback = TRUE;

      /*
         Save the index of the last request in the list so that later only the
         requests 0..max_request_index-1 have to be searched through.
       */

      if (i + 1 > pclient->max_request_index)
         pclient->max_request_index = i + 1;

      bm_unlock_buffer(buffer_handle);
   }
#endif                          /* LOCAL_ROUTINES */

   return BM_SUCCESS;
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Place an event request based on certain characteristics.
Multiple event requests can be placed for each buffer, which
are later identified by their request ID. They can contain different callback
routines. Example see bm_open_buffer() and bm_receive_event()
@param buffer_handle buffer handle obtained via bm_open_buffer()
@param event_id event ID for requested events. Use EVENTID_ALL
to receive events with any ID.
@param trigger_mask trigger mask for requested events.
The requested events must have at least one bit in its
trigger mask common with the requested trigger mask. Use TRIGGER_ALL to
receive events with any trigger mask.
@param sampling_type specifies how many events to receive.
A value of GET_ALL receives all events which
match the specified event ID and trigger mask. If the events are consumed slower
than produced, the producer is automatically slowed down. A value of GET_NONBLOCKING
receives as much events as possible without slowing down the producer. GET_ALL is
typically used by the logger, while GET_NONBLOCKING is typically used by analyzers.
@param request_id request ID returned by the function.
This ID is passed to the callback routine and must
be used in the bm_delete_request() routine.
@param func allback routine which gets called when an event of the
specified type is received.
@return BM_SUCCESS, BM_INVALID_HANDLE <br>
BM_NO_MEMORY  too many requests. The value MAX_EVENT_REQUESTS in midas.h
should be increased.
*/
INT bm_request_event(HNDLE buffer_handle, short int event_id,
                     short int trigger_mask,
                     INT sampling_type, HNDLE * request_id, void (*func) (HNDLE, HNDLE, EVENT_HEADER *,
                                                                          void *))
{
   INT idx, status;

   /* allocate new space for the local request list */
   if (_request_list_entries == 0) {
      _request_list = (REQUEST_LIST *) M_MALLOC(sizeof(REQUEST_LIST));
      memset(_request_list, 0, sizeof(REQUEST_LIST));
      if (_request_list == NULL) {
         cm_msg(MERROR, "bm_request_event", "not enough memory to allocate request list buffer");
         return BM_NO_MEMORY;
      }

      _request_list_entries = 1;
      idx = 0;
   } else {
      /* check for a deleted entry */
      for (idx = 0; idx < _request_list_entries; idx++)
         if (!_request_list[idx].buffer_handle)
            break;

      /* if not found, create new one */
      if (idx == _request_list_entries) {
         _request_list =
             (REQUEST_LIST *) realloc(_request_list, sizeof(REQUEST_LIST) * (_request_list_entries + 1));
         if (_request_list == NULL) {
            cm_msg(MERROR, "bm_request_event", "not enough memory to allocate request list buffer");
            return BM_NO_MEMORY;
         }

         memset(&_request_list[_request_list_entries], 0, sizeof(REQUEST_LIST));

         _request_list_entries++;
      }
   }

   /* initialize request list */
   _request_list[idx].buffer_handle = buffer_handle;
   _request_list[idx].event_id = event_id;
   _request_list[idx].trigger_mask = trigger_mask;
   _request_list[idx].dispatcher = func;

   *request_id = idx;

   /* add request in buffer structure */
   status = bm_add_event_request(buffer_handle, event_id, trigger_mask, sampling_type, func, idx);
   if (status != BM_SUCCESS)
      return status;

   return BM_SUCCESS;
}

/********************************************************************/
/**
Delete a previously placed request for a specific event
type in the client structure of the buffer refereced by buffer_handle.
@param buffer_handle  Handle to the buffer where the re-
                                quest should be placed in
@param request_id     Request id returned by bm_request_event
@return BM_SUCCESS, BM_INVALID_HANDLE, BM_NOT_FOUND, RPC_NET_ERROR
*/
INT bm_remove_event_request(INT buffer_handle, INT request_id)
{
   if (rpc_is_remote())
      return rpc_call(RPC_BM_REMOVE_EVENT_REQUEST, buffer_handle, request_id);

#ifdef LOCAL_ROUTINES
   {
      INT i, deleted;
      BUFFER_CLIENT *pclient;

      if (buffer_handle > _buffer_entries || buffer_handle <= 0) {
         cm_msg(MERROR, "bm_remove_event_request", "invalid buffer handle %d", buffer_handle);
         return BM_INVALID_HANDLE;
      }

      if (!_buffer[buffer_handle - 1].attached) {
         cm_msg(MERROR, "bm_remove_event_request", "invalid buffer handle %d", buffer_handle);
         return BM_INVALID_HANDLE;
      }

      /* lock buffer */
      bm_lock_buffer(buffer_handle);

      /* get a pointer to the proper client structure */
      pclient = &(_buffer[buffer_handle - 1].buffer_header->
                  client[bm_validate_client_index(&_buffer[buffer_handle - 1], TRUE)]);

      /* check all requests and set to zero if matching */
      for (i = 0, deleted = 0; i < pclient->max_request_index; i++)
         if (pclient->event_request[i].valid && pclient->event_request[i].id == request_id) {
            memset(&pclient->event_request[i], 0, sizeof(EVENT_REQUEST));
            deleted++;
         }

      /* calculate new max_request_index entry */
      for (i = MAX_EVENT_REQUESTS - 1; i >= 0; i--)
         if (pclient->event_request[i].valid)
            break;

      pclient->max_request_index = i + 1;

      /* caluclate new all_flag */
      pclient->all_flag = FALSE;

      for (i = 0; i < pclient->max_request_index; i++)
         if (pclient->event_request[i].valid && (pclient->event_request[i].sampling_type & GET_ALL)) {
            pclient->all_flag = TRUE;
            break;
         }

      bm_unlock_buffer(buffer_handle);

      if (!deleted)
         return BM_NOT_FOUND;
   }
#endif                          /* LOCAL_ROUTINES */

   return BM_SUCCESS;
}

/********************************************************************/
/**
Deletes an event request previously done with bm_request_event().
When an event request gets deleted, events of that requested type are
not received any more. When a buffer is closed via bm_close_buffer(), all
event requests from that buffer are deleted automatically
@param request_id request identifier given by bm_request_event()
@return BM_SUCCESS, BM_INVALID_HANDLE
*/
INT bm_delete_request(INT request_id)
{
   if (request_id < 0 || request_id >= _request_list_entries)
      return BM_INVALID_HANDLE;

   /* remove request entry from buffer */
   bm_remove_event_request(_request_list[request_id].buffer_handle, request_id);

   memset(&_request_list[request_id], 0, sizeof(REQUEST_LIST));

   return BM_SUCCESS;
}

#if 0                           // currently not used
static void bm_show_pointers(const BUFFER_HEADER * pheader)
{
   int i;
   const BUFFER_CLIENT *pclient;

   pclient = pheader->client;

   printf("buffer \'%s\', rptr: %d, wptr: %d, size: %d\n", pheader->name, pheader->read_pointer,
          pheader->write_pointer, pheader->size);
   for (i = 0; i < pheader->max_client_index; i++)
      if (pclient[i].pid) {
         printf("pointers: client %d \'%s\', rptr %d\n", i, pclient[i].name, pclient[i].read_pointer);
      }

   printf("done\n");
}
#endif

static void bm_validate_client_pointers(BUFFER_HEADER * pheader, BUFFER_CLIENT * pclient)
{
   assert(pheader->read_pointer >= 0 && pheader->read_pointer <= pheader->size);
   assert(pclient->read_pointer >= 0 && pclient->read_pointer <= pheader->size);

   if (pheader->read_pointer <= pheader->write_pointer) {

      if (pclient->read_pointer < pheader->read_pointer) {
         cm_msg(MINFO, "bm_validate_client_pointers",
                "Corrected read pointer for client \'%s\' on buffer \'%s\' from %d to %d", pclient->name,
                pheader->name, pclient->read_pointer, pheader->read_pointer);

         pclient->read_pointer = pheader->read_pointer;
      }

      if (pclient->read_pointer > pheader->write_pointer) {
         cm_msg(MINFO, "bm_validate_client_pointers",
                "Corrected read pointer for client \'%s\' on buffer \'%s\' from %d to %d", pclient->name,
                pheader->name, pclient->read_pointer, pheader->write_pointer);

         pclient->read_pointer = pheader->write_pointer;
      }

   } else {

      if (pclient->read_pointer < 0) {
         cm_msg(MINFO, "bm_validate_client_pointers",
                "Corrected read pointer for client \'%s\' on buffer \'%s\' from %d to %d", pclient->name,
                pheader->name, pclient->read_pointer, pheader->read_pointer);

         pclient->read_pointer = pheader->read_pointer;
      }

      if (pclient->read_pointer >= pheader->size) {
         cm_msg(MINFO, "bm_validate_client_pointers",
                "Corrected read pointer for client \'%s\' on buffer \'%s\' from %d to %d", pclient->name,
                pheader->name, pclient->read_pointer, pheader->read_pointer);

         pclient->read_pointer = pheader->read_pointer;
      }

      if (pclient->read_pointer > pheader->write_pointer && pclient->read_pointer < pheader->read_pointer) {
         cm_msg(MINFO, "bm_validate_client_pointers",
                "Corrected read pointer for client \'%s\' on buffer \'%s\' from %d to %d", pclient->name,
                pheader->name, pclient->read_pointer, pheader->read_pointer);

         pclient->read_pointer = pheader->read_pointer;
      }
   }
}

#if 0                           // currently not used
static void bm_validate_pointers(BUFFER_HEADER * pheader)
{
   BUFFER_CLIENT *pclient = pheader->client;
   int i;

   for (i = 0; i < pheader->max_client_index; i++)
      if (pclient[i].pid) {
         bm_validate_client_pointers(pheader, &pclient[i]);
      }
}
#endif

static BOOL bm_update_read_pointer(const char *caller_name, BUFFER_HEADER * pheader)
{
   BOOL did_move;
   int i;
   int min_rp;
   BUFFER_CLIENT *pclient;

   assert(caller_name);
   pclient = pheader->client;

   /* calculate global read pointer as "minimum" of client read pointers */
   min_rp = pheader->write_pointer;

   for (i = 0; i < pheader->max_client_index; i++)
      if (pclient[i].pid) {
#ifdef DEBUG_MSG
         cm_msg(MDEBUG, caller_name, "bm_update_read_pointer: client %d rp=%d", i, pclient[i].read_pointer);
#endif
         bm_validate_client_pointers(pheader, &pclient[i]);

         if (pheader->read_pointer <= pheader->write_pointer) {
            if (pclient[i].read_pointer < min_rp)
               min_rp = pclient[i].read_pointer;
         } else {
            if (pclient[i].read_pointer <= pheader->write_pointer) {
               if (pclient[i].read_pointer < min_rp)
                  min_rp = pclient[i].read_pointer;
            } else {
               int xptr = pclient[i].read_pointer - pheader->size;
               if (xptr < min_rp)
                  min_rp = xptr;
            }
         }
      }

   if (min_rp < 0)
      min_rp += pheader->size;

   assert(min_rp >= 0);
   assert(min_rp < pheader->size);

#ifdef DEBUG_MSG
   if (min_rp == pheader->read_pointer)
      cm_msg(MDEBUG, caller_name, "bm_update_read_pointer -> wp=%d", pheader->write_pointer);
   else
      cm_msg(MDEBUG, caller_name, "bm_update_read_pointer -> wp=%d, rp %d -> %d, size=%d",
             pheader->write_pointer, pheader->read_pointer, min_rp, pheader->size);
#endif

   did_move = (pheader->read_pointer != min_rp);

   pheader->read_pointer = min_rp;

   return did_move;
}

static void bm_wakeup_producers(const BUFFER_HEADER * pheader, const BUFFER_CLIENT * pc)
{
   int i;
   int size;
   const BUFFER_CLIENT *pctmp = pheader->client;

   /*
      If read pointer has been changed, it may have freed up some space
      for waiting producers. So check if free space is now more than 50%
      of the buffer size and wake waiting producers.
    */

   size = pc->read_pointer - pheader->write_pointer;
   if (size <= 0)
      size += pheader->size;

   if (size >= pheader->size * 0.5)
      for (i = 0; i < pheader->max_client_index; i++, pctmp++)
	if (pctmp->pid)
           if (pctmp->write_wait < size) {
#ifdef DEBUG_MSG
            cm_msg(MDEBUG, "Receive wake: rp=%d, wp=%d, level=%1.1lf",
                   pheader->read_pointer, pheader->write_pointer, 100 - 100.0 * size / pheader->size);
#endif
            ss_resume(pctmp->port, "B  ");
            }
}

static void bm_dispatch_event(int buffer_handle, EVENT_HEADER * pevent)
{
   int i;

   /* call dispatcher */
   for (i = 0; i < _request_list_entries; i++)
      if (_request_list[i].buffer_handle == buffer_handle &&
          bm_match_event(_request_list[i].event_id, _request_list[i].trigger_mask, pevent)) {
         /* if event is fragmented, call defragmenter */
         if ((pevent->event_id & 0xF000) == EVENTID_FRAG1 || (pevent->event_id & 0xF000) == EVENTID_FRAG)
            bm_defragment_event(buffer_handle, i, pevent, (void *) (pevent + 1), _request_list[i].dispatcher);
         else
            _request_list[i].dispatcher(buffer_handle, i, pevent, (void *) (pevent + 1));
      }
}

static void bm_dispatch_from_cache(BUFFER * pbuf, int buffer_handle)
{
   EVENT_HEADER *pevent;
   int size;

   pevent = (EVENT_HEADER *) (pbuf->read_cache + pbuf->read_cache_rp);
   size = pevent->data_size + sizeof(EVENT_HEADER);

   /* correct size for DWORD boundary */
   size = ALIGN8(size);

   /* increment read pointer */
   pbuf->read_cache_rp += size;

   if (pbuf->read_cache_rp == pbuf->read_cache_wp)
      pbuf->read_cache_rp = pbuf->read_cache_wp = 0;

   bm_dispatch_event(buffer_handle, pevent);
}

static void bm_convert_event_header(EVENT_HEADER * pevent, int convert_flags)
{
   /* now convert event header */
   if (convert_flags) {
      rpc_convert_single(&pevent->event_id, TID_SHORT, RPC_OUTGOING, convert_flags);
      rpc_convert_single(&pevent->trigger_mask, TID_SHORT, RPC_OUTGOING, convert_flags);
      rpc_convert_single(&pevent->serial_number, TID_DWORD, RPC_OUTGOING, convert_flags);
      rpc_convert_single(&pevent->time_stamp, TID_DWORD, RPC_OUTGOING, convert_flags);
      rpc_convert_single(&pevent->data_size, TID_DWORD, RPC_OUTGOING, convert_flags);
   }
}

static int bm_copy_from_cache(BUFFER * pbuf, void *destination, int max_size, int *buf_size,
                              int convert_flags)
{
   int status;
   EVENT_HEADER *pevent;
   int size;

   pevent = (EVENT_HEADER *) (pbuf->read_cache + pbuf->read_cache_rp);
   size = pevent->data_size + sizeof(EVENT_HEADER);

   if (size > max_size) {
      memcpy(destination, pevent, max_size);
      cm_msg(MERROR, "bm_receive_event", "event size %d larger than buffer size %d", size, max_size);
      *buf_size = max_size;
      status = BM_TRUNCATED;
   } else {
      memcpy(destination, pevent, size);
      *buf_size = size;
      status = BM_SUCCESS;
   }

   bm_convert_event_header((EVENT_HEADER *) destination, convert_flags);

   /* correct size for DWORD boundary */
   size = ALIGN8(size);

   pbuf->read_cache_rp += size;

   if (pbuf->read_cache_rp == pbuf->read_cache_wp)
      pbuf->read_cache_rp = pbuf->read_cache_wp = 0;

   return status;
}

static int bm_read_cache_has_events(const BUFFER * pbuf)
{
   if (pbuf->read_cache_size == 0)
      return 0;

   if (pbuf->read_cache_rp == pbuf->read_cache_wp)
      return 0;

   return 1;
}

static int bm_wait_for_free_space(int buffer_handle, BUFFER * pbuf, int async_flag, int requested_space)
{
   int status;
   BUFFER_HEADER *pheader = pbuf->buffer_header;
   char *pdata = (char *) (pheader + 1);

   /* make sure the buffer never completely full:
    * read pointer and write pointer would coincide
    * and the code cannot tell if it means the
    * buffer is 100% full or 100% empty. It will explode
    * or lose events */
   requested_space += 100;

   if (requested_space >= pheader->size)
      return BM_NO_MEMORY;

   while (1) {

      BUFFER_CLIENT *pc;
      int n_blocking;
      int i;
      int size;
      int idx;

      /* check if enough space in buffer */

      size = pheader->read_pointer - pheader->write_pointer;
      if (size <= 0)
         size += pheader->size;

#if 0
      printf
          ("bm_send_event: buffer pointers: read: %d, write: %d, free space: %d, bufsize: %d, event size: %d\n",
           pheader->read_pointer, pheader->write_pointer, size, pheader->size, requested_space);
#endif

      if (requested_space < size)       /* note the '<' to avoid 100% filling */
         return BM_SUCCESS;

      /* if not enough space, find out who's blocking */
      n_blocking = 0;

      for (i = 0, pc = pheader->client; i < pheader->max_client_index; i++, pc++)
         if (pc->pid) {
            if (pc->read_pointer == pheader->read_pointer) {
               /*
                  First assume that the client with the "minimum" read pointer
                  is not really blocking due to a GET_ALL request.
                */
               BOOL blocking = FALSE;
               int blocking_request_id = 0;
               int j;

               /* check if this request blocks */

               EVENT_REQUEST *prequest = pc->event_request;
               EVENT_HEADER *pevent_test = (EVENT_HEADER *) (pdata + pc->read_pointer);

               assert(pc->read_pointer >= 0);
               assert(pc->read_pointer <= pheader->size);

               for (j = 0; j < pc->max_request_index; j++, prequest++)
                  if (prequest->valid
                      && bm_match_event(prequest->event_id, prequest->trigger_mask, pevent_test)) {
                     if (prequest->sampling_type & GET_ALL) {
                        blocking = TRUE;
                        blocking_request_id = prequest->id;
                        break;
                     }
                  }

               if (blocking) {
                  n_blocking++;

                  if (pc->read_wait) {
                     char str[80];
#ifdef DEBUG_MSG
                     cm_msg(MDEBUG, "Send wake: rp=%d, wp=%d", pheader->read_pointer, pheader->write_pointer);
#endif
                     sprintf(str, "B %s %d", pheader->name, blocking_request_id);
                     ss_resume(pc->port, str);
                  }

               } else {
                  /*
                     The blocking guy has no GET_ALL request for this event
                     -> shift its read pointer.
                   */

                  int new_read_pointer;
                  int increment =
                      sizeof(EVENT_HEADER) + ((EVENT_HEADER *) (pdata + pc->read_pointer))->data_size;

                  /* correct increment for DWORD boundary */
                  increment = ALIGN8(increment);

                  assert(increment > 0);
                  assert(increment <= pheader->size);

                  new_read_pointer = (pc->read_pointer + increment) % pheader->size;

                  if (new_read_pointer > pheader->size - (int) sizeof(EVENT_HEADER))
                     new_read_pointer = 0;

                  pc->read_pointer = new_read_pointer;
               }
            }
         }
      /* client loop */
      if (n_blocking == 0) {

         BOOL moved;
         /*
            calculate new global read pointer as "minimum" of
            client read pointers
          */

         moved = bm_update_read_pointer("bm_send_event", pheader);

         if (!moved) {
            cm_msg(MERROR, "bm_wait_for_free_space",
                   "BUG: read pointer did not move while waiting for %d bytes, bytes available: %d, buffer size: %d",
                   requested_space, size, pheader->size);
            return BM_NO_MEMORY;
         }

         continue;
      }

      /* at least one client is blocking */

      bm_unlock_buffer(buffer_handle);

      /* return now in ASYNC mode */
      if (async_flag)
         return BM_ASYNC_RETURN;

#ifdef DEBUG_MSG
      cm_msg(MDEBUG, "Send sleep: rp=%d, wp=%d, level=%1.1lf",
             pheader->read_pointer, pheader->write_pointer, 100 - 100.0 * size / pheader->size);
#endif

      /* signal other clients wait mode */
      idx = bm_validate_client_index(pbuf, FALSE);
      if (idx >= 0)
         pheader->client[idx].write_wait = requested_space;

      bm_cleanup("bm_wait_for_free_space", ss_millitime(), FALSE);

      status = ss_suspend(1000, MSG_BM);

      /* make sure we do sleep in this loop:
       * if we are the mserver receiving data on the event
       * socket and the data buffer is full, ss_suspend() will
       * never sleep: it will detect data on the event channel,
       * call rpc_server_receive() (recursively, we already *are* in
       * rpc_server_receive()) and return without sleeping. Result
       * is a busy loop waiting for free space in data buffer */
      if (status != SS_TIMEOUT)
         ss_sleep(10);

      /* validate client index: we could have been removed from the buffer */
      idx = bm_validate_client_index(pbuf, FALSE);
      if (idx >= 0)
         pheader->client[idx].write_wait = 0;
      else {
         cm_msg(MERROR, "bm_wait_for_free_space", "our client index is no longer valid, exiting...");
         status = SS_ABORT;
      }

      /* return if TCP connection broken */
      if (status == SS_ABORT)
         return SS_ABORT;

#ifdef DEBUG_MSG
      cm_msg(MDEBUG, "Send woke up: rp=%d, wp=%d, level=%1.1lf",
             pheader->read_pointer, pheader->write_pointer, 100 - 100.0 * size / pheader->size);
#endif

      bm_lock_buffer(buffer_handle);
   }
}

/********************************************************************/
/**
Sends an event to a buffer.
This function check if the buffer has enough space for the
event, then copies the event to the buffer in shared memory.
If clients have requests for the event, they are notified via an UDP packet.
\code
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
 bm_open_buffer(EVENT_BUFFER_NAME, 2*MAX_EVENT_SIZE, &hbuf);

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
\endcode
@param buffer_handle Buffer handle obtained via bm_open_buffer()
@param source Address of event buffer
@param buf_size Size of event including event header in bytes
@param async_flag Synchronous/asynchronous flag. If FALSE, the function
blocks if the buffer has not enough free space to receive the event.
If TRUE, the function returns immediately with a
value of BM_ASYNC_RETURN without writing the event to the buffer
@return BM_SUCCESS, BM_INVALID_HANDLE, BM_INVALID_PARAM<br>
BM_ASYNC_RETURN Routine called with async_flag == TRUE and
buffer has not enough space to receive event<br>
BM_NO_MEMORY   Event is too large for network buffer or event buffer.
One has to increase MAX_EVENT_SIZE in midas.h and
recompile.
*/
INT bm_send_event(INT buffer_handle, void *source, INT buf_size, INT async_flag)
{
   EVENT_HEADER *pevent;

   /* check if event size is invalid */
   if (buf_size < sizeof(EVENT_HEADER)) {
      cm_msg(MERROR, "bm_send_event", "event size (%d) it too small", buf_size);
      return BM_INVALID_PARAM;
   }

   /* check if event size defined in header matches buf_size */
   if (ALIGN8(buf_size) != (INT) ALIGN8(((EVENT_HEADER *) source)->data_size + sizeof(EVENT_HEADER))) {
      cm_msg(MERROR, "bm_send_event", "event size (%d) mismatch in header (%d)",
             ALIGN8(buf_size), (INT) ALIGN8(((EVENT_HEADER *) source)->data_size + sizeof(EVENT_HEADER)));
      return BM_INVALID_PARAM;
   }

   if (rpc_is_remote())
      return rpc_call(RPC_BM_SEND_EVENT, buffer_handle, source, buf_size, async_flag);

#ifdef LOCAL_ROUTINES
   {
      BUFFER *pbuf;
      BUFFER_HEADER *pheader;
      BUFFER_CLIENT *pclient;
      EVENT_REQUEST *prequest;
      INT i, j, size, total_size, status;
      INT my_client_index;
      INT old_write_pointer;
      INT num_requests_client;
      char *pdata;
      INT request_id;

      pbuf = &_buffer[buffer_handle - 1];

      if (buffer_handle > _buffer_entries || buffer_handle <= 0) {
         cm_msg(MERROR, "bm_send_event", "invalid buffer handle %d", buffer_handle);
         return BM_INVALID_HANDLE;
      }

      if (!pbuf->attached) {
         cm_msg(MERROR, "bm_send_event", "invalid buffer handle %d", buffer_handle);
         return BM_INVALID_HANDLE;
      }

      pevent = (EVENT_HEADER *) source;
      total_size = buf_size;

      /* round up total_size to next DWORD boundary */
      total_size = ALIGN8(total_size);

      /* look if there is space in the cache */
      if (pbuf->write_cache_size) {
         status = BM_SUCCESS;

         if (pbuf->write_cache_size - pbuf->write_cache_wp < total_size)
            status = bm_flush_cache(buffer_handle, async_flag);

         if (status != BM_SUCCESS)
            return status;

         if (total_size < pbuf->write_cache_size) {
            memcpy(pbuf->write_cache + pbuf->write_cache_wp, source, total_size);

            pbuf->write_cache_wp += total_size;
            return BM_SUCCESS;
         }
      }

      /* we come here only for events that are too big to fit into the cache */

      /* lock the buffer */
      bm_lock_buffer(buffer_handle);

      /* calculate some shorthands */
      pheader = pbuf->buffer_header;
      pdata = (char *) (pheader + 1);
      my_client_index = bm_validate_client_index(pbuf, TRUE);
      pclient = pheader->client;

      /* check if buffer is large enough */
      if (total_size >= pheader->size) {
         bm_unlock_buffer(buffer_handle);
         cm_msg(MERROR, "bm_send_event",
                "total event size (%d) larger than size (%d) of buffer \'%s\'", total_size, pheader->size, pheader->name);
         return BM_NO_MEMORY;
      }

      status = bm_wait_for_free_space(buffer_handle, pbuf, async_flag, total_size);
      if (status != BM_SUCCESS) {
         bm_unlock_buffer(buffer_handle);
         return status;
      }

      /* we have space, so let's copy the event */
      old_write_pointer = pheader->write_pointer;

      if (pheader->write_pointer + total_size <= pheader->size) {
         memcpy(pdata + pheader->write_pointer, pevent, total_size);
         pheader->write_pointer = (pheader->write_pointer + total_size) % pheader->size;
         if (pheader->write_pointer > pheader->size - (int) sizeof(EVENT_HEADER))
            pheader->write_pointer = 0;
      } else {
         /* split event */
         size = pheader->size - pheader->write_pointer;

         memcpy(pdata + pheader->write_pointer, pevent, size);
         memcpy(pdata, (char *) pevent + size, total_size - size);

         pheader->write_pointer = total_size - size;
      }

      /* write pointer was incremented, but there should
       * always be some free space in the buffer and the
       * write pointer should never cacth up to the read pointer:
       * the rest of the code gets confused this happens (buffer 100% full)
       * as it is write_pointer == read_pointer can be either
       * 100% full or 100% empty. My solution: never fill
       * the buffer to 100% */
      assert(pheader->write_pointer != pheader->read_pointer);

      /* check which clients have a request for this event */
      for (i = 0; i < pheader->max_client_index; i++)
         if (pclient[i].pid) {
            prequest = pclient[i].event_request;
            num_requests_client = 0;
            request_id = -1;

            for (j = 0; j < pclient[i].max_request_index; j++, prequest++)
               if (prequest->valid && bm_match_event(prequest->event_id, prequest->trigger_mask, pevent)) {
                  if (prequest->sampling_type & GET_ALL)
                     pclient[i].num_waiting_events++;

                  num_requests_client++;
                  request_id = prequest->id;
               }

            /* if that client has a request and is suspended, wake it up */
            if (num_requests_client && pclient[i].read_wait) {
               char str[80];
#ifdef DEBUG_MSG
               cm_msg(MDEBUG, "Send wake: rp=%d, wp=%d", pheader->read_pointer, pheader->write_pointer);
#endif
               sprintf(str, "B %s %d", pheader->name, request_id);
               ss_resume(pclient[i].port, str);
            }

            /* if that client has no request, shift its read pointer */
            if (num_requests_client == 0 && pclient[i].read_pointer == old_write_pointer)
               pclient[i].read_pointer = pheader->write_pointer;
         }

      /* shift read pointer of own client */
      if (pclient[my_client_index].read_pointer == old_write_pointer)
         pclient[my_client_index].read_pointer = pheader->write_pointer;

      /* calculate global read pointer as "minimum" of client read pointers */

      bm_update_read_pointer("bm_send_event", pheader);

      /* update statistics */
      pheader->num_in_events++;

      /* unlock the buffer */
      bm_unlock_buffer(buffer_handle);
   }
#endif                          /* LOCAL_ROUTINES */

   return BM_SUCCESS;
}

/********************************************************************/
/**
Empty write cache.
This function should be used if events in the write cache
should be visible to the consumers immediately. It should be called at the
end of each run, otherwise events could be kept in the write buffer and will
flow to the data of the next run.
@param buffer_handle Buffer handle obtained via bm_open_buffer()
@param async_flag Synchronous/asynchronous flag.
If FALSE, the function blocks if the buffer has not
enough free space to receive the full cache. If TRUE, the function returns
immediately with a value of BM_ASYNC_RETURN without writing the cache.
@return BM_SUCCESS, BM_INVALID_HANDLE<br>
BM_ASYNC_RETURN Routine called with async_flag == TRUE
and buffer has not enough space to receive cache<br>
BM_NO_MEMORY Event is too large for network buffer or event buffer.
One has to increase MAX_EVENT_SIZE in midas.h
and recompile.
*/
INT bm_flush_cache(INT buffer_handle, INT async_flag)
{
   if (rpc_is_remote())
      return rpc_call(RPC_BM_FLUSH_CACHE, buffer_handle, async_flag);

#ifdef LOCAL_ROUTINES
   {
      BUFFER *pbuf;
      BUFFER_HEADER *pheader;
      BUFFER_CLIENT *pclient;
      EVENT_HEADER *pevent;
      INT i, size, total_size, status;
      INT my_client_index;
      INT old_write_pointer;
      char *pdata;

      pbuf = &_buffer[buffer_handle - 1];

      if (buffer_handle > _buffer_entries || buffer_handle <= 0) {
         cm_msg(MERROR, "bm_flush_cache", "invalid buffer handle %d", buffer_handle);
         return BM_INVALID_HANDLE;
      }

      if (!pbuf->attached) {
         cm_msg(MERROR, "bm_flush_cache", "invalid buffer handle %d", buffer_handle);
         return BM_INVALID_HANDLE;
      }

      if (pbuf->write_cache_size == 0)
         return BM_SUCCESS;

      /* check if anything needs to be flushed */
      if (pbuf->write_cache_rp == pbuf->write_cache_wp)
         return BM_SUCCESS;

      /* lock the buffer */
      bm_lock_buffer(buffer_handle);

      /* calculate some shorthands */
      pheader = _buffer[buffer_handle - 1].buffer_header;
      pdata = (char *) (pheader + 1);
      my_client_index = bm_validate_client_index(pbuf, TRUE);
      pclient = pheader->client;
      pevent = (EVENT_HEADER *) (pbuf->write_cache + pbuf->write_cache_rp);

#ifdef DEBUG_MSG
      cm_msg(MDEBUG, "bm_flush_cache initial: rp=%d, wp=%d", pheader->read_pointer, pheader->write_pointer);
#endif

      status = bm_wait_for_free_space(buffer_handle, pbuf, async_flag, pbuf->write_cache_wp);
      if (status != BM_SUCCESS) {
         bm_unlock_buffer(buffer_handle);
         return status;
      }

      /* we have space, so let's copy the event */
      old_write_pointer = pheader->write_pointer;

#ifdef DEBUG_MSG
      cm_msg(MDEBUG, "bm_flush_cache: found space rp=%d, wp=%d", pheader->read_pointer,
             pheader->write_pointer);
#endif

      while (pbuf->write_cache_rp < pbuf->write_cache_wp) {
         /* loop over all events in cache */

         assert(pbuf->write_cache_rp >= 0);
         assert(pbuf->write_cache_rp < pbuf->write_cache_size);

         pevent = (EVENT_HEADER *) (pbuf->write_cache + pbuf->write_cache_rp);
         total_size = pevent->data_size + sizeof(EVENT_HEADER);

         assert(total_size > 0);
         assert(total_size <= pheader->size);

         /* correct size for DWORD boundary */
         total_size = ALIGN8(total_size);

         if (pheader->write_pointer + total_size <= pheader->size) {
            memcpy(pdata + pheader->write_pointer, pevent, total_size);
            pheader->write_pointer = (pheader->write_pointer + total_size) % pheader->size;
            if (pheader->write_pointer > pheader->size - (int) sizeof(EVENT_HEADER))
               pheader->write_pointer = 0;
         } else {
            /* split event */
            size = pheader->size - pheader->write_pointer;

            memcpy(pdata + pheader->write_pointer, pevent, size);
            memcpy(pdata, (char *) pevent + size, total_size - size);

            pheader->write_pointer = total_size - size;
         }

         /* see comment for the same code in bm_send_event().
          * We make sure the buffer is nevere 100% full */
         assert(pheader->write_pointer != pheader->read_pointer);

         /* this loop does not loop forever because write_cache_rp
          * is monotonously incremented here. write_cache_wp does
          * not change */

         pbuf->write_cache_rp += total_size;
      }

      pbuf->write_cache_rp = pbuf->write_cache_wp = 0;

      /* check which clients are waiting */
      for (i = 0; i < pheader->max_client_index; i++)
         if (pclient[i].pid && pclient[i].read_wait) {
            char str[80];
#ifdef DEBUG_MSG
            cm_msg(MDEBUG, "Send wake: rp=%d, wp=%d", pheader->read_pointer, pheader->write_pointer);
#endif
            sprintf(str, "B %s %d", pheader->name, -1);
            ss_resume(pclient[i].port, str);
         }

      /* shift read pointer of own client */
      if (pclient[my_client_index].read_pointer == old_write_pointer)
         pclient[my_client_index].read_pointer = pheader->write_pointer;

      /* calculate global read pointer as "minimum" of client read pointers */

      bm_update_read_pointer("bm_flush_cache", pheader);

      /* update statistics */
      pheader->num_in_events++;

      /* unlock the buffer */
      bm_unlock_buffer(buffer_handle);
   }
#endif                          /* LOCAL_ROUTINES */

   return BM_SUCCESS;
}

/********************************************************************/
/**
Receives events directly.
This function is an alternative way to receive events without
a main loop.

It can be used in analysis systems which actively receive events,
rather than using callbacks. A analysis package could for example contain its own
command line interface. A command
like "receive 1000 events" could make it necessary to call bm_receive_event()
1000 times in a row to receive these events and then return back to the
command line prompt.
The according bm_request_event() call contains NULL as the
callback routine to indicate that bm_receive_event() is called to receive
events.
\code
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
  bm_open_buffer(EVENT_BUFFER_NAME, 2*MAX_EVENT_SIZE, &hbuf);
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
\endcode
@param buffer_handle buffer handle
@param destination destination address where event is written to
@param buf_size size of destination buffer on input, size of event plus
header on return.
@param async_flag Synchronous/asynchronous flag. If FALSE, the function
blocks if no event is available. If TRUE, the function returns immediately
with a value of BM_ASYNC_RETURN without receiving any event.
@return BM_SUCCESS, BM_INVALID_HANDLE <br>
BM_TRUNCATED   The event is larger than the destination buffer and was
               therefore truncated <br>
BM_ASYNC_RETURN No event available
*/
INT bm_receive_event(INT buffer_handle, void *destination, INT * buf_size, INT async_flag)
{
   if (rpc_is_remote()) {
      int status, old_timeout = 0;

      status = resize_net_send_buffer("bm_receive_event", *buf_size);
      if (status != SUCCESS)
         return status;  

      if (!async_flag) {
         old_timeout = rpc_get_option(-1, RPC_OTIMEOUT);
         rpc_set_option(-1, RPC_OTIMEOUT, 0);
      }

      status = rpc_call(RPC_BM_RECEIVE_EVENT, buffer_handle, destination, buf_size, async_flag);

      if (!async_flag) {
         rpc_set_option(-1, RPC_OTIMEOUT, old_timeout);
      }

      return status;
   }
#ifdef LOCAL_ROUTINES
   {
      BUFFER *pbuf;
      BUFFER_HEADER *pheader;
      BUFFER_CLIENT *pclient, *pc;
      char *pdata;
      INT convert_flags;
      INT i, size, max_size;
      INT status = 0;
      INT my_client_index;
      BOOL cache_is_full = FALSE;
      BOOL use_event_buffer = FALSE;
      int cycle = 0;

      pbuf = &_buffer[buffer_handle - 1];

      if (buffer_handle > _buffer_entries || buffer_handle <= 0) {
         cm_msg(MERROR, "bm_receive_event", "invalid buffer handle %d", buffer_handle);
         return BM_INVALID_HANDLE;
      }

      if (!pbuf->attached) {
         cm_msg(MERROR, "bm_receive_event", "invalid buffer handle %d", buffer_handle);
         return BM_INVALID_HANDLE;
      }

      max_size = *buf_size;
      *buf_size = 0;

      if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_REMOTE)
         convert_flags = rpc_get_server_option(RPC_CONVERT_FLAGS);
      else
         convert_flags = 0;

      /* look if there is anything in the cache */
      if (bm_read_cache_has_events(pbuf))
         return bm_copy_from_cache(pbuf, destination, max_size, buf_size, convert_flags);

    LOOP:
      /* make sure we do bm_validate_client_index() after sleeping */

      /* calculate some shorthands */
      pheader = pbuf->buffer_header;
      pdata = (char *) (pheader + 1);
      my_client_index = bm_validate_client_index(pbuf, TRUE);
      pclient = pheader->client;
      pc = pheader->client + my_client_index;

      /* first do a quick check without locking the buffer */
      if (async_flag == ASYNC && pheader->write_pointer == pc->read_pointer)
         return BM_ASYNC_RETURN;

      /* lock the buffer */
      bm_lock_buffer(buffer_handle);

      while (pheader->write_pointer == pc->read_pointer) {

         bm_unlock_buffer(buffer_handle);

         /* return now in ASYNC mode */
         if (async_flag == ASYNC)
            return BM_ASYNC_RETURN;

         pc->read_wait = TRUE;

         /* check again pointers (may have moved in between) */
         if (pheader->write_pointer == pc->read_pointer) {
#ifdef DEBUG_MSG
            cm_msg(MDEBUG, "Receive sleep: grp=%d, rp=%d wp=%d",
                   pheader->read_pointer, pc->read_pointer, pheader->write_pointer);
#endif

            status = ss_suspend(1000, MSG_BM);

#ifdef DEBUG_MSG
            cm_msg(MDEBUG, "Receive woke up: rp=%d, wp=%d", pheader->read_pointer, pheader->write_pointer);
#endif

            /* return if TCP connection broken */
            if (status == SS_ABORT)
               return SS_ABORT;
         }

         pc->read_wait = FALSE;

         /* validate client_index: somebody may have disconnected us from the buffer */
         bm_validate_client_index(pbuf, TRUE);

         bm_lock_buffer(buffer_handle);
      }

      /* check if event at current read pointer matches a request */

      do {
         int new_read_pointer;
         int total_size;        /* size of the event */
         EVENT_REQUEST *prequest;
         EVENT_HEADER *pevent = (EVENT_HEADER *) (pdata + pc->read_pointer);

         assert(pc->read_pointer >= 0);
         assert(pc->read_pointer <= pheader->size);

         total_size = pevent->data_size + sizeof(EVENT_HEADER);
         total_size = ALIGN8(total_size);

         assert(total_size > 0);
         assert(total_size <= pheader->size);

         prequest = pc->event_request;

         /* loop over all requests: if this event matches a request,
          * copy it to the read cache */

         for (i = 0; i < pc->max_request_index; i++, prequest++)
            if (prequest->valid && bm_match_event(prequest->event_id, prequest->trigger_mask, pevent)) {

               /* we found a request for this event, so copy it */

               if (pbuf->read_cache_size > 0 && total_size < pbuf->read_cache_size) {

                  /* copy event to cache, if there is room */

                  if (pbuf->read_cache_wp + total_size >= pbuf->read_cache_size) {
                     cache_is_full = TRUE;
                     break;     /* exit loop over requests */
                  }

                  if (pc->read_pointer + total_size <= pheader->size) {
                     /* copy event to cache */
                     memcpy(pbuf->read_cache + pbuf->read_cache_wp, pevent, total_size);
                  } else {
                     /* event is splitted */
                     size = pheader->size - pc->read_pointer;
                     memcpy(pbuf->read_cache + pbuf->read_cache_wp, pevent, size);
                     memcpy((char *) pbuf->read_cache + pbuf->read_cache_wp + size, pdata, total_size - size);
                  }

                  pbuf->read_cache_wp += total_size;

               } else {
                  int copy_size = total_size;

                  /* if there are events in the read cache,
                   * we should dispatch them before we
                   * despatch this oversize event */

                  if (bm_read_cache_has_events(pbuf)) {
                     cache_is_full = TRUE;
                     break;     /* exit loop over requests */
                  }

                  use_event_buffer = TRUE;

                  status = BM_SUCCESS;
                  if (copy_size > max_size) {
                     copy_size = max_size;
                     cm_msg(MERROR, "bm_receive_event",
                            "event size %d larger than buffer size %d", total_size, max_size);
                     status = BM_TRUNCATED;
                  }

                  if (pc->read_pointer + total_size <= pheader->size) {
                     /* event is not splitted */
                     memcpy(destination, pevent, copy_size);
                  } else {
                     /* event is splitted */
                     size = pheader->size - pc->read_pointer;

                     if (size > max_size)
                        memcpy(destination, pevent, max_size);
                     else
                        memcpy(destination, pevent, size);

                     if (total_size > max_size) {
                        if (size <= max_size)
                           memcpy((char *) destination + size, pdata, max_size - size);
                     } else
                        memcpy((char *) destination + size, pdata, total_size - size);
                  }

                  *buf_size = copy_size;

                  bm_convert_event_header((EVENT_HEADER *) destination, convert_flags);
               }

               /* update statistics */
               pheader->num_out_events++;
               break;           /* stop looping over requests */
            }

         if (cache_is_full)
            break;              /* exit from loop over events in data buffer, leaving the current event untouched */

         /* shift read pointer */

         assert(total_size > 0);
         assert(total_size <= pheader->size);

         new_read_pointer = pc->read_pointer + total_size;
         if (new_read_pointer >= pheader->size) {
            new_read_pointer = new_read_pointer % pheader->size;

            /* make sure we loop over the data buffer no more than once */
            cycle++;
            assert(cycle < 2);
         }

         /* make sure we do not split the event header at the end of the buffer */
         if (new_read_pointer > pheader->size - (int) sizeof(EVENT_HEADER))
            new_read_pointer = 0;

#ifdef DEBUG_MSG
         cm_msg(MDEBUG, "bm_receive_event -> wp=%d, rp %d -> %d (found=%d,size=%d)",
                pheader->write_pointer, pc->read_pointer, new_read_pointer, found, total_size);
#endif

         pc->read_pointer = new_read_pointer;

         if (use_event_buffer)
            break;              /* exit from loop over events in data buffer */

      } while (pheader->write_pointer != pc->read_pointer);

      /* calculate global read pointer as "minimum" of client read pointers */

      bm_update_read_pointer("bm_receive_event", pheader);

      /*
         If read pointer has been changed, it may have freed up some space
         for waiting producers. So check if free space is now more than 50%
         of the buffer size and wake waiting producers.
       */

      bm_wakeup_producers(pheader, pc);

      bm_unlock_buffer(buffer_handle);

      if (bm_read_cache_has_events(pbuf)) {
         assert(!use_event_buffer);     /* events only go into the _event_buffer when read cache is empty */
         return bm_copy_from_cache(pbuf, destination, max_size, buf_size, convert_flags);
      }

      if (use_event_buffer)
         return status;

      goto LOOP;
   }
#else                           /* LOCAL_ROUTINES */

   return SS_SUCCESS;
#endif
}

/********************************************************************/
/**
Skip all events in current buffer.

Useful for single event displays to see the newest events
@param buffer_handle      Handle of the buffer. Must be obtained
                          via bm_open_buffer.
@return BM_SUCCESS, BM_INVALID_HANDLE, RPC_NET_ERROR
*/
INT bm_skip_event(INT buffer_handle)
{
   if (rpc_is_remote())
      return rpc_call(RPC_BM_SKIP_EVENT, buffer_handle);

#ifdef LOCAL_ROUTINES
   {
      BUFFER *pbuf;
      BUFFER_HEADER *pheader;
      BUFFER_CLIENT *pclient;

      if (buffer_handle > _buffer_entries || buffer_handle <= 0) {
         cm_msg(MERROR, "bm_skip_event", "invalid buffer handle %d", buffer_handle);
         return BM_INVALID_HANDLE;
      }

      pbuf = &_buffer[buffer_handle - 1];
      pheader = pbuf->buffer_header;

      if (!pbuf->attached) {
         cm_msg(MERROR, "bm_skip_event", "invalid buffer handle %d", buffer_handle);
         return BM_INVALID_HANDLE;
      }

      /* clear cache */
      if (pbuf->read_cache_wp > pbuf->read_cache_rp)
         pbuf->read_cache_rp = pbuf->read_cache_wp = 0;

      bm_lock_buffer(buffer_handle);

      /* forward read pointer to global write pointer */
      pclient = pheader->client + bm_validate_client_index(pbuf, TRUE);
      pclient->read_pointer = pheader->write_pointer;

      bm_unlock_buffer(buffer_handle);
   }
#endif

   return BM_SUCCESS;
}

/********************************************************************/
/**
Check a buffer if an event is available and call the dispatch function if found.
@param buffer_name       Name of buffer
@return BM_SUCCESS, BM_INVALID_HANDLE, BM_TRUNCATED, BM_ASYNC_RETURN,
                    RPC_NET_ERROR
*/
INT bm_push_event(char *buffer_name)
{
#ifdef LOCAL_ROUTINES
   {
      BUFFER *pbuf;
      BUFFER_HEADER *pheader;
      BUFFER_CLIENT *pclient, *pc;
      char *pdata;
      INT i, size, buffer_handle;
      INT my_client_index;
      BOOL use_event_buffer = 0;
      BOOL cache_is_full = 0;
      int cycle = 0;

      for (i = 0; i < _buffer_entries; i++)
         if (strcmp(buffer_name, _buffer[i].buffer_header->name) == 0)
            break;
      if (i == _buffer_entries)
         return BM_INVALID_HANDLE;

      buffer_handle = i + 1;
      pbuf = &_buffer[buffer_handle - 1];

      if (!pbuf->attached)
         return BM_INVALID_HANDLE;

      /* return immediately if no callback routine is defined */
      if (!pbuf->callback)
         return BM_SUCCESS;

      if (_event_buffer_size == 0) {
         _event_buffer = (EVENT_HEADER *) M_MALLOC(1000);
         if (_event_buffer == NULL) {
            cm_msg(MERROR, "bm_push_event", "not enough memory to allocate cache buffer");
            return BM_NO_MEMORY;
         }
         _event_buffer_size = 1000;
      }

      /* look if there is anything in the cache */
      if (bm_read_cache_has_events(pbuf)) {
         bm_dispatch_from_cache(pbuf, buffer_handle);
         return BM_MORE_EVENTS;
      }

      /* calculate some shorthands */
      pheader = pbuf->buffer_header;
      pdata = (char *) (pheader + 1);
      my_client_index = bm_validate_client_index(pbuf, TRUE);
      pclient = pheader->client;
      pc = pheader->client + my_client_index;

      /* first do a quick check without locking the buffer */
      if (pheader->write_pointer == pc->read_pointer)
         return BM_SUCCESS;

      /* lock the buffer */
      bm_lock_buffer(buffer_handle);

      if (pheader->write_pointer == pc->read_pointer) {

         bm_unlock_buffer(buffer_handle);

         /* return if no event available */
         return BM_SUCCESS;
      }

      /* loop over all events in the buffer */

      do {
         int new_read_pointer;
         int total_size;        /* size of the event */
         EVENT_REQUEST *prequest;
         EVENT_HEADER *pevent = (EVENT_HEADER *) (pdata + pc->read_pointer);

         assert(pc->read_pointer >= 0);
         assert(pc->read_pointer <= pheader->size);

         total_size = pevent->data_size + sizeof(EVENT_HEADER);
         total_size = ALIGN8(total_size);

         assert(total_size > 0);
         assert(total_size <= pheader->size);

         prequest = pc->event_request;

         /* loop over all requests: if this event matches a request,
          * copy it to the read cache */

         for (i = 0; i < pc->max_request_index; i++, prequest++)
            if (prequest->valid && bm_match_event(prequest->event_id, prequest->trigger_mask, pevent)) {

               /* check if this is a recent event */
               if (prequest->sampling_type == GET_RECENT) {
                  if (ss_time() - pevent->time_stamp > 1) {
                     /* skip that event */
                     continue;
                  }

                  printf("now: %d, event: %d\n", ss_time(), pevent->time_stamp);
               }

               /* we found a request for this event, so copy it */

               if (pbuf->read_cache_size > 0 && total_size < pbuf->read_cache_size) {

                  /* copy event to cache, if there is room */

                  if (pbuf->read_cache_wp + total_size >= pbuf->read_cache_size) {
                     cache_is_full = TRUE;
                     break;     /* exit loop over requests */
                  }

                  if (pc->read_pointer + total_size <= pheader->size) {
                     /* copy event to cache */
                     memcpy(pbuf->read_cache + pbuf->read_cache_wp, pevent, total_size);
                  } else {
                     /* event is splitted */
                     size = pheader->size - pc->read_pointer;
                     memcpy(pbuf->read_cache + pbuf->read_cache_wp, pevent, size);
                     memcpy((char *) pbuf->read_cache + pbuf->read_cache_wp + size, pdata, total_size - size);
                  }

                  pbuf->read_cache_wp += total_size;

               } else {
                  /* copy event to copy buffer */

                  /* if there are events in the read cache,
                   * we should dispatch them before we
                   * despatch this oversize event */

                  if (bm_read_cache_has_events(pbuf)) {
                     cache_is_full = TRUE;
                     break;     /* exit loop over requests */
                  }

                  use_event_buffer = TRUE;

                  if (total_size > _event_buffer_size) {
                     //printf("realloc event buffer %d -> %d\n", _event_buffer_size, total_size);
                     _event_buffer = (EVENT_HEADER *) realloc(_event_buffer, total_size);
                     _event_buffer_size = total_size;
                  }

                  if (pc->read_pointer + total_size <= pheader->size) {
                     memcpy(_event_buffer, pevent, total_size);
                  } else {
                     /* event is splitted */
                     size = pheader->size - pc->read_pointer;

                     memcpy(_event_buffer, pevent, size);
                     memcpy((char *) _event_buffer + size, pdata, total_size - size);
                  }
               }

               /* update statistics */
               pheader->num_out_events++;
               break;           /* exit loop over event requests */

            }
         /* end of loop over event requests */
         if (cache_is_full)
            break;              /* exit from loop over events in data buffer, leaving the current event untouched */

         /* shift read pointer */

         assert(total_size > 0);
         assert(total_size <= pheader->size);

         new_read_pointer = pc->read_pointer + total_size;
         if (new_read_pointer >= pheader->size) {
            new_read_pointer = new_read_pointer % pheader->size;

            /* make sure we loop over the data buffer no more than once */
            cycle++;
            assert(cycle < 2);
         }

         /* make sure we do not split the event header at the end of the buffer */
         if (new_read_pointer > pheader->size - (int) sizeof(EVENT_HEADER))
            new_read_pointer = 0;

#ifdef DEBUG_MSG
         cm_msg(MDEBUG, "bm_push_event -> wp=%d, rp %d -> %d (found=%d,size=%d)",
                pheader->write_pointer, pc->read_pointer, new_read_pointer, found, total_size);
#endif

         pc->read_pointer = new_read_pointer;

         if (use_event_buffer)
            break;              /* exit from loop over events in data buffer */

      } while (pheader->write_pointer != pc->read_pointer);

      /* calculate global read pointer as "minimum" of client read pointers */

      bm_update_read_pointer("bm_push_event", pheader);

      /*
         If read pointer has been changed, it may have freed up some space
         for waiting producers. So check if free space is now more than 50%
         of the buffer size and wake waiting producers.
       */

      bm_wakeup_producers(pheader, pc);

      bm_unlock_buffer(buffer_handle);

      if (bm_read_cache_has_events(pbuf)) {
         assert(!use_event_buffer);     /* events only go into the _event_buffer when read cache is empty */
         bm_dispatch_from_cache(pbuf, buffer_handle);
         return BM_MORE_EVENTS;
      }

      if (use_event_buffer) {
         bm_dispatch_event(buffer_handle, _event_buffer);
         return BM_MORE_EVENTS;
      }

      return BM_SUCCESS;
   }
#else                           /* LOCAL_ROUTINES */

   return BM_SUCCESS;
#endif
}

/********************************************************************/
/**
Check if any requested event is waiting in a buffer
@return TRUE             More events are waiting<br>
        FALSE            No more events are waiting
*/
INT bm_check_buffers()
{
#ifdef LOCAL_ROUTINES
   {
      INT idx, status = 0;
      INT server_type, server_conn, tid;
      BOOL bMore;
      DWORD start_time;

      server_type = rpc_get_server_option(RPC_OSERVER_TYPE);
      server_conn = rpc_get_server_acception();
      tid = ss_gettid();

      /* if running as a server, buffer checking is done by client
         via ASYNC bm_receive_event */
      if (server_type == ST_SUBPROCESS || server_type == ST_MTHREAD)
         return FALSE;

      bMore = FALSE;
      start_time = ss_millitime();

      /* go through all buffers */
      for (idx = 0; idx < _buffer_entries; idx++) {
         if (server_type == ST_SINGLE && _buffer[idx].index != server_conn)
            continue;

         if (server_type != ST_SINGLE && _buffer[idx].index != tid)
            continue;

         if (!_buffer[idx].attached)
            continue;

         do {

            /* one bm_push_event could cause a run stop and a buffer close, which
               would crash the next call to bm_push_event(). So check for valid
               buffer on each call */
            if (idx < _buffer_entries && _buffer[idx].buffer_header->name != NULL)
               status = bm_push_event(_buffer[idx].buffer_header->name);

            if (status != BM_MORE_EVENTS)
               break;

            /* stop after one second */
            if (ss_millitime() - start_time > 1000) {
               bMore = TRUE;
               break;
            }

         } while (TRUE);
      }

      return bMore;

   }
#else                           /* LOCAL_ROUTINES */

   return FALSE;

#endif
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************/
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
      INT i;
      BUFFER_HEADER *pheader;
      BUFFER_CLIENT *pclient;

      /* Mark all buffer for read waiting */
      for (i = 0; i < _buffer_entries; i++) {
         if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_SINGLE
             && _buffer[i].index != rpc_get_server_acception())
            continue;

         if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_SINGLE && _buffer[i].index != ss_gettid())
            continue;

         if (!_buffer[i].attached)
            continue;

         pheader = _buffer[i].buffer_header;
         pclient = pheader->client + bm_validate_client_index(&_buffer[i], TRUE);
         pclient->read_wait = flag;
      }
   }
#endif                          /* LOCAL_ROUTINES */

   return BM_SUCCESS;
}

/********************************************************************/
INT bm_notify_client(char *buffer_name, int s)
/********************************************************************\

  Routine: bm_notify_client

  Purpose: Called by cm_dispatch_ipc. Send an event notification to
           the connected client

  Input:
    char  *buffer_name      Name of buffer
    int   s                 Network socket to client

  Output:
    none

  Function value:
    BM_SUCCESS              Successful completion

\********************************************************************/
{
   char buffer[32];
   NET_COMMAND *nc;
   INT i, convert_flags;
   static DWORD last_time = 0;

   for (i = 0; i < _buffer_entries; i++)
      if (strcmp(buffer_name, _buffer[i].buffer_header->name) == 0)
         break;
   if (i == _buffer_entries)
      return BM_INVALID_HANDLE;

   /* don't send notification if client has no callback defined
      to receive events -> client calls bm_receive_event manually */
   if (!_buffer[i].callback)
      return DB_SUCCESS;

   convert_flags = rpc_get_server_option(RPC_CONVERT_FLAGS);

   /* only send notification once each 500ms */
   if (ss_millitime() - last_time < 500 && !(convert_flags & CF_ASCII))
      return DB_SUCCESS;

   last_time = ss_millitime();

   if (convert_flags & CF_ASCII) {
      sprintf(buffer, "MSG_BM&%s", buffer_name);
      send_tcp(s, buffer, strlen(buffer) + 1, 0);
   } else {
      nc = (NET_COMMAND *) buffer;

      nc->header.routine_id = MSG_BM;
      nc->header.param_size = 0;

      if (convert_flags) {
         rpc_convert_single(&nc->header.routine_id, TID_DWORD, RPC_OUTGOING, convert_flags);
         rpc_convert_single(&nc->header.param_size, TID_DWORD, RPC_OUTGOING, convert_flags);
      }

      /* send the update notification to the client */
      send_tcp(s, (char *) buffer, sizeof(NET_COMMAND_HEADER), 0);
   }

   return BM_SUCCESS;
}

/********************************************************************/
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
   INT status, size, i, request_id;
   DWORD start_time;
   BOOL bMore;
   static BOOL bMoreLast = FALSE;

   if (_event_buffer_size == 0) {
      HNDLE hDB, odb_key;
      int max_event_size = MAX_EVENT_SIZE;

      /* get max event size from ODB */
      status = cm_get_experiment_database(&hDB, &odb_key);
      assert(status == SUCCESS);
      size = sizeof(INT);
      status = db_get_value(hDB, 0, "/Experiment/MAX_EVENT_SIZE", &max_event_size, &size, TID_DWORD, TRUE);

      size = max_event_size + sizeof(EVENT_HEADER);
      //printf("alloc event buffer %d\n", size);
      _event_buffer = (EVENT_HEADER *) M_MALLOC(size);
      if (!_event_buffer) {
         cm_msg(MERROR, "bm_poll_event", "not enough memory to allocate event buffer of size %d", size);
         return SS_ABORT;
      }
      _event_buffer_size = size;
   }

   start_time = ss_millitime();

   /* if we got event notification, turn off read_wait */
   if (!flag)
      bm_mark_read_waiting(FALSE);

   /* if called from yield, return if no more events */
   if (flag) {
      if (!bMoreLast)
         return FALSE;
   }

   bMore = FALSE;

   /* loop over all requests */
   for (request_id = 0; request_id < _request_list_entries; request_id++) {
      /* continue if no dispatcher set (manual bm_receive_event) */
      if (_request_list[request_id].dispatcher == NULL)
         continue;

      do {
         /* receive event */
         size = _event_buffer_size;
         status = bm_receive_event(_request_list[request_id].buffer_handle, _event_buffer, &size, ASYNC);
         //printf("receive_event buf %d, event %d, status %d\n", _event_buffer_size, size, status);

         /* call user function if successful */
         if (status == BM_SUCCESS)
            /* search proper request for this event */
            for (i = 0; i < _request_list_entries; i++)
               if ((_request_list[i].buffer_handle ==
                    _request_list[request_id].buffer_handle) &&
                   bm_match_event(_request_list[i].event_id, _request_list[i].trigger_mask, _event_buffer)) {
                  if ((_event_buffer->event_id & 0xF000) == EVENTID_FRAG1 ||
                      (_event_buffer->event_id & 0xF000) == EVENTID_FRAG)
                     bm_defragment_event(_request_list[i].buffer_handle, i, _event_buffer,
                                         (void *) (((EVENT_HEADER *) _event_buffer) + 1),
                                         _request_list[i].dispatcher);
                  else
                     _request_list[i].dispatcher(_request_list[i].buffer_handle, i,
                                                 _event_buffer,
                                                 (void *) (((EVENT_HEADER *) _event_buffer) + 1));
               }

         /* break if no more events */
         if (status == BM_ASYNC_RETURN)
            break;

         /* break if server died */
         if (status == RPC_NET_ERROR)
            return SS_ABORT;

         /* stop after one second */
         if (ss_millitime() - start_time > 1000) {
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

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Clears event buffer and cache.
If an event buffer is large and a consumer is slow in analyzing
events, events are usually received some time after they are produced.
This effect is even more experienced if a read cache is used
(via bm_set_cache_size()).
When changes to the hardware are made in the experience, the consumer will then
still analyze old events before any new event which reflects the hardware change.
Users can be fooled by looking at histograms which reflect the hardware change
many seconds after they have been made.

To overcome this potential problem, the analyzer can call
bm_empty_buffers() just after the hardware change has been made which
skips all old events contained in event buffers and read caches.
Technically this is done by forwarding the read pointer of the client.
No events are really deleted, they are still visible to other clients like
the logger.

Note that the front-end also contains write buffers which can delay the
delivery of events.
The standard front-end framework mfe.c reduces this effect by flushing
all buffers once every second.
@return BM_SUCCESS
*/
INT bm_empty_buffers()
{
   if (rpc_is_remote())
      return rpc_call(RPC_BM_EMPTY_BUFFERS);

#ifdef LOCAL_ROUTINES
   {
      INT idx, server_type, server_conn, tid;
      BUFFER *pbuf;
      BUFFER_CLIENT *pclient;

      server_type = rpc_get_server_option(RPC_OSERVER_TYPE);
      server_conn = rpc_get_server_acception();
      tid = ss_gettid();

      /* go through all buffers */
      for (idx = 0; idx < _buffer_entries; idx++) {
         if (server_type == ST_SINGLE && _buffer[idx].index != server_conn)
            continue;

         if (server_type != ST_SINGLE && _buffer[idx].index != tid)
            continue;

         if (!_buffer[idx].attached)
            continue;

         pbuf = &_buffer[idx];

         /* empty cache */
         pbuf->read_cache_rp = pbuf->read_cache_wp = 0;

         bm_lock_buffer(idx + 1);

         /* set read pointer to write pointer */
         pclient = (pbuf->buffer_header)->client + bm_validate_client_index(pbuf, TRUE);
         pclient->read_pointer = (pbuf->buffer_header)->write_pointer;

         bm_unlock_buffer(idx + 1);
      }

   }
#endif                          /* LOCAL_ROUTINES */

   return BM_SUCCESS;
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

#define MAX_DEFRAG_EVENTS 10

typedef struct {
   WORD event_id;
   DWORD data_size;
   DWORD received;
   EVENT_HEADER *pevent;
} EVENT_DEFRAG_BUFFER;

EVENT_DEFRAG_BUFFER defrag_buffer[MAX_DEFRAG_EVENTS];

/********************************************************************/
void bm_defragment_event(HNDLE buffer_handle, HNDLE request_id,
                         EVENT_HEADER * pevent, void *pdata, void (*dispatcher) (HNDLE, HNDLE, EVENT_HEADER *,
                                                                                 void *))
/********************************************************************\

  Routine: bm_defragment_event

  Purpose: Called internally from the event receiving routines
           bm_push_event and bm_poll_event to recombine event
           fragments and call the user callback routine upon
           completion.

  Input:
    HNDLE buffer_handle  Handle for the buffer containing event
    HNDLE request_id     Handle for event request
    EVENT_HEADER *pevent Pointer to event header
    void *pata           Pointer to event data
    dispatcher()         User callback routine

  Output:
    <calls dispatcher() after successfull recombination of event>

  Function value:
    void

\********************************************************************/
{
   INT i;
   static int j = -1;

   if ((pevent->event_id & 0xF000) == EVENTID_FRAG1) {
      /*---- start new event ----*/

      //printf("First Frag detected : Ser#:%d ID=0x%x \n", pevent->serial_number, pevent->event_id);

      /* check if fragments already stored */
      for (i = 0; i < MAX_DEFRAG_EVENTS; i++)
         if (defrag_buffer[i].event_id == (pevent->event_id & 0x0FFF))
            break;

      if (i < MAX_DEFRAG_EVENTS) {
         free(defrag_buffer[i].pevent);
         defrag_buffer[i].pevent = NULL;
         memset(&defrag_buffer[i].event_id, 0, sizeof(EVENT_DEFRAG_BUFFER));
         cm_msg(MERROR, "bm_defragement_event",
                "Received new event with ID %d while old fragments were not completed",
                (pevent->event_id & 0x0FFF));
      }

      /* search new slot */
      for (i = 0; i < MAX_DEFRAG_EVENTS; i++)
         if (defrag_buffer[i].event_id == 0)
            break;

      if (i == MAX_DEFRAG_EVENTS) {
         cm_msg(MERROR, "bm_defragment_event",
                "Not enough defragment buffers, please increase MAX_DEFRAG_EVENTS and recompile");
         return;
      }

      /* check event size */
      if (pevent->data_size != sizeof(DWORD)) {
         cm_msg(MERROR, "bm_defragment_event",
                "Received first event fragment with %s bytes instead of %d bytes, event ignored",
                pevent->data_size, sizeof(DWORD));
         return;
      }

      /* setup defragment buffer */
      defrag_buffer[i].event_id = (pevent->event_id & 0x0FFF);
      defrag_buffer[i].data_size = *(DWORD *) pdata;
      defrag_buffer[i].received = 0;
      defrag_buffer[i].pevent = (EVENT_HEADER *) malloc(sizeof(EVENT_HEADER) + defrag_buffer[i].data_size);

      if (defrag_buffer[i].pevent == NULL) {
         memset(&defrag_buffer[i].event_id, 0, sizeof(EVENT_DEFRAG_BUFFER));
         cm_msg(MERROR, "bm_defragement_event", "Not enough memory to allocate event defragment buffer");
         return;
      }

      memcpy(defrag_buffer[i].pevent, pevent, sizeof(EVENT_HEADER));
      defrag_buffer[i].pevent->event_id = defrag_buffer[i].event_id;
      defrag_buffer[i].pevent->data_size = defrag_buffer[i].data_size;

      // printf("First frag[%d] (ID %d) Ser#:%d sz:%d\n", i, defrag_buffer[i].event_id,
      //       pevent->serial_number, defrag_buffer[i].data_size);

      j = 0;

      return;
   }

   /* search buffer for that event */
   for (i = 0; i < MAX_DEFRAG_EVENTS; i++)
      if (defrag_buffer[i].event_id == (pevent->event_id & 0xFFF))
         break;

   if (i == MAX_DEFRAG_EVENTS) {
      /* no buffer available -> no first fragment received */
      cm_msg(MERROR, "bm_defragement_event",
             "Received fragment without first fragment (ID %d) Ser#:%d",
             pevent->event_id & 0x0FFF, pevent->serial_number);
      return;
   }

   /* add fragment to buffer */
   if (pevent->data_size + defrag_buffer[i].received > defrag_buffer[i].data_size) {
      free(defrag_buffer[i].pevent);
      defrag_buffer[i].pevent = NULL;
      memset(&defrag_buffer[i].event_id, 0, sizeof(EVENT_DEFRAG_BUFFER));
      cm_msg(MERROR, "bm_defragement_event",
             "Received fragments with more data (%d) than event size (%d)",
             pevent->data_size + defrag_buffer[i].received, defrag_buffer[i].data_size);
      return;
   }

   memcpy(((char *) defrag_buffer[i].pevent) + sizeof(EVENT_HEADER) +
          defrag_buffer[i].received, pdata, pevent->data_size);

   defrag_buffer[i].received += pevent->data_size;

   //printf("Other frag[%d][%d] (ID %d) Ser#:%d sz:%d\n", i, j++,
   //       defrag_buffer[i].event_id, pevent->serial_number, pevent->data_size);

   if (defrag_buffer[i].received == defrag_buffer[i].data_size) {
      /* event complete */
      dispatcher(buffer_handle, request_id, defrag_buffer[i].pevent, defrag_buffer[i].pevent + 1);
      free(defrag_buffer[i].pevent);
      defrag_buffer[i].pevent = NULL;
      memset(&defrag_buffer[i].event_id, 0, sizeof(EVENT_DEFRAG_BUFFER));
   }
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/**dox***************************************************************/
                                                                                                                               /** @} *//* end of bmfunctionc */

/**dox***************************************************************/
/** @addtogroup rpcfunctionc
 *
 *  @{  */

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************\
*                                                                    *
*                         RPC functions                              *
*                                                                    *
\********************************************************************/

/* globals */

RPC_CLIENT_CONNECTION _client_connection[MAX_RPC_CONNECTION];
RPC_SERVER_CONNECTION _server_connection;

static int _lsock;
RPC_SERVER_ACCEPTION _server_acception[MAX_RPC_CONNECTION];
static INT _server_acception_index = 0;
static INT _server_type;
static char _server_name[256];

static RPC_LIST *rpc_list = NULL;

int _opt_tcp_size = OPT_TCP_SIZE;


/********************************************************************\
*                       conversion functions                         *
\********************************************************************/

void rpc_calc_convert_flags(INT hw_type, INT remote_hw_type, INT * convert_flags)
{
   *convert_flags = 0;

   /* big/little endian conversion */
   if (((remote_hw_type & DRI_BIG_ENDIAN) &&
        (hw_type & DRI_LITTLE_ENDIAN)) || ((remote_hw_type & DRI_LITTLE_ENDIAN)
                                           && (hw_type & DRI_BIG_ENDIAN)))
      *convert_flags |= CF_ENDIAN;

   /* float conversion between IEEE and VAX G */
   if ((remote_hw_type & DRF_G_FLOAT) && (hw_type & DRF_IEEE))
      *convert_flags |= CF_VAX2IEEE;

   /* float conversion between VAX G and IEEE */
   if ((remote_hw_type & DRF_IEEE) && (hw_type & DRF_G_FLOAT))
      *convert_flags |= CF_IEEE2VAX;

   /* ASCII format */
   if (remote_hw_type & DR_ASCII)
      *convert_flags |= CF_ASCII;
}

/********************************************************************/
void rpc_get_convert_flags(INT * convert_flags)
{
   rpc_calc_convert_flags(rpc_get_option(0, RPC_OHW_TYPE), _server_connection.remote_hw_type, convert_flags);
}

/********************************************************************/
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
   *((short int *) (var)) = lo;
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
   *((short int *) (var)) = lo;

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
   *((short int *) (var)) = i1;
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
   *((short int *) (var)) = i1;
}

/********************************************************************/
void rpc_convert_single(void *data, INT tid, INT flags, INT convert_flags)
{

   if (convert_flags & CF_ENDIAN) {
      if (tid == TID_WORD || tid == TID_SHORT)
         WORD_SWAP(data);
      if (tid == TID_DWORD || tid == TID_INT || tid == TID_BOOL || tid == TID_FLOAT)
         DWORD_SWAP(data);
      if (tid == TID_DOUBLE)
         QWORD_SWAP(data);
   }

   if (((convert_flags & CF_IEEE2VAX) && !(flags & RPC_OUTGOING)) ||
       ((convert_flags & CF_VAX2IEEE) && (flags & RPC_OUTGOING))) {
      if (tid == TID_FLOAT)
         rpc_ieee2vax_float((float *) data);
      if (tid == TID_DOUBLE)
         rpc_ieee2vax_double((double *) data);
   }

   if (((convert_flags & CF_IEEE2VAX) && (flags & RPC_OUTGOING)) ||
       ((convert_flags & CF_VAX2IEEE) && !(flags & RPC_OUTGOING))) {
      if (tid == TID_FLOAT)
         rpc_vax2ieee_float((float *) data);
      if (tid == TID_DOUBLE)
         rpc_vax2ieee_double((double *) data);
   }
}

void rpc_convert_data(void *data, INT tid, INT flags, INT total_size, INT convert_flags)
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
   INT i, n, single_size;
   char *p;

   /* convert array */
   if (flags & (RPC_FIXARRAY | RPC_VARARRAY)) {
      single_size = tid_size[tid];
      /* don't convert TID_ARRAY & TID_STRUCT */
      if (single_size == 0)
         return;

      n = total_size / single_size;

      for (i = 0; i < n; i++) {
         p = (char *) data + (i * single_size);
         rpc_convert_single(p, tid, flags, convert_flags);
      }
   } else {
      rpc_convert_single(data, tid, flags, convert_flags);
   }
}

/********************************************************************\
*                       type ID functions                            *
\********************************************************************/

INT rpc_tid_size(INT id)
{
   if (id >= 0 && id < TID_LAST)
      return tid_size[id];

   return 0;
}

char *rpc_tid_name(INT id)
{
   if (id >= 0 && id < TID_LAST)
      return tid_name[id];
   else
      return "<unknown>";
}


/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************\
*                        client functions                            *
\********************************************************************/

/********************************************************************/
/**
Register RPC client for standalone mode (without standard
           midas server)
@param list           Array of RPC_LIST structures containing
                            function IDs and parameter definitions.
                            The end of the list must be indicated by
                            a function ID of zero.
@param name          Name of this client
@return RPC_SUCCESS
*/
INT rpc_register_client(const char *name, RPC_LIST * list)
{
   rpc_set_name(name);
   rpc_register_functions(rpc_get_internal_list(0), NULL);
   rpc_register_functions(list, NULL);

   return RPC_SUCCESS;
}

/********************************************************************/
/**
Register a set of RPC functions (both as clients or servers)
@param new_list       Array of RPC_LIST structures containing
                            function IDs and parameter definitions.
                            The end of the list must be indicated by
                            a function ID of zero.
@param func          Default dispatch function

@return RPC_SUCCESS, RPC_NO_MEMORY, RPC_DOUBLE_DEFINED
*/
INT rpc_register_functions(const RPC_LIST * new_list, INT(*func) (INT, void **))
{
   INT i, j, iold, inew;

   /* count number of new functions */
   for (i = 0; new_list[i].id != 0; i++) {
      /* check double defined functions */
      for (j = 0; rpc_list != NULL && rpc_list[j].id != 0; j++)
         if (rpc_list[j].id == new_list[i].id)
            return RPC_DOUBLE_DEFINED;
   }
   inew = i;

   /* count number of existing functions */
   for (i = 0; rpc_list != NULL && rpc_list[i].id != 0; i++);
   iold = i;

   /* allocate new memory for rpc_list */
   if (rpc_list == NULL)
      rpc_list = (RPC_LIST *) M_MALLOC(sizeof(RPC_LIST) * (inew + 1));
   else
      rpc_list = (RPC_LIST *) realloc(rpc_list, sizeof(RPC_LIST) * (iold + inew + 1));

   if (rpc_list == NULL) {
      cm_msg(MERROR, "rpc_register_functions", "out of memory");
      return RPC_NO_MEMORY;
   }

   /* append new functions */
   for (i = iold; i < iold + inew; i++) {
      memmove(rpc_list + i, new_list + i - iold, sizeof(RPC_LIST));

      /* set default dispatcher */
      if (rpc_list[i].dispatch == NULL)
         rpc_list[i].dispatch = func;

      /* check valid ID for user functions */
      if (new_list != rpc_get_internal_list(0) &&
          new_list != rpc_get_internal_list(1) && (rpc_list[i].id < RPC_MIN_ID
                                                   || rpc_list[i].id > RPC_MAX_ID))
         cm_msg(MERROR, "rpc_register_functions", "registered RPC function with invalid ID");
   }

   /* mark end of list */
   rpc_list[i].id = 0;

   return RPC_SUCCESS;
}



/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************/
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
      M_FREE(rpc_list);
   rpc_list = NULL;

   return RPC_SUCCESS;
}


/********************************************************************/
INT rpc_register_function(INT id, INT(*func) (INT, void **))
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

   for (i = 0; rpc_list != NULL && rpc_list[i].id != 0; i++)
      if (rpc_list[i].id == id)
         break;

   if (rpc_list[i].id == id)
      rpc_list[i].dispatch = func;
   else
      return RPC_INVALID_ID;

   return RPC_SUCCESS;
}


/********************************************************************/
INT rpc_client_dispatch(int sock)
/********************************************************************\

  Routine: rpc_client_dispatch

  Purpose: Gets called whenever a client receives data from the
           server. Get set via rpc_connect. Internal use only.

\********************************************************************/
{
   INT hDB, hKey, n;
   NET_COMMAND *nc;
   INT status = 0;
   char net_buffer[256];

   nc = (NET_COMMAND *) net_buffer;

   n = recv_tcp(sock, net_buffer, sizeof(net_buffer), 0);
   if (n <= 0)
      return SS_ABORT;

   if (nc->header.routine_id == MSG_ODB) {
      /* update a changed record */
      hDB = *((INT *) nc->param);
      hKey = *((INT *) nc->param + 1);
      status = db_update_record(hDB, hKey, 0);
   }

   else if (nc->header.routine_id == MSG_WATCHDOG) {
      nc->header.routine_id = 1;
      nc->header.param_size = 0;
      send_tcp(sock, net_buffer, sizeof(NET_COMMAND_HEADER), 0);
      status = RPC_SUCCESS;
   }

   else if (nc->header.routine_id == MSG_BM) {
      fd_set readfds;
      struct timeval timeout;

      /* receive further messages to empty TCP queue */
      do {
         FD_ZERO(&readfds);
         FD_SET(sock, &readfds);

         timeout.tv_sec = 0;
         timeout.tv_usec = 0;

         select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);

         if (FD_ISSET(sock, &readfds)) {
            n = recv_tcp(sock, net_buffer, sizeof(net_buffer), 0);
            if (n <= 0)
               return SS_ABORT;

            if (nc->header.routine_id == MSG_ODB) {
               /* update a changed record */
               hDB = *((INT *) nc->param);
               hKey = *((INT *) nc->param + 1);
               status = db_update_record(hDB, hKey, 0);
            }

            else if (nc->header.routine_id == MSG_WATCHDOG) {
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


/********************************************************************/
INT rpc_client_connect(const char *host_name, INT port, const char *client_name, HNDLE * hConnection)
/********************************************************************\

  Routine: rpc_client_connect

  Purpose: Establish a network connection to a remote client

  Input:
    char *host_name          IP address of host to connect to.
    INT  port                TPC port to connect to.
    char *clinet_name        Client program name

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
   INT i, status, idx;
   struct sockaddr_in bind_addr;
   INT sock;
   INT remote_hw_type, hw_type;
   char str[200];
   char version[32], v1[32];
   char local_prog_name[NAME_LENGTH];
   char local_host_name[HOST_NAME_LENGTH];
   struct hostent *phe;

#ifdef OS_WINNT
   {
      WSADATA WSAData;

      /* Start windows sockets */
      if (WSAStartup(MAKEWORD(1, 1), &WSAData) != 0)
         return RPC_NET_ERROR;
   }
#endif

   /* check if cm_connect_experiment was called */
   if (_client_name[0] == 0) {
      cm_msg(MERROR, "rpc_client_connect", "cm_connect_experiment/rpc_set_name not called");
      return RPC_NOT_REGISTERED;
   }

   /* check for broken connections */
   rpc_client_check();

   /* check if connection already exists */
   for (i = 0; i < MAX_RPC_CONNECTION; i++)
      if (_client_connection[i].send_sock != 0 &&
          strcmp(_client_connection[i].host_name, host_name) == 0 && _client_connection[i].port == port) {
         *hConnection = i + 1;
         return RPC_SUCCESS;
      }

   /* search for free entry */
   for (i = 0; i < MAX_RPC_CONNECTION; i++)
      if (_client_connection[i].send_sock == 0)
         break;

   /* open new network connection */
   if (i == MAX_RPC_CONNECTION) {
      cm_msg(MERROR, "rpc_client_connect", "maximum number of connections exceeded");
      return RPC_NO_CONNECTION;
   }

   /* create a new socket for connecting to remote server */
   sock = socket(AF_INET, SOCK_STREAM, 0);
   if (sock == -1) {
      cm_msg(MERROR, "rpc_client_connect", "cannot create socket");
      return RPC_NET_ERROR;
   }

   idx = i;
   strcpy(_client_connection[idx].host_name, host_name);
   strcpy(_client_connection[idx].client_name, client_name);
   _client_connection[idx].port = port;
   _client_connection[idx].exp_name[0] = 0;
   _client_connection[idx].transport = RPC_TCP;
   _client_connection[idx].rpc_timeout = DEFAULT_RPC_TIMEOUT;
   _client_connection[idx].rpc_timeout = DEFAULT_RPC_TIMEOUT;

   /* connect to remote node */
   memset(&bind_addr, 0, sizeof(bind_addr));
   bind_addr.sin_family = AF_INET;
   bind_addr.sin_addr.s_addr = 0;
   bind_addr.sin_port = htons((short) port);

#ifdef OS_VXWORKS
   {
      INT host_addr;

      host_addr = hostGetByName(host_name);
      memcpy((char *) &(bind_addr.sin_addr), &host_addr, 4);
   }
#else
   phe = gethostbyname(host_name);
   if (phe == NULL) {
      cm_msg(MERROR, "rpc_client_connect", "cannot lookup host name \'%s\'", host_name);
      return RPC_NET_ERROR;
   }
   memcpy((char *) &(bind_addr.sin_addr), phe->h_addr, phe->h_length);
#endif

#ifdef OS_UNIX
   do {
      status = connect(sock, (void *) &bind_addr, sizeof(bind_addr));

      /* don't return if an alarm signal was cought */
   } while (status == -1 && errno == EINTR);
#else
   status = connect(sock, (struct sockaddr *) &bind_addr, sizeof(bind_addr));
#endif

   if (status != 0) {
      /* cm_msg(MERROR, "rpc_client_connect", "cannot connect");
         message should be displayed by application */
      return RPC_NET_ERROR;
   }

   /* set TCP_NODELAY option for better performance */
   i = 1;
   setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *) &i, sizeof(i));

   /* send local computer info */
   rpc_get_name(local_prog_name);
   gethostname(local_host_name, sizeof(local_host_name));

   hw_type = rpc_get_option(0, RPC_OHW_TYPE);
   sprintf(str, "%d %s %s %s", hw_type, cm_get_version(), local_prog_name, local_host_name);

   send(sock, str, strlen(str) + 1, 0);

   /* receive remote computer info */
   i = recv_string(sock, str, sizeof(str), _rpc_connect_timeout);
   if (i <= 0) {
      cm_msg(MERROR, "rpc_client_connect", "timeout on receive remote computer info: %s", str);
      return RPC_NET_ERROR;
   }

   remote_hw_type = version[0] = 0;
   sscanf(str, "%d %s", &remote_hw_type, version);
   _client_connection[idx].remote_hw_type = remote_hw_type;
   _client_connection[idx].send_sock = sock;

   /* print warning if version patch level doesn't agree */
   strcpy(v1, version);
   if (strchr(v1, '.'))
      if (strchr(strchr(v1, '.') + 1, '.'))
         *strchr(strchr(v1, '.') + 1, '.') = 0;

   strcpy(str, cm_get_version());
   if (strchr(str, '.'))
      if (strchr(strchr(str, '.') + 1, '.'))
         *strchr(strchr(str, '.') + 1, '.') = 0;

   if (strcmp(v1, str) != 0) {
      sprintf(str, "remote MIDAS version \'%s\' differs from local version \'%s\'", version, cm_get_version());
      cm_msg(MERROR, "rpc_client_connect", str);
   }

   *hConnection = idx + 1;

   return RPC_SUCCESS;
}

/********************************************************************/
void rpc_client_check()
/********************************************************************\

  Routine: rpc_client_check

  Purpose: Check all client connections if remote client closed link

  Function value:
    RPC_SUCCESS              Successful completion
    RPC_NET_ERROR            Error in socket call
    RPC_NO_CONNECTION        Maximum number of connections reached
    RPC_NOT_REGISTERED       cm_connect_experiment was not called properly

\********************************************************************/
{
   INT i, status;

   /* check for broken connections */
   for (i = 0; i < MAX_RPC_CONNECTION; i++)
      if (_client_connection[i].send_sock != 0) {
         int sock;
         fd_set readfds;
         struct timeval timeout;
         char buffer[64];

         sock = _client_connection[i].send_sock;
         FD_ZERO(&readfds);
         FD_SET(sock, &readfds);

         timeout.tv_sec = 0;
         timeout.tv_usec = 0;

         do {
            status = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);
         } while (status == -1);        /* dont return if an alarm signal was cought */

         if (FD_ISSET(sock, &readfds)) {
            status = recv(sock, (char *) buffer, sizeof(buffer), 0);

            if (equal_ustring(buffer, "EXIT")) {
               /* normal exit */
               closesocket(sock);
               memset(&_client_connection[i], 0, sizeof(RPC_CLIENT_CONNECTION));
            }

            if (status <= 0) {
               cm_msg(MERROR, "rpc_client_check",
                      "Connection broken to \"%s\" on host %s",
                      _client_connection[i].client_name, _client_connection[i].host_name);

               /* connection broken -> reset */
               closesocket(sock);
               memset(&_client_connection[i], 0, sizeof(RPC_CLIENT_CONNECTION));
            }
         }
      }
}


/********************************************************************/
INT rpc_server_connect(const char *host_name, const char *exp_name)
/********************************************************************\

  Routine: rpc_server_connect

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
   INT i, status, flag;
   struct sockaddr_in bind_addr;
   INT sock, lsock1, lsock2, lsock3;
   INT listen_port1, listen_port2, listen_port3;
   INT remote_hw_type, hw_type;
   unsigned int size;
   char str[200], version[32], v1[32];
   char local_prog_name[NAME_LENGTH];
   struct hostent *phe;
   fd_set readfds;
   struct timeval timeout;
   int port = MIDAS_TCP_PORT;
   char *s;

#ifdef OS_WINNT
   {
      WSADATA WSAData;

      /* Start windows sockets */
      if (WSAStartup(MAKEWORD(1, 1), &WSAData) != 0)
         return RPC_NET_ERROR;
   }
#endif

   /* check if local connection */
   if (host_name[0] == 0)
      return RPC_SUCCESS;

   /* register system functions */
   rpc_register_functions(rpc_get_internal_list(0), NULL);

   /* check if cm_connect_experiment was called */
   if (_client_name[0] == 0) {
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
   if (lsock3 == -1) {
      cm_msg(MERROR, "rpc_server_connect", "cannot create socket");
      return RPC_NET_ERROR;
   }

   flag = 1;
   setsockopt(lsock1, SOL_SOCKET, SO_REUSEADDR, (char *) &flag, sizeof(INT));
   setsockopt(lsock2, SOL_SOCKET, SO_REUSEADDR, (char *) &flag, sizeof(INT));
   setsockopt(lsock3, SOL_SOCKET, SO_REUSEADDR, (char *) &flag, sizeof(INT));

   /* let OS choose any port number */
   memset(&bind_addr, 0, sizeof(bind_addr));
   bind_addr.sin_family = AF_INET;
   bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   bind_addr.sin_port = 0;

   status = bind(lsock1, (struct sockaddr *) &bind_addr, sizeof(bind_addr));
   bind_addr.sin_port = 0;
   status = bind(lsock2, (struct sockaddr *) &bind_addr, sizeof(bind_addr));
   bind_addr.sin_port = 0;
   status = bind(lsock3, (struct sockaddr *) &bind_addr, sizeof(bind_addr));
   if (status < 0) {
      cm_msg(MERROR, "rpc_server_connect", "cannot bind");
      return RPC_NET_ERROR;
   }

   /* listen for connection */
   status = listen(lsock1, 1);
   status = listen(lsock2, 1);
   status = listen(lsock3, 1);
   if (status < 0) {
      cm_msg(MERROR, "rpc_server_connect", "cannot listen");
      return RPC_NET_ERROR;
   }

   /* find out which port OS has chosen */
   size = sizeof(bind_addr);
#ifdef OS_WINNT
   getsockname(lsock1, (struct sockaddr *) &bind_addr, (int *) &size);
   listen_port1 = ntohs(bind_addr.sin_port);
   getsockname(lsock2, (struct sockaddr *) &bind_addr, (int *) &size);
   listen_port2 = ntohs(bind_addr.sin_port);
   getsockname(lsock3, (struct sockaddr *) &bind_addr, (int *) &size);
   listen_port3 = ntohs(bind_addr.sin_port);
#else
   getsockname(lsock1, (struct sockaddr *) &bind_addr, &size);
   listen_port1 = ntohs(bind_addr.sin_port);
   getsockname(lsock2, (struct sockaddr *) &bind_addr, &size);
   listen_port2 = ntohs(bind_addr.sin_port);
   getsockname(lsock3, (struct sockaddr *) &bind_addr, &size);
   listen_port3 = ntohs(bind_addr.sin_port);
#endif

   /* create a new socket for connecting to remote server */
   sock = socket(AF_INET, SOCK_STREAM, 0);
   if (sock == -1) {
      cm_msg(MERROR, "rpc_server_connect", "cannot create socket");
      return RPC_NET_ERROR;
   }

   /* extract port number from host_name */
   strlcpy(str, host_name, sizeof(str));
   s = strchr(str, ':');
   if (s) {
      *s = 0;
      port = strtoul(s + 1, NULL, 0);
   }

   /* connect to remote node */
   memset(&bind_addr, 0, sizeof(bind_addr));
   bind_addr.sin_family = AF_INET;
   bind_addr.sin_addr.s_addr = 0;
   bind_addr.sin_port = htons(port);

#ifdef OS_VXWORKS
   {
      INT host_addr;

      host_addr = hostGetByName(str);
      memcpy((char *) &(bind_addr.sin_addr), &host_addr, 4);
   }
#else
   phe = gethostbyname(str);
   if (phe == NULL) {
      cm_msg(MERROR, "rpc_server_connect", "cannot resolve host name \'%s\'", str);
      return RPC_NET_ERROR;
   }
   memcpy((char *) &(bind_addr.sin_addr), phe->h_addr, phe->h_length);
#endif

#ifdef OS_UNIX
   do {
      status = connect(sock, (struct sockaddr *) &bind_addr, sizeof(bind_addr));

      /* don't return if an alarm signal was cought */
   } while (status == -1 && errno == EINTR);
#else
   status = connect(sock, (struct sockaddr *) &bind_addr, sizeof(bind_addr));
#endif

   if (status != 0) {
/*    cm_msg(MERROR, "rpc_server_connect", "cannot connect"); message should be displayed by application */
      return RPC_NET_ERROR;
   }

   /* connect to experiment */
   if (exp_name[0] == 0)
      sprintf(str, "C %d %d %d %s Default", listen_port1, listen_port2, listen_port3, cm_get_version());
   else
      sprintf(str, "C %d %d %d %s %s", listen_port1, listen_port2, listen_port3, cm_get_version(), exp_name);

   send(sock, str, strlen(str) + 1, 0);
   i = recv_string(sock, str, sizeof(str), _rpc_connect_timeout);
   closesocket(sock);
   if (i <= 0) {
      cm_msg(MERROR, "rpc_server_connect", "timeout on receive status from server");
      return RPC_NET_ERROR;
   }

   status = version[0] = 0;
   sscanf(str, "%d %s", &status, version);

   if (status == 2) {
/*  message "undefined experiment" should be displayed by application */
      return CM_UNDEF_EXP;
   }

   /* print warning if version patch level doesn't agree */
   strcpy(v1, version);
   if (strchr(v1, '.'))
      if (strchr(strchr(v1, '.') + 1, '.'))
         *strchr(strchr(v1, '.') + 1, '.') = 0;

   strcpy(str, cm_get_version());
   if (strchr(str, '.'))
      if (strchr(strchr(str, '.') + 1, '.'))
         *strchr(strchr(str, '.') + 1, '.') = 0;

   if (strcmp(v1, str) != 0) {
      sprintf(str, "remote MIDAS version \'%s\' differs from local version \'%s\'", version, cm_get_version());
      cm_msg(MERROR, "rpc_server_connect", str);
   }

   /* wait for callback on send and recv socket with timeout */
   FD_ZERO(&readfds);
   FD_SET(lsock1, &readfds);
   FD_SET(lsock2, &readfds);
   FD_SET(lsock3, &readfds);

   timeout.tv_sec = _rpc_connect_timeout / 1000;
   timeout.tv_usec = 0;

   do {
      status = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);

      /* if an alarm signal was cought, restart select with reduced timeout */
      if (status == -1 && timeout.tv_sec >= WATCHDOG_INTERVAL / 1000)
         timeout.tv_sec -= WATCHDOG_INTERVAL / 1000;

   } while (status == -1);      /* dont return if an alarm signal was cought */

   if (!FD_ISSET(lsock1, &readfds)) {
      cm_msg(MERROR, "rpc_server_connect", "mserver subprocess could not be started (check path)");
      closesocket(lsock1);
      closesocket(lsock2);
      closesocket(lsock3);
      return RPC_NET_ERROR;
   }

   size = sizeof(bind_addr);

#ifdef OS_WINNT
   _server_connection.send_sock = accept(lsock1, (struct sockaddr *) &bind_addr, (int *) &size);
   _server_connection.recv_sock = accept(lsock2, (struct sockaddr *) &bind_addr, (int *) &size);
   _server_connection.event_sock = accept(lsock3, (struct sockaddr *) &bind_addr, (int *) &size);
#else
   _server_connection.send_sock = accept(lsock1, (struct sockaddr *) &bind_addr, &size);
   _server_connection.recv_sock = accept(lsock2, (struct sockaddr *) &bind_addr, &size);
   _server_connection.event_sock = accept(lsock3, (struct sockaddr *) &bind_addr, &size);
#endif

   if (_server_connection.send_sock == -1 || _server_connection.recv_sock == -1
       || _server_connection.event_sock == -1) {
      cm_msg(MERROR, "rpc_server_connect", "accept() failed");
      return RPC_NET_ERROR;
   }

   closesocket(lsock1);
   closesocket(lsock2);
   closesocket(lsock3);

   /* set TCP_NODELAY option for better performance */
   flag = 1;
   setsockopt(_server_connection.send_sock, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(flag));
   setsockopt(_server_connection.event_sock, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(flag));

   /* increase send buffer size to 2 Mbytes, on Linux also limited by sysctl net.ipv4.tcp_rmem and net.ipv4.tcp_wmem */
   flag = 2 * 1024 * 1024;
   status = setsockopt(_server_connection.event_sock, SOL_SOCKET, SO_SNDBUF, (char *) &flag, sizeof(flag));
   if (status != 0)
      cm_msg(MERROR, "rpc_server_connect", "cannot setsockopt(SOL_SOCKET, SO_SNDBUF), errno %d (%s)", errno,
             strerror(errno));

   /* send local computer info */
   rpc_get_name(local_prog_name);
   hw_type = rpc_get_option(0, RPC_OHW_TYPE);
   sprintf(str, "%d %s", hw_type, local_prog_name);

   send(_server_connection.send_sock, str, strlen(str) + 1, 0);

   /* receive remote computer info */
   i = recv_string(_server_connection.send_sock, str, sizeof(str), _rpc_connect_timeout);
   if (i <= 0) {
      cm_msg(MERROR, "rpc_server_connect", "timeout on receive remote computer info");
      return RPC_NET_ERROR;
   }

   sscanf(str, "%d", &remote_hw_type);
   _server_connection.remote_hw_type = remote_hw_type;

   /* set dispatcher which receives database updates */
   ss_suspend_set_dispatch(CH_CLIENT, &_server_connection, (int (*)(void)) rpc_client_dispatch);

   return RPC_SUCCESS;
}

/********************************************************************/
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

   if (hConn == -1) {
      /* close all open connections */
      for (i = MAX_RPC_CONNECTION - 1; i >= 0; i--)
         if (_client_connection[i].send_sock != 0)
            rpc_client_disconnect(i + 1, FALSE);

      /* close server connection from other clients */
      for (i = 0; i < MAX_RPC_CONNECTION; i++)
         if (_server_acception[i].recv_sock) {
            send(_server_acception[i].recv_sock, "EXIT", 5, 0);
            closesocket(_server_acception[i].recv_sock);
         }
   } else {
      /* notify server about exit */

      /* set FTCP mode (helps for rebooted VxWorks nodes) */
      rpc_set_option(hConn, RPC_OTRANSPORT, RPC_FTCP);
      rpc_client_call(hConn, bShutdown ? RPC_ID_SHUTDOWN : RPC_ID_EXIT);

      /* close socket */
      if (_client_connection[hConn - 1].send_sock)
         closesocket(_client_connection[hConn - 1].send_sock);

      memset(&_client_connection[hConn - 1], 0, sizeof(RPC_CLIENT_CONNECTION));
   }

   return RPC_SUCCESS;
}


/********************************************************************/
INT rpc_server_disconnect()
/********************************************************************\

  Routine: rpc_server_disconnect

  Purpose: Close a rpc connection to a MIDAS server and close all
           server connections from other clients

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
   static int rpc_server_disconnect_recursion_level = 0;

   if (rpc_server_disconnect_recursion_level)
      return RPC_SUCCESS;

   rpc_server_disconnect_recursion_level = 1;

   /* flush remaining events */
   rpc_flush_event();

   /* notify server about exit */
   rpc_call(RPC_ID_EXIT);

   /* close sockets */
   closesocket(_server_connection.send_sock);
   closesocket(_server_connection.recv_sock);
   closesocket(_server_connection.event_sock);

   memset(&_server_connection, 0, sizeof(RPC_SERVER_CONNECTION));

   /* remove semaphore */
   if (_mutex_rpc)
      ss_mutex_delete(_mutex_rpc);

   rpc_server_disconnect_recursion_level = 0;
   return RPC_SUCCESS;
}


/********************************************************************/
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


/********************************************************************/
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


/********************************************************************/
INT rpc_set_server_acception(INT idx)
/********************************************************************\

  Routine: rpc_set_server_acception

  Purpose: Set actual RPC server connection index

  Input:
    INT  idx                Server index

  Output:
    none

  Function value:
    RPC_SUCCESS             Successful completion

\********************************************************************/
{
   _server_acception_index = idx;
   return RPC_SUCCESS;
}


/********************************************************************/
INT rpc_get_option(HNDLE hConn, INT item)
/********************************************************************\

  Routine: rpc_get_option

  Purpose: Get actual RPC option

  Input:
    HNDLE hConn             RPC connection handle, -1 for server connection, -2 for rpc connect timeout

    INT   item              One of RPC_Oxxx

  Output:
    none

  Function value:
    INT                     Actual option

\********************************************************************/
{
   switch (item) {
   case RPC_OTIMEOUT:
      if (hConn == -1)
         return _server_connection.rpc_timeout;
      if (hConn == -2)
         return _rpc_connect_timeout;
      return _client_connection[hConn - 1].rpc_timeout;

   case RPC_OTRANSPORT:
      if (hConn == -1)
         return _server_connection.transport;
      return _client_connection[hConn - 1].transport;

   case RPC_OHW_TYPE:
      {
         INT tmp_type, size;
         DWORD dummy;
         unsigned char *p;
         float f;
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
         if (*p == 0x78)
            tmp_type |= DRI_LITTLE_ENDIAN;
         else if (*p == 0x12)
            tmp_type |= DRI_BIG_ENDIAN;
         else
            cm_msg(MERROR, "rpc_get_option", "unknown byte order format");

         /* floating point format */
         f = (float) 1.2345;
         dummy = 0;
         memcpy(&dummy, &f, sizeof(f));
         if ((dummy & 0xFF) == 0x19 &&
             ((dummy >> 8) & 0xFF) == 0x04 && ((dummy >> 16) & 0xFF) == 0x9E
             && ((dummy >> 24) & 0xFF) == 0x3F)
            tmp_type |= DRF_IEEE;
         else if ((dummy & 0xFF) == 0x9E &&
                  ((dummy >> 8) & 0xFF) == 0x40 && ((dummy >> 16) & 0xFF) == 0x19
                  && ((dummy >> 24) & 0xFF) == 0x04)
            tmp_type |= DRF_G_FLOAT;
         else
            cm_msg(MERROR, "rpc_get_option", "unknown floating point format");

         d = (double) 1.2345;
         dummy = 0;
         memcpy(&dummy, &d, sizeof(f));
         if ((dummy & 0xFF) == 0x8D &&  /* little endian */
             ((dummy >> 8) & 0xFF) == 0x97 && ((dummy >> 16) & 0xFF) == 0x6E
             && ((dummy >> 24) & 0xFF) == 0x12)
            tmp_type |= DRF_IEEE;
         else if ((dummy & 0xFF) == 0x83 &&     /* big endian */
                  ((dummy >> 8) & 0xFF) == 0xC0 && ((dummy >> 16) & 0xFF) == 0xF3
                  && ((dummy >> 24) & 0xFF) == 0x3F)
            tmp_type |= DRF_IEEE;
         else if ((dummy & 0xFF) == 0x13 &&
                  ((dummy >> 8) & 0xFF) == 0x40 && ((dummy >> 16) & 0xFF) == 0x83
                  && ((dummy >> 24) & 0xFF) == 0xC0)
            tmp_type |= DRF_G_FLOAT;
         else if ((dummy & 0xFF) == 0x9E &&
                  ((dummy >> 8) & 0xFF) == 0x40 && ((dummy >> 16) & 0xFF) == 0x18
                  && ((dummy >> 24) & 0xFF) == 0x04)
            cm_msg(MERROR, "rpc_get_option",
                   "MIDAS cannot handle VAX D FLOAT format. Please compile with the /g_float flag");
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

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Set RPC option
@param hConn              RPC connection handle, -1 for server connection, -2 for rpc connect timeout
@param item               One of RPC_Oxxx
@param value              Value to set
@return RPC_SUCCESS
*/
INT rpc_set_option(HNDLE hConn, INT item, INT value)
{
   switch (item) {
   case RPC_OTIMEOUT:
      if (hConn == -1)
         _server_connection.rpc_timeout = value;
      else if (hConn == -2)
         _rpc_connect_timeout = value;
      else
         _client_connection[hConn - 1].rpc_timeout = value;
      break;

   case RPC_OTRANSPORT:
      if (hConn == -1)
         _server_connection.transport = value;
      else
         _client_connection[hConn - 1].transport = value;
      break;

   case RPC_NODELAY:
      if (hConn == -1)
         setsockopt(_server_connection.send_sock, IPPROTO_TCP, TCP_NODELAY, (char *) &value, sizeof(value));
      else
         setsockopt(_client_connection[hConn - 1].send_sock, IPPROTO_TCP, TCP_NODELAY, (char *) &value,
                    sizeof(value));
      break;

   default:
      cm_msg(MERROR, "rpc_set_option", "invalid argument");
      break;
   }

   return 0;
}


/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************/
POINTER_T rpc_get_server_option(INT item)
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
      return (POINTER_T) _server_name;

   /* return 0 for local calls */
   if (_server_type == ST_NONE)
      return 0;

   /* check which connections belongs to caller */
   if (_server_type == ST_MTHREAD) {
      for (i = 0; i < MAX_RPC_CONNECTION; i++)
         if (_server_acception[i].tid == ss_gettid())
            break;
   } else if (_server_type == ST_SINGLE || _server_type == ST_REMOTE)
      i = MAX(0, _server_acception_index - 1);
   else
      i = 0;

   switch (item) {
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


/********************************************************************/
INT rpc_set_server_option(INT item, POINTER_T value)
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

   if (item == RPC_OSERVER_TYPE) {
      _server_type = value;
      return RPC_SUCCESS;
   }
   if (item == RPC_OSERVER_NAME) {
      strcpy(_server_name, (char *) value);
      return RPC_SUCCESS;
   }

   /* check which connections belongs to caller */
   if (_server_type == ST_MTHREAD) {
      for (i = 0; i < MAX_RPC_CONNECTION; i++)
         if (_server_acception[i].tid == ss_gettid())
            break;
   } else if (_server_type == ST_SINGLE || _server_type == ST_REMOTE)
      i = MAX(0, _server_acception_index - 1);
   else
      i = 0;

   switch (item) {
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


/********************************************************************/
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


/********************************************************************/
INT rpc_set_name(const char *name)
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


/********************************************************************/
INT rpc_set_debug(void (*func) (char *), INT mode)
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
   _debug_mode = mode;
   return RPC_SUCCESS;
}

/********************************************************************/
void rpc_debug_printf(const char *format, ...)
/********************************************************************\

  Routine: rpc_debug_print

  Purpose: Calls function set via rpc_set_debug to output a string.

  Input:
   char *str                Debug string

  Output:
    none

\********************************************************************/
{
   va_list argptr;
   char str[1000];

   if (_debug_mode) {
      va_start(argptr, format);
      vsprintf(str, (char *) format, argptr);
      va_end(argptr);

      if (_debug_print) {
         strcat(str, "\n");
         _debug_print(str);
      } else
         puts(str);
   }
}

/********************************************************************/
void rpc_va_arg(va_list * arg_ptr, INT arg_type, void *arg)
{
   switch (arg_type) {
      /* On the stack, the minimum parameter size is sizeof(int).
         To avoid problems on little endian systems, treat all
         smaller parameters as int's */
   case TID_BYTE:
   case TID_SBYTE:
   case TID_CHAR:
   case TID_WORD:
   case TID_SHORT:
      *((int *) arg) = va_arg(*arg_ptr, int);
      break;

   case TID_INT:
   case TID_BOOL:
      *((INT *) arg) = va_arg(*arg_ptr, INT);
      break;

   case TID_DWORD:
      *((DWORD *) arg) = va_arg(*arg_ptr, DWORD);
      break;

      /* float variables are passed as double by the compiler */
   case TID_FLOAT:
      *((float *) arg) = (float) va_arg(*arg_ptr, double);
      break;

   case TID_DOUBLE:
      *((double *) arg) = va_arg(*arg_ptr, double);
      break;

   case TID_ARRAY:
      *((char **) arg) = va_arg(*arg_ptr, char *);
      break;
   }
}

/********************************************************************/
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
   va_list ap, aptmp;
   char arg[8], arg_tmp[8];
   INT arg_type, transport, rpc_timeout;
   INT i, idx, status, rpc_index;
   INT param_size, arg_size, send_size;
   INT tid, flags;
   fd_set readfds;
   struct timeval timeout;
   char *param_ptr, str[80];
   BOOL bpointer, bbig;
   NET_COMMAND *nc;
   int send_sock;
   time_t start_time;

   idx = hConn - 1;

   if (_client_connection[idx].send_sock == 0) {
      cm_msg(MERROR, "rpc_client_call", "no rpc connection");
      return RPC_NO_CONNECTION;
   }

   send_sock = _client_connection[idx].send_sock;
   rpc_timeout = _client_connection[idx].rpc_timeout;
   transport = _client_connection[idx].transport;

   status = resize_net_send_buffer("rpc_client_call", NET_BUFFER_SIZE);
   if (status != SUCCESS)
      return RPC_EXCEED_BUFFER;

   nc = (NET_COMMAND *) _net_send_buffer;
   nc->header.routine_id = routine_id;

   if (transport == RPC_FTCP)
      nc->header.routine_id |= TCP_FAST;

   for (i = 0;; i++)
      if (rpc_list[i].id == routine_id || rpc_list[i].id == 0)
         break;
   rpc_index = i;
   if (rpc_list[i].id == 0) {
      sprintf(str, "invalid rpc ID (%d)", routine_id);
      cm_msg(MERROR, "rpc_client_call", str);
      return RPC_INVALID_ID;
   }

   /* examine variable argument list and convert it to parameter array */
   va_start(ap, routine_id);

   /* find out if we are on a big endian system */
   bbig = ((rpc_get_option(0, RPC_OHW_TYPE) & DRI_BIG_ENDIAN) > 0);

   for (i = 0, param_ptr = nc->param; rpc_list[rpc_index].param[i].tid != 0; i++) {
      tid = rpc_list[rpc_index].param[i].tid;
      flags = rpc_list[rpc_index].param[i].flags;

      bpointer = (flags & RPC_POINTER) || (flags & RPC_OUT) ||
          (flags & RPC_FIXARRAY) || (flags & RPC_VARARRAY) ||
          tid == TID_STRING || tid == TID_ARRAY || tid == TID_STRUCT || tid == TID_LINK;

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
      if (bbig) {
         if (tid == TID_BYTE || tid == TID_CHAR || tid == TID_SBYTE) {
            arg[0] = arg[3];
         }
         if (tid == TID_WORD || tid == TID_SHORT) {
            arg[0] = arg[2];
            arg[1] = arg[3];
         }
      }

      if (flags & RPC_IN) {
         if (bpointer)
            arg_size = tid_size[tid];
         else
            arg_size = tid_size[arg_type];

         /* for strings, the argument size depends on the string length */
         if (tid == TID_STRING || tid == TID_LINK)
            arg_size = 1 + strlen((char *) *((char **) arg));

         /* for varibale length arrays, the size is given by
            the next parameter on the stack */
         if (flags & RPC_VARARRAY) {
            memcpy(&aptmp, &ap, sizeof(ap));
            rpc_va_arg(&aptmp, TID_ARRAY, arg_tmp);

            if (flags & RPC_OUT)
               arg_size = *((INT *) * ((void **) arg_tmp));
            else
               arg_size = *((INT *) arg_tmp);

            *((INT *) param_ptr) = ALIGN8(arg_size);
            param_ptr += ALIGN8(sizeof(INT));
         }

         if (tid == TID_STRUCT || (flags & RPC_FIXARRAY))
            arg_size = rpc_list[rpc_index].param[i].n;

         /* always align parameter size */
         param_size = ALIGN8(arg_size);

         if ((POINTER_T) param_ptr - (POINTER_T) nc + param_size > _net_send_buffer_size) {
            cm_msg(MERROR, "rpc_client_call",
                   "parameters (%d) too large for network buffer (%d)",
                   (POINTER_T) param_ptr - (POINTER_T) nc + param_size, _net_send_buffer_size);
            return RPC_EXCEED_BUFFER;
         }

         if (bpointer)
            memcpy(param_ptr, (void *) *((void **) arg), arg_size);
         else {
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

   nc->header.param_size = (POINTER_T) param_ptr - (POINTER_T) nc->param;

   send_size = nc->header.param_size + sizeof(NET_COMMAND_HEADER);

   /* in FAST TCP mode, only send call and return immediately */
   if (transport == RPC_FTCP) {
      i = send_tcp(send_sock, (char *) nc, send_size, 0);

      if (i != send_size) {
         cm_msg(MERROR, "rpc_client_call", "send_tcp() failed");
         return RPC_NET_ERROR;
      }

      return RPC_SUCCESS;
   }

   /* in TCP mode, send and wait for reply on send socket */
   i = send_tcp(send_sock, (char *) nc, send_size, 0);
   if (i != send_size) {
      cm_msg(MERROR, "rpc_client_call",
             "send_tcp() failed, routine = \"%s\", host = \"%s\"",
             rpc_list[rpc_index].name, _client_connection[idx].host_name);
      return RPC_NET_ERROR;
   }

   /* make some timeout checking */
   if (rpc_timeout > 0) {
      start_time = ss_time();

      do {
         FD_ZERO(&readfds);
         FD_SET(send_sock, &readfds);

         timeout.tv_sec = 1;
         timeout.tv_usec = 0;

         status = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);

         if (status >= 0 && FD_ISSET(send_sock, &readfds))
            break;

         if (ss_time() - start_time > rpc_timeout / 1000)
            break;

      } while (status == -1 || status == 0);    /* continue again if signal was cought */

      if (!FD_ISSET(send_sock, &readfds)) {
         cm_msg(MERROR, "rpc_client_call",
                "rpc timeout after %d sec, routine = \"%s\", host = \"%s\", connection closed",
                (int) (ss_time() - start_time), rpc_list[rpc_index].name, _client_connection[idx].host_name);

         /* disconnect to avoid that the reply to this rpc_call comes at
            the next rpc_call */
         rpc_client_disconnect(hConn, FALSE);

         return RPC_TIMEOUT;
      }
   }

   /* receive result on send socket */
   i = recv_tcp(send_sock, _net_send_buffer, _net_send_buffer_size, 0);

   if (i <= 0) {
      cm_msg(MERROR, "rpc_client_call",
             "recv_tcp() failed, routine = \"%s\", host = \"%s\"",
             rpc_list[rpc_index].name, _client_connection[idx].host_name);
      return RPC_NET_ERROR;
   }

   /* extract result variables and place it to argument list */
   status = nc->header.routine_id;

   va_start(ap, routine_id);

   for (i = 0, param_ptr = nc->param; rpc_list[rpc_index].param[i].tid != 0; i++) {
      tid = rpc_list[rpc_index].param[i].tid;
      flags = rpc_list[rpc_index].param[i].flags;

      bpointer = (flags & RPC_POINTER) || (flags & RPC_OUT) ||
          (flags & RPC_FIXARRAY) || (flags & RPC_VARARRAY) ||
          tid == TID_STRING || tid == TID_ARRAY || tid == TID_STRUCT || tid == TID_LINK;

      if (bpointer)
         arg_type = TID_ARRAY;
      else
         arg_type = rpc_list[rpc_index].param[i].tid;

      if (tid == TID_FLOAT && !bpointer)
         arg_type = TID_DOUBLE;

      rpc_va_arg(&ap, arg_type, arg);

      if (rpc_list[rpc_index].param[i].flags & RPC_OUT) {
         tid = rpc_list[rpc_index].param[i].tid;
         flags = rpc_list[rpc_index].param[i].flags;

         arg_size = tid_size[tid];

         if (tid == TID_STRING || tid == TID_LINK)
            arg_size = strlen((char *) (param_ptr)) + 1;

         if (flags & RPC_VARARRAY) {
            arg_size = *((INT *) param_ptr);
            param_ptr += ALIGN8(sizeof(INT));
         }

         if (tid == TID_STRUCT || (flags & RPC_FIXARRAY))
            arg_size = rpc_list[rpc_index].param[i].n;

         /* return parameters are always pointers */
         if (*((char **) arg))
            memcpy((void *) *((char **) arg), param_ptr, arg_size);

         /* parameter size is always aligned */
         param_size = ALIGN8(arg_size);

         param_ptr += param_size;
      }
   }

   va_end(ap);

   return status;
}

/********************************************************************/
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
   va_list ap, aptmp;
   char arg[8], arg_tmp[8];
   INT arg_type, transport, rpc_timeout;
   INT i, idx, status;
   INT param_size, arg_size, send_size;
   INT tid, flags;
   fd_set readfds;
   struct timeval timeout;
   char *param_ptr, str[80];
   BOOL bpointer, bbig;
   NET_COMMAND *nc;
   int send_sock;
   time_t start_time;

   send_sock = _server_connection.send_sock;
   transport = _server_connection.transport;
   rpc_timeout = _server_connection.rpc_timeout;

   /* init network buffer */
   if (_net_send_buffer_size == 0) {
      status = resize_net_send_buffer("rpc_call", NET_BUFFER_SIZE);
      if (status != SUCCESS)
         return RPC_EXCEED_BUFFER;

      /* create a local mutex for multi-threaded applications */
      ss_mutex_create(&_mutex_rpc);
   }

   status = ss_mutex_wait_for(_mutex_rpc, 10000 + rpc_timeout);
   if (status != SS_SUCCESS) {
      cm_msg(MERROR, "rpc_call", "Mutex timeout");
      return RPC_MUTEX_TIMEOUT;
   }

   nc = (NET_COMMAND *) _net_send_buffer;
   nc->header.routine_id = routine_id;

   if (transport == RPC_FTCP)
      nc->header.routine_id |= TCP_FAST;

   for (i = 0;; i++)
      if (rpc_list[i].id == routine_id || rpc_list[i].id == 0)
         break;
   idx = i;
   if (rpc_list[i].id == 0) {
      ss_mutex_release(_mutex_rpc);
      sprintf(str, "invalid rpc ID (%d)", routine_id);
      cm_msg(MERROR, "rpc_call", str);
      return RPC_INVALID_ID;
   }

   /* examine variable argument list and convert it to parameter array */
   va_start(ap, routine_id);

   /* find out if we are on a big endian system */
   bbig = ((rpc_get_option(0, RPC_OHW_TYPE) & DRI_BIG_ENDIAN) > 0);

   for (i = 0, param_ptr = nc->param; rpc_list[idx].param[i].tid != 0; i++) {
      tid = rpc_list[idx].param[i].tid;
      flags = rpc_list[idx].param[i].flags;

      bpointer = (flags & RPC_POINTER) || (flags & RPC_OUT) ||
          (flags & RPC_FIXARRAY) || (flags & RPC_VARARRAY) ||
          tid == TID_STRING || tid == TID_ARRAY || tid == TID_STRUCT || tid == TID_LINK;

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
      if (bbig) {
         if (tid == TID_BYTE || tid == TID_CHAR || tid == TID_SBYTE) {
            arg[0] = arg[3];
         }
         if (tid == TID_WORD || tid == TID_SHORT) {
            arg[0] = arg[2];
            arg[1] = arg[3];
         }
      }

      if (flags & RPC_IN) {
         if (bpointer)
            arg_size = tid_size[tid];
         else
            arg_size = tid_size[arg_type];

         /* for strings, the argument size depends on the string length */
         if (tid == TID_STRING || tid == TID_LINK)
            arg_size = 1 + strlen((char *) *((char **) arg));

         /* for varibale length arrays, the size is given by
            the next parameter on the stack */
         if (flags & RPC_VARARRAY) {
            memcpy(&aptmp, &ap, sizeof(ap));
            rpc_va_arg(&aptmp, TID_ARRAY, arg_tmp);

            if (flags & RPC_OUT)
               arg_size = *((INT *) * ((void **) arg_tmp));
            else
               arg_size = *((INT *) arg_tmp);

            *((INT *) param_ptr) = ALIGN8(arg_size);
            param_ptr += ALIGN8(sizeof(INT));
         }

         if (tid == TID_STRUCT || (flags & RPC_FIXARRAY))
            arg_size = rpc_list[idx].param[i].n;

         /* always align parameter size */
         param_size = ALIGN8(arg_size);

         if ((POINTER_T) param_ptr - (POINTER_T) nc + param_size > _net_send_buffer_size) {
            ss_mutex_release(_mutex_rpc);
            cm_msg(MERROR, "rpc_call",
                   "parameters (%d) too large for network buffer (%d)",
                   (POINTER_T) param_ptr - (POINTER_T) nc + param_size, _net_send_buffer_size);
            return RPC_EXCEED_BUFFER;
         }

         if (bpointer)
            memcpy(param_ptr, (void *) *((void **) arg), arg_size);
         else {
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

   nc->header.param_size = (POINTER_T) param_ptr - (POINTER_T) nc->param;

   send_size = nc->header.param_size + sizeof(NET_COMMAND_HEADER);

   /* in FAST TCP mode, only send call and return immediately */
   if (transport == RPC_FTCP) {
      i = send_tcp(send_sock, (char *) nc, send_size, 0);

      if (i != send_size) {
         ss_mutex_release(_mutex_rpc);
         cm_msg(MERROR, "rpc_call", "send_tcp() failed");
         return RPC_NET_ERROR;
      }

      ss_mutex_release(_mutex_rpc);
      return RPC_SUCCESS;
   }

   /* in TCP mode, send and wait for reply on send socket */
   i = send_tcp(send_sock, (char *) nc, send_size, 0);
   if (i != send_size) {
      ss_mutex_release(_mutex_rpc);
      cm_msg(MERROR, "rpc_call", "send_tcp() failed");
      return RPC_NET_ERROR;
   }

   /* make some timeout checking */
   if (rpc_timeout > 0) {
      start_time = ss_time();

      do {
         FD_ZERO(&readfds);
         FD_SET(send_sock, &readfds);

         timeout.tv_sec = 1;
         timeout.tv_usec = 0;

         status = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);

         if (FD_ISSET(send_sock, &readfds))
            break;

         if (ss_time() - start_time > rpc_timeout / 1000)
            break;

      } while (status == -1 || status == 0);    /* continue again if signal was cought */

      if (!FD_ISSET(send_sock, &readfds)) {
         ss_mutex_release(_mutex_rpc);
         cm_msg(MERROR, "rpc_call", "rpc timeout after %d sec, routine = \"%s\", program abort",
                (int) (ss_time() - start_time), rpc_list[idx].name);
         abort();
      }
   }

   /* receive result on send socket */
   i = recv_tcp(send_sock, _net_send_buffer, _net_send_buffer_size, 0);

   if (i <= 0) {
      ss_mutex_release(_mutex_rpc);
      cm_msg(MERROR, "rpc_call", "recv_tcp() failed, routine = \"%s\"", rpc_list[idx].name);
      return RPC_NET_ERROR;
   }

   /* extract result variables and place it to argument list */
   status = nc->header.routine_id;

   va_start(ap, routine_id);

   for (i = 0, param_ptr = nc->param; rpc_list[idx].param[i].tid != 0; i++) {
      tid = rpc_list[idx].param[i].tid;
      flags = rpc_list[idx].param[i].flags;

      bpointer = (flags & RPC_POINTER) || (flags & RPC_OUT) ||
          (flags & RPC_FIXARRAY) || (flags & RPC_VARARRAY) ||
          tid == TID_STRING || tid == TID_ARRAY || tid == TID_STRUCT || tid == TID_LINK;

      if (bpointer)
         arg_type = TID_ARRAY;
      else
         arg_type = rpc_list[idx].param[i].tid;

      if (tid == TID_FLOAT && !bpointer)
         arg_type = TID_DOUBLE;

      rpc_va_arg(&ap, arg_type, arg);

      if (rpc_list[idx].param[i].flags & RPC_OUT) {
         tid = rpc_list[idx].param[i].tid;
         arg_size = tid_size[tid];

         if (tid == TID_STRING || tid == TID_LINK)
            arg_size = strlen((char *) (param_ptr)) + 1;

         if (flags & RPC_VARARRAY) {
            arg_size = *((INT *) param_ptr);
            param_ptr += ALIGN8(sizeof(INT));
         }

         if (tid == TID_STRUCT || (flags & RPC_FIXARRAY))
            arg_size = rpc_list[idx].param[i].n;

         /* return parameters are always pointers */
         if (*((char **) arg))
            memcpy((void *) *((char **) arg), param_ptr, arg_size);

         /* parameter size is always aligned */
         param_size = ALIGN8(arg_size);

         param_ptr += param_size;
      }
   }

   va_end(ap);

   ss_mutex_release(_mutex_rpc);

   return status;
}


/********************************************************************/
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

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Fast send_event routine which bypasses the RPC layer and
           sends the event directly at the TCP level.
@param buffer_handle      Handle of the buffer to send the event to.
                          Must be obtained via bm_open_buffer.
@param source             Address of the event to send. It must have
                          a proper event header.
@param buf_size           Size of event in bytes with header.
@param async_flag         SYNC / ASYNC flag. In ASYNC mode, the
                          function returns immediately if it cannot
                          send the event over the network. In SYNC
                          mode, it waits until the packet is sent
                          (blocking).
@param mode               Determines in which mode the event is sent.
                          If zero, use RPC socket, if one, use special
                          event socket to bypass RPC layer on the
                          server side.

@return BM_INVALID_PARAM, BM_ASYNC_RETURN, RPC_SUCCESS, RPC_NET_ERROR,
        RPC_NO_CONNECTION, RPC_EXCEED_BUFFER
*/
INT rpc_send_event(INT buffer_handle, void *source, INT buf_size, INT async_flag, INT mode)
{
   INT i;
   NET_COMMAND *nc;
   unsigned long flag;
   BOOL would_block = 0;
   DWORD aligned_buf_size;

   aligned_buf_size = ALIGN8(buf_size);
   _rpc_sock = mode == 0 ? _server_connection.send_sock : _server_connection.event_sock;

   if ((INT) aligned_buf_size != (INT) (ALIGN8(((EVENT_HEADER *) source)->data_size + sizeof(EVENT_HEADER)))) {
      cm_msg(MERROR, "rpc_send_event", "event size mismatch");
      return BM_INVALID_PARAM;
   }

   if (!rpc_is_remote())
      return bm_send_event(buffer_handle, source, buf_size, async_flag);

   /* init network buffer */
   if (!_tcp_buffer)
      _tcp_buffer = (char *) M_MALLOC(NET_TCP_SIZE);
   if (!_tcp_buffer) {
      cm_msg(MERROR, "rpc_send_event", "not enough memory to allocate network buffer");
      return RPC_EXCEED_BUFFER;
   }

   /* check if not enough space in TCP buffer */
   if (aligned_buf_size + 4 * 8 + sizeof(NET_COMMAND_HEADER) >= (DWORD) (_opt_tcp_size - _tcp_wp)
       && _tcp_wp != _tcp_rp) {
      /* set socket to nonblocking IO */
      if (async_flag == ASYNC) {
         flag = 1;
#ifdef OS_VXWORKS
         ioctlsocket(_rpc_sock, FIONBIO, (int) &flag);
#else
         ioctlsocket(_rpc_sock, FIONBIO, &flag);
#endif
      }

      i = send_tcp(_rpc_sock, _tcp_buffer + _tcp_rp, _tcp_wp - _tcp_rp, 0);

      if (i < 0)
#ifdef OS_WINNT
         would_block = (WSAGetLastError() == WSAEWOULDBLOCK);
#else
         would_block = (errno == EWOULDBLOCK);
#endif

      /* set socket back to blocking IO */
      if (async_flag == ASYNC) {
         flag = 0;
#ifdef OS_VXWORKS
         ioctlsocket(_rpc_sock, FIONBIO, (int) &flag);
#else
         ioctlsocket(_rpc_sock, FIONBIO, &flag);
#endif
      }

      /* increment read pointer */
      if (i > 0)
         _tcp_rp += i;

      /* check if whole buffer is sent */
      if (_tcp_rp == _tcp_wp)
         _tcp_rp = _tcp_wp = 0;

      if (i < 0 && !would_block) {
         cm_msg(MERROR, "rpc_send_event", "send_tcp() failed, return code = %d", i);
         return RPC_NET_ERROR;
      }

      /* return if buffer is not emptied */
      if (_tcp_wp > 0)
         return BM_ASYNC_RETURN;
   }

   if (mode == 0) {
      nc = (NET_COMMAND *) (_tcp_buffer + _tcp_wp);
      nc->header.routine_id = RPC_BM_SEND_EVENT | TCP_FAST;
      nc->header.param_size = 4 * 8 + aligned_buf_size;

      /* assemble parameters manually */
      *((INT *) (&nc->param[0])) = buffer_handle;
      *((INT *) (&nc->param[8])) = buf_size;

      /* send events larger than optimal buffer size directly */
      if (aligned_buf_size + 4 * 8 + sizeof(NET_COMMAND_HEADER) >= (DWORD) _opt_tcp_size) {
         /* send header */
         i = send_tcp(_rpc_sock, _tcp_buffer + _tcp_wp, sizeof(NET_COMMAND_HEADER) + 16, 0);
         if (i <= 0) {
            cm_msg(MERROR, "rpc_send_event", "send_tcp() failed, return code = %d", i);
            return RPC_NET_ERROR;
         }

         /* send data */
         i = send_tcp(_rpc_sock, (char *) source, aligned_buf_size, 0);
         if (i <= 0) {
            cm_msg(MERROR, "rpc_send_event", "send_tcp() failed, return code = %d", i);
            return RPC_NET_ERROR;
         }

         /* send last two parameters */
         *((INT *) (&nc->param[0])) = buf_size;
         *((INT *) (&nc->param[8])) = 0;
         i = send_tcp(_rpc_sock, &nc->param[0], 16, 0);
         if (i <= 0) {
            cm_msg(MERROR, "rpc_send_event", "send_tcp() failed, return code = %d", i);
            return RPC_NET_ERROR;
         }
      } else {
         /* copy event */
         memcpy(&nc->param[16], source, buf_size);

         /* last two parameters (buf_size and async_flag */
         *((INT *) (&nc->param[16 + aligned_buf_size])) = buf_size;
         *((INT *) (&nc->param[24 + aligned_buf_size])) = 0;

         _tcp_wp += nc->header.param_size + sizeof(NET_COMMAND_HEADER);
      }

   } else {

      /* send events larger than optimal buffer size directly */
      if (aligned_buf_size + 4 * 8 + sizeof(INT) >= (DWORD) _opt_tcp_size) {
         /* send buffer */
         i = send_tcp(_rpc_sock, (char *) &buffer_handle, sizeof(INT), 0);
         if (i <= 0) {
            cm_msg(MERROR, "rpc_send_event", "send_tcp() failed, return code = %d", i);
            return RPC_NET_ERROR;
         }

         /* send data */
         i = send_tcp(_rpc_sock, (char *) source, aligned_buf_size, 0);
         if (i <= 0) {
            cm_msg(MERROR, "rpc_send_event", "send_tcp() failed, return code = %d", i);
            return RPC_NET_ERROR;
         }
      } else {
         /* copy event */
         *((INT *) (_tcp_buffer + _tcp_wp)) = buffer_handle;
         _tcp_wp += sizeof(INT);
         memcpy(_tcp_buffer + _tcp_wp, source, buf_size);

         _tcp_wp += aligned_buf_size;
      }
   }

   return RPC_SUCCESS;
}


/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************/
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


/********************************************************************/
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

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Send event residing in the TCP cache buffer filled by
           rpc_send_event. This routine should be called when a
           run is stopped.

@return RPC_SUCCESS, RPC_NET_ERROR
*/
INT rpc_flush_event()
{
   INT i;

   if (!rpc_is_remote())
      return RPC_SUCCESS;

   /* return if rpc_send_event was not called */
   if (!_tcp_buffer || _tcp_wp == 0)
      return RPC_SUCCESS;

   /* empty TCP buffer */
   if (_tcp_wp > 0) {
      i = send_tcp(_rpc_sock, _tcp_buffer + _tcp_rp, _tcp_wp - _tcp_rp, 0);

      if (i != _tcp_wp - _tcp_rp) {
         cm_msg(MERROR, "rpc_flush_event", "send_tcp() failed");
         return RPC_NET_ERROR;
      }
   }

   _tcp_rp = _tcp_wp = 0;

   return RPC_SUCCESS;
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************/

typedef struct {
   int transition;
   int run_number;
   time_t trans_time;
   int sequence_number;
} TR_FIFO;

static TR_FIFO tr_fifo[10];
static int trf_wp, trf_rp;

static INT rpc_transition_dispatch(INT idx, void *prpc_param[])
/********************************************************************\

  Routine: rpc_transition_dispatch

  Purpose: Gets called when a transition function was registered and
           a transition occured. Internal use only.

  Input:
    INT    idx              RPC function ID
    void   *prpc_param      RPC parameters

  Output:
    none

  Function value:
    INT    return value from called user routine

\********************************************************************/
{
   INT status, i;

   /* erase error string */
   *(CSTRING(2)) = 0;

   if (idx == RPC_RC_TRANSITION) {
      for (i = 0; i < MAX_TRANSITIONS; i++)
         if (_trans_table[i].transition == CINT(0) && _trans_table[i].sequence_number == CINT(4))
            break;

      /* call registerd function */
      if (i < MAX_TRANSITIONS) {
         if (_trans_table[i].func)
            /* execute callback if defined */
            status = _trans_table[i].func(CINT(1), CSTRING(2));
         else {
            /* store transition in FIFO */
            tr_fifo[trf_wp].transition = CINT(0);
            tr_fifo[trf_wp].run_number = CINT(1);
            tr_fifo[trf_wp].trans_time = time(NULL);
            tr_fifo[trf_wp].sequence_number = CINT(4);
            trf_wp = (trf_wp + 1) % 10;
            status = RPC_SUCCESS;
         }
      } else
         status = RPC_SUCCESS;

   } else {
      cm_msg(MERROR, "rpc_transition_dispatch", "received unrecognized command");
      status = RPC_INVALID_ID;
   }

   return status;
}

/********************************************************************/
int cm_query_transition(int *transition, int *run_number, int *trans_time)
/********************************************************************\

  Routine: cm_query_transition

  Purpose: Query system if transition has occured. Normally, one
           registers callbacks for transitions via
           cm_register_transition. In some environments however,
           callbacks are not possible. In that case one spciefies
           a NULL pointer as the callback routine and can query
           transitions "manually" by calling this functions. A small
           FIFO takes care that no transition is lost if this functions
           did not get called between some transitions.

  Output:
    INT   *transition        Type of transition, one of TR_xxx
    INT   *run_nuber         Run number for transition
    time_t *trans_time       Time (in UNIX time) of transition

  Function value:
    FALSE  No transition occured since last call
    TRUE   Transition occured

\********************************************************************/
{

   if (trf_wp == trf_rp)
      return FALSE;

   if (transition)
      *transition = tr_fifo[trf_rp].transition;

   if (run_number)
      *run_number = tr_fifo[trf_rp].run_number;

   if (trans_time)
      *trans_time = (int) tr_fifo[trf_rp].trans_time;

   trf_rp = (trf_rp + 1) % 10;

   return TRUE;
}

/********************************************************************\
*                        server functions                            *
\********************************************************************/

#if 0
void debug_dump(unsigned char *p, int size)
{
   int i, j;
   unsigned char c;

   for (i = 0; i < (size - 1) / 16 + 1; i++) {
      printf("%p ", p + i * 16);
      for (j = 0; j < 16; j++)
         if (i * 16 + j < size)
            printf("%02X ", p[i * 16 + j]);
         else
            printf("   ");
      printf(" ");

      for (j = 0; j < 16; j++) {
         c = p[i * 16 + j];
         if (i * 16 + j < size)
            printf("%c", (c >= 32 && c < 128) ? p[i * 16 + j] : '.');
      }
      printf("\n");
   }

   printf("\n");
}
#endif

/********************************************************************/
INT recv_tcp_server(INT idx, char *buffer, DWORD buffer_size, INT flags, INT * remaining)
/********************************************************************\

  Routine: recv_tcp_server

  Purpose: TCP receive routine with local cache. To speed up network
           performance, a 64k buffer is read in at once and split into
           several RPC command on successive calls to recv_tcp_server.
           Therefore, the number of recv() calls is minimized.

           This routine is ment to be called by the server process.
           Clients should call recv_tcp instead.

  Input:
    INT   idx                Index of server connection
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
   INT size, param_size;
   NET_COMMAND *nc;
   INT write_ptr, read_ptr, misalign;
   char *net_buffer;
   INT copied, status;
   INT sock;

   sock = _server_acception[idx].recv_sock;

   if (flags & MSG_PEEK) {
      status = recv(sock, buffer, buffer_size, flags);
      if (status == -1)
         cm_msg(MERROR, "recv_tcp_server",
                "recv(%d,MSG_PEEK) returned %d, errno: %d (%s)", buffer_size, status, errno, strerror(errno));
      return status;
   }

   if (!_server_acception[idx].net_buffer) {
      if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_REMOTE)
         _server_acception[idx].net_buffer_size = NET_TCP_SIZE;
      else
         _server_acception[idx].net_buffer_size = NET_BUFFER_SIZE;

      _server_acception[idx].net_buffer = (char *) M_MALLOC(_server_acception[idx].net_buffer_size);
      _server_acception[idx].write_ptr = 0;
      _server_acception[idx].read_ptr = 0;
      _server_acception[idx].misalign = 0;
   }
   if (!_server_acception[idx].net_buffer) {
      cm_msg(MERROR, "recv_tcp_server", "Cannot allocate %d bytes for network buffer",
             _server_acception[idx].net_buffer_size);
      return -1;
   }

   if (buffer_size < sizeof(NET_COMMAND_HEADER)) {
      cm_msg(MERROR, "recv_tcp_server", "parameters too large for network buffer");
      return -1;
   }

   copied = 0;
   param_size = -1;

   write_ptr = _server_acception[idx].write_ptr;
   read_ptr = _server_acception[idx].read_ptr;
   misalign = _server_acception[idx].misalign;
   net_buffer = _server_acception[idx].net_buffer;

   do {
      if (write_ptr - read_ptr >= (INT) sizeof(NET_COMMAND_HEADER) - copied) {
         if (param_size == -1) {
            if (copied > 0) {
               /* assemble split header */
               memcpy(buffer + copied, net_buffer + read_ptr, (INT) sizeof(NET_COMMAND_HEADER) - copied);
               nc = (NET_COMMAND *) (buffer);
            } else
               nc = (NET_COMMAND *) (net_buffer + read_ptr);

            param_size = (INT) nc->header.param_size;

            if (_server_acception[idx].convert_flags)
               rpc_convert_single(&param_size, TID_DWORD, 0, _server_acception[idx].convert_flags);
         }

         /* check if parameters fit in buffer */
         if (buffer_size < param_size + sizeof(NET_COMMAND_HEADER)) {
            cm_msg(MERROR, "recv_tcp_server", "parameters too large for network buffer");
            _server_acception[idx].read_ptr = _server_acception[idx].write_ptr = 0;
            return -1;
         }

         /* check if we have all parameters in buffer */
         if (write_ptr - read_ptr >= param_size + (INT) sizeof(NET_COMMAND_HEADER) - copied)
            break;
      }

      /* not enough data, so copy partially and get new */
      size = write_ptr - read_ptr;

      if (size > 0) {
         memcpy(buffer + copied, net_buffer + read_ptr, size);
         copied += size;
         read_ptr = write_ptr;
      }
#ifdef OS_UNIX
      do {
         write_ptr = recv(sock, net_buffer + misalign, _server_acception[idx].net_buffer_size - 8, flags);

         /* don't return if an alarm signal was cought */
      } while (write_ptr == -1 && errno == EINTR);
#else
      write_ptr = recv(sock, net_buffer + misalign, _server_acception[idx].net_buffer_size - 8, flags);
#endif

      /* abort if connection broken */
      if (write_ptr <= 0) {
         if (write_ptr == 0)
            cm_msg(MERROR, "recv_tcp_server", "rpc connection from \'%s\' on \'%s\' unexpectedly closed",
                   _server_acception[idx].prog_name, _server_acception[idx].host_name);
         else
            cm_msg(MERROR, "recv_tcp_server", "recv() returned %d, errno: %d (%s)", write_ptr, errno,
                   strerror(errno));

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

   if (remaining) {
      /* don't keep rpc_server_receive in an infinite loop */
      if (write_ptr - read_ptr < param_size)
         *remaining = 0;
      else
         *remaining = write_ptr - read_ptr;
   }

   _server_acception[idx].write_ptr = write_ptr;
   _server_acception[idx].read_ptr = read_ptr;
   _server_acception[idx].misalign = misalign;

   return size + copied;
}


/********************************************************************/
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
   INT idx;

   /* figure out to which connection socket belongs */
   for (idx = 0; idx < MAX_RPC_CONNECTION; idx++)
      if (_server_acception[idx].recv_sock == sock)
         break;

   return _server_acception[idx].write_ptr - _server_acception[idx].read_ptr;
}


/********************************************************************/
INT recv_event_server(INT idx, char *buffer, DWORD buffer_size, INT flags, INT * remaining)
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
    INT   idx                Index of server connection
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
   INT size, event_size, aligned_event_size = 0, *pbh, header_size;
   EVENT_HEADER *pevent;
   INT write_ptr, read_ptr, misalign;
   char *net_buffer;
   INT copied, status;
   INT sock;
   RPC_SERVER_ACCEPTION *psa;

   psa = &_server_acception[idx];
   sock = psa->event_sock;

   //printf("recv_event_server: idx %d, buffer %p, buffer_size %d\n", idx, buffer, buffer_size);

   if (flags & MSG_PEEK) {
      status = recv(sock, buffer, buffer_size, flags);
      if (status == -1)
         cm_msg(MERROR, "recv_event_server",
                "recv(%d,MSG_PEEK) returned %d, errno: %d (%s)", buffer_size, status, errno, strerror(errno));
      return status;
   }

   if (!psa->ev_net_buffer) {
      if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_REMOTE)
         psa->net_buffer_size = NET_TCP_SIZE;
      else
         psa->net_buffer_size = NET_BUFFER_SIZE;

      psa->ev_net_buffer = (char *) M_MALLOC(psa->net_buffer_size);

      //printf("allocate %p size %d\n", psa->ev_net_buffer, psa->net_buffer_size);

      psa->ev_write_ptr = 0;
      psa->ev_read_ptr = 0;
      psa->ev_misalign = 0;
   }
   if (!psa->ev_net_buffer) {
      cm_msg(MERROR, "recv_event_server", "Cannot allocate %d bytes for network buffer",
             psa->net_buffer_size);
      return -1;
   }

   header_size = (INT) (sizeof(EVENT_HEADER) + sizeof(INT));

   if ((INT) buffer_size < header_size) {
      cm_msg(MERROR, "recv_event_server", "parameters too large for network buffer");
      return -1;
   }

   copied = 0;
   event_size = -1;

   write_ptr = psa->ev_write_ptr;
   read_ptr = psa->ev_read_ptr;
   misalign = psa->ev_misalign;
   net_buffer = psa->ev_net_buffer;

   do {
      if (write_ptr - read_ptr >= header_size - copied) {
         if (event_size == -1) {
            if (copied > 0) {
               /* assemble split header */
               memcpy(buffer + copied, net_buffer + read_ptr, header_size - copied);
               pbh = (INT *) buffer;
            } else
               pbh = (INT *) (net_buffer + read_ptr);

            pevent = (EVENT_HEADER *) (pbh + 1);

            event_size = pevent->data_size;
            if (psa->convert_flags)
               rpc_convert_single(&event_size, TID_DWORD, 0, psa->convert_flags);

            aligned_event_size = ALIGN8(event_size);
         }

         /* check if data part fits in buffer */
         if ((INT) buffer_size < aligned_event_size + header_size) {
            cm_msg(MERROR, "recv_event_server", "event size %d too large for network buffer size %d", aligned_event_size + header_size, buffer_size);
            psa->ev_read_ptr = psa->ev_write_ptr = 0;
            return -1;
         }

         /* check if we have whole event in buffer */
         if (write_ptr - read_ptr >= aligned_event_size + header_size - copied)
            break;
      }

      /* not enough data, so copy partially and get new */
      size = write_ptr - read_ptr;

      if (size > 0) {
         memcpy(buffer + copied, net_buffer + read_ptr, size);
         copied += size;
         read_ptr = write_ptr;
      }
#ifdef OS_UNIX
      do {
         write_ptr = recv(sock, net_buffer + misalign, psa->net_buffer_size - 8, flags);

         /* don't return if an alarm signal was cought */
      } while (write_ptr == -1 && errno == EINTR);
#else
      write_ptr = recv(sock, net_buffer + misalign, psa->net_buffer_size - 8, flags);
#endif

      /* abort if connection broken */
      if (write_ptr <= 0) {
         cm_msg(MERROR, "recv_event_server", "recv() returned %d, errno: %d (%s)", write_ptr, errno,
                strerror(errno));

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
   if (size > 0) {
      memcpy(buffer + copied, net_buffer + read_ptr, size);
      read_ptr += size;
   }

   if (remaining)
      *remaining = write_ptr - read_ptr;

   psa->ev_write_ptr = write_ptr;
   psa->ev_read_ptr = read_ptr;
   psa->ev_misalign = misalign;

   /* convert header little endian/big endian */
   if (psa->convert_flags) {
      pevent = (EVENT_HEADER *) (((INT *) buffer) + 1);

      rpc_convert_single(buffer, TID_INT, 0, psa->convert_flags);
      rpc_convert_single(&pevent->event_id, TID_SHORT, 0, psa->convert_flags);
      rpc_convert_single(&pevent->trigger_mask, TID_SHORT, 0, psa->convert_flags);
      rpc_convert_single(&pevent->serial_number, TID_DWORD, 0, psa->convert_flags);
      rpc_convert_single(&pevent->time_stamp, TID_DWORD, 0, psa->convert_flags);
      rpc_convert_single(&pevent->data_size, TID_DWORD, 0, psa->convert_flags);
   }

   return header_size + event_size;
}


/********************************************************************/
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
   INT idx;

   /* figure out to which connection socket belongs */
   for (idx = 0; idx < MAX_RPC_CONNECTION; idx++)
      if (_server_acception[idx].event_sock == sock)
         break;

   return _server_acception[idx].ev_write_ptr - _server_acception[idx].ev_read_ptr;
}


/********************************************************************/
INT rpc_register_server(INT server_type, const char *name, INT * port, INT(*func) (INT, void **))
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
   struct sockaddr_in bind_addr;
   INT status, flag;
   unsigned int size;

#ifdef OS_WINNT
   {
      WSADATA WSAData;

      /* Start windows sockets */
      if (WSAStartup(MAKEWORD(1, 1), &WSAData) != 0)
         return RPC_NET_ERROR;
   }
#endif

   rpc_set_server_option(RPC_OSERVER_TYPE, server_type);

   /* register system functions */
   rpc_register_functions(rpc_get_internal_list(0), func);

   if (name != NULL)
      rpc_set_server_option(RPC_OSERVER_NAME, (POINTER_T) name);

   /* in subprocess mode, don't start listener */
   if (server_type == ST_SUBPROCESS)
      return RPC_SUCCESS;

   /* create a socket for listening */
   _lsock = socket(AF_INET, SOCK_STREAM, 0);
   if (_lsock == -1) {
      cm_msg(MERROR, "rpc_register_server", "socket(AF_INET, SOCK_STREAM) failed, errno %d (%s)", errno,
             strerror(errno));
      return RPC_NET_ERROR;
   }

   /* set close-on-exec flag to prevent child mserver processes from inheriting the listen socket */
#if defined(F_SETFD) && defined(FD_CLOEXEC)
   status = fcntl(_lsock, F_SETFD, fcntl(_lsock, F_GETFD) | FD_CLOEXEC);
   if (status < 0) {
      cm_msg(MERROR, "rpc_register_server", "fcntl(F_SETFD, FD_CLOEXEC) failed, errno %d (%s)", errno,
             strerror(errno));
      return RPC_NET_ERROR;
   }
#endif

   /* reuse address, needed if previous server stopped (30s timeout!) */
   flag = 1;
   status = setsockopt(_lsock, SOL_SOCKET, SO_REUSEADDR, (char *) &flag, sizeof(INT));
   if (status < 0) {
      cm_msg(MERROR, "rpc_register_server", "setsockopt(SO_REUSEADDR) failed, errno %d (%s)", errno,
             strerror(errno));
      return RPC_NET_ERROR;
   }

   /* bind local node name and port to socket */
   memset(&bind_addr, 0, sizeof(bind_addr));
   bind_addr.sin_family = AF_INET;
   bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);

   if (!port)
      bind_addr.sin_port = htons(MIDAS_TCP_PORT);
   else
      bind_addr.sin_port = htons((short) (*port));

   status = bind(_lsock, (struct sockaddr *) &bind_addr, sizeof(bind_addr));
   if (status < 0) {
      cm_msg(MERROR, "rpc_register_server", "bind() failed, errno %d (%s)", errno, strerror(errno));
      return RPC_NET_ERROR;
   }

   /* listen for connection */
#ifdef OS_MSDOS
   status = listen(_lsock, 1);
#else
   status = listen(_lsock, SOMAXCONN);
#endif
   if (status < 0) {
      cm_msg(MERROR, "rpc_register_server", "listen() failed, errno %d (%s)", errno, strerror(errno));
      return RPC_NET_ERROR;
   }

   /* return port wich OS has choosen */
   if (port && *port == 0) {
      size = sizeof(bind_addr);
#ifdef OS_WINNT
      getsockname(_lsock, (struct sockaddr *) &bind_addr, (int *) &size);
#else
      getsockname(_lsock, (struct sockaddr *) &bind_addr, &size);
#endif
      *port = ntohs(bind_addr.sin_port);
   }

   /* define callbacks for ss_suspend */
   if (server_type == ST_REMOTE)
      ss_suspend_set_dispatch(CH_LISTEN, &_lsock, (int (*)(void)) rpc_client_accept);
   else
      ss_suspend_set_dispatch(CH_LISTEN, &_lsock, (int (*)(void)) rpc_server_accept);

   return RPC_SUCCESS;
}

typedef struct {
   int tid;
   int buffer_size;
   char *buffer;
} TLS_POINTER;

TLS_POINTER *tls_buffer = NULL;
int tls_size = 0;

/********************************************************************/
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
   INT i, idx, routine_id, status;
   char *in_param_ptr, *out_param_ptr, *last_param_ptr;
   INT tid, flags;
   NET_COMMAND *nc_in, *nc_out;
   INT param_size, max_size;
   void *prpc_param[20];
   char str[1024], debug_line[1024], *return_buffer;
   int return_buffer_size;
   int return_buffer_tls;
#ifdef FIXED_BUFFER
   int initial_buffer_size = NET_BUFFER_SIZE;
#else
   int initial_buffer_size = 1024;
#endif

   /* return buffer must must use thread local storage multi-thread servers */
   if (!tls_size) {
      tls_buffer = (TLS_POINTER *) malloc(sizeof(TLS_POINTER));
      tls_buffer[tls_size].tid = ss_gettid();
      tls_buffer[tls_size].buffer_size = initial_buffer_size;
      tls_buffer[tls_size].buffer = (char *) malloc(tls_buffer[tls_size].buffer_size);
      tls_size = 1;
   }
   for (i = 0; i < tls_size; i++)
      if (tls_buffer[i].tid == ss_gettid())
         break;
   if (i == tls_size) {
      /* new thread -> allocate new buffer */
      tls_buffer = (TLS_POINTER *) realloc(tls_buffer, (tls_size + 1) * sizeof(TLS_POINTER));
      tls_buffer[tls_size].tid = ss_gettid();
      tls_buffer[tls_size].buffer_size = initial_buffer_size;
      tls_buffer[tls_size].buffer = (char *) malloc(tls_buffer[tls_size].buffer_size);
      tls_size++;
   }

   return_buffer_tls = i;
   return_buffer_size = tls_buffer[i].buffer_size;
   return_buffer = tls_buffer[i].buffer;
   assert(return_buffer);

   /* extract pointer array to parameters */
   nc_in = (NET_COMMAND *) buffer;

   /* convert header format (byte swapping) */
   if (convert_flags) {
      rpc_convert_single(&nc_in->header.routine_id, TID_DWORD, 0, convert_flags);
      rpc_convert_single(&nc_in->header.param_size, TID_DWORD, 0, convert_flags);
   }

   /* no result return in FAST TCP mode */
   if (nc_in->header.routine_id & TCP_FAST)
      sock = 0;

   /* find entry in rpc_list */
   routine_id = nc_in->header.routine_id & ~TCP_FAST;

   assert(rpc_list != NULL);

   for (i = 0;; i++)
      if (rpc_list[i].id == 0 || rpc_list[i].id == routine_id)
         break;
   idx = i;
   if (rpc_list[i].id == 0) {
      cm_msg(MERROR, "rpc_execute", "Invalid rpc ID (%d)", routine_id);
      return RPC_INVALID_ID;
   }

 again:

   in_param_ptr = nc_in->param;

   nc_out = (NET_COMMAND *) return_buffer;
   out_param_ptr = nc_out->param;

   sprintf(debug_line, "%s(", rpc_list[idx].name);

   for (i = 0; rpc_list[idx].param[i].tid != 0; i++) {
      tid = rpc_list[idx].param[i].tid;
      flags = rpc_list[idx].param[i].flags;

      if (flags & RPC_IN) {
         param_size = ALIGN8(tid_size[tid]);

         if (tid == TID_STRING || tid == TID_LINK)
            param_size = ALIGN8(1 + strlen((char *) (in_param_ptr)));

         if (flags & RPC_VARARRAY) {
            /* for arrays, the size is stored as a INT in front of the array */
            param_size = *((INT *) in_param_ptr);
            if (convert_flags)
               rpc_convert_single(&param_size, TID_INT, 0, convert_flags);
            param_size = ALIGN8(param_size);

            in_param_ptr += ALIGN8(sizeof(INT));
         }

         if (tid == TID_STRUCT)
            param_size = ALIGN8(rpc_list[idx].param[i].n);

         prpc_param[i] = in_param_ptr;

         /* convert data format */
         if (convert_flags) {
            if (flags & RPC_VARARRAY)
               rpc_convert_data(in_param_ptr, tid, flags, param_size, convert_flags);
            else
               rpc_convert_data(in_param_ptr, tid, flags, rpc_list[idx].param[i].n * tid_size[tid],
                                convert_flags);
         }

         db_sprintf(str, in_param_ptr, param_size, 0, rpc_list[idx].param[i].tid);
         if (rpc_list[idx].param[i].tid == TID_STRING) {
            /* check for long strings (db_create_record...) */
            if (strlen(debug_line) + strlen(str) + 2 < sizeof(debug_line)) {
               strcat(debug_line, "\"");
               strcat(debug_line, str);
               strcat(debug_line, "\"");
            } else
               strcat(debug_line, "...");
         } else
            strcat(debug_line, str);

         in_param_ptr += param_size;
      }

      if (flags & RPC_OUT) {
         param_size = ALIGN8(tid_size[tid]);

         if (flags & RPC_VARARRAY || tid == TID_STRING) {

            /* save maximum array length from the value of the next argument.
             * this means RPC_OUT arrays and strings should always be passed like this:
             * rpc_call(..., array_ptr, array_max_size, ...); */

            max_size = *((INT *) in_param_ptr);

            if (convert_flags)
               rpc_convert_single(&max_size, TID_INT, 0, convert_flags);
            max_size = ALIGN8(max_size);

            *((INT *) out_param_ptr) = max_size;

            /* save space for return array length */
            out_param_ptr += ALIGN8(sizeof(INT));

            /* use maximum array length from input */
            param_size += max_size;
         }

         if (rpc_list[idx].param[i].tid == TID_STRUCT)
            param_size = ALIGN8(rpc_list[idx].param[i].n);

         if ((POINTER_T) out_param_ptr - (POINTER_T) nc_out + param_size > return_buffer_size) {
#ifdef FIXED_BUFFER
            cm_msg(MERROR, "rpc_execute",
                   "return parameters (%d) too large for network buffer (%d)",
                   (POINTER_T) out_param_ptr - (POINTER_T) nc_out + param_size, return_buffer_size);

            return RPC_EXCEED_BUFFER;
#else
            int itls;
            int new_size = (POINTER_T) out_param_ptr - (POINTER_T) nc_out + param_size + 1024;

            if (0)
               cm_msg(MINFO, "rpc_execute",
                      "rpc_execute: return parameters (%d) too large for network buffer (%d), new buffer size (%d)",
                      (POINTER_T) out_param_ptr - (POINTER_T) nc_out + param_size, return_buffer_size, new_size);

            itls = return_buffer_tls;

            tls_buffer[itls].buffer_size = new_size;
            tls_buffer[itls].buffer = (char *) realloc(tls_buffer[itls].buffer, new_size);

            if (!tls_buffer[itls].buffer) {
               cm_msg(MERROR, "rpc_execute", "Cannot allocate return buffer of size %d", new_size);
               return RPC_EXCEED_BUFFER;
            }

            return_buffer_size = tls_buffer[itls].buffer_size;
            return_buffer = tls_buffer[itls].buffer;
            assert(return_buffer);

            goto again;
#endif
         }

         /* if parameter goes both directions, copy input to output */
         if (rpc_list[idx].param[i].flags & RPC_IN)
            memcpy(out_param_ptr, prpc_param[i], param_size);

         if (_debug_print && !(flags & RPC_IN))
            strcat(debug_line, "-");

         prpc_param[i] = out_param_ptr;
         out_param_ptr += param_size;
      }

      if (rpc_list[idx].param[i + 1].tid)
         strcat(debug_line, ", ");
   }

   //printf("predicted return size %d\n", (POINTER_T) out_param_ptr - (POINTER_T) nc_out);

   strcat(debug_line, ")");
   rpc_debug_printf(debug_line);

   last_param_ptr = out_param_ptr;

  /*********************************\
  *   call dispatch function        *
  \*********************************/
   if (rpc_list[idx].dispatch)
      status = rpc_list[idx].dispatch(routine_id, prpc_param);
   else
      status = RPC_INVALID_ID;

   if (routine_id == RPC_ID_EXIT || routine_id == RPC_ID_SHUTDOWN || routine_id == RPC_ID_WATCHDOG)
      status = RPC_SUCCESS;

   /* return immediately for closed down client connections */
   if (!sock && routine_id == RPC_ID_EXIT)
      return SS_EXIT;

   if (!sock && routine_id == RPC_ID_SHUTDOWN)
      return RPC_SHUTDOWN;

   /* Return if TCP connection broken */
   if (status == SS_ABORT)
      return SS_ABORT;

   /* if sock == 0, we are in FTCP mode and may not sent results */
   if (!sock)
      return RPC_SUCCESS;

   /* compress variable length arrays */
   out_param_ptr = nc_out->param;
   for (i = 0; rpc_list[idx].param[i].tid != 0; i++)
      if (rpc_list[idx].param[i].flags & RPC_OUT) {
         tid = rpc_list[idx].param[i].tid;
         flags = rpc_list[idx].param[i].flags;
         param_size = ALIGN8(tid_size[tid]);

         if (tid == TID_STRING) {
            max_size = *((INT *) out_param_ptr);
            param_size = strlen((char *) prpc_param[i]) + 1;
            param_size = ALIGN8(param_size);

            /* move string ALIGN8(sizeof(INT)) left */
            memmove(out_param_ptr, out_param_ptr + ALIGN8(sizeof(INT)), param_size);

            /* move remaining parameters to end of string */
            memmove(out_param_ptr + param_size,
                   out_param_ptr + max_size + ALIGN8(sizeof(INT)),
                   (POINTER_T) last_param_ptr - ((POINTER_T) out_param_ptr + max_size + ALIGN8(sizeof(INT))));
         }

         if (flags & RPC_VARARRAY) {
            /* store array length at current out_param_ptr */
            max_size = *((INT *) out_param_ptr);
            param_size = *((INT *) prpc_param[i + 1]);
            *((INT *) out_param_ptr) = param_size;      // store new array size
            if (convert_flags)
               rpc_convert_single(out_param_ptr, TID_INT, RPC_OUTGOING, convert_flags);

            out_param_ptr += ALIGN8(sizeof(INT));       // step over array size

            param_size = ALIGN8(param_size);

            /* move remaining parameters to end of array */
            memmove(out_param_ptr + param_size,
                   out_param_ptr + max_size,
                   (POINTER_T) last_param_ptr - ((POINTER_T) out_param_ptr + max_size));
         }

         if (tid == TID_STRUCT)
            param_size = ALIGN8(rpc_list[idx].param[i].n);

         /* convert data format */
         if (convert_flags) {
            if (flags & RPC_VARARRAY)
               rpc_convert_data(out_param_ptr, tid,
                                rpc_list[idx].param[i].flags | RPC_OUTGOING, param_size, convert_flags);
            else
               rpc_convert_data(out_param_ptr, tid,
                                rpc_list[idx].param[i].flags | RPC_OUTGOING,
                                rpc_list[idx].param[i].n * tid_size[tid], convert_flags);
         }

         out_param_ptr += param_size;
      }

   /* send return parameters */
   param_size = (POINTER_T) out_param_ptr - (POINTER_T) nc_out->param;
   nc_out->header.routine_id = status;
   nc_out->header.param_size = param_size;

   //printf("actual return size %d, buffer used %d\n", (POINTER_T) out_param_ptr - (POINTER_T) nc_out, sizeof(NET_COMMAND_HEADER) + param_size);

   /* convert header format (byte swapping) if necessary */
   if (convert_flags) {
      rpc_convert_single(&nc_out->header.routine_id, TID_DWORD, RPC_OUTGOING, convert_flags);
      rpc_convert_single(&nc_out->header.param_size, TID_DWORD, RPC_OUTGOING, convert_flags);
   }

   status = send_tcp(sock, return_buffer, sizeof(NET_COMMAND_HEADER) + param_size, 0);

   if (status < 0) {
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


/********************************************************************/
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

   INT i, j, idx, status, index_in;
   char *in_param_ptr, *out_param_ptr, *last_param_ptr;
   INT routine_id, tid, flags, array_tid, n_param;
   INT param_size, item_size, num_values;
   void *prpc_param[20];
   char *arpc_param[N_APARAM], *pc;
   char str[1024], debug_line[1024];
   char buffer1[ASCII_BUFFER_SIZE];     /* binary in */
   char buffer2[ASCII_BUFFER_SIZE];     /* binary out */
   char return_buffer[ASCII_BUFFER_SIZE];       /* ASCII out */

   /* parse arguments */
   arpc_param[0] = buffer;
   for (i = 1; i < N_APARAM; i++) {
      arpc_param[i] = strchr(arpc_param[i - 1], '&');
      if (arpc_param[i] == NULL)
         break;
      *arpc_param[i] = 0;
      arpc_param[i]++;
   }

   /* decode '%' */
   for (i = 0; i < N_APARAM && arpc_param[i]; i++)
      while ((pc = strchr(arpc_param[i], '%')) != NULL) {
         if (isxdigit(pc[1]) && isxdigit(pc[2])) {
            str[0] = pc[1];
            str[1] = pc[2];
            str[2] = 0;
            sscanf(str, "%02X", &i);

            *pc++ = i;
            while (pc[2]) {
               pc[0] = pc[2];
               pc++;
            }
         }
      }

   /* find entry in rpc_list */
   for (i = 0;; i++)
      if (rpc_list[i].id == 0 || strcmp(arpc_param[0], rpc_list[i].name) == 0)
         break;
   idx = i;
   routine_id = rpc_list[i].id;
   if (rpc_list[i].id == 0) {
      cm_msg(MERROR, "rpc_execute", "Invalid rpc name (%s)", arpc_param[0]);
      return RPC_INVALID_ID;
   }

   in_param_ptr = buffer1;
   out_param_ptr = buffer2;
   index_in = 1;

   sprintf(debug_line, "%s(", rpc_list[idx].name);

   for (i = 0; rpc_list[idx].param[i].tid != 0; i++) {
      tid = rpc_list[idx].param[i].tid;
      flags = rpc_list[idx].param[i].flags;

      if (flags & RPC_IN) {
         if (flags & RPC_VARARRAY) {
            sscanf(arpc_param[index_in++], "%d %d", &n_param, &array_tid);

            prpc_param[i] = in_param_ptr;
            for (j = 0; j < n_param; j++) {
               db_sscanf(arpc_param[index_in++], in_param_ptr, &param_size, 0, array_tid);
               in_param_ptr += param_size;
            }
            in_param_ptr = (char *) ALIGN8(((POINTER_T) in_param_ptr));

            strcat(debug_line, "<array>");
         } else {
            db_sscanf(arpc_param[index_in++], in_param_ptr, &param_size, 0, tid);
            param_size = ALIGN8(param_size);

            if (tid == TID_STRING || tid == TID_LINK)
               param_size = ALIGN8(1 + strlen((char *) (in_param_ptr)));

            /*
               if (tid == TID_STRUCT)
               param_size = ALIGN8( rpc_list[idx].param[i].n );
             */
            prpc_param[i] = in_param_ptr;

            db_sprintf(str, in_param_ptr, param_size, 0, rpc_list[idx].param[i].tid);
            if (rpc_list[idx].param[i].tid == TID_STRING) {
               /* check for long strings (db_create_record...) */
               if (strlen(debug_line) + strlen(str) + 2 < sizeof(debug_line)) {
                  strcat(debug_line, "\"");
                  strcat(debug_line, str);
                  strcat(debug_line, "\"");
               } else
                  strcat(debug_line, "...");
            } else
               strcat(debug_line, str);

            in_param_ptr += param_size;
         }

         if ((POINTER_T) in_param_ptr - (POINTER_T) buffer1 > ASCII_BUFFER_SIZE) {
            cm_msg(MERROR, "rpc_ascii_execute",
                   "parameters (%d) too large for network buffer (%d)", param_size, ASCII_BUFFER_SIZE);
            return RPC_EXCEED_BUFFER;
         }

      }

      if (flags & RPC_OUT) {
         param_size = ALIGN8(tid_size[tid]);

         if (flags & RPC_VARARRAY || tid == TID_STRING) {
            /* reserve maximum array length */
            param_size = atoi(arpc_param[index_in]);
            param_size = ALIGN8(param_size);
         }

/*
      if (rpc_list[idx].param[i].tid == TID_STRUCT)
        param_size = ALIGN8( rpc_list[idx].param[i].n );
*/
         if ((POINTER_T) out_param_ptr - (POINTER_T) buffer2 + param_size > ASCII_BUFFER_SIZE) {
            cm_msg(MERROR, "rpc_execute",
                   "return parameters (%d) too large for network buffer (%d)",
                   (POINTER_T) out_param_ptr - (POINTER_T) buffer2 + param_size, ASCII_BUFFER_SIZE);
            return RPC_EXCEED_BUFFER;
         }

         /* if parameter goes both directions, copy input to output */
         if (rpc_list[idx].param[i].flags & RPC_IN)
            memcpy(out_param_ptr, prpc_param[i], param_size);

         if (!(flags & RPC_IN))
            strcat(debug_line, "-");

         prpc_param[i] = out_param_ptr;
         out_param_ptr += param_size;
      }

      if (rpc_list[idx].param[i + 1].tid)
         strcat(debug_line, ", ");
   }

   strcat(debug_line, ")");
   rpc_debug_printf(debug_line);

   last_param_ptr = out_param_ptr;

   /*********************************\
   *   call dispatch function        *
   \*********************************/

   if (rpc_list[idx].dispatch)
      status = rpc_list[idx].dispatch(routine_id, prpc_param);
   else
      status = RPC_INVALID_ID;

   if (routine_id == RPC_ID_EXIT || routine_id == RPC_ID_SHUTDOWN || routine_id == RPC_ID_WATCHDOG)
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
   for (i = 0; rpc_list[idx].param[i].tid != 0; i++)
      if (rpc_list[idx].param[i].flags & RPC_OUT) {
         *out_param_ptr++ = '&';

         tid = rpc_list[idx].param[i].tid;
         flags = rpc_list[idx].param[i].flags;
         param_size = ALIGN8(tid_size[tid]);

         if (tid == TID_STRING && !(flags & RPC_VARARRAY)) {
            strcpy(out_param_ptr, (char *) prpc_param[i]);
            param_size = strlen((char *) prpc_param[i]);
         }

         else if (flags & RPC_VARARRAY) {
            if (rpc_list[idx].id == RPC_BM_RECEIVE_EVENT) {
               param_size = *((INT *) prpc_param[i + 1]);
               /* write number of bytes to output */
               sprintf(out_param_ptr, "%d", param_size);
               out_param_ptr += strlen(out_param_ptr) + 1;      /* '0' finishes param */
               memcpy(out_param_ptr, prpc_param[i], param_size);
               out_param_ptr += param_size;
               *out_param_ptr = 0;
            } else {
               if (rpc_list[idx].id == RPC_DB_GET_DATA1) {
                  param_size = *((INT *) prpc_param[i + 1]);
                  array_tid = *((INT *) prpc_param[i + 2]);
                  num_values = *((INT *) prpc_param[i + 3]);
               } else if (rpc_list[idx].id == RPC_DB_GET_DATA_INDEX) {
                  param_size = *((INT *) prpc_param[i + 1]);
                  array_tid = *((INT *) prpc_param[i + 3]);
                  num_values = 1;
               } else if (rpc_list[idx].id == RPC_HS_READ) {
                  param_size = *((INT *) prpc_param[i + 1]);
                  if (i == 6) {
                     array_tid = TID_DWORD;
                     num_values = param_size / sizeof(DWORD);
                  } else {
                     array_tid = *((INT *) prpc_param[10]);
                     num_values = *((INT *) prpc_param[11]);
                  }
               } else {         /* variable arrays of fixed type like hs_enum_events, hs_enum_vars */

                  param_size = *((INT *) prpc_param[i + 1]);
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
               for (j = 0; j < num_values; j++) {
                  *out_param_ptr++ = '&';
                  db_sprintf(out_param_ptr, prpc_param[i], item_size, j, array_tid);
                  out_param_ptr += strlen(out_param_ptr);
               }
            }
         }

/*
      else if (tid == TID_STRUCT)
        param_size = ALIGN8( rpc_list[idx].param[i].n );
*/
         else
            db_sprintf(out_param_ptr, prpc_param[i], param_size, 0, tid);

         out_param_ptr += strlen(out_param_ptr);

         if ((POINTER_T) out_param_ptr - (POINTER_T) return_buffer > ASCII_BUFFER_SIZE) {
            cm_msg(MERROR, "rpc_execute",
                   "return parameter (%d) too large for network buffer (%d)", param_size, ASCII_BUFFER_SIZE);
            return RPC_EXCEED_BUFFER;
         }
      }

   /* send return parameters */
   param_size = (POINTER_T) out_param_ptr - (POINTER_T) return_buffer + 1;

   status = send_tcp(sock, return_buffer, param_size, 0);

   if (status < 0) {
      cm_msg(MERROR, "rpc_execute", "send_tcp() failed");
      return RPC_NET_ERROR;
   }

   /* print return buffer */
   if (strlen(return_buffer) > sizeof(debug_line)) {
      memcpy(debug_line, return_buffer, sizeof(debug_line) - 10);
      strcat(debug_line, "...");
   } else
      sprintf(debug_line, "-> %s", return_buffer);
   rpc_debug_printf(debug_line);

   /* return SS_EXIT if RPC_EXIT is called */
   if (routine_id == RPC_ID_EXIT)
      return SS_EXIT;

   /* return SS_SHUTDOWN if RPC_SHUTDOWN is called */
   if (routine_id == RPC_ID_SHUTDOWN)
      return RPC_SHUTDOWN;

   return RPC_SUCCESS;
}

#define MAX_N_ALLOWED_HOSTS 100
static char allowed_host[MAX_N_ALLOWED_HOSTS][256];
static int  n_allowed_hosts = 0;

/********************************************************************/
INT rpc_clear_allowed_hosts()
/********************************************************************\
  Routine: rpc_clear_allowed_hosts

  Purpose: Clear list of allowed hosts and permit connections from anybody

  Input:
    none

  Output:
    none

  Function value:
    RPC_SUCCESS             Successful completion

\********************************************************************/
{
   n_allowed_hosts = 0;
   return RPC_SUCCESS;
}

/********************************************************************/
INT rpc_add_allowed_host(const char* hostname)
/********************************************************************\
  Routine: rpc_add_allowed_host

  Purpose: Permit connections from listed hosts only

  Input:
    none

  Output:
    none

  Function value:
    RPC_SUCCESS             Successful completion
    RPC_NO_MEMORY           Too many allowed hosts

\********************************************************************/
{
   if (n_allowed_hosts >= MAX_N_ALLOWED_HOSTS)
      return RPC_NO_MEMORY;

   strlcpy(allowed_host[n_allowed_hosts++], hostname, sizeof(allowed_host[0]));
   return RPC_SUCCESS;
}

/********************************************************************/
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
   INT idx, i;
   unsigned int size;
   int status;
   char command;
   INT sock;
   char version[NAME_LENGTH], v1[32];
   char experiment[NAME_LENGTH];
   INT port1, port2, port3;
   char *ptr;
   struct sockaddr_in acc_addr;
   struct hostent *phe;
   char str[100];
   char host_port1_str[30], host_port2_str[30], host_port3_str[30];
   char debug_str[30];
   char *argv[10];
   char net_buffer[256];
   struct linger ling;

   static struct callback_addr callback;

   if (lsock > 0) {
      size = sizeof(acc_addr);
#ifdef OS_WINNT
      sock = accept(lsock, (struct sockaddr *) &acc_addr, (int *) &size);
#else
      sock = accept(lsock, (struct sockaddr *) &acc_addr, &size);
#endif

      if (sock == -1)
         return RPC_NET_ERROR;
   } else {
      /* lsock is stdin -> already connected from inetd */

      size = sizeof(acc_addr);
      sock = lsock;
#ifdef OS_WINNT
      getpeername(sock, (struct sockaddr *) &acc_addr, (int *) &size);
#else
      getpeername(sock, (struct sockaddr *) &acc_addr, &size);
#endif
   }

   /* check access control list */
   if (n_allowed_hosts > 0) {
      int allowed = FALSE;
      struct hostent *remote_phe;
      char hname[256];
      struct in_addr remote_addr;

      /* save remote host address */
      memcpy(&remote_addr, &(acc_addr.sin_addr), sizeof(remote_addr));

      remote_phe = gethostbyaddr((char *) &remote_addr, 4, PF_INET);

      if (remote_phe == NULL) {
         /* use IP number instead */
         strlcpy(hname, (char *)inet_ntoa(remote_addr), sizeof(hname));
      } else
         strlcpy(hname, remote_phe->h_name, sizeof(hname));

      /* always permit localhost */
      if (strcmp(hname, "localhost.localdomain") == 0)
         allowed = TRUE;
      if (strcmp(hname, "localhost") == 0)
         allowed = TRUE;

      if (!allowed) {
         for (i=0 ; i<n_allowed_hosts ; i++)
            if (strcmp(hname, allowed_host[i]) == 0) {
               allowed = TRUE;
               break;
            }
      }

      if (!allowed) {
         cm_msg(MERROR, "rpc_server_accept", "rejecting connection from unallowed host \'%s\'", hname);
         closesocket(sock);
         return RPC_NET_ERROR;
      }
   }

   /* receive string with timeout */
   i = recv_string(sock, net_buffer, 256, 10000);
   rpc_debug_printf("Received command: %s", net_buffer);

   if (i > 0) {
      command = (char) toupper(net_buffer[0]);

      switch (command) {
      case 'S':

         /*----------- shutdown listener ----------------------*/
         closesocket(sock);
         return RPC_SHUTDOWN;

      case 'I':

         /*----------- return available experiments -----------*/
         cm_scan_experiments();
         for (i = 0; i < MAX_EXPERIMENT && exptab[i].name[0]; i++) {
            rpc_debug_printf("Return experiment: %s", exptab[i].name);
            sprintf(str, "%s", exptab[i].name);
            send(sock, str, strlen(str) + 1, 0);
         }
         send(sock, "", 1, 0);
         closesocket(sock);
         break;

      case 'C':

         /*----------- connect to experiment -----------*/

         /* get callback information */
         callback.experiment[0] = 0;
         port1 = port2 = version[0] = 0;

         //printf("net buffer \'%s\'\n", net_buffer);

         /* parse string in format "C port1 port2 port3 version expt" */
         /* example: C 51046 45838 56832 2.0.0 alpha */

         port1 = strtoul(net_buffer + 2, &ptr, 0);
         port2 = strtoul(ptr, &ptr, 0);
         port3 = strtoul(ptr, &ptr, 0);

         while (*ptr == ' ')
            ptr++;

         i = 0;
         for (; *ptr != 0 && *ptr != ' ' && i < (int) sizeof(version) - 1;)
            version[i++] = *ptr++;

         // ensure that we do not overwrite buffer "version"
         assert(i < (int) sizeof(version));
         version[i] = 0;

         // skip wjatever is left from the "version" string
         for (; *ptr != 0 && *ptr != ' ';)
            ptr++;

         while (*ptr == ' ')
            ptr++;

         i = 0;
         for (; *ptr != 0 && *ptr != ' ' && *ptr != '\n' && *ptr != '\r' && i < (int) sizeof(experiment) - 1;)
            experiment[i++] = *ptr++;

         // ensure that we do not overwrite buffer "experiment"
         assert(i < (int) sizeof(experiment));
         experiment[i] = 0;

         strlcpy(callback.experiment, experiment, NAME_LENGTH);

         /* print warning if version patch level doesn't agree */
         strlcpy(v1, version, sizeof(v1));
         if (strchr(v1, '.'))
            if (strchr(strchr(v1, '.') + 1, '.'))
               *strchr(strchr(v1, '.') + 1, '.') = 0;

         strlcpy(str, cm_get_version(), sizeof(str));
         if (strchr(str, '.'))
            if (strchr(strchr(str, '.') + 1, '.'))
               *strchr(strchr(str, '.') + 1, '.') = 0;

         if (strcmp(v1, str) != 0) {
            sprintf(str, "client MIDAS version %s differs from local version %s", version, cm_get_version());
            cm_msg(MERROR, "rpc_server_accept", str);

            sprintf(str, "received string: %s", net_buffer + 2);
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
            if (status != 0) {
               cm_msg(MERROR, "rpc_server_accept", "cannot get host name for IP address");
               break;
            }
         }
#else
         phe = gethostbyaddr((char *) &acc_addr.sin_addr, 4, PF_INET);
         if (phe == NULL) {
            /* use IP number instead */
            strlcpy(callback.host_name, (char *) inet_ntoa(acc_addr.sin_addr), HOST_NAME_LENGTH);
         } else
            strlcpy(callback.host_name, phe->h_name, HOST_NAME_LENGTH);
#endif

         if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_MPROCESS) {
            /* update experiment definition */
            cm_scan_experiments();

            /* lookup experiment */
            if (equal_ustring(callback.experiment, "Default"))
               idx = 0;
            else
               for (idx = 0; idx < MAX_EXPERIMENT && exptab[idx].name[0]; idx++)
                  if (equal_ustring(callback.experiment, exptab[idx].name))
                     break;

            if (idx == MAX_EXPERIMENT || exptab[idx].name[0] == 0) {
               sprintf(str, "experiment \'%s\' not defined in exptab\r", callback.experiment);
               cm_msg(MERROR, "rpc_server_accept", str);

               send(sock, "2", 2, 0);   /* 2 means exp. not found */
               closesocket(sock);
               break;
            }

            strlcpy(callback.directory, exptab[idx].directory, MAX_STRING_LENGTH);
            strlcpy(callback.user, exptab[idx].user, NAME_LENGTH);

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

            rpc_debug_printf("Spawn: %s %s %s %s %s %s %s %s %s %s",
                             argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8],
                             argv[9]);

            status = ss_spawnv(P_NOWAIT, (char *) rpc_get_server_option(RPC_OSERVER_NAME), argv);

            if (status != SS_SUCCESS) {
               rpc_debug_printf("Cannot spawn subprocess: %s\n", strerror(errno));

               sprintf(str, "3");       /* 3 means cannot spawn subprocess */
               send(sock, str, strlen(str) + 1, 0);
               closesocket(sock);
               break;
            }

            sprintf(str, "1 %s", cm_get_version());     /* 1 means ok */
            send(sock, str, strlen(str) + 1, 0);
            closesocket(sock);
         } else {
            sprintf(str, "1 %s", cm_get_version());     /* 1 means ok */
            send(sock, str, strlen(str) + 1, 0);
            closesocket(sock);
         }

         /* look for next free entry */
         for (idx = 0; idx < MAX_RPC_CONNECTION; idx++)
            if (_server_acception[idx].recv_sock == 0)
               break;
         if (idx == MAX_RPC_CONNECTION)
            return RPC_NET_ERROR;
         callback.index = idx;

        /*----- multi thread server ------------------------*/
         if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_MTHREAD)
            ss_thread_create(rpc_server_thread, (void *) (&callback));

        /*----- single thread server -----------------------*/
         if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_SINGLE ||
             rpc_get_server_option(RPC_OSERVER_TYPE) == ST_REMOTE)
            rpc_server_callback(&callback);

         break;

      default:
         cm_msg(MERROR, "rpc_server_accept", "received unknown command '%c' code %d", command, command);
         closesocket(sock);
         break;

      }
   } else {                     /* if i>0 */

      /* lingering needed for PCTCP */
      ling.l_onoff = 1;
      ling.l_linger = 0;
      setsockopt(sock, SOL_SOCKET, SO_LINGER, (char *) &ling, sizeof(ling));
      closesocket(sock);
   }

   return RPC_SUCCESS;
}

/********************************************************************/
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
   INT idx, i, version, status;
   unsigned int size;
   int sock;
   struct sockaddr_in acc_addr;
   INT client_hw_type = 0, hw_type;
   char str[100], client_program[NAME_LENGTH];
   char host_name[HOST_NAME_LENGTH];
   INT convert_flags;
   char net_buffer[256], *p;

   size = sizeof(acc_addr);
#ifdef OS_WINNT
   sock = accept(lsock, (struct sockaddr *) &acc_addr, (int *) &size);
#else
   sock = accept(lsock, (struct sockaddr *) &acc_addr, &size);
#endif

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
   strcpy(host_name, "(unknown)");

   strcpy(client_program, "(unknown)");

   /* look for next free entry */
   for (idx = 0; idx < MAX_RPC_CONNECTION; idx++)
      if (_server_acception[idx].recv_sock == 0)
         break;
   if (idx == MAX_RPC_CONNECTION) {
      closesocket(sock);
      return RPC_NET_ERROR;
   }

   /* receive string with timeout */
   i = recv_string(sock, net_buffer, sizeof(net_buffer), 10000);
   if (i <= 0) {
      closesocket(sock);
      return RPC_NET_ERROR;
   }

   /* get remote computer info */
   p = strtok(net_buffer, " ");
   if (p != NULL) {
      client_hw_type = atoi(p);
      p = strtok(NULL, " ");
   }
   if (p != NULL) {
      version = atoi(p);
      p = strtok(NULL, " ");
   }
   if (p != NULL) {
      strlcpy(client_program, p, sizeof(client_program));
      p = strtok(NULL, " ");
   }
   if (p != NULL) {
      strlcpy(host_name, p, sizeof(host_name));
      p = strtok(NULL, " ");
   }

   if (0)
      printf("rpc_client_accept: client_hw_type %d, version %d, client_name \'%s\', hostname \'%s\'\n",
             client_hw_type, version, client_program, host_name);

   /* save information in _server_acception structure */
   _server_acception[idx].recv_sock = sock;
   _server_acception[idx].send_sock = 0;
   _server_acception[idx].event_sock = 0;
   _server_acception[idx].remote_hw_type = client_hw_type;
   strcpy(_server_acception[idx].host_name, host_name);
   strcpy(_server_acception[idx].prog_name, client_program);
   _server_acception[idx].tid = ss_gettid();
   _server_acception[idx].last_activity = ss_millitime();
   _server_acception[idx].watchdog_timeout = 0;

   /* send my own computer id */
   hw_type = rpc_get_option(0, RPC_OHW_TYPE);
   sprintf(str, "%d %s", hw_type, cm_get_version());
   status = send(sock, str, strlen(str) + 1, 0);
   if (status != (INT) strlen(str) + 1)
      return RPC_NET_ERROR;

   rpc_set_server_acception(idx + 1);
   rpc_calc_convert_flags(hw_type, client_hw_type, &convert_flags);
   rpc_set_server_option(RPC_CONVERT_FLAGS, convert_flags);

   /* set callback function for ss_suspend */
   ss_suspend_set_dispatch(CH_SERVER, _server_acception, (int (*)(void)) rpc_server_receive);

   return RPC_SUCCESS;
}

/********************************************************************/
INT rpc_server_callback(struct callback_addr * pcallback)
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
   INT idx, status;
   int recv_sock, send_sock, event_sock;
   struct sockaddr_in bind_addr;
   struct hostent *phe;
   char str[100], client_program[NAME_LENGTH];
   char host_name[HOST_NAME_LENGTH];
   INT client_hw_type, hw_type;
   INT convert_flags;
   char net_buffer[256];
   struct callback_addr callback;
   char *p;
   int flag;

   /* copy callback information */
   memcpy(&callback, pcallback, sizeof(callback));
   idx = callback.index;

   /* create new sockets for TCP */
   recv_sock = socket(AF_INET, SOCK_STREAM, 0);
   send_sock = socket(AF_INET, SOCK_STREAM, 0);
   event_sock = socket(AF_INET, SOCK_STREAM, 0);
   if (event_sock == -1)
      return RPC_NET_ERROR;

   /* callback to remote node */
   memset(&bind_addr, 0, sizeof(bind_addr));
   bind_addr.sin_family = AF_INET;
   bind_addr.sin_port = htons(callback.host_port1);

#ifdef OS_VXWORKS
   {
      INT host_addr;

      host_addr = hostGetByName(callback.host_name);
      memcpy((char *) &(bind_addr.sin_addr), &host_addr, 4);
   }
#else
   phe = gethostbyname(callback.host_name);
   if (phe == NULL) {
      cm_msg(MERROR, "rpc_server_callback", "cannot lookup host name \'%s\'", callback.host_name);
      return RPC_NET_ERROR;
   }
   memcpy((char *) &(bind_addr.sin_addr), phe->h_addr, phe->h_length);
#endif

   /* connect receive socket */
#ifdef OS_UNIX
   do {
      status = connect(recv_sock, (void *) &bind_addr, sizeof(bind_addr));

      /* don't return if an alarm signal was cought */
   } while (status == -1 && errno == EINTR);
#else
   status = connect(recv_sock, (struct sockaddr *) &bind_addr, sizeof(bind_addr));
#endif

   if (status != 0) {
      cm_msg(MERROR, "rpc_server_callback", "cannot connect receive socket");
      goto error;
   }

   bind_addr.sin_port = htons(callback.host_port2);

   /* connect send socket */
#ifdef OS_UNIX
   do {
      status = connect(send_sock, (struct sockaddr *) &bind_addr, sizeof(bind_addr));

      /* don't return if an alarm signal was cought */
   } while (status == -1 && errno == EINTR);
#else
   status = connect(send_sock, (struct sockaddr *) &bind_addr, sizeof(bind_addr));
#endif

   if (status != 0) {
      cm_msg(MERROR, "rpc_server_callback", "cannot connect send socket");
      goto error;
   }

   bind_addr.sin_port = htons(callback.host_port3);

   /* connect event socket */
#ifdef OS_UNIX
   do {
      status = connect(event_sock, (struct sockaddr *) &bind_addr, sizeof(bind_addr));

      /* don't return if an alarm signal was cought */
   } while (status == -1 && errno == EINTR);
#else
   status = connect(event_sock, (struct sockaddr *) &bind_addr, sizeof(bind_addr));
#endif

   if (status != 0) {
      cm_msg(MERROR, "rpc_server_callback", "cannot connect event socket");
      goto error;
   }
#ifndef OS_ULTRIX               /* crashes ULTRIX... */
   /* increase send buffer size to 2 Mbytes, on Linux also limited by sysctl net.ipv4.tcp_rmem and net.ipv4.tcp_wmem */
   flag = 2 * 1024 * 1024;
   status = setsockopt(event_sock, SOL_SOCKET, SO_RCVBUF, (char *) &flag, sizeof(INT));
   if (status != 0)
      cm_msg(MERROR, "rpc_server_callback", "cannot setsockopt(SOL_SOCKET, SO_RCVBUF), errno %d (%s)", errno,
             strerror(errno));
#endif

   if (recv_string(recv_sock, net_buffer, 256, _rpc_connect_timeout) <= 0) {
      cm_msg(MERROR, "rpc_server_callback", "timeout on receive remote computer info");
      goto error;
   }
   //printf("rpc_server_callback: \'%s\'\n", net_buffer);

   /* get remote computer info */
   client_hw_type = strtoul(net_buffer, &p, 0);

   while (*p == ' ')
      p++;

   strlcpy(client_program, p, sizeof(client_program));

   //printf("hw type %d, name \'%s\'\n", client_hw_type, client_program);

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
   _server_acception[idx].recv_sock = recv_sock;
   _server_acception[idx].send_sock = send_sock;
   _server_acception[idx].event_sock = event_sock;
   _server_acception[idx].remote_hw_type = client_hw_type;
   strcpy(_server_acception[idx].host_name, host_name);
   strcpy(_server_acception[idx].prog_name, client_program);
   _server_acception[idx].tid = ss_gettid();
   _server_acception[idx].last_activity = ss_millitime();
   _server_acception[idx].watchdog_timeout = 0;

   /* send my own computer id */
   hw_type = rpc_get_option(0, RPC_OHW_TYPE);
   sprintf(str, "%d", hw_type);
   send(recv_sock, str, strlen(str) + 1, 0);

   rpc_set_server_acception(idx + 1);
   rpc_calc_convert_flags(hw_type, client_hw_type, &convert_flags);
   rpc_set_server_option(RPC_CONVERT_FLAGS, convert_flags);

   /* set callback function for ss_suspend */
   ss_suspend_set_dispatch(CH_SERVER, _server_acception, (int (*)(void)) rpc_server_receive);

   if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_REMOTE)
      rpc_debug_printf("Connection to %s:%s established\n",
                       _server_acception[idx].host_name, _server_acception[idx].prog_name);

   return RPC_SUCCESS;

 error:

   closesocket(recv_sock);
   closesocket(send_sock);
   closesocket(event_sock);

   return RPC_NET_ERROR;
}


/********************************************************************/
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
   int status, semaphore_alarm, semaphore_elog, semaphore_history, semaphore_msg;
   static DWORD last_checked = 0;

   memcpy(&callback, pointer, sizeof(callback));

   status = rpc_server_callback(&callback);

   if (status != RPC_SUCCESS)
      return status;

   /* create alarm and elog semaphores */
   ss_semaphore_create("ALARM", &semaphore_alarm);
   ss_semaphore_create("ELOG", &semaphore_elog);
   ss_semaphore_create("HISTORY", &semaphore_history);
   ss_semaphore_create("MSG", &semaphore_msg);
   cm_set_experiment_semaphore(semaphore_alarm, semaphore_elog, semaphore_history, semaphore_msg);

   do {
      status = ss_suspend(5000, 0);

      if (rpc_check_channels() == RPC_NET_ERROR)
         break;

      /* check alarms every 10 seconds */
      if (!rpc_is_remote() && ss_time() - last_checked > 10) {
         al_check();
         last_checked = ss_time();
      }

      cm_msg_flush_buffer();

   } while (status != SS_ABORT && status != SS_EXIT);

   /* delete entry in suspend table for this thread */
   ss_suspend_exit();

   return RPC_SUCCESS;
}


/********************************************************************/
INT rpc_server_receive(INT idx, int sock, BOOL check)
/********************************************************************\

  Routine: rpc_server_receive

  Purpose: Receive rpc commands and execute them. Close the connection
           if client has broken TCP pipe.

  Input:
    INT    idx              Index to _server_acception structure in-
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
   INT status, n_received;
   INT remaining, *pbh, start_time;
   char test_buffer[256], str[80];
   EVENT_HEADER *pevent;

   /* init network buffer */
   if (_net_recv_buffer_size == 0) {
      int size = MAX_EVENT_SIZE + sizeof(EVENT_HEADER) + 1024;
      _net_recv_buffer = (char *) M_MALLOC(size);
      if (_net_recv_buffer == NULL) {
         cm_msg(MERROR, "rpc_server_receive", "Cannot allocate %d bytes for network buffer", size);
         return RPC_EXCEED_BUFFER;
      }
      _net_recv_buffer_size = size;
      _net_recv_buffer_size_odb = 0;
      //printf("allocated _net_recv_buffer size %d\n", _net_recv_buffer_size);
   }

   /* init network buffer */
   if (_net_recv_buffer_size_odb == 0) {
      HNDLE hDB;
      int size;
      int max_event_size = MAX_EVENT_SIZE;

      /* get max event size from ODB */
      status = cm_get_experiment_database(&hDB, NULL);
      assert(status == CM_SUCCESS);

      if (hDB) {
         size = sizeof(INT);
         status = db_get_value(hDB, 0, "/Experiment/MAX_EVENT_SIZE", &max_event_size, &size, TID_DWORD, TRUE);

         size = max_event_size + sizeof(EVENT_HEADER) + 1024;

         _net_recv_buffer_size_odb = size;

         if (size > _net_recv_buffer_size) {
            _net_recv_buffer = (char *) realloc(_net_recv_buffer, size);
            if (_net_recv_buffer == NULL) {
               cm_msg(MERROR, "rpc_server_receive", "Cannot allocate %d bytes for network buffer", size);
               return RPC_EXCEED_BUFFER;
            }
            _net_recv_buffer_size = size;
            //printf("allocated _net_recv_buffer size %d\n", _net_recv_buffer_size);
         }
      }
   }

   /* only check if TCP connection is broken */
   if (check) {
#ifdef OS_WINNT
      n_received = recv(sock, test_buffer, sizeof(test_buffer), MSG_PEEK);
#else
      n_received = recv(sock, test_buffer, sizeof(test_buffer), MSG_PEEK|MSG_DONTWAIT);

      /* check if we caught a signal */
      if ((n_received == -1) && (errno == EAGAIN))
         return SS_SUCCESS;
#endif

      if (n_received == -1)
         cm_msg(MERROR, "rpc_server_receive",
                "recv(%d,MSG_PEEK) returned %d, errno: %d (%s)", sizeof(test_buffer),
                n_received, errno, strerror(errno));

      if (n_received <= 0)
         return SS_ABORT;

      return SS_SUCCESS;
   }

   remaining = 0;

   /* receive command */
   if (sock == _server_acception[idx].recv_sock) {
      do {
         if (_server_acception[idx].remote_hw_type == DR_ASCII)
            n_received =
                recv_string(_server_acception[idx].recv_sock, _net_recv_buffer, _net_recv_buffer_size, 10000);
         else
            n_received = recv_tcp_server(idx, _net_recv_buffer, _net_recv_buffer_size, 0, &remaining);

         if (n_received <= 0) {
            status = SS_ABORT;
            cm_msg(MERROR, "rpc_server_receive", "recv_tcp_server() returned %d, abort", n_received);
            goto error;
         }

         rpc_set_server_acception(idx + 1);

         if (_server_acception[idx].remote_hw_type == DR_ASCII)
            status = rpc_execute_ascii(_server_acception[idx].recv_sock, _net_recv_buffer);
         else
            status = rpc_execute(_server_acception[idx].recv_sock,
                                 _net_recv_buffer, _server_acception[idx].convert_flags);

         if (status == SS_ABORT) {
            cm_msg(MERROR, "rpc_server_receive", "rpc_execute() returned %d, abort", status);
            goto error;
         }

         if (status == SS_EXIT || status == RPC_SHUTDOWN) {
            if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_REMOTE)
               rpc_debug_printf("Connection to %s:%s closed\n",
                                _server_acception[idx].host_name, _server_acception[idx].prog_name);
            goto exit;
         }

      } while (remaining);
   } else {
      /* receive event */
      if (sock == _server_acception[idx].event_sock) {
         start_time = ss_millitime();

         do {
            n_received = recv_event_server(idx, _net_recv_buffer, _net_recv_buffer_size, 0, &remaining);

            if (n_received <= 0) {
               status = SS_ABORT;
               cm_msg(MERROR, "rpc_server_receive", "recv_event_server() returned %d, abort", n_received);
               goto error;
            }

            /* send event to buffer */
            pbh = (INT *) _net_recv_buffer;
            pevent = (EVENT_HEADER *) (pbh + 1);

            status = bm_send_event(*pbh, pevent, pevent->data_size + sizeof(EVENT_HEADER), SYNC);
            if (status != BM_SUCCESS)
               cm_msg(MERROR, "rpc_server_receive", "bm_send_event() returned %d", status);

            /* repeat for maximum 0.5 sec */
         } while (ss_millitime() - start_time < 500 && remaining);
      }
   }

   return RPC_SUCCESS;

 error:

   strlcpy(str, _server_acception[idx].host_name, sizeof(str));
   if (strchr(str, '.'))
      *strchr(str, '.') = 0;
   cm_msg(MTALK, "rpc_server_receive", "Program \'%s\' on host \'%s\' aborted",
          _server_acception[idx].prog_name, str);

 exit:

   /* disconnect from experiment as MIDAS server */
   if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_REMOTE) {
      HNDLE hDB, hKey;

      cm_get_experiment_database(&hDB, &hKey);

      /* only disconnect from experiment if previously connected.
         Necessary for pure RPC servers (RPC_SRVR) */
      if (hDB) {
#ifdef LOCAL_ROUTINES
         ss_alarm(0, cm_watchdog);
#endif

         bm_close_all_buffers();
         cm_delete_client_info(hDB, 0);
         db_close_all_databases();

         rpc_deregister_functions();

         cm_set_experiment_database(0, 0);

         if (_msg_mutex)
            ss_mutex_delete(_msg_mutex);
         _msg_mutex = 0;

         if (_msg_rb)
            rb_delete(_msg_rb);
         _msg_rb = 0;

         _msg_buffer = 0;
      }
   }

   /* close server connection */
   if (_server_acception[idx].recv_sock)
      closesocket(_server_acception[idx].recv_sock);
   if (_server_acception[idx].send_sock)
      closesocket(_server_acception[idx].send_sock);
   if (_server_acception[idx].event_sock)
      closesocket(_server_acception[idx].event_sock);

   /* free TCP cache */
   M_FREE(_server_acception[idx].net_buffer);
   _server_acception[idx].net_buffer = NULL;

   /* mark this entry as invalid */
   memset(&_server_acception[idx], 0, sizeof(RPC_SERVER_ACCEPTION));

   /* signal caller a shutdonw */
   if (status == RPC_SHUTDOWN)
      return status;

   /* don't abort if other than main connection is broken */
   if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_REMOTE)
      return SS_SUCCESS;

   return status;
}


/********************************************************************/
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
   for (i = 0; i < MAX_RPC_CONNECTION; i++)
      if (_server_acception[i].recv_sock != 0) {
         /* lingering needed for PCTCP */
         ling.l_onoff = 1;
         ling.l_linger = 0;
         setsockopt(_server_acception[i].recv_sock, SOL_SOCKET, SO_LINGER, (char *) &ling, sizeof(ling));
         closesocket(_server_acception[i].recv_sock);

         if (_server_acception[i].send_sock) {
            setsockopt(_server_acception[i].send_sock, SOL_SOCKET, SO_LINGER, (char *) &ling, sizeof(ling));
            closesocket(_server_acception[i].send_sock);
         }

         if (_server_acception[i].event_sock) {
            setsockopt(_server_acception[i].event_sock, SOL_SOCKET, SO_LINGER, (char *) &ling, sizeof(ling));
            closesocket(_server_acception[i].event_sock);
         }

         _server_acception[i].recv_sock = 0;
         _server_acception[i].send_sock = 0;
         _server_acception[i].event_sock = 0;
      }

   if (_lsock) {
      closesocket(_lsock);
      _lsock = 0;
      _server_registered = FALSE;
   }

   /* free suspend structures */
   ss_suspend_exit();

   return RPC_SUCCESS;
}


/********************************************************************/
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
   INT status, idx, i, convert_flags;
   NET_COMMAND nc;
   fd_set readfds;
   struct timeval timeout;

   for (idx = 0; idx < MAX_RPC_CONNECTION; idx++) {
      if (_server_acception[idx].recv_sock &&
          _server_acception[idx].tid == ss_gettid() &&
          _server_acception[idx].watchdog_timeout &&
          (ss_millitime() - _server_acception[idx].last_activity >
           (DWORD) _server_acception[idx].watchdog_timeout)) {
/* printf("Send watchdog message to %s on %s\n",
                _server_acception[idx].prog_name,
                _server_acception[idx].host_name); */

         /* send a watchdog message */
         nc.header.routine_id = MSG_WATCHDOG;
         nc.header.param_size = 0;

         convert_flags = rpc_get_server_option(RPC_CONVERT_FLAGS);
         if (convert_flags) {
            rpc_convert_single(&nc.header.routine_id, TID_DWORD, RPC_OUTGOING, convert_flags);
            rpc_convert_single(&nc.header.param_size, TID_DWORD, RPC_OUTGOING, convert_flags);
         }

         /* send the header to the client */
         i = send_tcp(_server_acception[idx].send_sock, (char *) &nc, sizeof(NET_COMMAND_HEADER), 0);

         if (i < 0)
            goto exit;

         /* make some timeout checking */
         FD_ZERO(&readfds);
         FD_SET(_server_acception[idx].send_sock, &readfds);
         FD_SET(_server_acception[idx].recv_sock, &readfds);

         timeout.tv_sec = _server_acception[idx].watchdog_timeout / 1000;
         timeout.tv_usec = (_server_acception[idx].watchdog_timeout % 1000) * 1000;

         do {
            status = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);

            /* if an alarm signal was cought, restart select with reduced timeout */
            if (status == -1 && timeout.tv_sec >= WATCHDOG_INTERVAL / 1000)
               timeout.tv_sec -= WATCHDOG_INTERVAL / 1000;

         } while (status == -1);        /* dont return if an alarm signal was cought */

         if (!FD_ISSET(_server_acception[idx].send_sock, &readfds) &&
             !FD_ISSET(_server_acception[idx].recv_sock, &readfds))
            goto exit;

         /* receive result on send socket */
         if (FD_ISSET(_server_acception[idx].send_sock, &readfds)) {
            i = recv_tcp(_server_acception[idx].send_sock, (char *) &nc, sizeof(nc), 0);
            if (i <= 0)
               goto exit;
         }
      }
   }

   return RPC_SUCCESS;

 exit:

   cm_msg(MINFO, "rpc_check_channels", "client [%s]%s failed watchdog test after %d sec",
          _server_acception[idx].host_name,
          _server_acception[idx].prog_name, _server_acception[idx].watchdog_timeout / 1000);

   /* disconnect from experiment */
   if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_REMOTE)
      cm_disconnect_experiment();

   /* close server connection */
   if (_server_acception[idx].recv_sock)
      closesocket(_server_acception[idx].recv_sock);
   if (_server_acception[idx].send_sock)
      closesocket(_server_acception[idx].send_sock);
   if (_server_acception[idx].event_sock)
      closesocket(_server_acception[idx].event_sock);

   /* free TCP cache */
   M_FREE(_server_acception[idx].net_buffer);
   _server_acception[idx].net_buffer = NULL;

   /* mark this entry as invalid */
   memset(&_server_acception[idx], 0, sizeof(RPC_SERVER_ACCEPTION));

   return RPC_NET_ERROR;
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/**dox***************************************************************/
                                                                                                                               /** @} *//* end of rpcfunctionc */

/**dox***************************************************************/
/** @addtogroup bkfunctionc
 *
 *  @{  */

/********************************************************************\
*                                                                    *
*                 Bank functions                                     *
*                                                                    *
\********************************************************************/

/********************************************************************/
/**
Initializes an event for Midas banks structure.
Before banks can be created in an event, bk_init() has to be called first.
@param event pointer to the area of event
*/
void bk_init(void *event)
{
   ((BANK_HEADER *) event)->data_size = 0;
   ((BANK_HEADER *) event)->flags = BANK_FORMAT_VERSION;
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************/
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

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Initializes an event for Midas banks structure for large bank size (> 32KBytes)
Before banks can be created in an event, bk_init32() has to be called first.
@param event pointer to the area of event
@return void
*/
void bk_init32(void *event)
{
   ((BANK_HEADER *) event)->data_size = 0;
   ((BANK_HEADER *) event)->flags = BANK_FORMAT_VERSION | BANK_FORMAT_32BIT;
}

/********************************************************************/
/**
Returns the size of an event containing banks.
The total size of an event is the value returned by bk_size() plus the size
of the event header (sizeof(EVENT_HEADER)).
@param event pointer to the area of event
@return number of bytes contained in data area of event
*/
INT bk_size(void *event)
{
   return ((BANK_HEADER *) event)->data_size + sizeof(BANK_HEADER);
}

/********************************************************************/
/**
Create a Midas bank.
The data pointer pdata must be used as an address to fill a
bank. It is incremented with every value written to the bank and finally points
to a location just after the last byte of the bank. It is then passed to
the function bk_close() to finish the bank creation.
\code
INT *pdata;
bk_init(pevent);
bk_create(pevent, "ADC0", TID_INT, &pdata);
*pdata++ = 123
*pdata++ = 456
bk_close(pevent, pdata);
\endcode
@param event pointer to the data area
@param name of the bank, must be exactly 4 charaters
@param type type of bank, one of the @ref Midas_Data_Types values defined in
midas.h
@param pdata pointer to the data area of the newly created bank
@return void
*/
void bk_create(void *event, const char *name, WORD type, void *pdata)
{
   if (((BANK_HEADER *) event)->flags & BANK_FORMAT_32BIT) {
      BANK32 *pbk32;

      pbk32 = (BANK32 *) ((char *) (((BANK_HEADER *) event) + 1) + ((BANK_HEADER *) event)->data_size);
      strncpy(pbk32->name, name, 4);
      pbk32->type = type;
      pbk32->data_size = 0;
      *((void **) pdata) = pbk32 + 1;
   } else {
      BANK *pbk;

      pbk = (BANK *) ((char *) (((BANK_HEADER *) event) + 1) + ((BANK_HEADER *) event)->data_size);
      strncpy(pbk->name, name, 4);
      pbk->type = type;
      pbk->data_size = 0;
      *((void **) pdata) = pbk + 1;
   }
}



/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************/
/**
Copy a bank given by name if found from a buffer source to a destination buffer.
@param * pevent pointing after the EVENT_HEADER (as in FE)
@param * psrce  pointing to EVENT_HEADER in this case (for ebch[i].pfragment)
@param bkname  Bank to be found and copied from psrce to pevent
@return EB_SUCCESS if bank found, 0 if not found (pdest untouched)
*/
INT bk_copy(char * pevent, char * psrce, const char * bkname) {

  INT status;
  DWORD bklen, bktype, bksze;
  BANK_HEADER * psBkh;
  BANK * psbkh;
  BANK32 *psbkh32;
  char * pdest;
  void * psdata;

  // source pointing on the BANKxx
  psBkh = (BANK_HEADER *) ((EVENT_HEADER *)psrce+1);
  // Find requested bank
  status = bk_find(psBkh, bkname, &bklen, &bktype, &psdata);
  // Return 0 if not found
  if (status != SUCCESS) return 0;

  // Check bank type...
  // You cannot mix BANK and BANK32 so make sure all the FE use either
  // bk_init(pevent) or bk_init32(pevent).
  if (bk_is32(psBkh)) {

    // pointer to the source bank header
    psbkh32 = ((BANK32 *)psdata -1);
    // Data size in the bank
    bksze = psbkh32->data_size;

    // Get to the end of the event
    pdest = (char *)(((BANK_HEADER *) pevent) + 1) + ((BANK_HEADER *)pevent)->data_size;
    // Copy from BANK32 to end of Data
    memmove(pdest, (char *)psbkh32, ALIGN8(bksze) + sizeof(BANK32));
    // Bring pointer to the next free location
    pdest += ALIGN8(bksze) + sizeof(BANK32);

  } else {

    // pointer to the source bank header
    psbkh = ((BANK *)psdata -1);
    // Data size in the bank
    bksze = psbkh->data_size;

    // Get to the end of the event
    pdest = (char *)(((BANK_HEADER *) pevent) + 1) + ((BANK_HEADER *)pevent)->data_size;
    // Copy from BANK to end of Data
    memmove(pdest, (char *)psbkh, ALIGN8(bksze) + sizeof(BANK));
    // Bring pointer to the next free location
    pdest += ALIGN8(bksze) + sizeof(BANK);
  }

  // Close bank (adjust BANK_HEADER size)
  bk_close(pevent, pdest);
  // Adjust EVENT_HEADER size
  ((EVENT_HEADER *)pevent-1)->data_size = ((BANK_HEADER *) pevent)->data_size + sizeof(BANK_HEADER);
  return SUCCESS;
}

/********************************************************************/
int bk_delete(void *event, const char *name)
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
   BANK *pbk;
   BANK32 *pbk32;
   DWORD dname;
   int remaining;

   if (((BANK_HEADER *) event)->flags & BANK_FORMAT_32BIT) {
      /* locate bank */
      pbk32 = (BANK32 *) (((BANK_HEADER *) event) + 1);
      strncpy((char *) &dname, name, 4);
      do {
         if (*((DWORD *) pbk32->name) == dname) {
            /* bank found, delete it */
            remaining = ((char *) event + ((BANK_HEADER *) event)->data_size +
                         sizeof(BANK_HEADER)) - ((char *) (pbk32 + 1) + ALIGN8(pbk32->data_size));

            /* reduce total event size */
            ((BANK_HEADER *) event)->data_size -= sizeof(BANK32) + ALIGN8(pbk32->data_size);

            /* copy remaining bytes */
            if (remaining > 0)
               memmove(pbk32, (char *) (pbk32 + 1) + ALIGN8(pbk32->data_size), remaining);
            return CM_SUCCESS;
         }

         pbk32 = (BANK32 *) ((char *) (pbk32 + 1) + ALIGN8(pbk32->data_size));
      } while ((DWORD) ((char *) pbk32 - (char *) event) <
               ((BANK_HEADER *) event)->data_size + sizeof(BANK_HEADER));
   } else {
      /* locate bank */
      pbk = (BANK *) (((BANK_HEADER *) event) + 1);
      strncpy((char *) &dname, name, 4);
      do {
         if (*((DWORD *) pbk->name) == dname) {
            /* bank found, delete it */
            remaining = ((char *) event + ((BANK_HEADER *) event)->data_size +
                         sizeof(BANK_HEADER)) - ((char *) (pbk + 1) + ALIGN8(pbk->data_size));

            /* reduce total event size */
            ((BANK_HEADER *) event)->data_size -= sizeof(BANK) + ALIGN8(pbk->data_size);

            /* copy remaining bytes */
            if (remaining > 0)
               memmove(pbk, (char *) (pbk + 1) + ALIGN8(pbk->data_size), remaining);
            return CM_SUCCESS;
         }

         pbk = (BANK *) ((char *) (pbk + 1) + ALIGN8(pbk->data_size));
      } while ((DWORD) ((char *) pbk - (char *) event) <
               ((BANK_HEADER *) event)->data_size + sizeof(BANK_HEADER));
   }

   return 0;
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Close the Midas bank priviously created by bk_create().
The data pointer pdata must be obtained by bk_create() and
used as an address to fill a bank. It is incremented with every value written
to the bank and finally points to a location just after the last byte of the
bank. It is then passed to bk_close() to finish the bank creation
@param event pointer to current composed event
@param pdata  pointer to the data
@return number of bytes contained in bank
*/
INT bk_close(void *event, void *pdata)
{
   if (((BANK_HEADER *) event)->flags & BANK_FORMAT_32BIT) {
      BANK32 *pbk32;

      pbk32 = (BANK32 *) ((char *) (((BANK_HEADER *) event) + 1) + ((BANK_HEADER *) event)->data_size);
      pbk32->data_size = (DWORD) ((char *) pdata - (char *) (pbk32 + 1));
      if (pbk32->type == TID_STRUCT && pbk32->data_size == 0)
         printf("Warning: bank %c%c%c%c has zero size\n",
                pbk32->name[0], pbk32->name[1], pbk32->name[2], pbk32->name[3]);
      ((BANK_HEADER *) event)->data_size += sizeof(BANK32) + ALIGN8(pbk32->data_size);
      return pbk32->data_size;
   } else {
      BANK *pbk;

      pbk = (BANK *) ((char *) (((BANK_HEADER *) event) + 1) + ((BANK_HEADER *) event)->data_size);
      pbk->data_size = (WORD) ((char *) pdata - (char *) (pbk + 1));
      if (pbk->type == TID_STRUCT && pbk->data_size == 0)
         printf("Warning: bank %c%c%c%c has zero size\n", pbk->name[0], pbk->name[1], pbk->name[2],
                pbk->name[3]);
      ((BANK_HEADER *) event)->data_size += sizeof(BANK) + ALIGN8(pbk->data_size);
      return pbk->data_size;
   }
}

/********************************************************************/
/**
Extract the MIDAS bank name listing of an event.
The bklist should be dimensioned with STRING_BANKLIST_MAX
which corresponds to a max of BANKLIST_MAX banks (midas.h: 32 banks max).
\code
INT adc_calib(EVENT_HEADER *pheader, void *pevent)
{
  INT    n_adc, nbanks;
  WORD   *pdata;
  char   banklist[STRING_BANKLIST_MAX];

  // Display # of banks and list of banks in the event
  nbanks = bk_list(pevent, banklist);
  printf("#banks:%d List:%s\n", nbanks, banklist);

  // look for ADC0 bank, return if not present
  n_adc = bk_locate(pevent, "ADC0", &pdata);
  ...
}
\endcode
@param event pointer to current composed event
@param bklist returned ASCII string, has to be booked with STRING_BANKLIST_MAX.
@return number of bank found in this event.
*/
INT bk_list(void *event, char *bklist)
{                               /* Full event */
   INT nbk, size;
   BANK *pmbk = NULL;
   BANK32 *pmbk32 = NULL;
   char *pdata;

   /* compose bank list */
   bklist[0] = 0;
   nbk = 0;
   do {
      /* scan all banks for bank name only */
      if (bk_is32(event)) {
         size = bk_iterate32(event, &pmbk32, &pdata);
         if (pmbk32 == NULL)
            break;
      } else {
         size = bk_iterate(event, &pmbk, &pdata);
         if (pmbk == NULL)
            break;
      }
      nbk++;

      if (nbk > BANKLIST_MAX) {
         cm_msg(MINFO, "bk_list", "over %i banks -> truncated", BANKLIST_MAX);
         return (nbk - 1);
      }
      if (bk_is32(event))
         strncat(bklist, (char *) pmbk32->name, 4);
      else
         strncat(bklist, (char *) pmbk->name, 4);
   }
   while (1);
   return (nbk);
}

/********************************************************************/
/**
Locates a MIDAS bank of given name inside an event.
@param event pointer to current composed event
@param name bank name to look for
@param pdata pointer to data area of bank, NULL if bank not found
@return number of values inside the bank
*/
INT bk_locate(void *event, const char *name, void *pdata)
{
   BANK *pbk;
   BANK32 *pbk32;
   DWORD dname;

   if (bk_is32(event)) {
      pbk32 = (BANK32 *) (((BANK_HEADER *) event) + 1);
      strncpy((char *) &dname, name, 4);
      while ((DWORD) ((char *) pbk32 - (char *) event) <
             ((BANK_HEADER *) event)->data_size + sizeof(BANK_HEADER)) {
         if (*((DWORD *) pbk32->name) == dname) {
            *((void **) pdata) = pbk32 + 1;
            if (tid_size[pbk32->type & 0xFF] == 0)
               return pbk32->data_size;
            return pbk32->data_size / tid_size[pbk32->type & 0xFF];
         }
         pbk32 = (BANK32 *) ((char *) (pbk32 + 1) + ALIGN8(pbk32->data_size));
      }
   } else {
      pbk = (BANK *) (((BANK_HEADER *) event) + 1);
      strncpy((char *) &dname, name, 4);
      while ((DWORD) ((char *) pbk - (char *) event) <
             ((BANK_HEADER *) event)->data_size + sizeof(BANK_HEADER)) {
         if (*((DWORD *) pbk->name) == dname) {
            *((void **) pdata) = pbk + 1;
            if (tid_size[pbk->type & 0xFF] == 0)
               return pbk->data_size;
            return pbk->data_size / tid_size[pbk->type & 0xFF];
         }
         pbk = (BANK *) ((char *) (pbk + 1) + ALIGN8(pbk->data_size));
      }

   }

   /* bank not found */
   *((void **) pdata) = NULL;
   return 0;
}

/********************************************************************/
/**
Finds a MIDAS bank of given name inside an event.
@param pbkh pointer to current composed event
@param name bank name to look for
@param bklen number of elemtents in bank
@param bktype bank type, one of TID_xxx
@param pdata pointer to data area of bank, NULL if bank not found
@return 1 if bank found, 0 otherwise
*/
INT bk_find(BANK_HEADER * pbkh, const char *name, DWORD * bklen, DWORD * bktype, void **pdata)
{
   BANK *pbk;
   BANK32 *pbk32;
   DWORD dname;

   if (bk_is32(pbkh)) {
      pbk32 = (BANK32 *) (pbkh + 1);
      strncpy((char *) &dname, name, 4);
      do {
         if (*((DWORD *) pbk32->name) == dname) {
            *((void **) pdata) = pbk32 + 1;
            if (tid_size[pbk32->type & 0xFF] == 0)
               *bklen = pbk32->data_size;
            else
               *bklen = pbk32->data_size / tid_size[pbk32->type & 0xFF];

            *bktype = pbk32->type;
            return 1;
         }
         pbk32 = (BANK32 *) ((char *) (pbk32 + 1) + ALIGN8(pbk32->data_size));
      } while ((DWORD) ((char *) pbk32 - (char *) pbkh) < pbkh->data_size + sizeof(BANK_HEADER));
   } else {
      pbk = (BANK *) (pbkh + 1);
      strncpy((char *) &dname, name, 4);
      do {
         if (*((DWORD *) pbk->name) == dname) {
            *((void **) pdata) = pbk + 1;
            if (tid_size[pbk->type & 0xFF] == 0)
               *bklen = pbk->data_size;
            else
               *bklen = pbk->data_size / tid_size[pbk->type & 0xFF];

            *bktype = pbk->type;
            return 1;
         }
         pbk = (BANK *) ((char *) (pbk + 1) + ALIGN8(pbk->data_size));
      } while ((DWORD) ((char *) pbk - (char *) pbkh) < pbkh->data_size + sizeof(BANK_HEADER));
   }

   /* bank not found */
   *((void **) pdata) = NULL;
   return 0;
}

/********************************************************************/
/**
Iterates through banks inside an event.
The function can be used to enumerate all banks of an event.
The returned pointer to the bank header has following structure:
\code
typedef struct {
char   name[4];
WORD   type;
WORD   data_size;
} BANK;
\endcode
where type is a TID_xxx value and data_size the size of the bank in bytes.
\code
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
\endcode
@param event Pointer to data area of event.
@param pbk pointer to the bank header, must be NULL for the first call to
this function.
@param pdata Pointer to the bank header, must be NULL for the first
call to this function
@return Size of bank in bytes
*/
INT bk_iterate(void *event, BANK ** pbk, void *pdata)
{
   if (*pbk == NULL)
      *pbk = (BANK *) (((BANK_HEADER *) event) + 1);
   else
      *pbk = (BANK *) ((char *) (*pbk + 1) + ALIGN8((*pbk)->data_size));

   *((void **) pdata) = (*pbk) + 1;

   if ((DWORD) ((char *) *pbk - (char *) event) >= ((BANK_HEADER *) event)->data_size + sizeof(BANK_HEADER)) {
      *pbk = *((BANK **) pdata) = NULL;
      return 0;
   }

   return (*pbk)->data_size;
}


/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************/
INT bk_iterate32(void *event, BANK32 ** pbk, void *pdata)
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
      *pbk = (BANK32 *) (((BANK_HEADER *) event) + 1);
   else
      *pbk = (BANK32 *) ((char *) (*pbk + 1) + ALIGN8((*pbk)->data_size));

   *((void **) pdata) = (*pbk) + 1;

   if ((DWORD) ((char *) *pbk - (char *) event) >= ((BANK_HEADER *) event)->data_size + sizeof(BANK_HEADER)) {
      *pbk = *((BANK32 **) pdata) = NULL;
      return 0;
   }

   return (*pbk)->data_size;
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Swaps bytes from little endian to big endian or vice versa for a whole event.

An event contains a flag which is set by bk_init() to identify
the endian format of an event. If force is FALSE, this flag is evaluated and
the event is only swapped if it is in the "wrong" format for this system.
An event can be swapped to the "wrong" format on purpose for example by a
front-end which wants to produce events in a "right" format for a back-end
analyzer which has different byte ordering.
@param event pointer to data area of event
@param force If TRUE, the event is always swapped, if FALSE, the event
is only swapped if it is in the wrong format.
@return 1==event has been swap, 0==event has not been swapped.
*/
INT bk_swap(void *event, BOOL force)
{
   BANK_HEADER *pbh;
   BANK *pbk;
   BANK32 *pbk32;
   void *pdata;
   WORD type;
   BOOL b32;

   pbh = (BANK_HEADER *) event;

   /* only swap if flags in high 16-bit */
   if (pbh->flags < 0x10000 && !force)
      return 0;

   /* swap bank header */
   DWORD_SWAP(&pbh->data_size);
   DWORD_SWAP(&pbh->flags);

   /* check for 32bit banks */
   b32 = ((pbh->flags & BANK_FORMAT_32BIT) > 0);

   pbk = (BANK *) (pbh + 1);
   pbk32 = (BANK32 *) pbk;

   /* scan event */
   while ((char *) pbk - (char *) pbh < (INT) pbh->data_size + (INT) sizeof(BANK_HEADER)) {
      /* swap bank header */
      if (b32) {
         DWORD_SWAP(&pbk32->type);
         DWORD_SWAP(&pbk32->data_size);
         pdata = pbk32 + 1;
         type = (WORD) pbk32->type;
      } else {
         WORD_SWAP(&pbk->type);
         WORD_SWAP(&pbk->data_size);
         pdata = pbk + 1;
         type = pbk->type;
      }

      /* pbk points to next bank */
      if (b32) {
         pbk32 = (BANK32 *) ((char *) (pbk32 + 1) + ALIGN8(pbk32->data_size));
         pbk = (BANK *) pbk32;
      } else {
         pbk = (BANK *) ((char *) (pbk + 1) + ALIGN8(pbk->data_size));
         pbk32 = (BANK32 *) pbk;
      }

      switch (type) {
      case TID_WORD:
      case TID_SHORT:
         while ((char *) pdata < (char *) pbk) {
            WORD_SWAP(pdata);
            pdata = (void *) (((WORD *) pdata) + 1);
         }
         break;

      case TID_DWORD:
      case TID_INT:
      case TID_BOOL:
      case TID_FLOAT:
         while ((char *) pdata < (char *) pbk) {
            DWORD_SWAP(pdata);
            pdata = (void *) (((DWORD *) pdata) + 1);
         }
         break;

      case TID_DOUBLE:
         while ((char *) pdata < (char *) pbk) {
            QWORD_SWAP(pdata);
            pdata = (void *) (((double *) pdata) + 1);
         }
         break;
      }
   }

   return CM_SUCCESS;
}

/**dox***************************************************************/

/** @} *//* end of bkfunctionc */


/**dox***************************************************************/
/** @addtogroup rbfunctionc
 *
 *  @{  */

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS
/********************************************************************/

/********************************************************************\
*                                                                    *
*                 Ring buffer functions                              *
*                                                                    *
* Provide an inter-thread buffer scheme for handling front-end       *
* events. This code allows concurrent data acquisition, calibration  *
* and network transfer on a multi-CPU machine. One thread reads      *
* out the data, passes it vis the ring buffer functions              *
* to another thread running on the other CPU, which can then         *
* calibrate and/or send the data over the network.                   *
*                                                                    *
\********************************************************************/

typedef struct {
   unsigned char *buffer;
   unsigned int size;
   unsigned int max_event_size;
   unsigned char *rp;
   unsigned char *wp;
   unsigned char *ep;
} RING_BUFFER;

#define MAX_RING_BUFFER 100

RING_BUFFER rb[MAX_RING_BUFFER];

volatile int _rb_nonblocking = 0;

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Set all rb_get_xx to nonblocking. Needed in multi-thread
environments for stopping all theads without deadlock
@return DB_SUCCESS
*/
int rb_set_nonblocking()
/********************************************************************\

  Routine: rb_set_nonblocking

  Purpose: Set all rb_get_xx to nonblocking. Needed in multi-thread
           environments for stopping all theads without deadlock

  Input:
    NONE

  Output:
    NONE

  Function value:
    DB_SUCCESS       Successful completion

\********************************************************************/
{
   _rb_nonblocking = 1;

   return DB_SUCCESS;
}

/********************************************************************/
/**
Create a ring buffer with a given size

Provide an inter-thread buffer scheme for handling front-end
events. This code allows concurrent data acquisition, calibration
and network transfer on a multi-CPU machine. One thread reads
out the data, passes it via the ring buffer functions
to another thread running on the other CPU, which can then
calibrate and/or send the data over the network.

@param size             Size of ring buffer, must be larger than
                         2*max_event_size
@param max_event_size   Maximum event size to be placed into
@param *handle          Handle to ring buffer
@return DB_SUCCESS, DB_NO_MEMORY, DB_INVALID_PARAM
*/
int rb_create(int size, int max_event_size, int *handle)
/********************************************************************\

  Routine: rb_create

  Purpose: Create a ring buffer with a given size

  Input:
    int size             Size of ring buffer, must be larger than
                         2*max_event_size
    int max_event_size   Maximum event size to be placed into
                         ring buffer
  Output:
    int *handle          Handle to ring buffer

  Function value:
    DB_SUCCESS           Successful completion
    DB_NO_MEMORY         Maximum number of ring buffers exceeded
    DB_INVALID_PARAM     Invalid event size specified

\********************************************************************/
{
   int i;

   for (i = 0; i < MAX_RING_BUFFER; i++)
      if (rb[i].buffer == NULL)
         break;

   if (i == MAX_RING_BUFFER)
      return DB_NO_MEMORY;

   if (size < max_event_size * 2)
      return DB_INVALID_PARAM;

   memset(&rb[i], 0, sizeof(RING_BUFFER));
   rb[i].buffer = (unsigned char *) M_MALLOC(size);
   assert(rb[i].buffer);
   rb[i].size = size;
   rb[i].max_event_size = max_event_size;
   rb[i].rp = rb[i].buffer;
   rb[i].wp = rb[i].buffer;
   rb[i].ep = rb[i].buffer;

   *handle = i + 1;

   return DB_SUCCESS;
}

/********************************************************************/
/**
Delete a ring buffer
@param handle  Handle of the ring buffer
@return  DB_SUCCESS
*/
int rb_delete(int handle)
/********************************************************************\

  Routine: rb_delete

  Purpose: Delete a ring buffer

  Input:
    none
  Output:
    int handle       Handle to ring buffer

  Function value:
    DB_SUCCESS       Successful completion

\********************************************************************/
{
   if (handle < 0 || handle >= MAX_RING_BUFFER || rb[handle - 1].buffer == NULL)
      return DB_INVALID_HANDLE;

   M_FREE(rb[handle - 1].buffer);
   rb[handle - 1].buffer = NULL;
   memset(&rb[handle - 1], 0, sizeof(RING_BUFFER));

   return DB_SUCCESS;
}

/********************************************************************/
/**
Retrieve write pointer where new data can be written
@param handle               Ring buffer handle
@param millisec             Optional timeout in milliseconds if
                              buffer is full. Zero to not wait at
                              all (non-blocking)
@param  **p                  Write pointer
@return DB_SUCCESS, DB_TIMEOUT, DB_INVALID_HANDLE
*/
int rb_get_wp(int handle, void **p, int millisec)
/********************************************************************\

Routine: rb_get_wp

  Purpose: Retrieve write pointer where new data can be written

  Input:
     int handle               Ring buffer handle
     int millisec             Optional timeout in milliseconds if
                              buffer is full. Zero to not wait at
                              all (non-blocking)

  Output:
    char **p                  Write pointer

  Function value:
    DB_SUCCESS       Successful completion

\********************************************************************/
{
   int h, i;
   unsigned char *rp;

   if (handle < 1 || handle > MAX_RING_BUFFER || rb[handle - 1].buffer == NULL)
      return DB_INVALID_HANDLE;

   h = handle - 1;

   for (i = 0; i <= millisec / 10; i++) {

      rp = rb[h].rp;            // keep local copy, rb[h].rp might be changed by other thread

      /* check if enough size for wp >= rp without wrap-around */
      if (rb[h].wp >= rp
          && rb[h].wp + rb[h].max_event_size <= rb[h].buffer + rb[h].size - rb[h].max_event_size) {
         *p = rb[h].wp;
         return DB_SUCCESS;
      }

      /* check if enough size for wp >= rp with wrap-around */
      if (rb[h].wp >= rp && rb[h].wp + rb[h].max_event_size > rb[h].buffer + rb[h].size - rb[h].max_event_size && rb[h].rp > rb[h].buffer) {    // next increment of wp wraps around, so need space at beginning
         *p = rb[h].wp;
         return DB_SUCCESS;
      }

      /* check if enough size for wp < rp */
      if (rb[h].wp < rp && rb[h].wp + rb[h].max_event_size < rp) {
         *p = rb[h].wp;
         return DB_SUCCESS;
      }

      if (millisec == 0)
         return DB_TIMEOUT;

      if (_rb_nonblocking)
         return DB_TIMEOUT;

      /* wait one time slice */
      ss_sleep(10);
   }

   return DB_TIMEOUT;
}

/********************************************************************/
/** rb_increment_wp

Increment current write pointer, making the data at
the write pointer available to the receiving thread
@param handle               Ring buffer handle
@param size                 Number of bytes placed at the WP
@return DB_SUCCESS, DB_INVALID_PARAM, DB_INVALID_HANDLE
*/
int rb_increment_wp(int handle, int size)
/********************************************************************\

  Routine: rb_increment_wp

  Purpose: Increment current write pointer, making the data at
           the write pointer available to the receiving thread

  Input:
     int handle               Ring buffer handle
     int size                 Number of bytes placed at the WP

  Output:
    NONE

  Function value:
    DB_SUCCESS                Successful completion
    DB_INVALID_PARAM          Event size too large or invalid handle
\********************************************************************/
{
   int h;
   unsigned char *old_wp, *new_wp;

   if (handle < 1 || handle > MAX_RING_BUFFER || rb[handle - 1].buffer == NULL)
      return DB_INVALID_HANDLE;

   h = handle - 1;

   if ((DWORD) size > rb[h].max_event_size)
      return DB_INVALID_PARAM;

   old_wp = rb[h].wp;
   new_wp = rb[h].wp + size;

   /* wrap around wp if not enough space */
   if (new_wp > rb[h].buffer + rb[h].size - rb[h].max_event_size) {
      rb[h].ep = new_wp;
      new_wp = rb[h].buffer;
      assert(rb[h].rp != rb[h].buffer);
   }

   rb[h].wp = new_wp;

   return DB_SUCCESS;
}

/********************************************************************/
/**
Obtain the current read pointer at which new data is
available with optional timeout

@param  handle               Ring buffer handle
@param  millisec             Optional timeout in milliseconds if
                             buffer is full. Zero to not wait at
                             all (non-blocking)

@param **p                 Address of pointer pointing to newly
                             available data. If p == NULL, only
                             return status.
@return  DB_SUCCESS, DB_TIEMOUT, DB_INVALID_HANDLE

*/
int rb_get_rp(int handle, void **p, int millisec)
/********************************************************************\

  Routine: rb_get_rp

  Purpose: Obtain the current read pointer at which new data is
           available with optional timeout

  Input:
    int handle               Ring buffer handle
    int millisec             Optional timeout in milliseconds if
                             buffer is full. Zero to not wait at
                             all (non-blocking)

  Output:
    char **p                 Address of pointer pointing to newly
                             available data. If p == NULL, only
                             return status.

  Function value:
    DB_SUCCESS       Successful completion

\********************************************************************/
{
   int i, h;

   if (handle < 1 || handle > MAX_RING_BUFFER || rb[handle - 1].buffer == NULL)
      return DB_INVALID_HANDLE;

   h = handle - 1;

   for (i = 0; i <= millisec / 10; i++) {

      if (rb[h].wp != rb[h].rp) {
         if (p != NULL)
            *p = rb[handle - 1].rp;
         return DB_SUCCESS;
      }

      if (millisec == 0)
         return DB_TIMEOUT;

      if (_rb_nonblocking)
         return DB_TIMEOUT;

      /* wait one time slice */
      ss_sleep(10);
   }

   return DB_TIMEOUT;
}

/********************************************************************/
/**
Increment current read pointer, freeing up space for the writing thread.

@param handle               Ring buffer handle
@param size                 Number of bytes to free up at current
                              read pointer
@return  DB_SUCCESS, DB_INVALID_PARAM

*/
int rb_increment_rp(int handle, int size)
/********************************************************************\

  Routine: rb_increment_rp

  Purpose: Increment current read pointer, freeing up space for
           the writing thread.

  Input:
     int handle               Ring buffer handle
     int size                 Number of bytes to free up at current
                              read pointer

  Output:
    NONE

  Function value:
    DB_SUCCESS                Successful completion
    DB_INVALID_PARAM          Event size too large or invalid handle

\********************************************************************/
{
   int h;

   unsigned char *new_rp, *old_rp;

   if (handle < 1 || handle > MAX_RING_BUFFER || rb[handle - 1].buffer == NULL)
      return DB_INVALID_HANDLE;

   h = handle - 1;

   if ((DWORD) size > rb[h].max_event_size)
      return DB_INVALID_PARAM;

   old_rp = rb[h].rp;
   new_rp = rb[h].rp + size;

   /* wrap around if not enough space left */
   if (new_rp + rb[h].max_event_size > rb[h].buffer + rb[h].size)
      new_rp = rb[h].buffer;

   rb[handle - 1].rp = new_rp;

   return DB_SUCCESS;
}

/********************************************************************/
/**
Return number of bytes in a ring buffer

@param handle              Handle of the buffer to get the info
@param *n_bytes            Number of bytes in buffer
@return DB_SUCCESS, DB_INVALID_HANDLE
*/
int rb_get_buffer_level(int handle, int *n_bytes)
/********************************************************************\

  Routine: rb_get_buffer_level

  Purpose: Return number of bytes in a ring buffer

  Input:
    int handle              Handle of the buffer to get the info

  Output:
    int *n_bytes            Number of bytes in buffer

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Buffer handle is invalid

\********************************************************************/
{
   int h;

   if (handle < 1 || handle > MAX_RING_BUFFER || rb[handle - 1].buffer == NULL)
      return DB_INVALID_HANDLE;

   h = handle - 1;

   if (rb[h].wp >= rb[h].rp)
      *n_bytes = (POINTER_T) rb[h].wp - (POINTER_T) rb[h].rp;
   else
      *n_bytes =
          (POINTER_T) rb[h].ep - (POINTER_T) rb[h].rp + (POINTER_T) rb[h].wp - (POINTER_T) rb[h].buffer;

   return DB_SUCCESS;
}

/**dox***************************************************************/
                  /** @} *//* end of rbfunctionc */

/**dox***************************************************************/
                  /** @} *//* end of midasincludecode */
