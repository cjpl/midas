/*********************************************************************

  Name:         vf48.h
  Created by:   Pierre-Andre Amaudruz / Jean-Pierre Martin

  Contents:     48 ch Flash ADC / 20-64 Msps from J.-P. Martin
                
  $Log: vf48.h,v $
*********************************************************************/
#
#include <stdio.h>
#include <string.h>
#include "vmicvme.h"
 
#ifndef  __VF48_INCLUDE_H__
#define  __VF48_INCLUDE_H__

/* Definitions */
#define VF48_IDXMAX 1024
 
/* Registers */
#define  VF48_MAX_CHANNELS         (DWORD) 48
#define  VF48_SUCCESS              (int)   (1)
#define  VF48_ERR_PARM             (int)   (-1)
#define  VF48_ERR_NODATA           (int)   (503)
#define  VF48_ERR_HW               (int)   (603)
#define  VF48_CSR_REG_RW           (DWORD) (0)          /**< -RW-D16/32 */
#define  VF48_SELECTIVE_SET_W      (DWORD) (0x0010)
#define  VF48_SELECTIVE_CLR_W      (DWORD) (0x0014)
#define  VF48_TEST_REG_RW          (DWORD) (0x0020)
#define  VF48_FIRMWARE_R           (DWORD) (0x0030)  /**< -R-D16/32 */
#define  VF48_PARAM_DATA_RW        (DWORD) (0x0050)  /**< -RW-D16/32 */
#define  VF48_PARAM_ID_W           (DWORD) (0x0060)  /**< -W-D16/32 */
#define  VF48_SOFT_TRIG_W          (DWORD) (0x0080)
#define  VF48_GRP_REG_RW           (DWORD) (0x0090)
#define  VF48_NFRAME_R             (DWORD) (0x00A0)  /**< -R-D16/32 */
#define  VF48_GLOBAL_RESET_W       (DWORD) (0x00B0)  /**< -W */
#define  VF48_DATA_FIFO_R          (DWORD) (0x0100)  /**< -R-D32 */
/*
Parameter frame
15 ...    ...  
CCCCDDDD RVPP PPPP
C: Destination Card/Port N/A
R: Read bit (0:Write, 1:Read)
D: Destination channels (0..5)
	 bit 11..8
	        0: channel 1..8
	        1: channel 9..16
	        2: channel 17..24
	        3: channel 25..32
	        4: channel 33..40
	        5: channel 41..48
	        6: channel N/C
	    7..15: channel N/C
V: Version 0 for now (0:D16, 1:D32(extended))
P: Parameter ID
	Default values for the different PIDs
ID#   Def Value
1     0x0000 Pedestal
2     0x000A Hit Det Threshold
3	    0x0028 Clip Delay
4	    0x0020 Pre-Trigger
5	    0x0100 Segment size
6	    0x0190 K-coeff 
7	    0x0200 L-coeff
8	    0x1000 M-coeff
9	    0x0005 Feature Delay A
10    0x0000 Mbit1
          0x1: Data simulation
          0x2: Supress Raw Data
          0x8: Inverse Signal
11    0x0001 Feature Delay B
12    0x0005 Latency
13    0x0100 Firmware ID
14    0x0190 Attenuator
15    0x0050 Trigger threshold
16    0x00FF Active Channel Mask
17    0x0000 Mbit2
          0x1: Enable Channel Suppress
       0xff00: sampling divisor         // Temporary
*/

/* Parameters ID for Frontend */
#define  VF48_GRP_OFFSET           (DWORD) (12)   
#define  VF48_PARMA_BIT_RD         (DWORD) (0x80) 
#define  VF48_PEDESTAL             (DWORD) (1)    //** 0x0000
#define  VF48_HIT_THRESHOLD        (DWORD) (2)    //** 0x000A
#define  VF48_CLIP_DELAY           (DWORD) (3)    //** 0x0028
#define  VF48_PRE_TRIGGER          (DWORD) (4)    //** 0x0020
#define  VF48_SEGMENT_SIZE         (DWORD) (5)    //** 0x0100
#define  VF48_K_COEF               (DWORD) (6)    //** 0x0190
#define  VF48_L_COEF               (DWORD) (7)    //** 0x0200
#define  VF48_M_COEF               (DWORD) (8)    //** 0x1000
#define  VF48_DELAY_A              (DWORD) (9)    //** 0x0005
#define  VF48_MBIT1                (DWORD) (10)   //** 0x0000
#define  VF48_DELAY_B              (DWORD) (11)   //** 0x0001
#define  VF48_LATENCY              (DWORD) (12)   //** 0x0005
#define  VF48_FIRMWARE_ID          (DWORD) (13)   //** 0x0100
#define  VF48_ATTENUATOR           (DWORD) (14)   //** 0x0190
#define  VF48_TRIG_THRESHOLD       (DWORD) (15)   //** 0x0050
// #define  VF48_ACTIVE_CH_MASK       (DWORD) (16)   //** 0x00FF
// #define  VF48_MBIT2                (DWORD) (17)   //** 0x0000

#define  VF48_ACTIVE_CH_MASK       (DWORD) (9)   //** 0x00FF << Temporary
#define  VF48_MBIT2                (DWORD) (11)   //** 0x0000 << Temporary

/* CSR bit assignment */
/*
  CSR setting:
  	0: Run  0:stop, 1:start
  	1: Parameter ID ready
  	2: Parameter Data ready
  	3: Event Fifo Not empty	
*/
#define  VF48_CSR_START_ACQ        (DWORD) (0x00000001)
#define  VF48_CSR_PARM_ID_RDY      (DWORD) (0x00000002)
#define  VF48_CSR_PARM_DATA_RDY    (DWORD) (0x00000004)
#define  VF48_CSR_FE_NOTEMPTY      (DWORD) (0x00000008)
#define  VF48_CSR_CRC_ERROR        (DWORD) (0x00000020)
#define  VF48_CSR_EXT_TRIGGER      (DWORD) (0x00000080)
#define  VF48_CSR_FE_FULL          (DWORD) (0x00008000)
#define  VF48_RAW_DISABLE           0x2
#define  VF48_CH_SUPPRESS_ENABLE    0x1
#define  VF48_INVERSE_SIGNAL        0x8
#define  VF48_ALL_CHANNELS_ACTIVE   0xFF
/* Header definition */
#define  VF48_HEADER               (DWORD) (0x80000000)
#define  VF48_TIME_STAMP           (DWORD) (0xA0000000)
#define  VF48_CHANNEL              (DWORD) (0xC0000000)
#define  VF48_DATA                 (DWORD) (0x00000000)
#define  VF48_CFD_FEATURE          (DWORD) (0x40000000)
#define  VF48_Q_FEATURE            (DWORD) (0x50000000)
#define  VF48_TRAILER              (DWORD) (0xE0000000)

//#define  VF48_OUT_OF_SYNC          (DWORD) (0x88000000)
//#define  VF48_TIMEOUT              (DWORD) (0x10000000)

int  vf48_Setup(MVME_INTERFACE *mvme, DWORD base, int mode);
int  vf48_EventRead(MVME_INTERFACE *myvme, DWORD base, DWORD *event, int *elements);
int  vf48_EventRead64(MVME_INTERFACE *myvme, DWORD base, DWORD *event, int *elements);
int  vf48_GroupRead(MVME_INTERFACE *myvme, DWORD base, DWORD *event, int grp, int *elements);
int  vf48_DataRead(MVME_INTERFACE *myvme, DWORD base, DWORD *event, int *elements);
int  vf48_ExtTrgSet(MVME_INTERFACE *myvme, DWORD base);
int  vf48_ExtTrgClr(MVME_INTERFACE *myvme, DWORD base);
void vf48_Reset(MVME_INTERFACE *myvme, DWORD base);
int  vf48_AcqStart(MVME_INTERFACE *myvme, DWORD base);
int  vf48_AcqStop(MVME_INTERFACE *myvme, DWORD base);
int  vf48_NFrameRead(MVME_INTERFACE *myvme, DWORD base); 
int  vf48_CsrRead(MVME_INTERFACE *myvme, DWORD base); 
int  vf48_GrpRead(MVME_INTERFACE *myvme, DWORD base); 
int  vf48_FeFull(MVME_INTERFACE *myvme, DWORD base); 
int  vf48_EvtEmpty(MVME_INTERFACE *myvme, DWORD base); 
int  vf48_GrpEnable(MVME_INTERFACE *myvme, DWORD base, int grpbit);
int  vf48_GrpRead(MVME_INTERFACE *myvme, DWORD base); 
int  vf48_GrpOperationMode(MVME_INTERFACE *myvme, DWORD base, int grp, int opmode);
int  vf48_ParameterRead(MVME_INTERFACE *myvme, DWORD base, int grp, int param);
int  vf48_ParameterWrite(MVME_INTERFACE *myvme, DWORD base, int grp, int param, int value);
int  vf48_ParameterCheck(MVME_INTERFACE *myvme, DWORD base, int what);
int  vf48_SegmentSizeSet(MVME_INTERFACE *mvme, DWORD base, DWORD size);
int  vf48_SegmentSizeRead(MVME_INTERFACE *mvme, DWORD base, int grp);
int  vf48_TrgThresholdSet(MVME_INTERFACE *mvme, DWORD base, int grp, DWORD size);
int  vf48_TrgThresholdRead(MVME_INTERFACE *mvme, DWORD base, int grp);
int  vf48_HitThresholdSet(MVME_INTERFACE *mvme, DWORD base, int grp, DWORD size);
int  vf48_HitThresholdRead(MVME_INTERFACE *mvme, DWORD base, int grp);
int  vf48_ActiveChMaskSet(MVME_INTERFACE *mvme, DWORD base, int grp, DWORD size);
int  vf48_ActiveChMaskRead(MVME_INTERFACE *mvme, DWORD base, int grp);
int  vf48_RawDataSuppSet(MVME_INTERFACE *mvme, DWORD base, int grp, DWORD size);
int  vf48_RawDataSuppRead(MVME_INTERFACE *mvme, DWORD base, int grp);
int  vf48_ChSuppSet(MVME_INTERFACE *mvme, DWORD base, int grp, DWORD size);
int  vf48_ChSuppRead(MVME_INTERFACE *mvme, DWORD base, int grp);
int  vf48_DivisorWrite(MVME_INTERFACE *mvme, DWORD base, DWORD size);
int  vf48_DivisorRead(MVME_INTERFACE *mvme, DWORD base, int grp);
#endif

