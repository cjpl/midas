/********************************************************************\

  Name:         frontend.c
  Created by:   Stefan Ritt

  Contents:     Example Slow Control Frontend for equipment control
                through EPICS channel access.

  $Id$

\********************************************************************/

#include <stdio.h>
#include "midas.h"
#include "class/generic.h"
#include "device/epics_ca.h"

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
char *frontend_name = "feEpics";
/* The frontend file name, don't change it */
char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
BOOL frontend_call_loop = TRUE;

/* a frontend status page is displayed with this frequency in ms    */
INT display_period = 2000;

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
INT max_event_size_frag = 5 * 1024 * 1024;

/* maximum event size produced by this frontend */
INT max_event_size = 10000;

/* buffer size to hold events */
INT event_buffer_size = 10 * 10000;

/*-- Equipment list ------------------------------------------------*/

/* 
The following statement allocates 20 channels for the beamline
control through the epics channel access device driver. The 
EPICS channel names are stored under 
   
  /Equipment/Beamline/Settings/Devices/Beamline

while the channel names as the midas slow control sees them are
under 

  /Equipment/Beamline/Settings/Names

An example set of channel names is saved in the triumf.odb file
in this directory and can be loaded into the ODB with the odbedit
command

  load beamline.xml

before the frontend is started. The CMD_SET_LABEL statement 
actually defines who determines the label name. If this flag is
set, the CMD_SET_LABEL command in the device driver is disabled,
therefore the label is taken from EPICS, otherwise the label is
taken from MIDAS and set in EPICS.

The same can be done with the demand values. If the command
CMD_SET_DEMAND is disabled, the demand value is always determied
by EPICS.
*/

/* device driver list */
DEVICE_DRIVER epics_driver[] = {
  {"Beamline", epics_ca, 20, NULL},  /* disable CMD_SET_LABEL */
  {""}
};

EQUIPMENT equipment[] = {

   {"Beamline",                 /* equipment name */
    3, 0,                       /* event ID, trigger mask */
    "SYSTEM",                   /* event buffer */
    EQ_SLOW,                    /* equipment type */
    0,                          /* event source */
    "FIXED",                    /* format */
    TRUE,                       /* enabled */
    RO_RUNNING | RO_TRANSITIONS,        /* read when running and on transitions */
    60000,                      /* read every 60 sec */
    0,                          /* stop run after this event limit */
    0,                          /* number of sub events */
    1,                          /* log history every event */
    "", "", "",
    cd_gen_read,                /* readout routine */
    cd_gen,                     /* class driver main routine */
    epics_driver,               /* device driver list */
    NULL,                       /* init string */
    },

   {""}
};

/*-- Dummy routines ------------------------------------------------*/

INT poll_event(INT source[], INT count, BOOL test)
{
   return 1;
};
INT interrupt_configure(INT cmd, INT source[], PTYPE adr)
{
   return 1;
};

/*-- Frontend Init -------------------------------------------------*/

INT frontend_init()
{
   /* anable/disable certain command in device driver */


   return CM_SUCCESS;
}

/*-- Frontend Exit -------------------------------------------------*/

INT frontend_exit()
{
   return CM_SUCCESS;
}

/*-- Frontend Loop -------------------------------------------------*/
/* Issue a watchdog counter every second for the Epics world
   for R/W access control.
   This counter will appear in the measured variable under index 19.
*/
INT frontend_loop()
{
  int status, size;
  static HNDLE hDB, hWatch=0, hRespond;
  static DWORD watchdog_time=0;
  static float  dog=0.f, cat=0.f;
  
  /* slow down frontend not to eat all CPU cycles */
  /* ss_sleep(50); */
  cm_yield(50); /* 15Feb05 */
  
  if (ss_time() - watchdog_time > 1)
    {
      watchdog_time = ss_time();
      if (!hWatch)
	{
	  cm_get_experiment_database(&hDB, NULL);
	  status = db_find_key(hDB, 0, "/equipment/Beamline/variables/demand", &hWatch);
	  status = db_find_key(hDB, 0, "/equipment/Beamline/variables/measured", &hRespond);
	  if (status != DB_SUCCESS) {
	    cm_msg(MERROR, "frontend_loop", "key not found");
	    return FE_ERR_HW;
	  }
	}
      if (hWatch) {
	/* Check if Epics alive */
	size = sizeof(float);
	db_get_data_index(hDB, hRespond, &cat, &size, 19, TID_FLOAT);
	if (abs(cat - dog) > 10.f)
	  cm_msg(MINFO,"feEpics","R/W Access to Epics is in jeopardy!");
	
	db_set_data_index(hDB, hWatch, &dog, sizeof(float), 19, TID_FLOAT);
      }
      if (!((INT)++dog % 100)) dog = 0.f;
    }
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
