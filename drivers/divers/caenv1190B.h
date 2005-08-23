/*********************************************************************

  Name:         v1190B.h
  Created by:   Pierre-Andre Amaudruz

  Contents:     V1190B 64ch. TDC
                
  $Log$
  Revision 1.1  2005/08/23 22:27:20  olchanski
  TRIUMF drivers for sundry CAEN VME modules

*********************************************************************/

#ifndef V1190B_H_INCLUDED
#define V1190B_H_INCLUDED 1

#
/* V1190 Base address */
#define  V1190_MAX_CHANNELS      (uint16_t) 64
#define  V1190_REG_BASE          (uint16_t) (0x1000>>1)
#define  V1190_CR_RW             (uint16_t) (0x1000>>1)
#define  V1190_SR_RO             (uint16_t) (0x1002>>1)
#define  V1190_DATA_READY        (uint16_t) (0x0001)
#define  V1190_ALMOST_FULL       (uint16_t) (0x0002)
#define  V1190_FULL              (uint16_t) (0x0004)
#define  V1190_TRIGGER_MATCH     (uint16_t) (0x0008)
#define  V1190_HEADER_ENABLE     (uint16_t) (0x0010)
#define  V1190_GEO_REG_RW        (uint16_t) (0x001E)
#define  V1190_MODULE_RESET_WO   (uint16_t) (0x1014>>1)
#define  V1190_SOFT_CLEAR_WO     (uint16_t) (0x1016>>1)
#define  V1190_SOFT_EVT_RESET_WO (uint16_t) (0x1018>>1)
#define  V1190_SOFT_TRIGGER_WO   (uint16_t) (0x101A>>1)
#define  V1190_EVT_CNT_RO        (uint16_t) (0x101C>>1)
#define  V1190_EVT_STORED_RO     (uint16_t) (0x1020>>1)
#define  V1190_FIRM_REV_RO       (uint16_t) (0x1026>>1)
#define  V1190_MICRO_RW          (uint16_t) (0x102E>>1)
#define  V1190_MICRO_HAND_RO     (uint16_t) (0x1030>>1)
#define  V1190_MICRO_WR_OK       (uint16_t) (0x0001)
#define  V1190_MICRO_RD_OK       (uint16_t) (0x0002)
#define  V1190_MICRO_TDCID       (uint16_t) (0x6000)
#define  V1190_RESOLUTION_RO     (uint16_t) (0x2600)
#define  V1190_TRIGGER_MATCH_WO  (uint16_t) (0x0000)
#define  V1190_CONTINUOUS_WO     (uint16_t) (0x0100)
#define  V1190_ACQ_MODE_RO       (uint16_t) (0x0200)
 
int  udelay(int usec);
int  v1190_EventRead(DWORD *base, DWORD *pdest, int * nentry);
int  v1190_DataRead(DWORD *base, uint32_t *dest, int maxwords);
void v1190_SoftReset(uint16_t *pbase);
void v1190_SoftClear(uint16_t *pbase);
void v1190_SoftTrigger(uint16_t *pbase);
void v1190_DataReset(uint16_t *pbase);
int  v1190_DataReady(uint16_t *pbase);
int  v1190_EvtCounter(uint16_t *pbase);
int  v1190_EvtStored(uint16_t *pbase);
int  v1190_MicroCheck(const uint16_t *pbase, int what);
int  v1190_MicroWrite(uint16_t *pbase, uint16_t data);
int  v1190_MicroRead(const uint16_t *pbase);
int  v1190_MicroFlush(const uint16_t *pbase);
int  v1190B_TdcIdList(uint16_t *pbase);
int  v1190B_ResolutionRead(uint16_t *pbase);
int  v1190B_TriggerMatchingSet(uint16_t *pbase);
int  v1190B_AcqModeRead(uint16_t *pbase);
int  v1190B_ContinuousSet(uint16_t *pbase);
int  v1190_GeoWrite(WORD *pbase, int geo);
int  v1190_Setup(uint16_t *pbase, int mode);
int  v1190_Status(uint16_t *pbase);
void v1190_SetEdgeDetection(uint16_t *pbase,int enableLeading,int enableTrailing);

#endif
/* end */
