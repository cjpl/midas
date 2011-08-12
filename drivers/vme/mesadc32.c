/*********************************************************************
  Name:         mesadc32.c
  Created by:   

  Contents:     MESADC32 32ch 11..13bit Peak sensing ADC
 
  $Id$
*********************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mesadc32drv.h"
#include "mvmestd.h"

/*****************************************************************/
/*
Read MESADC32 register value
*/
static uint32_t regReadReg(MVME_INTERFACE *mvme, uint32_t base, int offset)
{
  mvme_set_am(mvme, MVME_AM_A32);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  return mvme_read_value(mvme, base + offset);
}

/*****************************************************************/
/*
Write MESADC32 register value
*/
static void regWriteReg(MVME_INTERFACE *mvme, uint32_t base, int offset, uint32_t value)
{
  mvme_set_am(mvme, MVME_AM_A32);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base + offset, value);
}

/*****************************************************************/
/*
  Read MESADC32 register value
*/
int mesadc32_ReadData(MVME_INTERFACE *mvme, uint32_t base, uint32_t *pdata, int *nw32)
{
  int i;
  *nw32 = regReadReg(mvme, base, MESADC32_BUF_DATA_LEN);
  mvme_set_am(mvme, MVME_AM_A32);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  if (*nw32) {
    for (i=0 ; i<*nw32 ; i++) {
      *pdata = mvme_read_value(mvme, base + MESADC32_EVENT_READOUT_BUFFER);
      pdata++;
    }
    return 0;
  }
  return MESADC32_EMPTY;
}

/* User calls */
/*****************************************************************/
uint32_t mesadc32_RegisterRead(MVME_INTERFACE *mvme, uint32_t base, int offset)
{
  return regReadReg(mvme, base, offset);
}

/*****************************************************************/
void mesadc32_RegisterWrite(MVME_INTERFACE *mvme, uint32_t base, int offset, uint32_t value)
{
  regWriteReg(mvme, base, offset, value);
}

/*****************************************************************/
void mesadc32_Reset(MVME_INTERFACE *mvme, uint32_t base)
{
  regWriteReg(mvme, base, 0, 0);
}

/*****************************************************************/
void mesadc32_ResolutionSet(MVME_INTERFACE *mvme, uint32_t base, uint32_t resolution)
{
  uint32_t reg;

  reg = MESADC32_ADC_RESOLUTION;
  if ((resolution >= 0) && (resolution <=4))
    regWriteReg(mvme, base, reg, resolution);
  else
    printf("Resolution out of range (%d)\n", resolution);
  return;
}

/*****************************************************************/
void mesadc32_ThresholdSet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t thres)
{
  uint32_t reg;

  if (thres > 0x1FFF) {
    printf("Threshold, channel out of range (%d)\n", channel);
  } else if (channel < MESADC32_MAX_CHANNELS) {
    reg = MESADC32_THRESHOLD_0 + 2*channel;
    regWriteReg(mvme, base, reg, thres);
  } else
    printf("Threshold, channel out of range (%d)\n", channel);
  return;
}

/*****************************************************************/
void mesadc32_ThresholdGet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t *thres)
{
  uint32_t reg;

  if (channel < MESADC32_MAX_CHANNELS) {
    reg = MESADC32_THRESHOLD_0 + 2*channel;
    *thres = regReadReg(mvme, base, reg);
  }
  else
    printf("Threshold, channel out of range (%d)\n", channel);
  return;
}

/*****************************************************************/
void mesadc32_AcqCtl(MVME_INTERFACE *mvme, uint32_t base, uint32_t operation)
{
  //  uint32_t reg;
  
  /*  reg = regRead(mvme, base, MESADC32_ACQUISITION_CONTROL);  
  switch (operation) {
  case RUN_START:
    regWrite(mvme, base, MESADC32_ACQUISITION_CONTROL, (reg | 0x4));
    break;
  case RUN_STOP:
    regWrite(mvme, base, MESADC32_ACQUISITION_CONTROL, (reg & ~(0x4)));
    break;
  case COUNT_ALL_TRIGGER:
    regWrite(mvme, base, MESADC32_ACQUISITION_CONTROL, (reg & ~(0x08)));
    break;
  default:
    break;
  }
  */
}

/*****************************************************************/
int rcWait(MVME_INTERFACE *mvme, uint32_t base, uint32_t modAdd, int timeout) 
{
  uint16_t reg;

  do {
    reg = regReadReg(mvme, base, MESADC32_RC_SENT_RTN_STAT);
    ss_sleep(1); // >100us
  } while ((timeout--) && (reg & 0x1));
  if (timeout == 0) {
    printf("Timeout to the MSCF16 %d\n", modAdd);
    if (reg & 0x2) printf("address collision\n");
    if (reg & 0x4) printf("any collision (no termination?)\n");
    if (reg & 0x8) printf("no response from the bus (no valid address?)\n");
    // Deactivate control bus
    regWriteReg(mvme, base, MESADC32_NIM_BUSY, 0x0);
    return -1;
  }
  return 0;
}

/*****************************************************************/
uint32_t mesadc32_MSCF16_IDC(MVME_INTERFACE *mvme, uint32_t base, uint32_t modAdd)
{
  uint16_t reg;
  // Activate control bus
  regWriteReg(mvme, base, MESADC32_NIM_BUSY, 0x3);
  ss_sleep(1); // >400us
  // Get Module ID-Code (should read 20 for the MSCF16
  regWriteReg(mvme, base, MESADC32_RC_MODNUM, modAdd);
  ss_sleep(1); // >400us
  regWriteReg(mvme, base, MESADC32_RC_OPCODE, 6);
  ss_sleep(1); // >400us
  // Init send request
  regWriteReg(mvme, base, MESADC32_RC_DATA, 0);
  ss_sleep(1); // >400us
  if (rcWait(mvme, base, modAdd, 10)) return -1;
  reg = regReadReg(mvme, base, MESADC32_RC_DATA);
  ss_sleep(1); // >400us
  //  if (reg & 0x1) printf("RC status ON  ");
  //  else           printf("RC status OFF ");
  //  printf("ID-Code : %d [0x%x]\n", ((reg>>1) & 0x7F), reg);
  //  if (((reg>>1) & 0x7F)) retsta = 0;
  // Deactivate control bus
  regWriteReg(mvme, base, MESADC32_NIM_BUSY, 0x0);
  return reg;
}

/*****************************************************************/
uint32_t mesadc32_MSCF16_GainSet(MVME_INTERFACE *mvme, uint32_t base, uint32_t modAdd, int subAdd, int data)
{
  uint32_t reg;
  // Activate control bus
  regWriteReg(mvme, base, MESADC32_NIM_BUSY, 0x3);
  ss_sleep(1); // >400us
  // Get Module ID-Code (should read 20 for the MSCF16
  regWriteReg(mvme, base, MESADC32_RC_MODNUM, modAdd);
  ss_sleep(1); // >400us
  regWriteReg(mvme, base, MESADC32_RC_OPCODE, 16);
  ss_sleep(1); // >400us
  // Write sub first address 
  regWriteReg(mvme, base, MESADC32_RC_ADDRESS, subAdd);
  ss_sleep(1); // >400us
  // Write sub last address
  regWriteReg(mvme, base, MESADC32_RC_DATA, data);
  ss_sleep(1); // >400us
  if (rcWait(mvme, base, modAdd, 10)) return -1;
  reg = regReadReg(mvme, base, MESADC32_RC_DATA);
  ss_sleep(1); // >400us
  if (reg == data) {
    //    printf("Set Gain complete for ID:%d sub:%d data:%d\n",modAdd, subAdd, data);
    // deactivate control bus
    regWriteReg(mvme, base, MESADC32_NIM_BUSY, 0x0);
    return 0;
  } else {
    printf("Set Gain failed for ID:%d  sub:%d data:%d [%d]\n",modAdd, subAdd, data, reg);
    // deactivate control bus
    regWriteReg(mvme, base, MESADC32_NIM_BUSY, 0x0);
    return -1;
  }
}


/*****************************************************************/
uint32_t mesadc32_MSCF16_GainGet(MVME_INTERFACE *mvme, uint32_t base, uint32_t modAdd, int subAdd, uint32_t *data)
{
  // Activate control bus
  regWriteReg(mvme, base, MESADC32_NIM_BUSY, 0x3);
  ss_sleep(1); // >400us
  // Get Module ID-Code (should read 20 for the MSCF16
  regWriteReg(mvme, base, MESADC32_RC_MODNUM, modAdd);
  ss_sleep(1); // >400us
  regWriteReg(mvme, base, MESADC32_RC_OPCODE, 18);
  ss_sleep(1); // >400us
  // Write sub first address 
  regWriteReg(mvme, base, MESADC32_RC_ADDRESS, subAdd);
  ss_sleep(1); // >400us
  // Write sub last address
  regWriteReg(mvme, base, MESADC32_RC_DATA, 0);
  ss_sleep(1); // >400us
  if (rcWait(mvme, base, modAdd, 10)) return -1;
  *data = regReadReg(mvme, base, MESADC32_RC_DATA);
  ss_sleep(1); // >400us
  // deactivate control bus
  regWriteReg(mvme, base, MESADC32_NIM_BUSY, 0x0);
  return 0;
}

/*****************************************************************/
uint32_t mesadc32_MSCF16_RCon(MVME_INTERFACE *mvme, uint32_t base, uint32_t modAdd)
{
  // Activate control bus
  regWriteReg(mvme, base, MESADC32_NIM_BUSY, 0x3);
  ss_sleep(1); // >400us
  // Get Module ID-Code (should read 20 for the MSCF16
  regWriteReg(mvme, base, MESADC32_RC_MODNUM, modAdd);
  ss_sleep(1); // >400us
  // Active RC 
  regWriteReg(mvme, base, MESADC32_RC_OPCODE, 3);
  ss_sleep(1); // >400us
  // deactive control bus
  regWriteReg(mvme, base, MESADC32_NIM_BUSY, 0x0);
  return 0;
}

/*****************************************************************/
void  mesadc32_Status(MVME_INTERFACE *mvme, uint32_t base)
{
  printf("================================================\n");
  printf("MESADC32 at A32 0x%x\n", (int)base);
  // printf("Board ID             : 0x%x\n", regRead(mvme, base, MESADC32_BOARD_ID));
  printf("FW revision          : 0x%x\n", regReadReg (mvme, base, MESADC32_FIRM_REV));
  printf("Data format          : 0x%x\n", regReadReg (mvme, base, MESADC32_DATA_LEN_FMT));
  printf("Multi Event mode     : 0x%x\n", regReadReg (mvme, base, MESADC32_MULTIEVENT));
  printf("EndOfEvent marker    : 0x%x\n", regReadReg (mvme, base, MESADC32_MARKING_TYPE));
  printf("ADC Resolution mode  : 0x%x\n", regReadReg (mvme, base, MESADC32_ADC_RESOLUTION));
  printf("ADC Input range      : 0x%x\n", regReadReg (mvme, base, MESADC32_INPUT_RANGE));
  printf("Test Pulse setting   : 0x%x\n", regReadReg (mvme, base, MESADC32_TESTPULSER));
  // printf("Acquisition status   : 0x%8.8x\n", regRead(mvme, base, MESADC32_ACQUISITION_STATUS));
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
int  mesadc32_Setup(MVME_INTERFACE *mvme, uint32_t base, int mode)
{
  WORD reg;

  switch (mode) {
  case 0x0:
    printf("--------------------------------------------\n");
    printf("Setup Skip\n");
    printf("--------------------------------------------\n");
    break;
  case 0x1:
    printf("--------------------------------------------\n");
    printf("Standard Setup\n");
    printf("--------------------------------------------\n");
    // Stop acquisition (0x603A W=0)
    regWriteReg(mvme, base, MESADC32_START_ACQ, 0);
    
    // Module address (0x6000 W=1), (0x6002 W=add)

    // Soft reset to restore defaults (0x6008 W=0)
    regWriteReg(mvme, base, MESADC32_SOFT_RESET, 0);

    // Module ID in the Header (0x6004 W=0x01..0x0y)

    // Read firmware revision (0x600E R=0x01.10)
    reg = regReadReg (mvme, base, MESADC32_FIRM_REV);
    printf("FW rev: %x\n", reg);

    // Set data length format (0x6032 W=2) D32
    regWriteReg(mvme, base, MESADC32_DATA_LEN_FMT, 2);
    
    // Set multievent off, (0X6036 w=0) use 0x6034 for enable next event
    regWriteReg(mvme, base, MESADC32_MULTIEVENT, 0);

    // Enable EoE marker for time stamp (0x6038 W=1)
    regWriteReg(mvme, base, MESADC32_MARKING_TYPE, 1);

    // Set adc resolution (0x6042 W=0[2K])
    regWriteReg(mvme, base, MESADC32_ADC_RESOLUTION, 0);

    // Gate Generator 0, 1 default d20/w50
    // Input range (0x6060 W=0 0..4V input)
    regWriteReg(mvme, base, MESADC32_INPUT_RANGE, 0);

    // CTRA 00: (0x6069 W'00:VME frq, external reset disable)
    regWriteReg(mvme, base, MESADC32_TS_SOURCE, 0);
    
    // CTRA  Reset all counters (0x6090 W 1)
    regWriteReg(mvme, base, MESADC32_RESET_CTL_AB, 1);
    // NIM gate1 (0x606A W=0),
    // NIM Fast Reset 

    // Reset (0x6034)
    regWriteReg(mvme, base, MESADC32_READOUT_RESET, 0);

    // Start acquisition (0x603A W=1)
    regWriteReg(mvme, base, MESADC32_START_ACQ, 1);

    // Read buffer data length (0x6030)
    reg = regReadReg (mvme, base, MESADC32_BUF_DATA_LEN);
    printf("Data in Fifo: %x\n", reg);

    // Read Data ready (0x603E R==1)
    reg = regReadReg (mvme, base, MESADC32_DATA_READY);
    printf("Data ready: %i\n", reg);

    // After reading the Fifo (0x0000) 
    // Issue the MESADC32_READOUT_RESET to allow new event
    // Reset (0x6034)
    // regWriteReg(mvme, base, MESADC32_READOUT_RESET, 0);

    break;
  default:
    printf("Unknown setup mode\n");
    return -1;
  }
  mesadc32_Status(mvme, base);
  return 0;
}

/*****************************************************************/
/*-PAA- For test purpose only */
#ifdef MAIN_ENABLE
int main (int argc, char* argv[]) {

  uint32_t MESADC32_BASE  = 0x10000000;
  uint32_t data[1000];
  int i, status, reg;
  MVME_INTERFACE *myvme;

  //  uint32_t data[100000], n32word, n32read;
  //  int status, channel, i, j, nchannels=0, chanmask;

  if (argc>1) {
    sscanf(argv[1],"%x", &MESADC32_BASE);
  }

  // Test under vmic
  status = mvme_open(&myvme, 0);

  mesadc32_Setup(myvme, MESADC32_BASE, 1);

  for (i=0;i<16;i++) {
    status = mesadc32_MSCF16_IDC(myvme, MESADC32_BASE, i);
    if (status > 0) printf("MSCF16[%d]: RC-bus:%d - ID_Code:%d\n", i, status & 0x1, ((status>>1)&0x7f));
  }
  printf("\n");

  for (i=0;i<32;i++) {
    mesadc32_ThresholdSet(myvme, MESADC32_BASE, i, 1360);
    mesadc32_ThresholdGet(myvme, MESADC32_BASE, i, &(data[0]));
    printf("Threshold: 0, ch:%i Data:0x%x (0x%x)\n", i, data[0], status);
  }
  
  for (i=0;i<16;i++) {
    status = mesadc32_MSCF16_GainSet(myvme, MESADC32_BASE, 1, i, 2*i);
    printf("Gain Mod: 0, ch:%i Data:0x%x (0x%x)\n", i, 2*i, status);
  }
  printf("\n");

  for (i=0;i<16;i++) {
    status = mesadc32_MSCF16_GainGet(myvme, MESADC32_BASE, 1, i, &(data[0]));
    printf("Gain Mod: 0, ch:%i Data:0x%x (0x%x)\n", i, data[0], status);
  }

  // CTRA  Reset all counters (0x6090 W 1)
  mesadc32_RegisterWrite(myvme, MESADC32_BASE, MESADC32_TS_DIVIDER, 10000);
  mesadc32_RegisterWrite(myvme, MESADC32_BASE, MESADC32_RESET_CTL_AB, 1);
  mesadc32_RegisterWrite(myvme, MESADC32_BASE, MESADC32_TESTPULSER, 0x6);
  for (;;){
    do {
      //    mesadc32_Status(myvme, MESADC32_BASE);
      // Read buffer data length (0x6030)
      reg = mesadc32_RegisterRead(myvme, MESADC32_BASE, MESADC32_BUF_DATA_LEN);
    } while (reg==0);
    if (reg) {
      printf("Data in Fifo: 0x%x\n", reg);
      reg = 0;
      mesadc32_ReadData(myvme, MESADC32_BASE, &(data[0]), &reg);
      for (i=0;i<reg;i++) {
	printf("Data[%2.2i]:0x%08x\n", i, data[i]);
      } 
      // Reset (0x6034)
      mesadc32_RegisterWrite(myvme, MESADC32_BASE, MESADC32_READOUT_RESET, 0);
    }
  }
  /*
  do {
    status = regRead(myvme, MESADC32_BASE, MESADC32_ACQUISITION_STATUS);
    printf("Acq Status1:0x%x\n", status);
  } while ((status & 0x8)==0);
  
  n32read = mesadc32_dataRead(myvme, MESADC32_BASE, &(data[0]), n32word);
  printf("n32read:%d\n", n32read);
  
  for (i=0; i<n32read;i+=4) {
    printf("[%d]:0x%x - [%d]:0x%x - [%d]:0x%x - [%d]:0x%x\n"
	   , i, data[i], i+1, data[i+1], i+2, data[i+2], i+3, data[i+3]);
  }
  
  do {
    status = regRead(myvme, MESADC32_BASE, MESADC32_ACQUISITION_STATUS);
    printf("Acq Status2:0x%x\n", status);
  } while ((status & 0x8)==0);
  
  n32read = mesadc32_dataRead(myvme, MESADC32_BASE, &(data[0]), n32word);
  printf("n32read:%d\n", n32read);
  
  for (i=0; i<n32read;i+=4) {
    printf("[%d]:0x%x - [%d]:0x%x - [%d]:0x%x - [%d]:0x%x\n"
	   , i, data[i], i+1, data[i+1], i+2, data[i+2], i+3, data[i+3]);
  }
  */
  
  
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
