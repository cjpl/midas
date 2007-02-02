
/********************************************************************\

  Name:         fevme.cxx
  Created by:   K.Olchanski

  Contents:     Frontend for the generic VME DAQ using CAEN ADC, TDC, TRIUMF VMEIO and VF48
$Id$
\********************************************************************/

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <assert.h>
#include "midas.h"
#include "mvmestd.h"

#define HAVE_V792
#define HAVE_V895
#define HAVE_V1190B

extern "C" {
#ifdef HAVE_VMEIO
#include "vme/vmeio.h"
#endif
#ifdef HAVE_V792
#include "vme/v792.h"
#endif
#ifdef HAVE_V895
#include "v895.h"
#endif
#ifdef HAVE_V1190B
#include "vme/v1190B.h"
#endif
#ifdef HAVE_VF48
#include "vme/VF48.h"
#endif
}

/* make frontend functions callable from the C framework */
#ifdef __cplusplus
extern "C" {
#endif

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
   char *frontend_name = "fevme";
/* The frontend file name, don't change it */
   char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
   BOOL frontend_call_loop = FALSE;

/* a frontend status page is displayed with this frequency in ms */
   INT display_period = 000;

/* maximum event size produced by this frontend */
   INT max_event_size = 100*1024;

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
   INT max_event_size_frag = 1024*1024;

/* buffer size to hold events */
   INT event_buffer_size = 200*1024;

  extern INT run_state;
  extern HNDLE hDB;

/*-- Function declarations -----------------------------------------*/
  INT frontend_init();
  INT frontend_exit();
  INT begin_of_run(INT run_number, char *error);
  INT end_of_run(INT run_number, char *error);
  INT pause_run(INT run_number, char *error);
  INT resume_run(INT run_number, char *error);
  INT frontend_loop();

  INT read_event(char *pevent, INT off);
/*-- Bank definitions ----------------------------------------------*/

/*-- Equipment list ------------------------------------------------*/

  EQUIPMENT equipment[] = {

    {"Trigger",               /* equipment name */
     {1, TRIGGER_ALL,         /* event ID, trigger mask */
      "SYSTEM",               /* event buffer */
      EQ_POLLED,              /* equipment type */
      LAM_SOURCE(0, 0xFFFFFF),                      /* event source */
      "MIDAS",                /* format */
      TRUE,                   /* enabled */
      RO_RUNNING,             /* read only when running */

      500,                    /* poll for 500ms */
      0,                      /* stop run after this event limit */
      0,                      /* number of sub events */
      0,                      /* don't log history */
      "", "", "",}
     ,
     read_event,      /* readout routine */
     NULL, NULL,
     NULL,       /* bank list */
    }
    ,

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

MVME_INTERFACE *gVme = 0;

int gVmeioBase = 0x780000;
int gAdcBase   = 0x110000;
int gDisBase[] = { 0xE00000, 0 };
int gTdcBase   = 0xf10000;
int gVF48base  = 0xa00000;

int mvme_read16(int addr)
{
  mvme_set_dmode(gVme, MVME_DMODE_D16);
  return mvme_read_value(gVme, addr);
}

int mvme_read32(int addr)
{
  mvme_set_dmode(gVme, MVME_DMODE_D32);
  return mvme_read_value(gVme, addr);
}

void encodeU32(char*pdata,uint32_t value)
{
  pdata[0] = (value&0x000000FF)>>0;
  pdata[1] = (value&0x0000FF00)>>8;
  pdata[2] = (value&0x00FF0000)>>16;
  pdata[3] = (value&0xFF000000)>>24;
}

uint32_t odbReadUint32(const char*name,int index,uint32_t defaultValue = 0)
{
  int status;
  uint32_t value = 0;
  int size = 4;
  HNDLE hkey;
  status = db_get_data_index(hDB,hkey,&value,&size,index,TID_DWORD);
  if (status != SUCCESS)
    {
      cm_msg (MERROR,frontend_name,"Cannot read \'%s\'[%d] from odb, status %d",name,index,status);
      return defaultValue;
    }
  return value;
}

int enable_trigger()
{
#ifdef HAVE_VMEIO
  vmeio_OutputSet(gVme,gVmeioBase,0xFFFF);
#endif
  return 0;
}


int disable_trigger()
{
#ifdef HAVE_VMEIO
  vmeio_OutputSet(gVme,gVmeioBase,0);
#endif
  return 0;
}

INT init_vme_modules()
{
#ifdef HAVE_VMEIO
  vmeio_OutputSet(gVme,gVmeioBase,0);
  vmeio_StrobeClear(gVme,gVmeioBase);
  printf("VMEIO at 0x%x CSR is 0x%x\n",gVmeioBase,vmeio_CsrRead(gVme,gVmeioBase));
#endif

#ifdef HAVE_V792
  v792_SingleShotReset(gVme,gAdcBase);
  v792_Status(gVme,gAdcBase);
#endif

#ifdef HAVE_V895
  for (int i=0; gDisBase[i] != 0; i++)
    {
      v895_Status(gVme,gDisBase[i]);

      v895_writeReg16(gVme,gDisBase[i],0x40,255); // width 0-7
      v895_writeReg16(gVme,gDisBase[i],0x42,255); // width 8-15
      v895_writeReg16(gVme,gDisBase[i],0x48,56); // majority
      v895_writeReg16(gVme,gDisBase[i],0x4A,0xFFFF); // enable all channels
      //v895_writeReg8(gVme,gDisBase[i],0x4A,0x0000); // enable all channels
      for (int j=0; j<16; j++)
  {
    int thr_mV = 12;
    v895_writeReg16(gVme,gDisBase[i],j*2,thr_mV); // threshold in mV
  }
      //v895_TestPulse(gVme,gDisBase[i]); // fire test pulse

      //v895_writeReg16(gVme,gDisBase[i],0*2,250); // threshold in mV
    }
#endif

#ifdef HAVE_V1190B
  v1190_SoftClear(gVme,gTdcBase);
  v1190_Status(gVme,gTdcBase);
#endif

#ifdef HAVE_VF48
  VF48_Reset(gVme,gVF48base);
  printf("VF48 at 0x%x CSR is 0x%x\n",gVF48base,VF48_CsrRead(gVme,gVF48base));
#endif

  return SUCCESS;
}

/*-- Frontend Init -------------------------------------------------*/

INT frontend_init()
{
  int status;

  setbuf(stdout,NULL);
  setbuf(stderr,NULL);

  status = mvme_open(&gVme,0);
  status = mvme_set_am(gVme, MVME_AM_A24_ND);

  init_vme_modules();

  disable_trigger();

  return status;
}

static int gHaveRun = 0;

/*-- Frontend Exit -------------------------------------------------*/

INT frontend_exit()
{
  gHaveRun = 0;
  disable_trigger();

  mvme_close(gVme);

  return SUCCESS;
}

/*-- Begin of Run --------------------------------------------------*/

INT begin_of_run(INT run_number, char *error)
{
  gHaveRun = 1;
  printf("begin run %d\n",run_number);

  init_vme_modules();

  enable_trigger();
  return SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/
INT end_of_run(INT run_number, char *error)
{
  static bool gInsideEndRun = false;

  if (gInsideEndRun)
    {
      printf("breaking recursive end_of_run()\n");
      return SUCCESS;
    }

  gInsideEndRun = true;

  gHaveRun = 0;
  printf("end run %d\n",run_number);
  disable_trigger();

  gInsideEndRun = false;

  return SUCCESS;
}

/*-- Pause Run -----------------------------------------------------*/
INT pause_run(INT run_number, char *error)
{
  gHaveRun = 0;
  disable_trigger();
  return SUCCESS;
}

/*-- Resume Run ----------------------------------------------------*/
INT resume_run(INT run_number, char *error)
{
  gHaveRun = 1;
  enable_trigger();
  return SUCCESS;
}

/*-- Frontend Loop -------------------------------------------------*/
INT frontend_loop()
{
  /* if frontend_call_loop is true, this routine gets called when
     the frontend is idle or once between every event */
  return SUCCESS;
}

/********************************************************************\

  Readout routines for different events

\********************************************************************/

/*-- Trigger event routines ----------------------------------------*/
extern "C" INT poll_event(INT source, INT count, BOOL test)
/* Polling routine for events. Returns TRUE if event
   is available. If test equals TRUE, don't return. The test
   flag is used to time the polling */
{
  //printf("poll_event %d %d %d!\n",source,count,test);

  for (int i=0 ; i<count ; i++)
  {
    int lam = v792_DataReady(gVme,gAdcBase);
    //printf("source: 0x%x, lam: 0x%x\n", source, lam);

    if (lam)
      if (!test)
        return TRUE;
  }

  return FALSE;

#if 0
  for (int i = 0; i < count; i++)
    {
      usleep(1000);
      if (true)
  if (!test)
    return v792_DataReady(gVme,gAdcBase);
    }
  return 1;

  return v792_DataReady(gVme,gAdcBase);
#endif
}

/*-- Interrupt configuration ---------------------------------------*/
extern "C" INT interrupt_configure(INT cmd, INT source, PTYPE adr)
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

int read_v792(int base,const char*bname,char*pevent,int nchan)
{
  int i;
  int wcount = 0;
  DWORD data[100];
  int counter = 0;
  WORD* pdata;

  /* Event counter */
  v792_EvtCntRead(gVme, base, &counter);

  /* Read Event */
  v792_EventRead(gVme, base, data, &wcount);

  /* create ADCS bank */
  bk_create(pevent, bname, TID_WORD, &pdata);

  for (i=0; i<32; i++)
    pdata[i] = 0;

  for (i=0; i<wcount; i++)
    {
      uint32_t w = data[i];
      if (((w>>24)&0x7) != 0) continue;
      int chan = (w>>16)&0x1F;
      int val  = (w&0x1FFF);
      pdata[chan] = val;
    }

  //printf("counter %6d, words: %3d, header: 0x%08x, ADC0: 0x%08x, ADC0,1,2: %6d %6d %6d\n",counter,wcount,data[0],pdata[0],pdata[0],pdata[1],pdata[2]);

  pdata += nchan;
  bk_close(pevent, pdata);

  return wcount;
}

#ifdef HAVE_V1190B
int read_tdc(int base,char*pevent)
{
  int time0 = 0;
  int time1 = 0;
  int time2 = 0;
  int count;
  const int kDataSize = 10000;
  DWORD data[kDataSize];
  int* pdata32;

  /* create TOPA bank */
  bk_create(pevent, "TDCS", TID_INT, &pdata32);

  //while (count <
  count = v1190_DataRead(gVme, base, data, 10000);

  if (count > 0)
    {
#if 0
      if (count > 1000)
  printf("reading TDC got %d words\n",count);

      printf("reading TDC got %d words\n",count);
#endif
      for (int i=0; i<count; i++)
  {
    int code = 0x1F&(data[i]>>27);

    if (data[i] == 0)
      continue;

    switch (code)
      {
      case 0:
        {
    int edge = 0x1&(data[i]>>26);
    int chan = 0x3F&(data[i]>>19);
    int time = 0x3FFFF&data[i];
#if 0
    printf("tdc %3d: 0x%08x, code 0x%02x, edge %d, chan %2d, time %6d\n",
           i,
           data[i],
           code,
           edge,
           chan,
           time);
#endif
    if ((chan==0) && (time0==0))
      time0 = time;
    if ((chan==1) && (time0!=0) && (time1==0) && (time>time0))
      time1 = time;
    if ((chan==2) && (time0!=0) && (time2==0) && (time>time0))
      time2 = time;
        }
        break;
        //case 0x18:
    //break;
      default:
#if 0
        printf("tdc %3d: 0x%08x, code 0x%02x\n",i,data[i],code);
#endif
        break;
      }
  }

#if 0
      if (time1 && time2)
  printf("time %8d %8d %8d, diff %8d\n",time0,time1,time2,time1-time2);
#endif
    }

  int xdata = time1-time2;
  if (xdata < -10000)
    xdata = 0;
  else if (xdata > 10000)
    xdata = 0;

#if 0
  if (xdata == 0)
    printf("count %d, time %8d %8d %8d, diff %8d\n",count,time0,time1,time2,xdata);
#endif

  *pdata32++ = xdata;

  bk_close(pevent, pdata32);

  v1190_SoftClear(gVme,base);

  return 0;
}
#endif

#ifdef HAVE_VF48
int read_vf48(int base,char* pevent)
{
  int count = 0xF;
  DWORD buf[10000];
  int* pdata32;

  /* create TOPA bank */
  bk_create(pevent, "VF48", TID_INT, &pdata32);

  VF48_EventRead(gVme, base, buf, &count);

  for (int i=0; i<count; i++)
    *pdata32++ = buf[i];

  bk_close(pevent, pdata32);

  return 0;
}
#endif

INT read_event(char *pevent, INT off)
{
  //printf("read event!\n");

  /* init bank structure */
  bk_init32(pevent);

#ifdef HAVE_V792
  read_v792(gAdcBase,"ADC1",pevent,32);
#endif
#ifdef HAVE_V1190B
  read_tdc(gTdcBase,pevent);
#endif
#ifdef HAVE_VF48
  read_vf48(gVF48base,pevent);
#endif

  return bk_size(pevent);
}

