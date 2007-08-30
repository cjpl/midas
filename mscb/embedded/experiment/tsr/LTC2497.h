/**********************************************************************************\
  Name:         LTC2497.h
  Created by:   Brian Lee						     Jul/11/2007, recovered on Aug/24/2007

  Contents:     LTC2497 ADC embedded application header for MSCB

  Version:		 Rev 1.0

  Requirement:  Need to have I2C_hander.c and I2C_handler.h compiled together

  Last updated: Aug/24/2007
  				- Recovery

  $Id: $
\**********************************************************************************/

#ifndef _LTC2497_H
#define _LTC2497_H

#define LTC2497_ADR 0x14
#define LTC2497_FS 1.25 //Full scale is half of Vref, so (2.5 / 2)
#define LTC2497_CONVERSIONTIME 160 //in ms

//LTC2497 parameters definition
//CMDs: cmd, addr, control
#define READ_S1 1, LTC2497_ADR, 0xB8
#define READ_S2 2, LTC2497_ADR, 0xB9
#define READ_S3 3, LTC2497_ADR, 0xBA
#define READ_S4 4, LTC2497_ADR, 0xBB
#define READ_S5 5, LTC2497_ADR, 0xB0
#define READ_S6 6, LTC2497_ADR, 0xB1
#define READ_S7 7, LTC2497_ADR, 0xB2
#define READ_S8 8, LTC2497_ADR, 0xB3
#define READ_DIFF1 9, LTC2497_ADR, 0xA4
#define READ_DIFF2 10, LTC2497_ADR, 0xA5
#define READ_DIFF3 11, LTC2497_ADR, 0xA6
#define READ_DIFF4 12, LTC2497_ADR, 0xA7

//LTC2497 function declarations
void LTC2497_Cmd(char cmd, unsigned char addr, unsigned char control, float *varToBeWritten);

#endif