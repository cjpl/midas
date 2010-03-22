/*********************************************************************

  Name:         isegvhsdrv.h
  Created by:   P.-A. Amaudruz

  Contents:     isegvhs HV 12-channels VME
                
  $Id$
*********************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mvmestd.h"

#ifndef  ISEGVHSDRV_INCLUDE_H
#define  ISEGVHSDRV_INCLUDE_H

uint32_t isegvhs_RegisterRead(MVME_INTERFACE *mvme, DWORD base, int offset);
float    isegvhs_RegisterReadFloat(MVME_INTERFACE *mvme, DWORD base, int offset);

void     isegvhs_RegisterWrite(MVME_INTERFACE *mvme, DWORD base, int offset, uint32_t value);
void     isegvhs_RegisterWriteFloat(MVME_INTERFACE *mvme, DWORD base, int offset, float value);

void isegvhs_Reset(MVME_INTERFACE *mvme, DWORD base);
void isegvhs_Status(MVME_INTERFACE *mvme, DWORD base);

#endif // ISEGVHS_INCLUDE_H
