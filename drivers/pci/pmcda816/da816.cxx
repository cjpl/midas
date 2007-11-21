/*********************************************************************

  Name:         da816.cxx
  Created by:   Konstantin Olchanski

  Cotents:      Routines for accessing the AWG PMC-DA816 board
                from ALPHI TECHNOLOGY CORPORATION

 * See manual at
 * http://www.alphitech.com
 * http://www.alphitech.com/doc/pmcda816.pdf
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

#include "da816.h"

/********************************************************************/
/**
Set the alpha/beta coefficients for the Volt2Dac conversion.
Only one set of param for all the channels.
@param alpha conversion slope coeff
@param beta  conversion offset coeff
@return int
*/
int da816_ScaleSet(ALPHIDA816 * da816, double alpha, double beta)
{
  da816->alpha = alpha;
  da816->beta  = beta;
  return (0);
}

/********************************************************************/
/**
Open channel to pmcda16
@param regs pointer to register area
@param data pointer to data buffer area
@return int status
*/
int da816_Open(ALPHIDA816 ** da816)
{
  int dummy;

  // Book space
  *da816 = (ALPHIDA816 *) calloc(1, sizeof(ALPHIDA816));

  FILE *fp = fopen("/dev/pmcda816","r+");
  if (fp==0)
    {
      fprintf(stderr,"Cannot open the PMC-DA816 device, errno: %d (%s)\n",
        errno,strerror(errno));
      return -errno;
    }

  int fd = fileno(fp);

  (*da816)->regs = (char*)mmap(0,0x1000,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
  if ((*da816)->regs == NULL)
    {
      printf("regs: 0x%p\n",(*da816)->regs);
      fprintf(stderr,"Cannot mmap() PMC-DA816 registers, errno: %d (%s)\n",errno,strerror(errno));
      return -errno;
    }

  (*da816)->data = (char*)mmap(0,0x80000,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0x1000);
  if ((*da816)->data == NULL)
    {
      fprintf(stderr,"Cannot mmap() PMC-DA816 data buffer, errno: %d (%s)\n",errno,strerror(errno));
      return -errno;
    }
  
  fclose(fp);

  //memset((*da816)->data, 0, 0x80000);

  // Set initial scaling coefficients
  da816_ScaleSet(*da816, 10.0/32767. , 0.0);
  
  dummy = (*da816)->data[0]; // flush posted PCI writes
  return 0;
}

/********************************************************************/
/**
Reset  all the registers of the pmcda16
@param regs pointer to registers area
@return int
*/
void da816_Close(ALPHIDA816 * da816)
{
  if (da816) {
    if (da816->regs) munmap(da816->regs,0x1000);
    if (da816->data) munmap(da816->data,0x80000);
    free (da816);
    da816 = NULL;
  }
}

/********************************************************************/
/**
Reset  all the registers of the pmcda16
@param regs pointer to registers area
@return int
*/
int da816_Reset(ALPHIDA816 * da816)
{
  for (int i=0; i<0x14; i+=4)
    DA816_D16(da816->regs+i) = 0;
  DA816_D16(da816->regs+DA816_CLK_SAMPLE_RESET) = 0;
  DA816_D16(da816->regs+DA816_ADDRESS_RESET) = 0;
  DA816_D16(da816->regs+DA816_DACS_RESET) = 0;
  DA816_D16(da816->regs+DA816_BANK_CTL_0) = 0x0;
  DA816_D16(da816->regs+DA816_CTL_STAT_0) = 0x0;
  return da816->regs[0]; // flush posted PCI writes
}

/********************************************************************/
/**
Reset Address only of the pmcda16
@param regs pointer to registers area
@return int
*/
int da816_AddReset(ALPHIDA816 * da816)
{
  DA816_D16(da816->regs+DA816_ADDRESS_RESET) = 1;
  //  DA816_D16(da816->regs+DA816_DACS_RESET)    = 1;
  //  DA816_D16(da816->regs+DA816_DACS_UPDATES)  = 1;
  return da816->regs[0]; // flush posted PCI writes
}

/********************************************************************/
/**
Switch to bank 0 or 1
@return int
*/
int da816_BankSwitch(ALPHIDA816 * da816, int onoff)
{
  DA816_D32(da816->regs+DA816_SWITCH_BANKS) = onoff;
  return da816->regs[0]; // flush posted PCI writes
}
/********************************************************************/
/**
Update the Dac (by hand) before the first clock
@return int
*/
int da816_Update(ALPHIDA816 * da816)
{
  DA816_D08(da816->regs+DA816_CTL_STAT_0) = DA816_AUTO_UPDATE_DAC;
  return da816->regs[0]; // flush posted PCI writes
}

/********************************************************************/
/**
Get maximum number of samples
@return int maximim of samples per channel
*/
int da816_MaxSamplesGet()
{
  return 0x4000;
}

/********************************************************************/
/**
Convert Volt to Dac value
@param  volt voltage to convert
@return int  dac value
*/
int16_t da816_Volt2Dac(ALPHIDA816 * da816, double volt)
{
  return (int16_t) ((volt - da816->beta) / da816->alpha);
}

/********************************************************************/
/**
Convert Dac to Volt value
@param  dac Dac value to convert
@return double volt value
*/
double da816_Dac2Volt (ALPHIDA816 * da816, int16_t dac)
{
  return (double) (dac * da816->alpha + da816->beta);
}

/********************************************************************/
/**
Dump pmcda16 data
@param addr
*/
void dump_data(ALPHIDA816 * da816)
{
  printf("PMC-DA816 data dump:\n");
  for (int i=0; i<0x80000; i+=4)
    {
      uint32_t s = DA816_D32(da816->regs+i);
      printf("data[0x%06x] = 0x%08x\n",i,s);
    }
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
char* chanAddr(char* data, int ibank, int ichan, int offset)
{
  return data + ibank*0x40000 + ichan*0x8000 + 2*offset;
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
int da816_BankActiveRead(ALPHIDA816 * da816)
{
      int stat0 = DA816_D08(da816->regs+DA816_CTL_STAT_0);
      stat0 &= 0x1;
      int stat1 = DA816_D08(da816->regs+DA816_CTL_STAT_1);
      stat1 &= 0x1;
      return (stat0 | (stat1<<1));

}

/********************************************************************/
/**
Write to DACx directly for mode 2
@return int
*/
int da816_DacWrite(ALPHIDA816 * da816, short *data, int * chlist, int nch)
{
  int16_t dummy = 0;

  for (int i=0; i<nch; i++) {
    DA816_D16((da816->regs+DA816_DAC0102)+2*chlist[i]) = data[i];
  }

  return dummy;
}

/********************************************************************/
/**
Fill particular buffer with nSamples of data
@param data  AWG data set
@param numSamples number of samples
@param ibuf internal buffer to fill (0 or 1)
@param ichan channel to fill (0..7)
@param *arg pointer to the register control
@return int
*/
int fillSinBuffer(ALPHIDA816 * da816 , int numSamples, int ibuf, int ichan, double*arg)
{
  int16_t dummy = 0;
  volatile int16_t* b = (int16_t*)chanAddr(da816->data, ibuf, ichan, 0);

  //  printf("fillSin ibuf:%d b:%p\n", ibuf, b);
  for (int i=0; i<numSamples; i++)
    {
      b[i] = (int16_t)(0x7000*sin(M_PI*(*arg)/20));
      //dummy += b[i];
      (*arg) += 1.0;

      //if ((*arg) > 20000)
      //  *arg = -20000;
    }
  return dummy;
}

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
int da816_LinLoad(ALPHIDA816 * da816, double vin, double vout, int npts, int * offset, int ibuf, int ichan)
{
  printf("LinLoad: buf %d, chan %d, offset %d, from %f to %f in %d steps\n", ibuf, ichan, *offset, vin, vout, npts);

  int16_t dummy = 0;
  int16_t increment = 0;
  volatile int16_t* b = (int16_t*)chanAddr(da816->data, ibuf, ichan, *offset);

  int16_t din  = da816_Volt2Dac(da816, vin);
  int16_t dout = da816_Volt2Dac(da816, vout);
  printf("LinLoad b: %p npts:%06d vin:%f din:%d vout:%f dout:%d\n", b, npts, vin, din, vout, dout);

  if (npts == 1) {
    b[0] = (int16_t) din;
    printf("Volt single Dac[0]:%i\n", b[0]);
  } else {
    increment =  (dout-din) / (npts-1);
    for (int i=0; i<npts; i++)
      {
	b[i] = din + (i * increment);
	//b[i] = 3000*(i&1);
	printf("b:%p Volt dac[%i]:%d\n", &(b[i]), i, b[i]);
      }
  }
  *offset += npts;

#if 0  
  for (i=0; i<0x80000; i+=2)
    {
      *(int16_t*)(da816->data + i) = 3000;
    }
#endif
  
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
int da816_DacVoltRead(ALPHIDA816 * da816, int npts, int offset, int ibuf, int ichan, int flag)
{
  int16_t dummy = 0;
  volatile int16_t* b = (int16_t*)chanAddr(da816->data, ibuf, ichan, offset);
  FILE *fin = 0;

  if (flag) {
    char filename[64];
    sprintf(filename, "da816-ch%02d.log", ichan);
    fin = fopen(filename, "a");
    fprintf(fin, "-------%s --------------------------\n", filename);
  }

  for (int i=0; i<npts; i++) {
    double volt = da816_Dac2Volt(da816, b[i]);
    if (flag)
      fprintf(fin, "Ch:%d Buf:%d Off:%d Dac:%d Volt:%f \n", ichan, ibuf, offset+i, b[i], volt);
    printf("Mod:Ch:%d Buf:%d Off:%d Dac:%d Volt:%f \n", ichan, ibuf, offset+i, b[i], volt);
  }

  if (flag)
    fclose(fin);
  return dummy;
}

/*****************************************************************/
int  da816_DirectDacWrite(ALPHIDA816 * da816, int16_t din, int chan)
{
  int16_t dummy = 0;

  DA816_D16((da816->regs+DA816_DAC0102+(chan*2))) = din;
  return dummy;
}

/*****************************************************************/
int  da816_DirectVoltWrite(ALPHIDA816 * da816, double vin, int chan, double *arg)
{
  int16_t dummy = 0;

  int16_t din  = da816_Volt2Dac(da816, vin);
  DA816_D16((da816->regs+DA816_DAC0102+(chan*2))) = din;
  printf("Volt:%e Dac: %d chan:%d Reg:0x%x  ", vin, din, chan, DA816_DAC0102+(chan*2));
  printf("Dac read: %d\n", DA816_D16(da816->regs+DA816_DAC0102+(chan*2)));
  return dummy;
}

/*****************************************************************/
int  da816_SampleSet(ALPHIDA816 * da816, int bank, int samples)
{
  int16_t dummy = 0;

  printf("da816_SampleSet: bank %d, samples %d\n", bank, samples);

  if (bank == 0) {
    DA816_D32(da816->regs+DA816_LAST_ADDR_0)  = samples-1; // last addr bank 0
  } else if (bank == 1) {
    DA816_D32(da816->regs+DA816_LAST_ADDR_1)  = samples-1; // last addr bank 1
  }

  return dummy;
}

/*****************************************************************/
int  da816_ClkSMEnable(ALPHIDA816 * da816, int bank)
{
  int16_t dummy = 0;

  DA816_D08(da816->regs+DA816_CTL_STAT_0)   = DA816_CLK_OUTPUT_ENABLE | DA816_EXTERNAL_CLK | DA816_SM_ENABLE | DA816_AUTO_UPDATE_DAC;

  return dummy;
}

/*****************************************************************/
int  da816_ClkSMDisable(ALPHIDA816 * da816, int bank)
{
  int16_t dummy = 0;

  if (bank == 0) {
    DA816_D08(da816->regs+DA816_CTL_STAT_0)   = DA816_CLK_OUTPUT_ENABLE; // ctrl 0
  } else if (bank == 1) {
    DA816_D08(da816->regs+DA816_CTL_STAT_1)   = DA816_CLK_OUTPUT_ENABLE; // ctrl 1
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
int  da816_Setup(ALPHIDA816 * da816, int samples, int mode)
{
  int dummy;

  printf("da816_Setup: Set mode %d with %d samples\n", mode, samples);

  dummy = DA816_D08(da816->regs+DA816_CTL_STAT_0);
  if ((dummy & 1) == 0) {
    printf("da816_Setup: NOTICE: Enable writing to bank 0 (set active bank 1)\n");
    da816_BankSwitch(da816, 9999);
  }

  switch (mode) {
  case 0x1:
    printf("Default setting after power up (mode:%d)\n", mode);
    printf("100KHz internal Clock, Clk Output, auto-update\n");
    DA816_D32(da816->regs+DA816_CLK_SAMP_INT) = DA816_CLK_10KHZ; // sampling clock divisor
    DA816_D32(da816->regs+DA816_LAST_ADDR_0)  = samples; // last addr bank 0
    DA816_D32(da816->regs+DA816_LAST_ADDR_1)  = samples; // last addr bank 1
    DA816_D08(da816->regs+DA816_BANK_CTL_0)   = DA816_SWITCH_AT_ADDR_0 | DA816_INT_BANK_DONE;             // BANK0 ctrl
    DA816_D08(da816->regs+DA816_BANK_CTL_1)   = DA816_SWITCH_AT_ADDR_0 | DA816_INT_BANK_DONE;             // BANK1 ctrl
    //
    // Remove cap scs45 next to sic1 close to 10pin for getting the Clock Out!
    DA816_D08(da816->regs+DA816_CTL_STAT_0)   = DA816_CLK_OUTPUT_ENABLE | DA816_INTERNAL_CLK | DA816_SM_ENABLE; // ctrl 0
    DA816_D08(da816->regs+DA816_CTL_STAT_1)   = 0; // DA816_INT_CLK_ENABLE;                         // ctrl 1
    dummy = da816->regs[0]; // flush posted PCI writes
    break;

  case 0x2:
    printf("Direct Auto update to channel 1..16 (mode:%d)\n", mode);
    //    printf("\n");
    DA816_D32(da816->regs+DA816_CLK_SAMP_INT) = DA816_CLK_100KHZ; // sampling clock divisor
    //    DA816_D32(da816->regs+DA816_LAST_ADDR_0)  = samples; // last addr bank 0
    //   DA816_D32(da816->regs+DA816_LAST_ADDR_1)  = samples; // last addr bank 1
    DA816_D08(da816->regs+DA816_BANK_CTL_0)   = 0;                        // BANK0 ctrl
    DA816_D08(da816->regs+DA816_BANK_CTL_1)   = 0;                        // BANK1 ctrl
    DA816_D08(da816->regs+DA816_CTL_STAT_0)   = DA816_INTERNAL_CLK | DA816_AUTO_UPDATE_DAC;          // ctrl 0
    DA816_D08(da816->regs+DA816_CTL_STAT_1)   = 0;                        // ctrl 1

    dummy = da816->regs[0]; // flush posted PCI writes
    break;

  case 0x3:
    printf("External Clk, loop on samples, stay in bank0 (mode:%d)\n", mode);
    //    printf("\n");
    DA816_D32(da816->regs+DA816_CLK_SAMP_INT) = DA816_CLK_100KHZ; // sampling clock divisor
    DA816_D32(da816->regs+DA816_LAST_ADDR_0)  = samples; // last addr bank 0
    DA816_D32(da816->regs+DA816_LAST_ADDR_1)  = 0; // samples; // last addr bank 1
    DA816_D08(da816->regs+DA816_BANK_CTL_0)   = 0; // DA816_SWITCH_AT_ADDR_0;             // BANK0 ctrl
    DA816_D08(da816->regs+DA816_BANK_CTL_1)   = 0; // DA816_SWITCH_AT_ADDR_0;             // BANK1 ctrl
    DA816_D08(da816->regs+DA816_CTL_STAT_0)   = DA816_CLK_OUTPUT_ENABLE | DA816_SM_ENABLE | DA816_EXTERNAL_CLK | DA816_AUTO_UPDATE_DAC; // ctrl 0
    DA816_D08(da816->regs+DA816_CTL_STAT_1)   = 0; // ctrl 1
    
    printf("Setup: LAST_ADDR_0: %d, LAST_ADDR_1: %d, BANK_CTL_0: 0x%02x, BANK_CTL_1: 0x%02x, CTL_STAT_0: 0x%02x, CTL_STAT_1: 0x%02x\n",
	   DA816_D32(da816->regs+DA816_LAST_ADDR_0),
	   DA816_D32(da816->regs+DA816_LAST_ADDR_1),
	   DA816_D08(da816->regs+DA816_BANK_CTL_0),
	   DA816_D08(da816->regs+DA816_BANK_CTL_1),
	   DA816_D08(da816->regs+DA816_CTL_STAT_0),
	   DA816_D08(da816->regs+DA816_CTL_STAT_1));

    dummy = da816->regs[0]; // flush posted PCI writes
    break;

  case 0x4:
    printf("Internal Clk, switch to other bank at 0 (mode:%d)\n", mode);
    //    printf("\n");
    DA816_D32(da816->regs+DA816_CLK_SAMP_INT) = DA816_CLK_100KHZ; // Not used as external
    DA816_D32(da816->regs+DA816_LAST_ADDR_0)  = samples-1; // last addr bank 0
    DA816_D32(da816->regs+DA816_LAST_ADDR_1)  = samples-1; // last addr bank 1
    DA816_D08(da816->regs+DA816_BANK_CTL_0)   = 1; // DA816_SWITCH_AT_ADDR_0;             // Stay in BANK0 ctrl
    DA816_D08(da816->regs+DA816_BANK_CTL_1)   = 0; // DA816_SWITCH_AT_ADDR_0;             // Stay in BANK1 ctrl
    DA816_D08(da816->regs+DA816_CTL_STAT_0)   = DA816_CLK_OUTPUT_ENABLE | DA816_INTERNAL_CLK | DA816_SM_ENABLE; // ctrl 0
    DA816_D08(da816->regs+DA816_CTL_STAT_1)   = 0; 
    dummy = da816->regs[0]; // flush posted PCI writes
    break;

  case 0x5:
    printf("Internal Clk Off, (mode:%d)\n", mode);
    //    printf("\n");
    DA816_D32(da816->regs+DA816_CLK_SAMP_INT) = DA816_CLK_100KHZ; // Not used as external
    DA816_D32(da816->regs+DA816_LAST_ADDR_0)  = samples; // last addr bank 0
    DA816_D32(da816->regs+DA816_LAST_ADDR_1)  = samples; // last addr bank 1
    DA816_D08(da816->regs+DA816_BANK_CTL_0)   = 0; // DA816_SWITCH_AT_ADDR_0;             // Stay in BANK0 ctrl
    DA816_D08(da816->regs+DA816_BANK_CTL_1)   = 0; // DA816_SWITCH_AT_ADDR_0;             // Stay in BANK1 ctrl
    DA816_D08(da816->regs+DA816_CTL_STAT_0)   = DA816_CLK_OUTPUT_ENABLE | DA816_SM_ENABLE | DA816_INTERNAL_CLK ; // ctrl 0
    DA816_D08(da816->regs+DA816_CTL_STAT_1)   = 0; // ctrl 1
    dummy = da816->regs[0]; // flush posted PCI writes
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

int  da816_Status(ALPHIDA816 * da816, int mode)
{
  int stat = DA816_D32(da816->regs+DA816_REGS_D32);
  switch (mode) {
  case 0x1:
    printf("PMC-DA816: Status reg : %p : 0x%08x\n",da816->regs+DA816_REGS_D32, stat);
    printf("%d:Mode Bank 0     ", (stat & 0x3));
    printf("%d:Mode Bank 1     ", (stat & 0x300) >> 8);
    printf("%s:Active Bank    \n", stat & 0x00010000 ? "1" : "0");
    printf("%s:Clk oupt En     ",  stat & 0x00020000 ? "y" : "n");
    printf("%s:Internal Clk    ",  stat & 0x00040000 ? "y" : "n");
    printf("%s:External Clk   \n", stat & 0x00080000 ? "y" : "n");
    printf("%s:Underflow       ",  stat & 0x00100000 ? "y" : "n");
    printf("%s:SM enabled      ",  stat & 0x00200000 ? "y" : "n");
    printf("%s:FP Reset  En    ",  stat & 0x00400000 ? "y" : "n");
    printf("%s:Auto update    \n", stat & 0x00800000 ? "y" : "n");

    printf("%s:FP Reset State  ",  stat & 0x01000000 ? "y" : "n");
    printf("%s:FP Rst Int En   ",  stat & 0x02000000 ? "y" : "n");
    printf("%s:Clk Int En     \n", stat & 0x04000000 ? "y" : "n");
    printf("%s:Int B0  done    ",  stat & 0x10000000 ? "y" : "n");
    printf("%s:Int B1  done    ",  stat & 0x20000000 ? "y" : "n");
    printf("%s:Int clk done    ",  stat & 0x40000000 ? "y" : "n");
    printf("%s:Int FP  done   \n", stat & 0x80000000 ? "y" : "n");
    printf("Clock divisor: %d\n", DA816_D32(da816->regs+DA816_CLK_SAMP_INT));
    printf("SM address: %d\n", DA816_D32(da816->regs+DA816_ADDRESS_SM));
    printf("Last addr bank0: %d, bank1: %d\n", DA816_D32(da816->regs+DA816_LAST_ADDR_0), DA816_D32(da816->regs+DA816_LAST_ADDR_1));

    break;
  case 0x2:
    printf("%s:FP Reset State  ", stat & 0x01000000 ? "y" : "n");
    printf("%s:FP Rst Int En   ", stat & 0x02000000 ? "y" : "n");
    printf("%s:Clk Int Enabe   ", stat & 0x04000000 ? "y" : "n");
    printf("%s:Int B0 done     ", stat & 0x10000000 ? "y" : "n");
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
int main(int argc, char* argv[])
{
  ALPHIDA816 *al=0;
  int dummy=0;
  int i;
  int16_t din=0;
  double arg = 0, ddout;
  int chan = 0;

  int    samples = 10;

  if (argc > 1)
    chan = atoi(argv[1]) - 1;

  if (da816_Open( &al) < 0)
    {
      fprintf(stderr,"Cannot open PMC-DA816 device!\n");
      exit(1);
    }

  printf("alpha:%e\n offset:%e\n", al->alpha, al->beta);
  da816_Status(al, 1);


#if 1
  if (argc == 3) {
    chan = atoi(argv[1]);
    //    din  = (int16_t) atoi(argv[2]);
    ddout  = (double) atof(argv[2]);
    
    samples = 0;

  }
  da816_Setup(al, samples, 2);
  
  printf("alpha:%e\noffset:%e\n", al->alpha, al->beta);
  
  for (i=0;i<8;i++) {
    da816_DirectVoltWrite(al, ddout, i, &arg);
    //    da816_DirectDacWrite(al, (int16_t)ddout, i); // , &arg);
  }
  DA816_D32(al->regs+DA816_DACS_UPDATES) = 0;
  da816_Status(al, 1);  
#endif


#if 0
  int status = 0;
  da816_Reset(al);
  DA816_D08(al->regs+DA816_CTL_STAT_0)   = 0; // ctrl 0
  DA816_D08(al->regs+DA816_CTL_STAT_1)   = 0; // ctrl 0
  /* clear data memory */
  printf("Active banks: %d\n", da816_BankActiveRead(al));
  int pts = 0;

  status = da816_LinLoad( al, 10.0 , -10., 11,  &pts, 1, 2); pts = 0;
  status = da816_LinLoad( al, 10.0 , -10., 21,  &pts, 0, 2);
  printf("pts:%d\n", pts);

  da816_DacVoltRead(al, pts, 0, 0, 2, 1);
 
  printf("Active banks: %d\n", da816_BankActiveRead(al));
  samples = pts;
  
  status = da816_Setup(al, samples, 4);
  status = da816_SampleSet(al, 1, 11);
  status = da816_SampleSet(al, 0, samples);

  printf("Active banks: %d\n", da816_BankActiveRead(al));

#endif

  da816_Close(al);
  return dummy;
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

/* end file */
