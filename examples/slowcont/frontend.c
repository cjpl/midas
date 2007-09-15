/********************************************************************\

  Name:         frontend.c
  Created by:   Stefan Ritt

  Contents:     Example Slow Control Frontend program. Defines two
                slow control equipments, one for a HV device and one
                for a multimeter (usually a general purpose PC plug-in
                card with A/D inputs/outputs. As a device driver,
                the "null" driver is used which simulates a device
                without accessing any hardware. The used class drivers
                cd_hv and cd_multi act as a link between the ODB and
                the equipment and contain some functionality like
                ramping etc. To form a fully functional frontend,
                the device driver "null" has to be replaces with
                real device drivers.

  $Id$

\********************************************************************/

#include <stdio.h>
#include "midas.h"
#include "class/hv.h"
#include "class/multi.h"
#include "device/nulldev.h"
#include "bus/null.h"

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
char *frontend_name = "SC Frontend";
/* The frontend file name, don't change it */
char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
BOOL frontend_call_loop = FALSE;

/* a frontend status page is displayed with this frequency in ms    */
INT display_period = 1000;

/* maximum event size produced by this frontend */
INT max_event_size = 10000;

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
INT max_event_size_frag = 5 * 1024 * 1024;

/* buffer size to hold events */
INT event_buffer_size = 10 * 10000;

/*-- Equipment list ------------------------------------------------*/

/* device driver list */
DEVICE_DRIVER hv_driver[] = {
   {"Dummy Device", nulldev, 16, null},
   {""}
};

DEVICE_DRIVER multi_driver[] = {
   {"Input", nulldev, 2, null, DF_INPUT},
   {"Output", nulldev, 2, null, DF_OUTPUT},
   {""}
};


EQUIPMENT equipment[] = {

   {"HV",                       /* equipment name */
    {3, 0,                       /* event ID, trigger mask */
     "SYSTEM",                  /* event buffer */
     EQ_SLOW,                   /* equipment type */
     0,                         /* event source */
     "FIXED",                   /* format */
     TRUE,                      /* enabled */
     RO_RUNNING | RO_TRANSITIONS,        /* read when running and on transitions */
     60000,                     /* read every 60 sec */
     0,                         /* stop run after this event limit */
     0,                         /* number of sub events */
     10000,                         /* log history every event */
     "", "", ""} ,
    cd_hv_read,                 /* readout routine */
    cd_hv,                      /* class driver main routine */
    hv_driver,                  /* device driver list */
    NULL,                       /* init string */
    },

   {"Environment",              /* equipment name */
    {4, 0,                      /* event ID, trigger mask */
     "SYSTEM",                  /* event buffer */
     EQ_SLOW,                   /* equipment type */
     0,                         /* event source */
     "FIXED",                   /* format */
     TRUE,                      /* enabled */
     RO_RUNNING | RO_TRANSITIONS,        /* read when running and on transitions */
     60000,                     /* read every 60 sec */
     0,                         /* stop run after this event limit */
     0,                         /* number of sub events */
     1,                         /* log history every event */
     "", "", ""} ,
    cd_multi_read,              /* readout routine */
    cd_multi,                   /* class driver main routine */
    multi_driver,               /* device driver list */
    NULL,                       /* init string */
    },

   {""}
};


/*-- Dummy routines ------------------------------------------------*/

INT poll_event(INT source[], INT count, BOOL test)
{
   return 1;
};
INT interrupt_configure(INT cmd, INT source[], POINTER_T adr)
{
   return 1;
};

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

/*-- Resume Run ----------------------------------------------------*/

INT resume_run(INT run_number, char *error)
{
   return CM_SUCCESS;
}

/*------------------------------------------------------------------*/
