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
uint32_t V1740_NSAMPLES_MODE[11] = { (1024*192), (512*192), (256*192), (128*192), (64*192), (32*192)
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
void v1740_GroupSet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t what, uint32_t that)
{
  uint32_t reg, mask;

  if (what == V1740_GROUP_THRESHOLD)   mask = 0x0FFF;
  if (what == V1740_GROUP_DAC)         mask = 0xFFFF;
  reg = what | (channel << 8);
  //  printf("base:0x%x reg:0x%x, this:%x\n", base, reg, that);
  regWrite(mvme, base, reg, (that & mask));
  that = regRead(mvme, base, (V1740_GROUP_STATUS | (channel << 8)));
  //  printf("Group %d  status:0x%x\n", channel, that);
}

/*****************************************************************/
uint32_t v1740_GroupGet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t what)
{
  uint32_t reg, mask;

  if (what == V1740_GROUP_THRESHOLD)   mask = 0x0FFF;
  if (what == V1740_GROUP_DAC)         mask = 0xFFFF;
  reg = what | (channel << 8);
  return regRead(mvme, base, reg);
}

/*****************************************************************/
void v1740_AcqCtl(MVME_INTERFACE *mvme, uint32_t base, uint32_t operation)
{
  uint32_t reg;
  
  reg = regRead(mvme, base, V1740_ACQUISITION_CONTROL);  
  switch (operation) {
  case V1740_RUN_START:
    regWrite(mvme, base, V1740_ACQUISITION_CONTROL, (reg | 0x4));
    break;
  case V1740_RUN_STOP:
    regWrite(mvme, base, V1740_ACQUISITION_CONTROL, (reg & ~(0x4)));
    break;
  case V1740_REGISTER_RUN_MODE:
    regWrite(mvme, base, V1740_ACQUISITION_CONTROL, (reg & ~(0x3)));
    break;
  case V1740_SIN_RUN_MODE:
    regWrite(mvme, base, V1740_ACQUISITION_CONTROL, (reg | 0x01));
    break;
  case V1740_SIN_GATE_RUN_MODE:
    regWrite(mvme, base, V1740_ACQUISITION_CONTROL, (reg | 0x02));
    break;
  case V1740_MULTI_BOARD_SYNC_MODE:
    regWrite(mvme, base, V1740_ACQUISITION_CONTROL, (reg | 0x03));
    break;
  case V1740_COUNT_ACCEPTED_TRIGGER:
    regWrite(mvme, base, V1740_ACQUISITION_CONTROL, (reg | 0x08));
    break;
  case V1740_COUNT_ALL_TRIGGER:
    regWrite(mvme, base, V1740_ACQUISITION_CONTROL, (reg & ~(0x08)));
    break;
  case V1740_DOWNSAMPLE_ENABLE:
    regWrite(mvme, base, V1740_ACQUISITION_CONTROL, (reg & 0x10));
    break;
  case V1740_DOWNSAMPLE_DISABLE:
    regWrite(mvme, base, V1740_ACQUISITION_CONTROL, (reg & ~(0x10)));
    break;
  default:
    break;
  }
}

/*****************************************************************/
void v1740_GroupConfig(MVME_INTERFACE *mvme, uint32_t base, uint32_t operation)
{
  uint32_t reg;
  
  regWrite(mvme, base, V1740_GROUP_CONFIG, 0x10);  // must be set to 1
  reg = regRead(mvme, base, V1740_GROUP_CONFIG);  
  printf("Channel_config1: 0x%x\n", regRead(mvme, base, V1740_GROUP_CONFIG));  
  switch (operation) {
  case V1740_TRIGGER_UNDERTH:
    regWrite(mvme, base, V1740_GROUP_CONFIG, (reg | 0x40));
    break;
  case V1740_TRIGGER_OVERTH:
    regWrite(mvme, base, V1740_GROUP_CONFIG, (reg & ~(0x40)));
    break;
  default:
    break;
  }
  printf("Channel_config2: 0x%x\n", regRead(mvme, base, V1740_GROUP_CONFIG));  
}

/*****************************************************************/
void v1740_info(MVME_INTERFACE *mvme, uint32_t base, int *ngroups, uint32_t *n32word)
{
  int i, grpmask;

  // Evaluate the event size
  // Number of samples per group (8 channels)
  *n32word = 8 * V1740_NSAMPLES_MODE[regRead(mvme, base, V1740_BUFFER_ORGANIZATION)];

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
  for (i=0;i<n32w;i++) {
    *pdata = regRead(mvme, base, V1740_EVENT_READOUT_BUFFER);
    //    printf("DataRead base:0x%x add:0x%x idx:%d 0x%x\n", base, V1740_EVENT_READOUT_BUFFER, i, *pdata);
    if (*pdata != 0xffffffff)
      pdata++;
    else
      break;
  }
  return i;
}

/*-PAA- Contains the KO modifications for reading the V1740/42 
        Presently the block mode seem to fail on the last bytes, work around
        is to read the last bytes through normal PIO. Caen has been
        contacted about this problem.
 */
void v1740_DataBlockRead(MVME_INTERFACE* mvme, uint32_t base     , uint32_t* pbuf32, int nwords32)
{
  int status, i, to_read32;
  uint32_t w;

  if (nwords32 < 100) { // PIO
    for (i=0; i<nwords32; i++) {
      w = regRead(mvme, base, 0);
      //printf("word %d: 0x%08x\n", i, w);                                                    
      *pbuf32++ = w;
    }
  } else {
    mvme_set_dmode(mvme, MVME_DMODE_D32);
    //    mvme_set_blt(mvme, MVME_BLT_BLT32);                                                     
    //mvme_set_blt(mvme, MVME_BLT_MBLT64);                                                    
    //mvme_set_blt(mvme, MVME_BLT_2EVME);                                                     
    mvme_set_blt(mvme, MVME_BLT_2ESST);

    while (nwords32>0) {
      to_read32 = nwords32;
      to_read32 &= ~0x3;
      if (to_read32*4 >= 0xFF0)
        to_read32 = 0xFF0/4;
      else
        to_read32 = nwords32 - 8;
      if (to_read32 <= 0)
        break;
      //printf("going to read: read %d, total %d\n", to_read32*4, nwords32*4);                
      status = mvme_read(mvme, pbuf32, base, to_read32*4);
      //printf("read %d, status %d, total %d\n", to_read32*4, status, nwords32*4);            
      nwords32 -= to_read32;
      pbuf32 += to_read32;
    }

    while (nwords32) {
      *pbuf32 = regRead(mvme, base, 0);
      pbuf32++;
      nwords32--;
    }

    //v1742_status(mvme, base);                                                               
    //abort();                                                                                
  }
}

#if 0
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
  //  printf("DataBlockRead n32w:%d 0x%x\n", *nentry, *nentry);
  mvme_set_blt(  mvme, MVME_BLT_MBLT64);
  //
  // Transfer in MBLT64 (8bytes), nentry is in 32bits
  status = mvme_read(mvme, pdest, base+V1740_EVENT_READOUT_BUFFER, *nentry<<2);
  mvme_set_blt(  mvme, 0);
  if (status != MVME_SUCCESS)
    return 0;

  return (*nentry);
}

#endif

/*****************************************************************/
void  v1740_Status(MVME_INTERFACE *mvme, uint32_t base)
{
  printf("================================================\n");
  printf("V1740 at A32 0x%x\n", (int)base);
  printf("Board ID           (0xEF08)  : 0x%x\n", regRead(mvme, base, V1740_BOARD_ID));
  printf("Board Info         (0x8140)  : 0x%x\n", regRead(mvme, base, V1740_BOARD_INFO));
  printf("ROC FPGA FW Rev    (0x8124)  : 0x%x\n", regRead(mvme, base, V1740_ROC_FPGA_FW_REV));
  printf("Group Config       (0x8000)  : 0x%8.8x\n", regRead(mvme, base, V1740_GROUP_CONFIG));
  printf("Group0 Threshold   (0x1080)  : 0x%8.8x\n", regRead(mvme, base, V1740_GROUP_THRESHOLD));
  printf("Group0 DAC         (0x1098)  : 0x%8.8x\n", regRead(mvme, base, V1740_GROUP_DAC));
  printf("Group0 Ch Trg Mask (0x10A8)  : 0x%8.8x\n", regRead(mvme, base, V1740_CH_TRG_MASK));
  printf("Acquisition control(0x8100)  : 0x%8.8x\n", regRead(mvme, base, V1740_ACQUISITION_CONTROL));
  printf("Acquisition status (0x800C)  : 0x%8.8x\n", regRead(mvme, base, V1740_ACQUISITION_STATUS));
  printf("Trg Src En Mask    (0x810C)  : 0x%8.8x\n", regRead(mvme, base, V1740_TRIG_SRCE_EN_MASK));
  printf("FP Trg Out En Mask (0x8110)  : 0x%8.8x\n", regRead(mvme, base, V1740_FP_TRIGGER_OUT_EN_MASK));
  printf("FP IO Control      (0x811C)  : 0x%8.8x\n", regRead(mvme, base, V1740_FP_IO_CONTROL));
  printf("Group Enable Mask  (0x8120)  : 0x%8.8x\n", regRead(mvme, base, V1740_GROUP_EN_MASK));
  printf("VME Control        (0xEF00)  : 0x%8.8x\n", regRead(mvme, base, V1740_VME_CONTROL));
  printf("VME Status         (0xEF04)  : 0x%8.8x\n", regRead(mvme, base, V1740_VME_STATUS));
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

  uint32_t data[100000], n32word, n32read, nevt;
  int status, group, i, ngroups, chanmask;

  if (argc>1) {
    sscanf(argv[1],"%x", &V1740_BASE);
    printf("Base:0x%x\n", V1740_BASE);
  }

  // Test under vmic
  status = mvme_open(&myvme, 0);

  v1740_Setup(myvme, V1740_BASE, 1);
  // Run control by register
  v1740_AcqCtl(myvme, V1740_BASE, V1740_REGISTER_RUN_MODE);
  // Soft or External trigger
  v1740_TrgCtl(myvme, V1740_BASE, V1740_TRIG_SRCE_EN_MASK     , V1740_SOFT_TRIGGER|V1740_EXTERNAL_TRIGGER);
  // Soft and External trigger output
  v1740_TrgCtl(myvme, V1740_BASE, V1740_FP_TRIGGER_OUT_EN_MASK, V1740_SOFT_TRIGGER|V1740_EXTERNAL_TRIGGER);
  // Enabled groups
  v1740_GroupCtl(myvme, V1740_BASE, V1740_GROUP_EN_MASK, 0x0F);
  //  sleep(1);

  group = 0;
  // Start run then wait for trigger
  v1740_AcqCtl(myvme, V1740_BASE, V1740_RUN_START);

  regWrite(myvme, V1740_BASE, V1740_SOFTWARE_TRIGGER, 0); 
  sleep(1);
  regWrite(myvme, V1740_BASE, V1740_SOFTWARE_TRIGGER, 0); 
  sleep(1);
  regWrite(myvme, V1740_BASE, V1740_SOFTWARE_TRIGGER, 0); 

  // Evaluate the event size
  // Number of samples per groups
  n32word  =  1<<regRead(myvme, V1740_BASE, V1740_BUFFER_ORGANIZATION);
  // times the number of active groups
  chanmask = 0xff & regRead(myvme, V1740_BASE, V1740_GROUP_EN_MASK); 
  for (ngroups=0, i=0;i<8;i++) 
    if (chanmask & (1<<i))
      ngroups++;
  printf("chanmask : %x translante in %d groups of 8 channels each\n", chanmask, ngroups);
  n32word *= ngroups;
  n32word /= 2;   // 2 samples per 32bit word
  n32word += 4;   // Headers
  printf("Expected n32word:%d\n", n32word);

  //  printf("Occupancy for group %d = %d\n", group, v1740_BufferOccupancy(myvme, V1740_BASE, group)); 

#if 0
  do {
    status = regRead(myvme, V1740_BASE, V1740_ACQUISITION_STATUS);
    printf("Acq Status1:0x%x\n", status);
  } while ((status & 0x8)==0);
  
  n32read = v1740_DataRead(myvme, V1740_BASE, &(data[0]), n32word);
  printf("n32read:%d\n", n32read);
  
  for (i=0; i<n32read;i+=4) {
    printf("[%d]:0x%x - [%d]:0x%x - [%d]:0x%x - [%d]:0x%x\n"
	   , i, data[i], i+1, data[i+1], i+2, data[i+2], i+3, data[i+3]);
  }
  
  v1740_AcqCtl(myvme, V1740_BASE, V1740_RUN_STOP);
#endif

  // Read Data
  nevt = regRead(myvme, V1740_BASE, V1740_EVENT_STORED);
  n32word = regRead(myvme, V1740_BASE, V1740_EVENT_SIZE);
  printf("nevt:%d n32word:%d\n", nevt, n32word);
      
#if 1
  n32read = v1740_DataRead(myvme, V1740_BASE, &(data[0]), n32word);
#endif
#if 0
  n32read = v1740_DataBlockRead(myvme, V1740_BASE, &(data[0]), &n32word);
#endif
  printf("n32read:%d  n32requested:%d\n", n32read, n32word);
  
  for (i=0; i<32;i+=4) { // n32read;i+=4) {
    printf("[%d]:0x%x - [%d]:0x%x - [%d]:0x%x - [%d]:0x%x\n"
	   , i, data[i], i+1, data[i+1], i+2, data[i+2], i+3, data[i+3]);
  }
  
  v1740_AcqCtl(myvme, V1740_BASE, V1740_RUN_STOP);
  
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
