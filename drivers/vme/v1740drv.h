/*********************************************************************

  Name:         v1740drv.h
  Created by:   K.Olchanski

  Contents:     v1740 8-channel 200 MHz 12-bit ADC

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

void     v1740_Status(MVME_INTERFACE *mvme, uint32_t a32base);
int      v1740_Setup(MVME_INTERFACE *mvme, uint32_t a32base, int mode);
void     v1740_info(MVME_INTERFACE *mvme, uint32_t a32base, int *nch, uint32_t *n32w);
uint32_t v1740_DataRead(MVME_INTERFACE *mvme,uint32_t a32base, uint32_t *pdata, uint32_t n32w);
//PAA- replaced with KO from V1742 example
void     v1740_DataBlockRead(MVME_INTERFACE* mvme, uint32_t base     , uint32_t* pbuf32, int nwords32);
void     v1740_GroupThresholdSet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t threshold);
void     v1740_GroupDACSet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t dac);
int      v1740_GroupDACGet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t *dac);
void     v1740_GroupSet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t what, uint32_t that);
uint32_t v1740_GroupGet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t what);
void     v1740_GroupConfig(MVME_INTERFACE *mvme, uint32_t base, uint32_t operation);
void     v1740_Align64Set(MVME_INTERFACE *mvme, uint32_t base);

#endif // v1740DRV_INCLUDE_H
