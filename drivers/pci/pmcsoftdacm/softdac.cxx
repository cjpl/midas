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

/********************************************************************/
/** 
Open channel to pmcda16 
@param regs
@param data
@return int
*/
int softdac_open(char** regs)
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

  return 0;
}

/********************************************************************/
/** 
Reset  pmcda16 
@param regs
@return int
*/
int softdac_reset(char* regs)
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
int softdac_getMaxSamples()
{
  return 0x4000;
}

/********************************************************************/
/** 
Get the current pmcda16 status register
@param regs
@return int
*/
void softdac_status(char* regs)
{
  printf("PMC-SOFTDAC-M status:\n");
  for (int i=0; i<14; i+=4)
    {
      uint32 s = D32(regs+0x80000+i);
      printf("regs[0x%04x] = 0x%08x\n",i,s);
    }
}

/********************************************************************/
/** 
Dump pmcda16 data
@param addr
*/
void softdac_dump(char* addr)
{
  printf("PMC-SOFTDAC-M data dump:\n");
  for (int i=0; i<0x80000; i+=4)
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
char* chanAddr(char* data, int ibank, int ichan)
{
  return data + ibank*0x40000 + ichan*0x8000;
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
int da816DacWrite(char * regs, unsigned short *data, int * chlist, int nch)
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
@param data  AWG data set
@param numSamples number of samples
@param ibuf internal buffer to fill (0 or 1)
@param ichan channel to fill (0..7)
@param *arg pointer to the register control 
@return int
*/
int fillLinBuffer(char*data, int numSamples, int ibuf, int ichan, double*arg)
{
  int16_t dummy = 0;
  volatile int16_t* b = (int16_t*)chanAddr(data, ibuf, ichan);

  if (ibuf == 0) {
    //    printf("fillLin ibuf:%d b:%p\n", ibuf, b);
    for (int i=0; i<numSamples; i++)
      {
	b[i] = (int16_t)(i);
      }
  } else if (ibuf == 1) {   
    //    printf("fillLin ibuf:%d b:%p\n", ibuf, b);
    for (int i=0; i<numSamples; i++)
      {
	b[i] = (int16_t)(-2*i);
      }
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
int fillSinBuffer(char*data, int numSamples, int ibuf, int ichan, double*arg)
{
  int16_t dummy = 0;
  volatile int16_t* b = (int16_t*)chanAddr(data, ibuf, ichan);

  //*arg = 0;

  //  printf("fillSin ibuf:%d b:%p\n", ibuf, b);
  for (int i=0; i<numSamples; i++)
    {
      b[i] = 0x8000 + (int16_t)(0x7fff*sin(M_PI*(*arg)/5000000.0)*sin(M_PI*(*arg)/20));
      //dummy += b[i];
      (*arg) += 1.0;

      //if ((*arg) > 20000)
      //	*arg = -20000;
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
#if 0
int  softdac_Setup(char * regs, int samples, int mode)
{
  int dummy;

  switch (mode) {
  case 0x1:
    printf("Default setting after power up (mode:%d)\n", mode);
    printf("100KHz internal Clock, Clk Output, auto-update\n");
    D32(regs+CLK_SAMP_INT) = CLK_100KHZ; // sampling clock divisor
    D32(regs+LAST_ADDR_0)  = samples; // last addr bank 0
    D32(regs+LAST_ADDR_1)  = samples; // last addr bank 1
    D08(regs+BANK_CTL_0)   = SWITCH_AT_ADDR_0 | INT_BANK_DONE;             // BANK0 ctrl
    D08(regs+BANK_CTL_1)   = SWITCH_AT_ADDR_0 | INT_BANK_DONE;             // BANK1 ctrl
    //
    // Remove cap scs45 next to sic1 close to 10pin for getting the Clock Out!   
    D08(regs+CTL_STAT_0)   = CLK_OUTPUT_ENABLE | INTERNAL_CLK | SM_ENABLE; // ctrl 0
    D08(regs+CTL_STAT_1)   = 0; // INT_CLK_ENABLE;                         // ctrl 1
    dummy = regs[0]; // flush posted PCI writes
    break;

  case 0x2:
    printf("Direct Auto update to channel 1..16 (mode:%d)\n", mode);
    //    printf("\n");
    D32(regs+CLK_SAMP_INT) = CLK_100KHZ; // sampling clock divisor
    D32(regs+LAST_ADDR_0)  = samples; // last addr bank 0
    D32(regs+LAST_ADDR_1)  = samples; // last addr bank 1
    D08(regs+BANK_CTL_0)   = 0;                        // BANK0 ctrl
    D08(regs+BANK_CTL_1)   = 0;                        // BANK1 ctrl
    D08(regs+CTL_STAT_0)   = AUTO_UPDATE_DAC;          // ctrl 0
    D08(regs+CTL_STAT_1)   = 0;                        // ctrl 1

    dummy = regs[0]; // flush posted PCI writes
    break;

  default:
    printf("Unknown setup mode\n");
    return -1;
  }
  //    softdac_status(regs);
  return 0;
}
#endif

/*****************************************************************/
/**
*/
int  softdac_Status(char * regs, int mode)
{

  int stat = D32(regs+OFFSET+REGS_D32);
  switch (mode) {
  case 0x1:
    printf("%s:Mode Bank 0     ", (stat & 0x3) == 1 ? "1" : "?");
    printf("%s:Mode Bank 1     ", (stat & 0x300) == 0x100 ? "1" : "?");
    printf("%s:Active Bank     ", stat & 0x00010000 ? "1" : "0");
    printf("%s:Clk oupt En     ", stat & 0x00020000 ? "y" : "n");
    printf("%s:Internal Clk   \n", stat & 0x00040000 ? "y" : "n");
    printf("%s:External Clk    ", stat & 0x00080000 ? "y" : "n");
    printf("%s:SM enabled      ", stat & 0x00200000 ? "y" : "n");
    printf("%s:FP Reset  En    ", stat & 0x00400000 ? "y" : "n");
    printf("%s:Auto update    \n", stat & 0x00800000 ? "y" : "n");

    printf("%s:FP Reset State  ", stat & 0x01000000 ? "y" : "n");
    printf("%s:FP Rst Int En   ", stat & 0x02000000 ? "y" : "n");
    printf("%s:Clk Int En     \n", stat & 0x04000000 ? "y" : "n");
    printf("%s:Int B0  done    ", stat & 0x10000000 ? "y" : "n");
    printf("%s:Int B1  done    ", stat & 0x20000000 ? "y" : "n");
    printf("%s:Int clk done    ", stat & 0x40000000 ? "y" : "n");
    printf("%s:Int FP  done   \n", stat & 0x80000000 ? "y" : "n");

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
int main(int argc,char* argv[])
{

  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  int dummy;

  int chan = 0;
  if (argc > 1)
    chan = atoi(argv[1]) - 1;

  char *regs = NULL;
  unsigned short  vdac[16];  
  int chlist[16];

  if (softdac_open(&regs) < 0)
    {
      fprintf(stderr,"Cannot open PMC-SOFTDAC-M device!\n");
      exit(1);
    }

  softdac_reset(regs);

  softdac_status(regs);

#if 0
  while (1)
    {
      softdac_status(regs);
      printf("chan DAC%02d\n", 1+chan);
      sleep(1);
    }
#endif

  /* clear data memory */
  memset(regs, 0, 0x80000);
  dummy = regs[0]; // flush posted PCI writes
  
  int samples = softdac_getMaxSamples();

  double arg = 0;
  int xchan = 1;
  fillLinBuffer(regs, samples, 0, chan, &arg);
  fillLinBuffer(regs, samples, 1, chan, &arg);

  arg = 0;
  xchan = 0;
  fillSinBuffer(regs, samples, 0, xchan, &arg);
  fillSinBuffer(regs, samples, 1, xchan, &arg);

  dummy = regs[0]; // flush posted PCI writes

  //softdac_Setup(regs, samples, 1);

#if 1
  D32(regs + 0x80048) = 0x1B;
  D32(regs + 0x80020) = 0xf00f;
  D32(regs + 0x80048) = 0x02;

  D32(regs+CLK_SAMP_INT) = CLK_500KHZ; // sampling clock divisor
  D32(regs+LAST_ADDR_0)  = samples; // last addr bank 0
  D32(regs+LAST_ADDR_1)  = samples; // last addr bank 1
  D08(regs+BANK_CTL_0)   = SWITCH_AT_ADDR_0 | INT_BANK_DONE;             // BANK0 ctrl
  D08(regs+BANK_CTL_1)   = SWITCH_AT_ADDR_0 | INT_BANK_DONE;             // BANK1 ctrl
  D08(regs+CTL_STAT_0)   = CLK_OUTPUT_ENABLE | INTERNAL_CLK | SM_ENABLE; // ctrl 0
  D08(regs+CTL_STAT_1)   = 0;                                            // ctrl 1

  dummy = D32(regs+CLK_SAMP_INT); // flush posted PCI writes

  softdac_status(regs);
#endif

  while (0)
    {
      int regStat0 = D32(regs+OFFSET+0x12);
      if (regStat0 & 1)
	write(1,"1",1);
      else
	write(1,"0",1);
    }

#if 0
  softdac_Setup(regs, 0, 2);
  for (int i=0;i<4;i++) {
    vdac[i] = 20000*(i+1);
    chlist[i] = i;
  }
  da816DacWrite(regs, vdac, chlist, 4);

#endif
  softdac_Status(regs, 1);

#if 1
  int iloop = 0;
  int nextBuf = 0;
  while (1)
    {
      int stat = D32(regs+OFFSET+REGS_D32);
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

      //softdac_Status(regs, 2);
      printf("loop %d, stat 0x%08x, active %d, refill %d.", iloop, stat, activeBuf, nextBuf);
      fillSinBuffer(regs, samples, nextBuf, xchan, &arg);
      nextBuf = 1-nextBuf;
      iloop = 0;
    }
#endif

  return dummy;
}

#endif
/* end file */
