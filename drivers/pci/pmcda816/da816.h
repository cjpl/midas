/*********************************************************************
  Name:         da816.h
  Created by:   Konstantin Olchanski / Pierre-Andre Amaudruz
  Contributors: 

  Contents:                     
  $Id$
*********************************************************************/
#ifndef  ALPHIDA816_INCLUDE_H
#define  ALPHIDA816_INCLUDE_H

#define DA816_OFFSET           0x0000

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  double alpha;
  double beta;
  char * data;
  char * regs;
} ALPHIDA816;

typedef char           uint08;
typedef unsigned short uint16;
typedef unsigned int   uint32;

#define D08(ptr) (*(volatile uint08*)(ptr))
#define D16(ptr) (*(volatile uint16*)(ptr))
#define D32(ptr) (*(volatile uint32*)(ptr))

#define CLK_SAMP_INT        (DA816_OFFSET+0x00)  // I4
#define ADDRESS_SM          (DA816_OFFSET+0x04)  // I4
#define LAST_ADDR_0         (DA816_OFFSET+0x08)  // I4
#define LAST_ADDR_1         (DA816_OFFSET+0x0C)  // I4
#define BANK_CTL_0          (DA816_OFFSET+0x10)  // I1
#define BANK_CTL_1          (DA816_OFFSET+0x11)  // I1
#define CTL_STAT_0          (DA816_OFFSET+0x12)  // I1
#define CTL_STAT_1          (DA816_OFFSET+0x13)  // I1
#define CLK_SAMPLE_RESET    (DA816_OFFSET+0x14)  // I2
#define ADDRESS_RESET       (DA816_OFFSET+0x16)  // I2
#define DACS_RESET          (DA816_OFFSET+0x18)  // I2
#define DACS_UPDATES        (DA816_OFFSET+0x1A)  // I2
#define SWITCH_BANKS        (DA816_OFFSET+0x1C)  // I2
#define DAC0102             (DA816_OFFSET+0x20)  // I4
#define DAC0304             (DA816_OFFSET+0x24)  // I4
#define DAC0506             (DA816_OFFSET+0x28)  // I4
#define DAC0708             (DA816_OFFSET+0x2C)  // I4
#define DAC0910             (DA816_OFFSET+0x30)  // I4
#define DAC1112             (DA816_OFFSET+0x34)  // I4
#define DAC1314             (DA816_OFFSET+0x38)  // I4
#define DAC1516             (DA816_OFFSET+0x3C)  // I4


// CLK_SAMP_INT  32MHz (32k/2+N = sampling rate) 
#define CLK_100KHZ           318
#define CLK_50KHZ            638
#define CLK_10KHZ           3198

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
  
int  da816_Setup(ALPHIDA816 *, int samples, int mode);
int  da816_Open(ALPHIDA816 **);
void da816_Close(ALPHIDA816 *);
int  da816_Reset(ALPHIDA816 *);
int  da816_AddReset(ALPHIDA816 *);
int  da816_Update(ALPHIDA816 *);
  //int  da816_Status(ALPHIDA816 *);
int  da816_Status(ALPHIDA816 *, int mode );
int  da816_BankSwitch(ALPHIDA816 *, int onoff);
int  da816_BankActiveRead(ALPHIDA816 *);
int  da816_ScaleSet(ALPHIDA816 *, double alpha, double beta);
int  da816_MaxSamplesGet();

int  da816_DirectDacWrite(ALPHIDA816 *, uint16 din, int chan);
int  da816_DacWrite(ALPHIDA816 *, unsigned short *data, int * chlist, int nch);
int  da816_DirectVoltWrite(ALPHIDA816 *, double vin, int chan, double *arg);

int  da816_LinLoad(ALPHIDA816 *, double vin, double vout, int npts, int * offset, int ibuf, int ichan);
int  da816_SampleSet(ALPHIDA816 *, int bank, int sample);
int  da816_ClkSMEnable(ALPHIDA816 *, int bank);
int  da816_ClkSMDisable(ALPHIDA816 *, int bank);
uint16_t da816_Volt2Dac(double volt);
double da816_Dac2Volt (int16_t dac);


#ifdef __cplusplus
}
#endif

#endif // ALPHIDA816_INCLUDE_H

/* emacs
 * Local Variables:
 * mode:C
 * mode:font-lock
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */

