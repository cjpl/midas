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
  uint32_t sn=0, csr;
  uint64_t vendor=0;
  printf("ISEGVHS at A32 0x%x\n", (int)base);
  vendor = (regRead(mvme,base,ISEGVHS_VENDOR_ID)<<16) | regRead(mvme,base,ISEGVHS_VENDOR_ID+2);
  printf("Module Vendor     : 0x%x%x\n", regRead(mvme,base,ISEGVHS_VENDOR_ID), regRead(mvme,base,ISEGVHS_VENDOR_ID+2));
  printf("Module Vendor     : %s\t\t", (char *)&vendor);
  csr = regRead(mvme,base,ISEGVHS_MODULE_STATUS);
  printf("Module Status     : 0x%x\n >> ", csr);
  printf("%s", csr & 0x0001 ? "FineAdj /" : " NoFineAdj /"); 
  printf("%s", csr & 0x0080 ? "CmdCpl /" : " /"); 
  printf("%s", csr & 0x0100 ? "noError /" : " ERROR /"); 
  printf("%s", csr & 0x0200 ? "noRamp /" : " RAMPING /"); 
  printf("%s", csr & 0x0400 ? "Saf closed /" : " /"); 
  printf("%s", csr & 0x0800 ? "Evt Active /" : " /"); 
  printf("%s", csr & 0x1000 ? "Mod Good /" : " MOD NO good /"); 
  printf("%s", csr & 0x2000 ? "Supply Good /" : " Supply NO good /"); 
  printf("%s\n", csr & 0x4000 ? "Temp Good /" : " Temp NO good /"); 
  printf("Module Control    : 0x%x\t\t", regRead(mvme,base,ISEGVHS_MODULE_CONTROL));
  sn = regRead(mvme, base, ISEGVHS_SERIAL_NUMBER);
  printf("Module S/N        : %d\n", (sn<<16)|regRead(mvme, base, ISEGVHS_SERIAL_NUMBER+2));
  printf("Module Temp.      : %f\t\t", isegvhs_RegisterReadFloat(mvme, base, ISEGVHS_TEMPERATURE));
  printf("Module Supply P5  : %f\n", regReadFloat(mvme, base, ISEGVHS_SUPPLY_P5));
  printf("Module Supply P12 : %f\t\t", regReadFloat(mvme, base, ISEGVHS_SUPPLY_P12));
  printf("Module Supply N12 : %f\n", regReadFloat(mvme, base, ISEGVHS_SUPPLY_N12));
  printf("Module Trim V Max : %f\t\t", regReadFloat(mvme, base, ISEGVHS_VOLTAGE_MAX));
  printf("Module Trim I Max : %f\n", regReadFloat(mvme, base, ISEGVHS_CURRENT_MAX));
  printf("Module Event Stat : 0x%04x\t\t", regRead(mvme, base, ISEGVHS_MODULE_EVENT_STATUS));
  printf("Module Event Mask : 0x%04x\n", regRead(mvme, base, ISEGVHS_MODULE_EVENT_MASK));
  printf("Channel Event Stat: 0x%04x\t\t", regRead(mvme, base, ISEGVHS_CHANNEL_EVENT_STATUS));
  printf("Channel Event Mask: 0x%04x\n\n", regRead(mvme, base, ISEGVHS_CHANNEL_EVENT_MASK));
}

/*****************************************************************/
/*-PAA- For test purpose only */
#ifdef MAIN_ENABLE
int main (int argc, char* argv[]) {

  uint32_t ISEGVHS_BASE  = 0x4000;
  MVME_INTERFACE *myvme;
  int status, i;
  
  if (argc>1) {
    sscanf(argv[1],"%x", &ISEGVHS_BASE);
  }
  
  // Test under vmic
  status = mvme_open(&myvme, 0);

  isegvhs_Status(myvme, ISEGVHS_BASE);

  //    regWrite(myvme, ISEGVHS_BASE, ISEGVHS_MODULE_CONTROL, 0x40);    // doCLEAR
  //    regWrite(myvme, ISEGVHS_BASE, ISEGVHS_MODULE_CONTROL, 0x1000);    // setADJ
  //    regWrite(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_CHANNEL_CONTROL, 0x0);
  //  printf(" Channel 0: Status        :0x%x\n", regRead(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_CHANNEL_STATUS));
  //  printf(" Channel 0: Control       :0x%x\n\n", regRead(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_CHANNEL_CONTROL));

 
  // regWrite(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_CHANNEL_CONTROL, 0x4);
  //  regWriteFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_VOLTAGE_RAMP_SPEED, 5.);
  //  regWriteFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_CURRENT_RAMP_SPEED, 6.);

  //  regWriteFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_VOLTAGE_BOUND, 1.0);
  //  regWriteFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_VOLTAGE_NOMINAL, 2000.0);
  //  regWriteFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_VOLTAGE_SET, 0.0);

  //  regWrite(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_CHANNEL_CONTROL, 0x8);
  // regWriteFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_CURRENT_BOUND, 0.022);
  //  regWriteFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+ISEGVHS_CURRENT_NOMINAL, 0.0012);

  printf("Chn     Vrmp   Irmp      Vset      Vmes    Vbnd    Vnom      IsetmA   ImesmA  Ibnd   CSta   CCtl\n");
  for (i=0;i<12;i++) {
    printf("%02d ", i);
    regWriteFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+(i*ISEGVHS_CHANNEL_OFFSET)+ISEGVHS_VOLTAGE_BOUND, 0.5);
    //    regWriteFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+(i*ISEGVHS_CHANNEL_OFFSET)+ISEGVHS_CURRENT_BOUND, 0.004);
    printf("%8.2f ", regReadFloat(myvme, ISEGVHS_BASE, ISEGVHS_VOLTAGE_RAMP_SPEED));
    printf("%8.2f ", regReadFloat(myvme, ISEGVHS_BASE, ISEGVHS_CURRENT_RAMP_SPEED));
    printf("%8.2f ", regReadFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+(i*ISEGVHS_CHANNEL_OFFSET)+ISEGVHS_VOLTAGE_SET));
    printf("%8.2f ", regReadFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+(i*ISEGVHS_CHANNEL_OFFSET)+ISEGVHS_VOLTAGE_MEASURE));
    printf("%8.2f ", regReadFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+(i*ISEGVHS_CHANNEL_OFFSET)+ISEGVHS_VOLTAGE_BOUND));
    printf("%8.2f ", regReadFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+(i*ISEGVHS_CHANNEL_OFFSET)+ISEGVHS_VOLTAGE_NOMINAL));
    
    printf("%8.3f ", 1000.*regReadFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+(i*ISEGVHS_CHANNEL_OFFSET)+ISEGVHS_CURRENT_SET));
    printf("%8.3f ", 1000.*regReadFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+(i*ISEGVHS_CHANNEL_OFFSET)+ISEGVHS_CURRENT_MEASURE));
    printf("%8.2f ", regReadFloat(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+(i*ISEGVHS_CHANNEL_OFFSET)+ISEGVHS_CURRENT_BOUND));
    
    printf("0x%04x ", regRead(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+(i*ISEGVHS_CHANNEL_OFFSET)+ISEGVHS_CHANNEL_STATUS));
    printf("0x%04x ", regRead(myvme, ISEGVHS_BASE, ISEGVHS_CHANNEL_BASE+(i*ISEGVHS_CHANNEL_OFFSET)+ISEGVHS_CHANNEL_CONTROL));
    
    printf("\n");
  }
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
