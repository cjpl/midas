/********************************************************************\

  Name:         LTC_dac.c
  Created by:   Brian Lee  								Jun/14/2007


  Contents:     LTC1665/2600 DAC user interface

  $Id$

\********************************************************************/
//  need to have T2KASUM defined

#include "../../mscbemb.h"
#include "SPI_handler.h"
#include "LTC_dac.h"

/* LTC1665/2600 pins (8-bit DACs)*/
sbit SUM_DAC_CSn = P1 ^ 5;
sbit BIAS_DAC_CSn = P1 ^ 4;
sbit DAC_IN = P1 ^ 3;
sbit DAC_SCK = P0 ^ 7;    // DAC Serial Clock

void LTCdac_Init(void)
{
   SUM_DAC_CSn  = 1; // unselect LTC2600
   BIAS_DAC_CSn  = 1; // unselect LTC1665
   
   LTCdac_Cmd(LTC1665_LOAD_ALL, 200);   
}

void LTCdac_Cmd(unsigned char cmd, unsigned char addr, unsigned char chipFlag, unsigned char datMSB,
				unsigned char datLSB)
{
	if(chipFlag == LTC1665)
	{
		DAC_SCK = 0; //SCK must be low before Chip Select is pulled low
		BIAS_DAC_CSn = 0; //select LTC1665
		SPI_WriteByte((addr << 4) | (datLSB >> 4)); //Send address and upper 4 bits of Input Code
		SPI_WriteByte((datLSB << 4) | (0x00)); //Send the lower 4 bits of the data with Don't Cares
		BIAS_DAC_CSn = 1; //unselect LTC1665 after communication is done
	}
	else if(chipFlag == LTC2600)
	{
		DAC_SCK = 0; //SCK must be low before Chip Select is pulled low
		SUM_DAC_CSn = 0; //select LTC2600
		SPI_WriteByte((cmd << 4) | addr); //Send the command and address
		SPI_WriteByte(datMSB); //Send the upper byte of Input Code
		SPI_WriteByte(datLSB); //Send the lower byte of Input Code
		SUM_DAC_CSn = 1; //unselect LTC2600 after communication is done
	}
}