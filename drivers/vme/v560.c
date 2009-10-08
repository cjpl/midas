/*********************************************************************

  Name:         v560.c
  Created by:   K.Olchanski

  Contents:     CAEN V560 16-channel 32-bit scaler

  $Id$
*********************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "v560.h"
#include "mvmestd.h"

/*****************************************************************/
/*
Read V560 register value
*/
static uint16_t regRead(MVME_INTERFACE *mvme, DWORD base, int offset)
{
  mvme_set_am(mvme, MVME_AM_A24);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  return mvme_read_value(mvme, base + offset);
}

/*****************************************************************/
/*
Write V560 register value
*/
static void regWrite(MVME_INTERFACE *mvme, DWORD base, int offset, uint16_t value)
{
  mvme_set_am(mvme, MVME_AM_A24);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base + offset, value);
}

uint16_t v560_RegisterRead(MVME_INTERFACE *mvme, DWORD base, int offset)
{
  return regRead(mvme,base,offset);
}

void     v560_RegisterWrite(MVME_INTERFACE *mvme, DWORD base, int offset, uint16_t value)
{
  regWrite(mvme,base,offset,value);
}

void v560_Read(MVME_INTERFACE *mvme, DWORD base, uint32_t data[16])
{
  int i;
  mvme_set_am(mvme, MVME_AM_A24);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  //mvme_set_blt(mvme, MVME_BLT_BLT32);
  //mvme_set_blt(mvme, MVME_BLT_NONE);
  //mvme_set_blt(mvme, 0);
  //mvme_read(mvme, data, base + 0x10, 16*4);

  for (i=0; i<16; i++)
    data[i] = mvme_read_value(mvme, base + 0x10 + 4*i);
}

/*****************************************************************/
void v560_Reset(MVME_INTERFACE *mvme, DWORD base)
{
  regRead(mvme,base,0x54); // VME VETO reset
  regRead(mvme,base,0x50); // scalers clear, vme interrupt clear and disable
}

/*****************************************************************/
void  v560_Status(MVME_INTERFACE *mvme, DWORD base)
{
  printf("CAEN V560 at A24 0x%x: version 0x%x, type 0x%x, code 0x%x, scaler status: 0x%x\n", (int)base,regRead(mvme,base,0xFE),regRead(mvme,base,0xFC),regRead(mvme,base,0xFA),regRead(mvme,base,0x58));
}

//end
