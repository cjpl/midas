/*********************************************************************

  Name:         v560.h
  Created by:   K.Olchanski

  Contents:     CAEN V560 16-channel 32-bit scaler

  $Id$
*********************************************************************/

#ifndef  V560_INCLUDE_H
#define  V560_INCLUDE_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mvmestd.h"

uint16_t v560_RegisterRead(MVME_INTERFACE *mvme, DWORD base, int offset);
void     v560_RegisterWrite(MVME_INTERFACE *mvme, DWORD base, int offset, uint16_t value);
void     v560_Read(MVME_INTERFACE *mvme, DWORD base, uint32_t data[16]);
void     v560_Reset(MVME_INTERFACE *mvme, DWORD base);
void     v560_Status(MVME_INTERFACE *mvme, DWORD base);

#endif // V560_INCLUDE_H
