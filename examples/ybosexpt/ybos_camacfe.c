/********************************************************************\

  Name:         ybosfe.c
  Created by:   Pierre-Andre / Stefan Ritt

  Contents:     Experiment specific readout code (user part) of
                Midas frontend. This example implement ...
                event" and a "scaler event" which are filled with
                random data. The trigger event is filled with
                two banks (ADC0 and TDC0), the scaler event with 
                one bank (SCLR).


  Revision history
  ------------------------------------------------------------------
  date         by    modification
  ---------    ---   ------------------------------------------------
  25-JAN-95    SR    created


\********************************************************************/

#include "midas.h"
#include "msystem.h"
#include "ybos.h"
#include "cam8210.h"

/* make frontend functions callable from the C framework */
#ifdef __cplusplus
extern "C" {
#endif

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
char *frontend_name = "Trigger Frontend";
/* The frontend file name, don't change it */
char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
BOOL frontend_call_loop = FALSE;

/* a frontend status page is displayed with this frequency in ms */
INT display_period = 0000;

/* buffer size to hold events */
INT event_buffer_size = 200000;

DWORD ext_crate1;
INT   dd;
/*-- Function declarations -----------------------------------------*/

INT frontend_init();
INT frontend_exit();
INT begin_of_run(INT run_number, char *error);
INT end_of_run(INT run_number, char *error);
INT pause_run(INT run_number, char *error);
INT resume_run(INT run_number, char *error);
INT frontend_loop();

INT poll_trigger_event(INT count, PTYPE test);
INT interrupt_configure(INT cmd, PTYPE adr);
INT read_trigger_event(char *pevent);
INT read_scaler_event(char *pevent);

/*-- Equipment list ------------------------------------------------*/

#define NUSE_INT

EQUIPMENT equipment[] = {

  { "Trigger",            /* equipment name */
    1, 0,                 /* event ID, trigger mask */
    "SYSTEM",             /* event buffer */
#ifdef USE_INT
    EQ_INTERRUPT,         /* equipment type */
#else
    EQ_POLLED,            /* equipment type */
#endif
    "MIDAS",              /* format */
    TRUE,                 /* enabled */
    RO_RUNNING |          /* read only when running */
    RO_ODB,               /* and update ODB */ 
    100,                  /* poll for 100ms */
    0,                    /* stop run after this event limit */
    0,                    /* don't log history */
    "", "", "",
    read_trigger_event,   /* readout routine */
#ifdef USE_INT
    interrupt_configure,  /* interrupt configuration */
#else
    poll_trigger_event,   /* polling routine */
#endif
    NULL,                 /* init string */
  },

  { "Scaler",             /* equipment name */
    2, 0,                 /* event ID, trigger mask */
    "SYSTEM",             /* event buffer */
    EQ_PERIODIC,          /* equipment type */
    "MIDAS",              /* format */
    FALSE,                /* enabled */
    RO_RUNNING |
    RO_TRANSITIONS |      /* read when running and on transitions */
    RO_ODB,               /* and update ODB */ 
    10000,                /* read every 10 sec */
    0,                    /* stop run after this event limit */
    0,                    /* log history */
    "", "", "",
    read_scaler_event,    /* readout routine */
    NULL,                 /* polling routine */
    NULL,                 /* init string */
  },

  { "" }
};

#ifdef __cplusplus
}
#endif

/********************************************************************\
              Callback routines for system transitions

  These routines are called whenever a system transition like start/
  stop of a run occurs. The routines are called on the following
  occations:

  frontend_init:  When the frontend program is started. This routine
                  should initialize the hardware.
  
  frontend_exit:  When the frontend program is shut down. Can be used
                  to releas any locked resources like memory, commu-
                  nications ports etc.

  begin_of_run:   When a new run is started. Clear scalers, open
                  rungates, etc.

  end_of_run:     Called on a request to stop a run. Can send 
                  end-of-run event and close run gates.

  pause_run:      When a run is paused. Should disable trigger events.

  resume_run:     When a run is resumed. Should enable trigger events.

\********************************************************************/

/*-- Frontend Init -------------------------------------------------*/

INT frontend_init()
{
  CAM_BC (&ext_crate1, 0, 1);                       /* Branch 0 Crate 1 */

  /* put here hardware initialization */

  return CM_SUCCESS;
}

/*-- Frontend Exit -------------------------------------------------*/

INT frontend_exit()
{
  return CM_SUCCESS;
}

/*-- Begin of Run --------------------------------------------------*/

INT begin_of_run(INT run_number, char *error)
{
  /* put here clear scalers etc. */
  CAM_CMD(&ext_crate1,30,9,24) ;  /* Camac clear Inhibit */

  CAM_CMD(&ext_crate1,10,0,9) ;   /* clear C212 */

  /* clear scalers */
  CAM_CMDAS (&ext_crate1,20,0,9,6);
  CAM_CMDAS (&ext_crate1,21,0,9,6);

  /* reset latch */
  CAM24_O(&ext_crate1,11,0,16,0x0);
  CAM24_O(&ext_crate1,11,0,16,0x7);
  CAM24_O(&ext_crate1,11,0,16,0x0);
  return CM_SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/

INT end_of_run(INT run_number, char *error)
{
  CAM_CMD(&ext_crate1,30,9,26) ;  /* Camac set Inhibit */
  return CM_SUCCESS;
}

/*-- Pause Run -----------------------------------------------------*/

INT pause_run(INT run_number, char *error)
{
  return CM_SUCCESS;
}

/*-- Resuem Run ----------------------------------------------------*/

INT resume_run(INT run_number, char *error)
{
  return CM_SUCCESS;
}

/*-- Frontend Loop -------------------------------------------------*/

INT frontend_loop()
{
  /* if frontend_call_loop is true, this routine gets called when
     the frontend is idle or between every event */
  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

/********************************************************************\
  
  Readout routines for different events

\********************************************************************/

/*-- Trigger event routines ----------------------------------------*/

INT poll_trigger_event(INT poll_count, PTYPE test)
{
  INT i;
  DWORD  pat;

  for (i=0 ; i<poll_count ; i++)
    {
      /* read the LAM register of the Crate controller to find out if
         ANY station has raised the LAM line.
         */
      CAM24_I(&ext_crate1,10,0,0,&dd);
      if (dd != 0)      /* Ignore LAM is not LAM_STATION */
        if (!test)
          return TRUE;
    }
  return FALSE;
}

/*-- Interrupt configuration for trigger event ---------------------*/

INT interrupt_configure(INT cmd, PTYPE adr)
{
#ifdef OS_VXWORKS
  switch(cmd)
    {
    case CMD_INTERRUPT_ENABLE:
      int_ces_enable(2);
      break;
    case CMD_INTERRUPT_DISABLE:
      int_ces_disable(2);
      break;
    case CMD_INTERRUPT_INSTALL:
      int_ces_book(2, adr);
      break;
    case CMD_INTERRUPT_DEINSTALL:
      int_ces_unbook(2);
      break;
    }
#endif
  return CM_SUCCESS;
}

/*-- Event readout -------------------------------------------------*/

INT read_trigger_event(char *pevent)
{
DWORD *pdata, i;

  CAM24_O(&ext_crate1,11,0,16,0x8);
  CAM24_O(&ext_crate1,11,0,16,0x0);

/*
  pdata = (DWORD *) pevent;
  i = rand() % 10;

  pdata[0] = i;
  pdata[i/4-1] = i;

  ss_sleep(10);
  return i;
*/
  /*----------------*/
  
  /* init bank structure */
  bk_init(pevent);

  /* create ADC bank */
  bk_create(pevent, "ADC0", TID_DWORD, &pdata);

  /* fill ADC bank with random data */
  for (i=0 ; i<8 ; i++)
    *pdata++ = rand() & 0xFFF;

  bk_close(pevent, pdata);

  /* create TDC bank */
  bk_create(pevent, "TDC0", TID_DWORD, &pdata);

  for (i=0;i<8;i++)
    {
      CAM24_IAS(&ext_crate1,20,0,0,(DWORD *) pdata,6);
      CAM24_IAS(&ext_crate1,21,0,0,(DWORD *) pdata,6);
    }
  /* fill TDC bank with random data */
/*  for (i=0 ; i<8 ; i++)
    *pdata++ = rand() & 0xFFF;
*/

  bk_close(pevent, pdata);

  CAM_CMD(&ext_crate1,10,0,9) ;   /* clear C212 */

  CAM24_O(&ext_crate1,11,0,16,0x7);
  CAM24_O(&ext_crate1,11,0,16,0x0);

  return bk_size(pevent);
}

/*-- Scaler event --------------------------------------------------*/

INT read_scaler_event(char *pevent)
{
DWORD *pdata, i;

  /* init bank structure */
  bk_init(pevent);

  /* create SCLR bank */
  bk_create(pevent, "SCLR", TID_DWORD, &pdata);

  /* fill scaler bank with random data */
  for (i=0 ; i<8 ; i++)
    *pdata++ = rand();

  bk_close(pevent, pdata);

  return bk_size(pevent);
}
