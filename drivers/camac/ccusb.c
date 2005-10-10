// CC-USB test harness


#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "musbstd.h"
#include "ccusb.h"
#include "mcstd.h"

#define MAX_CCUSB 32

static int   gNumCrates = 0;
static int   gSerNo[MAX_CCUSB];
static void* gHandle[MAX_CCUSB];

#define D16  0
#define D24  1

#define REG_VERSION 0
#define REG_LAMMASK 8
#define REG_LAM    12
#define REG_SERNO  15

int ccusb_init_crate(int c)
{
  int version;

  gHandle[c] = musb_open(0x16dc,0x0001,c,1,0);
  if (!gHandle[c])
    return -1;
  
  ccusb_flush(gHandle[c]);
  
  version   = ccusb_readReg(gHandle[c],REG_VERSION);
  gSerNo[c] = ccusb_readReg(gHandle[c],REG_SERNO);
  
  fprintf(stderr,"ccusb_init_crate: CAMAC crate %d is CCUSB instance %d, serial number %d, firmware 0x%x\n",c,c,gSerNo[c],version);

  if (c >= gNumCrates)
    gNumCrates = c+1;

  return 0;
}

int ccusb_init()
{
  int i;
  for (i=0; i<MAX_CCUSB; i++)
    {
      int status = ccusb_init_crate(i);
      if (status != 0)
	break;
    }

  if (gNumCrates < 1)
    {
      fprintf(stderr,"ccusb_init: Did not find any CC-USB CAMAC interfaces.\n");
      return -1;
    }

  return 0;
}

void* ccusb_getCrateHandle(int crate)
{
  assert(crate>=0);
  assert(crate<gNumCrates);
  assert(gHandle[crate]!=NULL);
  return gHandle[crate];
}

int ccusb_flush(void*handle)
{
  int count, rd;
  uint8_t buf[16*1024];

  for (count=0; count<1000; count++)
    {
      rd = musb_read(handle,0x86,buf,sizeof(buf),100);
      if (rd < 0)
	return 0;

      printf("ccusb_flush: count=%d, rd=%d, buf: 0x%02x 0x%02x 0x%02x 0x%02x\n",count,rd,buf[0],buf[1],buf[2],buf[3]);
    }

  ccusb_status(handle);

  printf("ccusb_flush: CCUSB is babbling. Please reset it by cycling power on the CAMAC crate.\n");

  musb_reset(handle);

  ccusb_status(handle);

  exit(1);

  return -1;
}

int ccusb_recover(void*handle)
{
  int status;
  int i;
  for (i=0; i<gNumCrates; i++)
    if (gHandle[i] == handle)
      {
	musb_close(handle);
	status = ccusb_init_crate(i);
	if (status == 0)
	  return 0;

	printf("ccusb-recover: Cannot reinitialize crate %d\n",i);
	exit(1);
      }

  assert(!"ccusb_recover: Cannot find handle!\n");
  return -1;
}

int ccusb_readReg(void*handle,int ireg)
{
  int rd, wr;
  static uint8_t  cmd[4];
  static uint16_t buf[4];
  cmd[0] = 1;
  cmd[1] = 0;
  cmd[2] = ireg;
  cmd[3] = 0;

  while (1)
    {
      wr = musb_write(handle,2,cmd,4,100);
      if (wr != 4)
	{
	  printf("ccusb_readReg: reg %d, musb_write() error %d\n",ireg,wr);
	  exit(1);
	  return -1;
	}

      rd = musb_read(handle,0x86,buf,8,100);
      if (rd < 0)
	{
	  printf("ccusb_readReg: reg %d, timeout: wr=%d, rd=%d, buf: 0x%x 0x%x 0x%x 0x%x\n",ireg,wr,rd,buf[0],buf[1],buf[2],buf[3]);
	  continue;
	}

      if (rd == 2)
	return buf[0];
      else if (rd == 4)
	return (buf[0] | (buf[1]<<16));
      else
	{
	  printf("ccusb_readReg: reg %d, unexpected data length: wr=%d, rd=%d, buf: 0x%x 0x%x 0x%x 0x%x\n",ireg,wr,rd,buf[0],buf[1],buf[2],buf[3]);
	  return -1;
	}
    }
}

int ccusb_writeReg(void*handle,int ireg,int value)
{
  int rd, wr;
  static uint8_t  cmd[8];
  cmd[0] = 5;
  cmd[1] = 0;
  cmd[2] = ireg;
  cmd[3] = 0;
  cmd[4] = value&0xFF;
  cmd[5] = (value>>8)&0xFF;
  cmd[6] = (value>>16)&0xFF;
  cmd[7] = (value>>24)&0xFF;

  wr = musb_write(handle,2,cmd,8,100);
  if (wr != 8)
    {
      printf("ccusb_writeReg: reg %d, musb_write() error %d\n",ireg,wr);
      exit(1);
      return -1;
    }

  rd = ccusb_readReg(handle,ireg);
  if (rd < 0)
    {
      printf("ccusb_writeReg: Register %d readback failed with error %d\n",ireg,rd);
      return -1;
    }

  if (rd != value)
    {
      printf("ccusb_writeReg: Register %d readback mismatch: wrote 0x%x, got 0x%x\n",ireg,value,rd);
      return -1;
    }

  return value;
}

int ccusb_reset(void*handle)
{
  int rd, wr;
  static uint8_t cmd[512];
  static uint16_t buf[512];
  cmd[0] = 255;
  cmd[1] = 255;
  cmd[2] = 0;
  cmd[3] = 0;
  wr = musb_write(handle,2,cmd,2,100);
  printf("reset: wr=%d\n",wr);
  return -1;
}

int ccusb_status(void*handle)
{
  int reg1;

  //ccusb_flush(handle);

  printf("CCUSB status:\n");
  printf("  reg0  firmware id: 0x%04x\n",ccusb_readReg(handle,0));
  reg1=ccusb_readReg(handle,1);
  printf("  reg1  global mode: 0x%04x: ",reg1);
  printf(" BuffOpt=%d,",reg1&7);
  printf(" EvtSepOpt=%d,",(reg1>>6)&1);
  printf(" HeaderOpt=%d,",(reg1>>8)&1);
  printf(" WdgFrq=%d,",(reg1>>9)&7);
  printf(" Arbitr=%d",(reg1>>12)&1);
  printf("\n");
  printf("  reg2  delays:      0x%04x\n",ccusb_readReg(handle,2));
  printf("  reg3  watchdog:    0x%04x\n",ccusb_readReg(handle,3));
  printf("  reg4  sel LED-B:   0x%04x\n",ccusb_readReg(handle,4));
  printf("  reg5  scaler freq: 0x%04x\n",ccusb_readReg(handle,5));
  printf("  reg6  sel LED-A:   0x%04x\n",ccusb_readReg(handle,6));
  printf("  reg7  NIM out:     0x%04x\n",ccusb_readReg(handle,7));
  printf("  reg8  LAM mask:  0x%06x\n",ccusb_readReg(handle,8));
  printf("  reg9  unknown:     0x%04x\n",ccusb_readReg(handle,9));
  printf("  reg10 action reg:  0x%04x\n",ccusb_readReg(handle,10));
  printf("  reg11 unknown:     0x%04x\n",ccusb_readReg(handle,11));
  printf("  reg12 LAM status:0x%06x\n",ccusb_readReg(handle,12));
  printf("  reg13 serial no:   0x%04x\n",ccusb_readReg(handle,13));
  printf("  reg14 unknown:     0x%04x\n",ccusb_readReg(handle,14));
  printf("  reg15 serial no:   0x%04x\n",ccusb_readReg(handle,15));

  return 0;
}

int ccusb_naf(void*handle,int n,int a,int f,int d24,int data)
{
  int rd, wr, bcount;
  static uint8_t cmd[16];
  static uint8_t buf[16];
  int naf = f | (a<<5) | (n<<9);

  if (d24) naf |= 0x4000;

  bcount = 6;
  cmd[0] = 12; // address
  cmd[1] = 0;
  cmd[2] = 1;  // word count
  cmd[3] = 0;
  cmd[4] = (naf&0x00FF);       // NAF low bits
  cmd[5] = (naf&0xFF00) >> 8;  // NAF high bits

  //printf("naf %d %d %d 0x%02x, d24=%d\n",n,a,f,f,d24);

  if ((f & 0x18) == 0x10) // send data for write commands
    {
      //printf("send write data 0x%x\n",data);
      bcount += 2;
      cmd[2] = 2;
      cmd[6] = (data&0x00FF);      // write data, low 8 bits
      cmd[7] = (data&0xFF00) >> 8; // write data, high 8 bits
      if (d24)
	{
	  bcount += 2;
	  cmd[2] = 3;
	  cmd[8] = (data&0xFF0000) >> 16; // write data, highest 8 bits
	  cmd[9] = 0;
	}
    }

  wr = musb_write(handle,2,cmd,bcount,1000);
  if (wr != bcount)
    {
      printf("ccusb_naf: musb_write() error %d\n",wr);
      exit(1);
      return -1;
    }

  rd = musb_read(handle,0x86,buf,sizeof(buf),1000);

  if (((f & 0x18) == 0)) // data from read commands
    {
      if (d24==D24)
	{
	  if (rd != 6)
	    {
	      printf("ccusb_naf: Reply to read command: Unexpected data length, should be 6: wr=%d, rd=%d, buf: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",wr,rd,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);

	      ccusb_flush(handle);
	      ccusb_status(handle);

	      return -1;
	    }
	  
	  return buf[0] | (buf[1]<<8)| (buf[2]<<16)| (buf[3]<<24);
	}
      else
	{
	  if (rd != 4)
	    {
	      printf("ccusb_naf: Reply to read command: Unexpected data length, should be 4: wr=%d, rd=%d, buf: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",wr,rd,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);

	      ccusb_flush(handle);
	      ccusb_status(handle);

	      return -1;
	    }
	  
	  return buf[0] | (buf[1]<<8);
	}
    }
  else if ((f & 0x18) == 0x18) // data from control commands
    {
      if (rd != 4)
	{
	  printf("ccusb_naf: Reply to control command: Unexpected data length: wr=%d, rd=%d, buf: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",wr,rd,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
	  return -1;
	}
      
      return buf[0] | (buf[1]<<8);
    }
  else if (((f & 0x18) == 0x10)) // data from write commands
    {
      if (rd != 4)
	{
	  printf("ccusb_naf: Reply to write command: Unexpected data length: wr=%d, rd=%d, buf: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",wr,rd,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
	  return -1;
	}
      
      return buf[0] | (buf[1]<<8);
    }

  //ccusb_flush(handle);
  
  return 0;
}

// implementation for the MIDAS CAMAC interface

int cam_init() // tested KO 16-SEP-2005
{
  int status = ccusb_init();
  if (status == 0)
    return 1; // success
  else
    return 0; // failure
}

void cam16i(const int c, const int n, const int a, const int f, WORD * d) // tested KO 17-SEP-2005
{
  *d = ccusb_naf(gHandle[c],n,a,f,D16,0) & 0x00FFFF;
}

void cam24i(const int c, const int n, const int a, const int f, DWORD * d) // tested KO 17-SEP-2005
{
  *d = ccusb_naf(gHandle[c],n,a,f,D24,0) & 0xFFFFFF;
}

void cam16i_q(const int c, const int n, const int a, const int f, WORD * d, int *x, int *q) // tested KO 17-SEP-2005
{
  int data = ccusb_naf(gHandle[c],n,a,f,D24,0);
  *d = data & 0x00FFFFFF;
  *q = data & 0x01000000;
  *x = data & 0x02000000;
}

void cam24i_q(const int c, const int n, const int a, const int f, DWORD * d, int *x, int *q) // tested KO 17-SEP-2005
{
  int data = ccusb_naf(gHandle[c],n,a,f,D24,0);
  *d = data & 0x00FFFFFF;
  *q = data & 0x01000000;
  *x = data & 0x02000000;
}

void cam16i_r(const int c, const int n, const int a, const int f, WORD ** d, const int r)
{
   // copy from ces8210
   int i;

   for (i = 0; i < r; i++)
      cam16i(c, n, a, f, (*d)++);
}

void cam24i_r(const int c, const int n, const int a, const int f, DWORD ** d, const int r)
{
   // copy from ces8210
   int i;

   for (i = 0; i < r; i++)
      cam24i(c, n, a, f, (*d)++);
}

void cam16i_rq(const int c, const int n, const int a, const int f, WORD ** d, const int r)
{
   int i, q, x;
   WORD data;
   WORD csr;

   for (i = 0; i < r; i++) {
      cam16i_q(c, n, a, f, &data, &x, &q);
      if (!q)
	break;

      *((*d)++) = data;
   }
}

void cam24i_rq(const int c, const int n, const int a, const int f, DWORD ** d, const int r)
{
   int i, q, x;
   DWORD data;
   WORD csr;

   for (i = 0; i < r; i++) {
      cam24i_q(c, n, a, f, &data, &x, &q);
      if (!q)
	break;

      *((*d)++) = data;
   }
}

void cam16i_sa(const int c, const int n, const int a, const int f, WORD ** d, const int r)
{
   // copy from ces8210
   int i;

   for (i = 0; i < r; i++)
      cam16i(c, n, a + i, f, (*d)++);
}

void cam24i_sa(const int c, const int n, const int a, const int f, DWORD ** d, const int r)
{
   // copy from ces8210
   int i;

   for (i = 0; i < r; i++)
      cam24i(c, n, a + i, f, (*d)++);
}

void cam16i_sn(const int c, const int n, const int a, const int f, WORD ** d, const int r)
{
   // copy from ces8210
   int nn;

   for (nn = n; nn < n + r; nn++)
      cam16i(c, nn, a, f, (*d)++);
}

void cam24i_sn(const int c, const int n, const int a, const int f, DWORD ** d, const int r)
{
   // copy from ces8210
   int nn;

   for (nn = n; nn < n + r; nn++)
      cam24i(c, nn, a, f, (*d)++);
}

void cami(const int c, const int n, const int a, const int f, WORD * d)
{
  cam16i(c,n,a,f,d);
}

void cam16o(const int c, const int n, const int a, const int f, WORD d)
{
  ccusb_naf(gHandle[c],n,a,f,D16,d);
}

void cam24o(const int c, const int n, const int a, const int f, DWORD d)
{
  ccusb_naf(gHandle[c],n,a,f,D24,d);
}

void cam16o_q(const int c, const int n, const int a, const int f, WORD d, int *x, int *q) // tested KO 17-SEP-2005
{
  int status = ccusb_naf(gHandle[c],n,a,f,D16,d);
  *q = status & 1;
  *x = status & 2;
}

void cam24o_q(const int c, const int n, const int a, const int f, DWORD d, int *x, int *q) // tested KO 17-SEP-2005
{
  int status = ccusb_naf(gHandle[c],n,a,f,D24,d);
  *q = status & 1;
  *x = status & 2;
}

void cam16o_r(const int c, const int n, const int a, const int f, WORD * d, const int r)
{
  int i;
  for (i=0; i<r; i++)
    cam16o(c,n,a,f,d[i]);
}

void cam24o_r(const int c, const int n, const int a, const int f, DWORD * d, const int r)
{
  int i;
  for (i=0; i<r; i++)
    cam24o(c,n,a,f,d[i]);
}

void camo(const int c, const int n, const int a, const int f, WORD d)
{
  cam16o(c,n,a,f,d);
}

int camc_chk(const int c)
{
  if (gHandle[c])
    return 1;
  else
    return 0;
}

void camc(const int c, const int n, const int a, const int f) // tested KO 17-SEP-2005
{
  ccusb_naf(gHandle[c],n,a,f,D16,0);
}

void camc_q(const int c, const int n, const int a, const int f, int *q) // tested KO 17-SEP-2005
{
  int status = ccusb_naf(gHandle[c],n,a,f,D16,0);
  //printf("cnaf %d %d %d %d, camc status 0x%x\n",c,n,a,f,status);
  *q = status & 1;
}

void camc_sa(const int c, const int n, const int a, const int f, const int r)
{
  // copy from kcs2927

   int i;

   for (i = 0; i < r; i++)
      camc(c, n, a + i, f);
}

void camc_sn(const int c, const int n, const int a, const int f, const int r)
{
  // copy from kcs2927

   int i;

   for (i = 0; i < r; i++)
      camc(c, n + i, a, f);
}

void cam_inhibit_set(const int c) // tested KO 16-SEP-2005
{
  camc(c,29,9,24);
}

void cam_inhibit_clear(const int c) // tested KO 16-SEP-2005
{
  camc(c,29,9,26);
}

void cam_crate_clear(const int c) // tested KO 16-SEP-2005
{
  camc(c,28,9,29);
}

void cam_crate_zinit(const int c) // tested KO 16-SEP-2005
{
  camc(c,28,8,29);
}

void cam_lam_enable(const int c, const int n) // tested KO 16-SEP-2005
{
  camc(c, n, 0, 26);
}

void cam_lam_disable(const int c, const int n) // tested KO 16-SEP-2005
{
  camc(c, n, 0, 24);
}

void cam_lam_read(const int c, DWORD * lam) // tested KO 16-SEP-2005
{
  *lam = ccusb_readReg(gHandle[c],REG_LAM);
}

void cam_lam_clear(const int c, const int n)
{
  /* blank, same as the kcs2927 and ces8210 */
}

#ifdef CCUSB_TEST

// test harness

int test_ccusb()
{
  void* handle;

  if (ccusb_init() != 0)
    return 1;

#if 0
  void *handle = musb_open(0x16dc,0x0001,0,1,0);
  if (!handle)
    {
      fprintf(stderr,"Cannot open the CC-USB interfece...\n");
      return 1;
    }
#endif

  handle = ccusb_getCrateHandle(0);

  //ccusb_reset(handle);

  if (0)
    {
      musb_reset(handle);
      ccusb_status(handle);
      return 0;
    }

  if (0)
  {
    int i;

    for (i=6; i<=8; i++)
      {
	int val = read_reg16(handle,i);
	printf("reg %2d is 0x%02x\n",i,val);
      }
  }

  ccusb_status(handle);

  if (0)
    {
      int i;

      // size the registers
      
      for (i=6; i<=8; i++)
	{
	  int rval;
	  int wval = 0xFFFFFFFF;
	  ccusb_writeReg(handle,i,wval);
	  rval = ccusb_readReg(handle,i);
	  printf("reg %2d write 0x%x, read 0x%02x\n",i,wval,rval);
	}
    }

  //ccusb_writeReg(handle,8,0x12345678);

  //ccusb_flush(handle);
  
  while (1)
    {
      int i, data;
      for (i=0; i<=0; i++)
	{
	  //ccusb_status(handle);
	  data = ccusb_naf(handle,21,i,0,D24,0);
	  printf("address %d, data %d\n",i,data);
	  sleep(1);
	}
    }
}

int main(int argc,char*argv[])
{
  int status;
  int c = 0;
  int n;
  void* handle;

  status = cam_init();
  if (status != 1)
    {
      printf("Error: Cannot initialize camac: cam_init status %d\n",status);
      return 1;
    }

  cam_inhibit_clear(c);

#if 1
  for (n=0; n<=24; n++)
    cam_lam_disable(c,n);
#endif

  cam_lam_enable(c,22);

  handle = ccusb_getCrateHandle(c);
  ccusb_status(handle);

  while (0)
    {
      int lam;
      cam_lam_read(c,&lam);
      printf("lam status 0x%x expected 0x%x\n",lam,1<<(22-1));
      sleep(1);
      cam_crate_clear(c);
      sleep(1);
      ccusb_status(handle);
    }

  if (0)
    {
      for (n=0; n<=24; n++)
	{
	  int val;
	  cam24i(c,n,0,0,&val);
	  printf("station %d, data 0x%x\n",n,val);
	}
    }

  while (0)
    {
      for (n=20; n<=22; n++)
	{
	  int q;
	  camc_q(c,n,0,24,&q);
	  printf("station %d, q %d\n",n,q);
	  sleep(1);
	}
    }

  while (0)
    {
      for (n=0; n<=24; n++)
	{
	  int data, q, x;
	  cam24i_q(c,n,0,0,&data,&x,&q);
	  printf("station %d, data 0x%x, q %d, x %d\n",n,data,q,x);
	  sleep(1);
	}
    }

  while (0)
    {
      for (n=20; n<=22; n++)
	{
	  int data = 0xdeadbeef, q, x;
	  cam24o_q(c,n,0,16,data,&x,&q);
	  printf("station %d, data 0x%x, q %d, x %d\n",n,data,q,x);
	  sleep(1);
	}
    }

  while (1)
    {
      static int count = 0;
      int data;
      cam24i(c,21,0,0,&data);
      if (count%100 == 0 || data == 0xFFFFFF) printf("count %d, data 0x%06x, %d\n",count,data,data);
      count++;
      //sleep(1);
    }

  return 0;
}

#endif

// end
