/*********************************************************************

  Name:         ov1720drv.h
  Created by:   K.Olchanski
                    implementation of the CAENCommLib functions
  Contents:     v1720 8-channel 200 MHz 12-bit ADC

  $Id$
                
*********************************************************************/
#ifndef  OV1720DRV_INCLUDE_H
#define  OV1720DRV_INCLUDE_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "CAENComm.h"
#include "v1720.h"

CAENComm_ErrorCode ov1720_ChannelSet(int handle, uint32_t channel, uint32_t what, uint32_t that);
CAENComm_ErrorCode ov1720_ChannelGet(int handle, uint32_t channel, uint32_t what, uint32_t *data);
CAENComm_ErrorCode ov1720_ChannelGet(int handle, uint32_t channel, uint32_t what, uint32_t *data);
CAENComm_ErrorCode ov1720_ChannelThresholdSet(int handle, uint32_t channel, uint32_t threshold);
CAENComm_ErrorCode ov1720_ChannelOUThresholdSet(int handle, uint32_t channel, uint32_t threshold);
CAENComm_ErrorCode ov1720_ChannelDACSet(int handle, uint32_t channel, uint32_t dac);
CAENComm_ErrorCode ov1720_ChannelDACGet(int handle, uint32_t channel, uint32_t *dac);
CAENComm_ErrorCode ov1720_AcqCtl(int handle, uint32_t operation);
CAENComm_ErrorCode ov1720_ChannelConfig(int handle, uint32_t operation);
CAENComm_ErrorCode ov1720_info(int handle, int *nchannels, uint32_t *data);
CAENComm_ErrorCode ov1720_BufferOccupancy(int handle, uint32_t channel, uint32_t *data);
CAENComm_ErrorCode ov1720_BufferFree(int handle, int nbuffer, uint32_t *mode);
CAENComm_ErrorCode ov1720_Status(int handle);
CAENComm_ErrorCode ov1720_Setup(int handle, int mode);


#endif // OV1720DRV_INCLUDE_H
