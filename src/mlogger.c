/********************************************************************\

  Name:         mlogger.c
  Created by:   Stefan Ritt

  Contents:     MIDAS logger program

  $Log$
  Revision 1.31  1999/11/10 10:38:36  midas
  Lock database for db_notify_clients

  Revision 1.30  1999/11/09 13:17:26  midas
  Added secure ODB feature

  Revision 1.29  1999/10/27 15:14:56  midas
  Exclude /dev/null from open file check

  Revision 1.28  1999/10/27 08:17:25  midas
  Increased history size to 15 and added limit check

  Revision 1.27  1999/10/18 14:47:16  midas
  fixed compiler warning

  Revision 1.26  1999/10/18 14:46:04  midas
  fixed compiler warning

  Revision 1.25  1999/10/18 14:41:52  midas
  Use /programs/<name>/Watchdog timeout in all programs as timeout value. The
  default value can be submitted by calling cm_connect_experiment1(..., timeout)

  Revision 1.24  1999/10/15 12:15:31  midas
  Made logger timout controlled by /logger/logger timeout.

  Revision 1.23  1999/10/11 15:03:52  midas
  Fixed bug with daemon flag

  Revision 1.22  1999/10/11 15:02:23  midas
  Added -D (daemon) flag

  Revision 1.21  1999/10/06 08:43:38  midas
  Fixed bug with system history (event_id instead of index)

  Revision 1.20  1999/09/23 12:45:49  midas
  Added 32 bit banks

  Revision 1.19  1999/09/22 15:39:36  midas
  Logger won't start run if disk file already exists

  Revision 1.18  1999/08/23 13:28:55  midas
  Round up system history to 10

  Revision 1.17  1999/08/23 12:59:08  midas
  Fixed bug with defining system events

  Revision 1.16  1999/08/17 13:02:58  midas
  Revised history system

  Revision 1.14  1999/07/23 07:00:56  midas
  Added automatic tape rewind when tape gets full

  Revision 1.13  1999/06/28 12:36:00  midas
  Added period limit for writing histories

  Revision 1.12  1999/06/25 08:29:01  midas
  Fixed bug which prevented logge to write data when JMidas is open

  Revision 1.11  1999/06/23 13:38:08  midas
  Increased ASCII buffer in dump_write

  Revision 1.10  1999/06/23 09:31:01  midas
  Modified error message

  Revision 1.9  1999/05/03 10:39:56  midas
  Log system hisotry not peridically, but on every change

  Revision 1.8  1999/04/29 12:14:34  midas
  Allow for open statistics record when starting run

  Revision 1.7  1999/04/27 15:15:49  midas
  Added system event in history

  Revision 1.6  1998/12/10 12:50:11  midas
  - removed spinning bars "|\-/" which caused terminal hangup if started in bg
  - changed behaviour if logger finds tape with data. Previously the tape was
    spooled automatically, now a error message is printed which asks the user
    to either spool the tape manually or erase it manually.
  - logger abort with "!" now works without a return under UNIX

  Revision 1.5  1998/11/25 23:43:42  midas
  Addef fflush() after printf(".. bars ..")

  Revision 1.4  1998/10/29 14:37:01  midas
  Added '!' to stop logger

  Revision 1.3  1998/10/22 12:40:34  midas
  Added "oflag" to ss_tape_open()

  Revision 1.2  1998/10/12 12:19:02  midas
  Added Log tag in header


\********************************************************************/

#include "midas.h"
#include "msystem.h"
#include "hardware.h"
#include "ftplib.h"

#define INCLUDE_LOGGING
#include "ybos.h"

/*---- globals -----------------------------------------------------*/

#define LOGGER_TIMEOUT 60000

#define MAX_CHANNELS 10
#define MAX_HISTORY  20

BOOL in_stop_transition = FALSE;
BOOL auto_restart = FALSE;
BOOL tape_message = TRUE;

LOG_CHN log_chn[MAX_CHANNELS];

struct {
  WORD  event_id;
  void  *buffer;
  INT   buffer_size;
  HNDLE hKeyVar;
  DWORD period;
  DWORD last_log;
} hist_log[MAX_HISTORY];

HNDLE hDB;

/*---- ODB records -------------------------------------------------*/

char *channel_str = "\
[Settings]\n\
Active = BOOL : 1\n\
Type = STRING : [8] Disk\n\
Filename = STRING : [256] run%05d.mid\n\
Format = STRING : [8] MIDAS\n\
ODB Dump = BOOL : 1\n\
Log messages = DWORD : 0\n\
Buffer = STRING : [32] SYSTEM\n\
Event ID = INT : -1\n\
Trigger Mask = INT : -1\n\
Event limit = DWORD : 0\n\
Byte limit = DOUBLE : 0\n\
Tape capacity = DOUBLE : 0\n\
\n\
[Statistics]\n\
Events written = DOUBLE : 0\n\
Bytes written = DOUBLE : 0\n\
Bytes written total = DOUBLE : 0\n\
Files written = INT : 0\n\
";

/*---- data structures for MIDAS format ----------------------------*/

typedef struct {
  char *buffer;
  char *write_pointer;
} MIDAS_INFO;

/*---- forward declarations ----------------------------------------*/

void receive_event(HNDLE hBuf, HNDLE request_id, EVENT_HEADER *pheader, void *pevent);
INT  log_write(LOG_CHN *log_chn, EVENT_HEADER *pheader);
void log_system_history(HNDLE hDB, HNDLE hKey, void *info);

/*== common code FAL/MLOGGER start =================================*/

/*---- Logging initialization --------------------------------------*/

void logger_init()
{
INT   size, status;
BOOL  flag;
HNDLE hKey;
char  str[256];

  /*---- create /logger entries -----*/

  cm_get_path(str);
  size = sizeof(str);
  db_get_value(hDB, 0, "/Logger/Data dir", str, &size, TID_STRING);

  strcpy(str, "midas.log");
  size = sizeof(str);
  db_get_value(hDB, 0, "/Logger/Message file", str, &size, TID_STRING);

  size = sizeof(BOOL);
  flag = TRUE;
  db_get_value(hDB, 0, "/Logger/Write data", &flag, &size, TID_BOOL);

  flag = FALSE;
  db_get_value(hDB, 0, "/Logger/ODB Dump", &flag, &size, TID_BOOL);

  strcpy(str, "run%05d.odb");
  size = sizeof(str);
  db_get_value(hDB, 0, "/Logger/ODB Dump File", str, &size, TID_STRING);

  flag = FALSE;
  size = sizeof(BOOL);
  db_get_value(hDB, 0, "/Logger/Auto restart", &flag, &size, TID_BOOL);

  flag = TRUE;
  db_get_value(hDB, 0, "/Logger/Tape message", &flag, &size, TID_BOOL);

  /* create at least one logging channel */
  status = db_find_key(hDB, 0, "/Logger/Channels/0", &hKey);
  if (status != DB_SUCCESS)
    {
    /* if no channels are defined, define at least one */
    status = db_create_record(hDB, 0, "/Logger/Channels/0", channel_str);
    if (status != DB_SUCCESS)
      cm_msg(MERROR, "logger_init", "Cannot create channel entry in database");
    }
}

/*---- ODB dump routine --------------------------------------------*/

void log_odb_dump(LOG_CHN *log_chn, short int event_id, INT run_number)
{
INT          status, buffer_size, size;
EVENT_HEADER *pevent;

  /* write ODB dump */
  buffer_size = 10000;
  do
    {
    pevent = malloc(buffer_size);
    if (pevent == NULL)
      {
      cm_msg(MERROR, "log_odb_dump", "Cannot allocate ODB dump buffer");
      break;
      }

    size = buffer_size - sizeof(EVENT_HEADER);
    status = db_copy(hDB, 0, (char *) (pevent+1), &size, "");
    if (status != DB_TRUNCATED)
      {
      bm_compose_event(pevent, event_id, MIDAS_MAGIC, buffer_size-sizeof(EVENT_HEADER)-size+1, 
                       run_number);
      log_write(log_chn, pevent);
      break;
      }

    /* increase buffer size if truncated */
    free(pevent);
    buffer_size *= 2;
    } while (1);
 }

/*---- ODB save routine --------------------------------------------*/

void odb_save(char *filename)
{
int  size;
char dir[256];
char path[256];

  size = sizeof(dir);
  dir[0] = 0;
  db_get_value(hDB, 0, "/Logger/Data Dir", dir, &size, TID_STRING);
  if (dir[0] != 0)
    if (dir[strlen(dir)-1] != DIR_SEPARATOR)
      strcat(dir, DIR_SEPARATOR_STR);
  strcpy(path, dir);
  strcat(path, filename);

  db_save(hDB, 0, path, FALSE);
}

/*---- open tape and check for data --------------------------------*/

INT tape_open(char *dev, INT *handle)
{
INT  status, count;
char buffer[16];

  status = ss_tape_open(dev, O_RDWR | O_CREAT | O_TRUNC, handle);
  if (status != SS_SUCCESS)
    return status;

  /* check if tape contains data */
  count = sizeof(buffer);
  status = ss_tape_read(*handle, buffer, &count);

  if (count == sizeof(buffer))
    {
    /* tape contains data -> don't start */
    ss_tape_rskip(*handle, -1);
    cm_msg(MINFO, "tape_open", "Tape contains data, please spool tape with 'mtape seod'");
    cm_msg(MINFO, "tape_open", "or erase it with 'mtape weof', 'mtape rewind', then try again.");
    ss_tape_close(*handle);
    return SS_TAPE_ERROR;
    }

  return SS_SUCCESS;
}

/*---- open FTP channel --------------------------------------------*/

INT ftp_error(char *message)
{
  cm_msg(MERROR, "ftp_error", message);
  return 1;
}

INT ftp_open(char *destination, FTP_CON **con)
{
INT   status;
short port;
char  *token, host_name[HOST_NAME_LENGTH], 
      user[32], pass[32], directory[256], file_name[256];

  /* 
  destination should have the form:
  host, port, user, password, directory, run%05d.mid
  */

  /* break destination in components */
  token = strtok(destination, ",");
  if (token)
    strcpy(host_name, token);

  token = strtok(NULL, ", ");
  if (token)
    port = atoi(token);

  token = strtok(NULL, ", ");
  if (token)
    strcpy(user, token);

  token = strtok(NULL, ", ");
  if (token)
    strcpy(pass, token);

  token = strtok(NULL, ", ");
  if (token)
    strcpy(directory, token);

  token = strtok(NULL, ", ");
  if (token)
    strcpy(file_name, token);

#ifdef FAL_MAIN
  ftp_debug(NULL, ftp_error);
#else
  ftp_debug((int (*)(char *)) puts, ftp_error);
#endif

  status = ftp_login(con, host_name, port, user, pass, "");
  if (status >= 0)
    return status;

  status = ftp_chdir(*con, directory);
  if (status >= 0)
    return status;

  status = ftp_binary(*con);
  if (status >= 0)
    return status;

  if (ftp_open_write(*con, file_name) >= 0)
    return (*con)->err_no;

  return SS_SUCCESS;
}

/*---- MIDAS format routines ---------------------------------------*/

INT midas_flush_buffer(LOG_CHN *log_chn)
{     
INT         status, size, written;
MIDAS_INFO  *info;

  info = (MIDAS_INFO *) log_chn->format_info;
  size = (INT) info->write_pointer - (INT) info->buffer;

  if (size == 0)
    return SS_SUCCESS;

  /* write record to device */
  if (log_chn->type == LOG_TYPE_TAPE)
    status = ss_tape_write(log_chn->handle, info->buffer, size);
  else if (log_chn->type == LOG_TYPE_FTP)
    status = ftp_send(((FTP_CON *) log_chn->ftp_con)->data, info->buffer, size) == size ?
             SS_SUCCESS : SS_FILE_ERROR;
  else
    {
#ifdef OS_WINNT
    WriteFile((HANDLE) log_chn->handle, info->buffer, size, &written, NULL);
#else
    written = write(log_chn->handle, info->buffer, size);
#endif
    status = written == size ? SS_SUCCESS : SS_FILE_ERROR;
    }

  info->write_pointer = info->buffer;

  return status;
}


/*------------------------------------------------------------------*/

INT midas_write(LOG_CHN *log_chn, EVENT_HEADER *pevent, INT evt_size)
{
INT         status, size_left;
MIDAS_INFO  *info;
  
  info = (MIDAS_INFO *) log_chn->format_info;

  /* check if event fits into buffer */
  size_left = TAPE_BUFFER_SIZE - ((INT) info->write_pointer - (INT) info->buffer);

  if (size_left < evt_size)
    {
    /* copy first part of event */
    memcpy(info->write_pointer, pevent, size_left);
    info->write_pointer += size_left;

    /* flush buffer */
    status = midas_flush_buffer(log_chn);
    if (status != SS_SUCCESS)
      return status;

    /* several writes for large events */
    while (evt_size - size_left >= TAPE_BUFFER_SIZE)
      {
      memcpy(info->buffer, (char *) pevent + size_left, TAPE_BUFFER_SIZE);
      info->write_pointer += TAPE_BUFFER_SIZE;
      size_left += TAPE_BUFFER_SIZE;

      status = midas_flush_buffer(log_chn);
      if (status != SS_SUCCESS)
        return status;
      }

    /* copy remaining part of event */
    memcpy(info->buffer, (char *) pevent + size_left, evt_size - size_left);
    info->write_pointer = info->buffer + (evt_size - size_left);
    }
  else
    {
    /* copy event to buffer */
    memcpy(info->write_pointer, pevent, evt_size);
    info->write_pointer += evt_size;
    }
  
  /* update statistics */
  log_chn->statistics.events_written++;
  log_chn->statistics.bytes_written += pevent->data_size+sizeof(EVENT_HEADER);
  log_chn->statistics.bytes_written_total += pevent->data_size+sizeof(EVENT_HEADER);

  return SS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT midas_log_open(LOG_CHN *log_chn, INT run_number)
{
MIDAS_INFO   *info;
INT          status;

  /* allocate MIDAS buffer info */
  log_chn->format_info = malloc(sizeof(MIDAS_INFO));

  info = (MIDAS_INFO *) log_chn->format_info;
  if (info == NULL)
    {
    log_chn->handle = 0;
    return SS_NO_MEMORY;
    }

  /* allocate full ring buffer for that channel */
  if ( (info->buffer = (char  *) malloc(TAPE_BUFFER_SIZE)) == NULL)
    {
    free(info);
    log_chn->handle = 0;
    return SS_NO_MEMORY;
    }
  
  info->write_pointer = info->buffer;

  /* Create device channel */
  if (log_chn->type == LOG_TYPE_TAPE)
    {
    status = tape_open(log_chn->path, &log_chn->handle);
    if (status != SS_SUCCESS)
      {
      free(info->buffer);
      free(info);
      log_chn->handle = 0;
      return status;
      }
    }
  else if (log_chn->type == LOG_TYPE_FTP)
    {
    status = ftp_open(log_chn->path, (FTP_CON **) (&log_chn->ftp_con));
    if (status != SS_SUCCESS)
      {
      free(info->buffer);
      free(info);
      log_chn->handle = 0;
      return status;
      }
    else
      log_chn->handle = 1;
    }
  else
    {
    /* check if file exists */
    if (strstr(log_chn->path, "null") == NULL)
      {
      log_chn->handle = open(log_chn->path, O_RDONLY);
      if (log_chn->handle > 0)
        {
        close(log_chn->handle);
        free(info->buffer);
        free(info);
        log_chn->handle = 0;
        return SS_FILE_EXISTS;
        }
      }

#ifdef OS_WINNT       
    log_chn->handle = (int) CreateFile(log_chn->path, GENERIC_WRITE, FILE_SHARE_READ, NULL, 
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH | 
                      FILE_FLAG_SEQUENTIAL_SCAN, 0);

#else
    log_chn->handle = open(log_chn->path, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644);
#endif

    if (log_chn->handle < 0)
      {
      free(info->buffer);
      free(info);
      log_chn->handle = 0;
      return SS_FILE_ERROR;
      }
    }

  /* write ODB dump */
  if (log_chn->settings.odb_dump)
    log_odb_dump(log_chn, EVENTID_BOR, run_number);

  return SS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT midas_log_close(LOG_CHN *log_chn, INT run_number)
{
  /* write ODB dump */
  if (log_chn->settings.odb_dump)
    log_odb_dump(log_chn, EVENTID_EOR, run_number);

  midas_flush_buffer(log_chn);

  /* Write EOF if Tape */
  if (log_chn->type == LOG_TYPE_TAPE)
    {
    /* writing EOF mark on tape Fonly */
    ss_tape_write_eof(log_chn->handle);
    ss_tape_close(log_chn->handle);
    }
  else if (log_chn->type == LOG_TYPE_FTP)
    {
    ftp_close((FTP_CON *) log_chn->ftp_con);
    ftp_bye((FTP_CON *) log_chn->ftp_con);
    }
  else
#ifdef OS_WINNT
    CloseHandle((HANDLE) log_chn->handle);
#else
    close(log_chn->handle);
#endif

  free(((MIDAS_INFO *) log_chn->format_info)->buffer);
  free(log_chn->format_info);

  return SS_SUCCESS;
}

/*-- db_get_event_definition ---------------------------------------*/

typedef struct {
  short int event_id;
  WORD      format;
  HNDLE     hDefKey;
  } EVENT_DEF;

EVENT_DEF *db_get_event_definition(short int event_id)
{
INT   i, index, status, size;
char  str[80];
HNDLE hKey, hKeyRoot;
WORD  id;

#define EVENT_DEF_CACHE_SIZE 30
static EVENT_DEF *event_def=NULL;

  /* allocate memory for cache */
  if (event_def == NULL)
    event_def = calloc(EVENT_DEF_CACHE_SIZE, sizeof(EVENT_DEF));

  /* lookup if event definition in cache */
  for (i=0 ; event_def[i].event_id ; i++)
    if (event_def[i].event_id == event_id)
      return &event_def[i];

  /* search free cache entry */
  for (index=0 ; index<EVENT_DEF_CACHE_SIZE ; index++)
    if (event_def[index].event_id == 0)
      break;

  if (index == EVENT_DEF_CACHE_SIZE)
    {
    cm_msg(MERROR, "db_get_event_definition", "too many event definitions");
    return NULL;
    }

  /* check for system events */
  if (event_id < 0)
    {
    event_def[index].event_id = event_id;
    event_def[index].format   = FORMAT_ASCII;
    event_def[index].hDefKey  = 0;
    return &event_def[index];
    }

  status = db_find_key(hDB, 0, "/equipment", &hKeyRoot);
  if (status != DB_SUCCESS)
    {
    cm_msg(MERROR, "db_get_event_definition", "cannot find /equipment entry in ODB");
    return NULL;
    }

  for (i=0 ; ; i++)
    {
    /* search for client with specific name */
    status = db_enum_key(hDB, hKeyRoot, i, &hKey);
    if (status == DB_NO_MORE_SUBKEYS)
      {
      sprintf(str, "Cannot find event id %d under /equipment", event_id);
      cm_msg(MERROR, "db_get_event_definition", str);
      return NULL;
      }

    size = sizeof(id);
    status = db_get_value(hDB, hKey, "Common/Event ID", &id, &size, TID_WORD);
    if (status != DB_SUCCESS)
      continue;

    if (id == event_id)
      {
      /* set cache entry */
      event_def[index].event_id = id;

      size = sizeof(str);
      str[0] = 0;
      db_get_value(hDB, hKey, "Common/Format", str, &size, TID_STRING);

      if (equal_ustring(str, "Fixed"))
        event_def[index].format = FORMAT_FIXED;
      else if (equal_ustring(str, "ASCII"))
        event_def[index].format = FORMAT_ASCII;
      else if (equal_ustring(str, "MIDAS"))
        event_def[index].format = FORMAT_MIDAS;
      else if (equal_ustring(str, "YBOS"))
        event_def[index].format = FORMAT_YBOS;
      else if (equal_ustring(str, "DUMP"))
        event_def[index].format = FORMAT_DUMP;
      else
        {
        cm_msg(MERROR, "db_get_event_definition", "unknown data format");
        event_def[index].event_id = 0;
        return NULL;
        }

      db_find_key(hDB, hKey, "Variables", &event_def[index].hDefKey);
      return &event_def[index];
      }
    }
}

/*---- DUMP format routines ----------------------------------------*/

#define STR_INC(p,base) { p+=strlen(p); \
                          if (p > base+sizeof(base)) \
                            cm_msg(MERROR, "STR_INC", "ASCII buffer too small"); }


INT dump_write(LOG_CHN *log_chn, EVENT_HEADER *pevent, INT evt_size)
{
INT         status, size, i, j;
EVENT_DEF   *event_def;  
BANK_HEADER *pbh;
BANK        *pbk;
BANK32      *pbk32;
void        *pdata;
char        buffer[100000], *pbuf, name[5], type_name[10];
HNDLE       hKey;
KEY         key;
DWORD       bkname;
WORD        bktype;

  event_def = db_get_event_definition(pevent->event_id);
  if (event_def == NULL)
    return SS_SUCCESS;

  /* write event header */
  pbuf = buffer;
  name[4] = 0;

  if (pevent->event_id == EVENTID_BOR)
    sprintf(pbuf, "%%ID BOR NR %d\n", pevent->serial_number);
  else if (pevent->event_id == EVENTID_EOR)
    sprintf(pbuf, "%%ID EOR NR %d\n", pevent->serial_number);
  else
    sprintf(pbuf, "%%ID %d TR %d NR %d\n", pevent->event_id, pevent->trigger_mask, 
                                           pevent->serial_number);
  STR_INC(pbuf,buffer);

  /*---- MIDAS format ----------------------------------------------*/
  if (event_def->format == FORMAT_MIDAS)
    {
    LRS1882_DATA   *lrs1882;
    LRS1877_DATA   *lrs1877;
    LRS1877_HEADER *lrs1877_header;

    pbh = (BANK_HEADER *) (pevent+1);
    bk_swap(pbh, FALSE);

    pbk = NULL;
    pbk32 = NULL;
    do
      {
      /* scan all banks */
      if (bk_is32(pbh))
        {
        size = bk_iterate32(pbh, &pbk32, &pdata);
        if (pbk32 == NULL)
          break;
        bkname = *((DWORD *) pbk32->name);
        bktype = (WORD) pbk32->type;
        }
      else
        {
        size = bk_iterate(pbh, &pbk, &pdata);
        if (pbk == NULL)
          break;
        bkname = *((DWORD *) pbk->name);
        bktype = (WORD) pbk->type;
        }

      if (rpc_tid_size(bktype & 0xFF))
        size /= rpc_tid_size(bktype & 0xFF);
      
      lrs1882        = (LRS1882_DATA *)   pdata;
      lrs1877        = (LRS1877_DATA *)   pdata;
      lrs1877_header = (LRS1877_HEADER *) pdata;

      /* write bank header */
      *((DWORD *) name) = bkname;

      if ((bktype & 0xFF00) == 0)
        strcpy(type_name, rpc_tid_name(bktype & 0xFF));
      else if ((bktype & 0xFF00) == TID_LRS1882)
        strcpy(type_name, "LRS1882");
      else if ((bktype & 0xFF00) == TID_LRS1877)
        strcpy(type_name, "LRS1877");
      else if ((bktype & 0xFF00) == TID_PCOS3)
        strcpy(type_name, "PCOS3");
      else
        strcpy(type_name, "unknown");

      sprintf(pbuf, "BK %s TP %s SZ %d\n", name, type_name, size);
      STR_INC(pbuf, buffer);

      /* write data */
      for (i=0 ; i<size ; i++)
        {
        if ((bktype & 0xFF00) == 0)
          db_sprintf(pbuf, pdata, size, i, bktype & 0xFF);

        else if ((bktype & 0xFF00) == TID_LRS1882)
          sprintf(pbuf, "GA %d CH %02d DA %d", 
            lrs1882[i].geo_addr, lrs1882[i].channel, lrs1882[i].data);
        
        else if ((bktype & 0xFF00) == TID_LRS1877)
          {
          if (i==0) /* header */
            sprintf(pbuf, "GA %d BF %d CN %d", 
              lrs1877_header[i].geo_addr, lrs1877_header[i].buffer, lrs1877_header[i].count);
          else      /* data */
            sprintf(pbuf, "GA %d CH %02d ED %d DA %1.1lf", 
              lrs1877[i].geo_addr, lrs1877[i].channel, lrs1877[i].edge, lrs1877[i].data*0.5);
          }

        else if ((bktype & 0xFF00) == TID_PCOS3)
          sprintf(pbuf, "");
        else
          db_sprintf(pbuf, pdata, size, i, bktype & 0xFF);

        strcat(pbuf, "\n");
        STR_INC(pbuf,buffer);
        }

      } while (1);
    }

  /*---- FIXED format ----------------------------------------------*/
  if (event_def->format == FORMAT_FIXED)
    {
    if (event_def->hDefKey == 0)
      cm_msg(MERROR, "dump_write", "cannot find event definition");
    else
      {
      pdata = (char *) (pevent+1);
      for (i=0 ;; i++)
        {
        status = db_enum_key(hDB, event_def->hDefKey, i, &hKey);
        if (status == DB_NO_MORE_SUBKEYS)
          break;

        db_get_key(hDB, hKey, &key);
        sprintf(pbuf, "%s\n", key.name);
        STR_INC(pbuf, buffer);

        /* adjust for alignment */
        pdata = (void *) VALIGN(pdata, min(ss_get_struct_align(),key.item_size));

        for (j=0 ; j<key.num_values ; j++)
          {
          db_sprintf(pbuf, pdata, key.item_size, j, key.type);
          strcat(pbuf, "\n");
          STR_INC(pbuf, buffer);
          }

        /* shift data pointer to next item */
        (char *) pdata += key.item_size*key.num_values;
        }
      }
    }

  /*---- ASCII format ----------------------------------------------*/
  if (event_def->format == FORMAT_ASCII)
    {
    /* write event header to device */
    size = strlen(buffer);
    if (log_chn->type == LOG_TYPE_TAPE)
      status = ss_tape_write(log_chn->handle, buffer, size);
    else
      status = write(log_chn->handle, buffer, size) == 
                     size ? SS_SUCCESS : SS_FILE_ERROR;

    /* write event directly to device */
    size = strlen((char *) (pevent+1));
    if (log_chn->type == LOG_TYPE_TAPE)
      status = ss_tape_write(log_chn->handle, (char *) (pevent+1), size);
    else if (log_chn->type == LOG_TYPE_FTP)
      status = ftp_send(((FTP_CON *) log_chn->ftp_con)->data, buffer, size) == size ?
               SS_SUCCESS : SS_FILE_ERROR;
    else
      status = write(log_chn->handle, (char *) (pevent+1), size) == 
                     size ? SS_SUCCESS : SS_FILE_ERROR;
    }
  else
    {
    /* non-ascii format: only write buffer */

    /* insert empty line after each event */
    strcat(pbuf, "\n");
    size = strlen(buffer);

    /* write record to device */
    if (log_chn->type == LOG_TYPE_TAPE)
      status = ss_tape_write(log_chn->handle, buffer, size);
    else if (log_chn->type == LOG_TYPE_FTP)
      status = ftp_send(((FTP_CON *) log_chn->ftp_con)->data, buffer, size) == size ?
               SS_SUCCESS : SS_FILE_ERROR;
    else
      status = write(log_chn->handle, buffer, size) == 
                     size ? SS_SUCCESS : SS_FILE_ERROR;
    }

  /* update statistics */
  log_chn->statistics.events_written++;
  log_chn->statistics.bytes_written += size;
  log_chn->statistics.bytes_written_total += size;

  return status;
}

/*------------------------------------------------------------------*/

INT dump_log_open(LOG_CHN *log_chn, INT run_number)
{
INT status;

  /* Create device channel */
  if (log_chn->type == LOG_TYPE_TAPE)
    {
    status = tape_open(log_chn->path, &log_chn->handle);
    if (status != SS_SUCCESS)
      {
      log_chn->handle = 0;
      return status;
      }
    }
  else if (log_chn->type == LOG_TYPE_FTP)
    {
    status = ftp_open(log_chn->path, (FTP_CON **) (&log_chn->ftp_con));
    if (status != SS_SUCCESS)
      {
      log_chn->handle = 0;
      return status;
      }
    else
      log_chn->handle = 1;
    }
  else
    {
    log_chn->handle = open(log_chn->path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (log_chn->handle < 0)
      {
      log_chn->handle = 0;
      return SS_FILE_ERROR;
      }
    }

  /* write ODB dump */
  if (log_chn->settings.odb_dump)
    log_odb_dump(log_chn, EVENTID_BOR, run_number);

  return SS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT dump_log_close(LOG_CHN *log_chn, INT run_number)
{
  /* write ODB dump */
  if (log_chn->settings.odb_dump)
    log_odb_dump(log_chn, EVENTID_EOR, run_number);

  /* Write EOF if Tape */
  if (log_chn->type == LOG_TYPE_TAPE)
    {
    /* writing EOF mark on tape only */
    ss_tape_write_eof(log_chn->handle);
    ss_tape_close(log_chn->handle);
    }
  else if (log_chn->type == LOG_TYPE_FTP)
    {
    ftp_close((FTP_CON *) log_chn->ftp_con);
    ftp_bye((FTP_CON *) log_chn->ftp_con);
    }
  else
    close (log_chn->handle);

  return SS_SUCCESS;
}

/*---- ASCII format routines ---------------------------------------*/

INT ascii_write(LOG_CHN *log_chn, EVENT_HEADER *pevent, INT evt_size)
{
INT         status, size, i, j;
EVENT_DEF   *event_def;  
BANK_HEADER *pbh;
BANK        *pbk;
BANK32      *pbk32;
void        *pdata;
char        data[1000];
char        buffer[10000], name[5], type_name[10];
char        *ph, header_line[10000];
char        *pd, data_line[10000];
HNDLE       hKey, hKeyRoot;
KEY         key;
static short int last_event_id = -1;
DWORD       bkname;
WORD        bktype;

  if (pevent->serial_number == 1)
    last_event_id = -1;

  event_def = db_get_event_definition(pevent->event_id);
  if (event_def == NULL)
    return SS_SUCCESS;

  name[4]  = 0;
  header_line[0] = 0;
  data_line[0]   = 0;
  ph = header_line;
  pd = data_line;

  /* write run parameter at begin of run */
  if (pevent->event_id == EVENTID_BOR)
    {
    sprintf(header_line, "%%Run\t");
    sprintf(data_line, "%d\t", pevent->serial_number);    
    STR_INC(ph, header_line);
    STR_INC(pd, data_line);

    status = db_find_key(hDB, 0, "/Experiment/Run parameters", &hKeyRoot);
    if (status == DB_SUCCESS)
      {
      for (i=0 ;; i++)
        {
        status = db_enum_key(hDB, hKeyRoot, i, &hKey);
        if (status == DB_NO_MORE_SUBKEYS)
          break;

        db_get_key(hDB, hKey, &key);
        size = sizeof(data);
        db_get_data(hDB, hKey, data, &size, key.type);

        for (j=0 ; j<key.num_values ; j++)
          {
          if (key.num_values == 1)
            sprintf(ph, "%s\t", key.name);
          else
            sprintf(ph, "%s%d\t", key.name, j);

          STR_INC(ph, header_line);

          db_sprintf(pd, data, key.item_size, j, key.type);
          strcat(pd, "\t");
          STR_INC(pd, data_line);
          }
        }
      }
    }
  else
    {
    sprintf(ph, "%%", pevent->event_id);
    STR_INC(ph, header_line);
    }

  /*---- MIDAS format ----------------------------------------------*/
  if (event_def->format == FORMAT_MIDAS)
    {
    LRS1882_DATA   *lrs1882;
    LRS1877_DATA   *lrs1877;
    LRS1877_HEADER *lrs1877_header;

    pbh = (BANK_HEADER *) (pevent+1);
    bk_swap(pbh, FALSE);

    pbk = NULL;
    pbk32 = NULL;
    do
      {
      /* scan all banks */
      if (bk_is32(pbh))
        {
        size = bk_iterate32(pbh, &pbk32, &pdata);
        if (pbk32 == NULL)
          break;
        bkname = *((DWORD *) pbk32->name);
        bktype = (WORD) pbk32->type;
        }
      else
        {
        size = bk_iterate(pbh, &pbk, &pdata);
        if (pbk == NULL)
          break;
        bkname = *((DWORD *) pbk->name);
        bktype = (WORD) pbk->type;
        }

      if (rpc_tid_size(bktype & 0xFF))
        size /= rpc_tid_size(bktype & 0xFF);
      
      lrs1882        = (LRS1882_DATA *)   pdata;
      lrs1877        = (LRS1877_DATA *)   pdata;
      lrs1877_header = (LRS1877_HEADER *) pdata;

      /* write bank header */
      *((DWORD *) name) = bkname;

      if ((bktype & 0xFF00) == 0)
        strcpy(type_name, rpc_tid_name(bktype & 0xFF));
      else if ((bktype & 0xFF00) == TID_LRS1882)
        strcpy(type_name, "LRS1882");
      else if ((bktype & 0xFF00) == TID_LRS1877)
        strcpy(type_name, "LRS1877");
      else if ((bktype & 0xFF00) == TID_PCOS3)
        strcpy(type_name, "PCOS3");
      else
        strcpy(type_name, "unknown");

      sprintf(ph, "%s[%d]\t", name, size);
      STR_INC(ph, header_line);

      /* write data */
      for (i=0 ; i<size ; i++)
        {
        db_sprintf(pd, pdata, size, i, bktype & 0xFF);
        strcat(pd, "\t");
        STR_INC(pd, data_line);
        }

      } while (1);
    }

  /*---- FIXED format ----------------------------------------------*/
  if (event_def->format == FORMAT_FIXED)
    {
    if (event_def->hDefKey == 0)
      cm_msg(MERROR, "ascii_write", "cannot find event definition");
    else
      {
      pdata = (char *) (pevent+1);
      for (i=0 ;; i++)
        {
        status = db_enum_key(hDB, event_def->hDefKey, i, &hKey);
        if (status == DB_NO_MORE_SUBKEYS)
          break;

        db_get_key(hDB, hKey, &key);

        /* adjust for alignment */
        pdata = (void *) VALIGN(pdata, min(ss_get_struct_align(),key.item_size));

        for (j=0 ; j<key.num_values ; j++)
          {
          if (pevent->event_id != last_event_id)
            {
            if (key.num_values == 1)
              sprintf(ph, "%s\t", key.name);
            else
              sprintf(ph, "%s%d\t", key.name, j);

            STR_INC(ph, header_line);
            }

          db_sprintf(pd, pdata, key.item_size, j, key.type);
          strcat(pd, "\t");
          STR_INC(pd, data_line);
          }

        /* shift data pointer to next item */
        (char *) pdata += key.item_size*key.num_values;
        }
      }
    }

  if (*(pd-1) == '\t')
    *(pd-1) = '\n';

  if (last_event_id != pevent->event_id)
    {
    if (*(ph-1) == '\t')
      *(ph-1) = '\n';
    last_event_id = pevent->event_id;
    strcpy(buffer, header_line);
    strcat(buffer, data_line);
    }
  else
    strcpy(buffer, data_line);

  /* write buffer to device */
  size = strlen(buffer);

  if (log_chn->type == LOG_TYPE_TAPE)
    status = ss_tape_write(log_chn->handle, buffer, size);
  else if (log_chn->type == LOG_TYPE_FTP)
    status = ftp_send(((FTP_CON *) log_chn->ftp_con)->data, buffer, size) == size ?
             SS_SUCCESS : SS_FILE_ERROR;
  else
    status = write(log_chn->handle, buffer, size) == 
                   size ? SS_SUCCESS : SS_FILE_ERROR;

  /* update statistics */
  log_chn->statistics.events_written++;
  log_chn->statistics.bytes_written += size;
  log_chn->statistics.bytes_written_total += size;

  return status;
}

/*------------------------------------------------------------------*/

INT ascii_log_open(LOG_CHN *log_chn, INT run_number)
{
INT status;
EVENT_HEADER event;

  /* Create device channel */
  if (log_chn->type == LOG_TYPE_TAPE)
    {
    status = tape_open(log_chn->path, &log_chn->handle);
    if (status != SS_SUCCESS)
      {
      log_chn->handle = 0;
      return status;
      }
    }
  else if (log_chn->type == LOG_TYPE_FTP)
    {
    status = ftp_open(log_chn->path, (FTP_CON **) (&log_chn->ftp_con));
    if (status != SS_SUCCESS)
      {
      log_chn->handle = 0;
      return status;
      }
    else
      log_chn->handle = 1;
    }
  else
    {
    log_chn->handle = open(log_chn->path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (log_chn->handle < 0)
      {
      log_chn->handle = 0;
      return SS_FILE_ERROR;
      }
    }

  /* write ODB dump */
  if (log_chn->settings.odb_dump)
    {
    event.event_id = EVENTID_BOR;
    event.data_size = 0;
    event.serial_number = run_number;

    ascii_write(log_chn, &event, sizeof(EVENT_HEADER));
    }

  return SS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT ascii_log_close(LOG_CHN *log_chn, INT run_number)
{
  /* Write EOF if Tape */
  if (log_chn->type == LOG_TYPE_TAPE)
    {
    /* writing EOF mark on tape only */
    ss_tape_write_eof(log_chn->handle);
    ss_tape_close(log_chn->handle);
    }
  else if (log_chn->type == LOG_TYPE_FTP)
    {
    ftp_close((FTP_CON *) log_chn->ftp_con);
    ftp_bye((FTP_CON *) log_chn->ftp_con);
    }
  else
    close (log_chn->handle);

  return SS_SUCCESS;
}

/*---- log_open ----------------------------------------------------*/

INT log_open(LOG_CHN *log_chn, INT run_number)
{
INT status;

  if (equal_ustring(log_chn->settings.format, "YBOS"))
    {
    log_chn->format = FORMAT_YBOS;
	  status = ybos_log_open(log_chn, run_number);
    }
  else if (equal_ustring(log_chn->settings.format, "ASCII"))
    {
    log_chn->format = FORMAT_ASCII;
	  status = ascii_log_open(log_chn, run_number);
    }
  else if (equal_ustring(log_chn->settings.format, "DUMP"))
    {
    log_chn->format = FORMAT_DUMP;
	  status = dump_log_open(log_chn, run_number);
    }
  else /* default format is MIDAS */
    {
    log_chn->format = FORMAT_MIDAS;
    status = midas_log_open(log_chn, run_number);
    }

  return status;
}

/*---- log_close ---------------------------------------------------*/

INT log_close(LOG_CHN *log_chn, INT run_number)
{
  if (log_chn->format == FORMAT_YBOS)
    ybos_log_close(log_chn, run_number);

  if (log_chn->format == FORMAT_ASCII)
    ascii_log_close(log_chn, run_number);

  if (log_chn->format == FORMAT_DUMP)
    dump_log_close(log_chn, run_number);

  if (log_chn->format == FORMAT_MIDAS)
    midas_log_close(log_chn, run_number);

  log_chn->statistics.files_written++;
  log_chn->handle = 0;
  log_chn->ftp_con = NULL;

  return SS_SUCCESS;
}

/*---- log_write ---------------------------------------------------*/

INT log_write(LOG_CHN *log_chn, EVENT_HEADER *pevent)
{
INT    status, size, izero, watchdog_timeout;
DWORD  actual_time, start_time;
BOOL   watchdog_flag;
static BOOL stop_requested = FALSE;
static DWORD last_checked = 0;
HNDLE  htape, stats_hkey;
char   tape_name[256];
double dzero;

  start_time = ss_millitime();

  if (log_chn->format == FORMAT_YBOS)
    status = ybos_write(log_chn, pevent, pevent->data_size+sizeof(EVENT_HEADER));

  if (log_chn->format == FORMAT_ASCII)
    status = ascii_write(log_chn, pevent, pevent->data_size+sizeof(EVENT_HEADER));

  if (log_chn->format == FORMAT_DUMP)
    status = dump_write(log_chn, pevent, pevent->data_size+sizeof(EVENT_HEADER));

  if (log_chn->format == FORMAT_MIDAS)
    status = midas_write(log_chn, pevent, pevent->data_size+sizeof(EVENT_HEADER));

  actual_time = ss_millitime();
  if (actual_time - start_time > 3000)
    cm_msg(MINFO, "log_write", "Write operation took %d ms", actual_time - start_time);

  if (status != SS_SUCCESS && !stop_requested)
    {
    if (status == SS_IO_ERROR)
      cm_msg(MTALK, "log_write", "Physical IO error on %s, stopping run", log_chn->path);
    else
      cm_msg(MTALK, "log_write", "Error writing to %s, stopping run", log_chn->path);

    stop_requested = TRUE;
    cm_transition(TR_STOP, 0, NULL, 0, ASYNC);
    stop_requested = FALSE;

    return status;
    }

  /* check if event limit is reached */
  if (!stop_requested &&  !in_stop_transition &&
      log_chn->settings.event_limit > 0 &&
      log_chn->statistics.events_written >= log_chn->settings.event_limit)
    {
    stop_requested = TRUE;
    
    cm_msg(MTALK, "log_write", "stopping run after having received %d events", 
                                log_chn->settings.event_limit);

    status = cm_transition(TR_STOP, 0, NULL, 0, ASYNC);
    if (status != CM_SUCCESS)
      cm_msg(MERROR, "log_write", "cannot stop run after reaching event limit");
    stop_requested = FALSE;
    
    /* check if autorestart, main loop will take care of it */
    size = sizeof(BOOL);
    auto_restart = FALSE;
    db_get_value(hDB, 0, "/Logger/Auto restart", &auto_restart, &size, TID_BOOL);

    return status;
    }

  /* check if byte limit is reached */
  if (!stop_requested &&  !in_stop_transition &&
      log_chn->settings.byte_limit > 0 &&
      log_chn->statistics.bytes_written >= log_chn->settings.byte_limit)
    {
    stop_requested = TRUE;
    
    cm_msg(MTALK, "log_write", "stopping run after having received %1.0lf mega bytes", 
                                log_chn->statistics.bytes_written/1E6);

    status = cm_transition(TR_STOP, 0, NULL, 0, ASYNC);
    if (status != CM_SUCCESS)
      cm_msg(MERROR, "log_write", "cannot stop run after reaching bytes limit");
    stop_requested = FALSE;
    
    /* check if autorestart, main loop will take care of it */
    size = sizeof(BOOL);
    auto_restart = FALSE;
    db_get_value(hDB, 0, "/Logger/Auto restart", &auto_restart, &size, TID_BOOL);

    return status;
    }

  /* check if capacity is reached for tapes*/
  if (!stop_requested &&  !in_stop_transition &&
      log_chn->type == LOG_TYPE_TAPE &&
      log_chn->settings.tape_capacity > 0 &&
      log_chn->statistics.bytes_written_total >= log_chn->settings.tape_capacity)
    {
    stop_requested = TRUE;
    cm_msg(MTALK, "log_write", "tape capacity reached, stopping run");

    /* remember tape name */
    strcpy(tape_name, log_chn->path);
    stats_hkey = log_chn->stats_hkey;
    
    status = cm_transition(TR_STOP, 0, NULL, 0, ASYNC);
    if (status != CM_SUCCESS)
      cm_msg(MERROR, "log_write", "cannot stop run after reaching tape capacity");
    stop_requested = FALSE;

    /* rewind tape */
    ss_tape_open(tape_name, O_RDONLY, &htape);
    cm_msg(MTALK, "log_write", "rewinding tape %s, please wait", log_chn->path);

    cm_get_watchdog_params(&watchdog_flag, &watchdog_timeout);
    cm_set_watchdog_params(watchdog_flag, 300000);  /* 5 min for tape rewind */
    ss_tape_unmount(htape);
    ss_tape_close(htape);
    cm_set_watchdog_params(watchdog_flag, watchdog_timeout);

    /* zero statistics */
    dzero = izero = 0;
    db_set_value(hDB, stats_hkey, "Bytes written total", &dzero, 
                 sizeof(dzero), 1, TID_DOUBLE);
    db_set_value(hDB, stats_hkey, "Files written", &izero, 
                 sizeof(izero), 1, TID_INT);
    
    cm_msg(MTALK, "log_write", "Please insert new tape and start new run.");

    return status;
    }

  /* stop run if less than 10MB free disk space */
  actual_time = ss_millitime();
  if (log_chn->type == LOG_TYPE_DISK && actual_time - last_checked > 1000)
    {
    last_checked = actual_time;

    if (ss_disk_free(log_chn->path) < 1E7 && 
        !stop_requested && !in_stop_transition)
      {
      stop_requested = TRUE;
      cm_msg(MTALK, "log_write", "disk nearly full, stopping run");

      status = cm_transition(TR_STOP, 0, NULL, 0, ASYNC);
      if (status != CM_SUCCESS)
        cm_msg(MERROR, "log_write", "cannot stop run after reaching byte limit");
      stop_requested = FALSE;
      }
    }

  return status;
}

/*---- open_history ------------------------------------------------*/

void log_history(HNDLE hDB, HNDLE hKey, void *info);

INT open_history()
{
INT      size, index, i_tag, status, i, j, li, max_event_id;
INT      n_var, n_tags, n_names;
HNDLE    hKeyRoot, hKeyVar, hKeyNames, hLinkKey, hVarKey, hKeyEq, hHistKey, hKey;
DWORD    history;
TAG      *tag;
KEY      key, varkey, linkkey, histkey;
WORD     event_id;
char     str[256], eq_name[NAME_LENGTH], hist_name[NAME_LENGTH];
BOOL     single_names;

  /* set direcotry for history files */
  size = sizeof(str);
  str[0] = 0;
  db_get_value(hDB, 0, "/Logger/Data Dir", str, &size, TID_STRING);
  if (str[0] != 0)
    hs_set_path(str);

  if (db_find_key(hDB, 0, "/History", &hKeyRoot) != DB_SUCCESS)
    {
    /* create default history keys */

    if (db_find_key(hDB, 0, "/Equipment/Trigger/Statistics/Events per sec.", &hKeyEq) == DB_SUCCESS)
      db_create_link(hDB, 0, "/History/Links/System/Trigger per sec.", "/Equipment/Trigger/Statistics/Events per sec.");

    if (db_find_key(hDB, 0, "/Equipment/Trigger/Statistics/kBytes per sec.", &hKeyEq) == DB_SUCCESS)
      db_create_link(hDB, 0, "/History/Links/System/Trigger kB per sec.", "/Equipment/Trigger/Statistics/kBytes per sec.");
    }

  /*---- define equipment events as history ------------------------*/

  max_event_id = 0;

  status = db_find_key(hDB, 0, "/Equipment", &hKeyRoot);
  if (status != DB_SUCCESS)
    {
    cm_msg(MERROR, "open_history", "Cannot find Equipment entry in database");
    return 0;
    }

  /* loop over equipment */
  for (index=0; index < MAX_HISTORY ; index++)
    {
    status = db_enum_key(hDB, hKeyRoot, index, &hKeyEq);
    if (status != DB_SUCCESS)
      break;

    /* check history flag */
    size = sizeof(history);
    db_get_value(hDB, hKeyEq, "Common/Log history", &history, &size, TID_INT);

    /* define history tags only if log history flag is on */
    if (history > 0)
      {
      /* get equipment name */
      db_get_key(hDB, hKeyEq, &key);
      strcpy(eq_name, key.name);

      status = db_find_key(hDB, hKeyEq, "Variables", &hKeyVar);
      if (status != DB_SUCCESS)
        {
        cm_msg(MERROR, "open_history", "Cannot find /Equipment/%s/Variables entry in database", eq_name);
        return 0;
        }

      size = sizeof(event_id);
      db_get_value(hDB, hKeyEq, "Common/Event ID", &event_id, &size, TID_WORD);

      /* count keys in variables tree */
      for (n_var=0,n_tags=0 ;; n_var++)
        {
        status = db_enum_key(hDB, hKeyVar, n_var, &hKey);
        if (status == DB_NO_MORE_SUBKEYS)
          break;
        db_get_key(hDB, hKey, &key);
        n_tags += key.num_values;
        }

      if (n_var == 0)
        cm_msg(MERROR, "open_history", "defined event %d with no variables in ODB", event_id);

      /* create tag array */
      tag = malloc(sizeof(TAG)*n_tags);

      for (i=0,i_tag=0 ;; i++)
        {
        status = db_enum_key(hDB, hKeyVar, i, &hKey);
        if (status == DB_NO_MORE_SUBKEYS)
          break;

        /* get variable key */
        db_get_key(hDB, hKey, &varkey);

        /* look for names */
        db_find_key(hDB, hKeyEq, "Settings/Names", &hKeyNames);
        single_names = (hKeyNames > 0);
        if (hKeyNames)
          {
          /* define tags from names list */
          db_get_key(hDB, hKeyNames, &key);
          n_names = key.num_values;
          }
        else
          {
          sprintf(str, "Settings/Names %s", varkey.name);
          db_find_key(hDB, hKeyEq, str, &hKeyNames);
          if (hKeyNames)
            {
            /* define tags from names list */
            db_get_key(hDB, hKeyNames, &key);
            n_names = key.num_values;
            }
          }

        if (hKeyNames && varkey.num_values > n_names)
          cm_msg(MERROR, "open_history", "/Equipment/%s/Settings/%s contains only %d entries", 
                 eq_name, key.name, n_names);

        if (hKeyNames)
          {
          /* loop over array elements */
          for (j=0 ; j<key.num_values ; j++)
            {
            /* get name #j */
            size = NAME_LENGTH;
            db_get_data_index(hDB, hKeyNames, tag[i_tag].name, &size, j, TID_STRING);

            /* append variable key name for single name array */
            if (single_names)
              {
              strcat(tag[i_tag].name, " ");
              strcat(tag[i_tag].name, varkey.name);
              }

            tag[i_tag].type = varkey.type;
            tag[i_tag].n_data = 1;
            i_tag++;
            }
          }
        else
          {
          strcpy(tag[i_tag].name, varkey.name);
          tag[i_tag].type = varkey.type;
          tag[i_tag].n_data = varkey.num_values;
          i_tag++;
          }
        }

      hs_define_event(event_id, eq_name, tag, sizeof(TAG)*i_tag);
      free(tag);

      /* setup hist_log structure for this event */
      hist_log[index].event_id = event_id;
      hist_log[index].hKeyVar = hKeyVar;
      db_get_record_size(hDB, hKeyVar, 0, &size); 
      hist_log[index].buffer_size = size;
      hist_log[index].buffer = malloc(size);
      hist_log[index].period = history;
      hist_log[index].last_log = 0;
      if (hist_log[index].buffer == NULL)
        {
        cm_msg(MERROR, "open_history", "cannot allocate history buffer");
        return 0;
        }

      /* open hot link to variables */
      status = db_open_record(hDB, hKeyVar, hist_log[index].buffer, 
                              size, MODE_READ, log_history, NULL);
      if (status != DB_SUCCESS)
        cm_msg(MERROR, "open_history", "cannot open variable record for history logging");

      /* remember maximum event id for later use with system events */
      if (event_id > max_event_id)
        max_event_id = event_id;
      }
    else
      {
      hist_log[index].event_id = 0;
      hist_log[index].hKeyVar = 0;
      hist_log[index].buffer = NULL;
      hist_log[index].buffer_size = 0;
      hist_log[index].period = 0;
      }
    }

  if (index == MAX_HISTORY)
    {
    cm_msg(MERROR, "open_history", "too many equipments for history");
    return 0;
    }

  /*---- define linked trees ---------------------------------------*/

  /* round up event id */
  max_event_id = ((int) ((max_event_id+1)/ 10)+1) * 10;

  status = db_find_key(hDB, 0, "/History/Links", &hKeyRoot);
  if (status != DB_SUCCESS)
    {
    cm_msg(MERROR, "open_history", "Cannot find /History/Links entry in database");
    return 0;
    }

  for (li = 0 ; ; li++)
    {
    status = db_enum_link(hDB, hKeyRoot, li, &hHistKey);
    if (status == DB_NO_MORE_SUBKEYS)
      break;

    db_get_key(hDB, hHistKey, &histkey);
    strcpy(hist_name, histkey.name);
    db_enum_key(hDB, hKeyRoot, li, &hHistKey);

    db_get_key(hDB, hHistKey, &key);
    if (key.type != TID_KEY)
      {
      cm_msg(MERROR, "open_history", "Only subkeys allows in /history/links");
      continue;
      }

    /* count subkeys in link */
    for (i=n_var=0 ;; i++)
      {
      status = db_enum_key(hDB, hHistKey, i, &hKey);
      if (status == DB_NO_MORE_SUBKEYS)
        break;
      db_get_key(hDB, hKey, &key);
      if (key.type != TID_KEY)
        n_var++;
      }

    if (n_var == 0)
      cm_msg(MERROR, "open_history", "History event %s has no variables in ODB", hist_name);

    /* create tag array */
    tag = malloc(sizeof(TAG)*n_var);

    for (i=0,size=0,n_var=0 ;; i++)
      {
      status = db_enum_link(hDB, hHistKey, i, &hLinkKey);
      if (status == DB_NO_MORE_SUBKEYS)
        break;

      /* get link key */
      db_get_key(hDB, hLinkKey, &linkkey);

      if (linkkey.type == TID_KEY)
        continue;

      strcpy(tag[n_var].name, linkkey.name);

      /* get link target */
      db_enum_key(hDB, hHistKey, i, &hVarKey);
      db_get_key(hDB, hVarKey, &varkey);

      /* hot-link individual values */
      if (histkey.type == TID_KEY)
        db_open_record(hDB, hVarKey, NULL, varkey.total_size, MODE_READ, log_system_history, (void *) index);

      tag[n_var].type = varkey.type;
      tag[n_var].n_data = varkey.num_values;
      size += varkey.total_size;
      n_var++;
      }

    /* hot-link whole subtree */
    if (histkey.type == TID_LINK)
      db_open_record(hDB, hHistKey, NULL, size, MODE_READ, log_system_history, (void *) index);

    hs_define_event(max_event_id, hist_name, tag, sizeof(TAG)*n_var);
    free(tag);

    /* define system history */
  
    hist_log[index].event_id = max_event_id;
    hist_log[index].hKeyVar = hHistKey;
    hist_log[index].buffer_size = size;
    hist_log[index].buffer = malloc(size);
    hist_log[index].period = 10; /* 10 sec default period */
    hist_log[index].last_log = 0;
    if (hist_log[index].buffer == NULL)
      {
      cm_msg(MERROR, "open_history", "cannot allocate history buffer");
      return 0;
      }

    index++;
    max_event_id++;

    if (index == MAX_HISTORY)
      {
      cm_msg(MERROR, "open_history", "too many equipments for history");
      return 0;
      }
    }
  
  return CM_SUCCESS;
}

/*---- close_history -----------------------------------------------*/

void close_history()
{
INT   i, status;
HNDLE hKeyRoot, hKey;

  /* close system history */
  status = db_find_key(hDB, 0, "/History/Links", &hKeyRoot);
  if (status != DB_SUCCESS)
    {
    for (i=0 ; ; i++)
      {
      status = db_enum_key(hDB, hKeyRoot, i, &hKey);
      if (status == DB_NO_MORE_SUBKEYS)
        break;
      db_close_record(hDB, hKey);
      }
    }

  /* close event history */
  for (i=1 ; i<MAX_HISTORY ; i++)
    if (hist_log[i].hKeyVar)
      {
      db_close_record(hDB, hist_log[i].hKeyVar);
      free(hist_log[i].buffer);
      }
}

/*---- log_history -------------------------------------------------*/

void log_history(HNDLE hDB, HNDLE hKey, void *info)
{
INT i, size;

  for (i=0 ; i<MAX_HISTORY ; i++)
    if (hist_log[i].hKeyVar == hKey)
      break;

  if (i == MAX_HISTORY)
    return;

  /* check if over period */
  if (ss_time() - hist_log[i].last_log < hist_log[i].period)
    return;

  /* check if event size has changed */
  db_get_record_size(hDB, hKey, 0, &size); 
  if (size != hist_log[i].buffer_size)
    {
    close_history();
    open_history();
    }

  hs_write_event(hist_log[i].event_id, hist_log[i].buffer, hist_log[i].buffer_size);
  hist_log[i].last_log = ss_time();
}

/*------------------------------------------------------------------*/

void log_system_history(HNDLE hDB, HNDLE hKey, void *info)
{
INT   i, size, total_size, status, index;
KEY   key;

  index = (int) info;
  
  /* check if over period */
  if (ss_time() - hist_log[index].last_log < hist_log[index].period)
    return;

  for (i=0,total_size=0 ; ; i++)
    {
    status = db_enum_key(hDB, hist_log[index].hKeyVar, i, &hKey);
    if (status == DB_NO_MORE_SUBKEYS)
      break;

    db_get_key(hDB, hKey, &key);
    size = key.total_size;
    db_get_data(hDB, hKey, (char *)hist_log[index].buffer+total_size, &size, key.type);
    total_size += size;
    }
  
  if (total_size != hist_log[index].buffer_size)
    {
    close_history();
    open_history();
    }
  else
    hs_write_event(hist_log[index].event_id, hist_log[index].buffer, hist_log[index].buffer_size);

  hist_log[index].last_log = ss_time();

  /* simulate odb key update for hot links connected to system history */
  db_lock_database(hDB);
  db_notify_clients(hDB, hist_log[index].hKeyVar, FALSE);
  db_unlock_database(hDB);

}

/*------------------------------------------------------------------*/

INT log_callback(INT index, void *prpc_param[])
{
HNDLE  hKeyRoot, hKeyChannel;
INT    i, status, size, channel, izero, htape, online_mode, watchdog_timeout;
BOOL   watchdog_flag;
char   str[256];
double dzero;

  /* rewind tapes */
  if (index == RPC_LOG_REWIND)
    {
    channel = *((INT *) prpc_param[0]);

    /* loop over all channels */
    status = db_find_key(hDB, 0, "/Logger/Channels", &hKeyRoot);
    if (status != DB_SUCCESS)
      {
      cm_msg(MERROR, "log_callback", "cannot find Logger/Channels entry in database");
      return 0;
      }

    /* check online mode */
    online_mode = 0;
    size = sizeof(online_mode);
    db_get_value(hDB, 0, "/Runinfo/online mode", &online_mode, &size, TID_INT); 

    for (i=0; i<MAX_CHANNELS ; i++)
      {
      status = db_enum_key(hDB, hKeyRoot, i, &hKeyChannel);
      if (status == DB_NO_MORE_SUBKEYS)
        break;

      /* skip if wrong channel, -1 means rewind all channels */
      if (channel != i && channel != -1)
        continue;

      if (status == DB_SUCCESS)
        {
        size = sizeof(str);
        status = db_get_value(hDB, hKeyChannel, "Settings/Type", str, &size, TID_STRING);
        if (status != DB_SUCCESS)
          continue;

        if (equal_ustring(str, "Tape"))
          {
          size = sizeof(str);
          status = db_get_value(hDB, hKeyChannel, "Settings/Filename", str, &size, TID_STRING);
          if (status != DB_SUCCESS)
            continue;

          if (ss_tape_open(str, O_RDONLY, &htape) == SS_SUCCESS)
            {
            cm_msg(MTALK, "log_callback", "rewinding tape #%d, please wait", i);

            cm_get_watchdog_params(&watchdog_flag, &watchdog_timeout);
            cm_set_watchdog_params(watchdog_flag, 300000);  /* 5 min for tape rewind */
            ss_tape_rewind(htape);
            if (online_mode)
              ss_tape_unmount(htape);
            cm_set_watchdog_params(watchdog_flag, watchdog_timeout);

            cm_msg(MINFO, "log_callback", "Tape %s rewound sucessfully", str);
            }
          else
            cm_msg(MERROR, "log_callback", "Cannot rewind tape %s", str);

          ss_tape_close(htape);

          /* clear statistics */
          dzero = izero = 0;
          log_chn[i].statistics.bytes_written_total = 0;
          log_chn[i].statistics.files_written = 0;
          db_set_value(hDB, hKeyChannel, "Statistics/Bytes written total", &dzero, 
                       sizeof(dzero), 1, TID_DOUBLE);
          db_set_value(hDB, hKeyChannel, "Statistics/Files written", &izero, 
                       sizeof(izero), 1, TID_INT);
          }
        }
      }

    cm_msg(MTALK, "log_callback", "tape rewind finished");
    }

  return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

/********************************************************************\

                         transition callbacks

\********************************************************************/

/*------------------------------------------------------------------*/

INT tr_prestart(INT run_number, char *error)
/********************************************************************\

   Prestart: 
   
     Loop through channels defined in /logger/channels.
     Neglect channels with are not active.
     If "filename" contains a "%", substitute it by the
     current run number. Open logging channel and
     corresponding buffer. Place a event request
     into the buffer.

\********************************************************************/
{
INT          size, index, status;
HNDLE        hKeyRoot, hKeyChannel;
char         str[256], path[256], dir[256];
CHN_SETTINGS *chn_settings;
KEY          key;
BOOL         write_data, tape_flag = FALSE;

  /* save current ODB */
  odb_save("last.odb");

  /* read global logging flag */
  size = sizeof(BOOL);
  write_data = TRUE;
  db_get_value(hDB, 0, "/Logger/Write data", &write_data, &size, TID_BOOL);

  /* read tape message flag */
  size = sizeof(tape_message);
  db_get_value(hDB, 0, "/Logger/Tape message", &tape_message, &size, TID_BOOL);

  /* loop over all channels */
  status = db_find_key(hDB, 0, "/Logger/Channels", &hKeyRoot);
  if (status != DB_SUCCESS)
    {
    /* if no channels are defined, define at least one */
    status = db_create_record(hDB, 0, "/Logger/Channels/0/", channel_str);
    if (status != DB_SUCCESS)
      {
      strcpy(error, "Cannot create channel entry in database");
      cm_msg(MERROR, "tr_prestart", error);
      return 0;
      }
    
    status = db_find_key(hDB, 0, "/Logger/Channels", &hKeyRoot);
    if (status != DB_SUCCESS)
      {
      strcpy(error, "Cannot create channel entry in database");
      cm_msg(MERROR, "tr_prestart", error);
      return 0;
      }
    }

  for (index=0; index<MAX_CHANNELS ; index++)
    {
    status = db_enum_key(hDB, hKeyRoot, index, &hKeyChannel);
    if (status == DB_NO_MORE_SUBKEYS)
      break;

    /* correct channel record */
    db_get_key(hDB, hKeyChannel, &key);
    status = db_create_record(hDB, hKeyRoot, key.name, channel_str);
    if (status != DB_SUCCESS && status != DB_OPEN_RECORD)
      {
      cm_msg(MERROR, "tr_prestart", "Cannot create channel record");
      break;
      }

    if (status == DB_SUCCESS || status == DB_OPEN_RECORD)
      {
      /* check if channel is already open */
      if (log_chn[index].handle || log_chn[index].ftp_con)
        continue;

      /* save settings key */
      status = db_find_key(hDB, hKeyChannel, "Settings", &log_chn[index].settings_hkey);
      if (status != DB_SUCCESS)
        {
        strcpy(error, "Cannot find channel settings info");
        cm_msg(MERROR, "tr_prestart", error);
        return 0;
        }

      /* save statistics key */
      status = db_find_key(hDB, hKeyChannel, "Statistics", &log_chn[index].stats_hkey);
      if (status != DB_SUCCESS)
        {
        strcpy(error, "Cannot find channel statistics info");
        cm_msg(MERROR, "tr_prestart", error);
        return 0;
        }

      /* clear statistics */
      size = sizeof(CHN_STATISTICS);
      db_get_record(hDB, log_chn[index].stats_hkey, &log_chn[index].statistics, &size, 0);

      log_chn[index].statistics.events_written = 0;
      log_chn[index].statistics.bytes_written = 0;

      db_set_record(hDB, log_chn[index].stats_hkey, &log_chn[index].statistics, size, 0);

      /* get channel info structure */
      chn_settings = &log_chn[index].settings;
      size = sizeof(CHN_SETTINGS);
      status = db_get_record(hDB, log_chn[index].settings_hkey, chn_settings, &size, 0);
      if (status != DB_SUCCESS)
        {
        strcpy(error, "Cannot read channel info");
        cm_msg(MERROR, "tr_prestart", error);
        return 0;
        }

      /* don't start run if tape is full */
      if (log_chn[index].type == LOG_TYPE_TAPE &&
          chn_settings->tape_capacity > 0 &&
          log_chn[index].statistics.bytes_written_total >= chn_settings->tape_capacity)
        {
        strcpy(error, "Tape capacity reached. Please load new tape");
        cm_msg(MERROR, "tr_prestart", error);
        return 0;
        }

      /* check if active */
      if (!chn_settings->active || !write_data)
        continue;

      /* check for type */
      if (equal_ustring(chn_settings->type, "Tape"))
        log_chn[index].type = LOG_TYPE_TAPE;
      else if (equal_ustring(chn_settings->type, "FTP"))
        log_chn[index].type = LOG_TYPE_FTP;
      else
        log_chn[index].type = LOG_TYPE_DISK;

      /* if disk, precede filename with directory if not already there */
      if (log_chn[index].type == LOG_TYPE_DISK &&
          strchr(chn_settings->filename, DIR_SEPARATOR) == NULL)
        {
        size = sizeof(dir);
        dir[0] = 0;
        db_get_value(hDB, 0, "/Logger/Data Dir", dir, &size, TID_STRING);
        if (dir[0] != 0)
          if (dir[strlen(dir)-1] != DIR_SEPARATOR)
            strcat(dir, DIR_SEPARATOR_STR);
        strcpy(str, dir);
        }
      else
        str[0] = 0;
      strcat(str, chn_settings->filename);
      
      /* substitue "%d" by current run number */
      if (strchr(str, '%'))
        sprintf(path, str, run_number);
      else
        strcpy(path, str);

      strcpy(log_chn[index].path, path);

      if (log_chn[index].type == LOG_TYPE_TAPE && 
          log_chn[index].statistics.bytes_written_total == 0 &&
          tape_message)
        {
        tape_flag = TRUE;
        cm_msg(MTALK, "tr_prestart", "mounting tape #%d, please wait", index);
        }

      /* open logging channel */
      status = log_open(&log_chn[index], run_number);

      /* return if logging channel couldn't be opened */
      if (status != SS_SUCCESS)
        {
        if (status == SS_FILE_ERROR)
          sprintf(error, "cannot open file %s (Disk full?)", path);
        if (status == SS_FILE_EXISTS)
          sprintf(error, "file %s exists already, run start aborted", path);
        if (status == SS_NO_TAPE)
          sprintf(error, "no tape in device %s", path);
        if (status == SS_TAPE_ERROR)
          sprintf(error, "tape error, cannot start run");
        if (status == SS_DEV_BUSY)
          sprintf(error, "device %s used by someone else", path);
        if (status == FTP_NET_ERROR || status == FTP_RESPONSE_ERROR)
          sprintf(error, "cannot open FTP channel to [%s]", path);

        cm_msg(MERROR, "tr_prestart", error);
        return 0;
        }

      /* open hot link to statistics tree */
      status = db_open_record(hDB, log_chn[index].stats_hkey, &log_chn[index].statistics, 
                              sizeof(CHN_STATISTICS), MODE_WRITE, NULL, NULL);
      if (status != DB_SUCCESS)
        cm_msg(MERROR, "tr_prestart", "cannot open statistics record, probably other logger is using it");

      /* open hot link to settings tree */
      status = db_open_record(hDB, log_chn[index].settings_hkey, &log_chn[index].settings, 
                              sizeof(CHN_SETTINGS), MODE_READ, NULL, NULL);
      if (status != DB_SUCCESS)
        cm_msg(MERROR, "tr_prestart", "cannot open channel settings record, probably other logger is using it");

#ifndef FAL_MAIN
      /* open buffer */
      status = bm_open_buffer(chn_settings->buffer, EVENT_BUFFER_SIZE, &log_chn[index].buffer_handle);
      if (status != BM_SUCCESS && status != BM_CREATED)
        {
        sprintf(error, "Cannot open buffer %s", str);
        cm_msg(MERROR, "tr_prestart", error);
        return 0;
        }
      bm_set_cache_size(log_chn[index].buffer_handle, 100000, 0);

      /* place event request */
      status = bm_request_event(log_chn[index].buffer_handle, 
                                (short) chn_settings->event_id, 
                                (short) chn_settings->trigger_mask, 
                                GET_ALL, &log_chn[index].request_id, receive_event);

      if (status != BM_SUCCESS)
        {
        sprintf(error, "Cannot place event request");
        cm_msg(MERROR, "tr_prestart", error);
        return 0;
        }

      /* open message buffer if requested */
      if (chn_settings->log_messages)
        {
        status = bm_open_buffer(MESSAGE_BUFFER_NAME, MESSAGE_BUFFER_SIZE, &log_chn[index].msg_buffer_handle);
        if (status != BM_SUCCESS && status != BM_CREATED)
          {
          sprintf(error, "Cannot open buffer %s", MESSAGE_BUFFER_NAME);
          cm_msg(MERROR, "tr_prestart", error);
          return 0;
          }

        /* place event request */
        status = bm_request_event(log_chn[index].msg_buffer_handle, 
                                  (short) EVENTID_MESSAGE, 
                                  (short) chn_settings->log_messages, 
                                  GET_ALL, &log_chn[index].msg_request_id, receive_event);

        if (status != BM_SUCCESS)
          {
          sprintf(error, "Cannot place event request");
          cm_msg(MERROR, "tr_prestart", error);
          return 0;
          }
        }

#endif
      }
    }

  if (tape_flag && tape_message)
    cm_msg(MTALK, "tr_prestart", "tape mounting finished");

  /* reopen history channels if event definition has changed */
  close_history();
  open_history();

  return CM_SUCCESS;
}

/*-- poststop ------------------------------------------------------*/

INT tr_poststop(INT run_number, char *error)
/********************************************************************\

   Poststop: 
   
     Wait until buffers are empty, then close logging channels

\********************************************************************/
{
INT    i, size;
BOOL   flag, tape_flag=FALSE;
char   filename[256];
char   str[256];

  if (in_stop_transition)
    return CM_SUCCESS;

  in_stop_transition = TRUE;
  for (i=0 ; i<MAX_CHANNELS ; i++)
    {
    if (log_chn[i].handle || log_chn[i].ftp_con)
      {
      /* generate MTALK message */
      if (log_chn[i].type == LOG_TYPE_TAPE && tape_message)
        {
        tape_flag = TRUE;
        cm_msg(MTALK, "tr_poststop", "closing tape channel #%d, please wait", i);
        }

#ifndef FAL_MAIN
      /* wait until buffer is empty */
      if (log_chn[i].buffer_handle)
        {
        INT n_bytes;

        do
          {

          bm_get_buffer_level(log_chn[i].buffer_handle, &n_bytes);
          if (n_bytes > 0)
            cm_yield(100);

          } while (n_bytes > 0);
        }
#endif

      /* close logging channel */
      log_close(&log_chn[i], run_number);

      /* close statistics record */
      db_set_record(hDB, log_chn[i].stats_hkey, &log_chn[i].statistics,
                    sizeof(CHN_STATISTICS), 0);
      db_close_record(hDB, log_chn[i].stats_hkey);
      db_close_record(hDB, log_chn[i].settings_hkey);
      }
    }

  /* close buffers */
  for (i=0 ; i<MAX_CHANNELS ; i++)
    {
#ifndef FAL_MAIN
    if (log_chn[i].buffer_handle)
      {
      INT j;

      bm_close_buffer(log_chn[i].buffer_handle);
      for (j=i+1 ; j<MAX_CHANNELS ; j++)
        if (log_chn[j].buffer_handle == log_chn[i].buffer_handle)
          log_chn[j].buffer_handle = 0;
      }

    if (log_chn[i].msg_request_id)
      bm_delete_request(log_chn[i].msg_request_id);
#endif

    /* clear channel info */
    memset(&log_chn[i].handle, 0, sizeof(LOG_CHN));
    }

  /* ODB dump if requested */
  size = sizeof(flag);
  flag = 0;
  db_get_value(hDB, 0, "/Logger/ODB Dump", &flag, &size, TID_BOOL);
  if (flag)
    {
    strcpy(str, "run%d.odb");
    size = sizeof(str);
    str[0] = 0;
    db_get_value(hDB, 0, "/Logger/ODB Dump File", str, &size, TID_STRING);
    if (str[0] == 0)
      strcpy(str, "run%d.odb");

    /* substitue "%d" by current run number */
    if (strchr(str, '%'))
      sprintf(filename, str, run_number);
    else
      strcpy(filename, str);

    odb_save(filename);
    }

  in_stop_transition = FALSE;

  if (tape_flag & tape_message)
    cm_msg(MTALK, "tr_poststop", "all tape channels closed");

  return CM_SUCCESS;
}

/*== common code FAL/MLOGGER end ===================================*/

/*----- receive_event ----------------------------------------------*/

void receive_event(HNDLE hBuf, HNDLE request_id, EVENT_HEADER *pheader, void *pevent)
{
INT i;
  
  /* find logging channel for this request id */
  for (i=0 ; i<MAX_CHANNELS ; i++)
    {
    if (log_chn[i].handle == 0 && log_chn[i].ftp_con == NULL)
	    continue;
      
    /* write normal events */
    if (log_chn[i].request_id == request_id)
	    {
      log_write(&log_chn[i], pheader);
	    break;
	    }

    /* write messages */
    if (log_chn[i].msg_request_id == request_id)
	    {
      log_write(&log_chn[i], pheader);
	    break;
	    }
    }
}

/*------------------------ main ------------------------------------*/

main(int argc, char *argv[])
{
INT    status, msg, i, size, run_number, ch;
char   host_name[100], exp_name[NAME_LENGTH], dir[256];
BOOL   debug, daemon, save_mode;
DWORD  last_time_kb = 0;
DWORD  last_time_hist = 0;
DWORD  last_time_stat = 0;

  /* get default from environment */
  cm_get_environment(host_name, exp_name);

  debug = daemon = save_mode = FALSE;
  
  /* parse command line parameters */
  for (i=1 ; i<argc ; i++)
    {
    if (argv[i][0] == '-' && argv[i][1] == 'd')
      debug = TRUE;
    else if (argv[i][0] == '-' && argv[i][1] == 'D')
      daemon = TRUE;
    else if (argv[i][0] == '-' && argv[i][1] == 's')
      save_mode = TRUE;
    else if (argv[i][0] == '-')
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
        printf("usage: mlogger [-h Hostname] [-e Experiment] [-d] [-D] [-s]\n\n");
        return 1;
        }
      }
    }

  if (daemon)
    {
    printf("Becoming a daemon...\n");
    ss_daemon_init();
    }

  status = cm_connect_experiment(host_name, exp_name, "Logger", NULL);
  if (status != CM_SUCCESS)
    return 1;

  /* check if logger already running */
  status = cm_exist("Logger", FALSE);
  if (status == CM_SUCCESS)
    {
    printf("Logger runs already.\n");
    cm_disconnect_experiment();
    return 1;
    }

  cm_get_experiment_database(&hDB, NULL);

  /* turn off watchdog if in debug mode */
  if (debug)
    cm_set_watchdog_params(TRUE, 0);

  /* turn on save mode */
  if (save_mode)
    {
    cm_set_watchdog_params(FALSE, 0);
    db_protect_database(hDB);
    }

  /* turn off message display, turn on message logging */
  cm_set_msg_print(MT_ALL, 0, NULL);
  
  /* register transition callbacks */
  if (cm_register_transition(TR_PRESTART, tr_prestart) != CM_SUCCESS)
    {
    cm_msg(MERROR, "main", "cannot register callbacks");
    return 1;
    }
  
  cm_register_transition(TR_POSTSTOP, tr_poststop);  
  
  /* register callback for rewinding tapes */
  cm_register_function(RPC_LOG_REWIND, log_callback);

  /* initialize ODB */
  logger_init();

  /* open history logging */
  open_history();

  /* print startup message */
  size = sizeof(dir);
  db_get_value(hDB, 0, "/Logger/Data dir", dir, &size, TID_STRING);
  printf("MIDAS logger started. Stop with \"!\"\n");
  printf("Data directory is %s\n", dir);

  /* initialize ss_getchar() */
  ss_getchar(0);

  do
    {
    msg = cm_yield(1000);

    /* update channel statistics once every second */
    if (ss_millitime() - last_time_stat > 1000)
      {
      last_time_stat = ss_millitime();
      db_send_changed_records();
      }

    /* check for auto restart */
    if (auto_restart)
      {
      /* wait until analyzer EOR finished */
      ss_sleep(10000);

      auto_restart = FALSE;
      size = sizeof(run_number);
      db_get_value(hDB, 0, "/Runinfo/Run number", &run_number, &size, TID_INT);

      cm_msg(MTALK, "main", "starting new run");
      status = cm_transition(TR_START, run_number+1, NULL, 0, ASYNC);
      if (status != CM_SUCCESS)
        cm_msg(MERROR, "main", "cannot restart run");
      }

    /* check keyboard once every second */
    if (ss_millitime() - last_time_kb > 1000)
      {
      last_time_kb = ss_millitime();

      ch = 0;
      while (ss_kbhit())
        {
        ch = ss_getchar(0);
        if (ch == -1)
          ch = getchar();

        if ((char) ch == '!')
          break;
        }
      }

    } while (msg != RPC_SHUTDOWN && msg != SS_ABORT && ch != '!');
  
  /* reset terminal */
  ss_getchar(TRUE);

  /* close history logging */
  close_history();

  cm_disconnect_experiment();

  return 0;
}
