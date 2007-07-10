/*********************************************************************
  Name:         softdac.h
  Created by:   Konstantin Olchanski / Pierre-Andre Amaudruz
  Contributors: 

  Contents:                     
  $Id$
*********************************************************************/
#ifndef  ALPHISOFTDAC_INCLUDE_H
#define  ALPHISOFTDAC_INCLUDE_H

#define SOFTDAC_OFFSET           0x80000

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  double alpha;
  double beta;
  char * data;
  char * regs;
} ALPHISOFTDAC;

typedef char           uint08;
typedef unsigned short uint16;
typedef unsigned int   uint32;

#define D08(ptr) (*(volatile uint08*)(ptr))
#define D16(ptr) (*(volatile uint16*)(ptr))
#define D32(ptr) (*(volatile uint32*)(ptr))

#define CLK_SAMP_INT        (SOFTDAC_OFFSET+0x00)  // I4
#define ADDRESS_SM          (SOFTDAC_OFFSET+0x04)  // I4
#define LAST_ADDR_0         (SOFTDAC_OFFSET+0x08)  // I4
#define LAST_ADDR_1         (SOFTDAC_OFFSET+0x0C)  // I4
#define BANK_CTL_0          (SOFTDAC_OFFSET+0x10)  // I1
#define BANK_CTL_1          (SOFTDAC_OFFSET+0x11)  // I1
#define CTL_STAT_0          (SOFTDAC_OFFSET+0x12)  // I1
#define CTL_STAT_1          (SOFTDAC_OFFSET+0x13)  // I1
#define CLK_SAMPLE_RESET    (SOFTDAC_OFFSET+0x14)  // I2
#define ADDRESS_RESET       (SOFTDAC_OFFSET+0x16)  // I2
#define DACS_RESET          (SOFTDAC_OFFSET+0x18)  // I2
#define DACS_UPDATES        (SOFTDAC_OFFSET+0x1A)  // I2
#define SWITCH_BANKS        (SOFTDAC_OFFSET+0x1C)  // I2
#define DAC0102             (SOFTDAC_OFFSET+0x20)  // I4
#define DAC0304             (SOFTDAC_OFFSET+0x24)  // I4
#define DAC0506             (SOFTDAC_OFFSET+0x28)  // I4
#define DAC0708             (SOFTDAC_OFFSET+0x2C)  // I4
#define DAC0910             (SOFTDAC_OFFSET+0x30)  // I4
#define DAC1112             (SOFTDAC_OFFSET+0x34)  // I4
#define DAC1314             (SOFTDAC_OFFSET+0x38)  // I4
#define DAC1516             (SOFTDAC_OFFSET+0x3C)  // I4

// CLK_SAMP_INT  36MHz (36k/2+N = sampling rate) 
#define CLK_500KHZ            70
#define CLK_100KHZ            358
#define CMD_REG             (SOFTDAC_OFFSET+0x48)  // I4
#define IMMEDIATE_MODE        0x2
#define RANGE_P5V             0x8
#define RANGE_P10V            0x9
#define RANGE_PM5V            0xA
#define RANGE_PM10V           0xB
#define RANGE_PM2P5V          0xC
#define RANGE_M2P5P7P5V       0xD
#define SET_COEF              0x0

#define SWITCH_AT_ADDR_0     0x1
#define INT_BANK_DONE        0x4

// BANK_CTL_X
#define ACTIVE_BANK_RO       0x01
#define CLK_OUTPUT_ENABLE    0x02
#define INTERNAL_CLK         0x04
#define EXTERNAL_CLK         0x08
#define SM_ENABLE            0x20
#define FP_RESET_ENABLE      0x40
#define AUTO_UPDATE_DAC      0x80

// CTL_STAT_X
#define FP_RESET_STATE_RO    0x01  
#define INT_FP_RESET_ENABLE  0x02
#define INT_CLK_ENABLE       0x04
#define INT_BANK_0_DONE      0x10
#define INT_BANK_1_DONE      0x20
#define INT_CLK_DONE         0x40
#define INT_FP_DONE          0x80

#define REGS_D32             0x10
#define ACTIVE_BANK_D32      0x10000
  
int  softdac_Setup(ALPHISOFTDAC *, int samples, int mode);
int  softdac_DacVoltRead(ALPHISOFTDAC *, int npts, int offset, int ibuf, int ichan, int flag);
int  softdac_Open(ALPHISOFTDAC **);
void softdac_Close(ALPHISOFTDAC *);
int  softdac_Status(ALPHISOFTDAC *, int);
int  softdac_Reset(ALPHISOFTDAC *);
int  softdac_BankSwitch(ALPHISOFTDAC *, int onoff);
int  softdac_BankActiveRead(ALPHISOFTDAC *);
int  softdac_SampleSet(ALPHISOFTDAC *, int bank, int sample);
int  softdac_ClkSMEnable(ALPHISOFTDAC *, int bank);
int  softdac_ClkSMDisable(ALPHISOFTDAC *, int bank);
int  softdac_ScaleSet(ALPHISOFTDAC *, int range, double alpha, double beta);
int  softdac_LinLoad(ALPHISOFTDAC *, double vin, double vout, int npts, int * offset, int ibuf, int ichan);
int  softdac_DacVoltRead(ALPHISOFTDAC * al, int npts, int offset, int ibuf, int ichan, int flag);
int  softdac_DirectDacWrite(ALPHISOFTDAC * al, uint16 din, int chan, double *arg);
int  softdac_DacWrite(ALPHISOFTDAC * al, uint16 din, int chan, double *arg);
int  softdac_DirectVoltWrite(ALPHISOFTDAC * al, double vin, int chan, double *arg);

#ifdef __cplusplus
}
#endif

#endif // ALPHISOFTDAC_INCLUDE_H

/* emacs
 * Local Variables:
 * mode:C
 * mode:font-lock
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */

