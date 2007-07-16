/*********************************************************************
  @file
  Name:         vt48.c
  Created by:   Pierre-Andre Amaudruz, Chris Ohlmann

  Contents:      Routines for accessing the VT48 Triumf board

gcc -g -O2 -Wall -g -DMAIN_ENABLE -I/home1/midas/midas/include 
    -o vt48 vt48.c vmicvme.o -lm -lz -lutil -lnsl -lpthread -L/lib
    -lvme  
  
$Id$
*********************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vt48.h"

/********************************************************************/
/**
Read one Event
@param myvme vme structure
@param base  TF48 base address
@param pdest Destination pointer
@param entry return number of entry
@return void
*/
int vt48_EventRead(MVME_INTERFACE *myvme, DWORD base, DWORD *pdest, int *nentry)
{
  int cmode, timeout, i, nwords, bfull, bempty;
  DWORD hdata = 0;
    
  mvme_get_dmode(myvme, &cmode);
  mvme_set_dmode(myvme, MVME_DMODE_D32);

  /*
  *nentry = mvme_read_value(myvme, base+VT48_OCCUPANCY_RO);
  printf ("nentry:%x\n", *nentry);

  *nentry &= 0xFFF;
  for (i=0 ; i<*nentry ; i++) {
    pdest[i] = mvme_read_value(myvme, base+VT48_DATA_FIFO);
  }
  */

  /*
  nwords = mvme_read_value(myvme, base+VT48_OCCUPANCY_RO);
  bempty = nwords & 0x4000;
  bfull = nwords & 0x8000;
  nwords &= 0xFFF;  
  */
  
  *nentry = 0;
  
  timeout=300;  
  do {
    timeout--;
    if (!(mvme_read_value(myvme, base+VT48_OCCUPANCY_RO) & 0x4000)) {   // Buffer Not Empty
      hdata = mvme_read_value(myvme, base+VT48_DATA_FIFO);
    }
  } while (!((hdata & 0xF0000000) == VT48_HEADER) && (timeout)); 
    
  if (timeout == 0) {
    *nentry = 0;
    //printf("timeout on header  data:0x%lx\n", hdata);
    mvme_set_dmode(myvme, cmode);
    return VT48_ERR_NODATA;
  }

  pdest[*nentry] = hdata;
  *nentry += 1;
  timeout=2000;
  do {
    timeout--;
    if (!(mvme_read_value(myvme, base+VT48_OCCUPANCY_RO) & 0x4000)) {   // Buffer Not Empty
      pdest[*nentry] = mvme_read_value(myvme, base+VT48_DATA_FIFO);
      *nentry += 1;
      timeout=2000;
    }
  } while (!((pdest[*nentry-1] & 0xF0000000) == VT48_TRAILER) && timeout); 

  if (timeout == 0) {
    printf("timeout on Trailer  data:0x%x\n", pdest[*nentry-1]);
    printf("nentry:%d data:0x%x base:0x%x \n", *nentry, pdest[*nentry], base+VT48_DATA_FIFO);
  }
  *nentry--;
    
  mvme_set_dmode(myvme, cmode);
  return (VT48_SUCCESS);
}

/*****************************************************************/
/**
 */
void vt48_RegWrite(MVME_INTERFACE *mvme, DWORD base, DWORD reg, DWORD data)
{
  int cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  mvme_write_value(mvme, base+reg, data);
  mvme_set_dmode(mvme, cmode);
  return;
}

/**
 */
void vt48_WindowSet(MVME_INTERFACE *mvme, DWORD base, float window)
{
  int cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  printf("Window() Not implemented yet\n");
  mvme_set_dmode(mvme, cmode);
  return;
}

/**
 */
void vt48_WindowOffsetSet(MVME_INTERFACE *mvme, DWORD base, float offset)
{
  int cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  printf("WindowOffset() Not implemented yet\n");
  mvme_set_dmode(mvme, cmode);
  return;
}

/*****************************************************************/
/**
 */
DWORD vt48_RegRead(MVME_INTERFACE *mvme, DWORD base, WORD reg)
{
  int cmode;
  DWORD value;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  value = mvme_read_value(mvme, base+reg);
  mvme_set_dmode(mvme, cmode);
  return value;
}

/*****************************************************************/
/**
 */
void vt48_RegPrint(MVME_INTERFACE *mvme, DWORD base)
{
  int cmode, i, j;
  DWORD val1;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  printf("vt48 Register\n");
  vt48_RegWrite(mvme, base, VT48_CMD_REG, VT48_AMT_CFG_RW);
  usleep(10000);
  
  for (i=0, j=0;i<15;i++,j+=4) {
    val1 = vt48_RegRead(mvme, base, VT48_CSR0_REG+j);
    printf("AMT Register CSR%2.2d: 0x%x \n", i, val1);
  }
  mvme_set_dmode(mvme, cmode);
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
int  vt48_Setup(MVME_INTERFACE *mvme, DWORD base, int mode)
{
  int  cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);

  switch (mode) {
  case 0x1:
    printf("Default setting after power up (mode:%d)\n", mode);
    printf("Same configuration in both AMTs... Ch0-47 enabled\n");
    printf("Time relative to trigger, Trigger matching enabled\n");

    vt48_RegWrite(mvme, base, VT48_CMD_REG, 0x10);  // AMT Global Reset

    vt48_RegWrite(mvme, base, VT48_CSR0_REG, 0xf0200020); // enable direct (Trigger/Reset)
    vt48_RegWrite(mvme, base, VT48_CSR1_REG, 0x0000);     // mask window
    vt48_RegWrite(mvme, base, VT48_CSR2_REG, 0x0080);     // search window (# of 20ns clock)
    vt48_RegWrite(mvme, base, VT48_CSR3_REG, 0x0070);     // match window ( same)
    vt48_RegWrite(mvme, base, VT48_CSR4_REG, 0x0f80);     // reject offset (same) 
    vt48_RegWrite(mvme, base, VT48_CSR5_REG, 0x0000);     // event offset (same)
    vt48_RegWrite(mvme, base, VT48_CSR6_REG, 0x0fa0);     // bunch offset (same)
    vt48_RegWrite(mvme, base, VT48_CSR7_REG, 0x0000);     // coarse time offset (same)
    vt48_RegWrite(mvme, base, VT48_CSR8_REG, 0x0fff);     // counter roll over (same)
    vt48_RegWrite(mvme, base, VT48_CSR9_REG, 0xf3000301); // TDCs Id 0,1... (should remain fixed)
    vt48_RegWrite(mvme, base, VT48_CSR10_REG, 0x0af1);    // Different mask enable (trig match, relative)
    //vt48_RegWrite(mvme, base, VT48_CSR10_REG, 0x0871);  //                       (disable trigger match)
    //vt48_RegWrite(mvme, base, VT48_CSR10_REG, 0x0a71);  //                        disable relative)
    vt48_RegWrite(mvme, base, VT48_CSR11_REG, 0x0e11);    // More mask enable (overflows ... see man)
    vt48_RegWrite(mvme, base, VT48_CSR12_REG, 0x01ff);    // Enable Error report
    vt48_RegWrite(mvme, base, VT48_CSR13_REG, 0x0fff);    // Channel 0..11 enable (TDC1 all on, TDC2 - all on)
    vt48_RegWrite(mvme, base, VT48_CSR14_REG, 0x0fff);    // Channel 12..23 enable (TDC1 all on, TDC2 - all on)

    break;

  case 0x2:
    printf("Default setting after power up (mode:%d)\n", mode);
    printf("Same configuration in both AMTs... Ch0-23 (TDC ID=1), Ch24-47 (disable)\n");

    vt48_RegWrite(mvme, base, VT48_CMD_REG, 0x10);  // AMT Global Reset

    vt48_RegWrite(mvme, base, VT48_CSR0_REG, 0xf0200020); // enable direct (Trigger/Reset)
    vt48_RegWrite(mvme, base, VT48_CSR1_REG, 0x0000);     // mask window
    vt48_RegWrite(mvme, base, VT48_CSR2_REG, 0x0080);     // search window (# of 20ns clock)
    vt48_RegWrite(mvme, base, VT48_CSR3_REG, 0x0070);     // match window ( same)
    vt48_RegWrite(mvme, base, VT48_CSR4_REG, 0x0f80);     // reject offset (same) 
    vt48_RegWrite(mvme, base, VT48_CSR5_REG, 0x0000);     // event offset (same)
    vt48_RegWrite(mvme, base, VT48_CSR6_REG, 0x0fa0);     // bunch offset (same)
    vt48_RegWrite(mvme, base, VT48_CSR7_REG, 0x0000);     // coarse time offset (same)
    vt48_RegWrite(mvme, base, VT48_CSR8_REG, 0x0fff);     // counter roll over (same)
    vt48_RegWrite(mvme, base, VT48_CSR9_REG, 0xf3020301); // TDCs id ... (should remain fixed)
    vt48_RegWrite(mvme, base, VT48_CSR10_REG, 0x0af1);    // Different mask enable (trig match, relative)
    //vt48_RegWrite(mvme, base, VT48_CSR10_REG, 0x0871);  //                       (disable trigger match)
    //vt48_RegWrite(mvme, base, VT48_CSR10_REG, 0x0a71);  //                        disable relative)
    vt48_RegWrite(mvme, base, VT48_CSR11_REG, 0x0e11);    // More mask enable (overflows ... see man)
    vt48_RegWrite(mvme, base, VT48_CSR12_REG, 0x01ff);    // Enable Error report
    vt48_RegWrite(mvme, base, VT48_CSR13_REG, 0xf0000fff);// Channel 0..11 enable (TDC1 all on, TDC2 - all off)
    vt48_RegWrite(mvme, base, VT48_CSR14_REG, 0xf0000fff);// Channel 12..23 enable (TDC1 all on, TDC2 - all off)

    break;

  case 0x3:
    printf("Modified setting (mode:%d)\n", mode);
    printf("... nothing for now\n");

    break;
  case 0x11:
    printf("Modified setting (mode:%d)\n", mode);
    printf("... TRIGGER SENT!!\n");
    vt48_RegWrite(mvme, base, VT48_CMD_REG, 0xf);
    break;
  default:
    printf("Unknown setup mode\n");
    mvme_set_dmode(mvme, cmode);
    return -1;
  }

  // Configure AMTs
  vt48_RegWrite(mvme, base, VT48_CMD_REG, VT48_AMT_CFG_RW); // Write to the AMT 
  usleep(200);
  vt48_RegWrite(mvme, base, VT48_CMD_REG, VT48_AMT_CFG_RW); // Force update of Read-back Reg (CSR0..14) 
  usleep(200);
  
  // AMT Bunch Reset
  vt48_RegWrite(mvme, base, VT48_CMD_REG, 0x12);
  usleep(1);
  
  //  vt48_Status(mvme, base);
  mvme_set_dmode(mvme, cmode);
  return 0;
}

/*****************************************************************/
void  vt48_Status(MVME_INTERFACE *mvme, DWORD base)
{
  int cmode, i;
  DWORD val1, val2;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  printf("----- VT48 Status -- Base:0x%x -----\n", base);
  val1 = vt48_RegRead(mvme, base, VT48_CSR_RO);
  printf("CSR (Occupancy) : 0x%x\n", val1);
  vt48_RegWrite(mvme, base, VT48_CMD_REG, VT48_AMT_ID_R);
  usleep(100000);
  val1 = vt48_RegRead(mvme, base, VT48_ID1_REG_RO);
  val2 = vt48_RegRead(mvme, base, VT48_ID2_REG_RO);
  printf("AMTs ID         : 0x%x  - 0x%x\n", val1, val2);
  vt48_RegWrite(mvme, base, VT48_CMD_REG, VT48_AMT_STATUS_R);
  usleep(100000);

  // Read AMT3 Status Reg
  for (i=0;i<6;i++) {
    val1 = vt48_RegRead(mvme, base, VT48_CSR16_REG+i*4);
    printf("AMT3 CSR%2d     : 0x%8.8x\n", 16+i, val1);
  }

  // Read Control Registers
  for (i=0;i<15;i++) {
    val1 = vt48_RegRead(mvme, base, VT48_CSR0_REG+i*4);
    printf("AMT3 CTL%2d     : 0x%8.8x\n", i, val1);
  }

  // Read Read-back Reg from last configuration
  for (i=0;i<15;i++) {
    val1 = vt48_RegRead(mvme, base, VT48_CSR0_RB_REG+i*4);
    printf("AMT3 CSR-RB%2d  : 0x%8.8x\n", i, val1);
  }
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
/*-PAA- For test purpose only */
#ifdef MAIN_ENABLE
int main (int argc, char* argv[]) {
  int i;
  DWORD VT48_BASE  = 0x300000;
  MVME_INTERFACE *myvme;

  int status, mode;
  WORD reg;
  DWORD value;


  if ((argc == 2) && (strncmp(argv[1],"-h",2))==0) {

    printf("vt48 interactive:\n");
    printf("                   -h : help\n");
    printf("                  <mode> : 0:status, 1..9: setup, 77,78:test\n");
    printf("                  w <reg> <value>: Write all in hex\n");
    printf("                  r <reg>        : Read in hex\n");
  }
    

  // Test under vmic
  status = mvme_open(&myvme, 0);

  // Set am to A24 non-privileged Data
  mvme_set_am(myvme, MVME_AM_A24_ND);

  // Set dmode to D32
  mvme_set_dmode(myvme, MVME_DMODE_D32);

  /* parse command line parameters */
  switch (argc-1) {
  case 0:   // no args
    vt48_RegPrint(myvme, VT48_BASE);
    vt48_Setup(myvme, VT48_BASE, 1);
    vt48_RegPrint(myvme, VT48_BASE);
    break;
  case 1:   // 1 arg mode
    mode = atoi(argv[1]);
    switch (mode) {
    case 0:
      vt48_Status(myvme, VT48_BASE);
      break;
    case 77:
      vt48_RegWrite(myvme, VT48_BASE, VT48_CSR0_REG, 0xabcdef);
      while (1) {
	// vt48_RegWrite(myvme, VT48_BASE, VT48_CSR0_REG, 0xabcdef);
	i = vt48_RegRead(myvme, VT48_BASE, VT48_CSR0_REG);
	//    printf("0x%x\n", i);
	// usleep(1);
      }
      break;
    case 78:
      vt48_RegWrite(myvme, VT48_BASE, 0x1000, 0xabcdef);
      while (1) {
	// vt48_RegWrite(myvme, VT48_BASE, 0x1000, 0xabcdef);
	i = vt48_RegRead(myvme, VT48_BASE, 0x1000);
	//    printf("0x%x\n", i);
	// usleep(1);
      }
      break;
    default:
      vt48_Setup(myvme, VT48_BASE, mode);
      break;
    }
    break;
  case 2:   // 2 arg read 
    if ((argc == 3) && (strncmp(argv[1],"r",2))==0) {
      reg = strtoul(argv[2], NULL, 16);
      // Configure AMTs
      vt48_RegWrite(myvme, VT48_BASE, VT48_CMD_REG, VT48_AMT_CFG_RW);
      value = vt48_RegRead(myvme, VT48_BASE, reg*4 + VT48_CSR0_REG);
      printf("Read  : reg:0x%x  ->  val:0x%x\n", reg, value);
    }
    break;
  case 3:   // 3 arg write 
    if ((argc == 4) && (strncmp(argv[1],"w",2))==0) {
      reg = strtoul(argv[2], NULL, 16);
      value = strtoul(argv[3], NULL, 16);
      printf("Write : reg:0x%x  <-  val:0x%x\n", reg, value);
      vt48_RegWrite(myvme, VT48_BASE, reg, value);
      // Configure AMTs
      vt48_RegWrite(myvme, VT48_BASE, VT48_CMD_REG, VT48_AMT_CFG_RW);
    }
    break;
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
