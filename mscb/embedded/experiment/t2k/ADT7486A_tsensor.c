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
#include    "../../mscbemb.h"
#include "t2k-asum.h"
#include "SST_handler.h"
#include "ADT7486A_tsensor.h"

/* LED declarations */

/* MSCB user_data structure */
extern struct user_data_type xdata user_data[N_HV_CHN];

/* ADT7486A temperature array addresses 
   --> 2 sensors per chip, so each address is repeated twice
   --> Order does not matter unless specified correctly */
unsigned char addrArray[] = {0x4B};

//global variables for averaging purposes
signed char k = 0;
float dataBuffer = 0.0;

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

	// Turn All Leds ON

	delay_ms(1000);
}

void ADT7486A_Cmd(unsigned char addr, unsigned char writeLength, unsigned char readLength, 
				unsigned char command, unsigned char datMSB, unsigned char datLSB, unsigned char chNum)
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
		SST_DrvByte(addr); //target address
		SST_DrvLow();
		SST_DrvByte(writeLength); //WriteLength
		SST_DrvByte(readLength); //ReadLength
		if(writeLength != 0x00)
		{
			SST_DrvByte(command); //Optional : Commands
		}
		else //writeLength == 0x00, Ping command
		{			
			ADT7486A_Read(writeFCS_Org, DONT_READ_DATA);
			break;
		}
		if(writeLength == 0x03) //If there is data to be sent (0x03 writeLength)
		{
			//Send the data in little endian format, LSB before MSB
			SST_DrvByte(datLSB);
			SST_DrvByte(datMSB);
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
				if(command == 0x00) //Get internal temperature
				{
					DISABLE_INTERRUPTS;			
					user_data[chNum].internal_temp = dataBuffer / k;
					ENABLE_INTERRUPTS;				
				}
				else if((command == 0x01) || (command == 0x02)) //Get external temperature
				{
					DISABLE_INTERRUPTS;
					user_data[chNum].external_temp = dataBuffer / k;
					ENABLE_INTERRUPTS;
				}
				else if((command == 0xe0) || (command == 0xe1)) //Get external temperature offset
				{
					DISABLE_INTERRUPTS;
					user_data[chNum].external_tempOffset = dataBuffer / k;
					ENABLE_INTERRUPTS;
				}
				//reset averaging buffer variable
				dataBuffer = 0.0;
				
				//reset the counting variable used for averaging and break the loop
				k = 0;
				break;
			}		
		}
		else if(command == 0xF6)//if reset command
		{
			ADT7486A_Read(writeFCS_Org, DONT_READ_DATA);
		}

		//Delay for the next msg
		SST_Clear();
	}
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

void User_ADT7486A(void)
/**********************************************************************************\

  Routine: User_tsensor

  Purpose: User routine for ADT7486A temperature sensors with SST protocol

  Input:
    void

  Function value:
    void

\**********************************************************************************/
{	
	//Run a SST command
	//read the current channel's temperature
	/*
	if((cur_sub_addr() % 2) == 0) //even channel numbers
	{
		ADT7486A_Cmd(addrArray[cur_sub_addr()], GetExt1Temp, cur_sub_addr()); //Get external temperature 1 command
							   			 //don't forget to specify the correct address of ADT7486A chip
		ADT7486A_Cmd(addrArray[cur_sub_addr()], GetExt1Offset, cur_sub_addr());
		ADT7486A_Cmd(addrArray[cur_sub_addr()], GetIntTemp, cur_sub_addr());
		//ADT7486A_Cmd(addrArray[n], SetExt1Offset, 0x05, 0x05, n);
		//ADT7486A_Cmd(addrArray[n], ResetDevice, n);
	}
	else if((cur_sub_addr() % 2) != 0) //odd channel numbers
	{
		ADT7486A_Cmd(addrArray[cur_sub_addr()], GetExt2Temp, cur_sub_addr()); //Get external temperature 1 command
							   			 //don't forget to specify the correct address of ADT7486A chip
		ADT7486A_Cmd(addrArray[cur_sub_addr()], GetExt2Offset, cur_sub_addr());
		ADT7486A_Cmd(addrArray[cur_sub_addr()], GetIntTemp, cur_sub_addr());
	}
	ADT7486A_LedMng();
	*/
	
	signed char chNum = 0;

	for(chNum = 0; chNum < N_HV_CHN; chNum++)
	{
		if((chNum % 2) == 0) //if the channel number is even
		{
			//then run GetExt1Temp commands
			ADT7486A_Cmd(addrArray[chNum], GetExt2Temp, chNum);
			//and the GetIntTemp command
			ADT7486A_Cmd(addrArray[chNum], GetIntTemp, chNum);
		}
		else if((chNum % 2) != 0) //if the channel number is odd
		{
			//then run GetExt2Temp commands
			ADT7486A_Cmd(addrArray[chNum], GetExt1Temp, chNum);
			//and the GetIntTemp command
			ADT7486A_Cmd(addrArray[chNum], GetIntTemp, chNum);
		}
	}
}