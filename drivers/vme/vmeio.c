/*********************************************************************

  Name:         vmeio.c
  Created by:   Pierre-Andre Amaudruz

  Cotents:      Routines for accessing the VMEIO Triumf board

  $Id$
*********************************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "vmeio.h"

/********************************************************************/
/**
Set output in pulse mode
@param myvme vme structure
@param base  VMEIO base address
@param data  data to be written
@return void
*/
void vmeio_OutputSet(MVME_INTERFACE *myvme, DWORD base, DWORD data)
{
  mvme_set_am(myvme, MVME_AM_A24_ND);
  mvme_set_dmode(myvme, MVME_DMODE_D32);
  mvme_write_value(myvme, base+VMEIO_OUTSET, data & 0xFFFFFF);
}

/********************************************************************/
/**
Write to the sync output (pulse mode)
@param myvme vme structure
@param base  VMEIO base address
@param data  data to be written
@return void
*/
void vmeio_SyncWrite(MVME_INTERFACE *myvme, DWORD base, DWORD data)
{
  mvme_set_am(myvme, MVME_AM_A24_ND);
  mvme_set_dmode(myvme, MVME_DMODE_D32);
  mvme_write_value(myvme, base+VMEIO_OUTPULSE, data & 0xFFFFFF);
}

/********************************************************************/
/**
Writee to the Async output (latch mode)
@param myvme vme structure
@param base  VMEIO base address
@param data  data to be written
@return void
*/
void vmeio_AsyncWrite(MVME_INTERFACE *myvme, DWORD base, DWORD data)
{
  mvme_set_am(myvme, MVME_AM_A24_ND);
  mvme_set_dmode(myvme, MVME_DMODE_D32);
  mvme_write_value(myvme, base+VMEIO_OUTLATCH, data & 0xFFFFFF);
}

/********************************************************************/
/**
Read the CSR register
@param myvme vme structure
@param base  VMEIO base address
@return CSR status
*/
int vmeio_CsrRead(MVME_INTERFACE *myvme, DWORD base)
{
  int csr;
  mvme_set_am(myvme, MVME_AM_A24_ND);
  mvme_set_dmode(myvme, MVME_DMODE_D32);
  csr = mvme_read_value(myvme, base+VMEIO_RDCNTL);
  return (csr & 0xFF);
}

/********************************************************************/
/**
Read from the Async register
@param myvme vme structure
@param base  VMEIO base address
@return Async_Reg
*/
int vmeio_AsyncRead(MVME_INTERFACE *myvme, DWORD base)
{
  int csr;
  mvme_set_am(myvme, MVME_AM_A24_ND);
  mvme_set_dmode(myvme, MVME_DMODE_D32);
  csr = mvme_read_value(myvme, base+VMEIO_RDASYNC);
  return (csr & 0xFFFFFF);
}

/********************************************************************/
/**
Read from the Sync register
@param myvme vme structure
@param base  VMEIO base address
@return Sync_Reg
*/
int vmeio_SyncRead(MVME_INTERFACE *myvme, DWORD base)
{
  int csr;
  mvme_set_am(myvme, MVME_AM_A24_ND);
  mvme_set_dmode(myvme, MVME_DMODE_D32);
  csr = mvme_read_value(myvme, base+VMEIO_RDSYNC);
  return (csr & 0xFFFFFF);
}

/********************************************************************/
/**
Clear Strobe input
@param myvme vme structure
@param base  VMEIO base address
@return void
*/
void vmeio_StrobeClear(MVME_INTERFACE *myvme, DWORD base)
{
  mvme_set_am(myvme, MVME_AM_A24_ND);
  mvme_set_dmode(myvme, MVME_DMODE_D32);
  mvme_write_value(myvme, base+VMEIO_RDCNTL, 0);
}

/********************************************************************/
/**
Enable Interrupt source.
Only any of the first 8 inputs can generate interrupt.
@param myvme vme structure
@param base  VMEIO base address
@param input inputs 0..7 (LSB)
@return void
*/
void vmeio_IntEnable(MVME_INTERFACE *myvme, DWORD base, int input)
{
  mvme_set_am(myvme, MVME_AM_A24_ND);
  mvme_set_dmode(myvme, MVME_DMODE_D32);
  mvme_write_value(myvme, base+VMEIO_IRQENBL, input);
}

/********************************************************************/
/**
Select Interrupt source and arm interrupt
The CSR should be reset before this operation.
In Sync mode the strobe and the input have to be in coincidence.
In Async mode a logical level on the input will trigger the interrupt.
@param myvme vme structure
@param base  VMEIO base address
@param input inputs 0..7 if 1=> Sync, 0=> Async
@return void
*/
void vmeio_IntRearm(MVME_INTERFACE *myvme, DWORD base, int input)
{
  mvme_set_am(myvme, MVME_AM_A24_ND);
  mvme_set_dmode(myvme, MVME_DMODE_D32);
  mvme_write_value(myvme, base+VMEIO_INTSRC, input);
}


/********************************************************************/
/********************************************************************/
static void myisrvmeio(int sig, siginfo_t * siginfo, void *extra)
{
  fprintf(stderr, "myisrvmeio interrupt received \n");
}


#ifdef MAIN_ENABLE
int main () {

  MVME_INTERFACE *myvme;
  int myinfo = 1; // VME_INTERRUPT_SIGEVENT;

  DWORD VMEIO_BASE = 0x780000;
  int status, csr;
  int      data=0xf;

  // Test under vmic
  status = mvme_open(&myvme, 0);

  // Set am to A24 non-privileged Data
  mvme_set_am(myvme, MVME_AM_A24_ND);

  // Set dmode to D32
  mvme_set_dmode(myvme, MVME_DMODE_D32);

  csr = vmeio_CsrRead(myvme, VMEIO_BASE);
  printf("CSR Buffer: 0x%x\n", csr);

  if (0) {
    // Set 0xF in pulse mode
    vmeio_OutputSet(myvme, VMEIO_BASE, 0xF);

    // Write latch mode
    vmeio_AsyncWrite(myvme, VMEIO_BASE, 0xc);
    vmeio_AsyncWrite(myvme, VMEIO_BASE, 0x0);

    // Read from the Async Reg
    data = vmeio_AsyncRead(myvme, VMEIO_BASE);
    printf("Async Buffer: 0x%x\n", data);

    // Read from the Sync Reg
    data = vmeio_SyncRead(myvme, VMEIO_BASE);
    printf("Sync Buffer: 0x%x\n", data);

    for (;;) {
      // Write pulse
      vmeio_SyncWrite(myvme, VMEIO_BASE, 0xF);
      vmeio_SyncWrite(myvme, VMEIO_BASE, 0xF);
    }
  }

  // Interrupt test
  if (0) {
    mvme_interrupt_attach(myvme, 7, 0x80, myisrvmeio, &myinfo);

    // Pulse on out 2 only
    vmeio_OutputSet(myvme, VMEIO_BASE, 0x2);

    // Enable Interrupts
    vmeio_IntEnable(myvme, VMEIO_BASE, 0x3);

    // Clear CSR
    vmeio_StrobeClear(myvme, VMEIO_BASE);

    // Select type and Rearm
    vmeio_IntRearm(myvme, VMEIO_BASE, 3);

    // Delay int generation
    //    udelay(1000000);

    // Write latch mode
    vmeio_AsyncWrite(myvme, VMEIO_BASE, 0xc);
    vmeio_AsyncWrite(myvme, VMEIO_BASE, 0x0);

    mvme_interrupt_detach(myvme, 1, 0x00, &myinfo);
  }

  // Interrupt test
  if (1) {
    /*
      start the code for listening to the IRQ7 vector:0x80
      use the script sample below to issue the interrupt.
      # set output latch mode
      vme_poke -a VME_A24UD -d VME_D32 -A 0x780008 0x0
      # write 1 latch on output 1  --> interrupt generated
      vme_poke -a VME_A24UD -d VME_D32 -A 0x780010 0x1
      # Enable interrupt 1
      vme_poke -a VME_A24UD -d VME_D32 -A 0x780000 0x1
      # Clear CSR
      vme_poke -a VME_A24UD -d VME_D32 -A 0x78001c 0x0
      # Rearm interrupt source for Async
      vme_poke -a VME_A24UD -d VME_D32 -A 0x780004 0x0
      # write 0 latch on output 1
      vme_poke -a VME_A24UD -d VME_D32 -A 0x780010 0x0
      # display CSR
      vme_peek -a VME_A24UD -d VME_D32 -A 0x78001c
     */

    mvme_interrupt_attach(myvme, 7, 0x80, myisrvmeio, &myinfo);

    for (;;) {
      printf(".");
      sleep(1);
      fflush(stdout);
    }

    mvme_interrupt_detach(myvme, 1, 0x00, &myinfo);
  }
  status = mvme_close(myvme);
  return 1;
}
#endif
