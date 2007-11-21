/*********************************************************************

  Name:         v792.h
  Created by:   Pierre-Andre Amaudruz
  Contributors: Sergio Ballestrero

  Contents:     V792 32ch. QDC include
                V785 32ch. Peak ADC include
                V792 and V785 are mostly identical;
                v792_ identifies commands supported by both
                v792_ identifies commands supported by V792 only
                v785_ identifies commands supported by V785 only

  $Id$
*********************************************************************/
#ifndef  V792_INCLUDE_H
#define  V792_INCLUDE_H

#include <stdio.h>
#include <string.h>
#include "mvmestd.h"

#ifdef __cplusplus
extern "C" {
#endif

#define  V792_MAX_CHANNELS    (DWORD) 32
#define  V792_REG_BASE        (DWORD) (0x1000)
#define  V792_FIRM_REV        (DWORD) (0x1000)
#define  V792_GEO_ADDR_RW     (DWORD) (0x1002)
#define  V792_BIT_SET1_RW     (DWORD) (0x1006)
#define  V792_BIT_CLEAR1_WO   (DWORD) (0x1008)
#define  V792_SOFT_RESET      (DWORD) (0x1<<7)
#define  V792_INT_LEVEL_WO    (DWORD) (0x100A)
#define  V792_INT_VECTOR_WO   (DWORD) (0x100C)
#define  V792_CSR1_RO         (DWORD) (0x100E)
#define  V792_CR1_RW          (DWORD) (0x1010)
#define  V792_SINGLE_RST_WO   (DWORD) (0x1016)
#define  V792_EVTRIG_REG_RW   (DWORD) (0x1020)
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
#define  V792_IPED_RW         (DWORD) (0x1060)
#define  V792_SWCOMM_WO       (DWORD) (0x1068)
#define  V785_SLIDECONST_RW   (DWORD) (0x106A)
#define  V792_THRES_BASE      (DWORD) (0x1080)

WORD v792_Read16(MVME_INTERFACE *mvme, DWORD base, int offset);
void v792_Write16(MVME_INTERFACE *mvme, DWORD base, int offset, WORD value);

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
void v792_IntSet(MVME_INTERFACE *mvme, DWORD base, int level, int vector);
void v792_IntDisable(MVME_INTERFACE *mvme, DWORD base);
void v792_IntEnable(MVME_INTERFACE *mvme, DWORD base, int level);
int  v792_CSR1Read(MVME_INTERFACE *mvme, DWORD base);
int  v792_CSR2Read(MVME_INTERFACE *mvme, DWORD base);
int  v792_BitSet2Read(MVME_INTERFACE *mvme, DWORD base);
void v792_BitSet2Set(MVME_INTERFACE *mvme, DWORD base, WORD pat);
void v792_BitSet2Clear(MVME_INTERFACE *mvme, DWORD base, WORD pat);
int  v792_DataReady(MVME_INTERFACE *mvme, DWORD base);
int  v792_isEvtReady(MVME_INTERFACE *mvme, DWORD base);
void v792_EvtTriggerSet(MVME_INTERFACE *mvme, DWORD base, int count);
void v792_DataClear(MVME_INTERFACE *mvme, DWORD base);
void v792_OnlineSet(MVME_INTERFACE *mvme, DWORD base);
void v792_LowThEnable(MVME_INTERFACE *mvme, DWORD base);
void v792_LowThDisable(MVME_INTERFACE *mvme, DWORD base);
void v792_EmptyEnable(MVME_INTERFACE *mvme, DWORD base);
void v792_EvtCntReset(MVME_INTERFACE *mvme, DWORD base);
int  v792_Setup(MVME_INTERFACE *mvme, DWORD base, int mode);
void v792_DelayClearSet(MVME_INTERFACE *mvme, DWORD base, int delay);
void v792_SoftReset(MVME_INTERFACE *mvme, DWORD base);
void v792_ControlRegister1Write(MVME_INTERFACE *mvme, DWORD base, WORD pat);
WORD v792_ControlRegister1Read(MVME_INTERFACE *mvme, DWORD base);
void v792_Trigger(MVME_INTERFACE *mvme, DWORD base);
int  v792_isPresent(MVME_INTERFACE *mvme, DWORD base);

  enum v792_DataType {
    v792_typeMeasurement=0,
    v792_typeHeader     =2,
    v792_typeFooter     =4,
    v792_typeFiller     =6
  };

  typedef union {
    DWORD raw;
    struct Entry {
      unsigned adc:12; // bit0 here
      unsigned ov:1;
      unsigned un:1;
      unsigned _pad_1:2;
      unsigned channel:5;
      unsigned _pad_2:3;
      unsigned type:3;
      unsigned geo:5;
    } data ;
    struct Header {
      unsigned _pad_1:8; // bit0 here
      unsigned cnt:6;
      unsigned _pad_2:2;
      unsigned crate:8;
      unsigned type:3;
      unsigned geo:5;
    } header;
    struct Footer {
      unsigned evtCnt:24; // bit0 here
      unsigned type:3;
      unsigned geo:5;
    } footer;
  } v792_Data;

  typedef union {
    DWORD raw;
    struct {
      unsigned DataReady:1; // bit0 here
      unsigned GlobalDataReady:1;
      unsigned Busy:1;
      unsigned GlobalBusy:1;
      unsigned Amnesia:1;
      unsigned Purge:1;
      unsigned TermOn:1;
      unsigned TermOff:1;
      unsigned EventReady:1; //bit 8 here
    };
  } v792_StatusRegister1;
  typedef union {
    DWORD raw;
    struct {
      unsigned _pad_1:1; // bit0 here
      unsigned BufferEmpty:1;
      unsigned BufferFull:1;
      unsigned _pad_2:1;
      unsigned PB:4;
      //unsigned DSEL0:1;
      //unsigned DSEL1:1;
      //unsigned CSEL0:1;
      //unsigned CSEL1:1;
    };
  } v792_StatusRegister2;
  typedef union {
    DWORD raw;
    struct {
      unsigned _pad_1:2;
      unsigned BlkEnd:1;
      unsigned _pad_2:1;
      unsigned ProgReset:1;
      unsigned BErr:1;
      unsigned Align64:1;
    };
  } v792_ControlRegister1;
  typedef union {
    DWORD raw;
    struct {
      unsigned MemTest:1;
      unsigned OffLine:1;
      unsigned ClearData:1;
      unsigned OverRange:1;
      unsigned LowThresh:1;
      unsigned _pad_1:1;//bit5
      unsigned TestAcq:1;
      unsigned SLDEnable:1;
      unsigned StepTH:1;
      unsigned _pad_2:2;//bits 9-10
      unsigned AutoIncr:1;
      unsigned EmptyProg:1;
      unsigned SlideSubEnable:1;
      unsigned AllTrg:1;
    };
  } v792_BitSet2Register;

  void v792_printEntry(const v792_Data* v);

#ifdef __cplusplus
}
#endif

#endif // V792_INCLUDE_H

/* emacs
 * Local Variables:
 * mode:C
 * mode:font-lock
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
