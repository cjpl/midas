/********************************************************************\

  Name:         frontend.c
  Created by:   Stefan Ritt

  Contents:     Experiment specific readout code (user part) of
                Midas frontend. This example simulates a "trigger
                event" and a "scaler event" which are filled with
                CAMAC or random data. The trigger event is filled 
                with two banks (ADC0 and TDC0), the scaler event 
                with one bank (SCLR).

  $Log$
  Revision 1.5  1998/11/09 09:14:41  midas
  Added code to simulate random data

  Revision 1.4  1998/10/29 14:27:46  midas
  Added note about FE_ERR_HW in frontend_init()

  Revision 1.3  1998/10/28 15:50:58  midas
  Changed lam to DWORD

  Revision 1.2  1998/10/12 12:18:58  midas
  Added Log tag in header


\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "midas.h"
#include "mcstd.h"
#include "experim.h"

/* make frontend functions callable from the C framework */
#ifdef __cplusplus
extern "C" {
#endif

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
char *frontend_name = "Sample Frontend";
/* The frontend file name, don't change it */
char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
BOOL frontend_call_loop = FALSE;

/* a frontend status page is displayed with this frequency in ms */
INT display_period = 3000;

/* buffer size to hold events */
INT event_buffer_size = DEFAULT_EVENT_BUFFER_SIZE;

/* number of channels */
#define N_ADC  8  
#define N_TDC  8  
#define N_SCLR 8

/*-- Function declarations -----------------------------------------*/

INT frontend_init();
INT frontend_exit();
INT begin_of_run(INT run_number, char *error);
INT end_of_run(INT run_number, char *error);
INT pause_run(INT run_number, char *error);
INT resume_run(INT run_number, char *error);
INT frontend_loop();

INT read_trigger_event(char *pevent);
INT read_scaler_event(char *pevent);

/*-- Equipment list ------------------------------------------------*/

#undef USE_INT

EQUIPMENT equipment[] = {

  { "Trigger",            /* equipment name */
    1, 0,                 /* event ID, trigger mask */
    "SYSTEM",             /* event buffer */
#ifdef USE_INT
    EQ_INTERRUPT,         /* equipment type */
#else
    EQ_POLLED,            /* equipment type */
#endif
    CRATE(0),             /* event source */
    "MIDAS",              /* format */
    TRUE,                 /* enabled */
    RO_RUNNING |          /* read only when running */
    RO_ODB,               /* and update ODB */ 
    500,                  /* poll for 500ms */
    0,                    /* stop run after this event limit */
    0,                    /* don't log history */
    "", "", "",
    read_trigger_event,   /* readout routine */
  },

  { "Scaler",             /* equipment name */
    2, 0,                 /* event ID, trigger mask */
    "SYSTEM",             /* event buffer */
    EQ_PERIODIC,          /* equipment type */
    0,                    /* event source */
    "MIDAS",              /* format */
    TRUE,                 /* enabled */
    RO_RUNNING |
    RO_TRANSITIONS |      /* read when running and on transitions */
    RO_ODB,               /* and update ODB */ 
    10000,                /* read every 10 sec */
    0,                    /* stop run after this event limit */
    0,                    /* log history */
    "", "", "",
    read_scaler_event,    /* readout routine */
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
  /* put here hardware initialization */

  /* print message and return FE_ERR_HW if frontend should not be started */

  return SUCCESS;
}

/*-- Frontend Exit -------------------------------------------------*/

INT frontend_exit()
{
  return SUCCESS;
}

/*-- Begin of Run --------------------------------------------------*/

INT begin_of_run(INT run_number, char *error)
{
  /* put here clear scalers etc. */

  return SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/

INT end_of_run(INT run_number, char *error)
{
  return SUCCESS;
}

/*-- Pause Run -----------------------------------------------------*/

INT pause_run(INT run_number, char *error)
{
  return SUCCESS;
}

/*-- Resuem Run ----------------------------------------------------*/

INT resume_run(INT run_number, char *error)
{
  return SUCCESS;
}

/*-- Frontend Loop -------------------------------------------------*/

INT frontend_loop()
{
  /* if frontend_call_loop is true, this routine gets called when
     the frontend is idle or once between every event */
  return SUCCESS;
}

/*------------------------------------------------------------------*/

/********************************************************************\
  
  Readout routines for different events

\********************************************************************/

/*-- Trigger event routines ----------------------------------------*/

INT poll_event(INT source, INT count, BOOL test)
/* Polling routine for events. Returns TRUE if event
   is available. If test equals TRUE, don't return. The test
   flag is used to time the polling */
{
int   i;
DWORD lam;

  for (i=0 ; i<count ; i++)
    {
    cam_lam_read(source >> 24, &lam);

    if (lam)
      if (!test)
        return TRUE;
    }

  return FALSE;
}

/*-- Interrupt configuration ---------------------------------------*/

INT interrupt_configure(INT cmd, INT source[], PTYPE adr)
{
  switch(cmd)
    {
    case CMD_INTERRUPT_ENABLE:
      break;
    case CMD_INTERRUPT_DISABLE:
      break;
    case CMD_INTERRUPT_ATTACH:
      break;
    case CMD_INTERRUPT_DETACH:
      break;
    }
  return SUCCESS;
}

/*-- Event readout -------------------------------------------------*/

INT read_trigger_event(char *pevent)
{
WORD *pdata, a;

  /* init bank structure */
  bk_init(pevent);

  /* create ADC bank */
  bk_create(pevent, "ADC0", TID_WORD, &pdata);

  /* read ADC bank */
  for (a=0 ; a<N_ADC ; a++)
    cami(1, 1, a, 0, pdata++);
    /* instead of using cami(), use following line to "simulate" data */
    /* *pdata++ = rand() % 1024; */

  bk_close(pevent, pdata);

  /* create TDC bank */
  bk_create(pevent, "TDC0", TID_WORD, &pdata);

  /* read TDC bank */
  for (a=0 ; a<N_TDC ; a++)
    cami(1, 2, a, 0, pdata++);

  bk_close(pevent, pdata);

  return bk_size(pevent);
}

/*-- Scaler event --------------------------------------------------*/

INT read_scaler_event(char *pevent)
{
DWORD *pdata, a;

  /* init bank structure */
  bk_init(pevent);

  /* create SCLR bank */
  bk_create(pevent, "SCLR", TID_DWORD, &pdata);

  /* read scaler bank */
  for (a=0 ; a<N_SCLR ; a++)
    cam24i(1, 3, a, 0, pdata++);

  bk_close(pevent, pdata);

  return bk_size(pevent);
}
