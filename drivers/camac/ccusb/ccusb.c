/********************************************************************\
  Name:         ccusb.c
  Created by:   Konstantin Olchanski & P.-A. Amaudruz

  Contents:     Wiener CAMAC Controller USB 
                Following musbstd
\********************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#if defined(OS_WINNT)           // Windows includes
#include <windows.h>
#endif

#include "ccusb.h"              // USB Camac controller header
#include "mcstd.h"              // MIDAS CAMAC interface

#define MAX_CCUSB 32

static int gNumCrates = 0;

#define READ_EP  0x86
#define WRITE_EP 0x02

static MUSB_INTERFACE *gUsb[MAX_CCUSB];

/********************************************************************/
int ccusb_init_crate(int c)
{
   int status, try, version;

   //if (gUsb[c])
   //  musb_close(gUsb[c]);
   //gUsb[c] = NULL;

   status = musb_open(&(gUsb[c]), 0x16dc, 0x0001, c, 1, 0);
   if (status != MUSB_SUCCESS)
      return CCUSB_ERROR;

   for (try=0; try<=1; try++) {

     // clear the "run" bit, issue "clear" command
     ccusb_writeReg(gUsb[c], CCUSB_ACTION, 4);
     ccusb_writeReg(gUsb[c], CCUSB_ACTION, 0);
     ccusb_flush(gUsb[c]);

     //ccusb_status(gUsb[c]);
     
     version = ccusb_readCamacReg(gUsb[c], CCUSB_VERSION);
     
     if (version != -1) {
       version &= 0xffff;
       
       fprintf(stderr, "ccusb_init_crate: CAMAC crate %d is CCUSB instance %d, firmware 0x%04x\n",
               c, c, version);
       
       if (c >= gNumCrates)
         gNumCrates = c + 1;
       
       return 0;
     }

     fprintf(stderr, "ccusb_init_crate: CAMAC crate %d is CCUSB instance %d is not talking to us, trying again...\n", c, c);
     
     musb_reset(gUsb[c]);
   }

   //if (gUsb[c])
   //  musb_close(gUsb[c]);
   //gUsb[c] = NULL;
     
   fprintf(stderr, "ccusb_init_crate: CAMAC crate %d is CCUSB instance %d is not talking to us\n", c, c);
   return CCUSB_ERROR;
}

/********************************************************************/
int ccusb_init()
{
   int i;
   for (i = 0; i < MAX_CCUSB; i++) {
      int status = ccusb_init_crate(i);
      if (status != 0)
         break;
   }

   if (gNumCrates < 1) {
      fprintf(stderr, "ccusb_init: Did not find any CC-USB CAMAC interfaces.\n");
      return CCUSB_ERROR;
   }

   return 0;
}

/********************************************************************/
MUSB_INTERFACE *ccusb_getCrateStruct(int crate)
{
   if (crate < 0) {
      fprintf(stderr, "ccusb_getCrateStruct: crate number %d should be positive\n", crate);
      abort();
   }

   if (crate >= gNumCrates) {
      fprintf(stderr, "ccusb_getCrateStruct: crate number %d does not exist: valid crate numbers are 0 through %d\n",
              crate, gNumCrates - 1);
      abort();
   }

   if (gUsb[crate] == NULL) {
      fprintf(stderr, "ccusb_getCrateStruct: crate number %d is invalid: USB interface pointer is NULL\n", crate);
      abort();
   }

   return gUsb[crate];
}

/********************************************************************/
int ccusb_flush(MUSB_INTERFACE * myusbcrate)
{
  int count, rd, flushed = 0;
   unsigned char buf[16 * 1024];

   //printf("ccusb_flush!\n");

   for (count = 0; count < 1000; count++) {
      rd = musb_read(myusbcrate, READ_EP, buf, sizeof(buf), 100);
      if (rd < 0) {
         if (flushed > 0)
            fprintf(stderr, "ccusb_flush: Flushed %d bytes\n", flushed);
         return 0;
      }

      //printf("ccusb_flush: count=%d, rd=%d, buf: 0x%02x 0x%02x 0x%02x 0x%02x\n", count,
      //rd, buf[0], buf[1], buf[2], buf[3]);

      flushed += rd;
   }

   ccusb_status(myusbcrate);

   fprintf(stderr, "ccusb_flush: CCUSB is babbling. Please reset it by cycling power on the CAMAC crate.\n");
   exit(1);

   return CCUSB_ERROR;
}

/********************************************************************/
int ccusb_readReg(MUSB_INTERFACE * myusbcrate, int ireg)
{
   int try, rd, wr;
   uint8_t cmd[4];
   uint16_t buf[4];

   assert(myusbcrate != NULL);

   assert(!"ccusb_readReg: CCUSB Firmware since version 0x02xx does not have any readable registers");

   if (ireg != CCUSB_ACTION)
     {
       printf("ccusb_readReg: Error: access to non-existant register %d: Per CCUSB manual version 2.3, only \"action register\" %d exists since firmware 0x101. Sorry.\n",
              ireg, CCUSB_ACTION);
       return CCUSB_ERROR;
     }

   cmd[0] = 1;
   cmd[1] = 0;
   cmd[2] = ireg;
   cmd[3] = 0;

   for (try=0; try<10; try++) {
      wr = musb_write(myusbcrate, WRITE_EP, cmd, 4, 100);
      if (wr != 4) {
         printf("ccusb_readReg: reg %d, musb_write() error %d\n", ireg, wr);
	 musb_reset(myusbcrate);
	 //ccusb_flush(myusbcrate);
         exit(1);
         return CCUSB_ERROR;
      }

      rd = musb_read(myusbcrate, READ_EP, buf, 8, 100);
      if (rd < 0) {
         printf
             ("ccusb_readReg: reg %d, timeout: wr=%d, rd=%d, buf: 0x%x 0x%x 0x%x 0x%x\n",
              ireg, wr, rd, buf[0], buf[1], buf[2], buf[3]);
         continue;
      }

      if (rd == 2)
         return buf[0];
      else if (rd == 4)
         return (buf[0] | (buf[1] << 16));
      else {
         printf
             ("ccusb_readReg: reg %d, unexpected data length: wr=%d, rd=%d, buf: 0x%x 0x%x 0x%x 0x%x\n",
              ireg, wr, rd, buf[0], buf[1], buf[2], buf[3]);
         return CCUSB_ERROR;
      }
   }

   printf("ccusb_readReg: reg %d, too many retries, give up\n", ireg);
   return CCUSB_ERROR;
}

/********************************************************************/
int ccusb_writeReg(MUSB_INTERFACE * myusbcrate, int ireg, int value)
{
   int wr;
   static char cmd[8];

   assert(myusbcrate != NULL);

   cmd[0] = 5;
   cmd[1] = 0;
   cmd[2] = ireg;
   cmd[3] = 0;
   cmd[4] = value & 0xFF;
   cmd[5] = (value >> 8) & 0xFF;
   cmd[6] = (value >> 16) & 0xFF;
   cmd[7] = (value >> 24) & 0xFF;

   wr = musb_write(myusbcrate, WRITE_EP, cmd, 8, 100);
   if (wr != 8) {
      fprintf(stderr, "ccusb_writeReg: register %d, value %d: musb_write() error %d (%s)\n", ireg, value, wr, strerror(-wr));
      return CCUSB_ERROR;
   }

   if (0) {
      int rd = ccusb_readReg(myusbcrate, ireg);
      if (rd < 0) {
         fprintf(stderr, "ccusb_writeReg: Register %d readback failed with error %d\n", ireg, rd);
         return CCUSB_ERROR;
      }

      if (rd != value) {
         fprintf(stderr, "ccusb_writeReg: Register %d readback mismatch: wrote 0x%x, got 0x%x\n",
                 ireg, value, rd);
         return CCUSB_ERROR;
      }
   }

   return 0;
}

/********************************************************************/
int ccusb_readCamacReg(MUSB_INTERFACE * myusbcrate, int ireg)
{
  return ccusb_naf(myusbcrate, 25, ireg, 0, CCUSB_D24, 0);
}

/********************************************************************/
int ccusb_writeCamacReg(MUSB_INTERFACE * myusbcrate, int ireg, int value)
{
  return ccusb_naf(myusbcrate, 25, ireg, 16, CCUSB_D24, value);
}

/********************************************************************/
int ccusb_status(MUSB_INTERFACE * myusbcrate)
{
   int reg;

   printf("CCUSB status:\n");
   //printf("  reg1  action register: 0x%x\n", ccusb_readReg(myusbcrate, CCUSB_ACTION));
   printf("CCUSB CAMAC registers:\n");

   reg = ccusb_readCamacReg(myusbcrate, 0);
   printf("  reg0  firmware   : 0x%04x: version 0x%04x\n", reg, reg&0xFFFF);
   if (reg == -1)
     return CCUSB_ERROR;

   reg = ccusb_readCamacReg(myusbcrate, 1);
   printf("  reg1  global mode: 0x%08x:", reg);
   printf(" BuffOpt=%d,", reg & 0xF);
   printf(" MixBuffOpt=%d,", (reg >> 5) & 1);
   printf(" EvtSepOpt=%d,", (reg >> 6) & 1);
   printf(" HeaderOpt=%d,", (reg >> 8) & 1);
   printf(" Arbitr=%d", (reg >> 12) & 1);
   printf("\n");

   reg = ccusb_readCamacReg(myusbcrate, 2);
   printf("  reg2  delays:      0x%08x: LAM delay %d us, trigger delay %d us\n", reg, (reg&0xFF)>>8, (reg&0xFF));

   reg = ccusb_readCamacReg(myusbcrate, 3);
   printf("  reg3  scaler readout control:    0x%08x: period %.1f s or every %d events, \n", reg, 0.5*((reg&0xFF0000)>>16),(reg&0xFFFF));

   reg = ccusb_readCamacReg(myusbcrate, 4);
   printf("  reg4  LED source:  0x%08x", reg);
   printf(": Yellow: %c%c code %d", (reg&(1<<21))?'L':'.', (reg&(1<<20))?'I':'.' , (reg>>16)&7);
   printf(", Green: %c%c code %d", (reg&(1<<13))?'L':'.', (reg&(1<<12))?'I':'.' , (reg>>8)&7);
   printf(", Red: %c%c code %d", (reg&(1<<5))?'L':'.', (reg&(1<<4))?'I':'.' , (reg>>0)&7);
   printf("\n");

   reg = ccusb_readCamacReg(myusbcrate, 5);
   printf("  reg5  NIM source:  0x%08x", reg);
   printf(": Yellow: %c%c code %d", (reg&(1<<21))?'L':'.', (reg&(1<<20))?'I':'.' , (reg>>16)&7);
   printf(", Green: %c%c code %d", (reg&(1<<13))?'L':'.', (reg&(1<<12))?'I':'.' , (reg>>8)&7);
   printf(", Red: %c%c code %d", (reg&(1<<5))?'L':'.', (reg&(1<<4))?'I':'.' , (reg>>0)&7);
   printf("\n");

   printf("  reg6  Source selector:   0x%08x\n", ccusb_readCamacReg(myusbcrate, 6));
   printf("  reg7  Timing B:    0x%08x\n", ccusb_readCamacReg(myusbcrate, 7));
   printf("  reg8  Timing A:    0x%08x\n", ccusb_readCamacReg(myusbcrate, 8));
   printf("  reg9  LAM mask:    0x%08x\n", ccusb_readCamacReg(myusbcrate, 9));
   printf("  reg10 CAMAC LAM:   0x%08x\n", ccusb_readCamacReg(myusbcrate, 10));
   printf("  reg11 scaler A:    0x%08x\n", ccusb_readCamacReg(myusbcrate, 11));
   printf("  reg12 scaler B:    0x%08x\n", ccusb_readCamacReg(myusbcrate, 12));
   printf("  reg13 extended delays:     0x%08x\n", ccusb_readCamacReg(myusbcrate, 13));

   reg = ccusb_readCamacReg(myusbcrate, 14);
   printf("  reg14 usb buffering setup: 0x%08x: timeout %d s, number of buffers %d\n", reg, (reg&0xF00)>>8, reg&0xFF);
   printf("  reg15 broadcast map:       0x%08x\n", ccusb_readCamacReg(myusbcrate, 15));
   return 0;
}

/********************************************************************/

static int      gCamacStackRecording = 0;
static int      gCamacStackCounter = 0;
static uint16_t gCamacStack[1000];

int ccusb_startStack()
{
   assert(gCamacStackRecording == 0);
   gCamacStackRecording = 1;
   gCamacStackCounter = 2;
   return 0;
}

int ccusb_pushStack(int data)
{
   assert(gCamacStackRecording);
   gCamacStack[gCamacStackCounter++] = data;
   return 0;
}

int ccusb_readStack(MUSB_INTERFACE *musb, int iaddr, uint16_t buf[], int bufsize, int verbose)
{
   int wr, rd, bcount;

   bcount = 2;
   buf[0] = iaddr; // address
   
   wr = musb_write(musb, WRITE_EP, buf, bcount, 1000);
   if (wr != bcount) {
     fprintf(stderr, "ccusb_readStack: musb_write() error %d\n", wr);
     exit(1);
     return CCUSB_ERROR;
   }

   rd = musb_read(musb, READ_EP, buf, bufsize, 1000);
   if (rd < 0) {
     fprintf(stderr, "ccusb_readStack: musb_read() error %d\n", rd);
     exit(1);
     return CCUSB_ERROR;
   }
  
   if (verbose) {
     printf("ccusb_readStack: Reply length %d, buf: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
            rd, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
   }
   
   // test code buf[1] = 0xdead;

   if (verbose) {
     int i;
     printf("Reading data stack %d, have %d commands:\n", iaddr, rd/2);
     for (i=0; i<rd/2; i++)
       printf("entry %3d: 0x%04x\n", i, buf[i]);
     printf("End.\n");
   }

   return 0;
}

int ccusb_writeStack(MUSB_INTERFACE *musb, int iaddr, int verbose)
{
   int i, wr, rd, bcount;
   int mismatch;
   uint16_t buf[1000];

   assert(gCamacStackRecording == 1);
   assert(gCamacStackCounter > 1);

   if (verbose) {
     printf("Writing data stack %d, have %d commands:\n", iaddr, gCamacStackCounter - 2);
     for (i=2; i<gCamacStackCounter; i++)
       printf("entry %3d: 0x%04x\n", i - 2, gCamacStack[i]);
     printf("End.\n");
   }

   bcount = gCamacStackCounter * 2;
   gCamacStack[0] = 4 + iaddr;              // address
   gCamacStack[1] = gCamacStackCounter - 2; // word count
   
   wr = musb_write(musb, WRITE_EP, gCamacStack, bcount, 1000);
   if (wr != bcount) {
     fprintf(stderr, "ccusb_writeStack: musb_write() error %d\n", wr);
     exit(1);
     return CCUSB_ERROR;
   }

   gCamacStackRecording = 0;

   rd = ccusb_readStack(musb, iaddr, buf, sizeof(buf), verbose);
   if (rd < 0)
     return rd;

   if (gCamacStackCounter - 2 != buf[0]) {
     fprintf(stderr, "ccusb_writeStack: Stack word counter mismatch: should be %d, received %d\n", gCamacStackCounter - 2, buf[0]);
     exit(1);
     return CCUSB_ERROR;
   }

   mismatch = 0;
   for (i=0; i<buf[0]; i++)
     if (gCamacStack[i+2] != buf[i+1]) {
       mismatch++;
       fprintf(stderr, "ccusb_writeStack: Stack data mismatch: word %d should be 0x%04x, received 0x%04x\n", i, gCamacStack[i+2], buf[i+1]);
     }

   if (mismatch) {
     fprintf(stderr, "ccusb_writeStack: Error: Stack %d data mismatches: %d\n", iaddr, mismatch);
     exit(1);
     return CCUSB_ERROR;
   }

   if (verbose)
     printf("ccusb_writeStack: Stack %d data size %d is okey\n", iaddr, gCamacStackCounter);

   return 0;
}

int ccusb_writeDataStack(MUSB_INTERFACE *musb, int verbose)
{
   return ccusb_writeStack(musb, 2, verbose);
}

int ccusb_writeScalerStack(MUSB_INTERFACE *musb, int verbose)
{
   return ccusb_writeStack(musb, 3, verbose);
}

/********************************************************************/
int ccusb_readData(MUSB_INTERFACE *musb, char* buffer, int bufferSize, int timeout_ms)
{
  int rd;
  rd = musb_read(musb, READ_EP, buffer, bufferSize, timeout_ms);
  if (rd < 0)
    {
      fprintf(stderr, "ccusb_readData: musb_read() error %d (%s)\n", rd, strerror(-rd));

      if (rd == -110)
        return CCUSB_TIMEOUT;

      return CCUSB_ERROR;
    }
  
  return rd;
}

/********************************************************************/
int ccusb_naf(MUSB_INTERFACE * myusbcrate, int n, int a, int f, int d24, int data)
{
   int rd, wr, bcount;
   uint8_t cmd[16];
   //uint8_t buf[16];
   int naf;
   uint8_t buf[26700];

   assert(myusbcrate != NULL);

   naf = f | (a << 5) | (n << 9);

   if (d24)
      naf |= 0x4000;

   bcount = 6;
   cmd[0] = 12;                 // address
   cmd[1] = 0;
   cmd[2] = 1;                  // word count
   cmd[3] = 0;
   cmd[4] = (naf & 0x00FF);     // NAF low bits
   cmd[5] = (naf & 0xFF00) >> 8;        // NAF high bits

   //printf("naf %d %d %d 0x%02x, d24=%d\n",n,a,f,f,d24);

   if ((f & 0x18) == 0x10)      // send data for write commands
   {
      //printf("send write data 0x%x\n",data);
      bcount += 2;
      cmd[2] = 2;
      cmd[6] = (data & 0x00FF); // write data, low 8 bits
      cmd[7] = (data & 0xFF00) >> 8;    // write data, high 8 bits
      if (d24) {
         bcount += 2;
         cmd[2] = 3;
         cmd[8] = (data & 0xFF0000) >> 16;      // write data, highest 8 bits
         cmd[9] = 0;
      }
   }

   if (0)
     {
       printf("Sending %d bytes: ", bcount);
       for (rd=0; rd<bcount; rd++)
	 printf(" 0x%02x", cmd[rd]);
       printf("\n");
     }

   if (gCamacStackRecording)
     {
       int i;
       for (i=4; i<bcount; i+=2)
	 gCamacStack[gCamacStackCounter++] = (cmd[i]|cmd[i+1]<<8);
       return 0;
     }

   wr = musb_write(myusbcrate, WRITE_EP, cmd, bcount, 100);
   if (wr != bcount) {
      fprintf(stderr, "ccusb_naf: musb_write() error %d (%s)\n", wr, strerror(-wr));
      return CCUSB_ERROR;
   }

   rd = musb_read(myusbcrate, READ_EP, buf, sizeof(buf), 1000);
   if (rd < 0) {
      fprintf(stderr, "ccusb_naf: musb_read() error %d (%s)\n", rd, strerror(-rd));
      return CCUSB_ERROR;
   }

   if (0)
     {
       printf("ccusb_naf: Reply length %d, buf: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
	      rd, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
     }
       
   if (((f & 0x18) == 0))       // data from read commands
   {
      if (d24 == CCUSB_D24) {
         if (rd != 4) {
            fprintf(stderr,"ccusb_naf: Reply to 24-bit read command: Unexpected data length, should be 4: wr=%d, rd=%d, buf: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
                   wr, rd, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
            return CCUSB_ERROR;
         }

         return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
      } else {
         if (rd != 2 && rd != 4) {
            fprintf(stderr,"ccusb_naf: Reply to 16-bit read command: Unexpected data length, should be 4: wr=%d, rd=%d, buf: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
                   wr, rd, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
            return CCUSB_ERROR;
         }

         return buf[0] | (buf[1] << 8);
      }
   }
   else if ((f & 0x18) == 0x18)       // data from control commands
   {
      if (rd != 2 && rd != 4) {
         fprintf(stderr,"ccusb_naf: Reply to control command: Unexpected data length: wr=%d, rd=%d, buf: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
                wr, rd, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
         return CCUSB_ERROR;
      }

      return buf[0] | (buf[1] << 8);
   }
   else if (((f & 0x18) == 0x10))     // data from write commands
   {
      if (rd != 2) {
         fprintf(stderr,"ccusb_naf: Reply to write command: Unexpected data length: wr=%d, rd=%d, buf: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
                wr, rd, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
         return CCUSB_ERROR;
      }

      return buf[0] | (buf[1] << 8);
   }

   return 0;
}

int ccusb_AcqStart(MUSB_INTERFACE *c)
{
  int status = ccusb_writeReg(c, CCUSB_ACTION, 0x1);
  if (status != 0)
    fprintf(stderr, "ccusb_AcqStart: ccusb_writeReg() error %d\n", status);
  return status;
}

int ccusb_AcqStop(MUSB_INTERFACE *c)
{
  int status = ccusb_writeReg(c, CCUSB_ACTION, 0x0);
  if (status != 0)
    fprintf(stderr, "ccusb_AcqStop: ccusb_writeReg() error %d\n", status);
  return status;
}

// implementation for the MIDAS CAMAC interface

/********************************************************************/
int cam_init()                  // tested KO 16-SEP-2005
{
   int c, i, status = ccusb_init();
   if (status == 0)
     {
       // Clear the LAM register at initialization PAA-Feb/06
       for (c = 0; c < gNumCrates; c++)
	 {
	   if (ccusb_getCrateStruct(c))
	     for (i = 1; i < 24; i++)
	       cam_lam_disable(c, i);
	 }
       return 1; // success
     }
   else
     return 0;   // failure
}

/********************************************************************/
void cam16i(const int c, const int n, const int a, const int f, WORD * d)       // tested KO 17-SEP-2005
{
   *d = ccusb_naf(gUsb[c], n, a, f, CCUSB_D16, 0) & 0x00FFFF;
}

/********************************************************************/
void cam24i(const int c, const int n, const int a, const int f, DWORD * d)      // tested KO 17-SEP-2005
{
   *d = ccusb_naf(gUsb[c], n, a, f, CCUSB_D24, 0) & 0xFFFFFF;
}

/********************************************************************/
void cam16i_q(const int c, const int n, const int a, const int f, WORD * d, int *x, int *q)     // tested KO 17-SEP-2005
{
   int data = ccusb_naf(gUsb[c], n, a, f, CCUSB_D24, 0);
   *d = data & 0x00FFFFFF;
   *q = (data & 0x01000000) >> 24;
   *x = (data & 0x02000000) >> 25;
}

/********************************************************************/
void cam24i_q(const int c, const int n, const int a, const int f, DWORD * d, int *x, int *q)    // tested KO 17-SEP-2005
{
   int data = ccusb_naf(gUsb[c], n, a, f, CCUSB_D24, 0);
   *d = data & 0x00FFFFFF;
   *q = (data & 0x01000000) >> 24;
   *x = (data & 0x02000000) >> 25;
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
   cam16i(c, n, a, f, d);
}

/********************************************************************/
void cam16o(const int c, const int n, const int a, const int f, WORD d)
{
   ccusb_naf(gUsb[c], n, a, f, CCUSB_D16, d);
}

/********************************************************************/
void cam24o(const int c, const int n, const int a, const int f, DWORD d)
{
   ccusb_naf(gUsb[c], n, a, f, CCUSB_D24, d);
}

/********************************************************************/
void cam16o_q(const int c, const int n, const int a, const int f, WORD d, int *x, int *q)       // tested KO 17-SEP-2005
{
   int status = ccusb_naf(gUsb[c], n, a, f, CCUSB_D16, d);
   *q = status & 1;
   *x = status & 2;
}

/********************************************************************/
void cam24o_q(const int c, const int n, const int a, const int f, DWORD d, int *x, int *q)      // tested KO 17-SEP-2005
{
   int status = ccusb_naf(gUsb[c], n, a, f, CCUSB_D24, d);
   *q = status & 1;
   *x = status & 2;
}

/********************************************************************/
void cam16o_r(const int c, const int n, const int a, const int f, WORD * d, const int r)
{
   int i;
   for (i = 0; i < r; i++)
      cam16o(c, n, a, f, d[i]);
}

/********************************************************************/
void cam24o_r(const int c, const int n, const int a, const int f, DWORD * d, const int r)
{
   int i;
   for (i = 0; i < r; i++)
      cam24o(c, n, a, f, d[i]);
}

/********************************************************************/
void camo(const int c, const int n, const int a, const int f, WORD d)
{
   cam16o(c, n, a, f, d);
}

/********************************************************************/
int camc_chk(const int c)
{
   if (c < 0)
      return 0;
   else if (c >= gNumCrates)
      return 0;
   else if (gUsb[c] == NULL)
      return 0;

   return 1;
}

/********************************************************************/
void camc(const int c, const int n, const int a, const int f)   // tested KO 17-SEP-2005
{
   ccusb_naf(gUsb[c], n, a, f, CCUSB_D16, 0);
}

/********************************************************************/
void camc_q(const int c, const int n, const int a, const int f, int *q) // tested KO 17-SEP-2005
{
   int status = ccusb_naf(gUsb[c], n, a, f, CCUSB_D16, 0);
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
void cam_inhibit_set(const int c)       // tested KO 16-SEP-2005
{
   camc(c, 29, 9, 24);
}

/********************************************************************/
void cam_inhibit_clear(const int c)     // tested KO 16-SEP-2005
{
   camc(c, 29, 9, 26);
}

/********************************************************************/
void cam_crate_clear(const int c)       // tested KO 16-SEP-2005
{
   camc(c, 28, 9, 29);
}

/********************************************************************/
void cam_crate_zinit(const int c)       // tested KO 16-SEP-2005
{
   camc(c, 28, 8, 29);
}

/********************************************************************/
void cam_lam_enable(const int c, const int n)   // tested KO 16-SEP-2005
{
   int mask;

   mask = ccusb_readCamacReg(ccusb_getCrateStruct(c), CCUSB_LAMMASK);
   mask |= (1 << (n - 1));
   ccusb_writeCamacReg(ccusb_getCrateStruct(c), CCUSB_LAMMASK, mask);
   camc(c, n, 0, 26);
   //ccusb_status(ccusb_getCrateStruct(c));
}

/********************************************************************/
void cam_lam_disable(const int c, const int n)  // tested KO 16-SEP-2005
{
   int mask;
   camc(c, n, 0, 24);
   mask = ccusb_readCamacReg(ccusb_getCrateStruct(c), CCUSB_LAMMASK);
   mask &= ~(1 << (n - 1));
   ccusb_writeCamacReg(ccusb_getCrateStruct(c), CCUSB_LAMMASK, mask);
}

/********************************************************************/
void cam_lam_read(const int c, DWORD * lam)     // tested KO 16-SEP-2005
{
   *lam = ccusb_readCamacReg(ccusb_getCrateStruct(c), CCUSB_LAM);
   //ccusb_status(ccusb_getCrateStruct(c));
}

/********************************************************************/
void cam_lam_clear(const int c, const int n)
{
   /* blank, same as the kcs2927 and ces8210 */
}

/********************************************************************/
void cam_exit(void) // untested KO 20-AUG-2007
{
   int i;

   for (i = 0; i < gNumCrates; i++)
      if (gUsb[i]) {
         musb_close(gUsb[i]);
         gUsb[i] = NULL;
      }

   gNumCrates = 0;
}

/********************************************************************/
int cam_init_rpc(char *host_name, char *exp_name, char *fe_name,
                 char *client_name, char *rpc_server)
{
   /* dummy routine for compatibility */
   return 1;
}

/********************************************************************/

// end
