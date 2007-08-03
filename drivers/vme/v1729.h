/*********************************************************************

  Name:         v1729.h
  Created by:   Pierre-Andre Amaudruz

  Contents:     V1729 4channel /12 bit sampling ADC (1,2Gsps)

  $Log: v1729.h,v $
*********************************************************************/
#ifndef  V1729_INCLUDE_H
#define  V1729_INCLUDE_H

#include <stdio.h>
#include <string.h>

#include "mvmestd.h"

#define  V1729_SUCCESS                      1
#define  V1729_ERROR                       -1

#define  V1729_MAX_CHANNELS     (DWORD) 4
#define  V1729_MAX_CHANNEL_SIZE (DWORD) 2560
#define  V1729_RAM_SIZE         (DWORD) (4*2563)
#define  V1729_RESET_W          (DWORD) (0x0800)
#define  V1729_PRETRIG_LSB      (DWORD) (0x1800)
#define  V1729_PRETRIG_MSB      (DWORD) (0x1900)
#define  V1729_POSTTRIG_LSB     (DWORD) (0x1A00)
#define  V1729_POSTTRIG_MSB     (DWORD) (0x1B00)

#define  V1729_TRIGTYPE         (DWORD) (0x1D00)
#define  V1729_SOFT_TRIGGER      (DWORD)   (0x0)
#define  V1729_TRIG_ON_DISCR     (DWORD)   (0x1)
#define  V1729_EXTERNAL_TRIGGER  (DWORD)   (0x2)
#define  V1729_SOFT_OR_DISCR     (DWORD)   (0x3)
#define  V1729_RISING_EDGE       (DWORD)   (0x0)
#define  V1729_FALLING_EDGE      (DWORD)   (0x4)
#define  V1729_INHIBIT_RANDOM    (DWORD)   (0x0)
#define  V1729_AUTHORIZE_RANDOM  (DWORD)   (0x8)
#define  V1729_NORMAL_TRIGGER    (DWORD)   (0x0)
#define  V1729_EXT_TRIGG_NO_MASK (DWORD)  (0x10)

#define  V1729_TRIGCHAN         (DWORD) (0x1E00)
#define  V1729_DISABLE_ALL       (DWORD)   (0x0)
#define  V1729_ENABLE_CH1        (DWORD)   (0x1)
#define  V1729_ENABLE_CH2        (DWORD)   (0x2)
#define  V1729_ENABLE_CH3        (DWORD)   (0x4)
#define  V1729_ENABLE_CH4        (DWORD)   (0x8)
#define  V1729_ALL_FOUR          (DWORD)   (0xF)

#define  V1729_ACQ_START_W      (DWORD) (0x1700)
#define  V1729_SOFT_TRIGGER_W   (DWORD) (0x1C00)
#define  V1729_DATA_FIFO_R      (DWORD) (0x0D00)
#define  V1729_TRIGREC_R        (DWORD) (0x2000)
#define  V1729_FAST_READ_MODES  (DWORD) (0x2100)
#define  V1729_N_COL            (DWORD) (0x2200)
#define  V1729_CHANMASK         (DWORD) (0x2300)
#define  V1729_INTERRUPT_REG    (DWORD) (0x8000)
#define  V1729_FRQ_SAMPLING     (DWORD) (0x8100)

#define  V1729_1GSPS            (DWORD)  2
#define  V1729_2GSPS            (DWORD)  1

#define  V1729_VERSION_R        (DWORD) (0x8200)
#define  V1729_INTERRUPT_ENABLE (DWORD) (0x8300)   //

void v1729_AcqStart(MVME_INTERFACE *mvme, DWORD base);
void v1729_Reset(MVME_INTERFACE *mvme, DWORD base);
int  v1729_Setup(MVME_INTERFACE *mvme, DWORD base, int mode);
int  v1729_Status(MVME_INTERFACE *mvme, DWORD base);
int  v1729_isTrigger(MVME_INTERFACE *mvme, DWORD base);
void v1729_SoftTrigger(MVME_INTERFACE *mvme, DWORD base);
void v1729_PreTrigSet(MVME_INTERFACE *mvme, DWORD base, int value);
void v1729_PostTrigSet(MVME_INTERFACE *mvme, DWORD base, int value);
void v1729_TriggerTypeSet(MVME_INTERFACE *mvme, DWORD base, int value);
void v1729_ChannelSelect(MVME_INTERFACE *mvme, DWORD base, int value);
void v1729_FrqSamplingSet(MVME_INTERFACE *mvme, DWORD base, int value);
void v1729_NColsSet(MVME_INTERFACE *mvme, DWORD base, int value);
int  v1729_NColsGet(MVME_INTERFACE *mvme, DWORD base);
int  v1729_TimeCalibrationRun(MVME_INTERFACE *mvme, DWORD base, int flag);
int  v1729_PedestalRun(MVME_INTERFACE *mvme, DWORD base, int loop, int flag);
void v1729_DataRead(MVME_INTERFACE *mvme, DWORD base, WORD *pdest, int nch, int npt);
int  v1729_OrderData(MVME_INTERFACE *mvme, DWORD base, WORD *psrce, int *pdest, int nch, int ch, int npt);

#endif   // V1729_INCLUDE_H
