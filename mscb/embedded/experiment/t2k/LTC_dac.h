/********************************************************************\

  Name:         LTC_dac.h
  Created by:   Brian Lee  								Jun/14/2007


  Contents:     LTC1665/2600 DAC user interface

  $Id$

\********************************************************************/
//  need to have T2KASUM defined

#ifndef  _LTC_DAC_H
#define  _LTC_DAC_H

/* Chip flags */
#define LTC1665 0
#define LTC2600 1

/* LTC1665 related commands */
	//command format: Unused, address, chipFlag, unused, data(input by user)
#define LTC1665_LOAD_A		0x0, 0x1, LTC1665, 0x00
#define LTC1665_LOAD_B		0x0, 0x2, LTC1665, 0x00
#define LTC1665_LOAD_C		0x0, 0x3, LTC1665, 0x00
#define LTC1665_LOAD_D		0x0, 0x4, LTC1665, 0x00
#define LTC1665_LOAD_E		0x0, 0x5, LTC1665, 0x00
#define LTC1665_LOAD_F		0x0, 0x6, LTC1665, 0x00
#define LTC1665_LOAD_G		0x0, 0x7, LTC1665, 0x00
#define LTC1665_LOAD_H		0x0, 0x8, LTC1665, 0x00
#define LTC1665_LOAD_ALL	0x0, 0xF, LTC1665, 0x00
#define LTC1665_SLEEP		0x0, 0xE, LTC1665, 0x00, 0x00 //doesn't need input by user

/* LTC2600 related commands */
	//command format: command, address, chipFlag, MSB data(input by user), LSB data(input by user)
	//Write to and Update command is used for loading DACs (command 0x3)
#define LTC2600_LOAD_A		0x3, 0x0, LTC2600
#define LTC2600_LOAD_B		0x3, 0x1, LTC2600
#define LTC2600_LOAD_C		0x3, 0x2, LTC2600
#define LTC2600_LOAD_D		0x3, 0x3, LTC2600
#define LTC2600_LOAD_E		0x3, 0x4, LTC2600
#define LTC2600_LOAD_F		0x3, 0x5, LTC2600
#define LTC2600_LOAD_G		0x3, 0x6, LTC2600
#define LTC2600_LOAD_H		0x3, 0x7, LTC2600
#define LTC2600_LOAD_ALL	0x3, 0xF, LTC2600
#define LTC2600_PWRDOWN_ALL 0x4, 0xF, LTC2600, 0x00, 0x00 //doesn't need input by user

void LTCdac_Init(void);
void LTCdac_Cmd(unsigned char cmd, unsigned char addr, unsigned char chipFlag, unsigned char datMSB, unsigned char datLSB);

#endif

