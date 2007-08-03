/*********************************************************************

  Name:         sis3320.c
  Created by:   K.Olchanski

  Contents:     SIS3320 32-channel 32-bit multiscaler
                
  $Log$
  Revision 1.1  2006/05/25 05:53:42  alpha
  First commit

*********************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "sis3320drv.h"
#include "sis3320.h"
#include "mvmestd.h"

/*****************************************************************/
/*
Read sis3320 register value
*/
static uint32_t regRead(MVME_INTERFACE *mvme, uint32_t base, int offset)
{
  mvme_set_am(mvme, MVME_AM_A32);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  return mvme_read_value(mvme, base + offset);
}

/*****************************************************************/
/*
Write sis3320 register value
*/
static void regWrite(MVME_INTERFACE *mvme, uint32_t base, int offset, uint32_t value)
{
  mvme_set_am(mvme, MVME_AM_A32);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  mvme_write_value(mvme, base + offset, value);
}

uint32_t sis3320_RegisterRead(MVME_INTERFACE *mvme, uint32_t base, int offset)
{
  return regRead(mvme,base,offset);
}

void     sis3320_RegisterWrite(MVME_INTERFACE *mvme, uint32_t base, int offset, uint32_t value)
{
  regWrite(mvme,base,offset,value);
}

/*****************************************************************/
void sis3320_Reset(MVME_INTERFACE *mvme, uint32_t base)
{
  regWrite(mvme,base,SIS3320_KEY_RESET,0);
}

/*****************************************************************/
void  sis3320_Status(MVME_INTERFACE *mvme, uint32_t base)
{
  printf("SIS3320 at A32 0x%x\n", (int)base);
  printf("CSR: 0x%x\n", regRead(mvme,base,SIS3320_CONTROL_STATUS));
  printf("ModuleID and Firmware: 0x%x\n", regRead(mvme,base,SIS3320_MODID));
  //printf("Operation mode: 0x%x\n", regRead(mvme,base,SIS3320_OPERATION_MODE));
}

//end
