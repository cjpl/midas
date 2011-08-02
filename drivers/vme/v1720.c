/*********************************************************************

  Name:         v1720.c
  Created by:   Pierre-A. Amaudruz / K.Olchanski

  Contents:     V1720 8 ch. 12bit 250Msps
 
  $Id$
*********************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "v1720drv.h"
#include "mvmestd.h"

// Buffer organization map for number of samples
uint32_t V1720_NSAMPLES_MODE[11] = { (1<<20), (1<<19), (1<<18), (1<<17), (1<<16), (1<<15)
			       ,(1<<14), (1<<13), (1<<12), (1<<11), (1<<10)};

/*****************************************************************/
/*
Read V1720 register value
*/
static uint32_t regRead(MVME_INTERFACE *mvme, uint32_t base, int offset)
{
  mvme_set_am(mvme, MVME_AM_A32);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  return mvme_read_value(mvme, base + offset);
}

/*****************************************************************/
/*
Write V1720 register value
*/
static void regWrite(MVME_INTERFACE *mvme, uint32_t base, int offset, uint32_t value)
{
  mvme_set_am(mvme, MVME_AM_A32);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  mvme_write_value(mvme, base + offset, value);
}

/*****************************************************************/
uint32_t v1720_RegisterRead(MVME_INTERFACE *mvme, uint32_t base, int offset)
{
  return regRead(mvme, base, offset);
}

/*****************************************************************/
void v1720_RegisterWrite(MVME_INTERFACE *mvme, uint32_t base, int offset, uint32_t value)
{
  regWrite(mvme, base, offset, value);
}

/*****************************************************************/
void v1720_Reset(MVME_INTERFACE *mvme, uint32_t base)
{
  regWrite(mvme, base, V1720_SW_RESET, 0);
}

/*****************************************************************/
void v1720_TrgCtl(MVME_INTERFACE *mvme, uint32_t base, uint32_t reg, uint32_t mask)
{
  regWrite(mvme, base, reg, mask);
}

/*****************************************************************/
void v1720_ChannelCtl(MVME_INTERFACE *mvme, uint32_t base, uint32_t reg, uint32_t mask)
{
  regWrite(mvme, base, reg, mask);
}

/*****************************************************************/
void v1720_ChannelSet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t what, uint32_t that)
{
  uint32_t reg, mask;

  if (what == V1720_CHANNEL_THRESHOLD)   mask = 0x0FFF;
  if (what == V1720_CHANNEL_OUTHRESHOLD) mask = 0x0FFF;
  if (what == V1720_CHANNEL_DAC)         mask = 0xFFFF;
  reg = what | (channel << 8);
  printf("base:0x%x reg:0x%x, this:%x\n", base, reg, that);
  regWrite(mvme, base, reg, (that & 0xFFF));
}

/*****************************************************************/
uint32_t v1720_ChannelGet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t what)
{
  uint32_t reg, mask;

  if (what == V1720_CHANNEL_THRESHOLD)   mask = 0x0FFF;
  if (what == V1720_CHANNEL_OUTHRESHOLD) mask = 0x0FFF;
  if (what == V1720_CHANNEL_DAC)         mask = 0xFFFF;
  reg = what | (channel << 8);
  return regRead(mvme, base, reg);
}

/*****************************************************************/
void v1720_ChannelThresholdSet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t threshold)
{
  uint32_t reg;
  reg = V1720_CHANNEL_THRESHOLD | (channel << 8);
  printf("base:0x%x reg:0x%x, threshold:%x\n", base, reg, threshold);
  regWrite(mvme, base, reg, (threshold & 0xFFF));
}

/*****************************************************************/
void v1720_ChannelOUThresholdSet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t threshold)
{
  uint32_t reg;
  reg = V1720_CHANNEL_OUTHRESHOLD | (channel << 8);
  printf("base:0x%x reg:0x%x, outhreshold:%x\n", base, reg, threshold);
  regWrite(mvme, base, reg, (threshold & 0xFFF));
}

/*****************************************************************/
void v1720_ChannelDACSet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t dac)
{
  uint32_t reg;

  reg = V1720_CHANNEL_DAC | (channel << 8);
  printf("base:0x%x reg:0x%x, DAC:%x\n", base, reg, dac);
  regWrite(mvme, base, reg, (dac & 0xFFFF));
}

/*****************************************************************/
int v1720_ChannelDACGet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t *dac)
{
  uint32_t reg;
  int   status;

  reg = V1720_CHANNEL_DAC | (channel << 8);
  *dac = regRead(mvme, base, reg);
  reg = V1720_CHANNEL_STATUS | (channel << 8);
  status = regRead(mvme, base, reg);
  return status;
}

/*****************************************************************/
void v1720_Align64Set(MVME_INTERFACE *mvme, uint32_t base)
{
  regWrite(mvme, base, V1720_VME_CONTROL, V1720_ALIGN64);
}

/*****************************************************************/
void v1720_AcqCtl(MVME_INTERFACE *mvme, uint32_t base, uint32_t operation)
{
  uint32_t reg;
  
  reg = regRead(mvme, base, V1720_ACQUISITION_CONTROL);  
  switch (operation) {
  case V1720_RUN_START:
    regWrite(mvme, base, V1720_ACQUISITION_CONTROL, (reg | 0x4));
    break;
  case V1720_RUN_STOP:
    regWrite(mvme, base, V1720_ACQUISITION_CONTROL, (reg & ~(0x4)));
    break;
  case V1720_REGISTER_RUN_MODE:
    regWrite(mvme, base, V1720_ACQUISITION_CONTROL, (reg & ~(0x3)));
    break;
  case V1720_SIN_RUN_MODE:
    regWrite(mvme, base, V1720_ACQUISITION_CONTROL, (reg | 0x01));
    break;
  case V1720_SIN_GATE_RUN_MODE:
    regWrite(mvme, base, V1720_ACQUISITION_CONTROL, (reg | 0x02));
    break;
  case V1720_MULTI_BOARD_SYNC_MODE:
    regWrite(mvme, base, V1720_ACQUISITION_CONTROL, (reg | 0x03));
    break;
  case V1720_COUNT_ACCEPTED_TRIGGER:
    regWrite(mvme, base, V1720_ACQUISITION_CONTROL, (reg | 0x08));
    break;
  case V1720_COUNT_ALL_TRIGGER:
    regWrite(mvme, base, V1720_ACQUISITION_CONTROL, (reg & ~(0x08)));
    break;
  default:
    break;
  }
}

/*****************************************************************/
void v1720_ChannelConfig(MVME_INTERFACE *mvme, uint32_t base, uint32_t operation)
{
  uint32_t reg;
  
  regWrite(mvme, base, V1720_CHANNEL_CONFIG, 0x10);
  reg = regRead(mvme, base, V1720_CHANNEL_CONFIG);  
  printf("Channel_config1: 0x%x\n", regRead(mvme, base, V1720_CHANNEL_CONFIG));  
  switch (operation) {
  case V1720_TRIGGER_UNDERTH:
    regWrite(mvme, base, V1720_CHANNEL_CONFIG, (reg | 0x40));
    break;
  case V1720_TRIGGER_OVERTH:
    regWrite(mvme, base, V1720_CHANNEL_CONFIG, (reg & ~(0x40)));
    break;
  default:
    break;
  }
  printf("Channel_config2: 0x%x\n", regRead(mvme, base, V1720_CHANNEL_CONFIG));  
}

/*****************************************************************/
void v1720_info(MVME_INTERFACE *mvme, uint32_t base, int *nchannels, uint32_t *n32word)
{
  int i, chanmask;

  // Evaluate the event size
  // Number of samples per channels
  *n32word = V1720_NSAMPLES_MODE[regRead(mvme, base, V1720_BUFFER_ORGANIZATION)];

  // times the number of active channels
  chanmask = 0xff & regRead(mvme, base, V1720_CHANNEL_EN_MASK); 
  *nchannels = 0;
  for (i=0;i<8;i++) {
    if (chanmask & (1<<i))
      *nchannels += 1;
  }

  *n32word *= *nchannels;
  *n32word /= 2;   // 2 samples per 32bit word
  *n32word += 4;   // Headers
}

/*****************************************************************/
uint32_t v1720_BufferOccupancy(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel)
{
  uint32_t reg;
  reg = V1720_BUFFER_OCCUPANCY + (channel<<16);
  return regRead(mvme, base, reg);
}


/*****************************************************************/
uint32_t v1720_BufferFree(MVME_INTERFACE *mvme, uint32_t base, int nbuffer)
{
  int mode;

  mode = regRead(mvme, base, V1720_BUFFER_ORGANIZATION);
  if (nbuffer <= (1<< mode) ) {
    regWrite(mvme, base, V1720_BUFFER_FREE, nbuffer);
    return mode;
  } else
    return mode;
}

/*****************************************************************/
uint32_t v1720_BufferFreeRead(MVME_INTERFACE *mvme, uint32_t base)
{
  return regRead(mvme, base, V1720_BUFFER_FREE);
}

/*****************************************************************/
uint32_t v1720_DataRead(MVME_INTERFACE *mvme, uint32_t base, uint32_t *pdata, uint32_t n32w)
{
  uint32_t i;

  for (i=0;i<n32w;i++) {
    *pdata = regRead(mvme, base, V1720_EVENT_READOUT_BUFFER);
    //    printf ("pdata[%i]:%x\n", i, *pdata); 
//    if (*pdata != 0xffffffff)
    pdata++;
    //    else
      //      break;
  }
  return i;
}

/********************************************************************/
/** v1720_DataBlockRead
Read N entries (32bit) 
@param mvme vme structure
@param base  base address
@param pdest Destination pointer
@return nentry
*/
uint32_t v1720_DataBlockRead(MVME_INTERFACE *mvme, uint32_t base, uint32_t *pdest, uint32_t *nentry)
{
  int status;

  mvme_set_am(  mvme, MVME_AM_A32);
  mvme_set_dmode(  mvme, MVME_DMODE_D32);
  mvme_set_blt(  mvme, MVME_BLT_MBLT64);
  //  mvme_set_blt(  mvme, MVME_BLT_BLT32);
  //mvme_set_blt(  mvme, 0);

  // Transfer in MBLT64 (8bytes), nentry is in 32bits(VF48)
  // *nentry * 8 / 2
  status = mvme_read(mvme, pdest, base+V1720_EVENT_READOUT_BUFFER, *nentry<<2);
  if (status != MVME_SUCCESS)
    return 0;

  return (*nentry);
}


/*****************************************************************/
void  v1720_Status(MVME_INTERFACE *mvme, uint32_t base)
{
  printf("================================================\n");
  printf("V1720 at A32 0x%x\n", (int)base);
  printf("Board ID             : 0x%x\n", regRead(mvme, base, V1720_BOARD_ID));
  printf("Board Info           : 0x%x\n", regRead(mvme, base, V1720_BOARD_INFO));
  printf("Acquisition status   : 0x%8.8x\n", regRead(mvme, base, V1720_ACQUISITION_STATUS));
  printf("================================================\n");
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
int  v1720_Setup(MVME_INTERFACE *mvme, uint32_t base, int mode)
{
  switch (mode) {
  case 0x0:
    printf("--------------------------------------------\n");
    printf("Setup Skip\n");
    printf("--------------------------------------------\n");
  case 0x1:
    printf("--------------------------------------------\n");
    printf("Trigger from FP, 8ch, 1Ks, postTrigger 800\n");
    printf("--------------------------------------------\n");
    regWrite(mvme, base, V1720_BUFFER_ORGANIZATION,  0x0A);    // 1K buffer
    regWrite(mvme, base, V1720_TRIG_SRCE_EN_MASK,    0x4000);  // External Trigger
    regWrite(mvme, base, V1720_CHANNEL_EN_MASK,      0xFF);    // 8ch enable
    regWrite(mvme, base, V1720_POST_TRIGGER_SETTING, 800);     // PreTrigger (1K-800)
    regWrite(mvme, base, V1720_ACQUISITION_CONTROL,   0x00);   // Reset Acq Control
    printf("\n");
    break;
  case 0x2:
    printf("--------------------------------------------\n");
    printf("Trigger from LEMO\n");
    printf("--------------------------------------------\n");
    regWrite(mvme, base, V1720_BUFFER_ORGANIZATION, 1);
    printf("\n");
    break;
  default:
    printf("Unknown setup mode\n");
    return -1;
  }
  v1720_Status(mvme, base);
  return 0;
}

/*****************************************************************/
/*-PAA- For test purpose only */
#ifdef MAIN_ENABLE
int main (int argc, char* argv[]) {

  uint32_t V1720_BASE  = 0x32100000;

  MVME_INTERFACE *myvme;

  uint32_t data[100000], n32word, n32read;
  int status, channel, i, j, nchannels=0, chanmask;

  if (argc>1) {
    sscanf(argv[1],"%x", &V1720_BASE);
  }

  // Test under vmic
  status = mvme_open(&myvme, 0);

  v1720_Setup(myvme, V1720_BASE, 1);
  // Run control by register
  v1720_AcqCtl(myvme, V1720_BASE, V1720_REGISTER_RUN_MODE);
  // Soft or External trigger
  v1720_TrgCtl(myvme, V1720_BASE, V1720_TRIG_SRCE_EN_MASK     , V1720_SOFT_TRIGGER|V1720_EXTERNAL_TRIGGER);
  // Soft and External trigger output
  v1720_TrgCtl(myvme, V1720_BASE, V1720_FP_TRIGGER_OUT_EN_MASK, V1720_SOFT_TRIGGER|V1720_EXTERNAL_TRIGGER);
  // Enabled channels
  v1720_ChannelCtl(myvme, V1720_BASE, V1720_CHANNEL_EN_MASK, 0x3);
  //  sleep(1);

  channel = 0;
  // Start run then wait for trigger
  v1720_AcqCtl(myvme, V1720_BASE, V1720_RUN_START);
  sleep(1);

  //  regWrite(myvme, V1720_BASE, V1720_SW_TRIGGER, 1);

  // Evaluate the event size
  // Number of samples per channels
  n32word  =  1<<regRead(myvme, V1720_BASE, V1720_BUFFER_ORGANIZATION);
  // times the number of active channels
  chanmask = 0xff & regRead(myvme, V1720_BASE, V1720_CHANNEL_EN_MASK); 
  for (i=0;i<8;i++) 
    if (chanmask & (1<<i))
      nchannels++;
  printf("chanmask : %x , nchannels:  %d\n", chanmask, nchannels);
  n32word *= nchannels;
  n32word /= 2;   // 2 samples per 32bit word
  n32word += 4;   // Headers
  printf("n32word:%d\n", n32word);

  printf("Occupancy for channel %d = %d\n", channel, v1720_BufferOccupancy(myvme, V1720_BASE, channel)); 

  do {
    status = regRead(myvme, V1720_BASE, V1720_ACQUISITION_STATUS);
    printf("Acq Status1:0x%x\n", status);
  } while ((status & 0x8)==0);
  
  n32read = v1720_DataRead(myvme, V1720_BASE, &(data[0]), n32word);
  printf("n32read:%d\n", n32read);
  
  for (i=0; i<n32read;i+=4) {
    printf("[%d]:0x%x - [%d]:0x%x - [%d]:0x%x - [%d]:0x%x\n"
	   , i, data[i], i+1, data[i+1], i+2, data[i+2], i+3, data[i+3]);
  }
  
  do {
    status = regRead(myvme, V1720_BASE, V1720_ACQUISITION_STATUS);
    printf("Acq Status2:0x%x\n", status);
  } while ((status & 0x8)==0);
  
  n32read = v1720_dataRead(myvme, V1720_BASE, &(data[0]), n32word);
  printf("n32read:%d\n", n32read);
  
  for (i=0; i<n32read;i+=4) {
    printf("[%d]:0x%x - [%d]:0x%x - [%d]:0x%x - [%d]:0x%x\n"
	   , i, data[i], i+1, data[i+1], i+2, data[i+2], i+3, data[i+3]);
  }
  
  
  //  v1720_AcqCtl(myvme, V1720_BASE, RUN_STOP);
  
  regRead(myvme, V1720_BASE, V1720_ACQUISITION_CONTROL);
  status = mvme_close(myvme);

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
