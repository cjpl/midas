/*********************************************************************

  Name:         v792.h
  Created by:   Pierre-Andre Amaudruz

  Contents:     V792 32ch. QDC include
                
  $Id:$

*********************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdio.h>

#ifndef  __V792_INCLUDE_H__
#define  __V792_INCLUDE_H__

#define  V792_MAX_CHANNELS    (WORD) 32
#define  V792_REG_BASE        (WORD) (0x1000>>1)
#define  V792_FIRM_REV        (WORD) (0x1000>>1)
#define  V792_GEO_ADDR_RW     (WORD) (0x1002>>1)
#define  V792_BIT_SET1_RW     (WORD) (0x1006>>1)
#define  V792_BIT_CLEAR1_WO   (WORD) (0x1008>>1)
#define  V792_SOFT_RESET      (WORD) (0x1<<7)
#define  V792_CSR1_RO         (WORD) (0x100E>>1)
#define  V792_SINGLE_RST_WO   (WORD) (0x1016>>1)
#define  V792_EVT_CNT_L_RO    (WORD) (0x1024>>1)
#define  V792_EVT_CNT_H_RO    (WORD) (0x1026>>1)
#define  V792_INCR_EVT_WO     (WORD) (0x1028>>1)
#define  V792_INCR_OFFSET_WO  (WORD) (0x102A>>1)
#define  V792_BIT_SET2_RW     (WORD) (0x1032>>1)
#define  V792_BIT_CLEAR2_WO   (WORD) (0x1034>>1)
#define  V792_TEST_EVENT_WO   (WORD) (0x103E>>1)
#define  V792_EVT_CNT_RST_WO  (WORD) (0x1040>>1)
#define  V792_THRES_BASE      (WORD) (0x1080>>1)

void v792_EvtCntRead(WORD *pbase, DWORD *evtcnt);
void v792_CrateSet(WORD *pbase, DWORD *evtcnt);
int  v792_DataRead(DWORD *base, DWORD *pdest, int *nentry);
int  v792_EventRead(DWORD *base, DWORD *pdest, int *nentry);
int  v792_ThresholdWrite(WORD *base, WORD *threshold, int *nitems);
int  v792_DataReady(WORD *pbase);
void v792_SingleShotReset(WORD *pbase);
void v792_Status(WORD *pbase);
int  v792_GeoWrite(WORD *pbase, int geo);


#endif
