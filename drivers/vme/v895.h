/*********************************************************************

  Name:         v895.h
  Created by:   Konstantin Olchanski

  Contents:     V895 16ch. VME discriminator include

  $Id$

*********************************************************************/

#ifndef  __V895_INCLUDE_H__
#define  __V895_INCLUDE_H__

#include <stdio.h>
#include <string.h>
#include <stdio.h>

#include "mvmestd.h"

uint8_t v895_readReg8(MVME_INTERFACE *mvme,int base,int ireg)
{
  mvme_set_am(mvme,MVME_AM_A24);
  mvme_set_dmode(mvme,MVME_DMODE_D8);
  return mvme_read_value(mvme,base + ireg);
}

void v895_writeReg8(MVME_INTERFACE *mvme,int base,int ireg,uint8_t value)
{
  mvme_set_am(mvme,MVME_AM_A24);
  mvme_set_dmode(mvme,MVME_DMODE_D8);
  mvme_write_value(mvme,base + ireg,value);
}

uint16_t v895_readReg16(MVME_INTERFACE *mvme,int base,int ireg)
{
  mvme_set_am(mvme,MVME_AM_A24);
  mvme_set_dmode(mvme,MVME_DMODE_D16);
  return mvme_read_value(mvme,base + ireg);
}

void v895_writeReg16(MVME_INTERFACE *mvme,int base,int ireg,uint16_t value)
{
  mvme_set_am(mvme,MVME_AM_A24);
  mvme_set_dmode(mvme,MVME_DMODE_D16);
  mvme_write_value(mvme,base + ireg,value);
}

int v895_Status(MVME_INTERFACE *mvme,int base)
{
  printf("V895 at VME A24 0x%x: fixed code (0xFAF5): 0x%x, module type: 0x%x, version: 0x%x\n",
	 base,
	 v895_readReg16(mvme,base,0xfa),
	 v895_readReg16(mvme,base,0xfc),
	 v895_readReg16(mvme,base,0xfe));
  return 0;
}

void v895_TestPulse(MVME_INTERFACE *mvme,int base)
{
  v895_writeReg16(mvme,base,0x4C,1); // fire test pulse
}

#endif
// end file
