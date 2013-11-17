/********************************************************************\

  Name:         HISTORY.C
  Created by:   Stefan Ritt

  Contents:     MIDAS history functions

  $Id$

\********************************************************************/

#include "midas.h"
#include "msystem.h"
#ifndef HAVE_STRLCPY
#include "strlcpy.h"
#endif
#include <assert.h>

/** @defgroup hsfunctioncode Midas History Functions (hs_xxx)
 */

/**dox***************************************************************/
/** @addtogroup hsfunctioncode
 *
 *  @{  */

#if !defined(OS_VXWORKS)
/********************************************************************\
*                                                                    *
*                 History functions                                  *
*                                                                    *
\********************************************************************/

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

static HISTORY *_history;
static INT _history_entries = 0;
static char _hs_path_name[MAX_STRING_LENGTH];

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Sets the path for future history file accesses. Should
be called before any other history function is called.
@param path             Directory where history files reside
@return HS_SUCCESS
*/
INT hs_set_path(const char *path)
{
   /* set path locally and remotely */
   if (rpc_is_remote())
      rpc_call(RPC_HS_SET_PATH, path);

   strcpy(_hs_path_name, path);

   /* check for trailing directory seperator */
   if (strlen(_hs_path_name) > 0 && _hs_path_name[strlen(_hs_path_name) - 1] != DIR_SEPARATOR)
      strcat(_hs_path_name, DIR_SEPARATOR_STR);

   return HS_SUCCESS;
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************/

/**
Open history file belonging to certain date. Internal use
           only.
@param ltime          Date for which a history file should be opened.
@param suffix         File name suffix like "hst", "idx", "idf"
@param mode           R/W access mode
@param fh             File handle
@return HS_SUCCESS
*/
INT hs_open_file(time_t ltime, char *suffix, INT mode, int *fh)
{
   struct tm *tms;
   char file_name[256];
   time_t ttime;

   /* generate new file name YYMMDD.xxx */
#if !defined(OS_VXWORKS)
#if !defined(OS_VMS)
   tzset();
#endif
#endif
   ttime = (time_t) ltime;
   tms = localtime(&ttime);

   sprintf(file_name, "%s%02d%02d%02d.%s", _hs_path_name, tms->tm_year % 100, tms->tm_mon + 1, tms->tm_mday, suffix);

   /* open file, add O_BINARY flag for Windows NT */
   *fh = open(file_name, mode | O_BINARY, 0644);

   //printf("hs_open_file: time %d, file \'%s\', fh %d\n", (int)ltime, file_name, *fh);

   return HS_SUCCESS;
}

/********************************************************************/
INT hs_gen_index(DWORD ltime)
/********************************************************************\

  Routine: hs_gen_index

  Purpose: Regenerate index files ("idx" and "idf" files) for a given
           history file ("hst"). Interal use only.

  Input:
    time_t ltime            Date for which a history file should
                            be analyzed.

  Output:
    none

  Function value:
    HS_SUCCESS              Successful completion
    HS_FILE_ERROR           Index files cannot be created

\********************************************************************/
{
   char event_name[NAME_LENGTH];
   int fh, fhd, fhi;
   INT n;
   HIST_RECORD rec;
   INDEX_RECORD irec;
   DEF_RECORD def_rec;
   int recovering = 0;
   //time_t now = time(NULL);

   cm_msg(MINFO, "hs_gen_index", "generating index files for time %d", (int) ltime);
   printf("Recovering index files...\n");

   if (ltime == 0)
      ltime = (DWORD) time(NULL);

   /* open new index file */
   hs_open_file(ltime, "idx", O_RDWR | O_CREAT | O_TRUNC, &fhi);
   hs_open_file(ltime, "idf", O_RDWR | O_CREAT | O_TRUNC, &fhd);

   if (fhd < 0 || fhi < 0) {
      cm_msg(MERROR, "hs_gen_index", "cannot create index file");
      return HS_FILE_ERROR;
   }

   /* open history file */
   hs_open_file(ltime, "hst", O_RDONLY, &fh);
   if (fh < 0)
      return HS_FILE_ERROR;
   lseek(fh, 0, SEEK_SET);

   /* loop over file records in .hst file */
   do {
      n = read(fh, (char *) &rec, sizeof(rec));
      //printf("read %d\n", n);
      if (n < sizeof(rec))
         break;

      /* check if record type is definition */
      if (rec.record_type == RT_DEF) {
         /* read name */
         read(fh, event_name, sizeof(event_name));

         printf("Event definition %s, ID %d\n", event_name, rec.event_id);

         /* write definition index record */
         def_rec.event_id = rec.event_id;
         memcpy(def_rec.event_name, event_name, sizeof(event_name));
         def_rec.def_offset = TELL(fh) - sizeof(event_name) - sizeof(rec);
         write(fhd, (char *) &def_rec, sizeof(def_rec));

         //printf("data def at %d (age %d)\n", rec.time, now-rec.time);

         /* skip tags */
         lseek(fh, rec.data_size, SEEK_CUR);
      } else if (rec.record_type == RT_DATA && rec.data_size > 1 && rec.data_size < 1 * 1024 * 1024) {
         /* write index record */
         irec.event_id = rec.event_id;
         irec.time = rec.time;
         irec.offset = TELL(fh) - sizeof(rec);
         write(fhi, (char *) &irec, sizeof(irec));

         //printf("data rec at %d (age %d)\n", rec.time, now-rec.time);

         /* skip data */
         lseek(fh, rec.data_size, SEEK_CUR);
      } else {
         if (!recovering)
            cm_msg(MERROR, "hs_gen_index", "broken history file for time %d, trying to recover", (int) ltime);

         recovering = 1;
         lseek(fh, 1 - sizeof(rec), SEEK_CUR);

         continue;
      }

   } while (TRUE);

   close(fh);
   close(fhi);
   close(fhd);

   printf("...done.\n");

   return HS_SUCCESS;
}


/********************************************************************/
INT hs_search_file(DWORD * ltime, INT direction)
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
   time_t lt;
   int fh, fhd, fhi;
   struct tm *tms;

   if (*ltime == 0)
      *ltime = ss_time();

   lt = (time_t) * ltime;
   do {
      /* try to open history file for date "lt" */
      hs_open_file(lt, "hst", O_RDONLY, &fh);

      /* if not found, look for next day */
      if (fh < 0)
         lt += direction * 3600 * 24;

      /* stop if more than a year before starting point or in the future */
   } while (fh < 0 && (INT) * ltime - (INT) lt < 3600 * 24 * 365 && lt <= (time_t) ss_time());

   if (fh < 0)
      return HS_FILE_ERROR;

   if (lt != *ltime) {
      /* if switched to new day, set start_time to 0:00 */
      tms = localtime(&lt);
      tms->tm_hour = tms->tm_min = tms->tm_sec = 0;
      *ltime = (DWORD) mktime(tms);
   }

   /* check if index files are there */
   hs_open_file(*ltime, "idf", O_RDONLY, &fhd);
   hs_open_file(*ltime, "idx", O_RDONLY, &fhi);

   close(fh);
   if (fhd > 0)
      close(fhd);
   if (fhi > 0)
      close(fhi);

   /* generate them if not */
   if (fhd < 0 || fhi < 0)
      hs_gen_index(*ltime);

   return HS_SUCCESS;
}


/********************************************************************/
INT hs_define_event(DWORD event_id, const char *name, const TAG * tag, DWORD size)
/********************************************************************\

  Routine: hs_define_event

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
      HIST_RECORD rec, prev_rec;
      DEF_RECORD def_rec;
      time_t ltime;
      char str[256], event_name[NAME_LENGTH], *buffer;
      int fh, fhi, fhd;
      INT i, n, index, status, semaphore;
      struct tm *tmb;

      /* request semaphore */
      cm_get_experiment_semaphore(NULL, NULL, &semaphore, NULL);
      status = ss_semaphore_wait_for(semaphore, 5 * 1000);
      if (status != SS_SUCCESS)
         return SUCCESS;        /* someone else blocked the history system */

      /* allocate new space for the new history descriptor */
      if (_history_entries == 0) {
         _history = (HISTORY *) M_MALLOC(sizeof(HISTORY));
         memset(_history, 0, sizeof(HISTORY));
         if (_history == NULL) {
            ss_semaphore_release(semaphore);
            return HS_NO_MEMORY;
         }

         _history_entries = 1;
         index = 0;
      } else {
         /* check if history already open */
         for (i = 0; i < _history_entries; i++)
            if (_history[i].event_id == event_id)
               break;

         /* if not found, create new one */
         if (i == _history_entries) {
            _history = (HISTORY *) realloc(_history, sizeof(HISTORY) * (_history_entries + 1));
            memset(&_history[_history_entries], 0, sizeof(HISTORY));

            _history_entries++;
            if (_history == NULL) {
               _history_entries--;
               ss_semaphore_release(semaphore);
               return HS_NO_MEMORY;
            }
         }
         index = i;
      }

      /* assemble definition record header */
      rec.record_type = RT_DEF;
      rec.event_id = event_id;
      rec.time = (DWORD) time(NULL);
      rec.data_size = size;
      strncpy(event_name, name, NAME_LENGTH);

      /* if history structure not set up, do so now */
      if (!_history[index].hist_fh) {
         /* open history file */
         hs_open_file(rec.time, "hst", O_CREAT | O_RDWR, &fh);
         if (fh < 0) {
            ss_semaphore_release(semaphore);
            return HS_FILE_ERROR;
         }

         /* open index files */
         hs_open_file(rec.time, "idf", O_CREAT | O_RDWR, &fhd);
         hs_open_file(rec.time, "idx", O_CREAT | O_RDWR, &fhi);
         lseek(fh, 0, SEEK_END);
         lseek(fhi, 0, SEEK_END);
         lseek(fhd, 0, SEEK_END);

         /* regenerate index if missing */
         if (TELL(fh) > 0 && TELL(fhd) == 0) {
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

         ltime = (time_t) rec.time;
         tmb = localtime(&ltime);
         tmb->tm_hour = tmb->tm_min = tmb->tm_sec = 0;

         /* setup history structure */
         _history[index].hist_fh = fh;
         _history[index].index_fh = fhi;
         _history[index].def_fh = fhd;
         _history[index].def_offset = TELL(fh);
         _history[index].event_id = event_id;
         strcpy(_history[index].event_name, event_name);
         _history[index].base_time = (DWORD) mktime(tmb);
         _history[index].n_tag = size / sizeof(TAG);
         _history[index].tag = (TAG *) M_MALLOC(size);
         memcpy(_history[index].tag, tag, size);

         /* search previous definition */
         n = TELL(fhd) / sizeof(def_rec);
         def_rec.event_id = 0;
         for (i = n - 1; i >= 0; i--) {
            lseek(fhd, i * sizeof(def_rec), SEEK_SET);
            read(fhd, (char *) &def_rec, sizeof(def_rec));
            if (def_rec.event_id == event_id)
               break;
         }
         lseek(fhd, 0, SEEK_END);

         /* if definition found, compare it with new one */
         if (def_rec.event_id == event_id) {
            buffer = (char *) M_MALLOC(size);
            memset(buffer, 0, size);

            lseek(fh, def_rec.def_offset, SEEK_SET);
            read(fh, (char *) &prev_rec, sizeof(prev_rec));
            read(fh, str, NAME_LENGTH);
            read(fh, buffer, size);
            lseek(fh, 0, SEEK_END);

            if (prev_rec.data_size != size || strcmp(str, event_name) != 0 || memcmp(buffer, tag, size) != 0) {
               /* write definition to history file */
               write(fh, (char *) &rec, sizeof(rec));
               write(fh, event_name, NAME_LENGTH);
               write(fh, (char *) tag, size);

               /* write index record */
               def_rec.event_id = event_id;
               memcpy(def_rec.event_name, event_name, sizeof(event_name));
               def_rec.def_offset = _history[index].def_offset;
               write(fhd, (char *) &def_rec, sizeof(def_rec));
            } else
               /* definition identical, just remember old offset */
               _history[index].def_offset = def_rec.def_offset;

            M_FREE(buffer);
         } else {
            /* write definition to history file */
            write(fh, (char *) &rec, sizeof(rec));
            write(fh, event_name, NAME_LENGTH);
            write(fh, (char *) tag, size);

            /* write definition index record */
            def_rec.event_id = event_id;
            memcpy(def_rec.event_name, event_name, sizeof(event_name));
            def_rec.def_offset = _history[index].def_offset;
            write(fhd, (char *) &def_rec, sizeof(def_rec));
         }
      } else {
         fh = _history[index].hist_fh;
         fhd = _history[index].def_fh;

         /* compare definition with previous definition */
         buffer = (char *) M_MALLOC(size);
         memset(buffer, 0, size);

         lseek(fh, _history[index].def_offset, SEEK_SET);
         read(fh, (char *) &prev_rec, sizeof(prev_rec));
         read(fh, str, NAME_LENGTH);
         read(fh, buffer, size);

         lseek(fh, 0, SEEK_END);
         lseek(fhd, 0, SEEK_END);

         if (prev_rec.data_size != size || strcmp(str, event_name) != 0 || memcmp(buffer, tag, size) != 0) {
            /* save new definition offset */
            _history[index].def_offset = TELL(fh);

            /* write definition to history file */
            write(fh, (char *) &rec, sizeof(rec));
            write(fh, event_name, NAME_LENGTH);
            write(fh, (char *) tag, size);

            /* write index record */
            def_rec.event_id = event_id;
            memcpy(def_rec.event_name, event_name, sizeof(event_name));
            def_rec.def_offset = _history[index].def_offset;
            write(fhd, (char *) &def_rec, sizeof(def_rec));
         }

         M_FREE(buffer);
      }

      ss_semaphore_release(semaphore);
   }

   return HS_SUCCESS;
}


/********************************************************************/
INT hs_write_event(DWORD event_id, const void *data, DWORD size)
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
   HIST_RECORD rec, drec;
   DEF_RECORD def_rec;
   INDEX_RECORD irec;
   int fh, fhi, fhd, last_pos_data, last_pos_index;
   INT index, semaphore, status;
   struct tm tmb, tmr;
   time_t ltime;

   /* request semaphore */
   cm_get_experiment_semaphore(NULL, NULL, &semaphore, NULL);
   status = ss_semaphore_wait_for(semaphore, 5 * 1000);
   if (status != SS_SUCCESS) {
      cm_msg(MERROR, "hs_write_event", "semaphore timeout");
      return SUCCESS;           /* someone else blocked the history system */
   }

   /* find index to history structure */
   for (index = 0; index < _history_entries; index++)
      if (_history[index].event_id == event_id)
         break;
   if (index == _history_entries) {
      ss_semaphore_release(semaphore);
      return HS_UNDEFINED_EVENT;
   }

   /* assemble record header */
   rec.record_type = RT_DATA;
   rec.event_id = _history[index].event_id;
   rec.time = (DWORD) time(NULL);
   rec.def_offset = _history[index].def_offset;
   rec.data_size = size;

   irec.event_id = _history[index].event_id;
   irec.time = rec.time;

   /* check if new day */
   ltime = (time_t) rec.time;
   memcpy(&tmr, localtime(&ltime), sizeof(tmr));
   ltime = (time_t) _history[index].base_time;
   memcpy(&tmb, localtime(&ltime), sizeof(tmb));

   if (tmr.tm_yday != tmb.tm_yday) {
      /* close current history file */
      close(_history[index].hist_fh);
      close(_history[index].def_fh);
      close(_history[index].index_fh);

      /* open new history file */
      hs_open_file(rec.time, "hst", O_CREAT | O_RDWR, &fh);
      if (fh < 0) {
         ss_semaphore_release(semaphore);
         return HS_FILE_ERROR;
      }

      /* open new index file */
      hs_open_file(rec.time, "idx", O_CREAT | O_RDWR, &fhi);
      if (fhi < 0) {
         ss_semaphore_release(semaphore);
         return HS_FILE_ERROR;
      }

      /* open new definition index file */
      hs_open_file(rec.time, "idf", O_CREAT | O_RDWR, &fhd);
      if (fhd < 0) {
         ss_semaphore_release(semaphore);
         return HS_FILE_ERROR;
      }

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
      _history[index].base_time = (DWORD) mktime(&tmr);

      /* write definition from _history structure */
      drec.record_type = RT_DEF;
      drec.event_id = _history[index].event_id;
      drec.time = rec.time;
      drec.data_size = _history[index].n_tag * sizeof(TAG);

      write(fh, (char *) &drec, sizeof(drec));
      write(fh, _history[index].event_name, NAME_LENGTH);
      write(fh, (char *) _history[index].tag, drec.data_size);

      /* write definition index record */
      def_rec.event_id = _history[index].event_id;
      memcpy(def_rec.event_name, _history[index].event_name, sizeof(def_rec.event_name));
      def_rec.def_offset = _history[index].def_offset;
      write(fhd, (char *) &def_rec, sizeof(def_rec));
   }

   /* got to end of file */
   lseek(_history[index].hist_fh, 0, SEEK_END);
   last_pos_data = irec.offset = TELL(_history[index].hist_fh);

   /* write record header */
   write(_history[index].hist_fh, (char *) &rec, sizeof(rec));

   /* write data */
   if (write(_history[index].hist_fh, (char *) data, size) < (int) size) {
      /* disk maybe full? Do a roll-back! */
      lseek(_history[index].hist_fh, last_pos_data, SEEK_SET);
      TRUNCATE(_history[index].hist_fh);
      ss_semaphore_release(semaphore);
      return HS_FILE_ERROR;
   }

   /* write index record */
   lseek(_history[index].index_fh, 0, SEEK_END);
   last_pos_index = TELL(_history[index].index_fh);
   if (write(_history[index].index_fh, (char *) &irec, sizeof(irec)) < sizeof(irec)) {
      /* disk maybe full? Do a roll-back! */
      lseek(_history[index].hist_fh, last_pos_data, SEEK_SET);
      TRUNCATE(_history[index].hist_fh);
      lseek(_history[index].index_fh, last_pos_index, SEEK_SET);
      TRUNCATE(_history[index].index_fh);
      ss_semaphore_release(semaphore);
      return HS_FILE_ERROR;
   }

   ss_semaphore_release(semaphore);
   return HS_SUCCESS;
}


/********************************************************************/
INT hs_enum_events(DWORD ltime, char *event_name, DWORD * name_size, INT event_id[], DWORD * id_size)
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
   int fh, fhd;
   INT status, i, j, n;
   DEF_RECORD def_rec;

   if (rpc_is_remote())
      return rpc_call(RPC_HS_ENUM_EVENTS, ltime, event_name, name_size, event_id, id_size);

   /* search latest history file */
   status = hs_search_file(&ltime, -1);
   if (status != HS_SUCCESS) {
      cm_msg(MERROR, "hs_enum_events", "cannot find recent history file");
      return HS_FILE_ERROR;
   }

   /* open history and definition files */
   hs_open_file(ltime, "hst", O_RDONLY, &fh);
   hs_open_file(ltime, "idf", O_RDONLY, &fhd);
   if (fh < 0 || fhd < 0) {
      cm_msg(MERROR, "hs_enum_events", "cannot open index files");
      return HS_FILE_ERROR;
   }
   lseek(fhd, 0, SEEK_SET);

   /* loop over definition index file */
   n = 0;
   do {
      /* read event definition */
      j = read(fhd, (char *) &def_rec, sizeof(def_rec));
      if (j < (int) sizeof(def_rec))
         break;

      /* look for existing entry for this event id */
      for (i = 0; i < n; i++)
         if (event_id[i] == (INT) def_rec.event_id) {
            strcpy(event_name + i * NAME_LENGTH, def_rec.event_name);
            break;
         }

      /* new entry found */
      if (i == n) {
         if (i * NAME_LENGTH > (INT) * name_size || i * sizeof(INT) > (INT) * id_size) {
            cm_msg(MERROR, "hs_enum_events", "index buffer too small");
            close(fh);
            close(fhd);
            return HS_NO_MEMORY;
         }

         /* copy definition record */
         strcpy(event_name + i * NAME_LENGTH, def_rec.event_name);
         event_id[i] = def_rec.event_id;
         n++;
      }
   } while (TRUE);

   close(fh);
   close(fhd);
   *name_size = n * NAME_LENGTH;
   *id_size = n * sizeof(INT);

   return HS_SUCCESS;
}


/********************************************************************/
INT hs_count_events(DWORD ltime, DWORD * count)
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
   int fh, fhd;
   INT status, i, j, n;
   DWORD *id;
   DEF_RECORD def_rec;

   if (rpc_is_remote())
      return rpc_call(RPC_HS_COUNT_EVENTS, ltime, count);

   /* search latest history file */
   status = hs_search_file(&ltime, -1);
   if (status != HS_SUCCESS) {
      cm_msg(MERROR, "hs_count_events", "cannot find recent history file");
      return HS_FILE_ERROR;
   }

   /* open history and definition files */
   hs_open_file(ltime, "hst", O_RDONLY, &fh);
   hs_open_file(ltime, "idf", O_RDONLY, &fhd);
   if (fh < 0 || fhd < 0) {
      cm_msg(MERROR, "hs_count_events", "cannot open index files");
      return HS_FILE_ERROR;
   }

   /* allocate event id array */
   lseek(fhd, 0, SEEK_END);
   id = (DWORD *) M_MALLOC(TELL(fhd) / sizeof(def_rec) * sizeof(DWORD));
   lseek(fhd, 0, SEEK_SET);

   /* loop over index file */
   n = 0;
   do {
      /* read definition index record */
      j = read(fhd, (char *) &def_rec, sizeof(def_rec));
      if (j < (int) sizeof(def_rec))
         break;

      /* look for existing entries */
      for (i = 0; i < n; i++)
         if (id[i] == def_rec.event_id)
            break;

      /* new entry found */
      if (i == n) {
         id[i] = def_rec.event_id;
         n++;
      }
   } while (TRUE);


   M_FREE(id);
   close(fh);
   close(fhd);
   *count = n;

   return HS_SUCCESS;
}


/********************************************************************/
INT hs_get_event_id(DWORD ltime, const char *name, DWORD * id)
/********************************************************************\

  Routine: hs_get_event_id

  Purpose: Return event ID for a given name. If event cannot be found
           in current definition file, go back in time until found

  Input:
    DWORD  ltime            Date at which event ID should be looked for

  Output:
    DWORD  *id              Event ID

  Function value:
    HS_SUCCESS              Successful completion
    HS_FILE_ERROR           Cannot open history file
    HS_UNDEFINED_EVENT      Event "name" not found

\********************************************************************/
{
   int fh, fhd;
   INT status, i;
   DWORD lt;
   DEF_RECORD def_rec;

   if (rpc_is_remote())
      return rpc_call(RPC_HS_GET_EVENT_ID, ltime, name, id);

   /* search latest history file */
   if (ltime == 0)
      ltime = (DWORD) time(NULL);

   lt = ltime;

   do {
      status = hs_search_file(&lt, -1);
      if (status != HS_SUCCESS) {
         cm_msg(MERROR, "hs_count_events", "cannot find recent history file");
         return HS_FILE_ERROR;
      }

      /* open history and definition files */
      hs_open_file(lt, "hst", O_RDONLY, &fh);
      hs_open_file(lt, "idf", O_RDONLY, &fhd);
      if (fh < 0 || fhd < 0) {
         cm_msg(MERROR, "hs_count_events", "cannot open index files");
         return HS_FILE_ERROR;
      }

      /* loop over index file */
      *id = 0;
      do {
         /* read definition index record */
         i = read(fhd, (char *) &def_rec, sizeof(def_rec));
         if (i < (int) sizeof(def_rec))
            break;

         if (strcmp(name, def_rec.event_name) == 0) {
            *id = def_rec.event_id;
            close(fh);
            close(fhd);
            return HS_SUCCESS;
         }
      } while (TRUE);

      close(fh);
      close(fhd);

      /* not found -> go back one day */
      lt -= 3600 * 24;

   } while (lt > ltime - 3600 * 24 * 365 * 10); /* maximum 10 years */

   return HS_UNDEFINED_EVENT;
}


/********************************************************************/
INT hs_count_vars(DWORD ltime, DWORD event_id, DWORD * count)
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
   int fh, fhd;
   INT i, n, status;
   DEF_RECORD def_rec;
   HIST_RECORD rec;

   if (rpc_is_remote())
      return rpc_call(RPC_HS_COUNT_VARS, ltime, event_id, count);

   /* search latest history file */
   status = hs_search_file(&ltime, -1);
   if (status != HS_SUCCESS) {
      cm_msg(MERROR, "hs_count_tags", "cannot find recent history file");
      return HS_FILE_ERROR;
   }

   /* open history and definition files */
   hs_open_file(ltime, "hst", O_RDONLY, &fh);
   hs_open_file(ltime, "idf", O_RDONLY, &fhd);
   if (fh < 0 || fhd < 0) {
      cm_msg(MERROR, "hs_count_tags", "cannot open index files");
      return HS_FILE_ERROR;
   }

   /* search last definition */
   lseek(fhd, 0, SEEK_END);
   n = TELL(fhd) / sizeof(def_rec);
   def_rec.event_id = 0;
   for (i = n - 1; i >= 0; i--) {
      lseek(fhd, i * sizeof(def_rec), SEEK_SET);
      read(fhd, (char *) &def_rec, sizeof(def_rec));
      if (def_rec.event_id == event_id)
         break;
   }
   if (def_rec.event_id != event_id) {
      cm_msg(MERROR, "hs_count_tags", "event %d not found in index file", event_id);
      return HS_FILE_ERROR;
   }

   /* read definition */
   lseek(fh, def_rec.def_offset, SEEK_SET);
   read(fh, (char *) &rec, sizeof(rec));
   *count = rec.data_size / sizeof(TAG);

   close(fh);
   close(fhd);

   return HS_SUCCESS;
}


/********************************************************************/
INT hs_enum_vars(DWORD ltime, DWORD event_id, char *var_name, DWORD * size, DWORD * var_n, DWORD * n_size)
/********************************************************************\

  Routine: hs_enum_vars

  Purpose: Enumerate variable tags for a given date and event id

  Input:
    DWORD  ltime            Date at which tags should be enumerated
    DWORD  event_id         Event ID

  Output:
    char   *var_name        Array containing variable names
    DWORD  *size            Size of name array
    DWORD  *var_n           Array size of variable
    DWORD  *n_size          Size of n array

  Function value:
    HS_SUCCESS              Successful completion
    HS_NO_MEMEORY           Out of memory
    HS_FILE_ERROR           Cannot open history file

\********************************************************************/
{
   char str[256];
   int fh, fhd;
   INT i, n, status;
   DEF_RECORD def_rec;
   HIST_RECORD rec;
   TAG *tag;

   if (rpc_is_remote())
      return rpc_call(RPC_HS_ENUM_VARS, ltime, event_id, var_name, size);

   /* search latest history file */
   status = hs_search_file(&ltime, -1);
   if (status != HS_SUCCESS) {
      cm_msg(MERROR, "hs_enum_vars", "cannot find recent history file");
      return HS_FILE_ERROR;
   }

   /* open history and definition files */
   hs_open_file(ltime, "hst", O_RDONLY, &fh);
   hs_open_file(ltime, "idf", O_RDONLY, &fhd);
   if (fh < 0 || fhd < 0) {
      cm_msg(MERROR, "hs_enum_vars", "cannot open index files");
      return HS_FILE_ERROR;
   }

   /* search last definition */
   lseek(fhd, 0, SEEK_END);
   n = TELL(fhd) / sizeof(def_rec);
   def_rec.event_id = 0;
   for (i = n - 1; i >= 0; i--) {
      lseek(fhd, i * sizeof(def_rec), SEEK_SET);
      read(fhd, (char *) &def_rec, sizeof(def_rec));
      if (def_rec.event_id == event_id)
         break;
   }
   if (def_rec.event_id != event_id) {
      cm_msg(MERROR, "hs_enum_vars", "event %d not found in index file", event_id);
      return HS_FILE_ERROR;
   }

   /* read definition header */
   lseek(fh, def_rec.def_offset, SEEK_SET);
   read(fh, (char *) &rec, sizeof(rec));
   read(fh, str, NAME_LENGTH);

   /* read event definition */
   n = rec.data_size / sizeof(TAG);
   tag = (TAG *) M_MALLOC(rec.data_size);
   read(fh, (char *) tag, rec.data_size);

   if (n * NAME_LENGTH > (INT) * size || n * sizeof(DWORD) > *n_size) {

      /* store partial definition */
      for (i = 0; i < (INT) * size / NAME_LENGTH; i++) {
         strcpy(var_name + i * NAME_LENGTH, tag[i].name);
         var_n[i] = tag[i].n_data;
      }

      cm_msg(MERROR, "hs_enum_vars", "tag buffer too small");
      M_FREE(tag);
      close(fh);
      close(fhd);
      return HS_NO_MEMORY;
   }

   /* store full definition */
   for (i = 0; i < n; i++) {
      strcpy(var_name + i * NAME_LENGTH, tag[i].name);
      var_n[i] = tag[i].n_data;
   }
   *size = n * NAME_LENGTH;
   *n_size = n * sizeof(DWORD);

   M_FREE(tag);
   close(fh);
   close(fhd);

   return HS_SUCCESS;
}


/********************************************************************/
INT hs_get_var(DWORD ltime, DWORD event_id, const char *var_name, DWORD * type, INT * n_data)
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
   char str[256];
   int fh, fhd;
   INT i, n, status;
   DEF_RECORD def_rec;
   HIST_RECORD rec;
   TAG *tag;

   if (rpc_is_remote())
      return rpc_call(RPC_HS_GET_VAR, ltime, event_id, var_name, type, n_data);

   /* search latest history file */
   status = hs_search_file(&ltime, -1);
   if (status != HS_SUCCESS) {
      cm_msg(MERROR, "hs_get_var", "cannot find recent history file");
      return HS_FILE_ERROR;
   }

   /* open history and definition files */
   hs_open_file(ltime, "hst", O_RDONLY, &fh);
   hs_open_file(ltime, "idf", O_RDONLY, &fhd);
   if (fh < 0 || fhd < 0) {
      cm_msg(MERROR, "hs_get_var", "cannot open index files");
      return HS_FILE_ERROR;
   }

   /* search last definition */
   lseek(fhd, 0, SEEK_END);
   n = TELL(fhd) / sizeof(def_rec);
   def_rec.event_id = 0;
   for (i = n - 1; i >= 0; i--) {
      lseek(fhd, i * sizeof(def_rec), SEEK_SET);
      read(fhd, (char *) &def_rec, sizeof(def_rec));
      if (def_rec.event_id == event_id)
         break;
   }
   if (def_rec.event_id != event_id) {
      cm_msg(MERROR, "hs_get_var", "event %d not found in index file", event_id);
      return HS_FILE_ERROR;
   }

   /* read definition header */
   lseek(fh, def_rec.def_offset, SEEK_SET);
   read(fh, (char *) &rec, sizeof(rec));
   read(fh, str, NAME_LENGTH);

   /* read event definition */
   n = rec.data_size / sizeof(TAG);
   tag = (TAG *) M_MALLOC(rec.data_size);
   read(fh, (char *) tag, rec.data_size);

   /* search variable */
   for (i = 0; i < n; i++)
      if (strcmp(tag[i].name, var_name) == 0)
         break;

   close(fh);
   close(fhd);

   if (i < n) {
      *type = tag[i].type;
      *n_data = tag[i].n_data;
   } else {
      *type = *n_data = 0;
      cm_msg(MERROR, "hs_get_var", "variable %s not found", var_name);
      M_FREE(tag);
      return HS_UNDEFINED_VAR;
   }

   M_FREE(tag);
   return HS_SUCCESS;
}


/********************************************************************/
INT hs_get_tags(DWORD ltime, DWORD event_id, char event_name[NAME_LENGTH], int* n_tags, TAG** tags)
/********************************************************************\

  Routine: hs_get_tags

  Purpose: Get tags for event id

  Input:
    DWORD  ltime            Date at which variable definition should
                            be returned
    DWORD  event_id         Event ID

  Output:
    char    event_name[NAME_LENGTH] Event name from history file
    INT    *n_tags          Number of tags
    TAG   **tags            Pointer to array of tags (should be free()ed by the caller)

  Function value:
    HS_SUCCESS              Successful completion
    HS_NO_MEMEORY           Out of memory
    HS_FILE_ERROR           Cannot open history file

\********************************************************************/
{
   int fh, fhd;
   INT i, n, status;
   DEF_RECORD def_rec;
   HIST_RECORD rec;

   *n_tags = 0;
   *tags = NULL;

   if (rpc_is_remote())
      assert(!"RPC not implemented");

   /* search latest history file */
   status = hs_search_file(&ltime, -1);
   if (status != HS_SUCCESS) {
      cm_msg(MERROR, "hs_get_tags", "cannot find recent history file");
      return HS_FILE_ERROR;
   }

   /* open history and definition files */
   hs_open_file(ltime, "hst", O_RDONLY, &fh);
   hs_open_file(ltime, "idf", O_RDONLY, &fhd);
   if (fh < 0 || fhd < 0) {
      cm_msg(MERROR, "hs_get_tags", "cannot open index files");
      if (fh>0)
         close(fh);
      if (fhd>0)
         close(fhd);
      return HS_FILE_ERROR;
   }

   /* search last definition */
   lseek(fhd, 0, SEEK_END);
   n = TELL(fhd) / sizeof(def_rec);
   def_rec.event_id = 0;
   for (i = n - 1; i >= 0; i--) {
      lseek(fhd, i * sizeof(def_rec), SEEK_SET);
      read(fhd, (char *) &def_rec, sizeof(def_rec));
      if (def_rec.event_id == event_id)
         break;
   }

   if (def_rec.event_id != event_id) {
      //cm_msg(MERROR, "hs_get_tags", "event %d not found in index file", event_id);
      close(fh);
      close(fhd);
      return HS_UNDEFINED_EVENT;
   }

   /* read definition header */
   lseek(fh, def_rec.def_offset, SEEK_SET);
   status = read(fh, (char *) &rec, sizeof(rec));
   assert(status == sizeof(rec));

   status = read(fh, event_name, NAME_LENGTH);
   assert(status == NAME_LENGTH);

   /* read event definition */
   *n_tags = rec.data_size / sizeof(TAG);

   *tags = (TAG*) malloc(rec.data_size);

   status = read(fh, (char *) (*tags), rec.data_size);
   assert(status == rec.data_size);

   close(fh);
   close(fhd);

   return HS_SUCCESS;
}


INT hs_read(DWORD event_id, DWORD start_time, DWORD end_time, DWORD interval, const char *tag_name, DWORD var_index, DWORD * time_buffer, DWORD * tbsize, void *data_buffer, DWORD * dbsize, DWORD * type, DWORD * n)
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
   DWORD prev_time, last_irec_time;
   int fh, fhd, fhi, cp = 0;
   INT i, delta, index = 0, status, cache_size;
   INDEX_RECORD irec, *pirec;
   HIST_RECORD rec, drec;
   INT old_def_offset, var_size = 0, var_offset = 0;
   TAG *tag;
   char str[NAME_LENGTH];
   struct tm *tms;
   char *cache = NULL;
   time_t ltime;
   int rd;

   //printf("hs_read event %d, time %d:%d, tagname: \'%s\', varindex: %d\n", event_id, start_time, end_time, tag_name, var_index);

#if 0
   if (rpc_is_remote())
      return rpc_call(RPC_HS_READ, event_id, start_time, end_time, interval, tag_name, var_index, time_buffer, tbsize, data_buffer, dbsize, type, n);
#endif

   /* if not time given, use present to one hour in past */
   if (start_time == 0)
      start_time = (DWORD) time(NULL) - 3600;
   if (end_time == 0)
      end_time = (DWORD) time(NULL);

   *n = 0;
   prev_time = 0;
   last_irec_time = start_time;

   /* search history file for start_time */
   status = hs_search_file(&start_time, 1);
   if (status != HS_SUCCESS) {
      cm_msg(MERROR, "hs_read", "cannot find recent history file");
      *tbsize = *dbsize = *n = 0;
      return HS_FILE_ERROR;
   }

   /* open history and definition files */
   hs_open_file(start_time, "hst", O_RDONLY, &fh);
   hs_open_file(start_time, "idf", O_RDONLY, &fhd);
   hs_open_file(start_time, "idx", O_RDONLY, &fhi);
   if (fh < 0 || fhd < 0 || fhi < 0) {
      cm_msg(MERROR, "hs_read", "cannot open index files");
      *tbsize = *dbsize = *n = 0;
      if (fh > 0)
         close(fh);
      if (fhd > 0)
         close(fhd);
      if (fhi > 0)
         close(fhi);
      return HS_FILE_ERROR;
   }

   /* try to read index file into cache */
   lseek(fhi, 0, SEEK_END);
   cache_size = TELL(fhi);

   if (cache_size == 0) {
      goto nextday;
   }

   if (cache_size > 0) {
      cache = (char *) M_MALLOC(cache_size);
      if (cache) {
         lseek(fhi, 0, SEEK_SET);
         i = read(fhi, cache, cache_size);
         if (i < cache_size) {
            M_FREE(cache);
            if (fh > 0)
               close(fh);
            if (fhd > 0)
               close(fhd);
            if (fhi > 0)
               close(fhi);
            return HS_FILE_ERROR;
         }
      }

      /* search record closest to start time */
      if (cache == NULL) {
         lseek(fhi, 0, SEEK_END);
         delta = (TELL(fhi) / sizeof(irec)) / 2;
         lseek(fhi, delta * sizeof(irec), SEEK_SET);
         do {
            delta = (int) (abs(delta) / 2.0 + 0.5);
            rd = read(fhi, (char *) &irec, sizeof(irec));
            assert(rd == sizeof(irec));
            if (irec.time > start_time)
               delta = -delta;

            lseek(fhi, (delta - 1) * sizeof(irec), SEEK_CUR);
         } while (abs(delta) > 1 && irec.time != start_time);
         rd = read(fhi, (char *) &irec, sizeof(irec));
         assert(rd == sizeof(irec));
         if (irec.time > start_time)
            delta = -abs(delta);

         i = TELL(fhi) + (delta - 1) * sizeof(irec);
         if (i <= 0)
            lseek(fhi, 0, SEEK_SET);
         else
            lseek(fhi, (delta - 1) * sizeof(irec), SEEK_CUR);
         rd = read(fhi, (char *) &irec, sizeof(irec));
         assert(rd == sizeof(irec));
      } else {
         delta = (cache_size / sizeof(irec)) / 2;
         cp = delta * sizeof(irec);
         do {
            delta = (int) (abs(delta) / 2.0 + 0.5);
            pirec = (INDEX_RECORD *) (cache + cp);

            //printf("pirec %p, cache %p, cp %d\n", pirec, cache, cp);

            if (pirec->time > start_time)
               delta = -delta;

            cp = cp + delta * sizeof(irec);

            if (cp < 0)
               cp = 0;
         } while (abs(delta) > 1 && pirec->time != start_time);
         pirec = (INDEX_RECORD *) (cache + cp);
         if (pirec->time > start_time)
            delta = -abs(delta);

         if (cp <= delta * (int) sizeof(irec))
            cp = 0;
         else
            cp = cp + delta * sizeof(irec);

         if (cp >= cache_size)
            cp = cache_size - sizeof(irec);
         if (cp < 0)
            cp = 0;

         memcpy(&irec, (INDEX_RECORD *) (cache + cp), sizeof(irec));
         cp += sizeof(irec);
      }
   } else {                     /* file size > 0 */

      cache = NULL;
      irec.time = start_time;
   }

   /* read records, skip wrong IDs */
   old_def_offset = -1;
   last_irec_time = start_time - 24 * 60 * 60;
   do {
      //printf("time %d -> %d\n", last_irec_time, irec.time);

      if (irec.time < last_irec_time) {
         cm_msg(MERROR, "hs_read", "corrupted history data: time does not increase: %d -> %d", last_irec_time, irec.time);
         //*tbsize = *dbsize = *n = 0;
         if (fh > 0)
            close(fh);
         if (fhd > 0)
            close(fhd);
         if (fhi > 0)
            close(fhi);
         hs_gen_index(last_irec_time);
         return HS_SUCCESS;
      }
      last_irec_time = irec.time;
      if (irec.event_id == event_id && irec.time <= end_time && irec.time >= start_time) {
         /* check if record time more than "interval" seconds after previous time */
         if (irec.time >= prev_time + interval) {
            prev_time = irec.time;
            lseek(fh, irec.offset, SEEK_SET);
            rd = read(fh, (char *) &rec, sizeof(rec));
            if (rd != sizeof(rec)) {
               cm_msg(MERROR, "hs_read", "corrupted history data at time %d: read() of %d bytes returned %d, errno %d (%s)", (int) irec.time, (int)sizeof(rec), rd, errno, strerror(errno));
               //*tbsize = *dbsize = *n = 0;
               if (fh > 0)
                  close(fh);
               if (fhd > 0)
                  close(fhd);
               if (fhi > 0)
                  close(fhi);
               hs_gen_index(last_irec_time);
               return HS_SUCCESS;
            }

            /* if definition changed, read new definition */
            if ((INT) rec.def_offset != old_def_offset) {
               lseek(fh, rec.def_offset, SEEK_SET);
               read(fh, (char *) &drec, sizeof(drec));
               read(fh, str, NAME_LENGTH);

               tag = (TAG *) M_MALLOC(drec.data_size);
               if (tag == NULL) {
                  *n = *tbsize = *dbsize = 0;
                  if (cache)
                     M_FREE(cache);
                  close(fh);
                  close(fhd);
                  close(fhi);
                  return HS_NO_MEMORY;
               }
               read(fh, (char *) tag, drec.data_size);

               /* find index of tag_name in new definition */
               index = -1;
               for (i = 0; (DWORD) i < drec.data_size / sizeof(TAG); i++)
                  if (equal_ustring(tag[i].name, tag_name)) {
                     index = i;
                     break;
                  }

               /*
                  if ((DWORD) i == drec.data_size/sizeof(TAG))
                  {
                  *n = *tbsize = *dbsize = 0;
                  if (cache)
                  M_FREE(cache);

                  return HS_UNDEFINED_VAR;
                  }
                */

               if (index >= 0 && var_index >= tag[i].n_data) {
                  *n = *tbsize = *dbsize = 0;
                  if (cache)
                     M_FREE(cache);
                  M_FREE(tag);
                  close(fh);
                  close(fhd);
                  close(fhi);
                  return HS_WRONG_INDEX;
               }

               /* calculate offset for variable */
               if (index >= 0) {
                  *type = tag[i].type;

                  /* loop over all previous variables */
                  for (i = 0, var_offset = 0; i < index; i++)
                     var_offset += rpc_tid_size(tag[i].type) * tag[i].n_data;

                  /* strings have size n_data */
                  if (tag[index].type == TID_STRING)
                     var_size = tag[i].n_data;
                  else
                     var_size = rpc_tid_size(tag[index].type);

                  var_offset += var_size * var_index;
               }

               M_FREE(tag);
               old_def_offset = rec.def_offset;
               lseek(fh, irec.offset + sizeof(rec), SEEK_SET);
            }

            if (index >= 0) {
               /* check if data fits in buffers */
               if ((*n) * sizeof(DWORD) >= *tbsize || (*n) * var_size >= *dbsize) {
                  *dbsize = (*n) * var_size;
                  *tbsize = (*n) * sizeof(DWORD);
                  if (cache)
                     M_FREE(cache);
                  close(fh);
                  close(fhd);
                  close(fhi);
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
      if (cache) {
         if (cp >= cache_size) {
            i = -1;
            M_FREE(cache);
            cache = NULL;
         } else {

          try_again:

            i = sizeof(irec);

            memcpy(&irec, cache + cp, sizeof(irec));
            cp += sizeof(irec);

            /* if history file is broken ... */
            if (irec.time < last_irec_time || irec.time > last_irec_time + 24 * 60 * 60) {
               //if (irec.time < last_irec_time) {
               //printf("time %d -> %d, cache_size %d, cp %d\n", last_irec_time, irec.time, cache_size, cp);

               //printf("Seeking next record...\n");

               while (cp < cache_size) {
                  DWORD *evidp = (DWORD *) (cache + cp);
                  if (*evidp == event_id) {
                     //printf("Found at cp %d\n", cp);
                     goto try_again;
                  }

                  cp++;
               }

               i = -1;
            }
         }
      } else
         i = read(fhi, (char *) &irec, sizeof(irec));

      /* end of file: search next history file */
      if (i <= 0) {
         close(fh);
         close(fhd);
         close(fhi);
         fh = fhd = fhi = 0;

       nextday:

         /* advance one day */
         ltime = (time_t) last_irec_time;
         tms = localtime(&ltime);
         tms->tm_hour = tms->tm_min = tms->tm_sec = 0;
         last_irec_time = (DWORD) mktime(tms);

         last_irec_time += 3600 * 24;

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
         if (fh < 0 || fhd < 0 || fhi < 0) {
            cm_msg(MERROR, "hs_read", "cannot open index files");
            break;
         }

         /* try to read index file into cache */
         lseek(fhi, 0, SEEK_END);
         cache_size = TELL(fhi);

         if (cache_size == 0) {
            goto nextday;
         }

         lseek(fhi, 0, SEEK_SET);
         cache = (char *) M_MALLOC(cache_size);
         if (cache) {
            i = read(fhi, cache, cache_size);
            if (i < cache_size)
               break;
            /* read first record */
            cp = 0;
            memcpy(&irec, cache, sizeof(irec));
         } else {
            /* read first record */
            i = read(fhi, (char *) &irec, sizeof(irec));
            if (i <= 0)
               break;
            assert(i == sizeof(irec));
         }

         /* old definition becomes invalid */
         old_def_offset = -1;
      }
      //if (event_id==4 && irec.event_id == event_id)
      //  printf("time %d end %d\n", irec.time, end_time);
   } while (irec.time < end_time);

   if (cache)
      M_FREE(cache);
   if (fh)
      close(fh);
   if (fhd)
      close(fhd);
   if (fhi)
      close(fhi);

   *dbsize = *n * var_size;
   *tbsize = *n * sizeof(DWORD);

   return HS_SUCCESS;
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Display history for a given event at stdout. The output
can be redirected to be read by Excel for example. 
@param event_id         Event ID
@param start_time       Starting Date/Time
@param end_time         End Date/Time
@param interval         Minimum time in seconds between reported                                                                                
                            events. Can be used to skip events
@param binary_time      Display DWORD time stamp
@return HS_SUCCESS, HS_FILE_ERROR
*/
/********************************************************************/
INT hs_dump(DWORD event_id, DWORD start_time, DWORD end_time, DWORD interval, BOOL binary_time)
{
   DWORD prev_time, last_irec_time;
   time_t ltime;
   int fh, fhd, fhi;
   INT i, j, delta, status, n_tag = 0, old_n_tag = 0;
   INDEX_RECORD irec;
   HIST_RECORD rec, drec;
   INT old_def_offset, offset;
   TAG *tag = NULL, *old_tag = NULL;
   char str[NAME_LENGTH], data_buffer[10000];
   struct tm *tms;

   /* if not time given, use present to one hour in past */
   if (start_time == 0)
      start_time = (DWORD) time(NULL) - 3600;
   if (end_time == 0)
      end_time = (DWORD) time(NULL);

   /* search history file for start_time */
   status = hs_search_file(&start_time, 1);
   if (status != HS_SUCCESS) {
      cm_msg(MERROR, "hs_dump", "cannot find recent history file");
      return HS_FILE_ERROR;
   }

   /* open history and definition files */
   hs_open_file(start_time, "hst", O_RDONLY, &fh);
   hs_open_file(start_time, "idf", O_RDONLY, &fhd);
   hs_open_file(start_time, "idx", O_RDONLY, &fhi);
   if (fh < 0 || fhd < 0 || fhi < 0) {
      cm_msg(MERROR, "hs_dump", "cannot open index files");
      return HS_FILE_ERROR;
   }

   /* search record closest to start time */
   lseek(fhi, 0, SEEK_END);
   delta = (TELL(fhi) / sizeof(irec)) / 2;
   lseek(fhi, delta * sizeof(irec), SEEK_SET);
   do {
      delta = (int) (abs(delta) / 2.0 + 0.5);
      read(fhi, (char *) &irec, sizeof(irec));
      if (irec.time > start_time)
         delta = -delta;

      i = lseek(fhi, (delta - 1) * sizeof(irec), SEEK_CUR);
   } while (abs(delta) > 1 && irec.time != start_time);
   read(fhi, (char *) &irec, sizeof(irec));
   if (irec.time > start_time)
      delta = -abs(delta);

   i = TELL(fhi) + (delta - 1) * sizeof(irec);
   if (i <= 0)
      lseek(fhi, 0, SEEK_SET);
   else
      lseek(fhi, (delta - 1) * sizeof(irec), SEEK_CUR);
   read(fhi, (char *) &irec, sizeof(irec));

   /* read records, skip wrong IDs */
   old_def_offset = -1;
   prev_time = 0;
   last_irec_time = 0;
   do {
      if (irec.time < last_irec_time) {
         cm_msg(MERROR, "hs_dump", "corrupted history data: time does not increase: %d -> %d", last_irec_time, irec.time);
         hs_gen_index(last_irec_time);
         return HS_FILE_ERROR;
      }
      last_irec_time = irec.time;
      if (irec.event_id == event_id && irec.time <= end_time && irec.time >= start_time) {
         if (irec.time >= prev_time + interval) {
            prev_time = irec.time;
            lseek(fh, irec.offset, SEEK_SET);
            read(fh, (char *) &rec, sizeof(rec));

            /* if definition changed, read new definition */
            if ((INT) rec.def_offset != old_def_offset) {
               lseek(fh, rec.def_offset, SEEK_SET);
               read(fh, (char *) &drec, sizeof(drec));
               read(fh, str, NAME_LENGTH);

               if (tag == NULL)
                  tag = (TAG *) M_MALLOC(drec.data_size);
               else
                  tag = (TAG *) realloc(tag, drec.data_size);
               if (tag == NULL)
                  return HS_NO_MEMORY;
               read(fh, (char *) tag, drec.data_size);
               n_tag = drec.data_size / sizeof(TAG);

               /* print tag names if definition has changed */
               if (old_tag == NULL || old_n_tag != n_tag || memcmp(old_tag, tag, drec.data_size) != 0) {
                  printf("Date\t");
                  for (i = 0; i < n_tag; i++) {
                     if (tag[i].n_data == 1 || tag[i].type == TID_STRING)
                        printf("%s\t", tag[i].name);
                     else
                        for (j = 0; j < (INT) tag[i].n_data; j++)
                           printf("%s%d\t", tag[i].name, j);
                  }
                  printf("\n");

                  if (old_tag == NULL)
                     old_tag = (TAG *) M_MALLOC(drec.data_size);
                  else
                     old_tag = (TAG *) realloc(old_tag, drec.data_size);
                  memcpy(old_tag, tag, drec.data_size);
                  old_n_tag = n_tag;
               }

               old_def_offset = rec.def_offset;
               lseek(fh, irec.offset + sizeof(rec), SEEK_SET);
            }

            /* print time from header */
            if (binary_time)
               printf("%d ", irec.time);
            else {
               ltime = (time_t) irec.time;
               sprintf(str, "%s", ctime(&ltime) + 4);
               str[20] = '\t';
               printf("%s", str);
            }

            /* read data */
            read(fh, data_buffer, rec.data_size);

            /* interprete data from tag definition */
            offset = 0;
            for (i = 0; i < n_tag; i++) {
               /* strings have a length of n_data */
               if (tag[i].type == TID_STRING) {
                  printf("%s\t", data_buffer + offset);
                  offset += tag[i].n_data;
               } else if (tag[i].n_data == 1) {
                  /* non-array data */
                  db_sprintf(str, data_buffer + offset, rpc_tid_size(tag[i].type), 0, tag[i].type);
                  printf("%s\t", str);
                  offset += rpc_tid_size(tag[i].type);
               } else
                  /* loop over array data */
                  for (j = 0; j < (INT) tag[i].n_data; j++) {
                     db_sprintf(str, data_buffer + offset, rpc_tid_size(tag[i].type), 0, tag[i].type);
                     printf("%s\t", str);
                     offset += rpc_tid_size(tag[i].type);
                  }
            }
            printf("\n");
         }
      }

      /* read next index record */
      i = read(fhi, (char *) &irec, sizeof(irec));

      /* end of file: search next history file */
      if (i <= 0) {
         close(fh);
         close(fhd);
         close(fhi);

         /* advance one day */
         ltime = (time_t) last_irec_time;
         tms = localtime(&ltime);
         tms->tm_hour = tms->tm_min = tms->tm_sec = 0;
         last_irec_time = (DWORD) mktime(tms);

         last_irec_time += 3600 * 24;
         if (last_irec_time > end_time)
            break;

         /* search next file */
         status = hs_search_file((DWORD *) & last_irec_time, 1);
         if (status != HS_SUCCESS)
            break;

         /* open history and definition files */
         hs_open_file(last_irec_time, "hst", O_RDONLY, &fh);
         hs_open_file(last_irec_time, "idf", O_RDONLY, &fhd);
         hs_open_file(last_irec_time, "idx", O_RDONLY, &fhi);
         if (fh < 0 || fhd < 0 || fhi < 0) {
            cm_msg(MERROR, "hs_dump", "cannot open index files");
            break;
         }

         /* read first record */
         i = read(fhi, (char *) &irec, sizeof(irec));
         if (i <= 0)
            break;

         /* old definition becomes invalid */
         old_def_offset = -1;
      }
   } while (irec.time < end_time);

   M_FREE(tag);
   M_FREE(old_tag);
   close(fh);
   close(fhd);
   close(fhi);

   return HS_SUCCESS;
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************/
INT hs_fdump(const char *file_name, DWORD id, BOOL binary_time)
/********************************************************************\

  Routine: hs_fdump

  Purpose: Display history for a given history file

  Input:
    char   *file_name       Name of file to dump
    DWORD  event_id         Event ID
    BOOL   binary_time      Display DWORD time stamp

  Output:
    <screen output>

  Function value:
    HS_SUCCESS              Successful completion
    HS_FILE_ERROR           Cannot open history file

\********************************************************************/
{
   int fh;
   INT n;
   time_t ltime;
   HIST_RECORD rec;
   char event_name[NAME_LENGTH];
   char str[256];

   /* open file, add O_BINARY flag for Windows NT */
   sprintf(str, "%s%s", _hs_path_name, file_name);
   fh = open(str, O_RDONLY | O_BINARY, 0644);
   if (fh < 0) {
      cm_msg(MERROR, "hs_fdump", "cannot open file %s", str);
      return HS_FILE_ERROR;
   }

   /* loop over file records in .hst file */
   do {
      n = read(fh, (char *) &rec, sizeof(rec));
      if (n < sizeof(rec))
         break;

      /* check if record type is definition */
      if (rec.record_type == RT_DEF) {
         /* read name */
         read(fh, event_name, sizeof(event_name));

         if (rec.event_id == id || id == 0)
            printf("Event definition %s, ID %d\n", event_name, rec.event_id);

         /* skip tags */
         lseek(fh, rec.data_size, SEEK_CUR);
      } else {
         /* print data record */
         if (binary_time)
            sprintf(str, "%d ", rec.time);
         else {
            ltime = (time_t) rec.time;
            strcpy(str, ctime(&ltime) + 4);
            str[15] = 0;
         }
         if (rec.event_id == id || id == 0)
            printf("ID %d, %s, size %d\n", rec.event_id, str, rec.data_size);

         /* skip data */
         lseek(fh, rec.data_size, SEEK_CUR);
      }

   } while (TRUE);

   close(fh);

   return HS_SUCCESS;
}
#endif                          /* OS_VXWORKS hs section */

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/**dox***************************************************************/
         /** @} *//* end of hsfunctioncode */

/**dox***************************************************************/
