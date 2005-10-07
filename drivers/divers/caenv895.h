/*********************************************************************

  Name:         v895.h
  Created by:   Konstantin Olchanski

  Contents:     V895 16ch. VME discriminator include

  $Id:$

*********************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdio.h>

#ifndef  __V895_INCLUDE_H__
#define  __V895_INCLUDE_H__

uint8_t v895_readReg8(const void*ptr,int ireg)
{
  return *(uint8_t*)(((char*)ptr) + ireg);
}

void v895_writeReg8(void*ptr,int ireg,uint8_t value)
{
  *(uint8_t*)(((char*)ptr) + ireg) = value;
}

uint16_t v895_readReg16(const void*ptr,int ireg)
{
  return *(uint16_t*)(((char*)ptr) + ireg);
}

void v895_writeReg16(void*ptr,int ireg,uint16_t value)
{
  *(uint16_t*)(((char*)ptr) + ireg) = value;
}

int v895_Status(const DWORD*ptr)
{
  printf("V895 at %p: fixed code (0xFAF5): 0x%x, module type: 0x%x, version: 0x%x\n",
	 ptr,
	 v895_readReg16(ptr,0xfa),
	 v895_readReg16(ptr,0xfc),
	 v895_readReg16(ptr,0xfe));
  return 0;
}

void v895_TestPulse(DWORD*ptr)
{
  v895_writeReg16(ptr,0x4C,1); // fire test pulse
}

#endif
// end file
