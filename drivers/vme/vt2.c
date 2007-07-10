/*********************************************************************

  Name:         vt2.c
  Created by:   Pierre-Andre Amaudruz

  Contents:

  $Id$
*********************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include "vmicvme.h"
#include "vt2.h"

#ifndef HAVE_UDELAY
// this is the VMIC version of udelay()
int udelay(int usec)
{
  int i, j, k = 0;
  for (i=0; i<133; i++)
    for (j=0; j<usec; j++)
      k += (k+1)*j*k*(j+1);
  return k;
}
#endif

/*****************************************************************/
/*
Read single event, return event length (number of entries)
Uses single vme access! (1us/D32)
*/
int vt2_FifoRead(MVME_INTERFACE *mvme, DWORD base, DWORD *pdest, int nentry)
{
  int cmode, i;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  for (i=0 ; i<nentry ; i++) {
    pdest[i] = mvme_read_value(mvme, base+VT2_FIFO_RO);
    //    printf("pdest[%i] = %lx\n", i, pdest[i]);
  }
  mvme_set_dmode(mvme, cmode);
  return nentry;
}

/*****************************************************************/
/**
 return the Fifo level which is 64bits x 2048 with Almost full at 2000.
 the returned value is on 11 bits. It should be used as 2*level during read
 as the data is 64 bit wide.
 Data field 20bits : [0eaf][0000 0] 7FF fEmpty, fAlmostFull, Full, level
*/
int vt2_FifoLevelRead(MVME_INTERFACE *mvme, DWORD base)
{
  int  status, cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  status = mvme_read_value(mvme, base+VT2_FIFOSTATUS_RO);
  if (status & VT2_FULL_FLAG)
    status = -1;
  else
    status &= 0x7FF;
  mvme_set_dmode(mvme, cmode);
  return 2*status;
}

/*****************************************************************/
/**
 return the cycle number (10 bit)
*/
int vt2_CycleNumberRead(MVME_INTERFACE *mvme, DWORD base)
{
  int  status, cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  status = mvme_read_value(mvme, base+VT2_CYCLENUMBER_RO);
  status &= 0x3FF;
  mvme_set_dmode(mvme, cmode);
  return status;
}

/*****************************************************************/
int vt2_CSRRead(MVME_INTERFACE *mvme, DWORD base)
{
  int  status, cmode;

  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  status = mvme_read_value(mvme, base+VT2_CSR_RO);
  mvme_set_dmode(mvme, cmode);
  return status;
}

/*****************************************************************/
void vt2_ManReset(MVME_INTERFACE *mvme, DWORD base)
{
  int cmode;
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  mvme_write_value(mvme, base+VT2_CTL_WO, VT2_MANRESET);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void vt2_IntEnable(MVME_INTERFACE *mvme, DWORD base, int flag)
{
  int cmode;
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  if (flag == 1)
    mvme_write_value(mvme, base+VT2_CTL_WO, VT2_INTENABLE);
  else
    mvme_write_value(mvme, base+VT2_CTL_WO, VT2_INTENABLE<<16);

  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void vt2_KeepAlive(MVME_INTERFACE *mvme, DWORD base, int fset)
{
  int flag, cmode;
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  if (fset == 0)
    flag = (VT2_KEEPALIVE<<16);
  else
    flag = (VT2_KEEPALIVE);
  mvme_write_value(mvme, base+VT2_CTL_WO, flag);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
void vt2_CycleReset(MVME_INTERFACE *mvme, DWORD base, int fset)
{
  int flag, cmode;
  mvme_get_dmode(mvme, &cmode);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  if (fset == 0)
    flag = (VT2_CYCLERESET<<16);
  else
    flag = VT2_CYCLERESET;
  printf("0x%x  flag:0x%x\n", base+VT2_CTL_WO, flag);
  mvme_write_value(mvme, base+VT2_CTL_WO, flag);
  mvme_set_dmode(mvme, cmode);
}

/*****************************************************************/
/*-PAA- For test purpose only */
#ifdef MAIN_ENABLE
int main () {

  DWORD VT2_BASE = 0xe00000;
  DWORD dest[4000];
  int n;

  MVME_INTERFACE *myvme;

  int status, csr, i;

  // Test under vmic
  status = mvme_open(&myvme, 0);

  // Set am to A24 non-privileged Data
  mvme_set_am(myvme, MVME_AM_A24_ND);

  // Set dmode to D32
  mvme_set_dmode(myvme, MVME_DMODE_D32);

  csr = vt2_CSRRead(myvme, VT2_BASE);
  printf("CSR: 0x%x\n", csr);


  if (1) {
    printf("Manual Reset\n");
    vt2_ManReset(myvme, VT2_BASE);
  }

  if (0) {
    printf("Set Keep alive\n");
    vt2_KeepAlive(myvme, VT2_BASE, 1);
  }

  if (0) {
    printf("ReSet Keep alive\n");
    vt2_KeepAlive(myvme, VT2_BASE, 0);
  }

  if (0) {
    printf("Set Cycle Reset\n");
    vt2_CycleReset(myvme, VT2_BASE, 1);
  }

  if (0) {
    printf("ReSet Cycle Reset\n");
    vt2_CycleReset(myvme, VT2_BASE, 0);
  }

  udelay(100000);
  if (1) {
    printf("Read Fifo status\n");
    csr = vt2_CSRRead(myvme, VT2_BASE);
    printf("CSR : 0x%x\n", csr);
    csr = vt2_CycleNumberRead(myvme, VT2_BASE);
    printf("Read Cycle Number: %d\n", csr);
    csr = vt2_FifoLevelRead(myvme, VT2_BASE);
    printf("Read Fifo Level: %d\n", csr);
  }

  if (1) {
    printf("Read Fifo (16)\n");
    n = 16;
    vt2_FifoRead(myvme, VT2_BASE, &(dest[0]), n);
    for (i=0;i<n;i+=4) {
      printf("Data[%i]=%lx %lx %lx %lx  Diff:%ld (0x%lx)\n", i, dest[i], dest[i+1], dest[i+2], dest[i+3]
       , dest[i+3]-dest[i+1], dest[i+3]-dest[i+1]);
    }
  }

  status = mvme_close(myvme);
  return 1;
}
#endif

