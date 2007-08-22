/********************************************************************\

  Name:         feccusb.cxx
  Created by:   K.Olchanski

  Contents:     MIDAS Frontend for the CCUSB CAMAC-USB interface

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "midas.h"
#include "mcstd.h"
#include "ccusb.h"

/* make frontend functions callable from the C framework */
#ifdef __cplusplus
extern "C" {
#endif

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
char *frontend_name = "feccusb";
/* The frontend file name, don't change it */
char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
BOOL frontend_call_loop = FALSE;

/* a frontend status page is displayed with this frequency in ms */
INT display_period = 0;

#define MAX_EVENT (30*1024)

/* maximum event size produced by this frontend */
  INT max_event_size = (MAX_EVENT+1024);

/* buffer size to hold events */
  INT event_buffer_size = (10*MAX_EVENT+1024);

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
INT max_event_size_frag = 0;

/*-- Function declarations -----------------------------------------*/

INT frontend_init();
INT frontend_exit();
INT begin_of_run(INT run_number, char *error);
INT end_of_run(INT run_number, char *error);
INT pause_run(INT run_number, char *error);
INT resume_run(INT run_number, char *error);
INT frontend_loop();

  //INT poll_trigger_event(INT count, PTYPE test);
INT interrupt_configure(INT cmd, PTYPE adr);
INT read_event(char *pevent, INT off);

#define CRATE        0

/* Station addresses */

/* Slot positions */
//#define LAM_N       20
#define ADC1_N      20
//#define TDC1_N      18
#define SCL1_N      12
//#define DISPLAY_N    1

/* Number of slots */
#define N_ADC        1
#define N_TDC        1
#define N_SCL        1

/*-- Equipment list ------------------------------------------------*/

//#define USE_INT 1

EQUIPMENT equipment[] = {

  { "USBCAMAC",           /* equipment name */
    1, 0,                 /* event ID, trigger mask */
    "SYSTEM",             /* event buffer */
    EQ_PERIODIC,          /* equipment type */
    0,                    /* event source */
    "MIDAS",              /* format */
    TRUE,                 /* enabled */
    RO_RUNNING,           /* read only when running */
    10,                   /* poll time, ms */
    0,
    0,                    /* stop run after this event limit */
    0,                    /* don't log history */
    "", "", "",
    read_event,   /* readout routine */
    NULL,                 /* init string */
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

static int ccusbIsBad = 1;

int ccusbInit()
{
  WORD wbuf[1000];
  DWORD dwbuf[1000];
  int status;

  ccusbIsBad = 1;

  cam_exit();

  status = cam_init();
  if (status != SUCCESS)
    {
      al_trigger_alarm("ccusb_failure", "Cannot initialize CCUSB CAMAC interface", "Alarm", "", AT_INTERNAL);
      return SS_IO_ERROR;
    }

  ccusbIsBad = 0;

  al_reset_alarm("ccusb_failure");

  int version = ccusb_readCamacReg(ccusb_getCrateStruct(CRATE), CCUSB_VERSION);
  version &= 0xFFFF;
  printf("CCUSB firmware revision 0x%04x\n", version);

  int minVersion = 0x402;
  if (version < minVersion)
    {
      printf("Error: CCUSB firmware version 0x%04x has broken support for the CAMAC readout list function required by this program. Should have version 0x%04x or newer. Bye!\n", version, minVersion);
      exit(1);
    }

  if (1)
    {
      // setup the main readout list

      ccusb_startStack();

#ifdef ADC1_N
      /*---- Read & Clear ADCs ----*/
      for (int i=0 ;i<12; i++)
        cam16i(CRATE, ADC1_N, i, 2, wbuf);
#endif
      
#ifdef LAM_N
      /* Clear data, clear LAM*/
      camc(CRATE, LAM_N, 0, 9);
#endif
    }
  
  ccusb_writeDataStack(ccusb_getCrateStruct(CRATE), 1);

  if (1)
    {
      // setup the periodic "scaler" readout list

      ccusb_startStack();

#ifdef SCL1_N
      for (int i=0; i<N_SCL; i++)
	{
          for (int j=0; j<6; j++)
            cam24i(CRATE, SCL1_N+i, j, 2, dwbuf);
	}
#endif
      
      ccusb_writeScalerStack(ccusb_getCrateStruct(CRATE), 1);
    }

  printf("frontend_init: Done!\n");

  ccusb_writeCamacReg(ccusb_getCrateStruct(CRATE), 1, 0); // usb buffering control
  ccusb_writeCamacReg(ccusb_getCrateStruct(CRATE), 2, 0); // readout delay
  ccusb_writeCamacReg(ccusb_getCrateStruct(CRATE), 3, 0); // scaler stack control

  ccusb_writeCamacReg(ccusb_getCrateStruct(CRATE), CCUSB_MODE,     0);
  ccusb_writeCamacReg(ccusb_getCrateStruct(CRATE), CCUSB_USBSETUP, 0);

  //ccusb_writeCamacReg(ccusb_getCrateStruct(CRATE), CCUSB_MODE,     1<<5 | 1<<3);
  //ccusb_writeCamacReg(ccusb_getCrateStruct(CRATE), CCUSB_USBSETUP, 0x203);

  ccusb_writeCamacReg(ccusb_getCrateStruct(CRATE), 3, 0x20000); // scaler stack period

  ccusb_status(ccusb_getCrateStruct(CRATE));

  return SUCCESS;
}

INT frontend_init()
{
  int status;

  /* register CAMAC RPC functions */
  register_cnaf_callback(1);

  status = ccusbInit();

  // alsways return success, even if CAMAC
  // interface is missing - we will look
  // for it again and initialize it
  // at the begin-of-run time.

  return SUCCESS;
}

/*-- Frontend Exit -------------------------------------------------*/

INT frontend_exit()
{
  cam_exit();
  return CM_SUCCESS;
}

/*-- Begin of Run --------------------------------------------------*/

INT begin_of_run(INT run_number, char *error)
{
  int n;

  printf("begin_of_run %d\n", run_number);

  if (ccusbIsBad)
    {
      int status = ccusbInit();
      if (status != SUCCESS)
        return SS_IO_ERROR;
    }

  cam_inhibit_clear(CRATE);

#ifdef ADC1_N
  /* Disable LAM & Clear ADC */
  for (n=ADC1_N ; n< ADC1_N + N_ADC; n++) {
    camc(CRATE, n, 0, 24);
    camc(CRATE, n, 0, 9);
  }
#endif

#ifdef TDC1_N
  /* Disable LAM & Clear TDC */
  for (n=TDC1_N ; n< TDC1_N + N_TDC; n++) {
    camc(CRATE, n, 0, 24);
    camc(CRATE, n, 0, 9); 
  }
#endif

#ifdef SCL1_N  
  /* clear scalers */
  for (n=SCL1_N; n< SCL1_N + N_SCL; n++) {
    camc_sa(CRATE, n, 0, 9, 6); 
  }
#endif

#if 1
#ifdef LAM_N
  /* Enable THE LAM station LAM_N */
  camc(CRATE, LAM_N, 0, 26);

  /* Enable CC LAM */
  cam_lam_enable(CRATE, LAM_N);

  /* Clear LAM */
  camc(CRATE, LAM_N, 0, 9);
#endif
#endif

  int status = ccusb_AcqStart(ccusb_getCrateStruct(CRATE));

  if (status != CCUSB_SUCCESS)
    {
      fprintf(stderr,"Error: Cannot start the CCUSB CAMAC interface\n");

      if (!ccusbIsBad)
        {
          ccusbIsBad = 1;
          al_trigger_alarm("ccusb_failure", "Cannot talk to the CCUSB CAMAC interface", "Alarm", "", AT_INTERNAL);
        }

      ccusbIsBad = 1;
      return SS_IO_ERROR;
    }

  printf("begin_of_run done!\n");

  return CM_SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/

INT end_of_run(INT run_number, char *error)
{
  printf("end_of_run %d\n", run_number);

  if (ccusbIsBad)
    return SUCCESS;

  int status = ccusb_AcqStop(ccusb_getCrateStruct(CRATE));

  if (status != CCUSB_SUCCESS)
    {
      fprintf(stderr,"Error: Cannot stop the CCUSB CAMAC interface\n");

      if (!ccusbIsBad)
        {
          /* beware of recursive call to end_of_run() !!! */
          ccusbIsBad = 1;
          al_trigger_alarm("ccusb_failure", "Cannot talk to the CCUSB CAMAC interface", "Alarm", "", AT_INTERNAL);
        }

      ccusbIsBad = 1;
    }

  ccusb_flush(ccusb_getCrateStruct(CRATE));

#ifdef LAM_N
  /* Disable CC LAM */
  cam_lam_disable(CRATE, LAM_N);
#endif

  cam_inhibit_set(CRATE);

  printf("end_of_run done!\n");

  return CM_SUCCESS;
}

/*-- Pause Run -----------------------------------------------------*/

INT pause_run(INT run_number, char *error)
{
  if (ccusbIsBad)
    return SUCCESS;

  ccusb_AcqStop(ccusb_getCrateStruct(CRATE));

  return CM_SUCCESS;
}

/*-- Resume Run ----------------------------------------------------*/

INT resume_run(INT run_number, char *error)
{
  if (ccusbIsBad)
    return SUCCESS;

  ccusb_AcqStart(ccusb_getCrateStruct(CRATE));

  return CM_SUCCESS;
}

/*-- Frontend Loop -------------------------------------------------*/

INT frontend_loop()
{
  assert(!"we should not come here!");
  return SUCCESS;
}

/*------------------------------------------------------------------*/

/********************************************************************\
  
  Readout routines for different events

\********************************************************************/

/*-- Trigger event routines ----------------------------------------*/

extern "C" INT poll_event(INT source, INT count, BOOL test)
{
  assert(!"we should not come here!");
  return FALSE;
}

/*-- Interrupt configuration for trigger event ---------------------*/

INT interrupt_configure(INT cmd, PTYPE adr)
{
  assert(!"we should not come here!");
  return CM_SUCCESS;
}

/*-- Event readout -------------------------------------------------*/
INT read_event(char *pevent, INT off)
{
  WORD *pbkdat;
  int timeout_ms = 5000;

  //if (ccusbIsBad)
  //  return 0;

  //printf("read_event!\n");

  bk_init32(pevent);
  bk_create(pevent, "CCUS", TID_WORD, &pbkdat);

  int rd = ccusb_readData(ccusb_getCrateStruct(CRATE), (char*)pbkdat, MAX_EVENT, timeout_ms);

  if (rd == CCUSB_TIMEOUT)
    {
      fprintf(stderr,"Error: Data timeout from CCUSB CAMAC interface\n");
      return 0;
    }

  if (rd < 0)
    {
      fprintf(stderr,"Error: Cannot read data from CCUSB CAMAC interface\n");
      al_trigger_alarm("ccusb_failure", "Data read error from CCUSB CAMAC interface", "Alarm", "", AT_INTERNAL);
      ccusbIsBad = 1;
      return 0;
    }

  if (0)
    printf("read data: %d\n", rd);

  if (0)
    {
      for (int i=0; i<rd/2 && i<100; i++)
        {
          uint16_t *b16 = (uint16_t*)pbkdat;
          printf("data %3d: 0x%04x\n", i, b16[i]);
        }
    }

  if (rd > 0)
    {
      pbkdat += (rd+1)/2;
      bk_close(pevent, pbkdat);

      return bk_size(pevent);
    }

  return 0;
}

// end file
