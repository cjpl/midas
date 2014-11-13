/********************************************************************\

  Name:         mtfe.c
  Created by:   Stefan Ritt

  Contents:     Multi-threaded frontend example.

  $Id: frontend.c 4089 2007-11-27 07:28:17Z ritt@PSI.CH $

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "midas.h"
#include "msystem.h"
#include "mcstd.h"

/* make frontend functions callable from the C framework */
#ifdef __cplusplus
extern "C" {
#endif

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
const char *frontend_name = "Sample Frontend";
/* The frontend file name, don't change it */
const char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
BOOL frontend_call_loop = FALSE;

/* a frontend status page is displayed with this frequency in ms */
INT display_period = 3000;

/* maximum event size produced by this frontend */
INT max_event_size = 10000;

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
INT max_event_size_frag = 5 * 1024 * 1024;

/* buffer size to hold events */
INT event_buffer_size = 100 * 10000;

/*-- Function declarations -----------------------------------------*/

INT frontend_init();
INT frontend_exit();
INT begin_of_run(INT run_number, char *error);
INT end_of_run(INT run_number, char *error);
INT pause_run(INT run_number, char *error);
INT resume_run(INT run_number, char *error);
INT frontend_loop();

INT trigger_thread(void *param);
INT read_scaler_event(char *pevent, INT off);

INT poll_event(INT source, INT count, BOOL test);
INT interrupt_configure(INT cmd, INT source, POINTER_T adr);
   
/*-- Equipment list ------------------------------------------------*/

EQUIPMENT equipment[] = {

   {"Trigger",               /* equipment name */
    {1, 0,                   /* event ID, trigger mask */
     "SYSTEM",               /* event buffer */
     EQ_INTERRUPT,           /* equipment type */
     0,                      /* event source (not used) */
     "MIDAS",                /* format */
     TRUE,                   /* enabled */
     RO_RUNNING,             /* read only when running */
     500,                    /* poll for 500ms */
     0,                      /* stop run after this event limit */
     0,                      /* number of sub events */
     0,                      /* don't log history */
     "", "", "",},
    NULL,                    /* readout routine */
    },

   {""}
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
/* Polling is not used in this example */
{
   return 0;
}

static int _trigger_enabled = 0;
static int _trigger_readout_active = 0;
static int _trigger_readout_stopped = 0;
static int _stop_all_threads = 0;

/*-- Interrupt configuration ---------------------------------------*/

INT interrupt_configure(INT cmd, INT source, POINTER_T adr)
{
   switch (cmd) {
      case CMD_INTERRUPT_ENABLE:
         _trigger_enabled = TRUE;
         break;
      case CMD_INTERRUPT_DISABLE:
         _trigger_enabled = FALSE;
         /* wait until current readout has been finished */
         while (_trigger_readout_active) 
            ss_sleep(10);
         break;
      case CMD_INTERRUPT_ATTACH:
         /* create readout thread */
         if (source == 0) // equipment[0]
            ss_thread_create(trigger_thread, NULL);
         break;
      case CMD_INTERRUPT_DETACH:
         _stop_all_threads = TRUE;
         /* wait until readout thread has been stopped */
         while (!_trigger_readout_stopped)
            ss_sleep(10);
         break;
   }
   return SUCCESS;
}

/*-- Event readout -------------------------------------------------*/

INT trigger_thread(void *param)
{
   EVENT_HEADER *pevent;
   WORD *pdata, *padc;
   int i, status;
   INT rbh;   
   
   /* Initialize hardware here ... */
   
   /* Create ring buffer for inter-thread data exchange */
   rb_create(event_buffer_size, max_event_size, &rbh);
   
   /* Tell framework to receive events from this buffer */
   set_event_rb(rbh);
   
   while (!_stop_all_threads) {
      /* obtain buffer space */
      status = rb_get_wp(rbh, (void **)&pevent, 0);
      if (_stop_all_threads)
         break;
      if (status == DB_TIMEOUT) {
         ss_sleep(10);
         continue;
      }
      if (status != DB_SUCCESS)
         break;

      if (_trigger_enabled) {
         
         /* indicate activity to interrupt_configure() */
         _trigger_readout_active = TRUE;
         
         /* check for new event (poll) */
         status = ss_sleep(10); // for this demo, just sleep a bit
         
         if (status) { // if event available, read it out

            if (_stop_all_threads)
               break;
            
            bm_compose_event(pevent, 1, 0, 0, equipment[0].serial_number++);
            //            printf("%d ", equipment[0].serial_number);
            pdata = (WORD *)(pevent + 1);
            
            /* init bank structure */
            bk_init(pdata);
            
            /* create ADC0 bank */
            bk_create(pdata, "ADC0", TID_WORD, &padc);

            /* just put in some random numbers */
            for (i=0 ; i<10 ; i++)
               *padc++ = rand() % 1024;
            
            bk_close(pdata, padc);
            
            pevent->data_size = bk_size(pdata);
            
            /* send event to ring buffer */
            rb_increment_wp(rbh, sizeof(EVENT_HEADER) + pevent->data_size);
         }
         
         _trigger_readout_active = FALSE;
         
      } else // _trigger_enabled
         ss_sleep(10);
   }
   
   _trigger_readout_active = FALSE;
   _trigger_readout_stopped = TRUE;
   return 0;
}
