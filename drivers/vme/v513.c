/*********************************************************************

  Name:         v513.c
  Created by:   K.Olchanski

  Contents:     CAEN V513 16-channel NIM I/O register

  $Id$
*********************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "v513.h"
#include "mvmestd.h"

/*****************************************************************/
/*
Read V513 register value
*/
static uint16_t regRead(MVME_INTERFACE *mvme, DWORD base, int offset)
{
  mvme_set_am(mvme, MVME_AM_A24);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  return mvme_read_value(mvme, base + offset);
}

/*****************************************************************/
/*
Write V513 register value
*/
static void regWrite(MVME_INTERFACE *mvme, DWORD base, int offset, uint16_t value)
{
  mvme_set_am(mvme, MVME_AM_A24);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base + offset, value);
}

uint16_t v513_RegisterRead(MVME_INTERFACE *mvme, DWORD base, int offset)
{
  return regRead(mvme,base,offset);
}

void     v513_RegisterWrite(MVME_INTERFACE *mvme, DWORD base, int offset, uint16_t value)
{
  regWrite(mvme,base,offset,value);
}

uint16_t v513_Read(MVME_INTERFACE *mvme, DWORD base)
{
  return regRead(mvme,base,0x4);
}

void v513_Write(MVME_INTERFACE *mvme, DWORD base, uint16_t data)
{
  regWrite(mvme,base,0x4,data);
}

/*****************************************************************/
void v513_Reset(MVME_INTERFACE *mvme, DWORD base)
{
  regWrite(mvme,base,0x46,0); // reset to default configuration
  regWrite(mvme,base,0x42,0); // module reset
  //regRead(mvme,base,0x50); // scalers clear, vme interrupt clear and disable
  regRead(mvme,base,0); // flush posted writes
}

/*****************************************************************/
void  v513_Status(MVME_INTERFACE *mvme, DWORD base)
{
  printf("CAEN V513 at A24 0x%x: version 0x%x, type 0x%x, code 0x%x\n", (int)base,regRead(mvme,base,0xFE),regRead(mvme,base,0xFC),regRead(mvme,base,0xFA));
}

void v513_SetChannelMode(MVME_INTERFACE *mvme, DWORD base, int channel, int mode)
{
  regWrite(mvme,base,0x10 + 2*channel, mode);
}

int v513_GetChannelMode(MVME_INTERFACE *mvme, DWORD base, int channel)
{
  return regRead(mvme,base,0x10 + 2*channel);
}

//end
