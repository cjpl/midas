/*********************************************************************

  Name:         sis3820drv.h
  Created by:   K.Olchanski

  Contents:     sis3820 32-channel 32-bit multiscaler
                
  $Log$
  Revision 1.1  2006/05/25 05:53:42  alpha
  First commit

*********************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mvmestd.h"
#include "sis3820.h"

#ifndef  SIS3820DRV_INCLUDE_H
#define  SIS3820DRV_INCLUDE_H

uint32_t sis3820_RegisterRead(MVME_INTERFACE *mvme, DWORD base, int offset);
void     sis3820_RegisterWrite(MVME_INTERFACE *mvme, DWORD base, int offset, uint32_t value);
int  sis3820_FifoRead(MVME_INTERFACE *mvme, DWORD base, void *pdest, int wcount);
int  sis3820_DataReady(MVME_INTERFACE *mvme, DWORD base);
uint32_t sis3820_ScalerRead(MVME_INTERFACE *mvme, DWORD base, int ch);
void sis3820_Reset(MVME_INTERFACE *mvme, DWORD base);
void sis3820_Status(MVME_INTERFACE *mvme, DWORD base);

#endif // SIS3820_INCLUDE_H
