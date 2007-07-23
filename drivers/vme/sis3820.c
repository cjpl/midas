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

uint32_t sis3820_RegisterRead(MVME_INTERFACE *mvme, DWORD base, int offset)
{
  return regRead(mvme,base,offset);
}

void     sis3820_RegisterWrite(MVME_INTERFACE *mvme, DWORD base, int offset, uint32_t value)
{
  regWrite(mvme,base,offset,value);
}

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

  rd = mvme_read(mvme, pdest, base + 0x800000, wcount*4);
  //printf("fifo read wcount: %d, rd: %d\n",wcount,rd);

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
  printf("CSR: 0x%x\n", regRead(mvme,base,SIS3820_CONTROL_STATUS));
  printf("ModuleID and Firmware: 0x%x\n", regRead(mvme,base,SIS3820_MODID));
  printf("Operation mode: 0x%x\n", regRead(mvme,base,SIS3820_OPERATION_MODE));
}

//end
