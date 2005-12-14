/*********************************************************************

  Name:         VF48.h
  Created by:   Pierre-Andre Amaudruz / Jean-Pierre Martin

  Contents:     48 ch Flash ADC / 20-65msps from J.-P. Martin
                
  $Log: VF48.h,v $
*********************************************************************/
#
#include <stdio.h>
#include <string.h>
#include "vmicvme.h"
 
#ifndef  __VF48_INCLUDE_H__
#define  __VF48_INCLUDE_H__
 
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
#define  VF48_NFRAME_R             (DWORD) (0x00A0)  /**< -R-D16/32 */
#define  VF48_GLOBAL_RESET_W       (DWORD) (0x00B0)  /**< -W */
#define  VF48_DATA_FIFO_R          (DWORD) (0x0100)  /**< -R-D32 */
/*
Parameter frame
15 ...    ...  
CCCCDDDDRVPPPPPP
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
10    0x0000 Mbits
11    0x0001 Feature Delay B
12    0x0005 Latency
13    0x0100 Firmware ID
14    0x0190 Attenuator
15
16
*/

/* Parameters ID */
#define  VF48_GRP_OFFSET           (DWORD) (12)
#define  VF48_PARMA_BIT_RD         (DWORD) (0x80)
#define  VF48_PEDESTAL             (DWORD) (1)
#define  VF48_THRESHOLD            (DWORD) (2)
#define  VF48_CLIP_DELAY           (DWORD) (3)
#define  VF48_PRE_TRIGGER          (DWORD) (4)
#define  VF48_SEGMENT_SIZE         (DWORD) (5)
#define  VF48_K_COEF               (DWORD) (6)
#define  VF48_L_COEF               (DWORD) (7)
#define  VF48_M_COEF               (DWORD) (8)
#define  VF48_DELAY_A              (DWORD) (9)
#define  VF48_MBITS                (DWORD) (10)
#define  VF48_DELAY_B              (DWORD) (11)
#define  VF48_LATENCY              (DWORD) (12)
#define  VF48_FIRMWARE_ID          (DWORD) (13)
#define  VF48_ATTENUATOR           (DWORD) (14)

/* CSR bit assignment */
/*
  CSR setting:
  	0: Run  0:stop, 1:start
  	1: Parameter ID ready
  	2: Parameter Data ready
  	3: Event Fifo Not empty	
*/
#define  VF48_CSR_START_ACQ        (DWORD) (0x00000001)
#define  VF48_CSR_SUPPRESS_RAW     (DWORD) (0x00000010)
#define  VF48_CSR_SUPPRESS_ZERO    (DWORD) (0x00000020)
#define  VF48_CSR_EXT_TRIGGER      (DWORD) (0x00000080)
#define  VF48_CSR_FAKE_DATA        (DWORD) (0x00000040)
#define  VF48_CSR_FE_NOTEMPTY      (DWORD) (0x00000004)
#define  VF48_CSR_FE_FULL          (DWORD) (0x00008000)

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

int  VF48_EventRead(MVME_INTERFACE *myvme, DWORD base, DWORD *event, int *elements);
int  VF48_GroupRead(MVME_INTERFACE *myvme, DWORD base, DWORD *event, int grp, int *elements);
int  VF48_DataRead(MVME_INTERFACE *myvme, DWORD base, DWORD *event, int *elements);
int  VF48_ExtTrgSet(MVME_INTERFACE *myvme, DWORD base);
int  VF48_ExtTrgClr(MVME_INTERFACE *myvme, DWORD base);
void VF48_Reset(MVME_INTERFACE *myvme, DWORD base);
int  VF48_SegSizeSet(MVME_INTERFACE *myvme, DWORD base, int grp, int segsz);
int  VF48_SegSizeRead(MVME_INTERFACE *myvme, DWORD base, int grp);
int  VF48_PreTriggerSet(MVME_INTERFACE *myvme, DWORD base, int grp, int prtrg);
int  VF48_HitThresholdSet(MVME_INTERFACE *myvme, DWORD base, int grp, int wsz);
int  VF48_LatencySet(MVME_INTERFACE *myvme, DWORD base, int grp, int lat);
int  VF48_AcqStart(MVME_INTERFACE *myvme, DWORD base);
int  VF48_AcqStop(MVME_INTERFACE *myvme, DWORD base);
int  VF48_NFrameRead(MVME_INTERFACE *myvme, DWORD base); 
int  VF48_CsrRead(MVME_INTERFACE *myvme, DWORD base); 
int  VF48_FeFull(MVME_INTERFACE *myvme, DWORD base); 
int  VF48_EvtEmpty(MVME_INTERFACE *myvme, DWORD base); 
#endif

