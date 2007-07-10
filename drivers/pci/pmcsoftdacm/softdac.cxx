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
#include "alphidac17.h"

/********************************************************************/
/** 
Open channel to pmcda16 
@return int
*/
int softdac_Open(ALPHIPMC ** al)
{

  // Book space
  *al = (ALPHIPMC *) calloc(1, sizeof(ALPHIPMC));
  memset((char *) *al, 0, sizeof(ALPHIPMC));
  (*al)->regs = (char *) calloc(1, sizeof(char *));
  (*al)->data = (char *) calloc(1, sizeof(char *));

  FILE *fp = fopen("/dev/pmcsoftdacm","r+");
  if (fp==0)
    {
      fprintf(stderr,"Cannot open the PMC-SOFTDAC-M device, errno: %d (%s)\n",
	      errno,strerror(errno));
      return -errno;
    }

  int fd = fileno(fp);

  if ((*al)->regs)
    {
      (*al)->regs = (char*)mmap(0,0x100000,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
      if ((*al)->regs == NULL)
	{
	  printf("regs: 0x%p\n",(*al)->regs);
	  fprintf(stderr,"Cannot mmap() PMC-SOFTDAC-M registers, errno: %d (%s)\n",errno,strerror(errno));
	  return -errno;
	}
    }

  // Set initial scaling coefficients
  softdac_ScaleSet(*al, RANGE_PM10V, 0., 0.);

  return 0;
}

/********************************************************************/
/** 
*/
void softdac_Close(ALPHIPMC * al)
{
  free (al);
}

/********************************************************************/
/** 
Set the alpha/beta coefficients for the Volt2Dac conversion.
Only one set of param for all the channels.
@param alpha conversion slope coeff
@param beta  conversion offset coeff
@return int
*/
int softdac_ScaleSet(ALPHIPMC * al, int range, double alpha, double beta)
{
  switch (range) {
  case RANGE_P5V:
    al->alpha = 5.0/32767.;
    al->beta =  0.0;
    break;
  case RANGE_P10V:
    al->alpha = 10.0/65565.;
    al->beta =  0.0;
    break;
  case RANGE_PM5V:
    al->alpha = 5.0/32767.;
    al->beta =  -5.0;
    break;
  case RANGE_PM10V:
    al->alpha = 10.0/32767.;
    al->beta =  -10.0;
    break;
  case RANGE_PM2P5V:
    al->alpha = 2.5/32767.;
    al->beta =  -2.5;
    break;
  case RANGE_M2P5P7P5V:
    al->alpha = 7.5/32767.;
    al->beta =  -2.5;
    break;
  case SET_COEF:
    al->alpha = alpha;
    al->beta  = beta;
    break;
  }

  if (range != SET_COEF) {
    D32(al->regs+CMD_REG)      = 0x10 | range;
    D32(al->regs+CMD_REG)      = IMMEDIATE_MODE;
  }

  return (0);
}

/********************************************************************/
/** 
Reset softdac
@param regs
@return int
*/
int softdac_Reset(ALPHIPMC * al)
{
  for (int i=0; i<0x14; i+=4)
    D32(al->regs+OFFSET+i) = 0;
  D32(al->regs+OFFSET+0x14) = CLK_SAMPLE_RESET;
  D32(al->regs+OFFSET+0x16) = ADDRESS_RESET;
  D32(al->regs+OFFSET+0x18) = DACS_RESET;
  D32(al->regs+OFFSET+0x10) = 0xF0000000;
  return al->regs[OFFSET+0]; // flush posted PCI writes
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
uint16_t softdac_Volt2Dac(ALPHIPMC * al, double volt)
{
  return (uint16_t) ((volt - al->beta) / al->alpha);
}

/********************************************************************/
/** 
Convert Dac to Volt value
@param  dac Dac value to convert
@return double volt value
*/
double softdac_Dac2Volt (ALPHIPMC * al, uint16_t dac)
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
int softdac_BankActiveRead(ALPHIPMC * al)
{
  int stat0 = D08(al->regs+CTL_STAT_0);
  stat0 &= 0x1;
  int stat1 = D08(al->regs+CTL_STAT_1);
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
int softdac_DacWrite(ALPHIPMC * al, unsigned short *data, int * chlist, int nch)
{
  int16_t dummy = 0;

  for (int i=0; i<nch; i++) {
    D16((al->regs+DAC0102)+2*chlist[i]) = data[i];
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
int softdac_LinLoad(ALPHIPMC * al, double vin, double vout, int npts, int * offset, int ibuf, int ichan)
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
int softdac_DacVoltRead(ALPHIPMC * al, int npts, int offset, int ibuf, int ichan, int flag)
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
int  softdac_DirectDacWrite(ALPHIPMC * al, uint16_t din, int chan, double *arg)
{
  int16_t dummy = 0;

  D16((al->regs+DAC0102+(chan*2))) = din;
  printf("Dac chan:%d Reg:0x%x\n", chan,  DAC0102+(chan*2));
  return dummy;
}

/*****************************************************************/
int  softdac_DacWrite(ALPHIPMC * al, uint16_t din, int chan, double *arg)
{
  int16_t dummy = 0;

  D16((al->regs+DAC0102+(chan*2))) = din;
  printf("Dac chan:%d Reg:0x%x\n", chan,  DAC0102+(chan*2));
  return dummy;
}

/*****************************************************************/
int  softdac_DirectVoltWrite(ALPHIPMC * al, double vin, int chan, double *arg)
{
  int16_t dummy = 0;

  uint16_t din  = softdac_Volt2Dac(al, vin);
  D16((al->regs+DAC0102+(chan*2))) = din;
  printf("Volt chan:%d Reg:0x%x\n", chan,  DAC0102+(chan*2));
  return dummy;
}

/*****************************************************************/
int  softdac_SampleSet(ALPHIPMC * al, int bank, int samples)
{
  int16_t dummy = 0;
  
  if (bank == 0) {
    D32(al->regs+LAST_ADDR_0)  = samples; // last addr bank 0
  } else if (bank == 1) {
    D32(al->regs+LAST_ADDR_1)  = samples; // last addr bank 1
  }
  
  return dummy;
}

/*****************************************************************/
int  softdac_ClkSMEnable(ALPHIPMC * al, int bank)
{
  int16_t dummy = 0;
  
  if (bank == 0) {
    D08(al->regs+CTL_STAT_0)   = CLK_OUTPUT_ENABLE | EXTERNAL_CLK | SM_ENABLE; // ctrl 0
  } else if (bank == 1) {
    D08(al->regs+CTL_STAT_1)   = CLK_OUTPUT_ENABLE | EXTERNAL_CLK | SM_ENABLE; // ctrl 1
  }
  
  return dummy;
}

/*****************************************************************/
int  softdac_ClkSMDisable(ALPHIPMC * al, int bank)
{
  int16_t dummy = 0;
  
  if (bank == 0) {
    D08(al->regs+CTL_STAT_0)   = CLK_OUTPUT_ENABLE; // ctrl 0
  } else if (bank == 1) {
    D08(al->regs+CTL_STAT_1)   = CLK_OUTPUT_ENABLE; // ctrl 1
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

int  softdac_Setup(ALPHIPMC * al, int samples, int mode)
{
  int dummy;

  switch (mode) {
  case 0x1:
    printf("External Clk On, +/-10V range Stay in Bank 0 (mode:%d)\n", mode);
    D32(al->regs+CLK_SAMP_INT) = CLK_500KHZ; // Not used as external
    D32(al->regs+LAST_ADDR_0)  = samples; // last addr bank 0
    D32(al->regs+LAST_ADDR_1)  = 0; // last addr bank 1
    D08(al->regs+BANK_CTL_0)   = 0; // Stay in BANK0 ctrl
    D08(al->regs+BANK_CTL_1)   = 0; // Stay in BANK1 ctrl
    D08(al->regs+CTL_STAT_0)   = CLK_OUTPUT_ENABLE | INTERNAL_CLK | SM_ENABLE; // ctrl 0
    D08(al->regs+CTL_STAT_1)   = 0; // ctrl 1
    D32(al->regs+CMD_REG)      = 0x10 | RANGE_PM10V;
    D32(al->regs+CMD_REG)      = IMMEDIATE_MODE;
    break;

  case 0x2:
    printf("Direct Auto update to channel 1..16 (mode:%d)\n", mode);
    //    printf("\n");
    D32(al->regs+CLK_SAMP_INT) = CLK_500KHZ; // sampling clock divisor
    D32(al->regs+LAST_ADDR_0)  = samples; // last addr bank 0
    D32(al->regs+LAST_ADDR_1)  = samples; // last addr bank 1
    D08(al->regs+BANK_CTL_0)   = 0;                        // BANK0 ctrl
    D08(al->regs+BANK_CTL_1)   = 0;                        // BANK1 ctrl
    D08(al->regs+CTL_STAT_0)   = AUTO_UPDATE_DAC;          // ctrl 0
    D08(al->regs+CTL_STAT_1)   = 0;                        // ctrl 1
    D32(al->regs+CMD_REG)      = 0x10 | RANGE_PM10V;
    D32(al->regs+CMD_REG)      = IMMEDIATE_MODE;

    dummy = al->regs[0]; // flush posted PCI writes
    break;

  case 0x3:
    printf("External Clk, loop on samples, stay in bank0 (mode:%d)\n", mode);
    //    printf("\n");
    D32(al->regs+CLK_SAMP_INT) = CLK_500KHZ; // sampling clock divisor
    D32(al->regs+LAST_ADDR_0)  = samples; // last addr bank 0
    D32(al->regs+LAST_ADDR_1)  = 0; // last addr bank 1
    D08(al->regs+BANK_CTL_0)   = 0;                        // BANK0 ctrl
    D08(al->regs+BANK_CTL_1)   = 0;                        // BANK1 ctrl
    D08(al->regs+CTL_STAT_0)   = CLK_OUTPUT_ENABLE | SM_ENABLE | EXTERNAL_CLK ; // ctrl 0
    D08(al->regs+CTL_STAT_1)   = 0;                        // ctrl 1
    // Specific to the Softdac
    D32(al->regs+CMD_REG)      = 0x10 | RANGE_PM10V;
    D32(al->regs+CMD_REG)      = IMMEDIATE_MODE;

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
int  softdac_Status(ALPHIPMC * al, int mode)
{

  int stat = D32(al->regs+OFFSET+REGS_D32);
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
  ALPHIPMC *al=0;
  int dummy=0;
  int i;
  uint16 din=0;
  double arg = 0, ddout;
  int chan = 0;
  int samples = 10;

  if (argc > 1)
    chan = atoi(argv[1]) - 1;
  
  if (softdac_Open( &al) < 0)
    {
      fprintf(stderr,"Cannot open PMC-SOFTDAC device!\n");
      exit(1);
    }
  
  printf("alpha:%e\n offset:%e\n", al->alpha, al->beta);
  softdac_Status(al, 1);
  
#if 1
  if (argc == 3) {
    chan = atoi(argv[1]) - 1;
    din  = (uint16_t) atoi(argv[2]);
    samples = 0;
    softdac_Setup(al, samples, 2);

    softdac_ScaleSet(al, RANGE_PM10V, 10.0/32767., 0.0);
    printf("alpha:%e\noffset:%e\n", al->alpha, al->beta);

    for (i=0;i<8;i++) {
      ddout = 5.0;
      softdac_DirectVoltWrite(al, ddout, i, &arg);
    }
  }
  else
    printf("arg error\n");
  
#endif

#if 0
  status = softdac_LinLoad(al, -2.,2., pts,  &npts, 0, 0); npts=0;
  status = softdac_LinLoad(al, -4.,4., pts,  &npts, 0, 1); npts=0;
  status = softdac_LinLoad(al, -6.,6., pts,  &npts, 0, 2); npts=0;
  status = softdac_LinLoad(al, -10.,10., pts,  &npts, 0, 3);

  printf("......................pts:%d\n", pts);

  for (int i=0; i<4; i++)
    softdac_DacVoltRead(al, pts, 0, 0, i, 0);
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
