/*********************************************************************

  Name:         sis3320.c
  Created by:   K.Olchanski

  Contents:     SIS3320 8 ch. 12bit 250Msps
                
  $Log$
  Revision 1.1  2006/05/25 05:53:42  alpha
  First commit

*********************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "sis3320drv.h"
#include "sis3320.h"
#include "mvmestd.h"

/*****************************************************************/
/*
Read sis3320 register value
*/
static uint32_t regRead(MVME_INTERFACE *mvme, uint32_t base, int offset)
{
  mvme_set_am(mvme, MVME_AM_A32);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  return mvme_read_value(mvme, base + offset);
}

/*****************************************************************/
/*
Write sis3320 register value
*/
static void regWrite(MVME_INTERFACE *mvme, uint32_t base, int offset, uint32_t value)
{
  mvme_set_am(mvme, MVME_AM_A32);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  mvme_write_value(mvme, base + offset, value);
}

/*****************************************************************/
uint32_t sis3320_RegisterRead(MVME_INTERFACE *mvme, uint32_t base, int offset)
{
  return regRead(mvme,base,offset);
}

/*****************************************************************/
void sis3320_RegisterWrite(MVME_INTERFACE *mvme, uint32_t base, int offset, uint32_t value)
{
  regWrite(mvme, base, offset, value);
}

/*****************************************************************/
void sis3320_Reset(MVME_INTERFACE *mvme, uint32_t base)
{
  regWrite(mvme, base, SIS3320_KEY_RESET, 0);
}

/*****************************************************************/
void  sis3320_Status(MVME_INTERFACE *mvme, uint32_t base)
{
  printf("================================================\n");
  printf("SIS3320 at A32 0x%x\n", (int)base);
  printf("CSR                  : 0x%8.8x\n", regRead(mvme, base, SIS3320_CONTROL_STATUS));
  printf("ModuleID and Firmware: 0x%x\n", regRead(mvme, base, SIS3320_MODID));
  printf("Max Event Counter    :   %d\n", regRead(mvme, base, SIS3320_MAX_NOF_EVENT));
  printf("================================================\n");
}

/*****************************************************************/
/**
Sets all the necessary paramters for a given configuration.
The configuration is provided by the mode argument.
Add your own configuration in the case statement. Let me know
your setting if you want to include it in the distribution.
@param *mvme VME structure
@param  base Module base address
@param mode  Configuration mode number
@param *nentry number of entries requested and returned.
@return MVME_SUCCESS
*/
int  sis3320_Setup(MVME_INTERFACE *mvme, uint32_t base, int mode)
{
  switch (mode) {
  case 0x1:
    printf("--------------------------------------------\n");
    printf("200Msps 1evt (mode:%d)\n", mode);
    printf("Sample Length Stop, Sample Length 256\n");
    printf("Sample Start Address 0x0\n");
    printf("ADC input mode 32bits\n");
    printf("Trigger from LEMO\n");
    printf("--------------------------------------------\n");

    regWrite(mvme, base, SIS3320_MAX_NOF_EVENT, 1);
    regWrite(mvme, base, SIS3320_EVENT_CONFIG_ALL_ADC, EVENT_CONF_ENABLE_SAMPLE_LENGTH_STOP);
    regWrite(mvme, base, SIS3320_SAMPLE_LENGTH_ALL_ADC, 0x1FC);  // 0x200 or 512
    regWrite(mvme, base, SIS3320_SAMPLE_START_ADDRESS_ALL_ADC, 0x0);
    regWrite(mvme, base, SIS3320_ADC_INPUT_MODE_ALL_ADC, 0x20000);
    regWrite(mvme, base, SIS3320_ACQUISTION_CONTROL
	     , SIS3320_ACQ_SET_CLOCK_TO_200MHZ | SIS3320_ACQ_ENABLE_LEMO_START_STOP);
    printf("\n");
    break;
  case 0x2:
    printf("--------------------------------------------\n");
    printf("50Msps 1evt (mode:%d)\n", mode);
    printf("Sample Length Stop, Sample Length 256\n");
    printf("Sample Start Address 0x0\n");
    printf("ADC input mode 32bits\n");
    printf("Trigger from LEMO\n");
    printf("--------------------------------------------\n");

    regWrite(mvme, base, SIS3320_MAX_NOF_EVENT, 1);
    regWrite(mvme, base, SIS3320_EVENT_CONFIG_ALL_ADC, EVENT_CONF_ENABLE_SAMPLE_LENGTH_STOP);
    regWrite(mvme, base, SIS3320_SAMPLE_LENGTH_ALL_ADC, 0x1FC);  // 0x200 or 512
    regWrite(mvme, base, SIS3320_SAMPLE_START_ADDRESS_ALL_ADC, 0x0);
    regWrite(mvme, base, SIS3320_ADC_INPUT_MODE_ALL_ADC, 0x20000);
    regWrite(mvme, base, SIS3320_ACQUISTION_CONTROL
	     , SIS3320_ACQ_SET_CLOCK_TO_50MHZ | SIS3320_ACQ_ENABLE_LEMO_START_STOP);
    printf("\n");
    break;
  default:
    printf("Unknown setup mode\n");
    return -1;
  }
  sis3320_Status(mvme, base);
  return 0;
}

/*****************************************************************/
/*-PAA- For test purpose only */
#ifdef MAIN_ENABLE
int main (int argc, char* argv[]) {

  DWORD SIS3320_BASE  = 0x30000000;

  MVME_INTERFACE *myvme;

  uint32_t data, armed;
  int status, i;

  if (argc>1) {
    sscanf(argv[1],"%lx",&SIS3320_BASE);
  }

  // Test under vmic
  status = mvme_open(&myvme, 0);

  sis3320_Setup(myvme, SIS3320_BASE, 1);

  //  regWrite(myvme, SIS3320_BASE, SIS3320_START_DELAY, 0x20); // PreTrigger
    regWrite(myvme, SIS3320_BASE, SIS3320_STOP_DELAY, 0x80); // PostTrigger

  regWrite(myvme, SIS3320_BASE, SIS3320_KEY_ARM, 0x0);  // Arm Sampling

  armed = regRead(myvme, SIS3320_BASE, SIS3320_ACQUISTION_CONTROL);
  armed &= 0x10000;
  printf("Armed:%x\n", armed);
  while (armed) {
    armed = regRead(myvme, SIS3320_BASE, SIS3320_ACQUISTION_CONTROL);
    armed &= 0x10000;
    //    printf("Armed:%x\n", armed);
  };

  for (i=0;i<256;i++) {
    data = regRead(myvme, SIS3320_BASE, SIS3320_ADC1_OFFSET + (i<<2));
    printf("Data[0x%8.8x]=0x%x\n", SIS3320_BASE+SIS3320_ADC1_OFFSET+(i<<2), data);
  }
  //  regWrite(myvme, SIS3320_BASE, SIS3320_KEY_DISARM, 0x0);  // DisArm Sampling
  
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
