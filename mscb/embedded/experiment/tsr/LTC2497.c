/**********************************************************************************\
  Name:         LTC2497.c
  Created by:   Brian Lee						     Jul/11/2007, recovered on Aug/24/2007

  Contents:     LTC2497 ADC embedded application for MSCB

  Version:		 Rev 1.0

  Requirement:  Need to have I2C_hander.c and I2C_handler.h compiled together

  Last updated: Aug/24/2007
  				- Recovery

  $id: $
\**********************************************************************************/

#include "mscbemb.h"
#include "LTC2497.h"
#include "I2C_handler.h"

void LTC2497_Cmd(char cmd, unsigned char addr, unsigned char control, float * varToBeWritten)
{
	unsigned long int buffer = 0x0000; //temporarily storing 24-bit data to check signness

	//Write cycle, send addr and command
	I2C_Clear();
	I2C_Start();
	if(I2C_WriteByte(addr, I2C_WRITE_FLAG) != I2C_ACK) return;
	if(I2C_WriteByte(control, I2C_CMD_FLAG) != I2C_ACK) return;
	if(I2C_WriteByte(0x00, I2C_DATA_FLAG) != I2C_ACK) return; //dummy 8bit values, as specified in datasheet
	I2C_Stop();

	//Read cycle, receive the 24-bit ADC data
	//ignore the last 6 digits (bit0~5) which are always zero
	delay_ms(LTC2497_CONVERSIONTIME); //delay for conversion time
	I2C_Clear();	
	I2C_Start();
	if(I2C_WriteByte(addr, I2C_READ_FLAG) != I2C_ACK) return;	
	buffer = (unsigned long int) I2C_ReadByte(I2C_NACK) << 10;
	buffer |= (unsigned long int) I2C_ReadByte(I2C_NACK) << 2;
	buffer |= (unsigned long int) I2C_ReadByte(I2C_NACK) >> 6;

	if(buffer & 0x00020000) //if the read number is positive
	{		
		buffer &= 0x0001FFFF;
	}
	else //if the read number is negative 
	{
		buffer = (~buffer + 0x00000001) & 0x0003FFFF; //2's complement
	}

	//General ADC conversion for LTC2497
	*varToBeWritten = (((float)buffer) * (LTC2497_FS / 65535.0)); //take it as is

	//Set appropriate conversions for each measuremets
	if((cmd >= 0) && (cmd <= 4))
	{	
		//Temperature measurements from MAX66081UK
		//The devices are not yet on the board, so conversion is not written yet		
	}

	else if((cmd >= 5) && (cmd <= 8))
	{
		//Temperature measurements from LM94021BIMG
		*varToBeWritten = (((*varToBeWritten * 1000.0) - 980.0) / (-5.5357)) + 10.0; //for GS = 00
	}
	
	else if((cmd >= 9) && (cmd <= 12))
	{
		//Voltage measurements for DIFF1 ~ DIFF4
		*varToBeWritten = *varToBeWritten; //no conversion required;
	}
}
