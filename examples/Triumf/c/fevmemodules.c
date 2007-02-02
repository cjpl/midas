/********************************************************************\

  Name:         fevmemodules.c
  Created by:   

  Contents:    C code example of standarized frontend dealing with
  common VME module at Triumf.
  Used with the VMIC Fanuc VME processor board. 
  Missing parameters for the V1729

  $Id$
\********************************************************************/
#undef  V792_CODE
#undef  VADC0_CODE
#undef  VADC1_CODE
#undef  VADC2_CODE
#undef  VADC3_CODE
#undef  VTDC0_CODE
#define  VF48_CODE
#undef  VMEIO_CODE
#undef V1729_CODE

#include <stdio.h>
#include <stdlib.h>
#include "midas.h"
#include "mvmestd.h"
#include "vmicvme.h"
#include "vmeio.h"
#include "v1190B.h"
#include "v792.h"
#include "vf48.h"
#include "v1729.h"
#include "experim.h"

// VMEIO definition
#define P_BOE      0x1  // 
#define P_EOE      0x2  // 
#define P_CLR      0x4  // 
#define S_READOUT  0x8  //
#define S_RUNGATE  0x10 //

/* Interrupt vector */
int trig_level =  0;
#define TRIG_LEVEL  (int) 1
#define INT_LEVEL   (int) 3
#define INT_VECTOR  (int) 0x16
extern INT_INFO int_info;
int myinfo = VME_INTERRUPT_SIGEVENT;

/* make frontend functions callable from the C framework */
#ifdef __cplusplus
extern "C" {
#endif

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
char *frontend_name = "fevmemodules";
/* The frontend file name, don't change it */
char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
BOOL frontend_call_loop = FALSE;

/* a frontend status page is displayed with this frequency in ms */
INT display_period = 000;

/* maximum event size produced by this frontend */
INT max_event_size = 200000;

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
INT max_event_size_frag = 5 * 1024 * 1024;

/* buffer size to hold events */
INT event_buffer_size = 10 * 100000;

/* Hardware */
MVME_INTERFACE *myvme;
int  sRG = 0;    // state run gate
int  intflag = 0;
int  inRun = 0, missed=0;
int done=0, stop_req=0;
DWORD evlimit;

/* VME base address */
DWORD VMEIO_BASE = 0x780000;
DWORD VTDC0_BASE = 0xF10000;
DWORD VADC0_BASE = 0x100000;
DWORD VADC1_BASE = 0x200000;
DWORD VADC2_BASE = 0x300000;
DWORD VADC3_BASE = 0x400000;
  // DWORD VLAM_BASE  = 0x500000;
DWORD VLAM_BASE  = 0x800000;
DWORD VF48_BASE  = 0xAF0000;
DWORD V1729_BASE = 0x120000;
/* Globals */
#define  N_ADC 100
#define  N_TDC 100
#define  N_PTS 5000
extern HNDLE hDB;
HNDLE hSet;
TRIGGER_SETTINGS ts;

/* number of channels */
#define N_SCLR 4

/*-- Function declarations -----------------------------------------*/

INT frontend_init();
INT frontend_exit();
INT begin_of_run(INT run_number, char *error);
INT end_of_run(INT run_number, char *error);
INT pause_run(INT run_number, char *error);
INT resume_run(INT run_number, char *error);
INT frontend_loop();
extern void interrupt_routine(void);

INT read_trigger_event(char *pevent, INT off);
INT read_scaler_event(char *pevent, INT off);

void register_cnaf_callback(int debug);

BANK_LIST trigger_bank_list[] = {

   /* online banks */
   {"ADC0", TID_DWORD, N_ADC, NULL}
   ,
   {"ADC1", TID_DWORD, N_ADC, NULL}
   ,
   {"ADC2", TID_DWORD, N_ADC, NULL}
   ,
   {"ADC3", TID_DWORD, N_ADC, NULL}
   ,
   {"TDC0", TID_DWORD, N_TDC, NULL}
   ,

   {"WAVE", TID_DWORD, N_TDC, NULL}
   ,
   {"FWAV", TID_DWORD, N_PTS, NULL}
   ,

   {""}
   ,
};

/*-- Equipment list ------------------------------------------------*/

#undef USE_INT

EQUIPMENT equipment[] = {

   {"Trigger",               /* equipment name */
    {1, 0,                   /* event ID, trigger mask */
     "SYSTEM",               /* event buffer */
#ifdef USE_INT
     EQ_INTERRUPT,           /* equipment type */
#else
     EQ_PERIODIC, // POLLED,              /* equipment type */
#endif
     LAM_SOURCE(0, 0x0),     /* event source crate 0, all stations */
     "MIDAS",                /* format */
     TRUE,                   /* enabled */
     RO_RUNNING,          /* read only when running */
     500,                    /* poll for 500ms */
     0,                      /* stop run after this event limit */
     0,                      /* number of sub events */
     0,                      /* don't log history */
     "", "", "",},
    read_trigger_event,      /* readout routine */
    NULL, NULL,
    trigger_bank_list,
    }
   ,

   {"Scaler",                /* equipment name */
    {2, 0,                   /* event ID, trigger mask */
     "SYSTEM",               /* event buffer */
     EQ_PERIODIC ,           /* equipment type */
     0,                      /* event source */
     "MIDAS",                /* format */
     FALSE,                   /* enabled */
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

/********************************************************************/

/*-- Sequencer callback info  --------------------------------------*/
void seq_callback(INT hDB, INT hseq, void *info)
{
  printf("odb ... trigger settings touched\n");
}

/*-- Frontend Init -------------------------------------------------*/
INT frontend_init()
{
  int size, status;
  char set_str[80];

  /* Book Setting space */
  TRIGGER_SETTINGS_STR(trigger_settings_str);

  /* Map /equipment/Trigger/settings for the sequencer */
  sprintf(set_str, "/Equipment/Trigger/Settings");
  status = db_create_record(hDB, 0, set_str, strcomb(trigger_settings_str));
  status = db_find_key (hDB, 0, set_str, &hSet);
  if (status != DB_SUCCESS)
    cm_msg(MINFO,"FE","Key %s not found", set_str);
  
  /* Enable hot-link on settings/ of the equipment */
  size = sizeof(TRIGGER_SETTINGS);
  if ((status = db_open_record(hDB, hSet, &ts, size, MODE_READ
                               , seq_callback, NULL)) != DB_SUCCESS)
    return status;
  
  // Open VME interface   
  status = mvme_open(&myvme, 0);
  
  // Set am to A24 non-privileged Data
  mvme_set_am(myvme, MVME_AM_A24_ND);
  
  // Set dmode to D32
  mvme_set_dmode(myvme, MVME_DMODE_D32);

#if defined V1729_CODE
  // Collect Pedestals
  v1729_PedestalRun(myvme, V1729_BASE, 10, 1);
  
  // Do vernier calibration
  v1729_TimeCalibrationRun(myvme, V1729_BASE, 1);
#endif

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
  int i, csr, status, size;
   /* put here clear scalers etc. */

  /* read Triggger settings */
  size = sizeof(TRIGGER_SETTINGS);
  if ((status = db_get_record (hDB, hSet, &ts, &size, 0)) != DB_SUCCESS)
    return status;

  // Set am to A24 non-privileged Data
  mvme_set_am(myvme, MVME_AM_A24_ND);
  
  // Set dmode to D32
  mvme_set_dmode(myvme, MVME_DMODE_D32);
  
#if defined VTDC0_CODE
  //-------- TDCs -------------------------------------------
  // Check the TDC
  csr = mvme_read_value(myvme, VTDC0_BASE+V1190_FIRM_REV_RO);
  printf("Firmware revision: 0x%x\n", csr);
  
  // Set mode 1
  v1190_Setup(myvme, VTDC0_BASE, 1);

  if (ts.v1190.leresolution == 100)
    temp = LE_RESOLUTION_100;
  else if (ts.v1190.leresolution == 200)
    temp = LE_RESOLUTION_200;
  else if (ts.v1190.leresolution == 800)
    temp = LE_RESOLUTION_800;
  else
    temp = 3;
  if (temp != 2)
    v1190_LEResolutionSet(myvme, VTDC0_BASE, temp);

  // Adjust window width to 10us 
  // 10us == 0x18F
  temp = (DWORD) abs(ts.v1190.windowwidth / 25e-9);
  v1190_WidthSet(myvme, VTDC0_BASE, temp);

  // Adjust offset width to 10us 
  // 10us == 0xE70
  temp = ts.v1190.windowoffset / 25e-9;
  temp = temp < 0 ? ((DWORD)temp&0xFFF): (DWORD) temp;
  v1190_OffsetSet(myvme, VTDC0_BASE, temp);

  // Print Current status 
  printf("BOR : FELIXE V1190B settings\n");
  v1190_Status(myvme, VTDC0_BASE);
#endif

#if defined V792_CODE
  //-------- ADCs ------------------------------------------
  printf("ADC0\n");
  v792_Setup(myvme, VADC0_BASE, 2);
  printf("ADC1\n");
  v792_Setup(myvme, VADC1_BASE, 2);
  printf("ADC2\n");
  v792_Setup(myvme, VADC2_BASE, 2);
  printf("ADC3\n");
  v792_Setup(myvme, VADC3_BASE, 2);

  /* Initialize the V792s ADC0 --*/
  /* Load Threshold */
  for (i=0;i<V792_MAX_CHANNELS;i+=2) {
    printf("B Threshold[%i] = 0x%4.4x\t   -  ", i, ts.v792.threshold1[i]);
    printf("Threshold[%i] = 0x%4.4x\n", i+1, ts.v792.threshold1[i+1]);
  }
  v792_ThresholdWrite(myvme, VADC0_BASE, (WORD *)&(ts.v792.threshold1));
  
  for (i=0;i<V792_MAX_CHANNELS;i+=2) {
    printf("Threshold[%i] = 0x%4.4x\t   -  ", i, ts.v792.threshold1[i]);
    printf("Threshold[%i] = 0x%4.4x\n", i+1, ts.v792.threshold1[i+1]);
  }

  v792_DataClear(myvme, VADC0_BASE);
  csr = v792_CSR1Read(myvme, VADC0_BASE);
  printf("Data Ready ADC0: 0x%x\n", csr);
  
  /* Initialize the V792s ADC1 --*/
  /* Load Threshold */
  v792_ThresholdWrite(myvme, VADC1_BASE, (WORD *)&(ts.v792.thresholds2));
  v792_DataClear(myvme, VADC1_BASE);
  csr = v792_CSR1Read(myvme, VADC1_BASE);
  printf("Data Ready ADC1: 0x%x\n", csr);
  
  /* Initialize the V792s ADC2 --*/
  /* Load Threshold */
  v792_ThresholdWrite(myvme, VADC2_BASE, (WORD *)&(ts.v792.thresholds3));
  v792_DataClear(myvme, VADC2_BASE);
  csr = v792_CSR1Read(myvme, VADC2_BASE);
  printf("Data Ready ADC2: 0x%x\n", csr);
  
  /* Initialize the V792s ADC3 --*/
  /* Load Threshold */
  v792_ThresholdWrite(myvme, VADC3_BASE, (WORD *)&(ts.v792.thresold4));
  v792_DataClear(myvme, VADC3_BASE);
  csr = v792_CSR1Read(myvme, VADC3_BASE);
  printf("Data Ready ADC3: 0x%x\n", csr);
#endif


#if defined V1729_CODE
  //-------- 1/2 Gsps ADC (CAEN) -------------------------------------
  // Set mode 3 (128 full range)
  v1729_Setup(myvme, V1729_BASE, 3);
  v1729_PostTrigSet(myvme, V1729_BASE, ts.v1729.post_trigger);

  // Print Current status
  v1729_Status(myvme, V1729_BASE);

  // Start acquisition
  v1729_AcqStart(myvme, V1729_BASE);
#endif

#if defined VF48_CODE
  //-------- 20..65 Msps ADC (Uni Montreal) -------------------------
  /* Initialize the VF48 --*/
  vf48_Reset(myvme, VF48_BASE);

  // Stop DAQ for seting up the parameters
  vf48_AcqStop(myvme, VF48_BASE);
    
  if (ts.vf48.external_trigger) {
    vf48_ExtTrgSet(myvme, VF48_BASE);
    printf("vf48: External trigger mode\n");
  } else {
    vf48_ExtTrgClr(myvme, VF48_BASE);
    printf("vf48: Self triggering mode\n");
  }
  csr = vf48_CsrRead(myvme, VF48_BASE);
  printf("CSR Buffer: 0x%x\n", csr);

  // Segment Size
  vf48_SegmentSizeSet(myvme, VF48_BASE, ts.vf48.segment_size);
    for (i=0;i<6;i++) {
      printf("vf48: SegSize grp (%d) %i:%d\n", ts.vf48.segment_size
	     , i, vf48_SegmentSizeRead(myvme, VF48_BASE, i));
    }

  // Set Threshold high for external trigger, as the 
  // the self-trigger is always sending info to the collector 
  // and overwhelming the link. (max maybe 10 bits (0x3FF))
  // Threshold
  for (i=0;i<6;i++) {
    vf48_TrgThresholdSet(myvme, VF48_BASE, i, ts.vf48.trigger_threshold);
      printf("vf48: Trigger Threshold grp (%d) %i:%d\n", ts.vf48.trigger_threshold
	     , i, vf48_TrgThresholdRead(myvme, VF48_BASE, i));
  }

  // Set Threshold high for Hit detection
  for (i=0;i<6;i++) {
    vf48_HitThresholdSet(myvme, VF48_BASE, i, ts.vf48.hit_threshold);
      printf("vf48: Hit Threshold grp (%d) %i:%d\n", ts.vf48.hit_threshold
	     , i, vf48_HitThresholdRead(myvme, VF48_BASE, i));
  }

  // Active Channel Mask
  for (i=0;i<6;i++){
    vf48_ActiveChMaskSet(myvme, VF48_BASE, i, ts.vf48.active_channel_mask);
    printf("vf48: Active channel mask grp (%x) %i:0x%x\n", ts.vf48.active_channel_mask
	   , i, vf48_ActiveChMaskRead(myvme, VF48_BASE, i));
  }

  // Raw Data Suppression
  for (i=0;i<6;i++){
    vf48_RawDataSuppSet(myvme, VF48_BASE, i, ts.vf48.raw_data_suppress);
    printf("vf48: Raw Data Suppress grp (%x) %i:0x%x\n", ts.vf48.raw_data_suppress
	   , i, vf48_RawDataSuppRead(myvme, VF48_BASE, i));
  }

  // Channel Suppress Enable
  for (i=0;i<6;i++){
    vf48_ChSuppSet(myvme, VF48_BASE, i, ts.vf48.channel_suppress_enable);
    printf("vf48: Channel Suppression Enable grp (%x) %i:0x%x\n", ts.vf48.channel_suppress_enable
	   , i, 0x1 & vf48_ChSuppRead(myvme, VF48_BASE, i));
  }

  // Divisor
  vf48_DivisorWrite(myvme, VF48_BASE, ts.vf48.divisor);
  for (i=0;i<6;i++){
    printf("vf48: Divisor grp (%x) %i:0x%x\n", ts.vf48.divisor
	   , i, vf48_DivisorRead(myvme, VF48_BASE, i));
  }

  // PreTrigger
  for (i=0;i<6;i++) {
    vf48_ParameterWrite(myvme, VF48_BASE, i, VF48_PRE_TRIGGER, ts.vf48.pretrigger);
    printf("vf48: Pre Trigger grp %i:%d\n"
	   , i, vf48_ParameterRead(myvme, VF48_BASE, i, VF48_PRE_TRIGGER));
  }

  // Latency
  for (i=0;i<6;i++) {
    vf48_ParameterWrite(myvme, VF48_BASE, i, VF48_LATENCY, ts.vf48.latency);
    printf("vf48: Latency grp %i:%d\n"
	   , i, vf48_ParameterRead(myvme, VF48_BASE, i, VF48_LATENCY));
  }

  // K-coef
  for (i=0;i<6;i++) {
    vf48_ParameterWrite(myvme, VF48_BASE, i, VF48_K_COEF, ts.vf48.k_coeff);
    printf("vf48: K-coeff grp %i:%d\n"
	   , i, vf48_ParameterRead(myvme, VF48_BASE, i, VF48_K_COEF));
  }
  // L-coef
  for (i=0;i<6;i++) {
    vf48_ParameterWrite(myvme, VF48_BASE, i, VF48_L_COEF, ts.vf48.l_coeff);
    printf("vf48: L-coeff grp %i:%d\n"
	   , i, vf48_ParameterRead(myvme, VF48_BASE, i, VF48_L_COEF));
  }
  // M-coef
  for (i=0;i<6;i++) {
    vf48_ParameterWrite(myvme, VF48_BASE, i, VF48_M_COEF, ts.vf48.m_coeff);
    printf("vf48: M-coeff grp %i:%d\n"
	   , i, vf48_ParameterRead(myvme, VF48_BASE, i, VF48_M_COEF));
  }

  // Polarity
  for (i=0;i<6;i++) {
    DWORD pat;
    if (ts.vf48.inverse_signal){
      pat = vf48_ParameterRead(myvme, VF48_BASE, i, VF48_MBIT1);
      pat |= VF48_INVERSE_SIGNAL;
      vf48_ParameterWrite(myvme, VF48_BASE, i, VF48_MBIT1, pat);
    } else {
      pat = vf48_ParameterRead(myvme, VF48_BASE, i, VF48_MBIT1);
      pat &= ~(VF48_INVERSE_SIGNAL);
      vf48_ParameterWrite(myvme, VF48_BASE, i, VF48_MBIT1, pat);
    }
    printf("vf48: Polarity grp %i:%d\n"
	   , i, vf48_ParameterRead(myvme, VF48_BASE, i, VF48_MBIT1));
  }
  
  // Group Enable
  vf48_GrpEnable(myvme, VF48_BASE, ts.vf48.group_bitwise_all_0x3f_);
  printf("vf48: Group Enable :0x%x\n", vf48_GrpRead(myvme, VF48_BASE));

  // Start ACQ for VF48
  vf48_AcqStart(myvme, VF48_BASE);
#endif


#if defined VMEIO_CODE
  //-------- VMEIO (triumf board) ----------------------------------
  // Set am to A24 non-privileged Data
  mvme_set_am(myvme, MVME_AM_A24_ND);

  // Set dmode to D32
  mvme_set_dmode(myvme, MVME_DMODE_D32);

  // Set pulse mode
  vmeio_OutputSet(myvme, VMEIO_BASE, (P_BOE|P_EOE|P_CLR));
#endif

  //------ FINAL ACTIONS before BOR -----------
  // Send Clear pulse 
  //  vmeio_SyncWrite(myvme, VMEIO_BASE, P_CLR);

  /* Reset Latch */

  /********* commented out for other test purpose
  vmeio_SyncWrite(myvme, VMEIO_BASE, P_EOE);
  
  // Open run gate
  vmeio_AsyncWrite(myvme, VMEIO_BASE, S_RUNGATE);
  
  // Disable interrupt
  mvme_write_value(myvme, VLAM_BASE+4, 0x0);
  
  // Reset Latch
  mvme_write_value(myvme, VLAM_BASE, 0x1);
  
  // Clear pending interrupts
  mvme_write_value(myvme, VLAM_BASE+8, 0x0);
  //  printf("Vector:0x%lx\n", (mvme_read_value(myvme, VLAM_BASE+0x8) & 0xFF));
  
  // Enable interrupt
  inRun = 1;
  mvme_write_value(myvme, VLAM_BASE+4, inRun);
  */
  
  missed = 0; // for the v1729 missed trigger
  printf("End of BOR\n");
  return SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/
INT end_of_run(INT run_number, char *error)
{
  // Stop DAQ for seting up the parameters
  vf48_AcqStop(myvme, VF48_BASE);
    
  done = 0;
  stop_req = 0;
  inRun = 0;
  // Disable interrupt
  mvme_write_value(myvme, VLAM_BASE+4, inRun);
  trig_level = 0;
  // Close run gate
  vmeio_AsyncWrite(myvme, VMEIO_BASE, 0x0);
  return SUCCESS;
}

/*-- Pause Run -----------------------------------------------------*/
INT pause_run(INT run_number, char *error)
{
  inRun = 0;
  // Disable interrupt
  mvme_write_value(myvme, VLAM_BASE+4, inRun);
  trig_level = 0;
  // Close run gate
  vmeio_AsyncWrite(myvme, VMEIO_BASE, 0x0);
   
  return SUCCESS;
}

/*-- Resume Run ----------------------------------------------------*/
INT resume_run(INT run_number, char *error)
{
  trig_level = TRIG_LEVEL;

  // EOB Pulse
  vmeio_SyncWrite(myvme, VMEIO_BASE, P_EOE);

  // Open run gate
  vmeio_AsyncWrite(myvme, VMEIO_BASE, S_RUNGATE);

  // Enable interrupt
  inRun = 1;
  mvme_write_value(myvme, VLAM_BASE+4, inRun);
  return SUCCESS;
}

/*-- Frontend Loop -------------------------------------------------*/
INT frontend_loop()
{
  /* if frontend_call_loop is true, this routine gets called when
     the frontend is idle or once between every event */
  char str[128];

  if (stop_req && done==0) {
    db_set_value(hDB,0,"/logger/channels/0/Settings/Event limit", &evlimit, sizeof(evlimit), 1, TID_DWORD); 
    if (cm_transition(TR_STOP, 0, str, sizeof(str), ASYNC, FALSE) != CM_SUCCESS) {
      cm_msg(MERROR, "VF48 Timeout", "cannot stop run: %s", str);
    }
    inRun = 0;
    // Disable interrupt
    mvme_write_value(myvme, VLAM_BASE+4, inRun);
    done = 1;
    cm_msg(MERROR, "VF48 Timeout","VF48 Stop requested");
  }
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
    //    if (vmeio_CsrRead(myvme, VMEIO_BASE))
    //    if (lam > 10)
{
  int i;
  int lam = 0;
  
  for (i = 0; i < count; i++, lam++) {
    lam = vmeio_CsrRead(myvme, VMEIO_BASE);
    if (lam)
      if (!test)
	return lam;
  }
  
  return 0;
}

/*-- Interrupt configuration ---------------------------------------*/
INT interrupt_configure(INT cmd, INT source, PTYPE adr)
{
  int vec = 0;
  switch (cmd) {
  case CMD_INTERRUPT_ENABLE:
    if (inRun) mvme_write_value(myvme, VLAM_BASE+4, 0x1);
    break;
  case CMD_INTERRUPT_DISABLE:
    if (inRun) mvme_write_value(myvme, VLAM_BASE+4, 0x0);
    break;
  case CMD_INTERRUPT_ATTACH:
    mvme_set_dmode(myvme, MVME_DMODE_D32);
    mvme_interrupt_attach(myvme, INT_LEVEL, INT_VECTOR, (void *)adr, &myinfo);
    mvme_write_value(myvme, VLAM_BASE+0x10, INT_VECTOR);
    vec = mvme_read_value(myvme, VLAM_BASE+0x10);
    printf("Interrupt Attached to 0x%x for vector:0x%x\n", adr, vec&0xFF);
    break;
  case CMD_INTERRUPT_DETACH:
    printf("Interrupt Detach\n");
    break;
  }
  return SUCCESS;
}

/*-- Event readout -------------------------------------------------*/
int vf48_error = 0;
INT read_trigger_event(char *pevent, INT off)
{
#if defined VADC0_CODE
  DWORD  *pdata;
#endif
#if defined VTDC0_CODE
  DWORD  *pdata;
#endif
#if defined VF48_CODE
  DWORD  *pdata;
  int status, evtlength;
#endif
#if defined V1729_CODE
  INT   *pidata, timeout;
  WORD   data[20000];
  INT    nch, npt, nentry;
#endif

  evlimit = SERIAL_NUMBER(pevent);
  // Timing Begin of Event
  //  vmeio_SyncWrite(myvme, VMEIO_BASE, P_BOE);
  // Open READOUT State
  //  vmeio_AsyncWrite(myvme, VMEIO_BASE, (S_READOUT|S_RUNGATE));

  // > THE LINE BELOW SHOULD BE HERE AND NOWHERE ELSE < !
  bk_init32(pevent);

#if defined VADC0_CODE
  ///------------------------------
  ///------------------------------
  ///------------------------------
  /* create structured ADC0 bank */
  bk_create(pevent, "ADC0", TID_DWORD, &pdata);
  v792_EvtCntRead(myvme, VADC0_BASE, &evtcnt);
  /* Read Event */
  v792_EventRead(myvme, VADC0_BASE, pdata, &nentry);
  pdata += nentry;
  bk_close(pevent, pdata);
  v792_DataClear(myvme, VADC0_BASE);
#endif

#if defined VADC1_CODE
  ///------------------------------
  /* create structured ADC1 bank */
  bk_create(pevent, "ADC1", TID_DWORD, &pdata);
  /* Read Event */
  v792_EventRead(myvme, VADC1_BASE, pdata, &nentry);
  pdata += nentry;
  bk_close(pevent, pdata);
  //    v792_EvtCntReset(myvme, VADC1_BASE);
#endif

#if defined VADC2_CODE
  ///------------------------------
  /* create structured ADC2 bank */
  bk_create(pevent, "ADC2", TID_DWORD, &pdata);
  /* Read Event */
  v792_EventRead(myvme, VADC2_BASE, pdata, &nentry);
  pdata += nentry;
  bk_close(pevent, pdata);
#endif

#if defined VADC3_CODE
  ///------------------------------
  /* create structured ADC3 bank */
  bk_create(pevent, "ADC3", TID_DWORD, &pdata);
  /* Read Event */
  v792_EventRead(myvme, VADC3_BASE, pdata, &nentry);
  pdata += nentry;
  bk_close(pevent, pdata);
#endif

#if defined VTDC0_CODE
  ///-------------------------------
  /* create variable length TDC bank */
  bk_create(pevent, "TDC0", TID_DWORD, &pdata);
  /* Read Event */
  v1190_EventRead(myvme, VTDC0_BASE, pdata, &nentry);
  pdata += nentry;
  /* clear TDC */
  //  v1190_SoftClear(myvme, VTDC0_BASE);
  bk_close(pevent, pdata);
#endif

#if defined VF48_CODE
  ///-------------------------------
  /* create variable length WAVE bank */
  bk_create(pevent, "WAVE", TID_DWORD, &pdata);
  status = vf48_EventRead64(myvme, VF48_BASE, pdata, &evtlength);
  //  printf("read vf48 evtlength:%d\n", evtlength);
  pdata += (DWORD) evtlength;
  bk_close(pevent, pdata);
#endif

#if defined V1729_CODE
  ///-------------------------------
  timeout=20;
  while(!v1729_isTrigger(myvme, V1729_BASE) && timeout) {
    timeout--;
    ss_sleep(1);
  };
  if (timeout == 0) {
    printf("No Trigger evt#:%d missed:%d (%6.2f%%)\n", SERIAL_NUMBER(pevent)
	   , ++missed
	   , 100.*((float)missed/(float)SERIAL_NUMBER(pevent)));
  }
  npt = 2560;
  nch =    4;
  v1729_DataRead(myvme, V1729_BASE, data, nch, npt);
  bk_create(pevent, "FWAV", TID_INT, &pidata);
  *pidata++ = npt; 
  *pidata++ = data[7];
  v1729_OrderData(myvme, V1729_BASE, data, pidata, nch, 0, npt);
  pidata += npt;
  /*
   *pidata++ = 0x10000 | npt;
   v1729_OrderData(myvme, V1729_BASE, data, pidata, nch, 1, npt);
   pidata += npt;
  */
  bk_close(pevent, pidata);
  
  // Start acquisition
  v1729_AcqStart(myvme, V1729_BASE);
#endif

  ///-------------------------------
  // EOB  
  /*
    vmeio_AsyncWrite(myvme, VMEIO_BASE, S_RUNGATE);
    vmeio_SyncWrite(myvme, VMEIO_BASE, P_EOE);
    
    // Reset Latch EOB
    mvme_write_value(myvme, VLAM_BASE, 0x0);  // Nim out 
  */
  
  if (evtlength) {
    //    printf("evtlength: %d  bk_size:%d\n", evtlength, bk_size(pevent));
    return bk_size(pevent);
  }
  else
    return 0;
}

/*-- Scaler event --------------------------------------------------*/
INT read_scaler_event(char *pevent, INT off)
{
  DWORD *pdata;
  
  /* init bank structure */
  bk_init(pevent);
  
  /* create SCLR bank */
  bk_create(pevent, "SCLR", TID_DWORD, &pdata);
  
  /* read scaler bank */
  bk_close(pevent, pdata);
  
  //   return bk_size(pevent);
  // Nothing to send for now
  return 0;
}


