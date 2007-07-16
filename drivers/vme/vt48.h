/*********************************************************************

  Name:         VT48.h
  Created by:   Pierre-Andre Amaudruz, Chris Ohlmann

  Contents:      Routines for accessing the vt48 Triumf board

  $Id$
*********************************************************************/
#ifndef __VT48_INCLUDE_H__
#define __VT48_INCLUDE_H__

#include "mvmestd.h"
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VT48_SUCCESS                    1 // 
#define VT48_ERR_NODATA                10 // no data error... timeout 
#define VT48_PARAM_ERROR              100 // parameters error
#define VT48_CSR_RO            (DWORD) (0x0000) 
#define VT48_OCCUPANCY_RO      (DWORD) (0x0000) 
#define VT48_CMD_REG           (DWORD) (0x0004)
#define VT48_DATA_FIFO         (DWORD) (0x1000)
#define VT48_AMT_CFG_RW        (WORD)  (0x1)
#define VT48_AMT_STATUS_R      (WORD)  (0x2)
#define VT48_AMT_ID_R          (WORD)  (0x3)

#define VT48_HEADER            (DWORD) (0x10000000)
#define VT48_TRAILER           (DWORD) (0x80000000)

#define VT48_ID1_REG_RO        (DWORD) (0x0008)
#define VT48_ID2_REG_RO        (DWORD) (0x000C)

// Status Registers
#define VT48_CSR16_REG         (DWORD) (0x0020)
#define VT48_CSR17_REG         (DWORD) (0x0024)
#define VT48_CSR18_REG         (DWORD) (0x0028)
#define VT48_CSR19_REG         (DWORD) (0x002c)
#define VT48_CSR20_REG         (DWORD) (0x0030)
#define VT48_CSR21_REG         (DWORD) (0x0034)

#define VT48_CSR0_REG          (DWORD) (0x0040)
#define VT48_CSR1_REG          (DWORD) (0x0044)
#define VT48_CSR2_REG          (DWORD) (0x0048)
#define VT48_CSR3_REG          (DWORD) (0x004C)
#define VT48_CSR4_REG          (DWORD) (0x0050)
#define VT48_CSR5_REG          (DWORD) (0x0054)
#define VT48_CSR6_REG          (DWORD) (0x0058)
#define VT48_CSR7_REG          (DWORD) (0x005C)
#define VT48_CSR8_REG          (DWORD) (0x0060)
#define VT48_CSR9_REG          (DWORD) (0x0064)
#define VT48_CSR10_REG         (DWORD) (0x0068)
#define VT48_CSR11_REG         (DWORD) (0x006C)
#define VT48_CSR12_REG         (DWORD) (0x0070)
#define VT48_CSR13_REG         (DWORD) (0x0074)
#define VT48_CSR14_REG         (DWORD) (0x0078)

#define VT48_CSR0_RB_REG       (DWORD) (0x0080)
#define VT48_CSR1_RB_REG       (DWORD) (0x0084)
#define VT48_CSR2_RB_REG       (DWORD) (0x0088)
#define VT48_CSR3_RB_REG       (DWORD) (0x008C)
#define VT48_CSR4_RB_REG       (DWORD) (0x0090)
#define VT48_CSR5_RB_REG       (DWORD) (0x0094)
#define VT48_CSR6_RB_REG       (DWORD) (0x0098)
#define VT48_CSR7_RB_REG       (DWORD) (0x009C)
#define VT48_CSR8_RB_REG       (DWORD) (0x00A0)
#define VT48_CSR9_RB_REG       (DWORD) (0x00A4)
#define VT48_CSR10_RB_REG      (DWORD) (0x00A8)
#define VT48_CSR11_RB_REG      (DWORD) (0x00AC)
#define VT48_CSR12_RB_REG      (DWORD) (0x00B0)
#define VT48_CSR13_RB_REG      (DWORD) (0x00B4)
#define VT48_CSR14_RB_REG      (DWORD) (0x00B8)

/*-------------------------------------------*/  
/*
  enum vt48_DataType {
    vt48_typeasdx1 =0,
    vt48_typeasdx2 =1,
  };
*/
  
typedef union {
  DWORD id;
  struct Entry1 {
    unsigned id1:16;
    unsigned id2:16;
  } vtr0;
  struct csr0 {
    unsigned csr0:12;
    unsigned na1:4;
    unsigned csr1:12;
    unsigned na2:4;
  } vtr1;
} vt48_Reg;

  
int   vt48_EventRead(MVME_INTERFACE *myvme, DWORD base, DWORD *pdest, int *nentry);
void  vt48_RegWrite(MVME_INTERFACE *mvme, DWORD base, DWORD reg, DWORD data);
DWORD vt48_RegRead(MVME_INTERFACE *mvme, DWORD base, WORD reg);
void  vt48_StatusPrint(MVME_INTERFACE *mvme, DWORD base);
void  vt48_Status(MVME_INTERFACE *mvme, DWORD base);
int   vt48_Setup(MVME_INTERFACE *mvme, DWORD base, int mode); 
void  vt48_RegPrint(MVME_INTERFACE *mvme, DWORD base);
void  vt48_WindowSet(MVME_INTERFACE *mvme, DWORD base, float window);
void  vt48_WindowOffsetSet(MVME_INTERFACE *mvme, DWORD base, float offset);
      
#ifdef __cplusplus
}
#endif

#endif
