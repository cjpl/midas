/*********************************************************************

  Name:         ov1740drv.h
  Created by:   K.Olchanski
                    implementation of the CAENCommLib functions
  Contents:     v1740 64-channel 50 MHz 12-bit ADC

  $Id$
                
*********************************************************************/
#ifndef  OV1740DRV_INCLUDE_H
#define  OV1740DRV_INCLUDE_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <CAENComm.h>
#include "v1740.h"

CAENComm_ErrorCode ov1740_GroupSet(int handle, uint32_t channel, uint32_t what, uint32_t that);
CAENComm_ErrorCode ov1740_GroupGet(int handle, uint32_t channel, uint32_t what, uint32_t *data);
CAENComm_ErrorCode ov1740_GroupGet(int handle, uint32_t channel, uint32_t what, uint32_t *data);
CAENComm_ErrorCode ov1740_GroupThresholdSet(int handle, uint32_t channel, uint32_t threshold);
CAENComm_ErrorCode ov1740_GroupOUThresholdSet(int handle, uint32_t channel, uint32_t threshold);
CAENComm_ErrorCode ov1740_GroupDACSet(int handle, uint32_t channel, uint32_t dac);
CAENComm_ErrorCode ov1740_GroupDACGet(int handle, uint32_t channel, uint32_t *dac);
CAENComm_ErrorCode ov1740_AcqCtl(int handle, uint32_t operation);
CAENComm_ErrorCode ov1740_GroupConfig(int handle, uint32_t operation);
CAENComm_ErrorCode ov1740_info(int handle, int *nchannels, uint32_t *data);
CAENComm_ErrorCode ov1740_BufferOccupancy(int handle, uint32_t channel, uint32_t *data);
CAENComm_ErrorCode ov1740_BufferFree(int handle, int nbuffer, uint32_t *mode);
CAENComm_ErrorCode ov1740_Status(int handle);
CAENComm_ErrorCode ov1740_Setup(int handle, int mode);


#endif // OV1740DRV_INCLUDE_H
