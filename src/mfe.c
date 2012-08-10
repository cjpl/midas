/********************************************************************\

  Name:         mfe.c
  Created by:   Stefan Ritt

  Contents:     The system part of the MIDAS frontend. Has to be
                linked with user code to form a complete frontend

  $Id$

\********************************************************************/

#include <stdio.h>
#include <assert.h>
#include "midas.h"
#include "msystem.h"
#include "mcstd.h"

/*------------------------------------------------------------------*/

/* items defined in frontend.c */

extern char *frontend_name;
extern char *frontend_file_name;
extern BOOL frontend_call_loop;
extern INT max_event_size;
extern INT max_event_size_frag;
extern INT event_buffer_size;
extern INT display_period;
extern INT frontend_init(void);
extern INT frontend_exit(void);
extern INT frontend_loop(void);
extern INT begin_of_run(INT run_number, char *error);
extern INT end_of_run(INT run_number, char *error);
extern INT pause_run(INT run_number, char *error);
extern INT resume_run(INT run_number, char *error);
extern INT poll_event(INT source, INT count, BOOL test);
extern INT interrupt_configure(INT cmd, INT source, POINTER_T adr);

/*------------------------------------------------------------------*/

/* globals */

INT rpc_mode = 1; // 0 for RPC socket, 1 for event socket

#define SERVER_CACHE_SIZE  100000       /* event cache before buffer */

#define ODB_UPDATE_TIME      1000       /* 1 seconds for ODB update */

#define DEFAULT_FE_TIMEOUT  60000       /* 60 seconds for watchdog timeout */

INT run_state;                  /* STATE_RUNNING, STATE_STOPPED, STATE_PAUSED */
INT run_number;
DWORD actual_time;              /* current time in seconds since 1970 */
DWORD actual_millitime;         /* current time in milliseconds */
DWORD rate_period;              /* period in ms for rate calculations */

char host_name[HOST_NAME_LENGTH];
char exp_name[NAME_LENGTH];
char full_frontend_name[256];

INT max_bytes_per_sec;
INT optimize = 0;               /* set this to one to opimize TCP buffer size */
INT fe_stop = 0;                /* stop switch for VxWorks */
BOOL debug;                     /* disable watchdog messages from server */
DWORD auto_restart = 0;         /* restart run after event limit reached stop */
INT manual_trigger_event_id = 0;        /* set from the manual_trigger callback */
INT frontend_index = -1;        /* frontend index for event building */
BOOL lockout_readout_thread = TRUE; /* manual triggers, periodic events and 1Hz flush cache lockout the readout thread */

HNDLE hDB;

extern EQUIPMENT equipment[];

EQUIPMENT *interrupt_eq = NULL;
EQUIPMENT *multithread_eq = NULL;
BOOL slowcont_eq = FALSE;
void *event_buffer;
void *frag_buffer = NULL;

int *n_events;

/* inter-thread communication */
int rbh1=0, rbh2=0, rbh1_next=0, rbh2_next=0;
volatile int stop_all_threads = 0;
int readout_thread(void *param);
volatile int readout_thread_active = 0;
void mfe_error_check(void);

int send_event(INT idx, BOOL manual_trig);
int receive_trigger_event(EQUIPMENT *eq);
void send_all_periodic_events(INT transition);
void interrupt_routine(void);
void readout_enable(BOOL flag);
int readout_enabled(void);
void display(BOOL bInit);
void rotate_wheel(void);
BOOL logger_root();
INT check_polled_events(void);

/*---- ODB records -------------------------------------------------*/

#define EQUIPMENT_COMMON_STR "\
Event ID = WORD : 0\n\
Trigger mask = WORD : 0\n\
Buffer = STRING : [32] SYSTEM\n\
Type = INT : 0\n\
Source = INT : 0\n\
Format = STRING : [8] FIXED\n\
Enabled = BOOL : 0\n\
Read on = INT : 0\n\
Period = INT : 0\n\
Event limit = DOUBLE : 0\n\
Num subevents = DWORD : 0\n\
Log history = INT : 0\n\
Frontend host = STRING : [32] \n\
Frontend name = STRING : [32] \n\
Frontend file name = STRING : [256] \n\
Status = STRING : [256] \n\
Status color = STRING : [32] \n\
"

#define EQUIPMENT_STATISTICS_STR "\
Events sent = DOUBLE : 0\n\
Events per sec. = DOUBLE : 0\n\
kBytes per sec. = DOUBLE : 0\n\
"

/*------------------------------------------------------------------*/

void set_rate_period(int ms)
{
   rate_period = ms;
}

int get_rate_period()
{
   return rate_period;
}

/*-- transition callbacks ------------------------------------------*/

/*-- start ---------------------------------------------------------*/

INT tr_start(INT rn, char *error)
{
   INT i, status;

   /* disable interrupts or readout thread
    * if somehow it was not stopped from previous run */
   readout_enable(FALSE);

   /* reset serial numbers and statistics */
   for (i = 0; equipment[i].name[0]; i++) {
      equipment[i].serial_number = 0;
      equipment[i].subevent_number = 0;
      equipment[i].stats.events_sent = 0;
      equipment[i].stats.events_per_sec = 0;
      equipment[i].stats.kbytes_per_sec = 0;
      equipment[i].odb_in = equipment[i].odb_out = 0;
      n_events[i] = 0;
   }
   db_send_changed_records();

   status = begin_of_run(rn, error);

   if (status == CM_SUCCESS) {
      run_state = STATE_RUNNING;
      run_number = rn;

      send_all_periodic_events(TR_START);

      if (display_period) {
         ss_printf(14, 2, "Running ");
         ss_printf(36, 2, "%d", rn);
      }

      /* enable interrupts or readout thread */
      readout_enable(TRUE);
   }

   return status;
}

/*-- prestop -------------------------------------------------------*/

INT tr_stop(INT rn, char *error)
{
   INT status, i;
   EQUIPMENT *eq;

   /* disable interrupts or readout thread */
   readout_enable(FALSE);

   status = end_of_run(rn, error);

   /* check if event(s) happened just before disabling the trigger */
   if ((i = check_polled_events()) > 0) {
      // cm_msg(MINFO, "tr_stop", "sent remaining %d polled events", i);
   }

   if (status == CM_SUCCESS) {
      /* don't send events if already stopped */
      if (run_state != STATE_STOPPED)
         send_all_periodic_events(TR_STOP);

      run_state = STATE_STOPPED;
      run_number = rn;

      if (display_period)
         ss_printf(14, 2, "Stopped ");
   } else
      readout_enable(TRUE);

   for (i = 0; equipment[i].name[0]; i++) {
      /* read remaining events from ring buffers */
      if (equipment[i].info.eq_type & (EQ_MULTITHREAD | EQ_INTERRUPT)) {
         while (receive_trigger_event(equipment+i) > 0);
      }

      /* flush remaining buffered events */
      rpc_flush_event();
      if (equipment[i].buffer_handle) {
         INT err = bm_flush_cache(equipment[i].buffer_handle, SYNC);
         if (err != BM_SUCCESS) {
            cm_msg(MERROR, "tr_stop", "bm_flush_cache(SYNC) error %d", err);
            return err;
         }
      }
   }

   /* update final statistics record in ODB */
   for (i = 0; equipment[i].name[0]; i++) {
      eq = &equipment[i];
      eq->stats.events_sent += eq->events_sent;
      eq->stats.events_per_sec = 0;
      eq->stats.kbytes_per_sec = 0;
      eq->bytes_sent = 0;
      eq->events_sent = 0;
      n_events[i] = 0;
   }

   db_send_changed_records();

   return status;
}

/*-- pause ---------------------------------------------------------*/

INT tr_pause(INT rn, char *error)
{
   INT status;

   /* disable interrupts or readout thread */
   readout_enable(FALSE);

   status = pause_run(rn, error);

   if (status == CM_SUCCESS) {
      run_state = STATE_PAUSED;
      run_number = rn;

      send_all_periodic_events(TR_PAUSE);

      if (display_period)
         ss_printf(14, 2, "Paused  ");
   } else
      readout_enable(TRUE);

   return status;
}

/*-- resume --------------------------------------------------------*/

INT tr_resume(INT rn, char *error)
{
   INT status;

   status = resume_run(rn, error);

   if (status == CM_SUCCESS) {
      run_state = STATE_RUNNING;
      run_number = rn;

      send_all_periodic_events(TR_RESUME);

      if (display_period)
         ss_printf(14, 2, "Running ");

      /* enable interrupts or readout thread */
      readout_enable(TRUE);
   }

   return status;
}

/*------------------------------------------------------------------*/

INT manual_trigger(INT idx, void *prpc_param[])
{
   int i;

   i = idx; /* avoid compiler warning */
   manual_trigger_event_id = CWORD(0);
   return SUCCESS;
}

/*------------------------------------------------------------------*/

int sc_thread(void *info)
{
   DEVICE_DRIVER *device_drv = info;
   int i, status, cmd;
   int current_channel = 0;
   int current_priority_channel = 0;
   float value;
   int *last_update;
   unsigned int current_time;
   DWORD last_time;

   last_update = calloc(device_drv->channels, sizeof(int));
   last_time = ss_millitime();
   
   /* pass start command to device driver (neeeded for EPICS CA driver) */
   status = device_drv->dd(CMD_START, device_drv->dd_info);

   do {
      /* read one channel from device */
      for (cmd = CMD_GET_FIRST; cmd <= CMD_GET_LAST ; cmd++) {
         value = (float)ss_nan();
         status = device_drv->dd(cmd, device_drv->dd_info, current_channel, &value);

         ss_semaphore_wait_for(device_drv->semaphore, 1000);
         device_drv->mt_buffer->channel[current_channel].variable[cmd] = value;
         device_drv->mt_buffer->status = status;
         ss_semaphore_release(device_drv->semaphore);

         // printf("TID %d: channel %d, value %f\n", ss_gettid(), current_channel, value);
         
         if (device_drv->stop_thread)
            break;
      }

      /* switch to next channel in next loop */
      current_channel = (current_channel + 1) % device_drv->channels;

      /* check for priority channel */
      current_time = ss_millitime();
      i = (current_priority_channel + 1) % device_drv->channels;
      while (!(current_time - last_update[i] < 10000)) {
         i = (i + 1) % device_drv->channels;
         if (i == current_priority_channel) {
            /* non found, so finish */
            break;
         }
      }

      /* updated channel found, so read it additionally */
      if (current_time - last_update[i] < 10000) {
         current_priority_channel = i;

         for (cmd = CMD_GET_FIRST; cmd <= CMD_GET_LAST; cmd++) {
            value = (float)ss_nan();
            status = device_drv->dd(cmd, device_drv->dd_info, i, &value);

            ss_semaphore_wait_for(device_drv->semaphore, 1000);
            device_drv->mt_buffer->channel[i].variable[cmd] = value;
            device_drv->mt_buffer->status = status;
            ss_semaphore_release(device_drv->semaphore);
            
            // printf("TID %d: channel %d, value %f\n", ss_gettid(), current_channel, value);

            if (device_drv->stop_thread)
               break;
         }
      }

      /* check if anything to write to device */
      for (i = 0; i < device_drv->channels; i++) {

         for (cmd = CMD_SET_FIRST; cmd <= CMD_SET_LAST; cmd++) {
            if (!ss_isnan(device_drv->mt_buffer->channel[i].variable[cmd])) {
               ss_semaphore_wait_for(device_drv->semaphore, 1000);
               value = device_drv->mt_buffer->channel[i].variable[cmd];
               device_drv->mt_buffer->channel[i].variable[cmd] = (float) ss_nan();
               device_drv->mt_buffer->status = status;
               ss_semaphore_release(device_drv->semaphore);

               status = device_drv->dd(cmd, device_drv->dd_info, i, value);
               last_update[i] = ss_millitime();
            }
            if (device_drv->stop_thread)
               break;
         }
      }

      /* limit data rate if defined in equipment list */
      if (current_channel == 0)
         if (device_drv->pequipment && device_drv->pequipment->event_limit) {
            while (ss_millitime() - last_time < (DWORD)device_drv->pequipment->event_limit &&
                   !device_drv->stop_thread)
               ss_sleep(10);
            last_time = ss_millitime();
         }

   } while (device_drv->stop_thread == 0);
   
   /* send stop command to device driver */
   status = device_drv->dd(CMD_STOP, device_drv->dd_info);

   free(last_update);

   /* signal stopped thread */
   device_drv->stop_thread = 2;

   return SUCCESS;
}

/*------------------------------------------------------------------*/

INT device_driver(DEVICE_DRIVER * device_drv, INT cmd, ...)
{
   va_list argptr;
   HNDLE hKey;
   INT channel, status, i, j;
   float value, *pvalue;
   char *name, *label, str[256];

   va_start(argptr, cmd);
   status = FE_SUCCESS;

   switch (cmd) {
   case CMD_INIT:
      hKey = va_arg(argptr, HNDLE);

      if (device_drv->flags & DF_MULTITHREAD) {
         status = device_drv->dd(CMD_INIT, hKey, &device_drv->dd_info,
                                    device_drv->channels, device_drv->flags,
                                    device_drv->bd);

         if (status == FE_SUCCESS && (device_drv->flags & DF_MULTITHREAD)) {
            /* create inter-thread data exchange buffers */
            device_drv->mt_buffer = (DD_MT_BUFFER *) calloc(1, sizeof(DD_MT_BUFFER));
            device_drv->mt_buffer->n_channels = device_drv->channels;
            device_drv->mt_buffer->channel = (DD_MT_CHANNEL *) calloc(device_drv->channels, sizeof(DD_MT_CHANNEL));
            assert(device_drv->mt_buffer->channel);

            /* set all set and get values to NaN */
            for (i=0 ; i<device_drv->channels ; i++)
               for (j=CMD_SET_FIRST ; j<=CMD_GET_LAST ; j++)
                  device_drv->mt_buffer->channel[i].variable[j] = (float)ss_nan();

            /* get default names for this driver already now */
            for (i = 0; i < device_drv->channels; i++)
               device_drv->dd(CMD_GET_LABEL, device_drv->dd_info, i,
                                 device_drv->mt_buffer->channel[i].label);

            /* create semaphore */
            sprintf(str, "DD_%s", device_drv->name);
            status = ss_semaphore_create(str, &device_drv->semaphore);
            if (status != SS_CREATED && status != SS_SUCCESS)
               return FE_ERR_DRIVER;
            status = FE_SUCCESS;
         }
      } else {
         status = device_drv->dd(CMD_INIT, hKey, &device_drv->dd_info,
                                    device_drv->channels, device_drv->flags,
                                    device_drv->bd);
      }
      break;

   case CMD_START:
      if ((device_drv->flags & DF_MULTITHREAD) && device_drv->mt_buffer != NULL) {
         /* create dedicated thread for this device */
         device_drv->mt_buffer->thread_id = ss_thread_create(sc_thread, device_drv);
      }
      break;

   case CMD_STOP:
      if ((device_drv->flags & DF_MULTITHREAD) && device_drv->mt_buffer != NULL) {
         device_drv->stop_thread = 1;

         /* wait for max. 5 seconds until thread has gracefully stopped */
         for (i = 0; i < 500; i++) {
            if (device_drv->stop_thread == 2)
               break;
            ss_sleep(10);
         }

         /* if timeout expired, kill thread */
         if (i == 500)
            ss_thread_kill(device_drv->mt_buffer->thread_id);

         ss_semaphore_delete(device_drv->semaphore, TRUE);
         free(device_drv->mt_buffer->channel);
         free(device_drv->mt_buffer);
      }
      break;

   case CMD_EXIT:
      status = device_drv->dd(CMD_EXIT, device_drv->dd_info);
      break;

   case CMD_SET_LABEL:
      channel = va_arg(argptr, INT);
      label = va_arg(argptr, char *);
      status = device_drv->dd(CMD_SET_LABEL, device_drv->dd_info, channel, label);
      break;

   case CMD_GET_LABEL:
      channel = va_arg(argptr, INT);
      name = va_arg(argptr, char *);
      status = device_drv->dd(CMD_GET_LABEL, device_drv->dd_info, channel, name);
      break;

   default:

      if (cmd >= CMD_SET_FIRST && cmd <= CMD_SET_LAST) {

         /* transfer data to sc_thread for SET commands */
         channel = va_arg(argptr, INT);
         value = (float) va_arg(argptr, double);        // floats are passed as double
         if (device_drv->flags & DF_MULTITHREAD) {
            ss_semaphore_wait_for(device_drv->semaphore, 1000);
            device_drv->mt_buffer->channel[channel].variable[cmd] = value;
            status = device_drv->mt_buffer->status;
            ss_semaphore_release(device_drv->semaphore);
         } else {
            status = device_drv->dd(cmd, device_drv->dd_info, channel, value);
         }

      } else if (cmd >= CMD_GET_FIRST && cmd <= CMD_GET_LAST) {

         /* transfer data from sc_thread for GET commands */
         channel = va_arg(argptr, INT);
         pvalue = va_arg(argptr, float *);
         if (device_drv->flags & DF_MULTITHREAD) {
            ss_semaphore_wait_for(device_drv->semaphore, 1000);
            *pvalue = device_drv->mt_buffer->channel[channel].variable[cmd];
            status = device_drv->mt_buffer->status;
            ss_semaphore_release(device_drv->semaphore);
         } else
            status = device_drv->dd(cmd, device_drv->dd_info, channel, pvalue);

      } else {

         /* all remaining commands which are passed directly to the device driver */
         channel = va_arg(argptr, INT);
         pvalue = va_arg(argptr, float *);
         status = device_drv->dd(cmd, device_drv->dd_info, channel, pvalue);
      }

      break;
   }

   va_end(argptr);
   return status;
}

/*------------------------------------------------------------------*/

INT register_equipment(void)
{
   INT idx, size, status;
   char str[256];
   EQUIPMENT_INFO *eq_info;
   EQUIPMENT_STATS *eq_stats;
   HNDLE hKey;
   BANK_LIST *bank_list;
   DWORD dummy;

   /* get current ODB run state */
   size = sizeof(run_state);
   run_state = STATE_STOPPED;
   db_get_value(hDB, 0, "/Runinfo/State", &run_state, &size, TID_INT, TRUE);
   size = sizeof(run_number);
   run_number = 1;
   status =
       db_get_value(hDB, 0, "/Runinfo/Run number", &run_number, &size, TID_INT, TRUE);
   assert(status == SUCCESS);

   /* scan EQUIPMENT table from FRONTEND.C */
   for (idx = 0; equipment[idx].name[0]; idx++) {
      eq_info = &equipment[idx].info;
      eq_stats = &equipment[idx].stats;

      if (eq_info->event_id == 0) {
         printf("\nEvent ID 0 for %s not allowed\n", equipment[idx].name);
         cm_disconnect_experiment();
         ss_sleep(5000);
         exit(0);
      }

      /* init status */
      equipment[idx].status = FE_SUCCESS;

      /* check for % in equipment (needed for event building) */
      if (frontend_index != -1) {
         /* modify equipment name to <name>xx where xx is the frontend index */
         if (strchr(equipment[idx].name, '%')) {
            strcpy(str, equipment[idx].name);
            sprintf(equipment[idx].name, str, frontend_index);
         }

         /* modify event buffer name to <name>xx where xx is the frontend index */
         if (strchr(eq_info->buffer, '%')) {
            strcpy(str, eq_info->buffer);
            sprintf(eq_info->buffer, str, frontend_index);
         }
      } else {
         /* stip %.. */
         if (strchr(equipment[idx].name, '%'))
            *strchr(equipment[idx].name, '%') = 0;
         if (strchr(eq_info->buffer, '%'))
            *strchr(eq_info->buffer, '%') = 0;
      }

      sprintf(str, "/Equipment/%s/Common", equipment[idx].name);

      /* get last event limit from ODB */
      if (eq_info->eq_type != EQ_SLOW) {
         db_find_key(hDB, 0, str, &hKey);
         size = sizeof(double);
         if (hKey)
            db_get_value(hDB, hKey, "Event limit", &eq_info->event_limit, &size,
                         TID_DOUBLE, TRUE);
      }

      /* Create common subtree */
      status = db_check_record(hDB, 0, str, EQUIPMENT_COMMON_STR, FALSE);
      if (status == DB_NO_KEY || status == DB_STRUCT_MISMATCH) {
         db_create_record(hDB, 0, str, EQUIPMENT_COMMON_STR);
         db_find_key(hDB, 0, str, &hKey);
         db_set_record(hDB, hKey, eq_info, sizeof(EQUIPMENT_INFO), 0);
      } else if (status != DB_SUCCESS) {
         printf("Cannot check equipment record, status = %d\n", status);
         ss_sleep(3000);
      }
      db_find_key(hDB, 0, str, &hKey);
      assert(hKey);

      /* set fixed parameters from user structure */
      db_set_value(hDB, hKey, "Event ID", &eq_info->event_id, sizeof(WORD), 1, TID_WORD);
      db_set_value(hDB, hKey, "Type", &eq_info->eq_type, sizeof(INT), 1, TID_INT);
      db_set_value(hDB, hKey, "Source", &eq_info->source, sizeof(INT), 1, TID_INT);

      /* open hot link to equipment info */
      db_open_record(hDB, hKey, eq_info, sizeof(EQUIPMENT_INFO), MODE_READ, NULL, NULL);

      if (equal_ustring(eq_info->format, "YBOS"))
	      assert(!"YBOS not supported anymore");
      else if (equal_ustring(eq_info->format, "FIXED"))
         equipment[idx].format = FORMAT_FIXED;
      else                      /* default format is MIDAS */
         equipment[idx].format = FORMAT_MIDAS;

      gethostname(eq_info->frontend_host, sizeof(eq_info->frontend_host));
      strcpy(eq_info->frontend_name, full_frontend_name);
      strcpy(eq_info->frontend_file_name, frontend_file_name);
      sprintf(eq_info->status, "%s@%s", full_frontend_name, eq_info->frontend_host);
      strcpy(eq_info->status_color, "#00FF00");

      /* update variables in ODB */
      db_set_record(hDB, hKey, eq_info, sizeof(EQUIPMENT_INFO), 0);

      /*---- Create variables record ---------------------------------*/

      sprintf(str, "/Equipment/%s/Variables", equipment[idx].name);
      if (equipment[idx].event_descrip) {
         if (equipment[idx].format == FORMAT_FIXED)
            db_check_record(hDB, 0, str, (char *) equipment[idx].event_descrip, TRUE);
         else {
            /* create bank descriptions */
            bank_list = (BANK_LIST *) equipment[idx].event_descrip;

            for (; bank_list->name[0]; bank_list++) {
               /* mabye needed later...
                  if (bank_list->output_flag == 0)
                  continue;
                */

               if (bank_list->type == TID_STRUCT) {
                  sprintf(str, "/Equipment/%s/Variables/%s", equipment[idx].name,
                          bank_list->name);
                  status =
                      db_check_record(hDB, 0, str, strcomb((const char**)bank_list->init_str), TRUE);
                  if (status != DB_SUCCESS) {
                     printf("Cannot check/create record \"%s\", status = %d\n", str,
                            status);
                     ss_sleep(3000);
                  }
               } else {
                  sprintf(str, "/Equipment/%s/Variables/%s", equipment[idx].name,
                          bank_list->name);
                  dummy = 0;
                  db_set_value(hDB, 0, str, &dummy, rpc_tid_size(bank_list->type), 1,
                               bank_list->type);
               }
            }
         }
      } else
         db_create_key(hDB, 0, str, TID_KEY);

      sprintf(str, "/Equipment/%s/Variables", equipment[idx].name);
      db_find_key(hDB, 0, str, &hKey);
      equipment[idx].hkey_variables = hKey;

      /*---- Create and initialize statistics tree -------------------*/

      sprintf(str, "/Equipment/%s/Statistics", equipment[idx].name);

      status = db_check_record(hDB, 0, str, EQUIPMENT_STATISTICS_STR, TRUE);
      if (status != DB_SUCCESS) {
         printf("Cannot create/check statistics record \'%s\', error %d\n", str, status);
         ss_sleep(3000);
      }

      status = db_find_key(hDB, 0, str, &hKey);
      if (status != DB_SUCCESS) {
         printf("Cannot find statistics record \'%s\', error %d\n", str, status);
         ss_sleep(3000);
      }

      eq_stats->events_sent = 0;
      eq_stats->events_per_sec = 0;
      eq_stats->kbytes_per_sec = 0;

      /* open hot link to statistics tree */
      status = db_open_record(hDB, hKey, eq_stats, sizeof(EQUIPMENT_STATS), MODE_WRITE, NULL, NULL);
      if (status == DB_NO_ACCESS) {
         /* record is probably still in exclusive access by dead FE, so reset it */
         status = db_set_mode(hDB, hKey, MODE_READ | MODE_WRITE | MODE_DELETE, TRUE);
         if (status != DB_SUCCESS)
            cm_msg(MERROR, "register_equipment", 
                   "Cannot change access mode for record \'%s\', error %d", str, status);
         else
            cm_msg(MINFO, "register_equipment", "Recovered access mode for record \'%s\'", str);
         status = db_open_record(hDB, hKey, eq_stats, sizeof(EQUIPMENT_STATS), MODE_WRITE, NULL, NULL);
      }
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "register_equipment", 
                "Cannot open statistics record \'%s\', error %d", str, status);
         ss_sleep(3000);
      }

      /*---- open event buffer ---------------------------------------*/

      /* check for fragmented event */
      if (eq_info->eq_type & EQ_FRAGMENTED) {
         if (frag_buffer == NULL)
            frag_buffer = malloc(max_event_size_frag);

         if (frag_buffer == NULL) {
            cm_msg(MERROR, "register_equipment",
                  "Not enough memory to allocate buffer for fragmented events");
            return SS_NO_MEMORY;
         }
      }

      if (eq_info->buffer[0]) {
         status =
             bm_open_buffer(eq_info->buffer, 2*MAX_EVENT_SIZE,
                            &equipment[idx].buffer_handle);
         if (status != BM_SUCCESS && status != BM_CREATED) {
            cm_msg(MERROR, "register_equipment",
                   "Cannot open event buffer \"%s\" size %d, bm_open_buffer() status %d", eq_info->buffer, 2*MAX_EVENT_SIZE, status);
            return 0;
         }

         /* set the default buffer cache size */
         bm_set_cache_size(equipment[idx].buffer_handle, 0, SERVER_CACHE_SIZE);
      } else
         equipment[idx].buffer_handle = 0;
   }

   n_events = calloc(sizeof(int), idx);

   return SUCCESS;
}

/*------------------------------------------------------------------*/

INT initialize_equipment(void)
{
   INT idx, i, j, k, n, count;
   char str[256];
   DWORD start_time, delta_time;
   EQUIPMENT_INFO *eq_info;
   EQUIPMENT_STATS *eq_stats;
   BOOL manual_trig_flag = FALSE;

   /* scan EQUIPMENT table from FRONTEND.C */
   for (idx = 0; equipment[idx].name[0]; idx++) {
      eq_info = &equipment[idx].info;
      eq_stats = &equipment[idx].stats;

      /*---- initialize interrupt events -----------------------------*/

      if (eq_info->eq_type & EQ_INTERRUPT) {
         /* install interrupt for interrupt events */

         for (i = 0; equipment[i].name[0]; i++)
            if (equipment[i].info.eq_type & EQ_POLLED) {
               equipment[idx].status = FE_ERR_DISABLED;
               cm_msg(MINFO, "initialize_equipment",
                      "Interrupt readout cannot be combined with polled readout");
            }

         if (equipment[idx].status != FE_ERR_DISABLED) {
            if (eq_info->enabled) {
               interrupt_eq = &equipment[idx];
               
               /* create ring buffer for inter-thread data transfer */
               if (!rbh1) {
                  rb_create(event_buffer_size, max_event_size, &rbh1);
                  rbh2 = rbh1;
               }
               
               /* establish interrupt handler */
               interrupt_configure(CMD_INTERRUPT_ATTACH, idx,
                                   (POINTER_T) interrupt_routine);
            } else {
               equipment[idx].status = FE_ERR_DISABLED;
               cm_msg(MINFO, "initialize_equipment",
                      "Equipment %s disabled in file \"frontend.c\"",
                      equipment[idx].name);
            }
         }
      }

      /*---- evaluate polling count ----------------------------------*/

      if (eq_info->eq_type & (EQ_POLLED | EQ_MULTITHREAD)) {
         if (eq_info->eq_type & EQ_INTERRUPT) {
            if (eq_info->eq_type & EQ_POLLED)
               cm_msg(MERROR, "register_equipment",
                      "Equipment \"%s\" cannot be of type EQ_INTERRUPT and EQ_POLLED at the same time", 
                      equipment[idx].name);
            else
               cm_msg(MERROR, "register_equipment",
                      "Equipment \"%s\" cannot be of type EQ_INTERRUPT and EQ_MULTITHREAD at the same time", 
                      equipment[idx].name);
            return 0;
         }

         if (display_period)
            printf("\nCalibrating");

         count = 1;
         do {
            if (display_period)
               printf(".");

            start_time = ss_millitime();

            poll_event(equipment[idx].info.source, count, TRUE);

            delta_time = ss_millitime() - start_time;

            if (count == 1 && delta_time > eq_info->period * 1.2) {
               cm_msg(MERROR, "register_equipment", "Polling routine with count=1 takes %d ms", delta_time);
               ss_sleep(3000);
               break;
            }

            if (delta_time > 0)
               count = (INT) ((double) count * eq_info->period / delta_time);
            else
               count *= 100;

         } while (delta_time > eq_info->period * 1.2 || delta_time < eq_info->period * 0.8);

         equipment[idx].poll_count = count;
      }

      /*---- initialize multithread events -------------------------*/

      if (eq_info->eq_type & EQ_MULTITHREAD) {
         /* install interrupt for interrupt events */

         for (i = 0; equipment[i].name[0]; i++)
            if (equipment[i].info.eq_type & EQ_POLLED) {
               equipment[idx].status = FE_ERR_DISABLED;
               cm_msg(MINFO, "initialize_equipment",
                      "Multi-threaded readout cannot be combined with polled readout for equipment \'%s\'", equipment[i].name);
            }

         if (equipment[idx].status != FE_ERR_DISABLED) {
            if (eq_info->enabled) {
               if (multithread_eq) {
                  equipment[idx].status = FE_ERR_DISABLED;
                  cm_msg(MINFO, "initialize_equipment",
                         "Defined more than one equipment with multi-threaded readout for equipment \'%s\'", equipment[i].name);
               } else {
                  multithread_eq = &equipment[idx];

                  /* create ring buffer for inter-thread data transfer */
                  if (!rbh1) {
                     rb_create(event_buffer_size, max_event_size, &rbh1);
                     rbh2 = rbh1;
                  }

                  /* create hardware reading thread */
                  readout_enable(FALSE);
                  ss_thread_create(readout_thread, multithread_eq);
               }
            } else {
               equipment[idx].status = FE_ERR_DISABLED;
               cm_msg(MINFO, "initialize_equipment",
                      "Equipment %s disabled in file \"frontend.c\"",
                      equipment[idx].name);
            }
         }
      }

      /*---- initialize slow control equipment ---------------------*/

      if (eq_info->eq_type & EQ_SLOW) {

         set_equipment_status(equipment[idx].name, "Initializing...", "yellow");

         /* resolve duplicate device names */
         for (i = 0; equipment[idx].driver[i].name[0]; i++)
            for (j = i + 1; equipment[idx].driver[j].name[0]; j++)
               if (equal_ustring(equipment[idx].driver[i].name,
                                 equipment[idx].driver[j].name)) {
                  strcpy(str, equipment[idx].driver[i].name);
                  for (k = 0, n = 0; equipment[idx].driver[k].name[0]; k++)
                     if (equal_ustring(str, equipment[idx].driver[k].name))
                        sprintf(equipment[idx].driver[k].name, "%s_%d", str, n++);

                  break;
               }

         /* loop over equipment list and call class driver's init method */
         if (eq_info->enabled) {
            printf("%s:\n", equipment[idx].name);
            equipment[idx].status = equipment[idx].cd(CMD_INIT, &equipment[idx]);

            if (equipment[idx].status == FE_SUCCESS)
               strcpy(str, "Ok");
            else if (equipment[idx].status == FE_ERR_HW)
               strcpy(str, "Hardware error");
            else if (equipment[idx].status == FE_ERR_ODB)
               strcpy(str, "ODB error");
            else if (equipment[idx].status == FE_ERR_DRIVER)
               strcpy(str, "Driver error");
            else
               strcpy(str, "Error");
            
            if (equipment[idx].status == FE_SUCCESS)
               set_equipment_status(equipment[idx].name, str, "#00FF00");
            else {
               set_equipment_status(equipment[idx].name, str, "#FF0000");
               cm_msg(MERROR, "initialize_equipment", "Equipment %s disabled because of %s", equipment[idx].name, str);
            }

         } else {
            equipment[idx].status = FE_ERR_DISABLED;
            set_equipment_status(equipment[idx].name, "Disabled", "yellow");
         }

         /* now start threads if requested */
         if (equipment[idx].status == FE_SUCCESS)
            equipment[idx].cd(CMD_START, &equipment[idx]);   /* start threads for this equipment */

         /* remember that we have slowcontrol equipment (needed later for scheduler) */
         slowcont_eq = TRUE;

         /* let user read error messages */
         if (equipment[idx].status != FE_SUCCESS && 
             equipment[idx].status != FE_ERR_DISABLED)
            ss_sleep(3000);
      }

      /*---- register callback for manual triggered events -----------*/
      if (eq_info->eq_type & EQ_MANUAL_TRIG) {
         if (!manual_trig_flag)
            cm_register_function(RPC_MANUAL_TRIG, manual_trigger);

         manual_trig_flag = TRUE;
      }
   }

   if (slowcont_eq)
      cm_msg(MINFO, "initialize_equipment", "Slow control equipment initialized");

   return SUCCESS;
}

/*------------------------------------------------------------------*/

int set_equipment_status(const char *name, const char *eqipment_status, const char *status_color)
{
   int status, idx;
   char str[256];
   HNDLE hKey;

   for (idx = 0; equipment[idx].name[0]; idx++)
      if (equal_ustring(equipment[idx].name, name))
         break;

   if (equal_ustring(equipment[idx].name, name)) {
      sprintf(str, "/Equipment/%s/Common", name);
      db_find_key(hDB, 0, str, &hKey);
      assert(hKey);

      status = db_set_value(hDB, hKey, "Status", eqipment_status, 256, 1, TID_STRING);
      assert(status == DB_SUCCESS);
      status = db_set_value(hDB, hKey, "Status color", status_color, 32, 1, TID_STRING);
      assert(status == DB_SUCCESS);
   }

   return SUCCESS;
}

/*------------------------------------------------------------------*/

void update_odb(EVENT_HEADER * pevent, HNDLE hKey, INT format)
{
   INT size, i, status, n_data;
   char *pdata;
   char name[5];
   BANK_HEADER *pbh;
   BANK *pbk;
   BANK32 *pbk32;
   DWORD bkname;
   WORD bktype;
   HNDLE hKeyRoot, hKeyl;
   KEY key;


   /* outcommented since db_find_key does not work in FTCP mode, SR 25.4.03
      rpc_set_option(-1, RPC_OTRANSPORT, RPC_FTCP); */

   if (format == FORMAT_FIXED) {
      if (db_set_record(hDB, hKey, (char *) (pevent + 1),
                        pevent->data_size, 0) != DB_SUCCESS)
         cm_msg(MERROR, "update_odb", "event #%d size mismatch", pevent->event_id);
   } else if (format == FORMAT_MIDAS) {
      pbh = (BANK_HEADER *) (pevent + 1);
      pbk = NULL;
      pbk32 = NULL;
      do {
         /* scan all banks */
         if (bk_is32(pbh)) {
            size = bk_iterate32(pbh, &pbk32, &pdata);
            if (pbk32 == NULL)
               break;
            bkname = *((DWORD *) pbk32->name);
            bktype = (WORD) pbk32->type;
         } else {
            size = bk_iterate(pbh, &pbk, &pdata);
            if (pbk == NULL)
               break;
            bkname = *((DWORD *) pbk->name);
            bktype = (WORD) pbk->type;
         }

         n_data = size;
         if (rpc_tid_size(bktype & 0xFF))
            n_data /= rpc_tid_size(bktype & 0xFF);

         /* get bank key */
         *((DWORD *) name) = bkname;
         name[4] = 0;

         if (bktype == TID_STRUCT) {
            status = db_find_key(hDB, hKey, name, &hKeyRoot);
            if (status != DB_SUCCESS) {
               cm_msg(MERROR, "update_odb",
                      "please define bank %s in BANK_LIST in frontend.c", name);
               continue;
            }

            /* write structured bank */
            for (i = 0;; i++) {
               status = db_enum_key(hDB, hKeyRoot, i, &hKeyl);
               if (status == DB_NO_MORE_SUBKEYS)
                  break;

               db_get_key(hDB, hKeyl, &key);

               /* adjust for alignment */
               if (key.type != TID_STRING && key.type != TID_LINK)
                  pdata =
                      (void *) VALIGN(pdata, MIN(ss_get_struct_align(), key.item_size));

               status = db_set_data(hDB, hKeyl, pdata, key.item_size * key.num_values,
                                    key.num_values, key.type);
               if (status != DB_SUCCESS) {
                  cm_msg(MERROR, "update_odb", "cannot write %s to ODB", name);
                  continue;
               }

               /* shift data pointer to next item */
               pdata += key.item_size * key.num_values;
            }
         } else {
            /* write variable length bank  */
            if (n_data > 0)
               db_set_value(hDB, hKey, name, pdata, size, n_data, bktype & 0xFF);
         }

      } while (1);
   } else if (format == FORMAT_YBOS) {
     assert(!"YBOS not supported anymore");
   }

   rpc_set_option(-1, RPC_OTRANSPORT, RPC_TCP);
}

/*------------------------------------------------------------------*/

int send_event(INT idx, BOOL manual_trig)
{
   EQUIPMENT_INFO *eq_info;
   EVENT_HEADER *pevent, *pfragment;
   char *pdata;
   unsigned char *pd;
   INT i, status;
   DWORD sent, size;

   eq_info = &equipment[idx].info;

   /* check for fragmented event */
   if (eq_info->eq_type & EQ_FRAGMENTED)
      pevent = frag_buffer;
   else
      pevent = (EVENT_HEADER *)event_buffer;

   /* compose MIDAS event header */
   pevent->event_id = eq_info->event_id;
   pevent->trigger_mask = eq_info->trigger_mask;
   pevent->data_size = (INT) manual_trig;
   pevent->time_stamp = ss_time();
   pevent->serial_number = equipment[idx].serial_number++;

   equipment[idx].last_called = ss_millitime();

   /* call user readout routine */
   *((EQUIPMENT **) (pevent + 1)) = &equipment[idx];
   pevent->data_size = equipment[idx].readout((char *) (pevent + 1), 0);

   /* send event */
   if (pevent->data_size) {
      if (eq_info->eq_type & EQ_FRAGMENTED) {
         /* fragment event */
         if (pevent->data_size + sizeof(EVENT_HEADER) > (DWORD) max_event_size_frag) {
            cm_msg(MERROR, "send_event",
                   "Event size %ld larger than maximum size %d for frag. ev.",
                   (long) (pevent->data_size + sizeof(EVENT_HEADER)),
                   max_event_size_frag);
            return SS_NO_MEMORY;
         }

         /* compose fragments */
         pfragment = event_buffer;

         /* compose MIDAS event header */
         memcpy(pfragment, pevent, sizeof(EVENT_HEADER));
         pfragment->event_id |= EVENTID_FRAG1;

         /* store total event size */
         pd = (unsigned char *) (pfragment + 1);
         size = pevent->data_size;
         for (i = 0; i < 4; i++) {
            pd[i] = (unsigned char) (size & 0xFF);      /* little endian, please! */
            size >>= 8;
         }

         pfragment->data_size = sizeof(DWORD);

         pdata = (char *) (pevent + 1);

         for (i = 0, sent = 0; sent < pevent->data_size; i++) {
            if (i > 0) {
               pfragment = event_buffer;

               /* compose MIDAS event header */
               memcpy(pfragment, pevent, sizeof(EVENT_HEADER));
               pfragment->event_id |= EVENTID_FRAG;

               /* copy portion of event */
               size = pevent->data_size - sent;
               if (size > max_event_size - sizeof(EVENT_HEADER))
                  size = max_event_size - sizeof(EVENT_HEADER);

               memcpy(pfragment + 1, pdata, size);
               pfragment->data_size = size;
               sent += size;
               pdata += size;
            }

            /* send event to buffer */
            if (equipment[idx].buffer_handle) {
               status = rpc_send_event(equipment[idx].buffer_handle, pfragment,
                                       pfragment->data_size + sizeof(EVENT_HEADER), SYNC, rpc_mode);
               if (status != RPC_SUCCESS) {
                  cm_msg(MERROR, "send_event", "rpc_send_event(SYNC) error %d", status);
                  return status;
               }

               /* flush events from buffer */
               rpc_flush_event();
            }
         }

         if (equipment[idx].buffer_handle) {
            /* flush buffer cache on server side */
            status = bm_flush_cache(equipment[idx].buffer_handle, SYNC);
            if (status != BM_SUCCESS) {
               cm_msg(MERROR, "send_event", "bm_flush_cache(SYNC) error %d", status);
               return status;
            }
         }
      } else {
         /* send unfragmented event */

         if (pevent->data_size + sizeof(EVENT_HEADER) > (DWORD) max_event_size) {
            cm_msg(MERROR, "send_event", "Event size %ld larger than maximum size %d",
                   (long) (pevent->data_size + sizeof(EVENT_HEADER)), max_event_size);
            return SS_NO_MEMORY;
         }

         /* send event to buffer */
         if (equipment[idx].buffer_handle) {
            status = rpc_send_event(equipment[idx].buffer_handle, pevent,
                                    pevent->data_size + sizeof(EVENT_HEADER), SYNC, rpc_mode);
            if (status != BM_SUCCESS) {
               cm_msg(MERROR, "send_event", "bm_send_event(SYNC) error %d", status);
               return status;
            }
            rpc_flush_event();
            status = bm_flush_cache(equipment[idx].buffer_handle, SYNC);
            if (status != BM_SUCCESS) {
               cm_msg(MERROR, "send_event", "bm_flush_cache(SYNC) error %d", status);
               return status;
            }
         }

         /* send event to ODB if RO_ODB flag is set or history is on. Do not
            send SLOW events since the class driver does that */
         if ((eq_info->read_on & RO_ODB) ||
             (eq_info->history > 0 && (eq_info->eq_type & ~EQ_SLOW))) {
            update_odb(pevent, equipment[idx].hkey_variables, equipment[idx].format);
            equipment[idx].odb_out++;
         }
      }

      equipment[idx].bytes_sent += pevent->data_size + sizeof(EVENT_HEADER);
      equipment[idx].events_sent++;
   } else
      equipment[idx].serial_number--;

   for (i = 0; equipment[i].name[0]; i++)
      if (equipment[i].buffer_handle) {
         status = bm_flush_cache(equipment[i].buffer_handle, SYNC);
         if (status != BM_SUCCESS) {
            cm_msg(MERROR, "send_event", "bm_flush_cache(SYNC) error %d", status);
            return status;
         }
      }

   return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

void send_all_periodic_events(INT transition)
{
   EQUIPMENT_INFO *eq_info;
   INT i;

   for (i = 0; equipment[i].name[0]; i++) {
      eq_info = &equipment[i].info;

      if (!eq_info->enabled || equipment[i].status != FE_SUCCESS)
         continue;

      if (transition == TR_START && (eq_info->read_on & RO_BOR) == 0)
         continue;
      if (transition == TR_STOP && (eq_info->read_on & RO_EOR) == 0)
         continue;
      if (transition == TR_PAUSE && (eq_info->read_on & RO_PAUSE) == 0)
         continue;
      if (transition == TR_RESUME && (eq_info->read_on & RO_RESUME) == 0)
         continue;

      send_event(i, FALSE);
   }
}

/*------------------------------------------------------------------*/

static int _readout_enabled_flag = 0;

int readout_enabled()
{
   return _readout_enabled_flag;
}

void readout_enable(BOOL flag)
{
   _readout_enabled_flag = flag;

   if (interrupt_eq) {
      if (flag)
         interrupt_configure(CMD_INTERRUPT_ENABLE, 0, 0);
      else
         interrupt_configure(CMD_INTERRUPT_DISABLE, 0, 0);
   }

   if (multithread_eq) {
      /* readout thread might still be in readout, so wait until finished */
      if (flag == 0)
         while (readout_thread_active)
            ss_sleep(10);
   }
}

/*------------------------------------------------------------------*/

void interrupt_routine(void)
{
   int status;
   EVENT_HEADER *pevent;
   void *p;

   /* get pointer for upcoming event.
      This is a blocking call if no space available */
   status = rb_get_wp(rbh1, &p, 100000);

   if (status == DB_SUCCESS) {
      pevent = (EVENT_HEADER *)p;

      /* compose MIDAS event header */
      pevent->event_id = interrupt_eq->info.event_id;
      pevent->trigger_mask = interrupt_eq->info.trigger_mask;
      pevent->data_size = 0;
      pevent->time_stamp = actual_time;
      pevent->serial_number = interrupt_eq->serial_number++;

      /* call user readout routine */
      pevent->data_size = interrupt_eq->readout((char *) (pevent + 1), 0);

      /* send event */
      if (pevent->data_size) {

         /* put event into ring buffer */
         rb_increment_wp(rbh1, sizeof(EVENT_HEADER) + pevent->data_size);

      } else
         interrupt_eq->serial_number--;
   }
}

/*------------------------------------------------------------------*/

int readout_thread(void *param)
{
   int status, source;
   EVENT_HEADER *pevent;
   void *p;

   p = param; /* avoid compiler warning */
   while (!stop_all_threads) {
      /* obtain buffer space */
      if (rbh1_next) // if set by user code, use it
         rbh1 = rbh1_next;

      status = rb_get_wp(rbh1, &p, 0);
      if (stop_all_threads)
         break;
      if (status == DB_TIMEOUT) {
         // printf("readout_thread: Ring buffer is full, waiting for space!\n");
         ss_sleep(10);
         continue;
      }
      if (status != DB_SUCCESS)
         break;

      if (readout_enabled()) {
        
         /* indicate activity for readout_enable() */
         readout_thread_active = 1;

         /* check for new event */
         source = poll_event(multithread_eq->info.source, multithread_eq->poll_count, FALSE);

         if (source > 0) {

            if (stop_all_threads)
               break;

            pevent = (EVENT_HEADER *)p;
            /* put source at beginning of event, will be overwritten by 
               user readout code, just a special feature used by some 
               multi-source applications */
            *(INT *) (pevent + 1) = source;
            
            /* compose MIDAS event header */
            pevent->event_id = multithread_eq->info.event_id;
            pevent->trigger_mask = multithread_eq->info.trigger_mask;
            pevent->data_size = 0;
            pevent->time_stamp = actual_time;
            pevent->serial_number = multithread_eq->serial_number++;

            /* call user readout routine */
            pevent->data_size = multithread_eq->readout((char *) (pevent + 1), 0);

            /* check event size */
            if (pevent->data_size + sizeof(EVENT_HEADER) > (DWORD) max_event_size) {
               cm_msg(MERROR, "readout_thread",
                        "Event size %ld larger than maximum size %d",
                        (long) (pevent->data_size + sizeof(EVENT_HEADER)),
                        max_event_size);
               assert(FALSE);
            }

            if (pevent->data_size > 0) {
               /* put event into ring buffer */
               rb_increment_wp(rbh1, sizeof(EVENT_HEADER) + pevent->data_size);
            } else
               multithread_eq->serial_number--;
         }

         readout_thread_active = 0;

      } else // readout_enabled
        ss_sleep(10);

   }

   readout_thread_active = 0;

   return 0;
}

/*-- Receive event from readout thread or interrupt routine --------*/

void set_event_rb(INT rb)
{
   rbh2 = rb;
}

int receive_trigger_event(EQUIPMENT *eq)
{
   int status;
   EVENT_HEADER *prb, *pevent;
   void *p;
   int nbytes;

   if (0) {
      static int count = 0;
      if (((count++) % 100) == 0) {
         rb_get_buffer_level(rbh2, &nbytes);
         if (nbytes != 0)
            printf("mfe: ring buffer contains %d bytes\n", nbytes);
      }
   }

   status = rb_get_rp(rbh2, &p, 10);
   prb = (EVENT_HEADER *)p;
   if (status == DB_TIMEOUT)
      return 0;

   pevent = prb;

   /* send event */
   if (pevent->data_size) {
      if (eq->buffer_handle) {
         
         /* save event in temporary buffer to push it to the ODB later */
         if (eq->info.read_on & RO_ODB)
            memcpy(event_buffer, pevent, pevent->data_size + sizeof(EVENT_HEADER));

         /* send first event to ODB if logger writes in root format */
         if (pevent->serial_number == 0)
            if (logger_root())
               update_odb(pevent, eq->hkey_variables, eq->format);

         status = rpc_send_event(eq->buffer_handle, pevent,
                                 pevent->data_size + sizeof(EVENT_HEADER),
                                 SYNC, rpc_mode);

         if (status != SUCCESS) {
            cm_msg(MERROR, "receive_trigger_event", "rpc_send_event error %d", status);
            return -1;
         }

         eq->bytes_sent += pevent->data_size + sizeof(EVENT_HEADER);

         if (eq->info.num_subevents)
            eq->events_sent += eq->subevent_number;
         else
            eq->events_sent++;

         rotate_wheel();
      }
   }

   rb_increment_rp(rbh2, sizeof(EVENT_HEADER) + prb->data_size);

   return prb->data_size;
}

/*------------------------------------------------------------------*/

int message_print(const char *msg)
{
   char str[160];

   memset(str, ' ', 159);
   str[159] = 0;

   if (msg[0] == '[')
      msg = strchr(msg, ']') + 2;

   memcpy(str, msg, strlen(msg));
   ss_printf(0, 20, str);

   return 0;
}

/*------------------------------------------------------------------*/

void display(BOOL bInit)
{
   INT i, status;
   time_t full_time;
   char str[30];

   if (bInit) {
      ss_clear_screen();

      if (host_name[0])
         strcpy(str, host_name);
      else
         strcpy(str, "<local>");

      ss_printf(0, 0, "%s connected to %s. Press \"!\" to exit", full_frontend_name, str);
      ss_printf(0, 1,
                "================================================================================");
      ss_printf(0, 2, "Run status:   %s",
                run_state == STATE_STOPPED ? "Stopped" : run_state ==
                STATE_RUNNING ? "Running" : "Paused");
      ss_printf(25, 2, "Run number %d   ", run_number);
      ss_printf(0, 3,
                "================================================================================");
      ss_printf(0, 4,
                "Equipment     Status     Events     Events/sec Rate[kB/s] ODB->FE    FE->ODB");
      ss_printf(0, 5,
                "--------------------------------------------------------------------------------");
      for (i = 0; equipment[i].name[0]; i++)
         ss_printf(0, i + 6, "%s", equipment[i].name);
   }

   /* display time */
   time(&full_time);
   strcpy(str, ctime(&full_time) + 11);
   str[8] = 0;
   ss_printf(72, 0, "%s", str);

   for (i = 0; equipment[i].name[0]; i++) {
      status = equipment[i].status;

      if ((status == 0 || status == FE_SUCCESS) && equipment[i].info.enabled)
         ss_printf(14, i + 6, "OK       ");
      else if (!equipment[i].info.enabled)
         ss_printf(14, i + 6, "Disabled ");
      else if (status == FE_ERR_ODB)
         ss_printf(14, i + 6, "ODB Error");
      else if (status == FE_ERR_HW)
         ss_printf(14, i + 6, "HW Error ");
      else if (status == FE_ERR_DISABLED)
         ss_printf(14, i + 6, "Disabled ");
      else if (status == FE_ERR_DRIVER)
         ss_printf(14, i + 6, "Driver err");
      else
         ss_printf(14, i + 6, "Unknown  ");

      if (equipment[i].stats.events_sent > 1E9)
         ss_printf(25, i + 6, "%1.3lfG     ", equipment[i].stats.events_sent / 1E9);
      else if (equipment[i].stats.events_sent > 1E6)
         ss_printf(25, i + 6, "%1.3lfM     ", equipment[i].stats.events_sent / 1E6);
      else
         ss_printf(25, i + 6, "%1.0lf      ", equipment[i].stats.events_sent);
      ss_printf(36, i + 6, "%1.1lf      ", equipment[i].stats.events_per_sec);
      ss_printf(47, i + 6, "%1.1lf      ", equipment[i].stats.kbytes_per_sec);
      ss_printf(58, i + 6, "%ld       ", equipment[i].odb_in);
      ss_printf(69, i + 6, "%ld       ", equipment[i].odb_out);
   }

   /* go to next line */
   ss_printf(0, i + 6, "");
}

/*------------------------------------------------------------------*/

void rotate_wheel(void)
{
   static DWORD last_wheel = 0, wheel_index = 0;
   static char wheel_char[] = { '-', '\\', '|', '/' };

   if (display_period) {
      if (ss_millitime() - last_wheel > 300) {
         last_wheel = ss_millitime();
         ss_printf(79, 2, "%c", wheel_char[wheel_index]);
         wheel_index = (wheel_index + 1) % 4;
      }
   }
}

/*------------------------------------------------------------------*/

BOOL logger_root()
/* check if logger uses ROOT format */
{
   int size, i, status;
   char str[80];
   HNDLE hKeyRoot, hKey;

   if (db_find_key(hDB, 0, "/Logger/Channels", &hKeyRoot) == DB_SUCCESS) {
      for (i = 0;; i++) {
         status = db_enum_key(hDB, hKeyRoot, i, &hKey);
         if (status == DB_NO_MORE_SUBKEYS)
            break;

         strcpy(str, "MIDAS");
         size = sizeof(str);
         db_get_value(hDB, hKey, "Settings/Format", str, &size, TID_STRING, TRUE);

         if (equal_ustring(str, "ROOT"))
            return TRUE;
      }
   }

   return FALSE;
}

/*------------------------------------------------------------------*/

INT check_polled_events(void)
{
   EQUIPMENT_INFO *eq_info;
   EQUIPMENT *eq;
   EVENT_HEADER *pevent, *pfragment;
   DWORD readout_start, sent, size;
   INT i, idx, source, events_sent, status;
   char *pdata;
   unsigned char *pd;

   events_sent = 0;
   actual_millitime = ss_millitime();

   /*---- loop over equipment table -------------------------------*/
   for (idx = 0;; idx++) {
      eq = &equipment[idx];
      eq_info = &eq->info;

      /* check if end of equipment list */
      if (!eq->name[0])
         break;

      if (!eq_info->enabled)
         continue;

      if (eq->status != FE_SUCCESS)
         continue;

      if ((eq_info->eq_type & EQ_POLLED) == 0)
         continue;

      /*---- check polled events ----*/
      readout_start = actual_millitime;
      pevent = NULL;

      while ((source = poll_event(eq_info->source, eq->poll_count, FALSE)) > 0) {
         
         if (eq_info->eq_type & EQ_FRAGMENTED)
            pevent = frag_buffer;
         else
            pevent = (EVENT_HEADER *)event_buffer;

         /* compose MIDAS event header */
         pevent->event_id = eq_info->event_id;
         pevent->trigger_mask = eq_info->trigger_mask;
         pevent->data_size = 0;
         pevent->time_stamp = actual_time;
         pevent->serial_number = eq->serial_number;

         /* put source at beginning of event, will be overwritten by 
            user readout code, just a special feature used by some 
            multi-source applications */
         *(INT *) (pevent + 1) = source;

         if (eq->info.num_subevents) {
            eq->subevent_number = 0;
            do {
               *(INT *) ((char *) (pevent + 1) + pevent->data_size) = source;

               /* call user readout routine for subevent indicating offset */
               size = eq->readout((char *) (pevent + 1), pevent->data_size);
               pevent->data_size += size;
               if (size > 0) {
                  if (pevent->data_size + sizeof(EVENT_HEADER) >
                      (DWORD) max_event_size) {
                     cm_msg(MERROR, "check_polled_events",
                            "Event size %ld larger than maximum size %d",
                            (long) (pevent->data_size + sizeof(EVENT_HEADER)),
                            max_event_size);
                  }

                  eq->subevent_number++;
                  eq->serial_number++;
               }

               /* wait for next event */
               do {
                  source = poll_event(eq_info->source, eq->poll_count, FALSE);

                  if (source == FALSE) {
                     actual_millitime = ss_millitime();

                     /* repeat no more than period */
                     if (actual_millitime - readout_start > (DWORD) eq_info->period)
                        break;
                  }
               } while (source == FALSE);

            } while (eq->subevent_number < eq->info.num_subevents && source);

            /* notify readout routine about end of super-event */
            pevent->data_size = eq->readout((char *) (pevent + 1), -1);
         } else {
            /* call user readout routine indicating event source */
            pevent->data_size = eq->readout((char *) (pevent + 1), 0);

            /* check event size */
            if (eq_info->eq_type & EQ_FRAGMENTED) {
               if (pevent->data_size + sizeof(EVENT_HEADER) > (DWORD) max_event_size_frag) {
                  cm_msg(MERROR, "check_polled_events",
                        "Event size %ld larger than maximum size %d for frag. ev.",
                        (long) (pevent->data_size + sizeof(EVENT_HEADER)),
                        max_event_size_frag);
                  assert(FALSE);
               }
            } else {
               if (pevent->data_size + sizeof(EVENT_HEADER) > (DWORD) max_event_size) {
                  cm_msg(MERROR, "check_polled_events",
                        "Event size %ld larger than maximum size %d",
                        (long) (pevent->data_size + sizeof(EVENT_HEADER)),
                        max_event_size);
                  assert(FALSE);
               }
            }

            /* increment serial number if event read out sucessfully */
            if (pevent->data_size)
               eq->serial_number++;
         }

         /* send event */
         if (pevent->data_size) {

            /* check for fragmented event */
            if (eq_info->eq_type & EQ_FRAGMENTED) {
               /* compose fragments */
               pfragment = event_buffer;

               /* compose MIDAS event header */
               memcpy(pfragment, pevent, sizeof(EVENT_HEADER));
               pfragment->event_id |= EVENTID_FRAG1;

               /* store total event size */
               pd = (unsigned char *) (pfragment + 1);
               size = pevent->data_size;
               for (i = 0; i < 4; i++) {
                  pd[i] = (unsigned char) (size & 0xFF);      /* little endian, please! */
                  size >>= 8;
               }

               pfragment->data_size = sizeof(DWORD);

               pdata = (char *) (pevent + 1);

               for (i = 0, sent = 0; sent < pevent->data_size; i++) {
                  if (i > 0) {
                     pfragment = event_buffer;

                     /* compose MIDAS event header */
                     memcpy(pfragment, pevent, sizeof(EVENT_HEADER));
                     pfragment->event_id |= EVENTID_FRAG;

                     /* copy portion of event */
                     size = pevent->data_size - sent;
                     if (size > max_event_size - sizeof(EVENT_HEADER))
                        size = max_event_size - sizeof(EVENT_HEADER);

                     memcpy(pfragment + 1, pdata, size);
                     pfragment->data_size = size;
                     sent += size;
                     pdata += size;
                  }

                  /* send event to buffer */
                  if (equipment[idx].buffer_handle) {
                     status = rpc_send_event(equipment[idx].buffer_handle, pfragment,
                                             pfragment->data_size + sizeof(EVENT_HEADER), SYNC, rpc_mode);
                     if (status != RPC_SUCCESS) {
                        cm_msg(MERROR, "check_polled_events", "rpc_send_event(SYNC) error %d", status);
                        return status;
                     }

                     /* flush events from buffer */
                     rpc_flush_event();
                  }
               }
            
            } else { /*-------------------*/

               /* send unfragmented event */

               /* send first event to ODB if logger writes in root format */
               if (pevent->serial_number == 0)
                  if (logger_root())
                     update_odb(pevent, eq->hkey_variables, eq->format);

               status = rpc_send_event(eq->buffer_handle, pevent,
                                       pevent->data_size + sizeof(EVENT_HEADER),
                                       SYNC, rpc_mode);

               if (status != SUCCESS) {
                  cm_msg(MERROR, "check_polled_events", "rpc_send_event error %d", status);
                  break;
               }
            }

            eq->bytes_sent += pevent->data_size + sizeof(EVENT_HEADER);

            if (eq->info.num_subevents) {
               eq->events_sent += eq->subevent_number;
               events_sent += eq->subevent_number;
            } else {
               eq->events_sent++;
               events_sent++;
            }

            rotate_wheel();
         }

         actual_millitime = ss_millitime();

         /* repeat no more than period */
         if (actual_millitime - readout_start > (DWORD) eq_info->period)
            break;

         /* quit if event limit is reached */
         if (eq_info->event_limit > 0 &&
             eq->stats.events_sent + eq->events_sent >= eq_info->event_limit)
            break;
      }
   }

   return events_sent;
}

/*------------------------------------------------------------------*/

INT scheduler(void)
{
   EQUIPMENT_INFO *eq_info;
   EQUIPMENT *eq;
   EVENT_HEADER *pevent, *pfragment;
   DWORD last_time_network = 0, last_time_display = 0, last_time_flush = 0, 
      readout_start, sent, size, last_time_rate = 0;
   INT i, j, idx, status = 0, ch, source, state, old_flag;
   char str[80], *pdata;
   unsigned char *pd;
   BOOL buffer_done, flag, force_update = FALSE;

   INT opt_max = 0, opt_index = 0, opt_tcp_size = 128, opt_cnt = 0;
   INT err;

#ifdef OS_VXWORKS
   rpc_set_opt_tcp_size(1024);
#ifdef PPCxxx
   rpc_set_opt_tcp_size(NET_TCP_SIZE);
#endif
#endif

   /*----------------- MAIN equipment loop ------------------------------*/

   last_time_rate = ss_millitime();

   do {
      actual_millitime = ss_millitime();
      actual_time = ss_time();

      /*---- loop over equipment table -------------------------------*/
      for (idx = 0;; idx++) {
         eq = &equipment[idx];
         eq_info = &eq->info;

         /* check if end of equipment list */
         if (!eq->name[0])
            break;

         if (!eq_info->enabled)
            continue;

         if (eq->status != FE_SUCCESS)
            continue;

         /*---- call idle routine for slow control equipment ----*/
         if ((eq_info->eq_type & EQ_SLOW) && eq->status == FE_SUCCESS) {
            /* if equipment is multi-threaded, read all channel in one loop */
             
            if (eq_info->event_limit > 0) {
               if (actual_millitime - eq->last_idle >= (DWORD) eq_info->event_limit) {
                  eq->cd(CMD_IDLE, eq);
                  eq->last_idle = actual_millitime;
               }
            } else
               eq->cd(CMD_IDLE, eq);
         }

         if (run_state == STATE_STOPPED && (eq_info->read_on & RO_STOPPED) == 0)
            continue;
         if (run_state == STATE_PAUSED && (eq_info->read_on & RO_PAUSED) == 0)
            continue;
         if (run_state == STATE_RUNNING && (eq_info->read_on & RO_RUNNING) == 0)
            continue;

         /*---- check periodic events ----*/
         if ((eq_info->eq_type & EQ_PERIODIC) || (eq_info->eq_type & EQ_SLOW)) {
            if (eq_info->period == 0)
               continue;

            /* check if period over */
            if (actual_millitime - eq->last_called >= (DWORD) eq_info->period) {
               /* disable interrupts or readout thread during this event */
               old_flag = readout_enabled();
               if (old_flag && lockout_readout_thread)
                  readout_enable(FALSE);

               /* readout and send event */
               status = send_event(idx, FALSE);

               if (status != CM_SUCCESS) {
                  cm_msg(MERROR, "scheduler", "send_event error %d", status);
                  goto net_error;
               }

               /* re-enable the interrupt or readout thread after periodic */
               if (old_flag)
                  readout_enable(TRUE);
            }
         }

         /*---- check polled events ----*/
         if (eq_info->eq_type & EQ_POLLED) {
            readout_start = actual_millitime;
            pevent = NULL;

            while ((source = poll_event(eq_info->source, eq->poll_count, FALSE)) > 0) {
               
               if (eq_info->eq_type & EQ_FRAGMENTED)
                  pevent = frag_buffer;
               else
                  pevent = (EVENT_HEADER *)event_buffer;

               /* compose MIDAS event header */
               pevent->event_id = eq_info->event_id;
               pevent->trigger_mask = eq_info->trigger_mask;
               pevent->data_size = 0;
               pevent->time_stamp = actual_time;
               pevent->serial_number = eq->serial_number;

               /* put source at beginning of event, will be overwritten by 
                  user readout code, just a special feature used by some 
                  multi-source applications */
               *(INT *) (pevent + 1) = source;

               if (eq->info.num_subevents) {
                  eq->subevent_number = 0;
                  do {
                     *(INT *) ((char *) (pevent + 1) + pevent->data_size) = source;

                     /* call user readout routine for subevent indicating offset */
                     size = eq->readout((char *) (pevent + 1), pevent->data_size);
                     pevent->data_size += size;
                     if (size > 0) {
                        if (pevent->data_size + sizeof(EVENT_HEADER) >
                            (DWORD) max_event_size) {
                           cm_msg(MERROR, "scheduler",
                                  "Event size %ld larger than maximum size %d",
                                  (long) (pevent->data_size + sizeof(EVENT_HEADER)),
                                  max_event_size);
                        }

                        eq->subevent_number++;
                        eq->serial_number++;
                     }

                     /* wait for next event */
                     do {
                        source = poll_event(eq_info->source, eq->poll_count, FALSE);

                        if (source == FALSE) {
                           actual_millitime = ss_millitime();

                           /* repeat no more than period */
                           if (actual_millitime - readout_start > (DWORD) eq_info->period)
                              break;
                        }
                     } while (source == FALSE);

                  } while (eq->subevent_number < eq->info.num_subevents && source);

                  /* notify readout routine about end of super-event */
                  pevent->data_size = eq->readout((char *) (pevent + 1), -1);
               } else {
                  /* call user readout routine indicating event source */
                  pevent->data_size = eq->readout((char *) (pevent + 1), 0);

                  /* check event size */
                  if (eq_info->eq_type & EQ_FRAGMENTED) {
                     if (pevent->data_size + sizeof(EVENT_HEADER) > (DWORD) max_event_size_frag) {
                        cm_msg(MERROR, "send_event",
                              "Event size %ld larger than maximum size %d for frag. ev.",
                              (long) (pevent->data_size + sizeof(EVENT_HEADER)),
                              max_event_size_frag);
                        assert(FALSE);
                     }
                  } else {
                     if (pevent->data_size + sizeof(EVENT_HEADER) > (DWORD) max_event_size) {
                        cm_msg(MERROR, "scheduler",
                              "Event size %ld larger than maximum size %d",
                              (long) (pevent->data_size + sizeof(EVENT_HEADER)),
                              max_event_size);
                        assert(FALSE);
                     }
                  }

                  /* increment serial number if event read out sucessfully */
                  if (pevent->data_size)
                     eq->serial_number++;
               }

               /* send event */
               if (pevent->data_size) {

                  /* check for fragmented event */
                  if (eq_info->eq_type & EQ_FRAGMENTED) {
                     /* compose fragments */
                     pfragment = event_buffer;

                     /* compose MIDAS event header */
                     memcpy(pfragment, pevent, sizeof(EVENT_HEADER));
                     pfragment->event_id |= EVENTID_FRAG1;

                     /* store total event size */
                     pd = (unsigned char *) (pfragment + 1);
                     size = pevent->data_size;
                     for (i = 0; i < 4; i++) {
                        pd[i] = (unsigned char) (size & 0xFF);      /* little endian, please! */
                        size >>= 8;
                     }

                     pfragment->data_size = sizeof(DWORD);

                     pdata = (char *) (pevent + 1);

                     for (i = 0, sent = 0; sent < pevent->data_size; i++) {
                        if (i > 0) {
                           pfragment = event_buffer;

                           /* compose MIDAS event header */
                           memcpy(pfragment, pevent, sizeof(EVENT_HEADER));
                           pfragment->event_id |= EVENTID_FRAG;

                           /* copy portion of event */
                           size = pevent->data_size - sent;
                           if (size > max_event_size - sizeof(EVENT_HEADER))
                              size = max_event_size - sizeof(EVENT_HEADER);

                           memcpy(pfragment + 1, pdata, size);
                           pfragment->data_size = size;
                           sent += size;
                           pdata += size;
                        }

                        /* send event to buffer */
                        if (equipment[idx].buffer_handle) {
                           status = rpc_send_event(equipment[idx].buffer_handle, pfragment,
                                                   pfragment->data_size + sizeof(EVENT_HEADER), SYNC, rpc_mode);
                           if (status != RPC_SUCCESS) {
                              cm_msg(MERROR, "scheduler", "rpc_send_event(SYNC) error %d", status);
                              return status;
                           }

                           /* flush events from buffer */
                           rpc_flush_event();
                        }
                     }
                  
                  } else { /*-------------------*/

                     /* send unfragmented event */

                     /* send first event to ODB if logger writes in root format */
                     if (pevent->serial_number == 0)
                        if (logger_root())
                           update_odb(pevent, eq->hkey_variables, eq->format);

                     status = rpc_send_event(eq->buffer_handle, pevent,
                                             pevent->data_size + sizeof(EVENT_HEADER),
                                             SYNC, rpc_mode);

                     if (status != SUCCESS) {
                        cm_msg(MERROR, "scheduler", "rpc_send_event error %d", status);
                        goto net_error;
                     }
                  }

                  eq->bytes_sent += pevent->data_size + sizeof(EVENT_HEADER);

                  if (eq->info.num_subevents)
                     eq->events_sent += eq->subevent_number;
                  else
                     eq->events_sent++;

                  rotate_wheel();
               }

               actual_millitime = ss_millitime();

               /* send event to ODB */
               if (pevent->data_size && (eq_info->read_on & RO_ODB)) {
                  if (actual_millitime - eq->last_called > ODB_UPDATE_TIME) {
                     eq->last_called = actual_millitime;
                     update_odb(pevent, eq->hkey_variables, eq->format);
                     eq->odb_out++;
                  }
               }

               /* repeat no more than period */
               if (actual_millitime - readout_start > (DWORD) eq_info->period)
                  break;

               /* quit if event limit is reached */
               if (eq_info->event_limit > 0 &&
                   eq->stats.events_sent + eq->events_sent >= eq_info->event_limit)
                  break;
            }

         }

         /*---- send interrupt events ----*/
         if (eq_info->eq_type & (EQ_INTERRUPT | EQ_MULTITHREAD)) {
            readout_start = actual_millitime;

            do {
               size = receive_trigger_event(eq);
               if ((int)size == -1)
                  goto net_error;

               actual_millitime = ss_millitime();

               /* repeat no more than period */
               if (actual_millitime - readout_start > (DWORD) eq_info->period)
                  break;

               /* quit if event limit is reached */
               if (eq_info->event_limit > 0 &&
                   eq->stats.events_sent + eq->events_sent >= eq_info->event_limit)
                  break;

            } while (size > 0);

            /* send event to ODB */
            pevent = (EVENT_HEADER *)event_buffer;
            if (size > 0 && pevent->data_size && (eq_info->read_on & RO_ODB || eq_info->history)) {
               if (actual_millitime - eq->last_called > ODB_UPDATE_TIME && pevent != NULL) {
                  eq->last_called = actual_millitime;
                  update_odb(pevent, eq->hkey_variables, eq->format);
                  eq->odb_out++;
               }
            }
         }

         /*---- check if event limit is reached ----*/
         if (eq_info->eq_type != EQ_SLOW &&
             eq_info->event_limit > 0 &&
             eq->stats.events_sent + eq->events_sent >= eq_info->event_limit &&
             run_state == STATE_RUNNING) {
            /* stop run */
            if (cm_transition(TR_STOP, 0, str, sizeof(str), SYNC, FALSE) != CM_SUCCESS)
               cm_msg(MERROR, "scheduler", "cannot stop run: %s", str);

            /* check if autorestart, main loop will take care of it */
            size = sizeof(BOOL);
            flag = FALSE;
            db_get_value(hDB, 0, "/Logger/Auto restart", &flag, (INT *)&size, TID_BOOL, TRUE);

            if (flag)
               auto_restart = ss_time() + 20;   /* restart in 20 sec. */

            /* update event display correctly */
            force_update = TRUE;
         }
      }

      /*---- check for error messages periodically -------------------*/
      mfe_error_check();

      /*---- call frontend_loop periodically -------------------------*/
      if (frontend_call_loop) {
         status = frontend_loop();
         if (status == RPC_SHUTDOWN || status == SS_ABORT) {
            status = RPC_SHUTDOWN;
            break;
         }
      }

      /*---- check for deferred transitions --------------------------*/
      cm_check_deferred_transition();

      /*---- check for manual triggered events -----------------------*/
      if (manual_trigger_event_id) {
         old_flag = readout_enabled();
         if (old_flag && lockout_readout_thread)
            readout_enable(FALSE);

         /* readout and send event */
         status = BM_INVALID_PARAM;
         for (i = 0; equipment[i].name[0]; i++)
            if (equipment[i].info.event_id == manual_trigger_event_id) {
               status = send_event(i, TRUE);
               break;
            }

         manual_trigger_event_id = 0;

         if (status != CM_SUCCESS) {
            cm_msg(MERROR, "scheduler", "send_event error %d", status);
            goto net_error;
         }

         /* re-enable the interrupt after periodic */
         if (old_flag)
            readout_enable(TRUE);
      }

      /*---- calculate rates and update status page periodically -----*/
      if (force_update ||
          (display_period
           && actual_millitime - last_time_display > (DWORD) display_period)
          || (!display_period && actual_millitime - last_time_display > 3000)) {
         force_update = FALSE;

         for (i = 0; equipment[i].name[0]; i++) {
            eq = &equipment[i];
            eq->stats.events_sent += eq->events_sent;
            n_events[i] += eq->events_sent;
            eq->events_sent = 0;
         }

         /* calculate rates after requested period */
         if (actual_millitime - last_time_rate > (DWORD)get_rate_period()) {
            max_bytes_per_sec = 0;
            for (i = 0; equipment[i].name[0]; i++) {
               eq = &equipment[i];
               eq->stats.events_per_sec =
                   n_events[i] / ((actual_millitime - last_time_rate) / 1000.0);
               eq->stats.kbytes_per_sec =
                   eq->bytes_sent / 1024.0 / ((actual_millitime - last_time_rate) /
                                              1000.0);

               if ((INT) eq->bytes_sent > max_bytes_per_sec)
                  max_bytes_per_sec = eq->bytes_sent;

               eq->bytes_sent = 0;
               n_events[i] = 0;
            }

            max_bytes_per_sec = (DWORD)
                ((double) max_bytes_per_sec /
                 ((actual_millitime - last_time_rate) / 1000.0));

            last_time_rate = actual_millitime;
         }

         /* tcp buffer size evaluation */
         if (optimize) {
            opt_max = MAX(opt_max, (INT) max_bytes_per_sec);
            ss_printf(0, opt_index, "%6d : %5.1lf %5.1lf", opt_tcp_size,
                      opt_max / 1024.0, max_bytes_per_sec / 1024.0);
            if (++opt_cnt == 10) {
               opt_cnt = 0;
               opt_max = 0;
               opt_index++;
               opt_tcp_size = 1 << (opt_index + 7);
               rpc_set_opt_tcp_size(opt_tcp_size);
               if (1 << (opt_index + 7) > 0x8000) {
                  opt_index = 0;
                  opt_tcp_size = 1 << 7;
                  rpc_set_opt_tcp_size(opt_tcp_size);
               }
            }
         }

         /* propagate changes in equipment to ODB */
         rpc_set_option(-1, RPC_OTRANSPORT, RPC_FTCP);
         db_send_changed_records();
         rpc_set_option(-1, RPC_OTRANSPORT, RPC_TCP);

         if (display_period) {
            display(FALSE);

            /* check keyboard */
            ch = 0;
            status = 0;
            while (ss_kbhit()) {
               ch = ss_getchar(0);
               if (ch == -1)
                  ch = getchar();

               if (ch == '!')
                  status = RPC_SHUTDOWN;
            }

            if (ch > 0)
               display(TRUE);
            if (status == RPC_SHUTDOWN)
               break;
         }

         last_time_display = actual_millitime;
      }

      /*---- check to flush cache ------------------------------------*/
      if (actual_millitime - last_time_flush > 1000) {
         last_time_flush = actual_millitime;

         /* if cache on server is not filled in one second at current
            data rate, flush it now to make events available to consumers */

         if (max_bytes_per_sec < SERVER_CACHE_SIZE) {
            old_flag = readout_enabled();
            if (old_flag && lockout_readout_thread)
               readout_enable(FALSE);

            for (i = 0; equipment[i].name[0]; i++) {
               if (equipment[i].buffer_handle) {
                  /* check if buffer already flushed */
                  buffer_done = FALSE;
                  for (j = 0; j < i; j++)
                     if (equipment[i].buffer_handle == equipment[j].buffer_handle) {
                        buffer_done = TRUE;
                        break;
                     }

                  if (!buffer_done) {
                     rpc_set_option(-1, RPC_OTRANSPORT, RPC_FTCP);
                     rpc_flush_event();
                     err = bm_flush_cache(equipment[i].buffer_handle, ASYNC);
                     if ((err != BM_SUCCESS) && (err != BM_ASYNC_RETURN)) {
                        cm_msg(MERROR, "scheduler", "bm_flush_cache(ASYNC) error %d",
                               err);
                        return err;
                     }
                     rpc_set_option(-1, RPC_OTRANSPORT, RPC_TCP);
                  }
               }
            }

            if (old_flag)
               readout_enable(TRUE);
         }
      }

      /*---- check for auto restart --------------------------------*/
      if (auto_restart > 0 && ss_time() > auto_restart) {
         /* check if really stopped */
         size = sizeof(state);
         status = db_get_value(hDB, 0, "Runinfo/State", &state, (INT *)&size, TID_INT, TRUE);
         if (status != DB_SUCCESS)
            cm_msg(MERROR, "scheduler", "cannot get Runinfo/State in database");

         if (state == STATE_STOPPED) {
            auto_restart = 0;
            size = sizeof(run_number);
            status =
                  db_get_value(hDB, 0, "/Runinfo/Run number", &run_number, (INT*)&size, TID_INT,
                             TRUE);
            assert(status == SUCCESS);

            if (run_number <= 0) {
               cm_msg(MERROR, "main", "aborting on attempt to use invalid run number %d",
                      run_number);
               abort();
            }

            cm_msg(MTALK, "main", "starting new run");
            status = cm_transition(TR_START, run_number + 1, NULL, 0, SYNC, FALSE);
            if (status != CM_SUCCESS)
               cm_msg(MERROR, "main", "cannot restart run");
         }
      }

      /*---- check network messages ----------------------------------*/
      if ((run_state == STATE_RUNNING && interrupt_eq == NULL) || slowcont_eq) {
         /* only call yield once every 100ms when running */
         if (actual_millitime - last_time_network > 100) {
            status = cm_yield(0);
            last_time_network = actual_millitime;
         } else
            status = RPC_SUCCESS;
      } else
         /* when run is stopped or interrupts used, 
            call yield with 100ms timeout */
         status = cm_yield(100);

      /* exit for VxWorks */
      if (fe_stop)
         status = RPC_SHUTDOWN;

      /* exit if CTRL-C pressed */
      if (cm_is_ctrlc_pressed())
         status = RPC_SHUTDOWN;

   } while (status != RPC_SHUTDOWN && status != SS_ABORT);

 net_error:

   return status;
}

/*------------------------------------------------------------------*/

INT get_frontend_index()
{
   return frontend_index;
}

/*------------------------------------------------------------------*/

void (*mfe_error_dispatcher)(const char *) = NULL;

#define MFE_ERROR_SIZE 10
char mfe_error_str[MFE_ERROR_SIZE][256];
int mfe_error_r, mfe_error_w;
MUTEX_T *mfe_mutex = NULL;

void mfe_set_error(void (*dispatcher) (const char *))
{
   int status;

   mfe_error_dispatcher = dispatcher;
   mfe_error_r = mfe_error_w = 0;
   memset(mfe_error_str, 0, sizeof(mfe_error_str));

   if (mfe_mutex == NULL) {
      status = ss_mutex_create(&mfe_mutex);
      if (status != SS_SUCCESS && status != SS_CREATED)
         cm_msg(MERROR, "mfe_set_error", "Cannot create mutex\n");
   }

}

void mfe_error(const char *error)
/* central error dispatcher routine which can be called by any device
   or class driver */
{
   /* put error into FIFO */
   ss_mutex_wait_for(mfe_mutex, 1000);
   strlcpy(mfe_error_str[mfe_error_w], error, 256);
   mfe_error_w = (mfe_error_w + 1) % MFE_ERROR_SIZE;
   ss_mutex_release(mfe_mutex);
}

void mfe_error_check(void)
{
   if (mfe_mutex != NULL) {
      ss_mutex_wait_for(mfe_mutex, 1000);
      if (mfe_error_w != mfe_error_r) {
         if (mfe_error_dispatcher != NULL)
	    mfe_error_dispatcher(mfe_error_str[mfe_error_r]);
         mfe_error_r = (mfe_error_r + 1) % MFE_ERROR_SIZE;
      }
      ss_mutex_release(mfe_mutex);
   }
}

/*------------------------------------------------------------------*/

#ifdef OS_VXWORKS
int mfe(char *ahost_name, char *aexp_name, BOOL adebug)
#else
int main(int argc, char *argv[])
#endif
{
   INT status, i, j, size;
   INT daemon_flag;
   int sys_max_event_size = MAX_EVENT_SIZE;

   host_name[0] = 0;
   exp_name[0] = 0;
   debug = FALSE;
   daemon_flag = 0;

   setbuf(stdout, 0);
   setbuf(stderr, 0);

#ifdef SIGPIPE
   signal(SIGPIPE, SIG_IGN);
#endif

#ifdef OS_VXWORKS
   if (ahost_name)
      strcpy(host_name, ahost_name);
   if (aexp_name)
      strcpy(exp_name, aexp_name);
   debug = adebug;
#else

   /* get default from environment */
   cm_get_environment(host_name, sizeof(host_name), exp_name, sizeof(exp_name));

   /* parse command line parameters */
   for (i = 1; i < argc; i++) {
      if (argv[i][0] == '-' && argv[i][1] == 'd')
         debug = TRUE;
      else if (argv[i][0] == '-' && argv[i][1] == 'D')
         daemon_flag = 1;
      else if (argv[i][0] == '-' && argv[i][1] == 'O')
         daemon_flag = 2;
      else if (argv[i][0] == '-') {
         if (i + 1 >= argc || argv[i + 1][0] == '-')
            goto usage;
         if (argv[i][1] == 'e')
            strcpy(exp_name, argv[++i]);
         else if (argv[i][1] == 'h')
            strcpy(host_name, argv[++i]);
         else if (argv[i][1] == 'i')
            frontend_index = atoi(argv[++i]);
         else {
          usage:
            printf
                ("usage: frontend [-h Hostname] [-e Experiment] [-d] [-D] [-O] [-i n]\n");
            printf("         [-d]     Used to debug the frontend\n");
            printf("         [-D]     Become a daemon\n");
            printf("         [-O]     Become a daemon but keep stdout\n");
            printf("         [-i n]   Set frontend index (used for event building)\n");
            return 0;
         }
      }
   }
#endif

   /* check event and buffer sizes */
   if (event_buffer_size < 2 * max_event_size) {
      cm_msg(MERROR, "mainFE", "event_buffer_size %d too small for max. event size %d\n", event_buffer_size, max_event_size);
      ss_sleep(5000);
      return 1;
   }

#ifdef OS_VXWORKS
   /* override event_buffer_size in case of VxWorks
      take remaining free memory and use 20% of it for rb_ */
   event_buffer_size = 2 * 10 * (max_event_size + sizeof(EVENT_HEADER) + sizeof(INT));
   if (event_buffer_size > memFindMax()) {
      cm_msg(MERROR, "mainFE", "Not enough mem space for event size");
      return 0;
   }
   /* takes overall 20% of the available memory resource for rb_() */
   event_buffer_size = 0.2 * memFindMax();
#endif

   /* retrieve frontend index from environment if defined */
   if (getenv("MIDAS_FRONTEND_INDEX"))
      frontend_index = atoi(getenv("MIDAS_FRONTEND_INDEX"));

   /* add frontend index to frontend name if present */
   strcpy(full_frontend_name, frontend_name);
   if (frontend_index >= 0)
      sprintf(full_frontend_name + strlen(full_frontend_name), "%02d", frontend_index);

   /* inform user of settings */
   printf("Frontend name          :     %s\n", full_frontend_name);
   printf("Event buffer size      :     %d\n", event_buffer_size);
   printf("User max event size    :     %d\n", max_event_size);
   if (max_event_size_frag > 0)
      printf("User max frag. size    :     %d\n", max_event_size_frag);
   printf("# of events per buffer :     %d\n\n", event_buffer_size / max_event_size);

   if (daemon_flag) {
      printf("\nBecoming a daemon...\n");
      ss_daemon_init(daemon_flag == 2);
   }
  
   /* set default rate period */
   set_rate_period(3000);

   /* now connect to server */
   if (display_period) {
      if (host_name[0]) {
         if (exp_name[0])
            printf("Connect to experiment %s on host %s...\n", exp_name, host_name);
         else
            printf("Connect to experiment on host %s...\n", host_name);
      } else if (exp_name[0])
         printf("Connect to experiment %s...\n", exp_name);
      else
         printf("Connect to experiment...\n");
   }

   status = cm_connect_experiment1(host_name, exp_name, full_frontend_name,
                                   NULL, DEFAULT_ODB_SIZE, DEFAULT_FE_TIMEOUT);
   if (status != CM_SUCCESS) {
      cm_msg(MERROR, "mainFE", "Cannot connect to experiment \'%s\' on host \'%s\', status %d", exp_name, host_name, status);
      /* let user read message before window might close */
      ss_sleep(5000);
      return 1;
   }

   if (display_period)
      printf("OK\n");

   /* allocate buffer space */
   event_buffer = malloc(max_event_size);
   if (event_buffer == NULL) {
      cm_msg(MERROR, "mainFE", "mfe: Cannot allocate event buffer of max_event_size %d\n", max_event_size);
      return 1;
   }

   /* remomve any dead frontend */
   cm_cleanup(full_frontend_name, FALSE);

   /* shutdown previous frontend */
   status = cm_shutdown(full_frontend_name, FALSE);
   if (status == CM_SUCCESS && display_period) {
      printf("Previous frontend stopped\n");

      /* let user read message */
      ss_sleep(3000);
   }

   /* register transition callbacks */
   if (cm_register_transition(TR_START, tr_start, 500) != CM_SUCCESS ||
       cm_register_transition(TR_STOP, tr_stop, 500) != CM_SUCCESS ||
       cm_register_transition(TR_PAUSE, tr_pause, 500) != CM_SUCCESS ||
       cm_register_transition(TR_RESUME, tr_resume, 500) != CM_SUCCESS) {
      printf("Failed to start local RPC server");
      cm_disconnect_experiment();

      /* let user read message before window might close */
      ss_sleep(5000);
      return 1;
   }

   cm_get_experiment_database(&hDB, &status);
   /* set time from server */
#ifdef OS_VXWORKS
   cm_synchronize(NULL);
#endif

   size = sizeof(sys_max_event_size);
   status = db_get_value(hDB, 0, "/Experiment/MAX_EVENT_SIZE", &sys_max_event_size, &size, TID_DWORD, TRUE);

   if (max_event_size > sys_max_event_size) {
      cm_msg(MERROR, "mainFE", "Requested max_event_size (%d) exceeds max. system event size (%d)",
             max_event_size, sys_max_event_size);
      return 1;
   }

   /* turn off watchdog if in debug mode */
   if (debug)
      cm_set_watchdog_params(FALSE, 0);

   /* increase RPC timeout to 2min for logger with exabyte or blocked disk */
   rpc_set_option(-1, RPC_OTIMEOUT, 120000);

   /* set own message print function */
   if (display_period)
      cm_set_msg_print(MT_ALL, MT_ALL, message_print);

   /* reqister equipment in ODB */
   if (register_equipment() != SUCCESS) {
      if (display_period)
         printf("\n");
      cm_disconnect_experiment();

      /* let user read message before window might close */
      ss_sleep(5000);
      return 1;
   }

   /* call user init function */
   if (display_period)
      printf("Init hardware...\n");
   if (frontend_init() != SUCCESS) {
      if (display_period)
         printf("\n");
      cm_disconnect_experiment();

      /* let user read message before window might close */
      ss_sleep(5000);
      return 1;
   }

   initialize_equipment();

   if (display_period)
      printf("OK\n");

   /* initialize screen display */
   if (display_period) {
      ss_sleep(300);
      display(TRUE);
   }

   /* switch on interrupts or readout thread if running */
   if (run_state == STATE_RUNNING)
      readout_enable(TRUE);

   /* initialize ss_getchar */
   ss_getchar(0);

   /* call main scheduler loop */
   status = scheduler();

   /* stop readout thread */
   stop_all_threads = 1;
   rb_set_nonblocking();
   while (readout_thread_active)
      ss_sleep(100);

   /* reset terminal */
   ss_getchar(TRUE);

   /* switch off interrupts and detach */
   if (interrupt_eq) {
      interrupt_configure(CMD_INTERRUPT_DISABLE, 0, 0);
      interrupt_configure(CMD_INTERRUPT_DETACH, 0, 0);
   }
   
   /* call user exit function */
   frontend_exit();

   /* close slow control drivers */
   for (i = 0; equipment[i].name[0]; i++)
      if ((equipment[i].info.eq_type & EQ_SLOW) && equipment[i].status == FE_SUCCESS) {

         for (j = 0; equipment[i].driver[j].name[0]; j++)
            if (equipment[i].driver[j].flags & DF_MULTITHREAD)
               break;

         /* stop all threads if multithreaded */
         if (equipment[i].driver[j].name[0] && equipment[i].status == FE_SUCCESS)
            equipment[i].cd(CMD_STOP, &equipment[i]);   /* stop all threads */
      }
   
   for (i = 0; equipment[i].name[0]; i++)
      if ((equipment[i].info.eq_type & EQ_SLOW) && equipment[i].status == FE_SUCCESS)
         equipment[i].cd(CMD_EXIT, &equipment[i]);      /* close physical connections */

   free(n_events);

   /* close network connection to server */
   cm_disconnect_experiment();

   if (display_period) {
      if (status == RPC_SHUTDOWN) {
         ss_clear_screen();
         ss_printf(0, 0, "Frontend shut down.");
         ss_printf(0, 1, "");
      }
   }

   if (status != RPC_SHUTDOWN)
      printf("Network connection aborted.\n");

   return 0;
}

#ifdef LINK_TEST
char* frontend_name;
char* frontend_file_name;
BOOL  frontend_call_loop;
int event_buffer_size;
int max_event_size;
int max_event_size_frag;
int display_period;
EQUIPMENT equipment[1];
int frontend_init() { return 0; };
int frontend_exit() { return 0; };
int begin_of_run(int runno,char* errstr) { return 0; };
int end_of_run(int runno,char* errstr) { return 0; };
int pause_run(int runno,char* errstr) { return 0; };
int resume_run(int runno,char* errstr) { return 0; };
int interrupt_configure(INT cmd, INT source, POINTER_T adr) { return 0; };
int frontend_loop() { return 0; };
int poll_event(INT source, INT count, BOOL test) { return 0; };
#endif
