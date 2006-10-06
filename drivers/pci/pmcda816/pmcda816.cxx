/*
 * Test ALPHI TECHNOLOGY CORPORATION PMC-DA816
 *
 * See manual at
 * http://www.alphitech.com
 * http://www.alphitech.com/doc/pmcda816.pdf
 *
 * $Id$
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <sys/mman.h>

typedef unsigned short uint16;
typedef unsigned int   uint32;

#define D16(ptr) (*(volatile uint16*)(ptr))
#define D32(ptr) (*(volatile uint32*)(ptr))

int pmcda816_open(char** regs, char** data)
{
  FILE *fp = fopen("/dev/pmcda816","r+");
  if (fp==0)
    {
      fprintf(stderr,"Cannot open the PMC-DA816 device, errno: %d (%s)\n",
	      errno,strerror(errno));
      return -errno;
    }

  int fd = fileno(fp);

  if (regs)
    {
      *regs = (char*)mmap(0,0x1000,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
      if (*regs == NULL)
	{
	  printf("regs: 0x%p\n",regs);
	  fprintf(stderr,"Cannot mmap() PMC-DA816 registers, errno: %d (%s)\n",errno,strerror(errno));
	  return -errno;
	}
    }

  if (data)
    {
      *data = (char*)mmap(0,0x80000,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0x1000);
      if (*data == NULL)
	{
	  fprintf(stderr,"Cannot mmap() PMC-DA816 data buffer, errno: %d (%s)\n",errno,strerror(errno));
	  return -errno;
	}
    }

  return 0;
}

int pmcda816_reset(char* regs)
{
  for (int i=0; i<0x14; i+=4)
    D32(regs+i) = 0;
  D32(regs+0x14) = 0;
  D32(regs+0x16) = 0;
  D32(regs+0x18) = 0;
  D32(regs+0x10) = 0xF0000000;
  return regs[0]; // flush posted PCI writes
}

int pmcda816_getMaxSamples()
{
  return 0x4000;
}

void pmcda816_status(char* regs)
{
  printf("PMC-DA816 status:\n");
  for (int i=0; i<0x14; i+=4)
    {
      uint32 s = D32(regs+i);
      printf("regs[0x%04x] = 0x%08x\n",i,s);
    }
}

void dump_data(char* addr)
{
  printf("PMC-DA816 data dump:\n");
  for (int i=0; i<0x80000; i+=4)
    {
      uint32 s = D32(addr+i);
      printf("data[0x%06x] = 0x%08x\n",i,s);
    }
}

char* chanAddr(char* data, int ibank, int ichan)
{
  return data + ibank*0x40000 + ichan*0x8000;
}

int fillBuffer(char*data, int numSamples, int ibuf, int ichan, double*arg)
{
  int16_t dummy = 0;
  volatile int16_t* b = (int16_t*)chanAddr(data, ibuf, ichan);

  //*arg = 0;

  for (int i=0; i<numSamples; i++)
    {
      b[i] = (int16_t)(0x7000*sin(M_PI*(*arg)/20000));
      //dummy += b[i];
      (*arg) += 1.0;

      //if ((*arg) > 20000)
      //	*arg = -20000;
    }
  return dummy;
}

int main(int argc,char* argv[])
{
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  int dummy;

  int chan = 0;
  if (argc > 1)
    chan = atoi(argv[1]) - 1;

  char*regs = NULL;
  char*data = NULL;

  if (pmcda816_open(&regs, &data) < 0)
    {
      fprintf(stderr,"Cannot open PMC-DA816 device!\n");
      exit(1);
    }

  pmcda816_reset(regs);

  pmcda816_status(regs);

  /* clear data memory */
  memset(data, 0, 0x80000);
  dummy = data[0]; // flush posted PCI writes
  
  char* b0 = chanAddr(data, 0, chan);
  char* b1 = chanAddr(data, 1, chan);

  int samples = pmcda816_getMaxSamples();
  //int samples = 1000;
  for (int i=0; i<samples; i++)
    {
      D16(b0+2*i) = i/2;
    }

  for (int i=0; i<samples; i++)
    {
      D16(b1+2*i) = -i/2;
    }

  for (int i=0; i<100; i++)
    {
      D16(b0+2*i) = 0x7FFF;
    }

  double arg = 0;

  int xchan = 2;
  fillBuffer(data, samples, 0, xchan, &arg);
  fillBuffer(data, samples, 1, xchan, &arg);

  int nextBuf = 0;

  dummy = regs[0]; // flush posted PCI writes

  D32(regs+0x00) = 318; // sampling clock divisor
  D32(regs+0x08) = samples; // last addr bank 0
  D32(regs+0x0C) = samples; // last addr bank 1
  regs[0x10] = 1 | 4; // BANK0 ctrl
  regs[0x11] = 1 | 4; // BANK1 ctrl
  regs[0x12] = 0 | (1<<2) | (1<<1) | (1<<5); // ctrl 0
  regs[0x13] = 0; // ctrl 1
  dummy = regs[0]; // flush posted PCI writes

  pmcda816_status(regs);

#if 0
  while (1)
    {
      pmcda816_status(regs);
      printf("chan DAC%02d\n", 1+chan);
      sleep(1);
    }
#endif

  while (0)
    {
      int regStat0 = D32(regs+0x12);
      if (regStat0 & 1)
	write(1,"1",1);
      else
	write(1,"0",1);
    }

  int iloop = 0;
  while (1)
    {
      int stat = D32(regs+0x10);
      int activeBuf = 0;
      if (stat & 0x10000)
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

      printf("loop %d, stat 0x%08x, active %d, refill %d.", iloop, stat, activeBuf, nextBuf);
      fillBuffer(data, samples, nextBuf, xchan, &arg);
      nextBuf = 1-nextBuf;
      iloop = 0;
    }

  return dummy;
}

/* end file */
