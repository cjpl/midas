/**********************************************************************************\
  Name:         I2C_handler.c
  Created by:   Brian Lee						     Jul/11/2007, recovered on Aug/24/2007

  Contents:     I2C Protocol for any MSCB application (need I2C_hander.h)

  Version:		 Rev 1.1

  Requirement:  Need to have I2C_SDA and I2C_SCL defined in mscbemb.h

  Last updated: Aug/24/2007
  				- Recovery

  $Id: $
\**********************************************************************************/

//Include files
#include "../mscbemb.h"
#include "I2C_handler.h"

sbit I2C_SDA = MSCB_I2C_SDA;
sbit I2C_SCL = MSCB_I2C_SCL;

void I2C_Clear(void)
{
	//Bus freeTime between Stop and Start condition
	I2C_SDA = 1;
	I2C_SCL = 1;
	delay_us(I2C_T_BUF); 
}

void I2C_Start(void)
{
	//Bus freeTime between Stop and Start condition (Clearing function)
	I2C_Clear();

	//Start condition: I2C_SDA goes from HIGH to LOW while I2C_SCL is HIGH
	delay_us(I2C_SU_STA);
	I2C_SDA = 0;
	delay_us(I2C_HD_STA);
	I2C_SCL = 0;	
}

void I2C_Stop(void)
{
	//Stop condition: I2C_SDA goes from LOW to HIGH while I2C_SCL is HIGH
	I2C_SCL = 1;
	delay_us(I2C_SU_STO);
	I2C_SDA = 1;
}

unsigned char I2C_WriteByte(unsigned char datByte, unsigned char flag)
{
	char i = 0;
	unsigned char toBeSent = 0;

	if(flag == I2C_WRITE_FLAG)
	{
		toBeSent = (datByte << 1) & 0xFE;
	}
	else if(flag == I2C_READ_FLAG)
	{
		toBeSent = (datByte << 1) | 0x01;
	}
	else
	{
		toBeSent = datByte;
	}
	for(i = 7; i >= 0; i--)
	{
		I2C_SDA = (toBeSent >> i) & 0x01;
		delay_us(1); //SU;DAT
		I2C_ClockOnce();
	}

	if(I2C_SDA == 0)
	{
		I2C_ClockOnce();
		return I2C_ACK;
	}
	else if(I2C_SDA == 1)
	{
		I2C_ClockOnce();
		return I2C_NACK;
	}
	else
	{
		I2C_ClockOnce();
		return I2C_WRITE_ERROR;
	}
}

unsigned char I2C_ReadByte(bit flag)
{
	char i = 0;
	char dataRead = 0x00;

	for(i = 7; i >= 0; i--)
	{
		dataRead |= ((unsigned char) I2C_SDA) << i;
		I2C_ClockOnce();
	}

	//send ACK / NACK
	if(flag == ACK)
	{
		I2C_SDA = 0;
	}
	else if(flag == I2C_NACK)
	{
		I2C_SDA = 1;
	}
	I2C_ClockOnce();

	return dataRead;
}

void I2C_ClockOnce(void)
{
	I2C_SCL = 1;
	delay_us(I2C_T_HIGH);
	I2C_SCL = 0;
	delay_us(I2C_T_LOW);	
}