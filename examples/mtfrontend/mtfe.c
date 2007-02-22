/********************************************************************\

  Name:         mtfe.c
  Created by:   Stefan Ritt

  Contents:     Multi-threaded version of example frontend code.

  $Id: frontend.c 3384 2006-10-21 04:29:18Z amaudruz $

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "midas.h"
#include "msystem.h"
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

/* maximum event size produced by this frontend */
INT max_event_size = 1000000;

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
INT max_event_size_frag = 5 * 1024 * 1024;

/* buffer size to hold events */
INT event_buffer_size = 10 * 1000000;

/* number of channels */
#define N_ADC  4
#define N_TDC  4
#define N_SCLR 4

/* CAMAC crate and slots */
#define CRATE      0
#define SLOT_IO   23
#define SLOT_ADC   1
#define SLOT_TDC   2
#define SLOT_SCLR  3

/*-- Function declarations -----------------------------------------*/

INT frontend_init();
INT frontend_exit();
INT begin_of_run(INT run_number, char *error);
INT end_of_run(INT run_number, char *error);
INT pause_run(INT run_number, char *error);
INT resume_run(INT run_number, char *error);
INT frontend_loop();

INT read_trigger_event(char *pevent, INT off);
INT receive_trigger_event(char *pevent, INT off);
INT read_scaler_event(char *pevent, INT off);

void register_cnaf_callback(int debug);

/* inter-thread communication */
int rb_handle;
int stop_readout_thread;
int readout_thread(void *param);

/*-- Equipment list ------------------------------------------------*/

#undef USE_INT

EQUIPMENT equipment[] = {

   {"Trigger",               /* equipment name */
    {1, 0,                   /* event ID, trigger mask */
     "SYSTEM",               /* event buffer */
#ifdef USE_INT
     EQ_INTERRUPT,           /* equipment type */
#else
     EQ_POLLED,              /* equipment type */
#endif
     LAM_SOURCE(0, 0xFFFFFF),        /* event source crate 0, all stations */
     "MIDAS",                /* format */
     TRUE,                   /* enabled */
     RO_RUNNING |            /* read only when running */
     RO_ODB,                 /* and update ODB */
     500,                    /* poll for 500ms */
     0,                      /* stop run after this event limit */
     0,                      /* number of sub events */
     0,                      /* don't log history */
     "", "", "",},
    receive_trigger_event,   /* readout routine */
    },

   {"Scaler",                /* equipment name */
    {2, 0,                   /* event ID, trigger mask */
     "SYSTEM",               /* event buffer */
     EQ_PERIODIC | EQ_MANUAL_TRIG,   /* equipment type */
     0,                      /* event source */
     "MIDAS",                /* format */
     TRUE,                   /* enabled */
     RO_RUNNING | RO_TRANSITIONS |   /* read when running and on transitions */
     RO_ODB,                 /* and update ODB */
     10000,                  /* read every 10 sec */
     0,                      /* stop run after this event limit */
     0,                      /* number of sub events */
     0,                      /* log history */
     "", "", "",},
    read_scaler_event,       /* readout routine */
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
   /* hardware initialization */

   cam_init();
   cam_crate_clear(CRATE);
   cam_crate_zinit(CRATE);

   /* enable LAM in IO unit */
   camc(CRATE, SLOT_IO, 0, 26);

   /* enable LAM in crate controller */
   cam_lam_enable(CRATE, SLOT_IO);

   /* reset external LAM Flip-Flop */
   camo(CRATE, SLOT_IO, 1, 16, 0xFF);
   camo(CRATE, SLOT_IO, 1, 16, 0);

   /* register CNAF functionality from cnaf_callback.c with debug output */
   register_cnaf_callback(1);

   /* create ring buffer for inter-thread data transfer */
   rb_create(max_event_size * 10, max_event_size, &rb_handle);

   /* create hardware reading thread */
   stop_readout_thread = 0;
   ss_thread_create(readout_thread, NULL);

   return SUCCESS;
}

/*-- Frontend Exit -------------------------------------------------*/

INT frontend_exit()
{
   stop_readout_thread = 1;
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
   int i, status;

   for (i = 0; i < count; i++) {
      status = rb_get_rp(rb_handle, NULL, 10);

      if (status != DB_TIMEOUT)
         if (!test)
            return rb_handle;
   }

   return 0;
}

/*-- Interrupt configuration ---------------------------------------*/

INT interrupt_configure(INT cmd, INT source, POINTER_T adr)
{
   switch (cmd) {
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

/*-- Readout thread ------------------------------------------------*/

extern BOOL interrupt_enabled;

int readout_thread(void *param)
{
   int status;
   DWORD lam, data_size;
   EVENT_HEADER *pevent;
   void *p;

   do {
      if (interrupt_enabled) {
        
         cam_lam_read(LAM_SOURCE(0, 0xFFFFFF), &lam);
         if (lam & LAM_SOURCE_STATION(LAM_SOURCE(0, 0xFFFFFF))) {

            /* obtain buffer space */
            status = rb_get_wp(rb_handle, &p, 100);
            pevent = (EVENT_HEADER *)p;

            if (status == DB_SUCCESS) {
               /* compose event header */
               pevent->event_id = equipment[0].info.event_id;
               pevent->trigger_mask = equipment[0].info.trigger_mask;
               pevent->time_stamp = ss_time();

               /* call user readout routine */
               data_size = read_trigger_event(( char *) (pevent + 1), 0);
               pevent->data_size = data_size;

               if (data_size > 0)
                  /* put event into ring buffer */
                  rb_increment_wp(rb_handle, sizeof(EVENT_HEADER) + data_size);
            }
         }


      } else // run_state == STATE_RUNNING
        ss_sleep(10);

   } while (!stop_readout_thread);

   return 0;
}

/*-- Receive event from readout thread ------------------------------*/

INT receive_trigger_event(char *pevent, INT off)
{
   int status, data_size;
   EVENT_HEADER *prb;
   void *p;

   status = rb_get_rp(rb_handle, &p, 10);
   prb = (EVENT_HEADER *)p;
   if (status == DB_TIMEOUT)
      return 0;

   data_size = prb->data_size;

   memcpy(pevent, prb+1, data_size);

   /* copy trigger mask and time stamp, use serial from mfe framework */
   TRIGGER_MASK(pevent) = prb->trigger_mask;
   TIME_STAMP(pevent) = prb->time_stamp;

   /* free up space in ring buffer */
   rb_increment_rp(rb_handle, sizeof(EVENT_HEADER) + data_size);
   return data_size;
}

/*-- Event readout -------------------------------------------------*/

INT read_trigger_event(char *pevent, INT off)
{
   WORD *pdata, a;
   INT q, timeout;

   /* init bank structure */
   bk_init(pevent);

   /* create structured ADC0 bank */
   bk_create(pevent, "ADC0", TID_WORD, &pdata);

   /* wait for ADC conversion */
   for (timeout = 100; timeout > 0; timeout--) {
      camc_q(CRATE, SLOT_ADC, 0, 8, &q);
      if (q)
         break;
   }
   if (timeout == 0)
      ss_printf(0, 10, "No ADC gate!");

   /* use following code to read out real CAMAC ADC */
   /*
      for (a=0 ; a<N_ADC ; a++)
      cami(CRATE, SLOT_ADC, a, 0, pdata++);
    */

   /* Use following code to "simulate" data */
   for (a = 0; a < N_ADC; a++)
      *pdata++ = rand() % 1024;

   /* clear ADC */
   camc(CRATE, SLOT_ADC, 0, 9);

   bk_close(pevent, pdata);

   /* create variable length TDC bank */
   bk_create(pevent, "TDC0", TID_WORD, &pdata);

   /* use following code to read out real CAMAC TDC */
   /*
      for (a=0 ; a<N_TDC ; a++)
      cami(CRATE, SLOT_TDC, a, 0, pdata++);
    */

   /* Use following code to "simulate" data */
   for (a = 0; a < N_TDC; a++)
      *pdata++ = rand() % 1024;

   /* clear TDC */
   camc(CRATE, SLOT_TDC, 0, 9);

   bk_close(pevent, pdata);

   /* clear IO unit LAM */
   camc(CRATE, SLOT_IO, 0, 10);

   /* clear LAM in crate controller */
   cam_lam_clear(CRATE, SLOT_IO);

   /* reset external LAM Flip-Flop */
   camo(CRATE, SLOT_IO, 1, 16, 0xFF);
   camo(CRATE, SLOT_IO, 1, 16, 0);

   //ss_sleep(10);

   return bk_size(pevent);
}

/*-- Scaler event --------------------------------------------------*/

INT read_scaler_event(char *pevent, INT off)
{
   DWORD *pdata, a;

   /* init bank structure */
   bk_init(pevent);

   /* create SCLR bank */
   bk_create(pevent, "SCLR", TID_DWORD, &pdata);

   /* read scaler bank */
   for (a = 0; a < N_SCLR; a++)
      cam24i(CRATE, SLOT_SCLR, a, 0, pdata++);

   bk_close(pevent, pdata);

   return bk_size(pevent);
}
