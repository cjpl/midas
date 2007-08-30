/********************************************************************\

  Name:         AD5301_dac.c
  Created by:   Brian Lee  								Jun/07/2007


  Contents:     AD5301 DAC user interface

  $Id$

\********************************************************************/
//  need to have T2KASUM defined


#include "../../mscbemb.h"
#include "../../protocols/SMBus_handler.h"
#include "AD5301_dac.h"

void AD5301_Init(void)
{
	SMBus_Init(); // SMBus initialization (should be called after pca_operation)
}

void AD5301_Cmd(unsigned char addr, unsigned char datMSB, unsigned char datLSB, bit flag)
{
	unsigned char readValue = 0;
	watchdog_refresh(0);
	if(flag == AD5301_READ) //reading
	{
		SMBus_Start(); //make start bit
		SMBus_WriteByte(addr, READ); //send address with a Write flag on 
	    if (!ACK) return; // No slave ACK. Address is wrong or slave is missing.
	    readValue = SMBus_ReadByte();  // set data word
		//store to mscb user_data here
	    SMBus_Stop();
	}
	else //writing
	{		
		SMBus_Start();
		SMBus_WriteByte(addr, WRITE);
		if(!ACK) return;
		SMBus_WriteByte(datMSB, VAL);
		if(!ACK) return;
		SMBus_WriteByte(datLSB, VAL);
		if(!ACK) return;
		SMBus_Stop();
	}
}