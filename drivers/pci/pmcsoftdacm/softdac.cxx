/*********************************************************************

  Name:         softdac.c
  Created by:   Konstantin Olchanski

  Contents:      Routines for accessing the AWG PMC-SOFTDAC-M board
                from ALPHI TECHNOLOGY CORPORATION 
                
 * See manual at
 * http://www.alphitech.com
 * http://www.alphitech.com/doc/???.pdf
 *

  $Id$

- Restored 0x2 immediate mode 
- correct range limit comments
- correct range settings
- Range setting still doesn't work

*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <sys/mman.h>
#include "softdac.h"

/********************************************************************/
/** 
Open channel to pmcda16 
@return int
*/
int softdac_Open(ALPHISOFTDAC ** al)
{

  // Book space
  *al = (ALPHISOFTDAC *) calloc(1, sizeof(ALPHISOFTDAC));
  if (al == NULL) {
    printf ("cannot calloc softdac structure\n");
    return -1;
  }

  FILE *fp = fopen("/dev/pmcsoftdacm","r+");
  if (fp==0)
    {
      fprintf(stderr,"Cannot open the PMC-SOFTDAC-M device, errno: %d (%s)\n",
	      errno,strerror(errno));
      return -errno;
    }

  int fd = fileno(fp);

  // Allocate address space to the device
  (*al)->regs = (char*)mmap(0,0x100000,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
  if ((*al)->regs == NULL)
    {
      printf("regs: 0x%p\n",(*al)->regs);
      fprintf(stderr,"Cannot mmap() PMC-SOFTDAC-M registers, errno: %d (%s)\n",errno,strerror(errno));
      return -errno;
    }

  fclose(fp);
  fp = NULL;

  // Set initial scaling coefficients
  softdac_ScaleSet(*al, SOFTDAC_RANGE_PM10V, 0., 0.);

  return 0;
}

/********************************************************************/
/** 
*/
void softdac_Close(ALPHISOFTDAC * al)
{
  if (al) { 
    if (al->regs) munmap(al->regs, 0x100000);
    free (al);
    al = NULL;
  }
}

/********************************************************************/
/** 
Set the alpha/beta coefficients for the Volt2Dac conversion.
Only one set of param for all the channels.
@param alpha conversion slope coeff
@param beta  conversion offset coeff
@return int
*/
int softdac_ScaleSet(ALPHISOFTDAC * al, int range, double alpha, double beta)
{
  switch (range) {
  case SOFTDAC_RANGE_P5V:
    al->alpha = 5.0/65535.;
    al->beta =  0.0;
    break;
  case SOFTDAC_RANGE_P10V:
    al->alpha = 10.0/65535.;
    al->beta =  0.0;
    break;
  case SOFTDAC_RANGE_PM5V:
    al->alpha = 10/65535.;
    al->beta =  -5.0;
    break;
  case SOFTDAC_RANGE_PM10V:
    al->alpha = 20.0/65535.;
    al->beta =  -10.0;
    break;
  case SOFTDAC_RANGE_PM2P5V:
    al->alpha = 5.0/65535;
    al->beta =  -2.5;
    break;
  case SOFTDAC_RANGE_M2P5P7P5V:
    al->alpha = 10/65535;
    al->beta =  -2.5;
    break;
  case SOFTDAC_SET_COEF:
    al->alpha = alpha;
    al->beta  = beta;
    break;
  }

  if (range != SOFTDAC_SET_COEF) {
    for (int i =0;i<8;i++) {
      SOFTDAC_D16(al->regs+SOFTDAC_CMD_REG)      =  0x10 | range;
      SOFTDAC_D16(al->regs+SOFTDAC_DAC0102+2*i)   =  0x0;
    }
    SOFTDAC_D16(al->regs+SOFTDAC_CMD_REG)      =  0x02;
  }

  al->range = range;
  return (0);
}

/********************************************************************/
/** 
Reset softdac
@param regs
@return int
*/
int softdac_Reset(ALPHISOFTDAC * al)
{
  for (int i=0; i<0x14; i+=4)
    SOFTDAC_D32(al->regs+SOFTDAC_OFFSET+i) = 0;
  SOFTDAC_D32(al->regs+SOFTDAC_OFFSET+0x14) = SOFTDAC_CLK_SAMPLE_RESET;
  SOFTDAC_D32(al->regs+SOFTDAC_OFFSET+0x16) = SOFTDAC_ADDRESS_RESET;
  SOFTDAC_D32(al->regs+SOFTDAC_OFFSET+0x18) = SOFTDAC_DACS_RESET;
  SOFTDAC_D32(al->regs+SOFTDAC_OFFSET+0x10) = 0xF0000000;
  return al->regs[SOFTDAC_OFFSET+0]; // flush posted PCI writes
}

/********************************************************************/
/** 
Get maximum number of samples
@return int
*/
int softdac_MaxSamplesGet()
{
  return 0x4000;
}

/********************************************************************/
/** 
Convert Volt to Dac value
@param  volt voltage to convert
@return int  dac value
*/
uint16_t softdac_Volt2Dac(ALPHISOFTDAC * al, double volt)
{
  
  switch (al->range) {
  case SOFTDAC_RANGE_P5V:
    if (volt < 0.0) {
      printf("Out of range -> Limited to range (%e < 0.0)\n", volt);
      volt = 0.0;
    } else if (volt > 5.0) {
      printf("Out of range -> Limited to range  (%e > 5.0)\n", volt);
      volt = 5.0;
    }
    break;
  case SOFTDAC_RANGE_P10V:
    if (volt < 0.0) {
      printf("Out of range -> Limited to range  (%e < 0.0)\n", volt);
      volt = 0.0;
    } else if (volt > 10.0) {
      printf("Out of range -> Limited to range  (%e > 10.0)\n", volt);
      volt = 10.0;
    }
    break;
  case SOFTDAC_RANGE_PM5V:
    if (volt < -5.0) {
      printf("Out of range -> Limited to range  (%e < -5.0)\n", volt);
      volt = -5.0;
    } else if (volt > 5.0) {
      printf("Out of range -> Limited to range  (%e > 5.0)\n", volt);
      volt = 5.0;
    }
    break;
  case SOFTDAC_RANGE_PM10V:
    if (volt < -10.0) {
      printf("Out of range -> Limited to range  (%e < -10.0)\n", volt);
      volt = -10.0;
    } else if (volt > 10.0) {
      printf("Out of range -> Limited to range  (%e > +10.0)\n", volt);
      volt = 10.0;
    }
    break;
  case SOFTDAC_RANGE_PM2P5V:
    if (volt < -2.5) {
      printf("Out of range -> Limited to range  (%e < -2.5)\n", volt);
      volt = -2.5;
    } else if (volt > 2.5) {
      printf("Out of range -> Limited to range  (%e > 2.5)\n", volt);
      volt = 2.5;
    }
    break;
  case SOFTDAC_RANGE_M2P5P7P5V:
    if (volt < -2.5) {
      printf("Out of range -> Limited to range  (%e < -2.5)\n", volt);
      volt = -2.5;
    } else if (volt > 7.5) {
      printf("Out of range -> Limited to range  (%e > +7.5)\n", volt);
      volt = 7.5;
    }
    break;
  }
  return (uint16_t) ((volt - al->beta) / al->alpha);
}

/********************************************************************/
/** 
Convert Dac to Volt value
@param  dac Dac value to convert
@return double volt value
*/
double softdac_Dac2Volt (ALPHISOFTDAC * al, uint16_t dac)
{
  return (double) (dac * al->alpha + al->beta);
}

/********************************************************************/
/** 
Get channel address for a particular bank
@param data
@param ibank
@param ichan
@return int
*/
static char* chanAddr(char* data, int ibank, int ichan, int offset)
{
  return data + ibank*0x40000 + ichan*0x4000 + 2*offset;
}

/********************************************************************/
/** 
Get channel address for a particular bank
@param data  pointer to the data area
@param ibank  offset to the bank 0 or 1
@param ichan  offset within the bank for channel 0..7
@param offset offset withing the channel for data storage.
@return int   byte address of the location.
*/
int softdac_BankActiveRead(ALPHISOFTDAC * al)
{
  int stat0 = SOFTDAC_D08(al->regs+SOFTDAC_CTL_STAT_0);
  stat0 &= 0x1;
  int stat1 = SOFTDAC_D08(al->regs+SOFTDAC_CTL_STAT_1);
  stat1 &= 0x1;
  return (stat0 | (stat1<<1));
}

/********************************************************************/
/** 
Write to DACx directly for mode 2
@param data  AWG data set
@param numSamples number of samples
@param ibuf internal buffer to fill (0 or 1)
@param ichan channel to fill (0..7)
@param *arg pointer to the register control 
@return int
*/
#if 0
int softdac_DacWrite(ALPHISOFTDAC * al, unsigned short *data, int * chlist, int nch)
{
  int16_t dummy = 0;

  for (int i=0; i<nch; i++) {
    SOFTDAC_D16((al->regs+SOFTDAC_DAC0102)+2*chlist[i]) = data[i];
  }

  return dummy;
}
#endif

/********************************************************************/
/** 
Fill particular buffer with nSamples of data
@param vin inital voltage of the linear func
@param vout final voltage of the linear func
@param npts number of points to fill
@param offset initial number of points to offset within the channel
@param ibuf internal buffer to fill (0 or 1)
@param ichan channel to fill (0..7)
@return int status
*/
int softdac_LinLoad(ALPHISOFTDAC * al, double vin, double vout, int npts, int * offset, int ibuf, int ichan)
{
  int16_t dummy = 0;
  volatile uint16_t* b = (uint16_t*)chanAddr(al->regs, ibuf, ichan, *offset);

  uint16_t din  = softdac_Volt2Dac(al, vin);
  uint16_t dout = softdac_Volt2Dac(al, vout);
  printf("LinLoad b: %p npts:%06d vin:%f din:%d vout:%f dout:%d\n", b, npts, vin, din, vout, dout);

  if (npts == 1) {
    b[0] = (uint16_t) din;
    printf("Volt single Dac[0]:%i\n", b[0]);
  } else {
    for (int i=0; i<npts; i++)
      {
	b[i] = (uint16_t)(din + (i * (dout-din) / (npts-1) ));
	printf("&b[%i]:%p  npts:%d Ch:%d Volt dac[%i]:%d\n", i, &b[i], npts, ichan, i
	       , (uint16_t)(din + (i * (dout-din) / (npts-1) )));
      }
  }
  *offset += npts;

  return dummy;
}

/********************************************************************/
/** 
Read buffer
@param npts number of points to fill
@param offset initial number of points to offset within the channel
@param ibuf internal buffer to fill (0 or 1)
@param ichan channel to fill (0..7)
@return int status
*/
int softdac_DacVoltRead(ALPHISOFTDAC * al, int npts, int offset, int ibuf, int ichan, int flag)
{
  int16_t dummy = 0;
  volatile uint16_t* b = (uint16_t*)chanAddr(al->regs, ibuf, ichan, offset);
  FILE *fin = 0;
  
  if (flag) {
    char filename[64];
    sprintf(filename, "softdac-ch%02d.log", ichan);
    fin = fopen(filename, "a");
    fprintf(fin, "-------%s --------------------------\n", filename);
  }
  
  for (int i=0; i<npts; i++) {
    double volt = softdac_Dac2Volt(al, b[i]);
    if (flag) 
      fprintf(fin, "Ch:%d Buf:%d Off:%d Dac:%d Volt:%f \n", ichan, ibuf, offset+i, b[i], volt);
    printf("Mod:Ch:%d Buf:%d Off:%d Dac:%d %p Volt:%f \n", ichan, ibuf, offset+i, b[i], &(b[i]), volt);
  }
  
  if (flag)
    fclose(fin);
  return dummy;
}

/*****************************************************************/
int  softdac_DirectDacWrite(ALPHISOFTDAC * al, uint16_t din, int chan, double *arg)
{
  int16_t dummy = 0;

  SOFTDAC_D16((al->regs+SOFTDAC_DAC0102+(chan*2))) = din;
  printf("Dac chan:%d Reg:0x%x\n", chan,  SOFTDAC_DAC0102+(chan*2));
  return dummy;
}

/*****************************************************************/
int  softdac_DacWrite(ALPHISOFTDAC * al, uint16_t din, int chan, double *arg)
{
  int16_t dummy = 0;

  SOFTDAC_D16((al->regs+SOFTDAC_DAC0102+(chan*2))) = din;
  printf("Dac chan:%d Reg:0x%x\n", chan,  SOFTDAC_DAC0102+(chan*2));
  return dummy;
}

/*****************************************************************/
int  softdac_DirectVoltWrite(ALPHISOFTDAC * al, double vin, int chan, double *arg)
{
  int16_t dummy = 0;

  uint16_t din  = softdac_Volt2Dac(al, vin);
  SOFTDAC_D16((al->regs+SOFTDAC_DAC0102+(chan*2))) = din;
  printf("Volt chan:%d DacVolt:%06d Reg:0x%x\n", chan, din, SOFTDAC_DAC0102+(chan*2));
  return dummy;
}

/*****************************************************************/
void softdac_SampleSet(ALPHISOFTDAC * al, int bank, int samples)
{
  if (bank == 0) {
    SOFTDAC_D32(al->regs+SOFTDAC_LAST_ADDR_0)  = samples-1; // last addr bank 0
  } else if (bank == 1) {
    SOFTDAC_D32(al->regs+SOFTDAC_LAST_ADDR_1)  = samples-1; // last addr bank 1
  }
}

/*****************************************************************/
void  softdac_SMEnable(ALPHISOFTDAC * al)
{
  uint16_t ctl_stat;
  
  ctl_stat = SOFTDAC_D08(al->regs+SOFTDAC_CTL_STAT_0);
  ctl_stat |= SOFTDAC_CLK_OUTPUT_ENABLE |  SOFTDAC_SM_ENABLE;
  SOFTDAC_D08(al->regs+SOFTDAC_CTL_STAT_0)   = ctl_stat; // ctrl 0
}

/*****************************************************************/
void softdac_SMDisable(ALPHISOFTDAC * al)
{
  uint16_t ctl_stat;
  
  ctl_stat = SOFTDAC_D08(al->regs+SOFTDAC_CTL_STAT_0);
  ctl_stat &= ~SOFTDAC_SM_ENABLE;
  SOFTDAC_D08(al->regs+SOFTDAC_CTL_STAT_0)   = ctl_stat; // ctrl 0
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

int  softdac_Setup(ALPHISOFTDAC * al, int samples, int mode)
{
  int dummy;

  switch (mode) {
  case 0x1:
    printf("Internal Clk On, Stay in Bank 0 (mode:%d)\n", mode);
    SOFTDAC_D32(al->regs+SOFTDAC_CLK_SAMP_INT) = SOFTDAC_CLK_500KHZ; // Not used as external
    SOFTDAC_D32(al->regs+SOFTDAC_LAST_ADDR_0)  = samples; // last addr bank 0
    SOFTDAC_D32(al->regs+SOFTDAC_LAST_ADDR_1)  = 0; // last addr bank 1
    SOFTDAC_D08(al->regs+SOFTDAC_BANK_CTL_0)   = 0; // Stay in BANK0 ctrl
    SOFTDAC_D08(al->regs+SOFTDAC_BANK_CTL_1)   = 0; // Stay in BANK1 ctrl
    SOFTDAC_D08(al->regs+SOFTDAC_CTL_STAT_0)   = SOFTDAC_CLK_OUTPUT_ENABLE | SOFTDAC_INTERNAL_CLK ; // ctrl 0
    SOFTDAC_D08(al->regs+SOFTDAC_CTL_STAT_1)   = 0; // ctrl 1
    //    SOFTDAC_D32(al->regs+SOFTDAC_CMD_REG)      = 0x10 | SOFTDAC_RANGE_PM10V;
    //    SOFTDAC_D32(al->regs+SOFTDAC_CMD_REG)      = SOFTDAC_IMMEDIATE_MODE;
    break;

  case 0x2:
    printf("Direct Auto update to channel 1..16 (mode:%d)\n", mode);
    //    printf("\n");
    SOFTDAC_D32(al->regs+SOFTDAC_CLK_SAMP_INT) = SOFTDAC_CLK_500KHZ; // sampling clock divisor
    SOFTDAC_D32(al->regs+SOFTDAC_LAST_ADDR_0)  = samples; // last addr bank 0
    SOFTDAC_D32(al->regs+SOFTDAC_LAST_ADDR_1)  = samples; // last addr bank 1
    SOFTDAC_D08(al->regs+SOFTDAC_BANK_CTL_0)   = 0;                        // BANK0 ctrl
    SOFTDAC_D08(al->regs+SOFTDAC_BANK_CTL_1)   = 0;                        // BANK1 ctrl
    SOFTDAC_D08(al->regs+SOFTDAC_CTL_STAT_0)   = SOFTDAC_AUTO_UPDATE_DAC;          // ctrl 0
    SOFTDAC_D08(al->regs+SOFTDAC_CTL_STAT_1)   = 0;                        // ctrl 1
    //    SOFTDAC_D32(al->regs+SOFTDAC_CMD_REG)      = 0x10 | SOFTDAC_RANGE_PM10V;
    //    SOFTDAC_D32(al->regs+SOFTDAC_CMD_REG)      = SOFTDAC_IMMEDIATE_MODE;

    dummy = al->regs[0]; // flush posted PCI writes
    break;

  case 0x3:
    printf("External Clk, loop on samples, stay in bank0 (mode:%d)\n", mode);
    //    printf("\n");
    SOFTDAC_D32(al->regs+SOFTDAC_CLK_SAMP_INT) = SOFTDAC_CLK_500KHZ; // sampling clock divisor
    SOFTDAC_D32(al->regs+SOFTDAC_LAST_ADDR_0)  = samples; // last addr bank 0
    SOFTDAC_D32(al->regs+SOFTDAC_LAST_ADDR_1)  = 0; // last addr bank 1
    SOFTDAC_D08(al->regs+SOFTDAC_BANK_CTL_0)   = 0;                        // BANK0 ctrl
    SOFTDAC_D08(al->regs+SOFTDAC_BANK_CTL_1)   = 0;                        // BANK1 ctrl
    SOFTDAC_D08(al->regs+SOFTDAC_CTL_STAT_0)   = SOFTDAC_CLK_OUTPUT_ENABLE | SOFTDAC_EXTERNAL_CLK ; // ctrl 0
    SOFTDAC_D08(al->regs+SOFTDAC_CTL_STAT_1)   = 0;                        // ctrl 1
    // Specific to the Softdac
    //    SOFTDAC_D32(al->regs+SOFTDAC_CMD_REG)      = 0x10 | SOFTDAC_RANGE_PM10V;
    //    SOFTDAC_D32(al->regs+SOFTDAC_CMD_REG)      = SOFTDAC_IMMEDIATE_MODE;

    dummy = al->regs[0]; // flush posted PCI writes
    break;

  case 0x4:
    printf("Internal Clk, loop on samples, stay in bank0 (mode:%d)\n", mode);
    //    printf("\n");
    SOFTDAC_D32(al->regs+SOFTDAC_CLK_SAMP_INT) = SOFTDAC_CLK_500KHZ; // sampling clock divisor
    SOFTDAC_D32(al->regs+SOFTDAC_LAST_ADDR_0)  = samples; // last addr bank 0
    SOFTDAC_D32(al->regs+SOFTDAC_LAST_ADDR_1)  = 0; // last addr bank 1
    SOFTDAC_D08(al->regs+SOFTDAC_BANK_CTL_0)   = 0;                        // BANK0 ctrl
    SOFTDAC_D08(al->regs+SOFTDAC_BANK_CTL_1)   = 0;                        // BANK1 ctrl
    SOFTDAC_D08(al->regs+SOFTDAC_CTL_STAT_0)   = SOFTDAC_CLK_OUTPUT_ENABLE | SOFTDAC_INTERNAL_CLK ; // ctrl 0
    SOFTDAC_D08(al->regs+SOFTDAC_CTL_STAT_1)   = 0;                        // ctrl 1
    // Specific to the Softdac
    //    SOFTDAC_D16(al->regs+SOFTDAC_CMD_REG)      = 0x10 | SOFTDAC_RANGE_PM10V;
    //    SOFTDAC_D16(al->regs+SOFTDAC_CMD_REG)      = SOFTDAC_IMMEDIATE_MODE;
  
    dummy = al->regs[0]; // flush posted PCI writes
    break;

  default:
    printf("Unknown setup mode\n");
    return -1;
  }
  return 0;
}

/*****************************************************************/
/**
*/
int  softdac_Status(ALPHISOFTDAC * al, int mode)
{

  int cmdstat = SOFTDAC_D16(al->regs+SOFTDAC_CMD_REG);
  unsigned int stat = SOFTDAC_D32(al->regs+SOFTDAC_OFFSET+SOFTDAC_REGS_D32);
  unsigned int lsamp0 = SOFTDAC_D32(al->regs+SOFTDAC_LAST_ADDR_0);
  unsigned int lsamp1 = SOFTDAC_D32(al->regs+SOFTDAC_LAST_ADDR_1);
  switch (mode) {
  case 0x1:
    if ((stat & 0x3) == 0x0)      printf("Stay in Bank 0    ");
    if ((stat & 0x3) == 0x1)      printf("Switch at 0       ");
    if ((stat & 0x3) == 0x2)      printf("Dis.SM out:last   ");
    if ((stat & 0x3) == 0x3)      printf("Dis.SM out:last Ov");
    if ((stat & 0x300) == 0x0)    printf("Stay in Bank 1    ");
    if ((stat & 0x300) == 0x100)  printf("Switch at 0       ");
    if ((stat & 0x300) == 0x200)  printf("Dis.SM out:last   ");
    if ((stat & 0x300) == 0x300)  printf("Dis.SM out:last Ov");
    printf("%s:Active Bank     ", stat & 0x00010000 ? "1" : "0");
    printf("%s:Clk oupt En     ", stat & 0x00020000 ? "y" : "n");
    printf("%s:Internal Clk   \n", stat & 0x00040000 ? "y" : "n");
    printf("%s:External Clk    ", stat & 0x00080000 ? "y" : "n");
    printf("%s:SM enabled      ", stat & 0x00200000 ? "y" : "n");
    printf("%s:FP Reset  En    ", stat & 0x00400000 ? "y" : "n");
    printf("%s:Auto update    \n", stat & 0x00800000 ? "y" : "n");

    printf("%s:FP Reset State  ", stat & 0x01000000 ? "y" : "n");
    printf("%s:FP Rst Int En   ", stat & 0x02000000 ? "y" : "n");
    printf("%s:Clk Int En      ", stat & 0x04000000 ? "y" : "n");
    printf("%s:Int B0  done  \n", stat & 0x10000000 ? "y" : "n");
    printf("%s:Int B1  done    ", stat & 0x20000000 ? "y" : "n");
    printf("%s:Int clk done    ", stat & 0x40000000 ? "y" : "n");
    printf("%s:Int FP  done   \n", stat & 0x80000000 ? "y" : "n");
 
    printf("0x%2.2x:BANK_CTL_0 (0x10)", SOFTDAC_D08(al->regs+SOFTDAC_BANK_CTL_0));
    printf("   0x%2.2x:BANK_CTL_1 (0x11)\n", SOFTDAC_D08(al->regs+SOFTDAC_BANK_CTL_1));
    printf("0x%2.2x:CTL_STAT_0 (0x12)", SOFTDAC_D08(al->regs+SOFTDAC_CTL_STAT_0));
    printf("   0x%2.2x:CTL_STAT_1 (0x13)\n", SOFTDAC_D08(al->regs+SOFTDAC_CTL_STAT_1));
    printf("Base Addr (Offset) %p\n", al->regs);

    printf("Last Addr 0/1 [%d / %d] (0x08 / 0x0C)\n", lsamp0, lsamp1);
    printf("0x%8.8x: stat 0x13..0x10\n", stat);

  switch (al->range) {
  case SOFTDAC_RANGE_P5V:
    printf("Range : P5V (0x%x)\n", al->range);
    break;
  case SOFTDAC_RANGE_P10V:
    printf("Range : P10V (0x%x)\n", al->range);
    break;
  case SOFTDAC_RANGE_PM5V:
    printf("Range : PM5V (0x%x)\n", al->range);
    break;
  case SOFTDAC_RANGE_PM10V:
    printf("Range : PM10V (0x%x)\n", al->range);
    break;
  case SOFTDAC_RANGE_PM2P5V:
    printf("Range : PM2P5V (0x%x)\n", al->range);
    break;
  case SOFTDAC_RANGE_M2P5P7P5V:
    printf("Range : M2P5P7P5V (0x%x)\n", al->range);
    break;
  }
    printf("0x%x: cmdstat \n", cmdstat);

    break;
  case 0x2:
    printf("%s:FP Reset State  ", stat & 0x01000000 ? "y" : "n");
    printf("%s:FP Rst Int En   ", stat & 0x02000000 ? "y" : "n");
    printf("%s:Clk Int Enabe   ", stat & 0x04000000 ? "y" : "n");
    printf("%s:Int B0 done    \n", stat & 0x10000000 ? "y" : "n");
    printf("%s:Int B1 done     ", stat & 0x20000000 ? "y" : "n");
    printf("%s:Int clk done    ", stat & 0x40000000 ? "y" : "n");
    printf("%s:Int FP done    \n", stat & 0x80000000 ? "y" : "n");
    break;
    
  default:
    printf("Unknown setup mode\n");
    return -1;
  }
  return 0;
}

/********************************************************************/
/********************************************************************/
#ifdef MAIN_ENABLE
int main(int argc,char* argv[])
{
  ALPHISOFTDAC *al=0;
  int dummy=0;
  int i;
  uint16_t din=0;
  double arg = 0, ddout;
  int chan = 0;
  int samples = 10;

  if (argc > 1)
    chan = atoi(argv[1]);
  
  if (softdac_Open( &al) < 0)
    {
      fprintf(stderr,"Cannot open PMC-SOFTDAC device!\n");
      exit(1);
    }
  
  printf("alpha:%e  --  offset:%e\n", al->alpha, al->beta);
  softdac_Status(al, 1);
  
#if 0
  if (argc == 3) {
    chan = atoi(argv[1]);
    //    din  = (uint16_t) atoi(argv[2]);
    samples = 10;
    softdac_Setup(al, samples, 2);
    
    softdac_ScaleSet(al, SOFTDAC_RANGE_PM10V,  10.0/32767., -10.0);
    printf("alpha:%e  --  offset:%e\n", al->alpha, al->beta);
    for (i=chan;;) {
      ddout = -10.0;
      softdac_DirectVoltWrite(al, ddout, i, &arg); 
      ddout = 10.0;
      softdac_DirectVoltWrite(al, ddout, i, &arg);
   }
    softdac_Status(al, 1);
  }
  else
    printf("arg error\n");
  
#endif
#if 0
  if (argc == 3) {
    chan = atoi(argv[1]);
    din  = (uint16_t) atoi(argv[2]);
    samples = 0;
    softdac_Setup(al, samples, 2);

    softdac_ScaleSet(al, SOFTDAC_RANGE_PM10V, 10.0/32767., 0.0);
    printf("alpha:%e  --  offset:%e\n", al->alpha, al->beta);

    for (i=0;i<8;i++) {
      ddout = -10.0+i;
      softdac_DirectVoltWrite(al, ddout, i, &arg);
    }
  }
  else
    printf("arg error\n");
  
#endif

#if 1
  int status, pts=10, npts=0;
  float fdin  = atof(argv[2]);
  softdac_Reset(al);
  softdac_Setup(al, npts, 1);
  softdac_ScaleSet(al, SOFTDAC_RANGE_PM10V, 0., 0.);
  softdac_ScaleSet(al, SOFTDAC_RANGE_PM10V, 0., 0.);
  status = softdac_LinLoad(al, fdin , -1*fdin, pts,  &npts, 0, chan);
  printf("......................ch:%d pts:%d\n", chan, npts);
  softdac_DacVoltRead(al, npts, 0, 0, chan, 0);
  softdac_SampleSet(al, 0, npts);
  softdac_SMEnable(al);
  softdac_Status(al, 1);
#endif

#if 0
  int i, status, npts, pts;
  softdac_Reset(al);
  pts = 0;
  status = softdac_LinLoad(al, 10.,-8., 20,  &pts, 0, 0);
  status = softdac_LinLoad(al, -8.,6., 20,  &pts, 0, 0);
  status = softdac_LinLoad(al,  6.,-4., 20,  &pts, 0, 0);
  status = softdac_LinLoad(al, -4.,2., 20,  &pts, 0, 0);
  status = softdac_LinLoad(al,  2.,-2., 20,  &pts, 0, 0);
  status = softdac_LinLoad(al, -2.,4., 20,  &pts, 0, 0);
  status = softdac_LinLoad(al,  4.,-6., 20,  &pts, 0, 0);
  status = softdac_LinLoad(al, -6., 8, 20,  &pts, 0, 0);
  
  printf("pts:%d\n", pts);

  softdac_DacVoltRead(al, pts, 0, 0, 0, 0);
  softdac_DacVoltRead(al, pts, 0, 0, 0, 1);

  printf("Active banks: %d\n", softdac_BankActiveRead(al));
  npts = samples = pts-1;

  status = softdac_Setup(al, samples, 1);

  softdac_Status(al, 2);

  printf("Active banks: %d\n", softdac_BankActiveRead(al));
#endif

#if 0
  int i, status, npts, pts;

  softdac_Reset(al);
  pts = 0;
  status = softdac_LinLoad(al,  10.,-8., 20,  &pts, 0, 0); pts=0;
  status = softdac_LinLoad(al,  -8.,6., 20,  &pts, 0, 1); pts=0;
  status = softdac_LinLoad(al,  6.,-4., 20,  &pts, 0, 2); pts=0;
  status = softdac_LinLoad(al,  -4.,2., 20,  &pts, 0, 3); pts=0;
  status = softdac_LinLoad(al,  2.,-2., 20,  &pts, 0, 4); pts=0;
  status = softdac_LinLoad(al,  -2.,4., 20,  &pts, 0, 5); pts=0;
  status = softdac_LinLoad(al,  4.,-6., 20,  &pts, 0, 6); pts=0;
  status = softdac_LinLoad(al, -6., 8., 20,  &pts, 0, 7);
  
  printf("......................pts:%d\n", pts);

  for (int i=0; i<8; i++)
    softdac_DacVoltRead(al, pts, 0, 0, i, 0);

  printf("Active banks: %d\n", softdac_BankActiveRead(al));
  npts = samples = pts-1;

  status = softdac_Setup(al, samples, 1);
#endif

#if 0
  int samples = softdac_MaxSamplesGet();

  double arg = 0;
  int xchan = 1;
  fillLinBuffer(al, samples, 0, chan, &arg);
  fillLinBuffer(al, samples, 1, chan, &arg);

  arg = 0;
  xchan = 0;
  fillSinBuffer(al, samples, 0, xchan, &arg);
  fillSinBuffer(al, samples, 1, xchan, &arg);
#endif

  softdac_Close(al);
  return dummy;
}

#endif
/* end file */
