/*********************************************************************
  Name:         vt2.h
  Created by:   Pierre-Andre Amaudruz

  Contents: Simple interface to a VME TDC function
  vme_poke -a VME_A24UD -d VME_D32 -A 0xe00008 0x1
  vme_peek -a VME_A24UD -d VME_D32 -A 0xe00004
  vme_peek -a VME_A24UD -d VME_D32 -A 0xe0000c

  $Id$
*********************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdio.h>

#ifndef  __VT2_INCLUDE_H__
#define  __VT2_INCLUDE_H__

#ifndef MIDAS_TYPE_DEFINED
#define MIDAS_TYPE_DEFINED

typedef unsigned char BYTE;
typedef unsigned short int WORD;
#ifndef OS_WINNT // Windows defines already DWORD
typedef unsigned int DWORD;
#endif

#ifndef OS_WINNT
#ifndef OS_VXWORKS
typedef DWORD BOOL;
#endif
#endif

#endif                          /* MIDAS_TYPE_DEFINED */

#define  VT2_MAX_CHANNELS    (DWORD) 2
#define  VT2_CSR_RO          (DWORD) (0x0)
#define  VT2_FIFOSTATUS_RO   (DWORD) (0x4)
#define  VT2_CYCLENUMBER_RO  (DWORD) (0x8)
#define  VT2_CTL_WO          (DWORD) (0xc)
#define  VT2_INTREG          (DWORD) (0x10)
#define  VT2_FIFO_RO         (DWORD) (0x14)
#define  VT2_MANRESET        (DWORD) (0x1)
#define  VT2_CYCLERESET      (DWORD) (0x2)
#define  VT2_KEEPALIVE       (DWORD) (0x4)
#define  VT2_INTENABLE       (DWORD) (0x8)
#define  VT2_FULL_FLAG       (DWORD) (0x10000)
#define  VT2_ALMOSTFULL_FLAG (DWORD) (0x20000)
#define  VT2_EMPTY_FLAG      (DWORD) (0x40000)

#define  VT2_HSTART          (DWORD) (1<<31)
#define  VT2_HIT1            (DWORD) (1<<30)
#define  VT2_HIT2            (DWORD) (1<<29)
#define  VT2_HSTOP           (DWORD) (1<<28)

#define  VT2_CYCLE_MASK      (DWORD) (0x03FF0000)
#define  VT2_CYCLE_SHIFT     (DWORD) (16)

#ifdef  __VMICVME_H__
int  vt2_CSRRead(MVME_INTERFACE *mvme, DWORD base);
void vt2_ManReset(MVME_INTERFACE *mvme, DWORD base);
int  vt2_FifoLevelRead(MVME_INTERFACE *mvme, DWORD base);
int  vt2_FifoRead(MVME_INTERFACE *mvme, DWORD base, DWORD *pdest, int evtcnt);
int  vt2_CycleNumberRead(MVME_INTERFACE *mvme, DWORD base);
void vt2_CycleReset(MVME_INTERFACE *mvme, DWORD base, int fset);
void vt2_KeepAlive(MVME_INTERFACE *mvme, DWORD base, int fset);
#endif

#endif
