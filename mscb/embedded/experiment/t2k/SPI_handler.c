/**********************************************************************************\
  Name:         SPI_handler.c
  Created by:   Brian Lee						     Jun/14/2007


  Contents:     SPI protocol for T2K-FGD experiment

  Version:		Rev 1.0

  Last updated: 

  $id$
\**********************************************************************************/

#include "../../mscbemb.h"
#include "SPI_handler.h"

/* SPI Protocl Related Pins */
sbit SPI_SCK = P0 ^ 7;    // SPI Protocol Serial Clock
sbit SPI_MISO = P1 ^ 2;   // SPI Protocol Master In Slave Out
sbit SPI_MOSI  = P1 ^ 3;  // SPI Protocol Master Out Slave In

void SPI_Init(void)
{
	SPI_MOSI = 1; //pull the MOSI line high
}

void SPI_ClockOnce(void)
{
	delay_us(SPI_DELAY);
	SPI_SCK = 0;
	delay_us(SPI_DELAY);
	SPI_SCK = 1;
}

void SPI_WriteByte(unsigned char dataToSend)
{
	signed char i;
	
	for(i = 7; i >= 0; i--)
	{
	   SPI_MOSI = (dataToSend >> i) & 0x01;
	   SPI_ClockOnce();
	}
}

unsigned char SPI_ReadByte(void)
{
	signed char i = 0;
	unsigned char din = 0;
	unsigned char dataReceived = 0; 

	for(i = 7; i >= 0; i--) 
	{
		SPI_ClockOnce();
		din = SPI_MISO;
		dataReceived |= (din << i);		
	}

	return dataReceived;
}

