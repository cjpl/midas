/**********************************************************************************\
  Name:         ADT7486A_tsensor.c
  Created by:   Brian Lee						     May/11/2007


  Contents:     Temperature sensor array (ADT7486A) handler

  Version:		Rev 1.1

  Last updated: May/31/2007
  				- Finished Commenting (Rev 1.0 Completed)
				- Got rid of exp function, replaced with different and simpler algorithm
					(Rev 1.1)
				- added averaging scheme

  $Id$
\**********************************************************************************/

// --------------------------------------------------------
//  Include files
// --------------------------------------------------------
#include    "../../mscbemb.h"
#include "tsr.h"
#include "SST_handler.h"
#include "ADT7486A_tsensor.h"

/* LED declarations */
sbit led_1 = TSRLED_1;
sbit led_2 = TSRLED_2;
sbit led_3 = TSRLED_3;
sbit led_4 = TSRLED_4;
sbit led_5 = TSRLED_5;
sbit led_6 = TSRLED_6;
sbit led_7 = TSRLED_7;

/* MSCB user_data structure */
extern struct user_data_type xdata user_data[N_HV_CHN];

/* ADT7486A temperature array addresses 
   --> 2 sensors per chip, so each address is repeated twice
   --> Order does not matter unless specified correctly */
unsigned char addrArray[] = {0x50, 0x50, 0x4F, 0x4F, 0x4D, 0x4D, 0x4C, 0x4C};

//global variables for averaging purposes
signed char k = 0;
float dataBuffer = 0.0;

void ADT7486A_init(void)
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
	led_1 = LED_ON;
	led_2 = LED_ON;
	led_3 = LED_ON;
	led_4 = LED_ON;
	led_5 = LED_ON;
	led_6 = LED_ON;
	led_7 = LED_ON;	

	delay_ms(1000);
	led_1 = LED_OFF;
	led_2 = LED_OFF;
	led_3 = LED_OFF;
	led_4 = LED_OFF;
	led_5 = LED_OFF;
	led_6 = LED_OFF;
	led_7 = LED_OFF;
}

void ADT7486A_LedMng(void)
/**********************************************************************************\

  Routine: ADT7486A_LedMng (Led manager)

  Purpose: To turn on/off LEDs to indicate whether each corresponding
  		   external temperature sources have crossed the limit of
		   temperature

  Input:
    void

  Function value:
    void

\**********************************************************************************/
{
	//for each external temperature source, if it reaches over 30 degree
	//celsius, then turn on the according LED ON, otherwise OFF
	led_1 = (user_data[0].external_temp >= TEMP_LIMIT) ? LED_ON : LED_OFF;
	led_2 = (user_data[1].external_temp >= TEMP_LIMIT) ? LED_ON : LED_OFF;
	led_3 = (user_data[2].external_temp >= TEMP_LIMIT) ? LED_ON : LED_OFF;
	led_4 = (user_data[3].external_temp >= TEMP_LIMIT) ? LED_ON : LED_OFF;
	led_5 = (user_data[5].internal_temp >= TEMP_LIMIT) ? LED_ON : LED_OFF;
	led_6 = (user_data[6].external_temp >= TEMP_LIMIT) ? LED_ON : LED_OFF;
	led_7 = (user_data[7].external_temp >= TEMP_LIMIT) ? LED_ON : LED_OFF;
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

	//making sure the SST pin is set to push-pull before originating msgs
	P2MDOUT = 0x13; 

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
		SST_Read(writeFCS_Org, DONT_READ_DATA);
	}
	if(writeLength == 0x03) //If there is data to be sent (0x03 writeLength)
	{
		//Send the data in little endian format, LSB before MSB
		SST_DrvByte(datLSB);
		SST_DrvByte(datMSB);
		SST_Read(writeFCS_Org, DONT_READ_DATA);
	}

	//In this application only temperature measurements are really required,
	//so we just need 2 bytes of reading
	if((writeLength == 0x01) && (readLength == 0x02)) 
	{
		if(k != AVG_COUNT)
		{
			tempDataBuffer = SST_Read(writeFCS_Org, READ_DATA); //Get internal temperature	
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
		}		
	}
	else if(command == 0xF6)//if reset command
	{
		SST_Read(writeFCS_Org, DONT_READ_DATA);
	}

	//Delay for the next msg
	SST_Clear();
}

void User_tsensor(void)
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
	
	SST_SwitchLine(1); //Switch to SST1 line
	while(k != 8) ADT7486A_Cmd(addrArray[4], GetIntTemp, 5); //Get internal temperature 1 command		
	k = 0; //reset counter
	while(k != 8) ADT7486A_Cmd(addrArray[4], GetExt1Temp, 6); //Get external temperature 1 command
	k = 0; //reset counter
	while(k != 8) ADT7486A_Cmd(addrArray[5], GetExt2Temp, 7); //Get external temperature 1 command
	k = 0; //reset counter
	ADT7486A_LedMng();
}