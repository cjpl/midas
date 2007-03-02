/*   vppg.h
     Author: Suzannah Daviel 

     Include file for PPG (Pulse Programmer) for BNMR

  $Id$
*/
#ifndef _VPPG_INCLUDE_H_
#define _VPPG_INCLUDE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MIDAS_TYPE_DEFINED
#define MIDAS_TYPE_DEFINED
typedef unsigned short int WORD;
typedef int                INT;
typedef char               BYTE;
#ifdef __alpha
typedef unsigned int       DWORD;
#else
typedef unsigned long int  DWORD;
#endif
#endif /* MIDAS_TYPE_DEFINED */

#include "vmicvme.h"

// Masks
#define VPPG_DEFAULT_PPG_POL_MSK 0x000000  

// Registers
#define VPPG_PPG_RESET_REG       0x00   /* W     BYTE   W  */
#define VPPG_PPG_START_TRIGGER   0x01   /* W     BYTE   W  */
#define VPPG_BYTES_PER_WORD      0x02   /* 8     BYTE  RW  */
#define VPPG_TOGL_MEM_DEVICE     0x03   /* W     BYTE   W  */
#define VPPG_CLEAR_ADDR_COUNTER  0x04   /* W     BYTE   W  */
#define VPPG_LOAD_MEM            0x06   /* 1     BYTE  RW  */
#define VPPG_PROGRAMMING_FIN     0x07   /* 1     BYTE  RW  */
#define VPPG_OUTP_POL_MASK_HI    0x08   /* 8     BYTE  RW  */
#define VPPG_OUTP_POL_MASK_MID   0x09   /* 8     BYTE  RW  */
#define VPPG_OUTP_POL_MASK_LO    0x0A   /* 8     BYTE  RW  */
#define VPPG_POLZ_SOURCE_CONTROL 0x0B   /* 2     BYTE  RW  */
#define VPPG_VME_POLZ_SET        0x0C   /* 1     BYTE  RW  */
#define VPPG_VME_READ_STAT_REG   0x0D   /* 8     BYTE  RO  */
#define VPPG_VME_TRIGGER_REG     0x0E   /* 8     BYTE  RW  */
#define VPPG_VME_RESET           0x0F   /* 8     BYTE  RW  */
#define VPPG_VME_RESET_COUNTERS  0x10   /* 8     BYTE  R0  */
#define VPPG_VME_MCS_COUNT_HI    0X11   /* 8     BYTE  R0  */
#define VPPG_VME_MCS_COUNT_MH    0X12   /* 8     BYTE  R0  */
#define VPPG_VME_MCS_COUNT_ML    0X13   /* 8     BYTE  R0  */
#define VPPG_VME_MCS_COUNT_LO    0X14   /* 8     BYTE  R0  */
#define VPPG_VME_CYC_COUNT_HI    0X15   /* 8     BYTE  R0  */
#define VPPG_VME_CYC_COUNT_MH    0X16   /* 8     BYTE  R0  */
#define VPPG_VME_CYC_COUNT_ML    0X17   /* 8     BYTE  R0  */
#define VPPG_VME_CYC_COUNT_LO    0X18   /* 8     BYTE  R0  */
#define VPPG_VME_BEAM_CTL        0X19   /* 2     BYTE  RW  */
#define VPPG_VME_TRIG_CTL        0X1A   /* 2     BYTE  RW  */

struct parameters
{
    unsigned char   opcode;
    unsigned long   branch_addr;
    unsigned long   delay;
    unsigned long   flags;
    char            opcode_width;
    char            branch_width;
    char            delay_width;
    char            flag_width;
};
typedef struct parameters PARAM;

void  VPPGInit(MVME_INTERFACE *mvme, const DWORD base_adr);
int   VPPGLoad(MVME_INTERFACE *mvme,const DWORD base_adr, char *file);
DWORD VPPGPolmskWrite(MVME_INTERFACE *mvme, const DWORD base_adr, const DWORD pol);
DWORD VPPGPolmskRead(MVME_INTERFACE *mvme, const DWORD base_adr);
void  VPPGDisable(MVME_INTERFACE *mvme, const DWORD base_adr);
void  VPPGEnable(MVME_INTERFACE *mvme, const DWORD base_adr);
BYTE  VPPGStatusRead(MVME_INTERFACE *mvme, const DWORD base_adr);
PARAM lineRead( FILE *input);
void  byteOutputOrder(PARAM data, char *array);
BYTE  VPPGRegWrite(MVME_INTERFACE *mvme, const DWORD base_adr, 
			 DWORD reg_offset, BYTE value);
BYTE  VPPGRegRead(MVME_INTERFACE *mvme, const DWORD base_adr, 
			 DWORD reg_offset);
void  VPPGPolzSet(MVME_INTERFACE *mvme, const DWORD base_adr, BYTE value );
BYTE  VPPGPolzFlip(MVME_INTERFACE *mvme, const DWORD base_adr);
BYTE  VPPGPolzRead(MVME_INTERFACE *mvme, const DWORD base_adr);
void  VPPGBeamOn(MVME_INTERFACE *mvme, const DWORD base_adr);
void  VPPGBeamOff(MVME_INTERFACE *mvme, const DWORD base_adr);
void  VPPGBeamCtlPPG(MVME_INTERFACE *mvme, const DWORD base_adr);
BYTE  VPPGBeamCtlRegRead(MVME_INTERFACE *mvme, const DWORD base_adr);
void  VPPGPolzCtlVME(MVME_INTERFACE *mvme, const DWORD base_adr);
void  VPPGPolzCtlPPG(MVME_INTERFACE *mvme, const DWORD base_adr);
BYTE  VPPGDisableExtTrig(MVME_INTERFACE *mvme, const DWORD base_adr);
BYTE  VPPGEnableExtTrig(MVME_INTERFACE *mvme, const DWORD base_adr);
void  VPPGStopSequencer(MVME_INTERFACE *mvme, const DWORD base_adr);
void  VPPGStartSequencer(MVME_INTERFACE *mvme, const DWORD base_adr);
BYTE  VPPGExtTrigRegRead(MVME_INTERFACE *mvme, const DWORD base_adr);

#endif
