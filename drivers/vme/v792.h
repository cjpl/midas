/*********************************************************************

  Name:         v792.h
  Created by:   Pierre-Andre Amaudruz

  Contents:     V792 32ch. QDC include
                
  $Log: v792.h,v $
*********************************************************************/
#include <stdio.h>
#include <string.h>
#include "mvmestd.h"

#ifndef  V792_INCLUDE_H
#define  V792_INCLUDE_H

#define  V792_MAX_CHANNELS    (DWORD) 32
#define  V792_REG_BASE        (DWORD) (0x1000)
#define  V792_FIRM_REV        (DWORD) (0x1000)
#define  V792_GEO_ADDR_RW     (DWORD) (0x1002)
#define  V792_BIT_SET1_RW     (DWORD) (0x1006)
#define  V792_BIT_CLEAR1_WO   (DWORD) (0x1008)
#define  V792_SOFT_RESET      (DWORD) (0x1<<7)
#define  V792_CSR1_RO         (DWORD) (0x100E)
#define  V792_SINGLE_RST_WO   (DWORD) (0x1016)
#define  V792_CSR2_RO         (DWORD) (0x1022)
#define  V792_EVT_CNT_L_RO    (DWORD) (0x1024)
#define  V792_EVT_CNT_H_RO    (DWORD) (0x1026)
#define  V792_INCR_EVT_WO     (DWORD) (0x1028)
#define  V792_INCR_OFFSET_WO  (DWORD) (0x102A)
#define  V792_DELAY_CLEAR_RW  (DWORD) (0x102E)
#define  V792_BIT_SET2_RW     (DWORD) (0x1032)
#define  V792_BIT_CLEAR2_WO   (DWORD) (0x1034)
#define  V792_TEST_EVENT_WO   (DWORD) (0x103E)
#define  V792_EVT_CNT_RST_WO  (DWORD) (0x1040)
#define  V792_THRES_BASE      (DWORD) (0x1080)

void v792_EvtCntRead(MVME_INTERFACE *mvme, DWORD base, DWORD *evtcnt);
void v792_EvtCntReset(MVME_INTERFACE *mvme, DWORD base);
void v792_CrateSet(MVME_INTERFACE *mvme, DWORD base, DWORD *evtcnt);
void v792_DelayClearSet(MVME_INTERFACE *mvme, DWORD base, int delay);
int  v792_DataRead(MVME_INTERFACE *mvme, DWORD base, DWORD *pdest, int *nentry);
int  v792_EventRead(MVME_INTERFACE *mvme, DWORD base, DWORD *pdest, int *nentry);
int  v792_ThresholdWrite(MVME_INTERFACE *mvme, DWORD base, WORD *threshold);
int  v792_ThresholdRead(MVME_INTERFACE *mvme, DWORD base, WORD *threshold);
int  v792_DataReady(MVME_INTERFACE *mvme, DWORD base);
void v792_SingleShotReset(MVME_INTERFACE *mvme, DWORD base);
void v792_Status(MVME_INTERFACE *mvme, DWORD base);
int  v792_CSR1Read(MVME_INTERFACE *mvme, DWORD base);
int  v792_CSR2Read(MVME_INTERFACE *mvme, DWORD base);
void v792_RegBit2Set(MVME_INTERFACE *mvme, DWORD base, int pat);
void v792_RegBit2Clear(MVME_INTERFACE *mvme, DWORD base, int pat);

#endif // V792_INCLUDE_H
