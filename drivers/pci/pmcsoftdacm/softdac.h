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

#include <stdint.h>

// char *SOFTDAC_RANGE[] = {"P5V", "P10V", "PM5V", "PM10V", "PM2P5V", "M2P5P7P5V", "off"};
 
typedef struct { 
  double alpha; 
  double beta; 
  int   range; 
  char * data; 
  char * regs; 
} ALPHISOFTDAC; 
 
#define SOFTDAC_D08(ptr) (*(volatile uint8_t*)(ptr)) 
#define SOFTDAC_D16(ptr) (*(volatile uint16_t*)(ptr)) 
#define SOFTDAC_D32(ptr) (*(volatile uint32_t*)(ptr)) 
 
#define SOFTDAC_CLK_SAMP_INT        (SOFTDAC_OFFSET+0x00)  // I4 
#define SOFTDAC_ADDRESS_SM          (SOFTDAC_OFFSET+0x04)  // I4 
#define SOFTDAC_LAST_ADDR_0         (SOFTDAC_OFFSET+0x08)  // I4 
#define SOFTDAC_LAST_ADDR_1         (SOFTDAC_OFFSET+0x0C)  // I4 
#define SOFTDAC_BANK_CTL_0          (SOFTDAC_OFFSET+0x10)  // I1 
#define SOFTDAC_BANK_CTL_1          (SOFTDAC_OFFSET+0x11)  // I1 
#define SOFTDAC_CTL_STAT_0          (SOFTDAC_OFFSET+0x12)  // I1 SM and output business 
#define SOFTDAC_CTL_STAT_1          (SOFTDAC_OFFSET+0x13)  // I1 Interrupt business 
#define SOFTDAC_CLK_SAMPLE_RESET    (SOFTDAC_OFFSET+0x14)  // I2 
#define SOFTDAC_ADDRESS_RESET       (SOFTDAC_OFFSET+0x16)  // I2 
#define SOFTDAC_DACS_RESET          (SOFTDAC_OFFSET+0x18)  // I2 
#define SOFTDAC_DACS_UPDATES        (SOFTDAC_OFFSET+0x1A)  // I2 
#define SOFTDAC_SWITCH_BANKS        (SOFTDAC_OFFSET+0x1C)  // I2 
#define SOFTDAC_DAC0102             (SOFTDAC_OFFSET+0x20)  // I4 
#define SOFTDAC_DAC0304             (SOFTDAC_OFFSET+0x24)  // I4 
#define SOFTDAC_DAC0506             (SOFTDAC_OFFSET+0x28)  // I4 
#define SOFTDAC_DAC0708             (SOFTDAC_OFFSET+0x2C)  // I4 
#define SOFTDAC_DAC0910             (SOFTDAC_OFFSET+0x30)  // I4 
#define SOFTDAC_DAC1112             (SOFTDAC_OFFSET+0x34)  // I4 
#define SOFTDAC_DAC1314             (SOFTDAC_OFFSET+0x38)  // I4 
#define SOFTDAC_DAC1516             (SOFTDAC_OFFSET+0x3C)  // I4 
 
// CLK_SAMP_INT  36MHz (36k/2+N = sampling rate)  
#define SOFTDAC_CLK_500KHZ            70 
#define SOFTDAC_CLK_100KHZ            358 
#define SOFTDAC_CMD_REG             (SOFTDAC_OFFSET+0x48)  // I4 
#define SOFTDAC_IMMEDIATE_MODE        0x2 
#define SOFTDAC_RANGE_MAX               6
#define SOFTDAC_RANGE_P5V             0x8 
#define SOFTDAC_RANGE_P10V            0x9 
#define SOFTDAC_RANGE_PM5V            0xA 
#define SOFTDAC_RANGE_PM10V           0xB 
#define SOFTDAC_RANGE_PM2P5V          0xC 
#define SOFTDAC_RANGE_M2P5P7P5V       0xD 
#define SOFTDAC_SET_COEF              0x0 
 
#define SOFTDAC_SWITCH_AT_ADDR_0     0x1 
#define SOFTDAC_INT_BANK_DONE        0x4 
 
// BANK_CTL_X 
#define SOFTDAC_ACTIVE_BANK_RO       0x01 
#define SOFTDAC_CLK_OUTPUT_ENABLE    0x02 
#define SOFTDAC_INTERNAL_CLK         0x04 
#define SOFTDAC_EXTERNAL_CLK         0x08 
#define SOFTDAC_SM_ENABLE            0x20 
#define SOFTDAC_FP_RESET_ENABLE      0x40 
#define SOFTDAC_AUTO_UPDATE_DAC      0x80 
 
// CTL_STAT_X 
#define SOFTDAC_FP_RESET_STATE_RO    0x01   
#define SOFTDAC_INT_FP_RESET_ENABLE  0x02 
#define SOFTDAC_INT_CLK_ENABLE       0x04 
#define SOFTDAC_INT_BANK_0_DONE      0x10 
#define SOFTDAC_INT_BANK_1_DONE      0x20 
#define SOFTDAC_INT_CLK_DONE         0x40 
#define SOFTDAC_INT_FP_DONE          0x80 
 
#define SOFTDAC_REGS_D32             0x10 
#define SOFTDAC_ACTIVE_BANK_D32      0x10000 
   
int  softdac_Setup(ALPHISOFTDAC *, int samples, int mode); 
int  softdac_DacVoltRead(ALPHISOFTDAC *, int npts, int offset, int ibuf, int ichan, int flag); 
int  softdac_Open(ALPHISOFTDAC **); 
void softdac_Close(ALPHISOFTDAC *); 
int  softdac_Status(ALPHISOFTDAC *, int); 
int  softdac_Reset(ALPHISOFTDAC *); 
int  softdac_BankSwitch(ALPHISOFTDAC *, int onoff); 
int  softdac_BankActiveRead(ALPHISOFTDAC *); 
void softdac_SampleSet(ALPHISOFTDAC *, int, int); 
void softdac_SMEnable(ALPHISOFTDAC *); 
void softdac_SMDisable(ALPHISOFTDAC *); 
int  softdac_ScaleSet(ALPHISOFTDAC *, int range, double alpha, double beta); 
int  softdac_LinLoad(ALPHISOFTDAC *, double vin, double vout, int npts, int * offset, int ibuf, int ichan); 
int  softdac_DacVoltRead(ALPHISOFTDAC * al, int npts, int offset, int ibuf, int ichan, int flag); 
int  softdac_DirectDacWrite(ALPHISOFTDAC * al, uint16_t din, int chan, double *arg); 
int  softdac_DacWrite(ALPHISOFTDAC * al, uint16_t din, int chan, double *arg); 
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
 
