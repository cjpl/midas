/*********************************************************************
  Name:         isegvhs.c
  Created by:   P.-A. Amaudruz

  Contents:     ISEGVHS 32-channel 
                
  $Id$
*********************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "isegvhsdrv.h"
#include "isegvhs.h"
#include "mvmestd.h"

/*****************************************************************/
/*
Read isegvhs register value
*/
static uint32_t regRead(MVME_INTERFACE *mvme, DWORD base, int offset)
{
  mvme_set_am(mvme, MVME_AM_A16);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  return (mvme_read_value(mvme, base + offset) & 0xFFFF);
}

static float regReadFloat(MVME_INTERFACE *mvme, DWORD base, int offset)
{
  uint32_t temp, temp1;
  float ftemp;
  mvme_set_am(mvme, MVME_AM_A16);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  temp = mvme_read_value(mvme, base + offset);
  temp1 = mvme_read_value(mvme, base + offset+2);
  temp = (temp<<16) | (temp1 & 0xFFFF);
  //ftemp = *((float *)&temp);
  memcpy(&ftemp, &temp, 4);
  //  printf("regReadFloat : %f 0x%x\n", ftemp, temp);
  return (ftemp);
}

/*****************************************************************/
/*
Write isegvhs register value
*/
static void regWrite(MVME_INTERFACE *mvme, DWORD base, int offset, uint32_t value)
{
  mvme_set_am(mvme, MVME_AM_A16);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base + offset, value);
}

static void regWriteFloat(MVME_INTERFACE *mvme, DWORD base, int offset, float value)
{
  uint32_t temp;
  memcpy(&temp, &value, 4);
  mvme_set_am(mvme, MVME_AM_A16);
  mvme_set_dmode(mvme, MVME_DMODE_D16);
  mvme_write_value(mvme, base + offset, ((temp>>16)&0xFFFF));
  mvme_write_value(mvme, base + offset+2, (temp & 0xFFFF));
}

/*****************************************************************/
uint32_t isegvhs_RegisterRead(MVME_INTERFACE *mvme, DWORD base, int offset)
{
  return regRead(mvme, base, offset);
}

/*****************************************************************/
float isegvhs_RegisterReadFloat(MVME_INTERFACE *mvme, DWORD base, int offset)
{
  float temp;
  temp = regReadFloat(mvme, base, offset);
  //  printf("temp:%f\n", temp);
  return temp;
}

/*****************************************************************/
void     isegvhs_RegisterWrite(MVME_INTERFACE *mvme, DWORD base, int offset, uint32_t value)
{
  regWrite(mvme,base,offset,value);
}

/*****************************************************************/
void     isegvhs_RegisterWriteFloat(MVME_INTERFACE *mvme, DWORD base, int offset, float value)
{
  regWriteFloat(mvme, base, offset, value);
}

/*****************************************************************/
void  isegvhs_Status(MVME_INTERFACE *mvme, DWORD base)
{
  uint32_t sn=0;
  uint64_t vendor=0;
  printf("ISEGVHS at A32 0x%x\n", (int)base);
  vendor = (regRead(mvme,base,ISEGVHS_VENDOR_ID)<<16) | regRead(mvme,base,ISEGVHS_VENDOR_ID+2);
  printf("Module Vendor     : 0x%x%x\n", regRead(mvme,base,ISEGVHS_VENDOR_ID), regRead(mvme,base,ISEGVHS_VENDOR_ID+2));
  printf("Module Vendor     : %s\n", (char *)&vendor);
  printf("Module Status     : 0x%x\n", regRead(mvme,base,ISEGVHS_MODULE_STATUS));
  printf("Module Control    : 0x%x\n", regRead(mvme,base,ISEGVHS_MODULE_CONTROL));
  sn = regRead(mvme, base, ISEGVHS_SERIAL_NUMBER);
  printf("Module S/N        : %d\n", (sn<<16)|regRead(mvme, base, ISEGVHS_SERIAL_NUMBER+2));
  printf("Module Temp.      : %f\n", isegvhs_RegisterReadFloat(mvme, base, ISEGVHS_TEMPERATURE));
  printf("Module Supply P5  : %f\n", regReadFloat(mvme, base, ISEGVHS_SUPPLY_P5));
  printf("Module Supply P12 : %f\n", regReadFloat(mvme, base, ISEGVHS_SUPPLY_P12));
  printf("Module Supply N12 : %f\n", regReadFloat(mvme, base, ISEGVHS_SUPPLY_N12));
  printf("Module Voltage Max: %f\n", regReadFloat(mvme, base, ISEGVHS_VOLTAGE_MAX));
  printf("Module Current Max: %f\n", regReadFloat(mvme, base, ISEGVHS_CURRENT_MAX));
}

/*****************************************************************/
/*-PAA- For test purpose only */
#ifdef MAIN_ENABLE
int main (int argc, char* argv[]) {

  uint32_t ISEGVHS_BASE  = 0x4000;
  MVME_INTERFACE *myvme;
  int status;

  if (argc>1) {
    sscanf(argv[1],"%x", &ISEGVHS_BASE);
  }

  // Test under vmic
  status = mvme_open(&myvme, 0);

  isegvhs_Status(myvme, ISEGVHS_BASE);

  //  regWrite(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_CHANNEL_CONTROL, 0x0);
  printf(" Channel 0: Status        :0x%x\n", regRead(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_CHANNEL_STATUS));
  printf(" Channel 0: Control       :0x%x\n\n", regRead(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_CHANNEL_CONTROL));

 
  regWrite(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_CHANNEL_CONTROL, 0x4);
  regWriteFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_VOLTAGE_RAMP_SPEED, 30.);
  regWriteFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_CURRENT_RAMP_SPEED, 120.);

  regWriteFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_VOLTAGE_BOUND, 2.0);
  regWriteFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_VOLTAGE_NOMINAL, 2130.0);
  regWriteFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_VOLTAGE_SET, 1234.5678);

  regWrite(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_CHANNEL_CONTROL, 0x8);
  //  regWriteFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_CURRENT_BOUND, 0.003);
  //  regWriteFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_CURRENT_NOMINAL, 0.0012);

  printf(" Channel 0: Voltage Ramp: %f\n", regReadFloat(myvme, ISEGVHS_BASE, ISEGVHS_VOLTAGE_RAMP_SPEED));
  printf(" Channel 0: Current Ramp: %f\n\n", regReadFloat(myvme, ISEGVHS_BASE, ISEGVHS_CURRENT_RAMP_SPEED));

  printf(" Channel 0: Voltage set   : %f\n", regReadFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_VOLTAGE_SET));
  printf(" Channel 0: Voltage Mes   : %f\n", regReadFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_VOLTAGE_MEASURE));
  printf(" Channel 0: Voltage Bound : %f\n", regReadFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_VOLTAGE_BOUND));
  printf(" Channel 0: Voltage Nom   : %f\n\n", regReadFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_VOLTAGE_NOMINAL));

  printf(" Channel 0: Current set   : %f\n", regReadFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_CURRENT_SET));
  printf(" Channel 0: Current Mes   : %f\n", regReadFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_CURRENT_MEASURE));
  printf(" Channel 0: Current Bound : %f\n", regReadFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_CURRENT_BOUND));
  
  printf(" Channel 0: Status        :0x%x\n", regRead(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_CHANNEL_STATUS));
  printf(" Channel 0: Control       :0x%x\n\n", regRead(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_CHANNEL_CONTROL));
  
  printf("Module Status             : 0x%x\n", regRead(myvme,ISEGVHS_BASE,ISEGVHS_MODULE_STATUS));
  printf("Module Control            : 0x%x\n", regRead(myvme,ISEGVHS_BASE,ISEGVHS_MODULE_CONTROL));
  
  // Close VME   
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
