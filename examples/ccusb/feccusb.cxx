/********************************************************************\
  Name:         feccusb.cxx
  Created by:   Pierre-Andre Amaudruz

  Contents:     Experiment specific readout code (user part) of
                Midas frontend. This example provide a template 
                for data acquisition using the CC-USB from Wiener
\********************************************************************/

#include <libxxusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
//#include <cstdlib>
#include <string.h>
#include "midas.h"
#include "ccusb.h"
#include "OdbCCusb.h"

/* make frontend functions callable from the C framework */
#ifdef __cplusplus
extern "C" {
#endif

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
char const *frontend_name = "feccusb";
/* The frontend file name, don't change it */
char const *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
BOOL frontend_call_loop = FALSE;

/* a frontend status page is displayed with this frequency in ms */
INT display_period = 000;

/* maximum event size produced by this frontend */
INT max_event_size = 10000;

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
INT max_event_size_frag = 5 * 1024 * 1024;

/* buffer size to hold events */
INT event_buffer_size = 100 * 10000;

/* number of channels */
#define N_ADC  4
#define N_TDC  4
#define N_SCLR 4

/* CAMAC crate and slots */
#define SLOT_IO   23
#define SLOT_ADC  16
#define SLOT_TDC  15
#define SLOT_SCLR  3

xxusb_device_type devices[100];
struct usb_device *dev;
usb_dev_handle *udev;       // Device Handle

HNDLE hDB, hSetCC;
CCUSB_SETTINGS tscc;
/*-- Function declarations -----------------------------------------*/

INT frontend_init();
INT frontend_exit();
INT begin_of_run(INT run_number, char *error);
INT end_of_run(INT run_number, char *error);
INT pause_run(INT run_number, char *error);
INT resume_run(INT run_number, char *error);
INT frontend_loop();

INT read_trigger_event(char *pevent, INT off);
INT read_scaler_event(char *pevent, INT off);

INT poll_event(INT source, INT count, BOOL test);
INT interrupt_configure(INT cmd, INT source, POINTER_T adr);

/*-- Equipment list ------------------------------------------------*/

EQUIPMENT equipment[] = {

   {"Trigger",               /* equipment name */
    {1, 0,                   /* event ID, trigger mask */
     "SYSTEM",               /* event buffer */
     EQ_PERIODIC,            /* equipment type */
     LAM_SOURCE(0, 0xFFFFFF),        /* event source crate 0, all stations */
     "MIDAS",                /* format */
     TRUE,                   /* enabled */
     RO_RUNNING,             /* read only when running */
     100,                    /* poll for 100ms */
     0,                      /* stop run after this event limit */
     0,                      /* number of sub events */
     0,                      /* don't log history */
     "", "", "",},
    read_trigger_event,      /* readout routine */
    },

   {"Scaler",                /* equipment name */
    {2, 0,                   /* event ID, trigger mask */
     "SYSTEM",               /* event buffer */
     EQ_PERIODIC,            /* equipment type */
     0,                      /* event source */
     "MIDAS",                /* format */
     TRUE,                   /* enabled */
     RO_RUNNING | RO_TRANSITIONS | /* read when running and on transitions */
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
// 
//----------------------------------------------------------------
// callback when the key in ODB is touched (see db_open_record below)
void seq_callback(INT hDB, INT hseq, void *info)
{
  KEY key;
  db_get_key(hDB, hseq, &key);
  printf("odb ... %s touched (%d)\n", key.name, hseq);
}

//
//----------------------------------------------------------------
//
int ccUsbFlush(void) {
  short IntArray [10000];  //for FIFOREAD
  
  int ret=1;
  int k=0;
  while(ret>0) {
    ret = xxusb_bulk_read(udev, IntArray, 8192, 100);
    if(ret>0) {
      // printf("drain loops: %i (result %i)\n ",k,ret);                                     
      k++;
      if (k>100) ret=0;
    }
  }
  return (IntArray[0] & 0x8000) ? 0 : IntArray[0];
}

/*-- Frontend Init -------------------------------------------------*/
// Executed once at the start of the applicatoin
INT frontend_init() {

  int size, status;
  char set_str[80];
  CCUSB_SETTINGS_STR(ccusb_settings_str);

  cm_get_experiment_database(&hDB, &status);
  
  // Map /equipment/<eq_name>/settings 
  sprintf(set_str, "/Equipment/Trigger/Settings");
  
  // create the ODB Settings if not existing under Equipment/<eqname>/
  // If already existing, don't change anything 
  status = db_create_record(hDB, 0, set_str, strcomb(ccusb_settings_str));
  if (status != DB_SUCCESS) {
    cm_msg(MERROR, "ccusb", "cannot create record (%s)", set_str);
    return FE_ERR_ODB;
  }
  
  // Get the equipment/<eqname>/Settings key
  status = db_find_key(hDB, 0, set_str, &hSetCC);
  
  // Enable hot-link on /Settings of the equipment
  // Anything changed under /Settings will be triggering the callback (above)
  size = sizeof(CCUSB_SETTINGS);
  if ((status = db_open_record(hDB, hSetCC, &tscc, size, MODE_READ
			       , seq_callback, NULL)) != DB_SUCCESS) {
    cm_msg(MERROR, "ccusb", "cannot open record (%s)", set_str);
    return CM_DB_ERROR;
  }
  
  // hardware initialization   
  //Find XX_USB devices and open the first one found
  xxusb_devices_find(devices);
  dev = devices[0].usbdev;
  udev = xxusb_device_open(dev);

  // Make sure CC_USB opened OK
  if(!udev) {
    cm_msg(MERROR, "ccusb", "Failed to Open CC_USB");
    return FE_ERR_HW;
  } 

  // CAMAC CLEAR
  CAMAC_C(udev);
  // CAMAC Z
  CAMAC_Z(udev);
  
  /* print message and return FE_ERR_HW if frontend should not be started */
  cm_msg(MINFO, "ccusb", "CC-SUB ready");
  return SUCCESS;
}

/*-- Frontend Exit -------------------------------------------------*/
INT frontend_exit()
{
  // Close device
  xxusb_device_close(udev);
  return SUCCESS;
}

/*-- Begin of Run --------------------------------------------------*/
// Done on every begin of run 
INT begin_of_run(INT run_number, char *error) {

  //
  // Construct the CAMAC list to be loaded into the Stack
  long int stack[100];
  int i, ret;
  int q, x;
  long d24;

  // Create the CAMAC list to be performed on each LAM
  StackCreate(stack);

  // StackXxx(), MRAD16, WMARKER, etc and macros are defined in 
  // in ccusb.h
  //                   n      a   f  inc_A_counter
  StackFill(MRAD16, SLOT_ADC, 0 , 2, 12, stack);  
  StackFill(WMARKER, 0, 0, 0, 0xFEED, stack);
  StackClose(stack);

  // Debugging Check stack before writing
  printf("stack[0]:%ld\n", stack[0]);
  for (i = 0; i < stack[0]+1; i++) {
    printf("stack[%i]=0x%lx\n", i, stack[i]);
  }

  // Load stack into CC-USB
  ret = xxusb_stack_write(udev, 2, stack);
  if (ret<0) {
    printf("err on stack_write:%d\n", ret);
  } else {
    printf("Nbytes(ret) from stack_write:%d\n", ret);
    for (i = 0; i < (ret/2); i++) {
      printf("Wstack[%i]=0x%lx\n", i, stack[i]);
    }
  }
  
  // Debuggung Read stack 
  ret = xxusb_stack_read(udev, 2, stack);
  if (ret<0) {
    printf("err on stack_read:%d\n", ret);
  } else {
    printf("ret from stack_read:%d\n", ret);
    for (i = 0; i < (ret/2); i++) {
      printf("Rstack[%i]=0x%lx\n", i, stack[i]);
    }
  }
  
  // Remove Inhibit
  CAMAC_I(udev, FALSE);
  
  //  Define LAM time out to 100us for external LAM;
  // bits 0 to 15 in Delay Register N(25) A(2) F(16) 
  ret = CAMAC_write(udev, 25, 2, 16, 0x6400000, &q, &x);

  //  Enable LAM
  ret = CAMAC_read(udev, SLOT_ADC, 0, 26, &d24, &q, &x);

  // Set buffer size to 1 event at the time Buff
  // Opt in Global Mode register N(25) A(1) F(16)    
  ret = CAMAC_write(udev, 25, 1, 16, 0x7, &q, &x);

  //  Start DAQ mode
  ret = xxusb_register_write(udev, 1, 0x1);
  
  //  Clear module
  ret = CAMAC_read(udev, SLOT_ADC, 0, 9, &d24, &q, &x);

  return SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/
INT end_of_run(INT run_number, char *error)
{
  int ret;

  //  Stop DAQ mode
  ret = xxusb_register_write(udev, 1, 0x0);

  // -PAA-
  // flush data, these data are lost as the run is already closed.
  // will implement deferred transition later to fix this issue
  ret = ccUsbFlush();
  cm_msg(MINFO, "ccusb", "Flushed %d events", ret);
  return SUCCESS;
}

/*-- Pause Run -----------------------------------------------------*/
INT pause_run(INT run_number, char *error)
{
   return SUCCESS;
}

/*-- Resume Run ----------------------------------------------------*/
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

extern "C" INT poll_event(INT source, INT count, BOOL test)
/* Polling routine for events. Returns TRUE if event
   is available. If test equals TRUE, don't return. The test
   flag is used to time the polling */
{
   int i;
   DWORD lam=0;

   for (i = 0; i < count; i++) {
     ss_sleep(100);
     if (lam & LAM_SOURCE_STATION(source))
       if (!test)
	 return lam;
   }
   
   return 0;
}

/*-- Interrupt configuration ---------------------------------------*/

extern "C" INT interrupt_configure(INT cmd, INT source, POINTER_T adr)
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

/*-- Event readout -------------------------------------------------*/

INT read_trigger_event(char *pevent, INT off)
{
  WORD *pdata;
  int j, ret, nd32=0, evtsize, nevents;
  
  /* init bank structure */
  bk_init(pevent);
  
  /* create structured ADC0 bank */
  bk_create(pevent, "ADC0", TID_WORD, &pdata);
  
  // Read CC-USB buffer, use for 32-bit data
  ret = xxusb_bulk_read(udev, pdata, 8192, 500);  
  if (ret > 0) {
    nd32 = ret / 2;
    evtsize = (pdata[1] & 0xffff);  // # of words per event
    // ??? next line
    if (evtsize > 0x100) evtsize = 0x100;
    nevents = (pdata[0]& 0xffff);  // # of LAM in the buffer
    printf("Read data: ret:%d  nevent:%d, evtsize:%d\n", ret, nd32, nevents);
    if (nevents & 0x8000) return 0;  // Skip event
    if (nd32 > 0)
      for (j=0; j<=nd32; j++)
	printf("Data[%d]=0x%x\n", j, pdata[j]);
    // Adjust pointer (include nevents, evtsize)
    pdata += nd32;
    // Close bank
    bk_close(pevent, pdata);
    return bk_size(pevent); 
 } else {
    printf("no read\n");
    return 0;
  }
}
 

/*-- Scaler event --------------------------------------------------*/

INT read_scaler_event(char *pevent, INT off)
{
  //  DWORD *pdata, a;
  
  /* init bank structure */
  //  bk_init(pevent);
 
  //* create SCLR bank */
  //bk_create(pevent, "SCLR", TID_DWORD, &pdata);
  
  /* read scaler bank */
  //for (a = 0; a < N_SCLR; a++)
  //  cam24i(CRATE, SLOT_SCLR, a, 0, pdata++);
  
  //bk_close(pevent, pdata);
  
  return 0; // bk_size(pevent);
}
