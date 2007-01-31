/*********************************************************************
  Name:         alphidac.h
  Created by:   Konstantin Olchanski / Pierre-Andre Amaudruz
  Contributors: 

  Contents:                     
  $Log: alphidac.h,v $
*********************************************************************/
#ifndef  ALPHIPMC_INCLUDE_H
#define  ALPHIPMC_INCLUDE_H

#ifdef ALPHI_SOFTDAC
#define OFFSET           0x80000
#endif
#ifdef ALPHI_DA816
#define OFFSET           0x0000
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define CLK_SAMP_INT        (OFFSET+0x00)  // I4
#define ADDRESS_SM          (OFFSET+0x04)  // I4
#define LAST_ADDR_0         (OFFSET+0x08)  // I4
#define LAST_ADDR_1         (OFFSET+0x0C)  // I4
#define BANK_CTL_0          (OFFSET+0x10)  // I1
#define BANK_CTL_1          (OFFSET+0x11)  // I1
#define CTL_STAT_0          (OFFSET+0x12)  // I1
#define CTL_STAT_1          (OFFSET+0x13)  // I1
#define CLK_SAMPLE_RESET    (OFFSET+0x14)  // I2
#define ADDRESS_RESET       (OFFSET+0x16)  // I2
#define DACS_RESET          (OFFSET+0x18)  // I2
#define DACS_UPDATES        (OFFSET+0x1A)  // I2
#define SWITCH_BANKS        (OFFSET+0x1C)  // I2
#define DAC0102             (OFFSET+0x20)  // I4
#define DAC0304             (OFFSET+0x24)  // I4
#define DAC0506             (OFFSET+0x28)  // I4
#define DAC0708             (OFFSET+0x2C)  // I4
#define DAC0910             (OFFSET+0x30)  // I4
#define DAC1112             (OFFSET+0x34)  // I4
#define DAC1314             (OFFSET+0x38)  // I4
#define DAC1516             (OFFSET+0x3C)  // I4

#ifdef ALPHI_SOFTDAC
// CLK_SAMP_INT  36MHz (36k/2+N = sampling rate) 
#define CLK_500KHZ            70

#endif
#ifdef ALPHI_DA816
// CLK_SAMP_INT  32MHz (32k/2+N = sampling rate) 
#define CLK_100KHZ           318
#define CLK_50KHZ            638
#define CLK_10KHZ           3198
#endif
#ifdef ALPHI_SOFTDAC
// CLK_SAMP_INT  36MHz (36k/2+N = sampling rate) 
#define CLK_500KHZ            70
#endif

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

#ifdef __cplusplus
}
#endif

#endif // ALPHIPMC_INCLUDE_H

/* emacs
 * Local Variables:
 * mode:C
 * mode:font-lock
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */

