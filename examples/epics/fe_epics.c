/********************************************************************\

  Name:         fe_epics.c
  Created by:   Stefan Ritt

  Contents:     Example Slow Control Frontend program to work
                with Epics channel access to control a beamline.
  
  Revision 1.2  1998/10/12 12:18:59  midas
  Added Log tag in header


\********************************************************************/

#include <stdio.h>
#include "midas.h"
#include "cd_gen.h"
#include "chn_acc.h"

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
char *frontend_name = "BL Frontend";
/* The frontend file name, don't change it */
char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
BOOL frontend_call_loop = FALSE;

/* a frontend status page is displayed with this frequency in ms    */
INT display_period = 1000;

/* buffer size to hold events */
INT event_buffer_size = DEFAULT_EVENT_BUFFER_SIZE;

/*-- Equipment list ------------------------------------------------*/

/* device driver list */
DEVICE_DRIVER beam_driver[] = {
  { "Beamline",  chn_acc, 4 },
  { "" }
};

EQUIPMENT equipment[] = {

  { "Beamline",           /* equipment name */
    6, 0,                 /* event ID, trigger mask */
    "SYSTEM",             /* event buffer */
    EQ_SLOW,              /* equipment type */
    {0},                  /* event source */
    "FIXED",              /* format */
    TRUE,                 /* enabled */
    RO_RUNNING |
    RO_TRANSITIONS,       /* read when running and on transitions */
    60000,                /* read every 60 sec */
    0,                    /* stop run after this event limit */
    1,                    /* log history every event */
    "", "", "",
    cd_gen_read,          /* readout routine */
    cd_gen,               /* class driver main routine */
    beam_driver,          /* device driver list */
    NULL,                 /* init string */
  },

  { "" }
};


/*-- Dummy routines ------------------------------------------------*/

INT  poll_event(INT source[], INT count, BOOL test) {return 1;};
INT  interrupt_configure(INT cmd, INT source[], PTYPE adr) {return 1;};

/*-- Frontend Init -------------------------------------------------*/

INT frontend_init()
{
  return CM_SUCCESS;
}

/*-- Frontend Exit -------------------------------------------------*/

INT frontend_exit()
{
  return CM_SUCCESS;
}

/*-- Frontend Loop -------------------------------------------------*/

INT frontend_loop()
{
  return CM_SUCCESS;
}

/*-- Begin of Run --------------------------------------------------*/

INT begin_of_run(INT run_number, char *error)
{
  return CM_SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/

INT end_of_run(INT run_number, char *error)
{
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

/*------------------------------------------------------------------*/
