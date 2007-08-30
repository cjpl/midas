/********************************************************************\

  Name:         PCA9539_io.c
  Created by:   Brian Lee  								Jun/07/2007


  Contents:     PCA9539 I/O register user interface
  				This chip is used to enable different switches for
				the Charge Pump

  $Id$

\********************************************************************/
//  need to have T2KASUM defined


#include "../../mscbemb.h"
#include "../../protocols/SMBus_handler.h"
#include "PCA9539_io.h"

void PCA9539_Init(void)
{
	PCA9539_Cmd(OUTPUT_ENABLE); //put all pins to Output modes
	PCA9539_Cmd(ALL_LOW); //All Biases enabled
}

void PCA9539_Cmd(unsigned char addr, unsigned char cmd, unsigned char datByte, bit flag)
{
	unsigned char readValue = 0;
	watchdog_refresh(0);
	if(flag == PCA9539_READ) //reading
	{
		SMBus_Start(); //make start bit
		SMBus_WriteByte(addr, WRITE);
		if (!ACK) return; // No slave ACK. Address is wrong or slave is missing.
		SMBus_WriteByte(cmd, CMD);
		if (!ACK) return;
		SMBus_Start(); //make repeated start bit
		SMBus_WriteByte(addr, READ); //send address with a Write flag on 
	    if (!ACK) return; // No slave ACK. Address is wrong or slave is missing.
	    readValue = SMBus_ReadByte();  // receive the register's data
		//store to mscb user_data here
	    SMBus_Stop();
	}
	else //writing
	{		
		SMBus_Start();
		SMBus_WriteByte(addr, WRITE);
		if(!ACK) return;
		SMBus_WriteByte(cmd, CMD);
		if(!ACK) return;
		SMBus_WriteByte(datByte, VAL);
		if(!ACK) return;
		SMBus_Stop();
	}
}
