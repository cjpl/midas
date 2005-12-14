/*********************************************************************

  Name:         vmeio.c
  Created by:   Pierre-Andre Amaudruz

  Cotents:      Routines for accessing the VMEIO Triumf board
                
  $Log: vmeio.c,v $
*********************************************************************/

#include <stdio.h>
#include <string.h>
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
  mvme_write_value(myvme, base+VMEIO_RDCNTL, 0);
}

/********************************************************************/
#ifdef MAIN_ENABLE
int main () {
  
  MVME_INTERFACE *myvme;

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

  status = mvme_close(myvme);
  return 1;
}	
#endif
