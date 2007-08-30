/**********************************************************************************\
  Name:         ADT7486A_tsensor.c
  Created by:   Brian Lee						     May/11/2007


  Contents:     Temperature sensor array (ADT7486A) handler

  Version:		Rev 1.2

  Last updated: Jun/08/2007
  				- Finished Commenting (Rev 1.0 Completed)
				- Got rid of exp function, replaced with different and simpler algorithm
					(Rev 1.1)
				- added averaging scheme
				- Applied more modulization (SST_ReadByte() function)

  $Id$
\**********************************************************************************/

// --------------------------------------------------------
//  Include files
// --------------------------------------------------------
#include "../../mscbemb.h"
#include "../../protocols/SST_handler.h"
#include "ADT7486A_tsensor.h"

void ADT7486A_Init(void)
/**********************************************************************************\

  Routine: init_ADT7486A

  Purpose: ADT7486A temperature sensor array initialization

  Input:
    void

  Function value:
    void

\**********************************************************************************/
{	
	SST_Init();
}

void ADT7486A_Cmd(unsigned char addr, unsigned char writeLength, unsigned char readLength, 
				unsigned char command, unsigned char datMSB, unsigned char datLSB, float *varToBeWritten)
/**********************************************************************************\

  Routine: ADT7486A_Cmd 

  Purpose: To send commands to the clients

  Input:
    unsigned char addr			Address of the client Originator wants to talk to	
	unsigned char writeLength	Length of the data Originator wants to send	
	unsigned char readLength	Length of the data Originator wants to receive
	unsigned char command		Command (see ADT7486A manual)
	unsigned char datMSB		Most Significant byte of the temperature offset value
								(for SetExt1OffSet() and SetExt2OffSet() commands)
	unsigned char datLSB		Least Significant byte of the temperature offset value
								(for SetExt1OffSet() and SetExt2OffSet() commands)
	unsigned char chNum			The channel number, which is used to define which 
								row of user_data structure the temperature data should be
								written to

  Function value:
    void

\**********************************************************************************/
{
	float tempDataBuffer = 0x00;
	signed char k = 0;
	float dataBuffer = 0.0;
	unsigned char writeFCS_Org = 0x00; //originator's side write FCS	

	//Calculate originator's side write FCS
	writeFCS_Org = FCS_Step(command, FCS_Step(readLength, FCS_Step(writeLength, FCS_Step(addr, 0x00))));	
	//if the command is setOffSet commands, then there are 2 more bytes of data
	//so add it on
	if(writeLength == 0x03)
	{
		writeFCS_Org = FCS_Step(datMSB, FCS_Step(datLSB, writeFCS_Org));
	}

		//Run averaging starting here
	while(k != AVG_COUNT)
	{
		//making sure the SST pin is set to push-pull before originating msgs
		P2MDOUT = 0x01; 

		//Start the message
		SST_DrvLow();
		SST_DrvLow();
		SST_WriteByte(addr); //target address
		SST_DrvLow();
		SST_WriteByte(writeLength); //WriteLength
		SST_WriteByte(readLength); //ReadLength
		if(writeLength != 0x00)
		{
			SST_WriteByte(command); //Optional : Commands
		}
		else //writeLength == 0x00, Ping command
		{			
			ADT7486A_Read(writeFCS_Org, DONT_READ_DATA);
			break;
		}
		if(writeLength == 0x03) //If there is data to be sent (0x03 writeLength)
		{
			//Send the data in little endian format, LSB before MSB
			SST_WriteByte(datLSB);
			SST_WriteByte(datMSB);
			ADT7486A_Read(writeFCS_Org, DONT_READ_DATA);
			break;
		}

		//In this application only temperature measurements are really required,
		//so we just need 2 bytes of reading
		if((writeLength == 0x01) && (readLength == 0x02)) 
		{
			if(k != AVG_COUNT)
			{
				tempDataBuffer = ADT7486A_Read(writeFCS_Org, READ_DATA); //Get internal temperature	
				if((tempDataBuffer != -500.0) && (tempDataBuffer != -300.0) && (tempDataBuffer != 0))
				{
					dataBuffer += tempDataBuffer;
					k++;
				}
			}
			if(k == AVG_COUNT)
			{		
				DISABLE_INTERRUPTS;
				if(dataBuffer != (508.0 * AVG_COUNT))
				{
					*varToBeWritten = dataBuffer / k;
				}
				ENABLE_INTERRUPTS;				
			}		
		}
		else if(command == 0xF6)//if reset command
		{
			ADT7486A_Read(writeFCS_Org, DONT_READ_DATA);
		}

		//Clear for the next msg and delay for conversion to finish
		SST_Clear();
		delay_ms(ADT7486A_CONVERSION_TIME);
	}

	//reset averaging buffer variable
	dataBuffer = 0.0;
	
	//reset the counting variable used for averaging and break the loop
	k = 0;
}

float ADT7486A_Read(unsigned char writeFCS_Originator, unsigned char cmdFlag)
/**********************************************************************************\

  Routine: SST_Read

  Purpose: Read only needs to be working for reading 2 bytes of info
		   Which are Internal Temp, External Temp1, and External Temp2
		   Others should be verified on oscilloscope since mscb returns a data at maximum
		   4bytes, (allTemp and GetDIB commands take 6 bytes and 16 bytes, which is impossible anyway)

  Input:
    unsigned char writeFCS_Originator	Originator's writeFCS 
	unsigned char cmdFlag				a Flag used to distinguish
										if the command is a Ping/Reset
										or not

  Function value:
    float								value of Data given by 
										the Client in response to
										Originator's commands
										(converted Temperature value)

\**********************************************************************************/
{	
	//declare local variables
	unsigned char LSB_Received = 0x00; //Max 2bytes of info from 3 different possible commands
	unsigned char MSB_Received = 0x00; //for reading, GetIntTemp, GetExtTemp1, GetExtTemp2
	unsigned char writeFCS_Client = 0x00;
	unsigned char readFCS_Client = 0x00; //used for both writeFCS and readFCS
	float convertedTemp = 0.0;

	writeFCS_Client = SST_ReadByte(); //get Write FFCS from Client
	
	//writeFCS check
	if(writeFCS_Originator != writeFCS_Client)
	{
		return ((float) -300.0); //if the FCS's don't match up, then return -300
	}

	if(cmdFlag == 0x00) //If it's not the Ping Command or SetOffSet command
	{
		//Take the data in to a variable called datareceived
		LSB_Received = SST_ReadByte(); //get LSB
		MSB_Received = SST_ReadByte(); //get MSB
		readFCS_Client = SST_ReadByte(); //get Read FCS from Client

		//Here, reading data is finished, now analyze the received data

		//Check FCS
		//returns -300 because absolute low temperature is -273 degree Celsius, so this value
		//suitable for error reporting value
		//test = FCS(cmdData, 6); //then check for 5 bytes	
		if(FCS_Step(MSB_Received, FCS_Step(LSB_Received, 0x00)) != readFCS_Client) //readFCS check
		{
			return ((float) -300.0); //if the FCS's don't match up, then return -300
		}		
	
		//Conver the Temperature according to the structure of bits defined by
		//ADT7486A manual
		//Convert 2bytes of temperature from 
		//GetExt1OffSet/GetExt2OffSet/GetIntTemp()/GetExt1Temp()/GetExt2Temp() commands
		//Mask out the first bit (sign bit)
		//and if the sign bit is 1, then do two's complement
		if((MSB_Received & 0x80) == 0x80) 
		{
			MSB_Received = ~MSB_Received;
			LSB_Received = ~LSB_Received + 0x01;
		}
		
		//Calculate the converted Temperature
		convertedTemp = ((float) (MSB_Received * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 0.015625)) + ((float) (LSB_Received * 0.015625));
	}
	else //If it is the Ping Command or SetOffSet command
	{
		return -500;
	}

	//return the converted temperature value
	return convertedTemp;
}