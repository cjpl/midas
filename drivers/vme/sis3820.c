
/*********************************************************************

  Name:         sis3820.c
  Created by:   K.Olchanski

  Contents:     SIS3820 32-channel 32-bit multiscaler
                
  $Log$
  Revision 1.1  2006/05/25 05:53:42  alpha
  First commit

*********************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "sis3820drv.h"
#include "sis3820.h"
#include "mvmestd.h"

/*****************************************************************/
/*
Read sis3820 register value
*/
static uint32_t regRead(MVME_INTERFACE *mvme, DWORD base, int offset)
{
  mvme_set_am(mvme, MVME_AM_A32);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  return mvme_read_value(mvme, base + offset);
}

/*****************************************************************/
/*
Write sis3820 register value
*/
static void regWrite(MVME_INTERFACE *mvme, DWORD base, int offset, uint32_t value)
{
  mvme_set_am(mvme, MVME_AM_A32);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  mvme_write_value(mvme, base + offset, value);
}

/*****************************************************************/
uint32_t sis3820_RegisterRead(MVME_INTERFACE *mvme, DWORD base, int offset)
{
  return regRead(mvme, base, offset);
}

/*****************************************************************/
void     sis3820_RegisterWrite(MVME_INTERFACE *mvme, DWORD base, int offset, uint32_t value)
{
  regWrite(mvme,base,offset,value);
}

/*****************************************************************/
uint32_t sis3820_ScalerRead(MVME_INTERFACE *mvme, DWORD base, int ch)
{
  if (ch == 0)
    return regRead(mvme, base, SIS3820_COUNTER_CH1+ch*4);
  else
    return regRead(mvme, base, SIS3820_COUNTER_SHADOW_CH1+ch*4);
}

/*****************************************************************/
/*
Read nentry of data from the data buffer. Will use the DMA engine
if size is larger then 127 bytes. 
*/
int sis3820_FifoRead(MVME_INTERFACE *mvme, DWORD base, void *pdest, int wcount)
{
  int rd;
  int save_am;

    mvme_get_am(mvme,&save_am);
    mvme_set_blt(  mvme, MVME_BLT_MBLT64);
    mvme_set_am(   mvme, MVME_AM_A32_D64);

  rd = mvme_read(mvme, pdest, base + SIS3820_FIFO_BASE, wcount*4);
  //  printf("fifo read wcount: %d, rd: %d\n", wcount, rd);
  mvme_set_am(mvme, save_am);


  return wcount;
}

/*****************************************************************/
void sis3820_Reset(MVME_INTERFACE *mvme, DWORD base)
{
  regWrite(mvme,base,SIS3820_KEY_RESET,0);
}

/*****************************************************************/
int  sis3820_DataReady(MVME_INTERFACE *mvme, DWORD base)
{
  return regRead(mvme,base,SIS3820_FIFO_WORDCOUNTER);
}

/*****************************************************************/
void  sis3820_Status(MVME_INTERFACE *mvme, DWORD base)
{
  printf("SIS3820 at A32 0x%x\n", (int)base);
  printf("ModuleID and Firmware: 0x%x\n", regRead(mvme,base,SIS3820_MODID));
  printf("CSR: 0x%x\n", regRead(mvme,base,SIS3820_CONTROL_STATUS));
  printf("Operation mode: 0x%x\n", regRead(mvme,base,SIS3820_OPERATION_MODE));
  printf("Inhibit/Count disable: 0x%x\n", regRead(mvme, base, SIS3820_COUNTER_INHIBIT));
  printf("Counter Overflow: 0x%x\n", regRead(mvme, base, SIS3820_COUNTER_OVERFLOW));
}

/*****************************************************************/
/*-PAA- For test purpose only */
#ifdef MAIN_ENABLE
int main (int argc, char* argv[]) {
  int i;
  uint32_t SIS3820_BASE  = 0x38000000;
  uint32_t dest[32], scaler;
  MVME_INTERFACE *myvme;
  int status;

  if (argc>1) {
    sscanf(argv[1],"%x", &SIS3820_BASE);
  }

  // Test under vmic
  status = mvme_open(&myvme, 0);

  regWrite(myvme, SIS3820_BASE, SIS3820_OPERATION_MODE, 0x1);
  //  regWrite(myvme, SIS3820_BASE, SIS3820_COUNTER_INHIBIT, 0x00000000);  // Enabled
  regWrite(myvme, SIS3820_BASE, SIS3820_CONTROL_STATUS, 0x00);
  sis3820_Reset(myvme, SIS3820_BASE);
  for (i=0;i<8;i++) {
    scaler = sis3820_ScalerRead(myvme, SIS3820_BASE, i);
    printf("scaler[%i] = %d\n", i, scaler);
  }
  regWrite(myvme, SIS3820_BASE, SIS3820_KEY_OPERATION_ENABLE, 0x00);
  sleep(10);
  regWrite(myvme, SIS3820_BASE, SIS3820_KEY_OPERATION_DISABLE, 0x00);
  sis3820_Status(myvme, SIS3820_BASE);

  //  sis3820_RegisterWrite(myvme, SIS3820_BASE, SIS3820_COUNTER_CLEAR, 0xFFFFFFFF);
  //  sis3820_FifoRead(myvme, SIS3820_BASE, &dest[0], 32);
  for (i=0;i<8;i++) {
    scaler = sis3820_ScalerRead(myvme, SIS3820_BASE, i);
    printf("scaler[%i] = %d\n", i, scaler);
  }
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

//end
