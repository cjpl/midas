/*********************************************************************
  @file
  Name:         v792.c
  Created by:   Pierre-Andre Amaudruz
  Contributors: Sergio Ballestrero

  Contents:     V792 32ch. QDC
                V785 32ch. Peak ADC include
                V792 and V785 are mostly identical;
                v792_ identifies commands supported by both
                v792_ identifies commands supported by V792 only
                v785_ identifies commands supported by V785 only
                
  $Id$
*********************************************************************/
#include <stdio.h>
#include <string.h>
#include <signal.h>
#if defined(OS_LINUX)
#include <unistd.h>
#endif
#include "v792.h"

WORD v792_Read16(MVME_INTERFACE *mvme, DWORD base, int offset)
{
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  return mvme_read_value(mvme, base+offset);
}

void v792_Write16(MVME_INTERFACE *mvme, DWORD base, int offset, WORD value)
{
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+offset, value);
}

/*****************************************************************/
/*
Read single event, return event length (number of entries)
Uses single vme access! (1us/D32)
*/
int v792_EventRead(MVME_INTERFACE *mvme, DWORD base, DWORD *pdest, int *nentry)
{
  DWORD hdata;
  int   cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  *nentry = 0;
  if (v792_DataReady(mvme, base)) {
    do {
      hdata = mvme_read_value(mvme, base);
    } while (!(hdata & 0x02000000)); // skip up to the header

    pdest[*nentry] = hdata;
    *nentry += 1;
    do {
      pdest[*nentry] = mvme_read_value(mvme, base);
      *nentry += 1;
    } while (!(pdest[*nentry-1] & 0x04000000)); // copy until the trailer

    nentry--;
  }
  mvme_set_dmode(mvme, cmode);
  return *nentry;
}

/*****************************************************************/
/*
Read nentry of data from the data buffer. Will use the DMA engine
if size is larger than 127 bytes.
*/
int v792_DataRead(MVME_INTERFACE *mvme, DWORD base, DWORD *pdest, int *nentry)
{
  int  cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  *nentry = 128;
  if (v792_DataReady(mvme, base)) {
    mvme_read(mvme, pdest, base, *nentry*4);
  }
  mvme_set_dmode(mvme, cmode);
  return *nentry;
}

/*****************************************************************/
/**
 * Write Thresholds and read them back
 */
int v792_ThresholdWrite(MVME_INTERFACE *mvme, DWORD base, WORD *threshold)
{
  int k, cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  for (k=0; k<V792_MAX_CHANNELS ; k++) {
    mvme_write_value(mvme, base+V792_THRES_BASE+2*k, threshold[k] & 0x1FF);
  }

  for (k=0; k<V792_MAX_CHANNELS ; k++) {
    threshold[k] = mvme_read_value(mvme, base+V792_THRES_BASE+2*k) & 0x1FF;
  }

  mvme_set_dmode(mvme, cmode);
  return V792_MAX_CHANNELS;
}

/*****************************************************************/
/**
 * Read Thresholds
 */
int v792_ThresholdRead(MVME_INTERFACE *mvme, DWORD base, WORD *threshold)
{
  int k, cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);

  for (k=0; k<V792_MAX_CHANNELS ; k++) {
    threshold[k] = mvme_read_value(mvme, base+V792_THRES_BASE+2*k) & 0x1FF;
  }
  mvme_set_dmode(mvme, cmode);
  return V792_MAX_CHANNELS;
}

/*****************************************************************/
void v792_EvtTriggerSet(MVME_INTERFACE *mvme, DWORD base, int count)
{
  int cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V792_EVTRIG_REG_RW, (count & 0x1F));
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void v792_EvtCntRead(MVME_INTERFACE *mvme, DWORD base, DWORD *evtcnt)
{
  int cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  *evtcnt  = mvme_read_value(mvme, base+V792_EVT_CNT_L_RO);
  *evtcnt += (mvme_read_value(mvme, base+V792_EVT_CNT_H_RO) << 16);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
int v792_CSR1Read(MVME_INTERFACE *mvme, DWORD base)
{
  int status, cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  status = mvme_read_value(mvme, base+V792_CSR1_RO);
  mvme_set_dmode(mvme, cmode);
  return status;
}

/*****************************************************************/
int v792_isBusy(MVME_INTERFACE *mvme, DWORD base)
{
  int status, busy, timeout, cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  timeout = 1000;
  do {
    status = mvme_read_value(mvme, base+V792_CSR1_RO);
    busy = status & 0x4;
    timeout--;
  } while (busy || timeout);
  mvme_set_dmode(mvme, cmode);
  return (busy != 0 ? 1 : 0);
}

/*****************************************************************/
int v792_CSR2Read(MVME_INTERFACE *mvme, DWORD base)
{
  int status, cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  status = mvme_read_value(mvme, base+V792_CSR2_RO);
  mvme_set_dmode(mvme, cmode);
  return status;
}

/*****************************************************************/
int v792_BitSet2Read(MVME_INTERFACE *mvme, DWORD base)
{
  int status, cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  status = mvme_read_value(mvme, base+V792_BIT_SET2_RW);
  mvme_set_dmode(mvme, cmode);
  return status;
}

/*****************************************************************/
void v792_BitSet2Set(MVME_INTERFACE *mvme, DWORD base, WORD pat)
{
  int cmode;
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V792_BIT_SET1_RW, 0xA0);
  mvme_write_value(mvme, base+V792_BIT_CLEAR1_WO, 0xA0);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void v792_BitSet2Clear(MVME_INTERFACE *mvme, DWORD base, WORD pat)
{
  int cmode;
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V792_BIT_SET1_RW, 0xA0);
  mvme_write_value(mvme, base+V792_BIT_CLEAR1_WO, 0xA0);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void v792_DataClear(MVME_INTERFACE *mvme, DWORD base)
{
  int cmode;
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V792_BIT_SET2_RW, 0x4);
  mvme_write_value(mvme, base+V792_BIT_CLEAR2_WO, 0x4);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void v792_SoftReset(MVME_INTERFACE *mvme, DWORD base)
{
  int cmode;
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V792_BIT_SET1_RW, 0xA0);
  mvme_write_value(mvme, base+V792_BIT_CLEAR1_WO, 0xA0);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void v792_OnlineSet(MVME_INTERFACE *mvme, DWORD base)
{
  int cmode;
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V792_BIT_CLEAR2_WO, 0x2);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void v792_LowThEnable(MVME_INTERFACE *mvme, DWORD base)
{
  int cmode;
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V792_BIT_CLEAR2_WO, 0x10);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void v792_LowThDisable(MVME_INTERFACE *mvme, DWORD base)
{
  int cmode;
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V792_BIT_SET2_RW, 0x10);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void v792_EmptyEnable(MVME_INTERFACE *mvme, DWORD base)
{
  int cmode;
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V792_BIT_SET2_RW, 0x1000);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void v792_EvtCntReset(MVME_INTERFACE *mvme, DWORD base)
{
  int cmode;
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V792_EVT_CNT_RST_WO, 1);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void v792_SingleShotReset(MVME_INTERFACE *mvme, DWORD base)
{
  int cmode;
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V792_SINGLE_RST_WO, 1);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
int  v792_DataReady(MVME_INTERFACE *mvme, DWORD base)
{
  int data_ready, cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  data_ready = mvme_read_value(mvme, base+V792_CSR1_RO) & 0x1;
  mvme_set_dmode(mvme, cmode);
  return data_ready;
}

/*****************************************************************/
int  v792_isEvtReady(MVME_INTERFACE *mvme, DWORD base)
{
  int csr;
  csr = v792_CSR1Read(mvme, base);
  return (csr & 0x100);
}

/*****************************************************************/
void v792_IntSet(MVME_INTERFACE *mvme, DWORD base, int level, int vector)
{
  int cmode;
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V792_INT_VECTOR_WO, (vector & 0xFF));
  mvme_write_value(mvme, base+V792_INT_LEVEL_WO, (level & 0x7));
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void v792_IntEnable(MVME_INTERFACE *mvme, DWORD base, int level)
{
  int cmode;
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V792_EVTRIG_REG_RW, (level & 0x1F));
  /* Use the trigger buffer for int enable/disable
  mvme_write_value(mvme, base+V792_INT_LEVEL_WO, (level & 0x7));
  */
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void v792_IntDisable(MVME_INTERFACE *mvme, DWORD base)
{
  int cmode;
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V792_EVTRIG_REG_RW, 0);
  /* Use the trigger buffer for int enable/disable
     Setting a level 0 reboot the VMIC !
  mvme_write_value(mvme, base+V792_INT_LEVEL_WO, 0);
  */
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
/**
 * read Control Register 1 (0x1010,16 bit)
 */
WORD v792_ControlRegister1Read(MVME_INTERFACE *mvme, DWORD base) {
  int cmode;
  WORD pat;
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  pat = mvme_read_value(mvme, base+V792_CR1_RW);
  mvme_set_dmode(mvme, cmode);
  return pat;
}

/*****************************************************************/
/**
 * write Control Register 1 (0x1010,16 bit)
 */
void v792_ControlRegister1Write(MVME_INTERFACE *mvme, DWORD base, WORD pat) {
  int cmode;
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V792_CR1_RW,pat);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
/**
 * cause a software trigger
 */
void v792_Trigger(MVME_INTERFACE *mvme, DWORD base) {
  int cmode;
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base+V792_SWCOMM_WO, 0);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
/**
Sets all the necessary paramters for a given configuration.
The configuration is provided by the mode argument.
Add your own configuration in the case statement. Let me know
your setting if you want to include it in the distribution.
@param *mvme VME structure
@param  base Module base address
@param mode  Configuration mode number
@param *nentry number of entries requested and returned.
@return MVME_SUCCESS
*/
int  v792_Setup(MVME_INTERFACE *mvme, DWORD base, int mode)
{
  int  cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);

  switch (mode) {
  case 0x1:
    printf("Default setting after power up (mode:%d)\n", mode);
    printf("\n");
    break;
  case 0x2:
    printf("Modified setting (mode:%d)\n", mode);
    printf("Empty Enable, Over Range disable, Low Th Enable\n");
    v792_OnlineSet(mvme, base);
    v792_EmptyEnable(mvme, base);
    v792_LowThEnable(mvme, base);
    break;
  default:
    printf("Unknown setup mode\n");
    mvme_set_dmode(mvme, cmode);
    return -1;
  }
  mvme_set_dmode(mvme, cmode);
  return 0;
}

/*****************************************************************/
void  v792_Status(MVME_INTERFACE *mvme, DWORD base)
{
  int status, cmode, i;
  WORD threshold[V792_MAX_CHANNELS];

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  printf("V792 at VME A24 0x%06x:\n", base);
  status = mvme_read_value(mvme, base+V792_FIRM_REV);
  printf("Firmware revision: 0x%x\n", status);
  if (status == 0xFFFF) {
    printf("Module not present!\n");
    return;
  }
  status = v792_CSR1Read(mvme, base);
  printf("CSR1: 0x%x\n", status);
  printf("DataReady    :%s\t", status & 0x1 ? "Y" : "N");
  printf(" - Global Dready:%s\t", status & 0x2 ? "Y" : "N");
  printf(" - Busy         :%s\n", status & 0x4 ? "Y" : "N");
  printf("Global Busy  :%s\t", status & 0x8 ? "Y" : "N");
  printf(" - Amnesia      :%s\t", status & 0x10 ? "Y" : "N");
  printf(" - Purge        :%s\n", status & 0x20 ? "Y" : "N");
  printf("Term ON      :%s\t", status & 0x40 ? "Y" : "N");
  printf(" - TermOFF      :%s\t", status & 0x80 ? "Y" : "N");
  printf(" - Event Ready  :%s\n", status & 0x100 ? "Y" : "N");
  status = v792_CSR2Read(mvme, base);
  printf("CSR2: 0x%x\n", status);
  printf("Buffer Empty :%s\t", status & 0x2 ? "Y" : "N");
  printf(" - Buffer Full  :%s\n", status & 0x4 ? "Y" : "N");
  int dtype = (status & 0xF0) >> 4;
  printf("Daughter card type (CSEL/DSEL)  :%d%d%d%d (0x%x) ",
         status & 0x80 ? 1 : 0, 
         status & 0x40 ? 1 : 0,
         status & 0x20 ? 1 : 0,
         status & 0x10 ? 1 : 0,
         dtype);
  switch (dtype) {
  default:
    printf("\n");
    break;
  case 2:
    printf("V792 32ch QDC\n");
    break;
  }
  status = v792_BitSet2Read(mvme, base);
  printf("BitSet2: 0x%x\n", status);
  printf("Test Mem     :%s\t", status & 0x1 ? "Y" : "N");
  printf(" - Offline      :%s\t", status & 0x2 ? "Y" : "N");
  printf(" - Clear Data   :%s\n", status & 0x4  ? "Y" : "N");
  printf("Over Range En:%s\t", status & 0x8  ? "Y" : "N");
  printf(" - Low Thres En :%s\t", status & 0x10 ? "Y" : "N");
  printf(" - Auto Incr    :%s\n", status & 0x20 ? "Y" : "N");
  printf("Empty Enable :%s\t", status & 0x1000 ? "Y" : "N");
  printf(" - Slide sub En :%s\t", status & 0x2000 ? "Y" : "N");
  printf(" - All Triggers :%s\n", status & 0x4000 ? "Y" : "N");
  v792_EvtCntRead(mvme, base, &status);
  printf("Event counter: %d\n", status);
  printf("Iped value: %d\n", v792_Read16(mvme, base, V792_IPED_RW));

  v792_ThresholdRead(mvme, base, threshold);
  for (i=0;i<V792_MAX_CHANNELS;i+=2) {
    printf("Threshold[%2i] = 0x%4.4x\t   -  ", i, threshold[i]);
    printf("Threshold[%2i] = 0x%4.4x\n", i+1, threshold[i+1]);
  }
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
/**
 * decoded printout of readout entry
 * Not to be trusted for data decoding but acceptable for display
 * purpose as its implementation is strongly compiler dependent and
 * not flawless.
 * @param v
 */
void v792_printEntry(const v792_Data* v) {
  switch (v->data.type) {
  case v792_typeMeasurement:
    printf("Data=0x%08x Measurement ch=%3d v=%6d over=%1d under=%1d\n",
	   (int)v->raw,v->data.channel,v->data.adc,v->data.ov,v->data.un);
    break;
  case v792_typeHeader:
    printf("Data=0x%08x Header geo=%2x crate=%2x cnt=%2d\n",
    	   (int)v->raw,v->header.geo,v->header.crate,v->header.cnt);
    break;
  case v792_typeFooter:
    printf("Data=0x%08x Footer geo=%2x evtCnt=%7d\n",
    	   (int)v->raw,v->footer.geo,v->footer.evtCnt);
    break;
  case v792_typeFiller:
    printf("Data=0x%08x Filler\n",(int)v->raw);
    break;
  default:
    printf("Data=0x%08x Unknown %04x\n",(int)v->raw,v->data.type);
    break;
  }
}

/*****************************************************************/
int v792_isPresent(MVME_INTERFACE *mvme, DWORD base)
{
  int status, cmode;
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  status = mvme_read_value(mvme, base+V792_FIRM_REV);
  mvme_set_dmode(mvme, cmode);
  if (status == 0xFFFF)
    return 0;
  else
    return 1;
}

/*****************************************************************/
/*-PAA- For test purpose only */
#ifdef MAIN_ENABLE
int main (int argc, char* argv[]) {

  DWORD VMEIO_BASE = 0x780000;
  DWORD V792_BASE  = 0x500000;

  MVME_INTERFACE *myvme;

  int status, csr, i;
  DWORD cnt;
  DWORD dest[1000];
  WORD  threshold[32];

  if (argc>1) {
    sscanf(argv[1],"%lx",&V792_BASE);
  }

  // Test under vmic
  status = mvme_open(&myvme, 0);

  // Set am to A24 non-privileged Data
  mvme_set_am(myvme, MVME_AM_A24_ND);

  // Set dmode to D32
  mvme_set_dmode(myvme, MVME_DMODE_D32);

  v792_SingleShotReset(myvme, V792_BASE);

  for (i=0;i<V792_MAX_CHANNELS;i++) {
    threshold[i] = 0;
  }
  v792_ThresholdWrite(myvme, V792_BASE, threshold);
  v792_DataClear(myvme, V792_BASE);
  v792_Setup(myvme, V792_BASE, 2);

  v792_Status(myvme, V792_BASE);

  while (1) {
    do {
      csr = v792_CSR1Read(myvme, V792_BASE);
      printf("CSR1: 0x%x\n", csr);
      csr = v792_CSR2Read(myvme, V792_BASE);
      printf("CSR2: 0x%x\n", csr);
      printf("Busy : %d\n", v792_isBusy(myvme, V792_BASE));
#if defined(OS_LINUX)
      sleep(1);
#endif
    } while (csr == 0);

    // Read Event Counter
    v792_EvtCntRead(myvme, V792_BASE, &cnt);
    printf("Event counter: 0x%lx\n", cnt);

    // Set 0x3 in pulse mode for timing purpose
    mvme_write_value(myvme, VMEIO_BASE+0x8, 0xF);

    // Write pulse for timing purpose
    mvme_write_value(myvme, VMEIO_BASE+0xc, 0x2);

    //  cnt=32;
    //  v792_DataRead(myvme, V792_BASE, dest, &cnt);

    // Write pulse for timing purpose
    mvme_write_value(myvme, VMEIO_BASE+0xc, 0x8);

    //  for (i=0;i<32;i++) {
    //    printf("Data[%i]=0x%x\n", i, dest[i]);
    //  }

    //  status = 32;
    v792_EventRead(myvme, V792_BASE, dest, &status);
    printf("count: 0x%x\n", status);
    for (i=0;i<status;i++) {
      printf("%02d)",i);
      v792_printEntry((v792_Data*)&dest[i]);
    }
  }
  status = mvme_close(myvme);
  return 1;
}
#endif

/* emacs
 * Local Variables:
 * mode:C
 * mode:font-lock
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
