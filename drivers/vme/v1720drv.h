/*********************************************************************

  Name:         v1720drv.h
  Created by:   K.Olchanski

  Contents:     v1720 8-channel 250 MHz 12-bit ADC

  $Id$
                
*********************************************************************/
#ifndef  V1720DRV_INCLUDE_H
#define  V1720DRV_INCLUDE_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mvmestd.h"

#include "v1720.h"

uint32_t v1720_RegisterRead(MVME_INTERFACE *mvme, uint32_t a32base, int offset);
uint32_t v1720_BufferFreeRead(MVME_INTERFACE *mvme, uint32_t a32base);
uint32_t v1720_BufferOccupancy(MVME_INTERFACE *mvme, uint32_t a32base, uint32_t channel);
uint32_t v1720_BufferFree(MVME_INTERFACE *mvme, uint32_t base, int nbuffer);

void     v1720_AcqCtl(MVME_INTERFACE *mvme, uint32_t a32base, uint32_t operation);
void     v1720_ChannelCtl(MVME_INTERFACE *mvme, uint32_t a32base, uint32_t reg, uint32_t mask);
void     v1720_TrgCtl(MVME_INTERFACE *mvme, uint32_t a32base, uint32_t reg, uint32_t mask);

void     v1720_RegisterWrite(MVME_INTERFACE *mvme, uint32_t a32base, int offset, uint32_t value);
void     v1720_Reset(MVME_INTERFACE *mvme, uint32_t a32base);

void     v1720_Status(MVME_INTERFACE *mvme, uint32_t a32base);
int      v1720_Setup(MVME_INTERFACE *mvme, uint32_t a32base, int mode);
void     v1720_info(MVME_INTERFACE *mvme, uint32_t a32base, int *nch, uint32_t *n32w);
uint32_t v1720_DataRead(MVME_INTERFACE *mvme,uint32_t a32base, uint32_t *pdata, uint32_t n32w);
void     v1720_DataBlockRead(MVME_INTERFACE* mvme, uint32_t base     , uint32_t* pbuf32, int nwords32);
//uint32_t v1720_DataBlockRead(MVME_INTERFACE *mvme, uint32_t a32base, uint32_t *pdest, uint32_t *nentry);
void     v1720_ChannelThresholdSet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t threshold);
void     v1720_ChannelOUThresholdSet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t threshold);
void     v1720_ChannelDACSet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t dac);
int      v1720_ChannelDACGet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t *dac);
void     v1720_ChannelSet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t what, uint32_t that);
uint32_t v1720_ChannelGet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t what);
void     v1720_ChannelConfig(MVME_INTERFACE *mvme, uint32_t base, uint32_t operation);
void     v1720_Align64Set(MVME_INTERFACE *mvme, uint32_t base);

#endif // v1720DRV_INCLUDE_H
