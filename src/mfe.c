/********************************************************************\

  Name:         mfe.c
  Created by:   Stefan Ritt

  Contents:     The system part of the MIDAS frontend. Has to be
                linked with user code to form a complete frontend

  $Log$
  Revision 1.6  1999/09/27 13:49:04  midas
  Added bUnique parameter to cm_shutdown

  Revision 1.5  1999/09/23 12:45:48  midas
  Added 32 bit banks

  Revision 1.4  1999/06/23 09:38:51  midas
  - Added D8_BKTYPE
  - Fixed CAMAC server F24 bug
  - cm_synchronize only called for VxWorks

  Revision 1.3  1998/12/10 12:50:47  midas
  Program abort with "!" now works without a return under UNIX

  Revision 1.2  1998/10/12 12:19:01  midas
  Added Log tag in header


\********************************************************************/

#include <stdio.h>
#include "midas.h"
#include "msystem.h"
#include "mcstd.h"

#ifdef YBOS_SUPPORT
#include "ybos.h"
#endif

/*------------------------------------------------------------------*/

/* items defined in frontend.c */

extern char *frontend_name;
extern char *frontend_file_name;
extern BOOL frontend_call_loop;
extern INT  event_buffer_size;
extern INT  display_period;
extern INT  frontend_init(void);
extern INT  frontend_exit(void);
extern INT  frontend_loop(void);
extern INT  begin_of_run(INT run_number, char *error);
extern INT  end_of_run(INT run_number, char *error);
extern INT  pause_run(INT run_number, char *error);
extern INT  resume_run(INT run_number, char *error);
extern INT  poll_event(INT source, INT count, BOOL test);
extern INT  interrupt_configure(INT cmd, INT source, PTYPE adr);

/*------------------------------------------------------------------*/

/* globals */

#undef USE_EVENT_CHANNEL

#define SERVER_CACHE_SIZE 100000 /* event cache before buffer */

#define ODB_UPDATE_TIME    10000 /* 10 seconds for ODB update */

INT   run_state;       /* STATE_RUNNING, STATE_STOPPED, STATE_PAUSED */
INT   run_number;
DWORD actual_time;      /* current time in seconds since 1970 */
DWORD actual_millitime;       /* current time in milliseconds */

char  host_name[NAME_LENGTH];
char  exp_name[NAME_LENGTH];

INT   max_bytes_per_sec;
INT   optimize = 0;  /* set this to one to opimize TCP buffer size */
INT   fe_stop = 0;   /* stop switch for VxWorks */
BOOL  debug;         /* disable watchdog messages from server */

HNDLE hDB;

#ifdef YBOS_SUPPORT
struct {
  DWORD ybos_type;
  DWORD odb_type;
  INT tsize;
} id_map[] = {
  {A1_BKTYPE, TID_CHAR, 1},
  {I1_BKTYPE, TID_BYTE, 1},
  {I2_BKTYPE, TID_WORD, 2},
  {I4_BKTYPE, TID_DWORD, 4},
  {F4_BKTYPE, TID_FLOAT, 4},
  {D8_BKTYPE, TID_DOUBLE, 8},
  {0,0}
};
#endif

extern EQUIPMENT equipment[];

EQUIPMENT     *interrupt_eq = NULL;
EVENT_HEADER  *interrupt_odb_buffer;
BOOL          interrupt_odb_buffer_valid;

void send_all_periodic_events(INT transition);
void interrupt_routine(void);
void interrupt_enable(BOOL flag);

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
Event limit = DWORD : 0\n\
Log history = INT : 0\n\
Frontend host = STRING : [32] \n\
Frontend name = STRING : [32] \n\
Frontend file name = STRING : [256] \n\
"

#define EQUIPMENT_STATISTICS_STR "\
Events sent = DWORD : 0\n\
Events per sec. = DWORD : 0\n\
kBytes per sec. = DOUBLE : 0\n\
"

/*-- transition callbacks ------------------------------------------*/

/*-- start ---------------------------------------------------------*/

INT tr_start(INT rn, char *error)
{
INT i, status;

  /* reset serial numbers */
  for (i=0 ; equipment[i].name[0] ; i++)
    {
    equipment[i].serial_number = 1;
    equipment[i].odb_in = equipment[i].odb_out = 0;
    }

  status = begin_of_run(rn, error);

  if (status == CM_SUCCESS)
    {
    run_state = STATE_RUNNING;
    run_number = rn;
    }

  send_all_periodic_events(TR_START);

  if (display_period)
    {
    ss_printf(14, 2, "Running ");
    ss_printf(36, 2, "%d", rn);
    }

  /* enable interrupts */
  interrupt_enable(TRUE);
    
  return status;
}

/*-- prestop -------------------------------------------------------*/

INT tr_prestop(INT rn, char *error)
{
INT status;

  /* disable interrupts */
  interrupt_enable(FALSE);

  status = end_of_run(rn, error);

  /* don't send events if already stopped */
  if (run_state != STATE_STOPPED)
    send_all_periodic_events(TR_STOP);

  if (status == CM_SUCCESS)
    {
    run_state = STATE_STOPPED;
    run_number = rn;
    }

  if (display_period)
    ss_printf(14, 2, "Stopped ");

  return status;
}

/*-- pause ---------------------------------------------------------*/

INT tr_prepause(INT rn, char *error)
{
INT status;

  /* disable interrupts */
  interrupt_enable(FALSE);

  status = pause_run(rn, error);

  if (status == CM_SUCCESS)
    {
    run_state = STATE_PAUSED;
    run_number = rn;
    }

  send_all_periodic_events(TR_PAUSE);

  if (display_period)
    ss_printf(14, 2, "Paused  ");

  return status;
}

/*-- resume --------------------------------------------------------*/

INT tr_resume(INT rn, char *error)
{
INT status;

  status = resume_run(rn, error);

  if (status == CM_SUCCESS)
    {
    run_state = STATE_RUNNING;
    run_number = rn;
    }

  send_all_periodic_events(TR_RESUME);

  if (display_period)
    ss_printf(14, 2, "Running ");

  /* enable interrupts */
  interrupt_enable(TRUE);

  return status;
}

/*------------------------------------------------------------------*/

INT register_equipment(void)
{
INT    index, count, size, status, i, j, k, n;
char   str[256];
EQUIPMENT_INFO *eq_info;
EQUIPMENT_STATS *eq_stats;
DWORD  start_time, delta_time;
HNDLE  hKey;

  /* get current ODB run state */
  size = sizeof(run_state);
  run_state = STATE_STOPPED;
  db_get_value(hDB, 0, "/Runinfo/State", &run_state, &size, TID_INT);
  size = sizeof(run_number);
  run_number = 1;
  db_get_value(hDB, 0, "/Runinfo/Run number", &run_number, &size, TID_INT);

  /* scan EQUIPMENT table from FRONTEND.C */
  for (index=0 ; equipment[index].name[0] ; index++)
    {
    eq_info = &equipment[index].info;
    eq_stats = &equipment[index].stats;

    if (eq_info->event_id == 0)
      {
      printf("Event ID 0 for %s not allowed\n", equipment[index].name);
      ss_sleep(5000);
      }

    /* init status */
    equipment[index].status = FE_SUCCESS;

    sprintf(str, "/Equipment/%s/Common", equipment[index].name);
    
    /* get last event limit from ODB */
    db_find_key(hDB, 0, str, &hKey);
    if (hKey)
      db_get_value(hDB, hKey, "Event limit", &eq_info->event_limit, &size, TID_DWORD);
    
    /* Create common subtree */
    status = db_create_record(hDB, 0, str, EQUIPMENT_COMMON_STR);
    if (status != DB_SUCCESS)
      {
      printf("Cannot init equipment record, probably other FE is using it\n");
      ss_sleep(3000);
      }
    db_find_key(hDB, 0, str, &hKey);

    if (equal_ustring(eq_info->format, "YBOS"))
      equipment[index].format = FORMAT_YBOS;
    else if (equal_ustring(eq_info->format, "FIXED"))
      equipment[index].format = FORMAT_FIXED;
    else /* default format is MIDAS */
      equipment[index].format = FORMAT_MIDAS;

    gethostname(eq_info->frontend_host, sizeof(eq_info->frontend_host));
    strcpy(eq_info->frontend_name, frontend_name);
    strcpy(eq_info->frontend_file_name, frontend_file_name);

    /* set record from equipment[] table in frontend.c */
    db_set_record(hDB, hKey, eq_info, sizeof(EQUIPMENT_INFO), 0);

    /* open hot link to equipment info */
    db_open_record(hDB, hKey, eq_info, sizeof(EQUIPMENT_INFO), MODE_READ, NULL, NULL);

    /*---- Create variables record ---------------------------------*/
    sprintf(str, "/Equipment/%s/Variables", equipment[index].name);
    if (equipment[index].init_string)
      db_create_record(hDB, 0, str, equipment[index].init_string);
    else
      db_create_key(hDB, 0, str, TID_KEY);
    db_find_key(hDB, 0, str, &hKey);
    equipment[index].hkey_variables = hKey;

    /*---- Create and initialize statistics tree -------------------*/
    sprintf(str, "/Equipment/%s/Statistics", equipment[index].name);
    db_create_record(hDB, 0, str, EQUIPMENT_STATISTICS_STR);
    db_find_key(hDB, 0, str, &hKey);

    eq_stats->events_sent = 0;
    eq_stats->events_per_sec = 0;
    eq_stats->kbytes_per_sec = 0;

    /* open hot link to statistics tree */
    status = db_open_record(hDB, hKey, eq_stats, sizeof(EQUIPMENT_STATS), MODE_WRITE, NULL, NULL);
    if (status != DB_SUCCESS)
      {
      printf("Cannot open statistics record, probably other FE is using it\n");
      ss_sleep(3000);
      }

    /*---- open event buffer ---------------------------------------*/
    if (eq_info->buffer[0])
      {
      status = bm_open_buffer(eq_info->buffer, EVENT_BUFFER_SIZE, &equipment[index].buffer_handle);
      if (status != BM_SUCCESS && status != BM_CREATED)
        {
        cm_msg(MERROR, "register_equipment", 
"Cannot open event buffer. Try to reduce EVENT_BUFFER_SIZE in midas.h \
and rebuild the system.");
        return 0;         
        }

      /* set the default buffer cache size */
      bm_set_cache_size(equipment[index].buffer_handle, 0, SERVER_CACHE_SIZE);
      }
    else
      equipment[index].buffer_handle = 0;

    /*---- evaluate polling count ----------------------------------*/
    if (eq_info->eq_type == EQ_POLLED)
      {
      if (display_period)
        printf("\nCalibrating");

      count = 1;
      do
        {
        if (display_period)
          printf(".");

        start_time = ss_millitime();

        poll_event(equipment[index].info.source, count, TRUE);

        delta_time = ss_millitime() - start_time;

        if (delta_time > 0)
          count = (INT) ((double) count * 100 / delta_time);
        else
          count*=100;
        } while (delta_time > 120 || delta_time < 80);

      equipment[index].poll_count = (INT) ((double) eq_info->period / 100 * count);

      if (display_period)
        printf("OK\n");
      }

    /*---- initialize interrupt events -----------------------------*/
    if (eq_info->eq_type == EQ_INTERRUPT)
      {
      /* install interrupt for interrupt events */

      for (i=0 ; equipment[i].name[0] ; i++)
        if (equipment[i].info.eq_type == EQ_POLLED)
          {
          equipment[index].status = FE_ERR_DISABLED;
          cm_msg(MINFO, "register_equipment", 
            "Interrupt readout cannot be combined with polled readout");
          }

      if (equipment[index].status != FE_ERR_DISABLED)
        {
        if (eq_info->enabled)
          {
          if (interrupt_eq)
            {
            equipment[index].status = FE_ERR_DISABLED;
            cm_msg(MINFO, "register_equipment", 
              "Defined more than one equipment with interrupt readout");
            }
          else
            {
            interrupt_configure(CMD_INTERRUPT_ATTACH, eq_info->source, (PTYPE) interrupt_routine);
            interrupt_eq = &equipment[index];
            interrupt_odb_buffer = malloc(MAX_EVENT_SIZE+sizeof(EVENT_HEADER));
            }
          }
        else
          {
          equipment[index].status = FE_ERR_DISABLED;
          cm_msg(MINFO, "register_equipment", "Equipment %s disabled in file \"frontend.c\"",
                         equipment[index].name);
          }
        }
      }
    
    /*---- initialize slow control equipment -----------------------*/
    if (eq_info->eq_type == EQ_SLOW)
      {
      /* resolve duplicate device names */
      for (i=0 ; equipment[index].driver[i].name[0] ; i++)
        for (j=i+1 ; equipment[index].driver[j].name[0] ; j++)
          if (equal_ustring(equipment[index].driver[i].name,
                            equipment[index].driver[j].name))
            {
            strcpy(str, equipment[index].driver[i].name);
            for (k=0,n=0 ; equipment[index].driver[k].name[0] ; k++)
              if (equal_ustring(str, equipment[index].driver[k].name))
                sprintf(equipment[index].driver[k].name, "%s_%d", str, n++);
            
            break;
            }
  
      /* loop over equipment list and call class driver's init method */
      if (eq_info->enabled)
        equipment[index].status = equipment[index].cd(CMD_INIT, &equipment[index]);
      else
        {
        equipment[index].status = FE_ERR_DISABLED;
        cm_msg(MINFO, "register_equipment", "Equipment %s disabled in file \"frontend.c\"",
                       equipment[index].name);
        }

      /* let user read error messages */
      if (equipment[index].status != FE_SUCCESS)
        ss_sleep(3000);
      }
    }

  return SUCCESS;
}

/*------------------------------------------------------------------*/

void update_odb(EVENT_HEADER *pevent, HNDLE hKey, INT format)
{
INT               size, i, n, ni4, tsize, status;
void              *pdata;
char              name[5];
BANK_HEADER       *pbh;
BANK              *pbk;
BANK32            *pbk32;
void              *pydata;
DWORD             odb_type;
DWORD             *pyevt, bkname;
WORD              bktype;

  rpc_set_option(-1, RPC_OTRANSPORT, RPC_FTCP);

  if (format == FORMAT_FIXED)
    {
    if (db_set_record(hDB, hKey, (char *) (pevent+1),
                      pevent->data_size, 0) != DB_SUCCESS)
      cm_msg(MERROR, "update_odb", "event #%d size mismatch", pevent->event_id);
    }
  else if (format == FORMAT_MIDAS)
    {
    pbh = (BANK_HEADER *) (pevent+1);
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
      
      /* write bank to ODB */
      *((DWORD *) name) = bkname;
      name[4] = 0;

      n = rpc_tid_size(bktype & 0xFF) ? size / rpc_tid_size(bktype & 0xFF) : size;

      if (n>0)
        db_set_value(hDB, hKey, name, pdata, size, n, bktype & 0xFF);

      } while (1);
    }
  else if (format == FORMAT_YBOS)
    {
#ifdef YBOS_SUPPORT
    YBOS_BANK_HEADER *pybkh;

    /* skip the lrl (4 bytes per event) */
    pyevt = (DWORD *) (pevent+1);
    pybkh = NULL;
    do
	    {
	    /* scan all banks */
	    ni4 = ybk_iterate(pyevt, &pybkh, &pydata);
	    if (pybkh == NULL || ni4 == 0)
	      break;

	    /* find the corresponding odb type */
	    tsize = odb_type = 0;
      for (i=0;id_map[0].ybos_type > 0; i++)
	      {
        if (pybkh->type == id_map[i].ybos_type)
          {
		      odb_type = id_map[i].odb_type;
		      tsize = id_map[i].tsize;
 		      break;
          }
	      }

	    /* extract bank name (key name) */
	    *((DWORD *)name) = pybkh->name;
	    name[4] = 0;

      /* correct YBS number of entries */
      if (pybkh->type == D8_BKTYPE)
        ni4 /= 2;
      if (pybkh->type == I2_BKTYPE)
        ni4 *= 2;
      if (pybkh->type == I1_BKTYPE || pybkh->type == A1_BKTYPE)
        ni4 *= 4;

	    /* write bank to ODB, ni4 always in I*4 */
	    size = ni4 * tsize;
	    if ((status = db_set_value(hDB, hKey, name, pydata, size, ni4, odb_type & 0xFF)) != DB_SUCCESS)
	      {
	      printf("status:%i odb_type:%li name:%s ni4:%i size:%i tsize:%i\n",
		            status, odb_type, name, ni4, size, tsize);
	      for (i=0;i<6;i++)
		      printf("data: %f\n",*((float *)(pydata))++);
	      }
      } while (1);
#endif /* YBOS_SUPPORT */
    }

  rpc_set_option(-1, RPC_OTRANSPORT, RPC_TCP);
}

/*------------------------------------------------------------------*/

void send_all_periodic_events(INT transition)
{
EQUIPMENT_INFO *eq_info;
EVENT_HEADER   *pevent;
INT            i;

  for (i=0 ; equipment[i].name[0] ; i++)
    {
    eq_info = &equipment[i].info;

    if ((eq_info->eq_type != EQ_PERIODIC && eq_info->eq_type != EQ_SLOW) || 
        !eq_info->enabled || equipment[i].status != FE_SUCCESS)
      continue;

    if (transition == TR_START  && (eq_info->read_on & RO_BOR)    == 0)
      continue;
    if (transition == TR_STOP   && (eq_info->read_on & RO_EOR)    == 0)
      continue;
    if (transition == TR_PAUSE  && (eq_info->read_on & RO_PAUSE)  == 0)
      continue;
    if (transition == TR_RESUME && (eq_info->read_on & RO_RESUME) == 0)
      continue;

    pevent = eb_get_pointer();

    pevent->event_id      = eq_info->event_id;
    pevent->trigger_mask  = eq_info->trigger_mask;
    pevent->data_size     = 0;
    pevent->time_stamp    = ss_time();
    pevent->serial_number = equipment[i].serial_number++;

    equipment[i].last_called = ss_millitime();

    /* call user readout routine */
    *((EQUIPMENT **)(pevent+1)) = &equipment[i];
    pevent->data_size = equipment[i].readout((char *) (pevent+1));

    /* send event */
    if (pevent->data_size)
      {
      equipment[i].bytes_sent += pevent->data_size + sizeof(EVENT_HEADER);
      equipment[i].events_sent++;

      /* send event to buffer */
      if (equipment[i].buffer_handle)
        {
#ifdef USE_EVENT_CHANNEL
        eb_increment_pointer(equipment[i].buffer_handle, 
                             pevent->data_size + sizeof(EVENT_HEADER));
#else
        rpc_flush_event();
        bm_send_event(equipment[i].buffer_handle, pevent,
                       pevent->data_size + sizeof(EVENT_HEADER), SYNC);
        bm_flush_cache(equipment[i].buffer_handle, SYNC);
#endif
        }

      /* send event to ODB */
      if (eq_info->read_on & RO_ODB ||
          eq_info->history > 0)
        {
        update_odb(pevent, equipment[i].hkey_variables, equipment[i].format);
        equipment[i].odb_out++;
        }
      }
    else
      equipment[i].serial_number--;
    }

  /* emtpy event buffer */
  while (eb_send_events(TRUE) == BM_MORE_EVENTS);
}

/*------------------------------------------------------------------*/

BOOL interrupt_enabled, interrupt_blocked;

void interrupt_enable(BOOL flag)
{
  interrupt_enabled = flag;

  if (interrupt_eq)
    {
    if (interrupt_enabled && !interrupt_blocked)
      interrupt_configure(CMD_INTERRUPT_ENABLE,0,0);
    else
      interrupt_configure(CMD_INTERRUPT_DISABLE,0,0);
    }
}

void interrupt_block(BOOL flag)
{
  if (!flag && eb_buffer_full())
    {
    cm_msg(MERROR, "interrupt_block",
	   "Tried to enable interrupts when buffer is full.");
    return;
    }

  if (interrupt_blocked && flag)
    return;
  if (!interrupt_blocked && !flag)
    return;

  interrupt_blocked = flag;

  if (interrupt_eq)
    {
    if (interrupt_enabled && !interrupt_blocked)
      interrupt_configure(CMD_INTERRUPT_ENABLE,0,0);
    else
      interrupt_configure(CMD_INTERRUPT_DISABLE,0,0);
    }
}

/* Should be volatile - as it may be modified by an interrupt handler */
volatile INT daqPendingInterrupt = 0;

void interrupt_routine(void)
{
EVENT_HEADER *pevent;

  pevent = eb_get_pointer();
  if (pevent == NULL) 
    {
#ifdef OS_VXWORKS
    logMsg("DAQ hang: Buffer is unexpectedly %d%% full\n",
	   eb_get_level(),0,0,0,0,0);
#endif
    daqPendingInterrupt = 1;
    return;
    }
  
  /* compose MIDAS event header */
  pevent->event_id      = interrupt_eq->info.event_id;
  pevent->trigger_mask  = interrupt_eq->info.trigger_mask;
  pevent->data_size     = 0;
  pevent->time_stamp    = actual_time;
  pevent->serial_number = interrupt_eq->serial_number++;

  /* call user readout routine */
  pevent->data_size = interrupt_eq->readout((char *) (pevent+1));

  /* send event */
  if (pevent->data_size)
    {
    interrupt_eq->bytes_sent += pevent->data_size + sizeof(EVENT_HEADER);
    interrupt_eq->events_sent++;

    if (interrupt_eq->buffer_handle)
      {
#ifdef USE_EVENT_CHANNEL
      eb_increment_pointer(interrupt_eq->buffer_handle, 
                           pevent->data_size + sizeof(EVENT_HEADER));
#else
      rpc_send_event(interrupt_eq->buffer_handle, pevent,
                     pevent->data_size + sizeof(EVENT_HEADER), SYNC);
#endif
      }

    /* send event to ODB */
    if (interrupt_eq->info.read_on & RO_ODB ||
        interrupt_eq->info.history)
      {
      if (actual_millitime - interrupt_eq->last_called > ODB_UPDATE_TIME)
        {
        interrupt_eq->last_called = actual_millitime;
        memcpy(interrupt_odb_buffer, pevent, pevent->data_size + sizeof(EVENT_HEADER));
        interrupt_odb_buffer_valid = TRUE;
        interrupt_eq->odb_out++;
        }
      }
    }
  else
    interrupt_eq->serial_number--;

  if (interrupt_eq && eb_buffer_full())
    interrupt_block(TRUE);
}

/*------------------------------------------------------------------*/

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

void display(BOOL bInit)
{
INT    i, status;
time_t full_time;
char   str[30];

  if (bInit)
    {
    ss_clear_screen();

    if (host_name[0])
      strcpy(str, host_name);
    else
      strcpy(str, "<local>");

    ss_printf(0, 0, "%s connected to %s. Press \"!\" to exit", frontend_name, str);
    ss_printf(0, 1, "================================================================================");
    ss_printf(0, 2, "Run status:   %s", run_state == STATE_STOPPED ? "Stopped" :
                                        run_state == STATE_RUNNING ? "Running" :
                                        "Paused");
    ss_printf(25,2, "Run number %ld   ", run_number);
    ss_printf(0, 3, "================================================================================");
    ss_printf(0, 4, "Equipment     Status     Events     Events/sec Rate[kB/s] ODB->FE    FE->ODB");
    ss_printf(0, 5, "--------------------------------------------------------------------------------");
    for (i=0 ; equipment[i].name[0] ; i++)
      ss_printf(0, i+6, "%s", equipment[i].name);
    }

  /* display time */
  time(&full_time);
  strcpy(str, ctime(&full_time)+11);
  str[8] = 0;
  ss_printf(72, 0, "%s", str);

  for (i=0 ; equipment[i].name[0] ; i++)
    {
    status = equipment[i].status;

    if ((status == 0 || status == FE_SUCCESS) &&
        equipment[i].info.enabled)
      ss_printf(14, i+6, "OK       ");
    else if (!equipment[i].info.enabled)
      ss_printf(14, i+6, "Disabled ");
    else if (status == FE_ERR_ODB)
      ss_printf(14, i+6, "ODB Error");
    else if (status == FE_ERR_HW)
      ss_printf(14, i+6, "HW Error ");
    else if (status == FE_ERR_DISABLED)
      ss_printf(14, i+6, "Disabled ");
    else
      ss_printf(14, i+6, "Unknown  ");

    ss_printf(25, i+6, "%ld       ", equipment[i].stats.events_sent);
    ss_printf(36, i+6, "%ld       ", equipment[i].stats.events_per_sec);
    ss_printf(47, i+6, "%1.1lf      ", equipment[i].stats.kbytes_per_sec);
    ss_printf(58, i+6, "%ld       ", equipment[i].odb_in);
    ss_printf(69, i+6, "%ld       ", equipment[i].odb_out);
    }

  /* go to next line */
  ss_printf(0, i+6, "");
}

/*------------------------------------------------------------------*/

INT scheduler(void)
{
EQUIPMENT_INFO *eq_info;
EQUIPMENT      *eq;
EVENT_HEADER   *pevent;
DWORD          last_time_network=0, last_time_display=0,
               last_time_flush=0, readout_start;
INT            i, j, index, status, ch, source;
char           str[80];
BOOL           buffer_done;


INT opt_max=0, opt_index=0, opt_tcp_size=128, opt_cnt=0;


#ifdef OS_VXWORKS
  rpc_set_opt_tcp_size(1024);
#endif

  pevent = eb_get_pointer();

  do
    {
    actual_millitime = ss_millitime();
    actual_time = ss_time();

    /*---- loop over equipment table -------------------------------*/
    for (index=0 ; ; index++)
      {
      eq = &equipment[index];
      eq_info = &eq->info;

      /* check if end of equipment list */
      if (!eq->name[0])
        break;

      if (!eq_info->enabled)
        continue;

      if (equipment[index].status != FE_SUCCESS)
        continue;

      /*---- call idle routine for slow control equipment ----*/
      if (eq_info->eq_type == EQ_SLOW &&
          equipment[index].status == FE_SUCCESS)
        equipment[index].cd(CMD_IDLE, &equipment[index]);

      if (run_state == STATE_STOPPED && (eq_info->read_on & RO_STOPPED) == 0)
        continue;
      if (run_state == STATE_PAUSED  && (eq_info->read_on & RO_PAUSED)  == 0)
        continue;
      if (run_state == STATE_RUNNING && (eq_info->read_on & RO_RUNNING) == 0)
        continue;

      /*---- check periodic events ----*/
      if (eq_info->eq_type == EQ_PERIODIC || eq_info->eq_type == EQ_SLOW)
        {
        if (eq_info->period == 0)
          continue;

        /* if period over, call readout routine */
        if (actual_millitime - eq->last_called >= (DWORD) eq_info->period)
          {
          /* disable interrupts during readout */
          interrupt_block(TRUE);

          do
            {
            pevent = eb_get_pointer();
            
            /* if no space in buffer, empty it */
            if (pevent == NULL)
              {
              status = eb_send_events(FALSE);
              if (status == RPC_NET_ERROR)
                goto net_error;
              }
            } while (pevent == NULL);

          eq->last_called = actual_millitime;

          /* compose MIDAS event header */
          pevent->event_id      = eq_info->event_id;
          pevent->trigger_mask  = eq_info->trigger_mask;
          pevent->data_size     = 0;
          pevent->time_stamp    = actual_time;
          pevent->serial_number = eq->serial_number++;

          /* call user readout routine */
          *((EQUIPMENT **)(pevent+1)) = &equipment[index];
          pevent->data_size = eq->readout((char *) (pevent+1));

          /* send event */
          if (pevent->data_size)
            {
            eq->bytes_sent += pevent->data_size + sizeof(EVENT_HEADER);
            eq->events_sent++;

            if (eq->buffer_handle)
              {
#ifdef USE_EVENT_CHANNEL
              eb_increment_pointer(eq->buffer_handle, 
                                   pevent->data_size + sizeof(EVENT_HEADER));
#else
              rpc_flush_event();
              bm_send_event(eq->buffer_handle, pevent,
                            pevent->data_size + sizeof(EVENT_HEADER), SYNC);
#endif
              }

            /* send event to ODB for periodic events */
            if (eq_info->eq_type == EQ_PERIODIC &&
                (eq_info->read_on & RO_ODB ||
                 eq_info->history > 0))
              {
              update_odb(pevent, eq->hkey_variables, eq->format);
              eq->odb_out++;
              }
            }
          else
            eq->serial_number--;
          
          if (interrupt_eq && !eb_buffer_full())
            interrupt_block(FALSE);
          }
        }

      /*---- check polled events ----*/
      if (eq_info->eq_type == EQ_POLLED)
        {
        readout_start = actual_millitime;
        pevent = NULL;

        while ((source = poll_event(eq_info->source, eq->poll_count, FALSE)) > 0)
          {
          do
            {
            pevent = eb_get_pointer();
            
            /* if no space in buffer, empty it */
            if (pevent == NULL)
              {
              status = eb_send_events(FALSE);
              if (status == RPC_NET_ERROR)
                goto net_error;
              }
            } while (pevent == NULL);

          /* compose MIDAS event header */
          pevent->event_id      = eq_info->event_id;
          pevent->trigger_mask  = eq_info->trigger_mask;
          pevent->data_size     = 0;
          pevent->time_stamp    = actual_time;
          pevent->serial_number = eq->serial_number++;

          /* put source at beginning of event */
          *(INT *) (pevent+1) = source;

          /* call user readout routine */
          pevent->data_size = eq->readout((char *) (pevent+1));

          /* send event */
          if (pevent->data_size)
            {
            if (eq->buffer_handle)
              {
#ifdef USE_EVENT_CHANNEL
              eb_increment_pointer(eq->buffer_handle, 
                                   pevent->data_size + sizeof(EVENT_HEADER));
#else
              rpc_send_event(eq->buffer_handle, pevent,
                             pevent->data_size + sizeof(EVENT_HEADER), SYNC);
#endif

              status = eb_send_events(FALSE);
              if (status == RPC_NET_ERROR)
                goto net_error;

              eq->bytes_sent += pevent->data_size + sizeof(EVENT_HEADER);
              eq->events_sent++;
              }
            }
          else
            eq->serial_number--;

          actual_millitime = ss_millitime();

          /* repeat no more than period */
          if (actual_millitime - readout_start > (DWORD) eq_info->period)
            break;

          /* quit if event limit is reached */
          if (eq_info->event_limit &&
              eq->serial_number > eq_info->event_limit)
            break;
          }

        /* send remaining events in order to avoid partially sent events */
        while (eb_send_events(TRUE) == BM_MORE_EVENTS);

        /* send event to ODB */
        if (eq_info->read_on & RO_ODB ||
            eq_info->history)
          {
          if (actual_millitime - eq->last_called > ODB_UPDATE_TIME && pevent != NULL)
            {
            eq->last_called = actual_millitime;
            update_odb(pevent, eq->hkey_variables, eq->format);
            eq->odb_out++;
            }
          }
        }

      /*---- send interrupt events ----*/
      if (eq_info->eq_type == EQ_INTERRUPT)
        {
        readout_start = actual_millitime;

        do
          {
          status = eb_send_events(FALSE);
          if (status == RPC_NET_ERROR)
            goto net_error;

          } while (ss_millitime() - readout_start < (DWORD) eq_info->period &&
                   status == BM_MORE_EVENTS);
        
        /* send remaining events in order to avoid partially sent events */
        while (eb_send_events(TRUE) == BM_MORE_EVENTS);

        /* update ODB */
        if (interrupt_odb_buffer_valid)
          {
          update_odb(interrupt_odb_buffer, interrupt_eq->hkey_variables,
                     interrupt_eq->format);
          interrupt_odb_buffer_valid = FALSE;
          }

        /* check for low water mark */
        if (interrupt_eq && !eb_buffer_full())
          interrupt_block(FALSE);
        }

      /*---- check if event limit is reached ----*/
      if (eq_info->event_limit &&
          eq->serial_number > eq_info->event_limit &&
          run_state == STATE_RUNNING)
        {
        /* stop run */
        if (cm_transition(TR_STOP, 0, str, sizeof(str), SYNC) != CM_SUCCESS)
          cm_msg(MERROR, "scheduler", "cannot stop run: %s", str);
        }
      }

    /*---- check if Interrupt routine needs calling ----------------*/
    if (daqPendingInterrupt && !eb_buffer_full())
      {
      daqPendingInterrupt = 0;
      interrupt_routine();
      cm_msg(MINFO, "scheduler", "Interrupt pending: restarting front end.");
      }

    /*---- call frontend_loop periodically -------------------------*/
    if (frontend_call_loop)
      frontend_loop();

    /*---- calculate rates and update status page periodically -----*/
    if ((display_period && actual_millitime - last_time_display > (DWORD) display_period) ||
        (!display_period && actual_millitime - last_time_display > 5000))
      {
      /* calculate rates */
      if (actual_millitime != last_time_display)
        {
        max_bytes_per_sec = 0;
        for (i=0 ; equipment[i].name[0] ; i++)
          {
          eq = &equipment[i];
          j = eq->serial_number;
          if (j > 0)
            j--;
          eq->stats.events_sent = j;
          eq->stats.events_per_sec =
            (DWORD) (eq->events_sent/((actual_millitime-last_time_display)/1000.0));
          eq->stats.kbytes_per_sec =
            eq->bytes_sent/1024.0/((actual_millitime-last_time_display)/1000.0);

          if ((INT) eq->bytes_sent > max_bytes_per_sec)
            max_bytes_per_sec = eq->bytes_sent;

          eq->bytes_sent = 0;
          eq->events_sent = 0;
          }
        max_bytes_per_sec = (DWORD) 
          ((double)max_bytes_per_sec/((actual_millitime-last_time_display)/1000.0));

        /* tcp buffer size evaluation */
        if (optimize)
          {
          opt_max = max(opt_max, (INT)max_bytes_per_sec);
          ss_printf(0, opt_index, "%6d : %5.1lf %5.1lf", opt_tcp_size, opt_max/1024.0,
                    max_bytes_per_sec/1024.0);
          if (++opt_cnt == 10)
            {
            opt_cnt = 0;
            opt_max = 0;
            opt_index++;
            opt_tcp_size = 1<<(opt_index+7);
            rpc_set_opt_tcp_size(opt_tcp_size);
            if (1<<(opt_index+7) > 0x8000)
              {
              opt_index = 0;
              opt_tcp_size = 1<<7;
              rpc_set_opt_tcp_size(opt_tcp_size);
              }
            }
          }

        }

      /* propagate changes in equipment to ODB */
      rpc_set_option(-1, RPC_OTRANSPORT, RPC_FTCP);
      db_send_changed_records();
      rpc_set_option(-1, RPC_OTRANSPORT, RPC_TCP);

      if (display_period)
        {
        display(FALSE);

        /* check keyboard */
        ch = 0;
        status = 0;
/*
        if (ss_kbhit())
          {
#if defined(OS_MSDOS) || defined(OS_WINNT)
          ch = getch();
#else
          ch = getchar();
#endif
          }
*/
        while (ss_kbhit())
          {
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
    if (actual_millitime - last_time_flush > 1000)
      {
      last_time_flush = actual_millitime;
    
      /* if event buffer contains less than optimal TCP size, flush
         it now */
      if (max_bytes_per_sec < rpc_get_opt_tcp_size())
        eb_send_events(TRUE);

      /* if cache on server is not filled in one second at current
         data rate, flush it now to make events available to consumers */
      
      if (max_bytes_per_sec < SERVER_CACHE_SIZE)
        {
        for (i=0 ; equipment[i].name[0] ; i++)
          {
          if (equipment[i].buffer_handle)
            {
            /* check if buffer already flushed */
            buffer_done = FALSE;
            for (j=0 ; j<i ; j++)
              if (equipment[i].buffer_handle == equipment[j].buffer_handle)
                {
                buffer_done = TRUE;
                break;
                }

            if (!buffer_done)
              {
              rpc_set_option(-1, RPC_OTRANSPORT, RPC_FTCP);
              bm_flush_cache(equipment[i].buffer_handle, ASYNC);
              rpc_set_option(-1, RPC_OTRANSPORT, RPC_TCP);
              }
            }
          }
        }
      }

    /*---- check network messages ----------------------------------*/
    if (run_state == STATE_RUNNING && interrupt_eq == NULL)
      {
      /* only call yield once every 100ms when running */
      if (actual_millitime - last_time_network > 100)
        {
        status = cm_yield(0);
        last_time_network = actual_millitime;
        }
      else
        status = RPC_SUCCESS;
      }
    else
      /* when run is stopped or interrupts used, 
         call yield with 100ms timeout */
      status = cm_yield(100);

    /* exit for VxWorks */
    if (fe_stop)
      status = RPC_SHUTDOWN;

    } while (status != RPC_SHUTDOWN && status != SS_ABORT);

net_error:

  return status;
}

/*------------------------------------------------------------------*/

INT cnaf_callback(INT index, void *prpc_param[])
{
DWORD cmd, b, c, n, a, f, *pdword, *size, *x, *q, dtemp;
WORD  *pword, *pdata, temp;
INT   i, count;

  /* Decode parameters */
  cmd    = CDWORD(0);
  b      = CDWORD(1);
  c      = CDWORD(2);
  n      = CDWORD(3);
  a      = CDWORD(4);
  f      = CDWORD(5);
  pdword = CPDWORD(6);
  pword  = CPWORD(6);
  pdata  = CPWORD(6);
  size   = CPDWORD(7);
  x      = CPDWORD(8);
  q      = CPDWORD(9);

  /* determine repeat count */
  if (index == RPC_CNAF16)
    count = *size / sizeof(WORD);  /* 16 bit */
  else
    count = *size / sizeof(DWORD); /* 24 bit */

  switch (cmd)
    {
    /*---- special commands ----*/
    
    case CNAF_INHIBIT_SET: 
      cam_inhibit_set(c);
      break;
    case CNAF_INHIBIT_CLEAR:
      cam_inhibit_clear(c);
      break;
    case CNAF_CRATE_CLEAR:
      cam_crate_clear(c);
      break;
    case CNAF_CRATE_ZINIT:
      cam_crate_zinit(c);
      break;

    case CNAF_TEST:
      break;

    case CNAF:
      if (index == RPC_CNAF16)
        {
        for (i=0 ; i<count ; i++)
          if (f<16)
            cam16i_q(c, n, a, f, pword, (int *) x, (int *) q);
          else if (f<24)
            cam16o_q(c, n, a, f, pword[i], (int *) x, (int *) q);
          else
            cam16i_q(c, n, a, f, &temp, (int *) x, (int *) q);
        }
      else
        {
        for (i=0 ; i<count ; i++)
          if (f<16)
            cam24i_q(c, n, a, f, pdword, (int *) x, (int *) q);
          else if (f<24)
            cam24o_q(c, n, a, f, pdword[i], (int *) x, (int *) q);
          else
            cam24i_q(c, n, a, f, &dtemp, (int *) x, (int *) q);
        }

      break;

    case CNAF_nQ:
      if (index == RPC_CNAF16)
        {
        if (f<16)
          cam16i_rq(c, n, a, f, (WORD **) &pdword, count);
        }
      else
        {
        if (f<16)
          cam24i_rq(c, n, a, f, &pdword, count);
        }

      /* return reduced return size */
      *size = (int) pdword - (int) pdata;
      break;

    default:
      printf("cnaf: Unknown command 0x%X\n", cmd);
    }

  printf("cmd=%d c=%d n=%d a=%d f=%d d=%X\n", cmd, c, n, a, f, pdword[0]);

  return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

#ifdef OS_VXWORKS
int mfe(char *ahost_name, char *aexp_name, BOOL adebug)
#else
main(int argc, char *argv[])
#endif
{
INT    status, i, eb_size;

  host_name[0] = 0;
  exp_name[0] = 0;
  debug = FALSE;

#ifdef OS_VXWORKS
  if (ahost_name)
    strcpy(host_name, ahost_name);
  if (aexp_name)
    strcpy(exp_name, aexp_name);
  debug = adebug;
#else

  /* get default from environment */
  cm_get_environment(host_name, exp_name);
  
  /* parse command line parameters */
  for (i=1 ; i<argc ; i++)
    {
    if (argv[i][0] == '-' && argv[i][1] == 'd')
      debug = TRUE;
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
        printf("usage: frontend [-h Hostname] [-e Experiment] [-d]\n\n");
        return 0;
        }
      }
    }
#endif

  /* allocate event ring buffer to hold at least three events */
  eb_size = 3*(MAX_EVENT_SIZE + sizeof(EVENT_HEADER));
  eb_size = max(eb_size , 0x4000); /* minimum 16kb */
  if (eb_size > event_buffer_size)
    {
    printf("event_buffer_size too small for max. event size\n");
    ss_sleep(1000);
    }
  else
    eb_size = event_buffer_size;

  /* reduce memory size for MS-DOS */
#ifdef OS_MSDOS
  if (eb_size > 0x4000)
    eb_size = 0x4000; /* 16k */
#endif

  /* now connect to server */
  if (display_period)
    {
    if (host_name[0])
      printf("Connect to experiment %s on host %s...", exp_name, host_name);
    else
      printf("Connect to experiment %s...", exp_name);
    }

reconnect:

  status = cm_connect_experiment(host_name, exp_name, frontend_name, NULL);
  if (status != CM_SUCCESS)
    {
    /* let user read message before window might close */
    ss_sleep(5000);
    return 1;
    }

  if (display_period)
    printf("OK\n");

  status = eb_create_buffer(eb_size);
  if (status != CM_SUCCESS)
    {
    printf("Not enough memory. Decrease event_buffer_size in frontend.c\n");
    return 1;
    }

  /* shutdown previous frontend */
  status = cm_shutdown(frontend_name, FALSE);
  if (status == CM_SHUTDOWN && display_period)
    {
    printf("Previous frontend stopped\n");
    ss_sleep(1000);
    }

  /* register transition callbacks */
  if (cm_register_transition(TR_START, tr_start) != CM_SUCCESS ||
      cm_register_transition(TR_PRESTOP, tr_prestop) != CM_SUCCESS ||
      cm_register_transition(TR_PREPAUSE, tr_prepause) != CM_SUCCESS ||
      cm_register_transition(TR_RESUME, tr_resume) != CM_SUCCESS)
    {
    printf("Failed to start local RPC server");
    cm_disconnect_experiment();
    eb_free_buffer();

    /* let user read message before window might close */
    ss_sleep(5000);
    return 1;
    }

  /* register CNAF callback */
  cm_register_function(RPC_CNAF16, cnaf_callback); 
  cm_register_function(RPC_CNAF24, cnaf_callback); 

  cm_get_experiment_database(&hDB, &status);

  /* set time from server */
#ifdef OS_VXWORKS
  cm_synchronize(NULL);
#endif

  /* turn on keepalive messages with increased timeout */
  cm_set_watchdog_params(TRUE, 30000);

  /* turn off watchdog if in debug mode */
  if (debug)
    cm_set_watchdog_params(TRUE, 0);

  /* increase RPC timeout to 60sec for logger with exabyte */
  rpc_set_option(-1, RPC_OTIMEOUT, 60000);

  /* set own message print function */
  if (display_period)
    cm_set_msg_print(MT_ALL, MT_ALL, message_print);

  /* call user init function */
  if (display_period)
    printf("Init hardware...");
  if (frontend_init() != SUCCESS)
    {
    if (display_period)
      printf("\n");
    cm_disconnect_experiment();
    eb_free_buffer();

    /* let user read message before window might close */
    ss_sleep(5000);
    return 1;
    }

  /* reqister equipment in ODB */
  if (register_equipment() != SUCCESS)
    {
    if (display_period)
      printf("\n");
    cm_disconnect_experiment();
    eb_free_buffer();

    /* let user read message before window might close */
    ss_sleep(5000);
    return 1;
    }

  if (display_period)
    printf("OK\n");

  /* initialize screen display */
  if (display_period)
    {
    ss_sleep(1000);
    display(TRUE);
    }

  /* switch on interrupts if running */
  if (interrupt_eq && run_state == STATE_RUNNING)
    interrupt_enable(TRUE);

  /* initialize ss_getchar */
  ss_getchar(0);

  /* call main scheduler loop */
  status = scheduler();

  /* reset terminal */
  ss_getchar(TRUE);

  /* switch off interrupts */
  if (interrupt_eq)
    {
    interrupt_configure(CMD_INTERRUPT_DISABLE, 0, 0);
    interrupt_configure(CMD_INTERRUPT_DETACH, 0, 0);
    if (interrupt_odb_buffer)
      free(interrupt_odb_buffer);
    }

  /* detach interrupts */
  if (interrupt_eq != NULL)
    interrupt_configure(CMD_INTERRUPT_DETACH, interrupt_eq->info.source, 0);

  /* call user exit function */
  frontend_exit();

  /* close slow control drivers */
  for (i=0 ; equipment[i].name[0] ; i++)
    if (equipment[i].info.eq_type == EQ_SLOW &&
        equipment[i].status == FE_SUCCESS)
      equipment[i].cd(CMD_EXIT, &equipment[i]);

  /* close network connection to server */
  cm_disconnect_experiment();

  if (status == RPC_NET_ERROR || status == SS_ABORT)
    {
    printf("Network connection broken, trying to reestablish...\n");

    eb_free_buffer();

    /* try to reconnect */
    ss_sleep(12000);
    goto reconnect;
    }

  if (display_period)
    {
    if (status == RPC_SHUTDOWN)
      {
      ss_clear_screen();
      ss_printf(0, 0, "Frontend shut down.");
      ss_printf(0, 1, "");
      }
    }

  if (status != RPC_SHUTDOWN)
    printf("\nNetwork connection aborted.");

  eb_free_buffer();

  return 0;
}
