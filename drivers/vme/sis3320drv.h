/*********************************************************************

  Name:         sis3320drv.h
  Created by:   K.Olchanski

  Contents:     sis3320 8-channel 200 MHz 12-bit ADC

  $Id$
                
*********************************************************************/
#ifndef  SIS3320DRV_INCLUDE_H
#define  SIS3320DRV_INCLUDE_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mvmestd.h"

#include "sis3320.h"

uint32_t sis3320_RegisterRead(MVME_INTERFACE *mvme, uint32_t a32base, int offset);
void     sis3320_RegisterWrite(MVME_INTERFACE *mvme, uint32_t a32base, int offset, uint32_t value);
void sis3320_Reset(MVME_INTERFACE *mvme, uint32_t a32base);
void sis3320_Status(MVME_INTERFACE *mvme, uint32_t a32base);
int  sis3320_Setup(MVME_INTERFACE *mvme, uint32_t a32base, int mode);

#endif // SIS3320_INCLUDE_H
