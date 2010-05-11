/*********************************************************************

  Name:         v1740.c
  Created by:   Pierre-A. Amaudruz / K.Olchanski

  Contents:     V1740 64 ch. 12bit 65Msps
 
  $Id$
*********************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "v1740drv.h"
#include "mvmestd.h"

// Buffer organization map for number of samples
uint32_t NSAMPLES_MODE[11] = { (1024*192), (512*192), (256*192), (128*192), (64*192), (32*192)
			       ,(16*192), (8*192), (4*192), (2*192), (192)};

/*****************************************************************/
/*
Read V1740 register value
*/
static uint32_t regRead(MVME_INTERFACE *mvme, uint32_t base, int offset)
{
  mvme_set_am(mvme, MVME_AM_A32);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  return mvme_read_value(mvme, base + offset);
}

/*****************************************************************/
/*
Write V1740 register value
*/
static void regWrite(MVME_INTERFACE *mvme, uint32_t base, int offset, uint32_t value)
{
  mvme_set_am(mvme, MVME_AM_A32);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  mvme_write_value(mvme, base + offset, value);
}

/*****************************************************************/
uint32_t v1740_RegisterRead(MVME_INTERFACE *mvme, uint32_t base, int offset)
{
  return regRead(mvme, base, offset);
}

/*****************************************************************/
void v1740_RegisterWrite(MVME_INTERFACE *mvme, uint32_t base, int offset, uint32_t value)
{
  regWrite(mvme, base, offset, value);
}

/*****************************************************************/
void v1740_Reset(MVME_INTERFACE *mvme, uint32_t base)
{
  regWrite(mvme, base, V1740_SW_RESET, 0);
}

/*****************************************************************/
void v1740_TrgCtl(MVME_INTERFACE *mvme, uint32_t base, uint32_t reg, uint32_t mask)
{
  regWrite(mvme, base, reg, mask);
}

/*****************************************************************/
void v1740_GroupCtl(MVME_INTERFACE *mvme, uint32_t base, uint32_t reg, uint32_t mask)
{
  regWrite(mvme, base, reg, mask);
}

/*****************************************************************/
void v1740_GroupThreshold(MVME_INTERFACE *mvme, uint32_t base, uint16_t group, uint16_t threshold)
{
  uint32_t reg;
  
  reg = V1740_GROUP_THRESHOLD | (group << 8);
  regWrite(mvme, base, reg, (threshold & 0xFFF));
}

/*****************************************************************/
void v1740_GroupDAC(MVME_INTERFACE *mvme, uint32_t base, uint16_t group, uint16_t dac)
{
  uint32_t reg;
  
  reg = V1740_GROUP_DAC | (group << 8);
  regWrite(mvme, base, reg, (dac & 0xFFFF));
}

/*****************************************************************/
void v1740_AcqCtl(MVME_INTERFACE *mvme, uint32_t base, uint32_t operation)
{
  uint32_t reg;
  
  reg = regRead(mvme, base, V1740_ACQUISITION_CONTROL);  
  switch (operation) {
  case RUN_START:
    regWrite(mvme, base, V1740_ACQUISITION_CONTROL, (reg | 0x4));
    break;
  case RUN_STOP:
    regWrite(mvme, base, V1740_ACQUISITION_CONTROL, (reg & ~(0x4)));
    break;
  case REGISTER_RUN_MODE:
    regWrite(mvme, base, V1740_ACQUISITION_CONTROL, (reg & ~(0x3)));
    break;
  case SIN_RUN_MODE:
    regWrite(mvme, base, V1740_ACQUISITION_CONTROL, (reg | 0x01));
    break;
  case SIN_GATE_RUN_MODE:
    regWrite(mvme, base, V1740_ACQUISITION_CONTROL, (reg | 0x02));
    break;
  case MULTI_BOARD_SYNC_MODE:
    regWrite(mvme, base, V1740_ACQUISITION_CONTROL, (reg | 0x03));
    break;
  case COUNT_ACCEPTED_TRIGGER:
    regWrite(mvme, base, V1740_ACQUISITION_CONTROL, (reg | 0x08));
    break;
  case COUNT_ALL_TRIGGER:
    regWrite(mvme, base, V1740_ACQUISITION_CONTROL, (reg & ~(0x08)));
    break;
  case DOWNSAMPLE_ENABLE:
    regWrite(mvme, base, V1740_ACQUISITION_CONTROL, (reg & 0x10));
    break;
  case DOWNSAMPLE_DISABLE:
    regWrite(mvme, base, V1740_ACQUISITION_CONTROL, (reg & ~(0x10)));
    break;
  default:
    break;
  }
}

/*****************************************************************/
void v1740_info(MVME_INTERFACE *mvme, uint32_t base, int *ngroups, uint32_t *n32word)
{
  int i, grpmask;

  // Evaluate the event size
  // Number of samples per group (8 channels)
  *n32word = 8 * NSAMPLES_MODE[regRead(mvme, base, V1740_BUFFER_ORGANIZATION)];

  // times the number of active group
  grpmask = 0xff & regRead(mvme, base, V1740_GROUP_EN_MASK); 
  *ngroups = 0;
  for (i=0;i<8;i++) {
    if (grpmask & (1<<i))
      *ngroups += 1;
  }
  
  *n32word = *n32word * *ngroups * 12 / 32;  // 12bit/Sample , 32bits/DW
  *n32word += 4;    // Headers
}

/*****************************************************************/
uint32_t v1740_BufferOccupancy(MVME_INTERFACE *mvme, uint32_t base, uint32_t group)
{
  uint32_t reg;
  reg = V1740_GROUP_BUFFER_OCCUPANCY + (group<<16);
  return regRead(mvme, base, reg);
}


/*****************************************************************/
uint32_t v1740_BufferFree(MVME_INTERFACE *mvme, uint32_t base, int nbuffer)
{
  int mode;

  mode = regRead(mvme, base, V1740_BUFFER_ORGANIZATION);
  if (nbuffer <= (1<< mode) ) {
    regWrite(mvme, base, V1740_BUFFER_FREE, nbuffer);
    return mode;
  } else
    return mode;
}

/*****************************************************************/
uint32_t v1740_BufferFreeRead(MVME_INTERFACE *mvme, uint32_t base)
{
  return regRead(mvme, base, V1740_BUFFER_FREE);
}

/*****************************************************************/
uint32_t v1740_DataRead(MVME_INTERFACE *mvme, uint32_t base, uint32_t *pdata, uint32_t n32w)
{
  uint32_t i;

  mvme_set_am(  mvme, MVME_AM_A32);
  mvme_set_dmode(  mvme, MVME_DMODE_D32);
  n32w = 0xFFFFFF & regRead(mvme, base, V1740_EVENT_READOUT_BUFFER);
  //  printf("DataRead n32w:%d 0x%x\n", n32w, n32w);
  *pdata++ = n32w;
  for (i=0;i<n32w-1;i++) {
    *pdata = regRead(mvme, base, V1740_EVENT_READOUT_BUFFER);
    if (*pdata != 0xffffffff)
      pdata++;
    else
      break;
  }
  return i+1;
}

/********************************************************************/
/** v1740_DataBlockRead
Read N entries (32bit) 
@param mvme vme structure
@param base  base address
@param pdest Destination pointer
@return nentry
*/
uint32_t v1740_DataBlockRead(MVME_INTERFACE *mvme, uint32_t base, uint32_t *pdest, uint32_t *nentry)
{
  int status, ngroups;

  mvme_set_am(  mvme, MVME_AM_A32);
  mvme_set_dmode(  mvme, MVME_DMODE_D32);
  v1740_info(mvme, base, &ngroups, nentry);
  // *nentry = 0xFFFFFF & regRead(mvme, base, V1740_EVENT_READOUT_BUFFER);
  printf("DataBlockRead n32w:%d 0x%x\n", *nentry, *nentry);
  mvme_set_blt(  mvme, MVME_BLT_MBLT64);
  mvme_set_blt(  mvme, 0);
  //
  // Transfer in MBLT64 (8bytes), nentry is in 32bits
  status = mvme_read(mvme, pdest, base+V1740_EVENT_READOUT_BUFFER, *nentry<<2);
  if (status != MVME_SUCCESS)
    return 0;

  return (*nentry);
}


/*****************************************************************/
void  v1740_Status(MVME_INTERFACE *mvme, uint32_t base)
{
  printf("================================================\n");
  printf("V1740 at A32 0x%x\n", (int)base);
  printf("Board ID             : 0x%x\n", regRead(mvme, base, V1740_BOARD_ID));
  printf("Board Info           : 0x%x\n", regRead(mvme, base, V1740_BOARD_INFO));
  printf("Acquisition status   : 0x%8.8x\n", regRead(mvme, base, V1740_ACQUISITION_STATUS));
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
int  v1740_Setup(MVME_INTERFACE *mvme, uint32_t base, int mode)
{
  switch (mode) {
  case 0x0:
    printf("--------------------------------------------\n");
    printf("Setup Skip\n");
    printf("--------------------------------------------\n");
  case 0x1:
    printf("--------------------------------------------\n");
    printf("Trigger from FP, 8 group, 1Ks, postTrigger 800\n");
    printf("--------------------------------------------\n");
    regWrite(mvme, base, V1740_ACQUISITION_CONTROL,   0x00);   // Reset Acq Control
    regWrite(mvme, base, V1740_BUFFER_ORGANIZATION,  0x0A);    // 1K Nbuffers=1024, bufferSize=192
    regWrite(mvme, base, V1740_TRIG_SRCE_EN_MASK,    0x4000);  // External Trigger
    regWrite(mvme, base, V1740_GROUP_EN_MASK,      0xFF);    // 8grp enable
    regWrite(mvme, base, V1740_POST_TRIGGER_SETTING, 800);     // PreTrigger (1K-800)
    printf("\n");
    break;
  case 0x2:
    printf("--------------------------------------------\n");
    printf("Trigger from LEMO\n");
    printf("--------------------------------------------\n");
    regWrite(mvme, base, V1740_BUFFER_ORGANIZATION, 1);
    printf("\n");
    break;
  default:
    printf("Unknown setup mode\n");
    return -1;
  }
  v1740_Status(mvme, base);
  return 0;
}

/*****************************************************************/
/*-PAA- For test purpose only */
#ifdef MAIN_ENABLE
int main (int argc, char* argv[]) {

  uint32_t V1740_BASE  = 0x32100000;

  MVME_INTERFACE *myvme;

  uint32_t data[100000], n32word, n32read;
  int i, status, group, ngroups=0;

  if (argc>1) {
    sscanf(argv[1],"%x", &V1740_BASE);
  }

  // Test under vmic
  status = mvme_open(&myvme, 0);

  v1740_Setup(myvme, V1740_BASE, 1);
  // Run control by register
  v1740_AcqCtl(myvme, V1740_BASE, REGISTER_RUN_MODE);
  // Soft or External trigger
  v1740_TrgCtl(myvme, V1740_BASE, V1740_TRIG_SRCE_EN_MASK, V1740_SOFT_TRIGGER|V1740_EXTERNAL_TRIGGER);
  // Soft and External trigger output
  v1740_TrgCtl(myvme, V1740_BASE, V1740_FP_TRIGGER_OUT_EN_MASK, V1740_SOFT_TRIGGER|V1740_EXTERNAL_TRIGGER);
  // Enabled groups
  v1740_GroupCtl(myvme, V1740_BASE, V1740_GROUP_EN_MASK, 0x1);
  //  sleep(1);

  group = 0;
  // Evaluate the event size
  v1740_info(myvme, V1740_BASE, &ngroups, &n32word);
  printf("ngroups: %d  n32word: %d\n", ngroups, n32word);
  printf("Occupancy for group %d = %d\n", group, v1740_BufferOccupancy(myvme, V1740_BASE, group)); 

  // Start run then wait for trigger
  v1740_AcqCtl(myvme, V1740_BASE, RUN_START);
  sleep(1);

  regWrite(myvme, V1740_BASE, V1740_SOFTWARE_TRIGGER, 1);
  
#if 0
  do {
    status = regRead(myvme, V1740_BASE, V1740_ACQUISITION_STATUS);
    printf("Acq Status:0x%x\n", status);
  } while ((status & 0x8)==0);
  
  n32read = v1740_DataRead(myvme, V1740_BASE, &(data[0]), n32word);
  printf("n32read:%d\n", n32read);
}
#endif

#if 1
  n32read = v1740_DataBlockRead(myvme, V1740_BASE, &(data[0]), &n32word);
#endif  

  for (i=0; i<n32read;i+=4) {
    printf("[%3d]:0x%8.8x - [%3d]:0x%8.8x - [%3d]:0x%8.8x - [%3d]:0x%8.8x\n"
	 , i, data[i], i+1, data[i+1], i+2, data[i+2], i+3, data[i+3]);
  }
  
  v1740_AcqCtl(myvme, V1740_BASE, RUN_STOP);
  
  regRead(myvme, V1740_BASE, V1740_ACQUISITION_CONTROL);
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
