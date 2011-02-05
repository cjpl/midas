/*********************************************************************

  Name:         ov1720.c
  Created by:   Pierre-A. Amaudruz / K.Olchanski
                    implementation of the CAENCommLib functions
  Contents:     V1720 8 ch. 12bit 250Msps for Optical link
 
  $Id$
*********************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "ov1720drv.h"

// Buffer organization map for number of samples
uint32_t V1720_NSAMPLES_MODE[11] = { (1<<20), (1<<19), (1<<18), (1<<17), (1<<16), (1<<15)
              ,(1<<14), (1<<13), (1<<12), (1<<11), (1<<10)};

/*****************************************************************/
CAENComm_ErrorCode ov1720_ChannelSet(int handle, uint32_t channel, uint32_t what, uint32_t that)
{
  uint32_t reg, mask;

  if (what == V1720_CHANNEL_THRESHOLD)   mask = 0x0FFF;
  if (what == V1720_CHANNEL_OUTHRESHOLD) mask = 0x0FFF;
  if (what == V1720_CHANNEL_DAC)         mask = 0xFFFF;
  reg = what | (channel << 8);
  return CAENComm_Write32(handle, reg, (that & 0xFFF));
}

/*****************************************************************/
CAENComm_ErrorCode ov1720_ChannelGet(int handle, uint32_t channel, uint32_t what, uint32_t *data)
{
  uint32_t reg, mask;

  if (what == V1720_CHANNEL_THRESHOLD)   mask = 0x0FFF;
  if (what == V1720_CHANNEL_OUTHRESHOLD) mask = 0x0FFF;
  if (what == V1720_CHANNEL_DAC)         mask = 0xFFFF;
  reg = what | (channel << 8);
  return CAENComm_Read32(handle, reg, data);
}

/*****************************************************************/
CAENComm_ErrorCode ov1720_ChannelThresholdSet(int handle, uint32_t channel, uint32_t threshold)
{
  uint32_t reg;
  reg = V1720_CHANNEL_THRESHOLD | (channel << 8);
  printf("reg:0x%x, threshold:%x\n", reg, threshold);
  return CAENComm_Write32(handle, reg,(threshold & 0xFFF));
}

/*****************************************************************/
CAENComm_ErrorCode ov1720_ChannelOUThresholdSet(int handle, uint32_t channel, uint32_t threshold)
{
  uint32_t reg;
  reg = V1720_CHANNEL_OUTHRESHOLD | (channel << 8);
  printf("reg:0x%x, outhreshold:%x\n", reg, threshold);
  return CAENComm_Write32(handle, reg, (threshold & 0xFFF));
}

/*****************************************************************/
CAENComm_ErrorCode ov1720_ChannelDACSet(int handle, uint32_t channel, uint32_t dac)
{
  uint32_t reg;

  reg = V1720_CHANNEL_DAC | (channel << 8);
  printf("reg:0x%x, DAC:%x\n", reg, dac);
  return CAENComm_Write32(handle, reg, (dac & 0xFFFF));
}

/*****************************************************************/
CAENComm_ErrorCode ov1720_ChannelDACGet(int handle, uint32_t channel, uint32_t *dac)
{
  uint32_t reg;
  CAENComm_ErrorCode sCAEN;

  reg = V1720_CHANNEL_DAC | (channel << 8);
  sCAEN = CAENComm_Read32(handle, reg, dac);
  reg = V1720_CHANNEL_STATUS | (channel << 8);
  sCAEN = CAENComm_Read32(handle, reg, dac);
  return sCAEN;
}

/*****************************************************************/
CAENComm_ErrorCode ov1720_AcqCtl(int handle, uint32_t operation)
{
  uint32_t reg;
  CAENComm_ErrorCode sCAEN;
  
  sCAEN = CAENComm_Read32(handle, V1720_ACQUISITION_CONTROL, &reg);
  printf("sCAEN:%d ACQ Acq Control:0x%x\n", sCAEN, reg);

  switch (operation) {
  case V1720_RUN_START:
    sCAEN = CAENComm_Write32(handle, V1720_ACQUISITION_CONTROL, (reg | 0x4));
    break;
  case V1720_RUN_STOP:
    sCAEN = CAENComm_Write32(handle, V1720_ACQUISITION_CONTROL, (reg & ~( 0x4)));
    break;
  case V1720_REGISTER_RUN_MODE:
    sCAEN = CAENComm_Write32(handle, V1720_ACQUISITION_CONTROL, 0x0);
    break;
  case V1720_SIN_RUN_MODE:
    sCAEN = CAENComm_Write32(handle, V1720_ACQUISITION_CONTROL, 0x1);
    break;
  case V1720_SIN_GATE_RUN_MODE:
    sCAEN = CAENComm_Write32(handle, V1720_ACQUISITION_CONTROL, 0x2);
    break;
  case V1720_MULTI_BOARD_SYNC_MODE:
    sCAEN = CAENComm_Write32(handle, V1720_ACQUISITION_CONTROL, 0x3);
    break;
  case V1720_COUNT_ACCEPTED_TRIGGER:
    sCAEN = CAENComm_Write32(handle, V1720_ACQUISITION_CONTROL, (reg & ~( 0x8)));
    break;
  case V1720_COUNT_ALL_TRIGGER:
    sCAEN = CAENComm_Write32(handle, V1720_ACQUISITION_CONTROL, (reg | 0x8));
    break;
  default:
    printf("operation not defined\n");
    break;
  }
  return sCAEN;
}

/*****************************************************************/
CAENComm_ErrorCode ov1720_ChannelConfig(int handle, uint32_t operation)
{
  CAENComm_ErrorCode sCAEN;
  uint32_t reg, cfg;
  
  sCAEN = CAENComm_Write32(handle, V1720_CHANNEL_CONFIG, 0x10);
  sCAEN = CAENComm_Read32(handle, V1720_CHANNEL_CONFIG, &reg);  
  sCAEN = CAENComm_Read32(handle, V1720_CHANNEL_CONFIG, &cfg);  
  printf("Channel_config1: 0x%x\n", cfg);  

  switch (operation) {
  case V1720_TRIGGER_UNDERTH:
  sCAEN = CAENComm_Write32(handle, V1720_CHANNEL_CONFIG, (reg | 0x40));
    break;
  case V1720_TRIGGER_OVERTH:
    sCAEN = CAENComm_Write32(handle, V1720_CHANNEL_CONFIG, (reg & ~(0x40)));
    break;
  default:
    break;
  }
  sCAEN = CAENComm_Read32(handle, V1720_CHANNEL_CONFIG, &cfg);  
  printf("Channel_config2: 0x%x\n", cfg);
  return sCAEN;
}

/*****************************************************************/
CAENComm_ErrorCode ov1720_info(int handle, int *nchannels, uint32_t *data)
{
  CAENComm_ErrorCode sCAEN;
  int i, chanmask;
  uint32_t reg;

  // Evaluate the event size
  // Number of samples per channels
  sCAEN = CAENComm_Read32(handle, V1720_BUFFER_ORGANIZATION, &reg);  
  *data = V1720_NSAMPLES_MODE[reg];

  // times the number of active channels
  sCAEN = CAENComm_Read32(handle, V1720_CHANNEL_EN_MASK, &reg);  
  chanmask = 0xff & reg; 
  *nchannels = 0;
  for (i=0;i<8;i++) {
    if (chanmask & (1<<i))
      *nchannels += 1;
  }

  *data *= *nchannels;
  *data /= 2;   // 2 samples per 32bit word
  *data += 4;   // Headers
  return sCAEN;
}

/*****************************************************************/
CAENComm_ErrorCode ov1720_BufferOccupancy(int handle, uint32_t channel, uint32_t *data)
{
  uint32_t reg;

  reg = V1720_BUFFER_OCCUPANCY + (channel<<16);
  return  CAENComm_Read32(handle, reg, data);  
}

/*****************************************************************/
CAENComm_ErrorCode ov1720_BufferFree(int handle, int nbuffer, uint32_t *mode)
{
  CAENComm_ErrorCode sCAEN;

  sCAEN = CAENComm_Read32(handle, V1720_BUFFER_ORGANIZATION, mode);  
  if (nbuffer <= (1 << *mode) ) {
    sCAEN = CAENComm_Write32(handle,V1720_BUFFER_FREE, nbuffer);
    return sCAEN;
  } else
    return sCAEN;
}

/*****************************************************************/
CAENComm_ErrorCode ov1720_Status(int handle)
{
  CAENComm_ErrorCode sCAEN;
  uint32_t reg;

  printf("================================================\n");
  sCAEN = CAENComm_Read32(handle, V1720_BOARD_ID, &reg);  
  printf("Board ID             : 0x%x\n", reg);
  sCAEN = CAENComm_Read32(handle, V1720_BOARD_INFO, &reg);  
  printf("Board Info           : 0x%x\n", reg);
  sCAEN = CAENComm_Read32(handle, V1720_ACQUISITION_CONTROL, &reg);  
  printf("Acquisition control  : 0x%8.8x\n", reg);
  sCAEN = CAENComm_Read32(handle, V1720_ACQUISITION_STATUS, &reg);  
  printf("Acquisition status   : 0x%8.8x\n", reg);
  printf("================================================\n");
  return sCAEN;
}

/*****************************************************************/
/**
Sets all the necessary paramters for a given configuration.
The configuration is provided by the mode argument.
Add your own configuration in the case statement. Let me know
your setting if you want to include it in the distribution.
- <b>Mode 1</b> : 

@param *mvme VME structure
@param  base Module base address
@param mode  Configuration mode number
@return 0: OK. -1: Bad
*/
CAENComm_ErrorCode  ov1720_Setup(int handle, int mode)
{
  CAENComm_ErrorCode sCAEN;
  switch (mode) {
  case 0x0:
    printf("--------------------------------------------\n");
    printf("Setup Skip\n");
    printf("--------------------------------------------\n");
  case 0x1:
    printf("--------------------------------------------\n");
    printf("Trigger from FP, 8ch, 1Ks, postTrigger 800\n");
    printf("--------------------------------------------\n");
    sCAEN = CAENComm_Write32(handle, V1720_BUFFER_ORGANIZATION,   0x0A);    // 1K buffer
    sCAEN = CAENComm_Write32(handle, V1720_TRIG_SRCE_EN_MASK,     0x4000);  // External Trigger
    sCAEN = CAENComm_Write32(handle, V1720_CHANNEL_EN_MASK,       0xFF);    // 8ch enable
    sCAEN = CAENComm_Write32(handle, V1720_POST_TRIGGER_SETTING,  800);     // PreTrigger (1K-800)
    sCAEN = CAENComm_Write32(handle, V1720_ACQUISITION_CONTROL,   0x00);    // Reset Acq Control
    printf("\n");
    break;
  case 0x2:
    printf("--------------------------------------------\n");
    printf("Trigger from LEMO\n");
    printf("--------------------------------------------\n");
    sCAEN = CAENComm_Write32(handle, V1720_BUFFER_ORGANIZATION, 1);
    printf("\n");
    break;
  default:
    printf("Unknown setup mode\n");
    return -1;
  }
  return ov1720_Status(handle);
}

/*****************************************************************/
/*-PAA- For test purpose only */
#ifdef MAIN_ENABLE
int main (int argc, char* argv[]) {

  CAENComm_ErrorCode sCAEN;
  int handle;
  int nw;
  uint32_t i, status, Address, data[1000];

  // Open VME interface   
  sCAEN = CAENComm_OpenDevice(CAENComm_PCIE_OpticalLink, 0, 0, 0, &handle); 
  if (sCAEN != CAENComm_Success) {
    printf("CAENComm_OpenDevice: %d\n", sCAEN);
  }

  ov1720_Setup(handle, 1);
  // Run control by register
  sCAEN = CAENComm_Write32(handle, V1720_ACQUISITION_CONTROL, 0);
  // Soft or External trigger
  sCAEN = CAENComm_Write32(handle, V1720_TRIG_SRCE_EN_MASK, V1720_SOFT_TRIGGER|V1720_EXTERNAL_TRIGGER);
  // Soft and External trigger output
  sCAEN = CAENComm_Write32(handle, V1720_FP_TRIGGER_OUT_EN_MASK, V1720_SOFT_TRIGGER|V1720_EXTERNAL_TRIGGER);
  // Enabled channels
  sCAEN = CAENComm_Write32(handle, V1720_CHANNEL_EN_MASK, 0x3);
  //  sleep(1);

  // Start run then wait for trigger
  ov1720_AcqCtl(handle, V1720_RUN_START);
  sCAEN = CAENComm_Read32(handle, V1720_ACQUISITION_CONTROL, &status);
  printf("sCAEN:%d Acq Control:0x%x\n", sCAEN, status&0x1F);
  sCAEN = CAENComm_Read32(handle, V1720_BUFFER_ORGANIZATION, &status);
  printf("sCAEN:%d Buff org :0x%x\n", sCAEN, status);
  sCAEN = CAENComm_Read32(handle, V1720_ACQUISITION_STATUS, &status);
  printf("sCAEN:%d Acq Status:0x%x\n", sCAEN, status&0x1FF);

  sleep(1);
  sCAEN = CAENComm_Write32(handle,  V1720_SW_TRIGGER, 0);

  sCAEN = CAENComm_Write32(handle,  V1720_SW_TRIGGER, 0);
  sleep(1);

  sCAEN = CAENComm_Read32(handle, V1720_EVENT_STORED, &status);
  printf("sCAEN:%d Event Stored:0x%x\n", sCAEN, status);
  sCAEN = CAENComm_Read32(handle, V1720_EVENT_SIZE, &status);
  printf("sCAEN:%d Event Size:0x%x\n", sCAEN, status);

  ov1720_AcqCtl(handle, V1720_RUN_STOP);
  sCAEN = CAENComm_Read32(handle, V1720_ACQUISITION_CONTROL, &status);
  printf("sCAEN:%d Acq Control:0x%x\n", sCAEN, status&0x1F);
  sCAEN = CAENComm_Read32(handle, V1720_BUFFER_ORGANIZATION, &status);
  printf("sCAEN:%d Buff org :0x%x\n", sCAEN, status);
  sCAEN = CAENComm_Read32(handle, V1720_ACQUISITION_STATUS, &status);
  printf("sCAEN:%d Acq Status:0x%x\n", sCAEN, status&0x1FF);
  
  
#if 0
  Address = V1720_EVENT_READOUT_BUFFER;
  CAENComm_MultiRead32(handle, &Address, status/4, &(data[0]), &sCAEN);
  printf("sCAEN:%d MultiRead32 length:0x%x\n", sCAEN, status/4);
#endif

#if 0
  // SCAEN returns -13 when no more data, nw on the -13 returns the remain
  Address = V1720_EVENT_READOUT_BUFFER;
  for (i=0;i<32;i++) {
    sCAEN = CAENComm_BLTRead(handle, Address, &(data[0]), 512, &nw);
    printf("sCAEN:%d BLTRead length:0x%x\n", sCAEN, nw);
    if (nw == 0) break;
  }
#endif
  
#if 1
  Address = V1720_EVENT_READOUT_BUFFER;
  for (i=0;i<32;i++) {
    sCAEN = CAENComm_MBLTRead(handle, Address, &(data[0]), 128, &nw);
    printf("sCAEN:%d MBLTRead length:0x%x\n", sCAEN, nw);
    if (nw == 0) break;
  }
#endif
  
  for (i=0;i<nw;i++) {
    printf(" Data[%d] = 0x%8.8x\n", i, data[i]);
  }

  sCAEN = CAENComm_CloseDevice(handle); 
  return 1;

}
#endif

/* emacs
 * Local Variables:
 * mode:C
 * mode:font-lock
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */

//end
