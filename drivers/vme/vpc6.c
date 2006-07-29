/*********************************************************************
  @file
  Name:         vpc6.c
  Created by:   Pierre-Andre Amaudruz

  Contents:      Routines for accessing the vpc6 Triumf board

  $Log: vpc6.c,v $
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
    action = (0x10) | port;
  }
  else
    return VPC6_PARAM_ERROR;
  
  do {
    if (~(vpc6_isPortBusy(mvme, base, port))) {
      // Not busy, load bit string to PA
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
 * the port is either ASD01 or Buckeye for now.
 */
int vpc6_CfgRetrieve(MVME_INTERFACE *mvme, DWORD base, WORD port)
{
  int cmode, action, timeout=100;
  
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  if ((port > 0) && (port < 7)) {
    action = (0x00) | port;
  }
  
  do {
    if (~(vpc6_isPortBusy(mvme, base, port))) {
      // Not busy, load bit string to PA
      //      printf(" cfg request 0x%x %d\n", base+VPC6_CMD_WO, action);
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
  int cmode, action, timeout=100;
  WORD type;
  DWORD reg[4];
  
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  if ((port > 0) && (port < 7)) {
    action = (0x00) | port;
  }
  
  do {
    if (~(vpc6_isPortBusy(mvme, base, port))) {
      type  = mvme_read_value(mvme, base+VPC6_CR_RW);
      type  = ((type >> (2*(port-1))) & 0x03);   // ASD or Buck
      reg[0] = mvme_read_value(mvme, base+VPC6_RBCK_RO+(0x10*port)+0x00);
      reg[1] = mvme_read_value(mvme, base+VPC6_RBCK_RO+(0x10*port)+0x04);
      reg[2] = mvme_read_value(mvme, base+VPC6_RBCK_RO+(0x10*port)+0x08);
      reg[3] = mvme_read_value(mvme, base+VPC6_RBCK_RO+(0x10*port)+0x0C);
      vpc6_PortDisplay(type, port, reg);
      break;
    }
  } while (timeout--);
  
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
  reg[0] = mvme_read_value(mvme, base+VPC6_REG_RW+(0x10*(port-1))+0x00);
  reg[1] = mvme_read_value(mvme, base+VPC6_REG_RW+(0x10*(port-1))+0x04);
  reg[2] = mvme_read_value(mvme, base+VPC6_REG_RW+(0x10*(port-1))+0x08);
  reg[3] = mvme_read_value(mvme, base+VPC6_REG_RW+(0x10*(port-1))+0x0C);
  vpc6_PortDisplay(type, port, reg);

  mvme_set_dmode(mvme, cmode);
  return VPC6_SUCCESS;
}

/*****************************************************************/
/**
 * decoded printout of readout entry
 */
void vpc6_PortDisplay(WORD type, WORD port, DWORD * reg)
{
  printf("---- Port:%d Type:%s ----------------------------\n"
	 , port, type == VPC6_ASD01 ? "ASD01" : "Buckeye");
  vpc6_printEntry(type, 0, (vpc6_Reg *)&reg[0]);
  vpc6_printEntry(type, 1, (vpc6_Reg *)&reg[1]);
  vpc6_printEntry(type, 0, (vpc6_Reg *)&reg[2]);
  vpc6_printEntry(type, 1, (vpc6_Reg *)&reg[3]);
}

/*****************************************************************/
/**
 * decoded printout of readout entry
 * Not to be trusted for data decoding but acceptable for display
 * purpose as its implementation is strongly compiler dependent and
 * not flawless.
 * @param v
 */
void vpc6_printEntry(WORD type, WORD chip, const vpc6_Reg* v)
{
  if (type == VPC6_ASD01) {
    switch (chip) {
    case vpc6_typeasdx1:
      printf("Chip mode    :%s ", v->asdx1.chipmode == 0 ? "ADC" : "TOT");
      printf("     Channel mode :0x%04x \n", v->asdx1.channelmode);
      printf("Dead Time    :0x%02x WADC Run Current :0x%02x W ADC Int Gate:0x%02x Hysteresis:0x%02x\n"
	     , v->asdx1.deadtime, v->asdx1.wilkinsonRCurrent
	     , v->asdx1.wilkinsonIGate, v->asdx1.hysteresisDAC);
      break;
    case vpc6_typeasdx2:
      printf("W Threshold  :0x%02x Main Threshold   :0x%02x CalInjCap     :0x%02x CalMaskReg:0x%02x\n"
	     , v->asdx2.wilkinsonADC, v->asdx2.mainThresholdDAC
	     , v->asdx2.capInjCapSel, v->asdx2.calMaskReg);
      break;
    }
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
  
  def.asdx1.chipmode          = 0; //ADC
  def.asdx1.channelmode       = 0; // All on
  def.asdx1.deadtime          = 0; // 300ns
  def.asdx1.wilkinsonRCurrent = 0; // N/A
  def.asdx1.wilkinsonIGate    = 0; // N/A
  def.asdx1.hysteresisDAC     = 0; // N/A
  vpc6_ASDRegSet(mvme, base, port, 0, &def); 

  def.asdx2.wilkinsonADC      = 0; // N/A
  def.asdx2.mainThresholdDAC  = 0; // N/A
  def.asdx2.capInjCapSel      = 0; // N/A
  def.asdx2.calMaskReg        = 0; // N/A
  def.asdx2.notused           = 0; 
  vpc6_ASDRegSet(mvme, base, port, 1, &def); 

  def.asdx1.chipmode          = 0; //ADC
  def.asdx1.channelmode       = 0;
  def.asdx1.deadtime          = 0;
  def.asdx1.wilkinsonRCurrent = 0;
  def.asdx1.wilkinsonIGate    = 0;
  def.asdx1.hysteresisDAC     = 0;
  vpc6_ASDRegSet(mvme, base, port, 2, &def); 

  def.asdx2.wilkinsonADC      = 0;
  def.asdx2.mainThresholdDAC  = 0;
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
  
  mvme_write_value(mvme, base+VPC6_REG_RW+((port-1)*0x10)+(reg*0x04), Reg->asdcfg);
  printf("port:%d reg:%d addr: 0x%x\n",port, reg, VPC6_REG_RW+((port-1)*0x10)+(reg*0x04)); 
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
/*-PAA- For test purpose only */
#ifdef MAIN_ENABLE
int main (int argc, char* argv[]) {
  int i;
  DWORD VPC6_BASE  = 0xE00000;
  MVME_INTERFACE *myvme;

  int status;

  if (argc>1) {
    sscanf(argv[1],"%lx",&VPC6_BASE);
  }

  // Test under vmic
  status = mvme_open(&myvme, 0);

  // Set am to A24 non-privileged Data
  mvme_set_am(myvme, MVME_AM_A24_ND);

  // Set dmode to D32
  mvme_set_dmode(myvme, MVME_DMODE_D32);

  vpc6_Setup(myvme, VPC6_BASE, 2);

  for (i=1;i<7;i++) {
    vpc6_Status(myvme, VPC6_BASE, i);
    vpc6_ASDDefaultLoad(myvme, VPC6_BASE, i);
    vpc6_CfgRetrieve(myvme, VPC6_BASE, i);
    //vpc6_PortRegRead(myvme, VPC6_BASE, i);
    //vpc6_PortRegRBRead(myvme, VPC6_BASE, i);
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
