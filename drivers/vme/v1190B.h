/*********************************************************************

  Name:         v1190B.h
  Created by:   Pierre-Andre Amaudruz

  Contents:     V1190B 64ch. TDC

  $Id: v1190B.h 4613 2009-10-24 04:14:13Z olchanski $
*********************************************************************/

#include "mvmestd.h"

#ifndef V1190B_INCLUDE_H
#define V1190B_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

/* V1190 Base address */
#define  V1190_MAX_CHANNELS      (DWORD) 64
#define  V1190_REG_BASE          (DWORD) (0x1000)
#define  V1190_CR_RW             (DWORD) (0x1000)
#define  V1190_SR_RO             (DWORD) (0x1002)
#define  V1190_DATA_READY        (DWORD) (0x0001)
#define  V1190_ALMOST_FULL       (DWORD) (0x0002)
#define  V1190_FULL              (DWORD) (0x0004)
#define  V1190_TRIGGER_MATCH     (DWORD) (0x0008)
#define  V1190_HEADER_ENABLE     (DWORD) (0x0010)
#define  V1190_GEO_REG_RW        (DWORD) (0x001E)
#define  V1190_MODULE_RESET_WO   (DWORD) (0x1014)
#define  V1190_SOFT_CLEAR_WO     (DWORD) (0x1016)
#define  V1190_SOFT_EVT_RESET_WO (DWORD) (0x1018)
#define  V1190_SOFT_TRIGGER_WO   (DWORD) (0x101A)
#define  V1190_EVT_CNT_RO        (DWORD) (0x101C)
#define  V1190_EVT_STORED_RO     (DWORD) (0x1020)
#define  V1190_FIRM_REV_RO       (DWORD) (0x1026)
#define  V1190_MICRO_HAND_RO     (DWORD) (0x1030)
#define  V1190_MICRO_RW          (DWORD) (0x102E)
/* Micro code IDs */
#define  V1190_WINDOW_WIDTH_WO   (WORD) (0x1000)
#define  V1190_WINDOW_OFFSET_WO  (WORD) (0x1100)
#define  V1190_MICRO_WR_OK       (WORD) (0x0001)
#define  V1190_MICRO_RD_OK       (WORD) (0x0002)
#define  V1190_MICRO_TDCID       (WORD) (0x6000)
#define  V1190_EDGE_DETECTION_WO (WORD) (0x2200)
#define  V1190_LE_RESOLUTION_WO  (WORD) (0x2400)
#define  V1190_LEW_RESOLUTION_WO (WORD) (0x2500)
#define  V1190_RESOLUTION_RO     (WORD) (0x2600)
#define  V1190_TRIGGER_MATCH_WO  (WORD) (0x0000)
#define  V1190_CONTINUOUS_WO     (WORD) (0x0100)
#define  V1190_ACQ_MODE_RO       (WORD) (0x0200)

#define  LE_RESOLUTION_100       (WORD) (0x10)
#define  LE_RESOLUTION_200       (WORD) (0x01)
#define  LE_RESOLUTION_800       (WORD) (0x00)

int  udelay(int usec);
WORD v1190_Read16(MVME_INTERFACE *mvme, DWORD base, int offset);
DWORD v1190_Read32(MVME_INTERFACE *mvme, DWORD base, int offset);
void v1190_Write16(MVME_INTERFACE *mvme, DWORD base, int offset, WORD value);
int  v1190_EventRead(MVME_INTERFACE *mvme, DWORD base, DWORD *pdest, int *nentry);
int  v1190_DataRead(MVME_INTERFACE *mvme, DWORD base, DWORD *pdest, int nentry);
void v1190_SoftReset(MVME_INTERFACE *mvme, DWORD base);
void v1190_SoftClear(MVME_INTERFACE *mvme, DWORD base);
void v1190_SoftTrigger(MVME_INTERFACE *mvme, DWORD base);
void v1190_DataReset(MVME_INTERFACE *mvme, DWORD base);
int  v1190_DataReady(MVME_INTERFACE *mvme, DWORD base);
int  v1190_EvtCounter(MVME_INTERFACE *mvme, DWORD base);
int  v1190_EvtStored(MVME_INTERFACE *mvme, DWORD base);
int  v1190_MicroCheck(MVME_INTERFACE *mvme, const DWORD base, int what);
int  v1190_MicroWrite(MVME_INTERFACE *mvme, DWORD base, WORD data);
int  v1190_MicroRead(MVME_INTERFACE *mvme, const DWORD base);
int  v1190_MicroFlush(MVME_INTERFACE *mvme, const DWORD base);
void v1190_TdcIdList(MVME_INTERFACE *mvme, DWORD base);
int  v1190_ResolutionRead(MVME_INTERFACE *mvme, DWORD base);
void v1190_LEResolutionSet(MVME_INTERFACE *mvme, DWORD base, WORD le);
void v1190_LEWResolutionSet(MVME_INTERFACE *mvme, DWORD base, WORD le, WORD width);
void v1190_TriggerMatchingSet(MVME_INTERFACE *mvme, DWORD base);
void v1190_AcqModeRead(MVME_INTERFACE *mvme, DWORD base);
void v1190_ContinuousSet(MVME_INTERFACE *mvme, DWORD base);
void v1190_WidthSet(MVME_INTERFACE *mvme, DWORD base, WORD width);
void v1190_OffsetSet(MVME_INTERFACE *mvme, DWORD base, WORD offset);
void v1190_WidthSet_ns(MVME_INTERFACE *mvme, DWORD base, int width_ns);
void v1190_OffsetSet_ns(MVME_INTERFACE *mvme, DWORD base, int offset_ns);
int  v1190_GeoWrite(MVME_INTERFACE *mvme, DWORD base, int geo);
int  v1190_Setup(MVME_INTERFACE *mvme, DWORD base, int mode);
int  v1190_Status(MVME_INTERFACE *mvme, DWORD base);
void v1190_SetEdgeDetection(MVME_INTERFACE *mvme, DWORD base, int eLeading, int eTrailing);

#ifdef __cplusplus
}
#endif
#endif // V1190B_INCLUDE_H
