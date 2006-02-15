/********************************************************************\
  Name:         ccusb.c
  Created by:   Konstantin Olchanski & P.-A. Amaudruz

  Contents:     Wiener CAMAC Controller USB 
                Following musbstd
\********************************************************************/
#include <stdio.h>

#if defined(OS_LINUX)         // Linux includes
#include <stdint.h>
#endif

#include <assert.h>

#include "ccusb.h"    // USB Camac controller header
#include "mcstd.h"    // Midas CAMAC standard calls

#define MAX_CCUSB 32

static int   gNumCrates = 0;

#define D16  0
#define D24  1

#define REG_VERSION 0
#define REG_LAMMASK 8
#define REG_LAM    12
#define REG_SERNO  15

MUSB_INTERFACE *myusb[MAX_CCUSB];

/********************************************************************/
int ccusb_init_crate(int c)
{
  int status, version;

  status = musb_open(&(myusb[c]), 0x16dc,0x0001,c,1,0);
  if (status != MUSB_SUCCESS)
    return -1;
  
  ccusb_flush(myusb[c]);
  
  version   = ccusb_readReg(myusb[c],REG_VERSION);
  myusb[c]->SerNo = ccusb_readReg(myusb[c],REG_SERNO);
  
  fprintf(stderr,"ccusb_init_crate: CAMAC crate %d is CCUSB instance %d, serial number %d, firmware 0x%x\n"
    , c, c, myusb[c]->SerNo ,version);

  if (c >= gNumCrates)
    gNumCrates = c+1;

  return 0;
}

/********************************************************************/
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

/********************************************************************/
MUSB_INTERFACE *ccusb_getCrateStruct(int crate)
{
  assert(crate>=0);
  assert(crate<gNumCrates);
  assert(myusb[crate]!=NULL);
  return myusb[crate];
}

/********************************************************************/
int ccusb_flush(MUSB_INTERFACE *myusbcrate)
{
  int count, rd;
  char buf[16*1024];

  for (count=0; count<1000; count++)
    {
      rd = musb_read(myusbcrate,0x86,buf,sizeof(buf),100);
      if (rd < 0)
  return 0;

      printf("ccusb_flush: count=%d, rd=%d, buf: 0x%02x 0x%02x 0x%02x 0x%02x\n",count,rd,buf[0],buf[1],buf[2],buf[3]);
    }

  ccusb_status(myusbcrate);

  printf("ccusb_flush: CCUSB is babbling. Please reset it by cycling power on the CAMAC crate.\n");

  musb_reset(myusbcrate);

  ccusb_status(myusbcrate);

  exit(1);

  return -1;
}

/********************************************************************/
int ccusb_recover(MUSB_INTERFACE *myusbcrate)
{
  int status;
  int i;
  for (i=0; i<gNumCrates; i++)
    if (myusb[i] == myusbcrate)
      {
  musb_close(myusbcrate);
  status = ccusb_init_crate(i);
  if (status == 0)
    return 0;

  printf("ccusb-recover: Cannot reinitialize crate %d\n",i);
  exit(1);
      }

  assert(!"ccusb_recover: Cannot find handle!\n");
  return -1;
}

/********************************************************************/
int ccusb_readReg(MUSB_INTERFACE *myusbcrate, int ireg)
{
  int rd, wr;
  static char  cmd[4];
  static WORD buf[4];
  cmd[0] = 1;
  cmd[1] = 0;
  cmd[2] = ireg;
  cmd[3] = 0;

  while (1)
    {
      wr = musb_write(myusbcrate,2,cmd,4,100);
      if (wr != 4)
  {
    printf("ccusb_readReg: reg %d, musb_write() error %d\n",ireg,wr);
    exit(1);
    return -1;
  }

      rd = musb_read(myusbcrate,0x86,buf,8,100);
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

/********************************************************************/
int ccusb_writeReg(MUSB_INTERFACE *myusbcrate, int ireg, int value)
{
  int rd, wr;
  static char cmd[8];
  cmd[0] = 5;
  cmd[1] = 0;
  cmd[2] = ireg;
  cmd[3] = 0;
  cmd[4] = value&0xFF;
  cmd[5] = (value>>8)&0xFF;
  cmd[6] = (value>>16)&0xFF;
  cmd[7] = (value>>24)&0xFF;

  wr = musb_write(myusbcrate,2,cmd,8,100);
  if (wr != 8)
    {
      printf("ccusb_writeReg: reg %d, musb_write() error %d\n",ireg,wr);
      exit(1);
      return -1;
    }

  rd = ccusb_readReg(myusbcrate,ireg);
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

/********************************************************************/
int ccusb_reset(MUSB_INTERFACE *myusbcrate)
{
  int rd, wr;
  static char cmd[512];
  static WORD buf[512];
  cmd[0] = 255;
  cmd[1] = 255;
  cmd[2] = 0;
  cmd[3] = 0;
  wr = musb_write(myusbcrate,2,cmd,2,100);
  printf("reset: wr=%d\n",wr);
  return -1;
}

/********************************************************************/
int ccusb_status(MUSB_INTERFACE *myusbcrate)
{
  int reg1;
  
  //ccusb_flush(myusbcrate);

  printf("CCUSB status:\n");
  printf("  reg0  firmware id: 0x%04x\n",ccusb_readReg(myusbcrate,0));
  reg1=ccusb_readReg(myusbcrate,1);
  printf("  reg1  global mode: 0x%04x: ",reg1);
  printf(" BuffOpt=%d,",reg1&7);
  printf(" EvtSepOpt=%d,",(reg1>>6)&1);
  printf(" HeaderOpt=%d,",(reg1>>8)&1);
  printf(" WdgFrq=%d,",(reg1>>9)&7);
  printf(" Arbitr=%d",(reg1>>12)&1);
  printf("\n");
  printf("  reg2  delays:      0x%04x\n",ccusb_readReg(myusbcrate,2));
  printf("  reg3  watchdog:    0x%04x\n",ccusb_readReg(myusbcrate,3));
  printf("  reg4  sel LED-B:   0x%04x\n",ccusb_readReg(myusbcrate,4));
  printf("  reg5  scaler freq: 0x%04x\n",ccusb_readReg(myusbcrate,5));
  printf("  reg6  sel LED-A:   0x%04x\n",ccusb_readReg(myusbcrate,6));
  printf("  reg7  NIM out:     0x%04x\n",ccusb_readReg(myusbcrate,7));
  printf("  reg8  LAM mask:  0x%06x\n",ccusb_readReg(myusbcrate,8));
  printf("  reg9  unknown:     0x%04x\n",ccusb_readReg(myusbcrate,9));
  printf("  reg10 action reg:  0x%04x\n",ccusb_readReg(myusbcrate,10));
  printf("  reg11 unknown:     0x%04x\n",ccusb_readReg(myusbcrate,11));
  printf("  reg12 LAM status:0x%06x\n",ccusb_readReg(myusbcrate,12));
  printf("  reg13 serial no:   0x%04x\n",ccusb_readReg(myusbcrate,13));
  printf("  reg14 unknown:     0x%04x\n",ccusb_readReg(myusbcrate,14));
  printf("  reg15 serial no:   0x%04x\n",ccusb_readReg(myusbcrate,15));

  return 0;
}

/********************************************************************/
int ccusb_naf(MUSB_INTERFACE *myusbcrate, int n, int a, int f, int d24, int data)
{
  int rd, wr, bcount;
  static char cmd[16];
  static char buf[16];
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

  wr = musb_write(myusbcrate,2,cmd,bcount,1000);
  if (wr != bcount)
    {
      printf("ccusb_naf: musb_write() error %d\n",wr);
      exit(1);
      return -1;
    }

  rd = musb_read(myusbcrate, 0x86, buf, sizeof(buf), 1000);

  if (((f & 0x18) == 0)) // data from read commands
    {
      if (d24==D24)
  {
    if (rd != 6)
      {
        printf("ccusb_naf: Reply to read command: Unexpected data length, should be 6: wr=%d, rd=%d, buf: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",wr,rd,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);

        ccusb_flush(myusbcrate);
        ccusb_status(myusbcrate);

        return -1;
      }
    
    return buf[0] | (buf[1]<<8)| (buf[2]<<16)| (buf[3]<<24);
  }
      else
  {
    if (rd != 4)
      {
        printf("ccusb_naf: Reply to read command: Unexpected data length, should be 4: wr=%d, rd=%d, buf: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",wr,rd,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);

              ccusb_flush(myusbcrate);
        ccusb_status(myusbcrate);

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

  //ccusb_flush(myusbcrate);
  
  return 0;
}

// implementation for the MIDAS CAMAC interface

/********************************************************************/
int cam_init() // tested KO 16-SEP-2005
{
  int c, i, status = ccusb_init();
  if (status == 0) {
    // Clear the LAM register at initialization PAA-Feb/06
    for (c=0;c<gNumCrates;c++) {
      if (ccusb_getCrateStruct(c))
        for (i=1;i<24;i++)  cam_lam_disable(c,i);
    }
    return 1; // success
  }
  else
    return 0; // failure

  
}

/********************************************************************/
void cam16i(const int c, const int n, const int a, const int f, WORD * d) // tested KO 17-SEP-2005
{
  *d = ccusb_naf(myusb[c],n,a,f,D16,0) & 0x00FFFF;
}

/********************************************************************/
void cam24i(const int c, const int n, const int a, const int f, DWORD * d) // tested KO 17-SEP-2005
{
  *d = ccusb_naf(myusb[c],n,a,f,D24,0) & 0xFFFFFF;
}

/********************************************************************/
void cam16i_q(const int c, const int n, const int a, const int f, WORD * d, int *x, int *q) // tested KO 17-SEP-2005
{
  int data = ccusb_naf(myusb[c],n,a,f,D24,0);
  *d = data & 0x00FFFFFF;
  *q = (data & 0x01000000)>>24;
  *x = (data & 0x02000000)>>25;
}

/********************************************************************/
void cam24i_q(const int c, const int n, const int a, const int f, DWORD * d, int *x, int *q) // tested KO 17-SEP-2005
{
  int data = ccusb_naf(myusb[c],n,a,f,D24,0);
  *d = data & 0x00FFFFFF;
  *q = (data & 0x01000000)>>24;
  *x = (data & 0x02000000)>>25;
}

/********************************************************************/
void cam16i_r(const int c, const int n, const int a, const int f, WORD ** d, const int r)
{
   // copy from ces8210
   int i;

   for (i = 0; i < r; i++)
      cam16i(c, n, a, f, (*d)++);
}

/********************************************************************/
void cam24i_r(const int c, const int n, const int a, const int f, DWORD ** d, const int r)
{
   // copy from ces8210
   int i;

   for (i = 0; i < r; i++)
      cam24i(c, n, a, f, (*d)++);
}

/********************************************************************/
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

/********************************************************************/
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

/********************************************************************/
void cam16i_sa(const int c, const int n, const int a, const int f, WORD ** d, const int r)
{
   // copy from ces8210
   int i;

   for (i = 0; i < r; i++)
      cam16i(c, n, a + i, f, (*d)++);
}

/********************************************************************/
void cam24i_sa(const int c, const int n, const int a, const int f, DWORD ** d, const int r)
{
   // copy from ces8210
   int i;

   for (i = 0; i < r; i++)
      cam24i(c, n, a + i, f, (*d)++);
}

/********************************************************************/
void cam16i_sn(const int c, const int n, const int a, const int f, WORD ** d, const int r)
{
   // copy from ces8210
   int nn;

   for (nn = n; nn < n + r; nn++)
      cam16i(c, nn, a, f, (*d)++);
}

/********************************************************************/
void cam24i_sn(const int c, const int n, const int a, const int f, DWORD ** d, const int r)
{
   // copy from ces8210
   int nn;

   for (nn = n; nn < n + r; nn++)
      cam24i(c, nn, a, f, (*d)++);
}

/********************************************************************/
void cami(const int c, const int n, const int a, const int f, WORD * d)
{
  cam16i(c,n,a,f,d);
}

/********************************************************************/
void cam16o(const int c, const int n, const int a, const int f, WORD d)
{
  ccusb_naf(myusb[c],n,a,f,D16,d);
}

/********************************************************************/
void cam24o(const int c, const int n, const int a, const int f, DWORD d)
{
  ccusb_naf(myusb[c],n,a,f,D24,d);
}

/********************************************************************/
void cam16o_q(const int c, const int n, const int a, const int f, WORD d, int *x, int *q) // tested KO 17-SEP-2005
{
  int status = ccusb_naf(myusb[c],n,a,f,D16,d);
  *q = status & 1;
  *x = status & 2;
}

/********************************************************************/
void cam24o_q(const int c, const int n, const int a, const int f, DWORD d, int *x, int *q) // tested KO 17-SEP-2005
{
  int status = ccusb_naf(myusb[c],n,a,f,D24,d);
  *q = status & 1;
  *x = status & 2;
}

/********************************************************************/
void cam16o_r(const int c, const int n, const int a, const int f, WORD * d, const int r)
{
  int i;
  for (i=0; i<r; i++)
    cam16o(c,n,a,f,d[i]);
}

/********************************************************************/
void cam24o_r(const int c, const int n, const int a, const int f, DWORD * d, const int r)
{
  int i;
  for (i=0; i<r; i++)
    cam24o(c,n,a,f,d[i]);
}

/********************************************************************/
void camo(const int c, const int n, const int a, const int f, WORD d)
{
  cam16o(c,n,a,f,d);
}

/********************************************************************/
int camc_chk(const int c)
{
  if (myusb[c])
    return 1;
  else
    return 0;
}

/********************************************************************/
void camc(const int c, const int n, const int a, const int f) // tested KO 17-SEP-2005
{
  ccusb_naf(myusb[c],n,a,f,D16,0);
}

/********************************************************************/
void camc_q(const int c, const int n, const int a, const int f, int *q) // tested KO 17-SEP-2005
{
  int status = ccusb_naf(myusb[c],n,a,f,D16,0);
  //printf("cnaf %d %d %d %d, camc status 0x%x\n",c,n,a,f,status);
  *q = status & 1;
}

/********************************************************************/
void camc_sa(const int c, const int n, const int a, const int f, const int r)
{
  // copy from kcs2927

   int i;

   for (i = 0; i < r; i++)
      camc(c, n, a + i, f);
}

/********************************************************************/
void camc_sn(const int c, const int n, const int a, const int f, const int r)
{
  // copy from kcs2927

   int i;

   for (i = 0; i < r; i++)
      camc(c, n + i, a, f);
}

/********************************************************************/
void cam_inhibit_set(const int c) // tested KO 16-SEP-2005
{
  camc(c,29,9,24);
}

/********************************************************************/
void cam_inhibit_clear(const int c) // tested KO 16-SEP-2005
{
  camc(c,29,9,26);
}

/********************************************************************/
void cam_crate_clear(const int c) // tested KO 16-SEP-2005
{
  camc(c,28,9,29);
}

/********************************************************************/
void cam_crate_zinit(const int c) // tested KO 16-SEP-2005
{
  camc(c,28,8,29);
}

/********************************************************************/
void cam_lam_enable(const int c, const int n) // tested KO 16-SEP-2005
{
  int mask;

  mask = ccusb_readReg(ccusb_getCrateStruct(c), REG_LAMMASK);
  mask |= (1 << (n - 1));
  ccusb_writeReg(ccusb_getCrateStruct(c), REG_LAMMASK, mask);
  camc(c, n, 0, 26);
  
}

/********************************************************************/
void cam_lam_disable(const int c, const int n) // tested KO 16-SEP-2005
{
  int mask;
  camc(c, n, 0, 24);
  mask = ccusb_readReg(ccusb_getCrateStruct(c), REG_LAMMASK);
  mask &= ~(1 << (n - 1));
  ccusb_writeReg(ccusb_getCrateStruct(c), REG_LAMMASK, mask);
}

/********************************************************************/
void cam_lam_read(const int c, DWORD * lam) // tested KO 16-SEP-2005
{
  *lam = ccusb_readReg(myusb[c],REG_LAM);
}

/********************************************************************/
void cam_lam_clear(const int c, const int n)
{
  /* blank, same as the kcs2927 and ces8210 */
}

/********************************************************************/
void cam_exit(void)
{
  int i;
  
  for (i=0;i<gNumCrates;i++)
    if (myusb[i])
      musb_close(myusb[i]);
}

/********************************************************************/
int cam_init_rpc(char *host_name, char *exp_name, char *fe_name,
                        char *client_name, char *rpc_server)
{
   /* dummy routine for compatibility */
   return 1;
}

/********************************************************************/
/*----------------------------------------------------------------------*/
#ifdef MAIN_ENABLE
int main(int argc,char*argv[])
{
  int status;
  int c = 0;
  int i, n;

  status = cam_init();
  if (status != 1)
    {
      printf("Error: Cannot initialize camac: cam_init status %d\n",status);
      return 1;
    }

  // Initialization
#if 1
  for (i=0;i<4;i++) {
    cam_inhibit_set(c);
    printf("Inhibit Set\n");
    sleep(1);
    cam_inhibit_clear(c);
    printf("Inhibit Clear\n");
    sleep(1);
  }
#endif

  ccusb_status(ccusb_getCrateStruct(c));

  // LAM test
#if 0
  for (i=1;i<24;i++)  cam_lam_disable(c,i);
  while (1)
    {
      unsigned long lam;
      cam_lam_enable(c,22);
      cam_lam_enable(c,23);
      ccusb_status(ccusb_getCrateStruct(c));
      camc(c,22,0,9);
      camc(c,22,0,25);
      sleep(1);
      cam_lam_read(c, &lam);
      printf("lam status 0x%x expected 0x%x\n",lam,1<<(22-1));
    }
#endif

  // Scan crate for Read 24 /q/x
  while (0)
    {
      for (n=0; n<=24; n++)
  {
    unsigned long data;
    int q, x;
    cam24i_q(c,n,0,0,&data,&x,&q);
    printf("station %d, data 0x%x, q %d, x %d\n",n,data,q,x);
    sleep(1);
  }
    }

  // Access/Speed test  
  // cami/o ~370us per access (USB limitation in non-block mode)
  while (0)
    {
      static unsigned long data;
      cam24o(c,19,0,16,0x0);
      cam24o(c,19,0,16,0x1);
      cam24o(c,19,0,16,0x0);
      cam24i(c,22,0,0,&data);
      cam24o(c,19,0,16,0x2);
      cam24o(c,19,0,16,0x0);
    }

  return 0;
}

#endif
// end
