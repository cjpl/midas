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

class v895
{
 public:
  MVME_INTERFACE* fVme;
  uint32_t fVmeA24;

  v895(MVME_INTERFACE* vme,  uint32_t vmeA24) // ctor
    {
      fVme = vme;
      fVmeA24 = vmeA24;
    }

  uint8_t readReg8(int ireg)
  {
    mvme_set_dmode(fVme, MVME_DMODE_D8);
    return mvme_read_value(fVme, fVmeA24 + ireg);
  }

  void writeReg8(int ireg, uint8_t value)
  {
    mvme_set_dmode(fVme, MVME_DMODE_D8);
    mvme_write_value(fVme, fVmeA24 + ireg, value);
  }

  uint16_t readReg16(int ireg)
  {
    mvme_set_dmode(fVme, MVME_DMODE_D16);
    return mvme_read_value(fVme, fVmeA24 + ireg);
  }
  
  void writeReg16(int ireg, uint16_t value)
  {
    mvme_set_dmode(fVme, MVME_DMODE_D16);
    mvme_write_value(fVme, fVmeA24 + ireg, value);
  }
  
  int Status()
  {
    printf("V895 at VME A24 0x%x: fixed code (0xFAF5): 0x%x, module type: 0x%x, version: 0x%x\n",
	   fVmeA24,
	   readReg16(0xfa),
	   readReg16(0xfc),
	   readReg16(0xfe));
    return 0;
  }
  
  void TestPulse()
  {
    writeReg16(0x4C,1); // fire test pulse
  }
};

#endif
// end file
