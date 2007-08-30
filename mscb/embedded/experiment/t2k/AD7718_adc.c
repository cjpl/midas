/********************************************************************\

  Name:         AD7718_adc.c
  Created by:   Brian Lee  								Jun/14/2007


  Contents:     AD7718BRU ADC user interface

  $Id$

\********************************************************************/
//  need to have T2KASUM defined

#include <math.h>
#include "../../mscbemb.h"
#include "AD7718_adc.h"
#include "../../protocols/SPI_handler.h"

/* AD7718 related pins */
sbit ADC_ResetN = P1 ^ 1;       // !Reset
sbit ADC_csN  = P1 ^ 0;         // !Chip select
sbit ADC_ReadyN = P1 ^ 1;       // !Ready
sbit ADC_DIN = P1 ^ 3;
sbit ADC_SCK = P0 ^ 7;

void AD7718_Init(void)
{
	ADC_ResetN = 1;
	ADC_csN  = 1; // chip select
	ADC_ReadyN = 1; // input

	SPI_Init(); //SPI protocol initialization

	/* reset and wait for start-up of ADC */
	ADC_ResetN = 0;
	delay_ms(100);
	ADC_ResetN = 1;
	delay_ms(300);

	ADC_csN = 0;

	SPI_WriteByte(0x00 | REG_MODE);
	SPI_WriteByte(0x30 | 0x03);        // continuous conversion, 10 channel mode

	delay_ms(300);

	SPI_WriteByte(0x00 | REG_FILTER);
	SPI_WriteByte(ADC_SF_VALUE);

	/*In this application, we just use the default offset and gain */

	ADC_csN = 1;
	AD7718_Clear();
}

void AD7718_Cmd(unsigned char cmd, unsigned char adcChNum, float *varToBeWritten)
{
	float dataToBeStored = 0.0;

	//Turn the ADC CS_ bit to low to talk to this ADC device
	ADC_csN = 0;
	delay_us(SPI_DELAY);	

	SPI_WriteByte(0x00 | REG_CONTROL);
	SPI_WriteByte((adcChNum << 4) | 0x0F); // adc_chn, unipol, +2.56V range
	
	dataToBeStored = (((float) AD7718_Read()) / 16777216) * 2.56; //read in the 24 bits data

	if(cmd == 2) //+6V_Analog_IMon
	{	
		//Do according conversions and offsets		
		dataToBeStored *= 500 / 1.25; //convert to correct current with its ratio		
	}	
	else if(cmd == 3) //+6V_Analog_VMon
	{
		//Do according conversions and offsets
		dataToBeStored *= (5.991 / 1.515); //reverse voltage division
	}
	else if(cmd == 4) //+6V_Dig_IMon
	{
		//Do according conversions and offsets
		dataToBeStored *= 500 / 1.25; //convert to correct current with its ratio
	}
	else if(cmd == 5) //Bias_Readback
	{
		//Do according conversions
		/* convert to volts */
		dataToBeStored *= (90.2 / 2.182); //reverse voltage division
	}
	else if(cmd == 6) //+6V_Dig_VMon
	{
		//Do according conversions
		/* convert to volts */
		dataToBeStored *= (5.972 / 1.515); //reverse voltage division
	}
	else if(cmd == 7) //-6V_Analog_VMon
	{
		//Do according conversions
		/* convert to volts */
		dataToBeStored *= (-6.018 / 1.495); //reverse voltage division
	}
	else if(cmd == 8) //-6V_Analog_IMon
	{
		//Do according conversions
		/* convert to volts */
		dataToBeStored *= 500 / 1.25; //convert to correct current with its ratio
	}
	else if(cmd == 9) //BiasCurrentSense
	{
		//Do according conversions
		/* convert to volts */
		dataToBeStored *= 25 / 2;
	}
	else if(cmd == 10) //RefCurrentSense
	{
		//Do according conversions
		/* convert to volts */
		dataToBeStored *= 25;
	}
	else //if the command is inappropriate, just return
	{
		return;
	}

	/* round result to 6 digits */
	dataToBeStored = floor(dataToBeStored*1E6+0.5)/1E6;

	//save the Data
	DISABLE_INTERRUPTS;
	if(dataToBeStored != 0.0) *varToBeWritten = dataToBeStored;
	ENABLE_INTERRUPTS;

	//Turn the ADC CS_ bit to high for other ADC devices
	ADC_csN = 1;
	delay_us(SPI_DELAY);
}
		
unsigned long AD7718_Read(void)
{
	unsigned long dataReceived = 0;
	unsigned long start_time = 0;

	SPI_WriteByte(0x40 | REG_ADCDATA);
	
	start_time = time();

	while (ADC_ReadyN) 
	{
		yield();

		// abort if no ADC ready after 300ms
		if ((time() - start_time) > 20) 
		{
			 AD7718_Clear(); 
			
			 return 0;
		}
	}
	
	//Recieve the 24 bits data
	dataReceived = ((unsigned long) SPI_ReadByte()) << 16;
	dataReceived |= ((unsigned long) SPI_ReadByte()) << 8;
	dataReceived |= ((unsigned long) SPI_ReadByte());
	
	ADC_ReadyN = 1; //pull the ReadyN pin to high after reading is done
	AD7718_Clear();
	return dataReceived;
}

void AD7718_Clear(void)
{
	// reset ADC
	ADC_DIN = 1;
	delay_us(40 * SPI_DELAY); //reset communication to default stage			 
}
