/*********************************************************************

  Name:         v1740drv.h
  Created by:   P.-A. Amaudruz

  Contents:     v1740 64-channels 65Msps 12-bit ADC

  $Id$
                
*********************************************************************/
#ifndef  V1740DRV_INCLUDE_H
#define  V1740DRV_INCLUDE_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mvmestd.h"

#include "v1740.h"

uint32_t v1740_RegisterRead(MVME_INTERFACE *mvme, uint32_t a32base, int offset);
uint32_t v1740_BufferFreeRead(MVME_INTERFACE *mvme, uint32_t a32base);
uint32_t v1740_BufferOccupancy(MVME_INTERFACE *mvme, uint32_t a32base, uint32_t channel);
uint32_t v1740_BufferFree(MVME_INTERFACE *mvme, uint32_t base, int nbuffer);

void     v1740_AcqCtl(MVME_INTERFACE *mvme, uint32_t a32base, uint32_t operation);
void     v1740_GroupCtl(MVME_INTERFACE *mvme, uint32_t a32base, uint32_t reg, uint32_t mask);
void     v1740_TrgCtl(MVME_INTERFACE *mvme, uint32_t a32base, uint32_t reg, uint32_t mask);

void     v1740_RegisterWrite(MVME_INTERFACE *mvme, uint32_t a32base, int offset, uint32_t value);
void     v1740_Reset(MVME_INTERFACE *mvme, uint32_t a32base);
void     v1740_GroupThreshold(MVME_INTERFACE *mvme, uint32_t base, uint16_t group, uint16_t threshold);
void     v1740_GroupDAC(MVME_INTERFACE *mvme, uint32_t base, uint16_t group, uint16_t dac);
void     v1740_Status(MVME_INTERFACE *mvme, uint32_t a32base);
int      v1740_Setup(MVME_INTERFACE *mvme, uint32_t a32base, int mode);
void     v1740_info(MVME_INTERFACE *mvme, uint32_t a32base, int *nch, uint32_t *n32w);
uint32_t v1740_DataRead(MVME_INTERFACE *mvme,uint32_t a32base, uint32_t *pdata, uint32_t n32w);
uint32_t v1740_DataBlockRead(MVME_INTERFACE *mvme, uint32_t a32base, uint32_t *pdest, uint32_t *nentry);

#endif // v1740DRV_INCLUDE_H
