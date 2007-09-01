/********************************************************************\
  Name:         t2k-asum.c
  Created by:   Brian Lee  							Jun/07/2007


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol
                for T2K-ASUM test board

  $Id$

\********************************************************************/
//  need to have T2KASUM defined.

#include <stdio.h>
#include <math.h>
#include "../../mscbemb.h"
#include "t2k-asum.h"
#include "ADT7486A_tsensor.h"
#include "PCA9539_io.h"
#include "AD5301_dac.h"
#include "LTC_dac.h"
#include "AD7718_adc.h"

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

char code node_name[] = "T2K-ASUM";

/* Charge Pump */
sbit QPUMP_LED = P3 ^ 3;         // Bias status LED

struct user_data_type xdata user_data;

/* User Data structure declaration */
MSCB_INFO_VAR code vars[] = {

   1, UNIT_BYTE,            0, 0,           0, "Control",   &user_data.control,    // 0
   1, UNIT_BYTE,            0, 0,           0, "Status",    &user_data.status,      // 2
   1, UNIT_BYTE,            0, 0,           0, "D_BiasEN",   &user_data.biasEn,    // 1
   2, UNIT_BYTE,            0, 0,           0, "D_AsumTh",    &user_data.dac_asumThreshold,      // 3
   1, UNIT_BYTE,            0, 0,           0, "D_ChPump",    &user_data.dac_chPump,      // 4
   1, UNIT_BYTE,            0, 0,           0, "D_Bias1",    &user_data.biasDac1,      // 5
   1, UNIT_BYTE,            0, 0,           0, "D_Bias2",    &user_data.biasDac2,      // 6
   1, UNIT_BYTE,            0, 0,           0, "D_Bias3",    &user_data.biasDac3,      // 7
   1, UNIT_BYTE,            0, 0,           0, "D_Bias4",    &user_data.biasDac4,      // 8
   1, UNIT_BYTE,            0, 0,           0, "D_Bias5",    &user_data.biasDac5,      // 9
   1, UNIT_BYTE,            0, 0,           0, "D_Bias6",    &user_data.biasDac6,      // 10
   1, UNIT_BYTE,            0, 0,           0, "D_Bias7",    &user_data.biasDac7,      // 11
   1, UNIT_BYTE,            0, 0,           0, "D_Bias8",    &user_data.biasDac8,      // 12
   4, UNIT_AMPERE, PRFX_MILLI, 0, MSCBF_FLOAT, "+AnaIMon",    &user_data.pos_analogImonitor, // 13
   4, UNIT_VOLT,            0, 0, MSCBF_FLOAT, "+AnaVMon",    &user_data.pos_analogVmonitor, // 14 
   4, UNIT_AMPERE, PRFX_MILLI, 0, MSCBF_FLOAT, "DigIMon",    &user_data.digitalImonitor, // 15
   4, UNIT_VOLT,            0, 0, MSCBF_FLOAT, "BiasRB",    &user_data.biasReadBack, //16 
   4, UNIT_VOLT,            0, 0, MSCBF_FLOAT, "DigVMon",    &user_data.digitalVmonitor, // 17 
   4, UNIT_VOLT,            0, 0, MSCBF_FLOAT, "-AnaVMon",    &user_data.neg_analogVmonitor, // 18
   4, UNIT_AMPERE, PRFX_MILLI, 0, MSCBF_FLOAT, "-AnaIMon",    &user_data.neg_analogImonitor, // 19
   4, UNIT_AMPERE, PRFX_MILLI, 0, MSCBF_FLOAT, "RefCS",    &user_data.refCurrentSense, //20
   4, UNIT_AMPERE, PRFX_MILLI, 0, MSCBF_FLOAT, "BiasCS",    &user_data.biasCurrentSense, //21 
   4, UNIT_CELSIUS,         0, 0, MSCBF_FLOAT, "ExtTemp",  &user_data.external_temp, //22
   
   //Hidden variables (wth "read all" call)
   4, UNIT_CELSIUS,         0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "TempOS",  &user_data.external_tempOffset, //23(hidden)
   4, UNIT_CELSIUS,         0, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "IntTemp",  &user_data.internal_temp, //24(hidden)
   0
};


MSCB_INFO_VAR *variables = vars;

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

/*---- User init function ------------------------------------------*/

extern SYS_INFO sys_info;
unsigned char ADT7486A_addrArray[] = {ADT7486A_ADDR_ARRAY};

void user_init(unsigned char init)
{
   /* all output open drain */
   P1MDOUT = 0x00;

   PCA0MD   = 0x8;  // Sysclk (24.6MHz)
    
   /* default settings, set only when EEPROM is being erased and written */
	if(init)
	{
		user_data.control = 0x7E; //temperature and adc readings ON
		user_data.status   = 0x00;
		user_data.biasEn   = 0xFF;
		user_data.dac_asumThreshold   = 0x80;
		user_data.dac_chPump   = 0x00; //set to lowest scale, just to be safe
		user_data.biasDac1   = 0xFF;
		user_data.biasDac2   = 0xFF;
		user_data.biasDac3   = 0xFF;
		user_data.biasDac4   = 0xFF;
		user_data.biasDac5   = 0xFF;
		user_data.biasDac6   = 0xFF;
		user_data.biasDac7   = 0xFF;
		user_data.biasDac8   = 0xFF;		
		sys_info.group_addr = 400;
		sys_info.node_addr = 0xFF00;
	}   

	/* Cross Bar Settings */

	/* Comparator 0 Settings */
	CPT0CN = 0xC0; //Enable Comparator0 (functional, the one above is only for CrossBar)

	CPT0MX = 0x22; //Comparator0 MUX selection
	        //Negative input is set to P2 ^ 1, and Positive input is set to P2 ^ 0
	        // (P2 ^ 0 is the SST1, so we want to compare SST1 with the threshold voltage
	        //of !~0.8V on P2 ^ 1
	CPT0MD = 0x02; //Comparator0 Mode Selection
	        //Use default, adequate TYP (CP0 Response Time, no edge triggered interrupt)

	//Set P2 ^ 1 to Open-Drain and Analog Input so that it accepts the ~650mV set by voltage divider
	P2MDOUT &= 0xFD;
	P2MDIN &= 0xFD;

   /* set-up / initialize circuit components (order is important)*/
   ADT7486A_Init(); //Temperature measurements related initialization
	LTCdac_Init(); //LTC dacs initialization
	AD7718_Init(); //ADC initialization 		 
	pca_operation(Q_PUMP_INIT); //Charge Pump initialization (crossbar settings)
   pca_operation(Q_PUMP_OFF); //Initially turn it off	
	PCA9539_Init(); //PCA General I/O (Bais Enables) initialization	
	AD5301_Init(); //I2C DAC initialization (D_CH_PUMP)	
}

/*---- PCA initilalization -----------------------------------------*/

void pca_operation(unsigned char mode)
{
	if (mode == Q_PUMP_INIT) 
	{
		/* PCA setup for Frequency Output Mode on CEX0 */
		P0MDOUT  = 0x94; // Keep 4,7 for Rx,Enable + CEX0 output
		XBR0     = 0x05; // Enable SMB, UART
		XBR1     = 0x41; // Enable Xbar , Enable CEX0 on P0.
		PCA0MD   = 0x8;  // Sysclk (24.6MHz)
		PCA0CPL0 = 0x00;
		PCA0CPM0 = 0x46; // ECM, TOG, PWM
		PCA0CPH0 = 0x06 ; // 6 (for ~2MHz)
		PCA0CN   = 0x40; // Enable PCA Run Control
	} 
	else if (mode == Q_PUMP_OFF) 
	{
		XBR1 = (XBR1 & 0xF8) | 0x00; // turn off Frequency Output Mode
	} 
	else if (mode == Q_PUMP_ON) 
	{
		XBR1 = (XBR1 & 0xF8) | 0x01; // turn on Frequency Output Mode
	}
}

/*---- User write function -----------------------------------------*/

#pragma NOAREGS

void user_write(unsigned char index) reentrant
{
   unsigned char command, mask;

   // In case of you changing common control bit (ex. EXT/IN switch bit)
   // the change should be distributed to other channels.
   if (index == 0) {
       // preserve common command bits for all channels
       command = user_data.control;
       mask    = CONTROL_QPUMP; // common bits

       user_data.control &= ~mask;
       user_data.control |= (command & mask);
   }


}

/*---- User read function ------------------------------------------*/

unsigned char user_read(unsigned char index)
{
   if (index);
   return 0;
}

/*---- User function called vid CMD_USER command -------------------*/

unsigned char user_func(unsigned char *data_in, unsigned char *data_out)
{
   /* echo input data */
   data_out[0] = data_in[0];
   data_out[1] = data_in[1];
   return 2;
}

/*---- CTL/Status management ------------------------------------------*/
void Hardware_Update(void)
{
    // Hardware Update routine
    // Write down what you want to do here at each control state

   watchdog_refresh(0);	
}

/*---- User loop function ------------------------------------------*/
void user_loop(void)
{
	signed char chNum = 0;

 	watchdog_refresh(0);


	/*** Write down what you want to do here at each control state ***/

	if(user_data.control & CONTROL_TEMP_MEAS)
	{
		for(chNum = 0; chNum < ADT7486A_NUM; chNum++)
		{
			ADT7486A_Cmd(ADT7486A_addrArray[chNum], GetExt1Temp, &user_data.external_temp);
			ADT7486A_Cmd(ADT7486A_addrArray[chNum], GetIntTemp, &user_data.internal_temp);		
		}
	}

	if(user_data.control & CONTROL_ADC_MEAS)
	{
		//ADC Monitoring user defined routines
		//Store same values for each channel (8 channel for now)
		AD7718_Cmd(READ_AIN2, &user_data.pos_analogImonitor);
		AD7718_Cmd(READ_AIN3, &user_data.pos_analogVmonitor);
		AD7718_Cmd(READ_AIN4, &user_data.digitalImonitor);
		AD7718_Cmd(READ_AIN5, &user_data.biasReadBack);
		AD7718_Cmd(READ_AIN6, &user_data.digitalVmonitor);
		AD7718_Cmd(READ_AIN7, &user_data.neg_analogVmonitor);
		AD7718_Cmd(READ_AIN8, &user_data.neg_analogImonitor);
		if(user_data.control & CONTROL_QPUMP)
		{
			//if the charge pump is on, do the measurements
			AD7718_Cmd(READ_BIASCS, &user_data.refCurrentSense);
			AD7718_Cmd(READ_REFCS, &user_data.biasCurrentSense);
		}
		else
		{
			//if the charge pump is off, then put 0.0 for current readings
			user_data.refCurrentSense = 0.0;
			user_data.biasCurrentSense = 0.0;
		}
		watchdog_refresh(0);
	}

	if(user_data.control & CONTROL_QPUMP)
	{
		pca_operation(Q_PUMP_ON);
		QPUMP_LED = LED_ON;
	}
	else
	{
		pca_operation(Q_PUMP_OFF);
		QPUMP_LED = LED_OFF;
	}

	//Update the Bias_Enable status
	if(user_data.control & CONTROL_BIAS_EN)
	{
		PCA9539_Cmd(ADDR_PCA9539, 0x03, ~user_data.biasEn, PCA9539_WRITE);
	}			


	//Update the Charge Pump Threshold voltage
	if(user_data.control & CONTROL_D_CHPUMP)
	{
		AD5301_Cmd(ADDR_AD5301, (user_data.dac_chPump >> 4), (user_data.dac_chPump << 4), AD5301_WRITE);
	}

	// Update Bias Dac voltages as requested
	if(user_data.control & CONTROL_BIAS_DAC)
	{
		LTCdac_Cmd(LTC1665_LOAD_A, user_data.biasDac1);
		LTCdac_Cmd(LTC1665_LOAD_B, user_data.biasDac2);
		LTCdac_Cmd(LTC1665_LOAD_C, user_data.biasDac3);
		LTCdac_Cmd(LTC1665_LOAD_D, user_data.biasDac4);
		LTCdac_Cmd(LTC1665_LOAD_E, user_data.biasDac5);
		LTCdac_Cmd(LTC1665_LOAD_F, user_data.biasDac6);
		LTCdac_Cmd(LTC1665_LOAD_G, user_data.biasDac7);
		LTCdac_Cmd(LTC1665_LOAD_H, user_data.biasDac8);
	}

	// Update Bias Dac voltages as requested
	if(user_data.control & CONTROL_ASUM_TH)
	{
		LTCdac_Cmd(LTC2600_LOAD_H, (char) ((user_data.dac_asumThreshold >> 8) & 0xFF), (char) (user_data.dac_asumThreshold & 0xFF));
	}

	Hardware_Update();
	yield();
}


