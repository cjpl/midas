/*********************************************************************
  @file
  Name:         vpc6.c
  Created by:   Pierre-Andre Amaudruz, Chris Ohlmann

  Contents:      Routines for accessing the vpc6 Triumf board

  $Id$
*********************************************************************/
#include <stdio.h>
#include <string.h>
#include "vpc6.h"

/*****************************************************************/
/*
  Read VPC6 Busy state for a particular port.
  Port number start with 1
*/
int vpc6_isPortBusy(MVME_INTERFACE *mvme, DWORD base, WORD  port)
{
  int   cmode, busy;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  busy = mvme_read_value(mvme, base+VPC6_SR_RO);
  mvme_set_dmode(mvme, cmode);
  return (busy & (1 << (port-1)));
}

/*****************************************************************/
/**
 * Setup Preamp type of the board (2 bit per PA)
 * 00 : ASD, 01 : Buckeye
 */
void vpc6_PATypeWrite(MVME_INTERFACE *mvme, DWORD base, WORD data)
{
  int cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  data &= 0xFFF;
  mvme_write_value(mvme, base+VPC6_CR_RW, data);
  mvme_set_dmode(mvme, cmode);
  return;
}

/*****************************************************************/
/**
 * Read the 6 AP type of the board (either ASD01 or Buckeye for now.
 */
int vpc6_PATypeRead(MVME_INTERFACE *mvme, DWORD base)
{
  int cmode, type;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  type = mvme_read_value(mvme, base+VPC6_CR_RW);
  mvme_set_dmode(mvme, cmode);
  return (type & 0xFFF);
}

/*****************************************************************/
/**
 * Read Port type of the board (either ASD01 or Buckeye for now)
 * return the type of the requested port
 */
int vpc6_PortTypeRead(MVME_INTERFACE *mvme, DWORD base, WORD port)
{
  int cmode, type;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  type = mvme_read_value(mvme, base+VPC6_CR_RW);
  mvme_set_dmode(mvme, cmode);
  type  = ((type >> (2*(port-1))) & 0x03);   // ASD or Buck
  return (type & 0xFFF);
}

/*****************************************************************/
/**
 * Load full configuration for a given port.
 * the port is either ASD01 or Buckeye for now.
 */
int vpc6_PortCfgLoad(MVME_INTERFACE *mvme, DWORD base, WORD port)
{
  int cmode, action, timeout=100;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  if ((port > 0) && (port < 7)) {
    action = (0x18) | port;
  }
  else
    return VPC6_PARAM_ERROR;

  do {
    if (~(vpc6_isPortBusy(mvme, base, port))) {
      // Not busy, load bit string to PA
      mvme_write_value(mvme, base+VPC6_CMD_WO, action);
      usleep(10000);
      mvme_write_value(mvme, base+VPC6_CMD_WO, action);
      break;
    }
    else
      usleep(10000);
  } while (timeout--);

  if (timeout == 0)
    printf("Port %d still busy\n", port);

  mvme_set_dmode(mvme, cmode);
  return VPC6_SUCCESS;
}

/*****************************************************************/
/**
 * Retrieve bit string full configuration from a given port.
 * Loads bit string into Readback registers that can then be read.
 * the port is either ASD01 or Buckeye for now.
 */
int vpc6_CfgRetrieve(MVME_INTERFACE *mvme, DWORD base, WORD port)
{
  int cmode, action, timeout=100;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  if ((port > 0) && (port < 7)) {
    action = (0x10) | port;
  }

  do {
    if (~(vpc6_isPortBusy(mvme, base, port))) {
      // Not busy, read bit string from PA
      //      printf(" cfg request 0x%x %d\n", base+VPC6_CMD_WO, action);
      mvme_write_value(mvme, base+VPC6_CMD_WO, action);
      usleep(10000);
      mvme_write_value(mvme, base+VPC6_CMD_WO, action);
      break;
    }
    usleep(10000);
  } while (timeout--);

  if (timeout == 0)
    printf(" Port %d still busy\n", port);

  mvme_set_dmode(mvme, cmode);
  return VPC6_SUCCESS;
}

/*****************************************************************/
/**
 * Read the Readback and Display in a readable form the Port setting
 * No action to the PA board
 */
int vpc6_PortRegRBRead(MVME_INTERFACE *mvme, DWORD base, WORD port)
{
  int cmode, timeout=100;
  WORD type;
  DWORD reg[4];

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  do {
    if (~(vpc6_isPortBusy(mvme, base, port))) {
      type  = mvme_read_value(mvme, base+VPC6_CR_RW);
      type  = ((type >> (2*(port-1))) & 0x03);   // ASD or Buck
      reg[0] = mvme_read_value(mvme, base+VPC6_RBCK_RO+(0x10*(port-1))+0x00);
      reg[1] = mvme_read_value(mvme, base+VPC6_RBCK_RO+(0x10*(port-1))+0x04);
      reg[2] = mvme_read_value(mvme, base+VPC6_RBCK_RO+(0x10*(port-1))+0x08);
      reg[3] = mvme_read_value(mvme, base+VPC6_RBCK_RO+(0x10*(port-1))+0x0C);
      vpc6_PortDisplay(type, port, reg);
      break;
    }
  } while (timeout--);

  //  printf("DEBUG Readback Register:\n reg[0]=0x%8lx \n reg[1]=0x%8lx \n reg[2]=0x%8lx \n reg[3]=0x%8lx \n"
  //     , reg[0], reg[1], reg[2], reg[3]);

  if (timeout == 0)
    printf(" Port %d busy\n", port);

  mvme_set_dmode(mvme, cmode);
  return VPC6_SUCCESS;
}

/*****************************************************************/
/**
 * Read the Register and Display in a readable form the Port setting
 * No action to the PA board
 */
int vpc6_PortRegRead(MVME_INTERFACE *mvme, DWORD base, WORD port)
{
  int cmode;
  WORD type;
  DWORD reg[4];

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  type  = mvme_read_value(mvme, base+VPC6_CR_RW);
  type  = ((type >> (2*(port-1))) & 0x03);   // ASD or Buck
  reg[0] = mvme_read_value(mvme, base+VPC6_CFG_RW+(0x10*(port-1))+0x00);
  reg[1] = mvme_read_value(mvme, base+VPC6_CFG_RW+(0x10*(port-1))+0x04);
  reg[2] = mvme_read_value(mvme, base+VPC6_CFG_RW+(0x10*(port-1))+0x08);
  reg[3] = mvme_read_value(mvme, base+VPC6_CFG_RW+(0x10*(port-1))+0x0C);
  vpc6_PortDisplay(type, port, reg);

  //printf("DEBUG Configuration Register:\n reg[0]=0x%8lx \n reg[1]=0x%8lx \n reg[2]=0x%8lx \n reg[3]=0x%8lx \n"
  // , reg[0], reg[1], reg[2], reg[3]);

  mvme_set_dmode(mvme, cmode);
  return VPC6_SUCCESS;
}

/*****************************************************************/
/**
 * decoded printout of readout entry
 */
void vpc6_PortDisplay(WORD type, WORD port, DWORD * reg)
{
  printf("\n---- Port:%d Type:%s ----------------------------\n"
   , port, type == VPC6_ASD01 ? "ASD01" : "Buckeye");
  vpc6_EntryPrint(type, 0, (vpc6_Reg *)&reg[0]);
  if (type == VPC6_ASD01) {
    vpc6_EntryPrint(type, 1, (vpc6_Reg *)&reg[2]);
  }
}

/*****************************************************************/
/**
 * decoded printout of readout entry for ASD type preamp
 * Not to be trusted for data decoding but acceptable for display
 * purpose as its implementation is strongly compiler dependent and
 * not flawless.
 * @param v
 */
void vpc6_EntryPrint(WORD type, WORD chip, const vpc6_Reg* v)
{
  vpc6_Reg * v2 = v+1;


  if (type == VPC6_ASD01) {
    switch (chip) {
    case vpc6_asd_ch1_8:
      printf("Channel 1 to 8...\n");
      printf("Chip mode    :%s ", v->asdx1.chipmode == 0 ? "ADC" : "TOT");
      printf("     Channel mode :0x%04x \n", v->asdx1.channelmode);
      printf("Threshold    :0x%02x     Hysteresis   :0x%02x     CalInjCap    :0x%02x     CalMaskReg   :0x%02x\n"
       , v2->asdx2.mainThresholdDAC, v->asdx1.hysteresisDAC
       , v2->asdx2.capInjCapSel, v2->asdx2.calMaskReg);
      if (v->asdx1.chipmode == 0) {
  printf("ADC Threshold:0x%02x     Dead Time    :0x%02x WADC Run Current :0x%02x     WADC Int Gate:0x%02x\n"
         , ((v2->asdx2.wilkinsonADC << 1) | v ->asdx1.wilkinsonADC), v->asdx1.deadtime
         , v->asdx1.wilkinsonRCurrent, v->asdx1.wilkinsonIGate);
      }
      break;
    case vpc6_asd_ch9_16:
      printf("Channel 9 to 16...\n");
      printf("Chip mode    :%s ", v->asdx1.chipmode == 0 ? "ADC" : "TOT");
      printf("     Channel mode :0x%04x \n", v->asdx1.channelmode);
      printf("Threshold    :0x%02x     Hysteresis   :0x%02x     CalInjCap    :0x%02x     CalMaskReg   :0x%02x\n"
       , v2->asdx2.mainThresholdDAC, v->asdx1.hysteresisDAC
       , v2->asdx2.capInjCapSel, v2->asdx2.calMaskReg);
      if (v->asdx1.chipmode == 0) {
  printf("ADC Threshold:0x%02x     Dead Time    :0x%02x WADC Run Current :0x%02x     WADC Int Gate:0x%02x\n"
         , ((v2->asdx2.wilkinsonADC << 1) | v ->asdx1.wilkinsonADC), v->asdx1.deadtime
         , v->asdx1.wilkinsonRCurrent, v->asdx1.wilkinsonIGate);
      }
      break;
    }
  }
  else if (type == VPC6_BUCKEYE) {
    printf("Channel:  1 to 16...\n");
  }
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
int  vpc6_Setup(MVME_INTERFACE *mvme, DWORD base, int mode)
{
  int  cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  switch (mode) {
  case 0x1:
    printf("Default setting after power up (mode:%d)\n", mode);
    printf("All Buckeye\n");
    vpc6_PATypeWrite(mvme, base, VPC6_ALL_BUCKEYE);
    break;
  case 0x2:
    printf("Modified setting (mode:%d)\n", mode);
    printf("All ASD01\n");
    vpc6_PATypeWrite(mvme, base, VPC6_ALL_ASD);
    break;
  case 0x3:
    printf("Modified setting (mode:%d)\n", mode);
    printf("3 ASD and 3 Buckeye\n");
    vpc6_PATypeWrite(mvme, base, VPC6_3ASD_3BUCK);
    break;
  case 0x4:
    printf("Modified setting (mode:%d)\n", mode);
    printf("All Buckeye\n");
    vpc6_PATypeWrite(mvme, base, VPC6_3BUCK_3ASD);
    break;
  default:
    printf("Unknown setup mode\n");
    mvme_set_dmode(mvme, cmode);
    return -1;
  }
  mvme_set_dmode(mvme, cmode);
  return 0;
}

/*****************************************************************/
void  vpc6_Status(MVME_INTERFACE *mvme, DWORD base, WORD port)
{
  int status, cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  printf("vpc6 Status for Base:0x%lx\n", base);
  status = vpc6_isPortBusy(mvme, base, port);
  printf("Port Busy Status: 0x%x\n", status);
  status = vpc6_PortTypeRead(mvme, base, port);
  printf("PA type: 0x%x\n", status);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void  vpc6_ASDDefaultLoad(MVME_INTERFACE *mvme, DWORD base, WORD port)
{
  vpc6_Reg def;

  def.asdx1.chipmode          = 1; // ToT
  def.asdx1.channelmode       = 0; // All on
  def.asdx1.deadtime          = 0; // 300ns
  def.asdx1.wilkinsonRCurrent = 0; // N/A
  def.asdx1.wilkinsonIGate    = 0; // N/A
  def.asdx1.hysteresisDAC     = 7; // ~10mV hysteresis
  def.asdx1.wilkinsonADC      = 0; // bit0 here  N/A
  vpc6_ASDRegSet(mvme, base, port, 0, &def);

  def.asdx2.wilkinsonADC      = 0; // bits2..1 here  N/A
  def.asdx2.mainThresholdDAC  = 108; // N/A
  def.asdx2.capInjCapSel      = 0; // N/A
  def.asdx2.calMaskReg        = 0; // N/A
  def.asdx2.notused           = 0;
  vpc6_ASDRegSet(mvme, base, port, 1, &def);

  def.asdx1.chipmode          = 1; // ToT
  def.asdx1.channelmode       = 0;
  def.asdx1.deadtime          = 0;
  def.asdx1.wilkinsonRCurrent = 0;
  def.asdx1.wilkinsonIGate    = 0;
  def.asdx1.hysteresisDAC     = 7;
  def.asdx1.wilkinsonADC      = 0; // bit0 here  N/A
  vpc6_ASDRegSet(mvme, base, port, 2, &def);

  def.asdx2.wilkinsonADC      = 0; // bits2..1 here  N/A
  def.asdx2.mainThresholdDAC  = 108;
  def.asdx2.capInjCapSel      = 0;
  def.asdx2.calMaskReg        = 0;
  def.asdx2.notused           = 0;
  vpc6_ASDRegSet(mvme, base, port, 3, &def);
}

/*****************************************************************/
void  vpc6_ASDRegSet(MVME_INTERFACE *mvme, DWORD base, WORD port
         , WORD reg, vpc6_Reg * Reg)
{
  int  cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  mvme_write_value(mvme, base+VPC6_CFG_RW+((port-1)*0x10)+(reg*0x04), Reg->asdcfg);
  //DEBUG
  //printf("port:%d reg:%d addr: 0x%x\n",port, reg, VPC6_CFG_RW+((port-1)*0x10)+(reg*0x04));
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
int vpc6_ASDModeSet(MVME_INTERFACE *mvme, DWORD base, WORD port, int channel,  int mode)
{
  int  cmode;
  int ch;
  vpc6_Reg reg[2];

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  reg[0] = (vpc6_Reg) mvme_read_value(mvme, base+VPC6_CFG_RW+(0x10*(port-1))+0x00);
  reg[1] = (vpc6_Reg) mvme_read_value(mvme, base+VPC6_CFG_RW+(0x10*(port-1))+0x08);

  if((mode >= 0) && (mode < 4)) {
    if (channel == ALL_CHANNELS) {
      reg[0].asdx1.channelmode = 0x0000;
      reg[1].asdx1.channelmode = 0x0000;
      for (ch=0; ch<8; ch++) {
  reg[0].asdx1.channelmode |= mode << ch*2;
      }
      for (ch=0; ch<8; ch++) {
  reg[1].asdx1.channelmode |= mode << ch*2;
      }
    }
    else if ((channel >= 0) && (channel < 8)) {
      reg[0].asdx1.channelmode &= (~0x0003) << channel*2;
      reg[0].asdx1.channelmode |= mode << channel*2;
    }
    else if ((channel >= 8) && (channel < 16)) {
      reg[1].asdx1.channelmode &= (~0x0003) << (channel-8)*2;
      reg[1].asdx1.channelmode |= mode << channel*2;
    }
    else {
      printf("Invalid Channel # %d...\n", channel);
      printf("Channels 0..15  (%d for ALL)\n", ALL_CHANNELS);
      mvme_set_dmode(mvme, cmode);
      return -1;
    }
  }
  else {
    printf("Invalid ASD Channel Mode Setting... %d\n", mode);
    printf("Modes:  0 (ON), 1 (ON), 2 (Forced LOW), 3 (Forced HIGH)\n");
    mvme_set_dmode(mvme, cmode);
    return -1;
  }

  vpc6_ASDRegSet(mvme, base, port, 0, &reg[0]);
  vpc6_ASDRegSet(mvme, base, port, 2, &reg[1]);
  vpc6_PortCfgLoad(mvme, base, port);
  vpc6_CfgRetrieve(mvme, base, port);
  mvme_set_dmode(mvme, cmode);
  return 0;
}


/*****************************************************************/
int vpc6_ASDThresholdSet(MVME_INTERFACE *mvme, DWORD base, WORD port, int value)
{
  int  cmode;
  vpc6_Reg reg[2];

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  reg[0] = (vpc6_Reg) mvme_read_value(mvme, base+VPC6_CFG_RW+(0x10*(port-1))+0x04);
  reg[1] = (vpc6_Reg) mvme_read_value(mvme, base+VPC6_CFG_RW+(0x10*(port-1))+0x0C);

  // Threshold Value in mV (~9mV / fC)
  if ((value >= -256) && (value <= 254)) {
    reg[0].asdx2.mainThresholdDAC = (value / 2) + 128;
    reg[1].asdx2.mainThresholdDAC = (value / 2) + 128;
  }
  else {
    printf("Invalid Threshold Setting %d...\n", value);
    printf("Threshold = -256 .. 254 (in mV's)\n");
    mvme_set_dmode(mvme, cmode);
    return -1;
  }

  vpc6_ASDRegSet(mvme, base, port, 1, &reg[0]);
  vpc6_ASDRegSet(mvme, base, port, 3, &reg[1]);

  vpc6_PortCfgLoad(mvme, base, port);
  vpc6_CfgRetrieve(mvme, base, port);

  mvme_set_dmode(mvme, cmode);

  return 0;
}


/*****************************************************************/
int vpc6_ASDHysteresisSet(MVME_INTERFACE *mvme, DWORD base, WORD port, float value)
{
  int  cmode;
  int ivalue;
  vpc6_Reg reg[2];

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  reg[0] = (vpc6_Reg) mvme_read_value(mvme, base+VPC6_CFG_RW+(0x10*(port-1))+0x00);
  reg[1] = (vpc6_Reg) mvme_read_value(mvme, base+VPC6_CFG_RW+(0x10*(port-1))+0x08);

  // Hysteresis Value
  ivalue = (int) (value / 1.25);
  if ((ivalue >= 0x0) && (ivalue <= 0xF)) {
    reg[0].asdx1.hysteresisDAC = ivalue;
    reg[1].asdx1.hysteresisDAC = ivalue;
  }
  else {
    printf("Invalid Hysteresis Setting %f...\n", value);
    printf("Hysteresis = 0x0 .. 0xF (ie. 0 - 18.75mV)\n");
    mvme_set_dmode(mvme, cmode);
    return -1;
  }

  vpc6_ASDRegSet(mvme, base, port, 0, &reg[0]);
  vpc6_ASDRegSet(mvme, base, port, 2, &reg[1]);
  vpc6_PortCfgLoad(mvme, base, port);
  vpc6_CfgRetrieve(mvme, base, port);
  mvme_set_dmode(mvme, cmode);

  return 0;
}


/*****************************************************************/
int vpc6_BuckeyeModeSet(MVME_INTERFACE *mvme, DWORD base, WORD port, int channel,  int mode)
{

  return 0;
}







/*****************************************************************/
/*-PAA- For test purpose only */
#ifdef MAIN_ENABLE
int main (int argc, char* argv[]) {
  int i;
  int status;
  int argindex = 1;
  DWORD VPC6_BASE  = 0x300000;
  WORD port = 0;
  MVME_INTERFACE *myvme;
  int channel;
  int value;
  float fvalue;


  // Test under vmic
  status = mvme_open(&myvme, 0);

  // Set am to A24 non-privileged Data
  mvme_set_am(myvme, MVME_AM_A24_ND);

  // Set dmode to D32
  mvme_set_dmode(myvme, MVME_DMODE_D32);


  // Set VPC6 Port Modes (3 ASD / 3 Buckeye)
  vpc6_Setup(myvme, VPC6_BASE, 3);

  // Load Default Configurations
  for (i=1;i<7;i++) {
    vpc6_ASDDefaultLoad(myvme, VPC6_BASE, i);
    vpc6_PortCfgLoad(myvme, VPC6_BASE, i);
    vpc6_CfgRetrieve(myvme, VPC6_BASE, i);
  }

  printf("\n\n");


  if (argc > 1) {

    while (argindex < argc) {
      if ((strncmp(argv[argindex],"-help",2) == 0) && (argc == 2)) {
  printf("vpc6 command line usage...\n");
  printf("vpc6 -help -baseaddr [a] -port [p] -threshold [t] -hysteresis [h]\n");
  printf("     -channelmode [c] [m] -readback\n");
  printf("[a]: base address of vpc6 module (eg. 0x300000)\n");
  printf("[p]: port number (1..6; 0 for all ports!!! '0' NOT IMPLEMENTED IN FIRMWARE YET)\n");
  printf("[t]: theshold setting (-256..254 [mV])\n");
  printf("[h]: hysteresis setting (0x0..0xF [0-18.75mV])\n");
  printf("[c]: channel number (0..15; -1 for all channels)\n");
  printf("[m]: channel mode (0,1=On; 2=Forced Low; 3=Forced High)\n");
      }
      else if (strncmp(argv[argindex],"-baseaddr",2) == 0) {
  if ((argindex+1) < argc) {
    argindex = argindex + 1;
    sscanf(argv[argindex],"%lx",&VPC6_BASE);
    printf("Base Set... 0x%lx\n", VPC6_BASE);
  }
  else {
    printf("Invalid number of arguments for %s modifier\n",argv[argindex]);
    break;
  }
      }
      else if (strncmp(argv[argindex],"-port",2) == 0) {
  if ((argindex+1) < argc) {
    argindex = argindex + 1;
    sscanf(argv[argindex],"%d",&port);
    printf("Port Set... %d\n", port);
  }
  else {
    printf("Invalid number of arguments for %s modifier\n",argv[argindex]);
    break;
  }
      }
      else if (strncmp(argv[argindex],"-threshold",2) == 0) {
  if ((argindex+1) < argc) {
    argindex = argindex + 1;
    sscanf(argv[argindex],"%d",&value);
    vpc6_ASDThresholdSet(myvme, VPC6_BASE, port, value);
    printf("Threshold Set... %4.0d mV\n", value);
  }
  else {
    printf("Invalid number of arguments for %s modifier\n",argv[argindex]);
    break;
  }
      }
      else if (strncmp(argv[argindex],"-hysteresis",3) == 0) {
  if ((argindex+1) < argc) {
    argindex = argindex + 1;
    sscanf(argv[argindex],"%f",&fvalue);
    vpc6_ASDHysteresisSet(myvme, VPC6_BASE, port, fvalue);
    printf("Hysteresis Set... %4.2f\n", fvalue);
  }
  else {
    printf("Invalid number of arguments for %s modifier\n",argv[argindex]);
    break;
  }
      }
      else if (strncmp(argv[argindex],"-channelmode",2) == 0) {
  if ((argindex+2) < argc) {
    argindex = argindex + 1;
    sscanf(argv[argindex],"%d",&channel);
    argindex = argindex + 1;
    sscanf(argv[argindex],"%d",&value);
    vpc6_ASDModeSet(myvme, VPC6_BASE, port, channel, value);
    if (channel == -1) {
      printf("ALL Channels Set... %d\n", value);
    }
    else {
      printf("Channel %d Set... %d\n", channel, value);
    }
  }
  else {
    printf("Invalid number of arguments for %s modifier\n",argv[argindex]);
    break;
  }
  printf("Channel Mode Set...\n");
      }

      else if (strncmp(argv[argindex],"-readback",2) == 0) {
  vpc6_PortRegRBRead(myvme, VPC6_BASE, 1);
      }
      else {
  printf("Invalid Argument... %s\n", argv[argindex]);
  printf("'vpc6 -help' for command line usage...\n");
  break;
     }
      argindex++;
    }

  }




  //  vpc6_PortRegRead(myvme, VPC6_BASE, 1);
  //  vpc6_PortRegRBRead(myvme, VPC6_BASE, 1);
  printf("\n\n");








  /*** FOR TESTING ***/
  if(0) {

  vpc6_Setup(myvme, VPC6_BASE, 2);

  // for (i=1;i<7;i++) {
  for (i=1;i<2;i++) {
    //vpc6_Status(myvme, VPC6_BASE, i);
    //    vpc6_ASDDefaultLoad(myvme, VPC6_BASE, i);
    //    vpc6_PortCfgLoad(myvme, VPC6_BASE, i);
    //    vpc6_CfgRetrieve(myvme, VPC6_BASE, i);
    vpc6_PortRegRead(myvme, VPC6_BASE, i);
    vpc6_PortRegRBRead(myvme, VPC6_BASE, i);
  }

  }
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
