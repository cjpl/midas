/********************************************************************\

  Name:         frontend.c
  Created by:   Stefan Ritt

  Contents:     Experiment specific readout code (user part) of
                Midas frontend. This example simulates a "trigger
                event" and a "scaler event" which are filled with
                random data. The trigger event is filled with
                two banks (ADC0 and TDC0), the scaler event with 
                one bank (SCLR).

  $Log$
  Revision 1.2  1998/10/12 12:18:59  midas
  Added Log tag in header


\********************************************************************/

#include <stdio.h>
#include "midas.h"
#include "msystem.h"
#include "ybos.h"
#include "mcstd.h"
#include "camacnul.c"

/* make frontend functions callable from the C framework */
#ifdef __cplusplus
extern "C" {
#endif

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
char *frontend_name = "YbosFE";
/* The frontend file name, don't change it */
char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
BOOL frontend_call_loop = FALSE;

/* a frontend status page is displayed with this frequency in ms */
INT display_period = 5000;

/* buffer size to hold events */
INT event_buffer_size = DEFAULT_EVENT_BUFFER_SIZE;

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
INT files_dump (char *pevent);
INT gbl_run_number;
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
    "YBOS",               /* format */
    TRUE,                 /* enabled */
    RO_RUNNING,           /* read only when running */
    500,                  /* poll for 500ms */
    0,                    /* stop run after this event limit */
    0,                    /* don't log history */
    "", "", "",
    read_trigger_event,   /* readout routine */
    NULL,                 /* keep null */
    NULL                  /* init string */
  },

  { "Scaler",             /* equipment name */
    2, 0,                 /* event ID, trigger mask */
    "SYSTEM",             /* event buffer */
    EQ_PERIODIC,          /* equipment type */
    0,                    /* event source */
    "YBOS",               /* format */
    TRUE,                 /* enabled */
    RO_RUNNING |
    RO_TRANSITIONS |      /* read when running and on transitions */
    RO_ODB,               /* and update ODB */ 
    10000,                /* read every 10 sec */
    0,                    /* stop run after this event limit */
    0,                    /* log history */
    "", "", "",
    read_scaler_event,    /* readout routine */
    NULL,                 /* keep null */
    NULL                  /* init string */
  },

  { "File",               /* equipment name */
    7, 0x0000,            /* event ID, mask */
    "SYSTEM",             /* event buffer */
    EQ_PERIODIC,          /* equipment type */
    0,                    /* event source */
    "YBOS",               /* format */
    FALSE,                /* enabled */
    RO_BOR,               /* read only at BOR */
    10000,                /* read every 10 sec */
    0,                    /* stop run after this event limit */
    0,                    /* don't log history */
    "", "", "",
    files_dump,           /* file dump */
    NULL,                 /* keep null */
    NULL                  /* init string */
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
  gbl_run_number = run_number;
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
     the frontend is idle or between every event */
  return SUCCESS;
}

/*------------------------------------------------------------------*/

/********************************************************************\
  
  Readout routines for different events

\********************************************************************/

/*-- Trigger event routines ----------------------------------------*/

INT poll_event(INT source, INT count, BOOL test)
{
  INT dd, i;
  
  for (i=0 ; i<count ; i++)
    {
      /* read the LAM register of the Crate controller to find out if
         ANY station has raised the LAM line.
         */
      dd = 1;
      if (dd)      /* Ignore LAM if not LAM_STATION */
        if (!test)
          return TRUE;
    }
  return FALSE;
}

/*-- Interrupt configuration for trigger event ---------------------*/
INT interrupt_configure(INT cmd, INT source[], PTYPE adr)
{
  switch(cmd)
    {
    case CMD_INTERRUPT_ENABLE:
      cam_interrupt_enable();
      break;
    case CMD_INTERRUPT_DISABLE:
      cam_interrupt_disable();
      break;
    case CMD_INTERRUPT_ATTACH:
      cam_interrupt_attach((void (*)())adr);
      break;
    case CMD_INTERRUPT_DETACH:
      cam_interrupt_detach();
      break;
    }
  return SUCCESS;
}

/*-- Event readout -------------------------------------------------*/

INT read_trigger_event(char *pevent)
{
DWORD i;
DWORD *pbkdat;

ybk_init((DWORD *) pevent);

/* collect user hardware data */
/*---- USER Stuff ----*/
  ybk_create((DWORD *)pevent, "ADC0", I4_BKTYPE, (DWORD *)(&pbkdat));
  for (i=0 ; i<8 ; i++)
    *pbkdat++ = i & 0xFFF;
  ybk_close((DWORD *)pevent, pbkdat);

  ybk_create((DWORD *)pevent, "TDC0", I2_BKTYPE, (DWORD *)(&pbkdat));
  for (i=0 ; i<8 ; i++)
    *((WORD *)pbkdat)++ = (WORD)(0x10+i) & 0xFFF;
  ybk_close((DWORD *) pevent, pbkdat);

  ybk_create_chaos((DWORD *)pevent, "POJA", I2_BKTYPE, (DWORD *)(&pbkdat));
  for (i=0 ; i<9 ; i++)
    *((WORD *)pbkdat)++ = (WORD) (0x20+i) & 0xFFF;
  ybk_close_chaos((DWORD *) pevent, I2_BKTYPE, pbkdat);
/* END OF EVENT */

  return (ybk_size((DWORD *)pevent));
}

/*-- Scaler event --------------------------------------------------*/

INT read_scaler_event(char *pevent)
{
  DWORD i;
  DWORD *pbkdat;

  ybk_init((DWORD *) pevent);

  /* collect user hardware data */
  /*---- USER Stuff ----*/
  ybk_create((DWORD *)pevent, "SCAL", I4_BKTYPE, (DWORD *)(&pbkdat));
  for (i=0 ; i<16 ; i++)
    *pbkdat++ = (DWORD)(0xff+i) & 0xFFF;
  ybk_close((DWORD *)pevent, pbkdat);

  return (ybk_size((DWORD *)pevent));
}

/*-- File event -------------------------------------------------*/
INT files_dump (char *pevent)
{
  ybos_feodb_file_dump((EQUIPMENT *)&equipment[2].name
		       , (DWORD *)pevent, gbl_run_number, "trigger");

  equipment[2].odb_out++; 

  /* return 0 because I handle the "event send" by myself */
  return 0;
}

