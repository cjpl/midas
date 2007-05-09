/*********************************************************************

  Name:         softdac.c
  Created by:   Konstantin Olchanski

  Cotents:      Routines for accessing the AWG PMC-SOFTDAC-M board
                from ALPHI TECHNOLOGY CORPORATION 
                
 * See manual at
 * http://www.alphitech.com
 * http://www.alphitech.com/doc/???.pdf
 *
  $Id$
*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <sys/mman.h>
#define  ALPHI_SOFTDAC
#include "alphidac.h"

typedef char           uint08;
typedef unsigned short uint16;
typedef unsigned int   uint32;

#define D08(ptr) (*(volatile uint08*)(ptr))
#define D16(ptr) (*(volatile uint16*)(ptr))
#define D32(ptr) (*(volatile uint32*)(ptr))

// Device specifics
ALPHIPMC  softdac;

/********************************************************************/
/** 
Open channel to pmcda16 
@param regs
@param data
@return int
*/
int softdac_open(char** regs, char** data)
{
  FILE *fp = fopen("/dev/pmcsoftdacm","r+");
  if (fp==0)
    {
      fprintf(stderr,"Cannot open the PMC-SOFTDAC-M device, errno: %d (%s)\n",
	      errno,strerror(errno));
      return -errno;
    }

  int fd = fileno(fp);

  if (regs)
    {
      *regs = (char*)mmap(0,0x100000,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
      if (*regs == NULL)
	{
	  printf("regs: 0x%p\n",regs);
	  fprintf(stderr,"Cannot mmap() PMC-SOFTDAC-M registers, errno: %d (%s)\n",errno,strerror(errno));
	  return -errno;
	}
    }

  // Set initial scaling coeficients
  softdac_ScaleSet(RANGE_PM10V, 0., 0.);

  return 0;
}

/********************************************************************/
/** 
Set the alpha/beta coefficients for the Volt2Dac conversion.
Only one set of param for all the channels.
@param alpha conversion slope coeff
@param beta  conversion offset coeff
@return int
*/
int softdac_ScaleSet(int range, double alpha, double beta)
{
  switch (range) {
  case RANGE_P5V:
    softdac.alpha = 5.0/32767.;
    softdac.beta =  0.0;
    break;
  case RANGE_P10V:
    softdac.alpha = 10.0/65565.;
    softdac.beta =  0.0;
    break;
  case RANGE_PM5V:
    softdac.alpha = 5.0/32767.;
    softdac.beta =  -5.0;
    break;
  case RANGE_PM10V:
    softdac.alpha = 10.0/32767.;
    softdac.beta =  -10.0;
    break;
  case RANGE_PM2P5V:
    softdac.alpha = 2.5/32767.;
    softdac.beta =  -2.5;
    break;
  case RANGE_M2P5P7P5V:
    softdac.alpha = 7.5/32767.;
    softdac.beta =  -2.5;
    break;
  case SET_COEF:
    softdac.alpha = alpha;
    softdac.beta  = beta;
    break;
  }

  if (range != SET_COEF) {
    D32(softdac.regs+CMD_REG)      = 0x10 | range;
    D32(softdac.regs+CMD_REG)      = IMMEDIATE_MODE;
  }

  return (0);
}

/********************************************************************/
/** 
Reset softdac
@param regs
@return int
*/
int softdac_Reset(char* regs)
{
  for (int i=0; i<0x14; i+=4)
    D32(regs+OFFSET+i) = 0;
  D32(regs+OFFSET+0x14) = CLK_SAMPLE_RESET;
  D32(regs+OFFSET+0x16) = ADDRESS_RESET;
  D32(regs+OFFSET+0x18) = DACS_RESET;
  D32(regs+OFFSET+0x10) = 0xF0000000;
  return regs[OFFSET+0]; // flush posted PCI writes
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
uint16_t softdac_Volt2Dac(double volt)
{
  //  printf("volt:%f dac:%d\n", volt, (uint16_t) ((volt - softdac.beta) / softdac.alpha));
  return (uint16_t) ((volt - softdac.beta) / softdac.alpha);
}

/********************************************************************/
/** 
Convert Dac to Volt value
@param  dac Dac value to convert
@return double volt value
*/
double softdac_Dac2Volt (uint16_t dac)
{
  //  printf("dac:%d volt:%f\n", dac,(double) (dac * softdac.alpha + softdac.beta));
  return (double) (dac * softdac.alpha + softdac.beta);
}

/********************************************************************/
/** 
Get the current softdac status register
@param regs
@return int
*/
void softdac_status(char* regs)
{
  printf("PMC-SOFTDAC-M status:\n");
  for (int i=0; i<18; i+=4)
    {
      uint32 s = D32(regs+OFFSET+i);
      printf("regs[0x%04x] = 0x%08x\n",OFFSET+i,s);
    }
}

/********************************************************************/
/** 
Dump softdac data
@param addr
*/
void softdac_dump(char* addr)
{
  printf("PMC-SOFTDAC-M data dump:\n");
  for (int i=0; i<OFFSET; i+=4)
    {
      uint32 s = D32(addr+i);
      printf("data[0x%06x] = 0x%08x\n",i,s);
    }
}

/********************************************************************/
/** 
Get channel address for a particular bank
@param data
@param ibank
@param ichan
@return int
*/
char* chanAddr(char* data, int ibank, int ichan, int offset)
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
int softdac_BankActiveRead(void)
{
      int stat0 = D08(softdac.regs+CTL_STAT_0);
      stat0 &= 0x1;
      int stat1 = D08(softdac.regs+CTL_STAT_1);
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
int softdac_DacWrite(char * regs, unsigned short *data, int * chlist, int nch)
{
  int16_t dummy = 0;

  for (int i=0; i<nch; i++) {
    D16((regs+DAC0102)+2*chlist[i]) = data[i];
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
int softdac_LinLoad(double vin, double vout, int npts, int * offset, int ibuf, int ichan)
{
  int16_t dummy = 0;
  volatile uint16_t* b = (uint16_t*)chanAddr(softdac.regs, ibuf, ichan, *offset);

  uint16_t din  = softdac_Volt2Dac(vin);
  uint16_t dout = softdac_Volt2Dac(vout);
  printf("LinLoad b: %p npts:%06d vin:%f din:%d vout:%f dout:%d\n", b, npts, vin, din, vout, dout);

  if (npts == 1) {
    b[0] = (uint16_t) din;
    printf("Volt single Dac[0]:%i\n", b[0]);
  } else {
    for (int i=0; i<npts; i++)
      {
	b[i] = (uint16_t)(din + (i * (dout-din) / (npts-1) ));
	printf("Ch:%d Volt dac[%i]:%d\n", ichan, i, b[i]);
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
int softdac_DacVoltRead(int npts, int offset, int ibuf, int ichan, int flag)
{
  int16_t dummy = 0;
  volatile uint16_t* b = (uint16_t*)chanAddr(softdac.regs, ibuf, ichan, offset);
  FILE *fin = 0;
  
  if (flag) {
    char filename[64];
    sprintf(filename, "softdac-ch%02d.log", ichan);
    fin = fopen(filename, "a");
    fprintf(fin, "-------%s --------------------------\n", filename);
  }
  
  for (int i=0; i<npts; i++) {
    double volt = softdac_Dac2Volt(b[i]);
    if (flag) 
      fprintf(fin, "Ch:%d Buf:%d Off:%d Dac:%d Volt:%f \n", ichan, ibuf, offset+i, b[i], volt);
    printf("Mod:Ch:%d Buf:%d Off:%d Dac:%d %p Volt:%f \n", ichan, ibuf, offset+i, b[i], &(b[i]), volt);
  }
  
  if (flag)
    fclose(fin);
  return dummy;
}

/*****************************************************************/
int  softdac_DirectDacWrite(uint16_t din, int chan, double *arg)
{
  int16_t dummy = 0;

  D16((softdac.regs+DAC0102+(chan*2))) = din;
  printf("Dac chan:%d Reg:0x%x\n", chan,  DAC0102+(chan*2));
  return dummy;
}

/*****************************************************************/
int  softdac_dacWrite(uint16_t din, int chan, double *arg)
{
  int16_t dummy = 0;

  D16((softdac.regs+DAC0102+(chan*2))) = din;
  printf("Dac chan:%d Reg:0x%x\n", chan,  DAC0102+(chan*2));
  return dummy;
}

/*****************************************************************/
int  softdac_DirectVoltWrite(double vin, int chan, double *arg)
{
  int16_t dummy = 0;

  uint16_t din  = softdac_Volt2Dac(vin);
  D16((softdac.regs+DAC0102+(chan*2))) = din;
  printf("Volt chan:%d Reg:0x%x\n", chan,  DAC0102+(chan*2));
  return dummy;
}

/*****************************************************************/
int  softdac_SampleSet(char * regs, int bank, int samples)
{
  int16_t dummy = 0;
  
  if (bank == 0) {
    D32(regs+LAST_ADDR_0)  = samples; // last addr bank 0
  } else if (bank == 1) {
    D32(regs+LAST_ADDR_1)  = samples; // last addr bank 1
  }
  
  return dummy;
}

/*****************************************************************/
int  softdac_ClkSMEnable( char * regs, int bank)
{
  int16_t dummy = 0;
  
  if (bank == 0) {
    D08(regs+CTL_STAT_0)   = CLK_OUTPUT_ENABLE | EXTERNAL_CLK | SM_ENABLE; // ctrl 0
  } else if (bank == 1) {
    D08(regs+CTL_STAT_1)   = CLK_OUTPUT_ENABLE | EXTERNAL_CLK | SM_ENABLE; // ctrl 1
  }
  
  return dummy;
}

/*****************************************************************/
int  softdac_ClkSMDisable( char * regs, int bank)
{
  int16_t dummy = 0;
  
  if (bank == 0) {
    D08(regs+CTL_STAT_0)   = CLK_OUTPUT_ENABLE; // ctrl 0
  } else if (bank == 1) {
    D08(regs+CTL_STAT_1)   = CLK_OUTPUT_ENABLE; // ctrl 1
  }
  
  return dummy;
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

int  softdac_Setup(char * regs, int samples, int mode)
{
  int dummy;

  switch (mode) {
  case 0x1:
    printf("External Clk On, +/-10V range Stay in Bank 0 (mode:%d)\n", mode);
    D32(regs+CLK_SAMP_INT) = CLK_500KHZ; // Not used as external
    D32(regs+LAST_ADDR_0)  = samples; // last addr bank 0
    D32(regs+LAST_ADDR_1)  = 0; // last addr bank 1
    D08(regs+BANK_CTL_0)   = 0; // Stay in BANK0 ctrl
    D08(regs+BANK_CTL_1)   = 0; // Stay in BANK1 ctrl
    D08(regs+CTL_STAT_0)   = CLK_OUTPUT_ENABLE | INTERNAL_CLK | SM_ENABLE; // ctrl 0
    D08(regs+CTL_STAT_1)   = 0; // ctrl 1
    D32(regs+CMD_REG)      = 0x10 | RANGE_PM10V;
    D32(regs+CMD_REG)      = IMMEDIATE_MODE;
    break;

  case 0x2:
    printf("Direct Auto update to channel 1..16 (mode:%d)\n", mode);
    //    printf("\n");
    D32(regs+CLK_SAMP_INT) = CLK_500KHZ; // sampling clock divisor
    D32(regs+LAST_ADDR_0)  = samples; // last addr bank 0
    D32(regs+LAST_ADDR_1)  = samples; // last addr bank 1
    D08(regs+BANK_CTL_0)   = 0;                        // BANK0 ctrl
    D08(regs+BANK_CTL_1)   = 0;                        // BANK1 ctrl
    D08(regs+CTL_STAT_0)   = AUTO_UPDATE_DAC;          // ctrl 0
    D08(regs+CTL_STAT_1)   = 0;                        // ctrl 1
    D32(regs+CMD_REG)      = 0x10 | RANGE_PM10V;
    D32(regs+CMD_REG)      = IMMEDIATE_MODE;

    dummy = regs[0]; // flush posted PCI writes
    break;

  case 0x3:
    printf("External Clk, loop on samples, stay in bank0 (mode:%d)\n", mode);
    //    printf("\n");
    D32(regs+CLK_SAMP_INT) = CLK_500KHZ; // sampling clock divisor
    D32(regs+LAST_ADDR_0)  = samples; // last addr bank 0
    D32(regs+LAST_ADDR_1)  = 0; // last addr bank 1
    D08(regs+BANK_CTL_0)   = 0;                        // BANK0 ctrl
    D08(regs+BANK_CTL_1)   = 0;                        // BANK1 ctrl
    D08(regs+CTL_STAT_0)   = CLK_OUTPUT_ENABLE | SM_ENABLE | EXTERNAL_CLK ; // ctrl 0
    D08(regs+CTL_STAT_1)   = 0;                        // ctrl 1
    // Specific to the Softdac
    D32(regs+CMD_REG)      = 0x10 | RANGE_PM10V;
    D32(regs+CMD_REG)      = IMMEDIATE_MODE;

    dummy = regs[0]; // flush posted PCI writes
    break;

  default:
    printf("Unknown setup mode\n");
    return -1;
  }
  //    softdac_status(regs);
  return 0;
}

/*****************************************************************/
/**
*/
int  softdac_Status(char * regs, int mode)
{

  int stat = D32(regs+OFFSET+REGS_D32);
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

  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  double arg = 0;
  int dummy, chan=0, samples, status, npts;
  uint16_t din;
  unsigned short  vdac[16];  
  int chlist[16];

  if (argc > 1)
    chan = atoi(argv[1]) - 1;
  
  softdac.regs=NULL;

  if (softdac_open(&softdac.regs, &softdac.data) < 0)
    {
      fprintf(stderr,"Cannot open PMC-SOFTDAC device!\n");
      exit(1);
    }
  
  //  softdac_Reset(softdac.regs);

  softdac_Status(softdac.regs, 1);

#if 0
  while (1)
    {
      softdac_Status(softdac.regs, 1);
      printf("chan DAC%02d\n", 1+chan);
      sleep(1);
    }
#endif

#if 0
  /* clear data memory */
  memset(softdac.regs, 0, 0x80000);
  dummy = softdac.regs[0]; // flush posted PCI writes
  
  if (argc == 3) {
    chan = atoi(argv[1]) - 1;
    din  = (uint16_t) atoi(argv[2]);
  
    samples = softdac_MaxSamplesGet();
    samples = 10;
    softdac_Setup(softdac.regs, samples, 2);
    
    softdac_DirectDacWrite(din, chan, &arg);
  }
  else
    printf("arg error\n");

#endif

#if 1
  // Set initial scaling coeficients
  softdac_Reset(softdac.regs);
  /* clear data memory */
  memset(softdac.regs, 0, 0x80000);
  dummy = softdac.regs[0]; // flush posted PCI writes

  int pts = 10; npts=0;
  status = softdac_LinLoad( -2.,2., pts,  &npts, 0, 0); npts=0;
  status = softdac_LinLoad( -4.,4., pts,  &npts, 0, 1); npts=0;
  status = softdac_LinLoad( -6.,6., pts,  &npts, 0, 2); npts=0;
  status = softdac_LinLoad( -10.,10., pts,  &npts, 0, 3);

  printf("......................pts:%d\n", pts);

  for (int i=0; i<4; i++)
    softdac_DacVoltRead(pts, 0, 0, i, 0);
  
  D32(softdac.regs+LAST_ADDR_0)  = pts-1; // last addr bank 0
  D32(softdac.regs+LAST_ADDR_1)  = 0; // last addr bank 1
  D08(softdac.regs+BANK_CTL_0)   = 0;                        // BANK0 ctrl
  D08(softdac.regs+BANK_CTL_1)   = 0;                        // BANK1 ctrl
  D32(softdac.regs+CLK_SAMP_INT) = CLK_100KHZ; // sampling clock divisor
  D08(softdac.regs+CTL_STAT_0)   = CLK_OUTPUT_ENABLE | INTERNAL_CLK | SM_ENABLE; // ctrl 0
  D08(softdac.regs+CTL_STAT_1)   = 0;                        // ctrl 1
  D32(softdac.regs+CMD_REG)      = 0x10 | RANGE_PM10V;
  D32(softdac.regs+CMD_REG)      = IMMEDIATE_MODE;

#endif

#if 0
  softdac_Reset(softdac.regs);
  /* clear data memory */
  printf("Active banks: %d\n", softdac_BankActiveRead());
  memset(softdac.regs, 0, 0x80000);
  dummy = softdac.regs[0]; // flush posted PCI writes

  int pts = 0;
  status = softdac_LinLoad( 10.,-8., 20,  &pts, 0, 0);
  status = softdac_LinLoad( -8.,6., 20,  &pts, 0, 0);
  status = softdac_LinLoad( 6.,-4., 20,  &pts, 0, 0);
  status = softdac_LinLoad( -4.,2., 20,  &pts, 0, 0);
  status = softdac_LinLoad( 2.,-2., 20,  &pts, 0, 0);
  status = softdac_LinLoad( -2.,4., 20,  &pts, 0, 0);
  status = softdac_LinLoad( 4.,-6., 20,  &pts, 0, 0);
  status = softdac_LinLoad(-6., 8, 20,  &pts, 0, 0);
  
  printf("pts:%d\n", pts);

  softdac_DacVoltRead(pts, 0, 0, 0, 0);
  softdac_DacVoltRead(pts, 0, 0, 0, 1);

  printf("Active banks: %d\n", softdac_BankActiveRead());
  npts = samples = pts-1;

  softdac_status(softdac.regs);

  status = softdac_Setup(softdac.regs, samples, 1);

  //  status = softdac_SampleSet(softdac.regs, 0, samples);
  softdac_status(softdac.regs);
  //  status = softdac_ClkSMDisable(softdac.regs, 0);
  //  softdac_status(softdac.regs);
  //  status = softdac_ClkSMEnable(softdac.regs, 0);
  //  softdac_status(softdac.regs);
  //  softdac_Status(softdac.regs, 1);
  softdac_Status(softdac.regs, 2);

  printf("Active banks: %d\n", softdac_BankActiveRead());
  //  status = softdac_BankSwitch(1);
  //  printf("Active banks: %d\n", softdac_BankActiveRead());
#endif

#if 0
  softdac_Reset(softdac.regs);
  /* clear data memory */
  printf("Active banks: %d\n", softdac_BankActiveRead());
  memset(softdac.regs, 0, 0x80000);
  dummy = softdac.regs[0]; // flush posted PCI writes

  int pts = 0;
  status = softdac_LinLoad( 10.,-8., 20,  &pts, 0, 0); pts=0;
  status = softdac_LinLoad( -8.,6., 20,  &pts, 0, 1); pts=0;
  status = softdac_LinLoad( 6.,-4., 20,  &pts, 0, 2); pts=0;
  status = softdac_LinLoad( -4.,2., 20,  &pts, 0, 3); pts=0;
  status = softdac_LinLoad( 2.,-2., 20,  &pts, 0, 4); pts=0;
  status = softdac_LinLoad( -2.,4., 20,  &pts, 0, 5); pts=0;
  status = softdac_LinLoad( 4.,-6., 20,  &pts, 0, 6); pts=0;
  status = softdac_LinLoad(-6., 8., 20,  &pts, 0, 7);
  
  printf("......................pts:%d\n", pts);

  for (int i=0; i<8; i++)
    softdac_DacVoltRead(pts, 0, 0, i, 0);

  printf("Active banks: %d\n", softdac_BankActiveRead());
  npts = samples = pts-1;

  softdac_status(softdac.regs);

  status = softdac_Setup(softdac.regs, samples, 1);

  //  status = softdac_SampleSet(softdac.regs, 0, samples);
  softdac_status(softdac.regs);
  //  status = softdac_ClkSMDisable(softdac.regs, 0);
  //  softdac_status(softdac.regs);
  //  status = softdac_ClkSMEnable(softdac.regs, 0);
  //  softdac_status(softdac.regs);
  //  softdac_Status(softdac.regs, 1);
  softdac_Status(softdac.regs, 2);

  printf("Active banks: %d\n", softdac_BankActiveRead());
  //  status = softdac_BankSwitch(1);
  //  printf("Active banks: %d\n", softdac_BankActiveRead());
#endif

#if 0
  /* clear data memory */
  memset(softdac.regs, 0, OFFSET);
  dummy = softdac.regs[0]; // flush posted PCI writes
  
  int samples = softdac_MaxSamplesGet();

  double arg = 0;
  int xchan = 1;
  fillLinBuffer(softdac.regs, samples, 0, chan, &arg);
  fillLinBuffer(softdac.regs, samples, 1, chan, &arg);

  arg = 0;
  xchan = 0;
  fillSinBuffer(softdac.regs, samples, 0, xchan, &arg);
  fillSinBuffer(softdac.regs, samples, 1, xchan, &arg);

  dummy = softdac.regs[0]; // flush posted PCI writes

  //softdac_Setup(softdac.regs, samples, 1);

#endif

#if 0
  //int samples = 10;
  D32(softdac.regs + OFFSET + 0x48) = 0x1B;
  D32(softdac.regs + OFFSET + 0x20) = 0xf00f;
  D32(softdac.regs + OFFSET + 48) = 0x02;

  D32(softdac.regs+CLK_SAMP_INT) = CLK_500KHZ; // sampling clock divisor
  D32(softdac.regs+LAST_ADDR_0)  = samples; // last addr bank 0
  D32(softdac.regs+LAST_ADDR_1)  = samples; // last addr bank 1
  D08(softdac.regs+BANK_CTL_0)   = SWITCH_AT_ADDR_0 | INT_BANK_DONE;             // BANK0 ctrl
  D08(softdac.regs+BANK_CTL_1)   = SWITCH_AT_ADDR_0 | INT_BANK_DONE;             // BANK1 ctrl
  D08(softdac.regs+CTL_STAT_0)   = CLK_OUTPUT_ENABLE | INTERNAL_CLK | SM_ENABLE; // ctrl 0
  D08(softdac.regs+CTL_STAT_1)   = 0;                                            // ctrl 1

  dummy = D32(softdac.regs+CLK_SAMP_INT); // flush posted PCI writes
  softdac_status(softdac.regs);
#endif

#if 0
  while (0)
    {
      int softdac.regstat0 = D32(softdac.regs+OFFSET+0x12);
      if (softdac.regstat0 & 1)
	write(1,"1",1);
      else
	write(1,"0",1);
    }

#endif

#if 0
  softdac_Setup(softdac.regs, 0, 2);
  for (int i=0;i<4;i++) {
    vdac[i] = 20000*(i+1);
    chlist[i] = i;
  }
  softdac_DacWrite(softdac.regs, vdac, chlist, 4);

#endif
  softdac_Status(softdac.regs, 1);

#if 0
  int iloop = 0;
  int nextBuf = 0;
  while (1)
    {
      int stat = D32(softdac.regs+OFFSET+REGS_D32);
      int activeBuf = 0;
      if (stat & ACTIVE_BANK_D32)
	activeBuf = 1;
      if (activeBuf == nextBuf)
	{
	  // sleep here for a bit
	  iloop ++;
	  usleep(20000);
	  continue;
	}

      //usleep(1000);
      //sleep(1);
      //usleep(40000);

      //softdac_Status(softdac.regs, 2);
      printf("loop %d, stat 0x%08x, active %d, refill %d.", iloop, stat, activeBuf, nextBuf);
      fillSinBuffer(softdac.regs, samples, nextBuf, xchan, &arg);
      nextBuf = 1-nextBuf;
      iloop = 0;
    }
#endif

  return dummy;
}

#endif
/* end file */
