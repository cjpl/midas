/*********************************************************************

  Name:         mesadc32drv.h
  Created by:   K.Olchanski

  Contents:     mesadc32 8-channel 200 MHz 12-bit ADC

  $Id: mesadc32drv.h 5177 2011-08-12 02:26:08Z amaudruz $
                
*********************************************************************/
#ifndef  MESADC32DRV_INCLUDE_H
#define  MESADC32DRV_INCLUDE_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mvmestd.h"

#include "mesadc32.h"
int      mesadc32_ReadData(MVME_INTERFACE *mvme, uint32_t a32base, uint32_t *pdata, int *len);
void     mesadc32_RegisterWrite(MVME_INTERFACE *mvme, uint32_t base, int offset, uint32_t value);
uint32_t mesadc32_RegisterRead(MVME_INTERFACE *mvme, uint32_t a32base, int offset);
uint32_t mesadc32_BufferFreeRead(MVME_INTERFACE *mvme, uint32_t a32base);
uint32_t mesadc32_BufferOccupancy(MVME_INTERFACE *mvme, uint32_t a32base, uint32_t channel);
uint32_t mesadc32_BufferFree(MVME_INTERFACE *mvme, uint32_t base, int nbuffer);

void     mesadc32_AcqCtl(MVME_INTERFACE *mvme, uint32_t a32base, uint32_t operation);
void     mesadc32_ChannelCtl(MVME_INTERFACE *mvme, uint32_t a32base, uint32_t reg, uint32_t mask);
void     mesadc32_TrgCtl(MVME_INTERFACE *mvme, uint32_t a32base, uint32_t reg, uint32_t mask);

void     mesadc32_RegisterWrite(MVME_INTERFACE *mvme, uint32_t a32base, int offset, uint32_t value);
void     mesadc32_Reset(MVME_INTERFACE *mvme, uint32_t a32base);

void     mesadc32_Status(MVME_INTERFACE *mvme, uint32_t a32base);
int      mesadc32_Setup(MVME_INTERFACE *mvme, uint32_t a32base, int mode);
void     mesadc32_info(MVME_INTERFACE *mvme, uint32_t a32base, int *nch, uint32_t *n32w);

uint32_t mesadc32_MSCF16_Set(MVME_INTERFACE *mvme, uint32_t base, uint32_t modAdd, int subAdd, int data);
uint32_t mesadc32_MSCF16_Get(MVME_INTERFACE *mvme, uint32_t base, uint32_t modAdd, int subAdd, uint32_t *data);
uint32_t mesadc32_MSCF16_IDC(MVME_INTERFACE *mvme, uint32_t base, uint32_t modAdd);
uint32_t mesadc32_MSCF16_RConoff(MVME_INTERFACE *mvme, uint32_t base, uint32_t modAdd, int onoff);

void mesadc32_ThresholdSet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t thres);
void mesadc32_ThresholdGet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t *thres);

#endif // MESADC32DRV_INCLUDE_H
